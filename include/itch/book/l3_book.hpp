#pragma once

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
    explicit L3Book(std::string symbol = {});

    /// @brief Sets the stock symbol associated with this book.
    auto set_symbol(std::string symbol) -> void { m_symbol = std::move(symbol); }

    /// @brief The stock symbol associated with this book (may be empty).
    [[nodiscard]] auto symbol() const noexcept -> const std::string& { return m_symbol; }

    /// @brief Adds a new resting order to the book (ITCH `A`/`F`).
    auto add_order(
        std::uint64_t reference_number, Side side, std::uint32_t shares, std::uint32_t price
    ) -> void;

    /// @brief Removes `shares` from an order on execution (ITCH `E`/`C`),
    ///        deleting it when fully filled. Returns the shares actually removed.
    auto execute_order(std::uint64_t reference_number, std::uint32_t shares) -> std::uint32_t;

    /// @brief Removes `shares` from an order on a partial cancel (ITCH `X`),
    ///        deleting it when it reaches zero. Returns the shares actually removed.
    auto reduce_order(std::uint64_t reference_number, std::uint32_t shares) -> std::uint32_t;

    /// @brief Deletes an order in its entirety (ITCH `D`).
    auto delete_order(std::uint64_t reference_number) -> void;

    /// @brief Replaces an order with a new reference number, size, and price
    ///        on the same side (ITCH `U`).
    auto replace_order(
        std::uint64_t old_reference_number,
        std::uint64_t new_reference_number,
        std::uint32_t shares,
        std::uint32_t price
    ) -> void;

    /// @brief Whether an order with the given reference number is resting.
    [[nodiscard]] auto contains(std::uint64_t reference_number) const -> bool;

    /// @brief The raw limit price of a resting order, if present.
    [[nodiscard]] auto order_price(std::uint64_t reference_number) const
        -> std::optional<std::uint32_t>;

    /// @brief The side of a resting order, if present.
    [[nodiscard]] auto order_side(std::uint64_t reference_number) const -> std::optional<Side>;

    /// @brief The current best bid and offer.
    [[nodiscard]] auto bbo() const -> Bbo;

    /// @brief Aggregated L2 depth for a side, best level first.
    ///
    /// @param side The side to report.
    /// @param max_levels The maximum number of levels to return (0 == all).
    [[nodiscard]] auto depth(Side side, std::size_t max_levels = 0) const
        -> std::vector<DepthLevel>;

    /// @brief The resting orders at a given price on a side, in time priority.
    [[nodiscard]] auto orders_at(Side side, std::uint32_t price) const -> std::vector<OrderView>;

    /// @brief The number of active price levels on a side.
    [[nodiscard]] auto level_count(Side side) const noexcept -> std::size_t;

    /// @brief Whether the book has no resting orders on either side.
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

    [[nodiscard]] auto side_levels(Side side) noexcept -> std::vector<Level>&;
    [[nodiscard]] auto side_levels(Side side) const noexcept -> const std::vector<Level>&;

    // Allocates a node from the free list (or grows the pool) and returns its index.
    auto allocate_node() -> std::uint32_t;
    // Returns a node to the free list.
    auto free_node(std::uint32_t node_index) -> void;
    // Finds the index of the level at `price` on `side`, or NIL if absent.
    [[nodiscard]] auto find_level(Side side, std::uint32_t price) const -> std::uint32_t;
    // Finds, or inserts in sorted order, the level at `price`; returns its index.
    auto find_or_create_level(Side side, std::uint32_t price) -> std::uint32_t;
    // Unlinks a node from its level FIFO and removes the level if it empties.
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
