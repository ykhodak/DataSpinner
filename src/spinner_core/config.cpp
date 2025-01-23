#include "config.h"
#include "spinner_core_util.h"

namespace spnr
{
    spnr::Cfg& Cfg::cfg()
    {
        static Cfg c;
        return c;
    };

    bool Cfg::is_verbose_log_level()const
    {
        return(log_level_ >= LogLevel::verbose);
    };

    bool Cfg::is_debug_log_level()const
    {
        return(log_level_ >= LogLevel::debug);
    };

    bool Cfg::is_info_log_level()const
    {
        return(log_level_ >= LogLevel::info);
    };

    bool Cfg::load_cfg(const std::string& file_name, const std::string& cfg_prefix)
    {
        if (!std::filesystem::exists(file_name)) {
            std::cerr << "ERROR: opening config file " << file_name << std::endl;
            return false;
        }

        auto& c = cfg();

        std::ifstream file(file_name);
        std::string line;
        std::string log_velel;

        while (std::getline(file, line)) {
            if (line.empty() || line[0] == '#') {
                continue;
            }

            std::istringstream iss(line);
            std::string k, v;
            std::getline(iss, k, '=');
            std::getline(iss, v);

            if (k == "host")
                c.host_ = v;
            else if (k == "port")
                c.port_ = std::stoi(v);
            else if (k == "admin_port")
                c.admin_port_ = std::stoi(v);
            else if (k == "udp_port")
                c.udp_port_ = std::stoi(v);
            else if (k == "udp_host")
                c.udp_host_ = v;
            else if (k == "svr_spin_symbols") {
                c.svr_spin_symbols = spnr::split(v, ',');
            }
            else if (k == "debug_log")
                c.debug_log_ = v;
            else if (k == "stat_log")
                c.stat_log_ = v;
            else if (k == "client_context")
                c.client_context_ = v;
            else if (k == "emulate_slow_client")
                c.emulate_slow_client_ = (std::stoi(v) > 0);
            else if (k == "idle_on_delta")
                c.idle_on_delta_ = (std::stoi(v) > 0);
            else if (k == "publisher_spin_sleep_ns")
                c.publisher_spin_sleep_ns_ = std::stoi(v);

            else if (k == "stat_check_every_n_tick")
                c.stat_check_every_n_tick_ = std::stoi(v);
            else if (k == "stat_upd_every_nsec") {
                c.stat_upd_every_nsec_ = std::stoi(v);
                if (c.stat_upd_every_nsec_ <= 0)
                    c.stat_upd_every_nsec_ = 2;
            }
            else if (k == "log_level")
            {
                log_velel = v;
                if (v == "i")
                    c.log_level_ = LogLevel::info;
                if (v == "d")
                    c.log_level_ = LogLevel::debug;

                if (v == "v")
                    c.log_level_ = LogLevel::verbose;
            }
            else
            {
                if (!cfg_prefix.empty())
                {
                    if (k == "user_" + cfg_prefix) {
                        c.client_user = v;
                    }
                    else if (k == "sub_" + cfg_prefix) {
                        c.client_sub.push_back(v);
                    }
                }
            }
        }

        std::cout << "cfg host:" << c.host_ << std::endl;
        std::cout << "cfg  port:" << c.port_ << std::endl;
        std::cout << "cfg admin_port:" << c.admin_port_ << std::endl;
        std::cout << "cfg log_level:" << log_velel << std::endl;
        if (!c.client_user.empty()) {
            std::cout << "cfg user:" << c.client_user << std::endl;
        }
        if (!c.client_sub.empty()) {
            std::cout << "cfg subs:" << *(c.client_sub.begin()) << std::endl;
        }
        if (c.emulate_slow_client_) {
            std::cout << "cfg - will emulate slow client" << std::endl;
        }
        if (c.emulate_slow_client_) {
            std::cout << "cfg - will emulate slow client" << std::endl;
        }
		if(c.idle_on_delta_){
			std::cout << "cfg - will ignore delta packets" << std::endl;
		}
        std::cout << std::endl;

        return true;
    };
}
