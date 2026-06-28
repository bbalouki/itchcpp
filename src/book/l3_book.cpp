#include "itch/book/l3_book.hpp"

#include <algorithm>
#include <utility>

namespace itch::book {

L3Book::L3Book(std::string symbol) : m_symbol {std::move(symbol)} {}

auto L3Book::side_levels(Side side) noexcept -> std::vector<Level>& {
    return side == Side::buy ? m_bids : m_asks;
}

auto L3Book::side_levels(Side side) const noexcept -> const std::vector<Level>& {
    return side == Side::buy ? m_bids : m_asks;
}

auto L3Book::allocate_node() -> std::uint32_t {
    if (m_free_head != NIL) {
        const std::uint32_t index = m_free_head;
        m_free_head               = m_pool[index].next;
        m_pool[index]             = OrderNode {};
        return index;
    }
    m_pool.push_back(OrderNode {});
    return static_cast<std::uint32_t>(m_pool.size() - 1);
}

auto L3Book::free_node(std::uint32_t node_index) -> void {
    m_pool[node_index]      = OrderNode {};
    m_pool[node_index].next = m_free_head;
    m_free_head             = node_index;
}

auto L3Book::find_level(Side side, std::uint32_t price) const -> std::uint32_t {
    const auto& levels = side_levels(side);
    // Bids are sorted descending, asks ascending; binary search accordingly.
    if (side == Side::buy) {
        const auto iter = std::lower_bound(
            levels.begin(), levels.end(), price, [](const Level& level, std::uint32_t value) {
                return level.price > value;
            }
        );
        if (iter != levels.end() && iter->price == price) {
            return static_cast<std::uint32_t>(iter - levels.begin());
        }
    } else {
        const auto iter = std::lower_bound(
            levels.begin(), levels.end(), price, [](const Level& level, std::uint32_t value) {
                return level.price < value;
            }
        );
        if (iter != levels.end() && iter->price == price) {
            return static_cast<std::uint32_t>(iter - levels.begin());
        }
    }
    return NIL;
}

auto L3Book::find_or_create_level(Side side, std::uint32_t price) -> std::uint32_t {
    auto& levels = side_levels(side);
    auto  position =
        side == Side::buy
            ? std::lower_bound(
                  levels.begin(),
                  levels.end(),
                  price,
                  [](const Level& level, std::uint32_t value) { return level.price > value; }
              )
            : std::lower_bound(
                  levels.begin(), levels.end(), price, [](const Level& level, std::uint32_t value) {
                      return level.price < value;
                  }
              );
    if (position != levels.end() && position->price == price) {
        return static_cast<std::uint32_t>(position - levels.begin());
    }
    Level new_level {};
    new_level.price     = price;
    const auto inserted = levels.insert(position, new_level);
    return static_cast<std::uint32_t>(inserted - levels.begin());
}

auto L3Book::add_order(
    std::uint64_t reference_number, Side side, std::uint32_t shares, std::uint32_t price
) -> void {
    if (m_index.find(reference_number) != OrderIndex::NPOS) {
        return;  // Duplicate add; ignore to keep the book consistent.
    }
    const std::uint32_t node_index = allocate_node();
    OrderNode&          node       = m_pool[node_index];
    node.reference_number          = reference_number;
    node.shares                    = shares;
    node.price                     = price;
    node.side                      = side;

    const std::uint32_t level_index = find_or_create_level(side, price);
    Level&              level       = side_levels(side)[level_index];

    // Append to the tail of the level FIFO to preserve time priority.
    node.prev = level.tail;
    node.next = NIL;
    if (level.tail != NIL) {
        m_pool[level.tail].next = node_index;
    } else {
        level.head = node_index;
    }
    level.tail = node_index;
    level.total_shares += shares;
    ++level.order_count;

    m_index.insert(reference_number, node_index);
}

auto L3Book::unlink_node(std::uint32_t node_index) -> void {
    OrderNode&          node        = m_pool[node_index];
    const std::uint32_t level_index = find_level(node.side, node.price);
    if (level_index == NIL) {
        return;
    }
    auto&  levels = side_levels(node.side);
    Level& level  = levels[level_index];

    if (node.prev != NIL) {
        m_pool[node.prev].next = node.next;
    } else {
        level.head = node.next;
    }
    if (node.next != NIL) {
        m_pool[node.next].prev = node.prev;
    } else {
        level.tail = node.prev;
    }
    level.total_shares -= node.shares;
    --level.order_count;
    if (level.order_count == 0) {
        levels.erase(levels.begin() + level_index);
    }
}

auto L3Book::execute_order(std::uint64_t reference_number, std::uint32_t shares) -> std::uint32_t {
    const std::uint32_t node_index = m_index.find(reference_number);
    if (node_index == OrderIndex::NPOS) {
        return 0;
    }
    OrderNode&          node    = m_pool[node_index];
    const std::uint32_t removed = std::min(shares, node.shares);

    if (removed == node.shares) {
        unlink_node(node_index);
        free_node(node_index);
        m_index.erase(reference_number);
        return removed;
    }

    node.shares -= removed;
    const std::uint32_t level_index = find_level(node.side, node.price);
    if (level_index != NIL) {
        side_levels(node.side)[level_index].total_shares -= removed;
    }
    return removed;
}

auto L3Book::reduce_order(std::uint64_t reference_number, std::uint32_t shares) -> std::uint32_t {
    // A partial cancel removes shares exactly as an execution does.
    return execute_order(reference_number, shares);
}

auto L3Book::delete_order(std::uint64_t reference_number) -> void {
    const std::uint32_t node_index = m_index.find(reference_number);
    if (node_index == OrderIndex::NPOS) {
        return;
    }
    unlink_node(node_index);
    free_node(node_index);
    m_index.erase(reference_number);
}

auto L3Book::replace_order(
    std::uint64_t old_reference_number,
    std::uint64_t new_reference_number,
    std::uint32_t shares,
    std::uint32_t price
) -> void {
    const std::uint32_t node_index = m_index.find(old_reference_number);
    if (node_index == OrderIndex::NPOS) {
        return;
    }
    const Side side = m_pool[node_index].side;
    delete_order(old_reference_number);
    add_order(new_reference_number, side, shares, price);
}

auto L3Book::contains(std::uint64_t reference_number) const -> bool {
    return m_index.contains(reference_number);
}

auto L3Book::order_price(std::uint64_t reference_number) const -> std::optional<std::uint32_t> {
    const std::uint32_t node_index = m_index.find(reference_number);
    if (node_index == OrderIndex::NPOS) {
        return std::nullopt;
    }
    return m_pool[node_index].price;
}

auto L3Book::order_side(std::uint64_t reference_number) const -> std::optional<Side> {
    const std::uint32_t node_index = m_index.find(reference_number);
    if (node_index == OrderIndex::NPOS) {
        return std::nullopt;
    }
    return m_pool[node_index].side;
}

auto L3Book::bbo() const -> Bbo {
    Bbo result {};
    if (!m_bids.empty()) {
        result.has_bid    = true;
        result.bid_price  = StandardPrice {m_bids.front().price};
        result.bid_shares = m_bids.front().total_shares;
    }
    if (!m_asks.empty()) {
        result.has_ask    = true;
        result.ask_price  = StandardPrice {m_asks.front().price};
        result.ask_shares = m_asks.front().total_shares;
    }
    return result;
}

auto L3Book::depth(Side side, std::size_t max_levels) const -> std::vector<DepthLevel> {
    const auto&       levels = side_levels(side);
    const std::size_t count = max_levels == 0 ? levels.size() : std::min(max_levels, levels.size());
    std::vector<DepthLevel> result;
    result.reserve(count);
    for (std::size_t index = 0; index < count; ++index) {
        result.push_back(
            DepthLevel {
                StandardPrice {levels[index].price},
                levels[index].total_shares,
                levels[index].order_count
            }
        );
    }
    return result;
}

auto L3Book::orders_at(Side side, std::uint32_t price) const -> std::vector<OrderView> {
    std::vector<OrderView> result;
    const std::uint32_t    level_index = find_level(side, price);
    if (level_index == NIL) {
        return result;
    }
    const Level& level = side_levels(side)[level_index];
    result.reserve(level.order_count);
    for (std::uint32_t node_index = level.head; node_index != NIL;
         node_index               = m_pool[node_index].next) {
        const OrderNode& node = m_pool[node_index];
        result.push_back(
            OrderView {node.reference_number, node.shares, StandardPrice {node.price}}
        );
    }
    return result;
}

auto L3Book::level_count(Side side) const noexcept -> std::size_t {
    return side_levels(side).size();
}

}  // namespace itch::book
