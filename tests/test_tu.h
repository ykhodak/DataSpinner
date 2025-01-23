#pragma once

#include "spnr.h"
#include "md_ref.h"
#include "simulator.h"

namespace spnr
{
#define TEST_ARR_SIZE 16
#define TEST_STR_ARR_SIZE 4

    template<class T>
    struct S_Data 
    { 
        S_Data()
        {
            if constexpr (std::is_arithmetic<T>::value) {
                val_ = 0;
            }
        }

        S_Data(uint16_t idx, const T& v) :idx_(idx), val_(v) 
        {

        }

        bool operator ==(const S_Data& other)const 
        {
            return other.idx_ == idx_ && other.val_ == val_;
        }

        uint16_t idx_ = 0;
        T val_;
    };

    
    template<class T>
    std::ostream& operator << (std::ostream& os, const SOR_Data<T, TEST_ARR_SIZE>& d)
    {
        for (size_t i = 0; i < TEST_ARR_SIZE; ++i)
        {
            os << "[" << d.idx_[i] << "->" << d.val_[i] << "]";
        }
        return os;
    };

    template<class T, size_t N = TEST_ARR_SIZE>
    std::array<T, N> make_arr()
    {
        std::array<T, N> rv;
        std::iota(rv.begin(), rv.end(), 1);
        return rv;
    }

    template<class T, size_t N = TEST_ARR_SIZE>
    std::array<S_Data<T>, N> make_aos(int start_idx = 1, double dx = 1.0)
    {        
        std::array<S_Data<T>, N> aos;
        std::for_each(aos.begin(), aos.end(), [&start_idx, &dx](S_Data<T>& d)
            {
                d.idx_ = start_idx;
                if constexpr (std::is_same_v<T, std::string>) {                    
                    d.val_ = std::to_string(start_idx + 10 * dx);
                }
                else {
                    d.val_ = start_idx * (10 * dx);
                }
                start_idx = start_idx + 1;
            });
        return aos;
    }

    template<class T, size_t N = TEST_ARR_SIZE>
    SOR_Data<T, N> make_soa(int start_idx = 1, double dx = 1)
    {
        SOR_Data<T, N> soa;
        for (size_t i = 0; i < TEST_ARR_SIZE; ++i)
        {
            soa.idx_[i] = start_idx;
            if constexpr (std::is_same_v<T, std::string>) {
                soa.val_[i] = std::to_string(start_idx + 10 * dx);
            }
            else {
                soa.val_[i] = start_idx * (10 * dx);
            }
            start_idx = start_idx + 1;
        }
        return soa;
    }


    template<class T, size_t N = TEST_ARR_SIZE>
    S_Data<T> sum_aos(std::array<S_Data<T>, N>& aos)
    {
        S_Data<T> res;
        return std::accumulate(aos.begin(), aos.end(), res, [](const S_Data<T>& r, const S_Data<T>& v)
            {
                S_Data<T> res(r);
                res.idx_ += v.idx_;
                if constexpr (std::is_same_v<T, std::string>) {
                    if (!v.val_.empty()) {
                        res.val_ += v.val_[0];
                    }
                }
                else {
                    res.val_ += v.val_;
                }
                return res;
            });
    }

    template<class T, size_t N = TEST_ARR_SIZE>
    S_Data<T> sum_soa(SOR_Data<T, N>& soa)
    {
        S_Data<T> res;
        for (size_t i = 0; i < TEST_ARR_SIZE; ++i)
        {
            res.idx_ += soa.idx_[i];
            if constexpr (std::is_same_v<T, std::string>) {
                if (!soa.val_[i].empty()) {
                    res.val_ += soa.val_[i][0];
                }
            }
            else {
                res.val_ += soa.val_[i];
            }
        }
        return res;
    }    

    struct Data_Summary 
    {
        S_Data<int32_t>         int_summary;
        S_Data<double>          dbl_summary;
        Data_Summary();
        bool operator == (const Data_Summary& other)const;
    };

    struct AOS_Delta 
    {
        std::string                                         symbol_ = "XYZ";
        UDeltaSize                                          size_;
        std::array<S_Data<int32_t>, TEST_ARR_SIZE>          intdata_;
        std::array<S_Data<double>, TEST_ARR_SIZE>           dbldata_;
        std::array<uint64_t, TEST_ARR_SIZE>                 stamps_;

        static AOS_Delta make_delta(int start_idx = 1);
        Data_Summary make_summary();
        uint16_t calc_len()const;
        EMsgType msgtype()const { return EMsgType::md_delta; }
        friend BuffMsgArchive& operator<<(BuffMsgArchive& ar, const spnr::AOS_Delta& d);
    };    

