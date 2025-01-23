#include <cstdarg>
#include "spinner_core_util.h"
#include "config.h"

void spnr::trim(std::string& s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
        return !std::isspace(ch);
        }));

    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
        }).base(), s.end());
}

std::string spnr::join(const std::vector<std::string>& v, char separator) {
    return std::accumulate(std::begin(v) + 1, std::end(v), v[0],
        [&separator](std::string s0, std::string const& s1) { return s0 += separator + s1; });
};

std::vector<std::string> spnr::split(const std::string& s, char separator) 
{
    std::vector<std::string> rv;
    std::istringstream f(s);
    std::string s1;
    while (std::getline(f, s1, separator)) {
        rv.push_back(s1);
    }
    return rv;
};

std::string spnr::format_with_commas(int number) 
{
    std::string rv = std::to_string(number);
    int insertPosition = rv.length() - 3;

    while (insertPosition > 0) {
        rv.insert(insertPosition, ",");
        insertPosition -= 3;
    }
    return rv;
};

bool spnr::ensure_dir(const std::string name)
{
    if (!std::filesystem::exists(name)) {
        if (!std::filesystem::create_directory(name)) {
            std::cerr << "Failed to create directory: " << name << std::endl;
            return false;
        }
    }
    return true;
}

bool spnr::init_spinner_workspace(const std::string& cfg_file,
								const std::string& log_prefix,
								std::string cfg_prefix)
{
    if (!spnr::ensure_dir("logs"))
        return false;
    if (!spnr::Cfg::load_cfg(cfg_file, cfg_prefix))
        return false;
    auto& c = spnr::Cfg::cfg();
    c.debug_log_ = log_prefix + "-" + c.debug_log_;
    c.stat_log_ = log_prefix + "-" + c.stat_log_;
    spnr::Cfg::dbg_log_file_name_ = c.debug_log_;
    spnr::Cfg::stat_log_file_name_ = c.stat_log_;
    spnr::log("log started [%s][%s]", c.debug_log_.c_str(), c.stat_log_.c_str());


	
    return true;
};

std::ofstream& dbglog()
{
    static std::ofstream dlog("logs/" + spnr::Cfg::dbg_log_file_name_, std::ios_base::out | std::ios_base::app);
    return dlog;

}

std::ofstream& stlog()
{
    static std::ofstream slog("logs/" + spnr::Cfg::stat_log_file_name_, std::ios_base::out | std::ios_base::app);
    return slog;
};

std::string log_timestamp()
{
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    std::tm* local_time = std::localtime(&now_time);
    auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() % 1000;

    char buff[32];
    sprintf(buff, "%d:%d:%d.%zu", local_time->tm_hour, local_time->tm_min, local_time->tm_sec, milliseconds);
    std::string rv = buff;
    return rv;
}

void spnr::log(const char* format, ...)
{
    char buff[256];
    memset(buff, 0, sizeof buff);
    va_list arg;
    va_start(arg, format);
	vsprintf(buff, format, arg);
    va_end(arg);
    std::cout << buff << std::endl;
    dbglog() << "[" << log_timestamp() << "]" << buff << std::endl;
    dbglog().flush();
}

void spnr::errlog(const char* format, ...)
{
    char buff[256];
    memset(buff, 0, sizeof buff);

    va_list arg;
    va_start(arg, format);
	vsprintf(buff, format, arg);
    va_end(arg);
    std::cout << buff << std::endl;
    dbglog() << "[" << log_timestamp() << "]" << "ERR: " << buff << std::endl;
    dbglog().flush();
};

void spnr::statlog(const char* format, ...)
{
    char buff[256];
    memset(buff, 0, sizeof buff);

    va_list arg;
    va_start(arg, format);
	vsprintf(buff, format, arg);
    va_end(arg);
    time_t now = time(nullptr);
    stlog() << "[" << std::put_time(localtime(&now), "%T") << "] " << buff << std::endl;
    stlog().flush();
};

std::string spnr::format_size(double bytes) {
    const char* units[] = { "", "K", "M", "G", "T" };
    int unitIndex = 0;

    while (bytes >= 1024 && unitIndex < 4) {
        bytes /= 1024;
        unitIndex++;
    }

    std::ostringstream stm;
    stm << std::fixed << std::setprecision(2) << bytes;

    return stm.str() + " " + units[unitIndex];
}



//////// TickMetric //////////
void spnr::TickMetric::start_tick_metric()
{
    start_time_ = time(nullptr);
};

uint64_t spnr::TickMetric::next_tick_metric()
{
    return ++curr_count_;
};

size_t  spnr::TickMetric::calc_tick_metric_speed()
{
    auto total_duration = time(nullptr) - start_time_;
    if (total_duration > 0) {
        size_t upd_psec = (size_t)((double)curr_count_ / total_duration);
        updates_per_sec_ = upd_psec;
        count_ = curr_count_;
        return upd_psec;
    }
    return 0;
};

size_t spnr::TickMetric::get_tick_count()const 
{
    return count_;
};

