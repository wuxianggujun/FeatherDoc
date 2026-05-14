#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include <featherdoc/pdf/pdf_document_adapter.hpp>
#include <featherdoc/pdf/pdf_text_metrics.hpp>
#include <featherdoc/pdf/pdf_text_shaper.hpp>
#if defined(FEATHERDOC_BUILD_PDF_IMPORT)
#include <featherdoc/pdf/pdf_parser.hpp>
#endif
#include <featherdoc/pdf/pdf_writer.hpp>

#include <cstdint>
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

[[nodiscard]] std::string tiny_png_data() {
    constexpr unsigned char tiny_png_bytes[] = {
        0x89U, 0x50U, 0x4EU, 0x47U, 0x0DU, 0x0AU, 0x1AU, 0x0AU, 0x00U, 0x00U,
        0x00U, 0x0DU, 0x49U, 0x48U, 0x44U, 0x52U, 0x00U, 0x00U, 0x00U, 0x01U,
        0x00U, 0x00U, 0x00U, 0x01U, 0x08U, 0x02U, 0x00U, 0x00U, 0x00U, 0x90U,
        0x77U, 0x53U, 0xDEU, 0x00U, 0x00U, 0x00U, 0x0CU, 0x49U, 0x44U, 0x41U,
        0x54U, 0x08U, 0xD7U, 0x63U, 0xF8U, 0xCFU, 0xC0U, 0x00U, 0x00U, 0x03U,
        0x01U, 0x01U, 0x00U, 0x18U, 0xDDU, 0x8DU, 0xB0U, 0x00U, 0x00U, 0x00U,
        0x00U, 0x49U, 0x45U, 0x4EU, 0x44U, 0xAEU, 0x42U, 0x60U, 0x82U,
    };

    return {reinterpret_cast<const char *>(tiny_png_bytes),
            sizeof(tiny_png_bytes)};
}

[[nodiscard]] std::string tiny_rgba_png_data() {
    constexpr unsigned char tiny_png_bytes[] = {
        0x89U, 0x50U, 0x4EU, 0x47U, 0x0DU, 0x0AU, 0x1AU, 0x0AU, 0x00U, 0x00U,
        0x00U, 0x0DU, 0x49U, 0x48U, 0x44U, 0x52U, 0x00U, 0x00U, 0x00U, 0x02U,
        0x00U, 0x00U, 0x00U, 0x02U, 0x08U, 0x06U, 0x00U, 0x00U, 0x00U, 0x72U,
        0xB6U, 0x0DU, 0x24U, 0x00U, 0x00U, 0x00U, 0x16U, 0x49U, 0x44U, 0x41U,
        0x54U, 0x78U, 0xDAU, 0x63U, 0xF8U, 0xCFU, 0xC0U, 0xF0U, 0x1FU, 0x08U,
        0x1BU, 0x18U, 0x80U, 0xB4U, 0xC3U, 0x7FU, 0x20U, 0x0FU, 0x00U, 0x3EU,
        0x1AU, 0x06U, 0xBBU, 0x82U, 0x99U, 0xA3U, 0xF4U, 0x00U, 0x00U, 0x00U,
        0x00U, 0x49U, 0x45U, 0x4EU, 0x44U, 0xAEU, 0x42U, 0x60U, 0x82U,
    };

    return {reinterpret_cast<const char *>(tiny_png_bytes),
            sizeof(tiny_png_bytes)};
}

[[nodiscard]] std::string quadrant_png_data() {
    constexpr unsigned char quadrant_png_bytes[] = {
        0x89U, 0x50U, 0x4EU, 0x47U, 0x0DU, 0x0AU, 0x1AU, 0x0AU, 0x00U, 0x00U,
        0x00U, 0x0DU, 0x49U, 0x48U, 0x44U, 0x52U, 0x00U, 0x00U, 0x00U, 0x08U,
        0x00U, 0x00U, 0x00U, 0x04U, 0x08U, 0x02U, 0x00U, 0x00U, 0x00U, 0x3CU,
        0xAFU, 0xE9U, 0xA7U, 0x00U, 0x00U, 0x00U, 0x1DU, 0x49U, 0x44U, 0x41U,
        0x54U, 0x78U, 0xDAU, 0x63U, 0xF8U, 0xCFU, 0xC0U, 0x00U, 0x47U, 0x48U,
        0xCCU, 0xFFU, 0x0CU, 0x38U, 0x25U, 0x18U, 0x4EU, 0x20U, 0xD0U, 0xFFU,
        0x3BU, 0x08U, 0x84U, 0x53U, 0x02U, 0x00U, 0x8FU, 0xCEU, 0x25U, 0x09U,
        0x87U, 0xCFU, 0x36U, 0xFCU, 0x00U, 0x00U, 0x00U, 0x00U, 0x49U, 0x45U,
        0x4EU, 0x44U, 0xAEU, 0x42U, 0x60U, 0x82U,
    };

    return {reinterpret_cast<const char *>(quadrant_png_bytes),
            sizeof(quadrant_png_bytes)};
}

void write_binary_file(const std::filesystem::path &path,
                       std::string_view data) {
    std::ofstream stream(path, std::ios::binary);
    REQUIRE(stream.good());
    stream.write(data.data(), static_cast<std::streamsize>(data.size()));
    REQUIRE(stream.good());
}

[[nodiscard]] std::string read_binary_file(const std::filesystem::path &path) {
    std::ifstream stream(path, std::ios::binary);
    REQUIRE(stream.good());
    return {std::istreambuf_iterator<char>{stream},
            std::istreambuf_iterator<char>{}};
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

    if (const auto configured =
            environment_value("FEATHERDOC_TEST_PROPORTIONAL_FONT");
        !configured.empty()) {
        candidates.emplace_back(configured);
    }

#if defined(_WIN32)
    candidates.emplace_back("C:/Windows/Fonts/arial.ttf");
    candidates.emplace_back("C:/Windows/Fonts/segoeui.ttf");
    candidates.emplace_back("C:/Windows/Fonts/calibri.ttf");
#endif

    return candidates;
}

[[nodiscard]] std::vector<std::filesystem::path> candidate_rtl_fonts() {
    std::vector<std::filesystem::path> candidates;

    if (const auto configured = environment_value("FEATHERDOC_TEST_RTL_FONT");
        !configured.empty()) {
        candidates.emplace_back(configured);
    }

#if defined(_WIN32)
    candidates.emplace_back("C:/Windows/Fonts/arial.ttf");
    candidates.emplace_back("C:/Windows/Fonts/segoeui.ttf");
    candidates.emplace_back("C:/Windows/Fonts/times.ttf");
#endif

    return candidates;
}

[[nodiscard]] std::filesystem::path
first_existing_path(const std::vector<std::filesystem::path> &candidates) {
    for (const auto &candidate : candidates) {
        if (std::filesystem::exists(candidate)) {
            return candidate;
        }
    }
    return {};
}

[[nodiscard]] std::filesystem::path make_temp_font_file(std::string_view name) {
    const auto path = std::filesystem::current_path() / name;
    std::ofstream stream(path, std::ios::binary);
    stream << "featherdoc adapter font mapping fixture";
    return path;
}

#if defined(FEATHERDOC_BUILD_PDF_IMPORT)
[[nodiscard]] std::string
collect_text(const featherdoc::pdf::PdfParsedDocument &document) {
    std::string text;
    for (const auto &page : document.pages) {
        for (const auto &span : page.text_spans) {
            text += span.text;
        }
    }
    return text;
}

[[nodiscard]] std::string
collect_text(const featherdoc::pdf::PdfParsedPage &page) {
    std::string text;
    for (const auto &span : page.text_spans) {
        text += span.text;
    }
    return text;
}
#endif

[[nodiscard]] std::string
collect_layout_text(const featherdoc::pdf::PdfDocumentLayout &layout) {
    std::string text;
    for (const auto &page : layout.pages) {
        for (const auto &text_run : page.text_runs) {
            text += text_run.text;
        }
    }
    return text;
}

featherdoc::Paragraph append_body_paragraph(featherdoc::Document &document,
                                            std::string_view text) {
    auto paragraph = document.paragraphs();
    while (paragraph.has_next()) {
        paragraph.next();
    }

    return paragraph.insert_paragraph_after(std::string{text});
}

} // namespace

TEST_CASE("document PDF adapter resolves run-level CJK font mappings") {
    const auto latin_font = first_existing_path(candidate_latin_fonts());
    const auto cjk_font = first_existing_path(candidate_cjk_fonts());
    if (latin_font.empty() || cjk_font.empty()) {
        MESSAGE("skipping adapter font mapping test: configure test Latin/CJK "
                "fonts");
        return;
    }

    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());
    CHECK(document.set_default_run_font_family("Unit Latin"));
    CHECK(document.set_default_run_east_asia_font_family("Unit CJK"));

    auto paragraph = document.paragraphs();
    REQUIRE(paragraph.has_next());
    REQUIRE(paragraph.add_run("Contract ABC ").has_next());
    REQUIRE(paragraph.add_run(utf8_from_u8(u8"中文测试")).has_next());

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.font_mappings = {
        featherdoc::pdf::PdfFontMapping{"Unit Latin", latin_font},
        featherdoc::pdf::PdfFontMapping{"Unit CJK", cjk_font},
    };
    options.use_system_font_fallbacks = false;

    const auto layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);

    REQUIRE_EQ(layout.pages.size(), 1U);
    REQUIRE_GE(layout.pages.front().text_runs.size(), 2U);

    const auto &latin_run = layout.pages.front().text_runs[0];
    CHECK_EQ(latin_run.text, "Contract ABC ");
    CHECK_EQ(latin_run.font_family, "Unit Latin");
    CHECK_EQ(latin_run.font_file_path, latin_font);
    CHECK_FALSE(latin_run.unicode);

    const auto &cjk_run = layout.pages.front().text_runs[1];
    CHECK_EQ(cjk_run.text, utf8_from_u8(u8"中文测试"));
    CHECK_EQ(cjk_run.font_family, "Unit CJK");
    CHECK_EQ(cjk_run.font_file_path, cjk_font);
    CHECK(cjk_run.unicode);
    CHECK_GT(cjk_run.baseline_origin.x_points,
             latin_run.baseline_origin.x_points + 24.0);
    CHECK_EQ(cjk_run.baseline_origin.y_points,
             doctest::Approx(latin_run.baseline_origin.y_points));
}

TEST_CASE("document PDF adapter carries shaped glyph run for file-backed text") {
    const auto latin_font = first_existing_path(candidate_latin_fonts());
    if (latin_font.empty()) {
        MESSAGE("skipping adapter glyph run test: configure test Latin font");
        return;
    }

    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());

    auto paragraph = document.paragraphs();
    REQUIRE(paragraph.has_next());
    auto first_run = paragraph.add_run("office");
    REQUIRE(first_run.has_next());
    CHECK(first_run.set_font_family("Unit Latin A"));
    auto second_run = paragraph.add_run(" after");
    REQUIRE(second_run.has_next());
    CHECK(second_run.set_font_family("Unit Latin B"));

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.font_mappings = {
        featherdoc::pdf::PdfFontMapping{"Unit Latin A", latin_font},
        featherdoc::pdf::PdfFontMapping{"Unit Latin B", latin_font},
    };
    options.use_system_font_fallbacks = false;

    const auto layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);

    REQUIRE_EQ(layout.pages.size(), 1U);
    REQUIRE_EQ(layout.pages.front().text_runs.size(), 2U);

    const auto &text_run = layout.pages.front().text_runs[0];
    CHECK_EQ(text_run.text, "office");
    CHECK_EQ(text_run.font_file_path, latin_font);
    const auto &after_run = layout.pages.front().text_runs[1];
    CHECK_EQ(after_run.text, " after");
    CHECK_EQ(after_run.font_file_path, latin_font);

    if (!featherdoc::pdf::pdf_text_shaper_has_harfbuzz()) {
        CHECK_FALSE(text_run.glyph_run.used_harfbuzz);
        CHECK(text_run.glyph_run.glyphs.empty());
        return;
    }

    CHECK(text_run.glyph_run.used_harfbuzz);
    CHECK(text_run.glyph_run.error_message.empty());
    CHECK_EQ(text_run.glyph_run.text, "office");
    CHECK_EQ(text_run.glyph_run.font_file_path, latin_font);
    CHECK(text_run.glyph_run.font_size_points == doctest::Approx(12.0));
    CHECK_EQ(text_run.glyph_run.direction,
             featherdoc::pdf::PdfGlyphDirection::left_to_right);
    CHECK_EQ(text_run.glyph_run.script_tag, "Latn");
    REQUIRE_FALSE(text_run.glyph_run.glyphs.empty());

    double total_advance = 0.0;
    for (const auto &glyph : text_run.glyph_run.glyphs) {
        total_advance += glyph.x_advance_points;
        CHECK_LT(glyph.cluster,
                 std::char_traits<char>::length("office"));
    }
    CHECK_GT(total_advance, 1.0);
    CHECK(after_run.baseline_origin.x_points ==
          doctest::Approx(text_run.baseline_origin.x_points +
                          featherdoc::pdf::glyph_run_x_advance_points(
                              text_run.glyph_run)));
}

TEST_CASE("document PDF adapter carries RTL shaped direction metadata") {
    const auto rtl_font = first_existing_path(candidate_rtl_fonts());
    if (rtl_font.empty()) {
        MESSAGE("skipping adapter RTL glyph direction test: configure test "
                "RTL font");
        return;
    }

    const auto expected_text = utf8_from_u8(u8"\u05E9\u05DC\u05D5\u05DD");

    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());

    auto paragraph = document.paragraphs();
    REQUIRE(paragraph.has_next());
    auto rtl_run = paragraph.add_run(expected_text);
    REQUIRE(rtl_run.has_next());
    CHECK(rtl_run.set_font_family("Unit RTL"));

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.font_mappings = {
        featherdoc::pdf::PdfFontMapping{"Unit RTL", rtl_font},
    };
    options.use_system_font_fallbacks = false;

    const auto layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);

    REQUIRE_EQ(layout.pages.size(), 1U);
    REQUIRE_EQ(layout.pages.front().text_runs.size(), 1U);

    const auto &text_run = layout.pages.front().text_runs.front();
    CHECK_EQ(text_run.text, expected_text);
    CHECK_EQ(text_run.font_file_path, rtl_font);

    if (!featherdoc::pdf::pdf_text_shaper_has_harfbuzz()) {
        CHECK_FALSE(text_run.glyph_run.used_harfbuzz);
        CHECK(text_run.glyph_run.glyphs.empty());
        return;
    }

    CHECK(text_run.glyph_run.used_harfbuzz);
    CHECK(text_run.glyph_run.error_message.empty());
    CHECK_EQ(text_run.glyph_run.text, expected_text);
    CHECK_EQ(text_run.glyph_run.font_file_path, rtl_font);
    CHECK_EQ(text_run.glyph_run.direction,
             featherdoc::pdf::PdfGlyphDirection::right_to_left);
    CHECK_EQ(text_run.glyph_run.script_tag, "Hebr");
    REQUIRE_FALSE(text_run.glyph_run.glyphs.empty());
}

TEST_CASE("document PDF adapter maps run RTL formatting into shaping options") {
    const auto latin_font = first_existing_path(candidate_latin_fonts());
    if (latin_font.empty()) {
        MESSAGE("skipping adapter run RTL shaping test: configure test Latin "
                "font");
        return;
    }

    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());

    auto paragraph = document.paragraphs();
    REQUIRE(paragraph.has_next());
    auto rtl_run = paragraph.add_run("office");
    REQUIRE(rtl_run.has_next());
    CHECK(rtl_run.set_font_family("Unit Latin RTL"));
    CHECK(rtl_run.set_rtl());

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.font_mappings = {
        featherdoc::pdf::PdfFontMapping{"Unit Latin RTL", latin_font},
    };
    options.use_system_font_fallbacks = false;

    const auto layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);

    REQUIRE_EQ(layout.pages.size(), 1U);
    REQUIRE_EQ(layout.pages.front().text_runs.size(), 1U);

    const auto &text_run = layout.pages.front().text_runs.front();
    CHECK_EQ(text_run.text, "office");
    CHECK_EQ(text_run.font_file_path, latin_font);

    if (!featherdoc::pdf::pdf_text_shaper_has_harfbuzz()) {
        CHECK_FALSE(text_run.glyph_run.used_harfbuzz);
        CHECK(text_run.glyph_run.glyphs.empty());
        return;
    }

    CHECK(text_run.glyph_run.used_harfbuzz);
    CHECK(text_run.glyph_run.error_message.empty());
    CHECK_EQ(text_run.glyph_run.direction,
             featherdoc::pdf::PdfGlyphDirection::right_to_left);
    CHECK_EQ(text_run.glyph_run.script_tag, "Latn");
    REQUIRE_FALSE(text_run.glyph_run.glyphs.empty());
}

TEST_CASE(
    "document PDF adapter prefers east Asia font mapping for mixed text") {
    const auto latin_font = first_existing_path(candidate_latin_fonts());
    const auto cjk_font = first_existing_path(candidate_cjk_fonts());
    if (latin_font.empty() || cjk_font.empty()) {
        MESSAGE("skipping adapter east Asia preference test: configure test "
                "Latin/CJK fonts");
        return;
    }

    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());
    CHECK(document.set_default_run_font_family("Unit Latin"));
    CHECK(document.set_default_run_east_asia_font_family("Unit CJK"));

    auto paragraph = document.paragraphs();
    REQUIRE(paragraph.has_next());
    REQUIRE(paragraph.add_run("Mixed font ").has_next());
    REQUIRE(paragraph.add_run(utf8_from_u8(u8"中文 Mixed")).has_next());

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.font_mappings = {
        featherdoc::pdf::PdfFontMapping{"Unit Latin", latin_font},
        featherdoc::pdf::PdfFontMapping{"Unit CJK", cjk_font},
    };
    options.use_system_font_fallbacks = false;

    const auto layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);

    REQUIRE_EQ(layout.pages.size(), 1U);
    REQUIRE_GE(layout.pages.front().text_runs.size(), 2U);

    const auto &latin_run = layout.pages.front().text_runs[0];
    CHECK_EQ(latin_run.text, "Mixed font ");
    CHECK_EQ(latin_run.font_family, "Unit Latin");
    CHECK_EQ(latin_run.font_file_path, latin_font);
    CHECK_FALSE(latin_run.unicode);

    const auto &cjk_run = layout.pages.front().text_runs[1];
    CHECK_EQ(cjk_run.text, utf8_from_u8(u8"中文 Mixed"));
    CHECK_EQ(cjk_run.font_family, "Unit CJK");
    CHECK_EQ(cjk_run.font_file_path, cjk_font);
    CHECK(cjk_run.unicode);
}

