#include "admin_connection.h"

class DeltaPublisher
{
public:
    template<class T>
    bool publish(const T& m) {
        return c_->send_message(m);
    }
    spnr::TcpConnection* c_ = nullptr;
} admin_publisher;


spnr::AdminServerConnection::AdminServerConnection(spnr::TcpConnectionFactory* r, struct bufferevent* b) :spnr::TcpConnection(r, b)
{
    admin_publisher.c_ = this;

    menu_.add_menu_option(spnr::ECommand::cmd_echo_text, "Echo back text");
    menu_.add_menu_option(spnr::ECommand::cmd_subscribe, "Send subscription for symbols (space separate symbols)");
    menu_.add_menu_option(spnr::ECommand::cmd_req_updates, "Request delta-updates for TEST symbol (u 10 i) (u 5 l)");
    menu_.add_menu_option(spnr::ECommand::cmd_send_intvec, "Send intvec (<size>)");
    menu_.add_menu_option(spnr::ECommand::cmd_send_bytevec, "Send bytevec (<size>)");
    menu_.add_menu_option(spnr::ECommand::cmd_send_doublevec, "Send doublevec (<size>)");
    menu_.add_menu_option(spnr::ECommand::cmd_quit, "Close connection");
}

void spnr::AdminServerConnection::on_login_accepted()
{
    sendmenu();
};

void spnr::AdminServerConnection::on_subscribe_msg(const spnr::SubscribeMsg& m)
{
    static auto& cfg = spnr::Cfg::cfg();
    const auto& sym = m.symbols();
    if (sym.size() > 0)
    {
        if (cfg.is_info_log_level())
        {
            spnr::log("sub: %d", sym.size());
            for (const auto& s : sym) {
                spnr::log("%s", s.c_str());
            }
        }

        spnr::send_regenerated_snapshot(snapshot_, sym, this);

        send_text_msg("Snapshot sent");
        spnr::send_generated_delta<spnr::MdDelta, DeltaPublisher>(sym, admin_publisher, 20);
        send_text_msg("Delta sent");
    }

    sendmenu();
};


void spnr::AdminServerConnection::on_command_msg(const spnr::CommandMsg& cmd)
{
    auto c = cmd.command();
    const auto& params = cmd.parameters();
    std::string param_str;
    if (params.size() > 0) {
        param_str = spnr::join(params);
    }

    spnr::log("on-command [%c] [%s] [%s]", static_cast<char>(c), cmd.client_context().c_str(), param_str.c_str());

    switch (c)
    {
    case spnr::ECommand::cmd_echo_text:
    {
        spnr::TextMsg m(param_str);
        send_message(m);
    }break;
    case spnr::ECommand::cmd_req_updates:
    {
        if (params.size() > 0)
        {
            char upd_type = 'i';
            if (params.size() > 1) {
                auto s = params[1];
                if (s.length() > 0)
                    upd_type = s[0];
            }

            size_t num = atoi(params[0].c_str());
            if (num < 1)num = 1;
            std::vector<std::string> symbols;
            symbols.push_back("TEST");
            spnr::send_generated_subtype_delta<MdDelta, DeltaPublisher>(symbols, admin_publisher, num, upd_type);
        }
    }break;
    case spnr::ECommand::cmd_send_intvec:
    {
        if (params.size() > 0)
        {
            size_t num = atoi(params[0].c_str());
            spnr::IntVecMsg m;
            m.set_client_context(cmd.client_context());
            for (size_t n = 1; n <= num; ++n)
            {
                if (!m.add_num(n))
                    break;
            }
            send_message(m);
        }
    }break;
    case spnr::ECommand::cmd_send_bytevec:
    {
        if (params.size() > 0)
        {
            size_t num = atoi(params[0].c_str());
            spnr::ByteVecMsg m;
            m.set_client_context(cmd.client_context());
            for (size_t n = 1; n <= num; ++n)
            {
                if (!m.add_num(n))
                    break;
            }
            send_message(m);
        }
    }break;
    case spnr::ECommand::cmd_send_doublevec:
    {
        if (params.size() > 0)
        {
            size_t num = atoi(params[0].c_str());
            spnr::DoubleVecMsg m;
            m.set_client_context(cmd.client_context());
            for (size_t n = 1; n <= num; ++n)
            {
                if (!m.add_num(n * 100.01))
                    break;
            }
            send_message(m);
        }
    }break;
	default:break;

    }
    sendmenu();
};

void spnr::AdminServerConnection::sendmenu()
{
    static auto& cfg = spnr::Cfg::cfg();
    send_message(menu_);
    if (cfg.is_verbose_log_level())
    {
        spnr::log("Menu sent.");
    }
}

spnr::SingleTcpConnectionFactory<spnr::AdminServerConnection> reg;

void spnr::AdminServerConnection::listener_cb(struct evconnlistener*, evutil_socket_t fd,
    struct sockaddr*, int, void* user_data)
{
    struct event_base* base = (event_base*)user_data;
    struct bufferevent* bev;

    bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
    if (!bev) {
        spnr::errlog("Error constructing bufferevent");
        event_base_loopbreak(base);
        return;
    }
    auto c = reg.create_connection(bev);
    spnr::log("new admin connection [%s] [%d %d]", c->si().as_str(), c->si().rcv_buff_size(), c->si().send_buff_size());
    bufferevent_setcb(bev, spnr::TcpConnection::readcb, spnr::TcpConnection::writecb, spnr::TcpConnection::eventcb, c);
    bufferevent_enable(bev, EV_READ | EV_WRITE | EV_TIMEOUT);
}
