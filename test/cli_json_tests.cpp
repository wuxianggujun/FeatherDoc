#include <sstream>
#include <string>
#include <vector>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "featherdoc_cli_json.hpp"

TEST_CASE("cli json_escape preserves plain text") {
    CHECK_EQ(featherdoc_cli::json_escape("plain text 123"), "plain text 123");
}

TEST_CASE("cli json_escape escapes JSON string control characters") {
    const std::string input = "quote\" slash\\ back\b form\f line\nreturn\r tab\t";
    CHECK_EQ(featherdoc_cli::json_escape(input),
             "quote\\\" slash\\\\ back\\b form\\f line\\nreturn\\r tab\\t");
}

TEST_CASE("cli write_json_string wraps escaped text in quotes") {
    std::ostringstream stream;
    featherdoc_cli::write_json_string(stream, "a\"b\nc");
    CHECK_EQ(stream.str(), "\"a\\\"b\\nc\"");
}

TEST_CASE("cli write_json_size_array writes compact numeric arrays") {
    std::ostringstream stream;
    featherdoc_cli::write_json_size_array(stream, {1U, 2U, 3U});
    CHECK_EQ(stream.str(), "[1,2,3]");
}

TEST_CASE("cli write_json_strings and lines escape string arrays") {
    std::ostringstream strings;
    featherdoc_cli::write_json_strings(strings, {"alpha", "b\"c"});
    CHECK_EQ(strings.str(), "[\"alpha\",\"b\\\"c\"]");

    std::ostringstream lines;
    featherdoc_cli::write_json_lines(lines, {"first", "second"});
    CHECK_EQ(lines.str(), "[\"first\",\"second\"]");
}

TEST_CASE("cli optional JSON writers preserve null semantics") {
    std::ostringstream stream;
    featherdoc_cli::write_json_optional_string(stream, std::string{"value"});
    stream << ',';
    featherdoc_cli::write_json_optional_string(stream,
                                               std::optional<std::string>{});
    stream << ',';
    featherdoc_cli::write_json_optional_u32(stream, std::uint32_t{7U});
    stream << ',';
    featherdoc_cli::write_json_optional_double(stream, 2.5);
    stream << ',';
    featherdoc_cli::write_json_optional_bool(stream, true);
    stream << ',';
    featherdoc_cli::write_json_optional_size(stream, std::size_t{9U});
    stream << ',';
    featherdoc_cli::write_json_optional_u32_value(
        stream, std::optional<std::uint32_t>{});
    stream << ',';
    featherdoc_cli::write_json_optional_bool_value(
        stream, std::optional<bool>{});

    CHECK_EQ(stream.str(), "\"value\",null,7,2.5,true,9,null,null");
}
