#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "featherdoc_cli_inspect_content_options_parse.hpp"

#include <string>
#include <string_view>
#include <vector>

TEST_CASE("cli inspect content options parse accepts paragraph filters") {
    featherdoc_cli::inspect_paragraphs_options options;
    std::string error;

    CHECK(featherdoc_cli::parse_inspect_paragraphs_options(
        {"inspect-paragraphs", "input.docx", "--paragraph", "3", "--json"}, 2U,
        options, error));

    REQUIRE(options.paragraph_index.has_value());
    CHECK(*options.paragraph_index == 3U);
    CHECK(options.json_output);
}

TEST_CASE("cli inspect content options parse accepts table filters") {
    featherdoc_cli::inspect_tables_options options;
    std::string error;

    CHECK(featherdoc_cli::parse_inspect_tables_options(
        {"inspect-tables", "input.docx", "--table", "2", "--json"}, 2U,
        options, error));

    REQUIRE(options.table_index.has_value());
    CHECK(*options.table_index == 2U);
    CHECK(options.json_output);
}

TEST_CASE("cli inspect content options parse validates paragraph errors") {
    featherdoc_cli::inspect_paragraphs_options duplicate;
    std::string duplicate_error;
    CHECK_FALSE(featherdoc_cli::parse_inspect_paragraphs_options(
        {"inspect-paragraphs", "input.docx", "--paragraph", "1",
         "--paragraph", "2"},
        2U, duplicate, duplicate_error));
    CHECK(duplicate_error == "duplicate --paragraph option");

    featherdoc_cli::inspect_paragraphs_options missing;
    std::string missing_error;
    CHECK_FALSE(featherdoc_cli::parse_inspect_paragraphs_options(
        {"inspect-paragraphs", "input.docx", "--paragraph"}, 2U, missing,
        missing_error));
    CHECK(missing_error == "missing value after --paragraph");

    featherdoc_cli::inspect_paragraphs_options invalid;
    std::string invalid_error;
    CHECK_FALSE(featherdoc_cli::parse_inspect_paragraphs_options(
        {"inspect-paragraphs", "input.docx", "--paragraph", "abc"}, 2U,
        invalid, invalid_error));
    CHECK(invalid_error == "invalid paragraph index: abc");

    featherdoc_cli::inspect_paragraphs_options unknown;
    std::string unknown_error;
    CHECK_FALSE(featherdoc_cli::parse_inspect_paragraphs_options(
        {"inspect-paragraphs", "input.docx", "--bogus"}, 2U, unknown,
        unknown_error));
    CHECK(unknown_error == "unknown option: --bogus");
}

TEST_CASE("cli inspect content options parse validates table errors") {
    featherdoc_cli::inspect_tables_options duplicate;
    std::string duplicate_error;
    CHECK_FALSE(featherdoc_cli::parse_inspect_tables_options(
        {"inspect-tables", "input.docx", "--table", "1", "--table", "2"}, 2U,
        duplicate, duplicate_error));
    CHECK(duplicate_error == "duplicate --table option");

    featherdoc_cli::inspect_tables_options missing;
    std::string missing_error;
    CHECK_FALSE(featherdoc_cli::parse_inspect_tables_options(
        {"inspect-tables", "input.docx", "--table"}, 2U, missing,
        missing_error));
    CHECK(missing_error == "missing value after --table");

    featherdoc_cli::inspect_tables_options invalid;
    std::string invalid_error;
    CHECK_FALSE(featherdoc_cli::parse_inspect_tables_options(
        {"inspect-tables", "input.docx", "--table", "abc"}, 2U, invalid,
        invalid_error));
    CHECK(invalid_error == "invalid table index: abc");

    featherdoc_cli::inspect_tables_options unknown;
    std::string unknown_error;
    CHECK_FALSE(featherdoc_cli::parse_inspect_tables_options(
        {"inspect-tables", "input.docx", "--bogus"}, 2U, unknown,
        unknown_error));
    CHECK(unknown_error == "unknown option: --bogus");
}
