#include "doctest.h"

#include "pdf_import_test_support.hpp"

#include <featherdoc/pdf/pdf_document_importer.hpp>
#include <featherdoc/pdf/pdf_parser.hpp>

#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <numeric>

TEST_CASE("PDFium parser detects merged summary-row table candidate spans") {
    const auto output_path =
        featherdoc::test_support::write_invoice_grid_merged_summary_pdf(
            "featherdoc-pdf-import-merged-summary-row-table-source.pdf");

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

    CHECK(table.rows[4].cells[0].has_text);
    CHECK_EQ(table.rows[4].cells[0].column_span, 3U);
    CHECK(featherdoc::test_support::contains_text(table.rows[4].cells[0].text,
                                                  "Grand total"));
    CHECK_FALSE(table.rows[4].cells[1].has_text);
    CHECK_FALSE(table.rows[4].cells[2].has_text);
    CHECK(table.rows[4].cells[3].has_text);
    CHECK_EQ(table.rows[4].cells[3].column_span, 1U);
    CHECK(featherdoc::test_support::contains_text(table.rows[4].cells[3].text,
                                                  "USD 135"));
}

TEST_CASE("PDFium parser detects inline subtotal-row table candidate spans") {
    const auto output_path =
        featherdoc::test_support::write_invoice_grid_inline_subtotal_pdf(
            "featherdoc-pdf-import-inline-subtotal-row-table-source.pdf");

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
    REQUIRE_EQ(table.rows.size(), 6U);
    REQUIRE_EQ(table.column_anchor_x_points.size(), 4U);
    REQUIRE_EQ(table.rows[2].cells.size(), 4U);
    REQUIRE_EQ(table.rows[5].cells.size(), 4U);

    CHECK(table.rows[2].cells[0].has_text);
    CHECK_EQ(table.rows[2].cells[0].column_span, 3U);
    CHECK(featherdoc::test_support::contains_text(table.rows[2].cells[0].text,
                                                  "Design subtotal"));
    CHECK_FALSE(table.rows[2].cells[1].has_text);
    CHECK_FALSE(table.rows[2].cells[2].has_text);
    CHECK(table.rows[2].cells[3].has_text);
    CHECK(featherdoc::test_support::contains_text(table.rows[2].cells[3].text,
                                                  "USD 100"));
    CHECK(featherdoc::test_support::contains_text(table.rows[3].cells[0].text,
                                                  "Visual validation"));
    CHECK(featherdoc::test_support::contains_text(table.rows[5].cells[0].text,
                                                  "Grand total"));
    CHECK_EQ(table.rows[5].cells[0].column_span, 3U);
    CHECK(featherdoc::test_support::contains_text(table.rows[5].cells[3].text,
                                                  "USD 135"));
}

TEST_CASE("PDFium parser detects right subtotal-row table candidate spans") {
    const auto output_path =
        featherdoc::test_support::write_invoice_grid_right_subtotal_pdf(
            "featherdoc-pdf-import-right-subtotal-row-table-source.pdf");

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
    REQUIRE_EQ(table.rows.size(), 6U);
    REQUIRE_EQ(table.column_anchor_x_points.size(), 4U);
    REQUIRE_EQ(table.rows[2].cells.size(), 4U);
    REQUIRE_EQ(table.rows[5].cells.size(), 4U);

    CHECK_FALSE(table.rows[2].cells[0].has_text);
    CHECK(table.rows[2].cells[1].has_text);
    CHECK_EQ(table.rows[2].cells[1].column_span, 2U);
    CHECK(featherdoc::test_support::contains_text(table.rows[2].cells[1].text,
                                                  "Design subtotal"));
    CHECK_FALSE(table.rows[2].cells[2].has_text);
    CHECK(table.rows[2].cells[3].has_text);
    CHECK(featherdoc::test_support::contains_text(table.rows[2].cells[3].text,
                                                  "USD 100"));
    CHECK(featherdoc::test_support::contains_text(table.rows[3].cells[0].text,
                                                  "Visual validation"));
    CHECK_FALSE(table.rows[5].cells[0].has_text);
    CHECK_EQ(table.rows[5].cells[1].column_span, 2U);
    CHECK(featherdoc::test_support::contains_text(table.rows[5].cells[1].text,
                                                  "Grand total"));
    CHECK(featherdoc::test_support::contains_text(table.rows[5].cells[3].text,
                                                  "USD 135"));
}

