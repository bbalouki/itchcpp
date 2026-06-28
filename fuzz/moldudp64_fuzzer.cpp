#include <cstddef>
#include <cstdint>
#include <span>

#include "itch/transport/moldudp64.hpp"
#include "itch/transport/pcap.hpp"
#include "itch/transport/soupbintcp.hpp"

// libFuzzer entry point for the transport decoders, which sit in front of the
// parser on untrusted network input. Arbitrary bytes are pushed through the
// MoldUDP64, SoupBinTCP, and pcap paths; any crash, overread, or sanitizer
// report is a genuine bug. None of these paths throw on malformed input by
// design, so there is nothing to catch.
extern "C" auto LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size) -> int {
    const auto* bytes = static_cast<const std::byte*>(static_cast<const void*>(data));
    const std::span<const std::byte> buffer {bytes, size};

    itch::transport::MoldUdp64Decoder mold {[](const itch::Message&) noexcept {}};
    mold.decode_packet(buffer);

    itch::transport::SoupBinDecoder soup {[](const itch::Message&) noexcept {}};
    soup.feed(buffer);

    itch::transport::PcapReader pcap {[](const itch::Message&) noexcept {}};
    pcap.read(buffer);

    return 0;
}
