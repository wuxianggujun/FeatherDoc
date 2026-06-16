#include "doctest.h"

#include "pdf_import_test_support.hpp"

#include <featherdoc/pdf/pdf_document_importer.hpp>
#include <featherdoc/pdf/pdf_parser.hpp>

#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <numeric>

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

TEST_CASE("PDF text importer preserves center two-by-two merged table cells") {
    const auto input_path =
        featherdoc::test_support::write_paragraph_center_merged_table_paragraph_pdf(
            "featherdoc-pdf-import-center-merged-table.pdf");
    const auto docx_path =
        std::filesystem::current_path() /
        "featherdoc-pdf-import-center-merged-table.docx";
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
             "Intro paragraph before center merged table\n"
             "Tail paragraph after center merged table\n");

    const auto imported_table = document.inspect_table(0U);
    REQUIRE(imported_table.has_value());
    CHECK_EQ(imported_table->row_count, 4U);
    CHECK_EQ(imported_table->column_count, 4U);
    CHECK_EQ(document.inspect_table_cells(0U).size(), 14U);

    const auto merged_anchor =
        document.inspect_table_cell_by_grid_column(0U, 1U, 1U);
    const auto right_continuation =
        document.inspect_table_cell_by_grid_column(0U, 1U, 2U);
    const auto lower_continuation =
        document.inspect_table_cell_by_grid_column(0U, 2U, 1U);
    const auto lower_right_continuation =
        document.inspect_table_cell_by_grid_column(0U, 2U, 2U);
    const auto ship_cell = document.inspect_table_cell_by_grid_column(0U, 3U, 0U);
    REQUIRE(merged_anchor.has_value());
    REQUIRE(right_continuation.has_value());
    REQUIRE(lower_continuation.has_value());
    REQUIRE(lower_right_continuation.has_value());
    REQUIRE(ship_cell.has_value());
    CHECK_EQ(merged_anchor->column_span, 2U);
    CHECK_EQ(merged_anchor->row_span, 2U);
    CHECK_EQ(merged_anchor->vertical_merge,
             featherdoc::cell_vertical_merge::restart);
    CHECK(featherdoc::test_support::contains_text(
        merged_anchor->text, "Owner assignment spans review"));
    CHECK_EQ(right_continuation->column_span, 2U);
    CHECK_EQ(right_continuation->row_span, 2U);
    CHECK_EQ(right_continuation->vertical_merge,
             featherdoc::cell_vertical_merge::restart);
    CHECK_EQ(lower_continuation->column_span, 2U);
    CHECK_EQ(lower_continuation->vertical_merge,
             featherdoc::cell_vertical_merge::continue_merge);
    CHECK_EQ(lower_continuation->text, "");
    CHECK_EQ(lower_right_continuation->column_span, 2U);
    CHECK_EQ(lower_right_continuation->vertical_merge,
             featherdoc::cell_vertical_merge::continue_merge);
    CHECK_EQ(lower_right_continuation->text, "");
    CHECK(featherdoc::test_support::contains_text(ship_cell->text, "Ship"));

    REQUIRE_FALSE(document.save());

    featherdoc::Document reopened(docx_path);
    REQUIRE_FALSE(reopened.open());
    CHECK_EQ(reopened.inspect_table_cells(0U).size(), 14U);
    const auto reopened_anchor =
        reopened.inspect_table_cell_by_grid_column(0U, 1U, 1U);
    const auto reopened_right_continuation =
        reopened.inspect_table_cell_by_grid_column(0U, 1U, 2U);
    const auto reopened_lower_continuation =
        reopened.inspect_table_cell_by_grid_column(0U, 2U, 1U);
    const auto reopened_lower_right_continuation =
        reopened.inspect_table_cell_by_grid_column(0U, 2U, 2U);
    REQUIRE(reopened_anchor.has_value());
    REQUIRE(reopened_right_continuation.has_value());
    REQUIRE(reopened_lower_continuation.has_value());
    REQUIRE(reopened_lower_right_continuation.has_value());
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
    CHECK_EQ(reopened_lower_right_continuation->column_span, 2U);
    CHECK_EQ(reopened_lower_right_continuation->vertical_merge,
             featherdoc::cell_vertical_merge::continue_merge);

    if (std::getenv("FEATHERDOC_KEEP_PDF_IMPORT_TEST_OUTPUTS") == nullptr) {
        std::filesystem::remove(docx_path);
    }
}

