#include <cstddef>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "featherdoc_cli_json_parse.hpp"
#include "featherdoc_cli_input.hpp"

namespace {

auto temp_json_path(std::string_view suffix) -> std::filesystem::path {
    const auto stamp =
        std::chrono::steady_clock::now().time_since_epoch().count();
    return std::filesystem::temp_directory_path() /
           ("featherdoc_cli_json_parse_" + std::to_string(stamp) +
            std::string(suffix));
}

void write_binary_file(const std::filesystem::path &path,
                       std::string_view content) {
    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    REQUIRE(stream.good());
    stream.write(content.data(), static_cast<std::streamsize>(content.size()));
    REQUIRE(stream.good());
}

} // namespace

TEST_CASE("cli JSON parse decodes escapes and Unicode code points") {
    const std::string text = "\"line\\n\\u0041\\uD83D\\uDE00\" tail";
    std::size_t index = 0U;
    std::string value;
    std::string error_message;

    CHECK(featherdoc_cli::parse_json_patch_string(text, index, value,
                                                  error_message));
    CHECK_EQ(value, std::string("line\nA") + "\xF0\x9F\x98\x80");
    CHECK_EQ(index, 26U);
}

TEST_CASE("cli JSON parse reports invalid Unicode surrogate pairs") {
    const std::string text = "\"\\uD83D\"";
    std::size_t index = 0U;
    std::string value;
    std::string error_message;

    CHECK_FALSE(featherdoc_cli::parse_json_patch_string(text, index, value,
                                                        error_message));
    CHECK(error_message.find("expected low surrogate") != std::string::npos);
}

TEST_CASE("cli JSON parse captures numeric tokens") {
    const std::string text = " -12.5e+2,";
    std::size_t index = 0U;
    std::string value;
    std::string error_message;

    featherdoc_cli::skip_json_patch_whitespace(text, index);
    CHECK(featherdoc_cli::parse_json_patch_number(text, index, value,
                                                  error_message));
    CHECK_EQ(value, "-12.5e+2");
    CHECK_EQ(index, 9U);
}

TEST_CASE("cli JSON parse skips nested values") {
    const std::string text =
        "  {\"a\":[1,true,null,{\"b\":\"c\"}],\"x\":-2.5e+3} rest";
    std::size_t index = 0U;
    std::string error_message;

    CHECK(featherdoc_cli::skip_json_patch_value(text, index, error_message));
    CHECK_EQ(text.substr(index), " rest");
}

TEST_CASE("cli JSON parse accepts unsigned index members") {
    std::size_t index = 0U;
    std::size_t value = 0U;
    std::string error_message;

    CHECK(featherdoc_cli::parse_json_patch_index_value(
        "\"42\"", index, value, "row", error_message));
    CHECK_EQ(value, 42U);
    CHECK_EQ(index, 4U);
}

TEST_CASE("cli JSON parse rejects invalid index members with context") {
    std::size_t index = 0U;
    std::size_t value = 0U;
    std::string error_message;

    CHECK_FALSE(featherdoc_cli::parse_json_patch_index_value(
        "-1", index, value, "row", error_message));
    CHECK(error_message.find("row") != std::string::npos);
}

TEST_CASE("cli JSON parse resolves matching string member aliases") {
    const auto primary = std::optional<std::string>{"slot"};
    const auto alias = std::optional<std::string>{"slot"};
    auto resolved = std::optional<std::string>{};
    std::string error_message;

    CHECK(featherdoc_cli::resolve_json_patch_string_member(
        primary, alias, "bookmark", "bookmark_name", resolved, error_message));
    REQUIRE(resolved.has_value());
    CHECK_EQ(*resolved, "slot");
}

TEST_CASE("cli JSON parse rejects conflicting string member aliases") {
    const auto primary = std::optional<std::string>{"left"};
    const auto alias = std::optional<std::string>{"right"};
    auto resolved = std::optional<std::string>{};
    std::string error_message;

    CHECK_FALSE(featherdoc_cli::resolve_json_patch_string_member(
        primary, alias, "bookmark", "bookmark_name", resolved, error_message));
    CHECK(error_message.find("must match") != std::string::npos);
}

TEST_CASE("cli JSON parse resolves index member aliases") {
    auto resolved = std::optional<std::size_t>{};
    std::string error_message;

    CHECK(featherdoc_cli::resolve_json_patch_index_member(
        std::size_t{3U}, std::size_t{3U}, "index", "part_index", resolved,
        error_message));
    REQUIRE(resolved.has_value());
    CHECK(*resolved == 3U);

    CHECK_FALSE(featherdoc_cli::resolve_json_patch_index_member(
        std::size_t{3U}, std::size_t{4U}, "index", "part_index", resolved,
        error_message));
    CHECK(error_message.find("must match") != std::string::npos);
}

