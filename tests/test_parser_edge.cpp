#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "itch/parser.hpp"

namespace {

// Minimal big-endian wire-frame writer mirroring the ITCH layout the parser
// expects: a 2-byte big-endian length prefix followed by the payload, whose
// header is type(1) + stock_locate(2) + tracking_number(2) + 48-bit timestamp.
class FrameWriter {
   public:
    auto add_system_event(
        std::uint16_t locate, std::uint16_t tracking, std::uint64_t timestamp, char event_code
    ) -> void {
        std::vector<std::uint8_t> payload;
        payload.push_back(static_cast<std::uint8_t>('S'));
        append_u16(payload, locate);
        append_u16(payload, tracking);
        append_u48(payload, timestamp);
        payload.push_back(static_cast<std::uint8_t>(event_code));
        append_frame(payload);
    }

    auto add_order(
        std::uint16_t locate,
        std::uint64_t order_ref,
        char          side,
        std::uint32_t shares,
        const char (&stock)[9],
        std::uint32_t price
    ) -> void {
        std::vector<std::uint8_t> payload;
        payload.push_back(static_cast<std::uint8_t>('A'));
        append_u16(payload, locate);
        append_u16(payload, 0);
        append_u48(payload, 0);
        append_u64(payload, order_ref);
        payload.push_back(static_cast<std::uint8_t>(side));
        append_u32(payload, shares);
        for (int idx = 0; idx < 8; ++idx) {
            payload.push_back(static_cast<std::uint8_t>(stock[idx]));
        }
        append_u32(payload, price);
        append_frame(payload);
    }

    // Writes a frame with a caller-chosen (possibly wrong) declared length.
    auto add_raw_frame(std::uint16_t declared_length, const std::vector<std::uint8_t>& payload)
        -> void {
        m_bytes.push_back(static_cast<std::uint8_t>(declared_length >> 8));
        m_bytes.push_back(static_cast<std::uint8_t>(declared_length & 0xFF));
        m_bytes.insert(m_bytes.end(), payload.begin(), payload.end());
    }

    [[nodiscard]] auto bytes() const -> const std::vector<std::uint8_t>& { return m_bytes; }

    [[nodiscard]] auto as_byte_span() const -> std::span<const std::byte> {
        return std::as_bytes(std::span {m_bytes});
    }

   private:
    static auto append_u16(std::vector<std::uint8_t>& out, std::uint16_t value) -> void {
        out.push_back(static_cast<std::uint8_t>(value >> 8));
        out.push_back(static_cast<std::uint8_t>(value & 0xFF));
    }
    static auto append_u32(std::vector<std::uint8_t>& out, std::uint32_t value) -> void {
        for (int shift = 24; shift >= 0; shift -= 8) {
            out.push_back(static_cast<std::uint8_t>((value >> shift) & 0xFF));
        }
    }
    static auto append_u48(std::vector<std::uint8_t>& out, std::uint64_t value) -> void {
        for (int shift = 40; shift >= 0; shift -= 8) {
            out.push_back(static_cast<std::uint8_t>((value >> shift) & 0xFF));
        }
    }
    static auto append_u64(std::vector<std::uint8_t>& out, std::uint64_t value) -> void {
        for (int shift = 56; shift >= 0; shift -= 8) {
            out.push_back(static_cast<std::uint8_t>((value >> shift) & 0xFF));
        }
    }
    auto append_frame(const std::vector<std::uint8_t>& payload) -> void {
        add_raw_frame(static_cast<std::uint16_t>(payload.size()), payload);
    }

    std::vector<std::uint8_t> m_bytes;
};

constexpr char STOCK_AAPL[9] = "AAPL    ";

}  // namespace

TEST(ParserEdge, EndiannessRoundTrip) {
    EXPECT_EQ(itch::utils::swap_bytes<std::uint16_t>(0x1234), 0x3412);
    EXPECT_EQ(itch::utils::swap_bytes<std::uint32_t>(0x11223344), 0x44332211u);
    EXPECT_EQ(itch::utils::swap_bytes<std::uint64_t>(0x0102030405060708ull), 0x0807060504030201ull);

    // Swapping twice is the identity.
    const std::uint32_t value = 0xDEADBEEF;
    EXPECT_EQ(itch::utils::swap_bytes(itch::utils::swap_bytes(value)), value);

    // A single byte is unchanged.
    EXPECT_EQ(itch::utils::swap_bytes<std::uint8_t>(0x7F), 0x7F);
}

