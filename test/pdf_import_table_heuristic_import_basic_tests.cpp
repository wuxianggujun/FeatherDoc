#include "doctest.h"

#include "pdf_import_test_support.hpp"

#include <featherdoc/pdf/pdf_document_importer.hpp>
#include <featherdoc/pdf/pdf_parser.hpp>

#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <numeric>

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

TEST_CASE("PDF text importer keeps free-form column drift prose as paragraphs") {
    const auto input_path =
        featherdoc::test_support::write_free_form_column_drift_prose_pdf(
            "featherdoc-pdf-import-free-form-column-drift-prose-import.pdf");
    const auto docx_path =
        std::filesystem::current_path() /
        "featherdoc-pdf-import-free-form-column-drift-prose.docx";
    std::filesystem::remove(docx_path);

    featherdoc::Document document(docx_path);
    featherdoc::pdf::PdfDocumentImportOptions options;
    options.import_table_candidates_as_tables = true;

    const auto import_result =
        featherdoc::pdf::import_pdf_text_document(input_path, document, options);
    REQUIRE_MESSAGE(import_result.success, import_result.error_message);
    CHECK_EQ(import_result.failure_kind,
             featherdoc::pdf::PdfDocumentImportFailureKind::none);
    CHECK_EQ(import_result.tables_imported, 0U);
    CHECK_GE(import_result.paragraphs_imported, 2U);
    CHECK_FALSE(document.inspect_table(0U).has_value());

    const auto text = featherdoc::test_support::collect_document_text(document);
    CHECK(featherdoc::test_support::contains_text(
        text, "Free-form column drift sample"));
    CHECK(featherdoc::test_support::contains_text(text, "Topic"));
    CHECK(featherdoc::test_support::contains_text(text, "Closed"));

    REQUIRE_FALSE(document.save());

    featherdoc::Document reopened(docx_path);
    REQUIRE_FALSE(reopened.open());
    CHECK_FALSE(reopened.inspect_table(0U).has_value());
    const auto reopened_text =
        featherdoc::test_support::collect_document_text(reopened);
    CHECK(featherdoc::test_support::contains_text(
        reopened_text, "Free-form column drift sample"));
    CHECK(featherdoc::test_support::contains_text(reopened_text, "Topic"));
    CHECK(featherdoc::test_support::contains_text(reopened_text, "Closed"));

    if (std::getenv("FEATHERDOC_KEEP_PDF_IMPORT_TEST_OUTPUTS") == nullptr) {
        std::filesystem::remove(docx_path);
    }
}

TEST_CASE("PDF text importer can opt in to two-row table import") {
    const auto input_path =
        featherdoc::test_support::write_two_row_header_data_table_pdf(
            "featherdoc-pdf-import-two-row-table.pdf");
    const auto docx_path =
        std::filesystem::current_path() / "featherdoc-pdf-import-two-row-table.docx";
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
             "Two-row table sample\n"
             "Tail paragraph after two-row table\n");

    const auto imported_table = document.inspect_table(0U);
    REQUIRE(imported_table.has_value());
    CHECK_EQ(imported_table->row_count, 2U);
    CHECK_EQ(imported_table->column_count, 3U);
    REQUIRE_EQ(imported_table->row_repeats_header.size(), 2U);
    CHECK_FALSE(imported_table->row_repeats_header[0]);
    CHECK(featherdoc::test_support::contains_text(imported_table->text,
                                                  "Item"));
    CHECK(featherdoc::test_support::contains_text(imported_table->text,
                                                  "INV-2026-05"));
    CHECK(featherdoc::test_support::contains_text(imported_table->text,
                                                  "QA Team"));
    CHECK(featherdoc::test_support::contains_text(imported_table->text,
                                                  "2026-05-14"));

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
    CHECK_EQ(reopened_table->row_count, 2U);
    CHECK_EQ(reopened_table->column_count, 3U);
    CHECK(featherdoc::test_support::contains_text(reopened_table->text,
                                                  "INV-2026-05"));

    if (std::getenv("FEATHERDOC_KEEP_PDF_IMPORT_TEST_OUTPUTS") == nullptr) {
        std::filesystem::remove(docx_path);
    }
}

TEST_CASE("PDF text importer can opt in to two-column key-value table import") {
    const auto input_path =
        featherdoc::test_support::write_two_column_key_value_table_pdf(
            "featherdoc-pdf-import-key-value-table.pdf");
    const auto docx_path = std::filesystem::current_path() /
                           "featherdoc-pdf-import-key-value-table.docx";
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
             "Key-value table sample\n"
             "Tail paragraph after key-value table\n");

    const auto imported_table = document.inspect_table(0U);
    REQUIRE(imported_table.has_value());
    CHECK_EQ(imported_table->row_count, 4U);
    CHECK_EQ(imported_table->column_count, 2U);
    REQUIRE_EQ(imported_table->row_repeats_header.size(), 4U);
    CHECK_FALSE(imported_table->row_repeats_header[0]);
    CHECK(featherdoc::test_support::contains_text(imported_table->text,
                                                  "Invoice No"));
    CHECK(featherdoc::test_support::contains_text(imported_table->text,
                                                  "INV-2026-0514"));
    CHECK(featherdoc::test_support::contains_text(imported_table->text,
                                                  "Customer"));
    CHECK(featherdoc::test_support::contains_text(imported_table->text,
                                                  "FeatherDoc QA"));
    CHECK(featherdoc::test_support::contains_text(imported_table->text,
                                                  "Due Date"));
    CHECK(featherdoc::test_support::contains_text(imported_table->text,
                                                  "2026-05-14"));
    CHECK(featherdoc::test_support::contains_text(imported_table->text,
                                                  "Total"));
    CHECK(featherdoc::test_support::contains_text(imported_table->text,
                                                  "USD 480"));

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
    CHECK_EQ(reopened_table->row_count, 4U);
    CHECK_EQ(reopened_table->column_count, 2U);
    CHECK(featherdoc::test_support::contains_text(reopened_table->text,
                                                  "INV-2026-0514"));
    CHECK(featherdoc::test_support::contains_text(reopened_table->text,
                                                  "USD 480"));

    if (std::getenv("FEATHERDOC_KEEP_PDF_IMPORT_TEST_OUTPUTS") == nullptr) {
        std::filesystem::remove(docx_path);
    }
}

