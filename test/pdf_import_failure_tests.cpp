#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "pdf_import_test_support.hpp"

#include <featherdoc/pdf/pdf_document_importer.hpp>

#include <filesystem>

TEST_CASE("PDF text importer classifies disabled text extraction") {
    const auto input_path = featherdoc::test_support::write_import_structure_pdf(
        "featherdoc-pdf-import-disabled-text.pdf");

    featherdoc::Document document;
    featherdoc::pdf::PdfDocumentImportOptions options;
    options.parse_options.extract_text = false;

    const auto import_result =
        featherdoc::pdf::import_pdf_text_document(input_path, document, options);
    CHECK_FALSE(import_result.success);
    CHECK_EQ(import_result.failure_kind,
             featherdoc::pdf::PdfDocumentImportFailureKind::
                 extract_text_disabled);
    CHECK(featherdoc::test_support::contains_text(
        import_result.error_message, "extract_text=true"));
    CHECK(document.inspect_body_blocks().empty());
}

TEST_CASE("PDF text importer classifies disabled geometry extraction") {
    const auto input_path = featherdoc::test_support::write_import_structure_pdf(
        "featherdoc-pdf-import-disabled-geometry.pdf");

    featherdoc::Document document;
    featherdoc::pdf::PdfDocumentImportOptions options;
    options.parse_options.extract_geometry = false;

    const auto import_result =
        featherdoc::pdf::import_pdf_text_document(input_path, document, options);
    CHECK_FALSE(import_result.success);
    CHECK_EQ(import_result.failure_kind,
             featherdoc::pdf::PdfDocumentImportFailureKind::
                 extract_geometry_disabled);
    CHECK(featherdoc::test_support::contains_text(
        import_result.error_message, "extract_geometry=true"));
    CHECK(document.inspect_body_blocks().empty());
}

TEST_CASE("PDF text importer classifies PDFs without text paragraphs") {
    const auto input_path = featherdoc::test_support::write_blank_pdf(
        "featherdoc-pdf-import-no-text.pdf");

    featherdoc::Document document;
    const auto import_result =
        featherdoc::pdf::import_pdf_text_document(input_path, document);

    CHECK_FALSE(import_result.success);
    CHECK_EQ(import_result.failure_kind,
             featherdoc::pdf::PdfDocumentImportFailureKind::no_text_paragraphs);
    CHECK(featherdoc::test_support::contains_text(
        import_result.error_message, "text paragraphs only"));
}

TEST_CASE("PDF text importer classifies detected table candidates") {
    const auto input_path = featherdoc::test_support::write_table_like_grid_pdf(
        "featherdoc-pdf-import-table-candidate.pdf");

    featherdoc::Document document;
    const auto import_result =
        featherdoc::pdf::import_pdf_text_document(input_path, document);

    CHECK_FALSE(import_result.success);
    CHECK_EQ(
        import_result.failure_kind,
        featherdoc::pdf::PdfDocumentImportFailureKind::
            table_candidates_detected);
    CHECK(featherdoc::test_support::contains_text(
        import_result.error_message, "table-like structure candidates"));
}

TEST_CASE("PDF text importer classifies parse failures") {
    featherdoc::Document document;
    const auto import_result = featherdoc::pdf::import_pdf_text_document(
        std::filesystem::current_path() / "missing-import-source.pdf", document);

    CHECK_FALSE(import_result.success);
    CHECK_EQ(import_result.failure_kind,
             featherdoc::pdf::PdfDocumentImportFailureKind::parse_failed);
    CHECK(featherdoc::test_support::contains_text(
        import_result.error_message, "does not exist"));
}
