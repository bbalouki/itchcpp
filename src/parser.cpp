#include "parser.hpp"

#include <cstring>
#include <stdexcept>
#include <vector>


namespace itch {

namespace {

// Helper to unpack a value from a buffer at a given offset
template <typename T>
T unpack(const char* buffer, size_t& offset) {
    T value;
    std::memcpy(&value, buffer + offset, sizeof(T));
    offset += sizeof(T);
    return value;
}

// Specialization for char arrays (strings)
void unpack_string(const char* buffer, size_t& offset, char* dest,
                   size_t size) {
    std::memcpy(dest, buffer + offset, size);
    offset += size;
}

uint64_t unpack_timestamp(const char* buffer, size_t& offset) {
    uint16_t high = unpack<uint16_t>(buffer, offset);
    uint32_t low = unpack<uint32_t>(buffer, offset);
    return (static_cast<uint64_t>(high) << 32) | low;
}

}  // namespace

template <typename T>
void Parser::register_handler(char type) {
    m_handlers[type] = [](const char* buffer) -> Message {
        T msg;
        size_t offset = 1;  // Skip message type

        msg.stock_locate = unpack<uint16_t>(buffer, offset);
        msg.tracking_number = unpack<uint16_t>(buffer, offset);
        msg.timestamp = unpack_timestamp(buffer, offset);

        if constexpr (std::is_same_v<T, SystemEventMessage>) {
            msg.event_code = unpack<char>(buffer, offset);
        } else if constexpr (std::is_same_v<T, StockDirectoryMessage>) {
            unpack_string(buffer, offset, msg.stock, 8);
            msg.market_category = unpack<char>(buffer, offset);
            msg.financial_status_indicator = unpack<char>(buffer, offset);
            msg.round_lot_size = unpack<uint32_t>(buffer, offset);
            msg.round_lots_only = unpack<char>(buffer, offset);
            msg.issue_classification = unpack<char>(buffer, offset);
            unpack_string(buffer, offset, msg.issue_sub_type, 2);
            msg.authenticity = unpack<char>(buffer, offset);
            msg.short_sale_threshold_indicator = unpack<char>(buffer, offset);
            msg.ipo_flag = unpack<char>(buffer, offset);
            msg.luld_ref = unpack<char>(buffer, offset);
            msg.etp_flag = unpack<char>(buffer, offset);
            msg.etp_leverage_factor = unpack<uint32_t>(buffer, offset);
            msg.inverse_indicator = unpack<char>(buffer, offset);
        } else if constexpr (std::is_same_v<T, StockTradingActionMessage>) {
            unpack_string(buffer, offset, msg.stock, 8);
            msg.trading_state = unpack<char>(buffer, offset);
            msg.reserved = unpack<char>(buffer, offset);
            unpack_string(buffer, offset, msg.reason, 4);
        } else if constexpr (std::is_same_v<T, RegSHOMessage>) {
            unpack_string(buffer, offset, msg.stock, 8);
            msg.reg_sho_action = unpack<char>(buffer, offset);
        } else if constexpr (std::is_same_v<T,
                                            MarketParticipantPositionMessage>) {
            unpack_string(buffer, offset, msg.mpid, 4);
            unpack_string(buffer, offset, msg.stock, 8);
            msg.primary_market_maker = unpack<char>(buffer, offset);
            msg.market_maker_mode = unpack<char>(buffer, offset);
            msg.market_participant_state = unpack<char>(buffer, offset);
        } else if constexpr (std::is_same_v<T, MWCBDeclineLevelMessage>) {
            msg.level1 = unpack<uint64_t>(buffer, offset);
            msg.level2 = unpack<uint64_t>(buffer, offset);
            msg.level3 = unpack<uint64_t>(buffer, offset);
        } else if constexpr (std::is_same_v<T, MWCBStatusMessage>) {
            msg.breached_level = unpack<char>(buffer, offset);
        } else if constexpr (std::is_same_v<T, IPOQuotingPeriodUpdateMessage>) {
            unpack_string(buffer, offset, msg.stock, 8);
            msg.ipo_quotation_release_time = unpack<uint32_t>(buffer, offset);
            msg.ipo_quotation_release_qualifier = unpack<char>(buffer, offset);
            msg.ipo_price = unpack<uint32_t>(buffer, offset);
        } else if constexpr (std::is_same_v<T, LULDAuctionCollarMessage>) {
            unpack_string(buffer, offset, msg.stock, 8);
            msg.auction_collar_reference_price = unpack<uint32_t>(buffer, offset);
            msg.upper_auction_collar_price = unpack<uint32_t>(buffer, offset);
            msg.lower_auction_collar_price = unpack<uint32_t>(buffer, offset);
            msg.auction_collar_extension = unpack<uint32_t>(buffer, offset);
        } else if constexpr (std::is_same_v<T, OperationalHaltMessage>) {
            unpack_string(buffer, offset, msg.stock, 8);
            msg.market_code = unpack<char>(buffer, offset);
            msg.operational_halt_action = unpack<char>(buffer, offset);
        } else if constexpr (std::is_same_v<T, AddOrderMessage>) {
            msg.order_reference_number = unpack<uint64_t>(buffer, offset);
            msg.buy_sell_indicator = unpack<char>(buffer, offset);
            msg.shares = unpack<uint32_t>(buffer, offset);
            unpack_string(buffer, offset, msg.stock, 8);
            msg.price = unpack<uint32_t>(buffer, offset);
        } else if constexpr (std::is_same_v<T,
                                            AddOrderMPIDAttributionMessage>) {
            msg.order_reference_number = unpack<uint64_t>(buffer, offset);
            msg.buy_sell_indicator = unpack<char>(buffer, offset);
            msg.shares = unpack<uint32_t>(buffer, offset);
            unpack_string(buffer, offset, msg.stock, 8);
            msg.price = unpack<uint32_t>(buffer, offset);
            unpack_string(buffer, offset, msg.attribution, 4);
        } else if constexpr (std::is_same_v<T, OrderExecutedMessage>) {
            msg.order_reference_number = unpack<uint64_t>(buffer, offset);
            msg.executed_shares = unpack<uint32_t>(buffer, offset);
            msg.match_number = unpack<uint64_t>(buffer, offset);
        } else if constexpr (std::is_same_v<T, OrderExecutedWithPriceMessage>) {
            msg.order_reference_number = unpack<uint64_t>(buffer, offset);
            msg.executed_shares = unpack<uint32_t>(buffer, offset);
            msg.match_number = unpack<uint64_t>(buffer, offset);
            msg.printable = unpack<char>(buffer, offset);
            msg.execution_price = unpack<uint32_t>(buffer, offset);
        } else if constexpr (std::is_same_v<T, OrderCancelMessage>) {
            msg.order_reference_number = unpack<uint64_t>(buffer, offset);
            msg.cancelled_shares = unpack<uint32_t>(buffer, offset);
        } else if constexpr (std::is_same_v<T, OrderDeleteMessage>) {
            msg.order_reference_number = unpack<uint64_t>(buffer, offset);
        } else if constexpr (std::is_same_v<T, OrderReplaceMessage>) {
            msg.original_order_reference_number =
                unpack<uint64_t>(buffer, offset);
            msg.new_order_reference_number = unpack<uint64_t>(buffer, offset);
            msg.shares = unpack<uint32_t>(buffer, offset);
            msg.price = unpack<uint32_t>(buffer, offset);
        } else if constexpr (std::is_same_v<T, NonCrossTradeMessage>) {
            msg.order_reference_number = unpack<uint64_t>(buffer, offset);
            msg.buy_sell_indicator = unpack<char>(buffer, offset);
            msg.shares = unpack<uint32_t>(buffer, offset);
            unpack_string(buffer, offset, msg.stock, 8);
            msg.price = unpack<uint32_t>(buffer, offset);
            msg.match_number = unpack<uint64_t>(buffer, offset);
        } else if constexpr (std::is_same_v<T, CrossTradeMessage>) {
            msg.shares = unpack<uint64_t>(buffer, offset);
            unpack_string(buffer, offset, msg.stock, 8);
            msg.cross_price = unpack<uint32_t>(buffer, offset);
            msg.match_number = unpack<uint64_t>(buffer, offset);
            msg.cross_type = unpack<char>(buffer, offset);
        } else if constexpr (std::is_same_v<T, BrokenTradeMessage>) {
            msg.match_number = unpack<uint64_t>(buffer, offset);
        } else if constexpr (std::is_same_v<T, NOIIMessage>) {
            msg.paired_shares = unpack<uint64_t>(buffer, offset);
            msg.imbalance_shares = unpack<uint64_t>(buffer, offset);
            msg.imbalance_direction = unpack<char>(buffer, offset);
            unpack_string(buffer, offset, msg.stock, 8);
            msg.far_price = unpack<uint32_t>(buffer, offset);
            msg.near_price = unpack<uint32_t>(buffer, offset);
            msg.current_reference_price = unpack<uint32_t>(buffer, offset);
            msg.cross_type = unpack<char>(buffer, offset);
            msg.price_variation_indicator = unpack<char>(buffer, offset);
        } else if constexpr (std::is_same_v<
                                 T, RetailPriceImprovementIndicatorMessage>) {
            unpack_string(buffer, offset, msg.stock, 8);
            msg.interest_flag = unpack<char>(buffer, offset);
        } else if constexpr (std::is_same_v<T, DLCRMessage>) {
            unpack_string(buffer, offset, msg.stock, 8);
            msg.open_eligibility_status = unpack<char>(buffer, offset);
            msg.minimum_allowable_price = unpack<uint32_t>(buffer, offset);
            msg.maximum_allowable_price = unpack<uint32_t>(buffer, offset);
            msg.near_execution_price = unpack<uint32_t>(buffer, offset);
            msg.near_execution_time = unpack<uint64_t>(buffer, offset);
            msg.lower_price_range_collar = unpack<uint32_t>(buffer, offset);
            msg.upper_price_range_collar = unpack<uint32_t>(buffer, offset);
        }

        return msg;
    };
}

Parser::Parser() {
    m_message_sizes = {{'S', 12}, {'R', 39}, {'H', 25}, {'Y', 20}, {'L', 26},
                       {'V', 35}, {'W', 12}, {'K', 28}, {'J', 33}, {'h', 21},
                       {'A', 36}, {'F', 40}, {'E', 31}, {'C', 36}, {'X', 23},
                       {'D', 19}, {'U', 33}, {'P', 44}, {'Q', 40}, {'B', 19},
                       {'I', 50}, {'N', 20}, {'O', 48}};

    register_handler<SystemEventMessage>('S');
    register_handler<StockDirectoryMessage>('R');
    register_handler<StockTradingActionMessage>('H');
    register_handler<RegSHOMessage>('Y');
    register_handler<MarketParticipantPositionMessage>('L');
    register_handler<MWCBDeclineLevelMessage>('V');
    register_handler<MWCBStatusMessage>('W');
    register_handler<IPOQuotingPeriodUpdateMessage>('K');
    register_handler<LULDAuctionCollarMessage>('J');
    register_handler<OperationalHaltMessage>('h');
    register_handler<AddOrderMessage>('A');
    register_handler<AddOrderMPIDAttributionMessage>('F');
    register_handler<OrderExecutedMessage>('E');
    register_handler<OrderExecutedWithPriceMessage>('C');
    register_handler<OrderCancelMessage>('X');
    register_handler<OrderDeleteMessage>('D');
    register_handler<OrderReplaceMessage>('U');
    register_handler<NonCrossTradeMessage>('P');
    register_handler<CrossTradeMessage>('Q');
    register_handler<BrokenTradeMessage>('B');
    register_handler<NOIIMessage>('I');
    register_handler<RetailPriceImprovementIndicatorMessage>('N');
    register_handler<DLCRMessage>('O');
}

void Parser::parse(std::istream& is,
                   const std::function<void(const Message&)>& callback) {
    char message_type;
    while (is.get(message_type)) {
        auto size_it = m_message_sizes.find(message_type);
        if (size_it == m_message_sizes.end()) {
            throw std::runtime_error(std::string("Unknown message type: ") +
                                     message_type);
        }
        size_t length = size_it->second;

        std::vector<char> buffer(length);
        buffer[0] = message_type;

        if (!is.read(buffer.data() + 1, length - 1)) {
            break;  // End of stream or incomplete message
        }

        auto handler_it = m_handlers.find(message_type);
        if (handler_it != m_handlers.end()) {
            callback(handler_it->second(buffer.data()));
        }
    }
}

std::vector<Message> Parser::parse(std::istream& is) {
    std::vector<Message> messages;
    parse(is, [&](const Message& msg) { messages.push_back(msg); });
    return messages;
}

}  // namespace itch
