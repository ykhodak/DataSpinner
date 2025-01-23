#include <iostream>
#include <benchmark/benchmark.h>
#include "socket_buffer.h"
#include "test_tu.h"

/*
Run on (12 X 2592 MHz CPU s) Windows 11
CPU Caches:
  L1 Data 32 KiB (x6)
  L1 Instruction 32 KiB (x6)
  L2 Unified 256 KiB (x6)
  L3 Unified 12288 KiB (x1)
-----------------------------------------------------------------
Benchmark                       Time             CPU   Iterations
-----------------------------------------------------------------
BM_AOS_hash_merge_100        5785 ns         5720 ns       112000
BM_SOA_hash_merge_100        9452 ns         9417 ns        74667
BM_AOS_hash_merge_1000      69545 ns        69754 ns        11200
BM_SOA_hash_merge_1000      83270 ns        83705 ns         8960
BM_AOS_arr_merge_10           438 ns          443 ns      1659259
BM_SOA_arr_merge_10           420 ns          424 ns      1659259
BM_UNI_arr_merge_10           695 ns          698 ns      1120000
BM_AOS_arr_merge_100         4351 ns         4262 ns       172308
BM_SOA_arr_merge_100         4212 ns         4143 ns       165926
BM_UNI_arr_merge_100         6660 ns         6836 ns       112000
BM_AOS_arr_merge_1000       45481 ns        43945 ns        16000
BM_SOA_arr_merge_1000       44609 ns        44643 ns        11200
BM_UNI_arr_merge_1000       67445 ns        65569 ns        11200


Run on (4 X 2800 MHz CPU s) Ubuntu 24.04
CPU Caches:
  L1 Data 32 KiB (x4)
  L1 Instruction 32 KiB (x4)
  L2 Unified 256 KiB (x4)
  L3 Unified 6144 KiB (x1)
Load Average: 0.29, 0.32, 0.29
***WARNING*** CPU scaling is enabled, the benchmark real time measurements may be noisy and will incur extra overhead.
-----------------------------------------------------------------
Benchmark                       Time             CPU   Iterations
-----------------------------------------------------------------
BM_AOS_hash_merge_100       37847 ns        37846 ns        18463
BM_SOA_hash_merge_100       37858 ns        37857 ns        18524
BM_AOS_hash_merge_1000     351546 ns       351527 ns         1981
BM_SOA_hash_merge_1000     342101 ns       342088 ns         2038
BM_AOS_arr_merge_10           574 ns          574 ns      1217237
BM_SOA_arr_merge_10           536 ns          535 ns      1251276
BM_UNI_arr_merge_10           794 ns          794 ns       868695
BM_AOS_arr_merge_100         5394 ns         5393 ns       126772
BM_SOA_arr_merge_100         5674 ns         5674 ns       119802
BM_UNI_arr_merge_100         7986 ns         7986 ns        87045
BM_AOS_arr_merge_1000       57763 ns        57760 ns        11685
BM_SOA_arr_merge_1000       54067 ns        54065 ns        12594
BM_UNI_arr_merge_1000       80880 ns        80877 ns         8593
*/

//#define USE_EXTRA_BM_REF

#ifdef USE_EXTRA_BM_REF
spnr::MessagePrinter delta_pub;

static void BM_make_aos_int(benchmark::State& state) {    
    for (auto _ : state)
    {
        auto aos = spnr::make_aos<int32_t>();
        benchmark::DoNotOptimize(aos);
    }
}

static void BM_make_aos_double(benchmark::State& state) {
    for (auto _ : state)
    {
        auto aos = spnr::make_aos<double>();
        benchmark::DoNotOptimize(aos);
    }
}

static void BM_make_soa_int(benchmark::State& state) {
    for (auto _ : state)
    {
        auto aos = spnr::make_soa<int32_t>();
        benchmark::DoNotOptimize(aos);
    }
}