TEST_CASE("document PDF adapter falls back to configured CJK font file path") {
    const auto latin_font =
        make_temp_font_file("featherdoc-adapter-latin-fallback.ttf");
    const auto cjk_font =
        make_temp_font_file("featherdoc-adapter-cjk-fallback.ttf");

    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());
    CHECK(document.set_default_run_font_family("Unit Latin"));
    CHECK(document.set_default_run_east_asia_font_family("Missing CJK"));

    auto paragraph = document.paragraphs();
    REQUIRE(paragraph.has_next());
    REQUIRE(paragraph.add_run("Contract ").has_next());
    REQUIRE(paragraph.add_run(utf8_from_u8(u8"中文")).has_next());

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.font_mappings = {
        featherdoc::pdf::PdfFontMapping{"Unit Latin", latin_font},
    };
    options.cjk_font_file_path = cjk_font;
    options.use_system_font_fallbacks = false;

    const auto layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);

    REQUIRE_EQ(layout.pages.size(), 1U);
    REQUIRE_GE(layout.pages.front().text_runs.size(), 2U);

    const auto &latin_run = layout.pages.front().text_runs[0];
    CHECK_EQ(latin_run.text, "Contract ");
    CHECK_EQ(latin_run.font_family, "Unit Latin");
    CHECK_EQ(latin_run.font_file_path, latin_font);
    CHECK_FALSE(latin_run.unicode);

    const auto &cjk_run = layout.pages.front().text_runs[1];
    CHECK_EQ(cjk_run.text, utf8_from_u8(u8"中文"));
    CHECK_EQ(cjk_run.font_family, "Missing CJK");
    CHECK_EQ(cjk_run.font_file_path, cjk_font);
    CHECK(cjk_run.unicode);
}

TEST_CASE("document PDF adapter maps bold italic underline run styling") {
    const auto regular_font =
        make_temp_font_file("featherdoc-adapter-regular.ttf");
    const auto bold_italic_font =
        make_temp_font_file("featherdoc-adapter-bold-italic.ttf");

    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());
    CHECK(document.set_default_run_font_family("Unit Style"));

    auto paragraph = document.paragraphs();
    REQUIRE(paragraph.has_next());
    REQUIRE(
        paragraph
            .add_run("Styled PDF", featherdoc::formatting_flag::bold |
                                       featherdoc::formatting_flag::italic |
                                       featherdoc::formatting_flag::underline)
            .has_next());

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.font_mappings = {
        featherdoc::pdf::PdfFontMapping{"Unit Style", regular_font},
        featherdoc::pdf::PdfFontMapping{"Unit Style", bold_italic_font, true,
                                        true},
    };
    options.use_system_font_fallbacks = false;

    const auto layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);

    REQUIRE_EQ(layout.pages.size(), 1U);
    REQUIRE_GE(layout.pages.front().text_runs.size(), 1U);

    const auto &styled_run = layout.pages.front().text_runs.front();
    CHECK_EQ(styled_run.text, "Styled PDF");
    CHECK_EQ(styled_run.font_family, "Unit Style");
    CHECK_EQ(styled_run.font_file_path, bold_italic_font);
    CHECK(styled_run.bold);
    CHECK(styled_run.italic);
    CHECK(styled_run.underline);
    CHECK_FALSE(styled_run.unicode);
    CHECK_FALSE(styled_run.synthetic_bold);
    CHECK_FALSE(styled_run.synthetic_italic);
}

TEST_CASE("document PDF adapter marks synthetic styles for missing file font "
          "variants") {
    const auto regular_font =
        make_temp_font_file("featherdoc-adapter-synthetic-regular.ttf");

    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());
    CHECK(document.set_default_run_font_family("Unit Synthetic"));

    auto paragraph = document.paragraphs();
    REQUIRE(paragraph.has_next());
    REQUIRE(paragraph.add_run("Synthetic styled PDF",
                              featherdoc::formatting_flag::bold |
                                  featherdoc::formatting_flag::italic)
                .has_next());

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.font_mappings = {
        featherdoc::pdf::PdfFontMapping{"Unit Synthetic", regular_font},
    };
    options.use_system_font_fallbacks = false;

    const auto layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);

    REQUIRE_EQ(layout.pages.size(), 1U);
    REQUIRE_GE(layout.pages.front().text_runs.size(), 1U);

    const auto &styled_run = layout.pages.front().text_runs.front();
    CHECK_EQ(styled_run.text, "Synthetic styled PDF");
    CHECK_EQ(styled_run.font_family, "Unit Synthetic");
    CHECK_EQ(styled_run.font_file_path, regular_font);
    CHECK(styled_run.bold);
    CHECK(styled_run.italic);
    CHECK(styled_run.synthetic_bold);
    CHECK(styled_run.synthetic_italic);
}

TEST_CASE("document PDF adapter resolves inherited run style formatting") {
    const auto regular_font =
        make_temp_font_file("featherdoc-adapter-style-regular.ttf");
    const auto bold_italic_font =
        make_temp_font_file("featherdoc-adapter-style-bold-italic.ttf");

    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());

    auto style_definition = featherdoc::character_style_definition{};
    style_definition.name = "PDF Styled Character";
    style_definition.run_text_color = std::string{"336699"};
    style_definition.run_bold = true;
    style_definition.run_italic = true;
    style_definition.run_underline = true;
    style_definition.run_font_size_points = 15.5;
    style_definition.run_font_family = std::string{"Unit Styled"};
    REQUIRE(document.ensure_character_style("PdfStyledCharacter",
                                            style_definition));
    REQUIRE(document.materialize_style_run_properties("PdfStyledCharacter"));

    auto paragraph = document.paragraphs();
    REQUIRE(paragraph.has_next());
    auto run = paragraph.add_run("Inherited PDF");
    REQUIRE(run.has_next());
    REQUIRE(document.set_run_style(run, "PdfStyledCharacter"));

    const auto resolved =
        document.resolve_style_properties("PdfStyledCharacter");
    REQUIRE(resolved.has_value());
    REQUIRE(resolved->run_font_family.value.has_value());
    CHECK_EQ(*resolved->run_font_family.value, "Unit Styled");
    REQUIRE(resolved->run_text_color.value.has_value());
    CHECK_EQ(*resolved->run_text_color.value, "336699");
    CHECK_EQ(resolved->run_bold.value.value_or(false), true);
    CHECK_EQ(resolved->run_italic.value.value_or(false), true);
    CHECK_EQ(resolved->run_underline.value.value_or(false), true);
    REQUIRE(resolved->run_font_size_points.value.has_value());
    CHECK_EQ(*resolved->run_font_size_points.value, doctest::Approx(15.5));

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.font_mappings = {
        featherdoc::pdf::PdfFontMapping{"Unit Styled", regular_font},
        featherdoc::pdf::PdfFontMapping{"Unit Styled", bold_italic_font, true,
                                        true},
    };
    options.use_system_font_fallbacks = false;

    const auto layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);

    REQUIRE_EQ(layout.pages.size(), 1U);
    REQUIRE_EQ(layout.pages.front().text_runs.size(), 1U);

    const auto &styled_run = layout.pages.front().text_runs.front();
    CHECK_EQ(styled_run.text, "Inherited PDF");
    CHECK_EQ(styled_run.font_family, "Unit Styled");
    CHECK_EQ(styled_run.font_file_path, bold_italic_font);
    CHECK_EQ(styled_run.font_size_points, doctest::Approx(15.5));
    CHECK_EQ(styled_run.fill_color.red, doctest::Approx(0x33 / 255.0));
    CHECK_EQ(styled_run.fill_color.green, doctest::Approx(0x66 / 255.0));
    CHECK_EQ(styled_run.fill_color.blue, doctest::Approx(0x99 / 255.0));
    CHECK(styled_run.bold);
    CHECK(styled_run.italic);
    CHECK(styled_run.underline);
}

TEST_CASE(
    "document PDF adapter resolves inherited east Asia run style formatting") {
    const auto latin_font =
        make_temp_font_file("featherdoc-adapter-style-eastasia-latin.ttf");
    const auto cjk_font =
        make_temp_font_file("featherdoc-adapter-style-eastasia-cjk.ttf");

    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());

    auto style_definition = featherdoc::character_style_definition{};
    style_definition.name = "PDF East Asia Character";
    style_definition.run_font_family = std::string{"Unit EastAsia Latin"};
    style_definition.run_east_asia_font_family =
        std::string{"Unit EastAsia CJK"};
    style_definition.run_font_size_points = 14.0;
    REQUIRE(document.ensure_character_style("PdfEastAsiaCharacter",
                                            style_definition));
    REQUIRE(document.materialize_style_run_properties("PdfEastAsiaCharacter"));

    auto paragraph = document.paragraphs();
    REQUIRE(paragraph.has_next());
    auto run = paragraph.add_run(utf8_from_u8(u8"中文样式"));
    REQUIRE(run.has_next());
    REQUIRE(document.set_run_style(run, "PdfEastAsiaCharacter"));

    const auto resolved =
        document.resolve_style_properties("PdfEastAsiaCharacter");
    REQUIRE(resolved.has_value());
    REQUIRE(resolved->run_font_family.value.has_value());
    CHECK_EQ(*resolved->run_font_family.value, "Unit EastAsia Latin");
    REQUIRE(resolved->run_east_asia_font_family.value.has_value());
    CHECK_EQ(*resolved->run_east_asia_font_family.value, "Unit EastAsia CJK");
    REQUIRE(resolved->run_font_size_points.value.has_value());
    CHECK_EQ(*resolved->run_font_size_points.value, doctest::Approx(14.0));

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.font_mappings = {
        featherdoc::pdf::PdfFontMapping{"Unit EastAsia Latin", latin_font},
        featherdoc::pdf::PdfFontMapping{"Unit EastAsia CJK", cjk_font},
    };
    options.use_system_font_fallbacks = false;

    const auto layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);

    REQUIRE_EQ(layout.pages.size(), 1U);
    REQUIRE_EQ(layout.pages.front().text_runs.size(), 1U);

    const auto &styled_run = layout.pages.front().text_runs.front();
    CHECK_EQ(styled_run.text, utf8_from_u8(u8"中文样式"));
    CHECK_EQ(styled_run.font_family, "Unit EastAsia CJK");
    CHECK_EQ(styled_run.font_file_path, cjk_font);
    CHECK_EQ(styled_run.font_size_points, doctest::Approx(14.0));
    CHECK(styled_run.unicode);
}

TEST_CASE("document PDF adapter resolves inherited east Asia run styling") {
    const auto latin_font = make_temp_font_file(
        "featherdoc-adapter-style-eastasia-combo-latin.ttf");
    const auto cjk_font =
        make_temp_font_file("featherdoc-adapter-style-eastasia-combo-cjk.ttf");
    const auto cjk_bold_italic_font = make_temp_font_file(
        "featherdoc-adapter-style-eastasia-combo-cjk-bold-italic.ttf");

    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());

    auto style_definition = featherdoc::character_style_definition{};
    style_definition.name = "PDF East Asia Styled Character";
    style_definition.run_font_family = std::string{"Unit EastAsia Combo Latin"};
    style_definition.run_east_asia_font_family =
        std::string{"Unit EastAsia Combo CJK"};
    style_definition.run_text_color = std::string{"336699"};
    style_definition.run_bold = true;
    style_definition.run_italic = true;
    style_definition.run_font_size_points = 18.0;
    REQUIRE(document.ensure_character_style("PdfEastAsiaStyledCharacter",
                                            style_definition));
    REQUIRE(document.materialize_style_run_properties(
        "PdfEastAsiaStyledCharacter"));

    auto paragraph = document.paragraphs();
    REQUIRE(paragraph.has_next());
    auto run = paragraph.add_run(utf8_from_u8(u8"中文样式"));
    REQUIRE(run.has_next());
    REQUIRE(document.set_run_style(run, "PdfEastAsiaStyledCharacter"));

    const auto resolved =
        document.resolve_style_properties("PdfEastAsiaStyledCharacter");
    REQUIRE(resolved.has_value());
    REQUIRE(resolved->run_font_family.value.has_value());
    CHECK_EQ(*resolved->run_font_family.value, "Unit EastAsia Combo Latin");
    REQUIRE(resolved->run_east_asia_font_family.value.has_value());
    CHECK_EQ(*resolved->run_east_asia_font_family.value,
             "Unit EastAsia Combo CJK");
    REQUIRE(resolved->run_text_color.value.has_value());
    CHECK_EQ(*resolved->run_text_color.value, "336699");
    REQUIRE(resolved->run_font_size_points.value.has_value());
    CHECK_EQ(*resolved->run_font_size_points.value, doctest::Approx(18.0));
    CHECK_EQ(resolved->run_bold.value.value_or(false), true);
    CHECK_EQ(resolved->run_italic.value.value_or(false), true);

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.font_mappings = {
        featherdoc::pdf::PdfFontMapping{"Unit EastAsia Combo Latin",
                                        latin_font},
        featherdoc::pdf::PdfFontMapping{"Unit EastAsia Combo CJK", cjk_font},
        featherdoc::pdf::PdfFontMapping{"Unit EastAsia Combo CJK",
                                        cjk_bold_italic_font, true, true},
    };
    options.use_system_font_fallbacks = false;

    const auto layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);

    REQUIRE_EQ(layout.pages.size(), 1U);
    REQUIRE_EQ(layout.pages.front().text_runs.size(), 1U);

    const auto &styled_run = layout.pages.front().text_runs.front();
    CHECK_EQ(styled_run.text, utf8_from_u8(u8"中文样式"));
    CHECK_EQ(styled_run.font_family, "Unit EastAsia Combo CJK");
    CHECK_EQ(styled_run.font_file_path, cjk_bold_italic_font);
    CHECK_EQ(styled_run.font_size_points, doctest::Approx(18.0));
    CHECK_EQ(styled_run.fill_color.red, doctest::Approx(0x33 / 255.0));
    CHECK_EQ(styled_run.fill_color.green, doctest::Approx(0x66 / 255.0));
    CHECK_EQ(styled_run.fill_color.blue, doctest::Approx(0x99 / 255.0));
    CHECK(styled_run.bold);
    CHECK(styled_run.italic);
    CHECK_FALSE(styled_run.underline);
    CHECK(styled_run.unicode);
}

TEST_CASE("document PDF adapter preserves table cell run styling") {
    const auto regular_font =
        make_temp_font_file("featherdoc-adapter-table-regular.ttf");
    const auto bold_font =
        make_temp_font_file("featherdoc-adapter-table-bold.ttf");

    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());
    CHECK(document.set_default_run_font_family("Unit Table"));

    auto style_definition = featherdoc::character_style_definition{};
    style_definition.name = "PDF Table Cell Accent";
    style_definition.run_text_color = std::string{"C00000"};
    style_definition.run_font_size_points = 16.0;
    REQUIRE(document.ensure_character_style("PdfTableCellAccent",
                                            style_definition));
    REQUIRE(document.materialize_style_run_properties("PdfTableCellAccent"));

    auto table = document.append_table(1U, 1U);
    REQUIRE(table.has_next());
    CHECK(table.set_width_twips(2880U));
    CHECK(table.set_column_width_twips(0U, 2880U));

    auto cell = table.find_cell(0U, 0U);
    REQUIRE(cell.has_value());
    auto paragraph = cell->paragraphs();
    REQUIRE(paragraph.has_next());
    auto strong_run = paragraph.add_run(
        "Strong ", featherdoc::formatting_flag::bold |
                       featherdoc::formatting_flag::underline);
    REQUIRE(strong_run.has_next());
    auto accent_run = paragraph.add_run("Accent");
    REQUIRE(accent_run.has_next());
    REQUIRE(document.set_run_style(accent_run, "PdfTableCellAccent"));

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.font_mappings = {
        featherdoc::pdf::PdfFontMapping{"Unit Table", regular_font},
        featherdoc::pdf::PdfFontMapping{"Unit Table", bold_font, true, false},
    };
    options.use_system_font_fallbacks = false;

    const auto layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);

    REQUIRE_EQ(layout.pages.size(), 1U);
    REQUIRE_EQ(layout.pages.front().rectangles.size(), 1U);
    REQUIRE_GE(layout.pages.front().text_runs.size(), 2U);

    const auto &cell_rectangle = layout.pages.front().rectangles.front();
    CHECK_GT(cell_rectangle.bounds.height_points, 24.0);

    const auto &strong_text = layout.pages.front().text_runs[0];
    CHECK_EQ(strong_text.text, "Strong ");
    CHECK_EQ(strong_text.font_family, "Unit Table");
    CHECK_EQ(strong_text.font_file_path, bold_font);
    CHECK(strong_text.bold);
    CHECK(strong_text.underline);

    const auto &accent_text = layout.pages.front().text_runs[1];
    CHECK_EQ(accent_text.text, "Accent");
    CHECK_EQ(accent_text.font_family, "Unit Table");
    CHECK_EQ(accent_text.font_file_path, regular_font);
    CHECK_EQ(accent_text.font_size_points, doctest::Approx(16.0));
    CHECK_EQ(accent_text.fill_color.red, doctest::Approx(192.0 / 255.0));
    CHECK_EQ(accent_text.fill_color.green, doctest::Approx(0.0));
    CHECK_EQ(accent_text.fill_color.blue, doctest::Approx(0.0));
    const auto row_top =
        cell_rectangle.bounds.y_points + cell_rectangle.bounds.height_points;
    CHECK(accent_text.baseline_origin.y_points ==
          doctest::Approx(row_top - 4.0 - 16.0));
}

TEST_CASE("document PDF adapter preserves table cell paragraph breaks") {
    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());

    auto table = document.append_table(1U, 1U);
    REQUIRE(table.has_next());
    CHECK(table.set_width_twips(2880U));
    CHECK(table.set_column_width_twips(0U, 2880U));

    auto cell = table.find_cell(0U, 0U);
    REQUIRE(cell.has_value());
    auto first_paragraph = cell->paragraphs();
    REQUIRE(first_paragraph.has_next());
    CHECK(first_paragraph.set_text("First"));
    auto second_paragraph = first_paragraph.insert_paragraph_after("Second");
    REQUIRE(second_paragraph.has_next());

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.use_system_font_fallbacks = false;
    options.line_height_points = 14.0;

    const auto layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);

    REQUIRE_EQ(layout.pages.size(), 1U);
    REQUIRE_EQ(layout.pages.front().rectangles.size(), 1U);
    REQUIRE_EQ(layout.pages.front().text_runs.size(), 2U);
    CHECK_EQ(layout.pages.front().text_runs[0].text, "First");
    CHECK_EQ(layout.pages.front().text_runs[1].text, "Second");

    const auto &first_run = layout.pages.front().text_runs[0];
    const auto &second_run = layout.pages.front().text_runs[1];
    CHECK(first_run.baseline_origin.y_points ==
          doctest::Approx(second_run.baseline_origin.y_points + 14.0));
    CHECK_EQ(layout.pages.front().rectangles.front().bounds.height_points,
             doctest::Approx(36.0));
}

