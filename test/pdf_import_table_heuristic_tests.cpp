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
    CHECK_EQ(reopened_table->text, "Cell A1\t\t\n\tCell B2\t\n\t\tCell C3");

    if (std::getenv("FEATHERDOC_KEEP_PDF_IMPORT_TEST_OUTPUTS") == nullptr) {
        std::filesystem::remove(docx_path);
    }
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

TEST_CASE("PDF table import preserves mixed order across page boundary") {
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
    CHECK_EQ(import_result.tables_imported, 2U);

    const auto blocks = document.inspect_body_blocks();
    REQUIRE_EQ(blocks.size(), 4U);
    CHECK_EQ(blocks[0].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(blocks[1].kind, featherdoc::body_block_kind::table);
    CHECK_EQ(blocks[2].kind, featherdoc::body_block_kind::table);
    CHECK_EQ(blocks[3].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(featherdoc::test_support::collect_document_text(document),
             "Intro paragraph before page break\n"
             "Tail paragraph after page break\n");

    const auto first_table = document.inspect_table(0U);
    const auto second_table = document.inspect_table(1U);
    REQUIRE(first_table.has_value());
    REQUIRE(second_table.has_value());
    CHECK_EQ(first_table->text,
             "Page one table A1\t\t\n\tPage one table B2\t\n\t\tPage one table C3");
    CHECK_EQ(second_table->text,
             "Page two table A1\t\t\n\tPage two table B2\t\n\t\tPage two table C3");

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
