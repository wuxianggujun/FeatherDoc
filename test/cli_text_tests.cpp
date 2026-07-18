#include <filesystem>
#include <new>
#include <optional>
#include <stdexcept>
#include <string>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "featherdoc_cli_argv.hpp"
#include "featherdoc_cli_text.hpp"

#include <featherdoc/detail/path.hpp>

TEST_CASE("cli text helpers quote command arguments predictably") {
    CHECK(featherdoc_cli::quote_cli_argument("") == "\"\"");
    CHECK(featherdoc_cli::quote_cli_argument("plain") == "plain");
    CHECK(featherdoc_cli::quote_cli_argument("two words") == "\"two words\"");
    CHECK(featherdoc_cli::quote_cli_argument("say \"hi\"") ==
          "\"say \\\"hi\\\"\"");
    CHECK(featherdoc_cli::quote_cli_argument("owner's") == "\"owner's\"");
}

TEST_CASE("cli text helpers format booleans and display placeholders") {
    CHECK(std::string{featherdoc_cli::yes_no(true)} == "yes");
    CHECK(std::string{featherdoc_cli::yes_no(false)} == "no");
    CHECK(std::string{featherdoc_cli::json_bool(true)} == "true");
    CHECK(std::string{featherdoc_cli::json_bool(false)} == "false");

    CHECK(featherdoc_cli::optional_display_value(
              std::optional<std::string>{"visible"}) == "visible");
    CHECK(featherdoc_cli::optional_display_value(
              std::optional<std::string>{}) == "-");
    CHECK(featherdoc_cli::optional_size_display_value(
              std::optional<std::size_t>{42U}) == "42");
    CHECK(featherdoc_cli::optional_size_display_value(
              std::optional<std::size_t>{}) == "-");
}

TEST_CASE("cli text helpers escape paragraph text and strip UTF-8 BOM") {
    CHECK(featherdoc_cli::format_paragraph_text("") == "<empty>");
    CHECK(featherdoc_cli::format_paragraph_text("a\nb\rc\td") ==
          "a\\nb\\rc\\td");
    CHECK(featherdoc_cli::strip_utf8_bom("\xEF\xBB\xBFtext") == "text");
    CHECK(featherdoc_cli::strip_utf8_bom("text") == "text");
}

TEST_CASE("cli text helpers handle ASCII case and Word path filters") {
    CHECK(featherdoc_cli::lower_ascii_copy("Report.DOCX") == "report.docx");
    CHECK(featherdoc_cli::is_docx_path(std::filesystem::path{"Report.DOCX"}));
    CHECK_FALSE(
        featherdoc_cli::is_docx_path(std::filesystem::path{"Report.txt"}));
    CHECK(featherdoc_cli::is_word_temporary_path(
        std::filesystem::path{"~$draft.docx"}));
    CHECK_FALSE(featherdoc_cli::is_word_temporary_path(
        std::filesystem::path{"draft.docx"}));
}

TEST_CASE("cli paths decode UTF-8 independently of the Windows code page") {
    CHECK(featherdoc_cli::path_from_cli_utf8({}).empty());

    const auto encoded = std::string{"目录/中文-日本語-🙂 文档.docx"};
    const auto path = featherdoc_cli::path_from_cli_utf8(encoded);
    CHECK_EQ(featherdoc::detail::path_to_utf8(path), encoded);
}

