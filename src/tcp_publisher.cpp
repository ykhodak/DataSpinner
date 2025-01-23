#include "spnr.h"
#include "records.h"
#include "md_ref.h"
#include "simulator.h"
#include "tcp_connection.h"
#include "admin_connection.h"

class MdServerConnection : public spnr::TcpConnection, 
    public spnr::MetricsObserver,
    public spnr::HB_Observer<MdServerConnection>
{
    spnr::MdRefTable  snapshot_;
    spnr::STR_SET     subscription_;
public:
    MdServerConnection(spnr::TcpConnectionFactory* r, struct bufferevent* b) 
        :spnr::TcpConnection(r, b),
        spnr::MetricsObserver(spnr::Cfg::cfg().stat_check_every_n_tick_, spnr::Cfg::cfg().stat_upd_every_nsec_),
        spnr::HB_Observer<MdServerConnection>(this, spnr::Cfg::cfg().stat_check_every_n_tick_, spnr::Cfg::cfg().hb_timeout_sec_)
    {
    }

    void on_login_accepted()override
    {
        spnr::log("on_login_accepted");
        reset_tick_observer(name_);
        reset_hb_time();
    };

    void on_subscribe_msg(const spnr::SubscribeMsg& m)override
    {
        static auto& cfg = spnr::Cfg::cfg();
        const auto& sym = m.symbols();
        if (sym.size() > 0)
        {
            if (cfg.is_verbose_log_level())
            {
                spnr::log("sub: %d", sym.size());
                for (const auto& s : sym) {
                    spnr::log("%s", s.c_str());
                }
            }

            auto f = dynamic_cast<spnr::MultiTcpConnectionFactory*>(factory_);
            f->add_buscriptions(this, sym);
        }
    };

    void on_hb_msg(const spnr::HBMsg& m) override
    {
        spnr::TcpConnection::on_hb_msg(m);
        spnr::log("on_hb[%s]", spnr::format_with_commas(m.counter()).c_str());
    };
};


class MdServerConnection;
struct ConnInfo
{
    ConnInfo() {}
    ConnInfo(std::unique_ptr<MdServerConnection>&& p) :ptr_(std::move(p)) {}

    std::unordered_set<std::string> subscribed_;
    std::unique_ptr<MdServerConnection> ptr_;
};

using SVR_CONNECTIONS = std::unordered_map<MdServerConnection*, ConnInfo>;


