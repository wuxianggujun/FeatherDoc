#include "doctest.h"

#include "pdf_import_test_support.hpp"

#include <featherdoc/pdf/pdf_document_importer.hpp>
#include <featherdoc/pdf/pdf_parser.hpp>

#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <numeric>

TEST_CASE("PDFium parser detects wide merged-header table candidate spans") {
    const auto output_path =
        featherdoc::test_support::
            write_paragraph_merged_header_table_paragraph_pdf(
                "featherdoc-pdf-import-merged-table-source.pdf");

    featherdoc::pdf::PdfiumParser parser;
    const auto parse_result = parser.parse(output_path, {});
    REQUIRE_MESSAGE(parse_result.success, parse_result.error_message);
    REQUIRE_EQ(parse_result.document.pages.size(), 1U);

    const auto &page = parse_result.document.pages.front();
    REQUIRE_EQ(page.table_candidates.size(), 1U);
    REQUIRE_EQ(page.content_blocks.size(), 3U);
    CHECK_EQ(page.content_blocks[0].kind,
             featherdoc::pdf::PdfParsedContentBlockKind::paragraph);
    CHECK_EQ(page.content_blocks[1].kind,
             featherdoc::pdf::PdfParsedContentBlockKind::table_candidate);
    CHECK_EQ(page.content_blocks[2].kind,
             featherdoc::pdf::PdfParsedContentBlockKind::paragraph);

    const auto &table = page.table_candidates.front();
    REQUIRE_EQ(table.rows.size(), 3U);
    REQUIRE_EQ(table.rows[0].cells.size(), 3U);
    CHECK(table.rows[0].cells[0].has_text);
    CHECK_EQ(table.rows[0].cells[0].column_span, 2U);
    CHECK(featherdoc::test_support::contains_text(
        table.rows[0].cells[0].text, "Merged header spans two cols"));
    CHECK_FALSE(table.rows[0].cells[1].has_text);
    CHECK_EQ(table.rows[0].cells[1].column_span, 1U);
    CHECK(table.rows[0].cells[2].has_text);
    CHECK(featherdoc::test_support::contains_text(
        table.rows[0].cells[2].text, "Status"));
    CHECK_EQ(table.rows[1].cells[0].column_span, 1U);
    CHECK_EQ(table.rows[1].cells[1].column_span, 1U);
    CHECK_EQ(table.rows[1].cells[2].column_span, 1U);
}

TEST_CASE("PDFium parser detects minimal vertical merged table candidate spans") {
    const auto output_path =
        featherdoc::test_support::write_paragraph_vertical_merged_table_paragraph_pdf(
            "featherdoc-pdf-import-vertical-merged-table-source.pdf");

    featherdoc::pdf::PdfiumParser parser;
    const auto parse_result = parser.parse(output_path, {});
    REQUIRE_MESSAGE(parse_result.success, parse_result.error_message);
    REQUIRE_EQ(parse_result.document.pages.size(), 1U);

    const auto &page = parse_result.document.pages.front();
    REQUIRE_EQ(page.table_candidates.size(), 1U);
    REQUIRE_EQ(page.content_blocks.size(), 3U);
    CHECK_EQ(page.content_blocks[0].kind,
             featherdoc::pdf::PdfParsedContentBlockKind::paragraph);
    CHECK_EQ(page.content_blocks[1].kind,
             featherdoc::pdf::PdfParsedContentBlockKind::table_candidate);
    CHECK_EQ(page.content_blocks[2].kind,
             featherdoc::pdf::PdfParsedContentBlockKind::paragraph);

    const auto &table = page.table_candidates.front();
    REQUIRE_EQ(table.rows.size(), 3U);
    REQUIRE_EQ(table.rows[0].cells.size(), 3U);
    CHECK(table.rows[0].cells[0].has_text);
    CHECK_EQ(table.rows[0].cells[0].column_span, 1U);
    CHECK_EQ(table.rows[0].cells[0].row_span, 2U);
    CHECK(featherdoc::test_support::contains_text(table.rows[0].cells[0].text,
                                                  "Project"));
    CHECK_FALSE(table.rows[1].cells[0].has_text);
    CHECK_EQ(table.rows[1].cells[0].row_span, 1U);
    CHECK(table.rows[1].cells[1].has_text);
    CHECK(featherdoc::test_support::contains_text(table.rows[1].cells[1].text,
                                                  "Alpha"));
    CHECK(table.rows[2].cells[0].has_text);
    CHECK_EQ(table.rows[2].cells[0].row_span, 1U);
    CHECK(featherdoc::test_support::contains_text(table.rows[2].cells[0].text,
                                                  "Milestone"));
}

