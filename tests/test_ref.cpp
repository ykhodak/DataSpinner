#include <iostream>
#include <limits>
#include "gtest/gtest.h"
#include "test_tu.h"
#include <cmath>
#include <inttypes.h>

class PrintLen
{
public:
    template<class T>
    bool publish(const T& m) {
        std::cout << "len=" << m.calc_len() << std::endl;
        return true;
    }
} print_len;

spnr::MessagePrinter delta_printer;

TEST(ref, make_one_delta)
{    
    spnr::make_one_delta<spnr::MdDelta, spnr::MessagePrinter>("AAPL", delta_printer);
}

TEST(ref, make_n_int_delta)
{
    spnr::make_n_int_delta<spnr::MdDelta, spnr::MessagePrinter>("AAPL", delta_printer, 3);
}

TEST(ref, n_int_calc_len)
{
    spnr::make_n_int_delta<spnr::MdDelta, PrintLen>("AAPL", print_len, 3);
}

TEST(ref, one_delta_calc_len)
{
    std::cout << "sizeof(MdDelta) = " << sizeof(spnr::MdDelta) << std::endl;
    spnr::make_one_delta<spnr::MdDelta, PrintLen>("AAPL", print_len);
}

TEST(delta, sum_aos_int)
{
    auto aos = spnr::make_aos<int32_t>(1);
    auto r1 = spnr::sum_aos(aos);

    auto soa = spnr::make_soa<int32_t>(1);
    auto r2 = spnr::sum_soa(soa);

    ASSERT_EQ(r1, r2) << "aos != soa sum, something wrong in iteration";

    std::cout << r1.idx_ << "->" << r1.val_ << std::endl;
    std::cout << r2.idx_ << "->" << r2.val_ << std::endl;
}

TEST(delta, sum_aos_double)
{
    auto aos = spnr::make_aos<double>(1, 1.01);
    auto r1 = spnr::sum_aos(aos);

    auto soa = spnr::make_soa<double>(1, 1.01);
    auto r2 = spnr::sum_soa(soa);

    ASSERT_EQ(r1, r2) << "aos != soa sum, something wrong in iteration";

    std::cout << r1.idx_ << "->" << r1.val_ << std::endl;
    std::cout << r2.idx_ << "->" << r2.val_ << std::endl;
}

TEST(delta, sum_aos_str)
{
    auto aos = spnr::make_aos<std::string>(0.1);
    auto r1 = spnr::sum_aos(aos);

    auto soa = spnr::make_soa<std::string>(0.1);
    auto r2 = spnr::sum_soa(soa);

    ASSERT_EQ(r1, r2) << "aos != soa sum, something wrong in iteration";

    std::cout << r1.idx_ << "->" << r1.val_ << std::endl;
    std::cout << r2.idx_ << "->" << r2.val_ << std::endl;
}

TEST(delta, sum_AOS_Delta)
{
    auto aos = spnr::AOS_Delta::make_delta();
    std::cout << "aos-delta=" << aos << std::endl;
    auto soa = spnr::SOA_Delta::make_delta();
    std::cout << "soa-delta=" << soa << std::endl;

    auto s1 = aos.make_summary();
    auto s2 = soa.make_summary();
    ASSERT_EQ(s1, s2) << "summary aos != soa, something wrong in iteration";
    std::cout << "aos-summary=" << s1 << std::endl;
    std::cout << "soa-summary=" << s2 << std::endl;
}

TEST(delta, len_AOS_Delta)
{
    auto aos = spnr::AOS_Delta::make_delta();
    std::cout << "aos-delta=" << aos << std::endl;
    auto soa = spnr::SOA_Delta::make_delta();
    std::cout << "soa-delta=" << soa << std::endl;

    auto s1 = aos.calc_len();
    auto s2 = soa.calc_len();
    ASSERT_EQ(s1, s2) << "len aos != soa, something wrong in iteration";
    std::cout << "aos-len=" << s1 << " sizeof(aos-delta)=" << sizeof(spnr::AOS_Delta) << std::endl;
    std::cout << "soa-len=" << s2 << " sizeof(soa-delta)=" << sizeof(spnr::SOA_Delta) << std::endl;
}

TEST(delta, merge_AOS_Hash)
{
    spnr::AOS_SymbolHashSnapshot aos;
    spnr::SOA_SymbolHashSnapshot soa;
    for (int i = 1; i <= 1000; ++i) {
        auto aos_d = spnr::AOS_Delta::make_delta(i);
        aos.merge(aos_d);

        auto soa_d = spnr::SOA_Delta::make_delta(i);
        soa.merge(soa_d);
    }

    ASSERT_EQ(aos, soa) << "len aos != soa, something wrong in iteration";
    std::cout << "hash-snapshot-size aos=" << sizeof(aos) << "["
        << aos.intdata_.size() << "," << aos.dbldata_.size() << "," << aos.strdata_.size() << "]soa=" << sizeof(soa) << "["
        << soa.intdata_.size() << "," << soa.dbldata_.size() << "," << soa.strdata_.size() << "]"
        << std::endl;
}

TEST(delta, merge_AOS_Arr)
{
    spnr::AOS_SymbolArrSnapshot aos;
    spnr::SOA_SymbolArrSnapshot soa;
    for (int i = 1; i <= 1000; ++i) {
        auto aos_d = spnr::AOS_Delta::make_delta(i);
        aos.merge(aos_d);

        auto soa_d = spnr::SOA_Delta::make_delta(i);
        soa.merge(soa_d);
    }

    ASSERT_EQ(aos, soa) << "len aos != soa, something wrong in iteration";
    std::cout << "arr-snapshot-size aos=" << sizeof(aos) << "["
        << aos.intdata_.size() << "," << aos.dbldata_.size() << "," << aos.strdata_.size() << "]soa=" << sizeof(soa) << "["
        << soa.intdata_.size() << "," << soa.dbldata_.size() << "," << soa.strdata_.size() << "]"
        << std::endl;
}


/*
TEST(number, double)
{
    double big_pi = std::numbers::pi * 10000000;
    uint64_t int_part = (uint64_t)big_pi;
    uint16_t dec_part = (uint16_t)(std::round((big_pi - int_part) * 10000));
    printf(">>> big_pi = [%0.8f] => [%].[%]", big_pi, int_part, dec_part);
    auto u = spnr::dblf_to_uni(997, big_pi);
    auto d2 = spnr::uni_to_dbl(u);
    printf(">>> in-uni = [%0.8f] => [%].[%] idx=[%]", d2, (uint64_t)u.bits.int_part, u.bits.dec_part, u.bits.index);
}

TEST(number, int)
{
    int64_t d = 2321312321312121;
    auto u = spnr::intf_to_uni(997, d);
    auto d2 = u.bits.int_part;
    printf(">>>> d=[%] ud=[%] in-uni=[%] idx=[%]", d, d2, (uint64_t)u.bits.int_part, u.bits.index);    
}
*/
