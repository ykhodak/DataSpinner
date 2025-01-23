#include "test_tu.h"
#include <cmath>

namespace spnr
{
    Data_Summary::Data_Summary()
    {        
    }

    bool Data_Summary::operator == (const Data_Summary& other)const
    {
        return other.int_summary == int_summary && other.dbl_summary == dbl_summary;
    }

    AOS_Delta AOS_Delta::make_delta(int start_idx)
    {
        AOS_Delta rv;
        rv.intdata_ = make_aos<int32_t>(start_idx);
        rv.dbldata_ = make_aos<double>(start_idx);
        rv.stamps_ = make_arr<uint64_t>();

        rv.size_.sz.intdata_len = TEST_ARR_SIZE;
        rv.size_.sz.dbldata_len = TEST_ARR_SIZE;
        rv.size_.sz.stamps_len = TEST_ARR_SIZE;
        return rv;
    }

    Data_Summary AOS_Delta::make_summary()
    {
        Data_Summary rv;
        rv.int_summary = sum_aos(intdata_);
        rv.dbl_summary = sum_aos(dbldata_);
        return rv;
    }

    uint16_t AOS_Delta::calc_len()const 
    {
        uint16_t rv = sizeof(int32_t);
        rv += spnr::str_archive_len(symbol_);
        rv += (sizeof(uint16_t) + sizeof(int32_t)) * TEST_ARR_SIZE;
        rv += (sizeof(uint16_t) + sizeof(double)) * TEST_ARR_SIZE;
        //for (uint8_t i = 0; i < TEST_STR_ARR_SIZE; ++i) {
        //    rv += sizeof(uint16_t) + sizeof(uint16_t) + strdata_[i].val_.length();
        //}
        rv += sizeof(uint64_t) * TEST_ARR_SIZE;
        return rv;
    };

    uint16_t SOA_Delta::calc_len()const 
    {
        uint16_t rv = sizeof(int32_t);
        rv += spnr::str_archive_len(symbol_);
        rv += sizeof(uint16_t) * TEST_ARR_SIZE;
        rv += sizeof(int32_t) * TEST_ARR_SIZE;
        rv += sizeof(uint16_t) * TEST_ARR_SIZE;
        rv += sizeof(double) * TEST_ARR_SIZE;
        //rv += sizeof(uint16_t) * TEST_STR_ARR_SIZE;
        //for (uint8_t i = 0; i < TEST_STR_ARR_SIZE; ++i) {
        //    rv += sizeof(uint16_t) + strdata_.val_[i].length();
        //}
        rv += sizeof(uint64_t) * TEST_ARR_SIZE;
        return rv;
    };

    BuffMsgArchive& operator<<(BuffMsgArchive& ar, const spnr::AOS_Delta& d)
    {
        ar << d.symbol_;
        ar << d.size_.d;
        ar.store_idxdata(d.size_.sz.intdata_len, d.intdata_.cbegin(), d.intdata_.cend());
        ar.store_idxdata(d.size_.sz.dbldata_len, d.dbldata_.cbegin(), d.dbldata_.cend());
        //ar.store_idxdata(d.size_.sz.strdata_len, d.strdata_.cbegin(), d.strdata_.cend());
        ar.store_arr(d.size_.sz.stamps_len, d.stamps_.cbegin(), d.stamps_.cend());
        return ar;
    };

    BuffMsgArchive& operator<<(BuffMsgArchive& ar, const spnr::SOA_Delta& d) 
    {
        ar << d.symbol_;
        ar << d.size_.d;
        ar.store_sor_data(d.size_.sz.intdata_len, d.intdata_);
        ar.store_sor_data(d.size_.sz.dbldata_len, d.dbldata_);
        //ar.store_sor_data(d.size_.sz.strdata_len, d.strdata_);
        ar.store_arr(d.size_.sz.stamps_len, d.stamps_.cbegin(), d.stamps_.cend());
        return ar;
    };

    SOA_Delta SOA_Delta::make_delta(int start_idx)
    {
        SOA_Delta rv;        
        rv.intdata_ = make_soa<int32_t>(start_idx);
        rv.dbldata_ = make_soa<double>(start_idx);
        //rv.strdata_ = make_soa<std::string>(start_idx);
        rv.stamps_ = make_arr<uint64_t>();

        rv.size_.sz.intdata_len = TEST_ARR_SIZE;
        rv.size_.sz.dbldata_len = TEST_ARR_SIZE;
        //rv.size_.sz.strdata_len = TEST_STR_ARR_SIZE;
        rv.size_.sz.stamps_len = TEST_ARR_SIZE;
        return rv;
    }

