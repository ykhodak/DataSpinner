#include "udp_connection.h"

void spnr::UdpConnection::dispose_connection()
{
    factory_->dispose_connection(this);
};

void spnr::UdpConnection::set_sin(const sockaddr_in& sin) 
{
    sin_ = sin;
};

bool spnr::UdpConnection::udp_stream_in_archive(const sockaddr_in& sin, spnr::EMsgType mt, const spnr::BuffMsgArchive& ar)
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
        on_login_msg(sin, m);
    }break;
    case EMsgType::login_reply:
    {
        LoginReplyMsg m;
        ar >> m;
        if (!ar()) {
            return false;
        }
        on_login_reply_msg(sin, m);
    }break;
    case EMsgType::text:
    {
        TextMsg m;
        ar >> m;
        if (!ar()) {
            return false;
        }
        on_text_msg(sin, m);
    }break;
    case EMsgType::heartbeat:
    {
        HBMsg m;
        ar >> m;
        if (!ar()) {
            return false;
        }
        on_hb_msg(sin, m);
    }break;
    case EMsgType::subscribe:
    {
        SubscribeMsg m;
        ar >> m;
        if (!ar()) {
            return false;
        }
        on_subscribe_msg(sin, m);
    }break;
    case EMsgType::md_refference:
    {
        MdRef m;
        ar >> m;
        if (!ar()) {
            return false;
        }
        on_md_reference_msg(sin, m);
    }break;
    case EMsgType::md_delta:
    {
        MdDelta m;
        ar >> m;
        if (!ar()) {
            return false;
        }
        on_md_delta_msg(sin, m);
    }break;
    case EMsgType::interactive_menu:
    {
        MenuMsg m;
        ar >> m;
        if (!ar()) {
            return false;
        }
        on_menu_msg(sin, m);
    }break;
    case EMsgType::intvec:
    {
        IntVecMsg m;
        ar >> m;
        if (!ar()) {
            return false;
        }
        on_intvec_msg(sin, m);
    }break;
    case EMsgType::bytevec:
    {
        ByteVecMsg m;
        ar >> m;
        if (!ar()) {
            return false;
        }
        on_bytevec_msg(sin, m);
    }break;
    case EMsgType::doublevec:
    {
        DoubleVecMsg m;
        ar >> m;
        if (!ar()) {
            return false;
        }
        on_doublevec_msg(sin, m);
    }break;
    case EMsgType::command:
    {
        CommandMsg m;
        ar >> m;
        if (!ar()) {
            return false;
        }
        on_command_msg(sin, m);
    }break;
    default:
    {
        spnr::errlog("unrecognized message");
    }break;
    }

    return true;
};


