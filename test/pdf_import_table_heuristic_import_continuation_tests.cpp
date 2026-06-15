#include "doctest.h"

#include "pdf_import_test_support.hpp"

#include <featherdoc/pdf/pdf_document_importer.hpp>
#include <featherdoc/pdf/pdf_parser.hpp>

#include <cstddef>
#include <cstdint>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <numeric>
#include <vector>

namespace {

struct ExpectedTableContinuationDiagnostic {
    std::size_t page_index{0U};
    std::size_t block_index{0U};
    std::size_t source_row_offset{0U};
    std::uint32_t continuation_confidence{0U};
    std::uint32_t minimum_continuation_confidence{0U};
    featherdoc::pdf::PdfTableContinuationHeaderMatchKind header_match_kind{
        featherdoc::pdf::PdfTableContinuationHeaderMatchKind::none};
    featherdoc::pdf::PdfTableContinuationBlocker blocker{
        featherdoc::pdf::PdfTableContinuationBlocker::none};
    featherdoc::pdf::PdfTableContinuationDisposition disposition{
        featherdoc::pdf::PdfTableContinuationDisposition::none};
    bool has_previous_table{false};
    bool is_first_block_on_page{false};
    bool is_near_page_top{false};
    bool source_rows_consistent{false};
    bool column_count_matches{false};
    bool column_anchors_match{false};
    bool previous_has_repeating_header{false};
    bool source_has_repeating_header{false};
    bool header_matches_previous{false};
    bool skipped_repeating_header{false};
};

[[nodiscard]] ExpectedTableContinuationDiagnostic
expected_compatible_table_continuation(std::size_t page_index,
                                       std::size_t block_index) {
    ExpectedTableContinuationDiagnostic expected;
    expected.page_index = page_index;
    expected.block_index = block_index;
    expected.continuation_confidence = 85U;
    expected.header_match_kind =
        featherdoc::pdf::PdfTableContinuationHeaderMatchKind::not_required;
    expected.disposition =
        featherdoc::pdf::PdfTableContinuationDisposition::
            merged_with_previous_table;
    expected.has_previous_table = true;
    expected.is_first_block_on_page = true;
    expected.is_near_page_top = true;
    expected.source_rows_consistent = true;
    expected.column_count_matches = true;
    expected.column_anchors_match = true;
    expected.header_matches_previous = true;
    return expected;
}

[[nodiscard]] ExpectedTableContinuationDiagnostic
expected_repeated_header_continuation(std::size_t page_index,
                                      std::size_t block_index) {
    auto expected = expected_compatible_table_continuation(page_index,
                                                          block_index);
    expected.source_row_offset = 1U;
    expected.continuation_confidence = 95U;
    expected.header_match_kind =
        featherdoc::pdf::PdfTableContinuationHeaderMatchKind::exact;
    expected.previous_has_repeating_header = true;
    expected.source_has_repeating_header = true;
    expected.skipped_repeating_header = true;
    return expected;
}

[[nodiscard]] ExpectedTableContinuationDiagnostic
expected_no_previous_table_continuation(std::size_t page_index,
                                        std::size_t block_index) {
    ExpectedTableContinuationDiagnostic expected;
    expected.page_index = page_index;
    expected.block_index = block_index;
    expected.header_match_kind =
        featherdoc::pdf::PdfTableContinuationHeaderMatchKind::not_required;
    expected.blocker =
        featherdoc::pdf::PdfTableContinuationBlocker::no_previous_table;
    expected.disposition =
        featherdoc::pdf::PdfTableContinuationDisposition::created_new_table;
    expected.is_near_page_top = true;
    expected.source_rows_consistent = true;
    expected.header_matches_previous = true;
    return expected;
}

void check_table_continuation_diagnostic(
    const featherdoc::pdf::PdfTableContinuationDiagnostic &diagnostic,
    const ExpectedTableContinuationDiagnostic &expected) {
    CHECK_EQ(diagnostic.page_index, expected.page_index);
    CHECK_EQ(diagnostic.block_index, expected.block_index);
    CHECK_EQ(diagnostic.source_row_offset, expected.source_row_offset);
    CHECK_EQ(diagnostic.continuation_confidence,
             expected.continuation_confidence);
    CHECK_EQ(diagnostic.minimum_continuation_confidence,
             expected.minimum_continuation_confidence);
    CHECK_EQ(diagnostic.header_match_kind, expected.header_match_kind);
    CHECK_EQ(diagnostic.blocker, expected.blocker);
    CHECK_EQ(diagnostic.disposition, expected.disposition);
    CHECK_EQ(diagnostic.has_previous_table, expected.has_previous_table);
    CHECK_EQ(diagnostic.is_first_block_on_page,
             expected.is_first_block_on_page);
    CHECK_EQ(diagnostic.is_near_page_top, expected.is_near_page_top);
    CHECK_EQ(diagnostic.source_rows_consistent,
             expected.source_rows_consistent);
    CHECK_EQ(diagnostic.column_count_matches, expected.column_count_matches);
    CHECK_EQ(diagnostic.column_anchors_match, expected.column_anchors_match);
    CHECK_EQ(diagnostic.previous_has_repeating_header,
             expected.previous_has_repeating_header);
    CHECK_EQ(diagnostic.source_has_repeating_header,
             expected.source_has_repeating_header);
    CHECK_EQ(diagnostic.header_matches_previous,
             expected.header_matches_previous);
    CHECK_EQ(diagnostic.skipped_repeating_header,
             expected.skipped_repeating_header);
}

void check_initial_table_continuation_diagnostic(
    const std::vector<featherdoc::pdf::PdfTableContinuationDiagnostic>
        &diagnostics,
    std::uint32_t minimum_continuation_confidence = 0U,
    std::size_t page_index = 0U, std::size_t block_index = 1U,
    bool source_has_repeating_header = false) {
    REQUIRE_FALSE(diagnostics.empty());
    auto expected =
        expected_no_previous_table_continuation(page_index, block_index);
    expected.minimum_continuation_confidence =
        minimum_continuation_confidence;
    expected.source_has_repeating_header = source_has_repeating_header;
    check_table_continuation_diagnostic(diagnostics.front(), expected);
}

}  // namespace

