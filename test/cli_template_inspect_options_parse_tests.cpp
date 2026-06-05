#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "featherdoc_cli_template_inspect_options_parse.hpp"

#include <string>
#include <string_view>
#include <vector>

TEST_CASE("cli template inspect options parse accepts paragraph selection") {
    featherdoc_cli::template_inspect_paragraphs_options options;
    std::string error;

    CHECK(featherdoc_cli::parse_template_inspect_paragraphs_options(
        {"template-inspect-paragraphs", "input.docx", "--part",
         "section-header", "--section", "1", "--paragraph", "3", "--json"},
        2U, options, error));

    CHECK(options.part == featherdoc_cli::validation_part_family::section_header);
    REQUIRE(options.section_index.has_value());
    CHECK(*options.section_index == 1U);
    REQUIRE(options.paragraph_index.has_value());
    CHECK(*options.paragraph_index == 3U);
    CHECK(options.json_output);
}

TEST_CASE("cli template inspect options parse accepts run selection") {
    featherdoc_cli::template_inspect_runs_options options;
    std::string error;

    CHECK(featherdoc_cli::parse_template_inspect_runs_options(
        {"template-inspect-runs", "input.docx", "5", "--part", "footer",
         "--index", "2", "--run", "4", "--json"},
        3U, options, error));

    CHECK(options.part == featherdoc_cli::validation_part_family::footer);
    REQUIRE(options.part_index.has_value());
    CHECK(*options.part_index == 2U);
    REQUIRE(options.run_index.has_value());
    CHECK(*options.run_index == 4U);
    CHECK(options.json_output);
}

TEST_CASE("cli template inspect options parse accepts table selectors") {
    featherdoc_cli::template_inspect_tables_options tables;
    std::string tables_error;
    CHECK(featherdoc_cli::parse_template_inspect_tables_options(
        {"template-inspect-tables", "input.docx", "--bookmark", "chapter-1",
         "--json"},
        2U, tables, tables_error));
    REQUIRE(tables.bookmark_name.has_value());
    CHECK(*tables.bookmark_name == "chapter-1");
    CHECK(tables.json_output);

    featherdoc_cli::template_inspect_table_rows_options rows;
    std::string rows_error;
    CHECK(featherdoc_cli::parse_template_inspect_table_rows_options(
        {"template-inspect-table-rows", "input.docx", "--after-text", "Intro",
         "--header-cell", "Title", "--row", "4"},
        2U, rows, rows_error));
    REQUIRE(rows.selector.after_paragraph_text.has_value());
    CHECK(*rows.selector.after_paragraph_text == "Intro");
    CHECK(rows.selector.header_cell_texts.size() == 1U);
    CHECK(rows.selector.header_cell_texts.front() == "Title");
    REQUIRE(rows.row_index.has_value());
    CHECK(*rows.row_index == 4U);

    featherdoc_cli::template_inspect_table_cells_options cells;
    std::string cells_error;
    CHECK(featherdoc_cli::parse_template_inspect_table_cells_options(
        {"template-inspect-table-cells", "input.docx", "--bookmark",
         "table-a", "--row", "2", "--grid-column", "3"},
        2U, cells, cells_error));
    REQUIRE(cells.bookmark_name.has_value());
    CHECK(*cells.bookmark_name == "table-a");
    REQUIRE(cells.row_index.has_value());
    CHECK(*cells.row_index == 2U);
    REQUIRE(cells.grid_column.has_value());
    CHECK(*cells.grid_column == 3U);
}

TEST_CASE("cli template inspect options parse reports validation errors") {
    featherdoc_cli::template_inspect_paragraphs_options paragraph_duplicate;
    std::string paragraph_error;
    CHECK_FALSE(featherdoc_cli::parse_template_inspect_paragraphs_options(
        {"template-inspect-paragraphs", "input.docx", "--paragraph", "1",
         "--paragraph", "2"},
        2U, paragraph_duplicate, paragraph_error));
    CHECK(paragraph_error == "duplicate --paragraph option");

    featherdoc_cli::template_inspect_table_cells_options missing_row;
    std::string missing_row_error;
    CHECK_FALSE(featherdoc_cli::parse_template_inspect_table_cells_options(
        {"template-inspect-table-cells", "input.docx", "--cell", "1"}, 2U,
        missing_row, missing_row_error));
    CHECK(missing_row_error == "--cell requires --row");

    featherdoc::template_table_selector selector;
    selector.table_index = 1U;
    selector.after_paragraph_text = std::string("Intro");
    std::string selector_error;
    CHECK_FALSE(featherdoc_cli::validate_template_table_selector(
        selector, false, false, false, selector_error));
    CHECK(selector_error ==
          "cannot combine a table index or --bookmark with --after-text or --header-cell");
}
