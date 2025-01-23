#pragma once
#include "socket_buffer.h"

namespace spnr
{   
    class BuffMsgArchive
    {
    public:
        bool operator ()()const { return is_valid_; }
        void set_invalid_archive()const { is_valid_ = false; }

        inline bool buffer_write(const void* data, size_t size) 
        {
            if (!bytebuff_.write(data, size)) {
                is_valid_ = false;
                return false;
            };
            return true;
        }

        inline bool buffer_read(void* data, size_t size)const
        {
            auto res = bytebuff_.read(data, size);
            if(!res)
            {
                is_valid_ = false;
                return false;                
            };
            return true;
        }


        inline bool store_header(const UMsgHeader& h)
        {
            uint64_t hdr_val = spnr::hton64(h.d);
            if (!buffer_write(&hdr_val, sizeof(hdr_val)))
                return false;
            uint64_t stamp = spnr::hton64(spnr::time_stamp());
            if (!buffer_write(&stamp, sizeof(stamp)))
                return false;
           
            return is_valid_;
        };

        template<class ForwardIterator>
        void store_arr(uint8_t len, ForwardIterator begin, ForwardIterator end);
        template<class ForwardIterator>
        bool load_arr(uint8_t len, ForwardIterator begin, ForwardIterator end)const;

        template<class ForwardIterator>
        void store_idxdata(uint8_t len, ForwardIterator begin, ForwardIterator end);
        template<class ForwardIterator>
        bool load_idxdata(uint8_t len, ForwardIterator begin, ForwardIterator end)const;

        template<class T, size_t N>
        void store_sor_data(uint8_t size, const SOR_Data<T, N>& );
        template<class T, size_t N>
        void load_sor_data(uint8_t size, SOR_Data<T, N>&)const;

        SingleRecordBuffer& bytebuff() {return bytebuff_;}        

    protected:        
        mutable bool is_valid_{ true };
        SingleRecordBuffer   bytebuff_;

    private:
        friend BuffMsgArchive& operator<<(BuffMsgArchive& ar, uint8_t d){ar.buffer_write(&d, sizeof(d));return ar;};
        friend const BuffMsgArchive& operator>>(const BuffMsgArchive& ar, uint8_t& d)
			{
				if(!ar.buffer_read(&d, sizeof(d)))
				{
					ar.is_valid_ = false;
					errlog("archive read error");
				};        
				return ar;
			};
        friend BuffMsgArchive& operator<<(BuffMsgArchive& ar, uint16_t d){d = htons(d);ar.buffer_write(&d, sizeof(d));return ar;};
        friend const BuffMsgArchive& operator>>(const BuffMsgArchive& ar, uint16_t& d)
			{
				if (!ar.buffer_read(&d, sizeof(d)))
				{
					ar.is_valid_ = false;
					errlog("archive read error");
				};
				d = ntohs(d);
				return ar;
			};
        friend BuffMsgArchive& operator<<(BuffMsgArchive& ar, int32_t d){d = htonl(d);ar.buffer_write(&d, sizeof(d));return ar;};
        friend const BuffMsgArchive& operator>>(const BuffMsgArchive& ar, int32_t& d)
			{
				if (!ar.buffer_read(&d, sizeof(d)))
				{
					ar.is_valid_ = false;
					errlog("archive read error");
				};
				d = ntohl(d);
				return ar;
			};
        friend BuffMsgArchive& operator<<(BuffMsgArchive& ar, uint32_t d){d = htonl(d);ar.buffer_write(&d, sizeof(d));return ar;};
        friend const BuffMsgArchive& operator>>(const BuffMsgArchive& ar, uint32_t& d)
			{
				if (!ar.buffer_read(&d, sizeof(d)))
				{
					ar.is_valid_ = false;
					errlog("archive read error");
				};
				d = ntohl(d);
				return ar;				
			};
        friend BuffMsgArchive& operator<<(BuffMsgArchive& ar, uint64_t d){d = spnr::hton64(d);ar.buffer_write(&d, sizeof(d));return ar;};
        friend const BuffMsgArchive& operator>>(const BuffMsgArchive& ar, uint64_t& d)
			{
				if (!ar.buffer_read(&d, sizeof(d)))
				{
					ar.is_valid_ = false;
					errlog("archive read error");
				};
				d = spnr::ntoh64(d);
				return ar;
			};
        friend BuffMsgArchive& operator<<(BuffMsgArchive& ar, const double& d1){const uint64_t* d = reinterpret_cast<const uint64_t*>(&d1);ar << *d;return ar;};
        friend const BuffMsgArchive& operator>>(const BuffMsgArchive& ar, double& d1){uint64_t* d = reinterpret_cast< uint64_t*>(&d1);ar >> *d;return ar;};

        template<class T>
        friend BuffMsgArchive& operator<<(BuffMsgArchive& ar, const std::vector<T>& v);
        template<class T>
        friend const BuffMsgArchive& operator>>(const BuffMsgArchive& ar, std::vector< T>& v);

        template<class K, class V>
        friend BuffMsgArchive& operator<<(BuffMsgArchive& ar, const std::unordered_map<K, V>& m);
        template<class K, class V>
        friend const BuffMsgArchive& operator>>(const BuffMsgArchive& ar, std::unordered_map<K, V>& m);

        template<size_t N>
        friend BuffMsgArchive& operator<<(BuffMsgArchive& ar, const std::bitset<N>& fields);
        template<size_t N>
        friend const BuffMsgArchive& operator>>(const BuffMsgArchive& ar, std::bitset<N>& fields);


        friend BuffMsgArchive& operator<<(BuffMsgArchive& ar, const std::string& s);
        friend const BuffMsgArchive& operator>>(const BuffMsgArchive& ar, std::string& s);
        friend BuffMsgArchive& operator<<(BuffMsgArchive& ar, const StrData& s);
        friend const BuffMsgArchive& operator>>(const BuffMsgArchive& ar, StrData& s);
    };


