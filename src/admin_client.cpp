#include "spnr.h"
#include "records.h"
#include "md_ref.h"
#include "tcp_connection.h"

namespace spnr {
    class AdminClientConnection : public spnr::TcpConnection
    {
    public:

        AdminClientConnection(spnr::TcpConnectionFactory* r, bufferevent* b) :spnr::TcpConnection(r, b) {}

        void on_connected()override
        {
            static auto& cfg = Cfg::cfg();
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
            if (m.reply_code() == 0) {
                spnr::errlog("login failed [%d] [%s]", m.reply_code(), m.reply_description().c_str());
                close_connection();
                return;
            }
            spnr::log("login accepted [%d]", m.reply_code());
            stat_.start_reader_stat();
        };



        void on_text_msg(const spnr::TextMsg& m)override
        {
            spnr::log("on-text [%s] wire-latency: %d [%d]", m.text().c_str(), stat_.last_msg_wire_latency(), stat_.last_msg_len());
        };

        void on_menu_msg(const spnr::MenuMsg& m)override
        {
            std::cout << "--------------------------" << std::endl;
            const auto& mlist = m.menu();
            for (const auto& s : mlist)
            {
                std::cout << (char)s.idx_ << " - " << s.val_ << std::endl;
            }
            spnr::ECommand cmd = spnr::ECommand::cmd_none;
            std::vector<std::string> params;
            while (cmd == spnr::ECommand::cmd_none)
            {
                params.clear();
                std::cout << ">";
                std::string val;
                std::getline(std::cin, val);

                std::stringstream ss(val);
                std::string word;


                if (std::getline(ss, word, ' '))
                {
                    cmd = spnr::parse_command(word);
                    if (cmd != spnr::ECommand::cmd_none)
                    {
                        while (std::getline(ss, word, ' ')) {
                            params.push_back(word);
                        }
                    }
                }
            }//while cmd

            if (cmd == spnr::ECommand::cmd_quit)
            {
                close_connection();
            }
            else if (cmd == spnr::ECommand::cmd_subscribe)
            {
                spnr::SubscribeMsg sub;
                sub.add_symbols(params);
                send_message(sub);
            }
            else
            {
                static auto& cfg = Cfg::cfg();
                spnr::CommandMsg cmsg(cmd, cfg.client_context_);
                cmsg.add_parameters(params);
                send_message(cmsg);
            }
        }

        void on_md_reference_msg(const spnr::MdRef& m)
        {
            static auto& cfg = Cfg::cfg();
            if (cfg.is_info_log_level()) {
                std::cout << m;
            }
            spnr::log("md[%s] wire-latency: %d [%d]", m.symbol().c_str(), stat_.last_msg_wire_latency(), stat_.last_msg_len());
        };

        void on_md_delta_msg(const spnr::MdDelta& m)
        {
            static auto& cfg = Cfg::cfg();
            if (cfg.is_info_log_level()) {
                std::cout << m;
            }
            spnr::log("mdd[%s] wire-latency: %d [%d]", m.symbol().c_str(), stat_.last_msg_wire_latency(), stat_.last_msg_len());
        };

        void on_intvec_msg(const spnr::IntVecMsg& m)override
        {
            static auto& cfg = Cfg::cfg();
            if (cfg.is_info_log_level())
            {
                const auto& data = m.data();
                spnr::log("intvec: [%d] [%s]", data.size(), m.client_context().c_str());
                for (const auto& d : data) {
                    spnr::log("%d", d);
                }
            }
            spnr::log("ivec wire-latency: %d [%d]", stat_.last_msg_wire_latency(), stat_.last_msg_len());
        };

        void on_bytevec_msg(const spnr::ByteVecMsg& m)override
        {
            static auto& cfg = Cfg::cfg();
            if (cfg.is_info_log_level())
            {
                const auto& data = m.data();
                spnr::log("bytevec: [%d] [%s]", data.size(), m.client_context().c_str());
                for (const auto& d : data) {
                    spnr::log("%d", d);
                }
            }
            spnr::log("bvec wire-latency: %d [%d]", stat_.last_msg_wire_latency(), stat_.last_msg_len());
        };

        void on_doublevec_msg(const spnr::DoubleVecMsg& m)
        {
            static auto& cfg = Cfg::cfg();
            if (cfg.is_info_log_level())
            {
                const auto& data = m.data();
                spnr::log("doublevec: [%d] [%s]", data.size(), m.client_context().c_str());
                for (const auto& d : data) {
                    spnr::log("%.4lf", d);
                }
            }
            spnr::log("lvec wire-latency: %d [%d]", stat_.last_msg_wire_latency(), stat_.last_msg_len());
        };
    };
}

int main(int argc, char** argv)
{
    if (!spnr::init_workspace(argc, argv, "tcp-admin-cl"))
        return 1;
    spnr::SingleTcpConnectionFactory<spnr::AdminClientConnection> reg;
    return spnr::run_tcp_connector(spnr::Cfg::cfg().host_, spnr::Cfg::cfg().admin_port_, &reg);
}
