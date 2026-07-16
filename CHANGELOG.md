# Changelog

All notable changes to this project are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [1.6.2] - 2026-07-11

### Fixed

- Sign-conversion warning in the Arrow/Parquet `WriteTable` chunk size
  calculation.

## [1.6.1] - 2026-07-11

### Fixed

- Arrow/Parquet static targets now link correctly when shared libraries are
  not built (`ITCH_WITH_ARROW` with static vcpkg triplets).

## [1.6.0] - 2026-07-11

### Added

- **Feed ingestion / transport** (`itch/transport/`): `MoldUdp64Decoder` and
  `SoupBinDecoder` for live and recovery framing, a `PcapReader` for
  `.pcap`/`.pcapng` captures, and a `SequenceTracker` for per-session gap
  detection, all implemented in-house with no libpcap dependency.
- **Full-market book engine** (`itch::book::BookManager`, `L3Book`): allocation-
  light, multi-symbol L3 order book reconstruction with an object pool,
  intrusive FIFO price levels, flat sorted ladders, O(1) order lookup, BBO
  change callbacks, and L2/L3 depth snapshots.
- **Zero-copy overlay API** (`itch/overlay.hpp`): lazy typed views
  (`MessageView`, `AddOrderView`, ...) that decode only the fields the caller
  reads.
- **Analytics** (`itch::analytics`): `BarBuilder` (time/tick/volume clocks),
  `Vwap`/`Twap`, spread/mid/depth/queue-imbalance and order-flow-imbalance
  helpers, NOII decoding, and auction (opening/closing/halt/IPO) reconstruction.
- **Interoperability**: a dependency-free CSV sink, optional Arrow/Parquet
  export (`ITCH_WITH_ARROW`), the `itch-tool` CLI (`stats`, `inspect`,
  `filter`, `convert`), and native Python bindings (`itchcpp`, pybind11)
  mirroring the pure-Python `itch`/`itchfeed` package layout and semantics.
- **Simulation & ecosystem**: a timestamp-paced `ReplayEngine`, a full ITCH
  encoder/writer (`itch/encoder.hpp`) with a guaranteed
  `parse(encode(msg)) == msg` round-trip, a `VenuePolicy` extension seam
  (`itch::venue::Nasdaq50`) for future venues/versions, and Conan packaging
  alongside the existing vcpkg port.
- Doxygen documentation site (`docs/Doxyfile`) published to GitHub Pages on
  every push to `main`, and a local `docs` CMake target
  (`-DITCH_BUILD_DOCUMENTATION=ON`).
- `CONTRIBUTING.md` documenting the build matrix, code style, and the
  versioning/ABI compatibility policy.

## [1.1.0] - 2026-01-17

### Added

- `itch::LimitOrderBook`, a single-symbol order book built from the message
  stream, with an example and dedicated tests.
- CMake `FetchContent` support as an alternative to vcpkg for consuming the
  library.

### Changed

- Reorganized packaging metadata (moved vcpkg port config to `packaging/`)
  and renamed CMake targets/exports for consistency.
- Adopted trailing return types across the codebase and tightened compiler
  warnings (`-Wshadow`, sign-conversion, unreferenced parameters).

## [1.0.0] - 2025-10-29

### Added

- Initial release: an allocation-free, single-pass NASDAQ TotalView-ITCH 5.0
  parser with all message types deserialized into a type-safe `itch::Message`
  `std::variant`, plus `std::vector`- and callback-based parsing entry points
  with optional message-type filtering.

[Unreleased]: https://github.com/bbalouki/itchcpp/compare/v1.6.2...HEAD
[1.6.2]: https://github.com/bbalouki/itchcpp/compare/v1.6.1...v1.6.2
[1.6.1]: https://github.com/bbalouki/itchcpp/compare/v1.6.0...v1.6.1
[1.6.0]: https://github.com/bbalouki/itchcpp/compare/v1.1.0...v1.6.0
[1.1.0]: https://github.com/bbalouki/itchcpp/compare/v1.0.0...v1.1.0
[1.0.0]: https://github.com/bbalouki/itchcpp/releases/tag/v1.0.0
