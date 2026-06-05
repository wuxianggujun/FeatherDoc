#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "featherdoc_cli_style_options_parse.hpp"

#include <string>
#include <string_view>
#include <vector>

TEST_CASE("cli style options parse accepts inspect styles options") {
    featherdoc_cli::inspect_styles_options options;
    std::string error;

    CHECK(featherdoc_cli::parse_inspect_styles_options(
        {"inspect-styles", "input.docx", "--style", "Normal", "--usage",
         "--json"},
        2U, options, error));

    REQUIRE(options.style_id.has_value());
    CHECK(*options.style_id == "Normal");
    CHECK(options.usage_output);
    CHECK(options.json_output);
}

TEST_CASE("cli style options parse validates inspect styles errors") {
    featherdoc_cli::inspect_styles_options duplicate_style;
    std::string duplicate_style_error;
    CHECK_FALSE(featherdoc_cli::parse_inspect_styles_options(
        {"inspect-styles", "input.docx", "--style", "Normal", "--style",
         "Heading1"},
        2U, duplicate_style, duplicate_style_error));
    CHECK(duplicate_style_error == "duplicate --style option");

    featherdoc_cli::inspect_styles_options missing_style;
    std::string missing_style_error;
    CHECK_FALSE(featherdoc_cli::parse_inspect_styles_options(
        {"inspect-styles", "input.docx", "--style"}, 2U, missing_style,
        missing_style_error));
    CHECK(missing_style_error == "missing value after --style");

    featherdoc_cli::inspect_styles_options unknown;
    std::string unknown_error;
    CHECK_FALSE(featherdoc_cli::parse_inspect_styles_options(
        {"inspect-styles", "input.docx", "--bogus"}, 2U, unknown,
        unknown_error));
    CHECK(unknown_error == "unknown option: --bogus");
}

TEST_CASE("cli style options parse accepts style inheritance options") {
    featherdoc_cli::inspect_style_inheritance_options options;
    std::string error;

    CHECK(featherdoc_cli::parse_inspect_style_inheritance_options(
        {"inspect-style-inheritance", "input.docx", "Normal", "--json"}, 3U,
        options, error));
    CHECK(options.json_output);

    featherdoc_cli::inspect_style_inheritance_options unknown;
    std::string unknown_error;
    CHECK_FALSE(featherdoc_cli::parse_inspect_style_inheritance_options(
        {"inspect-style-inheritance", "input.docx", "Normal", "--bogus"},
        3U, unknown, unknown_error));
    CHECK(unknown_error == "unknown option: --bogus");
}

TEST_CASE("cli style options parse accepts rename style options") {
    featherdoc_cli::rename_style_options options;
    std::string error;

    CHECK(featherdoc_cli::parse_rename_style_options(
        {"rename-style", "input.docx", "OldStyle", "NewStyle", "--output",
         "out.docx", "--json"},
        4U, options, error));

    REQUIRE(options.output_path.has_value());
    CHECK(options.output_path->filename().string() == "out.docx");
    CHECK(options.json_output);
}

TEST_CASE("cli style options parse validates rename style errors") {
    featherdoc_cli::rename_style_options duplicate_output;
    std::string duplicate_output_error;
    CHECK_FALSE(featherdoc_cli::parse_rename_style_options(
        {"rename-style", "input.docx", "OldStyle", "NewStyle", "--output",
         "a.docx", "--output", "b.docx"},
        4U, duplicate_output, duplicate_output_error));
    CHECK(duplicate_output_error == "duplicate --output option");

    featherdoc_cli::rename_style_options missing_output;
    std::string missing_output_error;
    CHECK_FALSE(featherdoc_cli::parse_rename_style_options(
        {"rename-style", "input.docx", "OldStyle", "NewStyle", "--output"},
        4U, missing_output, missing_output_error));
    CHECK(missing_output_error == "missing path after --output");

    featherdoc_cli::rename_style_options unknown;
    std::string unknown_error;
    CHECK_FALSE(featherdoc_cli::parse_rename_style_options(
        {"merge-style", "input.docx", "OldStyle", "NewStyle", "--bogus"},
        4U, unknown, unknown_error));
    CHECK(unknown_error == "unknown option: --bogus");
}