size_t spnr::TickMetric::get_tick_speed()const
{
    return updates_per_sec_;
};

spnr::MetricsObserver::MetricsObserver(size_t check_every_n_tick, size_t recalc_every_n_sec)
    :check_every_n_tick_(check_every_n_tick), recalc_every_n_sec_(recalc_every_n_sec)
{

};

void spnr::MetricsObserver::reset_tick_observer(const std::string& name)
{
    observer_name_ = name;
    mtick_.start_tick_metric();
    check_time_ = time(nullptr);
};

size_t spnr::MetricsObserver::next_tick_metric(std::function<void(void)> on_recalc)
{
    auto updates_generated = mtick_.next_tick_metric();
    if (!check_every_n_tick_ || !(updates_generated % check_every_n_tick_))
    {
        auto now = time(nullptr);
        size_t tdelta = static_cast<size_t>(now - check_time_);
        if (tdelta > recalc_every_n_sec_)
        {
            mtick_.calc_tick_metric_speed();
            if (on_recalc) {
                on_recalc();
            }
            check_time_ = now;
        }
    }
    return updates_generated;
};

size_t spnr::MetricsObserver::next_tick_publisher_metrics(uint32_t session_index)
{
    return next_tick_metric([this, session_index]()
        {
            auto updates_per_sec = mtick_.get_tick_speed();
            auto updates_generated = mtick_.get_tick_count();
            spnr::log("tick[%s]: %st @ %st/s [%s]", 
                observer_name_.c_str(), 
                spnr::format_size(updates_generated).c_str(), 
                spnr::format_size(updates_per_sec).c_str(), 
                format_with_commas(session_index).c_str());
        });
};





spnr::HB_SubscriberRunner::HB_SubscriberRunner(size_t hb_period) :subscriber_hb_period_sec_(hb_period)
{

};

bool spnr::HB_SubscriberRunner::is_subscriber_stopped()const 
{
    return subscriber_stopped_.load(); 
};

void spnr::HB_SubscriberRunner::stop_subscriber() 
{ 
    subscriber_stopped_.store(true); 
    subscriber_cv_hb_.notify_one();
};

void spnr::HB_SubscriberRunner::wait_subscriber_for_hb_time()
{
    std::unique_lock lk(subscriber_mtx_hb_);
    subscriber_cv_hb_.wait_for(lk, std::chrono::seconds(subscriber_hb_period_sec_), [this]()
        {
            return is_subscriber_stopped();
        });
};

void spnr::HB_SubscriberRunner::set_subscriber_hb_period(size_t sec)
{
    subscriber_hb_period_sec_ = sec;
};

void spnr::WireReaderStat::start_reader_stat()
{
    stat_start_ = time(nullptr);
    stat_total_progress_ = 0;
};

void spnr::WireReaderStat::on_reader_progress(uint16_t num, EMsgType mt, uint16_t wire_latency, uint32_t session_idx)
{
    static const auto& cfg = Cfg::cfg();

    if (mt == EMsgType::login_reply) {
        base_latency_ = wire_latency;
        spnr::log("clock sync - setting base_latency to [%d]", base_latency_);
    }
    else
    {
        if (wire_latency < base_latency_) {
            base_latency_ = wire_latency;
            spnr::log("clock sync - resetting base_latency to [%d]", base_latency_);
        }
        else {
            wire_latency -= base_latency_;
        }
    }


    last_msg_wire_len_ = num;
    last_msg_wire_latency_ = wire_latency;
    total_msg_wire_latency_ += wire_latency;
    if (wire_latency > max_msg_wire_latency_) {
        max_msg_wire_latency_ = wire_latency;
    }
    ++total_packets_progress_;
    stat_total_progress_ += num;
    time_t t = time(nullptr);
    auto tdelta = t - stat_log_time_;
    if (tdelta > static_cast<long int>(cfg.stat_upd_every_nsec_))
    {
        if (total_packets_progress_ > 0) {
            avg_msg_wire_latency_ = total_msg_wire_latency_ / total_packets_progress_;
        }
        auto thr = calc_reader_throughput();
        statlog("[L] %sB/s [%st] [%st/s] usecACM[%d/%d/%d] [%s]",
            format_size(thr.thr_bytes).c_str(),
            format_size(total_packets_progress_).c_str(),
            format_size(thr.thr_packets).c_str(),
            avg_msg_wire_latency_, last_msg_wire_latency_, max_msg_wire_latency_, format_with_commas(session_idx).c_str());

        stat_log_time_ = t;
    }
};

spnr::WireReaderStat::throughput spnr::WireReaderStat::calc_reader_throughput()const
{
    spnr::WireReaderStat::throughput rv;
    auto end = time(nullptr);
    auto duration = (end - stat_start_);
    if (duration > 0)
    {
        rv.thr_bytes = (uint32_t)((double)stat_total_progress_ / duration);
        rv.thr_packets = (uint32_t)((double)total_packets_progress_ / duration);
    }
    return rv;
}
