# IMPROVEMENTS

This document is a prioritized audit of the current ITCHCPP codebase. It exists
to chart the path from a capable single-protocol file parser to infrastructure
that can credibly claim to be "one of the best in quantitative finance" and be
"usable everywhere in finance."

Every finding below is tied to a specific location in the current source so it
can be acted on directly. Items are tagged by priority:

- **P0** — correctness, safety, or a claim/reality gap that undermines trust. Fix first.
- **P1** — meaningful quality, performance, or process gaps. Fix soon.
- **P2** — polish and consistency. Fix opportunistically (Boy Scout Rule).

New capabilities (transport, analytics, bindings, etc.) live in
[FEATURES.md](FEATURES.md). This file is about making what already exists correct,
fast, honest, and maintainable.

---

## 1. Correctness & specification compliance

### 1.1 Per-message length is never validated against the expected message size — P0

**Problem.** The framing loop reads the 2-byte length prefix, checks only that the
declared payload fits in the buffer, then dispatches on the type byte
([src/parser.cpp:291-322](src/parser.cpp)). The per-type `unpack_*` functions read
a fixed number of bytes based on the struct layout, never cross-checked against the
declared `length`. A message whose `length` is shorter than its type implies (a
corrupt or maliciously crafted frame) still dispatches, and the unpacker reads
fields belonging to the _next_ frame.

**Impact.** Silent misparsing of adjacent data with no error raised. In a finance
context this is the worst failure mode: wrong numbers that look plausible. It is
also a robustness hole for untrusted input.

**Fix.** Maintain a compile-time table of expected payload size per message type
and validate `length` against it before unpacking. On mismatch, route to the error
policy (see 1.2) rather than proceeding. This pairs naturally with the dispatch
rewrite in 2.1.

### 1.2 The library prints to `std::cerr` on unknown message types — P0

**Problem.** When an unknown type byte is encountered the parser writes to
`std::cerr` ([src/parser.cpp:318](src/parser.cpp)).

**Impact.** A library must never write to a global stream on a caller's behalf: it
corrupts the host application's output, is not thread-safe, and cannot be silenced.
It also violates the project's own house rule favoring `std::print`/`std::format`
over stream I/O.

**Fix.** Replace with an explicit error policy: an optional error callback, an
accumulating diagnostics counter, or a `std::expected` return on the typed-parse
path. Default behavior should be silent-skip-and-count, configurable by the caller.

### 1.3 `swap_bytes` relies on union type-punning — P0

**Problem.** Byte swapping is implemented by writing one union member and reading
another ([include/itch/parser.hpp:150-161](include/itch/parser.hpp)). Reading an
inactive union member is undefined behavior in C++ (it is only well-defined in C).

**Impact.** Works today by compiler grace, but it is UB on the parser hot path —
exactly the code that must be bulletproof. Aggressive optimizers are within their
rights to miscompile it.

**Fix.** Use `std::byteswap` (C++23, already a target standard) with a `std::endian`
check; fall back to `std::bit_cast` or compiler intrinsics
(`__builtin_bswap*` / `_byteswap_*`) for C++20.

---

## 2. Performance — the headline gap versus the README

The README promises "multi-gigabyte-per-second" throughput and "zero-overhead
deserialization." The implementation does not currently match that framing. These
items close the gap, or correct the claims.

### 2.1 Dispatch uses `std::map<char, std::function<...>>` — P0

**Problem.** Message dispatch is a `std::map<char, std::function<Message(const char*)>>`
([include/itch/parser.hpp:133-134](include/itch/parser.hpp),
[src/parser.cpp:250-322](src/parser.cpp)). Every single message pays for a
red-black-tree lookup (pointer chasing, O(log n)) _plus_ a `std::function`
type-erased call (indirect call, possible heap, no inlining).

**Impact.** This is the single biggest divergence from the "gigabytes/second,
zero-overhead" promise. On a feed of hundreds of millions of messages per day this
dominates the cost.

**Fix.** Dispatch on the type byte through a 256-entry flat function-pointer table
or a `switch`. Type bytes are dense ASCII, so a flat array is cache-friendly and
branch-predictable, and lets the compiler inline each unpacker. Combine with 1.1's
size table so dispatch and validation share one lookup.

### 2.2 The "zero-copy / zero-overhead deserialization" claim is inaccurate — P0

**Problem.** The README's architecture section states the parser reads messages
"without performing heavy data conversions" and copies bytes "skipping the usual
per-field decoding steps" ([README.md:83-85](README.md)). The actual code does
field-by-field `memcpy` plus per-field byte swap into separate host-order structs
([src/parser.cpp:69-263](src/parser.cpp)). That is per-field decoding, not
zero-copy.

**Impact.** Overstated claims are a credibility risk with an infrastructure
audience that will read the source.

**Fix.** Either (a) implement a genuine zero-copy overlay API that returns typed
views over the raw buffer with lazy, on-access endianness conversion, or
(b) correct the wording to describe what it does: a fast, allocation-free,
single-pass field decoder. Option (a) is the stronger product; see
[FEATURES.md](FEATURES.md).

