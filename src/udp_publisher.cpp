#include "spnr.h"
#include "records.h"
#include "md_ref.h"
#include "simulator.h"
#include "udp_connection.h"

bool operator == (const sockaddr_in& a1, const sockaddr_in& a2)
{
    return (a1.sin_port == a2.sin_port && a1.sin_addr.s_addr == a2.sin_addr.s_addr);
}

struct sockaddr_in_hash {
    size_t operator()(const sockaddr_in& addr) const {
        std::hash<int> hasher;
        return hasher(addr.sin_family) ^ hasher(addr.sin_port) ^ hasher(addr.sin_addr.s_addr);
    }
};


namespace spnr {

    struct UdpConnInfo :
        public spnr::MetricsObserver,
        public HB_Observer<UdpConnInfo>
    {
    public:
        UdpConnInfo(std::string ip, int port, std::string user) :
            spnr::MetricsObserver(spnr::Cfg::cfg().stat_check_every_n_tick_, spnr::Cfg::cfg().stat_upd_every_nsec_),
            spnr::HB_Observer<UdpConnInfo>(this, spnr::Cfg::cfg().stat_check_every_n_tick_, spnr::Cfg::cfg().hb_timeout_sec_),
            client_ip_(ip), client_port_(port), client_user_name_(user) 
        {
            reset_hb_time();
            reset_tick_observer(user);
        }
    
        inline uint32_t writer_session_index()const { return writer_session_index_; }
        uint32_t        next_writer_session_index()const { return ++writer_session_index_; };

        std::string             name()const { return client_user_name_; }
        time_t                  last_hb_time()const { return last_hb_time_; };
        void                    reset_hb_time() { last_hb_time_ = time(nullptr); };



        std::string         client_ip_;
        int                 client_port_ = 0;
        std::string         client_user_name_;
        mutable uint32_t    writer_session_index_{ 0 };
        std::atomic<time_t> last_hb_time_ = 0;
    };



    using UPD_SUBSCRIBERS = std::unordered_map<sockaddr_in, std::unique_ptr<UdpConnInfo>, sockaddr_in_hash>;