class DeltaDispatcher: public spnr::MultiTcpConnectionFactory
{
    std::atomic<uint16_t>   connections_count_;
    uint64_t                padding_ = 0;
    SVR_CONNECTIONS         connections_;
    std::mutex              mtx_conn_;
    std::condition_variable cv_conn_;
    
#define GUARD_DISP  std::lock_guard<std::mutex> lk(mtx_conn_);
public:
    template<class T>
    bool publish(const T& m) {
        GUARD_DISP;
        auto conn_list = get_subscribed_connections(m.symbol());
        size_t connections_in_throttle = 0;
        for (auto& c : conn_list) 
        {
            if (!c->send_message(m)) {
                spnr::errlog("failed to send msg to connection [%s]", c->si().as_str());
                c->close_connection();
                return false;
            }
            else             
            {
                if (c->is_throttle_on()) {
                    ++connections_in_throttle;
                }

                auto tick_num = c->next_tick_publisher_metrics(c->writer_session_index());
                auto hb_delta = c->next_hb_check(tick_num);
                if (hb_delta != 0) 
                {
                    spnr::errlog("HB timeout [%d] [%s] [%s]", hb_delta, c->name().c_str(), c->si().as_str());
                    c->close_connection();
                }
            }

            if (connections_in_throttle == conn_list.size()) {
                spnr::log("All connections in throttle [%d] yeilding", connections_in_throttle);
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }
        return true;
    }

    uint16_t connections_count()
    {
        return connections_count_.load();
    };

    void wait_for_connections() 
    {
        if (connections_count() == 0)
        {
            std::unique_lock<std::mutex> lk(mtx_conn_);
            cv_conn_.wait(lk, [this]() {
                return (connections_count() > 0);
                });
        }
    };

    spnr::TcpConnection* create_connection(bufferevent* b)override
    {
        auto c = std::make_unique<MdServerConnection>(this, b);
        auto rv = c.get();
        GUARD_DISP;
        connections_[rv] = ConnInfo(std::move(c));
        connections_count_.store(static_cast<uint16_t>(connections_.size()));
        cv_conn_.notify_one();
        return rv;
    };

    void dispose_connection(spnr::TcpConnection* c)override
    {
        GUARD_DISP;
        connections_.erase(dynamic_cast<MdServerConnection*>(c));
        connections_count_.store(static_cast<uint16_t>(connections_.size()));
        spnr::log("connection disposed");
    };

    void add_buscriptions(spnr::TcpConnection* c1, const std::vector<std::string>& symbols)override
    {
        auto c = dynamic_cast<MdServerConnection*>(c1);

        GUARD_DISP;
        auto i = connections_.find(c);
        if (i != connections_.end()) {
            auto& ci = i->second;
            for (const auto& s : symbols) {
                ci.subscribed_.insert(s);
            }
        }
        else {
            spnr::errlog("failed to find connection to add subscription.");
        }
    };

    std::vector<MdServerConnection*> get_subscribed_connections(const std::string& symbol) 
    {
        //GUARD_DISP; don't guard this
        std::vector<MdServerConnection*> rv;
        for (auto& i : connections_) 
        {
            if (i.first->is_valid()) {
                auto& ci = i.second;
                auto k = ci.subscribed_.find(symbol);
                if (k != ci.subscribed_.end()) {
                    rv.push_back(i.first);
                }
            }
        }
        return rv;
    }
} delta_dispatcher;



static void listener_cb(struct evconnlistener*, evutil_socket_t fd,
    struct sockaddr*, int, void* user_data)
{
    struct event_base* base = (event_base*)user_data;
    struct bufferevent* bev;

    bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE|BEV_OPT_THREADSAFE);
    if (!bev) {
        spnr::errlog("Error constructing bufferevent");
        event_base_loopbreak(base);
        return;
    }
       
    auto c = delta_dispatcher.create_connection(bev);

    spnr::log("new connection [%s] [%d %d]", c->si().as_str(), c->si().rcv_buff_size(), c->si().send_buff_size());
    bufferevent_setcb(bev, spnr::TcpConnection::readcb, spnr::TcpConnection::writecb, spnr::TcpConnection::eventcb, c);
    bufferevent_enable(bev, EV_READ | EV_WRITE | EV_TIMEOUT);
}

static void task_publisher()
{
    auto& cfg = spnr::Cfg::cfg();

    auto symbols = cfg.svr_spin_symbols;
    while (1) {

        if (delta_dispatcher.connections_count() == 0) 
        {
            spnr::log("dispatcher wating for connections");
            delta_dispatcher.wait_for_connections();
            spnr::log("dispatcher detected connection");
        }

        for (const auto& symbol : symbols)
        {
            spnr::make_one_delta<spnr::MdDelta, DeltaDispatcher>(symbol, delta_dispatcher);
        }

        if (cfg.publisher_spin_sleep_ns_ > 0) {
            std::this_thread::sleep_for(std::chrono::nanoseconds(cfg.publisher_spin_sleep_ns_));
        }
    }
}


int main(int argc, char** argv)
{
    if (!spnr::init_workspace(argc, argv, "tcp-pub"))
        return 1;

    auto& cfg = spnr::Cfg::cfg();

    std::thread t_pub(task_publisher);
    std::vector<spnr::listener_info> linfo;
    linfo.emplace_back(cfg.port_, listener_cb);
    linfo.emplace_back(cfg.admin_port_, spnr::AdminServerConnection::listener_cb);
    auto rv = spnr::run_tcp_listener(linfo);
    t_pub.join();
    return rv;
}