TEST_CASE("PDFium parser detects middle-amount subtotal-row table candidate spans") {
    const auto output_path = featherdoc::test_support::
        write_invoice_grid_middle_amount_subtotal_pdf(
            "featherdoc-pdf-import-middle-amount-subtotal-row-table-source.pdf");

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
    REQUIRE_EQ(table.rows.size(), 6U);
    REQUIRE_EQ(table.column_anchor_x_points.size(), 4U);
    REQUIRE_EQ(table.rows[2].cells.size(), 4U);
    REQUIRE_EQ(table.rows[5].cells.size(), 4U);

    CHECK(table.rows[2].cells[0].has_text);
    CHECK_EQ(table.rows[2].cells[0].column_span, 2U);
    CHECK(featherdoc::test_support::contains_text(table.rows[2].cells[0].text,
                                                  "Design subtotal"));
    CHECK_FALSE(table.rows[2].cells[1].has_text);
    CHECK(table.rows[2].cells[2].has_text);
    CHECK(featherdoc::test_support::contains_text(table.rows[2].cells[2].text,
                                                  "USD 100"));
    CHECK_FALSE(table.rows[2].cells[3].has_text);
    CHECK_EQ(table.rows[1].cells[3].row_span, 1U);
    CHECK(featherdoc::test_support::contains_text(table.rows[3].cells[0].text,
                                                  "Visual validation"));
    CHECK_EQ(table.rows[5].cells[0].column_span, 2U);
    CHECK(featherdoc::test_support::contains_text(table.rows[5].cells[0].text,
                                                  "Grand total"));
    CHECK(featherdoc::test_support::contains_text(table.rows[5].cells[2].text,
                                                  "USD 135"));
    CHECK_FALSE(table.rows[5].cells[3].has_text);
}

TEST_CASE(
    "PDFium parser keeps cross-page subtotal rows with a missing body cell aligned") {
    const auto output_path = featherdoc::test_support::
        write_invoice_grid_pagebreak_subtotal_missing_unit_pdf(
            "featherdoc-pdf-import-pagebreak-subtotal-missing-unit-table-source.pdf");

    featherdoc::pdf::PdfiumParser parser;
    const auto parse_result = parser.parse(output_path, {});
    REQUIRE_MESSAGE(parse_result.success, parse_result.error_message);
    REQUIRE_EQ(parse_result.document.pages.size(), 2U);

    const auto &first_page = parse_result.document.pages[0];
    const auto &second_page = parse_result.document.pages[1];
    REQUIRE_EQ(first_page.table_candidates.size(), 1U);
    REQUIRE_EQ(second_page.table_candidates.size(), 1U);

    const auto &first_table = first_page.table_candidates.front();
    const auto &second_table = second_page.table_candidates.front();
    REQUIRE_EQ(first_table.column_anchor_x_points.size(), 4U);
    REQUIRE_EQ(second_table.column_anchor_x_points.size(), 4U);
    REQUIRE_EQ(first_table.rows.size(), 4U);
    REQUIRE_EQ(second_table.rows.size(), 4U);
    REQUIRE_EQ(second_table.rows[1].cells.size(), 4U);

    CHECK(featherdoc::test_support::contains_text(
        second_table.rows[1].cells[0].text, "Regression evidence"));
    CHECK(featherdoc::test_support::contains_text(
        second_table.rows[1].cells[1].text, "1"));
    CHECK_FALSE(second_table.rows[1].cells[2].has_text);
    CHECK(second_table.rows[1].cells[3].has_text);
    CHECK(featherdoc::test_support::contains_text(
        second_table.rows[1].cells[3].text, "USD 10"));
    CHECK_EQ(second_table.rows[3].cells[0].column_span, 3U);
    CHECK(featherdoc::test_support::contains_text(
        second_table.rows[3].cells[0].text, "Grand total"));
}

TEST_CASE(
    "PDFium parser keeps cross-page subtotal sparse body rows aligned") {
    const auto output_path = featherdoc::test_support::
        write_invoice_grid_pagebreak_subtotal_sparse_body_pdf(
            "featherdoc-pdf-import-pagebreak-subtotal-sparse-body-table-source.pdf");

    featherdoc::pdf::PdfiumParser parser;
    const auto parse_result = parser.parse(output_path, {});
    REQUIRE_MESSAGE(parse_result.success, parse_result.error_message);
    REQUIRE_EQ(parse_result.document.pages.size(), 2U);

    const auto &first_page = parse_result.document.pages[0];
    const auto &second_page = parse_result.document.pages[1];
    REQUIRE_EQ(first_page.table_candidates.size(), 1U);
    REQUIRE_EQ(second_page.table_candidates.size(), 1U);

    const auto &first_table = first_page.table_candidates.front();
    const auto &second_table = second_page.table_candidates.front();
    REQUIRE_EQ(first_table.column_anchor_x_points.size(), 4U);
    REQUIRE_EQ(second_table.column_anchor_x_points.size(), 4U);
    REQUIRE_EQ(first_table.rows.size(), 4U);
    REQUIRE_EQ(second_table.rows.size(), 4U);
    REQUIRE_EQ(second_table.rows[1].cells.size(), 4U);

    CHECK(featherdoc::test_support::contains_text(
        second_table.rows[1].cells[0].text, "Sparse evidence"));
    CHECK_FALSE(second_table.rows[1].cells[1].has_text);
    CHECK_FALSE(second_table.rows[1].cells[2].has_text);
    CHECK(second_table.rows[1].cells[3].has_text);
    CHECK(featherdoc::test_support::contains_text(
        second_table.rows[1].cells[3].text, "USD 10"));
    CHECK(featherdoc::test_support::contains_text(
        second_table.rows[2].cells[1].text, "1"));
    CHECK(featherdoc::test_support::contains_text(
        second_table.rows[2].cells[2].text, "USD 15"));
    CHECK_EQ(second_table.rows[3].cells[0].column_span, 3U);
    CHECK(featherdoc::test_support::contains_text(
        second_table.rows[3].cells[0].text, "Grand total"));
}