TEST_CASE("document PDF adapter maps paragraph alignment and indentation") {
    const auto output_path = std::filesystem::current_path() /
                             "featherdoc-adapter-paragraph-layout.pdf";

    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());

    auto centered = document.paragraphs();
    REQUIRE(centered.has_next());
    CHECK(centered.set_text("Centered paragraph"));
    CHECK(centered.set_alignment(featherdoc::paragraph_alignment::center));

    auto right = append_body_paragraph(document, "Right paragraph");
    REQUIRE(right.has_next());
    CHECK(right.set_alignment(featherdoc::paragraph_alignment::right));

    auto hanging = append_body_paragraph(
        document,
        "Hanging indentation wraps across multiple lines with a visible "
        "second line offset.");
    REQUIRE(hanging.has_next());
    CHECK(hanging.set_indent_left_twips(720U));
    CHECK(hanging.set_hanging_indent_twips(360U));

    auto first_line = append_body_paragraph(
        document,
        "First line indentation demonstrates a visible inset only on the "
        "first rendered line.");
    REQUIRE(first_line.has_next());
    CHECK(first_line.set_indent_left_twips(360U));
    CHECK(first_line.set_first_line_indent_twips(360U));

    const auto centered_summary = document.inspect_paragraph(0U);
    REQUIRE(centered_summary.has_value());
    CHECK_EQ(centered_summary->alignment,
             std::optional<featherdoc::paragraph_alignment>{
                 featherdoc::paragraph_alignment::center});

    const auto hanging_summary = document.inspect_paragraph(2U);
    REQUIRE(hanging_summary.has_value());
    CHECK_EQ(hanging_summary->indent_left_twips.value_or(0U), 720U);
    CHECK_EQ(hanging_summary->hanging_indent_twips.value_or(0U), 360U);

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.use_system_font_fallbacks = false;
    options.page_size = featherdoc::pdf::PdfPageSize{240.0, 360.0};
    options.margin_left_points = 36.0;
    options.margin_right_points = 36.0;
    options.margin_top_points = 36.0;
    options.margin_bottom_points = 36.0;
    options.line_height_points = 14.0;

    const auto layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);

    REQUIRE_EQ(layout.pages.size(), 1U);
    const auto &text_runs = layout.pages.front().text_runs;
    REQUIRE_GE(text_runs.size(), 7U);

    const auto content_width = options.page_size.width_points -
                               options.margin_left_points -
                               options.margin_right_points;
    const auto centered_width =
        featherdoc::pdf::measure_text_width_points(
            "Centered paragraph", options.font_size_points, "Helvetica");
    const auto right_width = featherdoc::pdf::measure_text_width_points(
        "Right paragraph", options.font_size_points, "Helvetica");

    CHECK_EQ(text_runs[0].text, "Centered paragraph");
    CHECK(text_runs[0].baseline_origin.x_points ==
          doctest::Approx(options.margin_left_points +
                          (content_width - centered_width) / 2.0));
    CHECK_EQ(text_runs[1].text, "Right paragraph");
    CHECK(text_runs[1].baseline_origin.x_points ==
          doctest::Approx(options.margin_left_points + content_width -
                          right_width));

    auto hanging_index = std::size_t{0U};
    for (; hanging_index < text_runs.size(); ++hanging_index) {
        if (text_runs[hanging_index].text.find("Hanging") !=
            std::string::npos) {
            break;
        }
    }
    REQUIRE_LT(hanging_index + 1U, text_runs.size());
    CHECK(text_runs[hanging_index].baseline_origin.x_points ==
          doctest::Approx(options.margin_left_points + 18.0));
    CHECK(text_runs[hanging_index + 1U].baseline_origin.x_points ==
          doctest::Approx(options.margin_left_points + 36.0));

    auto first_line_index = std::size_t{0U};
    for (; first_line_index < text_runs.size(); ++first_line_index) {
        if (text_runs[first_line_index].text.find("First line") !=
            std::string::npos) {
            break;
        }
    }
    REQUIRE_LT(first_line_index + 1U, text_runs.size());
    CHECK(text_runs[first_line_index].baseline_origin.x_points ==
          doctest::Approx(options.margin_left_points + 36.0));
    CHECK(text_runs[first_line_index + 1U].baseline_origin.x_points ==
          doctest::Approx(options.margin_left_points + 18.0));

    featherdoc::pdf::PdfioGenerator generator;
    const auto write_result = generator.write(
        layout, output_path, featherdoc::pdf::PdfWriterOptions{});
    REQUIRE_MESSAGE(write_result.success, write_result.error_message);
}

TEST_CASE("document PDF adapter emits bullet list prefixes") {
    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());

    auto paragraph = document.paragraphs();
    REQUIRE(paragraph.has_next());
    CHECK(
        document.set_paragraph_list(paragraph, featherdoc::list_kind::bullet));
    CHECK(paragraph.add_run("Bullet item").has_next());

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.use_system_font_fallbacks = false;

    const auto layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);

    REQUIRE_EQ(layout.pages.size(), 1U);
    REQUIRE_GE(layout.pages.front().text_runs.size(), 1U);
    CHECK_EQ(collect_layout_text(layout),
             utf8_from_u8(u8"\u2022\tBullet item"));
}

TEST_CASE("document PDF adapter emits decimal list prefixes") {
    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());

    auto first = document.paragraphs();
    REQUIRE(first.has_next());
    CHECK(document.set_paragraph_list(first, featherdoc::list_kind::decimal));
    CHECK(first.add_run("First").has_next());

    auto second = first.insert_paragraph_after("");
    REQUIRE(second.has_next());
    CHECK(document.set_paragraph_list(second, featherdoc::list_kind::decimal));
    CHECK(second.add_run("Second").has_next());

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.use_system_font_fallbacks = false;

    const auto layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);

    REQUIRE_EQ(layout.pages.size(), 1U);
    REQUIRE_GE(layout.pages.front().text_runs.size(), 2U);
    CHECK_EQ(layout.pages.front().text_runs[0].text, "1.\tFirst");
    CHECK_EQ(layout.pages.front().text_runs[1].text, "2.\tSecond");
}

TEST_CASE("document PDF adapter emits nested decimal list prefixes") {
    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());

    auto first = document.paragraphs();
    REQUIRE(first.has_next());
    CHECK(document.set_paragraph_list(first, featherdoc::list_kind::decimal));
    CHECK(first.add_run("First").has_next());

    auto first_child = first.insert_paragraph_after("");
    REQUIRE(first_child.has_next());
    CHECK(document.set_paragraph_list(first_child,
                                      featherdoc::list_kind::decimal, 1U));
    CHECK(first_child.add_run("First child").has_next());

    auto second = first_child.insert_paragraph_after("");
    REQUIRE(second.has_next());
    CHECK(document.set_paragraph_list(second, featherdoc::list_kind::decimal));
    CHECK(second.add_run("Second").has_next());

    auto second_child = second.insert_paragraph_after("");
    REQUIRE(second_child.has_next());
    CHECK(document.set_paragraph_list(second_child,
                                      featherdoc::list_kind::decimal, 1U));
    CHECK(second_child.add_run("Second child").has_next());

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.use_system_font_fallbacks = false;

    const auto layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);

    REQUIRE_EQ(layout.pages.size(), 1U);
    REQUIRE_GE(layout.pages.front().text_runs.size(), 4U);
    CHECK_EQ(layout.pages.front().text_runs[0].text, "1.\tFirst");
    CHECK_EQ(layout.pages.front().text_runs[1].text, "1.1.\tFirst child");
    CHECK_EQ(layout.pages.front().text_runs[2].text, "2.\tSecond");
    CHECK_EQ(layout.pages.front().text_runs[3].text, "2.1.\tSecond child");
}

TEST_CASE("document PDF adapter honors custom decimal numbering patterns") {
    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());

    auto definition = featherdoc::numbering_definition{};
    definition.name = "PdfAdapterLegalOutline";
    definition.levels = {
        featherdoc::numbering_level_definition{featherdoc::list_kind::decimal,
                                               3U, 0U, "Article %1"},
        featherdoc::numbering_level_definition{featherdoc::list_kind::decimal,
                                               1U, 1U, "%1.%2)"},
    };

    const auto numbering_id = document.ensure_numbering_definition(definition);
    REQUIRE(numbering_id.has_value());

    auto first = document.paragraphs();
    REQUIRE(first.has_next());
    CHECK(document.set_paragraph_numbering(first, *numbering_id, 0U));
    CHECK(first.add_run("Scope").has_next());

    auto child = first.insert_paragraph_after("");
    REQUIRE(child.has_next());
    CHECK(document.set_paragraph_numbering(child, *numbering_id, 1U));
    CHECK(child.add_run("Details").has_next());

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.use_system_font_fallbacks = false;

    const auto layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);

    REQUIRE_EQ(layout.pages.size(), 1U);
    REQUIRE_GE(layout.pages.front().text_runs.size(), 2U);
    CHECK_EQ(layout.pages.front().text_runs[0].text, "Article 3\tScope");
    CHECK_EQ(layout.pages.front().text_runs[1].text, "3.1)\tDetails");
}

TEST_CASE("document PDF adapter emits default section header and footer text") {
    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());

    auto body = document.paragraphs();
    REQUIRE(body.has_next());
    CHECK(body.set_text("Body text"));

    auto &header = document.ensure_section_header_paragraphs(0U);
    REQUIRE(header.has_next());
    CHECK(header.set_text("Header text"));

    auto &footer = document.ensure_section_footer_paragraphs(0U);
    REQUIRE(footer.has_next());
    CHECK(footer.set_text("Footer text"));

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.use_system_font_fallbacks = false;
    options.render_headers_and_footers = true;
    options.header_footer_font_size_points = 9.0;

    const auto layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);

    REQUIRE_EQ(layout.pages.size(), 1U);

    bool found_body = false;
    bool found_header = false;
    bool found_footer = false;
    double body_y = 0.0;
    double header_y = 0.0;
    double footer_y = 0.0;
    double header_font_size = 0.0;
    double footer_font_size = 0.0;

    for (const auto &text_run : layout.pages.front().text_runs) {
        if (text_run.text == "Body text") {
            found_body = true;
            body_y = text_run.baseline_origin.y_points;
        } else if (text_run.text == "Header text") {
            found_header = true;
            header_y = text_run.baseline_origin.y_points;
            header_font_size = text_run.font_size_points;
        } else if (text_run.text == "Footer text") {
            found_footer = true;
            footer_y = text_run.baseline_origin.y_points;
            footer_font_size = text_run.font_size_points;
        }
    }

    CHECK(found_body);
    CHECK(found_header);
    CHECK(found_footer);
    CHECK(header_y > body_y);
    CHECK(footer_y < body_y);
    CHECK_EQ(header_y, doctest::Approx(options.page_size.height_points -
                                       options.header_y_offset_points));
    CHECK_EQ(footer_y, doctest::Approx(options.footer_y_offset_points));
    CHECK_EQ(header_font_size,
             doctest::Approx(options.header_footer_font_size_points));
    CHECK_EQ(footer_font_size,
             doctest::Approx(options.header_footer_font_size_points));
}

TEST_CASE("document PDF adapter expands header and footer page placeholders") {
    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());

    auto body = document.paragraphs();
    REQUIRE(body.has_next());
    CHECK(body.set_text("Placeholder body line 0"));
    for (auto index = 1U; index < 18U; ++index) {
        auto paragraph = append_body_paragraph(
            document, "Placeholder body line " + std::to_string(index));
        REQUIRE(paragraph.has_next());
    }

    auto &header = document.ensure_section_header_paragraphs(0U);
    REQUIRE(header.has_next());
    CHECK(header.set_text("Long header {{section_page}}/"
                          "{{section_total_pages}}"));

    auto &footer = document.ensure_section_footer_paragraphs(0U);
    REQUIRE(footer.has_next());
    CHECK(footer.set_text("Page {{page}} of {{total_pages}}"));

    featherdoc::section_page_setup setup{};
    setup.width_twips = 12240U;
    setup.height_twips = 4320U;
    setup.margins.top_twips = 720U;
    setup.margins.bottom_twips = 720U;
    setup.margins.left_twips = 1440U;
    setup.margins.right_twips = 1440U;
    setup.margins.header_twips = 240U;
    setup.margins.footer_twips = 240U;
    REQUIRE(document.set_section_page_setup(0U, setup));

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.use_system_font_fallbacks = false;
    options.render_headers_and_footers = true;
    options.expand_header_footer_page_placeholders = true;
    options.font_size_points = 12.0;
    options.line_height_points = 16.0;
    options.header_footer_font_size_points = 8.0;

    const auto layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);
    REQUIRE_GE(layout.pages.size(), 2U);

    const auto total_pages = std::to_string(layout.pages.size());
    const auto first_page_text = collect_layout_text(
        featherdoc::pdf::PdfDocumentLayout{{}, {layout.pages.front()}});
    const auto last_page_text = collect_layout_text(
        featherdoc::pdf::PdfDocumentLayout{{}, {layout.pages.back()}});
    const auto all_text = collect_layout_text(layout);

    CHECK_NE(first_page_text.find("Long header 1/" + total_pages),
             std::string::npos);
    CHECK_NE(first_page_text.find("Page 1 of " + total_pages),
             std::string::npos);
    CHECK_NE(last_page_text.find("Long header " + total_pages + "/" +
                                 total_pages),
             std::string::npos);
    CHECK_NE(last_page_text.find("Page " + total_pages + " of " +
                                 total_pages),
             std::string::npos);
    CHECK_EQ(all_text.find("{{page}}"), std::string::npos);
    CHECK_EQ(all_text.find("{{total_pages}}"), std::string::npos);
}

TEST_CASE("document PDF adapter applies first section page setup") {
    const auto output_path = std::filesystem::current_path() /
                             "featherdoc-adapter-section-page-setup.pdf";

    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());

    auto body = document.paragraphs();
    REQUIRE(body.has_next());
    CHECK(body.set_text("Section page setup body"));

    auto &header = document.ensure_section_header_paragraphs(0U);
    REQUIRE(header.has_next());
    CHECK(header.set_text("Section setup header"));

    auto &footer = document.ensure_section_footer_paragraphs(0U);
    REQUIRE(footer.has_next());
    CHECK(footer.set_text("Section setup footer"));

    featherdoc::section_page_setup setup{};
    setup.orientation = featherdoc::page_orientation::landscape;
    setup.width_twips = 15840U;
    setup.height_twips = 12240U;
    setup.margins.top_twips = 1440U;
    setup.margins.bottom_twips = 720U;
    setup.margins.left_twips = 1800U;
    setup.margins.right_twips = 900U;
    setup.margins.header_twips = 360U;
    setup.margins.footer_twips = 420U;
    REQUIRE(document.set_section_page_setup(0U, setup));

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.use_system_font_fallbacks = false;
    options.render_headers_and_footers = true;
    options.font_size_points = 12.0;
    options.line_height_points = 16.0;

    const auto layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);

    REQUIRE_EQ(layout.pages.size(), 1U);
    const auto &page = layout.pages.front();
    CHECK_EQ(page.size.width_points, doctest::Approx(792.0));
    CHECK_EQ(page.size.height_points, doctest::Approx(612.0));

    bool found_body = false;
    bool found_header = false;
    bool found_footer = false;
    double body_x = 0.0;
    double body_y = 0.0;
    double header_y = 0.0;
    double footer_y = 0.0;
    for (const auto &text_run : page.text_runs) {
        if (text_run.text == "Section page setup body") {
            found_body = true;
            body_x = text_run.baseline_origin.x_points;
            body_y = text_run.baseline_origin.y_points;
        } else if (text_run.text == "Section setup header") {
            found_header = true;
            header_y = text_run.baseline_origin.y_points;
        } else if (text_run.text == "Section setup footer") {
            found_footer = true;
            footer_y = text_run.baseline_origin.y_points;
        }
    }

    CHECK(found_body);
    CHECK(found_header);
    CHECK(found_footer);
    CHECK_EQ(body_x, doctest::Approx(90.0));
    CHECK_EQ(body_y, doctest::Approx(528.0));
    CHECK_EQ(header_y, doctest::Approx(594.0));
    CHECK_EQ(footer_y, doctest::Approx(21.0));

#if defined(FEATHERDOC_BUILD_PDF_IMPORT)
    featherdoc::pdf::PdfioGenerator generator;
    const auto write_result = generator.write(
        layout, output_path, featherdoc::pdf::PdfWriterOptions{});
    REQUIRE_MESSAGE(write_result.success, write_result.error_message);

    featherdoc::pdf::PdfiumParser parser;
    const auto parse_result = parser.parse(output_path, {});
    REQUIRE_MESSAGE(parse_result.success, parse_result.error_message);
    REQUIRE_EQ(parse_result.document.pages.size(), 1U);
    CHECK_EQ(parse_result.document.pages.front().size.width_points,
             doctest::Approx(792.0));
    CHECK_EQ(parse_result.document.pages.front().size.height_points,
             doctest::Approx(612.0));

    const auto extracted_text = collect_text(parse_result.document);
    CHECK_NE(extracted_text.find("Section setup header"), std::string::npos);
    CHECK_NE(extracted_text.find("Section page setup body"), std::string::npos);
    CHECK_NE(extracted_text.find("Section setup footer"), std::string::npos);
#endif
}

