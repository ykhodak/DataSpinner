#include "tcp_connection.h"

void spnr::TcpConnection::writecb(struct bufferevent* bev, void* ctx)
{
    auto c = (spnr::TcpConnection*)ctx;
    struct evbuffer* output = bufferevent_get_output(bev);
    size_t len = evbuffer_get_length(output);
    if (len != 0) {
        spnr::log("writecb [%s] [%zu]", c->si().as_str(), len);
    }
};

void spnr::TcpConnection::readcb(bufferevent* bev, void* ctx)
{
    auto c = (spnr::TcpConnection*)ctx;
    evbuffer* b = bufferevent_get_input(bev);
    if (!c->read_connection_buffer(b)) {
        spnr::errlog("read_buffer failed, closing connection.");
        c->close_connection();
    };
};


void spnr::TcpConnection::eventcb(struct bufferevent* bev, short events, void* ctx)
{
    auto c = (spnr::TcpConnection*)ctx;
    bool free_con = false;
    spnr::log("conn_eventcb [%s]", c->si().as_str());
    if (events & BEV_EVENT_CONNECTED) {
        spnr::log("client connected");
        c->on_connected();
    }
    else if (events & BEV_EVENT_EOF) {
        spnr::log("Connection closed [%s].", c->si().as_str());
        free_con = true;
    }
    else if (events & BEV_EVENT_ERROR) {
        spnr::errlog("Got an error on the connection[%s]: %s", c->si().as_str(), strerror(errno));
        free_con = true;
    }
    else if (events & BEV_EVENT_TIMEOUT) {
        spnr::log("Time out.");
    }

    if (free_con)
    {
        auto c = (spnr::TcpConnection*)ctx;
        c->close_connection();
        c->dispose_connection();
        bufferevent_free(bev);
    }
}

void spnr::TcpConnection::dispose_connection()
{
    factory_->dispose_connection(this);
};


spnr::TcpConnection::TcpConnection(TcpConnectionFactory* f, bufferevent* b)
    :factory_(f), bev_(b)
{
    auto fd = bufferevent_getfd(b);
    si_ = spnr::get_socket_info(fd, true);
    skt_.set_socket_fd(si_.fd_);
    last_hb_time_.store(0);
};

void spnr::TcpConnection::on_login_msg(const spnr::LoginMsg& m)
{
    loged_in_user_ = m.user();
    name_ = loged_in_user_;
    spnr::log("login accepted [%s]", loged_in_user_.c_str());
    skt_.set_name(name_);

    spnr::LoginReplyMsg r;
    r.set_reply_code(1);
    r.set_reply_description("accepted");
    send_message(r);
    stat_.start_reader_stat();
    on_login_accepted();
};

bool spnr::TcpConnection::is_logged_in()const
{
    return (!loged_in_user_.empty());
};

bool spnr::TcpConnection::read_connection_buffer(evbuffer* b)
{
    static const auto& cfg = spnr::Cfg::cfg();

    size_t buff_len = evbuffer_get_length(b);
    constexpr size_t hdr_len = sizeof(UMsgHeader);
    BuffMsgArchive ar;
    auto& bb = ar.bytebuff();

    while (buff_len > hdr_len) // The size field hasn't arrived.
    {
        if (cfg.emulate_slow_client_) {
            std::this_thread::sleep_for(std::chrono::microseconds(1));
        }

        uint64_t hdr_data;
        evbuffer_copyout(b, &hdr_data, hdr_len);
        UMsgHeader h;
        h.d = spnr::ntoh64(hdr_data);

        if (h.h.pfix != TAG_SOH) {
            spnr::errlog("corrupted msg, expected SOH [%d]", h.h.pfix);
            return false;
        };

        uint32_t l2 = h.h.len + hdr_len;
        if (buff_len < l2) {
            return true;/* The record hasn't arrived */
        }

        evbuffer_drain(b, hdr_len);

        if (reader_session_index() != 0) {
            auto delta = h.h.session_idx - reader_session_index();
            if (delta != 1) {
                spnr::errlog("session index gap [%d] [%d-%d]", delta, h.h.session_idx, reader_session_index());
                //return false;
            }
        }
        set_reader_session_index(h.h.session_idx);

        
        /// copying data directly into internal archive buffer ///
        if (h.h.len > ARCHIVE_BUFF_CAPACITY) {
            errlog("record read error, buffer overflow");
            return false;
        }        
        if (evbuffer_remove(b, bb.buff_, h.h.len) == -1) {
            errlog("record read error");
            return false;
        };
        bb.len_ = h.h.len;
        bb.read_ = 0;

		EMsgType mt = static_cast<EMsgType>(h.h.mtype);

        uint64_t wire_tstamp = 0;
        ar >> wire_tstamp;
        uint64_t wire_in_tstamp = spnr::time_stamp();
        uint64_t wire_latency = wire_in_tstamp - wire_tstamp;       
        stat_.on_reader_progress(h.h.len, mt, wire_latency, reader_session_index());

		bool skip_packet = cfg.idle_on_delta_ && (mt == EMsgType::md_delta);
		
		if(!skip_packet){
			if(!stream_in_archive(mt, ar))
				return false;
		}
		
        buff_len = evbuffer_get_length(b);
    }
    return true;
};

bool spnr::TcpConnection::send_text_msg(const std::string& text)
{
    spnr::TextMsg m(text);
    return send_message(m);
};

bool spnr::TcpConnection::send_hb()
{
    HBMsg m(reader_session_index_);
    return send_message(m);
};

