#include "doctest.h"

#include "pdf_import_test_support.hpp"

#include <featherdoc/pdf/pdf_document_importer.hpp>
#include <featherdoc/pdf/pdf_parser.hpp>

#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <numeric>

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
