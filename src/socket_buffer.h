#pragma once

#include "util.h"

//// ARCHIVE_BUFF_CAPACITY - max size of one packet ///
#define ARCHIVE_BUFF_CAPACITY   2 * 1024
#define SOCKET_OUT_Q_MAX_SIZE   1000 * 1024

namespace spnr
{
#ifdef _DEBUG
    void debug_read_stream(const char* buf, size_t len, const std::string& pfix);
#endif

    class SingleRecordBuffer
    {
    public:
        bool      write(const void* data, size_t size)
			{
				size_t new_len = len_ + size;
				if (new_len > ARCHIVE_BUFF_CAPACITY)[[unlikely]]{
					spnr::errlog("Archive out-serialize error, buffer too small for data [%d] [%d]", ARCHIVE_BUFF_CAPACITY, new_len);
					return false;
				}
				memcpy(buff_ + len_, data, size);
				len_ = new_len;
				return true;				
			};
		
        bool      read(void* data, size_t size)const
			{
				size_t new_read = read_ + size;
				if (new_read > len_)[[unlikely]] {
					spnr::errlog("Archive in-serialize error, buffer len override [%d/r=%d/len=%d]", ARCHIVE_BUFF_CAPACITY, new_read, len_);
					return false;
				}

				size_t new_read_pos = new_read + start_pos_;
				if (new_read_pos > ARCHIVE_BUFF_CAPACITY )[[unlikely]] {
					spnr::errlog("Archive in-serialize error, buffer too small for data [%d/r=%d/len=%d]", ARCHIVE_BUFF_CAPACITY, new_read, len_);
					return false;
				}
				memcpy(data, buff_ + read_ + start_pos_, size);
				read_ = new_read;
				return true;
			};
		
        const char*      buff()const {return buff_ + start_pos_;}
        size_t           len()const {return len_ - start_pos_;}
        void             set_start(size_t val)const { start_pos_ = val; }
        uint32_t  read_session_index()const
			{
				constexpr size_t hdr_len = sizeof(spnr::UMsgHeader);
				uint64_t hdr_data;
				memcpy(&hdr_data, buff_, hdr_len);
				spnr::UMsgHeader h;
				h.d = spnr::ntoh64(hdr_data);
				auto sidx = h.h.session_idx;
				return sidx;
			};

    protected:
        char buff_[ARCHIVE_BUFF_CAPACITY];
        size_t len_ = 0;
        mutable size_t read_ = 0;
        mutable size_t start_pos_ = 0;

        friend class TcpConnection;
        friend class UdpConnection;
    };

    class SocketQueue
    {
    public:
        evutil_socket_t fd()const { return fd_; }
        
    protected:
        evutil_socket_t fd_ = 0;
        
    };

    class TcpSocketQueue: public SocketQueue
    {
    public:
        TcpSocketQueue();
        void set_socket_fd(evutil_socket_t fd);
        void set_name(const std::string& s);
        bool send_buffer(const SingleRecordBuffer& ob);
        uint32_t next_session_index()const { return ++session_index_; };
        bool is_throttle_on()const;

    protected:
        bool add_to_cache(const SingleRecordBuffer& buff, size_t start);
        bool send_cache();
        void clear_cache() {};
        bool empty()const { return buffers_.empty(); }
        void on_cache_send_stat(size_t cache_size);

        void start_throttle();
        void end_throttle();
        

    protected:
        
        std::string     name_;
        
        std::deque<SingleRecordBuffer> buffers_;
        mutable uint32_t session_index_{ 0 };
        time_t stat_log_time_{ 0 };
        size_t stat_send_num_{ 0 };
        size_t stat_curr_len_{0};
        size_t stat_avg_len_{ 0 };
        size_t stat_max_len_{ 0 };
        size_t stat_total_len_{ 0 };
        static constexpr size_t high_water_mark_ = SOCKET_OUT_Q_MAX_SIZE * 0.7;
        static constexpr size_t low_water_mark_ = SOCKET_OUT_Q_MAX_SIZE * 0.01;
        std::atomic<bool> throttle_on_;
    };    
};
