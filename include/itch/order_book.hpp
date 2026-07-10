#pragma once

/// @file
/// @brief An in-memory limit order book reconstructed from a stream of parsed
///        ITCH 5.0 messages.
///
/// `LimitOrderBook` maintains the resting bid/ask price levels for a single
/// instrument, applying Add/Execute/Cancel/Delete/Replace order events as they
/// arrive so callers can query the current book depth or render it for
/// inspection.
///
/// @author Bertin Balouki SIMYELI

#include <cstdint>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <ostream>
#include <set>
#include <variant>

#include "itch/messages.hpp"

namespace itch {

struct PriceLevel;

/// @struct Order
/// @brief Represents a single resting order within the Limit Order Book.
///
/// Encapsulates all necessary state data for an order, including its unique
/// reference number, side (Buy/Sell), quantity, and price. It also maintains
/// a pointer to its parent PriceLevel for efficient traversal.
struct Order {
    uint64_t order_reference_number;  ///< Unique identifier assigned to the order by the exchange.
    char     buy_sell_indicator;      ///< 'B' for Buy, 'S' for Sell.
    uint32_t shares;                  ///< Current quantity of shares available in this order.
    uint32_t price;                   ///< Limit price of the order.
    PriceLevel* level;                ///< Pointer to the price level containing this order.
    std::string stock;                ///< The stock symbol for this order.

    /// @brief Constructs an order with the given identity, side, quantity, and price.
    ///
    /// @param ref_num The exchange-assigned order reference number.
    /// @param side 'B' for Buy, 'S' for Sell.
    /// @param shrs The initial quantity of shares in the order.
    /// @param prc The limit price of the order.
    /// @param stk The stock symbol for this order.
    Order(uint64_t ref_num, char side, uint32_t shrs, uint32_t prc, const std::string& stk)
        : order_reference_number {ref_num},
          buy_sell_indicator {side},
          shares {shrs},
          price {prc},
          level {nullptr},
          stock {stk} {}
};

/// Iterator type for navigating the list of orders within a price level.
using OrderIt = std::list<std::shared_ptr<Order>>::iterator;

/// @struct PriceLevel
/// @brief Represents a specific price node in the order book.
///
/// Maintains a queue of orders at a specific price, enforcing Time-Priority
/// (FIFO) execution. It tracks the aggregate share volume for this level.
struct PriceLevel {
    uint32_t total_shares {0};                 ///< Aggregate volume of shares at this price.
    std::list<std::shared_ptr<Order>> orders;  ///< FIFO queue of orders.

    /// @brief Appends an order to the end of the queue (Time priority).
    /// @param order Shared pointer to the order to add.
    auto add_order(const std::shared_ptr<Order>& order) -> void;

    /// @brief Removes a specific order from the queue.
    /// @param order_it Iterator pointing to the order to remove.
    auto remove_order(const OrderIt& order_it) -> void;
};

/// @class LimitOrderBook
/// @brief Manages the state of a limit order book for a specific financial instrument.
///
/// The LimitOrderBook ingests raw Nasdaq ITCH 5.0 messages and reconstructs the
/// full depth of market. It supports standard order lifecycle events including
/// addition, execution, cancellation, deletion, and replacement.
///
/// @note This implementation uses `std::map` for price levels to ensure
/// keys (prices) are always sorted, allowing for O(log n) insertion and deletion
/// of price levels, and O(1) access to the best bid/ask.
class LimitOrderBook {
   public:
    /// @brief Constructs an order book scoped to a single stock symbol.
    ///
    /// @param stock_symbol The symbol of the instrument this book tracks; only
    ///                     messages for this symbol affect the book state.
    LimitOrderBook(const std::string& stock_symbol) : m_stock_symbol(stock_symbol) {}
    /// @brief Dispatches and processes a generic ITCH message.
    ///
    /// Identifies the specific type within the `Message` variant and routes it
    /// to the appropriate internal handler to update the book state.
    ///
    /// @param message The parsed ITCH message variant.
    auto process(const Message& message) -> void;

    /// @brief Visualizes the current state of the order book to an output stream.
    ///
    /// Prints the best Ask and Bid levels formatted in a table.
    ///
    /// @param out The output stream (e.g., std::cout) to write to.
    /// @param delay_ms Optional delay in milliseconds between printing each line.
    ///                 Useful for slowing down output during debugging or replays
    ///                 to create an animation effect. Defaults to 0 (no delay).
    auto print(std::ostream& out, unsigned int delay_ms = 0) const -> void;

