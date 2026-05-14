#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include <featherdoc/pdf/pdf_text_shaper.hpp>

#include <cstdlib>
#include <filesystem>
#include <numeric>
#include <string>
#include <string_view>
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

} // namespace

TEST_CASE("text shaper returns an empty run for empty text") {
    const auto run = featherdoc::pdf::shape_pdf_text(
        "", featherdoc::pdf::PdfTextShaperOptions{"missing-font.ttf", 12.0});

    CHECK(run.glyphs.empty());
    CHECK(run.error_message.empty());
    CHECK_FALSE(run.used_harfbuzz);
}

TEST_CASE("text shaper rejects invalid font size") {
    const auto run = featherdoc::pdf::shape_pdf_text(
        "office", featherdoc::pdf::PdfTextShaperOptions{"missing-font.ttf", 0.0});

    CHECK(run.glyphs.empty());
    CHECK_FALSE(run.error_message.empty());
    CHECK(run.error_message.find("Font size") != std::string::npos);
}

TEST_CASE("text shaper reports missing font paths") {
    const auto run = featherdoc::pdf::shape_pdf_text(
        "office",
        featherdoc::pdf::PdfTextShaperOptions{"missing-font-file.ttf", 12.0});

    CHECK(run.glyphs.empty());
    CHECK_FALSE(run.error_message.empty());
    CHECK(run.error_message.find("does not exist") != std::string::npos);
}

TEST_CASE("text shaper produces HarfBuzz glyph positions when available") {
    const auto font_path = first_existing_path(candidate_latin_fonts());
    if (font_path.empty()) {
        MESSAGE("skipping HarfBuzz shaper test: set "
                "FEATHERDOC_TEST_PROPORTIONAL_FONT to run it");
        return;
    }

    constexpr auto text = "office";
    const featherdoc::pdf::PdfTextShaperOptions options{font_path, 12.0};
    const auto run = featherdoc::pdf::shape_pdf_text(text, options);

    CHECK_EQ(run.text, text);
    CHECK_EQ(run.font_file_path, font_path);
    CHECK(run.font_size_points == doctest::Approx(12.0));

    if (!featherdoc::pdf::pdf_text_shaper_has_harfbuzz()) {
        CHECK_FALSE(run.used_harfbuzz);
        CHECK(run.glyphs.empty());
        CHECK(run.error_message.find("not enabled") != std::string::npos);
        return;
    }

    CHECK(run.used_harfbuzz);
    CHECK(run.error_message.empty());
    CHECK_EQ(run.direction,
             featherdoc::pdf::PdfGlyphDirection::left_to_right);
    REQUIRE_FALSE(run.glyphs.empty());

    const auto total_advance = std::accumulate(
        run.glyphs.begin(), run.glyphs.end(), 0.0,
        [](double total, const featherdoc::pdf::PdfGlyphPosition &glyph) {
            return total + glyph.x_advance_points;
        });
    CHECK_GT(total_advance, 1.0);

    bool has_nonzero_glyph = false;
    for (const auto &glyph : run.glyphs) {
        CHECK_LT(glyph.cluster, std::char_traits<char>::length(text));
        has_nonzero_glyph = has_nonzero_glyph || glyph.glyph_id != 0U;
    }
    CHECK(has_nonzero_glyph);
}

TEST_CASE("text shaper records right-to-left direction when available") {
    const auto font_path = first_existing_path(candidate_rtl_fonts());
    if (font_path.empty()) {
        MESSAGE("skipping HarfBuzz RTL direction test: set "
                "FEATHERDOC_TEST_RTL_FONT to run it");
        return;
    }

    const auto text = utf8_from_u8(u8"\u05E9\u05DC\u05D5\u05DD");
    const featherdoc::pdf::PdfTextShaperOptions options{font_path, 12.0};
    const auto run = featherdoc::pdf::shape_pdf_text(text, options);

    CHECK_EQ(run.text, text);
    CHECK_EQ(run.font_file_path, font_path);

    if (!featherdoc::pdf::pdf_text_shaper_has_harfbuzz()) {
        CHECK_FALSE(run.used_harfbuzz);
        CHECK(run.glyphs.empty());
        CHECK(run.error_message.find("not enabled") != std::string::npos);
        return;
    }

    CHECK(run.used_harfbuzz);
    CHECK(run.error_message.empty());
    CHECK_EQ(run.direction,
             featherdoc::pdf::PdfGlyphDirection::right_to_left);
    REQUIRE_FALSE(run.glyphs.empty());
}