TEST_CASE("document PDF adapter switches page setup at section boundaries") {
    const auto output_path = std::filesystem::current_path() /
                             "featherdoc-adapter-multi-section-page-setup.pdf";

    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());

    auto first = document.paragraphs();
    REQUIRE(first.has_next());
    CHECK(first.set_text("Section one body"));

    auto &section0_header = document.ensure_section_header_paragraphs(0U);
    REQUIRE(section0_header.has_next());
    CHECK(section0_header.set_text("Section one header"));

    CHECK(document.append_section(false));
    REQUIRE_EQ(document.section_count(), 2U);

    auto second = append_body_paragraph(document, "Section two body");
    REQUIRE(second.has_next());

    auto &section1_header = document.ensure_section_header_paragraphs(1U);
    REQUIRE(section1_header.has_next());
    CHECK(section1_header.set_text("Section two header"));

    featherdoc::section_page_setup first_setup{};
    first_setup.orientation = featherdoc::page_orientation::portrait;
    first_setup.width_twips = 12240U;
    first_setup.height_twips = 15840U;
    first_setup.margins.top_twips = 1440U;
    first_setup.margins.bottom_twips = 1440U;
    first_setup.margins.left_twips = 1440U;
    first_setup.margins.right_twips = 1440U;
    first_setup.margins.header_twips = 360U;
    first_setup.margins.footer_twips = 360U;

    featherdoc::section_page_setup second_setup{};
    second_setup.orientation = featherdoc::page_orientation::landscape;
    second_setup.width_twips = 15840U;
    second_setup.height_twips = 12240U;
    second_setup.margins.top_twips = 720U;
    second_setup.margins.bottom_twips = 720U;
    second_setup.margins.left_twips = 1800U;
    second_setup.margins.right_twips = 900U;
    second_setup.margins.header_twips = 300U;
    second_setup.margins.footer_twips = 300U;
    REQUIRE(document.set_section_page_setup(0U, first_setup));
    REQUIRE(document.set_section_page_setup(1U, second_setup));

    const auto body_blocks = document.inspect_body_blocks();
    REQUIRE_GE(body_blocks.size(), 3U);
    CHECK_EQ(body_blocks.front().section_index, 0U);
    CHECK_EQ(body_blocks.back().section_index, 1U);

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.use_system_font_fallbacks = false;
    options.render_headers_and_footers = true;
    options.font_size_points = 12.0;
    options.line_height_points = 16.0;

    const auto layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);

    REQUIRE_GE(layout.pages.size(), 2U);
    CHECK_EQ(layout.pages[0].section_index, 0U);
    CHECK_EQ(layout.pages[0].size.width_points, doctest::Approx(612.0));
    CHECK_EQ(layout.pages[0].size.height_points, doctest::Approx(792.0));
    CHECK_EQ(layout.pages[1].section_index, 1U);
    CHECK_EQ(layout.pages[1].size.width_points, doctest::Approx(792.0));
    CHECK_EQ(layout.pages[1].size.height_points, doctest::Approx(612.0));

    const auto first_page_text = collect_layout_text(
        featherdoc::pdf::PdfDocumentLayout{{}, {layout.pages[0]}});
    const auto second_page_text = collect_layout_text(
        featherdoc::pdf::PdfDocumentLayout{{}, {layout.pages[1]}});
    CHECK_NE(first_page_text.find("Section one body"), std::string::npos);
    CHECK_NE(first_page_text.find("Section one header"), std::string::npos);
    CHECK_EQ(first_page_text.find("Section two body"), std::string::npos);
    CHECK_NE(second_page_text.find("Section two body"), std::string::npos);
    CHECK_NE(second_page_text.find("Section two header"), std::string::npos);
    CHECK_EQ(second_page_text.find("Section one header"), std::string::npos);

#if defined(FEATHERDOC_BUILD_PDF_IMPORT)
    featherdoc::pdf::PdfioGenerator generator;
    const auto write_result = generator.write(
        layout, output_path, featherdoc::pdf::PdfWriterOptions{});
    REQUIRE_MESSAGE(write_result.success, write_result.error_message);

    featherdoc::pdf::PdfiumParser parser;
    const auto parse_result = parser.parse(output_path, {});
    REQUIRE_MESSAGE(parse_result.success, parse_result.error_message);
    REQUIRE_GE(parse_result.document.pages.size(), 2U);
    CHECK_EQ(parse_result.document.pages[0].size.width_points,
             doctest::Approx(612.0));
    CHECK_EQ(parse_result.document.pages[0].size.height_points,
             doctest::Approx(792.0));
    CHECK_EQ(parse_result.document.pages[1].size.width_points,
             doctest::Approx(792.0));
    CHECK_EQ(parse_result.document.pages[1].size.height_points,
             doctest::Approx(612.0));

    const auto extracted_text = collect_text(parse_result.document);
    CHECK_NE(extracted_text.find("Section one body"), std::string::npos);
    CHECK_NE(extracted_text.find("Section two body"), std::string::npos);
#endif
}

TEST_CASE("document PDF adapter renders first page section header and footer") {
    const auto output_path = std::filesystem::current_path() /
                             "featherdoc-adapter-first-page-header-footer.pdf";

    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());

    auto first_body = document.paragraphs();
    REQUIRE(first_body.has_next());
    CHECK(first_body.set_text("First body line 0"));
    for (auto index = 1U; index < 18U; ++index) {
        auto paragraph = append_body_paragraph(
            document, "First body line " + std::to_string(index));
        REQUIRE(paragraph.has_next());
    }

    auto &default_header = document.ensure_section_header_paragraphs(0U);
    REQUIRE(default_header.has_next());
    CHECK(default_header.set_text("Default section header"));

    auto &default_footer = document.ensure_section_footer_paragraphs(0U);
    REQUIRE(default_footer.has_next());
    CHECK(default_footer.set_text("Default section footer"));

    auto &first_header = document.ensure_section_header_paragraphs(
        0U, featherdoc::section_reference_kind::first_page);
    REQUIRE(first_header.has_next());
    CHECK(first_header.set_text("First page section header"));

    auto &first_footer = document.ensure_section_footer_paragraphs(
        0U, featherdoc::section_reference_kind::first_page);
    REQUIRE(first_footer.has_next());
    CHECK(first_footer.set_text("First page section footer"));

    const auto section = document.inspect_section(0U);
    REQUIRE(section.has_value());
    CHECK(section->different_first_page_enabled);

    featherdoc::section_page_setup setup{};
    setup.width_twips = 12240U;
    setup.height_twips = 4320U;
    setup.margins.top_twips = 720U;
    setup.margins.bottom_twips = 720U;
    setup.margins.left_twips = 1440U;
    setup.margins.right_twips = 1440U;
    setup.margins.header_twips = 240U;
    setup.margins.footer_twips = 240U;
    REQUIRE(document.set_section_page_setup(0U, setup));

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.use_system_font_fallbacks = false;
    options.render_headers_and_footers = true;
    options.font_size_points = 12.0;
    options.line_height_points = 16.0;
    options.header_footer_font_size_points = 8.0;

    const auto layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);

    REQUIRE_GE(layout.pages.size(), 2U);
    CHECK_EQ(layout.pages[0].section_index, 0U);
    CHECK_EQ(layout.pages[0].section_page_index, 0U);
    CHECK_EQ(layout.pages[1].section_index, 0U);
    CHECK_EQ(layout.pages[1].section_page_index, 1U);

    const auto first_page_text = collect_layout_text(
        featherdoc::pdf::PdfDocumentLayout{{}, {layout.pages[0]}});
    const auto second_page_text = collect_layout_text(
        featherdoc::pdf::PdfDocumentLayout{{}, {layout.pages[1]}});
    CHECK_NE(first_page_text.find("First page section header"),
             std::string::npos);
    CHECK_NE(first_page_text.find("First page section footer"),
             std::string::npos);
    CHECK_EQ(first_page_text.find("Default section header"), std::string::npos);
    CHECK_EQ(first_page_text.find("Default section footer"), std::string::npos);
    CHECK_NE(second_page_text.find("Default section header"),
             std::string::npos);
    CHECK_NE(second_page_text.find("Default section footer"),
             std::string::npos);
    CHECK_EQ(second_page_text.find("First page section header"),
             std::string::npos);
    CHECK_EQ(second_page_text.find("First page section footer"),
             std::string::npos);

    featherdoc::pdf::PdfioGenerator generator;
    const auto write_result = generator.write(
        layout, output_path, featherdoc::pdf::PdfWriterOptions{});
    REQUIRE_MESSAGE(write_result.success, write_result.error_message);

#if defined(FEATHERDOC_BUILD_PDF_IMPORT)
    featherdoc::pdf::PdfiumParser parser;
    const auto parse_result = parser.parse(output_path, {});
    REQUIRE_MESSAGE(parse_result.success, parse_result.error_message);
    REQUIRE_GE(parse_result.document.pages.size(), 2U);

    auto parsed_first_page_text = std::string{};
    for (const auto &text_span : parse_result.document.pages[0].text_spans) {
        parsed_first_page_text += text_span.text;
    }
    auto parsed_second_page_text = std::string{};
    for (const auto &text_span : parse_result.document.pages[1].text_spans) {
        parsed_second_page_text += text_span.text;
    }

    CHECK_NE(parsed_first_page_text.find("First page section header"),
             std::string::npos);
    CHECK_EQ(parsed_first_page_text.find("Default section header"),
             std::string::npos);
    CHECK_NE(parsed_second_page_text.find("Default section header"),
             std::string::npos);
    CHECK_EQ(parsed_second_page_text.find("First page section header"),
             std::string::npos);
#endif
}

TEST_CASE("document PDF adapter renders even page section header and footer") {
    const auto output_path = std::filesystem::current_path() /
                             "featherdoc-adapter-even-page-header-footer.pdf";

    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());

    auto first_body = document.paragraphs();
    REQUIRE(first_body.has_next());
    CHECK(first_body.set_text("Even body line 0"));
    for (auto index = 1U; index < 18U; ++index) {
        auto paragraph = append_body_paragraph(
            document, "Even body line " + std::to_string(index));
        REQUIRE(paragraph.has_next());
    }

    auto &default_header = document.ensure_section_header_paragraphs(0U);
    REQUIRE(default_header.has_next());
    CHECK(default_header.set_text("Odd default section header"));

    auto &default_footer = document.ensure_section_footer_paragraphs(0U);
    REQUIRE(default_footer.has_next());
    CHECK(default_footer.set_text("Odd default section footer"));

    auto &even_header = document.ensure_section_header_paragraphs(
        0U, featherdoc::section_reference_kind::even_page);
    REQUIRE(even_header.has_next());
    CHECK(even_header.set_text("Even page section header"));

    auto &even_footer = document.ensure_section_footer_paragraphs(
        0U, featherdoc::section_reference_kind::even_page);
    REQUIRE(even_footer.has_next());
    CHECK(even_footer.set_text("Even page section footer"));

    const auto section = document.inspect_section(0U);
    REQUIRE(section.has_value());
    REQUIRE(section->even_and_odd_headers_enabled.has_value());
    CHECK(*section->even_and_odd_headers_enabled);

    featherdoc::section_page_setup setup{};
    setup.width_twips = 12240U;
    setup.height_twips = 4320U;
    setup.margins.top_twips = 720U;
    setup.margins.bottom_twips = 720U;
    setup.margins.left_twips = 1440U;
    setup.margins.right_twips = 1440U;
    setup.margins.header_twips = 240U;
    setup.margins.footer_twips = 240U;
    REQUIRE(document.set_section_page_setup(0U, setup));

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.use_system_font_fallbacks = false;
    options.render_headers_and_footers = true;
    options.font_size_points = 12.0;
    options.line_height_points = 16.0;
    options.header_footer_font_size_points = 8.0;

    const auto layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);

    REQUIRE_GE(layout.pages.size(), 3U);
    CHECK_EQ(layout.pages[0].section_page_index, 0U);
    CHECK_EQ(layout.pages[1].section_page_index, 1U);
    CHECK_EQ(layout.pages[2].section_page_index, 2U);

    const auto first_page_text = collect_layout_text(
        featherdoc::pdf::PdfDocumentLayout{{}, {layout.pages[0]}});
    const auto second_page_text = collect_layout_text(
        featherdoc::pdf::PdfDocumentLayout{{}, {layout.pages[1]}});
    const auto third_page_text = collect_layout_text(
        featherdoc::pdf::PdfDocumentLayout{{}, {layout.pages[2]}});
    CHECK_NE(first_page_text.find("Odd default section header"),
             std::string::npos);
    CHECK_NE(first_page_text.find("Odd default section footer"),
             std::string::npos);
    CHECK_EQ(first_page_text.find("Even page section header"),
             std::string::npos);
    CHECK_NE(second_page_text.find("Even page section header"),
             std::string::npos);
    CHECK_NE(second_page_text.find("Even page section footer"),
             std::string::npos);
    CHECK_EQ(second_page_text.find("Odd default section header"),
             std::string::npos);
    CHECK_NE(third_page_text.find("Odd default section header"),
             std::string::npos);
    CHECK_NE(third_page_text.find("Odd default section footer"),
             std::string::npos);
    CHECK_EQ(third_page_text.find("Even page section header"),
             std::string::npos);

    featherdoc::pdf::PdfioGenerator generator;
    const auto write_result = generator.write(
        layout, output_path, featherdoc::pdf::PdfWriterOptions{});
    REQUIRE_MESSAGE(write_result.success, write_result.error_message);

#if defined(FEATHERDOC_BUILD_PDF_IMPORT)
    featherdoc::pdf::PdfiumParser parser;
    const auto parse_result = parser.parse(output_path, {});
    REQUIRE_MESSAGE(parse_result.success, parse_result.error_message);
    REQUIRE_GE(parse_result.document.pages.size(), 3U);

    const auto parsed_first_page_text =
        collect_text(parse_result.document.pages[0]);
    const auto parsed_second_page_text =
        collect_text(parse_result.document.pages[1]);
    const auto parsed_third_page_text =
        collect_text(parse_result.document.pages[2]);

    CHECK_NE(parsed_first_page_text.find("Odd default section header"),
             std::string::npos);
    CHECK_EQ(parsed_first_page_text.find("Even page section header"),
             std::string::npos);
    CHECK_NE(parsed_second_page_text.find("Even page section header"),
             std::string::npos);
    CHECK_EQ(parsed_second_page_text.find("Odd default section header"),
             std::string::npos);
    CHECK_NE(parsed_third_page_text.find("Odd default section header"),
             std::string::npos);
    CHECK_EQ(parsed_third_page_text.find("Even page section header"),
             std::string::npos);
#endif
}

TEST_CASE("document PDF adapter resolves linked section headers and footers") {
    const auto output_path = std::filesystem::current_path() /
                             "featherdoc-adapter-linked-section-header-footer."
                             "pdf";

    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());

    auto first_body = document.paragraphs();
    REQUIRE(first_body.has_next());
    CHECK(first_body.set_text("Linked section one body"));

    auto &default_header = document.ensure_section_header_paragraphs(0U);
    REQUIRE(default_header.has_next());
    CHECK(default_header.set_text("Linked inherited default header"));

    auto &default_footer = document.ensure_section_footer_paragraphs(0U);
    REQUIRE(default_footer.has_next());
    CHECK(default_footer.set_text("Linked inherited default footer"));

    auto &even_header = document.ensure_section_header_paragraphs(
        0U, featherdoc::section_reference_kind::even_page);
    REQUIRE(even_header.has_next());
    CHECK(even_header.set_text("Linked inherited even header"));

    auto &even_footer = document.ensure_section_footer_paragraphs(
        0U, featherdoc::section_reference_kind::even_page);
    REQUIRE(even_footer.has_next());
    CHECK(even_footer.set_text("Linked inherited even footer"));

    CHECK(document.append_section(false));
    REQUIRE_EQ(document.section_count(), 2U);

    for (auto index = 0U; index < 18U; ++index) {
        auto paragraph = append_body_paragraph(
            document, "Linked section two body line " + std::to_string(index));
        REQUIRE(paragraph.has_next());
    }

    featherdoc::section_page_setup setup{};
    setup.width_twips = 12240U;
    setup.height_twips = 4320U;
    setup.margins.top_twips = 720U;
    setup.margins.bottom_twips = 720U;
    setup.margins.left_twips = 1440U;
    setup.margins.right_twips = 1440U;
    setup.margins.header_twips = 240U;
    setup.margins.footer_twips = 240U;
    REQUIRE(document.set_section_page_setup(0U, setup));
    REQUIRE(document.set_section_page_setup(1U, setup));

    const auto section = document.inspect_section(1U);
    REQUIRE(section.has_value());
    CHECK_FALSE(section->header.has_default);
    CHECK_FALSE(section->header.has_even);
    CHECK(section->header.default_linked_to_previous);
    CHECK(section->header.even_linked_to_previous);
    REQUIRE(section->header.resolved_default_section_index.has_value());
    CHECK_EQ(*section->header.resolved_default_section_index, 0U);
    REQUIRE(section->header.resolved_even_section_index.has_value());
    CHECK_EQ(*section->header.resolved_even_section_index, 0U);
    CHECK(section->footer.default_linked_to_previous);
    CHECK(section->footer.even_linked_to_previous);
    REQUIRE(section->footer.resolved_default_section_index.has_value());
    CHECK_EQ(*section->footer.resolved_default_section_index, 0U);
    REQUIRE(section->footer.resolved_even_section_index.has_value());
    CHECK_EQ(*section->footer.resolved_even_section_index, 0U);

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.use_system_font_fallbacks = false;
    options.render_headers_and_footers = true;
    options.font_size_points = 12.0;
    options.line_height_points = 16.0;
    options.header_footer_font_size_points = 8.0;

    const auto layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);

    REQUIRE_GE(layout.pages.size(), 3U);
    CHECK_EQ(layout.pages[0].section_index, 0U);
    CHECK_EQ(layout.pages[1].section_index, 1U);
    CHECK_EQ(layout.pages[2].section_index, 1U);

    const auto first_page_text = collect_layout_text(
        featherdoc::pdf::PdfDocumentLayout{{}, {layout.pages[0]}});
    const auto second_page_text = collect_layout_text(
        featherdoc::pdf::PdfDocumentLayout{{}, {layout.pages[1]}});
    const auto third_page_text = collect_layout_text(
        featherdoc::pdf::PdfDocumentLayout{{}, {layout.pages[2]}});
    CHECK_NE(first_page_text.find("Linked inherited default header"),
             std::string::npos);
    CHECK_NE(second_page_text.find("Linked inherited even header"),
             std::string::npos);
    CHECK_NE(second_page_text.find("Linked inherited even footer"),
             std::string::npos);
    CHECK_EQ(second_page_text.find("Linked inherited default header"),
             std::string::npos);
    CHECK_NE(third_page_text.find("Linked inherited default header"),
             std::string::npos);
    CHECK_NE(third_page_text.find("Linked inherited default footer"),
             std::string::npos);
    CHECK_EQ(third_page_text.find("Linked inherited even header"),
             std::string::npos);

    featherdoc::pdf::PdfioGenerator generator;
    const auto write_result = generator.write(
        layout, output_path, featherdoc::pdf::PdfWriterOptions{});
    REQUIRE_MESSAGE(write_result.success, write_result.error_message);

#if defined(FEATHERDOC_BUILD_PDF_IMPORT)
    featherdoc::pdf::PdfiumParser parser;
    const auto parse_result = parser.parse(output_path, {});
    REQUIRE_MESSAGE(parse_result.success, parse_result.error_message);
    REQUIRE_GE(parse_result.document.pages.size(), 3U);

    const auto parsed_second_page_text =
        collect_text(parse_result.document.pages[1]);
    const auto parsed_third_page_text =
        collect_text(parse_result.document.pages[2]);

    CHECK_NE(parsed_second_page_text.find("Linked inherited even header"),
             std::string::npos);
    CHECK_EQ(parsed_second_page_text.find("Linked inherited default header"),
             std::string::npos);
    CHECK_NE(parsed_third_page_text.find("Linked inherited default header"),
             std::string::npos);
    CHECK_EQ(parsed_third_page_text.find("Linked inherited even header"),
             std::string::npos);
#endif
}

