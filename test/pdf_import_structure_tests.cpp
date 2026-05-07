#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "pdf_import_test_support.hpp"

#include <featherdoc/pdf/pdf_document_importer.hpp>
#include <featherdoc/pdf/pdf_parser.hpp>

#include <filesystem>

TEST_CASE("PDFium parser groups text spans into lines and paragraphs") {
    const auto output_path =
        featherdoc::test_support::write_import_structure_pdf(
            "featherdoc-pdf-import-structure.pdf");

    featherdoc::pdf::PdfiumParser parser;
    const auto parse_result = parser.parse(output_path, {});
    REQUIRE_MESSAGE(parse_result.success, parse_result.error_message);
    REQUIRE_EQ(parse_result.document.pages.size(), 1U);

    const auto &parsed_page = parse_result.document.pages.front();
    CHECK_GE(parsed_page.text_spans.size(), 60U);
    REQUIRE_EQ(parsed_page.text_lines.size(), 3U);
    REQUIRE_EQ(parsed_page.paragraphs.size(), 2U);

    CHECK(featherdoc::test_support::contains_text(
        parsed_page.text_lines[0].text, "First paragraph line one"));
    CHECK(featherdoc::test_support::contains_text(
        parsed_page.text_lines[1].text, "First paragraph line two"));
    CHECK(featherdoc::test_support::contains_text(
        parsed_page.text_lines[2].text, "Second paragraph starts"));

    CHECK(featherdoc::test_support::contains_text(
        parsed_page.paragraphs[0].text, "First paragraph line one"));
    CHECK(featherdoc::test_support::contains_text(
        parsed_page.paragraphs[0].text, "First paragraph line two"));
    CHECK(featherdoc::test_support::contains_text(
        parsed_page.paragraphs[1].text, "Second paragraph starts"));
    CHECK_GT(parsed_page.paragraphs[0].bounds.height_points, 0.0);
    CHECK_GT(parsed_page.paragraphs[1].bounds.height_points, 0.0);
}

TEST_CASE("PDF text importer builds a plain FeatherDoc document") {
    const auto input_path =
        featherdoc::test_support::write_import_structure_pdf(
            "featherdoc-pdf-import-document.pdf");
    const auto docx_path =
        std::filesystem::current_path() / "featherdoc-pdf-import-document.docx";
    std::filesystem::remove(docx_path);

    featherdoc::Document document(docx_path);
    const auto import_result =
        featherdoc::pdf::import_pdf_text_document(input_path, document);
    REQUIRE_MESSAGE(import_result.success, import_result.error_message);
    CHECK_EQ(import_result.failure_kind,
             featherdoc::pdf::PdfDocumentImportFailureKind::none);
    CHECK_EQ(import_result.paragraphs_imported, 2U);
    CHECK_EQ(featherdoc::test_support::collect_document_text(document),
             "First paragraph line one\nFirst paragraph line two\n"
             "Second paragraph starts\n");

    REQUIRE_FALSE(document.save());

    featherdoc::Document reopened(docx_path);
    REQUIRE_FALSE(reopened.open());
    CHECK_EQ(featherdoc::test_support::collect_document_text(reopened),
             "First paragraph line one\nFirst paragraph line two\n"
             "Second paragraph starts\n");

    std::filesystem::remove(docx_path);
}
