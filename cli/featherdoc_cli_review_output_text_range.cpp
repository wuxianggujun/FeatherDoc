#include "featherdoc_cli_review_output.hpp"

#include "featherdoc_cli_json.hpp"

#include <iostream>
#include <ostream>
#include <string_view>
#include <vector>

namespace featherdoc_cli {
namespace {

void write_json_text_range_preview_segment(
    std::ostream &stream,
    const featherdoc::text_range_preview_segment &segment) {
    stream << "{\"paragraph_index\":" << segment.paragraph_index
           << ",\"text_offset\":" << segment.text_offset
           << ",\"text_length\":" << segment.text_length << ",\"text\":";
    write_json_string(stream, segment.text);
    stream << '}';
}

} // namespace

void write_json_text_range_preview(
    std::ostream &stream, const featherdoc::text_range_preview &preview) {
    stream << "{\"start_paragraph_index\":" << preview.start_paragraph_index
           << ",\"start_text_offset\":" << preview.start_text_offset
           << ",\"end_paragraph_index\":" << preview.end_paragraph_index
           << ",\"end_text_offset\":" << preview.end_text_offset
           << ",\"text_length\":" << preview.text_length
           << ",\"plain_text_runs_supported\":"
           << json_bool(preview.plain_text_runs_supported) << ",\"text\":";
    write_json_string(stream, preview.text);
    stream << ",\"segments\":[";
    for (std::size_t index = 0U; index < preview.segments.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        write_json_text_range_preview_segment(stream, preview.segments[index]);
    }
    stream << "]}";
}

void write_json_text_range_matches(
    std::ostream &stream, std::string_view command, std::string_view query,
    const std::vector<featherdoc::text_range_preview> &matches) {
    stream << "{\"command\":";
    write_json_string(stream, command);
    stream << ",\"ok\":true,\"query\":";
    write_json_string(stream, query);
    stream << ",\"matches_count\":" << matches.size() << ",\"matches\":[";
    for (std::size_t index = 0U; index < matches.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        stream << "{\"index\":" << index << ",\"preview\":";
        write_json_text_range_preview(stream, matches[index]);
        stream << '}';
    }
    stream << "]}\n";
}

void print_text_range_preview(std::ostream &stream,
                              const featherdoc::text_range_preview &preview) {
    stream << "start_paragraph_index=" << preview.start_paragraph_index
           << " start_text_offset=" << preview.start_text_offset
           << " end_paragraph_index=" << preview.end_paragraph_index
           << " end_text_offset=" << preview.end_text_offset
           << " text_length=" << preview.text_length
           << " plain_text_runs_supported="
           << yes_no(preview.plain_text_runs_supported) << " text=";
    write_json_string(stream, preview.text);
    for (const auto &segment : preview.segments) {
        stream << '\n'
               << "segment paragraph_index=" << segment.paragraph_index
               << " text_offset=" << segment.text_offset
               << " text_length=" << segment.text_length << " text=";
        write_json_string(stream, segment.text);
    }
}

void print_text_range_matches(
    std::ostream &stream, std::string_view query,
    const std::vector<featherdoc::text_range_preview> &matches) {
    stream << "query=";
    write_json_string(stream, query);
    stream << " matches_count=" << matches.size();
    for (std::size_t index = 0U; index < matches.size(); ++index) {
        stream << '\n' << "match index=" << index << ' ';
        print_text_range_preview(stream, matches[index]);
    }
    stream << '\n';
}

auto report_expected_revision_text_mismatch(
    std::string_view command, std::string_view expected_text,
    const featherdoc::text_range_preview &preview, bool json_output) -> bool {
    constexpr std::string_view message =
        "expected text did not match selected text";
    if (json_output) {
        std::cerr << "{\"command\":";
        write_json_string(std::cerr, command);
        std::cerr << ",\"ok\":false,\"stage\":\"validate\",\"message\":";
        write_json_string(std::cerr, message);
        std::cerr << ",\"expected_text\":";
        write_json_string(std::cerr, expected_text);
        std::cerr << ",\"actual_text\":";
        write_json_string(std::cerr, preview.text);
        std::cerr << ",\"preview\":";
        write_json_text_range_preview(std::cerr, preview);
        std::cerr << "}\n";
        return false;
    }

    std::cerr << message << '\n' << "expected_text=";
    write_json_string(std::cerr, expected_text);
    std::cerr << '\n' << "actual_text=";
    write_json_string(std::cerr, preview.text);
    std::cerr << '\n' << "selected_range=";
    print_text_range_preview(std::cerr, preview);
    std::cerr << '\n';
    return false;
}

} // namespace featherdoc_cli
