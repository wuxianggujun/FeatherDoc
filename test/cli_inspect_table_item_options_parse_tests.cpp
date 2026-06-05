#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "featherdoc_cli_inspect_table_item_options_parse.hpp"

#include <string>
#include <string_view>
#include <vector>

TEST_CASE("cli inspect table item options parse accepts row selector") {
    featherdoc_cli::inspect_table_rows_options options;
    std::string error;

    CHECK(featherdoc_cli::parse_inspect_table_rows_options(
        {"inspect-table-rows", "input.docx", "0", "--row", "4", "--json"}, 3U,
        options, error));

    REQUIRE(options.row_index.has_value());
    CHECK(*options.row_index == 4U);
    CHECK(options.json_output);
}

TEST_CASE("cli inspect table item options parse accepts cell selectors") {
    featherdoc_cli::inspect_table_cells_options cell_options;
    std::string cell_error;
    CHECK(featherdoc_cli::parse_inspect_table_cells_options(
        {"inspect-table-cells", "input.docx", "0", "--row", "2", "--cell", "1",
         "--json"},
        3U, cell_options, cell_error));
    REQUIRE(cell_options.row_index.has_value());
    CHECK(*cell_options.row_index == 2U);
    REQUIRE(cell_options.cell_index.has_value());
    CHECK(*cell_options.cell_index == 1U);
    CHECK_FALSE(cell_options.grid_column.has_value());
    CHECK(cell_options.json_output);

    featherdoc_cli::inspect_table_cells_options grid_options;
    std::string grid_error;
    CHECK(featherdoc_cli::parse_inspect_table_cells_options(
        {"inspect-table-cells", "input.docx", "0", "--row", "2",
         "--grid-column", "5"},
        3U, grid_options, grid_error));
    REQUIRE(grid_options.row_index.has_value());
    CHECK(*grid_options.row_index == 2U);
    CHECK_FALSE(grid_options.cell_index.has_value());
    REQUIRE(grid_options.grid_column.has_value());
    CHECK(*grid_options.grid_column == 5U);
}

TEST_CASE("cli inspect table item options parse validates row errors") {
    featherdoc_cli::inspect_table_rows_options duplicate;
    std::string duplicate_error;
    CHECK_FALSE(featherdoc_cli::parse_inspect_table_rows_options(
        {"inspect-table-rows", "input.docx", "0", "--row", "1", "--row", "2"},
        3U, duplicate, duplicate_error));
    CHECK(duplicate_error == "duplicate --row option");

    featherdoc_cli::inspect_table_rows_options missing;
    std::string missing_error;
    CHECK_FALSE(featherdoc_cli::parse_inspect_table_rows_options(
        {"inspect-table-rows", "input.docx", "0", "--row"}, 3U, missing,
        missing_error));
    CHECK(missing_error == "missing value after --row");

    featherdoc_cli::inspect_table_rows_options invalid;
    std::string invalid_error;
    CHECK_FALSE(featherdoc_cli::parse_inspect_table_rows_options(
        {"inspect-table-rows", "input.docx", "0", "--row", "abc"}, 3U,
        invalid, invalid_error));
    CHECK(invalid_error == "invalid row index: abc");

    featherdoc_cli::inspect_table_rows_options unknown;
    std::string unknown_error;
    CHECK_FALSE(featherdoc_cli::parse_inspect_table_rows_options(
        {"inspect-table-rows", "input.docx", "0", "--bogus"}, 3U, unknown,
        unknown_error));
    CHECK(unknown_error == "unknown option: --bogus");
}

TEST_CASE("cli inspect table item options parse validates cell selector errors") {
    featherdoc_cli::inspect_table_cells_options duplicate_cell;
    std::string duplicate_cell_error;
    CHECK_FALSE(featherdoc_cli::parse_inspect_table_cells_options(
        {"inspect-table-cells", "input.docx", "0", "--row", "1", "--cell", "1",
         "--cell", "2"},
        3U, duplicate_cell, duplicate_cell_error));
    CHECK(duplicate_cell_error == "duplicate --cell option");

    featherdoc_cli::inspect_table_cells_options missing_cell;
    std::string missing_cell_error;
    CHECK_FALSE(featherdoc_cli::parse_inspect_table_cells_options(
        {"inspect-table-cells", "input.docx", "0", "--row", "1", "--cell"},
        3U, missing_cell, missing_cell_error));
    CHECK(missing_cell_error == "missing value after --cell");

    featherdoc_cli::inspect_table_cells_options invalid_cell;
    std::string invalid_cell_error;
    CHECK_FALSE(featherdoc_cli::parse_inspect_table_cells_options(
        {"inspect-table-cells", "input.docx", "0", "--row", "1", "--cell",
         "abc"},
        3U, invalid_cell, invalid_cell_error));
    CHECK(invalid_cell_error == "invalid cell index: abc");

    featherdoc_cli::inspect_table_cells_options duplicate_grid;
    std::string duplicate_grid_error;
    CHECK_FALSE(featherdoc_cli::parse_inspect_table_cells_options(
        {"inspect-table-cells", "input.docx", "0", "--row", "1",
         "--grid-column", "1", "--grid-column", "2"},
        3U, duplicate_grid, duplicate_grid_error));
    CHECK(duplicate_grid_error == "duplicate --grid-column option");

    featherdoc_cli::inspect_table_cells_options invalid_grid;
    std::string invalid_grid_error;
    CHECK_FALSE(featherdoc_cli::parse_inspect_table_cells_options(
        {"inspect-table-cells", "input.docx", "0", "--row", "1",
         "--grid-column", "abc"},
        3U, invalid_grid, invalid_grid_error));
    CHECK(invalid_grid_error == "invalid grid column: abc");
}

TEST_CASE("cli inspect table item options parse validates selector relationships") {
    featherdoc_cli::inspect_table_cells_options cell_without_row;
    std::string cell_without_row_error;
    CHECK_FALSE(featherdoc_cli::parse_inspect_table_cells_options(
        {"inspect-table-cells", "input.docx", "0", "--cell", "1"}, 3U,
        cell_without_row, cell_without_row_error));
    CHECK(cell_without_row_error == "--cell requires --row");

    featherdoc_cli::inspect_table_cells_options grid_without_row;
    std::string grid_without_row_error;
    CHECK_FALSE(featherdoc_cli::parse_inspect_table_cells_options(
        {"inspect-table-cells", "input.docx", "0", "--grid-column", "1"}, 3U,
        grid_without_row, grid_without_row_error));
    CHECK(grid_without_row_error == "--grid-column requires --row");

    featherdoc_cli::inspect_table_cells_options conflicting;
    std::string conflicting_error;
    CHECK_FALSE(featherdoc_cli::parse_inspect_table_cells_options(
        {"inspect-table-cells", "input.docx", "0", "--row", "1", "--cell", "2",
         "--grid-column", "3"},
        3U, conflicting, conflicting_error));
    CHECK(conflicting_error == "--cell and --grid-column are mutually exclusive");

    featherdoc_cli::inspect_table_cells_options unknown;
    std::string unknown_error;
    CHECK_FALSE(featherdoc_cli::parse_inspect_table_cells_options(
        {"inspect-table-cells", "input.docx", "0", "--bogus"}, 3U, unknown,
        unknown_error));
    CHECK(unknown_error == "unknown option: --bogus");
}