#if defined(FEATHERDOC_BUILD_PDF_IMPORT)
TEST_CASE("document PDF adapter header and footer output round-trips through "
          "PDFium") {
    const auto output_path = std::filesystem::current_path() /
                             "featherdoc-adapter-header-footer-roundtrip.pdf";

    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());

    auto body = document.paragraphs();
    REQUIRE(body.has_next());
    CHECK(body.set_text("Roundtrip body text"));

    auto &header = document.ensure_section_header_paragraphs(0U);
    REQUIRE(header.has_next());
    CHECK(header.set_text("Roundtrip header text"));

    auto &footer = document.ensure_section_footer_paragraphs(0U);
    REQUIRE(footer.has_next());
    CHECK(footer.set_text("Roundtrip footer text"));

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.use_system_font_fallbacks = false;
    options.render_headers_and_footers = true;

    const auto layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);

    featherdoc::pdf::PdfioGenerator generator;
    const auto write_result = generator.write(
        layout, output_path, featherdoc::pdf::PdfWriterOptions{});
    REQUIRE_MESSAGE(write_result.success, write_result.error_message);

    featherdoc::pdf::PdfiumParser parser;
    const auto parse_result = parser.parse(output_path, {});
    REQUIRE_MESSAGE(parse_result.success, parse_result.error_message);

    const auto extracted_text = collect_text(parse_result.document);
    CHECK_NE(extracted_text.find("Roundtrip header text"), std::string::npos);
    CHECK_NE(extracted_text.find("Roundtrip body text"), std::string::npos);
    CHECK_NE(extracted_text.find("Roundtrip footer text"), std::string::npos);
}
#endif

TEST_CASE("PDF writer accepts standard font style variants and underlines") {
    const auto output_path =
        std::filesystem::current_path() / "featherdoc-styled-base-fonts.pdf";

    featherdoc::pdf::PdfDocumentLayout layout;
    layout.metadata.title = "FeatherDoc underlined text writer test";
    layout.metadata.creator = "FeatherDoc tests";

    featherdoc::pdf::PdfPageLayout page;
    page.size = featherdoc::pdf::PdfPageSize{};
    page.text_runs.push_back(featherdoc::pdf::PdfTextRun{
        featherdoc::pdf::PdfPoint{72.0, 720.0},
        "Underlined PDF text",
        "Helvetica",
        {},
        14.0,
        featherdoc::pdf::PdfRgbColor{0.1, 0.2, 0.3},
        false,
        false,
        true,
        false,
    });
    page.text_runs.push_back(featherdoc::pdf::PdfTextRun{
        featherdoc::pdf::PdfPoint{72.0, 690.0},
        "Bold italic PDF text",
        "Helvetica",
        {},
        14.0,
        featherdoc::pdf::PdfRgbColor{0.0, 0.0, 0.0},
        true,
        true,
        false,
        false,
    });
    const auto latin_font = first_existing_path(candidate_latin_fonts());
    if (!latin_font.empty()) {
        page.text_runs.push_back(featherdoc::pdf::PdfTextRun{
            featherdoc::pdf::PdfPoint{72.0, 660.0},
            "Synthetic file font style",
            "Unit File Font",
            latin_font,
            14.0,
            featherdoc::pdf::PdfRgbColor{0.0, 0.0, 0.0},
            true,
            true,
            false,
            false,
            0.0,
            true,
            true,
        });
    }
    layout.pages.push_back(std::move(page));

    featherdoc::pdf::PdfioGenerator generator;
    const auto write_result = generator.write(
        layout, output_path, featherdoc::pdf::PdfWriterOptions{});
    REQUIRE_MESSAGE(write_result.success, write_result.error_message);
    CHECK_GT(write_result.bytes_written, 0U);

#if defined(FEATHERDOC_BUILD_PDF_IMPORT)
    featherdoc::pdf::PdfiumParser parser;
    const auto parse_result = parser.parse(output_path, {});
    REQUIRE_MESSAGE(parse_result.success, parse_result.error_message);

    const auto extracted_text = collect_text(parse_result.document);
    CHECK_NE(extracted_text.find("Underlined PDF text"), std::string::npos);
    CHECK_NE(extracted_text.find("Bold italic PDF text"), std::string::npos);
    if (!latin_font.empty()) {
        CHECK_NE(extracted_text.find("Synthetic file font style"),
                 std::string::npos);
    }
#endif
}

#if defined(FEATHERDOC_BUILD_PDF_IMPORT)
TEST_CASE("PDF writer preserves per-page layout sizes") {
    const auto output_path =
        std::filesystem::current_path() / "featherdoc-mixed-page-sizes.pdf";

    featherdoc::pdf::PdfDocumentLayout layout;
    layout.metadata.title = "FeatherDoc mixed page size writer test";
    layout.metadata.creator = "FeatherDoc tests";

    featherdoc::pdf::PdfPageLayout portrait_page;
    portrait_page.size = featherdoc::pdf::PdfPageSize::letter_portrait();
    portrait_page.text_runs.push_back(featherdoc::pdf::PdfTextRun{
        featherdoc::pdf::PdfPoint{72.0, 720.0},
        "Portrait page size",
        "Helvetica",
        {},
        14.0,
        featherdoc::pdf::PdfRgbColor{0.0, 0.0, 0.0},
        false,
        false,
        false,
        false,
    });
    layout.pages.push_back(std::move(portrait_page));

    featherdoc::pdf::PdfPageLayout landscape_page;
    landscape_page.size = featherdoc::pdf::PdfPageSize{792.0, 612.0};
    landscape_page.text_runs.push_back(featherdoc::pdf::PdfTextRun{
        featherdoc::pdf::PdfPoint{72.0, 540.0},
        "Landscape page size",
        "Helvetica",
        {},
        14.0,
        featherdoc::pdf::PdfRgbColor{0.0, 0.0, 0.0},
        false,
        false,
        false,
        false,
    });
    layout.pages.push_back(std::move(landscape_page));

    featherdoc::pdf::PdfioGenerator generator;
    const auto write_result = generator.write(
        layout, output_path, featherdoc::pdf::PdfWriterOptions{});
    REQUIRE_MESSAGE(write_result.success, write_result.error_message);

    featherdoc::pdf::PdfiumParser parser;
    const auto parse_result = parser.parse(output_path, {});
    REQUIRE_MESSAGE(parse_result.success, parse_result.error_message);
    REQUIRE_EQ(parse_result.document.pages.size(), 2U);

    CHECK_EQ(parse_result.document.pages[0].size.width_points,
             doctest::Approx(612.0));
    CHECK_EQ(parse_result.document.pages[0].size.height_points,
             doctest::Approx(792.0));
    CHECK_EQ(parse_result.document.pages[1].size.width_points,
             doctest::Approx(792.0));
    CHECK_EQ(parse_result.document.pages[1].size.height_points,
             doctest::Approx(612.0));

    const auto extracted_text = collect_text(parse_result.document);
    CHECK_NE(extracted_text.find("Portrait page size"), std::string::npos);
    CHECK_NE(extracted_text.find("Landscape page size"), std::string::npos);
}

TEST_CASE("document PDF adapter output round-trips CJK text through PDFium") {
    const auto latin_font = first_existing_path(candidate_latin_fonts());
    const auto cjk_font = first_existing_path(candidate_cjk_fonts());
    if (latin_font.empty() || cjk_font.empty()) {
        MESSAGE(
            "skipping adapter PDF roundtrip: configure test Latin/CJK fonts");
        return;
    }

    const auto cjk_text = utf8_from_u8(u8"中文合同测试");
    const auto output_path = std::filesystem::current_path() /
                             "featherdoc-adapter-cjk-roundtrip.pdf";

    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());
    CHECK(document.set_default_run_font_family("Unit Latin"));
    CHECK(document.set_default_run_east_asia_font_family("Unit CJK"));

    auto paragraph = document.paragraphs();
    REQUIRE(paragraph.has_next());
    REQUIRE(paragraph.add_run("Contract ABC ").has_next());
    REQUIRE(paragraph.add_run(cjk_text).has_next());

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.metadata.title = "FeatherDoc adapter CJK roundtrip";
    options.font_mappings = {
        featherdoc::pdf::PdfFontMapping{"Unit Latin", latin_font},
        featherdoc::pdf::PdfFontMapping{"Unit CJK", cjk_font},
    };
    options.use_system_font_fallbacks = false;

    const auto layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);

    featherdoc::pdf::PdfioGenerator generator;
    const auto write_result = generator.write(
        layout, output_path, featherdoc::pdf::PdfWriterOptions{});
    REQUIRE_MESSAGE(write_result.success, write_result.error_message);

    const auto pdf_bytes = read_binary_file(output_path);
    CHECK_NE(pdf_bytes.find("/Font<</F1"), std::string::npos);
    CHECK_NE(pdf_bytes.find("/F2 "), std::string::npos);

    featherdoc::pdf::PdfiumParser parser;
    const auto parse_result = parser.parse(output_path, {});
    REQUIRE_MESSAGE(parse_result.success, parse_result.error_message);

    const auto extracted_text = collect_text(parse_result.document);
    CHECK_NE(extracted_text.find("Contract ABC "), std::string::npos);
    CHECK_NE(extracted_text.find(cjk_text), std::string::npos);
}
#endif

TEST_CASE("document PDF adapter emits inline images as PDF image blocks") {
    const auto image_path =
        std::filesystem::current_path() / "featherdoc-adapter-inline-image.png";
    const auto output_path =
        std::filesystem::current_path() / "featherdoc-adapter-inline-image.pdf";
    write_binary_file(image_path, tiny_png_data());

    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());
    auto paragraph = document.paragraphs();
    REQUIRE(paragraph.has_next());
    CHECK(paragraph.set_text("Image intro"));
    REQUIRE(document.append_image(image_path, 20U, 10U));

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.use_system_font_fallbacks = false;
    options.render_inline_images = true;

    const auto layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);

    REQUIRE_EQ(layout.pages.size(), 1U);
    REQUIRE_EQ(layout.pages.front().images.size(), 1U);

    const auto &image = layout.pages.front().images.front();
    CHECK_EQ(image.content_type, "image/png");
    CHECK_EQ(image.bounds.x_points,
             doctest::Approx(options.margin_left_points));
    CHECK_EQ(image.bounds.width_points, doctest::Approx(15.0));
    CHECK_EQ(image.bounds.height_points, doctest::Approx(7.5));
    CHECK(image.cleanup_source_after_write);
    CHECK(std::filesystem::exists(image.source_path));

    const auto images = document.inline_images();
    REQUIRE_EQ(images.size(), 1U);
    REQUIRE(images.front().paragraph_index.has_value());
    CHECK_EQ(*images.front().paragraph_index, 1U);

    REQUIRE_GE(layout.pages.front().text_runs.size(), 1U);
    CHECK_GT(layout.pages.front().text_runs.front().baseline_origin.y_points,
             image.bounds.y_points + image.bounds.height_points);

    featherdoc::pdf::PdfioGenerator generator;
    const auto write_result = generator.write(
        layout, output_path, featherdoc::pdf::PdfWriterOptions{});
    REQUIRE_MESSAGE(write_result.success, write_result.error_message);
    CHECK_GT(write_result.bytes_written, 0U);
    CHECK_FALSE(std::filesystem::exists(image.source_path));

    const auto pdf_bytes = read_binary_file(output_path);
    CHECK_NE(pdf_bytes.find("/XObject<</Im1"), std::string::npos);

#if defined(FEATHERDOC_BUILD_PDF_IMPORT)
    featherdoc::pdf::PdfiumParser parser;
    const auto parse_result = parser.parse(output_path, {});
    REQUIRE_MESSAGE(parse_result.success, parse_result.error_message);
    CHECK_NE(collect_text(parse_result.document).find("Image intro"),
             std::string::npos);
#endif
}

TEST_CASE("document PDF adapter positions floating images on PDF pages") {
    const auto image_path = std::filesystem::current_path() /
                            "featherdoc-adapter-floating-image.png";
    const auto output_path = std::filesystem::current_path() /
                             "featherdoc-adapter-floating-image.pdf";
    write_binary_file(image_path, quadrant_png_data());

    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());
    auto paragraph = document.paragraphs();
    REQUIRE(paragraph.has_next());
    CHECK(paragraph.set_text("Floating image anchor"));

    featherdoc::floating_image_options floating_options;
    floating_options.horizontal_reference =
        featherdoc::floating_image_horizontal_reference::page;
    floating_options.horizontal_offset_px = 144;
    floating_options.vertical_reference =
        featherdoc::floating_image_vertical_reference::page;
    floating_options.vertical_offset_px = 120;
    floating_options.wrap_mode = featherdoc::floating_image_wrap_mode::none;
    floating_options.crop = featherdoc::floating_image_crop{500U, 0U, 0U, 0U};
    REQUIRE(document.append_floating_image(image_path, 96U, 48U,
                                           floating_options));

    const auto drawing_images = document.drawing_images();
    REQUIRE_EQ(drawing_images.size(), 1U);
    CHECK_EQ(drawing_images.front().placement,
             featherdoc::drawing_image_placement::anchored_object);
    REQUIRE(drawing_images.front().floating_options.has_value());

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.use_system_font_fallbacks = false;
    options.render_inline_images = true;

    const auto layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);

    REQUIRE_EQ(layout.pages.size(), 1U);
    REQUIRE_EQ(layout.pages.front().images.size(), 1U);
    CHECK_NE(collect_layout_text(layout).find("Floating image anchor"),
             std::string::npos);

    const auto &image = layout.pages.front().images.front();
    CHECK_EQ(image.content_type, "image/png");
    CHECK_EQ(image.bounds.x_points, doctest::Approx(108.0));
    CHECK_EQ(image.bounds.width_points, doctest::Approx(72.0));
    CHECK_EQ(image.bounds.height_points, doctest::Approx(36.0));
    CHECK_EQ(image.bounds.y_points,
             doctest::Approx(options.page_size.height_points - 90.0 - 36.0));
    CHECK(image.cleanup_source_after_write);
    CHECK_FALSE(image.draw_behind_text);
    CHECK_EQ(image.crop_left_per_mille, 500U);
    CHECK_EQ(image.crop_top_per_mille, 0U);
    CHECK_EQ(image.crop_right_per_mille, 0U);
    CHECK_EQ(image.crop_bottom_per_mille, 0U);
    CHECK(std::filesystem::exists(image.source_path));

    featherdoc::pdf::PdfioGenerator generator;
    const auto write_result = generator.write(
        layout, output_path, featherdoc::pdf::PdfWriterOptions{});
    REQUIRE_MESSAGE(write_result.success, write_result.error_message);
    CHECK_GT(write_result.bytes_written, 0U);
    CHECK_FALSE(std::filesystem::exists(image.source_path));

    const auto pdf_bytes = read_binary_file(output_path);
    CHECK_NE(pdf_bytes.find("/XObject<</Im1"), std::string::npos);

#if defined(FEATHERDOC_BUILD_PDF_IMPORT)
    featherdoc::pdf::PdfiumParser parser;
    const auto parse_result = parser.parse(output_path, {});
    REQUIRE_MESSAGE(parse_result.success, parse_result.error_message);
    CHECK_NE(collect_text(parse_result.document).find("Floating image anchor"),
             std::string::npos);
#endif
}

TEST_CASE("document PDF adapter marks behind-text floating images") {
    const auto image_path = std::filesystem::current_path() /
                            "featherdoc-adapter-behind-text-image.png";
    const auto output_path = std::filesystem::current_path() /
                             "featherdoc-adapter-behind-text-image.pdf";
    write_binary_file(image_path, quadrant_png_data());

    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());
    auto paragraph = document.paragraphs();
    REQUIRE(paragraph.has_next());
    CHECK(paragraph.set_text("Behind text floating image anchor"));

    featherdoc::floating_image_options floating_options;
    floating_options.horizontal_reference =
        featherdoc::floating_image_horizontal_reference::margin;
    floating_options.horizontal_offset_px = 24;
    floating_options.vertical_reference =
        featherdoc::floating_image_vertical_reference::paragraph;
    floating_options.vertical_offset_px = 0;
    floating_options.wrap_mode = featherdoc::floating_image_wrap_mode::none;
    floating_options.behind_text = true;
    REQUIRE(document.append_floating_image(image_path, 96U, 48U,
                                           floating_options));

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.use_system_font_fallbacks = false;
    options.render_inline_images = true;

    const auto layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);

    REQUIRE_EQ(layout.pages.size(), 1U);
    REQUIRE_EQ(layout.pages.front().images.size(), 1U);
    CHECK_NE(collect_layout_text(layout).find("Behind text floating image anchor"),
             std::string::npos);

    const auto &image = layout.pages.front().images.front();
    CHECK_EQ(image.content_type, "image/png");
    CHECK(image.draw_behind_text);
    CHECK_EQ(image.bounds.x_points,
             doctest::Approx(options.margin_left_points + 18.0));
    CHECK_EQ(image.bounds.width_points, doctest::Approx(72.0));
    CHECK_EQ(image.bounds.height_points, doctest::Approx(36.0));

    featherdoc::pdf::PdfioGenerator generator;
    const auto write_result = generator.write(
        layout, output_path, featherdoc::pdf::PdfWriterOptions{});
    REQUIRE_MESSAGE(write_result.success, write_result.error_message);
    CHECK_GT(write_result.bytes_written, 0U);

#if defined(FEATHERDOC_BUILD_PDF_IMPORT)
    featherdoc::pdf::PdfiumParser parser;
    const auto parse_result = parser.parse(output_path, {});
    REQUIRE_MESSAGE(parse_result.success, parse_result.error_message);
    CHECK_NE(collect_text(parse_result.document)
                 .find("Behind text floating image anchor"),
             std::string::npos);
#endif
}

TEST_CASE("document PDF adapter wraps text around square floating images") {
    const auto image_path = std::filesystem::current_path() /
                            "featherdoc-adapter-square-wrap-image.png";
    const auto output_path = std::filesystem::current_path() /
                             "featherdoc-adapter-square-wrap-image.pdf";
    write_binary_file(image_path, quadrant_png_data());

    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());
    featherdoc::floating_image_options floating_options;
    floating_options.horizontal_reference =
        featherdoc::floating_image_horizontal_reference::column;
    floating_options.horizontal_offset_px = 0;
    floating_options.vertical_reference =
        featherdoc::floating_image_vertical_reference::paragraph;
    floating_options.vertical_offset_px = 0;
    floating_options.wrap_mode = featherdoc::floating_image_wrap_mode::square;
    floating_options.wrap_distance_right_px = 16U;
    REQUIRE(document.append_floating_image(image_path, 96U, 72U,
                                           floating_options));
    append_body_paragraph(
        document,
        "Square wrap text should flow beside the floating image before it "
        "continues below the image on later PDF lines. The paragraph keeps "
        "running long enough to leave the vertical image area and return to "
        "the normal left margin once the wrapped region has ended.");

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.use_system_font_fallbacks = false;
    options.render_inline_images = true;
    options.line_height_points = 14.0;

    const auto layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);

    REQUIRE_EQ(layout.pages.size(), 1U);
    REQUIRE_EQ(layout.pages.front().images.size(), 1U);
    REQUIRE_GE(layout.pages.front().text_runs.size(), 2U);

    const auto &image = layout.pages.front().images.front();
    const auto &first_text = layout.pages.front().text_runs.front();
    CHECK_EQ(image.bounds.x_points, doctest::Approx(options.margin_left_points));
    CHECK_EQ(first_text.baseline_origin.x_points,
             doctest::Approx(image.bounds.x_points +
                             image.bounds.width_points + 12.0));
    CHECK_GT(first_text.baseline_origin.y_points, image.bounds.y_points);
    CHECK_LT(first_text.baseline_origin.y_points,
             image.bounds.y_points + image.bounds.height_points);

    auto returned_to_margin = false;
    for (const auto &text_run : layout.pages.front().text_runs) {
        if (text_run.baseline_origin.y_points < image.bounds.y_points &&
            text_run.baseline_origin.x_points ==
                doctest::Approx(options.margin_left_points)) {
            returned_to_margin = true;
        }
    }
    CHECK(returned_to_margin);

    featherdoc::pdf::PdfioGenerator generator;
    const auto write_result = generator.write(
        layout, output_path, featherdoc::pdf::PdfWriterOptions{});
    REQUIRE_MESSAGE(write_result.success, write_result.error_message);
    CHECK_GT(write_result.bytes_written, 0U);
}