static void BM_make_soa_double(benchmark::State& state) {
    for (auto _ : state)
    {
        auto aos = spnr::make_soa<double>();
        benchmark::DoNotOptimize(aos);
    }
}


static void BM_sum_aos_int(benchmark::State& state) {
    auto aos = spnr::make_aos<int32_t>();
    for (auto _ : state)
    {
        auto r = spnr::sum_aos(aos);
        benchmark::DoNotOptimize(r);
    }
}

static void BM_sum_aos_double(benchmark::State& state) {
    auto aos = spnr::make_aos<double>();
    for (auto _ : state)
    {
        auto r = spnr::sum_aos(aos);
        benchmark::DoNotOptimize(r);
    }
}

static void BM_sum_soa_int(benchmark::State& state) {
    auto aos = spnr::make_soa<int32_t>();
    for (auto _ : state)
    {
        auto r = spnr::sum_soa(aos);
        benchmark::DoNotOptimize(r);
    }
}

static void BM_sum_soa_double(benchmark::State& state) {
    auto soa = spnr::make_soa<double>();
    for (auto _ : state)
    {
        auto r = spnr::sum_soa(soa);
        benchmark::DoNotOptimize(r);
    }
}

static void BM_aos_make_delta(benchmark::State& state) {
    for (auto _ : state)
    {
        auto r = spnr::AOS_Delta::make_delta();
        benchmark::DoNotOptimize(r);
    }
}

static void BM_soa_make_delta(benchmark::State& state) {
    for (auto _ : state)
    {
        auto r = spnr::SOA_Delta::make_delta();
        benchmark::DoNotOptimize(r);
    }
}

static void BM_aos_make_summary(benchmark::State& state) {
    auto aos = spnr::AOS_Delta::make_delta();
    for (auto _ : state)
    {
        auto r = aos.make_summary();
        benchmark::DoNotOptimize(r);
    }
}

static void BM_soa_make_summary(benchmark::State& state) {
    auto soa = spnr::SOA_Delta::make_delta();
    for (auto _ : state)
    {
        auto r = soa.make_summary();
        benchmark::DoNotOptimize(r);
    }
}

static void BM_aos_len(benchmark::State& state) {
    auto aos = spnr::AOS_Delta::make_delta();
    for (auto _ : state)
    {
        auto r = aos.calc_len();
        benchmark::DoNotOptimize(r);
    }
}

static void BM_soa_len(benchmark::State& state) {
    auto soa = spnr::SOA_Delta::make_delta();
    for (auto _ : state)
    {
        auto r = soa.calc_len();
        benchmark::DoNotOptimize(r);
    }
}

static void BM_AOS_serialize_out(benchmark::State& state) {
    auto aos = spnr::AOS_Delta::make_delta();
    for (auto _ : state)
    {
        auto r = spnr::benchmark_test_write_payload(aos);
        benchmark::DoNotOptimize(r);
    }
}

static void BM_SOA_serialize_out(benchmark::State& state) {
    auto soa = spnr::SOA_Delta::make_delta();
    for (auto _ : state)
    {
        auto r = spnr::benchmark_test_write_payload(soa);
        benchmark::DoNotOptimize(r);
    }
}

BENCHMARK(BM_make_aos_int);
BENCHMARK(BM_make_aos_double);
BENCHMARK(BM_make_soa_int);
BENCHMARK(BM_make_soa_double);
BENCHMARK(BM_aos_make_delta);
BENCHMARK(BM_soa_make_delta);

BENCHMARK(BM_sum_aos_int);
BENCHMARK(BM_sum_aos_double);
BENCHMARK(BM_sum_soa_int);
BENCHMARK(BM_sum_soa_double);
BENCHMARK(BM_aos_make_summary);
BENCHMARK(BM_soa_make_summary);

BENCHMARK(BM_aos_len);
BENCHMARK(BM_soa_len);

