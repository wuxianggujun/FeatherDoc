#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "pdf_import_test_support.hpp"

#include <featherdoc/pdf/pdf_parser.hpp>

TEST_CASE("PDFium parser detects sparse table-like grid candidates") {
    const auto output_path =
        featherdoc::test_support::write_table_like_grid_pdf(
            "featherdoc-pdf-import-table-like-grid.pdf");

    featherdoc::pdf::PdfiumParser parser;
    const auto parse_result = parser.parse(output_path, {});
    REQUIRE_MESSAGE(parse_result.success, parse_result.error_message);
    REQUIRE_EQ(parse_result.document.pages.size(), 1U);

    const auto &page = parse_result.document.pages.front();
    REQUIRE_EQ(page.table_candidates.size(), 1U);

    const auto &table = page.table_candidates.front();
    REQUIRE_EQ(table.rows.size(), 3U);
    REQUIRE_EQ(table.column_anchor_x_points.size(), 3U);
    CHECK_GT(table.bounds.width_points, 0.0);
    CHECK_GT(table.bounds.height_points, 0.0);

    REQUIRE_EQ(table.rows[0].cells.size(), 3U);
    REQUIRE_EQ(table.rows[1].cells.size(), 3U);
    REQUIRE_EQ(table.rows[2].cells.size(), 3U);

    CHECK(table.rows[0].cells[0].has_text);
    CHECK(featherdoc::test_support::contains_text(
        table.rows[0].cells[0].text, "Cell A1"));
    CHECK_FALSE(table.rows[0].cells[1].has_text);
    CHECK_FALSE(table.rows[0].cells[2].has_text);

    CHECK_FALSE(table.rows[1].cells[0].has_text);
    CHECK(table.rows[1].cells[1].has_text);
    CHECK(featherdoc::test_support::contains_text(
        table.rows[1].cells[1].text, "Cell B2"));
    CHECK_FALSE(table.rows[1].cells[2].has_text);

    CHECK_FALSE(table.rows[2].cells[0].has_text);
    CHECK_FALSE(table.rows[2].cells[1].has_text);
    CHECK(table.rows[2].cells[2].has_text);
    CHECK(featherdoc::test_support::contains_text(
        table.rows[2].cells[2].text, "Cell C3"));
}

TEST_CASE("PDFium parser does not classify two-column prose as table candidate") {
    const auto output_path =
        featherdoc::test_support::write_two_column_pdf(
            "featherdoc-pdf-import-two-column.pdf");

    featherdoc::pdf::PdfiumParser parser;
    const auto parse_result = parser.parse(output_path, {});
    REQUIRE_MESSAGE(parse_result.success, parse_result.error_message);
    REQUIRE_EQ(parse_result.document.pages.size(), 1U);

    const auto &page = parse_result.document.pages.front();
    CHECK(page.table_candidates.empty());
}