### 2.3 The order book is allocation-heavy — P1

**Problem.** Each resting order is a `std::shared_ptr<Order>` stored in a
`std::list`, with price levels held in `std::map`
([include/itch/order_book.hpp:25-61](include/itch/order_book.hpp),
[src/order_book.cpp](src/order_book.cpp)). That is an atomic refcount and a heap
allocation per order, plus a node allocation per list entry and per map node.

**Impact.** This directly contradicts the zero-allocation positioning and makes the
book the bottleneck for any real reconstruction workload (book rebuilds touch every
add/cancel/execute/replace message).

**Fix.** Move to an object pool / arena for orders, intrusive FIFO lists at each
price level, and flat (vector-backed) price ladders with O(1) order lookup by
reference number. Tracked in detail under the Phase 2 book engine in
[FEATURES.md](FEATURES.md).

### 2.4 No published, reproducible benchmark numbers — P1

**Problem.** A Google Benchmark suite exists
([benchmarks/parser_bench.cpp](benchmarks/parser_bench.cpp)) but the README quotes
no concrete results, only aspirational ranges.

**Impact.** "High performance" is unverifiable, and there is no regression signal
when 2.1–2.3 land.

**Fix.** Publish a results table (hardware, compiler, dataset, msgs/sec, GB/s) and
wire the benchmark into CI to catch regressions.

---

## 3. API modernization

### 3.1 Replace `(const char*, size_t)` with `std::span<const std::byte>` — P1

**Problem.** The buffer APIs take raw pointer/size pairs
([include/itch/parser.hpp:61-91](include/itch/parser.hpp)).

**Fix.** Accept `std::span<const std::byte>`. It carries the size, prevents
pointer/length desync, and is the modern idiom the project's standards call for.
Keep the pointer/size overloads as thin shims for C interop.

### 3.2 Offer a non-throwing `std::expected` parse path — P1

**Problem.** Errors are reported only via exceptions (`std::runtime_error` on
truncation, [src/parser.cpp:296-309](src/parser.cpp)).

**Impact.** Exceptions on the hot path are awkward for latency-sensitive callers,
and the project's standards explicitly suggest `std::expected` for predictable
failure paths.

**Fix.** Provide a `std::expected<T, ParseError>`-based entry point alongside the
throwing wrappers.

### 3.3 Introduce a typed fixed-point `Price` — P1

**Problem.** Prices are exposed as raw `uint32_t` and the caller is expected to
divide by `PRICE_DIVISOR` (10000) themselves
([include/itch/messages.hpp:327-329](include/itch/messages.hpp)), with the further
trap that MWCB decline levels use 8 decimals, not 4.

**Impact.** Easy to misuse; mixing the two scales silently produces wrong prices.

**Fix.** Add a strong `Price` type encoding scale, with explicit conversions to
`double`/decimal. Make the 4-vs-8 decimal distinction unrepresentable as a bug.

### 3.4 Provide timestamp-to-wall-clock helpers — P1

**Problem.** Timestamps are exposed as raw nanoseconds-past-midnight `uint64_t`
with no utilities to combine them with a session date.

**Fix.** Add helpers that convert to `std::chrono` time points given a session
date, plus formatting helpers.

---

## 4. Adherence to the project's own C++ standards

The repository ships a detailed [CLAUDE.md](CLAUDE.md) of C++ standards. Several
are violated by the current code.

### 4.1 Symbols shorter than three characters — P2

**Problem.** The standard requires every symbol be at least three characters, yet
the code uses `val`, `src`, `dst`, `arr`, `sv`, single-letter loop indices `i`,
and template parameter `T` (e.g.
[include/itch/parser.hpp:150-161](include/itch/parser.hpp),
[include/itch/indicators.hpp:9](include/itch/indicators.hpp)).

**Fix.** Rename to descriptive identifiers during the relevant refactors.

### 4.2 Stream I/O instead of `std::print`/`std::format` — P2

**Problem.** The standard mandates `std::print`/`std::println`/`std::format`, but
output uses `std::cout`/`std::cerr` and iostream manipulators
([src/order_book.cpp:29-68](src/order_book.cpp), [src/parser.cpp:318](src/parser.cpp)).

**Fix.** Migrate output paths to `std::format`/`std::print` (overlaps with 1.2).

### 4.3 `std::map` globals defined in a header — P2

**Problem.** [include/itch/indicators.hpp](include/itch/indicators.hpp) defines
several non-trivially-constructed `std::map` objects at namespace scope in a header.
Each translation unit that includes it pays dynamic-initialization cost, and the
maps risk duplication.

**Fix.** Replace with `constexpr` frozen lookups (sorted `std::array` of pairs with
binary search, or a compile-time map type) or functions returning `string_view`,
keeping the header dependency-light.

---

## 5. Build, CI, and repository hygiene

