#pragma once

/// @file
/// @brief The extension point for adding new ITCH-like venues/protocol versions
///        alongside NASDAQ TotalView-ITCH 5.0.
///
/// Defines the `VenuePolicy` concept that a venue policy type must satisfy, and
/// `Nasdaq50`, the concrete policy for the only protocol this library
/// implements today.
///
/// @author Bertin Balouki SIMYELI

#include <concepts>
#include <string_view>

#include "itch/detail/wire.hpp"

namespace itch::venue {

/// @brief Concept describing a venue/protocol policy.
///
/// ITCHCPP today implements NASDAQ TotalView-ITCH 5.0 concretely. This concept is
/// the extension seam for adding ITCH-like venues and versions (NASDAQ BX/PSX,
/// older ITCH 4.1, and other venue feeds) without rewriting the dispatch
/// machinery: a policy names itself and enumerates its message-type registry, and
/// the dispatch-table builder can be parameterized on it.
///
/// A conforming policy provides:
///  - `static constexpr std::string_view name()` identifying the venue/version.
///  - `static void for_each_message_type(Visitor&&)` invoking
///    `visitor.template operator()<MsgType>(char)` once per message type, the same
///    shape as `itch::detail::for_each_message_type`.
template <typename Policy>
concept VenuePolicy = requires {
    { Policy::name() } -> std::convertible_to<std::string_view>;
};

/// @brief The concrete policy for NASDAQ TotalView-ITCH 5.0 (the only venue
///        implemented today). New venues are added by writing another policy of
///        the same shape and parameterizing the dispatch builder on it.
struct Nasdaq50 {
    /// @brief The identifying name of this venue/protocol version.
    ///
    /// @return The human-readable venue/version name.
    [[nodiscard]] static constexpr auto name() noexcept -> std::string_view {
        return "NASDAQ TotalView-ITCH 5.0";
    }

    /// @brief Enumerates this venue's message-type registry.
    ///
    /// @tparam Visitor The visitor type; must be callable as
    ///                 `visitor.template operator()<MsgType>(char)`.
    /// @param visitor The visitor invoked once per message type.
    template <typename Visitor>
    static constexpr auto for_each_message_type(Visitor&& visitor) -> void {
        detail::for_each_message_type(static_cast<Visitor&&>(visitor));
    }
};

static_assert(VenuePolicy<Nasdaq50>);

}  // namespace itch::venue
