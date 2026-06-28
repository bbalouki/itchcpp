# Modern C++ Best Practices

## Language & Compiler

- Target C++17 minimum; prefer C++20/23. Set `CMAKE_CXX_STANDARD` explicitly; disable extensions (`-pedantic`).
- Enable `-Wall -Wextra -Wconversion -Wsign-conversion` and treat warnings as errors (`-Werror`).

## Style & Naming

- `m_` prefix for private members; `snake_case` for variables/functions; `PascalCase` for types; `SCREAMING_SNAKE_CASE` for macros and Constants.
- All symbols (variable, parameters, functions) must be at least 3 characters long.
- Alwayse Use `std::print`/`std::println`/`std::format` for output.
- No magic literals, replace with named `constexpr` values. Declare variables close to first use.
- Keep functions short and flat; remove unused variables, parameters, and includes.
- Use `#pragma once`; keep headers to declarations and inline/template definitions only.
- Do not use mdash (`--`) in code, comments, or documentation.

## Variables & Types

- Always initialize variables at declaration; prefer brace initialization `T x{value}` to prevent narrowing.
- Use `auto` when type is obvious; `const` by default; `constexpr` for compile-time constants.
- Avoid `using namespace std` in headers. Use `enum class`; use `std::string_view` for read-only strings.
- Never mix signed/unsigned arithmetic; never use C-style casts, use `static_cast`, `dynamic_cast`, etc.
- Never use raw pointers for ownership; use references or smart pointers instead. Avoid `void*` and `reinterpret_cast`.
- Never use `goto` or `longjmp`; prefer structured control flow and RAII for cleanup.
- Never use C-style arrays or C-style strings; prefer `std::array`, `std::vector`, and `std::string` for safety and convenience.

## Memory Management

- Follow RAII: tie every resource (memory, file, mutex) to an object's lifetime.
- Use `std::unique_ptr`/`std::shared_ptr`/`std::weak_ptr` via `make_unique`/`make_shared`, never `new`/`delete` directly.
- Follow Rule of Zero (all RAII members) or Rule of Five (when owning a raw resource).
- Detect leaks with AddressSanitizer; use standard containers over raw arrays.

## Functions & Classes

- One logical operation per function; always use trailing return types.
- Pass non-trivial types as `const T&`; return by value (rely on NRVO/RVO).
- Mark `[[nodiscard]]` where return values must not be ignored; mark `noexcept` when applicable.
- Single-argument constructors must be `explicit`; keep members `private`; use `override`/`final` on overrides.
- Prefer composition over inheritance; declare base destructors `virtual` (or `protected` non-virtual).
- Do not call virtual functions from constructors or destructors.

## Error Handling

- Use exceptions for errors (not for normal control flow); catch by `const` reference to avoid slicing.
- Maintain a consistent exception-safety guarantee: basic, strong, or no-throw.
- Use `static_assert` for compile-time invariants; `assert()` for debug-only preconditions, never on user input.
- Consider `std::expected<T, E>` (C++23) for predictable, performance-sensitive failure paths.

## Templates & Concurrency

- Constrain templates with C++20 concepts; use `if constexpr` over SFINAE/`enable_if`.
- Use variadic templates and fold expressions instead of recursive specializations.
- Protect shared mutable state with `std::mutex`; prefer `std::jthread` over `std::thread`.
- Use `std::atomic<T>` for lock-free single-variable operations; never `volatile` for inter-thread communication.
- Minimize lock duration; avoid callbacks or virtual calls while holding a lock; prefer `std::async` and parallel algorithms.

## Performance & Build

- Profile before optimizing; prefer `std::vector` for cache-friendly access; use `reserve()` when size is known.
- Use move semantics to avoid deep copies; prefer standard algorithms (`std::sort`, `std::transform`) over hand-written loops.
- Use CMake (≥ 3.25) with target-centric paradigm: `target_include_directories`, `target_compile_options`, `target_link_libraries`.
- Use `FetchContent` or vcpkg/Conan for dependencies; use out-of-source builds and CMake Presets.
- Enable `CMAKE_EXPORT_COMPILE_COMMANDS=ON` for tooling; use `ccache`/`sccache` to speed up rebuilds.

## Testing & Tooling

- Use GoogleTest/Catch2 with CTest; write tests before or alongside implementation.
- Run tests under AddressSanitizer and UndefinedBehaviourSanitizer; use fuzz testing for parsers and untrusted input.
- Enforce clang-format in CI; use clang-tidy with `cppcoreguidelines-*`, `modernize-*`, `readability-*` checks.
- Use debuggers (GDB, LLDB) actively rather than `std::cout`; use `compile_commands.json` for accurate tooling.
- Use real implementations; only mock external dependencies (LLM APIs, cloud services).
- Test public interface behavior, not implementation details, keeps tests resilient to refactoring.
- Tests must be fast, isolated, and have descriptive names; cover edge cases and error conditions.
- Location: `tests/unittests/` following source structure.

## CI/CD & Maintenance

## Versioning & Breaking Changes

- Follow Semantic Versioning 2.0.0 (`MAJOR.MINOR.PATCH`).
- A breaking change is any backward-incompatible modification to the public API, including data schemas, CLI, and server communication formats.
- CI must build and test before merging; build across GCC, Clang, and MSVC.
- Document the _why_, not the _what_; use Doxygen (`///`) for API docs and `//` for implementation notes.
- Keep public API surface minimal; mark deprecated APIs with `[[deprecated("reason")]]` and include a migration path.
- Follow the Boy Scout Rule; track technical debt explicitly; maintain a `CHANGELOG.md` and `CONTRIBUTING.md`.

---
