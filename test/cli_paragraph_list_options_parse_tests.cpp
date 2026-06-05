#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "featherdoc_cli_paragraph_list_options_parse.hpp"

#include <featherdoc.hpp>

#include <string>
#include <string_view>
#include <vector>

TEST_CASE("cli paragraph list options parse accepts list options") {
    featherdoc_cli::paragraph_list_options options;
    std::string error;

    CHECK(featherdoc_cli::parse_paragraph_list_options(
        {"set-paragraph-list", "input.docx", "0", "--kind", "decimal",
         "--level", "2", "--output", "out.docx", "--json"},
        3U, options, error));

    CHECK(options.has_kind);
    CHECK(options.kind == featherdoc::list_kind::decimal);
    REQUIRE(options.level.has_value());
    CHECK(*options.level == 2U);
    REQUIRE(options.output_path.has_value());
    CHECK(options.output_path->filename().string() == "out.docx");
    CHECK(options.json_output);
}

TEST_CASE("cli paragraph list options parse validates list errors") {
    featherdoc_cli::paragraph_list_options missing_kind;
    std::string missing_kind_error;
    CHECK_FALSE(featherdoc_cli::parse_paragraph_list_options(
        {"set-paragraph-list", "input.docx", "0", "--json"}, 3U,
        missing_kind, missing_kind_error));
    CHECK(missing_kind_error == "missing --kind <bullet|decimal>");

    featherdoc_cli::paragraph_list_options duplicate_kind;
    std::string duplicate_kind_error;
    CHECK_FALSE(featherdoc_cli::parse_paragraph_list_options(
        {"set-paragraph-list", "input.docx", "0", "--kind", "bullet",
         "--kind", "decimal"},
        3U, duplicate_kind, duplicate_kind_error));
    CHECK(duplicate_kind_error == "duplicate --kind option");

    featherdoc_cli::paragraph_list_options bad_level;
    std::string bad_level_error;
    CHECK_FALSE(featherdoc_cli::parse_paragraph_list_options(
        {"set-paragraph-list", "input.docx", "0", "--kind", "bullet",
         "--level", "x"},
        3U, bad_level, bad_level_error));
    CHECK(bad_level_error == "invalid list level: x");
}

TEST_CASE("cli paragraph list options parse accepts clear options") {
    featherdoc_cli::clear_paragraph_list_options options;
    std::string error;

    CHECK(featherdoc_cli::parse_clear_paragraph_list_options(
        {"clear-paragraph-list", "input.docx", "0", "--output", "out.docx",
         "--json"},
        3U, options, error));

    REQUIRE(options.output_path.has_value());
    CHECK(options.output_path->filename().string() == "out.docx");
    CHECK(options.json_output);
}

TEST_CASE("cli paragraph list options parse validates clear errors") {
    featherdoc_cli::clear_paragraph_list_options duplicate_output;
    std::string duplicate_output_error;
    CHECK_FALSE(featherdoc_cli::parse_clear_paragraph_list_options(
        {"clear-paragraph-list", "input.docx", "0", "--output", "a.docx",
         "--output", "b.docx"},
        3U, duplicate_output, duplicate_output_error));
    CHECK(duplicate_output_error == "duplicate --output option");

    featherdoc_cli::clear_paragraph_list_options unknown;
    std::string unknown_error;
    CHECK_FALSE(featherdoc_cli::parse_clear_paragraph_list_options(
        {"clear-paragraph-list", "input.docx", "0", "--bogus"}, 3U, unknown,
        unknown_error));
    CHECK(unknown_error == "unknown option: --bogus");
}
