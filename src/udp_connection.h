#pragma once
#include "socket_buffer.h"
#include "archive.h"

namespace spnr
{
    class UdpConnectionFactory;

    class UdpConnection
    {
    public:
        UdpConnection(UdpConnectionFactory* f, evutil_socket_t fd);        

        virtual void register_udp_subscriber(const sockaddr_in&, const LoginMsg&) {};
        virtual void unregister_udp_subscriber(const sockaddr_in&) {};

        template<class T>
        int send_message_to(const sockaddr_in& addr, const T& m, uint32_t session_index)
        {
            auto h = spnr::start_header(m.msgtype());
            h.h.len += m.calc_len();
            h.h.session_idx = session_index;
            //writer_session_index_ = h.h.session_idx;
            BuffMsgArchive ar;
            if (!ar.store_header(h))
                return -1;
            ar << m;
            const auto& ob = ar.bytebuff();
            auto bytes_sent = sendto(si_.fd_, ob.buff(), ob.len(), 0, (struct sockaddr*)&addr, sizeof(sockaddr_in));
            return bytes_sent;
        }        

        bool send_hb();

        void                    set_sin(const sockaddr_in& sin);
        const sockaddr_in&      sin()const { return sin_; }
        
        inline uint32_t         reader_session_index()const { return reader_session_index_; }
        inline void             set_reader_session_index(uint32_t v) { reader_session_index_ = v; }


        static void udp_publisher_readcb(evutil_socket_t fd, short, void* ctx);
        static void udp_subscriber_readcb(evutil_socket_t fd, short, void* ctx);

    protected:
        bool udp_stream_in_archive(const sockaddr_in& sin, EMsgType mt, const BuffMsgArchive& ar);

        virtual void on_login_msg       (const sockaddr_in&, const LoginMsg&) {};
        virtual void on_login_reply_msg (const sockaddr_in&, const LoginReplyMsg&) {};
        virtual void on_text_msg        (const sockaddr_in&, const TextMsg&) {};
        virtual void on_hb_msg          (const sockaddr_in&, const HBMsg&) {};
        virtual void on_menu_msg        (const sockaddr_in&, const MenuMsg&) {};
        virtual void on_subscribe_msg   (const sockaddr_in&, const SubscribeMsg&) {};
        virtual void on_md_reference_msg(const sockaddr_in&, const MdRef&) {};
        virtual void on_md_delta_msg    (const sockaddr_in&, const MdDelta&) {};
        virtual void on_intvec_msg      (const sockaddr_in&, const IntVecMsg&) {};
        virtual void on_bytevec_msg     (const sockaddr_in&, const ByteVecMsg&) {};
        virtual void on_doublevec_msg   (const sockaddr_in&, const DoubleVecMsg&) {};
        virtual void on_command_msg     (const sockaddr_in&, const CommandMsg&) {};

    protected:
        UdpConnectionFactory*   factory_;
        sockaddr_in             sin_;
        spnr::SocketInfo          si_;
        mutable WireReaderStat  stat_;
        uint32_t                reader_session_index_ = 0;        

    private:
        void dispose_connection();

    };

    class UdpConnectionFactory
    {
    public:
        virtual UdpConnection* create_connection(evutil_socket_t fd) = 0;
        virtual void dispose_connection(UdpConnection* ) = 0;
    };

    template<class C>
    class SingleUdpConnectionFactory : public spnr::UdpConnectionFactory
    {
    public:
        spnr::UdpConnection* create_connection(evutil_socket_t fd)override
        {
            c_ = std::make_unique<C>(this, fd);
            auto rv = c_.get();
            return rv;
        };

        void dispose_connection(UdpConnection* )override
        {
            c_.reset();
        };

        //void set_connection(std::unique_ptr<C>&& c) { c_ = std::move(c); }
        C* get_connection() { return c_.get(); }

    protected:
        std::unique_ptr<C> c_;
    };

    int run_udp_publisher(std::string host, int port, UdpConnectionFactory* f);
    int run_udp_subscriber(std::string host, int port, UdpConnectionFactory* f);
}
