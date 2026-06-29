# Changelog

All notable changes to this project are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added

- **Documentation site**: a Doxygen configuration (`docs/Doxyfile`) using the
  README as the landing page and the modern doxygen-awesome-css theme (dark-mode
  toggle, copy buttons, interactive table of contents, tree-view sidebar), plus a
  GitHub Actions workflow (`.github/workflows/docs.yml`) that builds and publishes
  it to GitHub Pages on every push to `main`. The local `docs` CMake target
  (`-DITCH_BUILD_DOCUMENTATION=ON`) builds the same styled documentation.

- **Phase 5 — Simulation & ecosystem** ([FEATURES.md](FEATURES.md)):
  - Replay engine (`itch/replay.hpp`): `ReplayEngine` drives a callback paced by
    the messages' nanosecond timestamps at original or scaled wall-clock speed, for
    realistic backtesting and system simulation.
  - ITCH encoder / writer (`itch/encoder.hpp`): `encode_message` and `encode_frame`
    serialize any `Message` back to valid wire bytes (the inverse of the parser),
    guaranteeing `parse(encode(msg)) == msg`. This synthesizes streams for tests,
    scenario generation, and golden fixtures, and backs the Python `to_bytes()`.
  - Multi-venue / multi-version extension seam (`itch/venue.hpp`): a `VenuePolicy`
    concept and the concrete `Nasdaq50` policy, so additional ITCH-like venues and
    versions (BX/PSX, ITCH 4.1) can be added without rewriting the dispatch
    machinery. Only the NASDAQ 5.0 policy is implemented.
  - Packaging maturity: a Conan recipe (`conanfile.py`), a `CONTRIBUTING.md`, and a
    documented versioning/ABI compatibility policy.

- **Phase 4 — Research & interoperability** ([FEATURES.md](FEATURES.md)):
  - CSV and streaming sinks (`itch/io/sink.hpp`, `itch/io/csv_sink.hpp`): a generic
    `MessageSink` interface and a `CsvSink` that flattens every message type into a
    normalized wide CSV table, with no dependencies.
  - Apache Arrow / Parquet export (`itch/io/arrow_export.hpp`), gated behind the
    optional `ITCH_WITH_ARROW` CMake option (Arrow via the `arrow` vcpkg feature);
    turns a feed into a columnar table for pandas/Polars/DuckDB/Spark. Off by
    default so the core stays dependency-free.
  - `itch-tool` command-line utility (`tools/itch_tool`, `ITCH_BUILD_TOOLS`): the
    `stats`, `inspect`, `filter`, and `convert` subcommands, auto-detecting raw
    ITCH versus `.pcap`/`.pcapng` input.
  - pybind11 Python bindings (`python/`, `ITCH_BUILD_PYTHON`): a native module that
    mirrors the pure-Python `itch` package's `MessageParser` and message-class
    API (typed messages, `decode_price`) so it can serve as a faster drop-in
    backend, plus the book engine and VWAP. Includes a `pyproject.toml`
    (scikit-build-core) and pytest tests.
  - Per-message Doxygen documentation for every ITCH message struct and field in
    `itch/messages.hpp`, sourced from the message semantics in the `itch` package.

- **Phase 3 — Analytics & microstructure** ([FEATURES.md](FEATURES.md)). A header-only
  `itch::analytics` layer driven off the Phase 2 trade tape and book:
  - Bar builders (`analytics/bars.hpp`): OHLCV aggregation over time, tick, and
    volume clocks via a single `BarBuilder` template parameterized by a clock policy.
  - VWAP and TWAP (`analytics/vwap.hpp`): running and interval volume- and
    time-weighted average prices.
  - Microstructure metrics (`analytics/microstructure.hpp`): spread, mid price,
    depth-at-level, top-of-book queue imbalance, and Cont-Kukanov-Stoikov order-flow
    imbalance.
  - NOII / imbalance (`analytics/imbalance.hpp`): `ImbalanceInfo` surfacing the
    `I` message fields with strong price types and a direction description.
  - Auction reconstruction (`analytics/auctions.hpp`): `AuctionTracker` pairs each
    Cross Trade (`Q`) with the latest NOII (`I`) context to reconstruct opening,
    closing, halt, and IPO crosses.
  - Analytics tests and a VWAP example (`examples/analytics/vwap_example.cpp`).

- **Phase 2 — Production order-book engine** ([FEATURES.md](FEATURES.md)). A
  full-market book engine alongside the existing single-symbol `LimitOrderBook`
  (which is unchanged and retained):
  - `itch::book::L3Book` (`itch/book/l3_book.hpp`): an allocation-light, order-level
    book. Orders live in a reusable object pool linked into intrusive FIFO queues,
    price levels are flat sorted ladders, and order lookup by reference number uses
    a flat open-addressed map (`itch/book/order_index.hpp`), removing the per-order
    `std::shared_ptr`, list node, and map node of the original design.
  - `itch::book::BookManager` (`itch/book/book_manager.hpp`): maintains a book per
    symbol from one pass over the feed, routed by stock locate code in O(1), with an
    optional symbol universe filter.
  - Best-bid/offer change events (`set_bbo_callback`) and configurable L2 aggregated
    and L3 order-level depth snapshots (`L3Book::depth`, `L3Book::orders_at`).
  - Trade-tape extraction (`itch/tape.hpp`, `set_trade_callback`) from `E`/`C`/`P`/`Q`
    messages, exposing the `printable` flag to distinguish displayable from hidden
    prints.
  - A zero-copy overlay parse API (`itch/overlay.hpp`): lazy typed views over the
    raw buffer with on-access endianness conversion, for callers that touch only a
    few fields per message.
  - A book/overlay benchmark (`benchmarks/book_bench.cpp`) and a book-engine example
    (`examples/book/book_engine_example.cpp`), with measured numbers in the README.