TEST_CASE(
    "PDFium parser detects multi-row middle-column merged table candidate spans") {
    const auto output_path =
        featherdoc::test_support::
            write_paragraph_middle_column_merged_table_paragraph_pdf(
                "featherdoc-pdf-import-middle-column-merged-table-source.pdf");

    featherdoc::pdf::PdfiumParser parser;
    const auto parse_result = parser.parse(output_path, {});
    REQUIRE_MESSAGE(parse_result.success, parse_result.error_message);
    REQUIRE_EQ(parse_result.document.pages.size(), 1U);

    const auto &page = parse_result.document.pages.front();
    REQUIRE_EQ(page.table_candidates.size(), 1U);
    REQUIRE_EQ(page.content_blocks.size(), 3U);
    CHECK_EQ(page.content_blocks[0].kind,
             featherdoc::pdf::PdfParsedContentBlockKind::paragraph);
    CHECK_EQ(page.content_blocks[1].kind,
             featherdoc::pdf::PdfParsedContentBlockKind::table_candidate);
    CHECK_EQ(page.content_blocks[2].kind,
             featherdoc::pdf::PdfParsedContentBlockKind::paragraph);

    const auto &table = page.table_candidates.front();
    REQUIRE_EQ(table.rows.size(), 3U);
    REQUIRE_EQ(table.rows[0].cells.size(), 3U);
    CHECK(table.rows[0].cells[1].has_text);
    CHECK_EQ(table.rows[0].cells[1].column_span, 1U);
    CHECK_EQ(table.rows[0].cells[1].row_span, 3U);
    CHECK(featherdoc::test_support::contains_text(table.rows[0].cells[1].text,
                                                  "Owner"));
    CHECK_FALSE(table.rows[1].cells[1].has_text);
    CHECK_EQ(table.rows[1].cells[1].row_span, 1U);
    CHECK_FALSE(table.rows[2].cells[1].has_text);
    CHECK_EQ(table.rows[2].cells[1].row_span, 1U);
    CHECK(table.rows[1].cells[0].has_text);
    CHECK(featherdoc::test_support::contains_text(table.rows[1].cells[0].text,
                                                  "Alpha"));
    CHECK(table.rows[2].cells[2].has_text);
    CHECK(featherdoc::test_support::contains_text(table.rows[2].cells[2].text,
                                                  "Done"));
}

TEST_CASE(
    "PDFium parser detects long repeated-header table candidate on every page") {
    const auto output_path =
        featherdoc::test_support::write_long_repeated_header_table_pdf(
            "featherdoc-pdf-import-long-repeated-header.pdf");

    featherdoc::pdf::PdfiumParser parser;
    const auto parse_result = parser.parse(output_path, {});
    REQUIRE_MESSAGE(parse_result.success, parse_result.error_message);
    REQUIRE_EQ(parse_result.document.pages.size(), 4U);

    for (const auto &page : parse_result.document.pages) {
        REQUIRE_EQ(page.table_candidates.size(), 1U);

        const auto &table = page.table_candidates.front();
        REQUIRE_EQ(table.rows.size(), 14U);
        REQUIRE_EQ(table.column_anchor_x_points.size(), 3U);
        REQUIRE_EQ(table.rows.front().cells.size(), 3U);
        CHECK(table.rows.front().cells[0].has_text);
        CHECK(table.rows.front().cells[1].has_text);
        CHECK(table.rows.front().cells[2].has_text);
        CHECK(featherdoc::test_support::contains_text(
            table.rows.front().cells[0].text, "Item"));
        CHECK(featherdoc::test_support::contains_text(
            table.rows.front().cells[1].text, "Owner"));
        CHECK(featherdoc::test_support::contains_text(
            table.rows.front().cells[2].text, "Status"));
    }
}

