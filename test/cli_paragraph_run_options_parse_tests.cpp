#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "featherdoc_cli_paragraph_run_options_parse.hpp"

#include <string>
#include <string_view>
#include <vector>

TEST_CASE("cli paragraph run options parse accepts inspect run filter") {
    featherdoc_cli::inspect_runs_options options;
    std::string error;

    CHECK(featherdoc_cli::parse_inspect_runs_options(
        {"inspect-runs", "input.docx", "1", "--run", "2", "--json"}, 3U,
        options, error));

    REQUIRE(options.run_index.has_value());
    CHECK(*options.run_index == 2U);
    CHECK(options.json_output);
}

TEST_CASE("cli paragraph run options parse validates inspect run errors") {
    featherdoc_cli::inspect_runs_options duplicate;
    std::string duplicate_error;
    CHECK_FALSE(featherdoc_cli::parse_inspect_runs_options(
        {"inspect-runs", "input.docx", "1", "--run", "1", "--run", "2"}, 3U,
        duplicate, duplicate_error));
    CHECK(duplicate_error == "duplicate --run option");

    featherdoc_cli::inspect_runs_options missing;
    std::string missing_error;
    CHECK_FALSE(featherdoc_cli::parse_inspect_runs_options(
        {"inspect-runs", "input.docx", "1", "--run"}, 3U, missing,
        missing_error));
    CHECK(missing_error == "missing value after --run");

    featherdoc_cli::inspect_runs_options invalid;
    std::string invalid_error;
    CHECK_FALSE(featherdoc_cli::parse_inspect_runs_options(
        {"inspect-runs", "input.docx", "1", "--run", "abc"}, 3U, invalid,
        invalid_error));
    CHECK(invalid_error == "invalid run index: abc");

    featherdoc_cli::inspect_runs_options unknown;
    std::string unknown_error;
    CHECK_FALSE(featherdoc_cli::parse_inspect_runs_options(
        {"inspect-runs", "input.docx", "1", "--bogus"}, 3U, unknown,
        unknown_error));
    CHECK(unknown_error == "unknown option: --bogus");
}

TEST_CASE("cli paragraph run options parse accepts mutation output options") {
    featherdoc_cli::set_paragraph_style_options paragraph_set;
    std::string paragraph_set_error;
    CHECK(featherdoc_cli::parse_set_paragraph_style_options(
        {"set-paragraph-style", "input.docx", "1", "Heading1", "--output",
         "out.docx", "--json"},
        4U, paragraph_set, paragraph_set_error));
    REQUIRE(paragraph_set.output_path.has_value());
    CHECK(paragraph_set.output_path->filename().string() == "out.docx");
    CHECK(paragraph_set.json_output);

    featherdoc_cli::clear_paragraph_style_options paragraph_clear;
    std::string paragraph_clear_error;
    CHECK(featherdoc_cli::parse_clear_paragraph_style_options(
        {"clear-paragraph-style", "input.docx", "1", "--output", "out.docx",
         "--json"},
        3U, paragraph_clear, paragraph_clear_error));
    CHECK(paragraph_clear.output_path.has_value());
    CHECK(paragraph_clear.json_output);

    featherdoc_cli::set_run_style_options run_set;
    std::string run_set_error;
    CHECK(featherdoc_cli::parse_set_run_style_options(
        {"set-run-style", "input.docx", "1", "2", "Emphasis", "--output",
         "out.docx", "--json"},
        5U, run_set, run_set_error));
    CHECK(run_set.output_path.has_value());
    CHECK(run_set.json_output);

    featherdoc_cli::clear_run_style_options run_clear;
    std::string run_clear_error;
    CHECK(featherdoc_cli::parse_clear_run_style_options(
        {"clear-run-style", "input.docx", "1", "2", "--output", "out.docx",
         "--json"},
        4U, run_clear, run_clear_error));
    CHECK(run_clear.output_path.has_value());
    CHECK(run_clear.json_output);
}

TEST_CASE("cli paragraph run options parse accepts font and language output options") {
    featherdoc_cli::set_run_font_family_options font_set;
    std::string font_set_error;
    CHECK(featherdoc_cli::parse_set_run_font_family_options(
        {"set-run-font-family", "input.docx", "1", "2", "Aptos", "--output",
         "out.docx", "--json"},
        5U, font_set, font_set_error));
    CHECK(font_set.output_path.has_value());
    CHECK(font_set.json_output);

    featherdoc_cli::clear_run_font_family_options font_clear;
    std::string font_clear_error;
    CHECK(featherdoc_cli::parse_clear_run_font_family_options(
        {"clear-run-font-family", "input.docx", "1", "2", "--output",
         "out.docx", "--json"},
        4U, font_clear, font_clear_error));
    CHECK(font_clear.output_path.has_value());
    CHECK(font_clear.json_output);

    featherdoc_cli::set_run_language_options language_set;
    std::string language_set_error;
    CHECK(featherdoc_cli::parse_set_run_language_options(
        {"set-run-language", "input.docx", "1", "2", "en-US", "--output",
         "out.docx", "--json"},
        5U, language_set, language_set_error));
    CHECK(language_set.output_path.has_value());
    CHECK(language_set.json_output);

    featherdoc_cli::clear_run_language_options language_clear;
    std::string language_clear_error;
    CHECK(featherdoc_cli::parse_clear_run_language_options(
        {"clear-run-language", "input.docx", "1", "2", "--output", "out.docx",
         "--json"},
        4U, language_clear, language_clear_error));
    CHECK(language_clear.output_path.has_value());
    CHECK(language_clear.json_output);
}

TEST_CASE("cli paragraph run options parse validates shared output errors") {
    featherdoc_cli::set_paragraph_style_options duplicate;
    std::string duplicate_error;
    CHECK_FALSE(featherdoc_cli::parse_set_paragraph_style_options(
        {"set-paragraph-style", "input.docx", "1", "Heading1", "--output",
         "a.docx", "--output", "b.docx"},
        4U, duplicate, duplicate_error));
    CHECK(duplicate_error == "duplicate --output option");

    featherdoc_cli::clear_run_language_options missing;
    std::string missing_error;
    CHECK_FALSE(featherdoc_cli::parse_clear_run_language_options(
        {"clear-run-language", "input.docx", "1", "2", "--output"}, 4U,
        missing, missing_error));
    CHECK(missing_error == "missing path after --output");

    featherdoc_cli::set_run_font_family_options unknown;
    std::string unknown_error;
    CHECK_FALSE(featherdoc_cli::parse_set_run_font_family_options(
        {"set-run-font-family", "input.docx", "1", "2", "Aptos", "--bogus"},
        5U, unknown, unknown_error));
    CHECK(unknown_error == "unknown option: --bogus");
}
