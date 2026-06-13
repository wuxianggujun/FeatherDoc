#include "doctest.h"

#include "pdf_import_test_support.hpp"

#include <featherdoc/pdf/pdf_document_importer.hpp>
#include <featherdoc/pdf/pdf_parser.hpp>

#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <numeric>

TEST_CASE("PDF text importer preserves merged summary-row table cells") {
    const auto input_path =
        featherdoc::test_support::write_invoice_grid_merged_summary_pdf(
            "featherdoc-pdf-import-merged-summary-row-table.pdf");
    const auto docx_path =
        std::filesystem::current_path() /
        "featherdoc-pdf-import-merged-summary-row-table.docx";
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

    const auto blocks = document.inspect_body_blocks();
    REQUIRE_EQ(blocks.size(), 3U);
    CHECK_EQ(blocks[0].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(blocks[1].kind, featherdoc::body_block_kind::table);
    CHECK_EQ(blocks[2].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(featherdoc::test_support::collect_document_text(document),
             "Invoice merged summary grid sample\n"
             "Footer note: merged summary row spans label columns\n");

    const auto imported_table = document.inspect_table(0U);
    REQUIRE(imported_table.has_value());
    CHECK_EQ(imported_table->row_count, 5U);
    CHECK_EQ(imported_table->column_count, 4U);
    CHECK_EQ(document.inspect_table_cells(0U).size(), 18U);

    const auto summary_label =
        document.inspect_table_cell_by_grid_column(0U, 4U, 0U);
    const auto covered_summary =
        document.inspect_table_cell_by_grid_column(0U, 4U, 2U);
    const auto summary_amount =
        document.inspect_table_cell_by_grid_column(0U, 4U, 3U);
    REQUIRE(summary_label.has_value());
    REQUIRE(covered_summary.has_value());
    REQUIRE(summary_amount.has_value());
    CHECK_EQ(summary_label->column_span, 3U);
    CHECK_EQ(covered_summary->cell_index, summary_label->cell_index);
    CHECK_EQ(covered_summary->column_span, 3U);
    CHECK(featherdoc::test_support::contains_text(summary_label->text,
                                                  "Grand total"));
    CHECK_EQ(summary_amount->column_span, 1U);
    CHECK(featherdoc::test_support::contains_text(summary_amount->text,
                                                  "USD 135"));

    REQUIRE_FALSE(document.save());

    featherdoc::Document reopened(docx_path);
    REQUIRE_FALSE(reopened.open());
    const auto reopened_blocks = reopened.inspect_body_blocks();
    REQUIRE_EQ(reopened_blocks.size(), 3U);
    CHECK_EQ(reopened_blocks[0].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(reopened_blocks[1].kind, featherdoc::body_block_kind::table);
    CHECK_EQ(reopened_blocks[2].kind, featherdoc::body_block_kind::paragraph);
    const auto reopened_summary_label =
        reopened.inspect_table_cell_by_grid_column(0U, 4U, 2U);
    const auto reopened_summary_amount =
        reopened.inspect_table_cell_by_grid_column(0U, 4U, 3U);
    REQUIRE(reopened_summary_label.has_value());
    REQUIRE(reopened_summary_amount.has_value());
    CHECK_EQ(reopened_summary_label->column_span, 3U);
    CHECK(featherdoc::test_support::contains_text(
        reopened_summary_label->text, "Grand total"));
    CHECK(featherdoc::test_support::contains_text(reopened_summary_amount->text,
                                                  "USD 135"));

    if (std::getenv("FEATHERDOC_KEEP_PDF_IMPORT_TEST_OUTPUTS") == nullptr) {
        std::filesystem::remove(docx_path);
    }
}

TEST_CASE("PDF text importer preserves inline subtotal-row table cells") {
    const auto input_path =
        featherdoc::test_support::write_invoice_grid_inline_subtotal_pdf(
            "featherdoc-pdf-import-inline-subtotal-row-table.pdf");
    const auto docx_path =
        std::filesystem::current_path() /
        "featherdoc-pdf-import-inline-subtotal-row-table.docx";
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

    const auto blocks = document.inspect_body_blocks();
    REQUIRE_EQ(blocks.size(), 3U);
    CHECK_EQ(blocks[0].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(blocks[1].kind, featherdoc::body_block_kind::table);
    CHECK_EQ(blocks[2].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(featherdoc::test_support::collect_document_text(document),
             "Invoice inline subtotal grid sample\n"
             "Footer note: inline subtotal keeps later data rows\n");

    const auto imported_table = document.inspect_table(0U);
    REQUIRE(imported_table.has_value());
    CHECK_EQ(imported_table->row_count, 6U);
    CHECK_EQ(imported_table->column_count, 4U);
    CHECK_EQ(document.inspect_table_cells(0U).size(), 20U);

    const auto subtotal_label =
        document.inspect_table_cell_by_grid_column(0U, 2U, 0U);
    const auto subtotal_covered =
        document.inspect_table_cell_by_grid_column(0U, 2U, 2U);
    const auto subtotal_amount =
        document.inspect_table_cell_by_grid_column(0U, 2U, 3U);
    const auto later_data =
        document.inspect_table_cell_by_grid_column(0U, 3U, 0U);
    const auto summary_label =
        document.inspect_table_cell_by_grid_column(0U, 5U, 2U);
    REQUIRE(subtotal_label.has_value());
    REQUIRE(subtotal_covered.has_value());
    REQUIRE(subtotal_amount.has_value());
    REQUIRE(later_data.has_value());
    REQUIRE(summary_label.has_value());
    CHECK_EQ(subtotal_label->column_span, 3U);
    CHECK_EQ(subtotal_covered->cell_index, subtotal_label->cell_index);
    CHECK(featherdoc::test_support::contains_text(subtotal_label->text,
                                                  "Design subtotal"));
    CHECK(featherdoc::test_support::contains_text(subtotal_amount->text,
                                                  "USD 100"));
    CHECK(featherdoc::test_support::contains_text(later_data->text,
                                                  "Visual validation"));
    CHECK_EQ(summary_label->column_span, 3U);
    CHECK(featherdoc::test_support::contains_text(summary_label->text,
                                                  "Grand total"));

    REQUIRE_FALSE(document.save());

    featherdoc::Document reopened(docx_path);
    REQUIRE_FALSE(reopened.open());
    const auto reopened_blocks = reopened.inspect_body_blocks();
    REQUIRE_EQ(reopened_blocks.size(), 3U);
    CHECK_EQ(reopened_blocks[0].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(reopened_blocks[1].kind, featherdoc::body_block_kind::table);
    CHECK_EQ(reopened_blocks[2].kind, featherdoc::body_block_kind::paragraph);
    const auto reopened_subtotal =
        reopened.inspect_table_cell_by_grid_column(0U, 2U, 2U);
    const auto reopened_later =
        reopened.inspect_table_cell_by_grid_column(0U, 3U, 0U);
    const auto reopened_summary =
        reopened.inspect_table_cell_by_grid_column(0U, 5U, 2U);
    REQUIRE(reopened_subtotal.has_value());
    REQUIRE(reopened_later.has_value());
    REQUIRE(reopened_summary.has_value());
    CHECK_EQ(reopened_subtotal->column_span, 3U);
    CHECK(featherdoc::test_support::contains_text(reopened_subtotal->text,
                                                  "Design subtotal"));
    CHECK(featherdoc::test_support::contains_text(reopened_later->text,
                                                  "Visual validation"));
    CHECK_EQ(reopened_summary->column_span, 3U);
    CHECK(featherdoc::test_support::contains_text(reopened_summary->text,
                                                  "Grand total"));

    if (std::getenv("FEATHERDOC_KEEP_PDF_IMPORT_TEST_OUTPUTS") == nullptr) {
        std::filesystem::remove(docx_path);
    }
}

TEST_CASE("PDF text importer preserves right subtotal-row table cells") {
    const auto input_path =
        featherdoc::test_support::write_invoice_grid_right_subtotal_pdf(
            "featherdoc-pdf-import-right-subtotal-row-table.pdf");
    const auto docx_path =
        std::filesystem::current_path() /
        "featherdoc-pdf-import-right-subtotal-row-table.docx";
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

    const auto blocks = document.inspect_body_blocks();
    REQUIRE_EQ(blocks.size(), 3U);
    CHECK_EQ(blocks[0].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(blocks[1].kind, featherdoc::body_block_kind::table);
    CHECK_EQ(blocks[2].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(featherdoc::test_support::collect_document_text(document),
             "Invoice right subtotal grid sample\n"
             "Footer note: right subtotal labels keep item column blank\n");

    const auto imported_table = document.inspect_table(0U);
    REQUIRE(imported_table.has_value());
    CHECK_EQ(imported_table->row_count, 6U);
    CHECK_EQ(imported_table->column_count, 4U);
    CHECK_EQ(document.inspect_table_cells(0U).size(), 22U);

    const auto subtotal_blank =
        document.inspect_table_cell_by_grid_column(0U, 2U, 0U);
    const auto subtotal_label =
        document.inspect_table_cell_by_grid_column(0U, 2U, 1U);
    const auto subtotal_covered =
        document.inspect_table_cell_by_grid_column(0U, 2U, 2U);
    const auto subtotal_amount =
        document.inspect_table_cell_by_grid_column(0U, 2U, 3U);
    const auto later_data =
        document.inspect_table_cell_by_grid_column(0U, 3U, 0U);
    const auto summary_label =
        document.inspect_table_cell_by_grid_column(0U, 5U, 2U);
    REQUIRE(subtotal_blank.has_value());
    REQUIRE(subtotal_label.has_value());
    REQUIRE(subtotal_covered.has_value());
    REQUIRE(subtotal_amount.has_value());
    REQUIRE(later_data.has_value());
    REQUIRE(summary_label.has_value());
    CHECK_EQ(subtotal_blank->column_span, 1U);
    CHECK_EQ(subtotal_blank->text, "");
    CHECK_EQ(subtotal_label->column_span, 2U);
    CHECK_EQ(subtotal_covered->cell_index, subtotal_label->cell_index);
    CHECK(featherdoc::test_support::contains_text(subtotal_label->text,
                                                  "Design subtotal"));
    CHECK(featherdoc::test_support::contains_text(subtotal_amount->text,
                                                  "USD 100"));
    CHECK(featherdoc::test_support::contains_text(later_data->text,
                                                  "Visual validation"));
    CHECK_EQ(summary_label->column_span, 2U);
    CHECK(featherdoc::test_support::contains_text(summary_label->text,
                                                  "Grand total"));

    REQUIRE_FALSE(document.save());

    featherdoc::Document reopened(docx_path);
    REQUIRE_FALSE(reopened.open());
    const auto reopened_blocks = reopened.inspect_body_blocks();
    REQUIRE_EQ(reopened_blocks.size(), 3U);
    CHECK_EQ(reopened_blocks[0].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(reopened_blocks[1].kind, featherdoc::body_block_kind::table);
    CHECK_EQ(reopened_blocks[2].kind, featherdoc::body_block_kind::paragraph);
    const auto reopened_subtotal =
        reopened.inspect_table_cell_by_grid_column(0U, 2U, 2U);
    const auto reopened_later =
        reopened.inspect_table_cell_by_grid_column(0U, 3U, 0U);
    const auto reopened_summary =
        reopened.inspect_table_cell_by_grid_column(0U, 5U, 2U);
    REQUIRE(reopened_subtotal.has_value());
    REQUIRE(reopened_later.has_value());
    REQUIRE(reopened_summary.has_value());
    CHECK_EQ(reopened_subtotal->column_span, 2U);
    CHECK(featherdoc::test_support::contains_text(reopened_subtotal->text,
                                                  "Design subtotal"));
    CHECK(featherdoc::test_support::contains_text(reopened_later->text,
                                                  "Visual validation"));
    CHECK_EQ(reopened_summary->column_span, 2U);
    CHECK(featherdoc::test_support::contains_text(reopened_summary->text,
                                                  "Grand total"));

    if (std::getenv("FEATHERDOC_KEEP_PDF_IMPORT_TEST_OUTPUTS") == nullptr) {
        std::filesystem::remove(docx_path);
    }
}

TEST_CASE("PDF text importer preserves middle-amount subtotal-row table cells") {
    const auto input_path = featherdoc::test_support::
        write_invoice_grid_middle_amount_subtotal_pdf(
            "featherdoc-pdf-import-middle-amount-subtotal-row-table.pdf");
    const auto docx_path =
        std::filesystem::current_path() /
        "featherdoc-pdf-import-middle-amount-subtotal-row-table.docx";
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

    const auto blocks = document.inspect_body_blocks();
    REQUIRE_EQ(blocks.size(), 3U);
    CHECK_EQ(blocks[0].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(blocks[1].kind, featherdoc::body_block_kind::table);
    CHECK_EQ(blocks[2].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(featherdoc::test_support::collect_document_text(document),
             "Invoice middle amount subtotal grid sample\n"
             "Footer note: subtotal amount can occupy a middle column\n");

    const auto imported_table = document.inspect_table(0U);
    REQUIRE(imported_table.has_value());
    CHECK_EQ(imported_table->row_count, 6U);
    CHECK_EQ(imported_table->column_count, 4U);
    CHECK_EQ(document.inspect_table_cells(0U).size(), 22U);

    const auto subtotal_label =
        document.inspect_table_cell_by_grid_column(0U, 2U, 0U);
    const auto subtotal_covered =
        document.inspect_table_cell_by_grid_column(0U, 2U, 1U);
    const auto subtotal_amount =
        document.inspect_table_cell_by_grid_column(0U, 2U, 2U);
    const auto subtotal_trailing_blank =
        document.inspect_table_cell_by_grid_column(0U, 2U, 3U);
    const auto later_data =
        document.inspect_table_cell_by_grid_column(0U, 3U, 0U);
    const auto summary_label =
        document.inspect_table_cell_by_grid_column(0U, 5U, 1U);
    const auto summary_amount =
        document.inspect_table_cell_by_grid_column(0U, 5U, 2U);
    REQUIRE(subtotal_label.has_value());
    REQUIRE(subtotal_covered.has_value());
    REQUIRE(subtotal_amount.has_value());
    REQUIRE(subtotal_trailing_blank.has_value());
    REQUIRE(later_data.has_value());
    REQUIRE(summary_label.has_value());
    REQUIRE(summary_amount.has_value());
    CHECK_EQ(subtotal_label->column_span, 2U);
    CHECK_EQ(subtotal_covered->cell_index, subtotal_label->cell_index);
    CHECK(featherdoc::test_support::contains_text(subtotal_label->text,
                                                  "Design subtotal"));
    CHECK(featherdoc::test_support::contains_text(subtotal_amount->text,
                                                  "USD 100"));
    CHECK_EQ(subtotal_trailing_blank->text, "");
    CHECK(featherdoc::test_support::contains_text(later_data->text,
                                                  "Visual validation"));
    CHECK_EQ(summary_label->column_span, 2U);
    CHECK(featherdoc::test_support::contains_text(summary_label->text,
                                                  "Grand total"));
    CHECK(featherdoc::test_support::contains_text(summary_amount->text,
                                                  "USD 135"));

    REQUIRE_FALSE(document.save());

    featherdoc::Document reopened(docx_path);
    REQUIRE_FALSE(reopened.open());
    const auto reopened_blocks = reopened.inspect_body_blocks();
    REQUIRE_EQ(reopened_blocks.size(), 3U);
    CHECK_EQ(reopened_blocks[0].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(reopened_blocks[1].kind, featherdoc::body_block_kind::table);
    CHECK_EQ(reopened_blocks[2].kind, featherdoc::body_block_kind::paragraph);
    const auto reopened_subtotal =
        reopened.inspect_table_cell_by_grid_column(0U, 2U, 1U);
    const auto reopened_later =
        reopened.inspect_table_cell_by_grid_column(0U, 3U, 0U);
    const auto reopened_summary =
        reopened.inspect_table_cell_by_grid_column(0U, 5U, 1U);
    REQUIRE(reopened_subtotal.has_value());
    REQUIRE(reopened_later.has_value());
    REQUIRE(reopened_summary.has_value());
    CHECK_EQ(reopened_subtotal->column_span, 2U);
    CHECK(featherdoc::test_support::contains_text(reopened_subtotal->text,
                                                  "Design subtotal"));
    CHECK(featherdoc::test_support::contains_text(reopened_later->text,
                                                  "Visual validation"));
    CHECK_EQ(reopened_summary->column_span, 2U);
    CHECK(featherdoc::test_support::contains_text(reopened_summary->text,
                                                  "Grand total"));

    if (std::getenv("FEATHERDOC_KEEP_PDF_IMPORT_TEST_OUTPUTS") == nullptr) {
        std::filesystem::remove(docx_path);
    }
}