TEST_CASE("document PDF adapter pushes top-bottom wrapped image text below") {
    const auto image_path = std::filesystem::current_path() /
                            "featherdoc-adapter-top-bottom-wrap-image.png";
    const auto output_path = std::filesystem::current_path() /
                             "featherdoc-adapter-top-bottom-wrap-image.pdf";
    write_binary_file(image_path, quadrant_png_data());

    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());
    featherdoc::floating_image_options floating_options;
    floating_options.horizontal_reference =
        featherdoc::floating_image_horizontal_reference::column;
    floating_options.horizontal_offset_px = 0;
    floating_options.vertical_reference =
        featherdoc::floating_image_vertical_reference::paragraph;
    floating_options.vertical_offset_px = 0;
    floating_options.wrap_mode =
        featherdoc::floating_image_wrap_mode::top_bottom;
    floating_options.wrap_distance_bottom_px = 16U;
    REQUIRE(document.append_floating_image(image_path, 96U, 72U,
                                           floating_options));
    append_body_paragraph(document,
                          "Top bottom text starts below the image.");

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.use_system_font_fallbacks = false;
    options.render_inline_images = true;
    options.line_height_points = 14.0;

    const auto layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);

    REQUIRE_EQ(layout.pages.size(), 1U);
    REQUIRE_EQ(layout.pages.front().images.size(), 1U);
    REQUIRE_GE(layout.pages.front().text_runs.size(), 1U);

    const auto &image = layout.pages.front().images.front();
    const auto &first_text = layout.pages.front().text_runs.front();
    CHECK_EQ(first_text.baseline_origin.x_points,
             doctest::Approx(options.margin_left_points));
    CHECK_LE(first_text.baseline_origin.y_points + options.font_size_points,
             image.bounds.y_points - 12.0);

    featherdoc::pdf::PdfioGenerator generator;
    const auto write_result = generator.write(
        layout, output_path, featherdoc::pdf::PdfWriterOptions{});
    REQUIRE_MESSAGE(write_result.success, write_result.error_message);
    CHECK_GT(write_result.bytes_written, 0U);
}

TEST_CASE(
    "document PDF adapter lays out inline images in body paragraph order") {
    const auto image_path = std::filesystem::current_path() /
                            "featherdoc-adapter-ordered-inline-image.png";
    write_binary_file(image_path, tiny_png_data());

    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());
    auto paragraph = document.paragraphs();
    REQUIRE(paragraph.has_next());
    CHECK(paragraph.set_text("Before image"));
    REQUIRE(document.append_image(image_path, 20U, 10U));

    auto body_template = document.body_template();
    REQUIRE(static_cast<bool>(body_template));
    auto after = body_template.append_paragraph("After image");
    REQUIRE(after.has_next());

    const auto images = document.inline_images();
    REQUIRE_EQ(images.size(), 1U);
    REQUIRE(images.front().paragraph_index.has_value());
    CHECK_EQ(*images.front().paragraph_index, 1U);

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.use_system_font_fallbacks = false;
    options.render_inline_images = true;

    const auto layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);

    REQUIRE_EQ(layout.pages.size(), 1U);
    REQUIRE_EQ(layout.pages.front().images.size(), 1U);
    REQUIRE_EQ(layout.pages.front().text_runs.size(), 2U);

    const auto &before_text = layout.pages.front().text_runs[0];
    const auto &after_text = layout.pages.front().text_runs[1];
    const auto &image = layout.pages.front().images.front();

    CHECK_EQ(before_text.text, "Before image");
    CHECK_EQ(after_text.text, "After image");
    CHECK_GT(before_text.baseline_origin.y_points,
             image.bounds.y_points + image.bounds.height_points);
    CHECK_GT(image.bounds.y_points, after_text.baseline_origin.y_points);
}

TEST_CASE("PDF writer accepts inline image layout blocks") {
    const auto image_path =
        std::filesystem::current_path() / "featherdoc-writer-inline-image.png";
    const auto output_path =
        std::filesystem::current_path() / "featherdoc-writer-inline-image.pdf";
    write_binary_file(image_path, tiny_png_data());

    featherdoc::pdf::PdfDocumentLayout layout;
    layout.metadata.title = "FeatherDoc image writer test";
    layout.metadata.creator = "FeatherDoc tests";

    featherdoc::pdf::PdfPageLayout page;
    page.size = featherdoc::pdf::PdfPageSize{};
    page.images.push_back(featherdoc::pdf::PdfImage{
        featherdoc::pdf::PdfRect{72.0, 640.0, 24.0, 24.0},
        image_path,
        "image/png",
        "Tiny image",
        true,
        false,
    });
    page.text_runs.push_back(featherdoc::pdf::PdfTextRun{
        featherdoc::pdf::PdfPoint{72.0, 620.0},
        "Image PDF text",
        "Helvetica",
        {},
        12.0,
        featherdoc::pdf::PdfRgbColor{0.0, 0.0, 0.0},
        false,
        false,
        false,
        false,
    });
    layout.pages.push_back(std::move(page));

    featherdoc::pdf::PdfioGenerator generator;
    const auto write_result = generator.write(
        layout, output_path, featherdoc::pdf::PdfWriterOptions{});
    REQUIRE_MESSAGE(write_result.success, write_result.error_message);
    CHECK_GT(write_result.bytes_written, 0U);

#if defined(FEATHERDOC_BUILD_PDF_IMPORT)
    featherdoc::pdf::PdfiumParser parser;
    const auto parse_result = parser.parse(output_path, {});
    REQUIRE_MESSAGE(parse_result.success, parse_result.error_message);
    CHECK_NE(collect_text(parse_result.document).find("Image PDF text"),
             std::string::npos);
#endif
}

TEST_CASE("PDF writer draws foreground images after text") {
    const auto image_path = std::filesystem::current_path() /
                            "featherdoc-writer-foreground-image.png";
    const auto output_path = std::filesystem::current_path() /
                             "featherdoc-writer-foreground-image.pdf";
    write_binary_file(image_path, quadrant_png_data());

    featherdoc::pdf::PdfDocumentLayout layout;
    layout.metadata.title = "FeatherDoc foreground image writer test";
    layout.metadata.creator = "FeatherDoc tests";

    featherdoc::pdf::PdfPageLayout page;
    page.size = featherdoc::pdf::PdfPageSize{};
    page.text_runs.push_back(featherdoc::pdf::PdfTextRun{
        featherdoc::pdf::PdfPoint{72.0, 724.0},
        "Foreground image layer",
        "Helvetica",
        {},
        18.0,
        featherdoc::pdf::PdfRgbColor{0.0, 0.0, 0.0},
        false,
        false,
        false,
        false,
    });
    page.images.push_back(featherdoc::pdf::PdfImage{
        featherdoc::pdf::PdfRect{72.0, 700.0, 144.0, 48.0},
        image_path,
        "image/png",
        "Foreground image",
        true,
        false,
    });
    page.images.back().draw_behind_text = false;
    layout.pages.push_back(std::move(page));

    featherdoc::pdf::PdfioGenerator generator;
    const auto write_result = generator.write(
        layout, output_path, featherdoc::pdf::PdfWriterOptions{});
    REQUIRE_MESSAGE(write_result.success, write_result.error_message);
    CHECK_GT(write_result.bytes_written, 0U);

    const auto pdf_bytes = read_binary_file(output_path);
    CHECK_NE(pdf_bytes.find("/XObject<</Im1"), std::string::npos);

#if defined(FEATHERDOC_BUILD_PDF_IMPORT)
    featherdoc::pdf::PdfiumParser parser;
    const auto parse_result = parser.parse(output_path, {});
    REQUIRE_MESSAGE(parse_result.success, parse_result.error_message);
    CHECK_NE(collect_text(parse_result.document).find("Foreground image layer"),
             std::string::npos);
#endif
}

TEST_CASE("document PDF adapter preserves inline RGBA PNG images for PDFio") {
    const auto image_path = std::filesystem::current_path() /
                            "featherdoc-adapter-inline-rgba-image.png";
    const auto output_path = std::filesystem::current_path() /
                             "featherdoc-adapter-inline-rgba-image.pdf";
    write_binary_file(image_path, tiny_rgba_png_data());

    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());
    auto paragraph = document.paragraphs();
    REQUIRE(paragraph.has_next());
    CHECK(paragraph.set_text("RGBA image intro"));
    REQUIRE(document.append_image(image_path, 20U, 10U));

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.use_system_font_fallbacks = false;
    options.render_inline_images = true;

    const auto layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);

    REQUIRE_EQ(layout.pages.size(), 1U);
    REQUIRE_EQ(layout.pages.front().images.size(), 1U);
    CHECK_NE(collect_layout_text(layout).find("RGBA image intro"),
             std::string::npos);

    const auto &image = layout.pages.front().images.front();
    CHECK_EQ(image.content_type, "image/png");
    CHECK_EQ(image.bounds.width_points, doctest::Approx(15.0));
    CHECK_EQ(image.bounds.height_points, doctest::Approx(7.5));
    CHECK(image.cleanup_source_after_write);
    CHECK(std::filesystem::exists(image.source_path));

    featherdoc::pdf::PdfioGenerator generator;
    const auto write_result = generator.write(
        layout, output_path, featherdoc::pdf::PdfWriterOptions{});
    REQUIRE_MESSAGE(write_result.success, write_result.error_message);
    CHECK_GT(write_result.bytes_written, 0U);
    const auto pdf_bytes = read_binary_file(output_path);
    CHECK_NE(pdf_bytes.find("/SMask"), std::string::npos);
    CHECK_FALSE(std::filesystem::exists(image.source_path));
    CHECK_NE(pdf_bytes.find("/XObject<</Im1"), std::string::npos);
}

TEST_CASE("document PDF adapter preserves body order when laying out tables") {
    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());

    auto before = document.paragraphs();
    REQUIRE(before.has_next());
    CHECK(before.set_text("Before table"));

    auto table = document.append_table(2U, 2U);
    REQUIRE(table.has_next());
    CHECK(table.set_width_twips(7200U));
    CHECK(table.set_column_width_twips(0U, 2400U));
    CHECK(table.set_column_width_twips(1U, 4800U));
    CHECK(table.set_cell_text(0U, 0U, "Name"));
    CHECK(table.set_cell_text(0U, 1U, "Amount"));
    CHECK(table.set_cell_text(1U, 0U, "Service"));
    CHECK(table.set_cell_text(1U, 1U, "100"));

    auto body_template = document.body_template();
    REQUIRE(static_cast<bool>(body_template));
    auto after = body_template.append_paragraph("After table");
    REQUIRE(after.has_next());

    const auto blocks = document.inspect_body_blocks();
    REQUIRE_EQ(blocks.size(), 3U);
    CHECK_EQ(blocks[0].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(blocks[0].item_index, 0U);
    CHECK_EQ(blocks[1].kind, featherdoc::body_block_kind::table);
    CHECK_EQ(blocks[1].item_index, 0U);
    CHECK_EQ(blocks[2].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(blocks[2].item_index, 1U);

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.use_system_font_fallbacks = false;

    const auto layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);

    REQUIRE_EQ(layout.pages.size(), 1U);
    CHECK_GE(layout.pages.front().rectangles.size(), 4U);

    std::vector<std::string> text_runs;
    for (const auto &run : layout.pages.front().text_runs) {
        if (!run.text.empty()) {
            text_runs.push_back(run.text);
        }
    }

    REQUIRE_GE(text_runs.size(), 6U);
    CHECK_EQ(text_runs[0], "Before table");
    CHECK_NE(std::find(text_runs.begin(), text_runs.end(), "Name"),
             text_runs.end());
    CHECK_NE(std::find(text_runs.begin(), text_runs.end(), "Amount"),
             text_runs.end());
    CHECK_EQ(text_runs.back(), "After table");

    const auto before_y =
        layout.pages.front().text_runs.front().baseline_origin.y_points;
    const auto after_y =
        layout.pages.front().text_runs.back().baseline_origin.y_points;
    CHECK_GT(before_y, after_y);
}

TEST_CASE("document PDF adapter keeps text after positioned tables below them") {
    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());

    auto before = document.paragraphs();
    REQUIRE(before.has_next());
    CHECK(before.set_text("Before positioned table"));

    auto table = document.append_table(1U, 1U);
    REQUIRE(table.has_next());
    CHECK(table.set_width_twips(2880U));
    CHECK(table.set_column_width_twips(0U, 2880U));
    CHECK(table.set_cell_text(0U, 0U, "Positioned cell"));

    auto row = table.rows();
    REQUIRE(row.has_next());
    CHECK(row.set_height_twips(720U, featherdoc::row_height_rule::exact));

    auto position = featherdoc::table_position{};
    position.horizontal_reference =
        featherdoc::table_position_horizontal_reference::margin;
    position.horizontal_offset_twips = 0;
    position.vertical_reference =
        featherdoc::table_position_vertical_reference::paragraph;
    position.vertical_offset_twips = 0;
    CHECK(table.set_position(position));

    auto body_template = document.body_template();
    REQUIRE(static_cast<bool>(body_template));
    auto after = body_template.append_paragraph("After positioned table");
    REQUIRE(after.has_next());

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.use_system_font_fallbacks = false;
    options.line_height_points = 16.0;
    options.paragraph_spacing_after_points = 6.0;

    const auto layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);

    REQUIRE_EQ(layout.pages.size(), 1U);
    REQUIRE_EQ(layout.pages.front().rectangles.size(), 1U);

    const auto &table_rect = layout.pages.front().rectangles.front();
    const featherdoc::pdf::PdfTextRun *after_text = nullptr;
    for (const auto &run : layout.pages.front().text_runs) {
        if (run.text == "After positioned table") {
            after_text = &run;
            break;
        }
    }

    REQUIRE(after_text != nullptr);
    CHECK_LT(after_text->baseline_origin.y_points, table_rect.bounds.y_points);
}

TEST_CASE("document PDF adapter repeats table header rows across pages") {
    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());

    auto table = document.append_table(6U, 2U);
    REQUIRE(table.has_next());
    CHECK(table.set_width_twips(2880U));
    CHECK(table.set_column_width_twips(0U, 1440U));
    CHECK(table.set_column_width_twips(1U, 1440U));
    auto position = featherdoc::table_position{};
    position.horizontal_reference =
        featherdoc::table_position_horizontal_reference::margin;
    position.horizontal_offset_twips = 0;
    position.vertical_reference =
        featherdoc::table_position_vertical_reference::page;
    position.vertical_offset_twips = 720;
    CHECK(table.set_position(position));
    CHECK(table.set_cell_text(0U, 0U, "Header"));
    CHECK(table.set_cell_text(0U, 1U, "Amount"));
    for (std::size_t row_index = 1U; row_index < 6U; ++row_index) {
        CHECK(table.set_cell_text(row_index, 0U,
                                  "Row " + std::to_string(row_index)));
        CHECK(table.set_cell_text(row_index, 1U,
                                  std::to_string(row_index * 10U)));
    }

    auto row = table.rows();
    for (std::size_t row_index = 0U; row_index < 6U; ++row_index) {
        REQUIRE(row.has_next());
        CHECK(row.set_height_twips(720U, featherdoc::row_height_rule::exact));
        if (row_index == 0U) {
            CHECK(row.set_repeats_header());
        }
        row.next();
    }

    const auto table_summary = document.inspect_table(0U);
    REQUIRE(table_summary.has_value());
    REQUIRE_EQ(table_summary->row_repeats_header.size(), 6U);
    CHECK(table_summary->row_repeats_header[0]);
    CHECK_FALSE(table_summary->row_repeats_header[1]);

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.use_system_font_fallbacks = false;
    options.page_size = featherdoc::pdf::PdfPageSize{300.0, 220.0};
    options.margin_left_points = 36.0;
    options.margin_right_points = 36.0;
    options.margin_top_points = 36.0;
    options.margin_bottom_points = 36.0;
    options.line_height_points = 14.0;
    options.paragraph_spacing_after_points = 0.0;

    const auto layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);

    REQUIRE_GE(layout.pages.size(), 2U);

    auto page_text_count = [](const featherdoc::pdf::PdfPageLayout &page,
                              std::string_view text) {
        auto count = 0U;
        for (const auto &run : page.text_runs) {
            if (run.text == text) {
                ++count;
            }
        }
        return count;
    };
    auto page_contains_row_text =
        [](const featherdoc::pdf::PdfPageLayout &page) {
            for (const auto &run : page.text_runs) {
                if (run.text.rfind("Row ", 0U) == 0U) {
                    return true;
                }
            }
            return false;
        };

    CHECK_EQ(page_text_count(layout.pages[0], "Header"), 1U);
    CHECK_EQ(page_text_count(layout.pages[1], "Header"), 1U);
    CHECK(page_contains_row_text(layout.pages[1]));
    for (const auto &page : layout.pages) {
        for (const auto &rectangle : page.rectangles) {
            CHECK_GE(rectangle.bounds.y_points,
                     options.margin_bottom_points - 0.01);
        }
    }
}