    template<class T>
    BuffMsgArchive& operator<<(BuffMsgArchive& ar, const std::vector<T>& v)
    {
        uint16_t len = v.size();
        ar << len;
        for (const auto& d : v)
        {
            ar << d;
        }
        return ar;
    };

    template<class T>
    const BuffMsgArchive& operator>>(const BuffMsgArchive& ar, std::vector< T>& v)
    {
        uint16_t len = 0;
        T d;
		if constexpr (std::is_arithmetic<T>::value)
		{
			d = 0;
		}		
        ar >> len;
        v.reserve(len);
        for (int i = 0; i < len; ++i)
        {
            ar >> d;
            v.push_back(d);
        }
        return ar;
    };

    template<class K, class V>
    BuffMsgArchive& operator<<(BuffMsgArchive& ar, const std::unordered_map<K, V>& m) 
    {
        uint16_t len = m.size();
        ar << len;
        for (const auto& d : m)
        {
            ar << d.first;
            ar << d.second;
        }
        return ar;
    };

    template<class K, class V>
    const BuffMsgArchive& operator>>(const BuffMsgArchive& ar, std::unordered_map<K, V>& m)
    {
        uint16_t len = 0;
        K k;
        V v;
		if constexpr (std::is_arithmetic<V>::value)
		{
			v = 0;
		}
        ar >> len;
        for (int i = 0; i < len; ++i)
        {
            ar >> k;
            ar >> v;
            m[k] = v;
        }
        return ar;
    };

 
    template<class ForwardIterator>
    void BuffMsgArchive::store_idxdata(uint8_t len, ForwardIterator begin, ForwardIterator end)
    {
        ForwardIterator i = begin;
        for (int j = 0; j < len; ++j)
        {
            if (i == end) {
                std::cerr << "ERROR. store_idxdata buffer overflow " << j << " " << std::distance(begin, end) << " len=" << (int)len << std::endl;
                return;
            }

            *this << i->idx_;
            *this << i->val_;
            ++i;
        }
    };

    template<class ForwardIterator>
    bool BuffMsgArchive::load_idxdata(uint8_t len, ForwardIterator begin, ForwardIterator end)const
    {
        ForwardIterator i = begin;
        for (int j = 0; j < len; ++j)
        {
            if (i == end) {
                std::cerr << "ERROR. load_idxdata buffer overflow " << j << " " << std::distance(begin, end) << std::endl;
                return false;
            }

            *this >> i->idx_;
            *this >> i->val_;
            ++i;
        }
        return true;
    };

    template<class T, size_t N>
    void BuffMsgArchive::store_sor_data(uint8_t len, const SOR_Data<T, N>& d)
    {
        for (int i = 0; i < len; ++i)
        {
			*this << d.idx_[i];
		}
        for (int i = 0; i < len; ++i)
        {
			*this << d.val_[i];
		}
    };

    template<class T, size_t N>
    void BuffMsgArchive::load_sor_data(uint8_t len, SOR_Data<T, N>& d)const
    {
        for (int i = 0; i < len; ++i)
        {
            *this >> d.idx_[i];
        }
        for (int i = 0; i < len; ++i)
        {
            *this >> d.val_[i];
        }
    };

    template<class ForwardIterator>
    void BuffMsgArchive::store_arr(uint8_t len, ForwardIterator begin, ForwardIterator end)
    {
        ForwardIterator i = begin;
        for (int j = 0; j < len; ++j)
        {
            if (i == end)[[unlikely]] {
                std::cerr << "ERROR. store_arr buffer overflow " << j << " " << std::distance(begin, end) << std::endl;
                return;
            }

            *this << *i;
            ++i;
        }
    };

    template<class ForwardIterator>
    bool BuffMsgArchive::load_arr(uint8_t len, ForwardIterator begin, ForwardIterator end)const
    {
        ForwardIterator i = begin;
        for (int j = 0; j < len; ++j)
        {
            if (i == end) {
                std::cerr << "ERROR. load_arr buffer overflow " << j << " " << std::distance(begin, end) << std::endl;
                return false;
            }

            *this >> *i;
            ++i;
        }
        return true;
    };

	template <class T>
    BuffMsgArchive& operator<<(BuffMsgArchive& ar, const T& d) 
    {
        ar << d.client_ctx_;
        ar << d.data_;
        return ar;
    };

	template <class T>
    const BuffMsgArchive& operator>>(const BuffMsgArchive& ar, T& d)
    {
        ar >> d.client_ctx_;
        ar >> d.data_;
        return ar;
    };
	
	
    template<size_t N>
    BuffMsgArchive& operator<<(BuffMsgArchive& ar, const std::bitset<N>& fields)
    {
        uint16_t num = fields.count();
        ar << num;
        for (uint16_t i = 0; i < N; ++i)
        {
            if (fields.test(i)) {                
                ar << i;
            }
        }
        return ar;
    };

    template<size_t N>
    const BuffMsgArchive& operator>>(const BuffMsgArchive& ar, std::bitset<N>& fields)
    {
        uint16_t num = 0;
        ar >> num;
        uint16_t idx = 0;
        for (size_t i = 0; i < num; ++i)
        {
            ar >> idx;
            fields.set(idx);
        }
        return ar;
    };

    
    inline UMsgHeader start_header(EMsgType mtype)
    {
        UMsgHeader h;
        h.h.pfix = TAG_SOH;
        h.h.mtype = static_cast<uint8_t>(mtype);
        h.h.len = WIRE_TSTAMP_LEN; // for STAMP
        h.h.session_idx = 0;
        return h;
    };
}
