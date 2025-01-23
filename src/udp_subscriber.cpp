#include "spnr.h"
#include "records.h"
#include "md_ref.h"
#include "simulator.h"
#include "udp_connection.h"

namespace spnr {
    class UdpSubscriberConnection : public spnr::UdpConnection
    {
    public:
        UdpSubscriberConnection(UdpConnectionFactory* f, evutil_socket_t fd) :spnr::UdpConnection(f, fd)
        {
        };
    protected:
        void on_login_reply_msg(const sockaddr_in&, const spnr::LoginReplyMsg& m)override
        {
            if (m.reply_code() == 0) {
                spnr::errlog("login failed [%d] [%s]", m.reply_code(), m.reply_description().c_str());
                return;
            }
            spnr::log("login accepted [%d]", m.reply_code());
            stat_.start_reader_stat();
        }

        void on_md_delta_msg(const sockaddr_in&, const spnr::MdDelta& m)override
        {
            static auto& cfg = spnr::Cfg::cfg();
            if (cfg.is_verbose_log_level()) {
                std::cout << m;
            }
            if (cfg.is_debug_log_level()) {
                spnr::log("d[%s] w-latency: %d [%d]", m.symbol().c_str(), stat_.last_msg_wire_latency(), stat_.last_msg_len());
            }
        }
        void on_text_msg(const sockaddr_in&, const spnr::TextMsg& m)override
        {
            spnr::log("on-text [%s] wire-latency: %d [%d]", m.text().c_str(), stat_.last_msg_wire_latency(), stat_.last_msg_len());
        };

        void on_login_msg(const sockaddr_in&, const LoginMsg& m)override
        {
            std::string user_name = m.user();
            spnr::errlog("unexpected login [%s]", user_name.c_str());
        }
        void register_udp_subscriber(const sockaddr_in&, const LoginMsg&) override
        {
            spnr::errlog("unexpected register_udp_subscriber");
        };
    };
};

class UdpSubscriberFactory : 
    public spnr::SingleUdpConnectionFactory<spnr::UdpSubscriberConnection>,
    public spnr::HB_SubscriberRunner
{
public:
    UdpSubscriberFactory() :HB_SubscriberRunner(spnr::Cfg::cfg().hb_period_sec_)
    {
    }
    void dispose_connection(spnr::UdpConnection* c)override
    {
        spnr::SingleUdpConnectionFactory<spnr::UdpSubscriberConnection>::dispose_connection(c);
        stop_subscriber();
    }
} subscriber_factory;


static void task_subscriber()
{
    while (!subscriber_factory.is_subscriber_stopped()) {
        auto c = subscriber_factory.get_connection();
        if (c) {
            c->send_hb();
        }

        if (!subscriber_factory.is_subscriber_stopped()) {
            subscriber_factory.wait_subscriber_for_hb_time();
        }
    }
    spnr::log("exiting subscriber thread");
}


int main(int argc, char** argv)
{
    if (!spnr::init_workspace(argc, argv, "udp-sub", false))
        return 1;

    auto& cfg = spnr::Cfg::cfg();
    std::thread t_sub(task_subscriber);
    auto rv = spnr::run_udp_subscriber(cfg.udp_host_, cfg.udp_port_, &subscriber_factory);
    subscriber_factory.stop_subscriber();
    t_sub.join();
    return rv;
}
