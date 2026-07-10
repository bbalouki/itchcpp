#!/usr/bin/env bash
#
# Downloads an official NASDAQ TotalView-ITCH 5.0 sample file for use by the
# benchmarks and the golden/conformance tests. The raw sample is a multi-
# gigabyte binary and is deliberately NOT committed to the repository: fetch it
# on demand instead.
#
# Usage:
#   scripts/fetch_sample.sh [destination_dir]
#
# Environment:
#   ITCH_SAMPLE_URL  Override the download URL (defaults to the NASDAQ archive).
#
# The default URL points at NASDAQ's public sample archive. NASDAQ occasionally
# changes the available dates; if the default 404s, browse the index and set
# ITCH_SAMPLE_URL to a current file:
#   https://emi.nasdaq.com/ITCH/Nasdaq%20ITCH/

set -euo pipefail

readonly DEFAULT_URL="https://emi.nasdaq.com/ITCH/Nasdaq%20ITCH/01302020.NASDAQ_ITCH50.gz"
readonly SAMPLE_URL="${ITCH_SAMPLE_URL:-${DEFAULT_URL}}"

dest_dir="${1:-data}"
mkdir -p "${dest_dir}"

archive_name="$(basename "${SAMPLE_URL}")"
archive_path="${dest_dir}/${archive_name}"
decompressed_path="${archive_path%.gz}"

if [[ -f "${decompressed_path}" ]]; then
    echo "Sample already present: ${decompressed_path}"
    exit 0
fi

echo "Downloading ITCH sample from: ${SAMPLE_URL}"
if command -v curl >/dev/null 2>&1; then
    curl -fL --retry 3 -o "${archive_path}" "${SAMPLE_URL}"
elif command -v wget >/dev/null 2>&1; then
    wget -O "${archive_path}" "${SAMPLE_URL}"
else
    echo "error: neither curl nor wget is available" >&2
    exit 1
fi

if [[ "${archive_path}" == *.gz ]]; then
    echo "Decompressing ${archive_path}"
    gzip -d -k "${archive_path}"
fi

echo "Sample ready: ${decompressed_path}"
