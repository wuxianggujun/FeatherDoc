#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include <featherdoc/pdf/pdf_parser.hpp>
#include <featherdoc/pdf/pdf_text_shaper.hpp>
#include <featherdoc/pdf/pdf_writer.hpp>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

auto utf8_from_u8(std::u8string_view text) -> std::string {
    return {reinterpret_cast<const char *>(text.data()), text.size()};
}

[[nodiscard]] std::string environment_value(const char *name) {
#if defined(_WIN32)
    char *value = nullptr;
    std::size_t size = 0;
    if (::_dupenv_s(&value, &size, name) != 0 || value == nullptr) {
        return {};
    }

    std::string result(value, size > 0 ? size - 1 : 0);
    std::free(value);
    return result;
#else
    const char *value = std::getenv(name);
    return value == nullptr ? std::string{} : std::string{value};
#endif
}

[[nodiscard]] std::vector<std::filesystem::path> candidate_cjk_fonts() {
    std::vector<std::filesystem::path> candidates;

    if (const auto configured = environment_value("FEATHERDOC_TEST_CJK_FONT");
        !configured.empty()) {
        candidates.emplace_back(configured);
    }

#if defined(_WIN32)
    candidates.emplace_back("C:/Windows/Fonts/Deng.ttf");
    candidates.emplace_back("C:/Windows/Fonts/NotoSansSC-VF.ttf");
    candidates.emplace_back("C:/Windows/Fonts/msyh.ttc");
    candidates.emplace_back("C:/Windows/Fonts/simhei.ttf");
    candidates.emplace_back("C:/Windows/Fonts/simsun.ttc");
#endif

    return candidates;
}

[[nodiscard]] std::vector<std::filesystem::path> candidate_latin_fonts() {
    std::vector<std::filesystem::path> candidates;

    if (const auto configured = environment_value("FEATHERDOC_TEST_LATIN_FONT");
        !configured.empty()) {
        candidates.emplace_back(configured);
    }

#if defined(_WIN32)
    candidates.emplace_back("C:/Windows/Fonts/arial.ttf");
    candidates.emplace_back("C:/Windows/Fonts/calibri.ttf");
    candidates.emplace_back("C:/Windows/Fonts/times.ttf");
#endif

    return candidates;
}

[[nodiscard]] std::filesystem::path find_cjk_font() {
    for (const auto &candidate : candidate_cjk_fonts()) {
        if (std::filesystem::exists(candidate)) {
            return candidate;
        }
    }
    return {};
}

[[nodiscard]] std::filesystem::path find_latin_font() {
    for (const auto &candidate : candidate_latin_fonts()) {
        if (std::filesystem::exists(candidate)) {
            return candidate;
        }
    }
    return {};
}

[[nodiscard]] std::string read_file_bytes(const std::filesystem::path &path) {
    std::ifstream input(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(input),
            std::istreambuf_iterator<char>()};
}

[[nodiscard]] std::string collect_text(
    const featherdoc::pdf::PdfParsedDocument &document) {
    std::string text;
    for (const auto &page : document.pages) {
        for (const auto &span : page.text_spans) {
            text += span.text;
        }
    }
    return text;
}

[[nodiscard]] std::size_t count_occurrences(std::string_view haystack,
                                            std::string_view needle) {
    std::size_t count = 0U;
    std::size_t offset = 0U;
    while ((offset = haystack.find(needle, offset)) != std::string_view::npos) {
        ++count;
        offset += needle.size();
    }
    return count;
}

} // namespace

