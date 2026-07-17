# Contributing to ITCHCPP

Thanks for your interest in improving ITCHCPP. This guide covers how to build,
test, and submit changes, and the compatibility policy the project follows.

## Getting started

ITCHCPP targets **C++20** (and builds under C++23). You need a recent compiler
(GCC 12+, Clang 15+, or MSVC 19.3x) and CMake 3.25+. Developer dependencies
(GoogleTest, Google Benchmark, and the optional pybind11/Arrow features) are
resolved through vcpkg.

```bash
cmake -S . -B build \
  -DITCH_BUILD_TESTS=ON -DITCH_BUILD_EXAMPLES=ON -DITCH_BUILD_TOOLS=ON \
  -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake
cmake --build build
ctest --test-dir build --output-on-failure
```

Optional features are off by default and enabled with:

- `-DITCH_BUILD_BENCHMARKS=ON`, Google Benchmark suites.
- `-DITCH_BUILD_FUZZERS=ON`, libFuzzer targets (Clang only).
- `-DITCH_BUILD_TOOLS=ON`, the `itch-tool` CLI.
- `-DITCH_BUILD_PYTHON=ON`, the pybind11 Python bindings (needs the `python`
  vcpkg feature: `-DVCPKG_MANIFEST_FEATURES=python`).
- `-DITCH_WITH_ARROW=ON`, Apache Arrow / Parquet export (needs the `arrow`
  vcpkg feature: `-DVCPKG_MANIFEST_FEATURES=arrow`).
- `-DITCH_BUILD_DOCUMENTATION=ON`, the Doxygen `docs` target. Build it with
  `cmake --build build --target docs` (needs Doxygen; the modern theme is fetched
  automatically). Output is written to `docs/html`. The same documentation is
  published to GitHub Pages from `main` by `.github/workflows/docs.yml`.

## Code style

- `snake_case` functions/variables,
  `PascalCase` types, `m_` private members, `SCREAMING_SNAKE_CASE` constants,
  symbols at least three characters, trailing return types, brace initialization,
  and `std::print`/`std::format` for output.
- Run `clang-format` (the repo ships a `.clang-format`) and `clang-tidy` before
  submitting; CI enforces both.
- Document the _why_ in comments; use Doxygen `///` for public API.

## Tests

- Add tests under `tests/` mirroring the source layout, using GoogleTest.
- Cover edge and error cases, not just the happy path. Parsers and decoders that
  consume untrusted input should also be exercised by a fuzz target.
- Keep tests deterministic and fast.

## Pull requests

- CI must pass: the build/test matrix (GCC, Clang, MSVC; C++20 and C++23),
  clang-format, clang-tidy, sanitizers (ASAN/UBSAN), coverage, fuzzing, and the
  Python-binding job.
- Keep the public API surface minimal; mark deprecations with
  `[[deprecated("reason")]]` and provide a migration path.
- Update [CHANGELOG](CHANGELOG.md) under `[Unreleased]` and follow the Boy
  Scout Rule.

## Versioning and compatibility policy

ITCHCPP follows [Semantic Versioning 2.0.0](https://semver.org). A breaking change
is any backward-incompatible modification to the public API, which for this project
includes:

- the C++ headers under `include/itch/` (types, function signatures, struct
  layouts of the wire message structs),
- the message wire formats and the encoder/parser round-trip contract,
- the `itch-tool` command-line interface,
- the Python module's public API.

The build options and CMake target names (`itch::itch`) are also part of the
public surface. Within a major version, source compatibility is preserved; the
binary (ABI) is **not** guaranteed stable across minor versions, so link against a
single version. New venues/protocols are added behind the `itch::venue::VenuePolicy`
seam without breaking existing policies.
