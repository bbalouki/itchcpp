# High-Performance NASDAQ ITCH 5.0 Parser

[![Build](https://github.com/bbalouki/itchcpp/actions/workflows/build.yml/badge.svg)](https://github.com/bbalouki/itchcpp/actions/workflows/build.yml)
![Linux](https://img.shields.io/badge/Linux-FCC624?logo=linux&logoColor=black)
![macOS](https://img.shields.io/badge/macOS-000000?logo=apple&logoColor=white)
![Windows](https://img.shields.io/badge/Windows-0078D6?logo=windows&logoColor=white)
![Vcpkg Version](https://img.shields.io/vcpkg/v/bbalouki-itch)
[![C++20](https://img.shields.io/badge/C++-20-blue.svg)](https://isocpp.org/std/the-standard)
[![CMake](https://img.shields.io/badge/CMake-3.25+-blue.svg)](https://cmake.org/)
[![CodeFactor](https://www.codefactor.io/repository/github/bbalouki/itchcpp/badge)](https://www.codefactor.io/repository/github/bbalouki/itchcpp)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![LinkedIn](https://img.shields.io/badge/LinkedIn-grey?logo=Linkedin&logoColor=white)](https://www.linkedin.com/in/bertin-balouki-simyeli-15b17a1a6/)
[![PayPal Me](https://img.shields.io/badge/PayPal%20Me-blue?logo=paypal)](https://paypal.me/bertinbalouki?country.x=SN&locale.x=en_US)

A modern, high-performance C++20 library for parsing NASDAQ TotalView-ITCH 5.0 protocol data feeds. This parser is designed for maximum speed, minimal memory overhead, and type safety, making it ideal for latency-sensitive financial applications, market data analysis, and quantitative research.

---

## Table of Contents

1.  [Project Philosophy](#project-philosophy)
2.  [Key Features](#key-features)
3.  [Architectural Overview](#architectural-overview)
    - [Fast, Allocation-Free Single-Pass Decoding](#fast-allocation-free-single-pass-decoding)
    - [Type-Safe Message Handling](#type-safe-message-handling)
    - [Endianness and Byte Order](#endianness-and-byte-order)
4.  [Getting Started](#getting-started)
    - [Prerequisites](#prerequisites)
    - [Building the Project](#building-the-project)
    - [Running Tests and Benchmarks](#running-tests-and-benchmarks)
5.  [Dependency Management](#dependency-management)
6.  [Comprehensive Usage Guide](#comprehensive-usage-guide)
    - [Example 1: Parsing a File into a Vector](#example-1-parsing-a-file-into-a-vector)
    - [Example 2: High-Performance Callback-Based Parsing](#example-2-high-performance-callback-based-parsing)
    - [Example 3: Filtering Messages by Type](#example-3-filtering-messages-by-type)
    - [Example 4: A Complete, Compilable Example](#example-4-a-complete-compilable-example)
7.  [API Reference: ITCH 5.0 Message Types](#api-reference-itch-50-message-types)
    - [System Event Messages](#system-event-messages)
    - [Stock and Administrative Messages](#stock-and-administrative-messages)
    - [Order Messages](#order-messages)
    - [Trade Messages](#trade-messages)
    - [Imbalance and Price Discovery Messages](#imbalance-and-price-discovery-messages)
8.  [Performance](#performance)
9.  [Coding Standards](#coding-standards)
10. [Frequently Asked Questions (FAQ)](#frequently-asked-questions-faq)
11. [Contributing](#contributing)
12. [License](#license)
13. [References](#references)

---

## Project Philosophy

The design of this ITCH parser is guided by three principles:

1.  **Performance Above All**: In market data processing, performance is not just a feature; it's a requirement. This library is architected to eliminate unnecessary overhead. It avoids dynamic memory allocations and stream I/O in its critical path, decoding each frame in a single forward pass. The goal is to convert raw binary data into a usable C++ struct as efficiently as possible.

2.  **Correctness and Safety**: Financial data is unforgiving. A single misinterpreted byte can corrupt analysis. We prioritize correctness by adhering strictly to the ITCH 5.0 specification and ensuring data is represented in a type-safe manner.

3.  **Modern and Maintainable C++**: The library leverages C++20 and above to provide a clean, maintainable, and efficient implementation. We use modern features to create a robust and developer-friendly API that is both powerful and easy to use correctly.

---

## Key Features

- **Real Feed Ingestion**: Consume ITCH as it actually arrives — MoldUDP64 UDP
  multicast framing, SoupBinTCP (Glimpse/recovery) framing, and `.pcap`/`.pcapng`
  captures — with per-session sequence tracking and gap detection. No libpcap
  dependency. See [Feed Ingestion](#feed-ingestion-transport).
- **Full-Market Book Engine**: Reconstruct every symbol on the feed in one pass
  with an allocation-light L3 order book (object pool, intrusive FIFO levels, flat
  ladders, open-addressed O(1) order lookup), with BBO change events, L2/L3 depth
  snapshots, and trade-tape extraction. See [Book Engine](#full-market-book-engine).
- **Zero-Copy Overlay API**: Inspect raw frames through lazy typed views that
  convert only the fields you read, for hot paths that touch a few fields per
  message.
- **Built-in Analytics**: Header-only microstructure layer — OHLCV bar builders
  (time/tick/volume clocks), VWAP/TWAP, spread, queue imbalance, order-flow
  imbalance, NOII surfacing, and auction reconstruction. See [Analytics](#analytics).
- **Interoperability**: CSV and Arrow/Parquet export, a batteries-included
  `itch-tool` CLI (inspect/filter/stats/convert), and native Python bindings
  (pybind11). See [Interoperability](#interoperability).
- **High Throughput**: Multi-gigabyte-per-second parsing on modern hardware (see [Benchmarks](#benchmarks) for measured numbers).
- **Allocation-Free Core**: The callback-based parsing loop performs zero dynamic memory allocations, minimizing latency and jitter.
- **Type-Safe API**: All ITCH messages are deserialized into a `std::variant` of dedicated, packed `struct`s, ensuring compile-time safety.
- **Specification Compliant**: Faithfully implements the message formats outlined in the Nasdaq TotalView-ITCH 5.0 specification.
- **Flexible Parsing Strategies**:
  - Eagerly parse a buffer into a `std::vector` for convenience.
  - Use a callback for efficient, low-memory stream processing of large files.
  - Filter messages by type at the parsing stage to reduce processing load.
- **Cross-Platform**: Compatible with any platform supporting a C++20 compiler and CMake.
- **Minimal Dependencies**: Zero runtime dependencies. Test and benchmark libraries are managed by `vcpkg`.

---

## Architectural Overview

The library is built on some key principles to deliver maximum performance and safety.

### Fast, Allocation-Free Single-Pass Decoding

The parser is a fast, allocation-free, single-pass field decoder. Each message type is defined as a packed `struct` whose layout mirrors the ITCH wire format, and each frame is decoded in one forward pass: fields are copied out and converted from network byte order directly into the typed `struct`, with no intermediate buffers and no per-message heap allocation on the callback path.

Dispatch is driven by a compile-time table keyed on the one-byte message type, so selecting the right decoder is a single flat-array lookup followed by a direct, inlinable call. There is no red-black-tree lookup and no type-erased `std::function` indirection. The frame length declared on the wire is validated against the size the message type requires before any field is read, so a truncated or malformed frame can never cause the decoder to read into an adjacent message.

This is genuine single-pass decoding rather than a zero-copy overlay: the bytes are converted eagerly into host-order fields. A lazy zero-copy view API (typed overlays over the raw buffer with on-access endianness conversion) is tracked as future work in [FEATURES.md](FEATURES.md).

### Type-Safe Message Handling

All possible ITCH messages are handled using a single, modern C++ object: `itch::Message`, which is a `std::variant`. This approach provides significant advantages over traditional designs:

- **Compile-Time Safety:** The compiler verifies that your code handles every possible message type. This prevents unexpected errors at runtime if a message type is overlooked.
- **High Performance:** This design allows the compiler to generate highly optimized code for processing different messages, which is typically faster than conventional virtual function lookups.
- **Efficient Memory Layout:** Messages are stored contiguously in memory, which improves CPU cache utilization and overall processing speed.

### Endianness and Byte Order

The ITCH protocol specifies that all multi-byte numbers use a **big-endian** (network) byte order. Most modern CPUs, however, use a **little-endian** format.

The parser handles this discrepancy automatically. After a message is read into a struct, the library transparently converts all multi-byte fields (such as prices and quantities) from the network order to the native order of the machine. This ensures all numerical data is immediately correct and ready for use without requiring any manual conversion.

---

## Getting Started

### Prerequisites

- **C++ Compiler**: A compiler with full C++20 support (e.g., GCC 10+, Clang 12+, MSVC 19.29+).
- **CMake**: Version 3.25 or newer.

### Building the Project

1.  **Clone the Repository**

    ```bash
    git clone https://github.com/bbalouki/itchcpp.git
    cd itchcpp
    ```

2.  **Configure with CMake**
    This step generates the build system and automatically downloads any developer dependencies if needed.

    ```bash
    cmake -S . -B build
    ```

3.  **Compile the Code**
    Build the library and any example targets.
    ```bash
    cmake --build build --config Release
    ```

### Running Tests and Benchmarks

The project includes unit tests and performance benchmarks that can be built by enabling the corresponding CMake options.

- **To build and run tests:**

  ```bash
  # Configure with tests enabled
  cmake -S . -B build -DITCH_BUILD_TESTS=ON

  # Build
  cmake --build build --config Release

  # Run tests from the build directory
  ctest --test-dir build --output-on-failure
  ```

- **To build and run benchmarks:**

  ```bash
  # Configure with benchmarks enabled
  cmake -S . -B build -DITCH_BUILD_BENCHMARKS=ON

  # Build
  cmake --build build --config Release

  # Run benchmarks
  ./build/benchmarks/parser_bench
  ```

---

## Dependency Management

This library has **zero external runtime dependencies**.

For development purposes, it uses two popular libraries:

- **Google Test**: For unit testing.
- **Google Benchmark**: For performance microbenchmarks.

These are handled automatically by Microsoft's `vcpkg` package manager. When you enable tests or benchmarks, **You will need to install them on your system separately or install them in manifest mode.**

---

## Comprehensive Usage Guide

Download some sample data [here](https://emi.nasdaq.com/ITCH/Nasdaq%20ITCH/)

### Example 1: Parsing a File into a Vector

This is the simplest approach. It reads an entire file into memory and returns a `std::vector` of parsed messages. This method is convenient for smaller files but may consume significant memory for large datasets.

```cpp
#include "itch/parser.hpp"
#include <fstream>
#include <iostream>
#include <vector>

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <path_to_itch_file>\n";
        return 1;
    }

    std::ifstream file(argv[1], std::ios::binary);
    if (!file) {
        std::cerr << "Error: Cannot open file " << argv[1] << "\n";
        return 1;
    }

    itch::Parser parser;
    try {
        // Parse the entire file stream into a vector of messages.
        std::vector<itch::Message> messages = parser.parse(file);
        std::cout << "Successfully parsed " << messages.size() << " messages.\n";

        // You can now iterate over the messages for analysis.
        for (const auto& msg : messages) {
            // ... process each message ...
        }

    } catch (const std::runtime_error& e) {
        std::cerr << "Parsing failed: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
```

### Example 2: High-Performance Callback-Based Parsing

This is the most memory-efficient way to process large files. The parser invokes your callback function for each message it finds, avoiding the need to store all messages in memory at once. For best performance, read the file into a buffer first.

```cpp
#include "itch/parser.hpp"
#include <iostream>
#include <vector>
#include <fstream>

// A simple callback function that counts messages
void message_counter(const itch::Message& message) {
    static long count = 0;
    count++;
    if (count % 1000000 == 0) {
        std::cout << "Processed " << count << " messages...\n";
    }
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <path_to_itch_file>\n";
        return 1;
    }

    std::ifstream file(argv[1], std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Error opening file!\n";
        return 1;
    }

    // Reading the entire file into a buffer minimizes I/O overhead during parsing.
    std::vector<char> buffer(std::istreambuf_iterator<char>(file), {});

    itch::Parser parser;
    try {
        // Parse the buffer and invoke `message_counter` for each message.
        parser.parse(buffer.data(), buffer.size(), &message_counter);
        std::cout << "Finished processing file.\n";
    } catch (const std::runtime_error& e) {
        std::cerr << "Parsing failed: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
```

### Example 3: Filtering Messages by Type

You can instruct the parser to only deserialize specific message types. This provides a significant performance boost if you are only interested in a subset of the data (e.g., just trades and quotes).

```cpp
#include "itch/parser.hpp"
#include <fstream>
#include <iostream>
#include <vector>

int main(int argc, char* argv[]) {
    if (argc != 2) { /*...*/ return 1; }

    std::ifstream file(argv[1], std::ios::binary);
    if (!file) { /*...*/ return 1; }

    itch::Parser parser;
    try {
        // We only care about Add Order, Order Executed, and Broken Trade messages.
        // The parser will skip all other message types.
        std::vector<char> filter = {'A', 'F', 'E', 'C', 'B'};
        std::vector<itch::Message> trade_events = parser.parse(file, filter);

        std::cout << "Found " << trade_events.size() << " relevant order and trade messages.\n";

    } catch (const std::runtime_error& e) {
        std::cerr << "Parsing failed: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
```

### Example 4: A Complete, Compilable Example

This standalone example demonstrates how to use `std::visit` with a visitor to process different message types and print their details correctly.

```cpp
#include "itch/parser.hpp"
#include <iostream>
#include <vector>
#include <fstream>
#include <iomanip> // For std::setprecision

// A visitor object to handle different message types.
// Using `if constexpr` ensures only valid code is compiled for each message type.
auto message_visitor = [](const auto& msg) {
    using T = std::decay_t<decltype(msg)>;

    // Set fixed-point notation for prices
    std::cout << std::fixed << std::setprecision(4);

    if constexpr (std::is_same_v<T, itch::AddOrderMessage>) {
        std::cout << "ADD ORDER: Ref #" << msg.order_reference_number
                  << ", " << msg.shares << " shares of "
                  << itch::to_string(msg.stock, sizeof(msg.stock)) // Helper to convert char array
                  << " @ $" << static_cast<double>(msg.price) / itch::PRICE_DIVISOR << "\n";
    } else if constexpr (std::is_same_v<T, itch::OrderExecutedMessage>) {
        std::cout << "EXECUTION: Ref #" << msg.order_reference_number
                  << ", executed " << msg.executed_shares << " shares.\n";
    } else if constexpr (std::is_same_v<T, itch::SystemEventMessage>) {
        std::cout << "SYSTEM EVENT: Code '" << msg.event_code
                  << "' (O=Start, S=SysHours, Q=MktHours, M=EndMkt, E=EndSys, C=End)\n";
    } else {
        // This block will be visited for any other message types.
    }
};

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <path_to_itch_file>\n";
        return 1;
    }

    std::ifstream file(argv[1], std::ios::binary);
    if (!file) {
        std::cerr << "Error: Cannot open file " << argv[1] << "\n";
        return 1;
    }

    itch::Parser parser;
    try {
        // Use a lambda to pass each message to our visitor
        parser.parse(file, [](const itch::Message& msg) {
            std::visit(message_visitor, msg);
        });
    } catch (const std::runtime_error& e) {
        std::cerr << "Parsing failed: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
```

### Example 5: Building and Displaying an Order Book

Beyond simple parsing, this library provides a high-level `LimitOrderBook` utility that reconstructs the full order book state for a given financial instrument from an ITCH data stream. This is essential for applications requiring market depth analysis, such as algorithmic trading or market making.

The `itch::LimitOrderBook` class processes ITCH messages and internally manages the addition, removal, and modification of orders, giving you a complete, real-time view of the market. This example demonstrates how to parse an ITCH file and use the messages to build an order book for a specific stock (e.g., "AAPL"). After processing all messages, it prints a snapshot of the final book state.

```cpp
#include <fstream>
#include <iostream>
#include "itch/parser.hpp"
#include "itch/order_book.hpp"

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <itch_file> <symbol>\n";
        return 1;
    }

    std::ifstream file(argv[1], std::ios::binary);
    if (!file) {
        std::cerr << "Error: Could not open file " << argv[1] << "\n";
        return 1;
    }

    // Initialize the order book for the target stock symbol
    itch::LimitOrderBook order_book(argv[2]);

    // Create a parser and configure it to only process book-related messages
    // This is a performance optimization.
    itch::Parser parser;
    std::vector<char> book_messages(order_book.book_messages.begin(),
                                    order_book.book_messages.end());

    try {
        // Define a callback to process each relevant message
        auto callback = [&](const itch::Message& msg) {
            order_book.process(msg);
        };

        // Start parsing the file with the filter and callback
        parser.parse(file, callback, book_messages);

        // Print the final state of the order book
        std::cout << "Final Order Book for " << argv[2] << ":\n";
        order_book.print(std::cout);

    } catch (const std::exception& e) {
        std::cerr << "An error occurred: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
```

#### Example Output

The `print()` method provides a formatted snapshot of the top of the book. It displays the total volume at each price level for both bids and asks.

```
+-----------------------------------------------------+
|                         AAPL                        |
+--------------------------+--------------------------+
|        ASKS (SELL)       |        BIDS (BUY)        |
+------------+-------------+------------+-------------+
|   PRICE    |    VOLUME   |   PRICE    |    VOLUME   |
+------------+-------------+------------+-------------+
|   173.5000 |      15,000 |   173.4900 |      23,500 |
|   173.5100 |      12,300 |   173.4800 |      18,900 |
|   173.5200 |       8,000 |   173.4700 |      31,200 |
|   173.5300 |      21,100 |   173.4600 |       5,600 |
|   173.5400 |       4,500 |   173.4500 |      14,400 |
+------------+-------------+------------+-------------+
```

### Feed Ingestion (Transport)

The raw parser assumes a concatenation of length-prefixed messages, but ITCH is
delivered inside transport framing. The `itch::transport` module decodes the
framing that actually arrives on the wire and on disk, then feeds the existing
parser. Everything is implemented in-house with **no libpcap dependency**.

| Decoder | Header | Purpose |
| ------- | ------ | ------- |
| `MoldUdp64Decoder` | `itch/transport/moldudp64.hpp` | UDP multicast framing for live dissemination. |
| `SoupBinDecoder` | `itch/transport/soupbintcp.hpp` | TCP framing for Glimpse snapshots and recovery/replay. |
| `PcapReader` | `itch/transport/pcap.hpp` | Replay a feed from a `.pcap`/`.pcapng` capture file. |
| `SequenceTracker` | `itch/transport/sequencing.hpp` | Per-session sequence tracking, gap detection, recovery hooks. |

```cpp
#include "itch/transport/pcap.hpp"

itch::transport::PcapReader reader{[](const itch::Message& msg) {
    // ... handle each decoded ITCH message ...
}};

// Surface any sequence gaps in the multicast stream.
reader.mold_decoder().tracker().set_gap_callback(
    [](std::string_view session, std::uint64_t expected, std::uint64_t got) {
        // ... drive recovery / log the gap ...
    });

reader.read_file("capture.pcapng");      // walks Ethernet/IP/UDP/MoldUDP64
// reader.messages_decoded(), reader.udp_datagrams(), tracker().gap_count()
```

For a live or captured MoldUDP64 datagram you already have in memory, call
`MoldUdp64Decoder::decode_packet(span)` directly; for a SoupBinTCP byte stream,
push segments through `SoupBinDecoder::feed(span)` as they arrive.

### Full-Market Book Engine

The original `LimitOrderBook` reconstructs a single, pre-selected symbol. The
Phase 2 `itch::book::BookManager` reconstructs **every** symbol on the feed in one
pass, routing each message to that security's book by stock locate code in O(1).
Each book (`itch::book::L3Book`) is allocation-light: orders live in a reusable
object pool linked into intrusive FIFO queues, price levels are kept in flat
sorted ladders, and order lookup by reference number uses a flat open-addressed
map, so there is no per-order heap allocation or atomic refcount on the hot path.

```cpp
#include "itch/book/book_manager.hpp"
#include "itch/parser.hpp"

itch::book::BookManager manager;
// manager.track_symbol("AAPL");      // optional: restrict to a universe

manager.set_bbo_callback([](const itch::book::L3Book& book, const itch::book::Bbo& bbo) {
    // best bid/offer for book.symbol() just changed
});
manager.set_trade_callback([](const itch::Trade& trade) {
    // trade.printable distinguishes displayable prints from hidden ones
});

itch::Parser parser;
parser.parse(buffer.data(), buffer.size(), [&](const itch::Message& msg) {
    manager.process(msg);
});

const itch::book::L3Book* book = manager.book_for_symbol("AAPL");
auto top  = book->bbo();                          // best bid/offer
auto l2   = book->depth(itch::book::Side::buy, 5); // top-5 aggregated bids
auto l3   = book->orders_at(itch::book::Side::buy, top.bid_price.raw()); // order-level
```

For callers that touch only a few fields per message, `itch/overlay.hpp` provides
a zero-copy alternative to the eager parser: `for_each_message(buffer, cb)` yields
a `MessageView` (and typed views like `AddOrderView`) that decode each field
lazily on access.

### Analytics

The header-only `itch::analytics` layer computes the metrics quants ask for,
directly off the trade tape and book so every downstream team does not
reimplement them.

| Component | Header | Provides |
| --------- | ------ | -------- |
| `BarBuilder<Clock>` | `analytics/bars.hpp` | OHLCV bars over `TimeClock`, `TickClock`, `VolumeClock`. |
| `Vwap`, `Twap` | `analytics/vwap.hpp` | Running/interval volume- and time-weighted average price. |
| spread / mid / imbalance | `analytics/microstructure.hpp` | Spread, mid, depth-at-level, queue imbalance, order-flow imbalance. |
| `ImbalanceInfo` | `analytics/imbalance.hpp` | Decoded NOII (`I`) imbalance data. |
| `AuctionTracker` | `analytics/auctions.hpp` | Opening/closing/halt/IPO cross reconstruction. |

```cpp
#include "itch/analytics/vwap.hpp"
#include "itch/analytics/bars.hpp"

itch::analytics::Vwap vwap;
itch::analytics::BarBuilder bars{itch::analytics::TimeClock{60'000'000'000ULL}, // 1-minute bars
                                 [](const itch::analytics::Bar& bar) { /* ... */ }};

manager.set_trade_callback([&](const itch::Trade& trade) {
    vwap.add(trade.price, trade.shares);
    bars.add(trade);
});
// ... parse the feed ...
bars.flush();
double session_vwap = vwap.value();
```

### Interoperability

Meet researchers in the tools they already use.

**Output sinks.** `itch/io/csv_sink.hpp` flattens any message stream into a wide
CSV table (dependency-free). With `-DITCH_WITH_ARROW=ON` (the `arrow` vcpkg
feature), `itch/io/arrow_export.hpp` writes Parquet for pandas/Polars/DuckDB/Spark.

**`itch-tool` CLI** (`-DITCH_BUILD_TOOLS=ON`). Inspect, filter, and convert feeds
without writing code; input may be a raw ITCH stream or a `.pcap`/`.pcapng`
capture (auto-detected):

```bash
itch-tool stats   data.itch                       # per-type message histogram
itch-tool inspect data.pcapng --limit 50          # human-readable dump
itch-tool filter  data.itch --types AEP --out trades.csv
itch-tool convert data.itch --out data.csv        # ITCH -> CSV (-> Parquet w/ Arrow)
```

**Python bindings** (`-DITCH_BUILD_PYTHON=ON`, pybind11). A native module that
mirrors the pure-Python [`itch`](https://github.com/bbalouki/itch) package's API
so it can act as a faster drop-in backend. See [python/README.md](python/README.md).

```python
import itchcpp
parser = itchcpp.MessageParser()
for message in parser.parse_file("01302020.NASDAQ_ITCH50"):
    if isinstance(message, itchcpp.AddOrderMessage):
        print(message.stock, message.decode_price("price"), message.shares)
```

---

## API Reference: ITCH 5.0 Message Types

This library supports all message types defined in the ITCH 5.0 specification. The `char` represents the message type on the wire.

### System Event Messages

| Type | Struct Name          | Description                                  |
| :--: | -------------------- | -------------------------------------------- |
| `S`  | `SystemEventMessage` | Signals a market or data feed handler event. |

### Stock and Administrative Messages

| Type | Struct Name                        | Description                                                                  |
| :--: | ---------------------------------- | ---------------------------------------------------------------------------- |
| `R`  | `StockDirectoryMessage`            | Conveys stock symbol directory information.                                  |
| `H`  | `StockTradingActionMessage`        | Indicates a change in trading status for a security (e.g., Halted, Trading). |
| `Y`  | `RegSHORestrictionMessage`         | Regulation SHO short sale price test restricted indicator.                   |
| `L`  | `MarketParticipantPositionMessage` | Conveys a market participant's status (e.g., Primary Market Maker).          |
| `V`  | `MWCBDeclineLevelMessage`          | Informs of the Market-Wide Circuit Breaker (MWCB) breach points for the day. |
| `W`  | `MWCBStatusMessage`                | Indicates that a MWCB level has been breached.                               |
| `h`  | `OperationalHaltMessage`           | Indicates an operational halt for a security on a specific market center.    |

### Order Messages

| Type | Struct Name                     | Description                                                                   |
| :--: | ------------------------------- | ----------------------------------------------------------------------------- |
| `A`  | `AddOrderMessage`               | A new order has been accepted (no MPID attribution).                          |
| `F`  | `AddOrderMPIDMessage`           | A new order has been accepted (with MPID attribution).                        |
| `E`  | `OrderExecutedMessage`          | An order on the book was executed in part or in full.                         |
| `C`  | `OrderExecutedWithPriceMessage` | An order was executed at a price different from the display price.            |
| `X`  | `OrderCancelMessage`            | An order was partially canceled; this message contains the canceled quantity. |
| `D`  | `OrderDeleteMessage`            | An order was canceled in its entirety and removed from the book.              |
| `U`  | `OrderReplaceMessage`           | An existing order has been replaced with a new order.                         |

### Trade Messages

| Type | Struct Name          | Description                                                       |
| :--: | -------------------- | ----------------------------------------------------------------- |
| `P`  | `TradeMessage`       | Reports a trade for a non-displayable order type.                 |
| `Q`  | `CrossTradeMessage`  | Reports a cross trade (e.g., Opening, Closing, IPO Cross).        |
| `B`  | `BrokenTradeMessage` | Reports that a previously disseminated execution has been broken. |

### Imbalance and Price Discovery Messages

| Type | Struct Name                               | Description                                                                 |
| :--: | ----------------------------------------- | --------------------------------------------------------------------------- |
| `K`  | `IPOQuotingPeriodUpdateMessage`           | Provides the anticipated IPO quotation release time.                        |
| `J`  | `LULDAuctionCollarMessage`                | Indicates auction collar thresholds for a security in a LULD Trading Pause. |
| `I`  | `NOIIMessage`                             | Net Order Imbalance Indicator message, used for opening/closing crosses.    |
| `N`  | `RetailPriceImprovementIndicator`         | Indicates the presence of Retail Price Improvement (RPII) interest.         |
| `O`  | `DirectListingCapitalRaisePriceDiscovery` | Disseminated for Direct Listing with Capital Raise (DLCR) securities.       |

---

## Performance

The parser is designed for high-throughput scenarios. Performance is heavily dependent on the underlying hardware (CPU speed, memory bandwidth, and I/O speed).

### Benchmarks

The numbers below were produced by [`benchmarks/parser_bench.cpp`](benchmarks/parser_bench.cpp) parsing from an in-memory buffer (file I/O excluded). They are reproducible with the setup described under [How to reproduce](#how-to-reproduce).

| Configuration | Throughput |
| ------------- | ---------- |
| `parse(..., callback)` — allocation-free callback path | **2.24 GiB/s** |
| `parse(...)` — collect all messages into a `std::vector` | 1.23 GiB/s |
| `parse(..., {'A','P','E','C','X'})` — filtered into a `std::vector` | 1.15 GiB/s |

- **Hardware**: x86-64, 16 logical cores @ 1.70 GHz (L1d 32 KiB, L2 512 KiB, L3 4 MiB).
- **Compiler**: Clang 22, `-DCMAKE_BUILD_TYPE=Release`, C++23.
- **Dataset**: official NASDAQ TotalView-ITCH 5.0 sample (`01302020`), a ~300 MB frame-aligned slice loaded fully into memory.

The collect/filter paths are slower than the callback path because they allocate and copy each parsed message into a `std::vector`; the callback path does neither, which is why it is the recommended interface for latency-sensitive processing.

#### Book engine and overlay

These are produced by [`benchmarks/book_bench.cpp`](benchmarks/book_bench.cpp).
The throughput figures below were measured on a synthetic, churn-heavy stream
(adds and deletes around a moving mid across 100 symbols) on the same hardware,
Clang 22 `-DCMAKE_BUILD_TYPE=Release`, C++23; book-rebuild throughput on real
data depends heavily on the depth and churn profile of the feed.

| Configuration | Throughput |
| ------------- | ---------- |
| `BookManager::process` — full multi-symbol L3 reconstruction | ~150 MiB/s |
| Eager `parse`, touching one field per message | ~5.4 GiB/s |
| Zero-copy `overlay::for_each_message`, touching one field per message | **~9.7 GiB/s** |

The overlay is materially faster than the eager decoder when only a few fields are
read, because it never decodes the fields the caller does not touch. Reproduce
with `./build/benchmarks/book_bench <itch_file>`.

### How to reproduce

```bash
# 1. Fetch an official sample (multi-GB; not committed to the repo).
scripts/fetch_sample.sh data

# 2. Configure and build with benchmarks enabled.
cmake -S . -B build -DITCH_BUILD_BENCHMARKS=ON -DITCH_CXX_STANDARD=23 -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release

# 3. Run the benchmark suite against the sample.
./build/benchmarks/parser_bench data/01302020.NASDAQ_ITCH50
```

The output reports throughput in GiB/s for each parsing strategy.

---

## Coding Standards

The project uses `.clang-format` to enforce a consistent code style. A `.clang-tidy` configuration is also provided to catch common programming errors and enforce best practices. Developers contributing to the project are expected to format their code using these tools.

---

## Frequently Asked Questions (FAQ)

- **What exactly is this NASDAQ ITCH data?**
  It's the most detailed data feed available from NASDAQ. It's not just stock prices; it’s every single order placed, modified, cancelled, and executed. It's the complete "play-by-play" of the market.

- **Why can't I just use a simple program to read this data?**
  The data is in a highly optimized, machine-only binary format, not human-readable text. More importantly, the volume and velocity are immense—a single day of trading can generate tens or hundreds of gigabytes of data. A standard program would be far too slow to keep up.

- **Is this a program I can just double-click and run?**
  No, it’s a specialized component—a library—that software developers use as the "engine" inside a larger application (like a trading platform or an analysis tool). It does the heavy lifting of data processing so they can focus on building the features their users need.

---

## Contributing

Contributions are welcome! Whether it's a bug report, a feature request, or a pull request, your input is valuable. Please feel free to open an issue or submit a PR.

## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.

## References

- **Nasdaq TotalView-ITCH 5.0 Specification:** The official [documentation](https://www.nasdaqtrader.com/content/technicalsupport/specifications/dataproducts/NQTVITCHspecification.pdf) is the definitive source for protocol details.
