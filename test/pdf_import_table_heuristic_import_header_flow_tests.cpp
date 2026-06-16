#include "doctest.h"

#include "pdf_import_test_support.hpp"

#include <featherdoc/pdf/pdf_document_importer.hpp>
#include <featherdoc/pdf/pdf_parser.hpp>

#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <numeric>

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
    REQUIRE_EQ(import_result.table_continuation_diagnostics.size(), 2U);
    CHECK_EQ(import_result.table_continuation_diagnostics[1].header_match_kind,
             featherdoc::pdf::PdfTableContinuationHeaderMatchKind::exact);

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
    REQUIRE_EQ(import_result.table_continuation_diagnostics.size(), 2U);
    CHECK_EQ(import_result.table_continuation_diagnostics[1].header_match_kind,
             featherdoc::pdf::PdfTableContinuationHeaderMatchKind::
                 normalized_text);

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
    REQUIRE_EQ(import_result.table_continuation_diagnostics.size(), 2U);
    CHECK_EQ(import_result.table_continuation_diagnostics[1].header_match_kind,
             featherdoc::pdf::PdfTableContinuationHeaderMatchKind::normalized_text);

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
    REQUIRE_EQ(import_result.table_continuation_diagnostics.size(), 2U);
    CHECK_EQ(import_result.table_continuation_diagnostics[1].header_match_kind,
             featherdoc::pdf::PdfTableContinuationHeaderMatchKind::plural_variant);

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
