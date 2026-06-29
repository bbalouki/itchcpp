#pragma once

#include "itch/messages.hpp"

namespace itch::io {

/// @brief A generic streaming sink for parsed ITCH messages.
///
/// Sinks are the universal output side of the library: a parse callback can hand
/// every message to a sink, which writes it somewhere (CSV, Arrow, a socket, a
/// counter). Implementations override `write`; `flush` is optional.
class MessageSink {
   public:
    MessageSink()                                          = default;
    MessageSink(const MessageSink&)                        = default;
    MessageSink(MessageSink&&) noexcept                    = default;
    auto operator=(const MessageSink&) -> MessageSink&     = default;
    auto operator=(MessageSink&&) noexcept -> MessageSink& = default;
    virtual ~MessageSink()                                 = default;

    /// @brief Consumes one parsed message.
    virtual auto write(const Message& message) -> void = 0;

    /// @brief Flushes any buffered output. The default is a no-op.
    virtual auto flush() -> void {}
};

}  // namespace itch::io
