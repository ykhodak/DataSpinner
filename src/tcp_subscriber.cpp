#include "spnr.h"
#include "records.h"
#include "md_ref.h"
#include "tcp_connection.h"

class MdClientConnection : public spnr::TcpConnection
{
public:
    MdClientConnection(spnr::TcpConnectionFactory* r, bufferevent* b) :spnr::TcpConnection(r, b) {}

    void on_connected()
    {
        static auto& cfg = spnr::Cfg::cfg();
        spnr::LoginMsg m;        
        m.set_user(cfg.client_user);
        m.set_token("user-token_1");
        spnr::log("trying to login as [%s]", cfg.client_user.c_str());
        send_message(m);
    }

    void on_login_msg(const spnr::LoginMsg& m)
    {
        spnr::errlog("Unexpected login msg from server [%s]", m.user().c_str());
    }

    void on_login_reply_msg(const spnr::LoginReplyMsg& m)
    {
        static auto& cfg = spnr::Cfg::cfg();
        if (m.reply_code() == 0) {
            spnr::errlog("login failed [%d] [%s]", m.reply_code(), m.reply_description().c_str());
            close_connection();
            return;
        }
        spnr::log("login accepted [%d]", m.reply_code());
        stat_.start_reader_stat();

        std::vector<std::string> symbols = cfg.client_sub;
        //symbols.push_back("AAPL");
        //symbols.push_back("IBM");

        spnr::SubscribeMsg sub;
        sub.add_symbols(symbols);
        send_message(sub);

        spnr::log("subscription send [%d]", symbols.size());
    };

    void on_md_reference_msg(const spnr::MdRef& m)
    {
        static auto& cfg = spnr::Cfg::cfg();
        if (cfg.is_debug_log_level()) {
            spnr::log("r[%s] w-latency: %d [%d]", m.symbol().c_str(), stat_.last_msg_wire_latency(), stat_.last_msg_len());
        }
    }

    void on_md_delta_msg(const spnr::MdDelta& m)
    {
        static auto& cfg = spnr::Cfg::cfg();
        if (cfg.is_verbose_log_level()) {
            std::cout << m;
        }
        if (cfg.is_debug_log_level()) {
            spnr::log("d[%s] w-latency: %d [%d]", m.symbol().c_str(), stat_.last_msg_wire_latency(), stat_.last_msg_len());
        }
    }
};

class TcpSubscriberFactory :    
    public spnr::SingleTcpConnectionFactory<MdClientConnection>,
    public spnr::HB_SubscriberRunner
{
public:
    TcpSubscriberFactory():HB_SubscriberRunner(spnr::Cfg::cfg().hb_period_sec_)
    {
    }
    void dispose_connection(spnr::TcpConnection* c)override
    {
        spnr::SingleTcpConnectionFactory<MdClientConnection>::dispose_connection(c);
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
    if (!spnr::init_workspace(argc, argv, "tcp-sub"))
        return 1;
    
    std::thread t_sub(task_subscriber);
    auto res = spnr::run_tcp_connector(spnr::Cfg::cfg().host_, spnr::Cfg::cfg().port_, &subscriber_factory);
    subscriber_factory.stop_subscriber();
    t_sub.join();
    return res;
}