TEST_CASE(
    "PDFium parser detects top-left two-by-two merged table candidate spans") {
    const auto output_path =
        featherdoc::test_support::write_paragraph_merged_corner_table_paragraph_pdf(
            "featherdoc-pdf-import-merged-corner-table-source.pdf");

    featherdoc::pdf::PdfiumParser parser;
    const auto parse_result = parser.parse(output_path, {});
    REQUIRE_MESSAGE(parse_result.success, parse_result.error_message);
    REQUIRE_EQ(parse_result.document.pages.size(), 1U);

    const auto &page = parse_result.document.pages.front();
    REQUIRE_EQ(page.table_candidates.size(), 1U);
    REQUIRE_EQ(page.content_blocks.size(), 3U);
    CHECK_EQ(page.content_blocks[0].kind,
             featherdoc::pdf::PdfParsedContentBlockKind::paragraph);
    CHECK_EQ(page.content_blocks[1].kind,
             featherdoc::pdf::PdfParsedContentBlockKind::table_candidate);
    CHECK_EQ(page.content_blocks[2].kind,
             featherdoc::pdf::PdfParsedContentBlockKind::paragraph);

    const auto &table = page.table_candidates.front();
    REQUIRE_EQ(table.rows.size(), 3U);
    REQUIRE_EQ(table.column_anchor_x_points.size(), 4U);
    REQUIRE_EQ(table.rows[0].cells.size(), 4U);
    REQUIRE_EQ(table.rows[1].cells.size(), 4U);
    REQUIRE_EQ(table.rows[2].cells.size(), 4U);

    CHECK(table.rows[0].cells[0].has_text);
    CHECK_EQ(table.rows[0].cells[0].column_span, 2U);
    CHECK_EQ(table.rows[0].cells[0].row_span, 2U);
    CHECK(featherdoc::test_support::contains_text(
        table.rows[0].cells[0].text, "Project overview and owner assignment"));
    CHECK_FALSE(table.rows[0].cells[1].has_text);
    CHECK(table.rows[0].cells[2].has_text);
    CHECK(featherdoc::test_support::contains_text(table.rows[0].cells[2].text,
                                                  "Status"));
    CHECK(table.rows[0].cells[3].has_text);
    CHECK(featherdoc::test_support::contains_text(table.rows[0].cells[3].text,
                                                  "Phase"));

    CHECK_FALSE(table.rows[1].cells[0].has_text);
    CHECK_EQ(table.rows[1].cells[0].row_span, 1U);
    CHECK_EQ(table.rows[1].cells[0].column_span, 1U);
    CHECK_FALSE(table.rows[1].cells[1].has_text);
    CHECK(table.rows[1].cells[2].has_text);
    CHECK(featherdoc::test_support::contains_text(table.rows[1].cells[2].text,
                                                  "Alpha"));
    CHECK(table.rows[1].cells[3].has_text);
    CHECK(featherdoc::test_support::contains_text(table.rows[1].cells[3].text,
                                                  "Open"));

    CHECK(table.rows[2].cells[0].has_text);
    CHECK(featherdoc::test_support::contains_text(table.rows[2].cells[0].text,
                                                  "Milestone"));
    CHECK(table.rows[2].cells[1].has_text);
    CHECK(featherdoc::test_support::contains_text(table.rows[2].cells[1].text,
                                                  "Beta"));
    CHECK(table.rows[2].cells[2].has_text);
    CHECK(featherdoc::test_support::contains_text(table.rows[2].cells[2].text,
                                                  "Done"));
    CHECK(table.rows[2].cells[3].has_text);
    CHECK(featherdoc::test_support::contains_text(table.rows[2].cells[3].text,
                                                  "Closed"));
}

