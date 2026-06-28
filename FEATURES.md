# FEATURES

This document is the product roadmap for ITCHCPP. Where [IMPROVEMENTS.md](IMPROVEMENTS.md)
is about making the existing parser correct, fast, and honest, this file is about
the new capabilities that turn a NASDAQ ITCH 5.0 file parser into a market-data
platform that quantitative teams, trading firms, and researchers can rely on
everywhere in their workflow.

## Vision

> A single, modern C++ library that ingests real exchange market data — live or
> historical — reconstructs the book, exposes microstructure analytics, and feeds
> both production trading systems and Python research notebooks, with the speed
> expected of latency-sensitive infrastructure.

Today ITCHCPP parses the raw, length-prefixed ITCH 5.0 message stream and
reconstructs a single-symbol order book. To reach the vision it needs to read the
data formats that actually arrive on the wire and on disk, scale the book to whole
markets, compute the metrics quants ask for, and meet researchers in the tools
they already use.

The roadmap is phased. Each phase is independently valuable and shippable; later
phases build on earlier ones. The order reflects what unlocks the most real-world
usage first.

---

## Phase 1 — Real feed ingestion

_Goal: make ITCHCPP usable on data as it actually exists, not just pre-stripped
message streams._

The current parser assumes a raw concatenation of length-prefixed messages. Real
ITCH is delivered inside transport framing. Without these decoders the library
cannot consume a live feed or most captured data.

- **MoldUDP64 decoder** — the UDP multicast framing NASDAQ uses for live
  dissemination. Unpacks sessions, sequence numbers, and message blocks, then
  feeds the existing message parser. This is the gateway to live data.
- **SoupBinTCP decoder** — the TCP framing used for the glimpse snapshot and
  recovery/replay services. Handles login, heartbeats, and sequenced payloads.
- **pcap ingestion** — replay market data straight from captured network traffic
  (`.pcap`/`.pcapng`), the most common form in which firms archive and share feeds.
- **Gap detection and recovery hooks** — track sequence numbers across the
  transport layer, surface gaps, and expose re-request hooks so a caller can drive
  recovery against a replay server.

---

## Phase 2 — Production order-book engine

_Goal: reconstruct the full market, fast, not just one symbol._

The current `LimitOrderBook` is constructed for a single symbol
([include/itch/order_book.hpp:75](include/itch/order_book.hpp)) and is allocation-
heavy (see [IMPROVEMENTS.md](IMPROVEMENTS.md) 2.3). A serious book engine handles
every symbol on the feed at line rate.

- **Multi-symbol book manager** — maintain books for all (or a selected universe
  of) symbols from one pass over the feed, keyed by stock locate code for O(1)
  routing.
- **Allocation-free internals** — order object pool / arena, intrusive FIFO queues
  per price level, and flat price ladders, with O(1) lookup of orders by reference
  number. Removes the per-order heap and refcount cost. (Deferred book rewrite
  tracked from [IMPROVEMENTS.md](IMPROVEMENTS.md) 2.3; the current `std::shared_ptr`
  book is correct but allocation-heavy.)
- **Zero-copy overlay parse API** — an alternative to the current single-pass field
  decoder: typed views over the raw buffer with lazy, on-access endianness
  conversion, for callers that touch only a few fields per message. The library
  today decodes eagerly into host-order structs; this overlay is tracked from
  [IMPROVEMENTS.md](IMPROVEMENTS.md) 2.2 as the stronger product direction.
- **BBO and top-of-book change events** — emit best-bid/offer updates as they
  happen, the primary signal most consumers need.
- **Depth snapshots** — configurable L2 aggregated depth and full L3 order-level
  detail, on demand or on every change.
- **Trade tape extraction** — a clean stream of executions, correctly
  distinguishing displayable from non-displayable/hidden prints using the
  printable flag.

---

## Phase 3 — Analytics & microstructure

_Goal: answer the questions quants actually ask, in the library, correctly._

Once the book and tape are reconstructed, the high-value layer is derived metrics.
Computing them centrally and correctly saves every downstream team from
reimplementing them.

- **Bar builders** — OHLCV aggregation over time, volume, and tick clocks.
- **VWAP / TWAP** — running and interval volume- and time-weighted average prices.
- **Order-flow imbalance and microstructure metrics** — spread, depth at level,
  queue imbalance, and order-flow imbalance, the staples of short-horizon research.
- **NOII / imbalance analytics** — surface the net order imbalance indicator data
  the feed already carries in a usable form.
- **Auction reconstruction** — opening, closing, halt, and IPO cross price
  discovery from the cross and NOII messages.

---

## Phase 4 — Research & interoperability

_Goal: meet quants where they live. This is what "everywhere in finance" means._

Most quantitative research happens in Python and columnar data tools. A
C++-only library, however fast, is invisible to that workflow until it has
bindings and standard export formats.

- **Python bindings (pybind11)** — first-class, zero-friction access from Python,
  exposing parsing, the book engine, and analytics. The single highest-leverage
  feature for adoption.
  For this , we already have a pure python library [itch](https://github.com/bbalouki/itch) so we want this to become a pure binding of the current library. But since it already has been published on pypi , we don't know the best action to take you decide.
  Next is the itch message documentation, the current c++ version doe not docoment each message, use the pytohn version to read each message documenation and write it on the C++ using doxygen style (///) and (///<)
- **Apache Arrow / Parquet export** — turn a feed into columnar tables that drop
  straight into pandas, Polars, DuckDB, and Spark pipelines.
- **CSV and streaming sinks** — simple, universal output for quick inspection and
  legacy tooling.
- **`itch-tool` CLI** — a batteries-included command-line tool to inspect, filter,
  convert (e.g. ITCH to Parquet), and produce message-type statistics/histograms
  without writing any code.

---

## Phase 5 — Simulation & ecosystem

_Goal: close the loop for backtesting and become a maintained, depended-on package._

- **Replay engine with timestamp pacing** — drive consumers at original or scaled
  wall-clock speed for realistic backtesting and system simulation.
- **ITCH encoder / writer** — synthesize valid ITCH streams for testing, scenario
  generation, and golden fixtures (also unblocks much of the testing work in
  [IMPROVEMENTS.md](IMPROVEMENTS.md) 6).
- **Multi-venue and multi-version support** — generalize via policy templates to
  NASDAQ BX/PSX, older ITCH 4.1, and other ITCH-like venue feeds, so one codebase
  covers a firm's whole equity data estate.
- **Packaging and project maturity** — publish to vcpkg and Conan, define a
  versioned ABI/compatibility policy, and maintain a `CHANGELOG.md` and
  `CONTRIBUTING.md` so the project is safe to depend on.

---

## Non-goals

To stay focused and best-in-class at what it does, ITCHCPP deliberately will not
become:

- An order-management or execution-management system (OMS/EMS).
- An exchange matching engine.
- A strategy/alpha research framework or backtester proper (it _feeds_ those; it
  is not one).

Staying a superb data layer — ingest, reconstruct, analyze, export — is the way to
be used everywhere rather than competing with the systems built on top of it.