TEST_CASE("cli path parsing rejects controls before constructing an OS path") {
    std::filesystem::path parsed_path;
    std::string error_message;

    const auto unicode_path = std::string{"目录/输入-日本語-🙂 文档.docx"};
    REQUIRE(featherdoc_cli::parse_cli_path_utf8(unicode_path, "input_path",
                                                parsed_path, error_message));
    CHECK_EQ(featherdoc::detail::path_to_utf8(parsed_path), unicode_path);
    CHECK(error_message.empty());

    const auto embedded_nul = std::string{"safe.docx"} + '\0' + "ignored.docx";
    CHECK_FALSE(featherdoc_cli::parse_cli_path_utf8(
        embedded_nul, "input_path", parsed_path, error_message));
    CHECK_NE(error_message.find("control characters"), std::string::npos);

    const auto escape_control = std::string{"safe"} + '\x1B' + ".docx";
    CHECK_FALSE(featherdoc_cli::parse_cli_path_utf8(
        escape_control, "input_path", parsed_path, error_message));
    CHECK_NE(error_message.find("control characters"), std::string::npos);

    const auto c1_control =
        std::string{"safe"} + std::string{"\xC2\x80", 2U} + ".docx";
    CHECK_FALSE(featherdoc_cli::parse_cli_path_utf8(
        c1_control, "input_path", parsed_path, error_message));
    CHECK_NE(error_message.find("control characters"), std::string::npos);
}

TEST_CASE("CLI argv validation accepts empty and Unicode UTF-8 arguments") {
    std::string executable{"featherdoc_cli"};
    std::string empty;
    std::string unicode{"目录/中文-日本語-🙂 文档.docx"};
    char *arguments[]{executable.data(), empty.data(), unicode.data()};

    CHECK(featherdoc_cli::cli_arguments_are_valid_utf8(3, arguments));
}

TEST_CASE("CLI argv validation rejects invalid UTF-8 and invalid arrays") {
    std::string executable{"featherdoc_cli"};
    std::string invalid{"\xF0\x28\x8C\x28", 4U};
    char *invalid_arguments[]{executable.data(), invalid.data()};
    CHECK_FALSE(
        featherdoc_cli::cli_arguments_are_valid_utf8(2, invalid_arguments));

    char *null_argument[]{nullptr};
    CHECK_FALSE(featherdoc_cli::cli_arguments_are_valid_utf8(1, nullptr));
    CHECK_FALSE(featherdoc_cli::cli_arguments_are_valid_utf8(1, null_argument));
    CHECK_FALSE(featherdoc_cli::cli_arguments_are_valid_utf8(0, nullptr));
    CHECK_FALSE(featherdoc_cli::cli_arguments_are_valid_utf8(-1, nullptr));
}

TEST_CASE("CLI top-level exception boundary converts failures to exit codes") {
    CHECK_EQ(featherdoc_cli::run_with_cli_exception_boundary(
                 []() -> int { return 7; }),
             7);
    CHECK_EQ(featherdoc_cli::run_with_cli_exception_boundary([]() -> int {
                 throw std::bad_alloc{};
             }),
             1);
    CHECK_EQ(featherdoc_cli::run_with_cli_exception_boundary([]() -> int {
                 throw std::runtime_error{"test failure"};
             }),
             1);
}

#ifdef _WIN32
TEST_CASE("Windows argv conversion distinguishes empty and invalid UTF-16") {
    const auto empty = featherdoc_cli::wide_argument_to_utf8(L"");
    REQUIRE(empty.has_value());
    CHECK(empty->empty());

    const wchar_t isolated_high_surrogate[]{static_cast<wchar_t>(0xD800),
                                            L'\0'};
    CHECK_FALSE(featherdoc_cli::wide_argument_to_utf8(isolated_high_surrogate)
                    .has_value());

    const wchar_t isolated_low_surrogate[]{static_cast<wchar_t>(0xDC00), L'\0'};
    CHECK_FALSE(featherdoc_cli::wide_argument_to_utf8(isolated_low_surrogate)
                    .has_value());
}

TEST_CASE("Windows argv conversion preserves Unicode text") {
    const auto converted = featherdoc_cli::wide_argument_to_utf8(
        L"目录/中文-日本語-\U0001F642 文档.docx");
    REQUIRE(converted.has_value());
    CHECK_EQ(*converted, "目录/中文-日本語-🙂 文档.docx");
}
#endif
