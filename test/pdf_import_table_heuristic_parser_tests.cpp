#include "doctest.h"

#include "pdf_import_test_support.hpp"

#include <featherdoc/pdf/pdf_document_importer.hpp>
#include <featherdoc/pdf/pdf_parser.hpp>

#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <numeric>

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
    REQUIRE_EQ(page.content_blocks.size(), 2U);
    CHECK_EQ(page.content_blocks[0].kind,
             featherdoc::pdf::PdfParsedContentBlockKind::paragraph);
    CHECK_EQ(page.content_blocks[1].kind,
             featherdoc::pdf::PdfParsedContentBlockKind::table_candidate);

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

TEST_CASE("PDFium parser detects conservative two-row header-data table candidate") {
    const auto output_path =
        featherdoc::test_support::write_two_row_header_data_table_pdf(
            "featherdoc-pdf-import-two-row-table-source.pdf");

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
    REQUIRE_EQ(table.rows.size(), 2U);
    REQUIRE_EQ(table.column_anchor_x_points.size(), 3U);
    REQUIRE_EQ(table.rows[0].cells.size(), 3U);
    REQUIRE_EQ(table.rows[1].cells.size(), 3U);
    CHECK(featherdoc::test_support::contains_text(table.rows[0].cells[0].text,
                                                  "Item"));
    CHECK(featherdoc::test_support::contains_text(table.rows[0].cells[1].text,
                                                  "Owner"));
    CHECK(featherdoc::test_support::contains_text(table.rows[0].cells[2].text,
                                                  "Due"));
    CHECK(featherdoc::test_support::contains_text(table.rows[1].cells[0].text,
                                                  "INV-2026-05"));
    CHECK(featherdoc::test_support::contains_text(table.rows[1].cells[1].text,
                                                  "QA Team"));
    CHECK(featherdoc::test_support::contains_text(table.rows[1].cells[2].text,
                                                  "2026-05-14"));
}

TEST_CASE("PDFium parser detects conservative two-column key-value table candidate") {
    const auto output_path =
        featherdoc::test_support::write_two_column_key_value_table_pdf(
            "featherdoc-pdf-import-key-value-table-source.pdf");

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
    REQUIRE_EQ(table.column_anchor_x_points.size(), 2U);
    REQUIRE_EQ(table.rows[0].cells.size(), 2U);
    REQUIRE_EQ(table.rows[3].cells.size(), 2U);
    CHECK(featherdoc::test_support::contains_text(table.rows[0].cells[0].text,
                                                  "Invoice No"));
    CHECK(featherdoc::test_support::contains_text(table.rows[0].cells[1].text,
                                                  "INV-2026-0514"));
    CHECK(featherdoc::test_support::contains_text(table.rows[1].cells[0].text,
                                                  "Customer"));
    CHECK(featherdoc::test_support::contains_text(table.rows[1].cells[1].text,
                                                  "FeatherDoc QA"));
    CHECK(featherdoc::test_support::contains_text(table.rows[3].cells[0].text,
                                                  "Total"));
    CHECK(featherdoc::test_support::contains_text(table.rows[3].cells[1].text,
                                                  "USD 480"));
}

TEST_CASE("PDFium parser detects borderless two-column key-value table candidate") {
    const auto output_path = featherdoc::test_support::
        write_two_column_borderless_key_value_table_pdf(
            "featherdoc-pdf-import-key-value-borderless-table-source.pdf");

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
    REQUIRE_EQ(table.column_anchor_x_points.size(), 2U);
    REQUIRE_EQ(table.rows[0].cells.size(), 2U);
    REQUIRE_EQ(table.rows[3].cells.size(), 2U);
    CHECK(featherdoc::test_support::contains_text(table.rows[0].cells[0].text,
                                                  "Invoice No"));
    CHECK(featherdoc::test_support::contains_text(table.rows[0].cells[1].text,
                                                  "INV-2026-0610"));
    CHECK(featherdoc::test_support::contains_text(table.rows[3].cells[0].text,
                                                  "Amount"));
    CHECK(featherdoc::test_support::contains_text(table.rows[3].cells[1].text,
                                                  "USD 960"));
}

TEST_CASE("PDFium parser detects conservative irregular-width header table candidate") {
    const auto output_path =
        featherdoc::test_support::write_irregular_width_header_table_pdf(
            "featherdoc-pdf-import-irregular-width-table-source.pdf");

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
    REQUIRE_EQ(table.column_anchor_x_points.size(), 3U);
    const double first_gap =
        table.column_anchor_x_points[1] - table.column_anchor_x_points[0];
    const double second_gap =
        table.column_anchor_x_points[2] - table.column_anchor_x_points[1];
    CHECK_GT(std::abs(second_gap - first_gap), 24.0);
    CHECK(featherdoc::test_support::contains_text(table.rows[0].cells[0].text,
                                                  "Item"));
    CHECK(featherdoc::test_support::contains_text(table.rows[0].cells[1].text,
                                                  "Description"));
    CHECK(featherdoc::test_support::contains_text(table.rows[0].cells[2].text,
                                                  "Amount"));
    CHECK(featherdoc::test_support::contains_text(table.rows[1].cells[0].text,
                                                  "SKU-01"));
    CHECK(featherdoc::test_support::contains_text(table.rows[1].cells[1].text,
                                                  "Annual subscription"));
    CHECK(featherdoc::test_support::contains_text(table.rows[3].cells[2].text,
                                                  "USD 880"));
}

