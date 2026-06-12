#include "doctest.h"

#include "pdf_import_test_support.hpp"

#include <featherdoc/pdf/pdf_document_importer.hpp>
#include <featherdoc/pdf/pdf_parser.hpp>

#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <numeric>

TEST_CASE(
    "PDF table import merges cross-page subtotal rows with amount-only body rows") {
    const auto input_path = featherdoc::test_support::
        write_invoice_grid_pagebreak_subtotal_amount_only_body_pdf(
            "featherdoc-pdf-import-pagebreak-subtotal-amount-only-body-table.pdf");
    const auto docx_path =
        std::filesystem::current_path() /
        "featherdoc-pdf-import-pagebreak-subtotal-amount-only-body-table.docx";
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
             "Invoice pagebreak subtotal amount only body sample\n"
             "Footer note: amount-only body row still aligns with the repeated header\n");

    const auto imported_table = document.inspect_table(0U);
    REQUIRE(imported_table.has_value());
    CHECK_EQ(imported_table->row_count, 7U);
    CHECK_EQ(imported_table->column_count, 4U);
    CHECK_EQ(document.inspect_table_cells(0U).size(), 24U);
    REQUIRE_EQ(imported_table->row_repeats_header.size(), 7U);
    CHECK(imported_table->row_repeats_header[0]);
    CHECK_FALSE(imported_table->row_repeats_header[4]);
    CHECK_FALSE(imported_table->row_repeats_header.back());

    const auto amount_only_item =
        document.inspect_table_cell_by_grid_column(0U, 4U, 0U);
    const auto amount_only_qty =
        document.inspect_table_cell_by_grid_column(0U, 4U, 1U);
    const auto amount_only_unit =
        document.inspect_table_cell_by_grid_column(0U, 4U, 2U);
    const auto amount_only_total =
        document.inspect_table_cell_by_grid_column(0U, 4U, 3U);
    const auto later_item =
        document.inspect_table_cell_by_grid_column(0U, 5U, 0U);
    const auto later_unit =
        document.inspect_table_cell_by_grid_column(0U, 5U, 2U);
    const auto grand_total_label =
        document.inspect_table_cell_by_grid_column(0U, 6U, 0U);
    const auto grand_total_amount =
        document.inspect_table_cell_by_grid_column(0U, 6U, 3U);
    REQUIRE(amount_only_item.has_value());
    REQUIRE(amount_only_qty.has_value());
    REQUIRE(amount_only_unit.has_value());
    REQUIRE(amount_only_total.has_value());
    REQUIRE(later_item.has_value());
    REQUIRE(later_unit.has_value());
    REQUIRE(grand_total_label.has_value());
    REQUIRE(grand_total_amount.has_value());
    CHECK_EQ(amount_only_item->text, "");
    CHECK_EQ(amount_only_qty->text, "");
    CHECK_EQ(amount_only_unit->text, "");
    CHECK(featherdoc::test_support::contains_text(amount_only_total->text,
                                                  "USD 10"));
    CHECK(featherdoc::test_support::contains_text(later_item->text,
                                                  "Documentation pass"));
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

    const auto reopened_amount_only_item =
        reopened.inspect_table_cell_by_grid_column(0U, 4U, 0U);
    const auto reopened_amount_only_total =
        reopened.inspect_table_cell_by_grid_column(0U, 4U, 3U);
    const auto reopened_grand_total =
        reopened.inspect_table_cell_by_grid_column(0U, 6U, 2U);
    REQUIRE(reopened_amount_only_item.has_value());
    REQUIRE(reopened_amount_only_total.has_value());
    REQUIRE(reopened_grand_total.has_value());
    CHECK_EQ(reopened_amount_only_item->text, "");
    CHECK(featherdoc::test_support::contains_text(
        reopened_amount_only_total->text, "USD 10"));
    CHECK_EQ(reopened_grand_total->column_span, 3U);
    CHECK(featherdoc::test_support::contains_text(reopened_grand_total->text,
                                                  "Grand total"));

    if (std::getenv("FEATHERDOC_KEEP_PDF_IMPORT_TEST_OUTPUTS") == nullptr) {
        std::filesystem::remove(docx_path);
    }
}

