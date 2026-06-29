#include <array>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <print>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "itch/io/csv_sink.hpp"
#include "itch/parser.hpp"
#include "itch/transport/pcap.hpp"

namespace {

// Reads an entire file into a byte buffer.
auto read_file(const std::string& path) -> std::vector<std::byte> {
    std::ifstream file {path, std::ios::binary};
    if (!file) {
        return {};
    }
    std::vector<char>      raw {std::istreambuf_iterator<char> {file}, {}};
    std::vector<std::byte> bytes(raw.size());
    if (!raw.empty()) {
        std::memcpy(bytes.data(), raw.data(), raw.size());
    }
    return bytes;
}

// Drives a callback over every message in a file, auto-detecting whether the
// input is a pcap/pcapng capture or a raw length-prefixed ITCH stream.
auto for_each_message(std::span<const std::byte> data, const itch::MessageCallback& callback)
    -> void {
    itch::transport::PcapReader reader {callback};
    if (reader.read(data)) {
        return;  // The input was a capture file.
    }
    itch::Parser parser;
    parser.parse(data, callback);
}

// Parses a comma-or-concatenated list of message-type characters (e.g. "AEP").
auto parse_type_filter(std::string_view spec) -> std::array<bool, 256> {
    std::array<bool, 256> wanted {};
    for (char chr : spec) {
        if (chr != ',') {
            wanted[static_cast<unsigned char>(chr)] = true;
        }
    }
    return wanted;
}

auto message_type_of(const itch::Message& message) -> char {
    return std::visit([](const auto& msg) { return msg.message_type; }, message);
}

auto cmd_stats(std::span<const std::byte> data) -> int {
    std::array<std::uint64_t, 256> counts {};
    std::uint64_t                  total = 0;
    for_each_message(data, [&](const itch::Message& msg) {
        ++counts[static_cast<unsigned char>(message_type_of(msg))];
        ++total;
    });
    std::println("{:<6} {:>14}", "type", "count");
    for (std::size_t type = 0; type < counts.size(); ++type) {
        if (counts[type] > 0) {
            std::println("{:<6} {:>14}", static_cast<char>(type), counts[type]);
        }
    }
    std::println("{:<6} {:>14}", "TOTAL", total);
    return 0;
}

auto cmd_inspect(std::span<const std::byte> data, std::uint64_t limit) -> int {
    std::uint64_t shown = 0;
    for_each_message(data, [&](const itch::Message& msg) {
        if (shown < limit) {
            std::cout << msg << '\n';  // Message provides operator<<.
            ++shown;
        }
    });
    std::println("(showed {} messages)", shown);
    return 0;
}

auto cmd_filter_or_convert(
    std::span<const std::byte>   data,
    const std::array<bool, 256>& wanted,
    bool                         has_filter,
    const std::string&           out_path
) -> int {
    std::ofstream file_out;
    if (!out_path.empty()) {
        file_out.open(out_path, std::ios::binary);
        if (!file_out) {
            std::println(stderr, "Error: cannot open output '{}'.", out_path);
            return 1;
        }
    }
    std::ostream&     out = out_path.empty() ? std::cout : file_out;
    itch::io::CsvSink sink {out};
    for_each_message(data, [&](const itch::Message& msg) {
        if (!has_filter || wanted[static_cast<unsigned char>(message_type_of(msg))]) {
            sink.write(msg);
        }
    });
    sink.flush();
    std::println(stderr, "Wrote {} rows.", sink.rows_written());
    return 0;
}

auto usage(const char* program) -> int {
    std::println(stderr, "itch-tool - inspect, filter, and convert NASDAQ ITCH 5.0 data");
    std::println(stderr, "Usage:");
    std::println(stderr, "  {} stats   <file>", program);
    std::println(stderr, "  {} inspect <file> [--limit N]", program);
    std::println(stderr, "  {} filter  <file> --types <ABC> [--out <file.csv>]", program);
    std::println(stderr, "  {} convert <file> [--to csv] [--out <file.csv>]", program);
    std::println(stderr, "Input may be a raw ITCH stream or a .pcap/.pcapng capture.");
    return 1;
}

}  // namespace

auto main(int argc, char* argv[]) -> int {
    const std::vector<std::string> args {argv, argv + argc};
    if (args.size() < 3) {
        return usage(argv[0]);
    }
    const std::string& command = args[1];
    const std::string& path    = args[2];

    std::string           out_path;
    std::string           types;
    std::uint64_t         limit = 20;
    std::array<bool, 256> wanted {};
    bool                  has_filter = false;
    for (std::size_t index = 3; index < args.size(); ++index) {
        if (args[index] == "--out" && index + 1 < args.size()) {
            out_path = args[++index];
        } else if (args[index] == "--types" && index + 1 < args.size()) {
            types      = args[++index];
            wanted     = parse_type_filter(types);
            has_filter = true;
        } else if (args[index] == "--limit" && index + 1 < args.size()) {
            limit = std::stoull(args[++index]);
        } else if (args[index] == "--to" && index + 1 < args.size()) {
            ++index;  // Only csv is supported without the Arrow build option.
        }
    }

    const std::vector<std::byte> data = read_file(path);
    if (data.empty()) {
        std::println(stderr, "Error: cannot read '{}' (missing or empty).", path);
        return 1;
    }
    const std::span<const std::byte> view {data};

    if (command == "stats") {
        return cmd_stats(view);
    }
    if (command == "inspect") {
        return cmd_inspect(view, limit);
    }
    if (command == "filter") {
        return cmd_filter_or_convert(view, wanted, has_filter, out_path);
    }
    if (command == "convert") {
        return cmd_filter_or_convert(view, wanted, has_filter, out_path);
    }
    return usage(argv[0]);
}
