#pragma once

#include <cstdint>
#include <functional>
#include <string>

#include "itch/price.hpp"

namespace itch {

/// @brief A single execution extracted from the feed (the "trade tape").
///
/// Trades arrive in several ITCH message types: executions of displayed orders
/// (`E`), executions at a different price (`C`), trades of non-displayable orders
/// (`P`), and cross trades (`Q`). They are normalized here into one record. The
/// `printable` flag carries the spec's displayable/non-displayable distinction so
/// consumers can correctly separate lit prints from hidden ones.
struct Trade {
    std::uint64_t timestamp {0};      ///< Nanoseconds past midnight.
    std::uint16_t stock_locate {0};   ///< Locate code identifying the security.
    std::string   symbol;             ///< Stock symbol (may be empty if unknown).
    StandardPrice price {};           ///< Execution price.
    std::uint64_t shares {0};         ///< Executed share quantity.
    std::uint64_t match_number {0};   ///< Exchange match number.
    char          side {'\0'};        ///< Resting order side ('B'/'S'), or '\0'.
    bool          printable {true};   ///< Whether the print is displayable.
    bool          is_cross {false};   ///< Whether this is a cross (auction) trade.
    char          cross_type {'\0'};  ///< Cross type for `Q` trades, else '\0'.
};

/// @brief Callback invoked for each extracted trade.
using TradeCallback = std::function<void(const Trade&)>;

}  // namespace itch
