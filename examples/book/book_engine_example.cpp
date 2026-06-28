#include <cstdint>
#include <fstream>
#include <print>
#include <vector>

#include "itch/book/book_manager.hpp"
#include "itch/parser.hpp"

// Reconstructs every symbol's order book in one pass over an ITCH file using the
// multi-symbol BookManager, printing best-bid/offer changes and the trade tape.
auto main(int argc, char* argv[]) -> int {
    if (argc < 2) {
        std::println(stderr, "Usage: {} <itch_file> [symbol]", argv[0]);
        return 1;
    }

    std::ifstream file {argv[1], std::ios::binary};
    if (!file) {
        std::println(stderr, "Error: cannot open '{}'.", argv[1]);
        return 1;
    }
    std::vector<char> buffer {std::istreambuf_iterator<char> {file}, {}};

    itch::book::BookManager manager;
    if (argc >= 3) {
        manager.track_symbol(argv[2]);  // Restrict to one symbol when requested.
    }

    std::uint64_t bbo_updates = 0;
    std::uint64_t trades      = 0;
    manager.set_bbo_callback([&](const itch::book::L3Book&, const itch::book::Bbo&) {
        ++bbo_updates;
    });
    manager.set_trade_callback([&](const itch::Trade&) { ++trades; });

    itch::Parser parser;
    parser.parse(buffer.data(), buffer.size(), [&](const itch::Message& msg) {
        manager.process(msg);
    });

    std::println(
        "Reconstructed {} books, {} BBO changes, {} trades.",
        manager.book_count(),
        bbo_updates,
        trades
    );
    return 0;
}
