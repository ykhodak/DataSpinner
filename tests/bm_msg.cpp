#include <iostream>
#include <benchmark/benchmark.h>
#include "socket_buffer.h"
#include "test_tu.h"
#include "md_ref.h"
#include "simulator.h"
#include "archive.h"

namespace spnr
{
    class BenchmarkTestVoidDeltaPublisher
    {
        /// don't serialize, only calculate size ///
    public:
        template<class T>
        bool publish(const T& m) {
            auto len = m.calc_len();
            benchmark::DoNotOptimize(len);
            return true;
        }
    };

    
}

static void BM_TextMsg_len(benchmark::State& state) {
    for (auto _ : state)
    {
        spnr::TextMsg m("test");
        auto len = m.calc_len();
        benchmark::DoNotOptimize(len);
    }
}

static void BM_ByteVecMsg_len(benchmark::State& state) {
    for (auto _ : state)
    {
        spnr::ByteVecMsg m;
        m.set_client_context("client_context");
        for (size_t n = 1; n <= 100; ++n)
        {
            if (!m.add_num(n))
                break;
        }        
        auto len = m.calc_len();
        benchmark::DoNotOptimize(len);
    }
}

static void BM_IntVecMsg_len(benchmark::State& state) {
    for (auto _ : state)
    {
        spnr::IntVecMsg m;
        m.set_client_context("client_context");
        for (size_t n = 1; n <= 100; ++n)
        {
            if (!m.add_num(n))
                break;
        }
        auto len = m.calc_len();
        benchmark::DoNotOptimize(len);
    }
}


static void BM_DoubleVecMsg_len(benchmark::State& state) {
    for (auto _ : state)
    {
        spnr::DoubleVecMsg m;
        m.set_client_context("client_context");
        for (size_t n = 1; n <= 100; ++n)
        {
            if (!m.add_num(n))
                break;
        }
        auto len = m.calc_len();
        benchmark::DoNotOptimize(len);
    }
}




spnr::BenchmarkTestVoidDeltaPublisher void_publisher;
spnr::BenchmarkTestBufferDeltaPublisher buffer_publisher;


spnr::MdDeltaBuilder<spnr::MdDelta, spnr::BenchmarkTestVoidDeltaPublisher> bld_vpub(void_publisher);
spnr::MdDeltaBuilder<spnr::MdDelta, spnr::BenchmarkTestBufferDeltaPublisher> bld_bpub(buffer_publisher);
std::vector<std::string> subs;
spnr::WireReaderStat pstat;


static void BM_IntBuilder_small_vpub(benchmark::State& state) {
    for (auto _ : state)
    {
        bld_vpub.begin("AAPL");
        for (int i = 1; i <= INT_DELTA_SIZE; i++) {
            bld_vpub.add_intdata(i, i * 10);
        }
        bld_vpub.end();
    }
}

static void BM_IntBuilder_small_bpub(benchmark::State& state) {
    for (auto _ : state)
    {
        bld_bpub.begin("AAPL");
        for (int i = 1; i <= INT_DELTA_SIZE; i++) {
            bld_bpub.add_intdata(i, i * 10);
        }
        bld_bpub.end();
    }
}


static void BM_DblBuilder_small_vpub(benchmark::State& state) {
    for (auto _ : state)
    {
        bld_vpub.begin("AAPL");
        for (int i = 1; i <= DBL_DELTA_SIZE; i++) {
            bld_vpub.add_dbldata(i, i * 100.1);
        }
        bld_vpub.end();
    }
}

static void BM_DblBuilder_small_bpub(benchmark::State& state) {
    for (auto _ : state)
    {
        bld_bpub.begin("AAPL");
        for (int i = 1; i <= DBL_DELTA_SIZE; i++) {
            bld_bpub.add_dbldata(i, i * 100.1);
        }
        bld_bpub.end();
    }
}

/*static void BM_StrBuilder_small_vpub(benchmark::State& state) {
    for (auto _ : state)
    {
        bld_vpub.begin("AAPL");
        for (int i = 1; i <= STR_DELTA_SIZE; i++) {
            std::string str = "-#" + std::to_string(i) + "-upd";
            bld_vpub.add_strdata(i, str);
        }
        bld_vpub.end();
    }
}

static void BM_StrBuilder_small_bpub(benchmark::State& state) {
    for (auto _ : state)
    {
        bld_bpub.begin("AAPL");
        for (int i = 1; i <= STR_DELTA_SIZE; i++) {
            std::string str = "-#" + std::to_string(i) + "-upd";
            bld_bpub.add_strdata(i, str);
        }
        bld_bpub.end();
    }
}

BENCHMARK(BM_StrBuilder_small_vpub);
BENCHMARK(BM_StrBuilder_small_bpub);
*/


static void BM_CompositeBuilder_vpub(benchmark::State& state) {
    int num = 20;
    for (auto _ : state)
    {
        bld_vpub.begin("AAPL");
        for (int i = 1; i <= num; i++) {
            bld_vpub.add_intdata(i, i * 10);
        }
        for (int i = 1; i <= num; i++) {
            bld_vpub.add_dbldata(i, i * 100.1);
        }
        /*for (int i = 1; i <= num; i++) {
            std::string str = "-#" + std::to_string(i) + "-upd";
            bld_vpub.add_strdata(i, str);
        }*/
        bld_vpub.end();
    }
}

static void BM_CompositeBuilder_bpub(benchmark::State& state) {
    int num = 20;
    for (auto _ : state)
    {
        bld_bpub.begin("AAPL");
        for (int i = 1; i <= num; i++) {
            bld_bpub.add_intdata(i, i * 10);
        }
        for (int i = 1; i <= num; i++) {
            bld_bpub.add_dbldata(i, i * 100.1);
        }
        /*for (int i = 1; i <= num; i++) {
            std::string str = "-#" + std::to_string(i) + "-upd";
            bld_bpub.add_strdata(i, str);
        }*/
        bld_bpub.end();
    }
}

static void BM_send_generated_delta_bpub(benchmark::State& state) {
    for (auto _ : state)
    {
        spnr::send_generated_delta<spnr::MdDelta, spnr::BenchmarkTestBufferDeltaPublisher>(subs, buffer_publisher, 20);
        //std::this_thread::sleep_for(std::chrono::nanoseconds(1));
    }
}

static void BM_calc_throughput(benchmark::State& state) {
    for (auto _ : state)
    {
        auto t = pstat.calc_reader_throughput();
        benchmark::DoNotOptimize(t);
    }
}

BENCHMARK(BM_TextMsg_len);
BENCHMARK(BM_ByteVecMsg_len);
BENCHMARK(BM_IntVecMsg_len);
BENCHMARK(BM_DoubleVecMsg_len);
BENCHMARK(BM_IntBuilder_small_vpub);
BENCHMARK(BM_IntBuilder_small_bpub);
BENCHMARK(BM_DblBuilder_small_vpub);
BENCHMARK(BM_DblBuilder_small_bpub);
BENCHMARK(BM_CompositeBuilder_vpub);
BENCHMARK(BM_CompositeBuilder_bpub);
BENCHMARK(BM_send_generated_delta_bpub);
BENCHMARK(BM_calc_throughput);

int main(int argc, char** argv)
{
    pstat.start_reader_stat();
    subs.push_back("AAPL");
    //subs.push_back("MSFT");

    ::benchmark::Initialize(&argc, argv);
    ::benchmark::RunSpecifiedBenchmarks();
}