TEST_CASE("document PDF adapter keeps cant-split table rows on one page") {
    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());

    auto before = document.paragraphs();
    REQUIRE(before.has_next());
    CHECK(before.set_text("Before cant-split table"));

    auto table = document.append_table(1U, 1U);
    REQUIRE(table.has_next());
    CHECK(table.set_width_twips(2880U));
    CHECK(table.set_column_width_twips(0U, 2880U));
    CHECK(table.set_cell_text(0U, 0U, "Cant split row"));

    auto row = table.rows();
    REQUIRE(row.has_next());
    CHECK(row.set_height_twips(1600U, featherdoc::row_height_rule::exact));
    CHECK(row.set_cant_split());

    const auto table_summary = document.inspect_table(0U);
    REQUIRE(table_summary.has_value());
    REQUIRE_EQ(table_summary->row_cant_split.size(), 1U);
    CHECK(table_summary->row_cant_split[0]);

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.use_system_font_fallbacks = false;
    options.page_size = featherdoc::pdf::PdfPageSize{300.0, 160.0};
    options.margin_left_points = 36.0;
    options.margin_right_points = 36.0;
    options.margin_top_points = 36.0;
    options.margin_bottom_points = 36.0;
    options.line_height_points = 16.0;
    options.paragraph_spacing_after_points = 6.0;

    const auto layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);

    REQUIRE_EQ(layout.pages.size(), 2U);

    auto page_has_text = [](const featherdoc::pdf::PdfPageLayout &page,
                            std::string_view text) {
        for (const auto &run : page.text_runs) {
            if (run.text == text) {
                return true;
            }
        }
        return false;
    };

    CHECK(page_has_text(layout.pages[0], "Before cant-split table"));
    CHECK_FALSE(page_has_text(layout.pages[0], "Cant split row"));
    CHECK(page_has_text(layout.pages[1], "Cant split row"));
    REQUIRE_FALSE(layout.pages[1].rectangles.empty());
    CHECK_GE(layout.pages[1].rectangles.front().bounds.y_points,
             options.margin_bottom_points - 0.01);
}

TEST_CASE("document PDF adapter lays out merged table cells") {
    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());

    auto table = document.append_table(2U, 3U);
    REQUIRE(table.has_next());
    CHECK(table.set_width_twips(9000U));
    CHECK(table.set_column_width_twips(0U, 3000U));
    CHECK(table.set_column_width_twips(1U, 3000U));
    CHECK(table.set_column_width_twips(2U, 3000U));
    auto top_left = table.find_cell(0U, 0U);
    REQUIRE(top_left.has_value());
    CHECK(top_left->set_text("Merged heading"));
    CHECK(top_left->merge_right());
    CHECK(table.set_cell_text(0U, 1U, "Tail"));
    CHECK(table.set_cell_text(1U, 0U, "A"));
    CHECK(table.set_cell_text(1U, 1U, "B"));
    CHECK(table.set_cell_text(1U, 2U, "C"));

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.use_system_font_fallbacks = false;

    const auto layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);

    REQUIRE_EQ(layout.pages.size(), 1U);
    REQUIRE_GE(layout.pages.front().rectangles.size(), 5U);

    const auto &first_cell = layout.pages.front().rectangles.front();
    const auto &second_cell = layout.pages.front().rectangles[1];
    CHECK_GT(first_cell.bounds.width_points, second_cell.bounds.width_points);
    CHECK_EQ(layout.pages.front().text_runs.front().text, "Merged heading");
}

TEST_CASE("document PDF adapter lays out vertically merged table cells") {
    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());

    auto table = document.append_table(2U, 2U);
    REQUIRE(table.has_next());
    CHECK(table.set_width_twips(2880U));
    CHECK(table.set_column_width_twips(0U, 1440U));
    CHECK(table.set_column_width_twips(1U, 1440U));

    auto first_row = table.rows();
    REQUIRE(first_row.has_next());
    CHECK(first_row.set_height_twips(720U, featherdoc::row_height_rule::exact));
    first_row.next();
    REQUIRE(first_row.has_next());
    CHECK(first_row.set_height_twips(720U, featherdoc::row_height_rule::exact));

    auto merged_cell = table.find_cell(0U, 0U);
    REQUIRE(merged_cell.has_value());
    CHECK(merged_cell->set_text("Merged down"));
    CHECK(merged_cell->merge_down());
    CHECK(table.set_cell_text(0U, 1U, "Top right"));
    CHECK(table.set_cell_text(1U, 1U, "Bottom right"));

    const auto cells = document.inspect_table_cells(0U);
    REQUIRE_EQ(cells.size(), 4U);
    CHECK_EQ(cells[0].vertical_merge, featherdoc::cell_vertical_merge::restart);
    CHECK_EQ(cells[0].row_span, 2U);
    CHECK_EQ(cells[2].vertical_merge,
             featherdoc::cell_vertical_merge::continue_merge);

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.use_system_font_fallbacks = false;
    options.line_height_points = 14.0;

    const auto layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);

    REQUIRE_EQ(layout.pages.size(), 1U);
    REQUIRE_EQ(layout.pages.front().rectangles.size(), 3U);
    REQUIRE_GE(layout.pages.front().text_runs.size(), 3U);

    const auto &merged_rectangle = layout.pages.front().rectangles.front();
    CHECK(merged_rectangle.bounds.height_points == doctest::Approx(72.0));
    CHECK_EQ(layout.pages.front().text_runs[0].text, "Merged");
    CHECK_EQ(layout.pages.front().text_runs[1].text, "down");
}

TEST_CASE("document PDF adapter maps table cell fill and margins") {
    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());

    auto table = document.append_table(1U, 1U);
    REQUIRE(table.has_next());
    CHECK(table.set_width_twips(2880U));
    CHECK(table.set_column_width_twips(0U, 2880U));

    auto cell = table.find_cell(0U, 0U);
    REQUIRE(cell.has_value());
    CHECK(cell->set_text("Padded cell"));
    CHECK(cell->set_fill_color("D9EAF7"));
    CHECK(cell->set_margin_twips(featherdoc::cell_margin_edge::top, 120U));
    CHECK(cell->set_margin_twips(featherdoc::cell_margin_edge::left, 240U));
    CHECK(cell->set_margin_twips(featherdoc::cell_margin_edge::bottom, 360U));
    CHECK(cell->set_margin_twips(featherdoc::cell_margin_edge::right, 480U));

    const auto cells = document.inspect_table_cells(0U);
    REQUIRE_EQ(cells.size(), 1U);
    REQUIRE(cells.front().fill_color.has_value());
    CHECK_EQ(*cells.front().fill_color, "D9EAF7");
    CHECK_EQ(cells.front().margin_top_twips.value_or(0U), 120U);
    CHECK_EQ(cells.front().margin_left_twips.value_or(0U), 240U);
    CHECK_EQ(cells.front().margin_bottom_twips.value_or(0U), 360U);
    CHECK_EQ(cells.front().margin_right_twips.value_or(0U), 480U);

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.use_system_font_fallbacks = false;
    options.line_height_points = 14.0;

    const auto layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);

    REQUIRE_EQ(layout.pages.size(), 1U);
    REQUIRE_EQ(layout.pages.front().rectangles.size(), 1U);
    REQUIRE_EQ(layout.pages.front().text_runs.size(), 1U);

    const auto &rectangle = layout.pages.front().rectangles.front();
    CHECK(rectangle.fill);
    CHECK(rectangle.fill_color.red == doctest::Approx(217.0 / 255.0));
    CHECK(rectangle.fill_color.green == doctest::Approx(234.0 / 255.0));
    CHECK(rectangle.fill_color.blue == doctest::Approx(247.0 / 255.0));
    CHECK(rectangle.bounds.x_points ==
          doctest::Approx(options.margin_left_points));
    CHECK(rectangle.bounds.width_points == doctest::Approx(144.0));
    CHECK(rectangle.bounds.height_points == doctest::Approx(38.0));

    const auto &text_run = layout.pages.front().text_runs.front();
    CHECK_EQ(text_run.text, "Padded cell");
    CHECK(text_run.baseline_origin.x_points ==
          doctest::Approx(rectangle.bounds.x_points + 12.0));

    const auto row_top =
        rectangle.bounds.y_points + rectangle.bounds.height_points;
    CHECK(text_run.baseline_origin.y_points ==
          doctest::Approx(row_top - 6.0 - options.font_size_points));
}

TEST_CASE("document PDF adapter uses table default cell margins") {
    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());

    auto table = document.append_table(1U, 1U);
    REQUIRE(table.has_next());
    CHECK(table.set_width_twips(2880U));
    CHECK(table.set_column_width_twips(0U, 2880U));
    CHECK(table.set_cell_margin_twips(featherdoc::cell_margin_edge::top, 100U));
    CHECK(
        table.set_cell_margin_twips(featherdoc::cell_margin_edge::left, 200U));
    CHECK(table.set_cell_margin_twips(featherdoc::cell_margin_edge::bottom,
                                      300U));
    CHECK(
        table.set_cell_margin_twips(featherdoc::cell_margin_edge::right, 400U));

    auto cell = table.find_cell(0U, 0U);
    REQUIRE(cell.has_value());
    CHECK(cell->set_text("Default margin"));

    const auto table_summary = document.inspect_table(0U);
    REQUIRE(table_summary.has_value());
    CHECK_EQ(table_summary->cell_margin_top_twips.value_or(0U), 100U);
    CHECK_EQ(table_summary->cell_margin_left_twips.value_or(0U), 200U);
    CHECK_EQ(table_summary->cell_margin_bottom_twips.value_or(0U), 300U);
    CHECK_EQ(table_summary->cell_margin_right_twips.value_or(0U), 400U);

    const auto cells = document.inspect_table_cells(0U);
    REQUIRE_EQ(cells.size(), 1U);
    CHECK_FALSE(cells.front().margin_left_twips.has_value());

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.use_system_font_fallbacks = false;
    options.line_height_points = 14.0;

    const auto layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);

    REQUIRE_EQ(layout.pages.size(), 1U);
    REQUIRE_EQ(layout.pages.front().rectangles.size(), 1U);
    REQUIRE_EQ(layout.pages.front().text_runs.size(), 1U);

    const auto &rectangle = layout.pages.front().rectangles.front();
    CHECK(rectangle.bounds.height_points == doctest::Approx(34.0));

    const auto &text_run = layout.pages.front().text_runs.front();
    CHECK(text_run.baseline_origin.x_points ==
          doctest::Approx(rectangle.bounds.x_points + 10.0));
    const auto row_top =
        rectangle.bounds.y_points + rectangle.bounds.height_points;
    CHECK(text_run.baseline_origin.y_points ==
          doctest::Approx(row_top - 5.0 - options.font_size_points));
}

TEST_CASE("document PDF adapter maps table alignment and indent") {
    {
        featherdoc::Document document;
        REQUIRE_FALSE(document.create_empty());

        auto table = document.append_table(1U, 1U);
        REQUIRE(table.has_next());
        CHECK(table.set_width_twips(1440U));
        CHECK(table.set_column_width_twips(0U, 1440U));
        CHECK(table.set_alignment(featherdoc::table_alignment::center));
        CHECK(table.set_cell_text(0U, 0U, "Centered"));

        const auto table_summary = document.inspect_table(0U);
        REQUIRE(table_summary.has_value());
        REQUIRE(table_summary->alignment.has_value());
        CHECK_EQ(*table_summary->alignment,
                 featherdoc::table_alignment::center);

        featherdoc::pdf::PdfDocumentAdapterOptions options;
        options.use_system_font_fallbacks = false;

        const auto layout =
            featherdoc::pdf::layout_document_paragraphs(document, options);

        REQUIRE_EQ(layout.pages.size(), 1U);
        REQUIRE_EQ(layout.pages.front().rectangles.size(), 1U);

        const auto &rectangle = layout.pages.front().rectangles.front();
        const auto available_width = options.page_size.width_points -
                                     options.margin_left_points -
                                     options.margin_right_points;
        CHECK(rectangle.bounds.x_points ==
              doctest::Approx(options.margin_left_points +
                              (available_width - 72.0) / 2.0));
    }

    {
        featherdoc::Document document;
        REQUIRE_FALSE(document.create_empty());

        auto table = document.append_table(1U, 1U);
        REQUIRE(table.has_next());
        CHECK(table.set_width_twips(1440U));
        CHECK(table.set_column_width_twips(0U, 1440U));
        CHECK(table.set_alignment(featherdoc::table_alignment::left));
        CHECK(table.set_indent_twips(720U));
        CHECK(table.set_cell_text(0U, 0U, "Indented"));

        const auto table_summary = document.inspect_table(0U);
        REQUIRE(table_summary.has_value());
        REQUIRE(table_summary->indent_twips.has_value());
        CHECK_EQ(*table_summary->indent_twips, 720U);

        featherdoc::pdf::PdfDocumentAdapterOptions options;
        options.use_system_font_fallbacks = false;

        const auto layout =
            featherdoc::pdf::layout_document_paragraphs(document, options);

        REQUIRE_EQ(layout.pages.size(), 1U);
        REQUIRE_EQ(layout.pages.front().rectangles.size(), 1U);

        const auto &rectangle = layout.pages.front().rectangles.front();
        CHECK(rectangle.bounds.x_points ==
              doctest::Approx(options.margin_left_points + 36.0));
    }

    {
        featherdoc::Document document;
        REQUIRE_FALSE(document.create_empty());

        auto table = document.append_table(1U, 1U);
        REQUIRE(table.has_next());
        CHECK(table.set_width_twips(1440U));
        CHECK(table.set_column_width_twips(0U, 1440U));
        CHECK(table.set_alignment(featherdoc::table_alignment::right));
        CHECK(table.set_cell_text(0U, 0U, "Right aligned"));

        featherdoc::pdf::PdfDocumentAdapterOptions options;
        options.use_system_font_fallbacks = false;

        const auto layout =
            featherdoc::pdf::layout_document_paragraphs(document, options);

        REQUIRE_EQ(layout.pages.size(), 1U);
        REQUIRE_EQ(layout.pages.front().rectangles.size(), 1U);

        const auto &rectangle = layout.pages.front().rectangles.front();
        const auto available_width = options.page_size.width_points -
                                     options.margin_left_points -
                                     options.margin_right_points;
        CHECK(rectangle.bounds.x_points ==
              doctest::Approx(options.margin_left_points + available_width -
                              72.0));
    }

    {
        featherdoc::Document document;
        REQUIRE_FALSE(document.create_empty());

        auto table = document.append_table(1U, 1U);
        REQUIRE(table.has_next());
        CHECK(table.set_width_twips(1440U));
        CHECK(table.set_column_width_twips(0U, 1440U));
        CHECK(table.set_cell_text(0U, 0U, "Positioned"));
        CHECK(table.set_alignment(featherdoc::table_alignment::right));
        CHECK(table.set_indent_twips(720U));

        auto position = featherdoc::table_position{};
        position.horizontal_reference =
            featherdoc::table_position_horizontal_reference::page;
        position.horizontal_offset_twips = 2160;
        position.vertical_reference =
            featherdoc::table_position_vertical_reference::page;
        position.vertical_offset_twips = 1440;
        CHECK(table.set_position(position));

        const auto table_summary = document.inspect_table(0U);
        REQUIRE(table_summary.has_value());
        REQUIRE(table_summary->position.has_value());
        CHECK_EQ(table_summary->position->horizontal_reference,
                 featherdoc::table_position_horizontal_reference::page);
        CHECK_EQ(table_summary->position->horizontal_offset_twips, 2160);
        CHECK_EQ(table_summary->position->vertical_reference,
                 featherdoc::table_position_vertical_reference::page);
        CHECK_EQ(table_summary->position->vertical_offset_twips, 1440);

        featherdoc::pdf::PdfDocumentAdapterOptions options;
        options.use_system_font_fallbacks = false;
        options.margin_left_points = 36.0;

        const auto layout =
            featherdoc::pdf::layout_document_paragraphs(document, options);

        REQUIRE_EQ(layout.pages.size(), 1U);
        REQUIRE_EQ(layout.pages.front().rectangles.size(), 1U);

        const auto &rectangle = layout.pages.front().rectangles.front();
        CHECK(rectangle.bounds.x_points == doctest::Approx(108.0));
        CHECK(rectangle.bounds.y_points + rectangle.bounds.height_points ==
              doctest::Approx(options.page_size.height_points - 72.0));
    }

    {
        featherdoc::Document document;
        REQUIRE_FALSE(document.create_empty());

        auto table = document.append_table(1U, 1U);
        REQUIRE(table.has_next());
        CHECK(table.set_width_twips(1440U));
        CHECK(table.set_column_width_twips(0U, 1440U));
        CHECK(table.set_cell_text(0U, 0U, "Negative margin positioned"));

        auto position = featherdoc::table_position{};
        position.horizontal_reference =
            featherdoc::table_position_horizontal_reference::margin;
        position.horizontal_offset_twips = -360;
        position.vertical_reference =
            featherdoc::table_position_vertical_reference::margin;
        position.vertical_offset_twips = -720;
        CHECK(table.set_position(position));

        featherdoc::pdf::PdfDocumentAdapterOptions options;
        options.use_system_font_fallbacks = false;
        options.margin_left_points = 60.0;
        options.margin_top_points = 80.0;

        const auto layout =
            featherdoc::pdf::layout_document_paragraphs(document, options);

        REQUIRE_EQ(layout.pages.size(), 1U);
        REQUIRE_EQ(layout.pages.front().rectangles.size(), 1U);

        const auto &rectangle = layout.pages.front().rectangles.front();
        CHECK(rectangle.bounds.x_points ==
              doctest::Approx(options.margin_left_points - 18.0));
        CHECK(rectangle.bounds.y_points + rectangle.bounds.height_points ==
              doctest::Approx(options.page_size.height_points -
                              options.margin_top_points + 36.0));
    }

    {
        featherdoc::Document document;
        REQUIRE_FALSE(document.create_empty());
        auto paragraph = document.paragraphs();
        REQUIRE(paragraph.has_next());
        CHECK(paragraph.set_text("Before positioned table"));

        auto table = document.append_table(1U, 1U);
        REQUIRE(table.has_next());
        CHECK(table.set_width_twips(1440U));
        CHECK(table.set_column_width_twips(0U, 1440U));
        CHECK(table.set_cell_text(0U, 0U, "Paragraph anchored"));

        auto position = featherdoc::table_position{};
        position.horizontal_reference =
            featherdoc::table_position_horizontal_reference::column;
        position.horizontal_offset_twips = 720;
        position.vertical_reference =
            featherdoc::table_position_vertical_reference::paragraph;
        position.vertical_offset_twips = -360;
        CHECK(table.set_position(position));

        featherdoc::pdf::PdfDocumentAdapterOptions options;
        options.use_system_font_fallbacks = false;
        options.margin_left_points = 64.0;
        options.margin_top_points = 72.0;
        options.line_height_points = 16.0;
        options.paragraph_spacing_after_points = 6.0;

        const auto layout =
            featherdoc::pdf::layout_document_paragraphs(document, options);

        REQUIRE_EQ(layout.pages.size(), 1U);
        REQUIRE_EQ(layout.pages.front().rectangles.size(), 1U);
        REQUIRE_GE(layout.pages.front().text_runs.size(), 2U);
        CHECK_EQ(layout.pages.front().text_runs.front().text,
                 "Before positioned table");

        const auto &rectangle = layout.pages.front().rectangles.front();
        const auto expected_row_top =
            options.page_size.height_points - options.margin_top_points -
            options.line_height_points - options.paragraph_spacing_after_points +
            18.0;
        CHECK(rectangle.bounds.x_points ==
              doctest::Approx(options.margin_left_points + 36.0));
        CHECK(rectangle.bounds.y_points + rectangle.bounds.height_points ==
              doctest::Approx(expected_row_top));
    }
}

