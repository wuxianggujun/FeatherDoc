#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include <featherdoc/pdf/pdf_parser.hpp>
#include <featherdoc/pdf/pdf_writer.hpp>

#include <filesystem>
#include <string>
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

} // namespace

TEST_CASE("PDFium parser groups text spans into lines and paragraphs") {
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
        std::filesystem::current_path() / "featherdoc-pdf-import-structure.pdf";

    featherdoc::pdf::PdfioGenerator generator;
    const auto write_result =
        generator.write(layout, output_path, featherdoc::pdf::PdfWriterOptions{});
    REQUIRE_MESSAGE(write_result.success, write_result.error_message);

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
