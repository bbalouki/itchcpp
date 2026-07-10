#pragma once

/// @file
/// @brief Reconstruction of Nasdaq auction (cross) events from the ITCH feed.
///
/// Combines the Net Order Imbalance Indicator (NOII) messages leading up to a
/// cross with the Cross Trade message that prints it, producing a single
/// `Auction` record per opening, closing, halt, or IPO cross.
///
/// @author Bertin Balouki SIMYELI

#include <array>
#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <variant>

#include "itch/analytics/imbalance.hpp"
#include "itch/indicators.hpp"
#include "itch/messages.hpp"
#include "itch/price.hpp"

namespace itch::analytics {

/// @brief A reconstructed auction (cross) event.
///
/// Nasdaq runs the opening, closing, halt, and IPO crosses; their price discovery
/// is disseminated through the NOII (`I`) messages leading up to the cross and the
/// Cross Trade (`Q`) message that prints it. This record pairs the executed cross
/// with the imbalance state that immediately preceded it.
struct Auction {
    std::uint64_t timestamp {0};
    std::uint16_t stock_locate {0};
    std::string   stock;
    char          cross_type {'\0'};     ///< 'O' open, 'C' close, 'H' halt/pause, 'I' IPO.
    StandardPrice cross_price {};        ///< The cross execution price.
    std::uint64_t cross_shares {0};      ///< Shares executed in the cross.
    std::uint64_t paired_shares {0};     ///< Paired shares from the latest NOII.
    std::uint64_t imbalance_shares {0};  ///< Imbalance shares from the latest NOII.
    char          imbalance_direction {'\0'};
    bool          had_imbalance {false};  ///< Whether NOII context was available.
};

/// @brief A human-readable description of a cross type code.
///
/// @param cross_type The single-character cross type code (e.g. 'O', 'C', 'H', 'I').
/// @return A short description of `cross_type`, or "Unknown" if unrecognized.
[[nodiscard]] inline auto cross_type_name(char cross_type) -> std::string_view {
    constexpr indicators::FrozenMap types {std::to_array<std::pair<char, std::string_view>>({
        {'O', "Opening Cross"},
        {'C', "Closing Cross"},
        {'H', "Cross for halted/paused security"},
        {'I', "Intraday/IPO Cross"},
    })};
    return types.at_or(cross_type, "Unknown");
}

/// @brief Reconstructs auctions from the NOII and Cross Trade message stream.
///
/// Feed every message with `process`: NOII (`I`) messages update the latest
/// imbalance state per security, and a Cross Trade (`Q`) finalizes an `Auction`
/// using the executed cross price together with that stored imbalance context,
/// which is delivered to the auction callback.
class AuctionTracker {
   public:
    using AuctionCallback = std::function<void(const Auction&)>;

    /// @brief Installs the callback invoked for each reconstructed auction.
    ///
    /// @param callback Function invoked with each `Auction` reconstructed from the stream.
    auto set_auction_callback(AuctionCallback callback) -> void {
        m_callback = std::move(callback);
    }

    /// @brief Processes one ITCH message, emitting an auction on each cross.
    ///
    /// @param message The ITCH message to process; only NOII and Cross Trade messages
    ///                affect state.
    auto process(const Message& message) -> void {
        if (const auto* noii = std::get_if<NOIIMessage>(&message)) {
            m_latest_imbalance[noii->stock_locate] = make_imbalance_info(*noii);
            return;
        }
        if (const auto* cross = std::get_if<CrossTradeMessage>(&message)) {
            Auction auction {};
            auction.timestamp    = cross->timestamp;
            auction.stock_locate = cross->stock_locate;
            auction.stock        = to_string(cross->stock, STOCK_LEN);
            auction.cross_type   = cross->cross_type;
            auction.cross_price  = StandardPrice {cross->cross_price};
            auction.cross_shares = cross->shares;

            const auto iter = m_latest_imbalance.find(cross->stock_locate);
            if (iter != m_latest_imbalance.end()) {
                auction.paired_shares       = iter->second.paired_shares;
                auction.imbalance_shares    = iter->second.imbalance_shares;
                auction.imbalance_direction = iter->second.imbalance_direction;
                auction.had_imbalance       = true;
            }
            if (m_callback) {
                m_callback(auction);
            }
            return;
        }
    }

   private:
    std::unordered_map<std::uint16_t, ImbalanceInfo> m_latest_imbalance;
    AuctionCallback                                  m_callback {};
};

}  // namespace itch::analytics
