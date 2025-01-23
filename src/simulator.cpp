#include "simulator.h"
#include "tcp_connection.h"
#include "ref_table.h"

void spnr::send_regenerated_snapshot(MdRefTable& ref_table, const std::vector<std::string>& symbols, TcpConnection* c)
{
    spnr::regenerate_ref_data(ref_table, symbols);
    ref_table.send_snapshot(symbols, c);
};

void spnr::regenerate_ref_data(MdRefTable& rt, const std::vector<std::string>& symbols) 
{
    for (const auto& s : symbols) 
    {
        auto r = rt.get_ref(s);
        if (r) {
            for (auto i = 0; i < 30; i++) 
            {
                {
                    auto idt = r->get_intdata(i);
                    if (idt) {
                        int32_t v = idt.value();
                        ++v;
                        if (v > 1000)v = 10;
                        r->set_intdata(i, v);
                    }
                }
                {
                    auto ddt = r->get_intdata(i);
                    if (ddt) {
                        double v = ddt.value();
                        v += 1.1;
                        if (v > 1000)v = 10;
                        r->set_dbldata(i, v);
                    }
                }
                {
                    auto sdt = r->get_strdata(i);
                    if (sdt) {
                        std::string v = sdt.value();
                        v += "+1";
                        if (v.length() > 80)v.resize(10);
                        r->set_strdata(i, v);
                    }
                }
            }
        }
        else {
//            INT_HMAP intdata;
            //DBL_HMAP dbldata;
            //STR_HMAP strdata;
            FIELDS_SET  int_fields;
            FIELDS_SET  dbl_fields;
            FIELDS_SET  str_fields;
            INT_FIELDS  intdata;
            DBL_FIELDS  dbldata;
            STR_FIELDS  strdata;


            for (auto i = 0; i < 30; i++) {
                int_fields.set(i);
                dbl_fields.set(i);
                str_fields.set(i);
                intdata[i] = 10 * i;
                dbldata[i] = 100.1 + 10.1 * i;
                strdata[i] = s + "ref string #" + std::to_string(i);
            }
            rt.add_ref(s, 
                std::move(int_fields), 
                std::move(intdata), 
                std::move(dbl_fields),
                std::move(dbldata),
                std::move(str_fields),
                std::move(strdata));
        }
    }
};



