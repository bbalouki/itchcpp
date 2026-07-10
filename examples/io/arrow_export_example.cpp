#include <format>
#include <iostream>

#ifdef ITCH_WITH_ARROW
#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <string_view>
#include <vector>

#include "itch/io/arrow_export.hpp"
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

}  // namespace
#endif  // ITCH_WITH_ARROW

// Demonstrates ArrowExporter turning a handful of synthesized messages into a
// Parquet file, the columnar hand-off point for pandas/Polars/DuckDB/Spark
// pipelines. Only meaningful when the library is built with
// -DITCH_WITH_ARROW=ON; otherwise this file still compiles but does nothing.
auto main() -> int {
#ifdef ITCH_WITH_ARROW
    const itch::SystemEventMessage system_event {
        .stock_locate    = 0,
        .tracking_number = 1,
        .timestamp       = 34'200'000'000'000ULL,
        .event_code      = 'O',
    };

    itch::AddOrderMessage buy_order {
        .stock_locate           = 1,
        .tracking_number        = 1,
        .timestamp              = 34'200'000'500'000ULL,
        .order_reference_number = 1001,
        .buy_sell_indicator     = 'B',
        .shares                 = 100,
        .stock                  = {},
        .price                  = 1500000,
    };
    fill_field(buy_order.stock, "AAPL");

    itch::AddOrderMessage sell_order {
        .stock_locate           = 1,
        .tracking_number        = 1,
        .timestamp              = 34'200'001'000'000ULL,
        .order_reference_number = 1002,
        .buy_sell_indicator     = 'S',
        .shares                 = 200,
        .stock                  = {},
        .price                  = 1500500,
    };
    fill_field(sell_order.stock, "AAPL");

    const std::vector<itch::Message> messages {system_event, buy_order, sell_order};

    itch::io::ArrowExporter exporter;
    for (const auto& message : messages) {
        exporter.append(message);
    }

    const auto parquet_path =
        std::filesystem::temp_directory_path() / "itchcpp_arrow_export_example.parquet";
    const bool wrote_ok = exporter.write_parquet(parquet_path.string());
    if (!wrote_ok) {
        std::cerr << std::format("Failed to write Parquet file: {}\n", exporter.error());
        return 1;
    }

    std::cout << std::format(
        "Appended {} rows and wrote Parquet file to {}\n", exporter.rows(), parquet_path.string()
    );
#else
    std::cout << "This example requires -DITCH_WITH_ARROW=ON\n";
#endif  // ITCH_WITH_ARROW
    return 0;
}
