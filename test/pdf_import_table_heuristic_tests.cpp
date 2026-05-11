#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "pdf_import_test_support.hpp"

#include <featherdoc/pdf/pdf_document_importer.hpp>
#include <featherdoc/pdf/pdf_parser.hpp>

#include <cstdlib>
#include <filesystem>

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

TEST_CASE("PDF text importer can opt in to table candidate import") {
    const auto input_path =
        featherdoc::test_support::write_table_like_grid_pdf(
            "featherdoc-pdf-import-table-opt-in.pdf");
    const auto docx_path =
        std::filesystem::current_path() / "featherdoc-pdf-import-table.docx";
    std::filesystem::remove(docx_path);

    featherdoc::Document document(docx_path);
    featherdoc::pdf::PdfDocumentImportOptions options;
    options.import_table_candidates_as_tables = true;

    const auto import_result =
        featherdoc::pdf::import_pdf_text_document(input_path, document, options);
    REQUIRE_MESSAGE(import_result.success, import_result.error_message);
    CHECK_EQ(import_result.failure_kind,
             featherdoc::pdf::PdfDocumentImportFailureKind::none);
    CHECK_EQ(import_result.paragraphs_imported, 1U);
    CHECK_EQ(import_result.tables_imported, 1U);
    CHECK_EQ(featherdoc::test_support::collect_document_text(document),
             "Grid sample header\n");

    const auto imported_table = document.inspect_table(0U);
    REQUIRE(imported_table.has_value());
    CHECK_EQ(imported_table->row_count, 3U);
    CHECK_EQ(imported_table->column_count, 3U);
    REQUIRE_EQ(imported_table->row_repeats_header.size(), 3U);
    CHECK_FALSE(imported_table->row_repeats_header[0]);
    REQUIRE(imported_table->style_id.has_value());
    CHECK_EQ(*imported_table->style_id, "TableGrid");

    const auto cell_a1 = document.inspect_table_cell(0U, 0U, 0U);
    const auto cell_b2 = document.inspect_table_cell(0U, 1U, 1U);
    const auto cell_c3 = document.inspect_table_cell(0U, 2U, 2U);
    REQUIRE(cell_a1.has_value());
    REQUIRE(cell_b2.has_value());
    REQUIRE(cell_c3.has_value());
    CHECK_EQ(cell_a1->text, "Cell A1");
    CHECK_EQ(cell_b2->text, "Cell B2");
    CHECK_EQ(cell_c3->text, "Cell C3");

    REQUIRE_FALSE(document.save());

    featherdoc::Document reopened(docx_path);
    REQUIRE_FALSE(reopened.open());
    const auto reopened_table = reopened.inspect_table(0U);
    REQUIRE(reopened_table.has_value());
    CHECK_EQ(reopened_table->row_count, 3U);
    CHECK_EQ(reopened_table->column_count, 3U);
    REQUIRE_EQ(reopened_table->row_repeats_header.size(), 3U);
    CHECK_FALSE(reopened_table->row_repeats_header[0]);
    CHECK_EQ(reopened_table->text, "Cell A1\t\t\n\tCell B2\t\n\t\tCell C3");

    // 保留该样本的导入 DOCX，供 E7 视觉验证复用；后续同名回归会覆盖它。
}

