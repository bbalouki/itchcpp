# Changelog

All notable changes to this project are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added

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