TEST_CASE("PDF table import merges compatible table candidates across page boundary") {
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
    CHECK_EQ(import_result.tables_imported, 1U);
    REQUIRE_EQ(import_result.table_continuation_diagnostics.size(), 2U);
    check_initial_table_continuation_diagnostic(
        import_result.table_continuation_diagnostics);
    check_table_continuation_diagnostic(
        import_result.table_continuation_diagnostics[1],
        expected_compatible_table_continuation(1U, 0U));

    const auto blocks = document.inspect_body_blocks();
    REQUIRE_EQ(blocks.size(), 3U);
    CHECK_EQ(blocks[0].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(blocks[1].kind, featherdoc::body_block_kind::table);
    CHECK_EQ(blocks[2].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(featherdoc::test_support::collect_document_text(document),
             "Intro paragraph before page break\n"
             "Tail paragraph after page break\n");

    const auto merged_table = document.inspect_table(0U);
    REQUIRE(merged_table.has_value());
    CHECK_EQ(merged_table->row_count, 6U);
    CHECK_EQ(merged_table->column_count, 3U);
    CHECK(featherdoc::test_support::contains_text(merged_table->text,
                                                  "Page one table A1"));
    CHECK(featherdoc::test_support::contains_text(merged_table->text,
                                                  "Page one table B2"));
    CHECK(featherdoc::test_support::contains_text(merged_table->text,
                                                  "Page one table C3"));
    CHECK(featherdoc::test_support::contains_text(merged_table->text,
                                                  "Page two table A1"));
    CHECK(featherdoc::test_support::contains_text(merged_table->text,
                                                  "Page two table B2"));
    CHECK(featherdoc::test_support::contains_text(merged_table->text,
                                                  "Page two table C3"));
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
    CHECK_EQ(reopened_table->row_count, 6U);
    CHECK_EQ(reopened_table->column_count, 3U);
    CHECK_FALSE(reopened.inspect_table(1U).has_value());

    if (std::getenv("FEATHERDOC_KEEP_PDF_IMPORT_TEST_OUTPUTS") == nullptr) {
        std::filesystem::remove(docx_path);
    }
}

TEST_CASE("PDF table import continuation diagnostic skips repeated source header") {
    const auto input_path =
        featherdoc::test_support::write_invoice_grid_pagebreak_subtotal_pdf(
            "featherdoc-pdf-import-continuation-repeated-header.pdf");
    const auto docx_path =
        std::filesystem::current_path() /
        "featherdoc-pdf-import-continuation-repeated-header.docx";
    std::filesystem::remove(docx_path);

    featherdoc::Document document(docx_path);
    featherdoc::pdf::PdfDocumentImportOptions options;
    options.import_table_candidates_as_tables = true;

    const auto import_result =
        featherdoc::pdf::import_pdf_text_document(input_path, document, options);
    REQUIRE_MESSAGE(import_result.success, import_result.error_message);
    CHECK_EQ(import_result.paragraphs_imported, 2U);
    CHECK_EQ(import_result.tables_imported, 1U);
    REQUIRE_EQ(import_result.table_continuation_diagnostics.size(), 2U);
    check_initial_table_continuation_diagnostic(
        import_result.table_continuation_diagnostics, 0U, 0U, 1U, true);
    check_table_continuation_diagnostic(
        import_result.table_continuation_diagnostics[1],
        expected_repeated_header_continuation(1U, 0U));

    const auto merged_table = document.inspect_table(0U);
    REQUIRE(merged_table.has_value());
    CHECK_EQ(merged_table->row_count, 7U);
    CHECK_EQ(merged_table->column_count, 4U);
    CHECK_FALSE(document.inspect_table(1U).has_value());

    if (std::getenv("FEATHERDOC_KEEP_PDF_IMPORT_TEST_OUTPUTS") == nullptr) {
        std::filesystem::remove(docx_path);
    }
}

TEST_CASE(
    "PDF table import continuation diagnostic reports repeated header mismatch") {
    const auto input_path =
        featherdoc::test_support::
            write_invoice_grid_pagebreak_subtotal_semantic_header_variant_pdf(
                "featherdoc-pdf-import-continuation-repeated-header-mismatch.pdf");
    const auto docx_path =
        std::filesystem::current_path() /
        "featherdoc-pdf-import-continuation-repeated-header-mismatch.docx";
    std::filesystem::remove(docx_path);

    featherdoc::Document document(docx_path);
    featherdoc::pdf::PdfDocumentImportOptions options;
    options.import_table_candidates_as_tables = true;

    const auto import_result =
        featherdoc::pdf::import_pdf_text_document(input_path, document, options);
    REQUIRE_MESSAGE(import_result.success, import_result.error_message);
    CHECK_EQ(import_result.paragraphs_imported, 2U);
    CHECK_EQ(import_result.tables_imported, 2U);
    REQUIRE_EQ(import_result.table_continuation_diagnostics.size(), 2U);
    check_initial_table_continuation_diagnostic(
        import_result.table_continuation_diagnostics, 0U, 0U, 1U, true);
    auto expected_diagnostic =
        expected_repeated_header_continuation(1U, 0U);
    expected_diagnostic.source_row_offset = 0U;
    expected_diagnostic.continuation_confidence = 70U;
    expected_diagnostic.header_match_kind =
        featherdoc::pdf::PdfTableContinuationHeaderMatchKind::none;
    expected_diagnostic.header_matches_previous = false;
    expected_diagnostic.skipped_repeating_header = false;
    expected_diagnostic.blocker =
        featherdoc::pdf::PdfTableContinuationBlocker::
            repeated_header_mismatch;
    expected_diagnostic.disposition =
        featherdoc::pdf::PdfTableContinuationDisposition::created_new_table;
    check_table_continuation_diagnostic(
        import_result.table_continuation_diagnostics[1], expected_diagnostic);

    REQUIRE(document.inspect_table(0U).has_value());
    REQUIRE(document.inspect_table(1U).has_value());
    CHECK_FALSE(document.inspect_table(2U).has_value());

    if (std::getenv("FEATHERDOC_KEEP_PDF_IMPORT_TEST_OUTPUTS") == nullptr) {
        std::filesystem::remove(docx_path);
    }
}

TEST_CASE("PDF table import continuation diagnostic reports column count mismatch") {
    const auto input_path =
        featherdoc::test_support::
            write_invoice_grid_pagebreak_subtotal_column_count_mismatch_pdf(
                "featherdoc-pdf-import-continuation-column-count-mismatch.pdf");
    const auto docx_path =
        std::filesystem::current_path() /
        "featherdoc-pdf-import-continuation-column-count-mismatch.docx";
    std::filesystem::remove(docx_path);

    featherdoc::Document document(docx_path);
    featherdoc::pdf::PdfDocumentImportOptions options;
    options.import_table_candidates_as_tables = true;

    const auto import_result =
        featherdoc::pdf::import_pdf_text_document(input_path, document, options);
    REQUIRE_MESSAGE(import_result.success, import_result.error_message);
    CHECK_EQ(import_result.paragraphs_imported, 2U);
    CHECK_EQ(import_result.tables_imported, 2U);
    REQUIRE_EQ(import_result.table_continuation_diagnostics.size(), 2U);
    check_initial_table_continuation_diagnostic(
        import_result.table_continuation_diagnostics, 0U, 0U, 1U, true);
    auto expected_diagnostic =
        expected_repeated_header_continuation(1U, 0U);
    expected_diagnostic.source_row_offset = 0U;
    expected_diagnostic.continuation_confidence = 30U;
    expected_diagnostic.header_match_kind =
        featherdoc::pdf::PdfTableContinuationHeaderMatchKind::none;
    expected_diagnostic.header_matches_previous = false;
    expected_diagnostic.column_count_matches = false;
    expected_diagnostic.column_anchors_match = false;
    expected_diagnostic.skipped_repeating_header = false;
    expected_diagnostic.blocker =
        featherdoc::pdf::PdfTableContinuationBlocker::column_count_mismatch;
    expected_diagnostic.disposition =
        featherdoc::pdf::PdfTableContinuationDisposition::created_new_table;
    check_table_continuation_diagnostic(
        import_result.table_continuation_diagnostics[1], expected_diagnostic);

    const auto first_table = document.inspect_table(0U);
    const auto second_table = document.inspect_table(1U);
    REQUIRE(first_table.has_value());
    REQUIRE(second_table.has_value());
    CHECK_EQ(first_table->column_count, 4U);
    CHECK_EQ(second_table->column_count, 3U);
    CHECK_FALSE(document.inspect_table(2U).has_value());

    if (std::getenv("FEATHERDOC_KEEP_PDF_IMPORT_TEST_OUTPUTS") == nullptr) {
        std::filesystem::remove(docx_path);
    }
}

TEST_CASE(
    "PDF table import can require a higher confidence before cross-page merge") {
    const auto input_path =
        featherdoc::test_support::write_paragraph_table_pagebreak_table_paragraph_pdf(
            "featherdoc-pdf-import-pagebreak-confidence-threshold.pdf");
    const auto docx_path =
        std::filesystem::current_path() /
        "featherdoc-pdf-import-pagebreak-confidence-threshold.docx";
    std::filesystem::remove(docx_path);

    featherdoc::Document document(docx_path);
    featherdoc::pdf::PdfDocumentImportOptions options;
    options.import_table_candidates_as_tables = true;
    options.min_table_continuation_confidence = 90U;

    const auto import_result =
        featherdoc::pdf::import_pdf_text_document(input_path, document, options);
    REQUIRE_MESSAGE(import_result.success, import_result.error_message);
    CHECK_EQ(import_result.paragraphs_imported, 2U);
    CHECK_EQ(import_result.tables_imported, 2U);
    REQUIRE_EQ(import_result.table_continuation_diagnostics.size(), 2U);
    check_initial_table_continuation_diagnostic(
        import_result.table_continuation_diagnostics, 90U);
    const auto &diagnostic = import_result.table_continuation_diagnostics[1];
    auto expected_diagnostic =
        expected_compatible_table_continuation(1U, 0U);
    expected_diagnostic.minimum_continuation_confidence = 90U;
    expected_diagnostic.blocker =
        featherdoc::pdf::PdfTableContinuationBlocker::
            continuation_confidence_below_threshold;
    expected_diagnostic.disposition =
        featherdoc::pdf::PdfTableContinuationDisposition::created_new_table;
    check_table_continuation_diagnostic(diagnostic, expected_diagnostic);

    const auto blocks = document.inspect_body_blocks();
    REQUIRE_EQ(blocks.size(), 4U);
    CHECK_EQ(blocks[0].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(blocks[1].kind, featherdoc::body_block_kind::table);
    CHECK_EQ(blocks[2].kind, featherdoc::body_block_kind::table);
    CHECK_EQ(blocks[3].kind, featherdoc::body_block_kind::paragraph);

    const auto first_table = document.inspect_table(0U);
    const auto second_table = document.inspect_table(1U);
    REQUIRE(first_table.has_value());
    REQUIRE(second_table.has_value());
    CHECK_EQ(first_table->row_count, 3U);
    CHECK_EQ(second_table->row_count, 3U);
    CHECK(featherdoc::test_support::contains_text(first_table->text,
                                                  "Page one table A1"));
    CHECK(featherdoc::test_support::contains_text(second_table->text,
                                                  "Page two table A1"));
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
    CHECK(reopened.inspect_table(0U).has_value());
    CHECK(reopened.inspect_table(1U).has_value());
    CHECK_FALSE(reopened.inspect_table(2U).has_value());

    if (std::getenv("FEATHERDOC_KEEP_PDF_IMPORT_TEST_OUTPUTS") == nullptr) {
        std::filesystem::remove(docx_path);
    }
}

TEST_CASE("PDF table import does not merge cross-page tables with incompatible widths") {
    const auto input_path =
        featherdoc::test_support::
            write_paragraph_table_pagebreak_wide_table_paragraph_pdf(
                "featherdoc-pdf-import-pagebreak-wide-table.pdf");
    const auto docx_path =
        std::filesystem::current_path() /
        "featherdoc-pdf-import-pagebreak-wide-table.docx";
    std::filesystem::remove(docx_path);

    featherdoc::Document document(docx_path);
    featherdoc::pdf::PdfDocumentImportOptions options;
    options.import_table_candidates_as_tables = true;

    const auto import_result =
        featherdoc::pdf::import_pdf_text_document(input_path, document, options);
    REQUIRE_MESSAGE(import_result.success, import_result.error_message);
    CHECK_EQ(import_result.paragraphs_imported, 2U);
    CHECK_EQ(import_result.tables_imported, 2U);
    REQUIRE_EQ(import_result.parsed_document.pages.size(), 2U);
    const auto &first_parsed_page = import_result.parsed_document.pages[0];
    REQUIRE_EQ(first_parsed_page.content_blocks.size(), 2U);
    CHECK_EQ(first_parsed_page.content_blocks[0].kind,
             featherdoc::pdf::PdfParsedContentBlockKind::paragraph);
    CHECK_EQ(first_parsed_page.content_blocks[1].kind,
             featherdoc::pdf::PdfParsedContentBlockKind::table_candidate);
    const auto &second_parsed_page = import_result.parsed_document.pages[1];
    REQUIRE_EQ(second_parsed_page.content_blocks.size(), 2U);
    CHECK_EQ(second_parsed_page.content_blocks[0].kind,
             featherdoc::pdf::PdfParsedContentBlockKind::table_candidate);
    CHECK_EQ(second_parsed_page.content_blocks[1].kind,
             featherdoc::pdf::PdfParsedContentBlockKind::paragraph);
    REQUIRE_EQ(import_result.table_continuation_diagnostics.size(), 2U);
    check_initial_table_continuation_diagnostic(
        import_result.table_continuation_diagnostics);
    auto expected_diagnostic =
        expected_compatible_table_continuation(1U, 0U);
    expected_diagnostic.continuation_confidence = 55U;
    expected_diagnostic.column_anchors_match = false;
    expected_diagnostic.blocker =
        featherdoc::pdf::PdfTableContinuationBlocker::column_anchors_mismatch;
    expected_diagnostic.disposition =
        featherdoc::pdf::PdfTableContinuationDisposition::created_new_table;
    check_table_continuation_diagnostic(
        import_result.table_continuation_diagnostics[1], expected_diagnostic);

    const auto blocks = document.inspect_body_blocks();
    REQUIRE_EQ(blocks.size(), 4U);
    CHECK_EQ(blocks[0].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(blocks[1].kind, featherdoc::body_block_kind::table);
    CHECK_EQ(blocks[2].kind, featherdoc::body_block_kind::table);
    CHECK_EQ(blocks[3].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(featherdoc::test_support::collect_document_text(document),
             "Intro paragraph before wide table\n"
             "Tail paragraph after wide table\n");

    const auto first_table = document.inspect_table(0U);
    const auto second_table = document.inspect_table(1U);
    REQUIRE(first_table.has_value());
    REQUIRE(second_table.has_value());
    CHECK_EQ(first_table->row_count, 3U);
    CHECK_EQ(first_table->column_count, 3U);
    CHECK_EQ(second_table->row_count, 3U);
    CHECK_EQ(second_table->column_count, 3U);
    CHECK(featherdoc::test_support::contains_text(first_table->text,
                                                  "Narrow page A1"));
    CHECK(featherdoc::test_support::contains_text(second_table->text,
                                                  "Wide page A1"));
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
    CHECK(reopened.inspect_table(0U).has_value());
    CHECK(reopened.inspect_table(1U).has_value());
    CHECK_FALSE(reopened.inspect_table(2U).has_value());

    if (std::getenv("FEATHERDOC_KEEP_PDF_IMPORT_TEST_OUTPUTS") == nullptr) {
        std::filesystem::remove(docx_path);
    }
}

TEST_CASE("PDF table import does not merge cross-page tables that start too low on the next page") {
    const auto input_path =
        featherdoc::test_support::
            write_paragraph_table_pagebreak_low_table_paragraph_pdf(
                "featherdoc-pdf-import-pagebreak-low-table.pdf");
    const auto docx_path =
        std::filesystem::current_path() /
        "featherdoc-pdf-import-pagebreak-low-table.docx";
    std::filesystem::remove(docx_path);

    featherdoc::Document document(docx_path);
    featherdoc::pdf::PdfDocumentImportOptions options;
    options.import_table_candidates_as_tables = true;

    const auto import_result =
        featherdoc::pdf::import_pdf_text_document(input_path, document, options);
    REQUIRE_MESSAGE(import_result.success, import_result.error_message);
    CHECK_EQ(import_result.paragraphs_imported, 2U);
    CHECK_EQ(import_result.tables_imported, 2U);
    REQUIRE_EQ(import_result.table_continuation_diagnostics.size(), 2U);
    check_initial_table_continuation_diagnostic(
        import_result.table_continuation_diagnostics);
    auto expected_diagnostic =
        expected_compatible_table_continuation(1U, 0U);
    expected_diagnostic.continuation_confidence = 45U;
    expected_diagnostic.is_near_page_top = false;
    expected_diagnostic.blocker =
        featherdoc::pdf::PdfTableContinuationBlocker::not_near_page_top;
    expected_diagnostic.disposition =
        featherdoc::pdf::PdfTableContinuationDisposition::created_new_table;
    check_table_continuation_diagnostic(
        import_result.table_continuation_diagnostics[1], expected_diagnostic);

    const auto blocks = document.inspect_body_blocks();
    REQUIRE_EQ(blocks.size(), 4U);
    CHECK_EQ(blocks[0].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(blocks[1].kind, featherdoc::body_block_kind::table);
    CHECK_EQ(blocks[2].kind, featherdoc::body_block_kind::table);
    CHECK_EQ(blocks[3].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(featherdoc::test_support::collect_document_text(document),
             "Intro paragraph before low table\n"
             "Tail paragraph after low table\n");

    const auto first_table = document.inspect_table(0U);
    const auto second_table = document.inspect_table(1U);
    REQUIRE(first_table.has_value());
    REQUIRE(second_table.has_value());
    CHECK_EQ(first_table->row_count, 3U);
    CHECK_EQ(second_table->row_count, 3U);
    CHECK(featherdoc::test_support::contains_text(first_table->text,
                                                  "Low first page A1"));
    CHECK(featherdoc::test_support::contains_text(second_table->text,
                                                  "Low second page A1"));
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
    CHECK(reopened.inspect_table(0U).has_value());
    CHECK(reopened.inspect_table(1U).has_value());
    CHECK_FALSE(reopened.inspect_table(2U).has_value());

    if (std::getenv("FEATHERDOC_KEEP_PDF_IMPORT_TEST_OUTPUTS") == nullptr) {
        std::filesystem::remove(docx_path);
    }
}

TEST_CASE("PDF table import does not merge through an intervening paragraph") {
    const auto input_path =
        featherdoc::test_support::
            write_paragraph_table_pagebreak_caption_table_paragraph_pdf(
                "featherdoc-pdf-import-pagebreak-caption-table.pdf");
    const auto docx_path =
        std::filesystem::current_path() /
        "featherdoc-pdf-import-pagebreak-caption-table.docx";
    std::filesystem::remove(docx_path);

    featherdoc::Document document(docx_path);
    featherdoc::pdf::PdfDocumentImportOptions options;
    options.import_table_candidates_as_tables = true;

    const auto import_result =
        featherdoc::pdf::import_pdf_text_document(input_path, document, options);
    REQUIRE_MESSAGE(import_result.success, import_result.error_message);
    CHECK_EQ(import_result.paragraphs_imported, 3U);
    CHECK_EQ(import_result.tables_imported, 2U);
    REQUIRE_EQ(import_result.table_continuation_diagnostics.size(), 2U);
    check_initial_table_continuation_diagnostic(
        import_result.table_continuation_diagnostics);
    auto expected_diagnostic =
        expected_no_previous_table_continuation(1U, 1U);
    check_table_continuation_diagnostic(
        import_result.table_continuation_diagnostics[1], expected_diagnostic);

    const auto blocks = document.inspect_body_blocks();
    REQUIRE_EQ(blocks.size(), 5U);
    CHECK_EQ(blocks[0].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(blocks[1].kind, featherdoc::body_block_kind::table);
    CHECK_EQ(blocks[2].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(blocks[3].kind, featherdoc::body_block_kind::table);
    CHECK_EQ(blocks[4].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(featherdoc::test_support::collect_document_text(document),
             "Intro paragraph before caption page\n"
             "Continuation note before table\n"
             "Tail paragraph after caption table\n");

    const auto first_table = document.inspect_table(0U);
    const auto second_table = document.inspect_table(1U);
    REQUIRE(first_table.has_value());
    REQUIRE(second_table.has_value());
    CHECK_EQ(first_table->row_count, 3U);
    CHECK_EQ(second_table->row_count, 3U);
    CHECK(featherdoc::test_support::contains_text(first_table->text,
                                                  "Caption first table A1"));
    CHECK(featherdoc::test_support::contains_text(second_table->text,
                                                  "Caption second table A1"));
    CHECK_FALSE(document.inspect_table(2U).has_value());

    REQUIRE_FALSE(document.save());

    featherdoc::Document reopened(docx_path);
    REQUIRE_FALSE(reopened.open());
    const auto reopened_blocks = reopened.inspect_body_blocks();
    REQUIRE_EQ(reopened_blocks.size(), 5U);
    CHECK_EQ(reopened_blocks[0].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(reopened_blocks[1].kind, featherdoc::body_block_kind::table);
    CHECK_EQ(reopened_blocks[2].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(reopened_blocks[3].kind, featherdoc::body_block_kind::table);
    CHECK_EQ(reopened_blocks[4].kind, featherdoc::body_block_kind::paragraph);
    CHECK(reopened.inspect_table(0U).has_value());
    CHECK(reopened.inspect_table(1U).has_value());
    CHECK_FALSE(reopened.inspect_table(2U).has_value());

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
    REQUIRE_EQ(import_result.table_continuation_diagnostics.size(), 2U);
    check_initial_table_continuation_diagnostic(
        import_result.table_continuation_diagnostics);
    auto expected_diagnostic =
        expected_compatible_table_continuation(0U, 2U);
    expected_diagnostic.continuation_confidence = 35U;
    expected_diagnostic.is_first_block_on_page = false;
    expected_diagnostic.is_near_page_top = false;
    expected_diagnostic.blocker =
        featherdoc::pdf::PdfTableContinuationBlocker::not_first_block_on_page;
    expected_diagnostic.disposition =
        featherdoc::pdf::PdfTableContinuationDisposition::created_new_table;
    check_table_continuation_diagnostic(
        import_result.table_continuation_diagnostics[1], expected_diagnostic);

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

TEST_CASE("PDF table import preserves same-page paragraph separation between table candidates") {
    const auto input_path =
        featherdoc::test_support::write_paragraph_table_note_table_paragraph_pdf(
            "featherdoc-pdf-import-paragraph-table-note-table-paragraph.pdf");
    const auto docx_path =
        std::filesystem::current_path() /
        "featherdoc-pdf-import-paragraph-table-note-table-paragraph.docx";
    std::filesystem::remove(docx_path);

    featherdoc::Document document(docx_path);
    featherdoc::pdf::PdfDocumentImportOptions options;
    options.import_table_candidates_as_tables = true;

    const auto import_result =
        featherdoc::pdf::import_pdf_text_document(input_path, document, options);
    REQUIRE_MESSAGE(import_result.success, import_result.error_message);
    CHECK_EQ(import_result.paragraphs_imported, 3U);
    CHECK_EQ(import_result.tables_imported, 2U);
    REQUIRE_EQ(import_result.parsed_document.pages.size(), 1U);
    const auto &parsed_page = import_result.parsed_document.pages.front();
    REQUIRE_EQ(parsed_page.content_blocks.size(), 5U);
    CHECK_EQ(parsed_page.content_blocks[0].kind,
             featherdoc::pdf::PdfParsedContentBlockKind::paragraph);
    CHECK_EQ(parsed_page.content_blocks[1].kind,
             featherdoc::pdf::PdfParsedContentBlockKind::table_candidate);
    CHECK_EQ(parsed_page.content_blocks[2].kind,
             featherdoc::pdf::PdfParsedContentBlockKind::paragraph);
    CHECK_EQ(parsed_page.content_blocks[3].kind,
             featherdoc::pdf::PdfParsedContentBlockKind::table_candidate);
    CHECK_EQ(parsed_page.content_blocks[4].kind,
             featherdoc::pdf::PdfParsedContentBlockKind::paragraph);

    const auto blocks = document.inspect_body_blocks();
    REQUIRE_EQ(blocks.size(), 5U);
    CHECK_EQ(blocks[0].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(blocks[0].block_index, 0U);
    CHECK_EQ(blocks[0].item_index, 0U);
    CHECK_EQ(blocks[1].kind, featherdoc::body_block_kind::table);
    CHECK_EQ(blocks[1].block_index, 1U);
    CHECK_EQ(blocks[1].item_index, 0U);
    CHECK_EQ(blocks[2].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(blocks[2].block_index, 2U);
    CHECK_EQ(blocks[2].item_index, 1U);
    CHECK_EQ(blocks[3].kind, featherdoc::body_block_kind::table);
    CHECK_EQ(blocks[3].block_index, 3U);
    CHECK_EQ(blocks[3].item_index, 1U);
    CHECK_EQ(blocks[4].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(blocks[4].block_index, 4U);
    CHECK_EQ(blocks[4].item_index, 2U);
    CHECK_EQ(featherdoc::test_support::collect_document_text(document),
             "Intro paragraph before same-page tables\n"
             "Note paragraph between tables\n"
             "Tail paragraph after same-page tables\n");

    const auto first_table = document.inspect_table(0U);
    const auto second_table = document.inspect_table(1U);
    REQUIRE(first_table.has_value());
    REQUIRE(second_table.has_value());
    CHECK_EQ(first_table->row_count, 3U);
    CHECK_EQ(first_table->column_count, 3U);
    CHECK_EQ(second_table->row_count, 3U);
    CHECK_EQ(second_table->column_count, 3U);
    CHECK(featherdoc::test_support::contains_text(first_table->text,
                                                  "First table A1"));
    CHECK(featherdoc::test_support::contains_text(second_table->text,
                                                  "Second table A1"));
    CHECK_FALSE(document.inspect_table(2U).has_value());

    REQUIRE_FALSE(document.save());

    featherdoc::Document reopened(docx_path);
    REQUIRE_FALSE(reopened.open());
    const auto reopened_blocks = reopened.inspect_body_blocks();
    REQUIRE_EQ(reopened_blocks.size(), 5U);
    CHECK_EQ(reopened_blocks[0].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(reopened_blocks[1].kind, featherdoc::body_block_kind::table);
    CHECK_EQ(reopened_blocks[2].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(reopened_blocks[3].kind, featherdoc::body_block_kind::table);
    CHECK_EQ(reopened_blocks[4].kind, featherdoc::body_block_kind::paragraph);
    CHECK(reopened.inspect_table(0U).has_value());
    CHECK(reopened.inspect_table(1U).has_value());
    CHECK_FALSE(reopened.inspect_table(2U).has_value());

    if (std::getenv("FEATHERDOC_KEEP_PDF_IMPORT_TEST_OUTPUTS") == nullptr) {
        std::filesystem::remove(docx_path);
    }
}