TEST_CASE("document PDF adapter maps table cell spacing") {
    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());

    auto table = document.append_table(2U, 2U);
    REQUIRE(table.has_next());
    CHECK(table.set_width_twips(2880U));
    CHECK(table.set_column_width_twips(0U, 1440U));
    CHECK(table.set_column_width_twips(1U, 1440U));
    CHECK(table.set_cell_spacing_twips(240U));
    CHECK(table.set_cell_text(0U, 0U, "A"));
    CHECK(table.set_cell_text(0U, 1U, "B"));
    CHECK(table.set_cell_text(1U, 0U, "C"));
    CHECK(table.set_cell_text(1U, 1U, "D"));

    const auto table_summary = document.inspect_table(0U);
    REQUIRE(table_summary.has_value());
    REQUIRE(table_summary->cell_spacing_twips.has_value());
    CHECK_EQ(*table_summary->cell_spacing_twips, 240U);

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.use_system_font_fallbacks = false;
    options.line_height_points = 14.0;

    const auto layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);

    REQUIRE_EQ(layout.pages.size(), 1U);
    REQUIRE_EQ(layout.pages.front().rectangles.size(), 4U);

    const auto &top_left = layout.pages.front().rectangles[0];
    const auto &top_right = layout.pages.front().rectangles[1];
    const auto &bottom_left = layout.pages.front().rectangles[2];

    CHECK(top_right.bounds.x_points ==
          doctest::Approx(top_left.bounds.x_points +
                          top_left.bounds.width_points + 12.0));
    CHECK(bottom_left.bounds.y_points + bottom_left.bounds.height_points ==
          doctest::Approx(top_left.bounds.y_points - 12.0));
}

TEST_CASE("document PDF adapter maps table and cell borders") {
    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());

    auto table = document.append_table(1U, 1U);
    REQUIRE(table.has_next());
    CHECK(table.set_width_twips(1440U));
    CHECK(table.set_column_width_twips(0U, 1440U));
    CHECK(table.set_border(
        featherdoc::table_border_edge::top,
        featherdoc::border_definition{featherdoc::border_style::single, 8U,
                                      "4472C4", 0U}));
    CHECK(table.set_border(
        featherdoc::table_border_edge::left,
        featherdoc::border_definition{featherdoc::border_style::single, 4U,
                                      "666666", 0U}));

    auto cell = table.find_cell(0U, 0U);
    REQUIRE(cell.has_value());
    CHECK(cell->set_text("Bordered"));
    CHECK(cell->set_border(
        featherdoc::cell_border_edge::top,
        featherdoc::border_definition{featherdoc::border_style::single, 16U,
                                      "C00000", 0U}));

    const auto table_summary = document.inspect_table(0U);
    REQUIRE(table_summary.has_value());
    REQUIRE(table_summary->borders.has_value());
    REQUIRE(table_summary->borders->left.has_value());
    CHECK_EQ(table_summary->borders->left->color, "666666");

    const auto cells = document.inspect_table_cells(0U);
    REQUIRE_EQ(cells.size(), 1U);
    REQUIRE(cells.front().border_top.has_value());
    CHECK_EQ(cells.front().border_top->size_eighth_points, 16U);
    CHECK_EQ(cells.front().border_top->color, "C00000");

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.use_system_font_fallbacks = false;

    const auto layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);

    REQUIRE_EQ(layout.pages.size(), 1U);
    REQUIRE_GE(layout.pages.front().lines.size(), 2U);

    const auto &top_border = layout.pages.front().lines[0];
    CHECK(top_border.line_width_points == doctest::Approx(2.0));
    CHECK(top_border.stroke_color.red == doctest::Approx(192.0 / 255.0));
    CHECK(top_border.stroke_color.green == doctest::Approx(0.0));
    CHECK(top_border.stroke_color.blue == doctest::Approx(0.0));

    const auto &left_border = layout.pages.front().lines[1];
    CHECK(left_border.line_width_points == doctest::Approx(0.5));
    CHECK(left_border.stroke_color.red == doctest::Approx(102.0 / 255.0));
    CHECK(left_border.stroke_color.green == doctest::Approx(102.0 / 255.0));
    CHECK(left_border.stroke_color.blue == doctest::Approx(102.0 / 255.0));
}

TEST_CASE("document PDF adapter maps dashed and dotted table borders") {
    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());

    auto table = document.append_table(1U, 2U);
    REQUIRE(table.has_next());
    CHECK(table.set_width_twips(2880U));
    CHECK(table.set_column_width_twips(0U, 1440U));
    CHECK(table.set_column_width_twips(1U, 1440U));
    CHECK(table.set_border(
        featherdoc::table_border_edge::inside_vertical,
        featherdoc::border_definition{featherdoc::border_style::dashed, 8U,
                                      "808080", 0U}));

    auto first_cell = table.find_cell(0U, 0U);
    REQUIRE(first_cell.has_value());
    CHECK(first_cell->set_text("Dashed"));

    auto second_cell = table.find_cell(0U, 1U);
    REQUIRE(second_cell.has_value());
    CHECK(second_cell->set_text("Dotted"));
    CHECK(second_cell->set_border(
        featherdoc::cell_border_edge::top,
        featherdoc::border_definition{featherdoc::border_style::dotted, 4U,
                                      "0000FF", 0U}));

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.use_system_font_fallbacks = false;

    const auto layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);

    REQUIRE_EQ(layout.pages.size(), 1U);
    auto dashed_count = 0U;
    auto dotted_count = 0U;
    for (const auto &line : layout.pages.front().lines) {
        if (line.line_width_points == doctest::Approx(1.0) &&
            line.dash_on_points == doctest::Approx(3.0) &&
            line.dash_off_points == doctest::Approx(2.0) &&
            line.line_cap == featherdoc::pdf::PdfLineCap::butt) {
            ++dashed_count;
        }
        if (line.line_width_points == doctest::Approx(0.5) &&
            line.dash_on_points == doctest::Approx(0.5) &&
            line.dash_off_points == doctest::Approx(1.0) &&
            line.line_cap == featherdoc::pdf::PdfLineCap::round) {
            ++dotted_count;
        }
    }

    CHECK_GE(dashed_count, 1U);
    CHECK_EQ(dotted_count, 1U);
}

TEST_CASE("document PDF adapter maps double table borders") {
    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());

    auto table = document.append_table(1U, 1U);
    REQUIRE(table.has_next());
    CHECK(table.set_width_twips(1440U));
    CHECK(table.set_column_width_twips(0U, 1440U));
    CHECK(table.set_border(
        featherdoc::table_border_edge::top,
        featherdoc::border_definition{featherdoc::border_style::double_line,
                                      12U, "1F4E79", 0U}));
    CHECK(table.set_cell_text(0U, 0U, "Double"));

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.use_system_font_fallbacks = false;

    const auto layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);

    REQUIRE_EQ(layout.pages.size(), 1U);
    REQUIRE_GE(layout.pages.front().lines.size(), 2U);

    const auto &outer_line = layout.pages.front().lines[0];
    const auto &inner_line = layout.pages.front().lines[1];
    CHECK(outer_line.line_width_points == doctest::Approx(0.5));
    CHECK(inner_line.line_width_points == doctest::Approx(0.5));
    CHECK(outer_line.start.y_points ==
          doctest::Approx(inner_line.start.y_points + 0.5));
    CHECK(outer_line.end.y_points ==
          doctest::Approx(inner_line.end.y_points + 0.5));
    CHECK(outer_line.stroke_color.red == doctest::Approx(31.0 / 255.0));
    CHECK(outer_line.stroke_color.green == doctest::Approx(78.0 / 255.0));
    CHECK(outer_line.stroke_color.blue == doctest::Approx(121.0 / 255.0));
}

TEST_CASE("document PDF adapter maps table row height and vertical alignment") {
    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());

    auto table = document.append_table(1U, 2U);
    REQUIRE(table.has_next());
    CHECK(table.set_width_twips(2880U));
    CHECK(table.set_column_width_twips(0U, 1440U));
    CHECK(table.set_column_width_twips(1U, 1440U));

    auto row = table.rows();
    REQUIRE(row.has_next());
    CHECK(row.set_height_twips(1200U, featherdoc::row_height_rule::exact));
    CHECK(row.set_cant_split());
    CHECK(row.set_repeats_header());

    auto left_cell = table.find_cell(0U, 0U);
    REQUIRE(left_cell.has_value());
    CHECK(left_cell->set_text("Center"));
    CHECK(left_cell->set_vertical_alignment(
        featherdoc::cell_vertical_alignment::center));

    auto right_cell = table.find_cell(0U, 1U);
    REQUIRE(right_cell.has_value());
    CHECK(right_cell->set_text("Bottom"));
    CHECK(right_cell->set_vertical_alignment(
        featherdoc::cell_vertical_alignment::bottom));

    const auto table_summary = document.inspect_table(0U);
    REQUIRE(table_summary.has_value());
    REQUIRE_EQ(table_summary->row_height_twips.size(), 1U);
    CHECK_EQ(table_summary->row_height_twips[0].value_or(0U), 1200U);
    REQUIRE_EQ(table_summary->row_height_rules.size(), 1U);
    REQUIRE(table_summary->row_height_rules[0].has_value());
    CHECK_EQ(*table_summary->row_height_rules[0],
             featherdoc::row_height_rule::exact);
    REQUIRE_EQ(table_summary->row_cant_split.size(), 1U);
    CHECK(table_summary->row_cant_split[0]);
    REQUIRE_EQ(table_summary->row_repeats_header.size(), 1U);
    CHECK(table_summary->row_repeats_header[0]);

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.use_system_font_fallbacks = false;
    options.line_height_points = 14.0;

    const auto layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);

    REQUIRE_EQ(layout.pages.size(), 1U);
    REQUIRE_EQ(layout.pages.front().rectangles.size(), 2U);
    REQUIRE_EQ(layout.pages.front().text_runs.size(), 2U);

    const auto &left_rectangle = layout.pages.front().rectangles[0];
    CHECK(left_rectangle.bounds.height_points == doctest::Approx(60.0));

    const auto &center_text = layout.pages.front().text_runs[0];
    CHECK_EQ(center_text.text, "Center");
    const auto row_top =
        left_rectangle.bounds.y_points + left_rectangle.bounds.height_points;
    CHECK(center_text.baseline_origin.y_points ==
          doctest::Approx(row_top - 23.0 - options.font_size_points));

    const auto &bottom_text = layout.pages.front().text_runs[1];
    CHECK_EQ(bottom_text.text, "Bottom");
    CHECK(bottom_text.baseline_origin.y_points ==
          doctest::Approx(row_top - 42.0 - options.font_size_points));
}

TEST_CASE("document PDF adapter maps table cell text direction") {
    const auto output_path = std::filesystem::current_path() /
                             "featherdoc-adapter-table-text-direction.pdf";

    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());

    auto table = document.append_table(1U, 1U);
    REQUIRE(table.has_next());
    CHECK(table.set_width_twips(2880U));
    CHECK(table.set_column_width_twips(0U, 2880U));

    auto row = table.rows();
    REQUIRE(row.has_next());
    CHECK(row.set_height_twips(1800U, featherdoc::row_height_rule::exact));

    auto cell = table.find_cell(0U, 0U);
    REQUIRE(cell.has_value());
    CHECK(cell->set_fill_color("EAF3F8"));
    CHECK(cell->set_text_direction(
        featherdoc::cell_text_direction::top_to_bottom_right_to_left));
    auto paragraph = cell->paragraphs();
    REQUIRE(paragraph.has_next());
    REQUIRE(paragraph.add_run("Vert").has_next());
    REQUIRE(paragraph.add_run("ical", featherdoc::formatting_flag::bold)
                .has_next());

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.use_system_font_fallbacks = false;
    options.line_height_points = 14.0;

    const auto layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);

    REQUIRE_EQ(layout.pages.size(), 1U);
    REQUIRE_EQ(layout.pages.front().rectangles.size(), 1U);

    const auto &cell_rectangle = layout.pages.front().rectangles.front();
    std::vector<const featherdoc::pdf::PdfTextRun *> vertical_text_runs;
    for (const auto &text_run : layout.pages.front().text_runs) {
        if (text_run.text == "Vert" || text_run.text == "ical") {
            vertical_text_runs.push_back(&text_run);
        }
    }

    REQUIRE_EQ(vertical_text_runs.size(), 2U);
    CHECK_EQ(vertical_text_runs[0]->rotation_degrees, doctest::Approx(90.0));
    CHECK_EQ(vertical_text_runs[1]->rotation_degrees, doctest::Approx(90.0));
    CHECK(vertical_text_runs[1]->bold);
    CHECK_EQ(vertical_text_runs[1]->baseline_origin.x_points,
             doctest::Approx(vertical_text_runs[0]->baseline_origin.x_points));
    CHECK_GT(vertical_text_runs[1]->baseline_origin.y_points,
             vertical_text_runs[0]->baseline_origin.y_points + 12.0);
    CHECK_GT(vertical_text_runs[0]->baseline_origin.x_points,
             cell_rectangle.bounds.x_points +
                 cell_rectangle.bounds.width_points - 8.0);
    CHECK_GT(vertical_text_runs[0]->baseline_origin.y_points,
             cell_rectangle.bounds.y_points + 3.0);
    CHECK_LT(vertical_text_runs[0]->baseline_origin.y_points,
             cell_rectangle.bounds.y_points +
                 cell_rectangle.bounds.height_points - 16.0);

    featherdoc::pdf::PdfioGenerator generator;
    const auto write_result = generator.write(
        layout, output_path, featherdoc::pdf::PdfWriterOptions{});
    REQUIRE_MESSAGE(write_result.success, write_result.error_message);
    CHECK_GT(write_result.bytes_written, 0U);

#if defined(FEATHERDOC_BUILD_PDF_IMPORT)
    featherdoc::pdf::PdfiumParser parser;
    const auto parse_result = parser.parse(output_path, {});
    REQUIRE_MESSAGE(parse_result.success, parse_result.error_message);

    const auto extracted_text = collect_text(parse_result.document);
    CHECK_NE(extracted_text.find("Vert"), std::string::npos);
    CHECK_NE(extracted_text.find("ical"), std::string::npos);
#endif
}

#if defined(FEATHERDOC_BUILD_PDF_IMPORT)
TEST_CASE("document PDF adapter table output round-trips through PDFium") {
    const auto output_path = std::filesystem::current_path() /
                             "featherdoc-adapter-table-roundtrip.pdf";

    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());
    auto intro = document.paragraphs();
    REQUIRE(intro.has_next());
    CHECK(intro.set_text("Invoice"));

    auto table = document.append_table(2U, 2U);
    REQUIRE(table.has_next());
    CHECK(table.set_cell_text(0U, 0U, "Item"));
    CHECK(table.set_cell_text(0U, 1U, "Price"));
    CHECK(table.set_cell_text(1U, 0U, "Design"));
    CHECK(table.set_cell_text(1U, 1U, "1200"));

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.metadata.title = "FeatherDoc adapter table roundtrip";
    options.use_system_font_fallbacks = false;

    const auto layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);

    featherdoc::pdf::PdfioGenerator generator;
    const auto write_result = generator.write(
        layout, output_path, featherdoc::pdf::PdfWriterOptions{});
    REQUIRE_MESSAGE(write_result.success, write_result.error_message);

    featherdoc::pdf::PdfiumParser parser;
    const auto parse_result = parser.parse(output_path, {});
    REQUIRE_MESSAGE(parse_result.success, parse_result.error_message);

    const auto extracted_text = collect_text(parse_result.document);
    CHECK_NE(extracted_text.find("Invoice"), std::string::npos);
    CHECK_NE(extracted_text.find("Item"), std::string::npos);
    CHECK_NE(extracted_text.find("Price"), std::string::npos);
    CHECK_NE(extracted_text.find("Design"), std::string::npos);
    CHECK_NE(extracted_text.find("1200"), std::string::npos);
}

TEST_CASE("document PDF adapter table pagination round-trips through PDFium") {
    const auto output_path = std::filesystem::current_path() /
                             "featherdoc-adapter-table-pagination-roundtrip.pdf";

    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());

    auto table = document.append_table(6U, 2U);
    REQUIRE(table.has_next());
    CHECK(table.set_width_twips(2880U));
    CHECK(table.set_column_width_twips(0U, 1440U));
    CHECK(table.set_column_width_twips(1U, 1440U));
    auto position = featherdoc::table_position{};
    position.horizontal_reference =
        featherdoc::table_position_horizontal_reference::margin;
    position.horizontal_offset_twips = 0;
    position.vertical_reference =
        featherdoc::table_position_vertical_reference::page;
    position.vertical_offset_twips = 720;
    CHECK(table.set_position(position));
    CHECK(table.set_cell_text(0U, 0U, "Header"));
    CHECK(table.set_cell_text(0U, 1U, "Amount"));
    for (std::size_t row_index = 1U; row_index < 6U; ++row_index) {
        CHECK(table.set_cell_text(row_index, 0U,
                                  "Row " + std::to_string(row_index)));
        CHECK(table.set_cell_text(row_index, 1U,
                                  std::to_string(row_index * 10U)));
    }

    auto row = table.rows();
    for (std::size_t row_index = 0U; row_index < 6U; ++row_index) {
        REQUIRE(row.has_next());
        CHECK(row.set_height_twips(720U, featherdoc::row_height_rule::exact));
        if (row_index == 0U) {
            CHECK(row.set_repeats_header());
        }
        row.next();
    }

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.metadata.title = "FeatherDoc adapter table pagination roundtrip";
    options.use_system_font_fallbacks = false;
    options.page_size = featherdoc::pdf::PdfPageSize{300.0, 220.0};
    options.margin_left_points = 36.0;
    options.margin_right_points = 36.0;
    options.margin_top_points = 36.0;
    options.margin_bottom_points = 36.0;
    options.line_height_points = 14.0;
    options.paragraph_spacing_after_points = 0.0;

    const auto layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);
    REQUIRE_GE(layout.pages.size(), 2U);

    featherdoc::pdf::PdfioGenerator generator;
    const auto write_result = generator.write(
        layout, output_path, featherdoc::pdf::PdfWriterOptions{});
    REQUIRE_MESSAGE(write_result.success, write_result.error_message);

    featherdoc::pdf::PdfiumParser parser;
    const auto parse_result = parser.parse(output_path, {});
    REQUIRE_MESSAGE(parse_result.success, parse_result.error_message);
    REQUIRE_GE(parse_result.document.pages.size(), 2U);

    const auto first_page_text = collect_text(parse_result.document.pages[0]);
    const auto second_page_text = collect_text(parse_result.document.pages[1]);
    CHECK_NE(first_page_text.find("Header"), std::string::npos);
    CHECK_NE(second_page_text.find("Header"), std::string::npos);
    CHECK_NE(second_page_text.find("Row "), std::string::npos);
}
#endif