TEST_CASE(
    "PDF table import merges cross-page subtotal rows with isolated amount-only body rows") {
    const auto input_path = featherdoc::test_support::
        write_invoice_grid_pagebreak_subtotal_isolated_amount_only_body_pdf(
            "featherdoc-pdf-import-pagebreak-subtotal-isolated-amount-only-body-table.pdf");
    const auto docx_path =
        std::filesystem::current_path() /
        "featherdoc-pdf-import-pagebreak-subtotal-isolated-amount-only-body-table.docx";
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
             "Invoice pagebreak subtotal isolated amount only body sample\n"
             "Footer note: isolated amount-only body row still aligns with the repeated header\n");

    const auto imported_table = document.inspect_table(0U);
    REQUIRE(imported_table.has_value());
    CHECK_EQ(imported_table->row_count, 6U);
    CHECK_EQ(imported_table->column_count, 4U);
    CHECK_EQ(document.inspect_table_cells(0U).size(), 20U);
    REQUIRE_EQ(imported_table->row_repeats_header.size(), 6U);
    CHECK(imported_table->row_repeats_header[0]);
    CHECK_FALSE(imported_table->row_repeats_header[4]);
    CHECK_FALSE(imported_table->row_repeats_header.back());

    const auto amount_only_item =
        document.inspect_table_cell_by_grid_column(0U, 4U, 0U);
    const auto amount_only_qty =
        document.inspect_table_cell_by_grid_column(0U, 4U, 1U);
    const auto amount_only_unit =
        document.inspect_table_cell_by_grid_column(0U, 4U, 2U);
    const auto amount_only_total =
        document.inspect_table_cell_by_grid_column(0U, 4U, 3U);
    const auto grand_total_label =
        document.inspect_table_cell_by_grid_column(0U, 5U, 0U);
    const auto grand_total_amount =
        document.inspect_table_cell_by_grid_column(0U, 5U, 3U);
    REQUIRE(amount_only_item.has_value());
    REQUIRE(amount_only_qty.has_value());
    REQUIRE(amount_only_unit.has_value());
    REQUIRE(amount_only_total.has_value());
    REQUIRE(grand_total_label.has_value());
    REQUIRE(grand_total_amount.has_value());
    CHECK_EQ(amount_only_item->text, "");
    CHECK_EQ(amount_only_qty->text, "");
    CHECK_EQ(amount_only_unit->text, "");
    CHECK(featherdoc::test_support::contains_text(amount_only_total->text,
                                                  "USD 10"));
    CHECK_EQ(grand_total_label->column_span, 3U);
    CHECK(featherdoc::test_support::contains_text(grand_total_label->text,
                                                  "Grand total"));
    CHECK(featherdoc::test_support::contains_text(grand_total_amount->text,
                                                  "USD 110"));
    CHECK_FALSE(document.inspect_table(1U).has_value());

    REQUIRE_FALSE(document.save());

    featherdoc::Document reopened(docx_path);
    REQUIRE_FALSE(reopened.open());
    const auto reopened_table = reopened.inspect_table(0U);
    REQUIRE(reopened_table.has_value());
    CHECK_EQ(reopened_table->row_count, 6U);
    CHECK_EQ(reopened_table->column_count, 4U);
    CHECK_FALSE(reopened.inspect_table(1U).has_value());

    const auto reopened_amount_only_item =
        reopened.inspect_table_cell_by_grid_column(0U, 4U, 0U);
    const auto reopened_amount_only_total =
        reopened.inspect_table_cell_by_grid_column(0U, 4U, 3U);
    const auto reopened_grand_total =
        reopened.inspect_table_cell_by_grid_column(0U, 5U, 2U);
    REQUIRE(reopened_amount_only_item.has_value());
    REQUIRE(reopened_amount_only_total.has_value());
    REQUIRE(reopened_grand_total.has_value());
    CHECK_EQ(reopened_amount_only_item->text, "");
    CHECK(featherdoc::test_support::contains_text(
        reopened_amount_only_total->text, "USD 10"));
    CHECK_EQ(reopened_grand_total->column_span, 3U);
    CHECK(featherdoc::test_support::contains_text(reopened_grand_total->text,
                                                  "Grand total"));

    if (std::getenv("FEATHERDOC_KEEP_PDF_IMPORT_TEST_OUTPUTS") == nullptr) {
        std::filesystem::remove(docx_path);
    }
}

