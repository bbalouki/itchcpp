#pragma once

/// @file
/// @brief Strongly typed, fixed-point price representation for ITCH price fields.
///
/// Wraps the raw on-wire integer price in a type that carries its own decimal
/// scale, so standard 4-decimal prices and MWCB 8-decimal decline-level prices
/// cannot be mixed, compared, or formatted with the wrong divisor by accident.
///
/// @author Bertin Balouki SIMYELI

#include <cstdint>
#include <format>
#include <string>
#include <type_traits>

namespace itch {

/// @brief A strongly typed fixed-point price carrying its own decimal scale.
///
/// ITCH prices are wire integers with an implied number of decimal places.
/// Standard price fields imply 4 decimals (divisor 10,000); MWCB decline-level
/// prices imply 8 decimals (divisor 100,000,000). Exposing both as a bare
/// `uint32_t`/`uint64_t` makes it trivially easy to divide by the wrong scale
/// and silently produce a wrong price.
///
/// `BasicPrice` encodes the scale in the type. The two scales are therefore
/// distinct types (`StandardPrice` vs `MwcbPrice`) that cannot be mixed,
/// compared, or assigned to one another by accident: the 4-vs-8 decimal mistake
/// becomes a compile error rather than a runtime bug.
///
/// @tparam RawType The unsigned integer type of the on-wire value.
/// @tparam Decimals The number of implied decimal places.
template <typename RawType, unsigned int Decimals>
class BasicPrice {
    static_assert(std::is_unsigned_v<RawType>, "Price raw storage must be an unsigned integer");

   public:
    using raw_type = RawType;

    /// @brief The number of implied decimal places for this price scale.
    static constexpr unsigned int decimals = Decimals;

    /// @brief Constructs a zero-valued price at this scale.
    constexpr BasicPrice() noexcept = default;

    /// @brief Wraps a raw on-wire price value at this scale.
    ///
    /// @param raw_value The raw integer price, exactly as it appears on the wire.
    explicit constexpr BasicPrice(raw_type raw_value) noexcept : m_raw {raw_value} {}

    /// @brief The underlying raw integer value, exactly as it appears on the wire.
    ///
    /// @return The raw integer price at this scale.
    [[nodiscard]] constexpr auto raw() const noexcept -> raw_type { return m_raw; }

    /// @brief The scale's divisor, i.e. 10^Decimals.
    ///
    /// @return The divisor used to convert the raw integer value into a
    ///         floating-point or decimal-string representation.
    [[nodiscard]] static constexpr auto divisor() noexcept -> raw_type {
        raw_type result {1};
        for (unsigned int exponent {0}; exponent < Decimals; ++exponent) {
            result *= 10;
        }
        return result;
    }

    /// @brief The price as a floating-point value (raw / 10^Decimals).
    ///
    /// @return The price converted to a `double`.
    [[nodiscard]] constexpr auto to_double() const noexcept -> double {
        return static_cast<double>(m_raw) / static_cast<double>(divisor());
    }

    /// @brief The price as a fixed-precision decimal string.
    ///
    /// @return The price formatted with exactly `Decimals` fractional digits.
    [[nodiscard]] auto to_string() const -> std::string {
        return std::format("{:.{}f}", to_double(), Decimals);
    }

    /// @brief Compares two prices of the same scale for equality by raw value.
    ///
    /// @param lhs The left-hand price.
    /// @param rhs The right-hand price.
    /// @return `true` if both prices have the same raw value.
    [[nodiscard]] friend constexpr auto operator==(BasicPrice, BasicPrice) noexcept -> bool =
                                                                                           default;
    /// @brief Orders two prices of the same scale by raw value.
    ///
    /// @param lhs The left-hand price.
    /// @param rhs The right-hand price.
    /// @return The three-way comparison result between `lhs` and `rhs`.
    [[nodiscard]] friend constexpr auto operator<=>(BasicPrice, BasicPrice) noexcept = default;

   private:
    raw_type m_raw {0};
};

/// @brief Price scale for every ITCH price field except MWCB decline levels.
using StandardPrice = BasicPrice<std::uint32_t, 4>;

/// @brief Price scale for MWCB decline-level prices (8 implied decimals).
using MwcbPrice = BasicPrice<std::uint64_t, 8>;

/// @brief Wraps a raw 4-decimal price value in the typed StandardPrice.
///
/// @param raw_value The raw on-wire 4-decimal price.
/// @return The value wrapped as a `StandardPrice`.
[[nodiscard]] constexpr auto make_price(std::uint32_t raw_value) noexcept -> StandardPrice {
    return StandardPrice {raw_value};
}

/// @brief Wraps a raw 8-decimal MWCB price value in the typed MwcbPrice.
///
/// @param raw_value The raw on-wire 8-decimal MWCB decline-level price.
/// @return The value wrapped as an `MwcbPrice`.
[[nodiscard]] constexpr auto make_mwcb_price(std::uint64_t raw_value) noexcept -> MwcbPrice {
    return MwcbPrice {raw_value};
}

}  // namespace itch

/// @brief Formats a price with its scale's number of decimals, e.g. "150.0000".
template <typename RawType, unsigned int Decimals>
struct std::formatter<itch::BasicPrice<RawType, Decimals>> : std::formatter<double> {
    /// @brief Formats `price` as a fixed-precision decimal string.
    ///
    /// @param price The price to format.
    /// @param ctx The format context to write the formatted output into.
    /// @return An output iterator positioned after the formatted text, per the
    ///         `std::formatter` contract.
    auto format(itch::BasicPrice<RawType, Decimals> price, std::format_context& ctx) const {
        return std::formatter<double>::format(price.to_double(), ctx);
    }
};
