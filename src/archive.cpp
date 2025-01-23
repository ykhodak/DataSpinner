#include "archive.h"
#include "md_ref.h"

namespace spnr
{
    BuffMsgArchive& operator<<(BuffMsgArchive& ar, const std::string& s)
    {
        uint16_t len = s.length();
        len = htons(len);
        ar.buffer_write(&len, sizeof(len));
        ar.buffer_write(s.c_str(), s.length());
        return ar;
    };

    const BuffMsgArchive& operator>>(const BuffMsgArchive& ar, std::string& s)
    {
        uint16_t len = 0;
        if (!ar.buffer_read(&len, sizeof(len)))
        {
            ar.is_valid_ = false;
            errlog("archive read error");
        };
        len = ntohs(len);
        s.resize(len);
        char* data = s.data();
        if (!ar.buffer_read(data, len))
        {
            ar.is_valid_ = false;
            errlog("archive read error");
        };
        data[len] = 0;
        return ar;
    };

    BuffMsgArchive& operator<<(BuffMsgArchive& ar, const StrData& s)
    {
        ar << s.idx_;
        ar << s.val_;
        return ar;
    };

    const BuffMsgArchive& operator>>(const BuffMsgArchive& ar, StrData& s)
    {
        ar >> s.idx_;
        ar >> s.val_;
        return ar;
    };


    BuffMsgArchive& operator<<(BuffMsgArchive& ar, const spnr::MdDelta& d) 
    {
        ar << d.symbol_;
        ar << d.size_.d;
        ar.store_sor_data(d.size_.sz.intdata_len, d.intdata_);
        ar.store_sor_data(d.size_.sz.dbldata_len, d.dbldata_);
        //ar.store_idxdata(d.size_.sz.strdata_len, d.strdata_.cbegin(), d.strdata_.cend());
        ar.store_arr(d.size_.sz.stamps_len, d.stamps_.cbegin(), d.stamps_.cend());
        return ar;
    };

    const BuffMsgArchive& operator>>(const BuffMsgArchive& ar, spnr::MdDelta& d) 
    {
        ar >> d.symbol_;
        ar >> d.size_.d;
        if (d.size_.sz.intdata_len > d.intdata_.idx_.size()) {
            ar.set_invalid_archive();
            return ar;
        }
        if (d.size_.sz.dbldata_len > d.dbldata_.idx_.size()) {
            ar.set_invalid_archive();
            return ar;
        }
        ar.load_sor_data(d.size_.sz.intdata_len, d.intdata_);
        ar.load_sor_data(d.size_.sz.dbldata_len, d.dbldata_);

        /*if (!ar.load_sor_data(d.size_.sz.intdata_len, d.intdata_) {
            ar.set_invalid_archive();
            return ar;
        };
        if (!ar.load_sor_data(d.size_.sz.dbldata_len, d.dbldata_.begin(), d.dbldata_.end())) {
            ar.set_invalid_archive();
            return ar;
        };*/
        /*if (!ar.load_idxdata(d.size_.sz.strdata_len, d.strdata_.begin(), d.strdata_.end())) {
            ar.set_invalid_archive();
            return ar;
        };*/
        if (!ar.load_arr(d.size_.sz.stamps_len, d.stamps_.begin(), d.stamps_.end())) {
            ar.set_invalid_archive();
            return ar;
        };
        return ar;
    };

    BuffMsgArchive& operator<<(BuffMsgArchive& ar, const spnr::MdRef& d) 
    {
        ar << d.symbol_;
        ar << d.int_fields_;
        ar << d.dbl_fields_;
        ar << d.str_fields_;
       
        for (size_t i = 0; i < SNAPSHOT_FIELDS_NUM; ++i)
        {
            if (d.int_fields_.test(i)) {
                const auto& val = d.intdata_[i];
                ar << val;
            }
        }
        for (size_t i = 0; i < SNAPSHOT_FIELDS_NUM; ++i)
        {
            if (d.dbl_fields_.test(i)) {
                const auto& val = d.dbldata_[i];
                ar << val;
            }
        }
        for (size_t i = 0; i < SNAPSHOT_FIELDS_NUM; ++i)
        {
            if (d.str_fields_.test(i)) {
                const auto& val = d.strdata_[i];
                ar << val;
            }
        }


        //ar << d.intdata_;
        //ar << d.dbldata_;
        //ar << d.strdata_;
        return ar;
    };