TEST_CASE("PDF text importer preserves center two-by-three merged table cells") {
    const auto input_path = featherdoc::test_support::
        write_paragraph_rectangular_merged_table_paragraph_pdf(
            "featherdoc-pdf-import-rectangular-merged-table.pdf");
    const auto docx_path =
        std::filesystem::current_path() /
        "featherdoc-pdf-import-rectangular-merged-table.docx";
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
             "Intro paragraph before rectangular merged table\n"
             "Tail paragraph after rectangular merged table\n");

    const auto imported_table = document.inspect_table(0U);
    REQUIRE(imported_table.has_value());
    CHECK_EQ(imported_table->row_count, 5U);
    CHECK_EQ(imported_table->column_count, 5U);
    CHECK_EQ(document.inspect_table_cells(0U).size(), 22U);

    const auto merged_anchor =
        document.inspect_table_cell_by_grid_column(0U, 1U, 1U);
    const auto right_continuation =
        document.inspect_table_cell_by_grid_column(0U, 1U, 2U);
    const auto first_lower_continuation =
        document.inspect_table_cell_by_grid_column(0U, 2U, 1U);
    const auto first_lower_right_continuation =
        document.inspect_table_cell_by_grid_column(0U, 2U, 2U);
    const auto second_lower_continuation =
        document.inspect_table_cell_by_grid_column(0U, 3U, 1U);
    const auto second_lower_right_continuation =
        document.inspect_table_cell_by_grid_column(0U, 3U, 2U);
    const auto ship_cell = document.inspect_table_cell_by_grid_column(0U, 4U, 0U);
    REQUIRE(merged_anchor.has_value());
    REQUIRE(right_continuation.has_value());
    REQUIRE(first_lower_continuation.has_value());
    REQUIRE(first_lower_right_continuation.has_value());
    REQUIRE(second_lower_continuation.has_value());
    REQUIRE(second_lower_right_continuation.has_value());
    REQUIRE(ship_cell.has_value());
    CHECK_EQ(merged_anchor->column_span, 2U);
    CHECK_EQ(merged_anchor->row_span, 3U);
    CHECK_EQ(merged_anchor->vertical_merge,
             featherdoc::cell_vertical_merge::restart);
    CHECK(featherdoc::test_support::contains_text(
        merged_anchor->text, "Owner assignment spans review"));
    CHECK_EQ(right_continuation->column_span, 2U);
    CHECK_EQ(right_continuation->row_span, 3U);
    CHECK_EQ(right_continuation->vertical_merge,
             featherdoc::cell_vertical_merge::restart);
    CHECK_EQ(first_lower_continuation->column_span, 2U);
    CHECK_EQ(first_lower_continuation->vertical_merge,
             featherdoc::cell_vertical_merge::continue_merge);
    CHECK_EQ(first_lower_continuation->text, "");
    CHECK_EQ(first_lower_right_continuation->column_span, 2U);
    CHECK_EQ(first_lower_right_continuation->vertical_merge,
             featherdoc::cell_vertical_merge::continue_merge);
    CHECK_EQ(first_lower_right_continuation->text, "");
    CHECK_EQ(second_lower_continuation->column_span, 2U);
    CHECK_EQ(second_lower_continuation->vertical_merge,
             featherdoc::cell_vertical_merge::continue_merge);
    CHECK_EQ(second_lower_continuation->text, "");
    CHECK_EQ(second_lower_right_continuation->column_span, 2U);
    CHECK_EQ(second_lower_right_continuation->vertical_merge,
             featherdoc::cell_vertical_merge::continue_merge);
    CHECK_EQ(second_lower_right_continuation->text, "");
    CHECK(featherdoc::test_support::contains_text(ship_cell->text, "Ship"));

    REQUIRE_FALSE(document.save());

    featherdoc::Document reopened(docx_path);
    REQUIRE_FALSE(reopened.open());
    CHECK_EQ(reopened.inspect_table_cells(0U).size(), 22U);
    const auto reopened_anchor =
        reopened.inspect_table_cell_by_grid_column(0U, 1U, 1U);
    const auto reopened_right_continuation =
        reopened.inspect_table_cell_by_grid_column(0U, 1U, 2U);
    const auto reopened_first_lower_continuation =
        reopened.inspect_table_cell_by_grid_column(0U, 2U, 1U);
    const auto reopened_second_lower_continuation =
        reopened.inspect_table_cell_by_grid_column(0U, 3U, 1U);
    REQUIRE(reopened_anchor.has_value());
    REQUIRE(reopened_right_continuation.has_value());
    REQUIRE(reopened_first_lower_continuation.has_value());
    REQUIRE(reopened_second_lower_continuation.has_value());
    CHECK_EQ(reopened_anchor->column_span, 2U);
    CHECK_EQ(reopened_anchor->row_span, 3U);
    CHECK_EQ(reopened_anchor->vertical_merge,
             featherdoc::cell_vertical_merge::restart);
    CHECK_EQ(reopened_right_continuation->column_span, 2U);
    CHECK_EQ(reopened_right_continuation->row_span, 3U);
    CHECK_EQ(reopened_right_continuation->vertical_merge,
             featherdoc::cell_vertical_merge::restart);
    CHECK_EQ(reopened_first_lower_continuation->column_span, 2U);
    CHECK_EQ(reopened_first_lower_continuation->vertical_merge,
             featherdoc::cell_vertical_merge::continue_merge);
    CHECK_EQ(reopened_second_lower_continuation->column_span, 2U);
    CHECK_EQ(reopened_second_lower_continuation->vertical_merge,
             featherdoc::cell_vertical_merge::continue_merge);

    if (std::getenv("FEATHERDOC_KEEP_PDF_IMPORT_TEST_OUTPUTS") == nullptr) {
        std::filesystem::remove(docx_path);
    }
}