TEST_CASE("PDFium parser detects borderless irregular-width header table candidate") {
    const auto output_path = featherdoc::test_support::
        write_borderless_irregular_width_header_table_pdf(
            "featherdoc-pdf-import-borderless-irregular-width-table-source.pdf");

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
    REQUIRE_EQ(table.column_anchor_x_points.size(), 3U);
    const double first_gap =
        table.column_anchor_x_points[1] - table.column_anchor_x_points[0];
    const double second_gap =
        table.column_anchor_x_points[2] - table.column_anchor_x_points[1];
    CHECK_GT(std::abs(second_gap - first_gap), 24.0);
    CHECK(featherdoc::test_support::contains_text(table.rows[0].cells[0].text,
                                                  "Item"));
    CHECK(featherdoc::test_support::contains_text(table.rows[0].cells[1].text,
                                                  "Description"));
    CHECK(featherdoc::test_support::contains_text(table.rows[1].cells[0].text,
                                                  "SKU-11"));
    CHECK(featherdoc::test_support::contains_text(table.rows[1].cells[1].text,
                                                  "Document import"));
    CHECK(featherdoc::test_support::contains_text(table.rows[3].cells[2].text,
                                                  "USD 680"));
}

TEST_CASE("PDFium parser does not classify two-row three-column prose as table candidate") {
    const auto output_path =
        featherdoc::test_support::write_two_row_three_column_prose_pdf(
            "featherdoc-pdf-import-two-row-prose.pdf");

    featherdoc::pdf::PdfiumParser parser;
    const auto parse_result = parser.parse(output_path, {});
    REQUIRE_MESSAGE(parse_result.success, parse_result.error_message);
    REQUIRE_EQ(parse_result.document.pages.size(), 1U);

    const auto &page = parse_result.document.pages.front();
    CHECK(page.table_candidates.empty());
}

TEST_CASE("PDFium parser does not classify free-form column drift prose as table candidate") {
    const auto output_path =
        featherdoc::test_support::write_free_form_column_drift_prose_pdf(
            "featherdoc-pdf-import-free-form-column-drift-prose.pdf");

    featherdoc::pdf::PdfiumParser parser;
    const auto parse_result = parser.parse(output_path, {});
    REQUIRE_MESSAGE(parse_result.success, parse_result.error_message);
    REQUIRE_EQ(parse_result.document.pages.size(), 1U);

    const auto &page = parse_result.document.pages.front();
    CHECK(page.table_candidates.empty());
    REQUIRE_GE(page.paragraphs.size(), 2U);
    const auto combined_text =
        std::accumulate(page.paragraphs.begin(), page.paragraphs.end(),
                        std::string{}, [](std::string text,
                                           const auto &paragraph) {
                            text += paragraph.text;
                            text.push_back('\n');
                            return text;
                        });
    CHECK(featherdoc::test_support::contains_text(combined_text, "Topic"));
    CHECK(featherdoc::test_support::contains_text(combined_text, "Closed"));
}

TEST_CASE("PDFium parser does not classify two-column short-label prose as table candidate") {
    const auto output_path =
        featherdoc::test_support::write_two_column_short_label_prose_pdf(
            "featherdoc-pdf-import-two-column-short-label-prose.pdf");

    featherdoc::pdf::PdfiumParser parser;
    const auto parse_result = parser.parse(output_path, {});
    REQUIRE_MESSAGE(parse_result.success, parse_result.error_message);
    REQUIRE_EQ(parse_result.document.pages.size(), 1U);

    const auto &page = parse_result.document.pages.front();
    CHECK(page.table_candidates.empty());
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

TEST_CASE("PDFium parser does not classify aligned numbered list as table candidate") {
    const auto output_path =
        featherdoc::test_support::write_aligned_list_pdf(
            "featherdoc-pdf-import-aligned-list.pdf");

    featherdoc::pdf::PdfiumParser parser;
    const auto parse_result = parser.parse(output_path, {});
    REQUIRE_MESSAGE(parse_result.success, parse_result.error_message);
    REQUIRE_EQ(parse_result.document.pages.size(), 1U);

    const auto &page = parse_result.document.pages.front();
    CHECK(page.table_candidates.empty());
}

TEST_CASE("PDFium parser does not classify invoice summary form as table candidate") {
    const auto output_path =
        featherdoc::test_support::write_invoice_summary_pdf(
            "featherdoc-pdf-import-invoice-summary.pdf");

    featherdoc::pdf::PdfiumParser parser;
    const auto parse_result = parser.parse(output_path, {});
    REQUIRE_MESSAGE(parse_result.success, parse_result.error_message);
    REQUIRE_EQ(parse_result.document.pages.size(), 1U);

    const auto &page = parse_result.document.pages.front();
    CHECK(page.table_candidates.empty());
}

TEST_CASE("PDFium parser detects invoice grid table candidate") {
    const auto output_path =
        featherdoc::test_support::write_invoice_grid_pdf(
            "featherdoc-pdf-import-invoice-grid.pdf");

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
    REQUIRE_EQ(table.rows[4].cells.size(), 4U);

    CHECK(table.rows[0].cells[0].has_text);
    CHECK(table.rows[0].cells[1].has_text);
    CHECK(table.rows[0].cells[2].has_text);
    CHECK(table.rows[0].cells[3].has_text);
    CHECK(featherdoc::test_support::contains_text(table.rows[0].cells[0].text,
                                                  "Item"));
    CHECK(featherdoc::test_support::contains_text(table.rows[0].cells[3].text,
                                                  "Total"));
    CHECK(featherdoc::test_support::contains_text(table.rows[1].cells[0].text,
                                                  "PDF export design"));
    CHECK(featherdoc::test_support::contains_text(table.rows[1].cells[3].text,
                                                  "USD 100"));
    CHECK(featherdoc::test_support::contains_text(table.rows[4].cells[0].text,
                                                  "Grand total"));
    CHECK(featherdoc::test_support::contains_text(table.rows[4].cells[3].text,
                                                  "USD 135"));
}