    std::array<UField4decDouble, TEST_ARR_SIZE> make_dbl_uni_arr(int start_idx = 1, double dx = 1)
    {
        std::array<UField4decDouble, TEST_ARR_SIZE> uni;
        for (size_t i = 0; i < TEST_ARR_SIZE; ++i)
        {
            uni[i] = spnr::dblf_to_uni(start_idx, start_idx * (10 * dx));
            start_idx = start_idx + 1;
        }
        return uni;
    }

    std::array<UFieldInt, TEST_ARR_SIZE> make_dbl_int_arr(int start_idx = 1, double dx = 1)
    {
        std::array<UFieldInt, TEST_ARR_SIZE> uni;
        for (size_t i = 0; i < TEST_ARR_SIZE; ++i)
        {
            uni[i] = spnr::intf_to_uni(start_idx, start_idx * (10 * dx));
            start_idx = start_idx + 1;
        }
        return uni;
    }


    //std::vector<spnr::UNI_Delta> make_uni_arr(size_t start_idx)
    UNI_Delta UNI_Delta::make_delta(int start_idx)
    {
        UNI_Delta rv;
        rv.intdata_ = make_dbl_int_arr(start_idx);
        rv.dbldata_ = make_dbl_uni_arr(start_idx);
        //rv.strdata_ = make_soa<std::string>(start_idx);
        rv.stamps_ = make_arr<uint64_t>();

        rv.size_.sz.intdata_len = TEST_ARR_SIZE;
        rv.size_.sz.dbldata_len = TEST_ARR_SIZE;
        //rv.size_.sz.strdata_len = TEST_STR_ARR_SIZE;
        rv.size_.sz.stamps_len = TEST_ARR_SIZE;
        return rv;
    };

    Data_Summary SOA_Delta::make_summary()
    {
        Data_Summary rv;
        rv.int_summary = sum_soa(intdata_);
        rv.dbl_summary = sum_soa(dbldata_);
        return rv;
    }

    

    bool operator == (const AOS_SymbolHashSnapshot& aos, const SOA_SymbolHashSnapshot& soa)
    {
        return (aos.intdata_ == soa.intdata_ && aos.dbldata_ == soa.dbldata_ && aos.strdata_ == soa.strdata_);
    };

    bool operator == (const AOS_SymbolArrSnapshot& aos, const SOA_SymbolArrSnapshot& soa) 
    {
        return (aos.intdata_ == soa.intdata_ && aos.dbldata_ == soa.dbldata_ && aos.strdata_ == soa.strdata_ &&
            aos.int_fields_ == soa.int_fields_ && aos.dbl_fields_ == soa.dbl_fields_ && aos.str_fields_ == soa.str_fields_);
    };

    UField4decDouble dblf_to_uni(uint16_t idx, const double& d)
    {
        UField4decDouble rv;
        rv.bits.index = idx;
        rv.bits.sign = (d < 0) ? 1 : 0;
        rv.bits.int_part = (uint64_t)d;
        rv.bits.dec_part = (uint16_t)(std::round((d - rv.bits.int_part) * 10000));
        return rv;
    };

    UFieldInt intf_to_uni(uint16_t idx, const int64_t& d)
    {
        UFieldInt rv;
        rv.bits.index = idx;
        rv.bits.int_part = (int64_t)d;
        return rv;
    }

    std::ostream& operator << (std::ostream& os, const Data_Summary& d)
    {
        os << d.int_summary;
        os << d.dbl_summary;
        return os;
    }

    std::ostream& operator << (std::ostream& os, const AOS_Delta& d)
    {
        os << "int=" << d.intdata_;
        os << "dbl=" << d.dbldata_;
//        os << "str=" << d.strdata_;
        os << "stamp=" << d.stamps_;
        return os;
    }

    std::ostream& operator << (std::ostream& os, const SOA_Delta& d)
    {
        os << "int=" << d.intdata_;
        os << "dbl=" << d.dbldata_;
		//      os << "str=" << d.strdata_;
        os << "stamp=" << d.stamps_;
        return os;
    }
};
