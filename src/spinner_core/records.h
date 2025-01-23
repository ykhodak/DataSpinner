#pragma once

#include "spnr.h"

namespace spnr
{    
    class BuffMsgArchive;

    class TextMsg
    {
    public:
        TextMsg();
        TextMsg(const std::string&);
        EMsgType msgtype()const { return EMsgType::text; }

        std::string& text() { return m_text; }
        const std::string& text()const { return m_text; }

        uint16_t calc_len()const;
    protected:        
        std::string m_text;
        friend BuffMsgArchive& operator<<(BuffMsgArchive& ar, const spnr::TextMsg& d);
        friend const BuffMsgArchive& operator>>(const BuffMsgArchive& ar, spnr::TextMsg& d);
    };

    class HBMsg
    {
    public:
        HBMsg();
        HBMsg(uint32_t num);
        EMsgType msgtype()const { return EMsgType::heartbeat; }

        uint32_t    counter()const { return counter_; }
        uint16_t    calc_len()const;
    protected:
        uint32_t counter_;
        friend BuffMsgArchive& operator<<(BuffMsgArchive& ar, const spnr::HBMsg& d);
        friend const BuffMsgArchive& operator>>(const BuffMsgArchive& ar, spnr::HBMsg& d);
    };


    class SubscribeMsg
    {
    public:
        SubscribeMsg() {};
        EMsgType msgtype()const { return EMsgType::subscribe; }
        const STR_VEC& symbols()const { return symbols_; };
        STR_VEC& symbols() { return symbols_; };

        void add_symbols(const STR_VEC& symbols);
        void add_symbol(const std::string& s);
        uint16_t calc_len()const;
    protected:
        STR_VEC symbols_;
        friend BuffMsgArchive& operator<<(BuffMsgArchive& ar, const spnr::SubscribeMsg& d);
        friend const BuffMsgArchive& operator>>(const BuffMsgArchive& ar, spnr::SubscribeMsg& d);
    };

    class LoginMsg
    {
    public:
        LoginMsg() {};
        EMsgType msgtype()const { return EMsgType::login; }
        
        std::string user()const { return user_; }
        std::string token()const { return token_; }
        uint16_t    version()const { return version_; }
        const STR_VEC& parameters()const { return params_; };
        STR_VEC& parameters() { return params_; };

        void set_user(const std::string& s){ user_ = s; }
        void set_token(const std::string& s){ token_ = s; }
        void add_parameters(const STR_VEC& params);

        uint16_t calc_len()const;
    protected:
        uint16_t    version_ = 1;
        std::string user_;
        std::string token_;
        STR_VEC params_;
        friend BuffMsgArchive& operator<<(BuffMsgArchive& ar, const spnr::LoginMsg& d);
        friend const BuffMsgArchive& operator>>(const BuffMsgArchive& ar, spnr::LoginMsg& d);
    };

    class LoginReplyMsg
    {
    public:
        LoginReplyMsg() {};
        EMsgType msgtype()const { return EMsgType::login_reply; }

        uint16_t    reply_code()const { return reply_code_; }
        void    set_reply_code(uint16_t v) { reply_code_ = v; }
        std::string  reply_description()const { return reply_desc_; }
        void    set_reply_description(const std::string& v) { reply_desc_ = v; }


        uint16_t    version()const { return version_; }
        const STR_VEC& parameters()const { return params_; };
        STR_VEC& parameters() { return params_; };

        void add_parameters(const STR_VEC& params);

        uint16_t calc_len()const;
    protected:
        uint16_t    version_ = 0;
        uint16_t    reply_code_ = 0;
        std::string reply_desc_;
        STR_VEC     params_;

        friend BuffMsgArchive& operator<<(BuffMsgArchive& ar, const spnr::LoginReplyMsg& d);
        friend const BuffMsgArchive& operator>>(const BuffMsgArchive& ar, spnr::LoginReplyMsg& d);
    };


    enum class ECommand : uint8_t
    {
        cmd_none = 0,
        cmd_subscribe = 's',
        cmd_req_updates = 'u',
        cmd_echo_text = 't',
        cmd_send_intvec = 'n',
        cmd_send_bytevec = 'b',
        cmd_send_doublevec = 'l',
        cmd_quit = 'q'
    };

    struct StrData { uint16_t idx_; std::string val_; };

    class MenuMsg
    {
    public:
        MenuMsg();
        EMsgType msgtype()const { return EMsgType::interactive_menu; }
        const std::vector<StrData>& menu()const { return menu_; };
        std::vector<StrData>& menu() { return menu_; };

        bool add_menu_option(ECommand c, const std::string& s);
        uint16_t calc_len()const;
    protected:
        std::vector<StrData> menu_;
        friend BuffMsgArchive& operator<<(BuffMsgArchive& ar, const spnr::MenuMsg& d);
        friend const BuffMsgArchive& operator>>(const BuffMsgArchive& ar, spnr::MenuMsg& d);
    };
    
    class CommandMsg
    {
    public:
        CommandMsg() {}
        CommandMsg(ECommand c, const std::string& client_ctx);
        EMsgType msgtype()const { return EMsgType::command; }
        ECommand command()const { return cmd_; }
        void     set_command(ECommand c) { cmd_ = c; }
        const STR_VEC& parameters()const { return parameters_; };
        STR_VEC& parameters() { return parameters_; };

        const std::string&      client_context()const { return client_ctx_; };
        void                    set_client_context(const std::string& v) { client_ctx_ = v; }
        bool add_parameter(const std::string& s);
        bool add_parameters(const std::vector<std::string>& v);
        uint16_t calc_len()const;
    protected:
        ECommand    cmd_{ ECommand::cmd_none };
        std::string client_ctx_;
        STR_VEC     parameters_;
        friend BuffMsgArchive& operator<<(BuffMsgArchive& ar, const spnr::CommandMsg& d);
        friend const BuffMsgArchive& operator>>(const BuffMsgArchive& ar, spnr::CommandMsg& d);
    };

#define MAX_NUM_VEC_SIZE 200

    template<class NUM, class T, EMsgType M>
    class NumVecMsg
    {
    public:
        using NUM_VEC = std::vector<NUM>;

        EMsgType msgtype()const { return M; }
        const NUM_VEC& data()const { return data_; };
        NUM_VEC& data() { return data_; };

        const std::string& client_context()const { return client_ctx_; };
        void               set_client_context(const std::string& v) { client_ctx_ = v; }


        bool add_num(const NUM& d) { 
            if (data_.size() > MAX_NUM_VEC_SIZE) {
                std::cerr << "ERROR max number of numerical array reached " << data_.size() << std::endl;
                return false;
            }
            data_.push_back(d); 
            return true; 
        };

        uint16_t calc_len()const{
            uint16_t rv = spnr::str_archive_len(client_ctx_);
            rv += spnr::num_arr_archive_len(data_);
            return rv;
        };    
    protected:

        std::string client_ctx_;
        NUM_VEC     data_;
        template<class P> friend BuffMsgArchive& operator<<(BuffMsgArchive& ar, const P& d);
        template<class P> friend const BuffMsgArchive& operator>>(const BuffMsgArchive& ar, P& d);
    };


    class ByteVecMsg:public NumVecMsg<uint8_t, ByteVecMsg, EMsgType::bytevec>{};
    class IntVecMsg :public NumVecMsg<uint32_t, IntVecMsg, EMsgType::intvec> {};
    class DoubleVecMsg :public NumVecMsg<double, DoubleVecMsg, EMsgType::doublevec> {};
};