TEST_CASE("PDF text importer can opt in to borderless key-value table import") {
    const auto input_path = featherdoc::test_support::
        write_two_column_borderless_key_value_table_pdf(
            "featherdoc-pdf-import-key-value-borderless-table.pdf");
    const auto docx_path =
        std::filesystem::current_path() /
        "featherdoc-pdf-import-key-value-borderless-table.docx";
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
             "Borderless key-value sample\n"
             "Tail paragraph after borderless key-value table\n");

    const auto imported_table = document.inspect_table(0U);
    REQUIRE(imported_table.has_value());
    CHECK_EQ(imported_table->row_count, 4U);
    CHECK_EQ(imported_table->column_count, 2U);
    CHECK(featherdoc::test_support::contains_text(imported_table->text,
                                                  "Invoice No"));
    CHECK(featherdoc::test_support::contains_text(imported_table->text,
                                                  "INV-2026-0610"));
    CHECK(featherdoc::test_support::contains_text(imported_table->text,
                                                  "FeatherDoc Ops"));
    CHECK(featherdoc::test_support::contains_text(imported_table->text,
                                                  "USD 960"));

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
    CHECK_EQ(reopened_table->row_count, 4U);
    CHECK_EQ(reopened_table->column_count, 2U);
    CHECK(featherdoc::test_support::contains_text(reopened_table->text,
                                                  "INV-2026-0610"));
    CHECK(featherdoc::test_support::contains_text(reopened_table->text,
                                                  "USD 960"));

    if (std::getenv("FEATHERDOC_KEEP_PDF_IMPORT_TEST_OUTPUTS") == nullptr) {
        std::filesystem::remove(docx_path);
    }
}

TEST_CASE("PDF text importer can opt in to irregular-width table import") {
    const auto input_path =
        featherdoc::test_support::write_irregular_width_header_table_pdf(
            "featherdoc-pdf-import-irregular-width-table.pdf");
    const auto docx_path =
        std::filesystem::current_path() /
        "featherdoc-pdf-import-irregular-width-table.docx";
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
             "Irregular width table sample\n"
             "Tail paragraph after irregular width table\n");

    const auto imported_table = document.inspect_table(0U);
    REQUIRE(imported_table.has_value());
    CHECK_EQ(imported_table->row_count, 4U);
    CHECK_EQ(imported_table->column_count, 3U);
    CHECK(featherdoc::test_support::contains_text(imported_table->text,
                                                  "Description"));
    CHECK(featherdoc::test_support::contains_text(imported_table->text,
                                                  "SKU-01"));
    CHECK(featherdoc::test_support::contains_text(imported_table->text,
                                                  "Annual subscription"));
    CHECK(featherdoc::test_support::contains_text(imported_table->text,
                                                  "Visual support"));
    CHECK(featherdoc::test_support::contains_text(imported_table->text,
                                                  "USD 880"));

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
    CHECK_EQ(reopened_table->row_count, 4U);
    CHECK_EQ(reopened_table->column_count, 3U);
    CHECK(featherdoc::test_support::contains_text(reopened_table->text,
                                                  "SKU-02"));
    CHECK(featherdoc::test_support::contains_text(reopened_table->text,
                                                  "USD 880"));

    if (std::getenv("FEATHERDOC_KEEP_PDF_IMPORT_TEST_OUTPUTS") == nullptr) {
        std::filesystem::remove(docx_path);
    }
}

TEST_CASE("PDF text importer can opt in to borderless irregular-width table import") {
    const auto input_path = featherdoc::test_support::
        write_borderless_irregular_width_header_table_pdf(
            "featherdoc-pdf-import-borderless-irregular-width-table.pdf");
    const auto docx_path =
        std::filesystem::current_path() /
        "featherdoc-pdf-import-borderless-irregular-width-table.docx";
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
             "Borderless irregular width table sample\n"
             "Tail paragraph after borderless irregular width table\n");

    const auto imported_table = document.inspect_table(0U);
    REQUIRE(imported_table.has_value());
    CHECK_EQ(imported_table->row_count, 4U);
    CHECK_EQ(imported_table->column_count, 3U);
    CHECK(featherdoc::test_support::contains_text(imported_table->text,
                                                  "Document import"));
    CHECK(featherdoc::test_support::contains_text(imported_table->text,
                                                  "Visual evidence"));
    CHECK(featherdoc::test_support::contains_text(imported_table->text,
                                                  "Import batch 77"));
    CHECK(featherdoc::test_support::contains_text(imported_table->text,
                                                  "USD 680"));

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
    CHECK_EQ(reopened_table->row_count, 4U);
    CHECK_EQ(reopened_table->column_count, 3U);
    CHECK(featherdoc::test_support::contains_text(reopened_table->text,
                                                  "SKU-12"));
    CHECK(featherdoc::test_support::contains_text(reopened_table->text,
                                                  "USD 680"));

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
