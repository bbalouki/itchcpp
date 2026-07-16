# Security Policy

## Supported Versions

ITCHCPP follows [Semantic Versioning](https://semver.org). Security fixes are
made against the latest `1.x` minor release. Older minor versions are not
patched separately.

| Version | Supported          |
| ------- | ------------------ |
| 1.6.x   | :white_check_mark: |
| < 1.6   | :x:                |

## Reporting a Vulnerability

ITCHCPP decodes untrusted, attacker-influenceable binary input (ITCH frames,
MoldUDP64/SoupBinTCP transport framing, and `.pcap`/`.pcapng` captures), so
parser and transport bugs (buffer overflows, out-of-bounds reads, integer
overflow, panics/aborts on malformed input) are treated as security issues,
not just correctness bugs.

**Please do not open a public GitHub issue for security vulnerabilities.**

Instead, report privately using
[GitHub Security Advisories](https://github.com/bbalouki/itchcpp/security/advisories/new)
for this repository. Include:

- A description of the vulnerability and its impact.
- Steps to reproduce, ideally a minimal input file or byte sequence (attach
  under the advisory, not in a public issue or PR).
- The affected version (`VERSION.txt` / package version) or commit SHA.
- Whether the issue was found via fuzzing (e.g. `fuzz/parser_fuzzer`,
  `fuzz/moldudp64_fuzzer`) or manual review.

You should expect an initial response within **5 business days**. We will work
with you to confirm the issue, assess severity, and coordinate a fix and
disclosure timeline before any public disclosure.

## Scope

In scope:

- The C++ parser, transport decoders, book engine, and encoder under
  `include/itch/` and `src/`.
- The Python bindings (`itchcpp` package).
- The `itch-tool` CLI.

Out of scope:

- Vulnerabilities in third-party dependencies (report upstream; note them here
  so we can track exposure).
- Denial of service via arbitrarily large, well-formed input (the parser is
  designed for high-throughput streaming, not resource capping).
