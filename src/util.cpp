#include "util.h"
#include "md_ref.h"
#include "tcp_connection.h"

int spnr::get_socket_error()
{
#if defined(_WIN32)
    return WSAGetLastError();
#else
    return errno;
#endif
};

bool spnr::is_would_block_socket_error()
{
#if defined(_WIN32)
    auto e = WSAGetLastError();
    bool would_block = (e == WSAEWOULDBLOCK);
#else
    auto e = errno;
    bool would_block = (e == EAGAIN || e == EWOULDBLOCK);
#endif                
    return would_block;
};

const char* spnr::get_socket_error_text() {

#if defined(_WIN32)
    static char message[256] = { 0 };
    FormatMessage(
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        0, WSAGetLastError(), 0, message, 256, 0);
    char* nl = strrchr(message, '\n');
    if (nl) *nl = 0;
    return message;
#else
    return strerror(errno);
#endif
}

std::optional<sockaddr_in> spnr::make_sockaddr(std::string host, int port) 
{
    sockaddr_in sin = { 0, 0, 0, 0 };
    memset(&sin, 0, sizeof(sin));

    if (!host.empty()) {
        struct hostent* he;
        if ((he = gethostbyname(host.c_str())) != NULL) {
			spnr::log("loaded address from hostname [%s]", host.c_str());
			memcpy(&sin.sin_addr, he->h_addr_list[0], he->h_length);
            //spnr::log("gethostbyname failed [%s]", host.c_str());
			//memcpy(&sin.sin_addr, host.c_str(), host.length());
            //return std::nullopt;
        }
		else{
			if(inet_pton(AF_INET, host.c_str(), &(sin.sin_addr)) == 1){
				spnr::log("loaded address from IP string");
			}
			else
			{
				spnr::errlog("Failed to load address from IP string [%s] [%d] [%s]",
						   host.c_str(),
						   spnr::get_socket_error(),
						   spnr::get_socket_error_text());
			}
			//memcpy(&sin.sin_addr, he->h_addr_list[0], he->h_length);
		}
    }

    sin.sin_family = AF_INET;
    sin.sin_port = htons(port);

    return std::optional<sockaddr_in>(sin);
};

bool spnr::init_workspace(int argc, char** argv, std::string log_prefix, bool use_ev_threads)
{
    if (argc < 2) {
        printf("Usage: %s config.properties <config-user-prefix>\n", argv[0]);
        return false;
    }

    std::string cfg_prefix = "";
    auto cfg_file = argv[1];
    if (argc > 2) {
        cfg_prefix = argv[2];
        log_prefix = log_prefix + "-" + cfg_prefix;
    }
	
    bool rv = spnr::init_spinner_workspace(cfg_file, log_prefix, cfg_prefix);

    if (rv) 
    {
#ifdef _WIN32
        WSADATA wsa_data;
        WSAStartup(0x0201, &wsa_data);
#endif

        bool ev_threads_enabled = false;

#ifdef EVTHREAD_USE_WINDOWS_THREADS_IMPLEMENTED
        if (use_ev_threads)
        {
            auto r = evthread_use_windows_threads();
            if (r == -1) {
                spnr::errlog("Failed to enable libevent threads support");
                return false;
            }
            ev_threads_enabled = true;
        }
#endif


#ifdef EVTHREAD_USE_PTHREADS_IMPLEMENTED	
        if (use_ev_threads)
        {
            auto r = evthread_use_pthreads();
            if (r == -1) {
                spnr::errlog("Failed to enable libevent threads support");
                return false;
            }
            ev_threads_enabled = true;
        }
#endif

        spnr::log("libevent threads support - [%s]", ev_threads_enabled ? "Yes" : "No");
    }

    return rv;
};

spnr::ECommand spnr::parse_command(const std::string& s)
{
    spnr::ECommand c = spnr::ECommand::cmd_none;
    if (s.length() > 0) {
        uint8_t v = s[0];
        switch (v)
        {
        case 's':
        case 'u':
        case 't':
        case 'n':
        case 'b':
        case 'l':
        case 'q':
            c = static_cast<spnr::ECommand>(v);
            break;
        default:
            spnr::errlog("unrecognized command [%s]", s.c_str());
            break;
        }
    }
    return c;
};



spnr::SocketInfo spnr::get_socket_info(evutil_socket_t fd, bool load_peer_skt_info)
{
    sockaddr_storage addr;
    socklen_t len = sizeof(addr);

    SocketInfo si;

    bool load_our_skt_info = true;

    if(load_our_skt_info)
    {
        memset(&addr, 0, sizeof(addr));
        if (getsockname(fd, (struct sockaddr*)&addr, &len))
        {
            perror("getsockname() failed");
        };

        if (addr.ss_family == AF_INET) {
            struct sockaddr_in* s = (struct sockaddr_in*)&addr;
            si.port_ = ntohs(s->sin_port);
            inet_ntop(AF_INET, &s->sin_addr, si.ip_, sizeof si.ip_);
        }
        else { // AF_INET6
            struct sockaddr_in6* s = (struct sockaddr_in6*)&addr;
            si.port_ = ntohs(s->sin6_port);
            inet_ntop(AF_INET6, &s->sin6_addr, si.ip_, sizeof si.ip_);
        }
    }

    if(load_peer_skt_info)
    {
        memset(&addr, 0, sizeof(addr));
        if (getpeername(fd, (struct sockaddr*)&addr, &len))
        {
            perror("getpeername() failed");
        };

        if (addr.ss_family == AF_INET) {
            struct sockaddr_in* s = (struct sockaddr_in*)&addr;
            si.peer_port_ = ntohs(s->sin_port);
            inet_ntop(AF_INET, &s->sin_addr, si.peer_ip_, sizeof si.peer_ip_);
        }
        else { // AF_INET6
            struct sockaddr_in6* s = (struct sockaddr_in6*)&addr;
            si.peer_port_ = ntohs(s->sin6_port);
            inet_ntop(AF_INET6, &s->sin6_addr, si.peer_ip_, sizeof si.peer_ip_);
        }
    }

    int n;
    socklen_t m = sizeof(n);
    getsockopt(fd, SOL_SOCKET, SO_RCVBUF, (char*)&n, &m);
    si.rcv_buff_size_ = n;
    getsockopt(fd, SOL_SOCKET, SO_SNDBUF, (char*)&n, &m);
    si.send_buff_size_ = n;
    si.fd_ = fd;

    return si;
}