void spnr::UdpConnection::udp_subscriber_readcb(evutil_socket_t fd, short, void* ctx)
{
	static const auto& cfg = spnr::Cfg::cfg();
    auto c = (spnr::UdpConnection*)ctx;
    spnr::BuffMsgArchive ar;
    auto& bb = ar.bytebuff();
    constexpr size_t hdr_len = sizeof(spnr::UMsgHeader);
    sockaddr_in  svr_addr;
    socklen_t size = sizeof(struct sockaddr_in);
    int len = recvfrom(fd, bb.buff_, sizeof(bb.buff_), 0, (struct sockaddr*)&svr_addr, &size);
    if (len == -1) {
        perror("recvfrom()");
    }
    else if (len == 0) {
        printf("Connection Closed\n");
    }
    else {
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(svr_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
		// int client_port = htons(svr_addr.sin_port);

        //spnr::log("upd-read [%s:%d] %d", client_ip, client_port, len);
        uint64_t* hdr_data = reinterpret_cast<uint64_t*>(bb.buff_);
        UMsgHeader h;
        h.d = spnr::ntoh64(*hdr_data);

        if (h.h.pfix != TAG_SOH) {
            spnr::errlog("corrupted UDP msg, expected SOH [%d]", h.h.pfix);
            return;
        };

        int l2 = h.h.len + hdr_len;
        if (len < l2) {
            spnr::errlog("incomplete message arrived over UDP [%d < %d]", len, l2);
        }

        if (c->reader_session_index() != 0) {
            auto delta = h.h.session_idx - c->reader_session_index();
            if (delta != 1) {
				// spnr::errlog("session index gap [%d] [%d-%d]", delta, h.h.session_idx, c->reader_session_index());
                //return;
            }
        }
        c->set_reader_session_index(h.h.session_idx);
        //spnr::log("upd-read #%d [%s:%d] %d", h.h.session_idx, client_ip, client_port, len);

        if (h.h.len > ARCHIVE_BUFF_CAPACITY) {
            spnr::errlog("record read error, buffer overflow in UDP [%d]", h.h.len);
            return;
        }
        bb.len_ = h.h.len;
        bb.read_ = 0;
        bb.start_pos_ = hdr_len;
        EMsgType mt = static_cast<EMsgType>(h.h.mtype);

        uint64_t wire_tstamp = 0;
        ar >> wire_tstamp;
        uint64_t wire_in_tstamp = spnr::time_stamp();
        auto wire_latency = wire_in_tstamp - wire_tstamp;
        c->stat_.on_reader_progress(h.h.len, mt, wire_latency, h.h.session_idx);
		bool skip_packet = cfg.idle_on_delta_ && (mt == EMsgType::md_delta);
		if(!skip_packet){
			if (!c->udp_stream_in_archive(svr_addr, mt, ar)) {
				spnr::errlog("failed to serialize in record from UDP [%d]", h.h.len);
				return;
			}
		}
    }
}

void spnr::UdpConnection::udp_publisher_readcb(evutil_socket_t fd, short , void* ctx)
{
    static auto& cfg = Cfg::cfg();

    auto c = (spnr::UdpConnection*)ctx;

    spnr::BuffMsgArchive ar;
    auto& bb = ar.bytebuff();
    constexpr size_t hdr_len = sizeof(spnr::UMsgHeader);
    sockaddr_in  client_addr;
	socklen_t size = sizeof(struct sockaddr_in);
    int len = recvfrom(fd, bb.buff_, sizeof(bb.buff_), 0, (struct sockaddr*)&client_addr, &size);

    if (len == -1) {
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
        int client_port = htons(client_addr.sin_port);
        if (cfg.is_verbose_log_level()) {
            spnr::errlog("recvfrom failed for [%s:%d] will diconnect client", client_ip, client_port);
        }
        c->unregister_udp_subscriber(client_addr);
        //perror("recvfrom()");
    }
    else if (len == 0) {
        printf("Connection Closed\n");
    }
    else {
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
        int client_port = htons(client_addr.sin_port);

        //spnr::log("upd-read [%s:%d] %d", client_ip, client_port, len);
        //printf("Read: len [%d] - content [%s]\n", len, bb.buff_);
        uint64_t* hdr_data = reinterpret_cast<uint64_t*>(bb.buff_);
        UMsgHeader h;
        h.d = spnr::ntoh64(*hdr_data);

        if (h.h.pfix != TAG_SOH) {
            spnr::errlog("corrupted UDP msg, expected SOH [%d]", h.h.pfix);
            return;
        };

        int l2 = h.h.len + hdr_len;
        if (len < l2) {
            spnr::errlog("incomplete message arrived over UDP [%d < %d]", len, l2);            
        }
       
        if (h.h.len > ARCHIVE_BUFF_CAPACITY) {
            spnr::errlog("record read error, buffer overflow in UDP [%d]", h.h.len);
            return;
        }
        bb.len_ = h.h.len;
        bb.read_ = 0;
        bb.start_pos_ = hdr_len;
        EMsgType mt = static_cast<EMsgType>(h.h.mtype);

        uint64_t wire_tstamp = 0;
        ar >> wire_tstamp;
        uint64_t wire_in_tstamp = spnr::time_stamp();
        auto wire_latency = wire_in_tstamp - wire_tstamp;
        c->stat_.on_reader_progress(h.h.len, mt, wire_latency, h.h.session_idx);

        if (mt == EMsgType::login) 
        {
            LoginMsg m;
            ar >> m;
            if (!ar()) {
                spnr::errlog("failed to serialize in UDP login [%d]", h.h.len);
                return;
            }

            spnr::LoginReplyMsg r;
            r.set_reply_code(1);
            r.set_reply_description("accepted");
            auto res = c->send_message_to(client_addr, r, 0);
            if (res < 0) {
                spnr::errlog("failed to serialize in UDP login reply [%s:%d] %d", client_ip, client_port, len);
                return;
            }

            std::string user_name = m.user();
            spnr::log("login accepted [%s]", user_name.c_str());
            c->register_udp_subscriber(client_addr, m);
        }
        else
        {
            if (!c->udp_stream_in_archive(client_addr, mt, ar)) {
                spnr::errlog("failed to serialize in record from UDP [%d]", h.h.len);
                return;
            }
        }
        //reader_session_index_ = h.h.session_idx;


       // sendto(fd, buf, len, 0, (struct sockaddr*)&client_addr, size);
    }
}

//...


int spnr::run_udp_publisher(std::string host, int port, UdpConnectionFactory* f)
{
    auto a = spnr::make_sockaddr(host, port);
    if (!a) {
        return -1;
    }
    sockaddr_in sin = a.value();

    //ent_base* base = nullptr;
    evutil_socket_t sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock_fd < 0) {
        spnr::errlog("Error creating udp socket: [%d] [%s]", spnr::get_socket_error(), spnr::get_socket_error_text());
        return 1;
    }

    if (bind(sock_fd, (struct sockaddr*)&sin, sizeof(sin))) {
        perror("bind()");
        exit(EXIT_FAILURE);
    }

    event_base* base = event_base_new();
    if (!base) {
        spnr::errlog("Could not initialize libevent");
        return 1;
    }
    auto c = f->create_connection(sock_fd);

    event* udp_event = event_new(base, sock_fd, EV_READ | EV_PERSIST, spnr::UdpConnection::udp_publisher_readcb, c);
    event_add(udp_event, NULL);

    spnr::log("UDP bind started [%s][%d]", host.c_str(), port);

    event_base_dispatch(base);
    event_base_free(base);
    spnr::log("UDP bind finished [%s][%d]", host.c_str(), port);
    return 0;
};