    // Type aliases for the bid and ask maps (sorted containers).
    using BidMap = std::map<uint32_t, PriceLevel, std::greater<uint32_t>>;
    using AskMap = std::map<uint32_t, PriceLevel, std::less<uint32_t>>;

    /// @brief The map of active Bids.
    ///
    /// @return A reference to the map of active Bids, sorted descending by price.
    auto get_bids() const -> const BidMap& { return m_bids; }

    /// @brief The map of active Asks.
    ///
    /// @return A reference to the map of active Asks, sorted ascending by price.
    auto get_asks() const -> const AskMap& { return m_asks; }

    /// Set of message type characters that affect the book state.
    const std::set<char> book_messages {'A', 'F', 'E', 'C', 'X', 'D', 'U'};

   private:
    std::string                 m_stock_symbol;
    BidMap                      m_bids;  ///< Ask price levels (Price -> Level). Sorted Low-to-High.
    AskMap                      m_asks;  ///< Bid price levels (Price -> Level). Sorted High-to-Low.
    std::map<uint64_t, OrderIt> m_orders;  ///< Hash map for O(1) order lookup by Reference Number.

    // Message Handlers
    /// @brief Handles an Add Order (no MPID) message: inserts a new resting
    ///        order into the book if it belongs to this instrument.
    ///
    /// @param msg The parsed Add Order message.
    auto handle_message(const AddOrderMessage& msg) -> void;

    /// @brief Handles an Add Order with MPID Attribution message: inserts a
    ///        new resting order into the book if it belongs to this instrument.
    ///
    /// @param msg The parsed Add Order (MPID Attribution) message.
    auto handle_message(const AddOrderMPIDAttributionMessage& msg) -> void;

    /// @brief Handles an Order Executed message: reduces or removes the
    ///        referenced order by its executed share quantity.
    ///
    /// @param msg The parsed Order Executed message.
    auto handle_message(const OrderExecutedMessage& msg) -> void;

    /// @brief Handles an Order Executed With Price message: reduces or removes
    ///        the referenced order by its executed share quantity.
    ///
    /// @param msg The parsed Order Executed With Price message.
    auto handle_message(const OrderExecutedWithPriceMessage& msg) -> void;

    /// @brief Handles an Order Cancel message: reduces or removes the
    ///        referenced order by its cancelled share quantity.
    ///
    /// @param msg The parsed Order Cancel message.
    auto handle_message(const OrderCancelMessage& msg) -> void;

    /// @brief Handles an Order Delete message: removes the referenced order
    ///        from the book entirely.
    ///
    /// @param msg The parsed Order Delete message.
    auto handle_message(const OrderDeleteMessage& msg) -> void;

    /// @brief Handles an Order Replace message: removes the original order and
    ///        inserts its replacement at the new reference number, quantity,
    ///        and price.
    ///
    /// @param msg The parsed Order Replace message.
    auto handle_message(const OrderReplaceMessage& msg) -> void;

    /// @brief Generic handler/sink for message types that do not impact book state.
    ///
    /// @tparam T The message type being ignored.
    /// @param msg The message instance, ignored.
    template <typename T>
    auto handle_message(const T& /* msg */) -> void { /* No-op for irrelevant messages */ }

    /// @brief Creates an Order object and inserts it into the appropriate PriceLevel.
    ///
    /// @param order_ref The exchange-assigned order reference number.
    /// @param side 'B' for Buy, 'S' for Sell; selects the bid or ask side.
    /// @param shares The initial quantity of shares in the order.
    /// @param price The limit price of the order.
    /// @param stock The stock symbol for this order.
    auto add_order(
        uint64_t order_ref, char side, uint32_t shares, uint32_t price, const std::string& stock
    ) -> void;

    /// @brief Reduces shares from an order or removes it entirely if fully depleted.
    ///
    /// @param order_ref The unique reference number of the target order.
    /// @param canceled_shares The number of shares to remove.
    auto remove_order(uint64_t order_ref, uint32_t canceled_shares) -> void;
};

}  // namespace itch