    struct SOA_Delta
    {
        std::string                                        symbol_ = "XYZ";
        UDeltaSize                                         size_;
        SOR_Data<int32_t, TEST_ARR_SIZE>                   intdata_;
        SOR_Data<double, TEST_ARR_SIZE>                    dbldata_;
        std::array<uint64_t, TEST_ARR_SIZE>                stamps_;

        static SOA_Delta make_delta(int start_idx = 1);
        Data_Summary make_summary();
        uint16_t calc_len()const;
        EMsgType msgtype()const { return EMsgType::md_delta; }
        friend BuffMsgArchive& operator<<(BuffMsgArchive& ar, const spnr::SOA_Delta& d);
    };

    union UField4decDouble
    {
        uint64_t value;
        struct
        {
            unsigned int index : 10;
            unsigned int sign : 1;
            int64_t int_part : 39;
            unsigned int dec_part : 14;
        } bits;
    };

    union UFieldInt
    {
        uint64_t value;
        struct
        {
            unsigned int index : 10;
            int64_t int_part : 54;            
        } bits;
    };


    UField4decDouble dblf_to_uni(uint16_t idx, const double& d);
    UFieldInt intf_to_uni(uint16_t idx, const int64_t& d);
    inline double uni_to_dbl(const UField4decDouble& v)
    {
        double rv = (double)(v.bits.int_part) + (double)(v.bits.dec_part / 10000.0);
        if (v.bits.sign) {
            rv *= -rv;
        }
        return rv;
    }
    struct UNI_Delta
    {
        std::string                                         symbol_ = "XYZ";
        UDeltaSize                                          size_;
        std::array<UFieldInt, TEST_ARR_SIZE>                intdata_;
        std::array<UField4decDouble, TEST_ARR_SIZE>         dbldata_;
        std::array<uint64_t, TEST_ARR_SIZE>                 stamps_;

        static UNI_Delta make_delta(int start_idx = 1);
        Data_Summary make_summary();
        uint16_t calc_len()const;
        EMsgType msgtype()const { return EMsgType::md_delta; }
        friend BuffMsgArchive& operator<<(BuffMsgArchive& ar, const spnr::SOA_Delta& d);
    };

    template<class DELTA>
    std::vector<DELTA> make_delta_arr(size_t num)
    {
        int index = 1;
        std::vector<DELTA> res;
        res.resize(num);
        std::generate(res.begin(), res.end(), [&index]()
            {
                return DELTA::make_delta(index);
                ++index;
            });
        return res;
    };


    struct HashSnapshot 
    {
        INT_HMAP            intdata_;
        DBL_HMAP            dbldata_;
        STR_HMAP            strdata_;
    };

//#define USE_VECTOR_AS_SNAPSHOT

    template <size_t SIZE=1100>
    struct ArrSnapshot
    {
        ArrSnapshot() 
        {
#ifdef USE_VECTOR_AS_SNAPSHOT
            intdata_.resize(SIZE);
            dbldata_.resize(SIZE);
            strdata_.resize(SIZE);
#endif
        }

        /// BM_AOS_arr_merge_1000       44643 ns
        /// BM_SOA_arr_merge_1000       54688 ns
        std::array<int32_t, SIZE>       intdata_;
        std::array<double, SIZE>        dbldata_;
        std::array<std::string, SIZE>   strdata_;        

#ifdef USE_VECTOR_AS_SNAPSHOT
        /// BM_AOS_arr_merge_1000       60938 ns
        /// BM_SOA_arr_merge_1000       62779 ns
        std::vector<int32_t>            intdata_;
        std::vector<double>             dbldata_;
        std::vector<std::string>        strdata_;
#endif

        std::bitset<SIZE>               int_fields_;
        std::bitset<SIZE>               dbl_fields_;
        std::bitset<SIZE>               str_fields_;
    };


    struct AOS_SymbolHashSnapshot: public HashSnapshot
    {
        void merge(const AOS_Delta& d)
        {
            for (int i = 0; i < d.size_.sz.intdata_len; ++i) {
                auto it = intdata_.find(d.intdata_[i].idx_);
                if (it == intdata_.end()) {
                    intdata_[d.intdata_[i].idx_] = d.intdata_[i].val_;
                }
                else {
                    it->second = d.intdata_[i].val_;
                }
            }

            for (int i = 0; i < d.size_.sz.dbldata_len; ++i) {
                auto it = dbldata_.find(d.dbldata_[i].idx_);
                if (it == dbldata_.end()) {
                    dbldata_[d.dbldata_[i].idx_] = d.dbldata_[i].val_;
                }
                else {
                    it->second = d.dbldata_[i].val_;
                }
            }
        };
    };

