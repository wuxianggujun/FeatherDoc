#include "doctest.h"

#include "pdf_import_test_support.hpp"

#include <featherdoc/pdf/pdf_document_importer.hpp>
#include <featherdoc/pdf/pdf_parser.hpp>

#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <numeric>

TEST_CASE(
    "PDF table import skips abbreviation-varied repeated source header rows while merging cross-page repeated-header table") {
    const auto input_path =
        featherdoc::test_support::
            write_paragraph_table_pagebreak_repeated_header_abbreviation_variant_pdf(
                "featherdoc-pdf-import-pagebreak-repeated-header-abbreviation-variant.pdf");
    const auto docx_path =
        std::filesystem::current_path() /
        "featherdoc-pdf-import-pagebreak-repeated-header-abbreviation-variant.docx";
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
             featherdoc::pdf::PdfTableContinuationDisposition::
                 merged_with_previous_table);
    CHECK_EQ(import_result.table_continuation_diagnostics[1].blocker,
             featherdoc::pdf::PdfTableContinuationBlocker::none);
    CHECK(import_result.table_continuation_diagnostics[1].header_matches_previous);
    CHECK_EQ(import_result.table_continuation_diagnostics[1].header_match_kind,
             featherdoc::pdf::PdfTableContinuationHeaderMatchKind::canonical_text);
    CHECK(import_result.table_continuation_diagnostics[1].skipped_repeating_header);
    CHECK_EQ(import_result.table_continuation_diagnostics[1].source_row_offset,
             1U);

    const auto blocks = document.inspect_body_blocks();
    REQUIRE_EQ(blocks.size(), 3U);
    CHECK_EQ(blocks[0].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(blocks[1].kind, featherdoc::body_block_kind::table);
    CHECK_EQ(blocks[2].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(featherdoc::test_support::collect_document_text(document),
             "Repeated header abbreviation variant sample\n"
             "Footer note: repeated source header abbreviation variant should merge cleanly\n");

    const auto imported_table = document.inspect_table(0U);
    REQUIRE(imported_table.has_value());
    CHECK_EQ(imported_table->row_count, 5U);
    CHECK_EQ(imported_table->column_count, 3U);
    REQUIRE_EQ(imported_table->row_repeats_header.size(), 5U);
    CHECK(imported_table->row_repeats_header[0]);
    CHECK_FALSE(imported_table->row_repeats_header[1]);
    CHECK_FALSE(imported_table->row_repeats_header[3]);
    CHECK_FALSE(imported_table->row_repeats_header.back());

    const auto header_qty_cell = document.inspect_table_cell(0U, 0U, 1U);
    const auto header_amount_cell = document.inspect_table_cell(0U, 0U, 2U);
    const auto boundary_cell_3 = document.inspect_table_cell(0U, 3U, 0U);
    const auto boundary_qty_cell = document.inspect_table_cell(0U, 3U, 1U);
    const auto boundary_amount_cell = document.inspect_table_cell(0U, 3U, 2U);
    REQUIRE(header_qty_cell.has_value());
    REQUIRE(header_amount_cell.has_value());
    REQUIRE(boundary_cell_3.has_value());
    REQUIRE(boundary_qty_cell.has_value());
    REQUIRE(boundary_amount_cell.has_value());
    CHECK(featherdoc::test_support::contains_text(header_qty_cell->text,
                                                  "Quantity"));
    CHECK(featherdoc::test_support::contains_text(header_amount_cell->text,
                                                  "Amount"));
    CHECK(featherdoc::test_support::contains_text(boundary_cell_3->text,
                                                  "Invoice merge"));
    CHECK(featherdoc::test_support::contains_text(boundary_qty_cell->text,
                                                  "6 units"));
    CHECK(featherdoc::test_support::contains_text(boundary_amount_cell->text,
                                                  "USD 60"));
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

    const auto reopened_boundary_qty_cell =
        reopened.inspect_table_cell(0U, 3U, 1U);
    REQUIRE(reopened_boundary_qty_cell.has_value());
    CHECK(featherdoc::test_support::contains_text(
        reopened_boundary_qty_cell->text, "6 units"));

    if (std::getenv("FEATHERDOC_KEEP_PDF_IMPORT_TEST_OUTPUTS") == nullptr) {
        std::filesystem::remove(docx_path);
    }
}

TEST_CASE(
    "PDF table import skips word-order-varied repeated source header rows while merging cross-page repeated-header table") {
    const auto input_path =
        featherdoc::test_support::
            write_paragraph_table_pagebreak_repeated_header_word_order_variant_pdf(
                "featherdoc-pdf-import-pagebreak-repeated-header-word-order-variant.pdf");
    const auto docx_path =
        std::filesystem::current_path() /
        "featherdoc-pdf-import-pagebreak-repeated-header-word-order-variant.docx";
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
             featherdoc::pdf::PdfTableContinuationDisposition::
                 merged_with_previous_table);
    CHECK_EQ(import_result.table_continuation_diagnostics[1].blocker,
             featherdoc::pdf::PdfTableContinuationBlocker::none);
    CHECK(import_result.table_continuation_diagnostics[1].header_matches_previous);
    CHECK_EQ(import_result.table_continuation_diagnostics[1].header_match_kind,
             featherdoc::pdf::PdfTableContinuationHeaderMatchKind::token_set);
    CHECK(import_result.table_continuation_diagnostics[1].skipped_repeating_header);
    CHECK_EQ(import_result.table_continuation_diagnostics[1].source_row_offset,
             1U);

    const auto blocks = document.inspect_body_blocks();
    REQUIRE_EQ(blocks.size(), 3U);
    CHECK_EQ(blocks[0].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(blocks[1].kind, featherdoc::body_block_kind::table);
    CHECK_EQ(blocks[2].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(featherdoc::test_support::collect_document_text(document),
             "Repeated header word order variant sample\n"
             "Footer note: repeated source header word order variant should merge cleanly\n");

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
    const auto boundary_owner_cell = document.inspect_table_cell(0U, 3U, 1U);
    const auto boundary_status_cell = document.inspect_table_cell(0U, 3U, 2U);
    REQUIRE(header_owner_cell.has_value());
    REQUIRE(header_status_cell.has_value());
    REQUIRE(boundary_cell_3.has_value());
    REQUIRE(boundary_owner_cell.has_value());
    REQUIRE(boundary_status_cell.has_value());
    CHECK(featherdoc::test_support::contains_text(header_owner_cell->text,
                                                  "Owner Name"));
    CHECK(featherdoc::test_support::contains_text(header_status_cell->text,
                                                  "Project Status"));
    CHECK(featherdoc::test_support::contains_text(boundary_cell_3->text,
                                                  "Invoice merge"));
    CHECK(featherdoc::test_support::contains_text(boundary_owner_cell->text,
                                                  "Import lane"));
    CHECK(featherdoc::test_support::contains_text(boundary_status_cell->text,
                                                  "Tracking verified"));
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

    const auto reopened_boundary_status_cell =
        reopened.inspect_table_cell(0U, 3U, 2U);
    REQUIRE(reopened_boundary_status_cell.has_value());
    CHECK(featherdoc::test_support::contains_text(
        reopened_boundary_status_cell->text, "Tracking verified"));

    if (std::getenv("FEATHERDOC_KEEP_PDF_IMPORT_TEST_OUTPUTS") == nullptr) {
        std::filesystem::remove(docx_path);
    }
}

TEST_CASE(
    "PDF table import skips unit-punctuation-varied repeated source header rows while merging cross-page repeated-header table") {
    const auto input_path =
        featherdoc::test_support::
            write_paragraph_table_pagebreak_repeated_header_unit_variant_pdf(
                "featherdoc-pdf-import-pagebreak-repeated-header-unit-variant.pdf");
    const auto docx_path =
        std::filesystem::current_path() /
        "featherdoc-pdf-import-pagebreak-repeated-header-unit-variant.docx";
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
             featherdoc::pdf::PdfTableContinuationDisposition::
                 merged_with_previous_table);
    CHECK_EQ(import_result.table_continuation_diagnostics[1].blocker,
             featherdoc::pdf::PdfTableContinuationBlocker::none);
    CHECK(import_result.table_continuation_diagnostics[1].header_matches_previous);
    CHECK_EQ(import_result.table_continuation_diagnostics[1].header_match_kind,
             featherdoc::pdf::PdfTableContinuationHeaderMatchKind::normalized_text);
    CHECK(import_result.table_continuation_diagnostics[1].skipped_repeating_header);
    CHECK_EQ(import_result.table_continuation_diagnostics[1].source_row_offset,
             1U);

    const auto blocks = document.inspect_body_blocks();
    REQUIRE_EQ(blocks.size(), 3U);
    CHECK_EQ(blocks[0].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(blocks[1].kind, featherdoc::body_block_kind::table);
    CHECK_EQ(blocks[2].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(featherdoc::test_support::collect_document_text(document),
             "Repeated header unit variant sample\n"
             "Footer note: repeated source header unit variant should merge cleanly\n");

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
    const auto header_amount_cell = document.inspect_table_cell(0U, 0U, 2U);
    const auto boundary_cell_3 = document.inspect_table_cell(0U, 3U, 0U);
    const auto boundary_owner_cell = document.inspect_table_cell(0U, 3U, 1U);
    const auto boundary_amount_cell = document.inspect_table_cell(0U, 3U, 2U);
    REQUIRE(header_owner_cell.has_value());
    REQUIRE(header_amount_cell.has_value());
    REQUIRE(boundary_cell_3.has_value());
    REQUIRE(boundary_owner_cell.has_value());
    REQUIRE(boundary_amount_cell.has_value());
    CHECK(featherdoc::test_support::contains_text(header_owner_cell->text,
                                                  "Owner Name"));
    CHECK(featherdoc::test_support::contains_text(header_amount_cell->text,
                                                  "Amount USD"));
    CHECK(featherdoc::test_support::contains_text(boundary_cell_3->text,
                                                  "Invoice merge"));
    CHECK(featherdoc::test_support::contains_text(boundary_owner_cell->text,
                                                  "Import lane"));
    CHECK(featherdoc::test_support::contains_text(boundary_amount_cell->text,
                                                  "USD 60"));
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

    const auto reopened_boundary_amount_cell =
        reopened.inspect_table_cell(0U, 3U, 2U);
    REQUIRE(reopened_boundary_amount_cell.has_value());
    CHECK(featherdoc::test_support::contains_text(
        reopened_boundary_amount_cell->text, "USD 60"));

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
    CHECK_FALSE(
        import_result.table_continuation_diagnostics[1].header_matches_previous);
    CHECK_EQ(import_result.table_continuation_diagnostics[1].header_match_kind,
             featherdoc::pdf::PdfTableContinuationHeaderMatchKind::none);
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