    const BuffMsgArchive& operator>>(const BuffMsgArchive& ar, spnr::MdRef& d)
    {
        ar >> d.symbol_;
        ar >> d.int_fields_;
        ar >> d.dbl_fields_;
        ar >> d.str_fields_;

        for (size_t i = 0; i < SNAPSHOT_FIELDS_NUM; ++i)
        {
            if (d.int_fields_.test(i)) {
                int32_t val = 0;
                ar >> val;
                d.intdata_[i] = val;
            }
        }
        for (size_t i = 0; i < SNAPSHOT_FIELDS_NUM; ++i)
        {
            if (d.dbl_fields_.test(i)) {
                double val = 0.0;
                ar >> val;
                d.dbldata_[i] = val;
            }
        }
        for (size_t i = 0; i < SNAPSHOT_FIELDS_NUM; ++i)
        {
            if (d.str_fields_.test(i)) {
                std::string val;
                ar >> val;
                d.strdata_[i] = val;
            }
        }

        //ar >> d.intdata_;
        //ar >> d.dbldata_;
        //ar >> d.strdata_;
        return ar;
    };

    BuffMsgArchive& operator<<(BuffMsgArchive& ar, const spnr::TextMsg& d) 
    {
        ar << d.m_text;
        return ar;
    };

    const BuffMsgArchive& operator>>(const BuffMsgArchive& ar, spnr::TextMsg& d)
    {
        ar >> d.m_text;
        return ar;
    };

    BuffMsgArchive& operator<<(BuffMsgArchive& ar, const spnr::HBMsg& d) 
    {
        ar << d.counter_;
        return ar;
    };

    const BuffMsgArchive& operator>>(const BuffMsgArchive& ar, spnr::HBMsg& d)
    {
        ar >> d.counter_;
        return ar;
    };

    BuffMsgArchive& operator<<(BuffMsgArchive& ar, const spnr::SubscribeMsg& d) 
    {
        ar << d.symbols_;
        return ar;
    };

    const BuffMsgArchive& operator>>(const BuffMsgArchive& ar, spnr::SubscribeMsg& d)
    {
        ar >> d.symbols_;
        return ar;
    };

    BuffMsgArchive& operator<<(BuffMsgArchive& ar, const spnr::LoginMsg& d) 
    {
        ar << d.version_;
        ar << d.user_;
        ar << d.token_;
        ar << d.params_;
        return ar;
    };

    const BuffMsgArchive& operator>>(const BuffMsgArchive& ar, spnr::LoginMsg& d)
    {
        ar >> d.version_;
        ar >> d.user_;
        ar >> d.token_;
        ar >> d.params_;
        return ar;
    };

    BuffMsgArchive& operator<<(BuffMsgArchive& ar, const spnr::LoginReplyMsg& d) 
    {
        ar << d.version_;
        ar << d.reply_code_;
        ar << d.reply_desc_;
        ar << d.params_;
        return ar;
    };

    const BuffMsgArchive& operator>>(const BuffMsgArchive& ar, spnr::LoginReplyMsg& d)
    {
        ar >> d.version_;
        ar >> d.reply_code_;
        ar >> d.reply_desc_;
        ar >> d.params_;
        return ar;
    };

    BuffMsgArchive& operator<<(BuffMsgArchive& ar, const spnr::MenuMsg& d) 
    {
        ar << d.menu_;
        return ar;
    };

    const BuffMsgArchive& operator>>(const BuffMsgArchive& ar, spnr::MenuMsg& d)
    {
        ar >> d.menu_;
        return ar;
    };

    BuffMsgArchive& operator<<(BuffMsgArchive& ar, const spnr::CommandMsg& d) 
    {
        uint8_t ptype = static_cast<uint8_t>(d.cmd_);
        ar << ptype;
        ar << d.client_ctx_;
        ar << d.parameters_;
        return ar;
    };

    const BuffMsgArchive& operator>>(const BuffMsgArchive& ar, spnr::CommandMsg& d)
    {
        uint8_t ptype = 0;
        ar >> ptype;
        ar >> d.client_ctx_;
        d.cmd_ = static_cast<ECommand>(ptype);
        ar >> d.parameters_;
        return ar;
    };
}
