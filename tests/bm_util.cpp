#include <iostream>
#include <benchmark/benchmark.h>
#include "socket_buffer.h"
#include "md_ref.h"
#include "simulator.h"
#include "archive.h"

static void BM_calc_time(benchmark::State& state) {    
    for (auto _ : state)
    {
        auto start_time = time(nullptr);
        benchmark::DoNotOptimize(start_time);
    }
}

static void BM_calc_chrono_time(benchmark::State& state) {
    for (auto _ : state)
    {
        auto now = std::chrono::system_clock::now();
        auto duration = now.time_since_epoch();
        benchmark::DoNotOptimize(duration);
    }
}


static void BM_calc_time_delta(benchmark::State& state) {
    time_t start_time = time(nullptr);
    for (auto _ : state)
    {
        auto tdelta = time(nullptr) - start_time;
        benchmark::DoNotOptimize(tdelta);
    }
}

static void BM_calc_chrono_delta(benchmark::State& state) {
    auto start = std::chrono::high_resolution_clock::now();
    for (auto _ : state)
    {
        auto tdelta = duration_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now() - start);
        benchmark::DoNotOptimize(tdelta);
    }
}

BENCHMARK(BM_calc_time);
BENCHMARK(BM_calc_chrono_time);
BENCHMARK(BM_calc_time_delta);
BENCHMARK(BM_calc_chrono_delta);

int main(int argc, char** argv)
{

    ::benchmark::Initialize(&argc, argv);
    ::benchmark::RunSpecifiedBenchmarks();
}
