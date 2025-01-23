#include "md_ref.h"

spnr::MdRef::MdRef() 
{

};

spnr::MdRef::MdRef(const std::string& s) :symbol_(s)
{

};

void spnr::MdRef::set_intdata(uint16_t i, const int32_t& v) 
{
    std::lock_guard<std::mutex> g(mtx_);
    intdata_[i] = v;
    int_fields_.set(i);
};

void spnr::MdRef::set_dbldata(uint16_t i, const double& v) 
{
    std::lock_guard<std::mutex> g(mtx_);
    dbldata_[i] = v;
    dbl_fields_.set(i);
};

void spnr::MdRef::set_strdata(uint16_t i, const std::string& v) 
{
    std::lock_guard<std::mutex> g(mtx_);
    strdata_[i] = v;
    str_fields_.set(i);
};

std::optional<int32_t> spnr::MdRef::get_intdata(uint16_t i)const 
{
    std::lock_guard<std::mutex> g(mtx_);
    if (int_fields_.test(i)) {
        return std::optional<int32_t>(intdata_[i]);
    }
    return std::nullopt;
};

std::optional<double> spnr::MdRef::get_dbldata(uint16_t i)const 
{
    std::lock_guard<std::mutex> g(mtx_);
    if (dbl_fields_.test(i)) {
        return std::optional<double>(dbldata_[i]);
    }
    return std::nullopt;
};

std::optional<std::string> spnr::MdRef::get_strdata(uint16_t i)const 
{
    std::lock_guard<std::mutex> g(mtx_);
    if (str_fields_.test(i)) {
        return std::optional<std::string>(strdata_[i]);
    }
    return std::nullopt;
};


uint16_t spnr::MdRef::calc_len()const 
{
    uint16_t rv = spnr::str_archive_len(symbol_);
    rv += sizeof(uint16_t) + int_fields_.count() * sizeof(uint16_t);
    rv += sizeof(uint16_t) + dbl_fields_.count() * sizeof(uint16_t);
    rv += sizeof(uint16_t) + str_fields_.count() * sizeof(uint16_t);

    rv += sizeof(int32_t) * int_fields_.count();
    rv += sizeof(double) * dbl_fields_.count();
    for(size_t i = 0; i < SNAPSHOT_FIELDS_NUM; ++i)
    {
        if (str_fields_.test(i)) {
            rv += sizeof(uint16_t) + strdata_[i].length();
        }
    }
    return rv;
};




//////////////////
spnr::MdDelta::MdDelta() 
{
    size_.d = 0;
};

spnr::MdDelta::MdDelta(std::string s) : symbol_(s)
{
    size_.d = 0;
};

spnr::MdDelta::~MdDelta() 
{

};

uint16_t spnr::MdDelta::calc_len()const
{
    uint16_t rv = sizeof(int32_t);
    rv += spnr::str_archive_len(symbol_);
    rv += (sizeof(uint16_t) + sizeof(int32_t)) * size_.sz.intdata_len;
    rv += (sizeof(uint16_t) + sizeof(double)) * size_.sz.dbldata_len;    
    rv += sizeof(uint64_t) * size_.sz.stamps_len;
    return rv;
}


bool spnr::MdDelta::add_time_stamp() 
{    
    if (size_.sz.stamps_len >= stamps_.size()) {
        std::cerr << "too many time stamps" << size_.sz.stamps_len << std::endl;
        return false;
    }
    stamps_[size_.sz.stamps_len] = spnr::time_stamp();
    size_.sz.stamps_len = size_.sz.stamps_len + 1;    

    return true;
};

namespace spnr
{
	std::ostream& operator<<(std::ostream& os, const spnr::MdRef& m)
	{
		os << "ref " << m.symbol() << std::endl;
        for (size_t i = 0; i < SNAPSHOT_FIELDS_NUM; ++i) {
            if (m.int_fields_[i]) 
            {
                os << i << " " << m.intdata_[i] << std::endl;
            }
        }

        for (size_t i = 0; i < SNAPSHOT_FIELDS_NUM; ++i) {
            if (m.dbl_fields_[i])
            {
                os << i << " " << m.dbldata_[i] << std::endl;
            }
        }

        for (size_t i = 0; i < SNAPSHOT_FIELDS_NUM; ++i) {
            if (m.str_fields_[i])
            {
                os << i << " " << m.strdata_[i] << std::endl;
            }
        }
		return os;
	};
	
	std::ostream& operator<<(std::ostream& os, const spnr::MdDelta& m)
	{
		os << "delta " << m.symbol() << std::endl;
		const auto& intdata = m.intdata();
		for (uint8_t i = 0; i < m.intdata_len(); i++) {
			os << intdata.idx_[i] << "-i:" << intdata.val_[i] << std::endl;
		}
		const auto& dbldata = m.dbldata();
		for (uint8_t i = 0; i < m.dbldata_len(); i++) {
			os << dbldata.idx_[i] << "-d:" << dbldata.val_[i] << std::endl;
		}
		if (m.stamps_len() > 0) {
			auto now = spnr::time_stamp();
			const auto& stamps = m.stamps();
			for (uint8_t i = 0; i < m.stamps_len(); i++) {
				os << "stamp:" << stamps[i] << " latency(usec)=" << now - stamps[i] << std::endl;
			}
		}
		return os;
	};
}
