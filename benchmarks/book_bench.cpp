/**
 * @file book_bench.cpp
 * @brief Benchmarks for the Phase 2 book engine and the zero-copy overlay.
 *
 * Measures three things against an in-memory ITCH buffer (file I/O excluded):
 *  - BM_BookRebuild: full multi-symbol book reconstruction through BookManager.
 *  - BM_EagerTouch: eager parse touching one field per message (the baseline).
 *  - BM_OverlayTouch: lazy overlay framing touching the same one field, which
 *    should be cheaper because the other fields are never decoded.
 *
 * Usage:
 *   ./book_bench <path_to_itch_data_file> [google benchmark options]
 */

#include <benchmark/benchmark.h>

#include <fstream>
#include <iostream>
#include <span>
#include <string>
#include <vector>

#include "itch/book/book_manager.hpp"
#include "itch/overlay.hpp"
#include "itch/parser.hpp"

namespace data {
// NOLINTNEXTLINE
std::string g_data_filename {};
}  // namespace data

namespace {

auto load_file(const std::string& path) -> std::vector<std::byte> {
    std::ifstream file {path, std::ios::binary};
    file.seekg(0, std::ios::end);
    const auto size = file.tellg();
    file.seekg(0, std::ios::beg);
    std::vector<std::byte> buffer(static_cast<std::size_t>(size));
    file.read(reinterpret_cast<char*>(buffer.data()), size);  // NOLINT
    return buffer;
}

class BookBenchmark : public benchmark::Fixture {
   public:
    std::vector<std::byte> itch_data;

    void SetUp(::benchmark::State& state) override {
        if (data::g_data_filename.empty()) {
            state.SkipWithError("ITCH data file not provided.");
            return;
        }
        itch_data = load_file(data::g_data_filename);
        if (itch_data.empty()) {
            state.SkipWithError("Failed to read ITCH data file.");
        }
    }
    void TearDown(const ::benchmark::State&) override {
        itch_data.clear();
        itch_data.shrink_to_fit();
    }
};

}  // namespace

BENCHMARK_F(BookBenchmark, BM_BookRebuild)(benchmark::State& state) {
    std::size_t total_bytes = 0;
    for ([[maybe_unused]] auto iter : state) {
        itch::Parser            parser;
        itch::book::BookManager manager;
        parser.parse(std::span<const std::byte> {itch_data}, [&](const itch::Message& msg) {
            manager.process(msg);
        });
        benchmark::DoNotOptimize(manager.book_count());
        total_bytes += itch_data.size();
    }
    state.SetBytesProcessed(static_cast<std::int64_t>(total_bytes));
}

BENCHMARK_F(BookBenchmark, BM_EagerTouch)(benchmark::State& state) {
    std::size_t total_bytes = 0;
    for ([[maybe_unused]] auto iter : state) {
        itch::Parser  parser;
        std::uint64_t sink = 0;
        parser.parse(std::span<const std::byte> {itch_data}, [&](const itch::Message& msg) {
            sink += std::visit(
                [](const auto& inner) -> std::uint64_t { return inner.stock_locate; }, msg
            );
        });
        benchmark::DoNotOptimize(sink);
        total_bytes += itch_data.size();
    }
    state.SetBytesProcessed(static_cast<std::int64_t>(total_bytes));
}

BENCHMARK_F(BookBenchmark, BM_OverlayTouch)(benchmark::State& state) {
    std::size_t total_bytes = 0;
    for ([[maybe_unused]] auto iter : state) {
        std::uint64_t sink = 0;
        itch::overlay::for_each_message(
            std::span<const std::byte> {itch_data},
            [&](const itch::overlay::MessageView& view) { sink += view.stock_locate(); }
        );
        benchmark::DoNotOptimize(sink);
        total_bytes += itch_data.size();
    }
    state.SetBytesProcessed(static_cast<std::int64_t>(total_bytes));
}

auto main(int argc, char** argv) -> int {
    if (argc < 2) {
        std::cerr << "Usage: ./book_bench <path_to_itch_data_file> [benchmark options]\n";
        return 1;
    }
    data::g_data_filename = argv[1];
    for (int index = 1; index < argc - 1; ++index) {
        argv[index] = argv[index + 1];
    }
    --argc;
    benchmark::Initialize(&argc, argv);
    if (benchmark::ReportUnrecognizedArguments(argc, argv)) {
        return 1;
    }
    benchmark::RunSpecifiedBenchmarks();
    benchmark::Shutdown();
    return 0;
}