TEST_CASE("PDF text importer preserves cross-column header table cells") {
    const auto input_path = featherdoc::test_support::
        write_paragraph_cross_column_header_table_paragraph_pdf(
            "featherdoc-pdf-import-cross-column-header-table.pdf");
    const auto docx_path =
        std::filesystem::current_path() /
        "featherdoc-pdf-import-cross-column-header-table.docx";
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
             "Intro paragraph before cross-column header table\n"
             "Tail paragraph after cross-column header table\n");

    const auto imported_table = document.inspect_table(0U);
    REQUIRE(imported_table.has_value());
    CHECK_EQ(imported_table->row_count, 4U);
    CHECK_EQ(imported_table->column_count, 4U);
    CHECK_EQ(document.inspect_table_cells(0U).size(), 13U);

    const auto group_header = document.inspect_table_cell(0U, 0U, 0U);
    const auto covered_header =
        document.inspect_table_cell_by_grid_column(0U, 0U, 3U);
    const auto item_header =
        document.inspect_table_cell_by_grid_column(0U, 1U, 0U);
    const auto due_header =
        document.inspect_table_cell_by_grid_column(0U, 1U, 3U);
    const auto first_item =
        document.inspect_table_cell_by_grid_column(0U, 2U, 0U);
    const auto second_due =
        document.inspect_table_cell_by_grid_column(0U, 3U, 3U);
    REQUIRE(group_header.has_value());
    REQUIRE(covered_header.has_value());
    REQUIRE(item_header.has_value());
    REQUIRE(due_header.has_value());
    REQUIRE(first_item.has_value());
    REQUIRE(second_due.has_value());
    CHECK_EQ(group_header->column_span, 4U);
    CHECK_EQ(covered_header->cell_index, 0U);
    CHECK_EQ(covered_header->column_span, 4U);
    CHECK(featherdoc::test_support::contains_text(
        group_header->text, "Project delivery overview"));
    CHECK_EQ(item_header->column_span, 1U);
    CHECK(featherdoc::test_support::contains_text(item_header->text, "Item"));
    CHECK(featherdoc::test_support::contains_text(due_header->text, "Due"));
    CHECK(featherdoc::test_support::contains_text(first_item->text, "DOC-17"));
    CHECK(featherdoc::test_support::contains_text(second_due->text,
                                                  "2026-05-21"));

    REQUIRE_FALSE(document.save());

    featherdoc::Document reopened(docx_path);
    REQUIRE_FALSE(reopened.open());
    const auto reopened_blocks = reopened.inspect_body_blocks();
    REQUIRE_EQ(reopened_blocks.size(), 3U);
    CHECK_EQ(reopened_blocks[0].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(reopened_blocks[1].kind, featherdoc::body_block_kind::table);
    CHECK_EQ(reopened_blocks[2].kind, featherdoc::body_block_kind::paragraph);
    const auto reopened_group_header =
        reopened.inspect_table_cell_by_grid_column(0U, 0U, 3U);
    const auto reopened_due =
        reopened.inspect_table_cell_by_grid_column(0U, 3U, 3U);
    REQUIRE(reopened_group_header.has_value());
    REQUIRE(reopened_due.has_value());
    CHECK_EQ(reopened_group_header->column_span, 4U);
    CHECK(featherdoc::test_support::contains_text(
        reopened_group_header->text, "Project delivery overview"));
    CHECK(featherdoc::test_support::contains_text(reopened_due->text,
                                                  "2026-05-21"));

    if (std::getenv("FEATHERDOC_KEEP_PDF_IMPORT_TEST_OUTPUTS") == nullptr) {
        std::filesystem::remove(docx_path);
    }
}

