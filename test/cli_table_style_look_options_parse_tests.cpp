#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "featherdoc_cli_table_style_look_options_parse.hpp"

#include <string>
#include <string_view>
#include <vector>

TEST_CASE("cli table style look options parse accepts check options") {
    featherdoc_cli::check_table_style_look_options options;
    std::string error;

    CHECK(featherdoc_cli::parse_check_table_style_look_options(
        {"check-table-style-look", "input.docx", "--fail-on-issue", "--json"},
        2U, options, error));

    CHECK(options.fail_on_issue);
    CHECK(options.json_output);
}

TEST_CASE("cli table style look options parse validates check unknown option") {
    featherdoc_cli::check_table_style_look_options options;
    std::string error;

    CHECK_FALSE(featherdoc_cli::parse_check_table_style_look_options(
        {"check-table-style-look", "input.docx", "--bogus"}, 2U, options,
        error));

    CHECK(error == "unknown option: --bogus");
}

TEST_CASE("cli table style look options parse defaults repair to plan only") {
    featherdoc_cli::repair_table_style_look_options options;
    std::string error;

    CHECK(featherdoc_cli::parse_repair_table_style_look_options(
        {"repair-table-style-look", "input.docx", "--json"}, 2U, options,
        error));

    CHECK(options.plan_only);
    CHECK_FALSE(options.apply);
    CHECK_FALSE(options.output_path.has_value());
    CHECK(options.json_output);
}

TEST_CASE("cli table style look options parse accepts repair apply output") {
    featherdoc_cli::repair_table_style_look_options options;
    std::string error;

    CHECK(featherdoc_cli::parse_repair_table_style_look_options(
        {"repair-table-style-look", "input.docx", "--apply", "--output",
         "out.docx", "--json"},
        2U, options, error));

    CHECK_FALSE(options.plan_only);
    CHECK(options.apply);
    REQUIRE(options.output_path.has_value());
    CHECK(options.output_path->filename().string() == "out.docx");
    CHECK(options.json_output);
}

TEST_CASE("cli table style look options parse validates repair mode constraints") {
    featherdoc_cli::repair_table_style_look_options conflicting_mode;
    std::string conflicting_mode_error;
    CHECK_FALSE(featherdoc_cli::parse_repair_table_style_look_options(
        {"repair-table-style-look", "input.docx", "--plan-only", "--apply"},
        2U, conflicting_mode, conflicting_mode_error));
    CHECK(conflicting_mode_error == "--plan-only and --apply are mutually exclusive");

    featherdoc_cli::repair_table_style_look_options output_without_apply;
    std::string output_without_apply_error;
    CHECK_FALSE(featherdoc_cli::parse_repair_table_style_look_options(
        {"repair-table-style-look", "input.docx", "--output", "out.docx"}, 2U,
        output_without_apply, output_without_apply_error));
    CHECK(output_without_apply_error == "--output requires --apply");

    featherdoc_cli::repair_table_style_look_options missing_output;
    std::string missing_output_error;
    CHECK_FALSE(featherdoc_cli::parse_repair_table_style_look_options(
        {"repair-table-style-look", "input.docx", "--apply"}, 2U,
        missing_output, missing_output_error));
    CHECK(missing_output_error ==
          "repair-table-style-look --apply requires --output <path>");
}

TEST_CASE("cli table style look options parse validates repair option errors") {
    featherdoc_cli::repair_table_style_look_options duplicate_output;
    std::string duplicate_output_error;
    CHECK_FALSE(featherdoc_cli::parse_repair_table_style_look_options(
        {"repair-table-style-look", "input.docx", "--apply", "--output",
         "a.docx", "--output", "b.docx"},
        2U, duplicate_output, duplicate_output_error));
    CHECK(duplicate_output_error == "duplicate --output option");

    featherdoc_cli::repair_table_style_look_options missing_path;
    std::string missing_path_error;
    CHECK_FALSE(featherdoc_cli::parse_repair_table_style_look_options(
        {"repair-table-style-look", "input.docx", "--output"}, 2U,
        missing_path, missing_path_error));
    CHECK(missing_path_error == "missing path after --output");

    featherdoc_cli::repair_table_style_look_options unknown;
    std::string unknown_error;
    CHECK_FALSE(featherdoc_cli::parse_repair_table_style_look_options(
        {"repair-table-style-look", "input.docx", "--bogus"}, 2U, unknown,
        unknown_error));
    CHECK(unknown_error == "unknown option: --bogus");
}

TEST_CASE("cli table style look options parse accepts look flags") {
    featherdoc_cli::table_style_look_options options;
    std::string error;

    CHECK(featherdoc_cli::parse_table_style_look_options(
        {"table-style-look", "input.docx", "--first-row", "true",
         "--last-row", "false", "--first-column", "true", "--last-column",
         "false", "--banded-rows", "true", "--banded-columns", "false",
         "--output", "out.docx", "--json"},
        2U, options, error));

    CHECK(featherdoc_cli::table_style_look_options_have_flag(options));
    REQUIRE(options.first_row.has_value());
    REQUIRE(options.last_row.has_value());
    REQUIRE(options.first_column.has_value());
    REQUIRE(options.last_column.has_value());
    REQUIRE(options.banded_rows.has_value());
    REQUIRE(options.banded_columns.has_value());
    CHECK(*options.first_row);
    CHECK_FALSE(*options.last_row);
    CHECK(*options.first_column);
    CHECK_FALSE(*options.last_column);
    CHECK(*options.banded_rows);
    CHECK_FALSE(*options.banded_columns);
    REQUIRE(options.output_path.has_value());
    CHECK(options.output_path->filename().string() == "out.docx");
    CHECK(options.json_output);
}

TEST_CASE("cli table style look options parse reports validation errors") {
    featherdoc_cli::table_style_look_options options;
    std::string error;

    CHECK_FALSE(featherdoc_cli::parse_table_style_look_options(
        {"table-style-look", "input.docx", "--first-row", "maybe"}, 2U,
        options, error));
    CHECK(error == "invalid --first-row value: maybe");
}
