#include <fstream>
#include <iostream>
#include <vector>

#include "parser.hpp"

void print_message(const itch::Message& msg) {
    std::visit([](const auto& arg) { std::cout << arg << '\n'; }, msg);
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <itch_file>" << '\n';
        return 1;
    }

    std::ifstream file(argv[1], std::ios::binary);
    if (!file) {
        std::cerr << "Error opening file: " << argv[1] << '\n';
        return 1;
    }

    itch::Parser parser;
    try {
        // Example using the callback-based parse method
        parser.parse(file, print_message);

        // Or, to get all messages at once:
        // file.clear();
        // file.seekg(0);
        // auto messages = parser.parse(file);
        // for (const auto& msg : messages) {
        //     print_message(msg);
        // }

    } catch (const std::exception& e) {
        std::cerr << "Error parsing file: " << e.what() << '\n';
        return 1;
    }

    return 0;
}
