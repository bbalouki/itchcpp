#pragma once

/// @file
/// @brief Serializes parsed ITCH messages back to valid wire bytes.
///
/// The encoder is the inverse of the parser: it writes each message's fields in
/// the spec's order and network (big-endian) byte order, including the 48-bit
/// timestamp. It is used to synthesize valid ITCH streams for testing, scenario
/// generation, and golden fixtures, and it guarantees the round-trip property
/// `parse(encode(msg)) == msg` for every supported message type.
///
/// @author Bertin Balouki SIMYELI

#include <cstddef>
#include <vector>

#include "itch/messages.hpp"

namespace itch {

/// @brief Encodes a message body and header without the 2-byte length prefix.
///
/// @param message The parsed message to encode.
/// @return The raw message bytes (type byte first), exactly as they appear inside
///         a frame.
[[nodiscard]] auto encode_message(const Message& message) -> std::vector<std::byte>;

/// @brief Encodes a complete length-prefixed frame (2-byte big-endian length
///        followed by the message bytes), ready to concatenate into a stream.
///
/// @param message The parsed message to encode.
/// @return The length-prefixed frame bytes, ready to append to a stream.
[[nodiscard]] auto encode_frame(const Message& message) -> std::vector<std::byte>;

}  // namespace itch
