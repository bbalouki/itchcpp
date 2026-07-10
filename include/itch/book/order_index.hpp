#pragma once

/// @file
/// @brief Allocation-light, open-addressed hash map from order reference
///        number to order-pool index.
///
/// This header declares `OrderIndex`, the lookup structure `L3Book` uses to
/// resolve an ITCH order reference number to its slot in the order pool in
/// O(1) without the per-insert/per-erase heap allocations of
/// `std::unordered_map`.
///
/// @author Bertin Balouki SIMYELI

#include <cstddef>
#include <cstdint>
#include <vector>

namespace itch::book {

/// @brief A flat, open-addressed hash map from order reference number to pool
///        index, used for O(1) order lookup without per-order heap allocation.
///
/// `std::unordered_map` allocates a node on every insert and frees one on every
/// erase, which on a feed of hundreds of millions of add/delete messages dominates
/// the book's cost and defeats the allocation-free goal. This map stores its slots
/// in a single contiguous vector, probes linearly (cache friendly), and uses
/// backward-shift deletion so heavy add/cancel churn does not accumulate
/// tombstones. It only allocates when it grows.
class OrderIndex {
   public:
    /// @brief Sentinel returned by `find` when a key is absent.
    static constexpr std::uint32_t NPOS = 0xFFFFFFFFU;

    /// @brief Constructs an empty map with the initial table capacity
    ///        pre-allocated.
    OrderIndex() { rehash(INITIAL_CAPACITY); }

    /// @brief The number of stored keys.
    /// @return The count of stored keys.
    [[nodiscard]] auto size() const noexcept -> std::size_t { return m_count; }

    /// @brief Whether the map holds no keys.
    /// @return True if the map holds no keys, false otherwise.
    [[nodiscard]] auto empty() const noexcept -> bool { return m_count == 0; }

    /// @brief Returns the value for `key`, or `NPOS` if absent.
    /// @param key The order reference number to look up.
    /// @return The stored pool index for `key`, or `NPOS` if absent.
    [[nodiscard]] auto find(std::uint64_t key) const noexcept -> std::uint32_t {
        std::size_t slot = hash(key) & m_mask;
        while (m_slots[slot].used) {
            if (m_slots[slot].key == key) {
                return m_slots[slot].value;
            }
            slot = (slot + 1) & m_mask;
        }
        return NPOS;
    }

    /// @brief Whether `key` is present.
    /// @param key The order reference number to check.
    /// @return True if `key` is present, false otherwise.
    [[nodiscard]] auto contains(std::uint64_t key) const noexcept -> bool {
        return find(key) != NPOS;
    }

    /// @brief Inserts or overwrites the value for `key`.
    /// @param key The order reference number to insert or update.
    /// @param value The pool index to associate with `key`.
    auto insert(std::uint64_t key, std::uint32_t value) -> void {
        if ((m_count + 1) * LOAD_FACTOR_DEN >= m_slots.size() * LOAD_FACTOR_NUM) {
            rehash(m_slots.size() * 2);
        }
        std::size_t slot = hash(key) & m_mask;
        while (m_slots[slot].used) {
            if (m_slots[slot].key == key) {
                m_slots[slot].value = value;
                return;
            }
            slot = (slot + 1) & m_mask;
        }
        m_slots[slot] = Slot {key, value, true};
        ++m_count;
    }

    /// @brief Removes `key` if present, repairing the probe chain in place.
    /// @param key The order reference number to remove.
    auto erase(std::uint64_t key) -> void {
        std::size_t slot = hash(key) & m_mask;
        while (m_slots[slot].used) {
            if (m_slots[slot].key == key) {
                remove_at(slot);
                return;
            }
            slot = (slot + 1) & m_mask;
        }
    }

    /// @brief Drops all keys (retaining capacity).
    auto clear() -> void {
        for (auto& slot : m_slots) {
            slot.used = false;
        }
        m_count = 0;
    }

   private:
    struct Slot {
        std::uint64_t key {0};
        std::uint32_t value {0};
        bool          used {false};
    };

    static constexpr std::size_t INITIAL_CAPACITY = 1024;
    static constexpr std::size_t LOAD_FACTOR_NUM  = 7;  // Grow past 70% load.
    static constexpr std::size_t LOAD_FACTOR_DEN  = 10;

    /// @brief Computes a finalizing hash mix (splitmix64) of a key so dense,
    ///        sequential reference numbers spread across the table instead of
    ///        clustering.
    /// @param key The order reference number to hash.
    /// @return The mixed hash value used to select a probe slot.
    [[nodiscard]] static auto hash(std::uint64_t key) noexcept -> std::size_t {
        key ^= key >> 33;
        key *= 0xFF51AFD7ED558CCDULL;
        key ^= key >> 33;
        key *= 0xC4CEB9FE1A85EC53ULL;
        key ^= key >> 33;
        return static_cast<std::size_t>(key);
    }

    /// @brief Removes the entry at `hole` and backward-shifts any entries in
    ///        its probe chain so lookups for other keys are not broken by the
    ///        gap.
    /// @param hole The slot index of the entry to remove.
    auto remove_at(std::size_t hole) -> void {
        m_slots[hole].used = false;
        --m_count;
        std::size_t next = (hole + 1) & m_mask;
        while (m_slots[next].used) {
            const std::size_t home = hash(m_slots[next].key) & m_mask;
            // Move the entry into the hole if its home position is not within the
            // cyclic interval (hole, next], i.e. the hole sits between its home
            // and its current slot.
            const bool movable =
                (hole <= next) ? (home <= hole || home > next) : (home <= hole && home > next);
            if (movable) {
                m_slots[hole]      = m_slots[next];
                m_slots[next].used = false;
                hole               = next;
            }
            next = (next + 1) & m_mask;
        }
    }

    /// @brief Grows (or initializes) the table to `new_capacity` and
    ///        re-inserts every previously stored key.
    /// @param new_capacity The new table capacity, must be a power of two.
    auto rehash(std::size_t new_capacity) -> void {
        std::vector<Slot> old = std::move(m_slots);
        m_slots.assign(new_capacity, Slot {});
        m_mask  = new_capacity - 1;
        m_count = 0;
        for (const auto& slot : old) {
            if (slot.used) {
                insert(slot.key, slot.value);
            }
        }
    }

    std::vector<Slot> m_slots;
    std::size_t       m_count {0};
    std::size_t       m_mask {0};
};

}  // namespace itch::book