TEST_CASE(
    "PDF table import merges cross-page subtotal rows with abbreviated repeated headers") {
    const auto input_path = featherdoc::test_support::
        write_invoice_grid_pagebreak_subtotal_header_variant_pdf(
            "featherdoc-pdf-import-pagebreak-subtotal-header-variant-table.pdf");
    const auto docx_path =
        std::filesystem::current_path() /
        "featherdoc-pdf-import-pagebreak-subtotal-header-variant-table.docx";
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
    CHECK(pagebreak_diagnostic.header_matches_previous);
    CHECK(pagebreak_diagnostic.skipped_repeating_header);
    CHECK_EQ(pagebreak_diagnostic.source_row_offset, 1U);
    CHECK_EQ(pagebreak_diagnostic.header_match_kind,
             featherdoc::pdf::PdfTableContinuationHeaderMatchKind::canonical_text);

    const auto blocks = document.inspect_body_blocks();
    REQUIRE_EQ(blocks.size(), 3U);
    CHECK_EQ(blocks[0].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(blocks[1].kind, featherdoc::body_block_kind::table);
    CHECK_EQ(blocks[2].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(featherdoc::test_support::collect_document_text(document),
             "Invoice pagebreak subtotal header variant sample\n"
             "Footer note: abbreviated repeated header subtotal rows merge cleanly\n");

    const auto imported_table = document.inspect_table(0U);
    REQUIRE(imported_table.has_value());
    CHECK_EQ(imported_table->row_count, 7U);
    CHECK_EQ(imported_table->column_count, 4U);
    CHECK_EQ(document.inspect_table_cells(0U).size(), 24U);
    REQUIRE_EQ(imported_table->row_repeats_header.size(), 7U);
    CHECK(imported_table->row_repeats_header[0]);
    CHECK_FALSE(imported_table->row_repeats_header[4]);
    CHECK_FALSE(imported_table->row_repeats_header.back());

    const auto header_quantity =
        document.inspect_table_cell_by_grid_column(0U, 0U, 1U);
    const auto header_amount =
        document.inspect_table_cell_by_grid_column(0U, 0U, 3U);
    const auto pagebreak_first_data =
        document.inspect_table_cell_by_grid_column(0U, 4U, 0U);
    const auto skipped_header_quantity =
        document.inspect_table_cell_by_grid_column(0U, 4U, 1U);
    const auto subtotal_label =
        document.inspect_table_cell_by_grid_column(0U, 2U, 0U);
    const auto subtotal_amount =
        document.inspect_table_cell_by_grid_column(0U, 2U, 3U);
    const auto grand_total_label =
        document.inspect_table_cell_by_grid_column(0U, 6U, 0U);
    const auto grand_total_amount =
        document.inspect_table_cell_by_grid_column(0U, 6U, 3U);
    REQUIRE(header_quantity.has_value());
    REQUIRE(header_amount.has_value());
    REQUIRE(pagebreak_first_data.has_value());
    REQUIRE(skipped_header_quantity.has_value());
    REQUIRE(subtotal_label.has_value());
    REQUIRE(subtotal_amount.has_value());
    REQUIRE(grand_total_label.has_value());
    REQUIRE(grand_total_amount.has_value());
    CHECK(featherdoc::test_support::contains_text(header_quantity->text,
                                                  "Quantity"));
    CHECK(featherdoc::test_support::contains_text(header_amount->text,
                                                  "Amount"));
    CHECK(featherdoc::test_support::contains_text(pagebreak_first_data->text,
                                                  "Regression evidence"));
    CHECK_FALSE(featherdoc::test_support::contains_text(
        skipped_header_quantity->text, "Qty"));
    CHECK_EQ(subtotal_label->column_span, 3U);
    CHECK(featherdoc::test_support::contains_text(subtotal_label->text,
                                                  "Design subtotal"));
    CHECK(featherdoc::test_support::contains_text(subtotal_amount->text,
                                                  "USD 100"));
    CHECK_EQ(grand_total_label->column_span, 3U);
    CHECK(featherdoc::test_support::contains_text(grand_total_label->text,
                                                  "Grand total"));
    CHECK(featherdoc::test_support::contains_text(grand_total_amount->text,
                                                  "USD 150"));

    REQUIRE_FALSE(document.save());

    featherdoc::Document reopened(docx_path);
    REQUIRE_FALSE(reopened.open());
    const auto reopened_table = reopened.inspect_table(0U);
    REQUIRE(reopened_table.has_value());
    CHECK_EQ(reopened_table->row_count, 7U);
    CHECK_EQ(reopened_table->column_count, 4U);
    REQUIRE_EQ(reopened_table->row_repeats_header.size(), 7U);
    CHECK(reopened_table->row_repeats_header[0]);
    CHECK_FALSE(reopened_table->row_repeats_header[4]);
    CHECK_FALSE(reopened.inspect_table(1U).has_value());

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
    "PDF table import keeps cross-page subtotal tables separate for semantic header variants") {
    const auto input_path = featherdoc::test_support::
        write_invoice_grid_pagebreak_subtotal_semantic_header_variant_pdf(
            "featherdoc-pdf-import-pagebreak-subtotal-semantic-header-variant-table.pdf");
    const auto docx_path =
        std::filesystem::current_path() /
        "featherdoc-pdf-import-pagebreak-subtotal-semantic-header-variant-table.docx";
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
    CHECK_FALSE(pagebreak_diagnostic.header_matches_previous);
    CHECK_FALSE(pagebreak_diagnostic.skipped_repeating_header);
    CHECK_EQ(pagebreak_diagnostic.source_row_offset, 0U);
    CHECK_EQ(pagebreak_diagnostic.header_match_kind,
             featherdoc::pdf::PdfTableContinuationHeaderMatchKind::none);
    CHECK_EQ(pagebreak_diagnostic.disposition,
             featherdoc::pdf::PdfTableContinuationDisposition::created_new_table);
    CHECK_EQ(pagebreak_diagnostic.blocker,
             featherdoc::pdf::PdfTableContinuationBlocker::repeated_header_mismatch);

    const auto blocks = document.inspect_body_blocks();
    REQUIRE_EQ(blocks.size(), 4U);
    CHECK_EQ(blocks[0].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(blocks[1].kind, featherdoc::body_block_kind::table);
    CHECK_EQ(blocks[2].kind, featherdoc::body_block_kind::table);
    CHECK_EQ(blocks[3].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(featherdoc::test_support::collect_document_text(document),
             "Invoice pagebreak subtotal semantic header variant sample\n"
             "Footer note: semantic repeated header variant stays separate\n");

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

    const auto first_quantity =
        document.inspect_table_cell_by_grid_column(0U, 0U, 1U);
    const auto first_subtotal =
        document.inspect_table_cell_by_grid_column(0U, 2U, 0U);
    const auto second_owner =
        document.inspect_table_cell_by_grid_column(1U, 0U, 1U);
    const auto second_phase =
        document.inspect_table_cell_by_grid_column(1U, 0U, 2U);
    const auto second_first_body =
        document.inspect_table_cell_by_grid_column(1U, 1U, 0U);
    const auto second_total =
        document.inspect_table_cell_by_grid_column(1U, 3U, 0U);
    REQUIRE(first_quantity.has_value());
    REQUIRE(first_subtotal.has_value());
    REQUIRE(second_owner.has_value());
    REQUIRE(second_phase.has_value());
    REQUIRE(second_first_body.has_value());
    REQUIRE(second_total.has_value());
    CHECK(featherdoc::test_support::contains_text(first_quantity->text,
                                                  "Quantity"));
    CHECK_EQ(first_subtotal->column_span, 3U);
    CHECK(featherdoc::test_support::contains_text(first_subtotal->text,
                                                  "Design subtotal"));
    CHECK(featherdoc::test_support::contains_text(second_owner->text,
                                                  "Owner"));
    CHECK(featherdoc::test_support::contains_text(second_phase->text,
                                                  "Phase"));
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
