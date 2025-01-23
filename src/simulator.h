#pragma once

#include "md_ref.h"
#include "archive.h"

namespace spnr
{
    class TcpConnection;
	class MdRefTable;

    template<class T>
    bool benchmark_test_write_payload(const T& m)
    {        
        auto h = spnr::start_header(m.msgtype());
        h.h.len += m.calc_len();

        BuffMsgArchive ar;
        if (!ar.store_header(h))
            return false;
        ar << m;
        return ar();
    };


    void send_regenerated_snapshot(MdRefTable& ref_table, const std::vector<std::string>& symbols, TcpConnection* c);
    void regenerate_ref_data(MdRefTable& ref_table, const std::vector<std::string>& symbols);

    template<class MDELTA, class PUBLISHER>
    void send_generated_delta(const std::vector<std::string>& symbols, PUBLISHER& pub, uint16_t num)
    {
        MdDeltaBuilder<MDELTA, PUBLISHER> bld(pub);
        for (const auto& s : symbols)
        {
            bld.begin(s);
            for (int i = 1; i <= num; i++) {
                bld.add_intdata(i, i * 10);
            }
            for (int i = 1; i <= num; i++) {
                bld.add_dbldata(i, i * 100.1);
            }
            bld.end();
        }
    };


    template<class MDELTA, class PUBLISHER>
    void send_generated_subtype_delta(const std::vector<std::string>& symbols, PUBLISHER& pub, uint16_t num, char dtype)
    {
        MdDeltaBuilder<MDELTA, PUBLISHER> bld(pub);

        if (dtype == 'i' || dtype == 'l' || dtype == 's')
        {
            for (const auto& s : symbols)
            {
                bld.begin(s);
                switch (dtype)
                {
                case 'i':
                {
                    for (int i = 1; i <= num; i++) {
                        bld.add_intdata(i, i * 10);
                    }
                }break;
                case 'l':
                {
                    for (int i = 1; i <= num; i++) {
                        bld.add_dbldata(i, i * 100.1);
                    }
                }break;
                /*case 's':
                {
                    for (int i = 1; i <= num; i++) {
                        std::string str = s + "-#" + std::to_string(i) + "-upd";
                        bld.add_strdata(i, str);
                    }
                }break;*/
                }
            }
            bld.end();
        }
    };

}
