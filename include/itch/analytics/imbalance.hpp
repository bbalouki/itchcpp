#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>

#include "itch/indicators.hpp"
#include "itch/messages.hpp"
#include "itch/price.hpp"

namespace itch::analytics {

/// @brief A usable, decoded view of a Net Order Imbalance Indicator (`I`) message.
///
/// The feed already carries the NOII data used during the opening and closing
/// crosses; this surfaces it with strong price types and a trimmed symbol instead
/// of the raw packed fields.
struct ImbalanceInfo {
    std::uint64_t timestamp {0};
    std::uint16_t stock_locate {0};
    std::string   stock;
    std::uint64_t paired_shares {0};
    std::uint64_t imbalance_shares {0};
    char          imbalance_direction {'\0'};
    StandardPrice far_price {};
    StandardPrice near_price {};
    StandardPrice current_reference_price {};
    char          cross_type {'\0'};
    char          price_variation_indicator {'\0'};
};

/// @brief Builds an `ImbalanceInfo` from a raw NOII message.
[[nodiscard]] inline auto make_imbalance_info(const NOIIMessage& msg) -> ImbalanceInfo {
    ImbalanceInfo info {};
    info.timestamp                 = msg.timestamp;
    info.stock_locate              = msg.stock_locate;
    info.stock                     = to_string(msg.stock, STOCK_LEN);
    info.paired_shares             = msg.paired_shares;
    info.imbalance_shares          = msg.imbalance_shares;
    info.imbalance_direction       = msg.imbalance_direction;
    info.far_price                 = StandardPrice {msg.far_price};
    info.near_price                = StandardPrice {msg.near_price};
    info.current_reference_price   = StandardPrice {msg.current_reference_price};
    info.cross_type                = msg.cross_type;
    info.price_variation_indicator = msg.price_variation_indicator;
    return info;
}

/// @brief A human-readable description of an imbalance direction code.
[[nodiscard]] inline auto imbalance_direction_name(char direction) -> std::string_view {
    constexpr indicators::FrozenMap directions {std::to_array<std::pair<char, std::string_view>>({
        {'B', "Buy imbalance"},
        {'S', "Sell imbalance"},
        {'N', "No imbalance"},
        {'O', "Insufficient orders to calculate"},
        {'P', "Paused"},
    })};
    return directions.at_or(direction, "Unknown");
}

}  // namespace itch::analytics
