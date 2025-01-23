#pragma once

#include <string>
#include <array>

namespace spnr
{
    template<class MDELTA, class PUBLISHER>
    class MdDeltaBuilder
    {
    public:
        MdDeltaBuilder(PUBLISHER&);
        void begin(const std::string& symbol);
        bool end();

        bool add_intdata(uint16_t i, const int32_t& v);
        bool add_dbldata(uint16_t i, const double& v);

    protected:
        void init_delta();
        bool flush_delta();
        bool flush_and_begin();

        inline MDELTA* d() { return &delta_; }

        PUBLISHER& publisher_;
        std::string     symbol_;
        MDELTA          delta_;
        uint16_t        int_idx_{ 0 };
        uint16_t        dbl_idx_{ 0 };
        uint16_t        str_idx_{ 0 };
    };

    template<class MDELTA, class PUBLISHER>
    MdDeltaBuilder<MDELTA, PUBLISHER>::MdDeltaBuilder(PUBLISHER& p) :publisher_(p)
    {

    };

    template<class MDELTA, class PUBLISHER>
    void MdDeltaBuilder<MDELTA, PUBLISHER>::begin(const std::string& symbol)
    {
        symbol_ = symbol;
        init_delta();
    }

    template<class MDELTA, class PUBLISHER>
    void MdDeltaBuilder<MDELTA, PUBLISHER>::init_delta()
    {
        delta_.symbol_ = symbol_;
        int_idx_ = dbl_idx_ = str_idx_ = 0;
    };

    template<class MDELTA, class PUBLISHER>
    bool MdDeltaBuilder<MDELTA, PUBLISHER>::flush_and_begin()
    {
        if (!flush_delta())
            return false;
        init_delta();
        return true;
    };

    template<class MDELTA, class PUBLISHER>
    bool MdDeltaBuilder<MDELTA, PUBLISHER>::end()
    {
        if (!flush_delta())
            return false;
        int_idx_ = dbl_idx_ = str_idx_ = 0;
        symbol_ = "";
        return true;
    };

    template<class MDELTA, class PUBLISHER>
    bool MdDeltaBuilder<MDELTA, PUBLISHER>::add_intdata(uint16_t i, const int32_t& v)
    {
        static constexpr uint16_t max_idx = INT_DELTA_SIZE;
        if (int_idx_ == max_idx) {
            if (!flush_and_begin())
                return false;
        }
        d()->intdata_.idx_[int_idx_] = i;
        d()->intdata_.val_[int_idx_] = v;
        ++int_idx_;
        return true;
    };

    template<class MDELTA, class PUBLISHER>
    bool MdDeltaBuilder<MDELTA, PUBLISHER>::add_dbldata(uint16_t i, const double& v)
    {
        static constexpr uint16_t max_idx = DBL_DELTA_SIZE;
        if (dbl_idx_ == max_idx) {
            if (!flush_and_begin())
                return false;
        }
        d()->dbldata_.idx_[dbl_idx_] = i;
        d()->dbldata_.val_[dbl_idx_] = v;

        ++dbl_idx_;
        return true;
    };

    template<class MDELTA, class PUBLISHER>
    bool MdDeltaBuilder<MDELTA, PUBLISHER>::flush_delta()
    {
        auto p = d();
        p->size_.d = 0;
        p->size_.sz.intdata_len = int_idx_;
        p->size_.sz.dbldata_len = dbl_idx_;
        p->stamps_[0] = spnr::time_stamp();
        p->size_.sz.stamps_len = 1;
        return publisher_.publish(*(p));
    };
};