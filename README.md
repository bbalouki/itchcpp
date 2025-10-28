
# High-Performance NASDAQ ITCH 5.0 Parser

[![build](https://github.com/bbalouki/itchcpp/actions/workflows/itch-ci.yml/badge.svg)](https://github.com/bbalouki/itchcpp/actions/workflows/itch-ci.yml)
[![C++20](https://img.shields.io/badge/C++-20-blue.svg)](https://isocpp.org/std/the-standard)
[![CMake](https://img.shields.io/badge/CMake-3.20+-blue.svg)](https://cmake.org/)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![LinkedIn](https://img.shields.io/badge/LinkedIn-grey?logo=Linkedin&logoColor=white)](https://www.linkedin.com/in/bertin-balouki-simyeli-15b17a1a6/)
[![PayPal Me](https://img.shields.io/badge/PayPal%20Me-blue?logo=paypal)](https://paypal.me/bertinbalouki?country.x=SN&locale.x=en_US)

A modern, high-performance C++20 library for parsing NASDAQ TotalView-ITCH 5.0 protocol data feeds. This parser is designed for maximum speed, minimal memory overhead, and type safety, making it ideal for latency-sensitive financial applications, market data analysis, and quantitative research.

---

## Table of Contents

1.  [Project Philosophy](#project-philosophy)
2.  [Key Features](#key-features)
3.  [Architectural Overview](#architectural-overview)
    *   [Zero-Overhead Deserialization](#zero-overhead-deserialization)
    *   [Type-Safe Message Handling](#type-safe-message-handling)
    *   [Endianness and Byte Order](#endianness-and-byte-order)
4.  [Getting Started](#getting-started)
    *   [Prerequisites](#prerequisites)
    *   [Building the Project](#building-the-project)
    *   [Running Tests and Benchmarks](#running-tests-and-benchmarks)
5.  [Dependency Management](#dependency-management)
6.  [Comprehensive Usage Guide](#comprehensive-usage-guide)
    *   [Example 1: Parsing a File into a Vector](#example-1-parsing-a-file-into-a-vector)
    *   [Example 2: High-Performance Callback-Based Parsing](#example-2-high-performance-callback-based-parsing)
    *   [Example 3: Filtering Messages by Type](#example-3-filtering-messages-by-type)
    *   [Example 4: A Complete, Compilable Example](#example-4-a-complete-compilable-example)
7.  [API Reference: ITCH 5.0 Message Types](#api-reference-itch-50-message-types)
    *   [System Event Messages](#system-event-messages)
    *   [Stock and Administrative Messages](#stock-and-administrative-messages)
    *   [Order Messages](#order-messages)
    *   [Trade Messages](#trade-messages)
    *   [Imbalance and Price Discovery Messages](#imbalance-and-price-discovery-messages)
8.  [Performance](#performance)
9.  [Coding Standards](#coding-standards)
10. [Frequently Asked Questions (FAQ)](#frequently-asked-questions-faq)
11. [Contributing](#contributing)
12. [License](#license)

---

## Project Philosophy

The design of this ITCH parser is guided by three principles:

1.  **Performance Above All**: In market data processing, performance is not just a feature; it's a requirement. This library is architected to eliminate unnecessary overhead. It avoids dynamic memory allocations, data copies, and stream I/O in its critical path. The goal is to convert raw binary data into a usable C++ struct as efficiently as possible.

2.  **Correctness and Safety**: Financial data is unforgiving. A single misinterpreted byte can corrupt analysis. We prioritize correctness by adhering strictly to the ITCH 5.0 specification and ensuring data is represented in a type-safe manner.

3.  **Modern and Maintainable C++**: The library leverages C++20 and above to provide a clean, maintainable, and efficient implementation. We use modern features to create a robust and developer-friendly API that is both powerful and easy to use correctly.

---

## Key Features

*   **High Throughput**: Capable of multi-gigabyte-per-second parsing speeds on modern hardware.
*   **Zero-Allocation Core**: The primary parsing loop performs zero dynamic memory allocations, minimizing latency and jitter.
*   **Type-Safe API**: All ITCH messages are deserialized into a `std::variant` of dedicated, packed `struct`s, ensuring compile-time safety.
*   **Specification Compliant**: Faithfully implements the message formats outlined in the Nasdaq TotalView-ITCH 5.0 specification.
*   **Flexible Parsing Strategies**:
    *   Eagerly parse a buffer into a `std::vector` for convenience.
    *   Use a callback for efficient, low-memory stream processing of large files.
    *   Filter messages by type at the parsing stage to reduce processing load.
*   **Cross-Platform**: Compatible with any platform supporting a C++20 compiler and CMake.
*   **Minimal Dependencies**: Zero runtime dependencies. Test and benchmark libraries are managed by `vcpkg`.

---

## Architectural Overview

The library is built on some key principles to deliver maximum performance and safety.

### Zero-Overhead Deserialization

To achieve its performance, the parser uses a zero-overhead deserialization strategy. The parser achieves its speed by reading messages directly from the raw data stream without performing heavy data conversions. Each message type is defined in a compact format that matches exactly how the data appears on the network. This means the parser can quickly copy incoming bytes into a message structure in memory, skipping the usual per-field decoding steps. The result is near-zero overhead when reading and processing messages.

### Type-Safe Message Handling

All possible ITCH messages are handled using a single, modern C++ object: `itch::Message`, which is a `std::variant`. This approach provides significant advantages over traditional designs:

*   **Compile-Time Safety:** The compiler verifies that your code handles every possible message type. This prevents unexpected errors at runtime if a message type is overlooked.
*   **High Performance:** This design allows the compiler to generate highly optimized code for processing different messages, which is typically faster than conventional virtual function lookups.
*   **Efficient Memory Layout:** Messages are stored contiguously in memory, which improves CPU cache utilization and overall processing speed.

### Endianness and Byte Order

The ITCH protocol specifies that all multi-byte numbers use a **big-endian** (network) byte order. Most modern CPUs, however, use a **little-endian** format.

The parser handles this discrepancy automatically. After a message is read into a struct, the library transparently converts all multi-byte fields (such as prices and quantities) from the network order to the native order of the machine. This ensures all numerical data is immediately correct and ready for use without requiring any manual conversion.

---

## Getting Started

### Prerequisites

*   **C++ Compiler**: A compiler with full C++20 support (e.g., GCC 10+, Clang 12+, MSVC 19.29+).
*   **CMake**: Version 3.20 or newer.

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

*   **To build and run tests:**
    ```bash
    # Configure with tests enabled
    cmake -S . -B build -DBUILD_TESTS=ON

    # Build
    cmake --build build --config Release

    # Run tests from the build directory
    ctest --test-dir build --output-on-failure
    ```

*   **To build and run benchmarks:**
    ```bash
    # Configure with benchmarks enabled
    cmake -S . -B build -DBUILD_BENCHMARKS=ON

    # Build
    cmake --build build --config Release

    # Run benchmarks
    ./build/benchmarks/benchmark
    ```

---

## Dependency Management

This library has **zero external runtime dependencies**.

For development purposes, it uses two popular libraries:
*   **Google Test**: For unit testing.
*   **Google Benchmark**: For performance microbenchmarks.

These are handled automatically by Microsoft's `vcpkg` package manager. When you enable tests or benchmarks, **You will need to install them on your system separately or install them in manifest mode.**

---

## Comprehensive Usage Guide

### Example 1: Parsing a File into a Vector

This is the simplest approach. It reads an entire file into memory and returns a `std::vector` of parsed messages. This method is convenient for smaller files but may consume significant memory for large datasets.

```cpp
#include "itch/parser.hpp"
#include <fstream>
#include <iostream>
#include <vector>

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <path_to_itch_file.bin>\n";
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
        std::cerr << "Usage: " << argv[0] << " <path_to_itch_file.bin>\n";
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
        std::cerr << "Usage: " << argv[0] << " <path_to_itch_file.bin>\n";
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

---

## API Reference: ITCH 5.0 Message Types

This library supports all message types defined in the ITCH 5.0 specification. The `char` represents the message type on the wire.

### System Event Messages

| Type | Struct Name          | Description                                    |
| :--: | -------------------- | ---------------------------------------------- |
| `S`  | `SystemEventMessage` | Signals a market or data feed handler event.   |

### Stock and Administrative Messages

| Type | Struct Name                        | Description                                                               |
| :--: | ---------------------------------- | ------------------------------------------------------------------------- |
| `R`  | `StockDirectoryMessage`            | Conveys stock symbol directory information.                               |
| `H`  | `StockTradingActionMessage`        | Indicates a change in trading status for a security (e.g., Halted, Trading).|
| `Y`  | `RegSHORestrictionMessage`         | Regulation SHO short sale price test restricted indicator.                |
| `L`  | `MarketParticipantPositionMessage` | Conveys a market participant's status (e.g., Primary Market Maker).       |
| `V`  | `MWCBDeclineLevelMessage`          | Informs of the Market-Wide Circuit Breaker (MWCB) breach points for the day.|
| `W`  | `MWCBStatusMessage`                | Indicates that a MWCB level has been breached.                            |
| `h`  | `OperationalHaltMessage`           | Indicates an operational halt for a security on a specific market center. |


### Order Messages

| Type | Struct Name                       | Description                                                            |
| :--: | --------------------------------- | ---------------------------------------------------------------------- |
| `A`  | `AddOrderMessage`                 | A new order has been accepted (no MPID attribution).                   |
| `F`  | `AddOrderMPIDMessage`             | A new order has been accepted (with MPID attribution).                 |
| `E`  | `OrderExecutedMessage`            | An order on the book was executed in part or in full.                  |
| `C`  | `OrderExecutedWithPriceMessage`   | An order was executed at a price different from the display price.     |
| `X`  | `OrderCancelMessage`              | An order was partially canceled; this message contains the canceled quantity. |
| `D`  | `OrderDeleteMessage`              | An order was canceled in its entirety and removed from the book.       |
| `U`  | `OrderReplaceMessage`             | An existing order has been replaced with a new order.                  |


### Trade Messages

| Type | Struct Name          | Description                                                                   |
| :--: | -------------------- | ----------------------------------------------------------------------------- |
| `P`  | `TradeMessage`       | Reports a trade for a non-displayable order type.                             |
| `Q`  | `CrossTradeMessage`  | Reports a cross trade (e.g., Opening, Closing, IPO Cross).                    |
| `B`  | `BrokenTradeMessage` | Reports that a previously disseminated execution has been broken.             |


### Imbalance and Price Discovery Messages

| Type | Struct Name                                  | Description                                                                  |
| :--: | -------------------------------------------- | ---------------------------------------------------------------------------- |
| `K`  | `IPOQuotingPeriodUpdateMessage`              | Provides the anticipated IPO quotation release time.                         |
| `J`  | `LULDAuctionCollarMessage`                   | Indicates auction collar thresholds for a security in a LULD Trading Pause.  |
| `I`  | `NOIIMessage`                                | Net Order Imbalance Indicator message, used for opening/closing crosses.     |
| `N`  | `RetailPriceImprovementIndicator`            | Indicates the presence of Retail Price Improvement (RPII) interest.          |
| `O`  | `DirectListingCapitalRaisePriceDiscovery`    | Disseminated for Direct Listing with Capital Raise (DLCR) securities.        |

---

## Performance

The parser is designed for high-throughput scenarios. Performance is heavily dependent on the underlying hardware (CPU speed, memory bandwidth, and I/O speed).

To run the performance benchmarks:
```bash
# Configure and build with benchmarks enabled
cmake -S . -B build -DBUILD_BENCHMARKS=ON
cmake --build build --config Release

# Run the benchmark suite
./build/benchmarks/benchmark
```
The output will show the parsing rate in messages per second and throughput in GB/s. On modern server hardware, expect throughput in the multi-gigabytes per second range when parsing from an in-memory buffer.

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
