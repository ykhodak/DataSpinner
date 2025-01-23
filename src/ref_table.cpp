#include "ref_table.h"
#include "tcp_connection.h"

////////////////// MdRefTable ///////////////////
spnr::MdRefTable::MdRefTable()
{

};

spnr::MdRef* spnr::MdRefTable::get_ref(const std::string& symbol)
{
    std::lock_guard<std::mutex> g(mtx_);
    spnr::MdRef* rv = nullptr;
    auto i = m_ref_table_.find(symbol);
    if (i != m_ref_table_.end())
    {
        return i->second.get();
    }
    return rv;
};

void spnr::MdRefTable::add_ref(const std::string& s, 
    FIELDS_SET&& int_fields,
    INT_FIELDS&& intdata, 
    FIELDS_SET&& dbl_fields,
    DBL_FIELDS&& dbldata, 
    FIELDS_SET&& str_fields,
    STR_FIELDS&& strdata)
{
    std::lock_guard<std::mutex> g(mtx_);
    auto r = std::make_unique<MdRef>(s);

    r->int_fields_ = int_fields;
    r->dbl_fields_ = dbl_fields;
    r->str_fields_ = str_fields;

    r->intdata_ = intdata;
    r->dbldata_ = dbldata;
    r->strdata_ = strdata;
    m_ref_table_[s] = std::move(r);
};

bool spnr::MdRefTable::send_snapshot(const std::vector<std::string>& symbols, TcpConnection* c)
{
    std::lock_guard<std::mutex> g(mtx_);
    for (const auto& s : symbols)
    {
        auto i = m_ref_table_.find(s);
        if (i != m_ref_table_.end())
        {
            c->send_message(*(i->second));
            //p->publish(*(i->second));
            //if (!i->second->write1(b))
            //    return false;
        }
    }
    return true;
};
