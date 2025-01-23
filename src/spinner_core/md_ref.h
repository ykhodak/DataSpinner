#pragma once

#include "records.h"
#include "delta_builder.h"

namespace spnr
{
    class MdRef 
    {
        friend class MdRefTable;
    public:
        MdRef();
        MdRef(const std::string& s);

        EMsgType msgtype()const { return EMsgType::md_refference; }
        const std::string& symbol()const { return symbol_; }

        void set_intdata(uint16_t i, const int32_t& v);
        void set_dbldata(uint16_t i, const double& v);
        void set_strdata(uint16_t i, const std::string& v);

        std::optional<int32_t>      get_intdata(uint16_t i)const;
        std::optional<double>       get_dbldata(uint16_t i)const;
        std::optional<std::string>  get_strdata(uint16_t i)const;

        uint16_t calc_len()const;
    protected:
        std::string         symbol_;

        INT_FIELDS  intdata_;
        DBL_FIELDS  dbldata_;
        STR_FIELDS  strdata_;

        FIELDS_SET  int_fields_;
        FIELDS_SET  dbl_fields_;
        FIELDS_SET  str_fields_;

        mutable std::mutex  mtx_;

        friend BuffMsgArchive& operator<<(BuffMsgArchive& ar, const spnr::MdRef& d);
        friend const BuffMsgArchive& operator>>(const BuffMsgArchive& ar, spnr::MdRef& d);
        friend std::ostream& operator<<(std::ostream& os, const spnr::MdRef& d);
    };


    class MdDelta
    {
        template<class MDELTA, class PUBLISHER> friend class MdDeltaBuilder;

    public:
        using PTR = std::shared_ptr<MdDelta>;

        MdDelta();
        MdDelta(std::string);
        ~MdDelta();
        EMsgType msgtype()const { return EMsgType::md_delta; }
        const std::string& symbol()const { return symbol_; }

        bool load(spnr::BuffMsgArchive& ar);
        uint16_t calc_len()const;

        const INT_SOR& intdata()const { return intdata_; }
        const DBL_SOR& dbldata()const { return dbldata_; }
        const STAMP_ARR& stamps()const { return stamps_; }

        uint8_t             intdata_len()const { return size_.sz.intdata_len; }
        uint8_t             dbldata_len()const { return size_.sz.dbldata_len; }
        uint8_t             stamps_len()const { return size_.sz.stamps_len; }

        bool                add_time_stamp();
    protected:

        std::string     symbol_;
        UDeltaSize      size_;
        INT_SOR         intdata_;
        DBL_SOR         dbldata_;
        STAMP_ARR       stamps_;

        friend BuffMsgArchive& operator<<(BuffMsgArchive& ar, const spnr::MdDelta& d);
        friend const BuffMsgArchive& operator>>(const BuffMsgArchive& ar, spnr::MdDelta& d);
        friend std::ostream& operator<<(std::ostream& os, const spnr::MdDelta& d);
    };
}
