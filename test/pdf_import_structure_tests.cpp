#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include <featherdoc.hpp>
#include <featherdoc/pdf/pdf_document_importer.hpp>
#include <featherdoc/pdf/pdf_parser.hpp>
#include <featherdoc/pdf/pdf_writer.hpp>

#include <filesystem>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>

namespace {

[[nodiscard]] featherdoc::pdf::PdfTextRun
make_text_run(double y_points, std::string text) {
    return featherdoc::pdf::PdfTextRun{
        featherdoc::pdf::PdfPoint{72.0, y_points},
        std::move(text),
        "Helvetica",
        {},
        12.0,
        featherdoc::pdf::PdfRgbColor{0.0, 0.0, 0.0},
        false,
        false,
        false,
        false,
    };
}

[[nodiscard]] bool contains_text(const std::string &haystack,
                                 const std::string &needle) {
    return haystack.find(needle) != std::string::npos;
}

[[nodiscard]] std::string collect_document_text(featherdoc::Document &document) {
    std::ostringstream stream;
    for (featherdoc::Paragraph paragraph = document.paragraphs();
         paragraph.has_next(); paragraph.next()) {
        for (featherdoc::Run run = paragraph.runs(); run.has_next();
             run.next()) {
            stream << run.get_text();
        }
        stream << '\n';
    }
    return stream.str();
}

[[nodiscard]] std::filesystem::path write_import_structure_pdf(
    std::string_view filename) {
    featherdoc::pdf::PdfDocumentLayout layout;
    layout.metadata.title = "FeatherDoc PDF import structure";
    layout.metadata.creator = "FeatherDoc test";

    featherdoc::pdf::PdfPageLayout page;
    page.size = featherdoc::pdf::PdfPageSize::letter_portrait();
    page.text_runs.push_back(make_text_run(720.0, "First paragraph line one"));
    page.text_runs.push_back(make_text_run(702.0, "First paragraph line two"));
    page.text_runs.push_back(make_text_run(650.0, "Second paragraph starts"));
    layout.pages.push_back(std::move(page));

    const auto output_path =
        std::filesystem::current_path() / std::string{filename};

    featherdoc::pdf::PdfioGenerator generator;
    const auto write_result =
        generator.write(layout, output_path, featherdoc::pdf::PdfWriterOptions{});
    REQUIRE_MESSAGE(write_result.success, write_result.error_message);
    return output_path;
}

[[nodiscard]] std::filesystem::path write_blank_pdf(std::string_view filename) {
    featherdoc::pdf::PdfDocumentLayout layout;
    layout.metadata.title = "FeatherDoc blank PDF import structure";
    layout.metadata.creator = "FeatherDoc test";

    featherdoc::pdf::PdfPageLayout page;
    page.size = featherdoc::pdf::PdfPageSize::letter_portrait();
    layout.pages.push_back(std::move(page));

    const auto output_path =
        std::filesystem::current_path() / std::string{filename};

    featherdoc::pdf::PdfioGenerator generator;
    const auto write_result =
        generator.write(layout, output_path, featherdoc::pdf::PdfWriterOptions{});
    REQUIRE_MESSAGE(write_result.success, write_result.error_message);
    return output_path;
}

} // namespace

TEST_CASE("PDFium parser groups text spans into lines and paragraphs") {
    const auto output_path =
        write_import_structure_pdf("featherdoc-pdf-import-structure.pdf");

    featherdoc::pdf::PdfiumParser parser;
    const auto parse_result = parser.parse(output_path, {});
    REQUIRE_MESSAGE(parse_result.success, parse_result.error_message);
    REQUIRE_EQ(parse_result.document.pages.size(), 1U);

    const auto &parsed_page = parse_result.document.pages.front();
    CHECK_GE(parsed_page.text_spans.size(), 60U);
    REQUIRE_EQ(parsed_page.text_lines.size(), 3U);
    REQUIRE_EQ(parsed_page.paragraphs.size(), 2U);

    CHECK(contains_text(parsed_page.text_lines[0].text, "First paragraph line one"));
    CHECK(contains_text(parsed_page.text_lines[1].text, "First paragraph line two"));
    CHECK(contains_text(parsed_page.text_lines[2].text, "Second paragraph starts"));

    CHECK(contains_text(parsed_page.paragraphs[0].text, "First paragraph line one"));
    CHECK(contains_text(parsed_page.paragraphs[0].text, "First paragraph line two"));
    CHECK(contains_text(parsed_page.paragraphs[1].text, "Second paragraph starts"));
    CHECK_GT(parsed_page.paragraphs[0].bounds.height_points, 0.0);
    CHECK_GT(parsed_page.paragraphs[1].bounds.height_points, 0.0);
}

TEST_CASE("PDF text importer builds a plain FeatherDoc document") {
    const auto input_path =
        write_import_structure_pdf("featherdoc-pdf-import-document.pdf");
    const auto docx_path =
        std::filesystem::current_path() / "featherdoc-pdf-import-document.docx";
    std::filesystem::remove(docx_path);

    featherdoc::Document document(docx_path);
    const auto import_result =
        featherdoc::pdf::import_pdf_text_document(input_path, document);
    REQUIRE_MESSAGE(import_result.success, import_result.error_message);
    CHECK_EQ(import_result.paragraphs_imported, 2U);
    CHECK_EQ(collect_document_text(document),
             "First paragraph line one\nFirst paragraph line two\n"
             "Second paragraph starts\n");

    REQUIRE_FALSE(document.save());

    featherdoc::Document reopened(docx_path);
    REQUIRE_FALSE(reopened.open());
    CHECK_EQ(collect_document_text(reopened),
             "First paragraph line one\nFirst paragraph line two\n"
             "Second paragraph starts\n");

    std::filesystem::remove(docx_path);
}

TEST_CASE("PDF text importer reports unsupported parse options") {
    const auto input_path =
        write_import_structure_pdf("featherdoc-pdf-import-options.pdf");

    featherdoc::Document document;
    featherdoc::pdf::PdfDocumentImportOptions options;
    options.parse_options.extract_geometry = false;

    const auto import_result =
        featherdoc::pdf::import_pdf_text_document(input_path, document, options);
    CHECK_FALSE(import_result.success);
    CHECK(contains_text(import_result.error_message, "extract_geometry=true"));
}

TEST_CASE("PDF text importer reports PDFs without text paragraphs") {
    const auto input_path =
        write_blank_pdf("featherdoc-pdf-import-no-text.pdf");

    featherdoc::Document document;
    const auto import_result =
        featherdoc::pdf::import_pdf_text_document(input_path, document);

    CHECK_FALSE(import_result.success);
    CHECK(contains_text(import_result.error_message, "text paragraphs only"));
}
