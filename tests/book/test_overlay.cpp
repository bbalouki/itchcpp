#include <gtest/gtest.h>

#include <span>
#include <vector>

#include "itch/messages.hpp"
#include "itch/overlay.hpp"
#include "itch/parser.hpp"
#include "transport/frame_builders.hpp"

namespace {

// Concatenates several length-prefixed frames into one buffer.
auto build_buffer(const std::vector<std::vector<std::byte>>& payloads) -> std::vector<std::byte> {
    std::vector<std::byte> buffer;
    for (const auto& payload : payloads) {
        const auto frame = itch::test::length_prefixed(payload);
        buffer.insert(buffer.end(), frame.begin(), frame.end());
    }
    return buffer;
}

}  // namespace

TEST(Overlay, ViewFieldsMatchEagerDecode) {
    const auto payload = itch::test::add_order_payload(7, 42, 'B', 500, "AAPL", 1500000);
    const auto buffer  = build_buffer({payload});

    // Eager decode for the reference values.
    itch::Parser               parser;
    std::vector<itch::Message> messages = parser.parse(std::span<const std::byte> {buffer});
    ASSERT_EQ(messages.size(), 1U);
    const auto& eager = std::get<itch::AddOrderMessage>(messages.front());

    // Lazy overlay decode of the same bytes.
    itch::overlay::AddOrderView view {};
    std::uint64_t               count = itch::overlay::for_each_message(
        std::span<const std::byte> {buffer},
        [&](const itch::overlay::MessageView& generic) {
            view = itch::overlay::AddOrderView {generic.data(), generic.size()};
        }
    );
    ASSERT_EQ(count, 1U);

    EXPECT_EQ(view.type(), 'A');
    EXPECT_EQ(view.stock_locate(), eager.stock_locate);
    EXPECT_EQ(view.timestamp(), eager.timestamp);
    EXPECT_EQ(view.order_reference_number(), eager.order_reference_number);
    EXPECT_EQ(view.buy_sell_indicator(), eager.buy_sell_indicator);
    EXPECT_EQ(view.shares(), eager.shares);
    EXPECT_EQ(view.price(), eager.price);
    EXPECT_EQ(itch::to_string(view.stock().data(), view.stock().size()), "AAPL");
}

TEST(Overlay, FramesMultipleMessagesAndSkipsUnknownTypes) {
    std::vector<std::vector<std::byte>> payloads = {
        itch::test::add_order_payload(1, 1, 'B', 100, "AAA", 1000000),
        itch::test::system_event_payload(5000, 'O'),
        itch::test::add_order_payload(1, 2, 'S', 200, "AAA", 1000100),
    };
    const auto buffer = build_buffer(payloads);

    std::uint64_t adds  = 0;
    const auto    total = itch::overlay::for_each_message(
        std::span<const std::byte> {buffer},
        [&](const itch::overlay::MessageView& view) {
            if (view.type() == 'A') {
                ++adds;
            }
        }
    );
    EXPECT_EQ(total, 3U);
    EXPECT_EQ(adds, 2U);
}

TEST(Overlay, UndersizedFrameIsSkipped) {
    // A frame claiming type 'A' but only 5 bytes long must not be delivered.
    std::vector<std::byte> payload = {
        std::byte {'A'}, std::byte {0}, std::byte {0}, std::byte {0}, std::byte {0}
    };
    const auto buffer = build_buffer({payload});
    const auto total =
        itch::overlay::for_each_message(std::span<const std::byte> {buffer}, [](const auto&) {});
    EXPECT_EQ(total, 0U);
}