TEST_CASE(
    "PDFium parser detects center two-by-two merged table candidate spans") {
    const auto output_path =
        featherdoc::test_support::write_paragraph_center_merged_table_paragraph_pdf(
            "featherdoc-pdf-import-center-merged-table-source.pdf");

    featherdoc::pdf::PdfiumParser parser;
    const auto parse_result = parser.parse(output_path, {});
    REQUIRE_MESSAGE(parse_result.success, parse_result.error_message);
    REQUIRE_EQ(parse_result.document.pages.size(), 1U);

    const auto &page = parse_result.document.pages.front();
    REQUIRE_EQ(page.table_candidates.size(), 1U);
    REQUIRE_EQ(page.content_blocks.size(), 3U);
    CHECK_EQ(page.content_blocks[0].kind,
             featherdoc::pdf::PdfParsedContentBlockKind::paragraph);
    CHECK_EQ(page.content_blocks[1].kind,
             featherdoc::pdf::PdfParsedContentBlockKind::table_candidate);
    CHECK_EQ(page.content_blocks[2].kind,
             featherdoc::pdf::PdfParsedContentBlockKind::paragraph);

    const auto &table = page.table_candidates.front();
    REQUIRE_EQ(table.rows.size(), 4U);
    REQUIRE_EQ(table.column_anchor_x_points.size(), 4U);
    REQUIRE_EQ(table.rows[0].cells.size(), 4U);
    REQUIRE_EQ(table.rows[1].cells.size(), 4U);
    REQUIRE_EQ(table.rows[2].cells.size(), 4U);
    REQUIRE_EQ(table.rows[3].cells.size(), 4U);

    CHECK(table.rows[1].cells[1].has_text);
    CHECK_EQ(table.rows[1].cells[1].column_span, 2U);
    CHECK_EQ(table.rows[1].cells[1].row_span, 2U);
    CHECK(featherdoc::test_support::contains_text(
        table.rows[1].cells[1].text, "Owner assignment spans review"));
    CHECK_FALSE(table.rows[1].cells[2].has_text);
    CHECK_FALSE(table.rows[2].cells[1].has_text);
    CHECK_FALSE(table.rows[2].cells[2].has_text);
    CHECK(featherdoc::test_support::contains_text(table.rows[0].cells[0].text,
                                                  "Item"));
    CHECK(featherdoc::test_support::contains_text(table.rows[3].cells[3].text,
                                                  "Closed"));
}

TEST_CASE(
    "PDFium parser detects center two-by-three merged table candidate spans") {
    const auto output_path = featherdoc::test_support::
        write_paragraph_rectangular_merged_table_paragraph_pdf(
            "featherdoc-pdf-import-rectangular-merged-table-source.pdf");

    featherdoc::pdf::PdfiumParser parser;
    const auto parse_result = parser.parse(output_path, {});
    REQUIRE_MESSAGE(parse_result.success, parse_result.error_message);
    REQUIRE_EQ(parse_result.document.pages.size(), 1U);

    const auto &page = parse_result.document.pages.front();
    REQUIRE_EQ(page.table_candidates.size(), 1U);
    REQUIRE_EQ(page.content_blocks.size(), 3U);
    CHECK_EQ(page.content_blocks[0].kind,
             featherdoc::pdf::PdfParsedContentBlockKind::paragraph);
    CHECK_EQ(page.content_blocks[1].kind,
             featherdoc::pdf::PdfParsedContentBlockKind::table_candidate);
    CHECK_EQ(page.content_blocks[2].kind,
             featherdoc::pdf::PdfParsedContentBlockKind::paragraph);

    const auto &table = page.table_candidates.front();
    REQUIRE_EQ(table.rows.size(), 5U);
    REQUIRE_EQ(table.column_anchor_x_points.size(), 5U);
    for (const auto &row : table.rows) {
        REQUIRE_EQ(row.cells.size(), 5U);
    }

    CHECK(table.rows[1].cells[1].has_text);
    CHECK_EQ(table.rows[1].cells[1].column_span, 2U);
    CHECK_EQ(table.rows[1].cells[1].row_span, 3U);
    CHECK(featherdoc::test_support::contains_text(
        table.rows[1].cells[1].text, "Owner assignment spans review"));
    CHECK_FALSE(table.rows[1].cells[2].has_text);
    CHECK_FALSE(table.rows[2].cells[1].has_text);
    CHECK_FALSE(table.rows[2].cells[2].has_text);
    CHECK_FALSE(table.rows[3].cells[1].has_text);
    CHECK_FALSE(table.rows[3].cells[2].has_text);
    CHECK(featherdoc::test_support::contains_text(table.rows[0].cells[4].text,
                                                  "Flag"));
    CHECK(featherdoc::test_support::contains_text(table.rows[4].cells[4].text,
                                                  "Green"));
}

