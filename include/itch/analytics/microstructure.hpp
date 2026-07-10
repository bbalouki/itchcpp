#pragma once

/// @file
/// @brief Best-bid-offer microstructure metrics derived from the order book.
///
/// Small, allocation-free functions that compute spread, mid price, queue
/// imbalance, depth, and order-flow imbalance from `Bbo`/`L3Book` snapshots.
///
/// @author Bertin Balouki SIMYELI

#include <cstddef>
#include <cstdint>
#include <limits>

#include "itch/book/l3_book.hpp"

namespace itch::analytics {

/// @brief The bid-ask spread in price units, or NaN if either side is empty.
///
/// @param bbo The best-bid-offer snapshot to compute the spread from.
/// @return `ask_price - bid_price`, or NaN if either side has no resting order.
[[nodiscard]] inline auto spread(const book::Bbo& bbo) -> double {
    if (!bbo.has_bid || !bbo.has_ask) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    return bbo.ask_price.to_double() - bbo.bid_price.to_double();
}

/// @brief The mid price ((bid + ask) / 2), or NaN if either side is empty.
///
/// @param bbo The best-bid-offer snapshot to compute the mid price from.
/// @return The average of the bid and ask prices, or NaN if either side has no
///         resting order.
[[nodiscard]] inline auto mid_price(const book::Bbo& bbo) -> double {
    if (!bbo.has_bid || !bbo.has_ask) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    return (bbo.bid_price.to_double() + bbo.ask_price.to_double()) / 2.0;
}

/// @brief Top-of-book queue imbalance in [-1, 1].
///
/// Positive values indicate more size resting on the bid than the offer
/// (buy-side pressure); negative values the reverse. Returns NaN when there is no
/// resting size at the top of either side.
///
/// @param bbo The best-bid-offer snapshot to compute the queue imbalance from.
/// @return The normalized bid/ask size imbalance in [-1, 1], or NaN if both sides
///         are empty.
[[nodiscard]] inline auto queue_imbalance(const book::Bbo& bbo) -> double {
    const double bid   = static_cast<double>(bbo.bid_shares);
    const double ask   = static_cast<double>(bbo.ask_shares);
    const double total = bid + ask;
    if (total <= 0.0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    return (bid - ask) / total;
}

/// @brief Total displayed shares within the best `levels` price levels of a side.
///
/// @param book The order book to query.
/// @param side The book side (bid or ask) to sum depth for.
/// @param levels Number of best price levels to include.
/// @return The total displayed shares across the requested levels.
[[nodiscard]] inline auto depth_at_level(
    const book::L3Book& book, book::Side side, std::size_t levels
) -> std::uint64_t {
    std::uint64_t total = 0;
    for (const auto& level : book.depth(side, levels)) {
        total += level.shares;
    }
    return total;
}

/// @brief The order-flow imbalance between two consecutive BBO observations.
///
/// Implements the standard order-flow-imbalance contribution of Cont, Kukanov and
/// Stoikov: each side compares the new best price and size against the previous to
/// attribute added or removed liquidity, and the offer contribution is subtracted
/// from the bid contribution. Positive values indicate net buy-side flow.
///
/// @param previous The prior best-bid-offer snapshot.
/// @param current The current best-bid-offer snapshot.
/// @return The signed order-flow-imbalance contribution between the two snapshots.
[[nodiscard]] inline auto order_flow_imbalance(const book::Bbo& previous, const book::Bbo& current)
    -> double {
    const double prev_bid_price = previous.bid_price.to_double();
    const double curr_bid_price = current.bid_price.to_double();
    const double prev_ask_price = previous.ask_price.to_double();
    const double curr_ask_price = current.ask_price.to_double();
    const double prev_bid_size  = static_cast<double>(previous.bid_shares);
    const double curr_bid_size  = static_cast<double>(current.bid_shares);
    const double prev_ask_size  = static_cast<double>(previous.ask_shares);
    const double curr_ask_size  = static_cast<double>(current.ask_shares);

    double bid_flow = 0.0;
    if (curr_bid_price > prev_bid_price) {
        bid_flow = curr_bid_size;
    } else if (curr_bid_price == prev_bid_price) {
        bid_flow = curr_bid_size - prev_bid_size;
    } else {
        bid_flow = -prev_bid_size;
    }

    double ask_flow = 0.0;
    if (curr_ask_price > prev_ask_price) {
        ask_flow = prev_ask_size;
    } else if (curr_ask_price == prev_ask_price) {
        ask_flow = curr_ask_size - prev_ask_size;
    } else {
        ask_flow = -curr_ask_size;
    }

    return bid_flow - ask_flow;
}

}  // namespace itch::analytics