int spnr::run_udp_subscriber(std::string host, int port, UdpConnectionFactory* f) 
{
    auto& cfg = spnr::Cfg::cfg();

    //  auto& cfg = spnr::Cfg::cfg();
    auto a = spnr::make_sockaddr(host, port);
    if (!a) {
        return -1;
    }
    sockaddr_in sin = a.value();
//    size_t sin_size = sizeof(sin);

    auto sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock_fd < 0) {
        spnr::errlog("Error creating udp socket: [%d] [%s]", spnr::get_socket_error(), spnr::get_socket_error_text());
        return 1;
    }

    event_base* base = event_base_new();
    if (!base) {
        spnr::errlog("Could not initialize libevent");
        return 1;
    }
    auto c = f->create_connection(sock_fd);
    c->set_sin(sin);

    struct event* udp_event = event_new(base, sock_fd, EV_READ | EV_PERSIST, spnr::UdpConnection::udp_subscriber_readcb, c);
    event_add(udp_event, NULL);
    //...
    
    spnr::LoginMsg m;
    m.set_user(cfg.client_user);
    m.set_token("user-token_1");
    spnr::log("trying to login [%s:%d] as [%s]", host.c_str(), port, cfg.client_user.c_str());
    auto res = c->send_message_to(sin, m, 0);
    if (res < 0) {
        spnr::errlog("failed to serialize in UDP login reply [%s:%d]", host, port);
        return 1;
    }

    event_base_dispatch(base);
    event_base_free(base);
    spnr::log("UDP subscriber finished [%s][%d]", host.c_str(), port);
    return 0;
};

spnr::UdpConnection::UdpConnection(UdpConnectionFactory* f, evutil_socket_t fd)
    :factory_(f)
{
    si_ = spnr::get_socket_info(fd, false);
};

bool spnr::UdpConnection::send_hb() 
{
    HBMsg m(reader_session_index_);
    return send_message_to(sin_, m, reader_session_index_);
};