TEST_CASE(
    "PDFium parser keeps cross-page subtotal amount-only body rows aligned") {
    const auto output_path = featherdoc::test_support::
        write_invoice_grid_pagebreak_subtotal_amount_only_body_pdf(
            "featherdoc-pdf-import-pagebreak-subtotal-amount-only-body-table-source.pdf");

    featherdoc::pdf::PdfiumParser parser;
    const auto parse_result = parser.parse(output_path, {});
    REQUIRE_MESSAGE(parse_result.success, parse_result.error_message);
    REQUIRE_EQ(parse_result.document.pages.size(), 2U);

    const auto &first_page = parse_result.document.pages[0];
    const auto &second_page = parse_result.document.pages[1];
    REQUIRE_EQ(first_page.table_candidates.size(), 1U);
    REQUIRE_EQ(second_page.table_candidates.size(), 1U);

    const auto &first_table = first_page.table_candidates.front();
    const auto &second_table = second_page.table_candidates.front();
    REQUIRE_EQ(first_table.column_anchor_x_points.size(), 4U);
    REQUIRE_EQ(second_table.column_anchor_x_points.size(), 4U);
    REQUIRE_EQ(first_table.rows.size(), 4U);
    REQUIRE_EQ(second_table.rows.size(), 4U);
    REQUIRE_EQ(second_table.rows[1].cells.size(), 4U);

    CHECK_FALSE(second_table.rows[1].cells[0].has_text);
    CHECK_FALSE(second_table.rows[1].cells[1].has_text);
    CHECK_FALSE(second_table.rows[1].cells[2].has_text);
    CHECK(second_table.rows[1].cells[3].has_text);
    CHECK(featherdoc::test_support::contains_text(
        second_table.rows[1].cells[3].text, "USD 10"));
    CHECK(featherdoc::test_support::contains_text(
        second_table.rows[2].cells[0].text, "Documentation pass"));
    CHECK(featherdoc::test_support::contains_text(
        second_table.rows[2].cells[2].text, "USD 15"));
    CHECK_EQ(second_table.rows[3].cells[0].column_span, 3U);
    CHECK(featherdoc::test_support::contains_text(
        second_table.rows[3].cells[0].text, "Grand total"));
}

TEST_CASE(
    "PDFium parser keeps cross-page subtotal isolated amount-only body rows aligned") {
    const auto output_path = featherdoc::test_support::
        write_invoice_grid_pagebreak_subtotal_isolated_amount_only_body_pdf(
            "featherdoc-pdf-import-pagebreak-subtotal-isolated-amount-only-body-table-source.pdf");

    featherdoc::pdf::PdfiumParser parser;
    const auto parse_result = parser.parse(output_path, {});
    REQUIRE_MESSAGE(parse_result.success, parse_result.error_message);
    REQUIRE_EQ(parse_result.document.pages.size(), 2U);

    const auto &first_page = parse_result.document.pages[0];
    const auto &second_page = parse_result.document.pages[1];
    REQUIRE_EQ(first_page.table_candidates.size(), 1U);
    REQUIRE_EQ(second_page.table_candidates.size(), 1U);

    const auto &first_table = first_page.table_candidates.front();
    const auto &second_table = second_page.table_candidates.front();
    REQUIRE_EQ(first_table.column_anchor_x_points.size(), 4U);
    REQUIRE_EQ(second_table.column_anchor_x_points.size(), 4U);
    REQUIRE_EQ(first_table.rows.size(), 4U);
    REQUIRE_EQ(second_table.rows.size(), 3U);
    REQUIRE_EQ(second_table.rows[1].cells.size(), 4U);

    CHECK_FALSE(second_table.rows[1].cells[0].has_text);
    CHECK_FALSE(second_table.rows[1].cells[1].has_text);
    CHECK_FALSE(second_table.rows[1].cells[2].has_text);
    CHECK(second_table.rows[1].cells[3].has_text);
    CHECK(featherdoc::test_support::contains_text(
        second_table.rows[1].cells[3].text, "USD 10"));
    CHECK_EQ(second_table.rows[2].cells[0].column_span, 3U);
    CHECK(featherdoc::test_support::contains_text(
        second_table.rows[2].cells[0].text, "Grand total"));
    CHECK(featherdoc::test_support::contains_text(
        second_table.rows[2].cells[3].text, "USD 110"));
}
