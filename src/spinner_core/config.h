#pragma once
#include "spnr.h"

namespace spnr
{
    enum class LogLevel
    {
        laconic,
        info,
        debug,
        verbose
    };


    struct Cfg
    {
        static Cfg& cfg();
        static bool load_cfg(const std::string& file_name, const std::string& cfg_prefix);

        bool is_verbose_log_level()const;
        bool is_debug_log_level()const;
        bool is_info_log_level()const;

        std::string host_, udp_host_;
        int         port_, admin_port_, udp_port_;
        bool        emulate_slow_client_{ false };
        std::string debug_log_;
        std::string stat_log_;
        std::string client_context_;
        std::string client_user;
        std::vector<std::string> client_sub;
        std::vector <std::string> svr_spin_symbols;
        LogLevel    log_level_;
        size_t      stat_upd_every_nsec_{ 2 };
        size_t      stat_check_every_n_tick_ = 10000;
        size_t      publisher_spin_sleep_ns_{1};
        size_t      hb_period_sec_{ 10 };
        size_t      hb_timeout_sec_{ 30 };
		bool        idle_on_delta_{false};// if true we don't process ticks, just read from socket

        inline static std::string dbg_log_file_name_ = "dbglog.txt";
        inline static std::string stat_log_file_name_ = "statlog.txt";
    };

}