TEST(ParserEdge, SpanOverloadParsesIdenticallyToPointer) {
    FrameWriter writer;
    writer.add_system_event(1, 2, 3, 'O');
    writer.add_order(1, 42, 'B', 100, STOCK_AAPL, 5000);

    itch::Parser parser;
    auto         via_span = parser.parse(writer.as_byte_span());
    ASSERT_EQ(via_span.size(), 2u);
    EXPECT_TRUE(std::holds_alternative<itch::SystemEventMessage>(via_span[0]));
    EXPECT_TRUE(std::holds_alternative<itch::AddOrderMessage>(via_span[1]));
}

TEST(ParserEdge, ZeroLengthFramesAreSkipped) {
    FrameWriter writer;
    writer.add_system_event(1, 2, 3, 'O');
    writer.add_raw_frame(0, {});  // Zero-length padding frame.
    writer.add_system_event(4, 5, 6, 'C');

    itch::Parser parser;
    auto         messages = parser.parse(writer.as_byte_span());
    EXPECT_EQ(messages.size(), 2u);
}

TEST(ParserEdge, UnknownTypeIsSkippedAndCounted) {
    FrameWriter writer;
    writer.add_system_event(1, 2, 3, 'O');
    // A well-formed-looking frame whose type byte '?' is not a known message.
    writer.add_raw_frame(4, {static_cast<std::uint8_t>('?'), 0, 0, 0});
    writer.add_system_event(4, 5, 6, 'C');

    itch::Parser parser;
    int          seen = 0;
    parser.parse(writer.as_byte_span(), [&](const itch::Message&) { ++seen; });

    EXPECT_EQ(seen, 2);
    EXPECT_EQ(parser.unknown_message_count(), 1u);
    EXPECT_EQ(parser.malformed_message_count(), 0u);
}

TEST(ParserEdge, UndersizedFrameIsRejectedNotDecodedIntoNextFrame) {
    FrameWriter writer;
    // A frame claiming type 'S' (needs 12 bytes) but only declaring 5: it must be
    // rejected as malformed rather than reading into the following frame.
    writer.add_raw_frame(5, {static_cast<std::uint8_t>('S'), 0, 0, 0, 0});
    writer.add_system_event(7, 8, 9, 'C');

    itch::Parser parser;
    auto         messages = parser.parse(writer.as_byte_span());

    ASSERT_EQ(messages.size(), 1u);
    ASSERT_TRUE(std::holds_alternative<itch::SystemEventMessage>(messages[0]));
    EXPECT_EQ(std::get<itch::SystemEventMessage>(messages[0]).stock_locate, 7);
    EXPECT_EQ(parser.malformed_message_count(), 1u);
}

TEST(ParserEdge, ErrorCallbackReceivesEachProblem) {
    FrameWriter writer;
    writer.add_raw_frame(4, {static_cast<std::uint8_t>('?'), 0, 0, 0});     // unknown
    writer.add_raw_frame(5, {static_cast<std::uint8_t>('S'), 0, 0, 0, 0});  // undersized

    itch::Parser                  parser;
    std::vector<itch::ParseError> errors;
    parser.set_error_callback([&](itch::ParseError error, char) { errors.push_back(error); });
    parser.parse(writer.as_byte_span(), [](const itch::Message&) {});

    ASSERT_EQ(errors.size(), 2u);
    EXPECT_EQ(errors[0], itch::ParseError::unknown_type);
    EXPECT_EQ(errors[1], itch::ParseError::size_mismatch);
}

TEST(ParserEdge, ThrowsOnTruncatedPayload) {
    FrameWriter writer;
    writer.add_system_event(1, 2, 3, 'O');
    // Declare a 12-byte frame but provide only 4 payload bytes.
    writer.add_raw_frame(12, {static_cast<std::uint8_t>('S'), 0, 0, 0});

    itch::Parser parser;
    EXPECT_THROW(parser.parse(writer.as_byte_span()), std::runtime_error);
}

#if defined(__cpp_lib_expected)
TEST(ParserEdge, TryParseReturnsTruncatedInsteadOfThrowing) {
    FrameWriter writer;
    writer.add_system_event(1, 2, 3, 'O');
    writer.add_raw_frame(12, {static_cast<std::uint8_t>('S'), 0, 0, 0});  // truncated

    itch::Parser parser;
    int          seen = 0;
    auto result = parser.try_parse(writer.as_byte_span(), [&](const itch::Message&) { ++seen; });

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), itch::ParseError::truncated);
    EXPECT_EQ(seen, 1);  // The first, complete frame was still delivered.
}

TEST(ParserEdge, TryParseSucceedsOnWellFormedInput) {
    FrameWriter writer;
    writer.add_system_event(1, 2, 3, 'O');
    writer.add_order(1, 42, 'B', 100, STOCK_AAPL, 5000);

    itch::Parser parser;
    auto         result = parser.try_parse(writer.as_byte_span());

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->size(), 2u);
}
#endif
