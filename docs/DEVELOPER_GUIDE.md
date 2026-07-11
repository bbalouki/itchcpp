# ITCHCPP Developer Guide

This guide is for people who want to understand, extend, or contribute to
ITCHCPP itself, not just use it. It answers the questions the README and
per-symbol Doxygen docs don't: what are the pieces, how do they actually call
into each other, and why is the codebase shaped this way?

- For **features and usage snippets**, see [README](../README.md).
- For **build commands, code style, and the PR process**, see
  [CONTRIBUTING](../CONTRIBUTING.md) - this guide won't repeat those.
- For **per-symbol API reference** (every class, function, parameter), build
  the Doxygen docs (`-DITCH_BUILD_DOCUMENTATION=ON`, target `docs`) or browse
  the [published copy](https://bbalouki.github.io/itchcpp/).
- For **runnable code demonstrating every feature**, see [Examples](../examples/).

---

## Table of Contents

1. [Repository Map](#1-repository-map)
2. [The Wire-Format Core](#2-the-wire-format-core)
3. [Two Ways to Read a Message: `Parser` vs. `overlay::MessageView`](#3-two-ways-to-read-a-message-parser-vs-overlaymessageview)
4. [Feed Ingestion (`transport/`)](#4-feed-ingestion-transport)
5. [The Book Engine: Two Designs, Not One Layered on the Other](#5-the-book-engine-two-designs-not-one-layered-on-the-other)
6. [Analytics](#6-analytics)
7. [IO Sinks](#7-io-sinks)
8. [Simulation & Round-Trip](#8-simulation--round-trip)
9. [The Venue Extension Seam](#9-the-venue-extension-seam)
10. [Build System Tour](#10-build-system-tour)
11. [Python Bindings](#11-python-bindings)
12. [Where Verification Lives](#12-where-verification-lives)
13. [How Do I Add...](#13-how-do-i-add)

---

## 1. Repository Map

| Directory                  | What lives there                                                                                                                                                                                 |
| -------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| `include/itch/`            | The public C++ API - every header a consumer includes. This is the contract; see [CONTRIBUTING](../CONTRIBUTING.md#versioning-and-compatibility-policy) for what counts as a breaking change to it. |
| `src/`                     | The implementation behind those headers, plus a handful of internals (e.g. `detail/wire.hpp` has no `.cpp`, it's header-only metadata). Mirrors `include/itch/`'s subdirectory structure.        |
| `python/`                  | The pybind11 bindings (`python/src/bindings.cpp`) and the pure-Python package that wraps them (`python/itchcpp/`), packaged via `pyproject.toml`/scikit-build-core.                              |
| `tests/`                   | GoogleTest suite, mirroring `include/`+`src/`'s layout.                                                                                                                                          |
| `examples/`                | Small, runnable, single-file programs, one per feature area - start here if you learn by reading working code.                                                                                   |
| `benchmarks/`              | Google Benchmark suites (`parser_bench`, `book_bench`).                                                                                                                                          |
| `fuzz/`                    | libFuzzer targets (Clang-only) for the two components that parse untrusted bytes: `Parser` and `MoldUdp64Decoder`.                                                                               |
| `tools/itch_tool/`         | The `itch-tool` CLI (stats/inspect/filter/convert subcommands over a real feed).                                                                                                                 |
| `cmake/`                   | Shared CMake helpers (`Helpers.cmake`, `Versions.cmake`) - option definitions, dependency pinning, reusable functions. See [§10](#10-build-system-tour).                                         |
| `packaging/bbalouki-itch/` | The vcpkg registry port definition for this library (a separate git checkout, since vcpkg ports typically live in the community registry).                                                       |
| `docs/`                    | Doxygen output (`docs/html`, gitignored) plus `Doxyfile.in`, the CMake-configured template that generates it.                                                                                    |

---

## 2. The Wire-Format Core

Three independent components - the eager `Parser`, the zero-copy `overlay`,
and the write-side `Encoder` - all need to agree on the same thing: which
type byte maps to which message struct, and how many bytes that message
occupies on the wire. Rather than each hand-rolling its own `switch` over
type bytes (and inevitably drifting out of sync as message types are added),
they all build their dispatch tables from one place:
[`include/itch/detail/wire.hpp`](../include/itch/detail/wire.hpp).

That header defines exactly two things:

- `detail::for_each_message_type(visitor)` - the canonical, single enumeration
  of all 23 ITCH message types, invoked as
  `visitor.template operator()<MsgType>(type_byte)` for each one, in spec
  order (`'S'` → `SystemEventMessage`, ... `'A'` → `AddOrderMessage`, ...
  `'O'` → `DLCRMessage`).
- `detail::WIRE_SIZE<MsgType>` - the exact on-wire byte size of a message
  type, derived as `sizeof(MsgType) - TIMESTAMP_STRUCT_PADDING` (every
  message struct stores its 48-bit wire timestamp in a 64-bit field, so it's
  always 2 bytes wider in memory than on the wire - `wire.hpp` centralizes
  that one correction so nobody has to remember it).

[`include/itch/messages.hpp`](../include/itch/messages.hpp) defines the 23
message structs themselves (plain aggregates, `#pragma pack(push, 1)`'d so
their in-memory layout can be reasoned about directly) plus the
`Message = std::variant<SystemEventMessage, ..., DLCRMessage>` that every
decoded message becomes.

`Parser`, `overlay`, and `Encoder` each call
`detail::for_each_message_type` at **compile time** (via `consteval`
functions) to build their own 256-entry, type-byte-indexed table - a decode
dispatch table for `Parser`, a size-validation table for `overlay`, nothing
extra needed for `Encoder` since it just switches on the `Message` variant's
active alternative. The payoff: add a message type to `wire.hpp`'s registry
once, and the parser, the overlay, and (with a small addition, see
[§13](#13-how-do-i-add)) the encoder all pick it up.

---

## 3. Two Ways to Read a Message: `Parser` vs. `overlay::MessageView`

These are **two fully independent, parallel entry points into the same
message set** - neither is built on top of the other, and their framing
loops never call each other. What they share is only the compile-time
registry in `wire.hpp` described above, which guarantees they can never
disagree about which type bytes are valid or how long a message is.

**`itch::Parser`** ([`include/itch/parser.hpp`](../include/itch/parser.hpp),
implemented in `src/parser.cpp`) is _eager_: its `consteval`-built
`DISPATCH_TABLE` maps each type byte to a `decode_typed<MsgType>` function
that unpacks **every field** of the message into a host-order struct,
converting endianness for each one regardless of whether the caller ever
reads it. This is the right choice when you're going to touch most/all of a
message's fields anyway - persisting to a sink, feeding a book, etc.

**`itch::overlay::MessageView`**
([`include/itch/overlay.hpp`](../include/itch/overlay.hpp), header-only, no
`.cpp`) is _lazy_: `overlay::for_each_message` runs its own independently
written 2-byte-length-prefix framing loop (validating length against a
parallel `SIZE_TABLE`, also built from `wire.hpp`), but instead of decoding
anything, it hands the callback a non-owning `MessageView{pointer, length}`
over the original buffer. Field accessors on the per-type `*View` subclasses
(`AddOrderView::price()`, `stock()`, ...) do a `memcpy` + endian-swap **on
demand**, only for the fields you actually call. This is the right choice for
selective/filtering hot paths - e.g. a strategy that only cares about a
handful of symbols' top-of-book price, and would rather not pay to decode 20
fields of every `AddOrderMessage` it sees. The tradeoff is that a
`MessageView` is only valid while the underlying buffer is alive (it holds a
raw pointer into it), whereas a decoded `Message` is a self-contained value.

`itch::Encoder` ([`include/itch/encoder.hpp`](../include/itch/encoder.hpp)) is
the write-side counterpart to both - see [§8](#8-simulation--round-trip).

See `examples/parser/parser_example.cpp` and `examples/overlay/overlay_example.cpp`
side by side for a concrete comparison.

---

## 4. Feed Ingestion (`transport/`)

There are three transport-layer entry points under
[`include/itch/transport/`](../include/itch/transport/), and they are **not**
peers of equal standing - the actual call graph, confirmed by reading each
`.cpp`, is:

```
.pcap/.pcapng file → PcapReader ──owns──▶ MoldUdp64Decoder ──owns──▶ Parser
                                                  ▲
raw UDP datagram ────────────────────────────────┘   (used directly, no pcap needed)

raw TCP byte stream → SoupBinDecoder ──owns (separately)──▶ Parser
```

- **`PcapReader`** (`transport/pcap.hpp`) reads a `.pcap`/`.pcapng` capture
  (classic or `pcapng`, byte-swapped variants handled) with no libpcap
  dependency - it walks Ethernet/VLAN/Linux-SLL link layers down to a UDP
  payload, applies an optional destination-port filter, and hands each
  payload to an **embedded `MoldUdp64Decoder`** it owns. It doesn't decode
  ITCH itself; it's pcap-framing bolted onto MoldUDP64.
- **`MoldUdp64Decoder`** (`transport/moldudp64.hpp`) is usable standalone -
  this is the path for live UDP multicast capture, feeding it one datagram
  at a time via `decode_packet()` without ever touching a pcap file. It
  strips the fixed 20-byte MoldUDP64 header, feeds the session/sequence
  info to its embedded `SequenceTracker`, and hands the remaining
  length-prefixed ITCH blocks to its own **embedded `Parser`**.
- **`SoupBinDecoder`** (`transport/soupbintcp.hpp`) is an **independent
  sibling**, not built on MoldUDP64 - it's the TCP/Glimpse/replay-session
  counterpart for a different NASDAQ delivery mechanism (reliable ordered TCP
  vs. UDP multicast). It reassembles a byte stream into discrete SoupBinTCP
  packets, learns the session/sequence number from the Login Accepted
  packet, and decodes sequenced/unsequenced data packets through its own
  **separate embedded `Parser`**.
- **`SequenceTracker`/`RetransmitRequester`** (`transport/sequencing.hpp`) is
  a small, protocol-agnostic, header-only utility shared _by pattern_, not by
  a single shared instance - both `MoldUdp64Decoder` and `SoupBinDecoder`
  embed their own private `SequenceTracker`. It tracks the next expected
  sequence number per session, reports gaps through an optional
  `GapCallback`, and - since this library never does network I/O itself -
  calls an injected `RetransmitRequester::request_retransmit()` so the
  _caller_ can issue the actual re-request against whatever recovery/replay
  service they use.

Because each decoder owns a private `Parser`, diagnostics counters
(`unknown_message_count()`, etc.) and `messages_decoded()` are per-decoder,
not global - if you're running multiple sessions concurrently, you get
independent bookkeeping for free.

See `examples/transport/pcap_replay_example.cpp` (file-based),
`examples/transport/moldudp64_example.cpp` (direct datagram decode + gap
recovery), and `examples/transport/soupbintcp_example.cpp` (TCP framing).

---

## 5. The Book Engine: Two Designs, Not One Layered on the Other

`include/itch/order_book.hpp` and `include/itch/book/` contain **two
independent single-symbol order-book implementations** with different
performance characteristics, plus a multi-symbol router built on top of the
second one.

- **`itch::LimitOrderBook`** (`order_book.hpp`, `src/order_book.cpp`) is the
  original design: `std::map<price, PriceLevel>` per side, each level a
  `std::list<std::shared_ptr<Order>>` FIFO queue, order lookup via a
  `std::map<reference, iterator>`. It consumes `Message` directly through its
  own `process()` (a `std::visit` dispatch), is single-symbol by
  construction, and has a `print()` method for quick terminal visualization.
  Simple and easy to reason about; every order is a heap allocation.
- **`itch::book::L3Book`** (`book/l3_book.hpp`, `src/book/l3_book.cpp`) is a
  from-scratch, allocation-light rewrite: orders live in a reusable object
  pool (flat vector + free list) linked into intrusive FIFO queues, and price
  levels are kept in a flat, sorted vector per side - no per-order heap
  allocation, no `shared_ptr` refcounting, no per-level node allocation.
  Crucially, **`L3Book` has no `process(Message)` method** - it exposes
  primitive mutations instead (`add_order`, `execute_order`, `reduce_order`,
  `delete_order`, `replace_order`) plus L2 (`depth()`) and true L3
  (`orders_at()`) queries and a `bbo()` accessor. Order lookup is O(1) via
  `book::OrderIndex` (`book/order_index.hpp`), a flat open-addressed hash map
  purpose-built for this access pattern (see its own doc comment for why
  `std::unordered_map` wasn't good enough at hundreds of millions of
  add/delete messages).
- **`itch::book::BookManager`** (`book/book_manager.hpp`,
  `src/book/book_manager.cpp`) is the `Message` → `L3Book` translator, and a
  **multi-symbol wrapper specifically around `L3Book`** (never
  `LimitOrderBook`). It owns a flat, `stock_locate`-indexed table of
  `{L3Book, last_bbo}` entries, routes each incoming `Message` to the right
  `L3Book` primitive call, synthesizes `itch::Trade` tape events (from
  `tape.hpp`) for executions and non-displayed prints, and fires a
  `BboCallback` only when a book's best bid/offer actually changes. This is
  how a single pass over a full-market feed reconstructs every symbol's book
  at once, without pre-constructing one `LimitOrderBook` per known symbol.

If you're deciding which to reach for: `LimitOrderBook` is the simpler,
single-symbol, map-based option; `BookManager`+`L3Book` is the
production-grade, multi-symbol, allocation-conscious option and is what the
book/BBO/trade-tape-oriented analytics examples build on.

See `examples/order_book/order_book_example.cpp` (`LimitOrderBook`) vs.
`examples/book/book_engine_example.cpp` (`BookManager`).

---

## 6. Analytics

Everything under [`include/itch/analytics/`](../include/itch/analytics/)
consumes `Message`, `itch::Trade` (from `tape.hpp`), or book state (an
`L3Book`/`Bbo`) - none of it is a decode path itself, and none of it mutates
a book. Think of this layer as pure functions/accumulators sitting downstream
of the book engine (or, for the free functions, directly beside it):

- **`Vwap`/`Twap`** (`vwap.hpp`) - running volume-weighted and time-weighted
  average price accumulators. `Vwap::add(price, shares)` accumulates an
  execution; `Twap::add(price, timestamp)` records that the price became
  `price` at `timestamp` (it time-weights by the gap between samples, so it
  takes a timestamp instead of a share count). Both expose `value()`/`reset()`.
- **`BarBuilder`** (`bars.hpp`) - OHLCV bar aggregation, parameterized by a
  clock policy (`TimeClock`, `TickClock`, `VolumeClock`) that decides when a
  bar closes; `add()` feeds a trade in, bars are surfaced as they close.
- **`microstructure.hpp`** - free functions computing point-in-time
  statistics against current book state: `spread`, `mid_price`,
  `queue_imbalance`, `depth_at_level`, `order_flow_imbalance`.
- **`imbalance.hpp`** - `make_imbalance_info` decodes a NOII message into a
  friendlier `ImbalanceInfo` (paired/imbalance shares, direction, cross
  prices).
- **`auctions.hpp`** - `AuctionTracker` reconstructs opening/closing/halt/IPO
  crosses by consuming the imbalance messages above plus the eventual cross
  trade, and reports the finished cross through a callback.

See `examples/analytics/vwap_example.cpp` (`Vwap`+`Twap`),
`examples/analytics/bars_example.cpp`, `examples/analytics/microstructure_example.cpp`,
and `examples/analytics/auctions_example.cpp`.

---

## 7. IO Sinks

[`include/itch/io/sink.hpp`](../include/itch/io/sink.hpp) defines
`itch::io::MessageSink`, a tiny abstract interface (`write(const Message&)`,
optional `flush()`). Sinks consume `Message`s **in parallel with**, not
downstream of, the book/analytics path - a parse callback can hand the same
message to a sink and to a book manager independently.

- **`CsvSink`** (`io/csv_sink.hpp`) flattens every message type into one
  wide, normalized CSV row schema.
- **`ArrowExporter`** (`io/arrow_export.hpp`) does the same into Arrow
  columnar builders, with `write_parquet()` to flush a file. This class only
  exists when the library is built with `-DITCH_WITH_ARROW=ON`; the header is
  guarded with `#ifdef ITCH_WITH_ARROW` so it compiles to nothing when the
  option is off, keeping the core dependency-free. `src/CMakeLists.txt`
  applies the `ITCH_WITH_ARROW` compile definition and the Arrow/Parquet link
  libraries to the `itch` target as `PUBLIC`, which is what makes this
  conditional API surface stay consistent between the library build and
  anything that links against it - a consumer either sees `ArrowExporter` at
  all (Arrow-enabled build) or doesn't (default build), never a broken
  half-state.

See `examples/io/csv_sink_example.cpp` and `examples/io/arrow_export_example.cpp`.

---

## 8. Simulation & Round-Trip

Two more pieces sit alongside the parse/decode path as thin, purpose-built
wrappers rather than alternative decoders:

- **`itch::ReplayEngine`** ([`include/itch/replay.hpp`](../include/itch/replay.hpp))
  wraps a private `Parser`-driven parse of a whole buffer, then re-invokes
  the caller's callback paced by each message's ITCH timestamp (scaled by a
  speed multiplier) - for backtesting/simulation against realistic timing,
  not a distinct decode format.
- **`itch::Encoder`** ([`include/itch/encoder.hpp`](../include/itch/encoder.hpp),
  `src/encoder.cpp`) is the write-side inverse of `Parser`: `encode_message`/
  `encode_frame` serialize a `Message` back to spec-order, big-endian wire
  bytes, sharing the same `wire.hpp` registry described in
  [§2](#2-the-wire-format-core). This is what makes the round-trip guarantee
  `parse(encode(msg)) == msg` possible, and it's also the tool this codebase
  (and its examples) use to synthesize valid ITCH streams without needing a
  committed multi-gigabyte sample file - `.gitignore` deliberately excludes
  real ITCH/pcap samples (see `scripts/fetch_sample.sh` for how to get a real
  one), so `encode_frame` is how tests and examples build realistic,
  self-contained input in-code instead.

See `examples/encoder/encoder_example.cpp` and `examples/replay/replay_example.cpp`.

---

## 9. The Venue Extension Seam

[`include/itch/venue.hpp`](../include/itch/venue.hpp) defines a `VenuePolicy`
concept and `Nasdaq50`, the concrete policy this library ships with. This is
a library-extension point, not an application-facing feature (which is why
it has no dedicated `examples/` entry) - it's the seam
[CONTRIBUTING](../CONTRIBUTING.md#versioning-and-compatibility-policy)
refers to when it says "new venues/protocols are added behind the
`itch::venue::VenuePolicy` seam without breaking existing policies." A
second venue with a different message set or dispatch table would implement
the same concept and plug in alongside `Nasdaq50` without any of the
existing transport/parser/book code needing to change.

---

## 10. Build System Tour

**Target graph** (arrows = "links against"):

```
itch (STATIC/SHARED, alias itch::itch)   <- everything below links this
 ├─ warnings::strict (PRIVATE, an INTERFACE library defined in cmake/Helpers.cmake:
 │    /W4 /WX on MSVC, -Wall -Wpedantic -Werror -Wconversion -Wshadow ... elsewhere)
 └─ optionally PUBLIC: Arrow::arrow_shared, Parquet::parquet_shared (if ITCH_WITH_ARROW)

itch_tests (tests/)                     -> itch::itch, GTest
parser_bench, book_bench (benchmarks/)  -> itch::itch, Google Benchmark
<name>_example (examples/)              -> itch::itch, warnings::strict
parser_fuzzer, moldudp64_fuzzer (fuzz/) -> itch::itch (Clang-only)
itch_tool (tools/itch_tool/)            -> itch::itch, warnings::strict
itchcpp_python (python/)                -> itch::itch, pybind11
```

The `itch` library's sources are exactly: `parser.cpp`, `messages.cpp`,
`order_book.cpp`, `transport/{moldudp64,soupbintcp,pcap}.cpp`,
`book/{l3_book,book_manager}.cpp`, `io/{csv_sink,arrow_export}.cpp`,
`encoder.cpp`, `replay.cpp`. `io/arrow_export.cpp` is _always_ compiled but is
a no-op translation unit unless `ITCH_WITH_ARROW` is defined - the option
adds `find_package(Arrow/Parquet CONFIG REQUIRED)`, a `PUBLIC` compile
definition, and `PUBLIC` link libraries to the `itch` target (see
[§7](#7-io-sinks) for why `PUBLIC` matters here). The core library is
otherwise dependency-free by design.

**Options** (all default `OFF` except `ITCH_CXX_STANDARD`, defined in
`cmake/Helpers.cmake`): `ITCH_BUILD_TESTS`, `ITCH_BUILD_BENCHMARKS`,
`ITCH_BUILD_EXAMPLES`, `ITCH_BUILD_FUZZERS`, `ITCH_BUILD_TOOLS`,
`ITCH_BUILD_PYTHON` gate the corresponding `add_subdirectory()` in the root
`CMakeLists.txt`; `ITCH_WITH_ARROW` gates the Arrow/Parquet wiring above; dev/CI
toggles `ITCH_ADD_COVERAGE_ANALYSIS`, `ITCH_APPLY_FORMATING`,
`ITCH_APPLY_CLANG_TIDY_GLOBALY`, `ITCH_BUILD_DOCUMENTATION`,
`ITCH_ENABLE_ADDRESS_SANITIZER` each map to a function in
`cmake/Helpers.cmake` invoked once from the root file
(`build_documentation()` fetches the `doxygen-awesome-css` theme via
`FetchContent` and wires up the `docs` target described at the top of this
guide; `apply_clang_tidy_globally()`, `add_coverage_analysis()`,
`apply_formatting()`, `enable_address_sanitizer()` do what their names say).

**`CMakePresets.json`** preset names are combinations of: dependency manager
(`_vcpkg`/`_conan`), build type/instrumentation (`_debug`/`_release`/
`_coverage`/`_sanitize`), generator (`_VS`/`_ninja`), and cross-compilation
(`_mingw`). The base preset turns on tests, examples, benchmarks,
clang-tidy, clang-format, and coverage - i.e. it's the "dev" configuration.

**Two packaging surfaces** ship this library to consumers who don't want to
`add_subdirectory()` it: `conanfile.py` (a `with_arrow` option that wires
straight through to the CMake toolchain via `CMakeToolchain.variables`) and
`packaging/bbalouki-itch/` (a vcpkg port; its `vcpkg.json` models the same
choice as an `arrow` _feature_, bridged into the CMake configure step via
`vcpkg_check_features()` in `portfile.cmake`).

---

## 11. Python Bindings

`python/src/bindings.cpp` is a **thin pybind11 wrapper directly exposing the
real C++ classes** - it `#include`s the actual library headers
(`itch/parser.hpp`, `itch/book/book_manager.hpp`,
`itch/analytics/vwap.hpp`, `itch/encoder.hpp`, `itch/messages.hpp`) and
registers `py::class_<>` bindings for `BookManager`, `L3Book`, `Bbo`, `Vwap`,
and every message struct almost verbatim. Message structs share a template
helper `bind_common<MsgType>()` for their common fields
(`message_type`/`stock_locate`/`tracking_number`/`timestamp`) plus
upstream-compatible convenience methods (`decode()`, `to_bytes()`, ...).

The one deliberately non-1:1 piece is the message-parsing surface: because
the Python package's stated goal is to be a **drop-in backend for the
pure-Python `itchfeed`/`itch` PyPI package**, `bindings.cpp` hand-implements
a `MessageParser` class matching that upstream package's exact iteration
semantics (a `MessageIterator` with `__iter__`/`__next__`, file-object
byte-cache refilling, type filtering, stop-on-`SystemEvent('C')`) - internally
it still uses the real `itch::Parser` per frame, but the Python-facing
iterator protocol is bespoke rather than a direct binding of `Parser::parse`.

Package layout (`python/itchcpp/`):

| Module          | Maps to                                                                                                                                                                            |
| --------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `messages.py`   | Re-exports native message classes from the compiled extension; registers them as virtual subclasses of `_bases.py`'s pure-Python ABCs so `isinstance()` matches upstream behavior. |
| `parser.py`     | Re-exports `MessageParser`/`AllMessages` ↔ `parser.hpp` (with the iterator caveat above).                                                                                          |
| `book.py`       | Re-exports `BookManager`/`L3Book`/`Bbo` ↔ `book/book_manager.hpp`+`book/l3_book.hpp` - called out in `python/README.md` as a "native extra" beyond the upstream API.               |
| `analytics.py`  | Re-exports `Vwap` ↔ `analytics/vwap.hpp` - also a native extra.                                                                                                                    |
| `indicators.py` | Pure Python, no C++ involved - vendored lookup dicts mirroring `include/itch/indicators.hpp`.                                                                                      |
| `_bases.py`     | Pure Python ABCs with no C++ involved, matching upstream `itch.messages`' base classes.                                                                                            |

See `python/README.md` for the usage snippet and full exposure inventory.

---

## 12. Where Verification Lives

- **`tests/`** - GoogleTest, `gtest_discover_tests`, mirroring
  `include/`+`src/`'s layout (top-level parser/messages/order_book/price-time/
  encoder/conformance tests, plus `analytics/`, `book/`, `io/`, `transport/`
  subdirectories). `test_conformance.cpp` is the golden/spec test - a
  deterministic, hand-built stream so the repo doesn't need to commit a large
  binary sample just to have an end-to-end conformance check.
- **`fuzz/`** - libFuzzer (Clang-only; the target list is a no-op on other
  compilers), covering the two components that parse untrusted bytes:
  `parser_fuzzer` (`Parser`) and `moldudp64_fuzzer` (`MoldUdp64Decoder`).
- **`benchmarks/`** - Google Benchmark, `parser_bench` and `book_bench`, for
  perf regressions.
- **`python/tests/`** - pytest, separate from the C++ suite;
  `test_parity.py` specifically checks behavioral parity against the
  upstream pure-Python `itchfeed` package, since that compatibility is a
  stated goal of the bindings (§11).

See [CONTRIBUTING](../CONTRIBUTING.md#getting-started) for the actual build
and `ctest`/`pytest` commands - CI runs all of the above across GCC, Clang,
and MSVC, at both C++20 and C++23.

---

## 13. How Do I Add...

**...a new ITCH message type?** Touch, in order: `messages.hpp` (add the
`#pragma pack(push, 1)`-friendly struct), `detail/wire.hpp`'s
`for_each_message_type` (register the type byte - this is what makes
`Parser`, `overlay`, and `Encoder` all pick it up), `parser.cpp` (add the
`unpack_message` overload that decodes it field-by-field), `overlay.hpp` (add
the matching `*View` subclass with lazy field accessors at the right byte
offsets), and `encoder.cpp` (add the case that serializes it back to wire
bytes). `messages.py`/`bindings.cpp` on the Python side if it should be
exposed there too.

**...a new analytics module?** It should consume `Message`, `itch::Trade`
(from `tape.hpp`), or book state (`L3Book`/`Bbo`) - see [§6](#6-analytics)
for the existing pattern (an accumulator with `add()`/`value()`/`reset()`, or
a set of free functions taking book state, or a tracker with a result
callback). It shouldn't need to know about transport or parsing at all.

**...a new IO sink?** Implement `itch::io::MessageSink`
(`write(const Message&)`, optional `flush()`) - see `CsvSink` for the
simplest concrete example. A sink should be independent of the book/analytics
path, not layered on it.

**...a new venue?** Implement the `itch::venue::VenuePolicy` concept
(`venue.hpp`) alongside `Nasdaq50` - see [§9](#9-the-venue-extension-seam).

Before opening a PR, also read
[CONTRIBUTING](../CONTRIBUTING.md#code-style) for style/testing expectations
and the [compatibility policy](../CONTRIBUTING.md#versioning-and-compatibility-policy)
for what counts as a breaking change.
