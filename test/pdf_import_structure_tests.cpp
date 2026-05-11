#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "pdf_import_test_support.hpp"

#include <featherdoc/pdf/pdf_document_importer.hpp>
#include <featherdoc/pdf/pdf_parser.hpp>

#include <algorithm>
#include <cstdlib>
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

TEST_CASE("PDFium parser recovers paragraph order from out-of-order text runs") {
    const auto output_path =
        featherdoc::test_support::write_out_of_order_paragraph_table_paragraph_pdf(
            "featherdoc-pdf-import-structure-reordered.pdf");

    featherdoc::pdf::PdfiumParser parser;
    const auto parse_result = parser.parse(output_path, {});
    REQUIRE_MESSAGE(parse_result.success, parse_result.error_message);
    REQUIRE_EQ(parse_result.document.pages.size(), 1U);

    const auto &parsed_page = parse_result.document.pages.front();
    REQUIRE_EQ(parsed_page.text_lines.size(), 6U);
    CHECK(featherdoc::test_support::contains_text(
        parsed_page.text_lines.front().text, "Intro paragraph line one"));
    CHECK(featherdoc::test_support::contains_text(
        parsed_page.text_lines[1].text, "Intro paragraph line two"));
    CHECK(featherdoc::test_support::contains_text(
        parsed_page.text_lines.back().text, "Tail paragraph after table"));
    CHECK_EQ(parsed_page.table_candidates.size(), 1U);
    CHECK(featherdoc::test_support::contains_text(
        parsed_page.paragraphs.front().text, "Intro paragraph line one"));
    CHECK(featherdoc::test_support::contains_text(
        parsed_page.paragraphs.back().text, "Tail paragraph after table"));
    REQUIRE_EQ(parsed_page.content_blocks.size(), 3U);
    CHECK_EQ(parsed_page.content_blocks[0].kind,
             featherdoc::pdf::PdfParsedContentBlockKind::paragraph);
    CHECK_EQ(parsed_page.content_blocks[1].kind,
             featherdoc::pdf::PdfParsedContentBlockKind::table_candidate);
    CHECK_EQ(parsed_page.content_blocks[2].kind,
             featherdoc::pdf::PdfParsedContentBlockKind::paragraph);
}

TEST_CASE("PDFium parser preserves two-column row-wise reading order from out-of-order text runs") {
    const auto output_path =
        featherdoc::test_support::write_out_of_order_two_column_pdf(
            "featherdoc-pdf-import-two-column-reordered.pdf");

    featherdoc::pdf::PdfiumParser parser;
    const auto parse_result = parser.parse(output_path, {});
    REQUIRE_MESSAGE(parse_result.success, parse_result.error_message);
    REQUIRE_EQ(parse_result.document.pages.size(), 1U);

    const auto &parsed_page = parse_result.document.pages.front();
    REQUIRE_EQ(parsed_page.text_lines.size(), 3U);
    CHECK(featherdoc::test_support::contains_text(
        parsed_page.text_lines[0].text, "Two column sample"));
    CHECK(featherdoc::test_support::contains_text(
        parsed_page.text_lines[1].text, "Left column alpha"));
    CHECK(featherdoc::test_support::contains_text(
        parsed_page.text_lines[1].text, "Right column alpha"));
    CHECK(featherdoc::test_support::contains_text(
        parsed_page.text_lines[2].text, "Left column beta"));
    CHECK(featherdoc::test_support::contains_text(
        parsed_page.text_lines[2].text, "Right column beta"));
    CHECK(parsed_page.table_candidates.empty());

    REQUIRE_EQ(parsed_page.paragraphs.size(), 3U);
    CHECK(featherdoc::test_support::contains_text(
        parsed_page.paragraphs[0].text, "Two column sample"));
    CHECK(featherdoc::test_support::contains_text(
        parsed_page.paragraphs[1].text, "Left column alpha"));
    CHECK(featherdoc::test_support::contains_text(
        parsed_page.paragraphs[1].text, "Right column alpha"));
    CHECK(featherdoc::test_support::contains_text(
        parsed_page.paragraphs[2].text, "Left column beta"));
    CHECK(featherdoc::test_support::contains_text(
        parsed_page.paragraphs[2].text, "Right column beta"));
}

