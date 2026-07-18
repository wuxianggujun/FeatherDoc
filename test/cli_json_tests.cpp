#include <sstream>
#include <string>
#include <vector>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include <featherdoc/detail/utf8.hpp>

#include "featherdoc_cli_json.hpp"

TEST_CASE("cli json_escape preserves plain text") {
    CHECK_EQ(featherdoc_cli::json_escape("plain text 123"), "plain text 123");
}

TEST_CASE("cli json_escape escapes JSON string control characters") {
    const std::string input = "quote\" slash\\ back\b form\f line\nreturn\r tab\t";
    CHECK_EQ(featherdoc_cli::json_escape(input),
             "quote\\\" slash\\\\ back\\b form\\f line\\nreturn\\r tab\\t");
}

TEST_CASE("core UTF-8 helper rejects malformed byte sequences") {
    CHECK(featherdoc::detail::is_valid_utf8("plain"));
    CHECK(featherdoc::detail::is_valid_utf8(
        std::string{reinterpret_cast<const char *>(u8"中文-日本語-🙂")}));

    CHECK_FALSE(featherdoc::detail::is_valid_utf8(std::string{"\xC0\xAF", 2U}));
    CHECK_FALSE(featherdoc::detail::is_valid_utf8(std::string{"\xE0\x80\x80", 3U}));
    CHECK_FALSE(featherdoc::detail::is_valid_utf8(std::string{"\xED\xA0\x80", 3U}));
    CHECK_FALSE(featherdoc::detail::is_valid_utf8(std::string{"\xF4\x90\x80\x80", 4U}));
    CHECK_FALSE(featherdoc::detail::is_valid_utf8(std::string{"\xF0\x9F", 2U}));
    CHECK_FALSE(featherdoc::detail::is_valid_utf8(std::string{"\x80", 1U}));
}

TEST_CASE("core UTF-8 sanitizer preserves valid text and escapes unsafe bytes") {
    CHECK_EQ(featherdoc::detail::sanitize_utf8_for_diagnostic("中文\n"),
             std::string{reinterpret_cast<const char *>(u8"中文")} + "\\x0A");

    const std::string invalid = std::string{"word/", 5U} +
                                std::string{"\xFF", 1U} +
                                std::string{"/\x01.xml", 6U};
    CHECK_EQ(featherdoc::detail::sanitize_utf8_for_diagnostic(invalid),
             "word/\\xFF/\\x01.xml");
}

TEST_CASE("cli json_escape emits legal JSON for all ASCII controls") {
    std::string input;
    for (unsigned char value = 0U; value < 0x20U; ++value) {
        input.push_back(static_cast<char>(value));
    }

    CHECK_EQ(featherdoc_cli::json_escape(input),
             "\\u0000\\u0001\\u0002\\u0003\\u0004\\u0005\\u0006\\u0007"
             "\\b\\t\\n\\u000B\\f\\r\\u000E\\u000F"
             "\\u0010\\u0011\\u0012\\u0013\\u0014\\u0015\\u0016\\u0017"
             "\\u0018\\u0019\\u001A\\u001B\\u001C\\u001D\\u001E\\u001F");
}

TEST_CASE("cli json_escape sanitizes malformed UTF-8 as JSON-safe ASCII") {
    const std::string invalid = std::string{"entry/", 6U} +
                                std::string{"\xF0\x28\x8C\x28", 4U};
    CHECK_EQ(featherdoc_cli::json_escape(invalid), "entry/\\\\xF0(\\\\x8C(");

    std::ostringstream stream;
    featherdoc_cli::write_json_string(stream, invalid);
    CHECK_EQ(stream.str(), "\"entry/\\\\xF0(\\\\x8C(\"");
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
