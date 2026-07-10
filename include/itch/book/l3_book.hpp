#pragma once

/// @file
/// @brief Single-symbol, order-level (L3) limit order book with
///        allocation-light internals.
///
/// This header declares `L3Book`, which reconstructs one security's full
/// order book from ITCH add/execute/cancel/delete/replace messages using an
/// object pool and intrusive FIFO queues instead of per-order heap
/// allocation, plus the small `Side`, `DepthLevel`, `OrderView`, and `Bbo`
/// value types used to query it.
///
/// @author Bertin Balouki SIMYELI

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "itch/book/order_index.hpp"
#include "itch/price.hpp"

namespace itch::book {

/// @brief Which side of the book an order rests on.
enum class Side : char {
    buy  = 'B',  ///< A bid.
    sell = 'S',  ///< An offer/ask.
};

/// @brief Aggregated state of one price level, for L2 depth snapshots.
struct DepthLevel {
    StandardPrice price {};         ///< The level's limit price.
    std::uint64_t shares {0};       ///< Total displayed shares resting at the level.
    std::uint32_t order_count {0};  ///< Number of resting orders at the level.
};

/// @brief A single resting order, for L3 order-level snapshots.
struct OrderView {
    std::uint64_t reference_number {0};  ///< Exchange order reference number.
    std::uint32_t shares {0};            ///< Shares still resting.
    StandardPrice price {};              ///< Limit price.
};

/// @brief Best bid and offer of a single book.
struct Bbo {
    bool          has_bid {false};  ///< Whether a bid side exists.
    bool          has_ask {false};  ///< Whether an ask side exists.
    StandardPrice bid_price {};     ///< Best (highest) bid price.
    std::uint64_t bid_shares {0};   ///< Shares at the best bid.
    StandardPrice ask_price {};     ///< Best (lowest) ask price.
    std::uint64_t ask_shares {0};   ///< Shares at the best ask.

    /// @brief Compares two BBO snapshots for equality of all fields.
    /// @param lhs The first snapshot to compare.
    /// @param rhs The second snapshot to compare.
    /// @return True if `lhs` and `rhs` have equal has_bid/has_ask/prices/
    ///         shares, false otherwise.
    [[nodiscard]] friend auto operator==(const Bbo&, const Bbo&) noexcept -> bool = default;
};

/// @brief A single-symbol, order-level (L3) limit order book with allocation-light
///        internals.
///
/// Unlike the original `LimitOrderBook`, which stored each order in a
/// `std::shared_ptr` inside a `std::list` and each price level in a `std::map`,
/// this engine holds orders in a reusable object pool (a flat vector with a free
/// list) linked into intrusive FIFO queues, and keeps each side's price levels in
/// a flat, sorted vector. There is no per-order heap allocation, no atomic
/// refcount, and no per-level node allocation on the hot path; order lookup by
/// reference number is O(1) and the best bid/offer is the front of a ladder.
class L3Book {
   public:
    /// @brief Constructs a book, optionally tagged with its stock symbol.
    /// @param symbol The ticker symbol to associate with the book (may be
    ///        empty).
    explicit L3Book(std::string symbol = {});

    /// @brief Sets the stock symbol associated with this book.
    /// @param symbol The ticker symbol to associate with the book.
    auto set_symbol(std::string symbol) -> void { m_symbol = std::move(symbol); }

    /// @brief The stock symbol associated with this book (may be empty).
    /// @return Const reference to the associated ticker symbol.
    [[nodiscard]] auto symbol() const noexcept -> const std::string& { return m_symbol; }

    /// @brief Adds a new resting order to the book (ITCH `A`/`F`).
    /// @param reference_number Exchange order reference number identifying
    ///        the new order.
    /// @param side The side of the book the order rests on.
    /// @param shares The number of shares in the new order.
    /// @param price The raw (unscaled) limit price of the new order.
    auto add_order(
        std::uint64_t reference_number, Side side, std::uint32_t shares, std::uint32_t price
    ) -> void;

    /// @brief Removes `shares` from an order on execution (ITCH `E`/`C`),
    ///        deleting it when fully filled. Returns the shares actually removed.
    /// @param reference_number Exchange order reference number of the order
    ///        being executed against.
    /// @param shares The number of shares to remove from the order.
    /// @return The number of shares actually removed (may be less than
    ///         `shares` if the order does not have that many resting).
    auto execute_order(std::uint64_t reference_number, std::uint32_t shares) -> std::uint32_t;

    /// @brief Removes `shares` from an order on a partial cancel (ITCH `X`),
    ///        deleting it when it reaches zero. Returns the shares actually removed.
    /// @param reference_number Exchange order reference number of the order
    ///        being reduced.
    /// @param shares The number of shares to remove from the order.
    /// @return The number of shares actually removed (may be less than
    ///         `shares` if the order does not have that many resting).
    auto reduce_order(std::uint64_t reference_number, std::uint32_t shares) -> std::uint32_t;

    /// @brief Deletes an order in its entirety (ITCH `D`).
    /// @param reference_number Exchange order reference number of the order
    ///        to delete.
    auto delete_order(std::uint64_t reference_number) -> void;

