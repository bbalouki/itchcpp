/**
 * @file bench_main.cpp
 * @brief Benchmarking suite for the ITCH message parser using Google Benchmark.
 *
 * This file defines a set of benchmarks to evaluate the performance of the
 * ITCH message parser implemented in the Parser class. It uses the Google
 * Benchmark library to measure execution time and throughput for different
 * parsing strategies, including callback-based parsing, collecting all parsed
 * messages, and filtering specific message types.
 *
 * The benchmark fixture `ParserBenchmark` is responsible for loading the ITCH
 * data file into memory once per benchmark run, ensuring that file I/O does
 * not skew the parsing performance measurements.
 *
 * Usage:
 *   ./benchmark.exe <path_to_itch_data_file> [google benchmark options]
 *
 * Example:
 *   ./benchmark.exe data/itch_data.bin --benchmark_filter=BM_ParseWithCallback
 *
 * Note:
 *   Ensure that the Google Benchmark library is properly linked during
 *   compilation.
 */

#include <benchmark/benchmark.h>

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "parser.hpp"

std::string g_data_filename;

// A benchmark fixture to load the specified data file once.
class ParserBenchmark : public benchmark::Fixture {
   public:
    std::vector<char> itch_data;

    void SetUp(::benchmark::State& state) override {
        if (g_data_filename.empty()) {
            state.SkipWithError(
                "ITCH data file not provided. Pass the file path as a "
                "command-line argument.");
            return;
        }

        std::ifstream file(g_data_filename, std::ios::binary);
        if (!file) {
            state.SkipWithError(("Failed to open ITCH data file: " + g_data_filename).c_str());
            return;
        }

        file.seekg(0, std::ios::end);
        size_t size = file.tellg();
        file.seekg(0, std::ios::beg);

        itch_data.resize(size);
        file.read(itch_data.data(), size);

        // Report the size of the data being processed.
        state.SetBytesProcessed(0);  // Clear any previous settings
        state.counters["FileSizeMB"] =
            benchmark::Counter(size / (1024.0 * 1024.0), benchmark::Counter::kIsIterationInvariant);
    }
    void TearDown(const ::benchmark::State& state) override {
        // Clear the memory.
        itch_data.clear();
        itch_data.shrink_to_fit();
    }
};

BENCHMARK_F(ParserBenchmark, BM_ParseWithCallback)(benchmark::State& state) {
    itch::Parser parser;
    size_t       total_bytes = 0;
    for (auto _ : state) {
        size_t message_count = 0;
        parser.parse(itch_data.data(), itch_data.size(),
                     [&](const itch::Message& msg) { benchmark::DoNotOptimize(++message_count); });
        total_bytes += itch_data.size();
    }
    // Report throughput in MB/s
    state.SetBytesProcessed(total_bytes);
}

BENCHMARK_F(ParserBenchmark, BM_ParseAndCollectAll)(benchmark::State& state) {
    itch::Parser parser;
    size_t       total_bytes = 0;
    for (auto _ : state) {
        auto messages = parser.parse(itch_data.data(), itch_data.size());
        benchmark::DoNotOptimize(messages.data());
        total_bytes += itch_data.size();
    }
    state.SetBytesProcessed(total_bytes);
}

BENCHMARK_F(ParserBenchmark, BM_ParseAndFilter)(benchmark::State& state) {
    itch::Parser parser;
    size_t       total_bytes = 0;
    for (auto _ : state) {
        auto messages = parser.parse(itch_data.data(), itch_data.size(), {'A', 'P', 'E', 'C', 'X'});
        benchmark::DoNotOptimize(messages.data());
        total_bytes += itch_data.size();
    }
    state.SetBytesProcessed(total_bytes);
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: ./benchmark.exe <path_to_itch_data_file> [google "
                     "benchmark options]\n";
        return 1;
    }
    g_data_filename = argv[1];

    // Manually remove the filename argument from the list before passing to
    // benchmark::Initialize. We do this by shifting all subsequent arguments
    // one position to the left.
    for (int i = 1; i < argc - 1; ++i) {
        argv[i] = argv[i + 1];
    }
    // Decrement the argument count to reflect the removal of our custom
    // argument.
    argc--;

    // Initialize Google Benchmark, passing it the remaining arguments.
    // Note: We "consume" the first two arguments (executable name and filename)
    // so Google Benchmark doesn't try to parse them.
    benchmark::Initialize(&argc, argv);

    // Check if any benchmarks are targeted to run. If not, we might want to
    // return.
    if (benchmark::ReportUnrecognizedArguments(argc, argv)) return 1;

    benchmark::RunSpecifiedBenchmarks();
    benchmark::Shutdown();
    return 0;
}
