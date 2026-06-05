#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "featherdoc_cli_inspect_numbering_options_parse.hpp"

#include <string>
#include <string_view>
#include <vector>

TEST_CASE("cli inspect numbering options parse accepts definition and json") {
    featherdoc_cli::inspect_numbering_options options;
    std::string error;

    CHECK(featherdoc_cli::parse_inspect_numbering_options(
        {"inspect-numbering", "input.docx", "--definition", "42", "--json"},
        2U, options, error));

    REQUIRE(options.definition_id.has_value());
    CHECK(*options.definition_id == 42U);
    CHECK_FALSE(options.instance_id.has_value());
    CHECK(options.json_output);
}

TEST_CASE("cli inspect numbering options parse accepts instance") {
    featherdoc_cli::inspect_numbering_options options;
    std::string error;

    CHECK(featherdoc_cli::parse_inspect_numbering_options(
        {"inspect-numbering", "input.docx", "--instance", "7"}, 2U, options,
        error));

    CHECK_FALSE(options.definition_id.has_value());
    REQUIRE(options.instance_id.has_value());
    CHECK(*options.instance_id == 7U);
    CHECK_FALSE(options.json_output);
}

TEST_CASE("cli inspect numbering options parse preserves combined filters behavior") {
    featherdoc_cli::inspect_numbering_options options;
    std::string error;

    CHECK(featherdoc_cli::parse_inspect_numbering_options(
        {"inspect-numbering", "input.docx", "--definition", "1", "--instance",
         "2"},
        2U, options, error));

    REQUIRE(options.definition_id.has_value());
    CHECK(*options.definition_id == 1U);
    REQUIRE(options.instance_id.has_value());
    CHECK(*options.instance_id == 2U);
}

TEST_CASE("cli inspect numbering options parse validates definition errors") {
    featherdoc_cli::inspect_numbering_options duplicate;
    std::string duplicate_error;
    CHECK_FALSE(featherdoc_cli::parse_inspect_numbering_options(
        {"inspect-numbering", "input.docx", "--definition", "1",
         "--definition", "2"},
        2U, duplicate, duplicate_error));
    CHECK(duplicate_error == "duplicate --definition option");

    featherdoc_cli::inspect_numbering_options missing;
    std::string missing_error;
    CHECK_FALSE(featherdoc_cli::parse_inspect_numbering_options(
        {"inspect-numbering", "input.docx", "--definition"}, 2U, missing,
        missing_error));
    CHECK(missing_error == "missing value after --definition");

    featherdoc_cli::inspect_numbering_options invalid;
    std::string invalid_error;
    CHECK_FALSE(featherdoc_cli::parse_inspect_numbering_options(
        {"inspect-numbering", "input.docx", "--definition", "abc"}, 2U,
        invalid, invalid_error));
    CHECK(invalid_error == "invalid numbering definition id: abc");
}

TEST_CASE("cli inspect numbering options parse validates instance and unknown errors") {
    featherdoc_cli::inspect_numbering_options duplicate;
    std::string duplicate_error;
    CHECK_FALSE(featherdoc_cli::parse_inspect_numbering_options(
        {"inspect-numbering", "input.docx", "--instance", "1", "--instance",
         "2"},
        2U, duplicate, duplicate_error));
    CHECK(duplicate_error == "duplicate --instance option");

    featherdoc_cli::inspect_numbering_options missing;
    std::string missing_error;
    CHECK_FALSE(featherdoc_cli::parse_inspect_numbering_options(
        {"inspect-numbering", "input.docx", "--instance"}, 2U, missing,
        missing_error));
    CHECK(missing_error == "missing value after --instance");

    featherdoc_cli::inspect_numbering_options invalid;
    std::string invalid_error;
    CHECK_FALSE(featherdoc_cli::parse_inspect_numbering_options(
        {"inspect-numbering", "input.docx", "--instance", "abc"}, 2U, invalid,
        invalid_error));
    CHECK(invalid_error == "invalid numbering instance id: abc");

    featherdoc_cli::inspect_numbering_options unknown;
    std::string unknown_error;
    CHECK_FALSE(featherdoc_cli::parse_inspect_numbering_options(
        {"inspect-numbering", "input.docx", "--bogus"}, 2U, unknown,
        unknown_error));
    CHECK(unknown_error == "unknown option: --bogus");
}
