#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "featherdoc_cli_template_table_mutation_options_parse.hpp"

#include <string>
#include <string_view>
#include <vector>

TEST_CASE("cli template table mutation options parse accepts core options") {
    featherdoc_cli::template_table_cell_text_options cell_text_options;
    std::string cell_text_error;
    CHECK(featherdoc_cli::parse_template_table_cell_text_options(
        {"--text", "hello", "--grid-column", "3", "--output", "out.docx",
         "--json"},
        0U, cell_text_options, cell_text_error));
    REQUIRE(cell_text_options.text.has_value());
    CHECK(*cell_text_options.text == "hello");
    REQUIRE(cell_text_options.grid_column.has_value());
    CHECK(*cell_text_options.grid_column == 3U);
    REQUIRE(cell_text_options.output_path.has_value());
    CHECK(cell_text_options.output_path->filename().string() == "out.docx");
    CHECK(cell_text_options.json_output);

    featherdoc_cli::template_table_cell_mutation_options cell_mutation_options;
    std::string cell_mutation_error;
    CHECK(featherdoc_cli::parse_template_table_cell_mutation_options(
        {"--output", "cell.docx", "--json"}, 0U, cell_mutation_options,
        cell_mutation_error));
    REQUIRE(cell_mutation_options.output_path.has_value());
    CHECK(cell_mutation_options.output_path->filename().string() == "cell.docx");
    CHECK(cell_mutation_options.json_output);

    featherdoc_cli::template_append_table_row_options append_options;
    std::string append_error;
    CHECK(featherdoc_cli::parse_template_append_table_row_options(
        {"--cell-count", "4", "--output", "rows.docx"}, 0U, append_options,
        append_error));
    REQUIRE(append_options.cell_count.has_value());
    CHECK(*append_options.cell_count == 4U);
    REQUIRE(append_options.output_path.has_value());
    CHECK(append_options.output_path->filename().string() == "rows.docx");

    featherdoc_cli::template_merge_table_cells_options merge_options;
    std::string merge_error;
    CHECK(featherdoc_cli::parse_template_merge_table_cells_options(
        {"--direction", "down", "--count", "2", "--json"}, 0U, merge_options,
        merge_error));
    CHECK(merge_options.direction == featherdoc_cli::table_merge_direction::down);
    REQUIRE(merge_options.has_direction);
    REQUIRE(merge_options.has_count);
    CHECK(merge_options.count == 2U);
    CHECK(merge_options.json_output);

    featherdoc_cli::template_unmerge_table_cells_options unmerge_options;
    std::string unmerge_error;
    CHECK(featherdoc_cli::parse_template_unmerge_table_cells_options(
        {"--direction", "right", "--output", "unmerge.docx"}, 0U,
        unmerge_options, unmerge_error));
    CHECK(unmerge_options.direction ==
          featherdoc_cli::table_merge_direction::right);
    REQUIRE(unmerge_options.has_direction);
    REQUIRE(unmerge_options.output_path.has_value());
    CHECK(unmerge_options.output_path->filename().string() == "unmerge.docx");

    featherdoc_cli::template_table_row_texts_options row_texts_options;
    std::string row_texts_error;
    CHECK(featherdoc_cli::parse_template_table_row_texts_options(
        {"--bookmark", "template-target", "--row", "alpha", "--cell", "beta",
         "--row", "gamma", "--cell", "delta", "--output", "rows.docx",
         "--json"},
        0U, row_texts_options, row_texts_error));
    REQUIRE(row_texts_options.bookmark_name.has_value());
    CHECK(*row_texts_options.bookmark_name == "template-target");
    REQUIRE(row_texts_options.rows.size() == 2U);
    REQUIRE(row_texts_options.rows[0].size() == 2U);
    REQUIRE(row_texts_options.rows[1].size() == 2U);
    CHECK(row_texts_options.rows[0][0] == "alpha");
    CHECK(row_texts_options.rows[0][1] == "beta");
    CHECK(row_texts_options.rows[1][0] == "gamma");
    CHECK(row_texts_options.rows[1][1] == "delta");
    REQUIRE(row_texts_options.output_path.has_value());
    CHECK(row_texts_options.output_path->filename().string() == "rows.docx");
    CHECK(row_texts_options.json_output);
}

TEST_CASE("cli template table mutation options parse reports validation errors") {
    featherdoc_cli::template_table_cell_text_options cell_text_options;
    std::string cell_text_error;
    CHECK_FALSE(featherdoc_cli::parse_template_table_cell_text_options(
        {"--grid-column", "2"}, 0U, cell_text_options, cell_text_error));
    CHECK(cell_text_error == "expected --text <text> or --text-file <path>");

    featherdoc_cli::template_append_table_row_options append_options;
    std::string append_error;
    CHECK_FALSE(featherdoc_cli::parse_template_append_table_row_options(
        {"--cell-count", "0"}, 0U, append_options, append_error));
    CHECK(append_error == "cell count must be greater than 0");

    featherdoc_cli::template_merge_table_cells_options merge_options;
    std::string merge_error;
    CHECK_FALSE(featherdoc_cli::parse_template_merge_table_cells_options(
        {"--direction", "right", "--count", "0"}, 0U, merge_options,
        merge_error));
    CHECK(merge_error == "merge count must be greater than zero");

    featherdoc_cli::template_unmerge_table_cells_options unmerge_options;
    std::string unmerge_error;
    CHECK_FALSE(featherdoc_cli::parse_template_unmerge_table_cells_options(
        {"--direction", "sideways"}, 0U, unmerge_options, unmerge_error));
    CHECK(unmerge_error == "invalid merge direction: sideways");

    featherdoc_cli::template_table_row_texts_options row_texts_options;
    std::string row_texts_error;
    CHECK_FALSE(featherdoc_cli::parse_template_table_row_texts_options(
        {"--bookmark", "template-target", "--cell", "orphan"}, 0U,
        row_texts_options, row_texts_error));
    CHECK(row_texts_error == "--cell requires --row");
}
