#pragma once

#include <cstddef>
#include <cstdint>

#include "itch/messages.hpp"

/// @file
/// @brief Single source of truth for the ITCH 5.0 wire layout metadata shared by
///        the eager parser, the zero-copy overlay, and the encoder.
///
/// Keeping the per-type wire size and the canonical list of message types in one
/// place means the dispatch table, the overlay accessors, and the writer can
/// never drift apart from one another.

namespace itch::detail {

/// @brief The on-wire ITCH timestamp is 48 bits (6 bytes), but every message
///        struct stores it in a 64-bit field, so each struct is exactly two
///        bytes wider than its encoding. The timestamp is the only field whose
///        storage width differs from its wire width.
constexpr std::size_t TIMESTAMP_STRUCT_PADDING = sizeof(std::uint64_t) - 6;

/// @brief The exact on-wire size, in bytes, of a fully formed message of type T.
template <typename MsgType>
constexpr std::size_t WIRE_SIZE = sizeof(MsgType) - TIMESTAMP_STRUCT_PADDING;

// Lock the padding assumption to the spec lengths so a future struct change that
// breaks the derivation is caught at compile time rather than at runtime.
static_assert(WIRE_SIZE<SystemEventMessage> == 12);
static_assert(WIRE_SIZE<StockDirectoryMessage> == 39);
static_assert(WIRE_SIZE<AddOrderMessage> == 36);
static_assert(WIRE_SIZE<NOIIMessage> == 50);
static_assert(WIRE_SIZE<DLCRMessage> == 48);

/// @brief Invokes `visitor.template operator()<MsgType>(type_byte)` once for each
///        ITCH 5.0 message type, in a fixed order.
///
/// This is the canonical registry of message types. Both the parser's dispatch
/// table and any other component that needs to enumerate the message set build
/// on it, so the set is defined exactly once.
template <typename Visitor>
constexpr auto for_each_message_type(Visitor&& visitor) -> void {
    visitor.template operator()<SystemEventMessage>('S');
    visitor.template operator()<StockDirectoryMessage>('R');
    visitor.template operator()<StockTradingActionMessage>('H');
    visitor.template operator()<RegSHOMessage>('Y');
    visitor.template operator()<MarketParticipantPositionMessage>('L');
    visitor.template operator()<MWCBDeclineLevelMessage>('V');
    visitor.template operator()<MWCBStatusMessage>('W');
    visitor.template operator()<IPOQuotingPeriodUpdateMessage>('K');
    visitor.template operator()<LULDAuctionCollarMessage>('J');
    visitor.template operator()<OperationalHaltMessage>('h');
    visitor.template operator()<AddOrderMessage>('A');
    visitor.template operator()<AddOrderMPIDAttributionMessage>('F');
    visitor.template operator()<OrderExecutedMessage>('E');
    visitor.template operator()<OrderExecutedWithPriceMessage>('C');
    visitor.template operator()<OrderCancelMessage>('X');
    visitor.template operator()<OrderDeleteMessage>('D');
    visitor.template operator()<OrderReplaceMessage>('U');
    visitor.template operator()<NonCrossTradeMessage>('P');
    visitor.template operator()<CrossTradeMessage>('Q');
    visitor.template operator()<BrokenTradeMessage>('B');
    visitor.template operator()<NOIIMessage>('I');
    visitor.template operator()<RetailPriceImprovementIndicatorMessage>('N');
    visitor.template operator()<DLCRMessage>('O');
}

}  // namespace itch::detail
