#pragma once


#include "spnr.h"
#include "spinner_core_util.h"
#include "records.h"
#include "config.h"

#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/util.h>
#include <event2/event.h>
#include <event2/listener.h>
#include <event2/event.h>
#include <event2/thread.h>

namespace spnr
{
    struct MsgHeader
    {
        uint8_t     pfix;
        uint8_t     mtype;
        uint16_t    len;
        uint32_t    session_idx;
    };

    union UMsgHeader
    {
        MsgHeader   h;
        uint64_t    d;
    };

	
    class MdRef;
    class MdDelta;

    struct listener_info
    {
        int port;
        evconnlistener_cb listener_cb;
    };

    
    int             get_socket_error();
    bool            is_would_block_socket_error();
    const char*     get_socket_error_text();
    std::optional<sockaddr_in>    make_sockaddr(std::string host, int port);

    inline uint64_t hton64(uint64_t v)
    {
        const uint32_t upper_n = htonl(v >> 32);
        const uint32_t lower_n = htonl((uint32_t)v);
        uint64_t rv = upper_n;
        rv = (rv << 32) + lower_n;
        return rv;
    };

    inline uint64_t ntoh64(uint64_t v)
    {
        const uint32_t upper_h = ntohl(v >> 32);
        const uint32_t lower_h = ntohl((uint32_t)v);
        uint64_t rv = upper_h;
        rv = (rv << 32) + lower_h;
        return rv;
    };    

   


    bool init_workspace(int argc, char** argv, std::string log_prefix, bool use_ev_threads = true);
    ECommand parse_command(const std::string& s);


    struct SocketInfo
    {
        const char* as_str()const
        {
            if (info_[0] == 0) {
#ifdef _WIN32
                sprintf(info_, "%zu/%s %d->%s %d [%d-%d]", fd_, ip_, port_, peer_ip_, peer_port_, send_buff_size_, rcv_buff_size_);
#else
                sprintf(info_, "%d/%s %d->%s %d [%d-%d]", fd_, ip_, port_, peer_ip_, peer_port_, send_buff_size_, rcv_buff_size_);
#endif
            }
            return info_;
        };

        int send_buff_size()const { return send_buff_size_; }
        int rcv_buff_size()const { return rcv_buff_size_; }

        evutil_socket_t fd_{ 0 };
        char ip_[32] = "";
        char peer_ip_[32] = "";
        unsigned int port_{ 0 }, peer_port_{ 0 };
        int send_buff_size_{ 0 }, rcv_buff_size_{ 0 };
        mutable char info_[128] = "";        
    };

    SocketInfo get_socket_info(evutil_socket_t fd, bool load_peer_skt_info);
};
