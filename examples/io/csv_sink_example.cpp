#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

#include "itch/io/csv_sink.hpp"
#include "itch/messages.hpp"

namespace {

// ITCH text fields are fixed-width and space-padded on the wire, not
// null-terminated C strings, so a plain string-literal designated initializer
// cannot fill them (there is no room for the implicit '\0'). This copies
// `value` in and pads the remainder with spaces instead.
template <std::size_t Size>
auto fill_field(char (&dest)[Size], std::string_view value) -> void {
    std::ranges::fill(dest, ' ');
    std::ranges::copy(value, dest);
}

auto make_sample_messages() -> std::vector<itch::Message> {
    const itch::SystemEventMessage system_event {
        .stock_locate    = 0,
        .tracking_number = 1,
        .timestamp       = 34'200'000'000'000ULL,
        .event_code      = 'O',
    };

    itch::AddOrderMessage add_order {
        .stock_locate           = 1,
        .tracking_number        = 1,
        .timestamp              = 34'200'000'500'000ULL,
        .order_reference_number = 1001,
        .buy_sell_indicator     = 'B',
        .shares                 = 100,
        .stock                  = {},
        .price                  = 1500000,
    };
    fill_field(add_order.stock, "AAPL");

    itch::StockTradingActionMessage trading_action {
        .stock_locate    = 1,
        .tracking_number = 1,
        .timestamp       = 34'200'001'000'000ULL,
        .stock           = {},
        .trading_state   = 'T',
        .reserved        = ' ',
        .reason          = {},
    };
    fill_field(trading_action.stock, "AAPL");
    fill_field(trading_action.reason, "T");

    return {system_event, add_order, trading_action};
}

}  // namespace

// Demonstrates CsvSink as a concrete itch::io::MessageSink: every one of the 23
// heterogeneous ITCH message structs flattens into the same wide CSV row schema,
// so downstream tooling never has to special-case which type it received.
auto main() -> int {
    const std::vector<itch::Message> messages = make_sample_messages();

    const auto csv_path = std::filesystem::temp_directory_path() / "itchcpp_csv_sink_example.csv";
    std::ofstream out_file {csv_path};

    itch::io::CsvSink      sink {out_file};
    itch::io::MessageSink& generic_sink = sink;  // exercise through the abstract interface
    for (const auto& message : messages) {
        generic_sink.write(message);
    }
    generic_sink.flush();
    out_file.close();

    std::cout << std::format("Wrote {} CSV rows to {}\n", sink.rows_written(), csv_path.string());

    std::ifstream in_file {csv_path};
    std::string   line;
    while (std::getline(in_file, line)) {
        std::cout << line << '\n';
    }
    return 0;
}
