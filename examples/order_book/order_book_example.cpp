#include <chrono>
#include <fstream>
#include <iostream>
#include <vector>

#include "itch/order_book.hpp"
#include "itch/parser.hpp"


int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <itch_file>\n";
        return 1;
    }

    std::ifstream file(argv[1], std::ios::binary);
    if (!file) {
        std::cerr << "Error: Could not open file " << argv[1] << "\n";
        return 1;
    }

    itch::Parser         parser;
    itch::LimitOrderBook order_book;

    try {
        auto processor = [&](auto&& msg) { order_book.process(msg); };
        parser.parse(file, processor);
        order_book.print(std::cout);

    } catch (const std::exception& e) {
        std::cerr << "An error occurred: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
