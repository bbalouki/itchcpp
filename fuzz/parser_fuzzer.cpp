#include <cstddef>
#include <cstdint>
#include <exception>
#include <span>

#include "itch/parser.hpp"

// libFuzzer entry point. Feeds arbitrary bytes straight through the framing and
// dispatch path. Truncation is reported by `parse` as an exception, which is an
// expected outcome on random input and is swallowed; any crash, overread, or
// sanitizer report on this path is a genuine bug.
extern "C" auto LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size) -> int {
    itch::Parser parser;

    // std::byte and the incoming bytes may legitimately alias; route through
    // void* to avoid a forbidden reinterpret_cast.
    const auto* bytes = static_cast<const std::byte*>(static_cast<const void*>(data));
    const std::span<const std::byte> buffer {bytes, size};

    try {
        parser.parse(buffer, [](const itch::Message&) noexcept {});
    } catch (const std::exception&) {
        // Truncated input is expected and benign for the fuzzer.
    }
    return 0;
}