    struct SOA_SymbolHashSnapshot : public HashSnapshot
    {
        void merge(const SOA_Delta& d)
        {
            for (int i = 0; i < d.size_.sz.intdata_len; ++i) {
                auto it = intdata_.find(d.intdata_.idx_[i]);
                if (it == intdata_.end()) {
                    intdata_[d.intdata_.idx_[i]] = d.intdata_.val_[i];
                }
                else {
                    it->second = d.intdata_.val_[i];
                }
            }

            for (int i = 0; i < d.size_.sz.dbldata_len; ++i) {
                auto it = dbldata_.find(d.dbldata_.idx_[i]);
                if (it == dbldata_.end()) {
                    dbldata_[d.dbldata_.idx_[i]] = d.dbldata_.val_[i];
                }
                else {
                    it->second = d.dbldata_.val_[i];
                }
            }
        }
    };

    struct AOS_SymbolArrSnapshot : public ArrSnapshot<>
    {
        void merge(const AOS_Delta& d)
        {
            for (int i = 0; i < d.size_.sz.intdata_len; ++i) {
                auto idx = d.intdata_[i].idx_;
                intdata_[idx] = d.intdata_[i].val_;
                int_fields_.set(idx);
            }
            
            for (int i = 0; i < d.size_.sz.dbldata_len; ++i) {
                auto idx = d.dbldata_[i].idx_;
                dbldata_[idx] = d.dbldata_[i].val_;
                dbl_fields_.set(idx);
            }
        }
    };

    struct SOA_SymbolArrSnapshot : public ArrSnapshot<>
    {
        void merge(const SOA_Delta& d)        
        {            
            for (int i = 0; i < d.size_.sz.intdata_len; ++i) {
                auto idx = d.intdata_.idx_[i];
                intdata_[idx] = d.intdata_.val_[i];
                int_fields_.set(idx);
            }
            
            for (int i = 0; i < d.size_.sz.dbldata_len; ++i) {
                auto idx = d.dbldata_.idx_[i];
                dbldata_[idx] = d.dbldata_.val_[i];
                dbl_fields_.set(idx);
            }
        }
    };

    struct UNI_SymbolArrSnapshot : public ArrSnapshot<>
    {
        void merge(const UNI_Delta& d)
        {
            
            for (int i = 0; i < d.size_.sz.intdata_len; ++i) {
                const auto& u = d.intdata_[i];
                auto idx = u.bits.index;
                intdata_[idx] = u.bits.int_part;
                int_fields_.set(idx);
            }
            
            for (int i = 0; i < d.size_.sz.dbldata_len; ++i) {
                const auto& u = d.dbldata_[i];
                auto idx = u.bits.index;
                dbldata_[idx] = spnr::uni_to_dbl(u);
                dbl_fields_.set(idx);
            }
            
			/* for (int i = 0; i < d.size_.sz.strdata_len; ++i) {
                auto idx = d.strdata_.idx_[i];
                strdata_[idx] = d.strdata_.val_[i];
                str_fields_.set(idx);
				}*/
        }
    };


    bool operator == (const AOS_SymbolHashSnapshot& aos, const SOA_SymbolHashSnapshot& soa);
    bool operator == (const AOS_SymbolArrSnapshot& aos, const SOA_SymbolArrSnapshot& soa);

    class BenchmarkTestBufferDeltaPublisher
    {
        /// don't send over socket only serialize ///
    public:
        template<class T>
        bool publish(const T& m) {
            return spnr::benchmark_test_write_payload(m);
        }
    };

    std::ostream& operator << (std::ostream& os, const AOS_Delta& d);
    std::ostream& operator << (std::ostream& os, const SOA_Delta& d);
    std::ostream& operator << (std::ostream& os, const Data_Summary& d);

    template<class T>
    std::ostream& operator << (std::ostream& os, const S_Data<T>& d)
    {
        os << "[" << d.idx_ << "->" << d.val_ << "]";
        return os;
    };

    template<class T>
    std::ostream& operator << (std::ostream& os, const std::array<S_Data<T>, TEST_ARR_SIZE>& d)
    {
        std::for_each(std::begin(d), std::end(d), [&os](const S_Data<T>& v)
            {
                os << v;
            });
        return os;
    };

    template<class T>
    std::ostream& operator << (std::ostream& os, const std::array<T, TEST_ARR_SIZE>& d)
    {
        std::for_each(std::begin(d), std::end(d), [&os](const T& v)
            {
                os << v << " ";
            });
        return os;
    };
}
