#include <cstdint>
#include <format>
#include <fstream>
#include <iostream>
#include <vector>

#include "itch/book/book_manager.hpp"
#include "itch/parser.hpp"

// Reconstructs every symbol's order book in one pass over an ITCH file using the
// multi-symbol BookManager, counting best-bid/offer changes and trades.
auto main(int argc, char* argv[]) -> int {
    if (argc < 2) {
        std::cerr << std::format("Usage: {} <itch_file> [symbol]\n", argv[0]);
        return 1;
    }

    std::ifstream file {argv[1], std::ios::binary};
    if (!file) {
        std::cerr << std::format("Error: cannot open '{}'.\n", argv[1]);
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

    std::cout << std::format(
        "Reconstructed {} books, {} BBO changes, {} trades.\n",
        manager.book_count(),
        bbo_updates,
        trades
    );
    return 0;
}