void spnr::TcpConnection::close_connection()
{
    if (bev_) {
        spnr::log("closing connection [%s]", name_.c_str());

#if defined(_WIN32)
        closesocket(si_.fd_);
#else
        shutdown(si_.fd_, SHUT_RDWR);
        close(si_.fd_);
#endif

        bev_ = nullptr;
    }
};

bool spnr::TcpConnection::is_valid()const
{
    return (bev_ != nullptr);
};

int spnr::run_tcp_listener(const std::vector<listener_info>& linfo)
{
    event_base* base = nullptr;
    evconnlistener* listener = nullptr;

    base = event_base_new();
    if (!base) {
        spnr::errlog("Could not initialize libevent");
        return 1;
    }

    for (const auto& i : linfo) {
        auto a = spnr::make_sockaddr("", i.port);
        if (a)
        {
            sockaddr_in sin = a.value();

            listener = evconnlistener_new_bind(base, i.listener_cb, (void*)base,
                LEV_OPT_REUSEABLE | LEV_OPT_CLOSE_ON_FREE, -1,
                (struct sockaddr*)&sin,
                sizeof(sin));

            if (!listener) {
                spnr::errlog("Could not create a listener");
                return 1;
            }

            char ip[64] = "";
            inet_ntop(AF_INET, &sin.sin_addr, ip, sizeof(ip));
            spnr::log("Started listener [%s] on port [%d]", ip, i.port);
        }
    }

    event_base_dispatch(base);
    evconnlistener_free(listener);
    event_base_free(base);
    spnr::log("Finished tcp listener");
    return 0;
}

int spnr::run_tcp_connector(std::string host, int port, TcpConnectionFactory* reg)
{
    event_base* base;
    bufferevent* bev;

    auto a = spnr::make_sockaddr(host, port);
    if (!a) {
        return -1;
    }
    sockaddr_in sin = a.value();

    base = event_base_new();
    if (!base) {
        spnr::log("Could not initialize libevent");
        return 1;
    }


    bev = bufferevent_socket_new(base, -1, BEV_OPT_CLOSE_ON_FREE);


    if (bufferevent_socket_connect(bev,
        (struct sockaddr*)&sin, sizeof(sin)) != 0)
    {
        spnr::errlog("Could not establish connection [%s] [%d]", host.c_str(), port);
        bufferevent_free(bev);
        return -1;
    }

    auto c = reg->create_connection(bev);

    bufferevent_setcb(bev, spnr::TcpConnection::readcb, spnr::TcpConnection::writecb, spnr::TcpConnection::eventcb, c);
    bufferevent_enable(bev, EV_READ | EV_WRITE | EV_TIMEOUT);


    event_base_dispatch(base);
    return 0;
};

bool spnr::TcpConnection::stream_in_archive(EMsgType mt, const BuffMsgArchive& ar)
{
    switch (mt)
    {
    case EMsgType::login:
    {
        LoginMsg m;
        ar >> m;
        if (!ar()) {
            return false;
        }
        on_login_msg(m);
    }break;
    case EMsgType::login_reply:
    {
        LoginReplyMsg m;
        ar >> m;
        if (!ar()) {
            return false;
        }
        on_login_reply_msg(m);
    }break;
    case EMsgType::text:
    {
        TextMsg m;
        ar >> m;
        if (!ar()) {
            return false;
        }
        on_text_msg(m);
    }break;
    case EMsgType::heartbeat:
    {
        HBMsg m;
        ar >> m;
        if (!ar()) {
            return false;
        }
        on_hb_msg(m);
    }break;
    case EMsgType::subscribe:
    {
        SubscribeMsg m;
        ar >> m;
        if (!ar()) {
            return false;
        }
        on_subscribe_msg(m);
    }break;
    case EMsgType::md_refference:
    {
        MdRef m;
        ar >> m;
        if (!ar()) {
            return false;
        }
        on_md_reference_msg(m);
    }break;
    case EMsgType::md_delta:
    {
        MdDelta m;
        ar >> m;
        if (!ar()) {
            return false;
        }
        on_md_delta_msg(m);
    }break;
    case EMsgType::interactive_menu:
    {
        MenuMsg m;
        ar >> m;
        if (!ar()) {
            return false;
        }
        on_menu_msg(m);
    }break;
    case EMsgType::intvec:
    {
        IntVecMsg m;
        ar >> m;
        if (!ar()) {
            return false;
        }
        on_intvec_msg(m);
    }break;
    case EMsgType::bytevec:
    {
        ByteVecMsg m;
        ar >> m;
        if (!ar()) {
            return false;
        }
        on_bytevec_msg(m);
    }break;
    case EMsgType::doublevec:
    {
        DoubleVecMsg m;
        ar >> m;
        if (!ar()) {
            return false;
        }
        on_doublevec_msg(m);
    }break;
    case EMsgType::command:
    {
        CommandMsg m;
        ar >> m;
        if (!ar()) {
            return false;
        }
        on_command_msg(m);
    }break;
    default:
    {
        spnr::errlog("unrecognized message");
    }break;
    }

    return true;
};

time_t spnr::TcpConnection::last_hb_time()const
{
    return last_hb_time_.load();
};

void spnr::TcpConnection::on_hb_msg(const spnr::HBMsg& )
{
    last_hb_time_.store(time(nullptr));
};

void spnr::TcpConnection::reset_hb_time() 
{
    last_hb_time_.store(time(nullptr));
};

bool spnr::TcpConnection::is_throttle_on()const 
{
    return skt_.is_throttle_on();
};
