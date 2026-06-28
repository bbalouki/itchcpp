#include "itch/transport/pcap.hpp"

#include <algorithm>
#include <cstring>
#include <fstream>
#include <utility>

namespace itch::transport {

namespace {

// Capture-file record fields follow the file's own byte order; network protocol
// headers (Ethernet/IP/UDP) are always big-endian. These helpers read either.
template <typename UintType>
auto read_le_or_be(std::span<const std::byte> data, std::size_t offset, bool swapped) -> UintType {
    UintType value {};
    std::memcpy(&value, data.data() + offset, sizeof(UintType));
    return swapped ? utils::swap_bytes(value) : value;
}

template <typename UintType>
auto read_net(std::span<const std::byte> data, std::size_t offset) -> UintType {
    UintType value {};
    std::memcpy(&value, data.data() + offset, sizeof(UintType));
    return utils::from_big_endian(value);
}

// Link-layer types we know how to unwrap (subset of the tcpdump/pcap registry).
constexpr std::uint32_t LINKTYPE_ETHERNET  = 1;
constexpr std::uint32_t LINKTYPE_RAW       = 101;
constexpr std::uint32_t LINKTYPE_LINUX_SLL = 113;

constexpr std::uint16_t ETHERTYPE_IPV4 = 0x0800;
constexpr std::uint16_t ETHERTYPE_IPV6 = 0x86DD;
constexpr std::uint16_t ETHERTYPE_VLAN = 0x8100;

constexpr std::uint8_t IP_PROTOCOL_UDP = 17;

constexpr std::size_t ETHERNET_HEADER_SIZE  = 14;
constexpr std::size_t VLAN_TAG_SIZE         = 4;
constexpr std::size_t LINUX_SLL_HEADER_SIZE = 16;
constexpr std::size_t IPV6_HEADER_SIZE      = 40;
constexpr std::size_t UDP_HEADER_SIZE       = 8;

constexpr std::uint32_t PCAP_MAGIC          = 0xA1B2C3D4;
constexpr std::uint32_t PCAP_MAGIC_SWAPPED  = 0xD4C3B2A1;
constexpr std::uint32_t PCAP_MAGIC_NANO     = 0xA1B23C4D;
constexpr std::uint32_t PCAP_MAGIC_NANO_SWP = 0x4D3CB2A1;
constexpr std::uint32_t PCAPNG_BLOCK_SHB    = 0x0A0D0D0A;

}  // namespace

PcapReader::PcapReader(MessageCallback callback) : m_mold {std::move(callback)} {}

auto PcapReader::read(std::span<const std::byte> capture) -> bool {
    if (capture.size() < sizeof(std::uint32_t)) {
        return false;
    }
    std::uint32_t magic {};
    std::memcpy(&magic, capture.data(), sizeof(magic));

    if (magic == PCAP_MAGIC || magic == PCAP_MAGIC_NANO) {
        return read_classic_pcap(capture, /*swapped=*/false);
    }
    if (magic == PCAP_MAGIC_SWAPPED || magic == PCAP_MAGIC_NANO_SWP) {
        return read_classic_pcap(capture, /*swapped=*/true);
    }
    if (magic == PCAPNG_BLOCK_SHB) {
        return read_pcapng(capture);
    }
    return false;
}

auto PcapReader::read_file(const std::string& path) -> bool {
    std::ifstream file {path, std::ios::binary};
    if (!file) {
        return false;
    }
    std::vector<char>      raw {std::istreambuf_iterator<char> {file}, {}};
    std::vector<std::byte> bytes(raw.size());
    std::memcpy(bytes.data(), raw.data(), raw.size());
    return read(std::span<const std::byte> {bytes});
}

auto PcapReader::read_classic_pcap(std::span<const std::byte> capture, bool swapped) -> bool {
    constexpr std::size_t GLOBAL_HEADER_SIZE = 24;
    constexpr std::size_t NETWORK_OFFSET     = 20;
    constexpr std::size_t RECORD_HEADER_SIZE = 16;
    constexpr std::size_t INCL_LEN_OFFSET    = 8;

    if (capture.size() < GLOBAL_HEADER_SIZE) {
        return false;
    }
    const auto link_type = read_le_or_be<std::uint32_t>(capture, NETWORK_OFFSET, swapped);

    std::size_t offset = GLOBAL_HEADER_SIZE;
    while (offset + RECORD_HEADER_SIZE <= capture.size()) {
        const auto incl_len =
            read_le_or_be<std::uint32_t>(capture, offset + INCL_LEN_OFFSET, swapped);
        offset += RECORD_HEADER_SIZE;
        if (offset + incl_len > capture.size()) {
            break;  // Truncated final record.
        }
        handle_frame(capture.subspan(offset, incl_len), link_type);
        offset += incl_len;
    }
    return true;
}

auto PcapReader::read_pcapng(std::span<const std::byte> capture) -> bool {
    constexpr std::size_t   BLOCK_HEADER_SIZE = 8;  // Type + total length.
    constexpr std::size_t   BOM_OFFSET        = 8;  // Byte-order magic in the SHB.
    constexpr std::uint32_t BOM_LITTLE        = 0x1A2B3C4D;
    constexpr std::uint32_t BLOCK_IDB         = 0x00000001;
    constexpr std::uint32_t BLOCK_SPB         = 0x00000003;
    constexpr std::uint32_t BLOCK_EPB         = 0x00000006;

    bool                       swapped = false;
    std::vector<std::uint32_t> interface_link_types;

    std::size_t offset = 0;
    while (offset + BLOCK_HEADER_SIZE <= capture.size()) {
        std::uint32_t block_type {};
        std::memcpy(&block_type, capture.data() + offset, sizeof(block_type));
        // The SHB's byte-order magic tells us how to read this section. Until we
        // have read it, block lengths are interpreted in host order, which is
        // correct for the SHB type/length fields we need to bootstrap from.
        const bool block_swapped = (block_type == PCAPNG_BLOCK_SHB) ? false : swapped;

        const auto total_length =
            read_le_or_be<std::uint32_t>(capture, offset + sizeof(std::uint32_t), block_swapped);
        if (total_length < BLOCK_HEADER_SIZE + sizeof(std::uint32_t) ||
            offset + total_length > capture.size()) {
            break;
        }

        if (block_type == PCAPNG_BLOCK_SHB) {
            const auto bom = read_net<std::uint32_t>(capture, offset + BOM_OFFSET);
            // If the byte-order magic reads as the canonical value in network
            // (big-endian) interpretation, the file is big-endian; otherwise it
            // is little-endian. Compare against both forms.
            std::uint32_t bom_native {};
            std::memcpy(&bom_native, capture.data() + offset + BOM_OFFSET, sizeof(bom_native));
            swapped = (bom_native != BOM_LITTLE);
            (void)bom;
            interface_link_types.clear();
        } else if (block_type == BLOCK_IDB) {
            // LinkType is the first 2-byte field of the IDB body.
            const auto link_type =
                read_le_or_be<std::uint16_t>(capture, offset + BLOCK_HEADER_SIZE, swapped);
            interface_link_types.push_back(link_type);
        } else if (block_type == BLOCK_EPB) {
            constexpr std::size_t EPB_IFACE_OFFSET  = BLOCK_HEADER_SIZE;
            constexpr std::size_t EPB_CAPLEN_OFFSET = BLOCK_HEADER_SIZE + 12;
            constexpr std::size_t EPB_DATA_OFFSET   = BLOCK_HEADER_SIZE + 20;
            const auto            iface =
                read_le_or_be<std::uint32_t>(capture, offset + EPB_IFACE_OFFSET, swapped);
            const auto cap_len =
                read_le_or_be<std::uint32_t>(capture, offset + EPB_CAPLEN_OFFSET, swapped);
            if (offset + EPB_DATA_OFFSET + cap_len <= capture.size() &&
                iface < interface_link_types.size()) {
                handle_frame(
                    capture.subspan(offset + EPB_DATA_OFFSET, cap_len), interface_link_types[iface]
                );
            }
        } else if (block_type == BLOCK_SPB) {
            constexpr std::size_t SPB_DATA_OFFSET = BLOCK_HEADER_SIZE + 4;
            // The SPB has no captured length, so derive it from the block size.
            if (total_length >= SPB_DATA_OFFSET + sizeof(std::uint32_t) &&
                !interface_link_types.empty()) {
                const std::size_t data_len = total_length - SPB_DATA_OFFSET - sizeof(std::uint32_t);
                handle_frame(
                    capture.subspan(offset + SPB_DATA_OFFSET, data_len),
                    interface_link_types.front()
                );
            }
        }

        offset += total_length;
    }
    return true;
}

auto PcapReader::handle_frame(std::span<const std::byte> frame, std::uint32_t link_type) -> void {
    // Strip the link layer down to the network (IP) header.
    std::span<const std::byte> network = frame;
    if (link_type == LINKTYPE_ETHERNET) {
        if (frame.size() < ETHERNET_HEADER_SIZE) {
            return;
        }
        std::size_t   header    = ETHERNET_HEADER_SIZE;
        std::uint16_t ethertype = read_net<std::uint16_t>(frame, 12);
        while (ethertype == ETHERTYPE_VLAN && frame.size() >= header + VLAN_TAG_SIZE) {
            ethertype = read_net<std::uint16_t>(frame, header + 2);
            header += VLAN_TAG_SIZE;
        }
        if (ethertype != ETHERTYPE_IPV4 && ethertype != ETHERTYPE_IPV6) {
            return;
        }
        network = frame.subspan(header);
    } else if (link_type == LINKTYPE_LINUX_SLL) {
        if (frame.size() < LINUX_SLL_HEADER_SIZE) {
            return;
        }
        network = frame.subspan(LINUX_SLL_HEADER_SIZE);
    } else if (link_type != LINKTYPE_RAW) {
        return;  // Unsupported link layer.
    }

    if (network.empty()) {
        return;
    }

    // Determine IP version and locate the UDP header.
    const auto   version = static_cast<std::uint8_t>(network[0]) >> 4;
    std::uint8_t protocol {0};
    std::size_t  ip_header_len {0};
    if (version == 4) {
        ip_header_len = (static_cast<std::uint8_t>(network[0]) & 0x0F) * 4U;
        if (network.size() < ip_header_len || ip_header_len < 20) {
            return;
        }
        protocol = static_cast<std::uint8_t>(network[9]);
    } else if (version == 6) {
        if (network.size() < IPV6_HEADER_SIZE) {
            return;
        }
        protocol      = static_cast<std::uint8_t>(network[6]);
        ip_header_len = IPV6_HEADER_SIZE;
    } else {
        return;
    }
    if (protocol != IP_PROTOCOL_UDP) {
        return;
    }

    const std::span<const std::byte> udp = network.subspan(ip_header_len);
    if (udp.size() < UDP_HEADER_SIZE) {
        return;
    }
    const auto dst_port = read_net<std::uint16_t>(udp, 2);
    if (m_port_filter.has_value() && dst_port != *m_port_filter) {
        return;
    }
    auto        udp_length  = read_net<std::uint16_t>(udp, 4);
    std::size_t payload_len = udp.size() - UDP_HEADER_SIZE;
    if (udp_length >= UDP_HEADER_SIZE) {
        payload_len = std::min<std::size_t>(payload_len, udp_length - UDP_HEADER_SIZE);
    }

    ++m_udp_datagrams;
    m_mold.decode_packet(udp.subspan(UDP_HEADER_SIZE, payload_len));
}

}  // namespace itch::transport