    /// @brief Replaces an order with a new reference number, size, and price
    ///        on the same side (ITCH `U`).
    /// @param old_reference_number Exchange order reference number of the
    ///        order being replaced.
    /// @param new_reference_number Exchange order reference number assigned
    ///        to the replacement order.
    /// @param shares The number of shares in the replacement order.
    /// @param price The raw (unscaled) limit price of the replacement order.
    auto replace_order(
        std::uint64_t old_reference_number,
        std::uint64_t new_reference_number,
        std::uint32_t shares,
        std::uint32_t price
    ) -> void;

    /// @brief Whether an order with the given reference number is resting.
    /// @param reference_number Exchange order reference number to look up.
    /// @return True if an order with `reference_number` is resting, false
    ///         otherwise.
    [[nodiscard]] auto contains(std::uint64_t reference_number) const -> bool;

    /// @brief The raw limit price of a resting order, if present.
    /// @param reference_number Exchange order reference number to look up.
    /// @return The order's raw limit price, or `std::nullopt` if no such
    ///         order is resting.
    [[nodiscard]] auto order_price(std::uint64_t reference_number
    ) const -> std::optional<std::uint32_t>;

    /// @brief The side of a resting order, if present.
    /// @param reference_number Exchange order reference number to look up.
    /// @return The order's side, or `std::nullopt` if no such order is
    ///         resting.
    [[nodiscard]] auto order_side(std::uint64_t reference_number) const -> std::optional<Side>;

    /// @brief The current best bid and offer.
    /// @return The book's current best bid and offer snapshot.
    [[nodiscard]] auto bbo() const -> Bbo;

    /// @brief Aggregated L2 depth for a side, best level first.
    ///
    /// @param side The side to report.
    /// @param max_levels The maximum number of levels to return (0 == all).
    /// @return The requested side's price levels, best (top of book) first.
    [[nodiscard]] auto depth(Side side, std::size_t max_levels = 0) const
        -> std::vector<DepthLevel>;

    /// @brief The resting orders at a given price on a side, in time priority.
    /// @param side The side to query.
    /// @param price The raw (unscaled) limit price to query.
    /// @return The resting orders at `price` on `side`, oldest first.
    [[nodiscard]] auto orders_at(Side side, std::uint32_t price) const -> std::vector<OrderView>;

    /// @brief The number of active price levels on a side.
    /// @param side The side to query.
    /// @return The count of active price levels on `side`.
    [[nodiscard]] auto level_count(Side side) const noexcept -> std::size_t;

    /// @brief Whether the book has no resting orders on either side.
    /// @return True if no orders are resting on either side, false otherwise.
    [[nodiscard]] auto empty() const noexcept -> bool { return m_index.empty(); }

   private:
    /// @brief Sentinel index meaning "no node".
    static constexpr std::uint32_t NIL = 0xFFFFFFFFU;

    /// @brief One resting order in the object pool, linked into a level's FIFO.
    struct OrderNode {
        std::uint64_t reference_number {0};
        std::uint32_t shares {0};
        std::uint32_t price {0};
        Side          side {Side::buy};
        std::uint32_t next {NIL};  ///< Next order in level FIFO, or next free node.
        std::uint32_t prev {NIL};  ///< Previous order in level FIFO.
    };

    /// @brief One price level holding the head/tail of an intrusive FIFO queue.
    struct Level {
        std::uint32_t price {0};
        std::uint64_t total_shares {0};
        std::uint32_t order_count {0};
        std::uint32_t head {NIL};
        std::uint32_t tail {NIL};
    };

    /// @brief The mutable level ladder for a side.
    /// @param side The side whose ladder to return.
    /// @return Reference to the vector of price levels for `side`.
    [[nodiscard]] auto side_levels(Side side) noexcept -> std::vector<Level>&;

    /// @brief The level ladder for a side.
    /// @param side The side whose ladder to return.
    /// @return Const reference to the vector of price levels for `side`.
    [[nodiscard]] auto side_levels(Side side) const noexcept -> const std::vector<Level>&;

    /// @brief Allocates a node from the free list (or grows the pool) and
    ///        returns its index.
    /// @return The pool index of the newly allocated node.
    auto allocate_node() -> std::uint32_t;

    /// @brief Returns a node to the free list.
    /// @param node_index The pool index of the node to free.
    auto free_node(std::uint32_t node_index) -> void;

    /// @brief Finds the index of the level at `price` on `side`, or NIL if
    ///        absent.
    /// @param side The side to search.
    /// @param price The raw (unscaled) limit price to find.
    /// @return The index of the matching level, or `NIL` if none exists.
    [[nodiscard]] auto find_level(Side side, std::uint32_t price) const -> std::uint32_t;

    /// @brief Finds, or inserts in sorted order, the level at `price`;
    ///        returns its index.
    /// @param side The side to search or insert into.
    /// @param price The raw (unscaled) limit price to find or create.
    /// @return The index of the existing or newly created level.
    auto find_or_create_level(Side side, std::uint32_t price) -> std::uint32_t;

    /// @brief Unlinks a node from its level FIFO and removes the level if it
    ///        empties.
    /// @param node_index The pool index of the node to unlink.
    auto unlink_node(std::uint32_t node_index) -> void;

    std::string            m_symbol;
    std::vector<OrderNode> m_pool;
    std::uint32_t          m_free_head {NIL};
    std::vector<Level>     m_bids;  ///< Sorted high to low; front is the best bid.
    std::vector<Level>     m_asks;  ///< Sorted low to high; front is the best ask.
    // Reference-number -> pool index for O(1), allocation-free order lookup.
    OrderIndex m_index;
};

}  // namespace itch::book