TEST_CASE("PDF text importer can opt in to invoice grid table import") {
    const auto input_path =
        featherdoc::test_support::write_invoice_grid_pdf(
            "featherdoc-pdf-import-invoice-grid.pdf");
    const auto docx_path =
        std::filesystem::current_path() / "featherdoc-pdf-import-invoice-grid.docx";
    std::filesystem::remove(docx_path);

    featherdoc::Document document(docx_path);
    featherdoc::pdf::PdfDocumentImportOptions options;
    options.import_table_candidates_as_tables = true;

    const auto import_result =
        featherdoc::pdf::import_pdf_text_document(input_path, document, options);
    REQUIRE_MESSAGE(import_result.success, import_result.error_message);
    CHECK_EQ(import_result.failure_kind,
             featherdoc::pdf::PdfDocumentImportFailureKind::none);
    CHECK_EQ(import_result.paragraphs_imported, 2U);
    CHECK_EQ(import_result.tables_imported, 1U);
    REQUIRE_EQ(import_result.table_continuation_diagnostics.size(), 1U);
    CHECK_EQ(import_result.table_continuation_diagnostics[0].disposition,
             featherdoc::pdf::PdfTableContinuationDisposition::created_new_table);
    CHECK_EQ(import_result.table_continuation_diagnostics[0].blocker,
             featherdoc::pdf::PdfTableContinuationBlocker::no_previous_table);

    const auto blocks = document.inspect_body_blocks();
    REQUIRE_EQ(blocks.size(), 3U);
    CHECK_EQ(blocks[0].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(blocks[0].block_index, 0U);
    CHECK_EQ(blocks[0].item_index, 0U);
    CHECK_EQ(blocks[1].kind, featherdoc::body_block_kind::table);
    CHECK_EQ(blocks[1].block_index, 1U);
    CHECK_EQ(blocks[1].item_index, 0U);
    CHECK_EQ(blocks[2].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(blocks[2].block_index, 2U);
    CHECK_EQ(blocks[2].item_index, 1U);
    CHECK_EQ(featherdoc::test_support::collect_document_text(document),
             "Invoice grid sample\n"
             "Footer note: invoice grid is intentionally regular\n");

    const auto imported_table = document.inspect_table(0U);
    REQUIRE(imported_table.has_value());
    CHECK_EQ(imported_table->row_count, 5U);
    CHECK_EQ(imported_table->column_count, 4U);
    REQUIRE_EQ(imported_table->row_repeats_header.size(), 5U);
    CHECK(imported_table->row_repeats_header[0]);
    CHECK_FALSE(imported_table->row_repeats_header[1]);
    CHECK(featherdoc::test_support::contains_text(imported_table->text,
                                                  "Item"));
    CHECK(featherdoc::test_support::contains_text(imported_table->text,
                                                  "Qty"));
    CHECK(featherdoc::test_support::contains_text(imported_table->text,
                                                  "Unit"));
    CHECK(featherdoc::test_support::contains_text(imported_table->text,
                                                  "Total"));
    CHECK(featherdoc::test_support::contains_text(imported_table->text,
                                                  "PDF export design"));
    CHECK(featherdoc::test_support::contains_text(imported_table->text,
                                                  "Visual validation"));
    CHECK(featherdoc::test_support::contains_text(imported_table->text,
                                                  "Regression evidence"));
    CHECK(featherdoc::test_support::contains_text(imported_table->text,
                                                  "Grand total"));
    CHECK(featherdoc::test_support::contains_text(imported_table->text,
                                                  "USD 135"));

    REQUIRE_FALSE(document.save());

    featherdoc::Document reopened(docx_path);
    REQUIRE_FALSE(reopened.open());
    const auto reopened_blocks = reopened.inspect_body_blocks();
    REQUIRE_EQ(reopened_blocks.size(), 3U);
    CHECK_EQ(reopened_blocks[0].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(reopened_blocks[1].kind, featherdoc::body_block_kind::table);
    CHECK_EQ(reopened_blocks[2].kind, featherdoc::body_block_kind::paragraph);
    const auto reopened_table = reopened.inspect_table(0U);
    REQUIRE(reopened_table.has_value());
    REQUIRE_EQ(reopened_table->row_repeats_header.size(), 5U);
    CHECK(reopened_table->row_repeats_header[0]);
    CHECK_FALSE(reopened_table->row_repeats_header[1]);

    if (std::getenv("FEATHERDOC_KEEP_PDF_IMPORT_TEST_OUTPUTS") == nullptr) {
        std::filesystem::remove(docx_path);
    }
}

TEST_CASE("PDF text importer preserves top-left two-by-two merged table cells") {
    const auto input_path =
        featherdoc::test_support::write_paragraph_merged_corner_table_paragraph_pdf(
            "featherdoc-pdf-import-merged-corner-table.pdf");
    const auto docx_path =
        std::filesystem::current_path() /
        "featherdoc-pdf-import-merged-corner-table.docx";
    std::filesystem::remove(docx_path);

    featherdoc::Document document(docx_path);
    featherdoc::pdf::PdfDocumentImportOptions options;
    options.import_table_candidates_as_tables = true;

    const auto import_result =
        featherdoc::pdf::import_pdf_text_document(input_path, document, options);
    REQUIRE_MESSAGE(import_result.success, import_result.error_message);
    CHECK_EQ(import_result.failure_kind,
             featherdoc::pdf::PdfDocumentImportFailureKind::none);
    CHECK_EQ(import_result.paragraphs_imported, 2U);
    CHECK_EQ(import_result.tables_imported, 1U);
    REQUIRE_EQ(import_result.table_continuation_diagnostics.size(), 1U);
    CHECK_EQ(import_result.table_continuation_diagnostics[0].disposition,
             featherdoc::pdf::PdfTableContinuationDisposition::created_new_table);
    CHECK_EQ(import_result.table_continuation_diagnostics[0].blocker,
             featherdoc::pdf::PdfTableContinuationBlocker::no_previous_table);

    const auto blocks = document.inspect_body_blocks();
    REQUIRE_EQ(blocks.size(), 3U);
    CHECK_EQ(blocks[0].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(blocks[1].kind, featherdoc::body_block_kind::table);
    CHECK_EQ(blocks[2].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(featherdoc::test_support::collect_document_text(document),
             "Intro paragraph before merged corner table\n"
             "Tail paragraph after merged corner table\n");

    const auto imported_table = document.inspect_table(0U);
    REQUIRE(imported_table.has_value());
    CHECK_EQ(imported_table->row_count, 3U);
    CHECK_EQ(imported_table->column_count, 4U);
    CHECK_EQ(document.inspect_table_cells(0U).size(), 10U);

    const auto merged_anchor = document.inspect_table_cell(0U, 0U, 0U);
    const auto right_continuation =
        document.inspect_table_cell_by_grid_column(0U, 0U, 1U);
    const auto lower_continuation = document.inspect_table_cell(0U, 1U, 0U);
    const auto lower_right_continuation =
        document.inspect_table_cell_by_grid_column(0U, 1U, 1U);
    const auto alpha_cell = document.inspect_table_cell_by_grid_column(0U, 1U, 2U);
    const auto final_label = document.inspect_table_cell(0U, 2U, 0U);
    const auto final_phase =
        document.inspect_table_cell_by_grid_column(0U, 2U, 3U);
    REQUIRE(merged_anchor.has_value());
    REQUIRE(right_continuation.has_value());
    REQUIRE(lower_continuation.has_value());
    REQUIRE(lower_right_continuation.has_value());
    REQUIRE(alpha_cell.has_value());
    REQUIRE(final_label.has_value());
    REQUIRE(final_phase.has_value());
    CHECK_EQ(merged_anchor->column_span, 2U);
    CHECK_EQ(merged_anchor->row_span, 2U);
    CHECK_EQ(merged_anchor->vertical_merge,
             featherdoc::cell_vertical_merge::restart);
    CHECK(featherdoc::test_support::contains_text(
        merged_anchor->text, "Project overview and owner assignment"));
    CHECK_EQ(right_continuation->column_span, 2U);
    CHECK_EQ(right_continuation->row_span, 2U);
    CHECK_EQ(right_continuation->vertical_merge,
             featherdoc::cell_vertical_merge::restart);
    CHECK(featherdoc::test_support::contains_text(
        right_continuation->text, "Project overview and owner assignment"));
    CHECK_EQ(lower_continuation->column_span, 2U);
    CHECK_EQ(lower_continuation->vertical_merge,
             featherdoc::cell_vertical_merge::continue_merge);
    CHECK_EQ(lower_continuation->text, "");
    CHECK_EQ(lower_right_continuation->column_span, 2U);
    CHECK_EQ(lower_right_continuation->vertical_merge,
             featherdoc::cell_vertical_merge::continue_merge);
    CHECK_EQ(lower_right_continuation->text, "");
    CHECK(featherdoc::test_support::contains_text(alpha_cell->text, "Alpha"));
    CHECK(featherdoc::test_support::contains_text(final_label->text,
                                                  "Milestone"));
    CHECK(featherdoc::test_support::contains_text(final_phase->text, "Closed"));

    REQUIRE_FALSE(document.save());

    featherdoc::Document reopened(docx_path);
    REQUIRE_FALSE(reopened.open());
    const auto reopened_blocks = reopened.inspect_body_blocks();
    REQUIRE_EQ(reopened_blocks.size(), 3U);
    CHECK_EQ(reopened_blocks[0].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(reopened_blocks[1].kind, featherdoc::body_block_kind::table);
    CHECK_EQ(reopened_blocks[2].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(featherdoc::test_support::collect_document_text(reopened),
             "Intro paragraph before merged corner table\n"
             "Tail paragraph after merged corner table\n");
    CHECK_EQ(reopened.inspect_table_cells(0U).size(), 10U);

    const auto reopened_anchor = reopened.inspect_table_cell(0U, 0U, 0U);
    const auto reopened_right_continuation =
        reopened.inspect_table_cell_by_grid_column(0U, 0U, 1U);
    const auto reopened_lower_continuation =
        reopened.inspect_table_cell(0U, 1U, 0U);
    const auto reopened_lower_right_continuation =
        reopened.inspect_table_cell_by_grid_column(0U, 1U, 1U);
    const auto reopened_final_phase =
        reopened.inspect_table_cell_by_grid_column(0U, 2U, 3U);
    REQUIRE(reopened_anchor.has_value());
    REQUIRE(reopened_right_continuation.has_value());
    REQUIRE(reopened_lower_continuation.has_value());
    REQUIRE(reopened_lower_right_continuation.has_value());
    REQUIRE(reopened_final_phase.has_value());
    CHECK_EQ(reopened_anchor->column_span, 2U);
    CHECK_EQ(reopened_anchor->row_span, 2U);
    CHECK_EQ(reopened_anchor->vertical_merge,
             featherdoc::cell_vertical_merge::restart);
    CHECK_EQ(reopened_right_continuation->column_span, 2U);
    CHECK_EQ(reopened_right_continuation->row_span, 2U);
    CHECK_EQ(reopened_right_continuation->vertical_merge,
             featherdoc::cell_vertical_merge::restart);
    CHECK_EQ(reopened_lower_continuation->column_span, 2U);
    CHECK_EQ(reopened_lower_continuation->vertical_merge,
             featherdoc::cell_vertical_merge::continue_merge);
    CHECK_EQ(reopened_lower_continuation->text, "");
    CHECK_EQ(reopened_lower_right_continuation->column_span, 2U);
    CHECK_EQ(reopened_lower_right_continuation->vertical_merge,
             featherdoc::cell_vertical_merge::continue_merge);
    CHECK_EQ(reopened_lower_right_continuation->text, "");
    CHECK(featherdoc::test_support::contains_text(reopened_final_phase->text,
                                                  "Closed"));

    if (std::getenv("FEATHERDOC_KEEP_PDF_IMPORT_TEST_OUTPUTS") == nullptr) {
        std::filesystem::remove(docx_path);
    }
}

TEST_CASE("PDF text importer preserves horizontal merged table cells") {
    const auto input_path =
        featherdoc::test_support::
            write_paragraph_merged_header_table_paragraph_pdf(
                "featherdoc-pdf-import-merged-table.pdf");
    const auto docx_path =
        std::filesystem::current_path() / "featherdoc-pdf-import-merged-table.docx";
    std::filesystem::remove(docx_path);

    featherdoc::Document document(docx_path);
    featherdoc::pdf::PdfDocumentImportOptions options;
    options.import_table_candidates_as_tables = true;

    const auto import_result =
        featherdoc::pdf::import_pdf_text_document(input_path, document, options);
    REQUIRE_MESSAGE(import_result.success, import_result.error_message);
    CHECK_EQ(import_result.failure_kind,
             featherdoc::pdf::PdfDocumentImportFailureKind::none);
    CHECK_EQ(import_result.paragraphs_imported, 2U);
    CHECK_EQ(import_result.tables_imported, 1U);
    REQUIRE_EQ(import_result.table_continuation_diagnostics.size(), 1U);
    CHECK_EQ(import_result.table_continuation_diagnostics[0].disposition,
             featherdoc::pdf::PdfTableContinuationDisposition::created_new_table);
    CHECK_EQ(import_result.table_continuation_diagnostics[0].blocker,
             featherdoc::pdf::PdfTableContinuationBlocker::no_previous_table);

    const auto blocks = document.inspect_body_blocks();
    REQUIRE_EQ(blocks.size(), 3U);
    CHECK_EQ(blocks[0].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(blocks[1].kind, featherdoc::body_block_kind::table);
    CHECK_EQ(blocks[2].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(featherdoc::test_support::collect_document_text(document),
             "Intro paragraph before merged table\n"
             "Tail paragraph after merged table\n");

    const auto imported_table = document.inspect_table(0U);
    REQUIRE(imported_table.has_value());
    CHECK_EQ(imported_table->row_count, 3U);
    CHECK_EQ(imported_table->column_count, 3U);
    CHECK_EQ(document.inspect_table_cells(0U).size(), 8U);

    const auto merged_header_cell = document.inspect_table_cell(0U, 0U, 0U);
    const auto covered_header_cell =
        document.inspect_table_cell_by_grid_column(0U, 0U, 1U);
    const auto status_cell =
        document.inspect_table_cell_by_grid_column(0U, 0U, 2U);
    const auto owner_cell =
        document.inspect_table_cell_by_grid_column(0U, 1U, 1U);
    REQUIRE(merged_header_cell.has_value());
    REQUIRE(covered_header_cell.has_value());
    REQUIRE(status_cell.has_value());
    REQUIRE(owner_cell.has_value());
    CHECK_EQ(merged_header_cell->column_span, 2U);
    CHECK_EQ(covered_header_cell->cell_index, 0U);
    CHECK_EQ(covered_header_cell->column_span, 2U);
    CHECK(featherdoc::test_support::contains_text(
        merged_header_cell->text, "Merged header spans two cols"));
    CHECK_EQ(status_cell->column_span, 1U);
    CHECK(featherdoc::test_support::contains_text(status_cell->text, "Status"));
    CHECK(featherdoc::test_support::contains_text(owner_cell->text, "Alice"));

    REQUIRE_FALSE(document.save());

    featherdoc::Document reopened(docx_path);
    REQUIRE_FALSE(reopened.open());
    const auto reopened_blocks = reopened.inspect_body_blocks();
    REQUIRE_EQ(reopened_blocks.size(), 3U);
    CHECK_EQ(reopened_blocks[0].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(reopened_blocks[1].kind, featherdoc::body_block_kind::table);
    CHECK_EQ(reopened_blocks[2].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(featherdoc::test_support::collect_document_text(reopened),
             "Intro paragraph before merged table\n"
             "Tail paragraph after merged table\n");
    CHECK_EQ(reopened.inspect_table_cells(0U).size(), 8U);

    const auto reopened_merged_header =
        reopened.inspect_table_cell_by_grid_column(0U, 0U, 1U);
    const auto reopened_status =
        reopened.inspect_table_cell_by_grid_column(0U, 0U, 2U);
    REQUIRE(reopened_merged_header.has_value());
    REQUIRE(reopened_status.has_value());
    CHECK_EQ(reopened_merged_header->column_span, 2U);
    CHECK(featherdoc::test_support::contains_text(
        reopened_merged_header->text, "Merged header spans two cols"));
    CHECK(featherdoc::test_support::contains_text(reopened_status->text,
                                                  "Status"));

    // 保留该样本的导入 DOCX，供 E7 视觉验证复用；后续同名回归会覆盖它。
}

TEST_CASE("PDF text importer preserves vertical merged table cells") {
    const auto input_path =
        featherdoc::test_support::write_paragraph_vertical_merged_table_paragraph_pdf(
            "featherdoc-pdf-import-vertical-merged-table.pdf");
    const auto docx_path = std::filesystem::current_path() /
                           "featherdoc-pdf-import-vertical-merged-table.docx";
    std::filesystem::remove(docx_path);

    featherdoc::Document document(docx_path);
    featherdoc::pdf::PdfDocumentImportOptions options;
    options.import_table_candidates_as_tables = true;

    const auto import_result =
        featherdoc::pdf::import_pdf_text_document(input_path, document, options);
    REQUIRE_MESSAGE(import_result.success, import_result.error_message);
    CHECK_EQ(import_result.failure_kind,
             featherdoc::pdf::PdfDocumentImportFailureKind::none);
    CHECK_EQ(import_result.paragraphs_imported, 2U);
    CHECK_EQ(import_result.tables_imported, 1U);
    REQUIRE_EQ(import_result.table_continuation_diagnostics.size(), 1U);
    CHECK_EQ(import_result.table_continuation_diagnostics[0].disposition,
             featherdoc::pdf::PdfTableContinuationDisposition::created_new_table);
    CHECK_EQ(import_result.table_continuation_diagnostics[0].blocker,
             featherdoc::pdf::PdfTableContinuationBlocker::no_previous_table);

    const auto blocks = document.inspect_body_blocks();
    REQUIRE_EQ(blocks.size(), 3U);
    CHECK_EQ(blocks[0].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(blocks[1].kind, featherdoc::body_block_kind::table);
    CHECK_EQ(blocks[2].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(featherdoc::test_support::collect_document_text(document),
             "Intro paragraph before vertical merged table\n"
             "Tail paragraph after vertical merged table\n");

    const auto imported_table = document.inspect_table(0U);
    REQUIRE(imported_table.has_value());
    CHECK_EQ(imported_table->row_count, 3U);
    CHECK_EQ(imported_table->column_count, 3U);
    CHECK_EQ(document.inspect_table_cells(0U).size(), 9U);

    const auto merged_anchor = document.inspect_table_cell(0U, 0U, 0U);
    const auto merged_continuation = document.inspect_table_cell(0U, 1U, 0U);
    const auto alpha_owner =
        document.inspect_table_cell_by_grid_column(0U, 1U, 1U);
    const auto final_label = document.inspect_table_cell(0U, 2U, 0U);
    REQUIRE(merged_anchor.has_value());
    REQUIRE(merged_continuation.has_value());
    REQUIRE(alpha_owner.has_value());
    REQUIRE(final_label.has_value());
    CHECK_EQ(merged_anchor->row_span, 2U);
    CHECK_EQ(merged_anchor->vertical_merge,
             featherdoc::cell_vertical_merge::restart);
    CHECK(featherdoc::test_support::contains_text(merged_anchor->text,
                                                  "Project"));
    CHECK_EQ(merged_continuation->vertical_merge,
             featherdoc::cell_vertical_merge::continue_merge);
    CHECK_EQ(merged_continuation->text, "");
    CHECK(featherdoc::test_support::contains_text(alpha_owner->text, "Alpha"));
    CHECK(featherdoc::test_support::contains_text(final_label->text,
                                                  "Milestone"));

    REQUIRE_FALSE(document.save());

    featherdoc::Document reopened(docx_path);
    REQUIRE_FALSE(reopened.open());
    const auto reopened_blocks = reopened.inspect_body_blocks();
    REQUIRE_EQ(reopened_blocks.size(), 3U);
    CHECK_EQ(reopened_blocks[0].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(reopened_blocks[1].kind, featherdoc::body_block_kind::table);
    CHECK_EQ(reopened_blocks[2].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(featherdoc::test_support::collect_document_text(reopened),
             "Intro paragraph before vertical merged table\n"
             "Tail paragraph after vertical merged table\n");

    const auto reopened_anchor = reopened.inspect_table_cell(0U, 0U, 0U);
    const auto reopened_continuation = reopened.inspect_table_cell(0U, 1U, 0U);
    const auto reopened_final_label = reopened.inspect_table_cell(0U, 2U, 0U);
    REQUIRE(reopened_anchor.has_value());
    REQUIRE(reopened_continuation.has_value());
    REQUIRE(reopened_final_label.has_value());
    CHECK_EQ(reopened_anchor->row_span, 2U);
    CHECK_EQ(reopened_anchor->vertical_merge,
             featherdoc::cell_vertical_merge::restart);
    CHECK_EQ(reopened_continuation->vertical_merge,
             featherdoc::cell_vertical_merge::continue_merge);
    CHECK_EQ(reopened_continuation->text, "");
    CHECK(featherdoc::test_support::contains_text(reopened_final_label->text,
                                                  "Milestone"));

    if (std::getenv("FEATHERDOC_KEEP_PDF_IMPORT_TEST_OUTPUTS") == nullptr) {
        std::filesystem::remove(docx_path);
    }
}

TEST_CASE("PDF text importer preserves multi-row middle-column merged table cells") {
    const auto input_path =
        featherdoc::test_support::
            write_paragraph_middle_column_merged_table_paragraph_pdf(
                "featherdoc-pdf-import-middle-column-merged-table.pdf");
    const auto docx_path =
        std::filesystem::current_path() /
        "featherdoc-pdf-import-middle-column-merged-table.docx";
    std::filesystem::remove(docx_path);

    featherdoc::Document document(docx_path);
    featherdoc::pdf::PdfDocumentImportOptions options;
    options.import_table_candidates_as_tables = true;

    const auto import_result =
        featherdoc::pdf::import_pdf_text_document(input_path, document, options);
    REQUIRE_MESSAGE(import_result.success, import_result.error_message);
    CHECK_EQ(import_result.failure_kind,
             featherdoc::pdf::PdfDocumentImportFailureKind::none);
    CHECK_EQ(import_result.paragraphs_imported, 2U);
    CHECK_EQ(import_result.tables_imported, 1U);
    REQUIRE_EQ(import_result.table_continuation_diagnostics.size(), 1U);
    CHECK_EQ(import_result.table_continuation_diagnostics[0].disposition,
             featherdoc::pdf::PdfTableContinuationDisposition::created_new_table);
    CHECK_EQ(import_result.table_continuation_diagnostics[0].blocker,
             featherdoc::pdf::PdfTableContinuationBlocker::no_previous_table);

    const auto blocks = document.inspect_body_blocks();
    REQUIRE_EQ(blocks.size(), 3U);
    CHECK_EQ(blocks[0].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(blocks[1].kind, featherdoc::body_block_kind::table);
    CHECK_EQ(blocks[2].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(featherdoc::test_support::collect_document_text(document),
             "Intro paragraph before vertical merged table\n"
             "Tail paragraph after vertical merged table\n");

    const auto imported_table = document.inspect_table(0U);
    REQUIRE(imported_table.has_value());
    CHECK_EQ(imported_table->row_count, 3U);
    CHECK_EQ(imported_table->column_count, 3U);
    CHECK_EQ(document.inspect_table_cells(0U).size(), 9U);

    const auto merged_anchor = document.inspect_table_cell(0U, 0U, 1U);
    const auto first_continuation = document.inspect_table_cell(0U, 1U, 1U);
    const auto second_continuation = document.inspect_table_cell(0U, 2U, 1U);
    const auto alpha_label = document.inspect_table_cell(0U, 1U, 0U);
    const auto final_status = document.inspect_table_cell(0U, 2U, 2U);
    REQUIRE(merged_anchor.has_value());
    REQUIRE(first_continuation.has_value());
    REQUIRE(second_continuation.has_value());
    REQUIRE(alpha_label.has_value());
    REQUIRE(final_status.has_value());
    CHECK_EQ(merged_anchor->row_span, 3U);
    CHECK_EQ(merged_anchor->vertical_merge,
             featherdoc::cell_vertical_merge::restart);
    CHECK(featherdoc::test_support::contains_text(merged_anchor->text, "Owner"));
    CHECK_EQ(first_continuation->vertical_merge,
             featherdoc::cell_vertical_merge::continue_merge);
    CHECK_EQ(first_continuation->text, "");
    CHECK_EQ(second_continuation->vertical_merge,
             featherdoc::cell_vertical_merge::continue_merge);
    CHECK_EQ(second_continuation->text, "");
    CHECK(featherdoc::test_support::contains_text(alpha_label->text, "Alpha"));
    CHECK(featherdoc::test_support::contains_text(final_status->text, "Done"));

    REQUIRE_FALSE(document.save());

    featherdoc::Document reopened(docx_path);
    REQUIRE_FALSE(reopened.open());
    const auto reopened_blocks = reopened.inspect_body_blocks();
    REQUIRE_EQ(reopened_blocks.size(), 3U);
    CHECK_EQ(reopened_blocks[0].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(reopened_blocks[1].kind, featherdoc::body_block_kind::table);
    CHECK_EQ(reopened_blocks[2].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(featherdoc::test_support::collect_document_text(reopened),
             "Intro paragraph before vertical merged table\n"
             "Tail paragraph after vertical merged table\n");

    const auto reopened_anchor = reopened.inspect_table_cell(0U, 0U, 1U);
    const auto reopened_first_continuation =
        reopened.inspect_table_cell(0U, 1U, 1U);
    const auto reopened_second_continuation =
        reopened.inspect_table_cell(0U, 2U, 1U);
    const auto reopened_final_status = reopened.inspect_table_cell(0U, 2U, 2U);
    REQUIRE(reopened_anchor.has_value());
    REQUIRE(reopened_first_continuation.has_value());
    REQUIRE(reopened_second_continuation.has_value());
    REQUIRE(reopened_final_status.has_value());
    CHECK_EQ(reopened_anchor->row_span, 3U);
    CHECK_EQ(reopened_anchor->vertical_merge,
             featherdoc::cell_vertical_merge::restart);
    CHECK_EQ(reopened_first_continuation->vertical_merge,
             featherdoc::cell_vertical_merge::continue_merge);
    CHECK_EQ(reopened_first_continuation->text, "");
    CHECK_EQ(reopened_second_continuation->vertical_merge,
             featherdoc::cell_vertical_merge::continue_merge);
    CHECK_EQ(reopened_second_continuation->text, "");
    CHECK(featherdoc::test_support::contains_text(reopened_final_status->text,
                                                  "Done"));

    if (std::getenv("FEATHERDOC_KEEP_PDF_IMPORT_TEST_OUTPUTS") == nullptr) {
        std::filesystem::remove(docx_path);
    }
}

TEST_CASE(
    "PDF table import merges cross-page repeated headers with merged header cells") {
    const auto input_path =
        featherdoc::test_support::
            write_paragraph_table_pagebreak_merged_repeated_header_table_paragraph_pdf(
                "featherdoc-pdf-import-pagebreak-merged-repeated-header-table.pdf");
    const auto docx_path =
        std::filesystem::current_path() /
        "featherdoc-pdf-import-pagebreak-merged-repeated-header-table.docx";
    std::filesystem::remove(docx_path);

    featherdoc::Document document(docx_path);
    featherdoc::pdf::PdfDocumentImportOptions options;
    options.import_table_candidates_as_tables = true;

    const auto import_result =
        featherdoc::pdf::import_pdf_text_document(input_path, document, options);
    REQUIRE_MESSAGE(import_result.success, import_result.error_message);
    CHECK_EQ(import_result.failure_kind,
             featherdoc::pdf::PdfDocumentImportFailureKind::none);
    CHECK_EQ(import_result.paragraphs_imported, 2U);
    CHECK_EQ(import_result.tables_imported, 1U);
    REQUIRE_EQ(import_result.table_continuation_diagnostics.size(), 2U);
    CHECK_EQ(import_result.table_continuation_diagnostics[1].disposition,
             featherdoc::pdf::PdfTableContinuationDisposition::merged_with_previous_table);
    CHECK_EQ(import_result.table_continuation_diagnostics[1].blocker,
             featherdoc::pdf::PdfTableContinuationBlocker::none);
    CHECK_EQ(import_result.table_continuation_diagnostics[1].source_row_offset,
             1U);

    const auto blocks = document.inspect_body_blocks();
    REQUIRE_EQ(blocks.size(), 3U);
    CHECK_EQ(blocks[0].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(blocks[1].kind, featherdoc::body_block_kind::table);
    CHECK_EQ(blocks[2].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(featherdoc::test_support::collect_document_text(document),
             "Merged repeated header source sample\n"
             "Footer note: merged repeated header should merge cleanly\n");

    const auto imported_table = document.inspect_table(0U);
    REQUIRE(imported_table.has_value());
    CHECK_EQ(imported_table->row_count, 5U);
    CHECK_EQ(imported_table->column_count, 3U);
    REQUIRE_EQ(imported_table->row_repeats_header.size(), 5U);
    CHECK(imported_table->row_repeats_header[0]);
    CHECK_FALSE(imported_table->row_repeats_header[1]);
    CHECK_FALSE(imported_table->row_repeats_header.back());
    CHECK_EQ(document.inspect_table_cells(0U).size(), 14U);

    const auto merged_header_cell = document.inspect_table_cell(0U, 0U, 0U);
    const auto repeated_header_cell =
        document.inspect_table_cell_by_grid_column(0U, 0U, 1U);
    const auto first_body_cell = document.inspect_table_cell(0U, 1U, 0U);
    const auto merged_tail_cell = document.inspect_table_cell(0U, 4U, 0U);
    REQUIRE(merged_header_cell.has_value());
    REQUIRE(repeated_header_cell.has_value());
    REQUIRE(first_body_cell.has_value());
    REQUIRE(merged_tail_cell.has_value());
    CHECK_EQ(merged_header_cell->column_span, 2U);
    CHECK_EQ(repeated_header_cell->column_span, 2U);
    CHECK(featherdoc::test_support::contains_text(
        merged_header_cell->text, "Merged header spans two cols"));
    CHECK(featherdoc::test_support::contains_text(first_body_cell->text,
                                                  "Feature alpha"));
    CHECK(featherdoc::test_support::contains_text(merged_tail_cell->text,
                                                  "Header dedupe"));

    REQUIRE_FALSE(document.save());

    featherdoc::Document reopened(docx_path);
    REQUIRE_FALSE(reopened.open());
    const auto reopened_blocks = reopened.inspect_body_blocks();
    REQUIRE_EQ(reopened_blocks.size(), 3U);
    CHECK_EQ(reopened_blocks[0].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(reopened_blocks[1].kind, featherdoc::body_block_kind::table);
    CHECK_EQ(reopened_blocks[2].kind, featherdoc::body_block_kind::paragraph);
    const auto reopened_table = reopened.inspect_table(0U);
    REQUIRE(reopened_table.has_value());
    CHECK_EQ(reopened_table->row_count, 5U);
    CHECK_EQ(reopened.inspect_table_cells(0U).size(), 14U);

    const auto reopened_merged_header =
        reopened.inspect_table_cell(0U, 0U, 0U);
    const auto reopened_repeated_header =
        reopened.inspect_table_cell_by_grid_column(0U, 0U, 1U);
    REQUIRE(reopened_merged_header.has_value());
    REQUIRE(reopened_repeated_header.has_value());
    CHECK_EQ(reopened_merged_header->column_span, 2U);
    CHECK_EQ(reopened_repeated_header->column_span, 2U);
    CHECK(featherdoc::test_support::contains_text(
        reopened_merged_header->text, "Merged header spans two cols"));

    if (std::getenv("FEATHERDOC_KEEP_PDF_IMPORT_TEST_OUTPUTS") == nullptr) {
        std::filesystem::remove(docx_path);
    }
}

TEST_CASE("PDF text importer marks repeated header rows on long invoice table") {
    const auto input_path =
        featherdoc::test_support::write_invoice_grid_repeat_header_pdf(
            "featherdoc-pdf-import-invoice-grid-repeat-header.pdf");
    const auto docx_path = std::filesystem::current_path() /
                           "featherdoc-pdf-import-invoice-grid-repeat-header.docx";
    std::filesystem::remove(docx_path);

    featherdoc::Document document(docx_path);
    featherdoc::pdf::PdfDocumentImportOptions options;
    options.import_table_candidates_as_tables = true;

    const auto import_result =
        featherdoc::pdf::import_pdf_text_document(input_path, document, options);
    REQUIRE_MESSAGE(import_result.success, import_result.error_message);
    CHECK_EQ(import_result.failure_kind,
             featherdoc::pdf::PdfDocumentImportFailureKind::none);
    CHECK_EQ(import_result.paragraphs_imported, 2U);
    CHECK_EQ(import_result.tables_imported, 1U);

    const auto blocks = document.inspect_body_blocks();
    REQUIRE_EQ(blocks.size(), 3U);
    CHECK_EQ(blocks[0].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(blocks[1].kind, featherdoc::body_block_kind::table);
    CHECK_EQ(blocks[2].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(featherdoc::test_support::collect_document_text(document),
             "Invoice repeat-header sample\n"
             "Footer note: repeated header continues\n");

    const auto imported_table = document.inspect_table(0U);
    REQUIRE(imported_table.has_value());
    CHECK_EQ(imported_table->row_count, 56U);
    CHECK_EQ(imported_table->column_count, 4U);
    REQUIRE_EQ(imported_table->row_height_twips.size(), 56U);
    REQUIRE_EQ(imported_table->row_height_rules.size(), 56U);
    REQUIRE(imported_table->row_height_twips[0].has_value());
    REQUIRE(imported_table->row_height_rules[0].has_value());
    CHECK_GT(*imported_table->row_height_twips[0], 400U);
    CHECK_EQ(*imported_table->row_height_rules[0],
             featherdoc::row_height_rule::exact);
    REQUIRE_EQ(imported_table->row_repeats_header.size(), 56U);
    CHECK(imported_table->row_repeats_header[0]);
    CHECK_FALSE(imported_table->row_repeats_header[1]);
    CHECK_FALSE(imported_table->row_repeats_header.back());
    CHECK(featherdoc::test_support::contains_text(imported_table->text,
                                                  "PDF export line item 1"));
    CHECK(featherdoc::test_support::contains_text(imported_table->text,
                                                  "PDF export line item 55"));

    REQUIRE_FALSE(document.save());

    featherdoc::Document reopened(docx_path);
    REQUIRE_FALSE(reopened.open());
    const auto reopened_table = reopened.inspect_table(0U);
    REQUIRE(reopened_table.has_value());
    CHECK_EQ(reopened_table->row_count, 56U);
    CHECK_EQ(reopened_table->column_count, 4U);
    REQUIRE_EQ(reopened_table->row_height_twips.size(), 56U);
    REQUIRE_EQ(reopened_table->row_height_rules.size(), 56U);
    REQUIRE(reopened_table->row_height_twips[0].has_value());
    REQUIRE(reopened_table->row_height_rules[0].has_value());
    CHECK_GT(*reopened_table->row_height_twips[0], 400U);
    CHECK_EQ(*reopened_table->row_height_rules[0],
             featherdoc::row_height_rule::exact);
    REQUIRE_EQ(reopened_table->row_repeats_header.size(), 56U);
    CHECK(reopened_table->row_repeats_header[0]);
    CHECK_FALSE(reopened_table->row_repeats_header[1]);
    CHECK_FALSE(reopened_table->row_repeats_header.back());

    if (std::getenv("FEATHERDOC_KEEP_PDF_IMPORT_TEST_OUTPUTS") == nullptr) {
        std::filesystem::remove(docx_path);
    }
}

TEST_CASE("PDF table import skips repeated source header rows while merging cross-page repeated-header table") {
    const auto input_path =
        featherdoc::test_support::
            write_paragraph_table_pagebreak_repeated_header_table_paragraph_pdf(
                "featherdoc-pdf-import-pagebreak-repeated-header-table.pdf");
    const auto docx_path =
        std::filesystem::current_path() /
        "featherdoc-pdf-import-pagebreak-repeated-header-table.docx";
    std::filesystem::remove(docx_path);

    featherdoc::Document document(docx_path);
    featherdoc::pdf::PdfDocumentImportOptions options;
    options.import_table_candidates_as_tables = true;

    const auto import_result =
        featherdoc::pdf::import_pdf_text_document(input_path, document, options);
    REQUIRE_MESSAGE(import_result.success, import_result.error_message);
    CHECK_EQ(import_result.failure_kind,
             featherdoc::pdf::PdfDocumentImportFailureKind::none);
    CHECK_EQ(import_result.paragraphs_imported, 2U);
    CHECK_EQ(import_result.tables_imported, 1U);

    const auto blocks = document.inspect_body_blocks();
    REQUIRE_EQ(blocks.size(), 3U);
    CHECK_EQ(blocks[0].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(blocks[1].kind, featherdoc::body_block_kind::table);
    CHECK_EQ(blocks[2].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(featherdoc::test_support::collect_document_text(document),
             "Repeated header source sample\n"
             "Footer note: repeated source header should merge cleanly\n");

    const auto imported_table = document.inspect_table(0U);
    REQUIRE(imported_table.has_value());
    CHECK_EQ(imported_table->row_count, 5U);
    CHECK_EQ(imported_table->column_count, 3U);
    REQUIRE_EQ(imported_table->row_repeats_header.size(), 5U);
    CHECK(imported_table->row_repeats_header[0]);
    CHECK_FALSE(imported_table->row_repeats_header[1]);
    CHECK_FALSE(imported_table->row_repeats_header[3]);
    CHECK_FALSE(imported_table->row_repeats_header.back());

    const auto header_cell = document.inspect_table_cell(0U, 0U, 0U);
    const auto boundary_cell_2 = document.inspect_table_cell(0U, 2U, 0U);
    const auto boundary_cell_3 = document.inspect_table_cell(0U, 3U, 0U);
    const auto boundary_cell_4 = document.inspect_table_cell(0U, 4U, 0U);
    REQUIRE(header_cell.has_value());
    REQUIRE(boundary_cell_2.has_value());
    REQUIRE(boundary_cell_3.has_value());
    REQUIRE(boundary_cell_4.has_value());
    CHECK(featherdoc::test_support::contains_text(header_cell->text, "Item"));
    CHECK(featherdoc::test_support::contains_text(boundary_cell_2->text,
                                                  "Metrics review"));
    CHECK(featherdoc::test_support::contains_text(boundary_cell_3->text,
                                                  "Invoice merge"));
    CHECK(featherdoc::test_support::contains_text(boundary_cell_4->text,
                                                  "Header dedupe"));
    CHECK_FALSE(document.inspect_table(1U).has_value());

    REQUIRE_FALSE(document.save());

    featherdoc::Document reopened(docx_path);
    REQUIRE_FALSE(reopened.open());
    const auto reopened_blocks = reopened.inspect_body_blocks();
    REQUIRE_EQ(reopened_blocks.size(), 3U);
    CHECK_EQ(reopened_blocks[0].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(reopened_blocks[1].kind, featherdoc::body_block_kind::table);
    CHECK_EQ(reopened_blocks[2].kind, featherdoc::body_block_kind::paragraph);

    const auto reopened_table = reopened.inspect_table(0U);
    REQUIRE(reopened_table.has_value());
    CHECK_EQ(reopened_table->row_count, 5U);
    CHECK_EQ(reopened_table->column_count, 3U);
    REQUIRE_EQ(reopened_table->row_repeats_header.size(), 5U);
    CHECK(reopened_table->row_repeats_header[0]);
    CHECK_FALSE(reopened_table->row_repeats_header[3]);
    CHECK_FALSE(reopened.inspect_table(1U).has_value());

    const auto reopened_boundary_cell_3 =
        reopened.inspect_table_cell(0U, 3U, 0U);
    const auto reopened_boundary_cell_4 =
        reopened.inspect_table_cell(0U, 4U, 0U);
    REQUIRE(reopened_boundary_cell_3.has_value());
    REQUIRE(reopened_boundary_cell_4.has_value());
    CHECK(featherdoc::test_support::contains_text(reopened_boundary_cell_3->text,
                                                  "Invoice merge"));
    CHECK(featherdoc::test_support::contains_text(reopened_boundary_cell_4->text,
                                                  "Header dedupe"));

    if (std::getenv("FEATHERDOC_KEEP_PDF_IMPORT_TEST_OUTPUTS") == nullptr) {
        std::filesystem::remove(docx_path);
    }
}

TEST_CASE(
    "PDF table import skips whitespace-varied repeated source header rows while merging cross-page repeated-header table") {
    const auto input_path =
        featherdoc::test_support::
            write_paragraph_table_pagebreak_repeated_header_whitespace_variant_pdf(
                "featherdoc-pdf-import-pagebreak-repeated-header-whitespace-variant.pdf");
    const auto docx_path =
        std::filesystem::current_path() /
        "featherdoc-pdf-import-pagebreak-repeated-header-whitespace-variant.docx";
    std::filesystem::remove(docx_path);

    featherdoc::Document document(docx_path);
    featherdoc::pdf::PdfDocumentImportOptions options;
    options.import_table_candidates_as_tables = true;

    const auto import_result =
        featherdoc::pdf::import_pdf_text_document(input_path, document, options);
    REQUIRE_MESSAGE(import_result.success, import_result.error_message);
    CHECK_EQ(import_result.failure_kind,
             featherdoc::pdf::PdfDocumentImportFailureKind::none);
    CHECK_EQ(import_result.paragraphs_imported, 2U);
    CHECK_EQ(import_result.tables_imported, 1U);

    const auto blocks = document.inspect_body_blocks();
    REQUIRE_EQ(blocks.size(), 3U);
    CHECK_EQ(blocks[0].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(blocks[1].kind, featherdoc::body_block_kind::table);
    CHECK_EQ(blocks[2].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(featherdoc::test_support::collect_document_text(document),
             "Repeated header whitespace variant sample\n"
             "Footer note: repeated source header whitespace should merge cleanly\n");

    const auto imported_table = document.inspect_table(0U);
    REQUIRE(imported_table.has_value());
    CHECK_EQ(imported_table->row_count, 5U);
    CHECK_EQ(imported_table->column_count, 3U);
    REQUIRE_EQ(imported_table->row_repeats_header.size(), 5U);
    CHECK(imported_table->row_repeats_header[0]);
    CHECK_FALSE(imported_table->row_repeats_header[1]);
    CHECK_FALSE(imported_table->row_repeats_header[3]);
    CHECK_FALSE(imported_table->row_repeats_header.back());

    const auto header_owner_cell = document.inspect_table_cell(0U, 0U, 1U);
    const auto header_status_cell = document.inspect_table_cell(0U, 0U, 2U);
    const auto boundary_cell_3 = document.inspect_table_cell(0U, 3U, 0U);
    const auto boundary_cell_4 = document.inspect_table_cell(0U, 4U, 0U);
    REQUIRE(header_owner_cell.has_value());
    REQUIRE(header_status_cell.has_value());
    REQUIRE(boundary_cell_3.has_value());
    REQUIRE(boundary_cell_4.has_value());
    CHECK(featherdoc::test_support::contains_text(header_owner_cell->text,
                                                  "Owner Name"));
    CHECK(featherdoc::test_support::contains_text(header_status_cell->text,
                                                  "Project Status"));
    CHECK(featherdoc::test_support::contains_text(boundary_cell_3->text,
                                                  "Invoice merge"));
    CHECK(featherdoc::test_support::contains_text(boundary_cell_4->text,
                                                  "Header dedupe"));
    CHECK_FALSE(document.inspect_table(1U).has_value());

    REQUIRE_FALSE(document.save());

    featherdoc::Document reopened(docx_path);
    REQUIRE_FALSE(reopened.open());
    const auto reopened_blocks = reopened.inspect_body_blocks();
    REQUIRE_EQ(reopened_blocks.size(), 3U);
    CHECK_EQ(reopened_blocks[0].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(reopened_blocks[1].kind, featherdoc::body_block_kind::table);
    CHECK_EQ(reopened_blocks[2].kind, featherdoc::body_block_kind::paragraph);

    const auto reopened_table = reopened.inspect_table(0U);
    REQUIRE(reopened_table.has_value());
    CHECK_EQ(reopened_table->row_count, 5U);
    CHECK_EQ(reopened_table->column_count, 3U);
    REQUIRE_EQ(reopened_table->row_repeats_header.size(), 5U);
    CHECK(reopened_table->row_repeats_header[0]);
    CHECK_FALSE(reopened_table->row_repeats_header[3]);
    CHECK_FALSE(reopened.inspect_table(1U).has_value());

    const auto reopened_boundary_cell_4 =
        reopened.inspect_table_cell(0U, 4U, 0U);
    REQUIRE(reopened_boundary_cell_4.has_value());
    CHECK(featherdoc::test_support::contains_text(reopened_boundary_cell_4->text,
                                                  "Header dedupe"));

    if (std::getenv("FEATHERDOC_KEEP_PDF_IMPORT_TEST_OUTPUTS") == nullptr) {
        std::filesystem::remove(docx_path);
    }
}

TEST_CASE(
    "PDF table import skips case and separator-varied repeated source header rows while merging cross-page repeated-header table") {
    const auto input_path =
        featherdoc::test_support::
            write_paragraph_table_pagebreak_repeated_header_text_variant_pdf(
                "featherdoc-pdf-import-pagebreak-repeated-header-text-variant.pdf");
    const auto docx_path =
        std::filesystem::current_path() /
        "featherdoc-pdf-import-pagebreak-repeated-header-text-variant.docx";
    std::filesystem::remove(docx_path);

    featherdoc::Document document(docx_path);
    featherdoc::pdf::PdfDocumentImportOptions options;
    options.import_table_candidates_as_tables = true;

    const auto import_result =
        featherdoc::pdf::import_pdf_text_document(input_path, document, options);
    REQUIRE_MESSAGE(import_result.success, import_result.error_message);
    CHECK_EQ(import_result.failure_kind,
             featherdoc::pdf::PdfDocumentImportFailureKind::none);
    CHECK_EQ(import_result.paragraphs_imported, 2U);
    CHECK_EQ(import_result.tables_imported, 1U);

    const auto blocks = document.inspect_body_blocks();
    REQUIRE_EQ(blocks.size(), 3U);
    CHECK_EQ(blocks[0].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(blocks[1].kind, featherdoc::body_block_kind::table);
    CHECK_EQ(blocks[2].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(featherdoc::test_support::collect_document_text(document),
             "Repeated header text variant sample\n"
             "Footer note: repeated source header text variant should merge cleanly\n");

    const auto imported_table = document.inspect_table(0U);
    REQUIRE(imported_table.has_value());
    CHECK_EQ(imported_table->row_count, 5U);
    CHECK_EQ(imported_table->column_count, 3U);
    REQUIRE_EQ(imported_table->row_repeats_header.size(), 5U);
    CHECK(imported_table->row_repeats_header[0]);
    CHECK_FALSE(imported_table->row_repeats_header[1]);
    CHECK_FALSE(imported_table->row_repeats_header[3]);
    CHECK_FALSE(imported_table->row_repeats_header.back());

    const auto header_item_cell = document.inspect_table_cell(0U, 0U, 0U);
    const auto header_owner_cell = document.inspect_table_cell(0U, 0U, 1U);
    const auto header_status_cell = document.inspect_table_cell(0U, 0U, 2U);
    const auto boundary_cell_3 = document.inspect_table_cell(0U, 3U, 0U);
    const auto boundary_cell_4 = document.inspect_table_cell(0U, 4U, 0U);
    REQUIRE(header_item_cell.has_value());
    REQUIRE(header_owner_cell.has_value());
    REQUIRE(header_status_cell.has_value());
    REQUIRE(boundary_cell_3.has_value());
    REQUIRE(boundary_cell_4.has_value());
    CHECK(featherdoc::test_support::contains_text(header_item_cell->text,
                                                  "Item"));
    CHECK(featherdoc::test_support::contains_text(header_owner_cell->text,
                                                  "Owner Name"));
    CHECK(featherdoc::test_support::contains_text(header_status_cell->text,
                                                  "Project Status"));
    CHECK(featherdoc::test_support::contains_text(boundary_cell_3->text,
                                                  "Invoice merge"));
    CHECK(featherdoc::test_support::contains_text(boundary_cell_4->text,
                                                  "Header dedupe"));
    CHECK_FALSE(document.inspect_table(1U).has_value());

    REQUIRE_FALSE(document.save());

    featherdoc::Document reopened(docx_path);
    REQUIRE_FALSE(reopened.open());
    const auto reopened_blocks = reopened.inspect_body_blocks();
    REQUIRE_EQ(reopened_blocks.size(), 3U);
    CHECK_EQ(reopened_blocks[0].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(reopened_blocks[1].kind, featherdoc::body_block_kind::table);
    CHECK_EQ(reopened_blocks[2].kind, featherdoc::body_block_kind::paragraph);

    const auto reopened_table = reopened.inspect_table(0U);
    REQUIRE(reopened_table.has_value());
    CHECK_EQ(reopened_table->row_count, 5U);
    CHECK_EQ(reopened_table->column_count, 3U);
    REQUIRE_EQ(reopened_table->row_repeats_header.size(), 5U);
    CHECK(reopened_table->row_repeats_header[0]);
    CHECK_FALSE(reopened_table->row_repeats_header[3]);
    CHECK_FALSE(reopened.inspect_table(1U).has_value());

    const auto reopened_boundary_cell_4 =
        reopened.inspect_table_cell(0U, 4U, 0U);
    REQUIRE(reopened_boundary_cell_4.has_value());
    CHECK(featherdoc::test_support::contains_text(reopened_boundary_cell_4->text,
                                                  "Header dedupe"));

    if (std::getenv("FEATHERDOC_KEEP_PDF_IMPORT_TEST_OUTPUTS") == nullptr) {
        std::filesystem::remove(docx_path);
    }
}

TEST_CASE(
    "PDF table import skips plural-varied repeated source header rows while merging cross-page repeated-header table") {
    const auto input_path =
        featherdoc::test_support::
            write_paragraph_table_pagebreak_repeated_header_plural_variant_pdf(
                "featherdoc-pdf-import-pagebreak-repeated-header-plural-variant.pdf");
    const auto docx_path =
        std::filesystem::current_path() /
        "featherdoc-pdf-import-pagebreak-repeated-header-plural-variant.docx";
    std::filesystem::remove(docx_path);

    featherdoc::Document document(docx_path);
    featherdoc::pdf::PdfDocumentImportOptions options;
    options.import_table_candidates_as_tables = true;

    const auto import_result =
        featherdoc::pdf::import_pdf_text_document(input_path, document, options);
    REQUIRE_MESSAGE(import_result.success, import_result.error_message);
    CHECK_EQ(import_result.failure_kind,
             featherdoc::pdf::PdfDocumentImportFailureKind::none);
    CHECK_EQ(import_result.paragraphs_imported, 2U);
    CHECK_EQ(import_result.tables_imported, 1U);

    const auto blocks = document.inspect_body_blocks();
    REQUIRE_EQ(blocks.size(), 3U);
    CHECK_EQ(blocks[0].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(blocks[1].kind, featherdoc::body_block_kind::table);
    CHECK_EQ(blocks[2].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(featherdoc::test_support::collect_document_text(document),
             "Repeated header plural variant sample\n"
             "Footer note: repeated source header plural variant should merge cleanly\n");

    const auto imported_table = document.inspect_table(0U);
    REQUIRE(imported_table.has_value());
    CHECK_EQ(imported_table->row_count, 5U);
    CHECK_EQ(imported_table->column_count, 3U);
    REQUIRE_EQ(imported_table->row_repeats_header.size(), 5U);
    CHECK(imported_table->row_repeats_header[0]);
    CHECK_FALSE(imported_table->row_repeats_header[1]);
    CHECK_FALSE(imported_table->row_repeats_header[3]);
    CHECK_FALSE(imported_table->row_repeats_header.back());

    const auto header_owner_cell = document.inspect_table_cell(0U, 0U, 1U);
    const auto header_status_cell = document.inspect_table_cell(0U, 0U, 2U);
    const auto boundary_cell_3 = document.inspect_table_cell(0U, 3U, 0U);
    const auto boundary_cell_4 = document.inspect_table_cell(0U, 4U, 0U);
    REQUIRE(header_owner_cell.has_value());
    REQUIRE(header_status_cell.has_value());
    REQUIRE(boundary_cell_3.has_value());
    REQUIRE(boundary_cell_4.has_value());
    CHECK(featherdoc::test_support::contains_text(header_owner_cell->text,
                                                  "Owner Name"));
    CHECK(featherdoc::test_support::contains_text(header_status_cell->text,
                                                  "Project Status"));
    CHECK(featherdoc::test_support::contains_text(boundary_cell_3->text,
                                                  "Invoice merge"));
    CHECK(featherdoc::test_support::contains_text(boundary_cell_4->text,
                                                  "Header dedupe"));
    CHECK_FALSE(document.inspect_table(1U).has_value());

    REQUIRE_FALSE(document.save());

    featherdoc::Document reopened(docx_path);
    REQUIRE_FALSE(reopened.open());
    const auto reopened_table = reopened.inspect_table(0U);
    REQUIRE(reopened_table.has_value());
    CHECK_EQ(reopened_table->row_count, 5U);
    CHECK_EQ(reopened_table->column_count, 3U);
    REQUIRE_EQ(reopened_table->row_repeats_header.size(), 5U);
    CHECK(reopened_table->row_repeats_header[0]);
    CHECK_FALSE(reopened_table->row_repeats_header[3]);
    CHECK_FALSE(reopened.inspect_table(1U).has_value());

    if (std::getenv("FEATHERDOC_KEEP_PDF_IMPORT_TEST_OUTPUTS") == nullptr) {
        std::filesystem::remove(docx_path);
    }
}

TEST_CASE(
    "PDF table import keeps semantic header variants as separate cross-page tables") {
    const auto input_path =
        featherdoc::test_support::
            write_paragraph_table_pagebreak_repeated_header_semantic_variant_pdf(
                "featherdoc-pdf-import-pagebreak-repeated-header-semantic-variant.pdf");
    const auto docx_path =
        std::filesystem::current_path() /
        "featherdoc-pdf-import-pagebreak-repeated-header-semantic-variant.docx";
    std::filesystem::remove(docx_path);

    featherdoc::Document document(docx_path);
    featherdoc::pdf::PdfDocumentImportOptions options;
    options.import_table_candidates_as_tables = true;

    const auto import_result =
        featherdoc::pdf::import_pdf_text_document(input_path, document, options);
    REQUIRE_MESSAGE(import_result.success, import_result.error_message);
    CHECK_EQ(import_result.failure_kind,
             featherdoc::pdf::PdfDocumentImportFailureKind::none);
    CHECK_EQ(import_result.paragraphs_imported, 2U);
    CHECK_EQ(import_result.tables_imported, 2U);
    REQUIRE_EQ(import_result.table_continuation_diagnostics.size(), 2U);
    CHECK_EQ(import_result.table_continuation_diagnostics[1].disposition,
             featherdoc::pdf::PdfTableContinuationDisposition::created_new_table);
    CHECK_EQ(import_result.table_continuation_diagnostics[1].blocker,
             featherdoc::pdf::PdfTableContinuationBlocker::repeated_header_mismatch);
    CHECK_EQ(import_result.table_continuation_diagnostics[1].source_row_offset,
             0U);

    const auto blocks = document.inspect_body_blocks();
    REQUIRE_EQ(blocks.size(), 4U);
    CHECK_EQ(blocks[0].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(blocks[1].kind, featherdoc::body_block_kind::table);
    CHECK_EQ(blocks[2].kind, featherdoc::body_block_kind::table);
    CHECK_EQ(blocks[3].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(featherdoc::test_support::collect_document_text(document),
             "Repeated header semantic variant sample\n"
             "Footer note: repeated source header semantic variant should keep a separate table\n");

    const auto first_table = document.inspect_table(0U);
    const auto second_table = document.inspect_table(1U);
    REQUIRE(first_table.has_value());
    REQUIRE(second_table.has_value());
    CHECK_EQ(first_table->row_count, 3U);
    CHECK_EQ(second_table->row_count, 3U);
    REQUIRE_EQ(first_table->row_repeats_header.size(), 3U);
    REQUIRE_EQ(second_table->row_repeats_header.size(), 3U);
    CHECK(first_table->row_repeats_header[0]);
    CHECK(second_table->row_repeats_header[0]);

    const auto second_header_owner_cell = document.inspect_table_cell(1U, 0U, 1U);
    const auto second_body_cell = document.inspect_table_cell(1U, 1U, 0U);
    REQUIRE(second_header_owner_cell.has_value());
    REQUIRE(second_body_cell.has_value());
    CHECK(featherdoc::test_support::contains_text(second_header_owner_cell->text,
                                                  "Owner Team"));
    CHECK(featherdoc::test_support::contains_text(second_body_cell->text,
                                                  "Invoice merge"));

    REQUIRE_FALSE(document.save());

    featherdoc::Document reopened(docx_path);
    REQUIRE_FALSE(reopened.open());
    const auto reopened_blocks = reopened.inspect_body_blocks();
    REQUIRE_EQ(reopened_blocks.size(), 4U);
    CHECK_EQ(reopened_blocks[0].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(reopened_blocks[1].kind, featherdoc::body_block_kind::table);
    CHECK_EQ(reopened_blocks[2].kind, featherdoc::body_block_kind::table);
    CHECK_EQ(reopened_blocks[3].kind, featherdoc::body_block_kind::paragraph);
    CHECK(reopened.inspect_table(0U).has_value());
    CHECK(reopened.inspect_table(1U).has_value());
    CHECK_FALSE(reopened.inspect_table(2U).has_value());

    if (std::getenv("FEATHERDOC_KEEP_PDF_IMPORT_TEST_OUTPUTS") == nullptr) {
        std::filesystem::remove(docx_path);
    }
}

TEST_CASE(
    "PDF table import merges long repeated-header source tables into one editable table") {
    const auto input_path =
        featherdoc::test_support::write_long_repeated_header_table_pdf(
            "featherdoc-pdf-import-long-repeated-header.pdf");
    const auto docx_path =
        std::filesystem::current_path() /
        "featherdoc-pdf-import-long-repeated-header.docx";
    std::filesystem::remove(docx_path);

    featherdoc::Document document(docx_path);
    featherdoc::pdf::PdfDocumentImportOptions options;
    options.import_table_candidates_as_tables = true;

    const auto import_result =
        featherdoc::pdf::import_pdf_text_document(input_path, document, options);
    REQUIRE_MESSAGE(import_result.success, import_result.error_message);
    CHECK_EQ(import_result.failure_kind,
             featherdoc::pdf::PdfDocumentImportFailureKind::none);
    CHECK_EQ(import_result.paragraphs_imported, 2U);
    CHECK_EQ(import_result.tables_imported, 1U);

    const auto blocks = document.inspect_body_blocks();
    REQUIRE_EQ(blocks.size(), 3U);
    CHECK_EQ(blocks[0].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(blocks[1].kind, featherdoc::body_block_kind::table);
    CHECK_EQ(blocks[2].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(featherdoc::test_support::collect_document_text(document),
             "Long repeated-header sample\n"
             "Footer note: long repeated-header sample\n");

    const auto imported_table = document.inspect_table(0U);
    REQUIRE(imported_table.has_value());
    CHECK_EQ(imported_table->row_count, 53U);
    CHECK_EQ(imported_table->column_count, 3U);
    REQUIRE_EQ(imported_table->row_repeats_header.size(), 53U);
    CHECK(imported_table->row_repeats_header[0]);
    CHECK_FALSE(imported_table->row_repeats_header[1]);
    CHECK_FALSE(imported_table->row_repeats_header[14]);
    CHECK_FALSE(imported_table->row_repeats_header[27]);
    CHECK_FALSE(imported_table->row_repeats_header[40]);

    const auto row_13_cell = document.inspect_table_cell(0U, 13U, 0U);
    const auto row_14_cell = document.inspect_table_cell(0U, 14U, 0U);
    const auto row_27_cell = document.inspect_table_cell(0U, 27U, 0U);
    const auto row_40_cell = document.inspect_table_cell(0U, 40U, 0U);
    REQUIRE(row_13_cell.has_value());
    REQUIRE(row_14_cell.has_value());
    REQUIRE(row_27_cell.has_value());
    REQUIRE(row_40_cell.has_value());
    CHECK(featherdoc::test_support::contains_text(row_13_cell->text,
                                                  "Feature13"));
    CHECK(featherdoc::test_support::contains_text(row_14_cell->text,
                                                  "Feature14"));
    CHECK(featherdoc::test_support::contains_text(row_27_cell->text,
                                                  "Feature27"));
    CHECK(featherdoc::test_support::contains_text(row_40_cell->text,
                                                  "Feature40"));

    REQUIRE_FALSE(document.save());

    featherdoc::Document reopened(docx_path);
    REQUIRE_FALSE(reopened.open());
    CHECK_EQ(featherdoc::test_support::collect_document_text(reopened),
             "Long repeated-header sample\n"
             "Footer note: long repeated-header sample\n");

    const auto reopened_blocks = reopened.inspect_body_blocks();
    REQUIRE_EQ(reopened_blocks.size(), 3U);
    CHECK_EQ(reopened_blocks[0].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(reopened_blocks[1].kind, featherdoc::body_block_kind::table);
    CHECK_EQ(reopened_blocks[2].kind, featherdoc::body_block_kind::paragraph);

    const auto reopened_table = reopened.inspect_table(0U);
    REQUIRE(reopened_table.has_value());
    CHECK_EQ(reopened_table->row_count, 53U);
    CHECK_EQ(reopened_table->column_count, 3U);
    REQUIRE_EQ(reopened_table->row_repeats_header.size(), 53U);
    CHECK(reopened_table->row_repeats_header[0]);
    CHECK_FALSE(reopened_table->row_repeats_header[14]);
    CHECK_FALSE(reopened_table->row_repeats_header[27]);
    CHECK_FALSE(reopened_table->row_repeats_header[40]);

    const auto reopened_row_40_cell =
        reopened.inspect_table_cell(0U, 40U, 0U);
    REQUIRE(reopened_row_40_cell.has_value());
    CHECK(featherdoc::test_support::contains_text(reopened_row_40_cell->text,
                                                  "Feature40"));

    if (std::getenv("FEATHERDOC_KEEP_PDF_IMPORT_TEST_OUTPUTS") == nullptr) {
        std::filesystem::remove(docx_path);
    }
}

TEST_CASE("PDF text importer preserves table title paragraph before imported table") {
    const auto input_path =
        featherdoc::test_support::write_table_title_paragraph_pdf(
            "featherdoc-pdf-import-table-title.pdf");
    const auto docx_path =
        std::filesystem::current_path() / "featherdoc-pdf-import-table-title.docx";
    std::filesystem::remove(docx_path);

    featherdoc::Document document(docx_path);
    featherdoc::pdf::PdfDocumentImportOptions options;
    options.import_table_candidates_as_tables = true;

    const auto import_result =
        featherdoc::pdf::import_pdf_text_document(input_path, document, options);
    REQUIRE_MESSAGE(import_result.success, import_result.error_message);
    CHECK_EQ(import_result.failure_kind,
             featherdoc::pdf::PdfDocumentImportFailureKind::none);
    CHECK_EQ(import_result.paragraphs_imported, 2U);
    CHECK_EQ(import_result.tables_imported, 1U);

    const auto blocks = document.inspect_body_blocks();
    REQUIRE_EQ(blocks.size(), 3U);
    CHECK_EQ(blocks[0].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(blocks[0].block_index, 0U);
    CHECK_EQ(blocks[1].kind, featherdoc::body_block_kind::table);
    CHECK_EQ(blocks[1].block_index, 1U);
    CHECK_EQ(blocks[2].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(blocks[2].block_index, 2U);
    CHECK_EQ(featherdoc::test_support::collect_document_text(document),
             "Table 1. Imported table title\n"
             "Tail paragraph after table title\n");

    const auto imported_table = document.inspect_table(0U);
    REQUIRE(imported_table.has_value());
    CHECK_EQ(imported_table->row_count, 3U);
    CHECK_EQ(imported_table->column_count, 3U);
    CHECK(featherdoc::test_support::contains_text(imported_table->text,
                                                  "Table title A1"));
    CHECK(featherdoc::test_support::contains_text(imported_table->text,
                                                  "Table title B2"));
    CHECK(featherdoc::test_support::contains_text(imported_table->text,
                                                  "Table title C3"));

    REQUIRE_FALSE(document.save());

    featherdoc::Document reopened(docx_path);
    REQUIRE_FALSE(reopened.open());
    const auto reopened_blocks = reopened.inspect_body_blocks();
    REQUIRE_EQ(reopened_blocks.size(), 3U);
    CHECK_EQ(reopened_blocks[0].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(reopened_blocks[1].kind, featherdoc::body_block_kind::table);
    CHECK_EQ(reopened_blocks[2].kind, featherdoc::body_block_kind::paragraph);

    if (std::getenv("FEATHERDOC_KEEP_PDF_IMPORT_TEST_OUTPUTS") == nullptr) {
        std::filesystem::remove(docx_path);
    }
}

TEST_CASE(
    "PDF text importer preserves note paragraph that only lightly overlaps table bounds") {
    const auto input_path =
        featherdoc::test_support::write_paragraph_table_touching_note_paragraph_pdf(
            "featherdoc-pdf-import-touching-note.pdf");
    const auto docx_path =
        std::filesystem::current_path() /
        "featherdoc-pdf-import-touching-note.docx";
    std::filesystem::remove(docx_path);

    featherdoc::Document document(docx_path);
    featherdoc::pdf::PdfDocumentImportOptions options;
    options.import_table_candidates_as_tables = true;

    const auto import_result =
        featherdoc::pdf::import_pdf_text_document(input_path, document, options);
    REQUIRE_MESSAGE(import_result.success, import_result.error_message);
    CHECK_EQ(import_result.failure_kind,
             featherdoc::pdf::PdfDocumentImportFailureKind::none);
    CHECK_EQ(import_result.paragraphs_imported, 3U);
    CHECK_EQ(import_result.tables_imported, 1U);

    const auto blocks = document.inspect_body_blocks();
    REQUIRE_EQ(blocks.size(), 4U);
    CHECK_EQ(blocks[0].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(blocks[1].kind, featherdoc::body_block_kind::table);
    CHECK_EQ(blocks[2].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(blocks[3].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(featherdoc::test_support::collect_document_text(document),
             "Intro paragraph before touching note\n"
             "Note paragraph touching table border\n"
             "Tail paragraph after touching note\n");

    const auto imported_table = document.inspect_table(0U);
    REQUIRE(imported_table.has_value());
    CHECK_EQ(imported_table->row_count, 3U);
    CHECK_EQ(imported_table->column_count, 3U);
    CHECK(featherdoc::test_support::contains_text(imported_table->text,
                                                  "Touching note A1"));
    CHECK(featherdoc::test_support::contains_text(imported_table->text,
                                                  "Touching note B2"));
    CHECK(featherdoc::test_support::contains_text(imported_table->text,
                                                  "Touching note C3"));

    REQUIRE_FALSE(document.save());

    featherdoc::Document reopened(docx_path);
    REQUIRE_FALSE(reopened.open());
    const auto reopened_blocks = reopened.inspect_body_blocks();
    REQUIRE_EQ(reopened_blocks.size(), 4U);
    CHECK_EQ(reopened_blocks[0].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(reopened_blocks[1].kind, featherdoc::body_block_kind::table);
    CHECK_EQ(reopened_blocks[2].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(reopened_blocks[3].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(featherdoc::test_support::collect_document_text(reopened),
             "Intro paragraph before touching note\n"
             "Note paragraph touching table border\n"
             "Tail paragraph after touching note\n");

    if (std::getenv("FEATHERDOC_KEEP_PDF_IMPORT_TEST_OUTPUTS") == nullptr) {
        std::filesystem::remove(docx_path);
    }
}

TEST_CASE(
    "PDF text importer preserves touching multi-line note paragraph outside table") {
    const auto input_path =
        featherdoc::test_support::
            write_paragraph_table_touching_multiline_note_paragraph_pdf(
                "featherdoc-pdf-import-touching-multiline-note.pdf");
    const auto docx_path =
        std::filesystem::current_path() /
        "featherdoc-pdf-import-touching-multiline-note.docx";
    std::filesystem::remove(docx_path);

    featherdoc::Document document(docx_path);
    featherdoc::pdf::PdfDocumentImportOptions options;
    options.import_table_candidates_as_tables = true;

    const auto import_result =
        featherdoc::pdf::import_pdf_text_document(input_path, document, options);
    REQUIRE_MESSAGE(import_result.success, import_result.error_message);
    CHECK_EQ(import_result.failure_kind,
             featherdoc::pdf::PdfDocumentImportFailureKind::none);
    CHECK_EQ(import_result.paragraphs_imported, 3U);
    CHECK_EQ(import_result.tables_imported, 1U);

    const auto blocks = document.inspect_body_blocks();
    REQUIRE_EQ(blocks.size(), 4U);
    CHECK_EQ(blocks[0].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(blocks[1].kind, featherdoc::body_block_kind::table);
    CHECK_EQ(blocks[2].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(blocks[3].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(featherdoc::test_support::collect_document_text(document),
             "Intro paragraph before touching multi-line note\n"
             "Note paragraph line one near table border\n"
             "Note paragraph line two stays outside table\n"
             "Tail paragraph after touching multi-line note\n");

    const auto imported_table = document.inspect_table(0U);
    REQUIRE(imported_table.has_value());
    CHECK_EQ(imported_table->row_count, 3U);
    CHECK_EQ(imported_table->column_count, 3U);
    CHECK(featherdoc::test_support::contains_text(imported_table->text,
                                                  "Touching multi-line A1"));
    CHECK(featherdoc::test_support::contains_text(imported_table->text,
                                                  "Touching multi-line B2"));
    CHECK(featherdoc::test_support::contains_text(imported_table->text,
                                                  "Touching multi-line C3"));

    REQUIRE_FALSE(document.save());

    featherdoc::Document reopened(docx_path);
    REQUIRE_FALSE(reopened.open());
    const auto reopened_blocks = reopened.inspect_body_blocks();
    REQUIRE_EQ(reopened_blocks.size(), 4U);
    CHECK_EQ(reopened_blocks[0].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(reopened_blocks[1].kind, featherdoc::body_block_kind::table);
    CHECK_EQ(reopened_blocks[2].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(reopened_blocks[3].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(featherdoc::test_support::collect_document_text(reopened),
             "Intro paragraph before touching multi-line note\n"
             "Note paragraph line one near table border\n"
             "Note paragraph line two stays outside table\n"
             "Tail paragraph after touching multi-line note\n");

    if (std::getenv("FEATHERDOC_KEEP_PDF_IMPORT_TEST_OUTPUTS") == nullptr) {
        std::filesystem::remove(docx_path);
    }
}

TEST_CASE(
    "PDF text importer preserves partial-overlap multi-line note paragraph as one block") {
    const auto input_path =
        featherdoc::test_support::
            write_paragraph_table_partial_overlap_multiline_note_paragraph_pdf(
                "featherdoc-pdf-import-partial-overlap-multiline-note.pdf");
    const auto docx_path =
        std::filesystem::current_path() /
        "featherdoc-pdf-import-partial-overlap-multiline-note.docx";
    std::filesystem::remove(docx_path);

    featherdoc::Document document(docx_path);
    featherdoc::pdf::PdfDocumentImportOptions options;
    options.import_table_candidates_as_tables = true;

    const auto import_result =
        featherdoc::pdf::import_pdf_text_document(input_path, document, options);
    REQUIRE_MESSAGE(import_result.success, import_result.error_message);
    CHECK_EQ(import_result.failure_kind,
             featherdoc::pdf::PdfDocumentImportFailureKind::none);
    CHECK_EQ(import_result.paragraphs_imported, 3U);
    CHECK_EQ(import_result.tables_imported, 1U);

    const auto blocks = document.inspect_body_blocks();
    REQUIRE_EQ(blocks.size(), 4U);
    CHECK_EQ(blocks[0].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(blocks[1].kind, featherdoc::body_block_kind::table);
    CHECK_EQ(blocks[2].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(blocks[3].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(featherdoc::test_support::collect_document_text(document),
             "Intro paragraph before partial-overlap note\n"
             "Partial overlap note line one near table border\n"
             "Partial overlap note line two stays outside table\n"
             "Tail paragraph after partial-overlap note\n");

    const auto imported_table = document.inspect_table(0U);
    REQUIRE(imported_table.has_value());
    CHECK_EQ(imported_table->row_count, 3U);
    CHECK_EQ(imported_table->column_count, 3U);
    CHECK(featherdoc::test_support::contains_text(imported_table->text,
                                                  "Partial overlap A1"));
    CHECK(featherdoc::test_support::contains_text(imported_table->text,
                                                  "Partial overlap B2"));
    CHECK(featherdoc::test_support::contains_text(imported_table->text,
                                                  "Partial overlap C3"));

    REQUIRE_FALSE(document.save());

    featherdoc::Document reopened(docx_path);
    REQUIRE_FALSE(reopened.open());
    const auto reopened_blocks = reopened.inspect_body_blocks();
    REQUIRE_EQ(reopened_blocks.size(), 4U);
    CHECK_EQ(reopened_blocks[0].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(reopened_blocks[1].kind, featherdoc::body_block_kind::table);
    CHECK_EQ(reopened_blocks[2].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(reopened_blocks[3].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(featherdoc::test_support::collect_document_text(reopened),
             "Intro paragraph before partial-overlap note\n"
             "Partial overlap note line one near table border\n"
             "Partial overlap note line two stays outside table\n"
             "Tail paragraph after partial-overlap note\n");

    if (std::getenv("FEATHERDOC_KEEP_PDF_IMPORT_TEST_OUTPUTS") == nullptr) {
        std::filesystem::remove(docx_path);
    }
}

TEST_CASE("PDF text importer preserves body order from out-of-order text runs") {
    const auto input_path =
        featherdoc::test_support::write_out_of_order_paragraph_table_paragraph_pdf(
            "featherdoc-pdf-import-structure-reordered-source.pdf");
    const auto docx_path =
        std::filesystem::current_path() /
        "featherdoc-pdf-import-structure-reordered-source.docx";
    std::filesystem::remove(docx_path);

    featherdoc::Document document(docx_path);
    featherdoc::pdf::PdfDocumentImportOptions options;
    options.import_table_candidates_as_tables = true;

    const auto import_result =
        featherdoc::pdf::import_pdf_text_document(input_path, document, options);
    REQUIRE_MESSAGE(import_result.success, import_result.error_message);
    CHECK_EQ(import_result.failure_kind,
             featherdoc::pdf::PdfDocumentImportFailureKind::none);
    CHECK_EQ(import_result.paragraphs_imported, 2U);
    CHECK_EQ(import_result.tables_imported, 1U);

    const auto blocks = document.inspect_body_blocks();
    REQUIRE_EQ(blocks.size(), 3U);
    CHECK_EQ(blocks[0].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(blocks[0].block_index, 0U);
    CHECK_EQ(blocks[1].kind, featherdoc::body_block_kind::table);
    CHECK_EQ(blocks[1].block_index, 1U);
    CHECK_EQ(blocks[2].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(blocks[2].block_index, 2U);
    CHECK_EQ(featherdoc::test_support::collect_document_text(document),
             "Intro paragraph line one\nIntro paragraph line two\n"
             "Tail paragraph after table\n");

    const auto imported_table = document.inspect_table(0U);
    REQUIRE(imported_table.has_value());
    CHECK_EQ(imported_table->row_count, 3U);
    CHECK_EQ(imported_table->column_count, 3U);
    CHECK(featherdoc::test_support::contains_text(imported_table->text,
                                                  "Cell A1"));
    CHECK(featherdoc::test_support::contains_text(imported_table->text,
                                                  "Cell B2"));
    CHECK(featherdoc::test_support::contains_text(imported_table->text,
                                                  "Cell C3"));

    REQUIRE_FALSE(document.save());

    featherdoc::Document reopened(docx_path);
    REQUIRE_FALSE(reopened.open());
    const auto reopened_blocks = reopened.inspect_body_blocks();
    REQUIRE_EQ(reopened_blocks.size(), 3U);
    CHECK_EQ(reopened_blocks[0].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(reopened_blocks[1].kind, featherdoc::body_block_kind::table);
    CHECK_EQ(reopened_blocks[2].kind, featherdoc::body_block_kind::paragraph);

    if (std::getenv("FEATHERDOC_KEEP_PDF_IMPORT_TEST_OUTPUTS") == nullptr) {
        std::filesystem::remove(docx_path);
    }
}

TEST_CASE("PDF table import does not keep empty leading paragraph") {
    const auto input_path =
        featherdoc::test_support::write_table_first_pdf(
            "featherdoc-pdf-import-table-first.pdf");
    const auto docx_path =
        std::filesystem::current_path() / "featherdoc-pdf-import-table-first.docx";
    std::filesystem::remove(docx_path);

    featherdoc::Document document(docx_path);
    featherdoc::pdf::PdfDocumentImportOptions options;
    options.import_table_candidates_as_tables = true;

    const auto import_result =
        featherdoc::pdf::import_pdf_text_document(input_path, document, options);
    REQUIRE_MESSAGE(import_result.success, import_result.error_message);
    CHECK_EQ(import_result.paragraphs_imported, 0U);
    CHECK_EQ(import_result.tables_imported, 1U);

    const auto blocks = document.inspect_body_blocks();
    REQUIRE_EQ(blocks.size(), 1U);
    CHECK_EQ(blocks[0].kind, featherdoc::body_block_kind::table);
    CHECK_EQ(blocks[0].block_index, 0U);
    CHECK_EQ(blocks[0].item_index, 0U);

    REQUIRE_FALSE(document.save());

    featherdoc::Document reopened(docx_path);
    REQUIRE_FALSE(reopened.open());
    const auto reopened_blocks = reopened.inspect_body_blocks();
    REQUIRE_EQ(reopened_blocks.size(), 1U);
    CHECK_EQ(reopened_blocks[0].kind, featherdoc::body_block_kind::table);
    CHECK_EQ(reopened_blocks[0].block_index, 0U);

    if (std::getenv("FEATHERDOC_KEEP_PDF_IMPORT_TEST_OUTPUTS") == nullptr) {
        std::filesystem::remove(docx_path);
    }
}

TEST_CASE("PDF table import merges compatible table candidates across page boundary") {
    const auto input_path =
        featherdoc::test_support::write_paragraph_table_pagebreak_table_paragraph_pdf(
            "featherdoc-pdf-import-paragraph-table-pagebreak-table-paragraph.pdf");
    const auto docx_path =
        std::filesystem::current_path() /
        "featherdoc-pdf-import-paragraph-table-pagebreak-table-paragraph.docx";
    std::filesystem::remove(docx_path);

    featherdoc::Document document(docx_path);
    featherdoc::pdf::PdfDocumentImportOptions options;
    options.import_table_candidates_as_tables = true;

    const auto import_result =
        featherdoc::pdf::import_pdf_text_document(input_path, document, options);
    REQUIRE_MESSAGE(import_result.success, import_result.error_message);
    CHECK_EQ(import_result.paragraphs_imported, 2U);
    CHECK_EQ(import_result.tables_imported, 1U);
    REQUIRE_EQ(import_result.table_continuation_diagnostics.size(), 2U);
    CHECK_EQ(import_result.table_continuation_diagnostics[1].disposition,
             featherdoc::pdf::PdfTableContinuationDisposition::merged_with_previous_table);
    CHECK_EQ(import_result.table_continuation_diagnostics[1].blocker,
             featherdoc::pdf::PdfTableContinuationBlocker::none);
    CHECK_EQ(import_result.table_continuation_diagnostics[1].source_row_offset,
             0U);

    const auto blocks = document.inspect_body_blocks();
    REQUIRE_EQ(blocks.size(), 3U);
    CHECK_EQ(blocks[0].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(blocks[1].kind, featherdoc::body_block_kind::table);
    CHECK_EQ(blocks[2].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(featherdoc::test_support::collect_document_text(document),
             "Intro paragraph before page break\n"
             "Tail paragraph after page break\n");

    const auto merged_table = document.inspect_table(0U);
    REQUIRE(merged_table.has_value());
    CHECK_EQ(merged_table->row_count, 6U);
    CHECK_EQ(merged_table->column_count, 3U);
    CHECK(featherdoc::test_support::contains_text(merged_table->text,
                                                  "Page one table A1"));
    CHECK(featherdoc::test_support::contains_text(merged_table->text,
                                                  "Page one table B2"));
    CHECK(featherdoc::test_support::contains_text(merged_table->text,
                                                  "Page one table C3"));
    CHECK(featherdoc::test_support::contains_text(merged_table->text,
                                                  "Page two table A1"));
    CHECK(featherdoc::test_support::contains_text(merged_table->text,
                                                  "Page two table B2"));
    CHECK(featherdoc::test_support::contains_text(merged_table->text,
                                                  "Page two table C3"));
    CHECK_FALSE(document.inspect_table(1U).has_value());

    REQUIRE_FALSE(document.save());

    featherdoc::Document reopened(docx_path);
    REQUIRE_FALSE(reopened.open());
    const auto reopened_blocks = reopened.inspect_body_blocks();
    REQUIRE_EQ(reopened_blocks.size(), 3U);
    CHECK_EQ(reopened_blocks[0].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(reopened_blocks[1].kind, featherdoc::body_block_kind::table);
    CHECK_EQ(reopened_blocks[2].kind, featherdoc::body_block_kind::paragraph);
    const auto reopened_table = reopened.inspect_table(0U);
    REQUIRE(reopened_table.has_value());
    CHECK_EQ(reopened_table->row_count, 6U);
    CHECK_EQ(reopened_table->column_count, 3U);
    CHECK_FALSE(reopened.inspect_table(1U).has_value());

    if (std::getenv("FEATHERDOC_KEEP_PDF_IMPORT_TEST_OUTPUTS") == nullptr) {
        std::filesystem::remove(docx_path);
    }
}

TEST_CASE(
    "PDF table import can require a higher confidence before cross-page merge") {
    const auto input_path =
        featherdoc::test_support::write_paragraph_table_pagebreak_table_paragraph_pdf(
            "featherdoc-pdf-import-pagebreak-confidence-threshold.pdf");
    const auto docx_path =
        std::filesystem::current_path() /
        "featherdoc-pdf-import-pagebreak-confidence-threshold.docx";
    std::filesystem::remove(docx_path);

    featherdoc::Document document(docx_path);
    featherdoc::pdf::PdfDocumentImportOptions options;
    options.import_table_candidates_as_tables = true;
    options.min_table_continuation_confidence = 90U;

    const auto import_result =
        featherdoc::pdf::import_pdf_text_document(input_path, document, options);
    REQUIRE_MESSAGE(import_result.success, import_result.error_message);
    CHECK_EQ(import_result.paragraphs_imported, 2U);
    CHECK_EQ(import_result.tables_imported, 2U);
    REQUIRE_EQ(import_result.table_continuation_diagnostics.size(), 2U);
    const auto &diagnostic = import_result.table_continuation_diagnostics[1];
    CHECK_EQ(diagnostic.disposition,
             featherdoc::pdf::PdfTableContinuationDisposition::created_new_table);
    CHECK_EQ(
        diagnostic.blocker,
        featherdoc::pdf::PdfTableContinuationBlocker::
            continuation_confidence_below_threshold);
    CHECK_EQ(diagnostic.continuation_confidence, 85U);
    CHECK_EQ(diagnostic.minimum_continuation_confidence, 90U);
    CHECK_EQ(diagnostic.source_row_offset, 0U);

    const auto blocks = document.inspect_body_blocks();
    REQUIRE_EQ(blocks.size(), 4U);
    CHECK_EQ(blocks[0].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(blocks[1].kind, featherdoc::body_block_kind::table);
    CHECK_EQ(blocks[2].kind, featherdoc::body_block_kind::table);
    CHECK_EQ(blocks[3].kind, featherdoc::body_block_kind::paragraph);

    const auto first_table = document.inspect_table(0U);
    const auto second_table = document.inspect_table(1U);
    REQUIRE(first_table.has_value());
    REQUIRE(second_table.has_value());
    CHECK_EQ(first_table->row_count, 3U);
    CHECK_EQ(second_table->row_count, 3U);
    CHECK(featherdoc::test_support::contains_text(first_table->text,
                                                  "Page one table A1"));
    CHECK(featherdoc::test_support::contains_text(second_table->text,
                                                  "Page two table A1"));
    CHECK_FALSE(document.inspect_table(2U).has_value());

    REQUIRE_FALSE(document.save());

    featherdoc::Document reopened(docx_path);
    REQUIRE_FALSE(reopened.open());
    const auto reopened_blocks = reopened.inspect_body_blocks();
    REQUIRE_EQ(reopened_blocks.size(), 4U);
    CHECK_EQ(reopened_blocks[0].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(reopened_blocks[1].kind, featherdoc::body_block_kind::table);
    CHECK_EQ(reopened_blocks[2].kind, featherdoc::body_block_kind::table);
    CHECK_EQ(reopened_blocks[3].kind, featherdoc::body_block_kind::paragraph);
    CHECK(reopened.inspect_table(0U).has_value());
    CHECK(reopened.inspect_table(1U).has_value());
    CHECK_FALSE(reopened.inspect_table(2U).has_value());

    if (std::getenv("FEATHERDOC_KEEP_PDF_IMPORT_TEST_OUTPUTS") == nullptr) {
        std::filesystem::remove(docx_path);
    }
}

TEST_CASE("PDF table import does not merge cross-page tables with incompatible widths") {
    const auto input_path =
        featherdoc::test_support::
            write_paragraph_table_pagebreak_wide_table_paragraph_pdf(
                "featherdoc-pdf-import-pagebreak-wide-table.pdf");
    const auto docx_path =
        std::filesystem::current_path() /
        "featherdoc-pdf-import-pagebreak-wide-table.docx";
    std::filesystem::remove(docx_path);

    featherdoc::Document document(docx_path);
    featherdoc::pdf::PdfDocumentImportOptions options;
    options.import_table_candidates_as_tables = true;

    const auto import_result =
        featherdoc::pdf::import_pdf_text_document(input_path, document, options);
    REQUIRE_MESSAGE(import_result.success, import_result.error_message);
    CHECK_EQ(import_result.paragraphs_imported, 2U);
    CHECK_EQ(import_result.tables_imported, 2U);
    REQUIRE_EQ(import_result.parsed_document.pages.size(), 2U);
    const auto &first_parsed_page = import_result.parsed_document.pages[0];
    REQUIRE_EQ(first_parsed_page.content_blocks.size(), 2U);
    CHECK_EQ(first_parsed_page.content_blocks[0].kind,
             featherdoc::pdf::PdfParsedContentBlockKind::paragraph);
    CHECK_EQ(first_parsed_page.content_blocks[1].kind,
             featherdoc::pdf::PdfParsedContentBlockKind::table_candidate);
    const auto &second_parsed_page = import_result.parsed_document.pages[1];
    REQUIRE_EQ(second_parsed_page.content_blocks.size(), 2U);
    CHECK_EQ(second_parsed_page.content_blocks[0].kind,
             featherdoc::pdf::PdfParsedContentBlockKind::table_candidate);
    CHECK_EQ(second_parsed_page.content_blocks[1].kind,
             featherdoc::pdf::PdfParsedContentBlockKind::paragraph);
    REQUIRE_EQ(import_result.table_continuation_diagnostics.size(), 2U);
    CHECK_EQ(import_result.table_continuation_diagnostics[1].disposition,
             featherdoc::pdf::PdfTableContinuationDisposition::created_new_table);
    CHECK_EQ(import_result.table_continuation_diagnostics[1].blocker,
             featherdoc::pdf::PdfTableContinuationBlocker::column_anchors_mismatch);

    const auto blocks = document.inspect_body_blocks();
    REQUIRE_EQ(blocks.size(), 4U);
    CHECK_EQ(blocks[0].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(blocks[1].kind, featherdoc::body_block_kind::table);
    CHECK_EQ(blocks[2].kind, featherdoc::body_block_kind::table);
    CHECK_EQ(blocks[3].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(featherdoc::test_support::collect_document_text(document),
             "Intro paragraph before wide table\n"
             "Tail paragraph after wide table\n");

    const auto first_table = document.inspect_table(0U);
    const auto second_table = document.inspect_table(1U);
    REQUIRE(first_table.has_value());
    REQUIRE(second_table.has_value());
    CHECK_EQ(first_table->row_count, 3U);
    CHECK_EQ(first_table->column_count, 3U);
    CHECK_EQ(second_table->row_count, 3U);
    CHECK_EQ(second_table->column_count, 3U);
    CHECK(featherdoc::test_support::contains_text(first_table->text,
                                                  "Narrow page A1"));
    CHECK(featherdoc::test_support::contains_text(second_table->text,
                                                  "Wide page A1"));
    CHECK_FALSE(document.inspect_table(2U).has_value());

    REQUIRE_FALSE(document.save());

    featherdoc::Document reopened(docx_path);
    REQUIRE_FALSE(reopened.open());
    const auto reopened_blocks = reopened.inspect_body_blocks();
    REQUIRE_EQ(reopened_blocks.size(), 4U);
    CHECK_EQ(reopened_blocks[0].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(reopened_blocks[1].kind, featherdoc::body_block_kind::table);
    CHECK_EQ(reopened_blocks[2].kind, featherdoc::body_block_kind::table);
    CHECK_EQ(reopened_blocks[3].kind, featherdoc::body_block_kind::paragraph);
    CHECK(reopened.inspect_table(0U).has_value());
    CHECK(reopened.inspect_table(1U).has_value());
    CHECK_FALSE(reopened.inspect_table(2U).has_value());

    if (std::getenv("FEATHERDOC_KEEP_PDF_IMPORT_TEST_OUTPUTS") == nullptr) {
        std::filesystem::remove(docx_path);
    }
}

TEST_CASE("PDF table import does not merge cross-page tables that start too low on the next page") {
    const auto input_path =
        featherdoc::test_support::
            write_paragraph_table_pagebreak_low_table_paragraph_pdf(
                "featherdoc-pdf-import-pagebreak-low-table.pdf");
    const auto docx_path =
        std::filesystem::current_path() /
        "featherdoc-pdf-import-pagebreak-low-table.docx";
    std::filesystem::remove(docx_path);

    featherdoc::Document document(docx_path);
    featherdoc::pdf::PdfDocumentImportOptions options;
    options.import_table_candidates_as_tables = true;

    const auto import_result =
        featherdoc::pdf::import_pdf_text_document(input_path, document, options);
    REQUIRE_MESSAGE(import_result.success, import_result.error_message);
    CHECK_EQ(import_result.paragraphs_imported, 2U);
    CHECK_EQ(import_result.tables_imported, 2U);
    REQUIRE_EQ(import_result.table_continuation_diagnostics.size(), 2U);
    CHECK_EQ(import_result.table_continuation_diagnostics[1].disposition,
             featherdoc::pdf::PdfTableContinuationDisposition::created_new_table);
    CHECK_EQ(import_result.table_continuation_diagnostics[1].blocker,
             featherdoc::pdf::PdfTableContinuationBlocker::not_near_page_top);

    const auto blocks = document.inspect_body_blocks();
    REQUIRE_EQ(blocks.size(), 4U);
    CHECK_EQ(blocks[0].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(blocks[1].kind, featherdoc::body_block_kind::table);
    CHECK_EQ(blocks[2].kind, featherdoc::body_block_kind::table);
    CHECK_EQ(blocks[3].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(featherdoc::test_support::collect_document_text(document),
             "Intro paragraph before low table\n"
             "Tail paragraph after low table\n");

    const auto first_table = document.inspect_table(0U);
    const auto second_table = document.inspect_table(1U);
    REQUIRE(first_table.has_value());
    REQUIRE(second_table.has_value());
    CHECK_EQ(first_table->row_count, 3U);
    CHECK_EQ(second_table->row_count, 3U);
    CHECK(featherdoc::test_support::contains_text(first_table->text,
                                                  "Low first page A1"));
    CHECK(featherdoc::test_support::contains_text(second_table->text,
                                                  "Low second page A1"));
    CHECK_FALSE(document.inspect_table(2U).has_value());

    REQUIRE_FALSE(document.save());

    featherdoc::Document reopened(docx_path);
    REQUIRE_FALSE(reopened.open());
    const auto reopened_blocks = reopened.inspect_body_blocks();
    REQUIRE_EQ(reopened_blocks.size(), 4U);
    CHECK_EQ(reopened_blocks[0].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(reopened_blocks[1].kind, featherdoc::body_block_kind::table);
    CHECK_EQ(reopened_blocks[2].kind, featherdoc::body_block_kind::table);
    CHECK_EQ(reopened_blocks[3].kind, featherdoc::body_block_kind::paragraph);
    CHECK(reopened.inspect_table(0U).has_value());
    CHECK(reopened.inspect_table(1U).has_value());
    CHECK_FALSE(reopened.inspect_table(2U).has_value());

    if (std::getenv("FEATHERDOC_KEEP_PDF_IMPORT_TEST_OUTPUTS") == nullptr) {
        std::filesystem::remove(docx_path);
    }
}

TEST_CASE("PDF table import does not merge through an intervening paragraph") {
    const auto input_path =
        featherdoc::test_support::
            write_paragraph_table_pagebreak_caption_table_paragraph_pdf(
                "featherdoc-pdf-import-pagebreak-caption-table.pdf");
    const auto docx_path =
        std::filesystem::current_path() /
        "featherdoc-pdf-import-pagebreak-caption-table.docx";
    std::filesystem::remove(docx_path);

    featherdoc::Document document(docx_path);
    featherdoc::pdf::PdfDocumentImportOptions options;
    options.import_table_candidates_as_tables = true;

    const auto import_result =
        featherdoc::pdf::import_pdf_text_document(input_path, document, options);
    REQUIRE_MESSAGE(import_result.success, import_result.error_message);
    CHECK_EQ(import_result.paragraphs_imported, 3U);
    CHECK_EQ(import_result.tables_imported, 2U);
    REQUIRE_EQ(import_result.table_continuation_diagnostics.size(), 2U);
    CHECK_EQ(import_result.table_continuation_diagnostics[1].disposition,
             featherdoc::pdf::PdfTableContinuationDisposition::created_new_table);
    CHECK_EQ(import_result.table_continuation_diagnostics[1].blocker,
             featherdoc::pdf::PdfTableContinuationBlocker::no_previous_table);

    const auto blocks = document.inspect_body_blocks();
    REQUIRE_EQ(blocks.size(), 5U);
    CHECK_EQ(blocks[0].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(blocks[1].kind, featherdoc::body_block_kind::table);
    CHECK_EQ(blocks[2].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(blocks[3].kind, featherdoc::body_block_kind::table);
    CHECK_EQ(blocks[4].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(featherdoc::test_support::collect_document_text(document),
             "Intro paragraph before caption page\n"
             "Continuation note before table\n"
             "Tail paragraph after caption table\n");

    const auto first_table = document.inspect_table(0U);
    const auto second_table = document.inspect_table(1U);
    REQUIRE(first_table.has_value());
    REQUIRE(second_table.has_value());
    CHECK_EQ(first_table->row_count, 3U);
    CHECK_EQ(second_table->row_count, 3U);
    CHECK(featherdoc::test_support::contains_text(first_table->text,
                                                  "Caption first table A1"));
    CHECK(featherdoc::test_support::contains_text(second_table->text,
                                                  "Caption second table A1"));
    CHECK_FALSE(document.inspect_table(2U).has_value());

    REQUIRE_FALSE(document.save());

    featherdoc::Document reopened(docx_path);
    REQUIRE_FALSE(reopened.open());
    const auto reopened_blocks = reopened.inspect_body_blocks();
    REQUIRE_EQ(reopened_blocks.size(), 5U);
    CHECK_EQ(reopened_blocks[0].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(reopened_blocks[1].kind, featherdoc::body_block_kind::table);
    CHECK_EQ(reopened_blocks[2].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(reopened_blocks[3].kind, featherdoc::body_block_kind::table);
    CHECK_EQ(reopened_blocks[4].kind, featherdoc::body_block_kind::paragraph);
    CHECK(reopened.inspect_table(0U).has_value());
    CHECK(reopened.inspect_table(1U).has_value());
    CHECK_FALSE(reopened.inspect_table(2U).has_value());

    if (std::getenv("FEATHERDOC_KEEP_PDF_IMPORT_TEST_OUTPUTS") == nullptr) {
        std::filesystem::remove(docx_path);
    }
}

TEST_CASE("PDF table import preserves paragraph table paragraph body order") {
    const auto input_path =
        featherdoc::test_support::write_paragraph_table_paragraph_pdf(
            "featherdoc-pdf-import-paragraph-table-paragraph.pdf");
    const auto docx_path = std::filesystem::current_path() /
                           "featherdoc-pdf-import-paragraph-table-paragraph.docx";
    std::filesystem::remove(docx_path);

    featherdoc::Document document(docx_path);
    featherdoc::pdf::PdfDocumentImportOptions options;
    options.import_table_candidates_as_tables = true;

    const auto import_result =
        featherdoc::pdf::import_pdf_text_document(input_path, document, options);
    REQUIRE_MESSAGE(import_result.success, import_result.error_message);
    CHECK_EQ(import_result.paragraphs_imported, 2U);
    CHECK_EQ(import_result.tables_imported, 1U);

    const auto blocks = document.inspect_body_blocks();
    REQUIRE_EQ(blocks.size(), 3U);
    CHECK_EQ(blocks[0].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(blocks[0].block_index, 0U);
    CHECK_EQ(blocks[0].item_index, 0U);
    CHECK_EQ(blocks[1].kind, featherdoc::body_block_kind::table);
    CHECK_EQ(blocks[1].block_index, 1U);
    CHECK_EQ(blocks[1].item_index, 0U);
    CHECK_EQ(blocks[2].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(blocks[2].block_index, 2U);
    CHECK_EQ(blocks[2].item_index, 1U);
    CHECK_EQ(featherdoc::test_support::collect_document_text(document),
             "Intro paragraph line one\nIntro paragraph line two\n"
             "Tail paragraph after table\n");

    REQUIRE_FALSE(document.save());

    featherdoc::Document reopened(docx_path);
    REQUIRE_FALSE(reopened.open());
    const auto reopened_blocks = reopened.inspect_body_blocks();
    REQUIRE_EQ(reopened_blocks.size(), 3U);
    CHECK_EQ(reopened_blocks[0].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(reopened_blocks[1].kind, featherdoc::body_block_kind::table);
    CHECK_EQ(reopened_blocks[2].kind, featherdoc::body_block_kind::paragraph);

    if (std::getenv("FEATHERDOC_KEEP_PDF_IMPORT_TEST_OUTPUTS") == nullptr) {
        std::filesystem::remove(docx_path);
    }
}

TEST_CASE("PDF table import preserves consecutive table body order") {
    const auto input_path =
        featherdoc::test_support::write_paragraph_table_table_paragraph_pdf(
            "featherdoc-pdf-import-paragraph-table-table-paragraph.pdf");
    const auto docx_path =
        std::filesystem::current_path() /
        "featherdoc-pdf-import-paragraph-table-table-paragraph.docx";
    std::filesystem::remove(docx_path);

    featherdoc::Document document(docx_path);
    featherdoc::pdf::PdfDocumentImportOptions options;
    options.import_table_candidates_as_tables = true;

    const auto import_result =
        featherdoc::pdf::import_pdf_text_document(input_path, document, options);
    REQUIRE_MESSAGE(import_result.success, import_result.error_message);
    CHECK_EQ(import_result.paragraphs_imported, 2U);
    CHECK_EQ(import_result.tables_imported, 2U);
    REQUIRE_EQ(import_result.table_continuation_diagnostics.size(), 2U);
    CHECK_EQ(import_result.table_continuation_diagnostics[1].disposition,
             featherdoc::pdf::PdfTableContinuationDisposition::created_new_table);
    CHECK_EQ(import_result.table_continuation_diagnostics[1].blocker,
             featherdoc::pdf::PdfTableContinuationBlocker::not_first_block_on_page);

    const auto blocks = document.inspect_body_blocks();
    REQUIRE_EQ(blocks.size(), 4U);
    CHECK_EQ(blocks[0].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(blocks[0].block_index, 0U);
    CHECK_EQ(blocks[0].item_index, 0U);
    CHECK_EQ(blocks[1].kind, featherdoc::body_block_kind::table);
    CHECK_EQ(blocks[1].block_index, 1U);
    CHECK_EQ(blocks[1].item_index, 0U);
    CHECK_EQ(blocks[2].kind, featherdoc::body_block_kind::table);
    CHECK_EQ(blocks[2].block_index, 2U);
    CHECK_EQ(blocks[2].item_index, 1U);
    CHECK_EQ(blocks[3].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(blocks[3].block_index, 3U);
    CHECK_EQ(blocks[3].item_index, 1U);
    CHECK_EQ(featherdoc::test_support::collect_document_text(document),
             "Intro paragraph before tables\n"
             "Tail paragraph after tables\n");

    const auto first_table = document.inspect_table(0U);
    const auto second_table = document.inspect_table(1U);
    REQUIRE(first_table.has_value());
    REQUIRE(second_table.has_value());
    CHECK_EQ(first_table->text,
             "First table A1\t\t\n\tFirst table B2\t\n\t\tFirst table C3");
    CHECK_EQ(second_table->text,
             "Second table A1\t\t\n\tSecond table B2\t\n\t\tSecond table C3");

    REQUIRE_FALSE(document.save());

    featherdoc::Document reopened(docx_path);
    REQUIRE_FALSE(reopened.open());
    const auto reopened_blocks = reopened.inspect_body_blocks();
    REQUIRE_EQ(reopened_blocks.size(), 4U);
    CHECK_EQ(reopened_blocks[0].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(reopened_blocks[1].kind, featherdoc::body_block_kind::table);
    CHECK_EQ(reopened_blocks[2].kind, featherdoc::body_block_kind::table);
    CHECK_EQ(reopened_blocks[3].kind, featherdoc::body_block_kind::paragraph);

    if (std::getenv("FEATHERDOC_KEEP_PDF_IMPORT_TEST_OUTPUTS") == nullptr) {
        std::filesystem::remove(docx_path);
    }
}

TEST_CASE("PDF table import preserves same-page paragraph separation between table candidates") {
    const auto input_path =
        featherdoc::test_support::write_paragraph_table_note_table_paragraph_pdf(
            "featherdoc-pdf-import-paragraph-table-note-table-paragraph.pdf");
    const auto docx_path =
        std::filesystem::current_path() /
        "featherdoc-pdf-import-paragraph-table-note-table-paragraph.docx";
    std::filesystem::remove(docx_path);

    featherdoc::Document document(docx_path);
    featherdoc::pdf::PdfDocumentImportOptions options;
    options.import_table_candidates_as_tables = true;

    const auto import_result =
        featherdoc::pdf::import_pdf_text_document(input_path, document, options);
    REQUIRE_MESSAGE(import_result.success, import_result.error_message);
    CHECK_EQ(import_result.paragraphs_imported, 3U);
    CHECK_EQ(import_result.tables_imported, 2U);
    REQUIRE_EQ(import_result.parsed_document.pages.size(), 1U);
    const auto &parsed_page = import_result.parsed_document.pages.front();
    REQUIRE_EQ(parsed_page.content_blocks.size(), 5U);
    CHECK_EQ(parsed_page.content_blocks[0].kind,
             featherdoc::pdf::PdfParsedContentBlockKind::paragraph);
    CHECK_EQ(parsed_page.content_blocks[1].kind,
             featherdoc::pdf::PdfParsedContentBlockKind::table_candidate);
    CHECK_EQ(parsed_page.content_blocks[2].kind,
             featherdoc::pdf::PdfParsedContentBlockKind::paragraph);
    CHECK_EQ(parsed_page.content_blocks[3].kind,
             featherdoc::pdf::PdfParsedContentBlockKind::table_candidate);
    CHECK_EQ(parsed_page.content_blocks[4].kind,
             featherdoc::pdf::PdfParsedContentBlockKind::paragraph);

    const auto blocks = document.inspect_body_blocks();
    REQUIRE_EQ(blocks.size(), 5U);
    CHECK_EQ(blocks[0].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(blocks[0].block_index, 0U);
    CHECK_EQ(blocks[0].item_index, 0U);
    CHECK_EQ(blocks[1].kind, featherdoc::body_block_kind::table);
    CHECK_EQ(blocks[1].block_index, 1U);
    CHECK_EQ(blocks[1].item_index, 0U);
    CHECK_EQ(blocks[2].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(blocks[2].block_index, 2U);
    CHECK_EQ(blocks[2].item_index, 1U);
    CHECK_EQ(blocks[3].kind, featherdoc::body_block_kind::table);
    CHECK_EQ(blocks[3].block_index, 3U);
    CHECK_EQ(blocks[3].item_index, 1U);
    CHECK_EQ(blocks[4].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(blocks[4].block_index, 4U);
    CHECK_EQ(blocks[4].item_index, 2U);
    CHECK_EQ(featherdoc::test_support::collect_document_text(document),
             "Intro paragraph before same-page tables\n"
             "Note paragraph between tables\n"
             "Tail paragraph after same-page tables\n");

    const auto first_table = document.inspect_table(0U);
    const auto second_table = document.inspect_table(1U);
    REQUIRE(first_table.has_value());
    REQUIRE(second_table.has_value());
    CHECK_EQ(first_table->row_count, 3U);
    CHECK_EQ(first_table->column_count, 3U);
    CHECK_EQ(second_table->row_count, 3U);
    CHECK_EQ(second_table->column_count, 3U);
    CHECK(featherdoc::test_support::contains_text(first_table->text,
                                                  "First table A1"));
    CHECK(featherdoc::test_support::contains_text(second_table->text,
                                                  "Second table A1"));
    CHECK_FALSE(document.inspect_table(2U).has_value());

    REQUIRE_FALSE(document.save());

    featherdoc::Document reopened(docx_path);
    REQUIRE_FALSE(reopened.open());
    const auto reopened_blocks = reopened.inspect_body_blocks();
    REQUIRE_EQ(reopened_blocks.size(), 5U);
    CHECK_EQ(reopened_blocks[0].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(reopened_blocks[1].kind, featherdoc::body_block_kind::table);
    CHECK_EQ(reopened_blocks[2].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(reopened_blocks[3].kind, featherdoc::body_block_kind::table);
    CHECK_EQ(reopened_blocks[4].kind, featherdoc::body_block_kind::paragraph);
    CHECK(reopened.inspect_table(0U).has_value());
    CHECK(reopened.inspect_table(1U).has_value());
    CHECK_FALSE(reopened.inspect_table(2U).has_value());

    if (std::getenv("FEATHERDOC_KEEP_PDF_IMPORT_TEST_OUTPUTS") == nullptr) {
        std::filesystem::remove(docx_path);
    }
}
