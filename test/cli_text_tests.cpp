#include <filesystem>
#include <optional>
#include <string>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "featherdoc_cli_text.hpp"

TEST_CASE("cli text helpers normalize Word part entries") {
    CHECK(featherdoc_cli::normalize_word_part_entry("document.xml") ==
          "word/document.xml");
    CHECK(featherdoc_cli::normalize_word_part_entry("word/styles.xml") ==
          "word/styles.xml");
    CHECK(featherdoc_cli::normalize_word_part_entry("/word/header1.xml") ==
          "word/header1.xml");
    CHECK(featherdoc_cli::normalize_word_part_entry("word\\footer1.xml") ==
          "word/footer1.xml");
}

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

    CHECK(featherdoc_cli::optional_display_value(std::optional<std::string>{
              "visible"}) == "visible");
    CHECK(featherdoc_cli::optional_display_value(std::optional<std::string>{}) ==
          "-");
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