    class UdpPublisherConnection : 
        public spnr::UdpConnection
    {
        std::atomic<uint16_t>   subscribers_count_;
        uint64_t                padding_ = 0;
        UPD_SUBSCRIBERS         subscribers_;
        std::mutex              mtx_conn_;
        std::condition_variable cv_conn_;
#define GUARD_SUB  std::lock_guard<std::mutex> lk(mtx_conn_);

    public:
        UdpPublisherConnection(spnr::UdpConnectionFactory* f,evutil_socket_t fd) 
            :spnr::UdpConnection(f, fd)            
        {            
        };

        template<class T>
        bool publish_to_subscribed(const T& m) 
        {
            static auto& cfg = Cfg::cfg();

            GUARD_SUB;
            std::vector<sockaddr_in> list2unreg;
            for (auto& s : subscribers_) 
            { 
                auto& sin = s.first;
                auto ci = s.second.get();
                auto bytes_sent = send_message_to(sin, m, ci->next_writer_session_index());
                if (bytes_sent < 0) 
                {
                    spnr::errlog("Error sending [%s:%d]%s data: [%d] [%s]", 
                        ci->client_ip_.c_str(),
                        ci->client_port_,
                        ci->client_user_name_.c_str(),
                        spnr::get_socket_error(), 
                        spnr::get_socket_error_text());
                    return false;
                }
                else {
                    auto tick_num = ci->next_tick_publisher_metrics(ci->writer_session_index());
                    auto hb_delta = ci->next_hb_check(tick_num);
                    if (hb_delta != 0)
                    {
                        spnr::errlog("HB timeout [%d] [%s]", hb_delta, ci->name().c_str());
                        list2unreg.push_back(sin);
                        //do_unregister_udp_subscriber(sin);
                        //c->close_connection();
                    }
                    else                    
                    {
                        if (cfg.is_verbose_log_level()) {
                            spnr::errlog("sent %d [%s:%d]%s", bytes_sent,
                                ci->client_ip_.c_str(),
                                ci->client_port_,
                                ci->client_user_name_.c_str());
                        }
                    }
                }
            }

            for (auto& sin : list2unreg) {
                do_unregister_udp_subscriber(sin);
            }
            return true;
        }

        uint16_t subscribers_count()
        {
            return subscribers_count_.load();
        };

        void wait_for_subscribers()
        {
            if (subscribers_count() == 0)
            {
                std::unique_lock<std::mutex> lk(mtx_conn_);
                cv_conn_.wait(lk, [this]() {
                    return (subscribers_count() > 0);
                    });
            }
        };


    protected:
        void on_login_msg(const sockaddr_in&, const spnr::LoginMsg& m)override
        {
            std::string user_name = m.user();
            spnr::log("login accepted [%s]", user_name.c_str());            
        }

        void register_udp_subscriber(const sockaddr_in& client_addr, const spnr::LoginMsg& m) override
        {
            GUARD_SUB;
            char client_ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
            int client_port = htons(client_addr.sin_port);
            std::string user_name = m.user();


            auto i = subscribers_.find(client_addr);
            if (i == subscribers_.end()) {
                //subscribers_[client_addr] = std::move{ client_ip, client_port, user_name };
                subscribers_.emplace(client_addr, std::move(std::make_unique<UdpConnInfo>(client_ip, client_port, user_name)) );
                //subscribers_[client_addr] = std::move(UdpConnInfo(client_ip, client_port, user_name));
                spnr::errlog("registered connection [%s:%d]%s", client_ip, client_port, user_name.c_str());
                subscribers_count_.store(static_cast<uint16_t>(subscribers_.size()));
                cv_conn_.notify_one();
            }
            else {
                spnr::errlog("failed to registered connection [%s:%d]%s", client_ip, client_port, user_name.c_str());
            }          
        };

        void unregister_udp_subscriber(const sockaddr_in& client_addr)override
        {            
            GUARD_SUB;
            do_unregister_udp_subscriber(client_addr);
        };

        void do_unregister_udp_subscriber(const sockaddr_in& client_addr)
        {
            static auto& cfg = Cfg::cfg();
            auto i = subscribers_.find(client_addr);
            if (i != subscribers_.end()) {
                auto ci = i->second.get();
                //subscribers_[client_addr] = { client_ip, client_port, user_name };
                spnr::errlog("unregistered connection [%s:%d]%s",
                    ci->client_ip_.c_str(),
                    ci->client_port_,
                    ci->client_user_name_.c_str());
                subscribers_.erase(i);
                subscribers_count_.store(static_cast<uint16_t>(subscribers_.size()));
            }
            else {
                if (cfg.is_verbose_log_level())
                {
                    char client_ip[INET_ADDRSTRLEN];
                    inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
                    int client_port = htons(client_addr.sin_port);

                    spnr::errlog("failed to unregistered connection (not found) [%s:%d]", client_ip, client_port);
                }
            }
        }


        void on_text_msg(const sockaddr_in&, const spnr::TextMsg& m)override
        {
            spnr::log("on-text [%s] wire-latency: %d [%d]", m.text().c_str(), stat_.last_msg_wire_latency(), stat_.last_msg_len());
        };

        void on_hb_msg(const sockaddr_in& sin, const spnr::HBMsg& m) override
        {
            spnr::UdpConnection::on_hb_msg(sin, m);
            spnr::log("on_hb[%s]", spnr::format_with_commas(m.counter()).c_str());
            GUARD_SUB;
            auto i = subscribers_.find(sin);
            if (i != subscribers_.end()) {
                i->second->last_hb_time_.store(time(nullptr));
            }
        };

    };
}

class UdpDispatcher : public spnr::SingleUdpConnectionFactory<spnr::UdpPublisherConnection> 
{
public:
    template<class T>
    bool publish(const T& m) {        
        if (c_) {
            return c_->publish_to_subscribed(m);            
        }
        return false;

    }
} udp_dispatcher;

void task_publisher()
{   
    auto& cfg = spnr::Cfg::cfg();

    auto symbols = cfg.svr_spin_symbols;
    while (1) {
        auto c = udp_dispatcher.get_connection();
        if (c) 
        {
            if (c->subscribers_count() == 0)
            {
                spnr::log("dispatcher wating for subscribers");
                c->wait_for_subscribers();
                spnr::log("dispatcher detected subscribers");
            }

            for (const auto& symbol : symbols)
            {
                spnr::make_one_delta<spnr::MdDelta, UdpDispatcher>(symbol, udp_dispatcher);
            }

            if (cfg.publisher_spin_sleep_ns_ > 0) {
                std::this_thread::sleep_for(std::chrono::nanoseconds(cfg.publisher_spin_sleep_ns_));
            }
        }
        else 
        {
            spnr::log("dispatcher wating for binding connection");
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
    }
}


int main(int argc, char** argv)
{
    if (!spnr::init_workspace(argc, argv, "udp-pub"))
        return 1;


    auto& cfg = spnr::Cfg::cfg();    
    std::thread t_pub(task_publisher);
    auto rv = spnr::run_udp_publisher(cfg.udp_host_, cfg.udp_port_, &udp_dispatcher);
    t_pub.join();
    return rv;
}
