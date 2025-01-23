#pragma once

#include "spnr.h"
#include "records.h"
#include "archive.h"
#include "md_ref.h"
#include "ref_table.h"
#include "simulator.h"
#include "tcp_connection.h"

namespace spnr
{
    class AdminServerConnection : public spnr::TcpConnection
    {
        spnr::MenuMsg     menu_;
        spnr::MdRefTable  snapshot_;
        spnr::STR_SET     subscription_;
    public:
        AdminServerConnection(spnr::TcpConnectionFactory* r, struct bufferevent* b);

        static void listener_cb(struct evconnlistener* listener, evutil_socket_t fd,
            struct sockaddr* sa, int socklen, void* user_data);
    protected:
        void sendmenu();
        void on_login_accepted()override;
        void on_subscribe_msg(const spnr::SubscribeMsg& m)override;
        void on_command_msg(const spnr::CommandMsg& cmd)override;        
    };
};