TEST_CASE(
    "PDFium parser preserves note line that sits close to table border") {
    const auto output_path =
        featherdoc::test_support::write_paragraph_table_touching_note_paragraph_pdf(
            "featherdoc-pdf-import-touching-note.pdf");

    featherdoc::pdf::PdfiumParser parser;
    const auto parse_result = parser.parse(output_path, {});
    REQUIRE_MESSAGE(parse_result.success, parse_result.error_message);
    REQUIRE_EQ(parse_result.document.pages.size(), 1U);

    const auto &parsed_page = parse_result.document.pages.front();
    REQUIRE_EQ(parsed_page.text_lines.size(), 6U);
    CHECK_EQ(parsed_page.table_candidates.size(), 1U);
    CHECK(featherdoc::test_support::contains_text(
        parsed_page.text_lines[0].text, "Intro paragraph before touching note"));
    CHECK(featherdoc::test_support::contains_text(
        parsed_page.text_lines[4].text, "Note paragraph touching table border"));
    CHECK(featherdoc::test_support::contains_text(
        parsed_page.text_lines[5].text, "Tail paragraph after touching note"));
}

TEST_CASE(
    "PDFium parser groups touching multi-line note outside table as one paragraph") {
    const auto output_path =
        featherdoc::test_support::
            write_paragraph_table_touching_multiline_note_paragraph_pdf(
                "featherdoc-pdf-import-touching-multiline-note.pdf");

    featherdoc::pdf::PdfiumParser parser;
    const auto parse_result = parser.parse(output_path, {});
    REQUIRE_MESSAGE(parse_result.success, parse_result.error_message);
    REQUIRE_EQ(parse_result.document.pages.size(), 1U);

    const auto &parsed_page = parse_result.document.pages.front();
    REQUIRE_EQ(parsed_page.text_lines.size(), 7U);
    CHECK_EQ(parsed_page.table_candidates.size(), 1U);
    CHECK(featherdoc::test_support::contains_text(
        parsed_page.text_lines[4].text,
        "Note paragraph line one near table border"));
    CHECK(featherdoc::test_support::contains_text(
        parsed_page.text_lines[5].text,
        "Note paragraph line two stays outside table"));
    CHECK(featherdoc::test_support::contains_text(
        parsed_page.text_lines[6].text,
        "Tail paragraph after touching multi-line note"));
    REQUIRE(parsed_page.text_lines[3].table_candidate_index.has_value());
    CHECK_FALSE(parsed_page.text_lines[4].table_candidate_index.has_value());

    REQUIRE_GE(parsed_page.paragraphs.size(), 4U);
    const auto note_paragraph = std::find_if(
        parsed_page.paragraphs.begin(), parsed_page.paragraphs.end(),
        [](const featherdoc::pdf::PdfParsedParagraph &paragraph) {
            return featherdoc::test_support::contains_text(
                paragraph.text, "Note paragraph line one near table border");
        });
    REQUIRE(note_paragraph != parsed_page.paragraphs.end());
    CHECK_FALSE(note_paragraph->table_candidate_index.has_value());
    CHECK(featherdoc::test_support::contains_text(
        note_paragraph->text, "Note paragraph line two stays outside table"));
    CHECK_FALSE(featherdoc::test_support::contains_text(
        note_paragraph->text, "Touching multi-line C3"));
    const auto table_paragraph = std::find_if(
        parsed_page.paragraphs.begin(), parsed_page.paragraphs.end(),
        [](const featherdoc::pdf::PdfParsedParagraph &paragraph) {
            return featherdoc::test_support::contains_text(
                paragraph.text, "Touching multi-line C3");
        });
    REQUIRE(table_paragraph != parsed_page.paragraphs.end());
    CHECK(table_paragraph->table_candidate_index.has_value());
    CHECK_FALSE(featherdoc::test_support::contains_text(
        table_paragraph->text, "Note paragraph line one near table border"));
}