TEST_CASE("PDF text importer preserves parallel group header table cells") {
    const auto input_path = featherdoc::test_support::
        write_paragraph_parallel_group_header_table_paragraph_pdf(
            "featherdoc-pdf-import-parallel-group-header-table.pdf");
    const auto docx_path =
        std::filesystem::current_path() /
        "featherdoc-pdf-import-parallel-group-header-table.docx";
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
             "Intro paragraph before parallel group header table\n"
             "Tail paragraph after parallel group header table\n");

    const auto imported_table = document.inspect_table(0U);
    REQUIRE(imported_table.has_value());
    CHECK_EQ(imported_table->row_count, 4U);
    CHECK_EQ(imported_table->column_count, 4U);
    CHECK_EQ(document.inspect_table_cells(0U).size(), 14U);

    const auto left_group = document.inspect_table_cell(0U, 0U, 0U);
    const auto left_covered =
        document.inspect_table_cell_by_grid_column(0U, 0U, 1U);
    const auto right_group =
        document.inspect_table_cell_by_grid_column(0U, 0U, 2U);
    const auto right_covered =
        document.inspect_table_cell_by_grid_column(0U, 0U, 3U);
    const auto item_header =
        document.inspect_table_cell_by_grid_column(0U, 1U, 0U);
    const auto status_header =
        document.inspect_table_cell_by_grid_column(0U, 1U, 2U);
    const auto first_item =
        document.inspect_table_cell_by_grid_column(0U, 2U, 0U);
    const auto second_due =
        document.inspect_table_cell_by_grid_column(0U, 3U, 3U);
    REQUIRE(left_group.has_value());
    REQUIRE(left_covered.has_value());
    REQUIRE(right_group.has_value());
    REQUIRE(right_covered.has_value());
    REQUIRE(item_header.has_value());
    REQUIRE(status_header.has_value());
    REQUIRE(first_item.has_value());
    REQUIRE(second_due.has_value());
    CHECK_EQ(left_group->column_span, 2U);
    CHECK_EQ(left_covered->cell_index, 0U);
    CHECK_EQ(left_covered->column_span, 2U);
    CHECK(featherdoc::test_support::contains_text(left_group->text,
                                                  "Delivery scope"));
    CHECK_EQ(right_group->column_span, 2U);
    CHECK_EQ(right_covered->cell_index, right_group->cell_index);
    CHECK_EQ(right_covered->column_span, 2U);
    CHECK(featherdoc::test_support::contains_text(right_group->text,
                                                  "Review status"));
    CHECK(featherdoc::test_support::contains_text(item_header->text, "Item"));
    CHECK(featherdoc::test_support::contains_text(status_header->text,
                                                  "Status"));
    CHECK(featherdoc::test_support::contains_text(first_item->text, "DOC-27"));
    CHECK(featherdoc::test_support::contains_text(second_due->text,
                                                  "2026-06-11"));

    REQUIRE_FALSE(document.save());

    featherdoc::Document reopened(docx_path);
    REQUIRE_FALSE(reopened.open());
    const auto reopened_blocks = reopened.inspect_body_blocks();
    REQUIRE_EQ(reopened_blocks.size(), 3U);
    CHECK_EQ(reopened_blocks[0].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(reopened_blocks[1].kind, featherdoc::body_block_kind::table);
    CHECK_EQ(reopened_blocks[2].kind, featherdoc::body_block_kind::paragraph);
    const auto reopened_left_group =
        reopened.inspect_table_cell_by_grid_column(0U, 0U, 1U);
    const auto reopened_right_group =
        reopened.inspect_table_cell_by_grid_column(0U, 0U, 3U);
    const auto reopened_due =
        reopened.inspect_table_cell_by_grid_column(0U, 3U, 3U);
    REQUIRE(reopened_left_group.has_value());
    REQUIRE(reopened_right_group.has_value());
    REQUIRE(reopened_due.has_value());
    CHECK_EQ(reopened_left_group->column_span, 2U);
    CHECK_EQ(reopened_right_group->column_span, 2U);
    CHECK(featherdoc::test_support::contains_text(reopened_left_group->text,
                                                  "Delivery scope"));
    CHECK(featherdoc::test_support::contains_text(reopened_right_group->text,
                                                  "Review status"));
    CHECK(featherdoc::test_support::contains_text(reopened_due->text,
                                                  "2026-06-11"));

    if (std::getenv("FEATHERDOC_KEEP_PDF_IMPORT_TEST_OUTPUTS") == nullptr) {
        std::filesystem::remove(docx_path);
    }
}

