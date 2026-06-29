#include "itch/encoder.hpp"

#include <array>
#include <cstdint>
#include <cstring>
#include <variant>

#include "itch/parser.hpp"  // utils::from_big_endian

namespace itch {

namespace {

/// @brief Appends fields to a byte buffer in network (big-endian) order.
class ByteWriter {
   public:
    // Appends an integral field of width sizeof(IntType) in big-endian order.
    template <typename IntType>
    auto put(IntType value) -> void {
        const auto                             big = utils::from_big_endian(value);
        std::array<std::byte, sizeof(IntType)> bytes {};
        std::memcpy(bytes.data(), &big, sizeof(IntType));
        m_bytes.insert(m_bytes.end(), bytes.begin(), bytes.end());
    }

    auto put_char(char value) -> void { m_bytes.push_back(static_cast<std::byte>(value)); }

    // Appends a fixed-width character field exactly as stored (no trimming).
    auto put_chars(const char* source, std::size_t size) -> void {
        for (std::size_t index = 0; index < size; ++index) {
            m_bytes.push_back(static_cast<std::byte>(source[index]));
        }
    }

    // Appends the 48-bit ITCH timestamp (2-byte high, 4-byte low), big-endian.
    auto put_timestamp(std::uint64_t timestamp) -> void {
        constexpr int LOWER_SHIFT = 32;
        put<std::uint16_t>(static_cast<std::uint16_t>(timestamp >> LOWER_SHIFT));
        put<std::uint32_t>(static_cast<std::uint32_t>(timestamp & 0xFFFFFFFFULL));
    }

    [[nodiscard]] auto take() -> std::vector<std::byte> { return std::move(m_bytes); }

