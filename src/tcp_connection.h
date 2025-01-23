#pragma once

#include "socket_buffer.h"
#include "archive.h"

namespace spnr
{
    class TcpConnectionFactory;

    class TcpConnection
    {
    public:
        TcpConnection(TcpConnectionFactory* f, struct bufferevent* b);        

        bool read_connection_buffer(evbuffer* b);

        bool send_text_msg(const std::string& text);
        bool send_hb();
        bool is_logged_in()const;

        void                    close_connection();
        bool                    is_valid()const;
        inline uint32_t         writer_session_index()const { return writer_session_index_; }
        inline uint32_t         reader_session_index()const { return reader_session_index_; }
        inline void             set_reader_session_index(uint32_t v) { reader_session_index_ = v; }
        const SocketInfo&       si()const { return si_; }
        const std::string&      name()const { return name_; }
        time_t                  last_hb_time()const;
        void                    reset_hb_time();
        bool                    is_throttle_on()const;

        template<class T>
        bool send_message(const T& m)
        {
            std::unique_lock lk(mtx_sender_);
            auto h = spnr::start_header(m.msgtype());
            h.h.len += m.calc_len();
            h.h.session_idx = skt_.next_session_index();
            writer_session_index_ = h.h.session_idx;

            BuffMsgArchive ar;
            if (!ar.store_header(h)) {
                close_connection();
                return false;
            }
            ar << m;
            auto rv = skt_.send_buffer(ar.bytebuff());
            if (!rv) {
                close_connection();
            }
            return rv;
        }

        static void eventcb(bufferevent* bev, short events, void* ctx);
        static void writecb(bufferevent* bev, void* ctx);
        static void readcb(bufferevent* bev, void* ctx);
    protected:
        bool stream_in_archive(EMsgType mt, const BuffMsgArchive& ar);

        virtual void on_connected() {};
        virtual void on_login_msg(const LoginMsg&);
        virtual void on_login_reply_msg(const LoginReplyMsg&) {};
        virtual void on_login_accepted() {};
        virtual void on_text_msg(const TextMsg&) {};
        virtual void on_hb_msg(const HBMsg&);
        virtual void on_menu_msg(const MenuMsg&) {};
        virtual void on_subscribe_msg(const SubscribeMsg&) {};
        virtual void on_md_reference_msg(const MdRef&) {};
        virtual void on_md_delta_msg(const MdDelta&) {};
        virtual void on_intvec_msg(const IntVecMsg&) {};
        virtual void on_bytevec_msg(const ByteVecMsg&) {};
        virtual void on_doublevec_msg(const DoubleVecMsg&) {};
        virtual void on_command_msg(const CommandMsg&) {};

    protected:
        std::string             loged_in_user_;        
        TcpSocketQueue          skt_;
        TcpConnectionFactory*   factory_ = nullptr;        
        bufferevent*            bev_ = nullptr;
        std::mutex              mtx_sender_;
        uint32_t                writer_session_index_ = 0;
        uint32_t                reader_session_index_ = 0;
        mutable WireReaderStat  stat_;
        spnr::SocketInfo          si_;
        std::string             name_;
        std::atomic<time_t>     last_hb_time_;

    private:
        void dispose_connection();
    };

    class TcpConnectionFactory
    {
    public:
        virtual TcpConnection* create_connection(bufferevent* b) = 0;
        virtual void dispose_connection(TcpConnection* ) = 0;
    };
    

    class MultiTcpConnectionFactory :public TcpConnectionFactory
    {
    public:
        virtual TcpConnection* create_connection(bufferevent* b) = 0;

        virtual void add_buscriptions(TcpConnection* c, const std::vector<std::string>& symbols) = 0;
    };


    template<class C>
    class SingleTcpConnectionFactory : public spnr::TcpConnectionFactory
    {
    public:
        spnr::TcpConnection* create_connection(bufferevent* b)override
        {
            c_ = std::make_unique<C>(this, b);
            auto rv = c_.get();
            return rv;
        };
        void dispose_connection(TcpConnection* )override
        {
            c_.reset();
        };
        void set_connection(std::unique_ptr<C>&& c) { c_ = std::move(c); }
        C* get_connection() { return c_.get(); }

    protected:
        std::unique_ptr<C> c_;
    };

    int run_tcp_listener(const std::vector<listener_info>& linfo);
    int run_tcp_connector(std::string host, int port, TcpConnectionFactory* reg);
};