TEST_CASE("PDF text importer preserves multilevel group header table cells") {
    const auto input_path = featherdoc::test_support::
        write_paragraph_multilevel_group_header_table_paragraph_pdf(
            "featherdoc-pdf-import-multilevel-group-header-table.pdf");
    const auto docx_path =
        std::filesystem::current_path() /
        "featherdoc-pdf-import-multilevel-group-header-table.docx";
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
             "Intro paragraph before multilevel group header table\n"
             "Tail paragraph after multilevel group header table\n");

    const auto imported_table = document.inspect_table(0U);
    REQUIRE(imported_table.has_value());
    CHECK_EQ(imported_table->row_count, 5U);
    CHECK_EQ(imported_table->column_count, 4U);
    CHECK_EQ(document.inspect_table_cells(0U).size(), 15U);

    const auto top_group = document.inspect_table_cell(0U, 0U, 0U);
    const auto top_covered =
        document.inspect_table_cell_by_grid_column(0U, 0U, 3U);
    const auto left_group =
        document.inspect_table_cell_by_grid_column(0U, 1U, 0U);
    const auto left_covered =
        document.inspect_table_cell_by_grid_column(0U, 1U, 1U);
    const auto right_group =
        document.inspect_table_cell_by_grid_column(0U, 1U, 2U);
    const auto right_covered =
        document.inspect_table_cell_by_grid_column(0U, 1U, 3U);
    const auto item_header =
        document.inspect_table_cell_by_grid_column(0U, 2U, 0U);
    const auto status_header =
        document.inspect_table_cell_by_grid_column(0U, 2U, 2U);
    const auto first_item =
        document.inspect_table_cell_by_grid_column(0U, 3U, 0U);
    const auto second_due =
        document.inspect_table_cell_by_grid_column(0U, 4U, 3U);
    REQUIRE(top_group.has_value());
    REQUIRE(top_covered.has_value());
    REQUIRE(left_group.has_value());
    REQUIRE(left_covered.has_value());
    REQUIRE(right_group.has_value());
    REQUIRE(right_covered.has_value());
    REQUIRE(item_header.has_value());
    REQUIRE(status_header.has_value());
    REQUIRE(first_item.has_value());
    REQUIRE(second_due.has_value());
    CHECK_EQ(top_group->column_span, 4U);
    CHECK_EQ(top_covered->cell_index, 0U);
    CHECK_EQ(top_covered->column_span, 4U);
    CHECK(featherdoc::test_support::contains_text(top_group->text,
                                                  "Program delivery dashboard"));
    CHECK_EQ(left_group->column_span, 2U);
    CHECK_EQ(left_covered->cell_index, left_group->cell_index);
    CHECK_EQ(right_group->column_span, 2U);
    CHECK_EQ(right_covered->cell_index, right_group->cell_index);
    CHECK(featherdoc::test_support::contains_text(left_group->text,
                                                  "Delivery scope"));
    CHECK(featherdoc::test_support::contains_text(right_group->text,
                                                  "Review status"));
    CHECK(featherdoc::test_support::contains_text(item_header->text, "Item"));
    CHECK(featherdoc::test_support::contains_text(status_header->text,
                                                  "Status"));
    CHECK(featherdoc::test_support::contains_text(first_item->text, "DOC-37"));
    CHECK(featherdoc::test_support::contains_text(second_due->text,
                                                  "2026-06-25"));

    REQUIRE_FALSE(document.save());

    featherdoc::Document reopened(docx_path);
    REQUIRE_FALSE(reopened.open());
    const auto reopened_blocks = reopened.inspect_body_blocks();
    REQUIRE_EQ(reopened_blocks.size(), 3U);
    CHECK_EQ(reopened_blocks[0].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(reopened_blocks[1].kind, featherdoc::body_block_kind::table);
    CHECK_EQ(reopened_blocks[2].kind, featherdoc::body_block_kind::paragraph);
    const auto reopened_top_group =
        reopened.inspect_table_cell_by_grid_column(0U, 0U, 3U);
    const auto reopened_left_group =
        reopened.inspect_table_cell_by_grid_column(0U, 1U, 1U);
    const auto reopened_right_group =
        reopened.inspect_table_cell_by_grid_column(0U, 1U, 3U);
    const auto reopened_due =
        reopened.inspect_table_cell_by_grid_column(0U, 4U, 3U);
    REQUIRE(reopened_top_group.has_value());
    REQUIRE(reopened_left_group.has_value());
    REQUIRE(reopened_right_group.has_value());
    REQUIRE(reopened_due.has_value());
    CHECK_EQ(reopened_top_group->column_span, 4U);
    CHECK_EQ(reopened_left_group->column_span, 2U);
    CHECK_EQ(reopened_right_group->column_span, 2U);
    CHECK(featherdoc::test_support::contains_text(
        reopened_top_group->text, "Program delivery dashboard"));
    CHECK(featherdoc::test_support::contains_text(reopened_left_group->text,
                                                  "Delivery scope"));
    CHECK(featherdoc::test_support::contains_text(reopened_right_group->text,
                                                  "Review status"));
    CHECK(featherdoc::test_support::contains_text(reopened_due->text,
                                                  "2026-06-25"));

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
