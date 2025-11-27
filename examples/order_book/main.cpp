#include <iostream>
#include <fstream>
#include <vector>
#include "itch/parser.hpp"
#include "itch/order_book.hpp"

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <path_to_itch_file>" << std::endl;
        return 1;
    }

    std::ifstream file(argv[1], std::ios::binary);
    if (!file) {
        std::cerr << "Error opening file: " << argv[1] << std::endl;
        return 1;
    }

    // Read the entire file into a buffer
    file.seekg(0, std::ios::end);
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    if (size < 0) {
        std::cerr << "Error: file size is negative." << std::endl;
        return 1;
    }

    std::vector<char> buffer(static_cast<size_t>(size));
    if (!file.read(buffer.data(), size)) {
        std::cerr << "Error reading file." << std::endl;
        return 1;
    }

    itch::Parser parser;
    itch::LimitOrderBook order_book;

    parser.parse(buffer.data(), buffer.size(), [&](const itch::Message& msg) {
        order_book.process(msg);
    });

    order_book.print(std::cout);

    return 0;
}