TEST_CASE("cli JSON parse accepts boolean member literals and strings") {
    {
        const std::string text = " true";
        std::size_t index = 0U;
        bool value = false;
        std::string error_message;

        CHECK(featherdoc_cli::parse_json_patch_bool_member_value(
            text, index, value, "required", error_message));
        CHECK(value);
        CHECK_EQ(index, text.size());
    }

    {
        const std::string text = "\"false\"";
        std::size_t index = 0U;
        bool value = true;
        std::string error_message;

        CHECK(featherdoc_cli::parse_json_patch_bool_member_value(
            text, index, value, "required", error_message));
        CHECK_FALSE(value);
        CHECK_EQ(index, text.size());
    }
}

TEST_CASE("cli JSON parse reads files and skips UTF-8 BOM") {
    const auto path = temp_json_path("_bom.json");
    write_binary_file(path, std::string("\xEF\xBB\xBF{\"ok\":true}"));

    std::string content;
    std::size_t index = 0U;
    std::string error_message;
    CHECK(featherdoc_cli::read_template_table_json_content(
        path, content, index, error_message));
    CHECK_EQ(content.substr(index), "{\"ok\":true}");

    std::filesystem::remove(path);
}

TEST_CASE("cli input accepts the 16 MiB boundary and rejects the next byte") {
    const auto exact_path = temp_json_path("_exact_limit.json");
    const auto oversized_path = temp_json_path("_over_limit.json");
    const std::string exact(featherdoc_cli::max_cli_input_bytes, 'a');
    write_binary_file(exact_path, exact);
    write_binary_file(oversized_path, exact + "b");

    std::string content;
    std::string error_message;
    CHECK(featherdoc_cli::read_bounded_utf8_file(
        exact_path, "test input", content, error_message));
    CHECK_EQ(content.size(), featherdoc_cli::max_cli_input_bytes);

    CHECK_FALSE(featherdoc_cli::read_bounded_utf8_file(
        oversized_path, "test input", content, error_message));
    CHECK(content.empty());
    CHECK(error_message.find("input limit") != std::string::npos);

    std::filesystem::remove(exact_path);
    std::filesystem::remove(oversized_path);
}

TEST_CASE("cli input rejects malformed raw UTF-8") {
    const auto path = temp_json_path("_invalid_utf8.json");
    write_binary_file(path, std::string{"\xF0\x28\x8C\x28", 4U});

    std::string content;
    std::string error_message;
    CHECK_FALSE(featherdoc_cli::read_bounded_utf8_file(
        path, "test input", content, error_message));
    CHECK(content.empty());
    CHECK(error_message.find("valid UTF-8") != std::string::npos);

    std::filesystem::remove(path);
}

TEST_CASE("cli input resets reused output and error state") {
    const auto valid_path = temp_json_path("_state_reset.json");
    const auto missing_path = temp_json_path("_state_reset_missing.json");
    write_binary_file(valid_path, "valid");
    std::filesystem::remove(missing_path);

    std::string content{"stale content"};
    std::string error_message{"stale error"};
    CHECK_FALSE(featherdoc_cli::read_bounded_utf8_file(
        missing_path, "test input", content, error_message));
    CHECK(content.empty());
    CHECK_FALSE(error_message.empty());

    std::size_t index = 99U;
    content = "stale content";
    error_message = "stale error";
    CHECK_FALSE(featherdoc_cli::read_template_table_json_content(
        missing_path, content, index, error_message));
    CHECK_EQ(index, 0U);
    CHECK(content.empty());
    CHECK_FALSE(error_message.empty());

    content = "stale content";
    error_message = "stale error";
    CHECK(featherdoc_cli::read_bounded_utf8_file(
        valid_path, "test input", content, error_message));
    CHECK_EQ(content, "valid");
    CHECK(error_message.empty());

    std::filesystem::remove(valid_path);
}

TEST_CASE("cli JSON whitespace follows the RFC 8259 four-byte set") {
    std::size_t index = 0U;
    featherdoc_cli::skip_json_patch_whitespace(" \t\n\rvalue", index);
    CHECK_EQ(index, 4U);

    index = 0U;
    featherdoc_cli::skip_json_patch_whitespace("\vvalue", index);
    CHECK_EQ(index, 0U);

    index = 0U;
    featherdoc_cli::skip_json_patch_whitespace("\fvalue", index);
    CHECK_EQ(index, 0U);
}

TEST_CASE("cli JSON skip enforces a maximum nesting depth of 128") {
    const auto nested_array = [](std::size_t depth) {
        return std::string(depth, '[') + "0" + std::string(depth, ']');
    };

    auto text = nested_array(featherdoc_cli::max_json_nesting_depth);
    std::size_t index = 0U;
    std::string error_message;
    CHECK(featherdoc_cli::skip_json_patch_value(text, index, error_message));
    CHECK_EQ(index, text.size());

    text = nested_array(featherdoc_cli::max_json_nesting_depth + 1U);
    index = 0U;
    error_message.clear();
    CHECK_FALSE(
        featherdoc_cli::skip_json_patch_value(text, index, error_message));
    CHECK(error_message.find("nesting depth") != std::string::npos);
}
