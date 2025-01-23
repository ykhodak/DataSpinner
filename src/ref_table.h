#include "md_ref.h"

namespace spnr
{
    class TcpConnection;

    using SYM2REF = std::unordered_map<std::string, std::unique_ptr<MdRef>>;
    class MdRefTable
    {
    public:
        MdRefTable();

        MdRef*  get_ref(const std::string& symbol);
        void    add_ref(const std::string& s, 
            FIELDS_SET&& int_fields,
            INT_FIELDS&& intdata, 
            FIELDS_SET&& dbl_fields,
            DBL_FIELDS&& dbldata, 
            FIELDS_SET&& str_fields,
            STR_FIELDS&& strdata);

        bool send_snapshot(const std::vector<std::string>& symbols, TcpConnection* c);

    protected:
        SYM2REF m_ref_table_;
        mutable std::mutex mtx_;
    };
}