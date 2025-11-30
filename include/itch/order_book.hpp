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

// Represent a single order in the order book.
struct Order {
    uint64_t    order_reference_number;
    char        buy_sell_indicator;
    uint32_t    shares;
    uint32_t    price;
    PriceLevel* level;

    Order(uint64_t ref_num, char side, uint32_t shrs, uint32_t prc)
        : order_reference_number{ref_num},
          buy_sell_indicator{side},
          shares{shrs},
          price{prc},
          level{nullptr} {}
};

using OrderIt = std::list<std::shared_ptr<Order>>::iterator;

// Represent a price level in the order book.
// Contains a list of orders at that price, in time priority.
struct PriceLevel {
    uint32_t                          total_shares{0};
    std::list<std::shared_ptr<Order>> orders;

    void add_order(std::shared_ptr<Order> order);
    void remove_order(OrderIt order_it);
};

/**
 * @class LimitOrderBook
 * @brief A class that represents and maintains a limit order book for a single stock.
 *
 * The LimitOrderBook processes ITCH 5.0 messages to build and maintain the
 * state of the order book. It handles adding, executing, canceling, deleting,
 * and replacing orders.
 */
class LimitOrderBook {
   public:
    /**
     * @brief Processes a single ITCH message and updates the order book accordingly.
     * @param message The ITCH message to process.
     */
    void process(const Message& message);

    /**
     * @brief Prints the current state of the order book to the given output stream.
     * @param out The output stream to print to.
     */
    void print(std::ostream& out, unsigned int delay_ms = 0) const;

    // Type aliases for the bid and ask maps.
    using BidMap = std::map<uint32_t, PriceLevel, std::greater<uint32_t>>;
    using AskMap = std::map<uint32_t, PriceLevel, std::less<uint32_t>>;

    const BidMap&        get_bids() const { return m_bids; }
    const AskMap&        get_asks() const { return m_asks; }
    const std::set<char> book_messages{'A', 'F', 'E', 'C', 'X', 'D', 'U'};

   private:
    BidMap                      m_bids;    ///< Map of bid price levels (price -> PriceLevel)
    AskMap                      m_asks;    ///< Map of ask price levels (price -> PriceLevel)
    std::map<uint64_t, OrderIt> m_orders;  ///< Map of orders by reference number

    // Handler methods for specific message types
    void handle_message(const AddOrderMessage& msg);
    void handle_message(const AddOrderMPIDAttributionMessage& msg);
    void handle_message(const OrderExecutedMessage& msg);
    void handle_message(const OrderExecutedWithPriceMessage& msg);
    void handle_message(const OrderCancelMessage& msg);
    void handle_message(const OrderDeleteMessage& msg);
    void handle_message(const OrderReplaceMessage& msg);
    // Generic handler for message types we don't care about
    template <typename T>
    void handle_message(const T& msg) { /* Ignore */ }

    // Helper methods
    void add_order(uint64_t order_ref, char side, uint32_t shares, uint32_t price);
    void remove_order(uint64_t order_ref, uint32_t canceled_shares);
};

}  // namespace itch
