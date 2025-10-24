#include <chrono>
#include <fstream>
#include <iostream>
#include <vector>

#include "parser.hpp"

namespace dt = std::chrono;

void printe_message(const itch::Message& msg) {
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
        std::cout << "--- Parsing with a callback ---\n";
        parser.parse(file, printe_message);

        // // Reset file stream to the beginning for the next example
        // file.clear();
        // file.seekg(0);

        // USAGE EXAMPLE 2:
        // auto start_time = dt::steady_clock::now();
        // std::cout << "\n--- Collecting all messages into a vector ---\n";
        // auto all_messages = parser.parse(file);
        // std::cout << "Total messages collected: " << all_messages.size()
        //           << "\n";
        // auto end_time = dt::steady_clock::now();
        // auto duration = end_time - start_time;
        // auto duration_ms = dt::duration_cast<dt::milliseconds>(duration);
        // std::cout << "Total time taken: " << duration_ms.count()
        //           << " milliseconds.\n";

        // Reset file stream again
        // file.clear();
        // file.seekg(0);

        // USAGE EXAMPLE 3: 
        // std::cout << "\n--- Filtering for Add ('A') and Executed ('E') "
        //              "messages ---\n";
        // std::vector<char> filter = {'S', 'N'};
        // auto filtered_messages = parser.parse(file, filter);
        // std::cout << "Filtered messages collected: " << filtered_messages.size()
        //           << "\n";
        // for (const auto& msg : filtered_messages) {
        //     std::visit(
        //         [](const auto& arg) { std::cout << "  - " << arg << '\n'; },
        //         msg);
        // }

    } catch (const std::exception& e) {
        std::cerr << "An error occurred: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