TEST_CASE("PDFium parser detects cross-column header table candidate spans") {
    const auto output_path = featherdoc::test_support::
        write_paragraph_cross_column_header_table_paragraph_pdf(
            "featherdoc-pdf-import-cross-column-header-table-source.pdf");

    featherdoc::pdf::PdfiumParser parser;
    const auto parse_result = parser.parse(output_path, {});
    REQUIRE_MESSAGE(parse_result.success, parse_result.error_message);
    REQUIRE_EQ(parse_result.document.pages.size(), 1U);

    const auto &page = parse_result.document.pages.front();
    REQUIRE_EQ(page.table_candidates.size(), 1U);
    REQUIRE_EQ(page.content_blocks.size(), 3U);
    CHECK_EQ(page.content_blocks[0].kind,
             featherdoc::pdf::PdfParsedContentBlockKind::paragraph);
    CHECK_EQ(page.content_blocks[1].kind,
             featherdoc::pdf::PdfParsedContentBlockKind::table_candidate);
    CHECK_EQ(page.content_blocks[2].kind,
             featherdoc::pdf::PdfParsedContentBlockKind::paragraph);

    const auto &table = page.table_candidates.front();
    REQUIRE_EQ(table.rows.size(), 4U);
    REQUIRE_EQ(table.column_anchor_x_points.size(), 4U);
    REQUIRE_EQ(table.rows[0].cells.size(), 4U);
    REQUIRE_EQ(table.rows[1].cells.size(), 4U);
    REQUIRE_EQ(table.rows[3].cells.size(), 4U);

    CHECK(table.rows[0].cells[0].has_text);
    CHECK_EQ(table.rows[0].cells[0].column_span, 4U);
    CHECK(featherdoc::test_support::contains_text(
        table.rows[0].cells[0].text, "Project delivery overview"));
    CHECK_FALSE(table.rows[0].cells[1].has_text);
    CHECK_FALSE(table.rows[0].cells[2].has_text);
    CHECK_FALSE(table.rows[0].cells[3].has_text);

    CHECK(table.rows[1].cells[0].has_text);
    CHECK(table.rows[1].cells[1].has_text);
    CHECK(table.rows[1].cells[2].has_text);
    CHECK(table.rows[1].cells[3].has_text);
    CHECK_EQ(table.rows[1].cells[0].column_span, 1U);
    CHECK_EQ(table.rows[1].cells[1].column_span, 1U);
    CHECK(featherdoc::test_support::contains_text(table.rows[1].cells[0].text,
                                                  "Item"));
    CHECK(featherdoc::test_support::contains_text(table.rows[1].cells[3].text,
                                                  "Due"));
    CHECK(featherdoc::test_support::contains_text(table.rows[2].cells[0].text,
                                                  "DOC-17"));
    CHECK(featherdoc::test_support::contains_text(table.rows[3].cells[3].text,
                                                  "2026-05-21"));
}

TEST_CASE("PDFium parser detects parallel group header table candidate spans") {
    const auto output_path = featherdoc::test_support::
        write_paragraph_parallel_group_header_table_paragraph_pdf(
            "featherdoc-pdf-import-parallel-group-header-table-source.pdf");

    featherdoc::pdf::PdfiumParser parser;
    const auto parse_result = parser.parse(output_path, {});
    REQUIRE_MESSAGE(parse_result.success, parse_result.error_message);
    REQUIRE_EQ(parse_result.document.pages.size(), 1U);

    const auto &page = parse_result.document.pages.front();
    REQUIRE_EQ(page.table_candidates.size(), 1U);
    REQUIRE_EQ(page.content_blocks.size(), 3U);
    CHECK_EQ(page.content_blocks[0].kind,
             featherdoc::pdf::PdfParsedContentBlockKind::paragraph);
    CHECK_EQ(page.content_blocks[1].kind,
             featherdoc::pdf::PdfParsedContentBlockKind::table_candidate);
    CHECK_EQ(page.content_blocks[2].kind,
             featherdoc::pdf::PdfParsedContentBlockKind::paragraph);

    const auto &table = page.table_candidates.front();
    REQUIRE_EQ(table.rows.size(), 4U);
    REQUIRE_EQ(table.column_anchor_x_points.size(), 4U);
    REQUIRE_EQ(table.rows[0].cells.size(), 4U);
    REQUIRE_EQ(table.rows[1].cells.size(), 4U);
    REQUIRE_EQ(table.rows[3].cells.size(), 4U);

    CHECK(table.rows[0].cells[0].has_text);
    CHECK_EQ(table.rows[0].cells[0].column_span, 2U);
    CHECK(featherdoc::test_support::contains_text(
        table.rows[0].cells[0].text, "Delivery scope"));
    CHECK_FALSE(table.rows[0].cells[1].has_text);
    CHECK(table.rows[0].cells[2].has_text);
    CHECK_EQ(table.rows[0].cells[2].column_span, 2U);
    CHECK(featherdoc::test_support::contains_text(
        table.rows[0].cells[2].text, "Review status"));
    CHECK_FALSE(table.rows[0].cells[3].has_text);

    CHECK(featherdoc::test_support::contains_text(table.rows[1].cells[0].text,
                                                  "Item"));
    CHECK(featherdoc::test_support::contains_text(table.rows[1].cells[3].text,
                                                  "Due"));
    CHECK(featherdoc::test_support::contains_text(table.rows[2].cells[0].text,
                                                  "DOC-27"));
    CHECK(featherdoc::test_support::contains_text(table.rows[3].cells[3].text,
                                                  "2026-06-11"));
}