### 5.1 Large binary artifacts committed to the repository — P1

**Problem.** The repo tracks a full uncompressed NASDAQ ITCH sample
(`01302020.NASDAQ_ITCH50`), its `.gz` counterpart, and a generated coverage report
(`docs/coverage.html`).

**Impact.** Bloated clone size, slow history, and a generated artifact under source
control. The data files in particular are large binaries that do not belong in git.

**Fix.** Remove from history (or migrate to Git LFS / an external download script),
add them to `.gitignore`, and generate coverage into the build tree only.

### 5.2 Inconsistent minimum CMake version — P2

**Problem.** The required CMake version disagrees across the project: README says
3.20, [cmake/Helpers.cmake:1](cmake/Helpers.cmake) says 3.23, and
[CLAUDE.md](CLAUDE.md) asks for >= 3.25.

**Fix.** Standardize on a single minimum (>= 3.25 enables the presets workflow the
standards call for) and align every `cmake_minimum_required` and the docs.

### 5.3 CI does not run on pull requests, and skips its own quality gates — P1

**Problem.** The workflow triggers only on `push` to `main`
([.github/workflows/build.yml:3-5](.github/workflows/build.yml)). It builds and
tests, but never runs clang-tidy, a clang-format check, sanitizers, or coverage —
even though [cmake/Helpers.cmake](cmake/Helpers.cmake) provides all of them.

**Impact.** Regressions and style/lint violations can merge unreviewed by CI;
PRs get no signal at all.

**Fix.** Add a `pull_request` trigger and dedicated jobs for clang-format check,
clang-tidy, an ASAN/UBSAN test run, and coverage reporting.

### 5.4 No fuzz target for the parser — P1

**Problem.** The project standards mandate fuzz testing for parsers on untrusted
input, but no fuzz target exists.

**Impact.** The framing/dispatch path (especially before 1.1 lands) is the textbook
candidate for crashes and overreads on malformed input.

**Fix.** Add a libFuzzer/AFL++ target that feeds arbitrary bytes through `parse`,
and run it in CI on a time budget.

### 5.5 Typos in public CMake helper names — P2

**Problem.** Helper functions are misspelled: `anable_address_sanitizer` and
`build_documenation` ([cmake/Helpers.cmake:136,206](cmake/Helpers.cmake)).

**Impact.** These are part of the build's public surface; renaming later is a
breaking change for anyone who depends on them.

**Fix.** Rename to `enable_address_sanitizer` / `build_documentation` now, while the
blast radius is small.

---

## 6. Testing

### 6.1 Add golden / conformance tests against the official sample — P1

**Problem.** There is no end-to-end conformance check that a known input produces
known output.

**Fix.** Parse a committed-or-fetched official sample and assert message counts per
type and spot-checked field values against a golden fixture.

### 6.2 Broaden edge-case coverage — P1

**Problem.** Current tests should be extended to cover the failure and boundary
modes that matter for a parser and a book.

**Fix.** Add cases for truncated buffers, zero-length frames, unknown type bytes,
endianness round-trips, and the full order-book lifecycle (add, execute,
execute-with-price, cancel, delete, replace, and crossed-book scenarios).

---

## Priority summary

| ID  | Item                                              | Priority | Area             |
| --- | ------------------------------------------------- | -------- | ---------------- |
| 1.1 | Validate message length vs. expected size         | P0       | Correctness      |
| 1.2 | Remove `std::cerr` from the library               | P0       | Correctness      |
| 1.3 | Replace union type-punning byte swap              | P0       | Correctness (UB) |
| 2.1 | Replace `std::map` + `std::function` dispatch     | P0       | Performance      |
| 2.2 | Fix or implement the zero-copy claim              | P0       | Honesty          |
| 2.3 | Allocation-free order book internals              | P1       | Performance      |
| 2.4 | Publish reproducible benchmarks                   | P1       | Performance      |
| 3.1 | `std::span` buffer API                            | P1       | API              |
| 3.2 | `std::expected` parse path                        | P1       | API              |
| 3.3 | Typed fixed-point `Price`                         | P1       | API              |
| 3.4 | Timestamp-to-wall-clock helpers                   | P1       | API              |
| 5.1 | Remove committed large binaries / coverage report | P1       | Repo hygiene     |
| 5.3 | CI on PRs + lint/sanitizer/coverage gates         | P1       | CI               |
| 5.4 | Add a parser fuzz target                          | P1       | Testing          |
| 6.1 | Golden / conformance tests                        | P1       | Testing          |
| 6.2 | Edge-case coverage                                | P1       | Testing          |
| 4.1 | Symbol length >= 3                                | P2       | Standards        |
| 4.2 | `std::print`/`std::format` output                 | P2       | Standards        |
| 4.3 | Frozen/`constexpr` indicator lookups              | P2       | Standards        |
| 5.2 | Single CMake minimum version                      | P2       | Build            |
| 5.5 | Fix CMake helper name typos                       | P2       | Build            |