   private:
    std::vector<std::byte> m_bytes;
};

// Writes the header common to every message: type, locate, tracking, timestamp.
template <typename MsgType>
auto write_header(ByteWriter& writer, const MsgType& msg) -> void {
    writer.put_char(msg.message_type);
    writer.put<std::uint16_t>(msg.stock_locate);
    writer.put<std::uint16_t>(msg.tracking_number);
    writer.put_timestamp(msg.timestamp);
}

auto write_body(ByteWriter& writer, const SystemEventMessage& msg) -> void {
    writer.put_char(msg.event_code);
}

auto write_body(ByteWriter& writer, const StockDirectoryMessage& msg) -> void {
    writer.put_chars(msg.stock, STOCK_LEN);
    writer.put_char(msg.market_category);
    writer.put_char(msg.financial_status_indicator);
    writer.put<std::uint32_t>(msg.round_lot_size);
    writer.put_char(msg.round_lots_only);
    writer.put_char(msg.issue_classification);
    writer.put_chars(msg.issue_sub_type, 2);
    writer.put_char(msg.authenticity);
    writer.put_char(msg.short_sale_threshold_indicator);
    writer.put_char(msg.ipo_flag);
    writer.put_char(msg.luld_ref);
    writer.put_char(msg.etp_flag);
    writer.put<std::uint32_t>(msg.etp_leverage_factor);
    writer.put_char(msg.inverse_indicator);
}

auto write_body(ByteWriter& writer, const StockTradingActionMessage& msg) -> void {
    writer.put_chars(msg.stock, STOCK_LEN);
    writer.put_char(msg.trading_state);
    writer.put_char(msg.reserved);
    writer.put_chars(msg.reason, 4);
}

auto write_body(ByteWriter& writer, const RegSHOMessage& msg) -> void {
    writer.put_chars(msg.stock, STOCK_LEN);
    writer.put_char(msg.reg_sho_action);
}

auto write_body(ByteWriter& writer, const MarketParticipantPositionMessage& msg) -> void {
    writer.put_chars(msg.mpid, 4);
    writer.put_chars(msg.stock, STOCK_LEN);
    writer.put_char(msg.primary_market_maker);
    writer.put_char(msg.market_maker_mode);
    writer.put_char(msg.market_participant_state);
}

auto write_body(ByteWriter& writer, const MWCBDeclineLevelMessage& msg) -> void {
    writer.put<std::uint64_t>(msg.level1);
    writer.put<std::uint64_t>(msg.level2);
    writer.put<std::uint64_t>(msg.level3);
}

auto write_body(ByteWriter& writer, const MWCBStatusMessage& msg) -> void {
    writer.put_char(msg.breached_level);
}

auto write_body(ByteWriter& writer, const IPOQuotingPeriodUpdateMessage& msg) -> void {
    writer.put_chars(msg.stock, STOCK_LEN);
    writer.put<std::uint32_t>(msg.ipo_quotation_release_time);
    writer.put_char(msg.ipo_quotation_release_qualifier);
    writer.put<std::uint32_t>(msg.ipo_price);
}

auto write_body(ByteWriter& writer, const LULDAuctionCollarMessage& msg) -> void {
    writer.put_chars(msg.stock, STOCK_LEN);
    writer.put<std::uint32_t>(msg.auction_collar_reference_price);
    writer.put<std::uint32_t>(msg.upper_auction_collar_price);
    writer.put<std::uint32_t>(msg.lower_auction_collar_price);
    writer.put<std::uint32_t>(msg.auction_collar_extension);
}

auto write_body(ByteWriter& writer, const OperationalHaltMessage& msg) -> void {
    writer.put_chars(msg.stock, STOCK_LEN);
    writer.put_char(msg.market_code);
    writer.put_char(msg.operational_halt_action);
}

auto write_body(ByteWriter& writer, const AddOrderMessage& msg) -> void {
    writer.put<std::uint64_t>(msg.order_reference_number);
    writer.put_char(msg.buy_sell_indicator);
    writer.put<std::uint32_t>(msg.shares);
    writer.put_chars(msg.stock, STOCK_LEN);
    writer.put<std::uint32_t>(msg.price);
}

auto write_body(ByteWriter& writer, const AddOrderMPIDAttributionMessage& msg) -> void {
    writer.put<std::uint64_t>(msg.order_reference_number);
    writer.put_char(msg.buy_sell_indicator);
    writer.put<std::uint32_t>(msg.shares);
    writer.put_chars(msg.stock, STOCK_LEN);
    writer.put<std::uint32_t>(msg.price);
    writer.put_chars(msg.attribution, 4);
}

auto write_body(ByteWriter& writer, const OrderExecutedMessage& msg) -> void {
    writer.put<std::uint64_t>(msg.order_reference_number);
    writer.put<std::uint32_t>(msg.executed_shares);
    writer.put<std::uint64_t>(msg.match_number);
}

auto write_body(ByteWriter& writer, const OrderExecutedWithPriceMessage& msg) -> void {
    writer.put<std::uint64_t>(msg.order_reference_number);
    writer.put<std::uint32_t>(msg.executed_shares);
    writer.put<std::uint64_t>(msg.match_number);
    writer.put_char(msg.printable);
    writer.put<std::uint32_t>(msg.execution_price);
}

auto write_body(ByteWriter& writer, const OrderCancelMessage& msg) -> void {
    writer.put<std::uint64_t>(msg.order_reference_number);
    writer.put<std::uint32_t>(msg.cancelled_shares);
}

auto write_body(ByteWriter& writer, const OrderDeleteMessage& msg) -> void {
    writer.put<std::uint64_t>(msg.order_reference_number);
}

auto write_body(ByteWriter& writer, const OrderReplaceMessage& msg) -> void {
    writer.put<std::uint64_t>(msg.original_order_reference_number);
    writer.put<std::uint64_t>(msg.new_order_reference_number);
    writer.put<std::uint32_t>(msg.shares);
    writer.put<std::uint32_t>(msg.price);
}

auto write_body(ByteWriter& writer, const NonCrossTradeMessage& msg) -> void {
    writer.put<std::uint64_t>(msg.order_reference_number);
    writer.put_char(msg.buy_sell_indicator);
    writer.put<std::uint32_t>(msg.shares);
    writer.put_chars(msg.stock, STOCK_LEN);
    writer.put<std::uint32_t>(msg.price);
    writer.put<std::uint64_t>(msg.match_number);
}

auto write_body(ByteWriter& writer, const CrossTradeMessage& msg) -> void {
    writer.put<std::uint64_t>(msg.shares);
    writer.put_chars(msg.stock, STOCK_LEN);
    writer.put<std::uint32_t>(msg.cross_price);
    writer.put<std::uint64_t>(msg.match_number);
    writer.put_char(msg.cross_type);
}

auto write_body(ByteWriter& writer, const BrokenTradeMessage& msg) -> void {
    writer.put<std::uint64_t>(msg.match_number);
}

auto write_body(ByteWriter& writer, const NOIIMessage& msg) -> void {
    writer.put<std::uint64_t>(msg.paired_shares);
    writer.put<std::uint64_t>(msg.imbalance_shares);
    writer.put_char(msg.imbalance_direction);
    writer.put_chars(msg.stock, STOCK_LEN);
    writer.put<std::uint32_t>(msg.far_price);
    writer.put<std::uint32_t>(msg.near_price);
    writer.put<std::uint32_t>(msg.current_reference_price);
    writer.put_char(msg.cross_type);
    writer.put_char(msg.price_variation_indicator);
}

auto write_body(ByteWriter& writer, const RetailPriceImprovementIndicatorMessage& msg) -> void {
    writer.put_chars(msg.stock, STOCK_LEN);
    writer.put_char(msg.interest_flag);
}

auto write_body(ByteWriter& writer, const DLCRMessage& msg) -> void {
    writer.put_chars(msg.stock, STOCK_LEN);
    writer.put_char(msg.open_eligibility_status);
    writer.put<std::uint32_t>(msg.minimum_allowable_price);
    writer.put<std::uint32_t>(msg.maximum_allowable_price);
    writer.put<std::uint32_t>(msg.near_execution_price);
    writer.put<std::uint64_t>(msg.near_execution_time);
    writer.put<std::uint32_t>(msg.lower_price_range_collar);
    writer.put<std::uint32_t>(msg.upper_price_range_collar);
}

}  // namespace

auto encode_message(const Message& message) -> std::vector<std::byte> {
    ByteWriter writer;
    std::visit(
        [&writer](const auto& concrete) {
            write_header(writer, concrete);
            write_body(writer, concrete);
        },
        message
    );
    return writer.take();
}

auto encode_frame(const Message& message) -> std::vector<std::byte> {
    const std::vector<std::byte> body = encode_message(message);
    ByteWriter                   writer;
    writer.put<std::uint16_t>(static_cast<std::uint16_t>(body.size()));
    std::vector<std::byte> frame = writer.take();
    frame.insert(frame.end(), body.begin(), body.end());
    return frame;
}

}  // namespace itch
