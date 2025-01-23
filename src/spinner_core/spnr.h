#pragma once

#include <string>
#include <memory>
#include <array>
#include <vector>
#include <deque>
#include <algorithm>
#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <stdio.h>
#include <cstring>
#include <iostream>
#include <chrono>
#include <thread>
#include <numeric>
#include <fstream>
#include <filesystem>
#include <optional>
#include <condition_variable>
#include <bitset>
#include <numbers>

#ifndef _WIN32
#include <arpa/inet.h>
#endif

namespace spnr
{
#define INT_DELTA_SIZE 6
#define DBL_DELTA_SIZE 18
#define STAMP_DELTA_SIZE 2
#define SNAPSHOT_FIELDS_NUM  1024

    template<class T, size_t N>
    struct SOR_Data
    {
        std::array<uint16_t, N> idx_;
        std::array <T, N> val_;
    };
    using INT_SOR = SOR_Data<int32_t,   INT_DELTA_SIZE>;
    using DBL_SOR = SOR_Data<double,    DBL_DELTA_SIZE>;
    
    using INT_FIELDS = std::array<int32_t, SNAPSHOT_FIELDS_NUM>;
    using DBL_FIELDS = std::array<double, SNAPSHOT_FIELDS_NUM>;
    using STR_FIELDS = std::array<std::string, SNAPSHOT_FIELDS_NUM>;
    using FIELDS_SET = std::bitset<SNAPSHOT_FIELDS_NUM>;

    using STAMP_ARR = std::array<uint64_t, STAMP_DELTA_SIZE>;

    using INT_HMAP  = std::unordered_map<uint16_t, int32_t>;
    using DBL_HMAP  = std::unordered_map<uint16_t, double>;
    using STR_HMAP  = std::unordered_map<uint16_t, std::string>;
    using STR_VEC   = std::vector<std::string>;
    using STR_SET   = std::unordered_set<std::string>;

#define WIRE_TSTAMP_LEN    sizeof(uint64_t)
#define TAG_SOH     1

    inline uint16_t str_archive_len(const std::string& s) { return (sizeof(uint16_t) + s.length()); };
    inline uint16_t str_arr_archive_len(const std::vector<std::string>& arr)
    {
        uint16_t r = sizeof(uint16_t);
        for (const auto& v : arr) {
            r += v.length() + sizeof(uint16_t);
        }
        return r;
    };
    template <class T>
    uint16_t num_arr_archive_len(const std::vector<T>& arr)
    {
        uint16_t r = sizeof(uint16_t) + sizeof(T) * arr.size();
        return r;
    };

    inline uint64_t time_stamp()
    {
        return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    }

    enum class EMsgType : uint8_t
    {
        none = 0,
        login = 1,
        login_reply,
        text,
        heartbeat,
        subscribe,
        md_refference,
        md_delta,
        interactive_menu,
        intvec,
        bytevec,
        doublevec,
        command
    };

    struct DeltaSize
    {
        uint8_t     intdata_len;
        uint8_t     dbldata_len;
        uint8_t     reserved;
        uint8_t     stamps_len;
    };

    union UDeltaSize
    {
        DeltaSize     sz;
        uint32_t      d;
    };

}
