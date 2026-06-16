#include "doctest.h"

#include "pdf_import_test_support.hpp"

#include <featherdoc/pdf/pdf_document_importer.hpp>
#include <featherdoc/pdf/pdf_parser.hpp>

#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <numeric>

TEST_CASE(
    "PDF table import keeps cross-page subtotal tables separate for anchor mismatches") {
    const auto input_path = featherdoc::test_support::
        write_invoice_grid_pagebreak_subtotal_anchor_mismatch_pdf(
            "featherdoc-pdf-import-pagebreak-subtotal-anchor-mismatch-table.pdf");
    const auto docx_path =
        std::filesystem::current_path() /
        "featherdoc-pdf-import-pagebreak-subtotal-anchor-mismatch-table.docx";
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

    const auto &pagebreak_diagnostic =
        import_result.table_continuation_diagnostics[1];
    CHECK(pagebreak_diagnostic.previous_has_repeating_header);
    CHECK(pagebreak_diagnostic.source_has_repeating_header);
    CHECK(pagebreak_diagnostic.header_matches_previous);
    CHECK_EQ(pagebreak_diagnostic.header_match_kind,
             featherdoc::pdf::PdfTableContinuationHeaderMatchKind::exact);
    CHECK(pagebreak_diagnostic.column_count_matches);
    CHECK_FALSE(pagebreak_diagnostic.column_anchors_match);
    CHECK_FALSE(pagebreak_diagnostic.skipped_repeating_header);
    CHECK_EQ(pagebreak_diagnostic.source_row_offset, 0U);
    CHECK_EQ(pagebreak_diagnostic.disposition,
             featherdoc::pdf::PdfTableContinuationDisposition::created_new_table);
    CHECK_EQ(pagebreak_diagnostic.blocker,
             featherdoc::pdf::PdfTableContinuationBlocker::column_anchors_mismatch);

    const auto blocks = document.inspect_body_blocks();
    REQUIRE_EQ(blocks.size(), 4U);
    CHECK_EQ(blocks[0].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(blocks[1].kind, featherdoc::body_block_kind::table);
    CHECK_EQ(blocks[2].kind, featherdoc::body_block_kind::table);
    CHECK_EQ(blocks[3].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(featherdoc::test_support::collect_document_text(document),
             "Invoice pagebreak subtotal anchor mismatch sample\n"
             "Footer note: anchor-mismatched subtotal table stays separate\n");

    const auto first_table = document.inspect_table(0U);
    const auto second_table = document.inspect_table(1U);
    REQUIRE(first_table.has_value());
    REQUIRE(second_table.has_value());
    CHECK_EQ(first_table->row_count, 4U);
    CHECK_EQ(first_table->column_count, 4U);
    CHECK_EQ(second_table->row_count, 4U);
    CHECK_EQ(second_table->column_count, 4U);
    CHECK_EQ(document.inspect_table_cells(0U).size(), 14U);
    CHECK_EQ(document.inspect_table_cells(1U).size(), 14U);
    REQUIRE_EQ(first_table->row_repeats_header.size(), 4U);
    REQUIRE_EQ(second_table->row_repeats_header.size(), 4U);
    CHECK(first_table->row_repeats_header[0]);
    CHECK(second_table->row_repeats_header[0]);

    const auto first_subtotal =
        document.inspect_table_cell_by_grid_column(0U, 2U, 0U);
    const auto second_header_qty =
        document.inspect_table_cell_by_grid_column(1U, 0U, 1U);
    const auto second_first_body =
        document.inspect_table_cell_by_grid_column(1U, 1U, 0U);
    const auto second_total =
        document.inspect_table_cell_by_grid_column(1U, 3U, 0U);
    REQUIRE(first_subtotal.has_value());
    REQUIRE(second_header_qty.has_value());
    REQUIRE(second_first_body.has_value());
    REQUIRE(second_total.has_value());
    CHECK_EQ(first_subtotal->column_span, 3U);
    CHECK(featherdoc::test_support::contains_text(first_subtotal->text,
                                                  "Design subtotal"));
    CHECK(featherdoc::test_support::contains_text(second_header_qty->text,
                                                  "Qty"));
    CHECK(featherdoc::test_support::contains_text(second_first_body->text,
                                                  "Regression evidence"));
    CHECK_EQ(second_total->column_span, 3U);
    CHECK(featherdoc::test_support::contains_text(second_total->text,
                                                  "Grand total"));
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

    const auto reopened_first_table = reopened.inspect_table(0U);
    const auto reopened_second_table = reopened.inspect_table(1U);
    REQUIRE(reopened_first_table.has_value());
    REQUIRE(reopened_second_table.has_value());
    CHECK_EQ(reopened_first_table->row_count, 4U);
    CHECK_EQ(reopened_second_table->row_count, 4U);
    CHECK_FALSE(reopened.inspect_table(2U).has_value());

    const auto reopened_first_subtotal =
        reopened.inspect_table_cell_by_grid_column(0U, 2U, 2U);
    const auto reopened_second_total =
        reopened.inspect_table_cell_by_grid_column(1U, 3U, 2U);
    REQUIRE(reopened_first_subtotal.has_value());
    REQUIRE(reopened_second_total.has_value());
    CHECK_EQ(reopened_first_subtotal->column_span, 3U);
    CHECK(featherdoc::test_support::contains_text(
        reopened_first_subtotal->text, "Design subtotal"));
    CHECK_EQ(reopened_second_total->column_span, 3U);
    CHECK(featherdoc::test_support::contains_text(reopened_second_total->text,
                                                  "Grand total"));

    if (std::getenv("FEATHERDOC_KEEP_PDF_IMPORT_TEST_OUTPUTS") == nullptr) {
        std::filesystem::remove(docx_path);
    }
}

TEST_CASE(
    "PDF table import keeps cross-page subtotal tables separate for local anchor drift") {
    const auto input_path = featherdoc::test_support::
        write_invoice_grid_pagebreak_subtotal_local_anchor_drift_pdf(
            "featherdoc-pdf-import-pagebreak-subtotal-local-anchor-drift-table.pdf");
    const auto docx_path =
        std::filesystem::current_path() /
        "featherdoc-pdf-import-pagebreak-subtotal-local-anchor-drift-table.docx";
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

    const auto &pagebreak_diagnostic =
        import_result.table_continuation_diagnostics[1];
    CHECK(pagebreak_diagnostic.previous_has_repeating_header);
    CHECK(pagebreak_diagnostic.source_has_repeating_header);
    CHECK_FALSE(pagebreak_diagnostic.column_anchors_match);
    CHECK_FALSE(pagebreak_diagnostic.skipped_repeating_header);
    CHECK_EQ(pagebreak_diagnostic.source_row_offset, 0U);
    CHECK_EQ(pagebreak_diagnostic.disposition,
             featherdoc::pdf::PdfTableContinuationDisposition::created_new_table);
    CHECK_EQ(pagebreak_diagnostic.blocker,
             featherdoc::pdf::PdfTableContinuationBlocker::column_anchors_mismatch);

    const auto blocks = document.inspect_body_blocks();
    REQUIRE_EQ(blocks.size(), 4U);
    CHECK_EQ(blocks[0].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(blocks[1].kind, featherdoc::body_block_kind::table);
    CHECK_EQ(blocks[2].kind, featherdoc::body_block_kind::table);
    CHECK_EQ(blocks[3].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(featherdoc::test_support::collect_document_text(document),
             "Invoice pagebreak subtotal local anchor drift sample\n"
             "Footer note: local anchor drift keeps subtotal table separate\n");

    const auto first_table = document.inspect_table(0U);
    const auto second_table = document.inspect_table(1U);
    REQUIRE(first_table.has_value());
    REQUIRE(second_table.has_value());
    CHECK_EQ(first_table->row_count, 4U);
    CHECK_EQ(second_table->row_count, 4U);
    CHECK_EQ(first_table->column_count, second_table->column_count);
    CHECK_FALSE(document.inspect_table(2U).has_value());

    const auto first_subtotal =
        document.inspect_table_cell_by_grid_column(0U, 2U, 0U);
    const auto second_first_body =
        document.inspect_table_cell_by_grid_column(1U, 1U, 0U);
    const auto second_total =
        document.inspect_table_cell_by_grid_column(1U, 3U, 0U);
    REQUIRE(first_subtotal.has_value());
    REQUIRE(second_first_body.has_value());
    REQUIRE(second_total.has_value());
    CHECK_EQ(first_subtotal->column_span, 3U);
    CHECK(featherdoc::test_support::contains_text(first_subtotal->text,
                                                  "Design subtotal"));
    CHECK(featherdoc::test_support::contains_text(second_first_body->text,
                                                  "Regression evidence"));
    CHECK_EQ(second_total->column_span, 3U);
    CHECK(featherdoc::test_support::contains_text(second_total->text,
                                                  "Grand total"));

    REQUIRE_FALSE(document.save());

    featherdoc::Document reopened(docx_path);
    REQUIRE_FALSE(reopened.open());
    const auto reopened_blocks = reopened.inspect_body_blocks();
    REQUIRE_EQ(reopened_blocks.size(), 4U);
    CHECK_EQ(reopened_blocks[0].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(reopened_blocks[1].kind, featherdoc::body_block_kind::table);
    CHECK_EQ(reopened_blocks[2].kind, featherdoc::body_block_kind::table);
    CHECK_EQ(reopened_blocks[3].kind, featherdoc::body_block_kind::paragraph);
    CHECK_FALSE(reopened.inspect_table(2U).has_value());

    const auto reopened_first_subtotal =
        reopened.inspect_table_cell_by_grid_column(0U, 2U, 2U);
    const auto reopened_second_total =
        reopened.inspect_table_cell_by_grid_column(1U, 3U, 2U);
    REQUIRE(reopened_first_subtotal.has_value());
    REQUIRE(reopened_second_total.has_value());
    CHECK_EQ(reopened_first_subtotal->column_span, 3U);
    CHECK(featherdoc::test_support::contains_text(
        reopened_first_subtotal->text, "Design subtotal"));
    CHECK_EQ(reopened_second_total->column_span, 3U);
    CHECK(featherdoc::test_support::contains_text(reopened_second_total->text,
                                                  "Grand total"));

    if (std::getenv("FEATHERDOC_KEEP_PDF_IMPORT_TEST_OUTPUTS") == nullptr) {
        std::filesystem::remove(docx_path);
    }
}

TEST_CASE(
    "PDF table import keeps cross-page subtotal tables separate for column count mismatches") {
    const auto input_path = featherdoc::test_support::
        write_invoice_grid_pagebreak_subtotal_column_count_mismatch_pdf(
            "featherdoc-pdf-import-pagebreak-subtotal-column-count-mismatch-table.pdf");
    const auto docx_path =
        std::filesystem::current_path() /
        "featherdoc-pdf-import-pagebreak-subtotal-column-count-mismatch-table.docx";
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

    const auto &pagebreak_diagnostic =
        import_result.table_continuation_diagnostics[1];
    CHECK(pagebreak_diagnostic.previous_has_repeating_header);
    CHECK(pagebreak_diagnostic.source_has_repeating_header);
    CHECK_FALSE(pagebreak_diagnostic.column_count_matches);
    CHECK_FALSE(pagebreak_diagnostic.column_anchors_match);
    CHECK_FALSE(pagebreak_diagnostic.skipped_repeating_header);
    CHECK_EQ(pagebreak_diagnostic.source_row_offset, 0U);
    CHECK_EQ(pagebreak_diagnostic.header_match_kind,
             featherdoc::pdf::PdfTableContinuationHeaderMatchKind::none);
    CHECK_FALSE(pagebreak_diagnostic.header_matches_previous);
    CHECK_EQ(pagebreak_diagnostic.disposition,
             featherdoc::pdf::PdfTableContinuationDisposition::created_new_table);
    CHECK_EQ(pagebreak_diagnostic.blocker,
             featherdoc::pdf::PdfTableContinuationBlocker::column_count_mismatch);

    const auto blocks = document.inspect_body_blocks();
    REQUIRE_EQ(blocks.size(), 4U);
    CHECK_EQ(blocks[0].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(blocks[1].kind, featherdoc::body_block_kind::table);
    CHECK_EQ(blocks[2].kind, featherdoc::body_block_kind::table);
    CHECK_EQ(blocks[3].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(featherdoc::test_support::collect_document_text(document),
             "Invoice pagebreak subtotal column count mismatch sample\n"
             "Footer note: column-count-mismatched subtotal table stays separate\n");

    const auto first_table = document.inspect_table(0U);
    const auto second_table = document.inspect_table(1U);
    REQUIRE(first_table.has_value());
    REQUIRE(second_table.has_value());
    CHECK_EQ(first_table->row_count, 4U);
    CHECK_EQ(first_table->column_count, 4U);
    CHECK_EQ(second_table->row_count, 4U);
    CHECK_EQ(second_table->column_count, 3U);
    CHECK_EQ(document.inspect_table_cells(0U).size(), 14U);
    CHECK_EQ(document.inspect_table_cells(1U).size(), 11U);
    REQUIRE_EQ(first_table->row_repeats_header.size(), 4U);
    REQUIRE_EQ(second_table->row_repeats_header.size(), 4U);
    CHECK(first_table->row_repeats_header[0]);
    CHECK(second_table->row_repeats_header[0]);

    const auto first_subtotal =
        document.inspect_table_cell_by_grid_column(0U, 2U, 0U);
    const auto second_header_qty =
        document.inspect_table_cell_by_grid_column(1U, 0U, 1U);
    const auto second_first_body =
        document.inspect_table_cell_by_grid_column(1U, 1U, 0U);
    const auto second_total =
        document.inspect_table_cell_by_grid_column(1U, 3U, 0U);
    REQUIRE(first_subtotal.has_value());
    REQUIRE(second_header_qty.has_value());
    REQUIRE(second_first_body.has_value());
    REQUIRE(second_total.has_value());
    CHECK_EQ(first_subtotal->column_span, 3U);
    CHECK(featherdoc::test_support::contains_text(first_subtotal->text,
                                                  "Design subtotal"));
    CHECK(featherdoc::test_support::contains_text(second_header_qty->text,
                                                  "Qty"));
    CHECK(featherdoc::test_support::contains_text(second_first_body->text,
                                                  "Regression evidence"));
    CHECK_EQ(second_total->column_span, 2U);
    CHECK(featherdoc::test_support::contains_text(second_total->text,
                                                  "Grand total"));
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

    const auto reopened_first_table = reopened.inspect_table(0U);
    const auto reopened_second_table = reopened.inspect_table(1U);
    REQUIRE(reopened_first_table.has_value());
    REQUIRE(reopened_second_table.has_value());
    CHECK_EQ(reopened_first_table->row_count, 4U);
    CHECK_EQ(reopened_first_table->column_count, 4U);
    CHECK_EQ(reopened_second_table->row_count, 4U);
    CHECK_EQ(reopened_second_table->column_count, 3U);
    CHECK_FALSE(reopened.inspect_table(2U).has_value());

    const auto reopened_first_subtotal =
        reopened.inspect_table_cell_by_grid_column(0U, 2U, 2U);
    const auto reopened_second_total =
        reopened.inspect_table_cell_by_grid_column(1U, 3U, 1U);
    REQUIRE(reopened_first_subtotal.has_value());
    REQUIRE(reopened_second_total.has_value());
    CHECK_EQ(reopened_first_subtotal->column_span, 3U);
    CHECK(featherdoc::test_support::contains_text(
        reopened_first_subtotal->text, "Design subtotal"));
    CHECK_EQ(reopened_second_total->column_span, 2U);
    CHECK(featherdoc::test_support::contains_text(reopened_second_total->text,
                                                  "Grand total"));

    if (std::getenv("FEATHERDOC_KEEP_PDF_IMPORT_TEST_OUTPUTS") == nullptr) {
        std::filesystem::remove(docx_path);
    }
}