TEST_CASE("PDFium parser detects multilevel group header table candidate spans") {
    const auto output_path = featherdoc::test_support::
        write_paragraph_multilevel_group_header_table_paragraph_pdf(
            "featherdoc-pdf-import-multilevel-group-header-table-source.pdf");

    featherdoc::pdf::PdfiumParser parser;
    const auto parse_result = parser.parse(output_path, {});
    REQUIRE_MESSAGE(parse_result.success, parse_result.error_message);
    REQUIRE_EQ(parse_result.document.pages.size(), 1U);

    const auto &page = parse_result.document.pages.front();
    REQUIRE_EQ(page.table_candidates.size(), 1U);
    REQUIRE_EQ(page.content_blocks.size(), 3U);
    CHECK_EQ(page.content_blocks[0].kind,
             featherdoc::pdf::PdfParsedContentBlockKind::paragraph);
    CHECK_EQ(page.content_blocks[1].kind,
             featherdoc::pdf::PdfParsedContentBlockKind::table_candidate);
    CHECK_EQ(page.content_blocks[2].kind,
             featherdoc::pdf::PdfParsedContentBlockKind::paragraph);

    const auto &table = page.table_candidates.front();
    REQUIRE_EQ(table.rows.size(), 5U);
    REQUIRE_EQ(table.column_anchor_x_points.size(), 4U);
    REQUIRE_EQ(table.rows[0].cells.size(), 4U);
    REQUIRE_EQ(table.rows[1].cells.size(), 4U);
    REQUIRE_EQ(table.rows[4].cells.size(), 4U);

    CHECK(table.rows[0].cells[0].has_text);
    CHECK_EQ(table.rows[0].cells[0].column_span, 4U);
    CHECK(featherdoc::test_support::contains_text(
        table.rows[0].cells[0].text, "Program delivery dashboard"));
    CHECK_FALSE(table.rows[0].cells[1].has_text);
    CHECK_FALSE(table.rows[0].cells[2].has_text);
    CHECK_FALSE(table.rows[0].cells[3].has_text);

    CHECK(table.rows[1].cells[0].has_text);
    CHECK_EQ(table.rows[1].cells[0].column_span, 2U);
    CHECK(featherdoc::test_support::contains_text(
        table.rows[1].cells[0].text, "Delivery scope"));
    CHECK_FALSE(table.rows[1].cells[1].has_text);
    CHECK(table.rows[1].cells[2].has_text);
    CHECK_EQ(table.rows[1].cells[2].column_span, 2U);
    CHECK(featherdoc::test_support::contains_text(
        table.rows[1].cells[2].text, "Review status"));
    CHECK_FALSE(table.rows[1].cells[3].has_text);

    CHECK(featherdoc::test_support::contains_text(table.rows[2].cells[0].text,
                                                  "Item"));
    CHECK(featherdoc::test_support::contains_text(table.rows[2].cells[3].text,
                                                  "Due"));
    CHECK(featherdoc::test_support::contains_text(table.rows[3].cells[0].text,
                                                  "DOC-37"));
    CHECK(featherdoc::test_support::contains_text(table.rows[4].cells[3].text,
                                                  "2026-06-25"));
}
