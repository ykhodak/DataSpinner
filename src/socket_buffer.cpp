#include <iomanip>
#include <filesystem>
#include "spnr.h"
#include "socket_buffer.h"
#include "archive.h"

///// TcpSocketQueue ////////
spnr::TcpSocketQueue::TcpSocketQueue() 
{
    throttle_on_ = false;
};

void spnr::TcpSocketQueue::set_socket_fd(evutil_socket_t fd)
{
    fd_ = fd;
};

void spnr::TcpSocketQueue::set_name(const std::string& s) 
{
    name_ = s;
};

bool spnr::TcpSocketQueue::send_buffer(const SingleRecordBuffer& ob) 
{    
    const auto& cfg = spnr::Cfg::cfg();

    if (!empty()) {
        auto cache_size = buffers_.size();
        on_cache_send_stat(cache_size);
        if (!send_cache()) {
            return false;
        }        
        if (empty()) {
            
            clear_cache();
			if (cfg.is_verbose_log_level())
            {
				spnr::log("reset empty cache");
			}
        }
        else 
        {
            if (!add_to_cache(ob, 0)) {
                return false;
            };

            return true;
        }
    }

    auto res = send(fd_, ob.buff(), ob.len(), 0);

    if (res == 0) {
        auto e = errno;
        errlog("archive write error=0, socket closed by remote peer [%d] %s:%d", e, __FILE__, __LINE__);
        return false;
    }
    else if (res == -1) {
        auto would_block = spnr::is_would_block_socket_error();
        if (would_block) {
            if (cfg.is_verbose_log_level())
            {
                errlog("send_buffer would_block");
            }
            if (!add_to_cache(ob, 0)) {
                return false;
            };
            return true;
        }
        else {
            spnr::errlog("archive write error=-1 [%s] %s:%d", spnr::get_socket_error_text(), __FILE__, __LINE__);
            return false;
        }
    }
    else if (res < static_cast<long int>(ob.len())) {
        if (!add_to_cache(ob, res)) {
			if (cfg.is_verbose_log_level())
            {
				spnr::log("partial write [%d] [idx=%d]", res, ob.read_session_index());
			}
            return false;
        }
    }
    //else {
    //    spnr::log("reg-write [%d] [idx=%d]", res, ob.read_session_index());
    //}

    //debug_read_stream(ob.buff(), ob.len(), "ob");

    return true;
};

void spnr::TcpSocketQueue::start_throttle() 
{
    throttle_on_.store(true);
};

void spnr::TcpSocketQueue::end_throttle()
{
    throttle_on_.store(false);
};

bool spnr::TcpSocketQueue::is_throttle_on()const
{
    return throttle_on_.load();
};


bool spnr::TcpSocketQueue::add_to_cache(const SingleRecordBuffer& buff, size_t start) 
{
    size_t qsize = buffers_.size();
    if (is_throttle_on())
    {        
        if (qsize < low_water_mark_) {
            spnr::log("end throttle at [%d]", qsize);
            end_throttle();
        }
        else {
            spnr::log("throttling at [%d]", qsize);
            return true;
        }
    }
    else 
    {
        if (qsize > high_water_mark_) {
            spnr::log("start throttle at [%d]", qsize);
            start_throttle();
            return true;
        }
    }

    buff.set_start(start);
    buffers_.push_back(buff);
    if (qsize >= SOCKET_OUT_Q_MAX_SIZE) {
        spnr::errlog("Socket queue overflow, client is too slow [%d]", buffers_.size());
        return false;
    }
    //spnr::log("add-cache [%d] [%d/%d] [idx=%d]", buffers_.size(), buff.len(), start, buff.read_session_index());
    return true;
};

bool spnr::TcpSocketQueue::send_cache()
{
    const auto& cfg = spnr::Cfg::cfg();

    while (!empty()) {
        const SingleRecordBuffer& b = buffers_.front();
        auto res = send(fd_, b.buff(), b.len(), 0);
        if (res == 0) {
            auto e = errno;
            errlog("cache line write error=0 [%d] %s:%d", e, __FILE__, __LINE__);
            return false;
        }
        else if (res == -1) {
            auto would_block = spnr::is_would_block_socket_error();
            if (would_block) {
                if (cfg.is_verbose_log_level())
                {
                    errlog("cache would_block");
                }
                return true;
            }
            else {
                spnr::errlog("archive write error=-1 [%s] %s:%d", spnr::get_socket_error_text(), __FILE__, __LINE__);
                return false;
            }
        }

        if (res == static_cast<long int>(b.len())) {
            auto idx = b.read_session_index();
            buffers_.pop_front();
            if (cfg.is_verbose_log_level())
            {
                spnr::log("unwinding cache [%d] [idx=%d]", buffers_.size(), idx);
            }
            if (is_throttle_on())
            {
                size_t qsize = buffers_.size();
                if (qsize < low_water_mark_) {
                    spnr::log("end throttle at [%d]", qsize);
                    end_throttle();
                }
            }
        }
        else {
            b.set_start(res);
			if (cfg.is_verbose_log_level())
            {
				spnr::log("partial sent [%d-%d] [%d]", res, b.len(), buffers_.size());
			}
        }
    }
    return true;
}

void spnr::TcpSocketQueue::on_cache_send_stat(size_t cache_size)
{
    const auto& cfg = Cfg::cfg();

    ++stat_send_num_;
    stat_curr_len_ = cache_size;
    stat_total_len_ += cache_size;
    if (cache_size > stat_max_len_) {
        stat_max_len_ = cache_size;
    }

    time_t t = time(nullptr);
    auto tdelta = t - stat_log_time_;
    if (tdelta > static_cast<long int>(cfg.stat_upd_every_nsec_))
    {
        stat_avg_len_ = stat_total_len_ / stat_send_num_;
        statlog("[C] cacheACM[%d/%d/%d]", stat_avg_len_, stat_curr_len_, stat_max_len_);
        stat_log_time_ = t;
    }
};

#ifdef _DEBUG
void spnr::debug_read_stream(const char* buf, size_t len, const std::string& pfix)
{
    constexpr size_t hdr_len = sizeof(spnr::UMsgHeader);
    while (len > 0) {
        uint64_t hdr_data;
        memcpy(&hdr_data, buf, hdr_len);
        spnr::UMsgHeader h;
        h.d = spnr::ntoh64(hdr_data);

        if (h.h.pfix != TAG_SOH) {
            spnr::errlog("corrupted stream, expected SOH [%d]", h.h.pfix);
            return;
        };

        uint32_t l2 = h.h.len + hdr_len;
        if (len < l2) {
            return;/* finished */
        }
        auto sidx = h.h.session_idx;
        auto mtype = h.h.mtype;
        spnr::log("pkt-%s: %d/%d %d", pfix.c_str(), sidx, mtype, l2);

        buf += l2;
        len -= l2;
    }
    //len -= 

};
#endif