- **Phase 1 — Real feed ingestion** ([FEATURES.md](FEATURES.md)). The library can
  now consume ITCH as it is actually delivered, not only pre-stripped message
  streams:
  - `itch::transport::MoldUdp64Decoder` (`itch/transport/moldudp64.hpp`) decodes
    the UDP multicast framing NASDAQ uses for live dissemination: it unpacks the
    session, sequence number, and message blocks of each datagram and feeds them
    to the existing parser, handling heartbeat and end-of-session packets.
  - `itch::transport::SoupBinDecoder` (`itch/transport/soupbintcp.hpp`) decodes
    the TCP framing used for the Glimpse snapshot and recovery/replay services,
    including login, heartbeats, logout, end-of-session, and sequenced/unsequenced
    data, buffering partial packets across stream segments.
  - `itch::transport::PcapReader` (`itch/transport/pcap.hpp`) replays a feed
    straight from a captured `.pcap`/`.pcapng` file. It is implemented in-house
    with no libpcap dependency, walking the Ethernet/IPv4/IPv6/UDP layers and
    feeding the payload through the MoldUDP64 decoder, with an optional UDP
    destination-port filter.
  - `itch::transport::SequenceTracker` and `RetransmitRequester`
    (`itch/transport/sequencing.hpp`) track per-session sequence numbers across
    the transport layer, surface gaps through a callback, and expose a re-request
    hook a caller can implement to drive recovery against a replay server.
  - A transport fuzz target (`fuzz/moldudp64_fuzzer.cpp`) and a `pcap` replay
    example (`examples/transport/pcap_replay_example.cpp`).

- `std::span<const std::byte>` overloads of `Parser::parse` as the preferred,
  size-carrying buffer interface. The `(const char*, size_t)` overloads remain as
  thin shims for C interop. (IMPROVEMENTS 3.1)
- Non-throwing `Parser::try_parse` entry points returning
  `std::expected<…, ParseError>` for latency-sensitive callers that prefer not to
  use exceptions on the hot path. Available when the standard library provides
  `std::expected` (C++23). (IMPROVEMENTS 3.2)
- An explicit error policy on `Parser`: `set_error_callback`, the
  `unknown_message_count` / `malformed_message_count` diagnostics counters, and
  `reset_diagnostics`. (IMPROVEMENTS 1.2)
- Per-message length validation: a frame whose declared length is smaller than its
  message type requires is now rejected (counted, and reported to the error
  callback) instead of being decoded into the following frame. (IMPROVEMENTS 1.1)
- Strongly typed fixed-point `Price` (`itch::StandardPrice`, `itch::MwcbPrice`) in
  `itch/price.hpp`, encoding the decimal scale in the type so the 4-vs-8 decimal
  distinction cannot be mixed up. (IMPROVEMENTS 3.3)
- Timestamp-to-wall-clock helpers in `itch/time.hpp` (`to_time_point`,
  `format_timestamp`, `format_time_point`). (IMPROVEMENTS 3.4)
- Parser fuzz target (`fuzz/parser_fuzzer.cpp`, `ITCH_BUILD_FUZZERS`) and a
  time-budgeted CI fuzzing job. (IMPROVEMENTS 5.4)
- Published, reproducible benchmark numbers in the README and a CI benchmark smoke
  job. (IMPROVEMENTS 2.4)
- `scripts/fetch_sample.sh` to download an official ITCH sample on demand.

### Changed

- **Behavior change:** the library no longer writes to `std::cerr` on an unknown
  message type. Unknown types are now silently skipped, counted, and surfaced
  through the optional error callback. Callers that relied on the stderr output
  should install an error callback or read the diagnostics counters.
  (IMPROVEMENTS 1.2)
- Message dispatch now uses a compile-time, type-byte-indexed flat table instead of
  a `std::map<char, std::function<…>>`, removing the per-message tree lookup and
  type-erased call. (IMPROVEMENTS 2.1)
- Byte swapping now uses `std::byteswap` / `std::bit_cast` instead of union
  type-punning, eliminating undefined behavior on the hot path. (IMPROVEMENTS 1.3)
- Order-book rendering and indicator lookups now use `std::format` and `constexpr`
  frozen tables instead of iostream manipulators and namespace-scope `std::map`
  globals. (IMPROVEMENTS 4.2, 4.3)
- Standardized the minimum CMake version on 3.25 across the project.
  (IMPROVEMENTS 5.2)
- README architecture section corrected to describe the parser as a fast,
  allocation-free, single-pass field decoder rather than a "zero-copy /
  zero-overhead" deserializer. (IMPROVEMENTS 2.2)

### Fixed

- Renamed misspelled public CMake helpers: `anable_address_sanitizer` →
  `enable_address_sanitizer`, `build_documenation` → `build_documentation`,
  `apply_formating` → `apply_formatting`, `apply_clang_tidy_globaly` →
  `apply_clang_tidy_globally`. (IMPROVEMENTS 5.5)
- CI now runs on pull requests and adds clang-format, clang-tidy, ASAN/UBSAN, and
  coverage jobs. (IMPROVEMENTS 5.3)
- Stopped tracking the generated `docs/coverage.html`; coverage is now generated
  into the build tree only. (IMPROVEMENTS 5.1)
