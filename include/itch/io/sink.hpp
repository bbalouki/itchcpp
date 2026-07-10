#pragma once

/// @file
/// @brief Abstract streaming sink interface for parsed ITCH messages.
///
/// `MessageSink` is the common output interface implemented by `CsvSink`,
/// `ArrowExporter`, and other consumers that receive one message at a time from
/// a parse callback.
///
/// @author Bertin Balouki SIMYELI

#include "itch/messages.hpp"

namespace itch::io {

/// @brief A generic streaming sink for parsed ITCH messages.
///
/// Sinks are the universal output side of the library: a parse callback can hand
/// every message to a sink, which writes it somewhere (CSV, Arrow, a socket, a
/// counter). Implementations override `write`; `flush` is optional.
class MessageSink {
   public:
    /// @brief Default-constructs an empty sink.
    MessageSink() = default;

    /// @brief Copy-constructs a sink.
    MessageSink(const MessageSink&) = default;

    /// @brief Move-constructs a sink.
    MessageSink(MessageSink&&) noexcept = default;

    /// @brief Copy-assigns a sink.
    ///
    /// @return Reference to this sink.
    auto operator=(const MessageSink&) -> MessageSink& = default;

    /// @brief Move-assigns a sink.
    ///
    /// @return Reference to this sink.
    auto operator=(MessageSink&&) noexcept -> MessageSink& = default;

    /// @brief Virtual destructor for safe polymorphic destruction.
    virtual ~MessageSink() = default;

    /// @brief Consumes one parsed message.
    ///
    /// @param message The parsed message to write.
    virtual auto write(const Message& message) -> void = 0;

    /// @brief Flushes any buffered output. The default is a no-op.
    virtual auto flush() -> void {}
};

}  // namespace itch::io
