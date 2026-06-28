#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <map>
#include <span>
#include <variant>
#include <vector>

#include "itch/parser.hpp"

// A golden/conformance test: a deterministic, hand-built ITCH stream with a known
// composition is parsed, and the per-type message counts and spot-checked field
// values are asserted against the known-good fixture. This gives an end-to-end
// "known input produces known output" check without committing a multi-gigabyte
// binary sample to the repository.
namespace {

class StreamBuilder {
   public:
    auto system_event(char event_code) -> void {
        std::vector<std::uint8_t> payload;
        payload.push_back(static_cast<std::uint8_t>('S'));
        header(payload, ++m_seq);
        payload.push_back(static_cast<std::uint8_t>(event_code));
        frame(payload);
    }

    auto add_order(std::uint64_t order_ref, char side, std::uint32_t shares, std::uint32_t price)
        -> void {
        std::vector<std::uint8_t> payload;
        payload.push_back(static_cast<std::uint8_t>('A'));
        header(payload, ++m_seq);
        u64(payload, order_ref);
        payload.push_back(static_cast<std::uint8_t>(side));
        u32(payload, shares);
        for (int idx = 0; idx < 8; ++idx) {
            payload.push_back(static_cast<std::uint8_t>("AAPL    "[idx]));
        }
        u32(payload, price);
        frame(payload);
    }

    auto order_executed(std::uint64_t order_ref, std::uint32_t executed, std::uint64_t match)
        -> void {
        std::vector<std::uint8_t> payload;
        payload.push_back(static_cast<std::uint8_t>('E'));
        header(payload, ++m_seq);
        u64(payload, order_ref);
        u32(payload, executed);
        u64(payload, match);
        frame(payload);
    }

    [[nodiscard]] auto span() const -> std::span<const std::byte> {
        return std::as_bytes(std::span {m_bytes});
    }

   private:
    static auto u16(std::vector<std::uint8_t>& out, std::uint16_t value) -> void {
        out.push_back(static_cast<std::uint8_t>(value >> 8));
        out.push_back(static_cast<std::uint8_t>(value & 0xFF));
    }
    static auto u32(std::vector<std::uint8_t>& out, std::uint32_t value) -> void {
        for (int shift = 24; shift >= 0; shift -= 8) {
            out.push_back(static_cast<std::uint8_t>((value >> shift) & 0xFF));
        }
    }
    static auto u48(std::vector<std::uint8_t>& out, std::uint64_t value) -> void {
        for (int shift = 40; shift >= 0; shift -= 8) {
            out.push_back(static_cast<std::uint8_t>((value >> shift) & 0xFF));
        }
    }
    static auto u64(std::vector<std::uint8_t>& out, std::uint64_t value) -> void {
        for (int shift = 56; shift >= 0; shift -= 8) {
            out.push_back(static_cast<std::uint8_t>((value >> shift) & 0xFF));
        }
    }
    static auto header(std::vector<std::uint8_t>& out, std::uint16_t seq) -> void {
        u16(out, seq);  // stock_locate
        u16(out, 0);    // tracking_number
        u48(out, seq);  // timestamp
    }
    auto frame(const std::vector<std::uint8_t>& payload) -> void {
        const auto length = static_cast<std::uint16_t>(payload.size());
        m_bytes.push_back(static_cast<std::uint8_t>(length >> 8));
        m_bytes.push_back(static_cast<std::uint8_t>(length & 0xFF));
        m_bytes.insert(m_bytes.end(), payload.begin(), payload.end());
    }

    std::vector<std::uint8_t> m_bytes;
    std::uint16_t             m_seq {0};
};

}  // namespace

TEST(Conformance, KnownStreamProducesKnownCountsAndFields) {
    StreamBuilder builder;
    // Known composition: 3 system events, 5 add orders, 2 executions.
    builder.system_event('O');
    builder.add_order(10, 'B', 100, 5000);
    builder.add_order(11, 'B', 200, 4900);
    builder.system_event('S');
    builder.add_order(12, 'S', 150, 5100);
    builder.add_order(13, 'S', 50, 5200);
    builder.order_executed(10, 40, 1);
    builder.add_order(14, 'B', 300, 4800);
    builder.order_executed(12, 150, 2);
    builder.system_event('C');

    itch::Parser parser;
    auto         messages = parser.parse(builder.span());

    std::map<char, int> counts;
    for (const auto& message : messages) {
        const char type = std::visit([](auto&& msg) { return msg.message_type; }, message);
        ++counts[type];
    }

    EXPECT_EQ(messages.size(), 10u);
    EXPECT_EQ(counts['S'], 3);
    EXPECT_EQ(counts['A'], 5);
    EXPECT_EQ(counts['E'], 2);
    EXPECT_EQ(parser.unknown_message_count(), 0u);
    EXPECT_EQ(parser.malformed_message_count(), 0u);

    // Spot-check fields of the first add order.
    const auto& first_add = std::get<itch::AddOrderMessage>(messages[1]);
    EXPECT_EQ(first_add.order_reference_number, 10u);
    EXPECT_EQ(first_add.buy_sell_indicator, 'B');
    EXPECT_EQ(first_add.shares, 100u);
    EXPECT_EQ(first_add.price, 5000u);
    EXPECT_EQ(itch::to_string(first_add.stock, 8), "AAPL");
}