TEST_CASE(
    "PDFium parser keeps partial-overlap multi-line note as one paragraph") {
    const auto output_path =
        featherdoc::test_support::
            write_paragraph_table_partial_overlap_multiline_note_paragraph_pdf(
                "featherdoc-pdf-import-partial-overlap-multiline-note.pdf");

    featherdoc::pdf::PdfiumParser parser;
    const auto parse_result = parser.parse(output_path, {});
    REQUIRE_MESSAGE(parse_result.success, parse_result.error_message);
    REQUIRE_EQ(parse_result.document.pages.size(), 1U);

    const auto &parsed_page = parse_result.document.pages.front();
    REQUIRE_EQ(parsed_page.text_lines.size(), 7U);
    CHECK_EQ(parsed_page.table_candidates.size(), 1U);
    CHECK(featherdoc::test_support::contains_text(
        parsed_page.text_lines[4].text,
        "Partial overlap note line one near table border"));
    CHECK(featherdoc::test_support::contains_text(
        parsed_page.text_lines[5].text,
        "Partial overlap note line two stays outside table"));
    CHECK(featherdoc::test_support::contains_text(
        parsed_page.text_lines[6].text,
        "Tail paragraph after partial-overlap note"));
    REQUIRE(parsed_page.text_lines[3].table_candidate_index.has_value());
    CHECK_FALSE(parsed_page.text_lines[4].table_candidate_index.has_value());

    REQUIRE_GE(parsed_page.paragraphs.size(), 4U);
    const auto note_paragraph = std::find_if(
        parsed_page.paragraphs.begin(), parsed_page.paragraphs.end(),
        [](const featherdoc::pdf::PdfParsedParagraph &paragraph) {
            return featherdoc::test_support::contains_text(
                paragraph.text, "Partial overlap note line one near table border");
        });
    REQUIRE(note_paragraph != parsed_page.paragraphs.end());
    CHECK_FALSE(note_paragraph->table_candidate_index.has_value());
    CHECK(featherdoc::test_support::contains_text(
        note_paragraph->text, "Partial overlap note line two stays outside table"));
    CHECK_FALSE(featherdoc::test_support::contains_text(
        note_paragraph->text, "Partial overlap C3"));
    const auto table_paragraph = std::find_if(
        parsed_page.paragraphs.begin(), parsed_page.paragraphs.end(),
        [](const featherdoc::pdf::PdfParsedParagraph &paragraph) {
            return featherdoc::test_support::contains_text(
                paragraph.text, "Partial overlap C3");
        });
    REQUIRE(table_paragraph != parsed_page.paragraphs.end());
    CHECK(table_paragraph->table_candidate_index.has_value());
    CHECK_FALSE(featherdoc::test_support::contains_text(
        table_paragraph->text, "Partial overlap note line one near table border"));
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

TEST_CASE("PDF text importer preserves two-column row-wise reading order from out-of-order text runs") {
    const auto input_path =
        featherdoc::test_support::write_out_of_order_two_column_pdf(
            "featherdoc-pdf-import-two-column-reordered-source.pdf");
    const auto docx_path =
        std::filesystem::current_path() /
        "featherdoc-pdf-import-two-column-reordered-source.docx";
    std::filesystem::remove(docx_path);

    featherdoc::Document document(docx_path);
    const auto import_result =
        featherdoc::pdf::import_pdf_text_document(input_path, document);
    REQUIRE_MESSAGE(import_result.success, import_result.error_message);
    CHECK_EQ(import_result.failure_kind,
             featherdoc::pdf::PdfDocumentImportFailureKind::none);
    CHECK_EQ(import_result.paragraphs_imported, 3U);
    CHECK_EQ(import_result.tables_imported, 0U);
    CHECK_EQ(featherdoc::test_support::collect_document_text(document),
             "Two column sample\n"
             "Left column alpha Right column alpha\n"
             "Left column beta Right column beta\n");

    const auto blocks = document.inspect_body_blocks();
    REQUIRE_EQ(blocks.size(), 3U);
    CHECK_EQ(blocks[0].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(blocks[0].block_index, 0U);
    CHECK_EQ(blocks[1].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(blocks[1].block_index, 1U);
    CHECK_EQ(blocks[2].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(blocks[2].block_index, 2U);

    REQUIRE_FALSE(document.save());

    featherdoc::Document reopened(docx_path);
    REQUIRE_FALSE(reopened.open());
    const auto reopened_blocks = reopened.inspect_body_blocks();
    REQUIRE_EQ(reopened_blocks.size(), 3U);
    CHECK_EQ(reopened_blocks[0].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(reopened_blocks[1].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(reopened_blocks[2].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(featherdoc::test_support::collect_document_text(reopened),
             "Two column sample\n"
             "Left column alpha Right column alpha\n"
             "Left column beta Right column beta\n");

    if (std::getenv("FEATHERDOC_KEEP_PDF_IMPORT_TEST_OUTPUTS") == nullptr) {
        std::filesystem::remove(docx_path);
    }
}
