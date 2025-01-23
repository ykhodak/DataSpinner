#pragma once

#include "spnr.h"
#include "md_ref.h"

namespace spnr
{
    void log(const char* format, ...);
    void errlog(const char* format, ...);
    void statlog(const char* format, ...);
    std::string format_size(double bytes);

    void trim(std::string& s);
    std::string join(const std::vector<std::string>& v, char separator = ' ');
    std::vector<std::string> split(const std::string& s, char separator = ' ');
    std::string format_with_commas(int number);
    bool ensure_dir(const std::string name);
    bool init_spinner_workspace(const std::string& cfg_file,
								const std::string& log_prefix,
								std::string cfg_prefix);

    class MessagePrinter
    {
    public:
        template<class T>
        bool publish(const T& m) {
            std::cout << m;
            return true;
        }
    };

    template<class MDELTA, class PUBLISHER>
    void make_one_delta(std::string symbol, PUBLISHER& pub)
    {
        MdDeltaBuilder<MDELTA, PUBLISHER> bld(pub);
        bld.begin(symbol);
        for (int i = 1; i <= INT_DELTA_SIZE; i++) {
            bld.add_intdata(i, i * 10);
        }
        for (int i = 1; i <= DBL_DELTA_SIZE; i++) {
            bld.add_dbldata(i, i * 100.1);
        }
        bld.end();
    };

    template<class MDELTA, class PUBLISHER>
    void make_n_int_delta(std::string symbol, PUBLISHER& pub, size_t N)
    {
        MdDeltaBuilder<MDELTA, PUBLISHER> bld(pub);
        bld.begin(symbol);
        for (size_t i = 1; i <= INT_DELTA_SIZE * N; i++) {
            bld.add_intdata(i, i * 10);
        }
        bld.end();
    }

    class TickMetric 
    {
    public:
        void        start_tick_metric();
        uint64_t    next_tick_metric();
        size_t      calc_tick_metric_speed();
        size_t      get_tick_speed()const;
        size_t      get_tick_count()const;

    protected:
        size_t                  count_ = 0;             ///was atomic
        time_t                  start_time_ = 0;
        uint64_t                curr_count_ = 0;
        size_t                  updates_per_sec_ = 0;   ///was atomic
    };

    class MetricsObserver 
    {
    public:
        MetricsObserver(size_t check_every_n_tick, size_t recalc_every_n_sec);
        size_t  next_tick_metric(std::function<void(void)> on_recalc = nullptr);
        void    reset_tick_observer(const std::string& name);
        size_t  next_tick_publisher_metrics(uint32_t session_index);
    protected:
        TickMetric  mtick_;
        size_t      check_every_n_tick_;
        size_t      recalc_every_n_sec_;
        time_t      check_time_ = 0;
        std::string observer_name_;
    };

    template<class T>
    class HB_Observer
    {
    public:
        HB_Observer(T* watched, size_t check_every_n_tick, size_t hb_timeout_sec);
        size_t next_hb_check(uint64_t tick_num);
    protected:
        T*          watched_ = nullptr;
        size_t      hb_obs_check_every_n_tick_ = 0;
        size_t      hb_obs_timeout_sec_ = 0;
    };

    class HB_SubscriberRunner 
    {
    public:
        HB_SubscriberRunner(size_t hb_period);

        bool is_subscriber_stopped()const;
        void stop_subscriber();
        void wait_subscriber_for_hb_time();
        void set_subscriber_hb_period(size_t sec);        

    protected:
        std::atomic<bool>       subscriber_stopped_;
        std::mutex              subscriber_mtx_hb_;
        std::condition_variable subscriber_cv_hb_;
        size_t                  subscriber_hb_period_sec_ = 10;
    };

    template<class T>
    HB_Observer<T>::HB_Observer(T* watched, size_t check_every_n_tick, size_t hb_timeout_sec)
        :watched_(watched), hb_obs_check_every_n_tick_(check_every_n_tick), hb_obs_timeout_sec_(hb_timeout_sec)
    {

    };

    template<class T>
    size_t HB_Observer<T>::next_hb_check(uint64_t tick_num)
    {
        if (!hb_obs_check_every_n_tick_ || !(tick_num % hb_obs_check_every_n_tick_))
        {
            auto hb_delta = static_cast<size_t>(time(nullptr) - watched_->last_hb_time());
            if (hb_delta > hb_obs_timeout_sec_) {
                return hb_delta;
            }            
        }
        return 0;
    };

    class WireReaderStat
    {
        struct throughput
        {
            uint32_t thr_bytes = 0;
            uint32_t thr_packets = 0;
        };

    public:
        void start_reader_stat();
        void on_reader_progress(uint16_t num, EMsgType m, uint16_t wire_latency, uint32_t session_idx);
        uint16_t last_msg_wire_latency()const { return last_msg_wire_latency_; }
        uint16_t last_msg_len()const { return last_msg_wire_len_; }
        throughput calc_reader_throughput()const;
		
    protected:
        size_t stat_total_progress_{ 0 };
        time_t stat_start_;
        time_t stat_log_time_{ 0 };
        size_t total_packets_progress_{ 0 };
        uint16_t last_msg_wire_len_{ 0 };
        uint16_t last_msg_wire_latency_{ 0 };
        uint16_t avg_msg_wire_latency_{ 0 };
        uint64_t total_msg_wire_latency_{ 0 };
        uint16_t max_msg_wire_latency_{ 0 };
        uint64_t base_latency_ = 0;
    };
}