TEST_CASE("PDFio embeds Unicode CJK font and PDFium extracts text") {
    const auto font_path = find_cjk_font();
    if (font_path.empty()) {
        MESSAGE("skipping CJK PDF smoke: no CJK font found; set "
                "FEATHERDOC_TEST_CJK_FONT to run it");
        return;
    }

    const auto expected_text = utf8_from_u8(u8"中文测试 ABC 123，标点。");
    const auto output_path =
        std::filesystem::current_path() / "featherdoc-cjk-font-roundtrip.pdf";

    featherdoc::pdf::PdfDocumentLayout layout;
    layout.metadata.title = "FeatherDoc CJK font roundtrip";
    layout.metadata.creator = "FeatherDoc test";

    featherdoc::pdf::PdfPageLayout page;
    page.size = featherdoc::pdf::PdfPageSize::a4_portrait();
    page.text_runs.push_back(featherdoc::pdf::PdfTextRun{
        featherdoc::pdf::PdfPoint{72.0, 720.0},
        expected_text,
        "CJK Test Font",
        font_path,
        16.0,
        featherdoc::pdf::PdfRgbColor{0.0, 0.0, 0.0},
        false,
        false,
        false,
        true,
    });
    layout.pages.push_back(std::move(page));

    featherdoc::pdf::PdfioGenerator generator;
    const auto write_result =
        generator.write(layout, output_path, featherdoc::pdf::PdfWriterOptions{});
    REQUIRE_MESSAGE(write_result.success, write_result.error_message);
    CHECK_GT(write_result.bytes_written, 0U);

    const auto pdf_bytes = read_file_bytes(output_path);
    CHECK_NE(pdf_bytes.find("/Type0"), std::string::npos);
    CHECK_NE(pdf_bytes.find("/CIDFontType2"), std::string::npos);
    CHECK_NE(pdf_bytes.find("/ToUnicode"), std::string::npos);
    CHECK_NE(pdf_bytes.find("/Identity-H"), std::string::npos);

    featherdoc::pdf::PdfiumParser parser;
    const auto parse_result = parser.parse(output_path, {});
    REQUIRE_MESSAGE(parse_result.success, parse_result.error_message);

    const auto extracted_text = collect_text(parse_result.document);
    CHECK_NE(extracted_text.find(expected_text), std::string::npos);
}

TEST_CASE("PDFio subsets embedded Unicode CJK font data") {
    const auto font_path = find_cjk_font();
    if (font_path.empty()) {
        MESSAGE("skipping CJK PDF subset smoke: no CJK font found; set "
                "FEATHERDOC_TEST_CJK_FONT to run it");
        return;
    }

    const auto expected_text = utf8_from_u8(u8"中文合同测试 ABC 123，标点。");

    featherdoc::pdf::PdfDocumentLayout layout;
    layout.metadata.title = "FeatherDoc CJK font subset";
    layout.metadata.creator = "FeatherDoc test";

    featherdoc::pdf::PdfPageLayout page;
    page.size = featherdoc::pdf::PdfPageSize::a4_portrait();
    page.text_runs.push_back(featherdoc::pdf::PdfTextRun{
        featherdoc::pdf::PdfPoint{72.0, 720.0},
        expected_text,
        "CJK Test Font",
        font_path,
        16.0,
        featherdoc::pdf::PdfRgbColor{0.0, 0.0, 0.0},
        false,
        false,
        false,
        true,
    });
    layout.pages.push_back(std::move(page));

    const auto subset_path =
        std::filesystem::current_path() / "featherdoc-cjk-font-subset.pdf";
    const auto full_path =
        std::filesystem::current_path() / "featherdoc-cjk-font-full.pdf";

    featherdoc::pdf::PdfioGenerator generator;
    featherdoc::pdf::PdfWriterOptions full_options;
    full_options.subset_unicode_fonts = false;
    const auto full_result = generator.write(layout, full_path, full_options);
    REQUIRE_MESSAGE(full_result.success, full_result.error_message);

    featherdoc::pdf::PdfWriterOptions subset_options;
    subset_options.subset_unicode_fonts = true;
    const auto subset_result =
        generator.write(layout, subset_path, subset_options);
    REQUIRE_MESSAGE(subset_result.success, subset_result.error_message);

#if defined(FEATHERDOC_ENABLE_PDF_FONT_SUBSET)
    CHECK_GT(full_result.bytes_written, subset_result.bytes_written);
#endif

    const auto subset_pdf_bytes = read_file_bytes(subset_path);
    CHECK_NE(subset_pdf_bytes.find("/Type0"), std::string::npos);
    CHECK_NE(subset_pdf_bytes.find("/CIDFontType2"), std::string::npos);
    CHECK_NE(subset_pdf_bytes.find("/ToUnicode"), std::string::npos);

    featherdoc::pdf::PdfiumParser parser;
    const auto parse_result = parser.parse(subset_path, {});
    REQUIRE_MESSAGE(parse_result.success, parse_result.error_message);

    const auto extracted_text = collect_text(parse_result.document);
    CHECK_NE(extracted_text.find(expected_text), std::string::npos);
}

