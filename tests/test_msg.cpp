#include <iostream>
#include <limits>
#include "gtest/gtest.h"
#include "archive.h"
#include "md_ref.h"

TEST(h2n, 3conv)
{
    for (int i = 0; i < 100; i += 10)
    {
        uint16_t d = 123 + i;
        auto d2 = htons(d);
        auto d3 = ntohs(d2);
        EXPECT_EQ(d, d3);
    }

    for (int i = 0; i < 100; i += 10)
    {
        uint32_t d = 123 + i;
        auto d2 = htonl(d);
        auto d3 = ntohl(d2);
        EXPECT_EQ(d, d3);
    }
}

TEST(h2n, uint64)
{
    for (int i = 0; i < 100; i += 10)
    {
        uint64_t d = 1000000;
        d *= 100;
        auto d2 = spnr::hton64(d);
        auto d3 = spnr::ntoh64(d2);
        EXPECT_EQ(d, d3);
        EXPECT_EQ(sizeof(d), 8);
    }

    uint64_t dval = uint64_t(-1);
    for (int i = 1; i < 110; i += 10)
    {
        uint64_t d = dval - i;
        auto d2 = spnr::hton64(d);
        auto d3 = spnr::ntoh64(d2);
        EXPECT_EQ(d, d3);
        EXPECT_EQ(sizeof(d), 8);
    }

    for (int i = 0; i < 100; i += 10)
    {
        uint64_t d = spnr::time_stamp();
        d *= 100;
        auto d2 = spnr::hton64(d);
        auto d3 = spnr::ntoh64(d2);
        EXPECT_EQ(d, d3);
        EXPECT_EQ(sizeof(d), 8);
    }

    spnr::MdDelta md;
    std::cout << "md-size=" << sizeof(md) << std::endl;
}

TEST(serialize, str)
{
    auto n = spnr::str_archive_len("TEST");
    EXPECT_EQ(n, 6);
}

TEST(serialize, str_arr)
{
    std::vector<std::string> arr;
    arr.push_back("10");
    arr.push_back("20");
    arr.push_back("30");
    arr.push_back("40");
    arr.push_back("50");
    auto n = spnr::str_arr_archive_len(arr);
    EXPECT_EQ(n, 22);
}

TEST(serialize, uint8_arr)
{
    std::vector<uint8_t> arr;
    arr.push_back(10);
    arr.push_back(20);
    arr.push_back(30);
    arr.push_back(40);
    arr.push_back(50);
    auto n = spnr::num_arr_archive_len(arr);
    EXPECT_EQ(n, 7);
}

TEST(serialize, int32_arr)
{
    std::vector<int32_t> arr;
    arr.push_back(10);
    arr.push_back(20);
    arr.push_back(30);
    arr.push_back(40);
    arr.push_back(50);
    auto n = spnr::num_arr_archive_len(arr);
    EXPECT_EQ(n, 22);
}

TEST(serialize, double_arr)
{
    std::vector<double> arr;
    arr.push_back(10.1);
    arr.push_back(20.1);
    arr.push_back(30.1);
    arr.push_back(40.1);
    arr.push_back(50.1);
    auto n = spnr::num_arr_archive_len(arr);
    EXPECT_EQ(n, 42);
}

TEST(serialize, uint64_arr)
{
    std::vector<uint64_t> arr;
    arr.push_back(10);
    arr.push_back(20);
    arr.push_back(30);
    arr.push_back(40);
    arr.push_back(50);
    auto n = spnr::num_arr_archive_len(arr);
    EXPECT_EQ(n, 42);
}
