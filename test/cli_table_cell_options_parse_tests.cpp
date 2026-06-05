#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "featherdoc_cli_table_cell_options_parse.hpp"

#include <string>
#include <string_view>
#include <vector>

TEST_CASE("cli table cell options parse accepts text and border options") {
    featherdoc_cli::table_cell_text_options text_options;
    std::string text_error;
    CHECK(featherdoc_cli::parse_table_cell_text_options(
        {"table-cell-text", "input.docx", "--text", "hello", "--grid-column",
         "2", "--json"},
        2U, text_options, text_error));
    REQUIRE(text_options.text.has_value());
    CHECK(*text_options.text == "hello");
    REQUIRE(text_options.grid_column.has_value());
    CHECK(*text_options.grid_column == 2U);
    CHECK(text_options.json_output);

    featherdoc_cli::table_cell_border_options border_options;
    std::string border_error;
    CHECK(featherdoc_cli::parse_table_cell_border_options(
        {"table-cell-border", "input.docx", "--style", "double_line",
         "--size", "12", "--color", "FF0000", "--space", "1",
         "--output", "out.docx", "--json"},
        2U, border_options, border_error));
    REQUIRE(border_options.style.has_value());
    CHECK(*border_options.style == featherdoc::border_style::double_line);
    REQUIRE(border_options.size_eighth_points.has_value());
    CHECK(*border_options.size_eighth_points == 12U);
    REQUIRE(border_options.color.has_value());
    CHECK(*border_options.color == "FF0000");
    REQUIRE(border_options.space_points.has_value());
    CHECK(*border_options.space_points == 1U);
    REQUIRE(border_options.output_path.has_value());
    CHECK(border_options.output_path->filename().string() == "out.docx");
    CHECK(border_options.json_output);
}

TEST_CASE("cli table cell options parse accepts merge options") {
    featherdoc_cli::merge_table_cells_options merge_options;
    std::string merge_error;
    CHECK(featherdoc_cli::parse_merge_table_cells_options(
        {"merge-table-cells", "input.docx", "--direction", "right",
         "--count", "3", "--output", "merged.docx", "--json"},
        2U, merge_options, merge_error));
    CHECK(merge_options.direction == featherdoc_cli::table_merge_direction::right);
    REQUIRE(merge_options.has_direction);
    REQUIRE(merge_options.has_count);
    CHECK(merge_options.count == 3U);
    REQUIRE(merge_options.output_path.has_value());
    CHECK(merge_options.output_path->filename().string() == "merged.docx");
    CHECK(merge_options.json_output);

    featherdoc_cli::unmerge_table_cells_options unmerge_options;
    std::string unmerge_error;
    CHECK(featherdoc_cli::parse_unmerge_table_cells_options(
        {"unmerge-table-cells", "input.docx", "--direction", "down",
         "--json"},
        2U, unmerge_options, unmerge_error));
    CHECK(unmerge_options.direction ==
          featherdoc_cli::table_merge_direction::down);
    REQUIRE(unmerge_options.has_direction);
    CHECK(unmerge_options.json_output);
}

TEST_CASE("cli table cell options parse reports validation errors") {
    featherdoc_cli::table_cell_text_options text_options;
    std::string text_error;
    CHECK_FALSE(featherdoc_cli::parse_table_cell_text_options(
        {"table-cell-text", "input.docx", "--bogus"}, 2U, text_options,
        text_error));
    CHECK(text_error == "unknown option: --bogus");

    featherdoc_cli::merge_table_cells_options merge_options;
    std::string merge_error;
    CHECK_FALSE(featherdoc_cli::parse_merge_table_cells_options(
        {"merge-table-cells", "input.docx", "--direction", "right",
         "--count", "0"},
        2U, merge_options, merge_error));
    CHECK(merge_error == "merge count must be greater than zero");

    featherdoc_cli::table_cell_border_options border_options;
    std::string border_error;
    CHECK_FALSE(featherdoc_cli::parse_table_cell_border_options(
        {"table-cell-border", "input.docx", "--style", "bogus"}, 2U,
        border_options, border_error));
    CHECK(border_error == "invalid border style: bogus");
}