TEST_CASE("PDFio writes shaped glyph CID text without duplicate extraction") {
    const auto font_path = find_latin_font();
    if (font_path.empty()) {
        MESSAGE("skipping shaped glyph PDF writer smoke: configure test Latin "
                "font");
        return;
    }
    if (!featherdoc::pdf::pdf_text_shaper_has_harfbuzz()) {
        MESSAGE("skipping shaped glyph PDF writer smoke: HarfBuzz unavailable");
        return;
    }

    const std::string expected_text = "office affinity efficient";
    constexpr double font_size = 18.0;
    auto glyph_run = featherdoc::pdf::shape_pdf_text(
        expected_text,
        featherdoc::pdf::PdfTextShaperOptions{font_path, font_size});
    if (!glyph_run.used_harfbuzz || !glyph_run.error_message.empty() ||
        glyph_run.glyphs.empty()) {
        MESSAGE("skipping shaped glyph PDF writer smoke: shaping failed");
        return;
    }

    const auto output_path =
        std::filesystem::current_path() / "featherdoc-shaped-glyph-text.pdf";

    featherdoc::pdf::PdfDocumentLayout layout;
    layout.metadata.title = "FeatherDoc shaped glyph text";
    layout.metadata.creator = "FeatherDoc test";

    featherdoc::pdf::PdfPageLayout page;
    page.size = featherdoc::pdf::PdfPageSize::a4_portrait();
    page.text_runs.push_back(featherdoc::pdf::PdfTextRun{
        featherdoc::pdf::PdfPoint{72.0, 720.0},
        expected_text,
        "Latin Test Font",
        font_path,
        font_size,
        featherdoc::pdf::PdfRgbColor{0.0, 0.0, 0.0},
        false,
        false,
        false,
        true,
        0.0,
        false,
        false,
        std::move(glyph_run),
    });
    layout.pages.push_back(std::move(page));

    featherdoc::pdf::PdfioGenerator generator;
    const auto write_result =
        generator.write(layout, output_path, featherdoc::pdf::PdfWriterOptions{});
    REQUIRE_MESSAGE(write_result.success, write_result.error_message);
    CHECK_GT(write_result.bytes_written, 0U);

    const auto pdf_bytes = read_file_bytes(output_path);
    CHECK_NE(pdf_bytes.find("/FeatherDocGlyph"), std::string::npos);
    CHECK_NE(pdf_bytes.find("/CIDToGIDMap"), std::string::npos);
    CHECK_NE(pdf_bytes.find("/ToUnicode"), std::string::npos);

    featherdoc::pdf::PdfiumParser parser;
    const auto parse_result = parser.parse(output_path, {});
    REQUIRE_MESSAGE(parse_result.success, parse_result.error_message);

    const auto extracted_text = collect_text(parse_result.document);
    CHECK_EQ(count_occurrences(extracted_text, expected_text), 1U);
}

