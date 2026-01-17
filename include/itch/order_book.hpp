#pragma once

#include <cstdint>
#include <functional>
#include <iomanip>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <variant>

#include "itch/messages.hpp"

namespace itch {

struct PriceLevel;

/**
 * @struct Order
 * @brief Represents a single resting order within the Limit Order Book.
 *
 * Encapsulates all necessary state data for an order, including its unique
 * reference number, side (Buy/Sell), quantity, and price. It also maintains
 * a pointer to its parent PriceLevel for efficient traversal.
 */
struct Order {
    uint64_t order_reference_number;  ///< Unique identifier assigned to the order by the exchange.
    char     buy_sell_indicator;      ///< 'B' for Buy, 'S' for Sell.
    uint32_t shares;                  ///< Current quantity of shares available in this order.
    uint32_t price;                   ///< Limit price of the order.
    PriceLevel* level;                ///< Pointer to the price level containing this order.
    std::string stock;                ///< The stock symbol for this order.

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

/**
 * @struct PriceLevel
 * @brief Represents a specific price node in the order book.
 *
 * Maintains a queue of orders at a specific price, enforcing Time-Priority
 * (FIFO) execution. It tracks the aggregate share volume for this level.
 */
struct PriceLevel {
    uint32_t total_shares {0};                 ///< Aggregate volume of shares at this price.
    std::list<std::shared_ptr<Order>> orders;  ///< FIFO queue of orders.

    /**
     * @brief Appends an order to the end of the queue (Time priority).
     * @param order Shared pointer to the order to add.
     */
    void add_order(std::shared_ptr<Order> order);

    /**
     * @brief Removes a specific order from the queue.
     * @param order_it Iterator pointing to the order to remove.
     */
    void remove_order(OrderIt order_it);
};

/**
 * @class LimitOrderBook
 * @brief Manages the state of a limit order book for a specific financial instrument.
 *
 * The LimitOrderBook ingests raw Nasdaq ITCH 5.0 messages and reconstructs the
 * full depth of market. It supports standard order lifecycle events including
 * addition, execution, cancellation, deletion, and replacement.
 *
 * @note This implementation uses `std::map` for price levels to ensure
 * keys (prices) are always sorted, allowing for O(log n) insertion and deletion
 * of price levels, and O(1) access to the best bid/ask.
 */
class LimitOrderBook {
   public:
    LimitOrderBook(const std::string& stock_symbol) : m_stock_symbol(stock_symbol) {}
    /**
     * @brief Dispatches and processes a generic ITCH message.
     *
     * Identifies the specific type within the `Message` variant and routes it
     * to the appropriate internal handler to update the book state.
     *
     * @param message The parsed ITCH message variant.
     */
    void process(const Message& message);

    /**
     * @brief Visualizes the current state of the order book to an output stream.
     *
     * Prints the best Ask and Bid levels formatted in a table.
     *
     * @param out The output stream (e.g., std::cout) to write to.
     * @param delay_ms Optional delay in milliseconds between printing each line.
     *                 Useful for slowing down output during debugging or replays
     *                 to create an animation effect. Defaults to 0 (no delay).
     */
    void print(std::ostream& out, unsigned int delay_ms = 0) const;

    // Type aliases for the bid and ask maps (sorted containers).
    using BidMap = std::map<uint32_t, PriceLevel, std::greater<uint32_t>>;
    using AskMap = std::map<uint32_t, PriceLevel, std::less<uint32_t>>;

    /**
     * @return A reference to the map of active Bids, sorted descending by price.
     */
    const BidMap& get_bids() const { return m_bids; }

    /**
     * @return A reference to the map of active Asks, sorted ascending by price.
     */
    const AskMap& get_asks() const { return m_asks; }

    /// Set of message type characters that affect the book state.
    const std::set<char> book_messages {'A', 'F', 'E', 'C', 'X', 'D', 'U'};

   private:
    std::string                 m_stock_symbol;
    BidMap                      m_bids;  ///< Ask price levels (Price -> Level). Sorted Low-to-High.
    AskMap                      m_asks;  ///< Bid price levels (Price -> Level). Sorted High-to-Low.
    std::map<uint64_t, OrderIt> m_orders;  ///< Hash map for O(1) order lookup by Reference Number.

    // Message Handlers
    void handle_message(const AddOrderMessage& msg);
    void handle_message(const AddOrderMPIDAttributionMessage& msg);
    void handle_message(const OrderExecutedMessage& msg);
    void handle_message(const OrderExecutedWithPriceMessage& msg);
    void handle_message(const OrderCancelMessage& msg);
    void handle_message(const OrderDeleteMessage& msg);
    void handle_message(const OrderReplaceMessage& msg);

    // Generic handler/sink for message types that do not impact book state.
    template <typename T>
    void handle_message(const T& /* msg */) { /* No-op for irrelevant messages */ }

    /**
     * @brief Creates an Order object and inserts it into the appropriate PriceLevel.
     */
    void add_order(
        uint64_t order_ref, char side, uint32_t shares, uint32_t price, const std::string& stock
    );

    /**
     * @brief Reduces shares from an order or removes it entirely if fully depleted.
     *
     * @param order_ref The unique reference number of the target order.
     * @param canceled_shares The number of shares to remove.
     */
    void remove_order(uint64_t order_ref, uint32_t canceled_shares);
};

}  // namespace itch
