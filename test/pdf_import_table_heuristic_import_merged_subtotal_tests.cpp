#include "doctest.h"

#include "pdf_import_test_support.hpp"

#include <featherdoc/pdf/pdf_document_importer.hpp>
#include <featherdoc/pdf/pdf_parser.hpp>

#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <numeric>

TEST_CASE(
    "PDF table import merges cross-page subtotal rows with repeated headers") {
    const auto input_path = featherdoc::test_support::
        write_invoice_grid_pagebreak_subtotal_pdf(
            "featherdoc-pdf-import-pagebreak-subtotal-row-table.pdf");
    const auto docx_path =
        std::filesystem::current_path() /
        "featherdoc-pdf-import-pagebreak-subtotal-row-table.docx";
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
    CHECK_EQ(import_result.table_continuation_diagnostics[0].disposition,
             featherdoc::pdf::PdfTableContinuationDisposition::created_new_table);
    CHECK_EQ(import_result.table_continuation_diagnostics[0].blocker,
             featherdoc::pdf::PdfTableContinuationBlocker::no_previous_table);

    const auto &pagebreak_diagnostic =
        import_result.table_continuation_diagnostics[1];
    CHECK_EQ(pagebreak_diagnostic.disposition,
             featherdoc::pdf::PdfTableContinuationDisposition::
                 merged_with_previous_table);
    CHECK_EQ(pagebreak_diagnostic.blocker,
             featherdoc::pdf::PdfTableContinuationBlocker::none);
    CHECK(pagebreak_diagnostic.header_matches_previous);
    CHECK(pagebreak_diagnostic.skipped_repeating_header);
    CHECK_EQ(pagebreak_diagnostic.source_row_offset, 1U);
    CHECK_EQ(pagebreak_diagnostic.header_match_kind,
             featherdoc::pdf::PdfTableContinuationHeaderMatchKind::exact);

    const auto blocks = document.inspect_body_blocks();
    REQUIRE_EQ(blocks.size(), 3U);
    CHECK_EQ(blocks[0].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(blocks[1].kind, featherdoc::body_block_kind::table);
    CHECK_EQ(blocks[2].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(featherdoc::test_support::collect_document_text(document),
             "Invoice pagebreak subtotal sample\n"
             "Footer note: pagebreak subtotal rows merge with the repeated header\n");

    const auto imported_table = document.inspect_table(0U);
    REQUIRE(imported_table.has_value());
    CHECK_EQ(imported_table->row_count, 7U);
    CHECK_EQ(imported_table->column_count, 4U);
    CHECK_EQ(document.inspect_table_cells(0U).size(), 24U);
    REQUIRE_EQ(imported_table->row_repeats_header.size(), 7U);
    CHECK(imported_table->row_repeats_header[0]);
    CHECK_FALSE(imported_table->row_repeats_header[1]);
    CHECK_FALSE(imported_table->row_repeats_header[4]);
    CHECK_FALSE(imported_table->row_repeats_header.back());

    const auto subtotal_label =
        document.inspect_table_cell_by_grid_column(0U, 2U, 0U);
    const auto subtotal_covered =
        document.inspect_table_cell_by_grid_column(0U, 2U, 2U);
    const auto subtotal_amount =
        document.inspect_table_cell_by_grid_column(0U, 2U, 3U);
    const auto pagebreak_first_data =
        document.inspect_table_cell_by_grid_column(0U, 4U, 0U);
    const auto pagebreak_header_duplicate =
        document.inspect_table_cell_by_grid_column(0U, 4U, 1U);
    const auto grand_total_label =
        document.inspect_table_cell_by_grid_column(0U, 6U, 0U);
    const auto grand_total_covered =
        document.inspect_table_cell_by_grid_column(0U, 6U, 2U);
    const auto grand_total_amount =
        document.inspect_table_cell_by_grid_column(0U, 6U, 3U);
    REQUIRE(subtotal_label.has_value());
    REQUIRE(subtotal_covered.has_value());
    REQUIRE(subtotal_amount.has_value());
    REQUIRE(pagebreak_first_data.has_value());
    REQUIRE(pagebreak_header_duplicate.has_value());
    REQUIRE(grand_total_label.has_value());
    REQUIRE(grand_total_covered.has_value());
    REQUIRE(grand_total_amount.has_value());
    CHECK_EQ(subtotal_label->column_span, 3U);
    CHECK_EQ(subtotal_covered->cell_index, subtotal_label->cell_index);
    CHECK(featherdoc::test_support::contains_text(subtotal_label->text,
                                                  "Design subtotal"));
    CHECK(featherdoc::test_support::contains_text(subtotal_amount->text,
                                                  "USD 100"));
    CHECK(featherdoc::test_support::contains_text(pagebreak_first_data->text,
                                                  "Regression evidence"));
    CHECK_FALSE(featherdoc::test_support::contains_text(
        pagebreak_header_duplicate->text, "Qty"));
    CHECK_EQ(grand_total_label->column_span, 3U);
    CHECK_EQ(grand_total_covered->cell_index, grand_total_label->cell_index);
    CHECK(featherdoc::test_support::contains_text(grand_total_label->text,
                                                  "Grand total"));
    CHECK(featherdoc::test_support::contains_text(grand_total_amount->text,
                                                  "USD 150"));

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
    CHECK_EQ(reopened_table->row_count, 7U);
    CHECK_EQ(reopened_table->column_count, 4U);
    REQUIRE_EQ(reopened_table->row_repeats_header.size(), 7U);
    CHECK(reopened_table->row_repeats_header[0]);
    CHECK_FALSE(reopened_table->row_repeats_header[4]);

    const auto reopened_subtotal =
        reopened.inspect_table_cell_by_grid_column(0U, 2U, 2U);
    const auto reopened_pagebreak_data =
        reopened.inspect_table_cell_by_grid_column(0U, 4U, 0U);
    const auto reopened_grand_total =
        reopened.inspect_table_cell_by_grid_column(0U, 6U, 2U);
    REQUIRE(reopened_subtotal.has_value());
    REQUIRE(reopened_pagebreak_data.has_value());
    REQUIRE(reopened_grand_total.has_value());
    CHECK_EQ(reopened_subtotal->column_span, 3U);
    CHECK(featherdoc::test_support::contains_text(reopened_subtotal->text,
                                                  "Design subtotal"));
    CHECK(featherdoc::test_support::contains_text(reopened_pagebreak_data->text,
                                                  "Regression evidence"));
    CHECK_EQ(reopened_grand_total->column_span, 3U);
    CHECK(featherdoc::test_support::contains_text(reopened_grand_total->text,
                                                  "Grand total"));

    if (std::getenv("FEATHERDOC_KEEP_PDF_IMPORT_TEST_OUTPUTS") == nullptr) {
        std::filesystem::remove(docx_path);
    }
}

TEST_CASE(
    "PDF table import merges cross-page subtotal rows with missing body cells") {
    const auto input_path = featherdoc::test_support::
        write_invoice_grid_pagebreak_subtotal_missing_unit_pdf(
            "featherdoc-pdf-import-pagebreak-subtotal-missing-unit-table.pdf");
    const auto docx_path =
        std::filesystem::current_path() /
        "featherdoc-pdf-import-pagebreak-subtotal-missing-unit-table.docx";
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

    const auto &pagebreak_diagnostic =
        import_result.table_continuation_diagnostics[1];
    CHECK_EQ(pagebreak_diagnostic.disposition,
             featherdoc::pdf::PdfTableContinuationDisposition::
                 merged_with_previous_table);
    CHECK_EQ(pagebreak_diagnostic.blocker,
             featherdoc::pdf::PdfTableContinuationBlocker::none);
    CHECK(pagebreak_diagnostic.column_count_matches);
    CHECK(pagebreak_diagnostic.column_anchors_match);
    CHECK(pagebreak_diagnostic.header_matches_previous);
    CHECK(pagebreak_diagnostic.skipped_repeating_header);
    CHECK_EQ(pagebreak_diagnostic.source_row_offset, 1U);
    CHECK_EQ(pagebreak_diagnostic.header_match_kind,
             featherdoc::pdf::PdfTableContinuationHeaderMatchKind::exact);

    const auto blocks = document.inspect_body_blocks();
    REQUIRE_EQ(blocks.size(), 3U);
    CHECK_EQ(blocks[0].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(blocks[1].kind, featherdoc::body_block_kind::table);
    CHECK_EQ(blocks[2].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(featherdoc::test_support::collect_document_text(document),
             "Invoice pagebreak subtotal missing unit sample\n"
             "Footer note: missing Unit cell still merges with the repeated header\n");

    const auto imported_table = document.inspect_table(0U);
    REQUIRE(imported_table.has_value());
    CHECK_EQ(imported_table->row_count, 7U);
    CHECK_EQ(imported_table->column_count, 4U);
    CHECK_EQ(document.inspect_table_cells(0U).size(), 24U);
    REQUIRE_EQ(imported_table->row_repeats_header.size(), 7U);
    CHECK(imported_table->row_repeats_header[0]);
    CHECK_FALSE(imported_table->row_repeats_header[4]);
    CHECK_FALSE(imported_table->row_repeats_header.back());

    const auto subtotal_label =
        document.inspect_table_cell_by_grid_column(0U, 2U, 0U);
    const auto missing_unit_item =
        document.inspect_table_cell_by_grid_column(0U, 4U, 0U);
    const auto missing_unit_qty =
        document.inspect_table_cell_by_grid_column(0U, 4U, 1U);
    const auto missing_unit_cell =
        document.inspect_table_cell_by_grid_column(0U, 4U, 2U);
    const auto missing_unit_total =
        document.inspect_table_cell_by_grid_column(0U, 4U, 3U);
    const auto later_unit =
        document.inspect_table_cell_by_grid_column(0U, 5U, 2U);
    const auto grand_total_label =
        document.inspect_table_cell_by_grid_column(0U, 6U, 0U);
    const auto grand_total_amount =
        document.inspect_table_cell_by_grid_column(0U, 6U, 3U);
    REQUIRE(subtotal_label.has_value());
    REQUIRE(missing_unit_item.has_value());
    REQUIRE(missing_unit_qty.has_value());
    REQUIRE(missing_unit_cell.has_value());
    REQUIRE(missing_unit_total.has_value());
    REQUIRE(later_unit.has_value());
    REQUIRE(grand_total_label.has_value());
    REQUIRE(grand_total_amount.has_value());
    CHECK_EQ(subtotal_label->column_span, 3U);
    CHECK(featherdoc::test_support::contains_text(subtotal_label->text,
                                                  "Design subtotal"));
    CHECK(featherdoc::test_support::contains_text(missing_unit_item->text,
                                                  "Regression evidence"));
    CHECK(featherdoc::test_support::contains_text(missing_unit_qty->text, "1"));
    CHECK_EQ(missing_unit_cell->text, "");
    CHECK(featherdoc::test_support::contains_text(missing_unit_total->text,
                                                  "USD 10"));
    CHECK(featherdoc::test_support::contains_text(later_unit->text,
                                                  "USD 15"));
    CHECK_EQ(grand_total_label->column_span, 3U);
    CHECK(featherdoc::test_support::contains_text(grand_total_label->text,
                                                  "Grand total"));
    CHECK(featherdoc::test_support::contains_text(grand_total_amount->text,
                                                  "USD 150"));
    CHECK_FALSE(document.inspect_table(1U).has_value());

    REQUIRE_FALSE(document.save());

    featherdoc::Document reopened(docx_path);
    REQUIRE_FALSE(reopened.open());
    const auto reopened_table = reopened.inspect_table(0U);
    REQUIRE(reopened_table.has_value());
    CHECK_EQ(reopened_table->row_count, 7U);
    CHECK_EQ(reopened_table->column_count, 4U);
    CHECK_FALSE(reopened.inspect_table(1U).has_value());

    const auto reopened_missing_unit =
        reopened.inspect_table_cell_by_grid_column(0U, 4U, 2U);
    const auto reopened_missing_total =
        reopened.inspect_table_cell_by_grid_column(0U, 4U, 3U);
    const auto reopened_grand_total =
        reopened.inspect_table_cell_by_grid_column(0U, 6U, 2U);
    REQUIRE(reopened_missing_unit.has_value());
    REQUIRE(reopened_missing_total.has_value());
    REQUIRE(reopened_grand_total.has_value());
    CHECK_EQ(reopened_missing_unit->text, "");
    CHECK(featherdoc::test_support::contains_text(reopened_missing_total->text,
                                                  "USD 10"));
    CHECK_EQ(reopened_grand_total->column_span, 3U);
    CHECK(featherdoc::test_support::contains_text(reopened_grand_total->text,
                                                  "Grand total"));

    if (std::getenv("FEATHERDOC_KEEP_PDF_IMPORT_TEST_OUTPUTS") == nullptr) {
        std::filesystem::remove(docx_path);
    }
}

TEST_CASE(
    "PDF table import merges cross-page subtotal rows with sparse body rows") {
    const auto input_path = featherdoc::test_support::
        write_invoice_grid_pagebreak_subtotal_sparse_body_pdf(
            "featherdoc-pdf-import-pagebreak-subtotal-sparse-body-table.pdf");
    const auto docx_path =
        std::filesystem::current_path() /
        "featherdoc-pdf-import-pagebreak-subtotal-sparse-body-table.docx";
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

    const auto &pagebreak_diagnostic =
        import_result.table_continuation_diagnostics[1];
    CHECK_EQ(pagebreak_diagnostic.disposition,
             featherdoc::pdf::PdfTableContinuationDisposition::
                 merged_with_previous_table);
    CHECK_EQ(pagebreak_diagnostic.blocker,
             featherdoc::pdf::PdfTableContinuationBlocker::none);
    CHECK(pagebreak_diagnostic.column_count_matches);
    CHECK(pagebreak_diagnostic.column_anchors_match);
    CHECK(pagebreak_diagnostic.header_matches_previous);
    CHECK(pagebreak_diagnostic.skipped_repeating_header);
    CHECK_EQ(pagebreak_diagnostic.source_row_offset, 1U);
    CHECK_EQ(pagebreak_diagnostic.header_match_kind,
             featherdoc::pdf::PdfTableContinuationHeaderMatchKind::exact);

    const auto blocks = document.inspect_body_blocks();
    REQUIRE_EQ(blocks.size(), 3U);
    CHECK_EQ(blocks[0].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(blocks[1].kind, featherdoc::body_block_kind::table);
    CHECK_EQ(blocks[2].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(featherdoc::test_support::collect_document_text(document),
             "Invoice pagebreak subtotal sparse body sample\n"
             "Footer note: sparse body row still merges with the repeated header\n");

    const auto imported_table = document.inspect_table(0U);
    REQUIRE(imported_table.has_value());
    CHECK_EQ(imported_table->row_count, 7U);
    CHECK_EQ(imported_table->column_count, 4U);
    CHECK_EQ(document.inspect_table_cells(0U).size(), 24U);
    REQUIRE_EQ(imported_table->row_repeats_header.size(), 7U);
    CHECK(imported_table->row_repeats_header[0]);
    CHECK_FALSE(imported_table->row_repeats_header[4]);
    CHECK_FALSE(imported_table->row_repeats_header.back());

    const auto sparse_item =
        document.inspect_table_cell_by_grid_column(0U, 4U, 0U);
    const auto sparse_qty =
        document.inspect_table_cell_by_grid_column(0U, 4U, 1U);
    const auto sparse_unit =
        document.inspect_table_cell_by_grid_column(0U, 4U, 2U);
    const auto sparse_total =
        document.inspect_table_cell_by_grid_column(0U, 4U, 3U);
    const auto later_qty =
        document.inspect_table_cell_by_grid_column(0U, 5U, 1U);
    const auto later_unit =
        document.inspect_table_cell_by_grid_column(0U, 5U, 2U);
    const auto grand_total_label =
        document.inspect_table_cell_by_grid_column(0U, 6U, 0U);
    const auto grand_total_amount =
        document.inspect_table_cell_by_grid_column(0U, 6U, 3U);
    REQUIRE(sparse_item.has_value());
    REQUIRE(sparse_qty.has_value());
    REQUIRE(sparse_unit.has_value());
    REQUIRE(sparse_total.has_value());
    REQUIRE(later_qty.has_value());
    REQUIRE(later_unit.has_value());
    REQUIRE(grand_total_label.has_value());
    REQUIRE(grand_total_amount.has_value());
    CHECK(featherdoc::test_support::contains_text(sparse_item->text,
                                                  "Sparse evidence"));
    CHECK_EQ(sparse_qty->text, "");
    CHECK_EQ(sparse_unit->text, "");
    CHECK(featherdoc::test_support::contains_text(sparse_total->text,
                                                  "USD 10"));
    CHECK(featherdoc::test_support::contains_text(later_qty->text, "1"));
    CHECK(featherdoc::test_support::contains_text(later_unit->text, "USD 15"));
    CHECK_EQ(grand_total_label->column_span, 3U);
    CHECK(featherdoc::test_support::contains_text(grand_total_label->text,
                                                  "Grand total"));
    CHECK(featherdoc::test_support::contains_text(grand_total_amount->text,
                                                  "USD 150"));
    CHECK_FALSE(document.inspect_table(1U).has_value());

    REQUIRE_FALSE(document.save());

    featherdoc::Document reopened(docx_path);
    REQUIRE_FALSE(reopened.open());
    const auto reopened_table = reopened.inspect_table(0U);
    REQUIRE(reopened_table.has_value());
    CHECK_EQ(reopened_table->row_count, 7U);
    CHECK_EQ(reopened_table->column_count, 4U);
    CHECK_FALSE(reopened.inspect_table(1U).has_value());

    const auto reopened_sparse_qty =
        reopened.inspect_table_cell_by_grid_column(0U, 4U, 1U);
    const auto reopened_sparse_unit =
        reopened.inspect_table_cell_by_grid_column(0U, 4U, 2U);
    const auto reopened_sparse_total =
        reopened.inspect_table_cell_by_grid_column(0U, 4U, 3U);
    const auto reopened_grand_total =
        reopened.inspect_table_cell_by_grid_column(0U, 6U, 2U);
    REQUIRE(reopened_sparse_qty.has_value());
    REQUIRE(reopened_sparse_unit.has_value());
    REQUIRE(reopened_sparse_total.has_value());
    REQUIRE(reopened_grand_total.has_value());
    CHECK_EQ(reopened_sparse_qty->text, "");
    CHECK_EQ(reopened_sparse_unit->text, "");
    CHECK(featherdoc::test_support::contains_text(reopened_sparse_total->text,
                                                  "USD 10"));
    CHECK_EQ(reopened_grand_total->column_span, 3U);
    CHECK(featherdoc::test_support::contains_text(reopened_grand_total->text,
                                                  "Grand total"));

    if (std::getenv("FEATHERDOC_KEEP_PDF_IMPORT_TEST_OUTPUTS") == nullptr) {
        std::filesystem::remove(docx_path);
    }
}
