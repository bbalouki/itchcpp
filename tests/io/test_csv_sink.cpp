#include <gtest/gtest.h>

#include <span>
#include <sstream>
#include <string>
#include <vector>

#include "itch/io/csv_sink.hpp"
#include "itch/parser.hpp"
#include "transport/frame_builders.hpp"

namespace {

auto build_buffer(const std::vector<std::vector<std::byte>>& payloads) -> std::vector<std::byte> {
    std::vector<std::byte> buffer;
    for (const auto& payload : payloads) {
        const auto frame = itch::test::length_prefixed(payload);
        buffer.insert(buffer.end(), frame.begin(), frame.end());
    }
    return buffer;
}

}  // namespace

TEST(CsvSink, WritesHeaderAndNormalizedRows) {
    const auto buffer = build_buffer({
        itch::test::system_event_payload(1000, 'O'),
        itch::test::add_order_payload(7, 42, 'B', 500, "AAPL", 1500000),
    });

    std::ostringstream out;
    itch::io::CsvSink  sink {out};
    itch::Parser       parser;
    parser.parse(std::span<const std::byte> {buffer}, [&](const itch::Message& msg) {
        sink.write(msg);
    });
    sink.flush();

    const std::string text = out.str();
    EXPECT_NE(text.find("message_type,timestamp,stock_locate"), std::string::npos);
    // System event row carries its event code in the extra column.
    EXPECT_NE(text.find("event=O"), std::string::npos);
    // Add order row carries symbol, side, shares, and a 4-decimal price.
    EXPECT_NE(text.find("AAPL,42,B,500,150.0000"), std::string::npos);
    EXPECT_EQ(sink.rows_written(), 2U);
}