BENCHMARK(BM_AOS_serialize_out);
BENCHMARK(BM_SOA_serialize_out);

#endif

template<class DATA, class SNAPSHOT, size_t N>
void run_merge(benchmark::State& state) 
{
    auto arr = spnr::make_delta_arr<DATA>(N);
    SNAPSHOT snap;
    for (auto _ : state)
    {
        for (const auto& d : arr) {
            snap.merge(d);
        }
        benchmark::DoNotOptimize(snap);
    }
}

static void BM_AOS_hash_merge_100(benchmark::State& state) {
    run_merge<spnr::AOS_Delta, spnr::AOS_SymbolHashSnapshot, 100>(state);
}

static void BM_AOS_hash_merge_1000(benchmark::State& state) {
    run_merge<spnr::AOS_Delta, spnr::AOS_SymbolHashSnapshot, 1000>(state);
}

static void BM_SOA_hash_merge_100(benchmark::State& state) {
    run_merge<spnr::SOA_Delta, spnr::SOA_SymbolHashSnapshot, 100>(state);
}

static void BM_SOA_hash_merge_1000(benchmark::State& state) {
    run_merge<spnr::SOA_Delta, spnr::SOA_SymbolHashSnapshot, 1000>(state);
}

static void BM_AOS_arr_merge_10(benchmark::State& state) {
    run_merge<spnr::AOS_Delta, spnr::AOS_SymbolArrSnapshot, 10>(state);
}

static void BM_AOS_arr_merge_100(benchmark::State& state) {
    run_merge<spnr::AOS_Delta, spnr::AOS_SymbolArrSnapshot, 100>(state);
}

static void BM_AOS_arr_merge_1000(benchmark::State& state) {
    run_merge<spnr::AOS_Delta, spnr::AOS_SymbolArrSnapshot, 1000>(state);
}

static void BM_SOA_arr_merge_10(benchmark::State& state) {
    run_merge<spnr::SOA_Delta, spnr::SOA_SymbolArrSnapshot, 10>(state);
}

static void BM_SOA_arr_merge_100(benchmark::State& state) {
    run_merge<spnr::SOA_Delta, spnr::SOA_SymbolArrSnapshot, 100>(state);
}

static void BM_SOA_arr_merge_1000(benchmark::State& state) {
    run_merge<spnr::SOA_Delta, spnr::SOA_SymbolArrSnapshot, 1000>(state);
}

static void BM_UNI_arr_merge_10(benchmark::State& state) {
    run_merge<spnr::UNI_Delta, spnr::UNI_SymbolArrSnapshot, 10>(state);
}

static void BM_UNI_arr_merge_100(benchmark::State& state) {
    run_merge<spnr::UNI_Delta, spnr::UNI_SymbolArrSnapshot, 100>(state);
}

static void BM_UNI_arr_merge_1000(benchmark::State& state) {
    run_merge<spnr::UNI_Delta, spnr::UNI_SymbolArrSnapshot, 1000>(state);
}

BENCHMARK(BM_AOS_hash_merge_100);
BENCHMARK(BM_SOA_hash_merge_100);
BENCHMARK(BM_AOS_hash_merge_1000);
BENCHMARK(BM_SOA_hash_merge_1000);

BENCHMARK(BM_AOS_arr_merge_10);
BENCHMARK(BM_SOA_arr_merge_10);
BENCHMARK(BM_UNI_arr_merge_10);
BENCHMARK(BM_AOS_arr_merge_100);
BENCHMARK(BM_SOA_arr_merge_100);
BENCHMARK(BM_UNI_arr_merge_100);
BENCHMARK(BM_AOS_arr_merge_1000);
BENCHMARK(BM_SOA_arr_merge_1000);
BENCHMARK(BM_UNI_arr_merge_1000);

int main(int argc, char** argv)
{    
    ::benchmark::Initialize(&argc, argv);
    ::benchmark::RunSpecifiedBenchmarks();
}
