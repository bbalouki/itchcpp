#include <chrono>
#include <fstream>
#include <iostream>
#include <vector>

#include "itch/parser.hpp"

namespace dt = std::chrono;

void print_message(const itch::Message& msg) {
    std::visit([](const auto& arg) { std::cout << arg << '\n'; }, msg);
}

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

    itch::Parser parser;

    try {
        // USAGE EXAMPLE 1:
        std::cout << "========================\n";
        std::cout << "Parsing with a callback\n";
        std::cout << "========================\n";

        parser.parse(file, print_message);

        // Reset file stream to the beginning for the next example
        file.clear();
        file.seekg(0);

        // USAGE EXAMPLE 2:
        std::cout << "=====================================\n";
        std::cout << "Collecting all messages into a vector\n";
        std::cout << "======================================\n";

        auto start_time   = dt::steady_clock::now();
        auto all_messages = parser.parse(file);
        std::cout << "Total messages: " << all_messages.size() << "\n";

        auto end_time    = dt::steady_clock::now();
        auto duration    = end_time - start_time;
        auto duration_ms = dt::duration_cast<dt::milliseconds>(duration);
        std::cout << "Total time: " << duration_ms.count() << " milliseconds.\n";

        // Reset file stream again
        file.clear();
        file.seekg(0);

        // USAGE EXAMPLE 3:
        std::cout << "===================================================\n";
        std::cout << "Filtering for Add ('A') and Executed ('E') messages\n ";
        std::cout << "===================================================\n";
        std::vector<char> filter            = {'A', 'E'};
        auto              filtered_messages = parser.parse(file, filter);
        std::cout << "Filtered messages collected: " << filtered_messages.size() << "\n";
        for (const auto& msg : filtered_messages) {
            std::visit([](const auto& arg) { std::cout << "  - " << arg << '\n'; }, msg);
        }

    } catch (const std::exception& e) {
        std::cerr << "An error occurred: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