TEST_CASE("PDFio reports a missing Unicode font path") {
    const auto missing_font_path =
        std::filesystem::current_path() / "featherdoc-missing-cjk-font.ttf";
    REQUIRE_FALSE(std::filesystem::exists(missing_font_path));

    featherdoc::pdf::PdfDocumentLayout layout;
    layout.metadata.title = "FeatherDoc missing CJK font diagnostic";
    layout.metadata.creator = "FeatherDoc test";

    featherdoc::pdf::PdfPageLayout page;
    page.size = featherdoc::pdf::PdfPageSize::a4_portrait();
    page.text_runs.push_back(featherdoc::pdf::PdfTextRun{
        featherdoc::pdf::PdfPoint{72.0, 720.0},
        utf8_from_u8(u8"中文测试 ABC 123"),
        "Missing CJK Test Font",
        missing_font_path,
        16.0,
        featherdoc::pdf::PdfRgbColor{0.0, 0.0, 0.0},
        false,
        false,
        false,
        true,
    });
    layout.pages.push_back(std::move(page));

    const auto output_path =
        std::filesystem::current_path() /
        "featherdoc-missing-cjk-font-diagnostic.pdf";

    featherdoc::pdf::PdfioGenerator generator;
    const auto write_result =
        generator.write(layout, output_path, featherdoc::pdf::PdfWriterOptions{});

    CHECK_FALSE(write_result.success);
    CHECK_NE(write_result.error_message.find("Unable to create PDF font"),
             std::string::npos);
    CHECK_NE(write_result.error_message.find(missing_font_path.string()),
             std::string::npos);
}

TEST_CASE("PDFio reports an unresolved Unicode font") {
    featherdoc::pdf::PdfDocumentLayout layout;
    layout.metadata.title = "FeatherDoc unresolved Unicode font diagnostic";
    layout.metadata.creator = "FeatherDoc test";

    featherdoc::pdf::PdfPageLayout page;
    page.size = featherdoc::pdf::PdfPageSize::a4_portrait();
    page.text_runs.push_back(featherdoc::pdf::PdfTextRun{
        featherdoc::pdf::PdfPoint{72.0, 720.0},
        utf8_from_u8(u8"中文测试 ABC 123"),
        "Unresolved CJK Test Font",
        {},
        16.0,
        featherdoc::pdf::PdfRgbColor{0.0, 0.0, 0.0},
        false,
        false,
        false,
        true,
    });
    layout.pages.push_back(std::move(page));

    const auto output_path =
        std::filesystem::current_path() /
        "featherdoc-unresolved-cjk-font-diagnostic.pdf";

    featherdoc::pdf::PdfioGenerator generator;
    const auto write_result =
        generator.write(layout, output_path, featherdoc::pdf::PdfWriterOptions{});

    CHECK_FALSE(write_result.success);
    CHECK_NE(write_result.error_message.find(
                 "Unicode PDF text requires an embedded font file"),
             std::string::npos);
    CHECK_NE(write_result.error_message.find("Unresolved CJK Test Font"),
             std::string::npos);
}

TEST_CASE("PDFio reports a corrupt Unicode font file") {
    const auto corrupt_font_path =
        std::filesystem::current_path() / "featherdoc-corrupt-cjk-font.ttf";
    {
        std::ofstream output(corrupt_font_path, std::ios::binary);
        output << "not a font";
    }

    featherdoc::pdf::PdfDocumentLayout layout;
    layout.metadata.title = "FeatherDoc corrupt CJK font diagnostic";
    layout.metadata.creator = "FeatherDoc test";

    featherdoc::pdf::PdfPageLayout page;
    page.size = featherdoc::pdf::PdfPageSize::a4_portrait();
    page.text_runs.push_back(featherdoc::pdf::PdfTextRun{
        featherdoc::pdf::PdfPoint{72.0, 720.0},
        utf8_from_u8(u8"中文测试 ABC 123"),
        "Corrupt CJK Test Font",
        corrupt_font_path,
        16.0,
        featherdoc::pdf::PdfRgbColor{0.0, 0.0, 0.0},
        false,
        false,
        false,
        true,
    });
    layout.pages.push_back(std::move(page));

    const auto output_path =
        std::filesystem::current_path() /
        "featherdoc-corrupt-cjk-font-diagnostic.pdf";

    featherdoc::pdf::PdfioGenerator generator;
    const auto write_result =
        generator.write(layout, output_path, featherdoc::pdf::PdfWriterOptions{});

    CHECK_FALSE(write_result.success);
    CHECK_NE(write_result.error_message.find("Unable to create PDF font"),
             std::string::npos);
    CHECK_NE(write_result.error_message.find(corrupt_font_path.string()),
             std::string::npos);
}
