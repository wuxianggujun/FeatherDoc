#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include <array>
#include <featherdoc/pdf/pdf_font_resolver.hpp>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <cstdint>
#include <optional>
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
    stream << "featherdoc test font mapping fixture";
    return path;
}

[[nodiscard]] std::optional<bool>
font_has_glyph(const std::filesystem::path &font_path, char32_t codepoint) {
    if (!std::filesystem::exists(font_path)) {
        return std::nullopt;
    }

    FT_Library library = nullptr;
    if (FT_Init_FreeType(&library) != 0 || library == nullptr) {
        return std::nullopt;
    }

    FT_Face face = nullptr;
    const auto font_path_string = font_path.string();
    if (FT_New_Face(library, font_path_string.c_str(), 0, &face) != 0 ||
        face == nullptr) {
        FT_Done_FreeType(library);
        return std::nullopt;
    }

    const auto glyph_index = FT_Get_Char_Index(face, static_cast<FT_ULong>(codepoint));

    FT_Done_Face(face);
    FT_Done_FreeType(library);
    return glyph_index != 0U;
}

[[nodiscard]] std::string utf8_from_codepoint(char32_t codepoint) {
    std::string text;
    if (codepoint <= 0x7FU) {
        text.push_back(static_cast<char>(codepoint));
    } else if (codepoint <= 0x7FFU) {
        text.push_back(static_cast<char>(0xC0U | (codepoint >> 6U)));
        text.push_back(static_cast<char>(0x80U | (codepoint & 0x3FU)));
    } else if (codepoint <= 0xFFFFU) {
        text.push_back(static_cast<char>(0xE0U | (codepoint >> 12U)));
        text.push_back(
            static_cast<char>(0x80U | ((codepoint >> 6U) & 0x3FU)));
        text.push_back(static_cast<char>(0x80U | (codepoint & 0x3FU)));
    } else {
        text.push_back(static_cast<char>(0xF0U | (codepoint >> 18U)));
        text.push_back(static_cast<char>(0x80U | ((codepoint >> 12U) & 0x3FU)));
        text.push_back(static_cast<char>(0x80U | ((codepoint >> 6U) & 0x3FU)));
        text.push_back(static_cast<char>(0x80U | (codepoint & 0x3FU)));
    }
    return text;
}

} // namespace

TEST_CASE("font resolver detects Unicode and CJK text") {
    CHECK_FALSE(featherdoc::pdf::pdf_text_requires_unicode("ABC 123"));
    CHECK(featherdoc::pdf::pdf_text_requires_unicode(utf8_from_u8(u8"中文")));

    CHECK_FALSE(featherdoc::pdf::pdf_text_contains_cjk("ABC 123"));
    CHECK(featherdoc::pdf::pdf_text_contains_cjk(utf8_from_u8(u8"中文")));
    CHECK(featherdoc::pdf::pdf_text_contains_cjk(utf8_from_u8(u8"かな")));
    CHECK(featherdoc::pdf::pdf_text_contains_cjk(utf8_from_u8(u8"カナ")));
    CHECK(featherdoc::pdf::pdf_text_contains_cjk(utf8_from_u8(u8"한글")));
    CHECK(featherdoc::pdf::pdf_text_contains_cjk(utf8_from_u8(u8"㊟")));
    CHECK(featherdoc::pdf::pdf_text_contains_cjk(utf8_from_u8(u8"㊣")));
}

TEST_CASE("font resolver prefers explicit mappings for CJK text") {
    const auto cjk_font = first_existing_path(candidate_cjk_fonts());
    if (cjk_font.empty()) {
        MESSAGE(
            "skipping resolver CJK mapping test: set FEATHERDOC_TEST_CJK_FONT");
        return;
    }

    featherdoc::pdf::PdfFontResolver resolver(
        featherdoc::pdf::PdfFontResolverOptions{
            {featherdoc::pdf::PdfFontMapping{"Unit CJK", cjk_font}},
            {},
            {},
            false,
        });

    const auto resolved = resolver.resolve("Unit Latin", "Unit CJK",
                                           utf8_from_u8(u8"合同中文测试"));

    CHECK_EQ(resolved.font_family, "Unit CJK");
    CHECK_EQ(resolved.font_file_path, cjk_font);
    CHECK(resolved.unicode);
}

TEST_CASE(
    "font resolver prefers explicit East Asia mappings for East Asian scripts") {
    const auto latin_font = make_temp_font_file("featherdoc-resolver-latin-eastasia.ttf");
    const auto east_asia_font =
        make_temp_font_file("featherdoc-resolver-eastasia.ttf");

    featherdoc::pdf::PdfFontResolver resolver(
        featherdoc::pdf::PdfFontResolverOptions{
            {featherdoc::pdf::PdfFontMapping{"Unit Latin", latin_font},
             featherdoc::pdf::PdfFontMapping{"Unit CJK", east_asia_font}},
            {},
            {},
            false,
        });

    for (const auto text : {utf8_from_u8(u8"かな"), utf8_from_u8(u8"カナ"),
                             utf8_from_u8(u8"한글"), utf8_from_u8(u8"㊟"),
                             utf8_from_u8(u8"㊣")}) {
        const auto resolved =
            resolver.resolve("Unit Latin", "Unit CJK", text);
        CHECK_EQ(resolved.font_family, "Unit CJK");
        CHECK_EQ(resolved.font_file_path, east_asia_font);
        CHECK(resolved.unicode);
    }
}

TEST_CASE(
    "font resolver falls back to east Asia fonts when a unicode glyph is missing") {
    const auto latin_font = first_existing_path(candidate_latin_fonts());
    const auto cjk_font = first_existing_path(candidate_cjk_fonts());
    if (latin_font.empty() || cjk_font.empty()) {
        MESSAGE("skipping glyph fallback test: configure test Latin/CJK fonts");
        return;
    }

    const std::array<char32_t, 8U> candidates{
        0x203BU, 0x2116U, 0x2121U, 0x2103U, 0x00A9U, 0x20A9U, 0x20A7U,
        0x20A0U,
    };

    std::optional<char32_t> selected_codepoint;
    for (const auto codepoint : candidates) {
        const auto latin_support = font_has_glyph(latin_font, codepoint);
        const auto cjk_support = font_has_glyph(cjk_font, codepoint);
        if (latin_support.has_value() && cjk_support.has_value() &&
            !*latin_support && *cjk_support) {
            selected_codepoint = codepoint;
            break;
        }
    }

    if (!selected_codepoint.has_value()) {
        MESSAGE("skipping glyph fallback test: no portable East Asia symbol "
                "candidate found");
        return;
    }

    featherdoc::pdf::PdfFontResolver resolver(
        featherdoc::pdf::PdfFontResolverOptions{
            {featherdoc::pdf::PdfFontMapping{"Unit Latin", latin_font},
             featherdoc::pdf::PdfFontMapping{"Unit CJK", cjk_font}},
            {},
            {},
            false,
        });

    const auto resolved = resolver.resolve(
        "Unit Latin", "Unit CJK", utf8_from_codepoint(*selected_codepoint));

    CHECK_EQ(resolved.font_family, "Unit CJK");
    CHECK_EQ(resolved.font_file_path, cjk_font);
    CHECK(resolved.unicode);
}

TEST_CASE("font resolver keeps Latin mappings non-Unicode") {
    const auto latin_font = first_existing_path(candidate_latin_fonts());
    if (latin_font.empty()) {
        MESSAGE("skipping resolver Latin mapping test: set "
                "FEATHERDOC_TEST_PROPORTIONAL_FONT");
        return;
    }

    featherdoc::pdf::PdfFontResolver resolver(
        featherdoc::pdf::PdfFontResolverOptions{
            {featherdoc::pdf::PdfFontMapping{"Unit Latin", latin_font}},
            {},
            {},
            false,
        });

    const auto resolved =
        resolver.resolve("Unit Latin", {}, "Contract ABC 123");

    CHECK_EQ(resolved.font_family, "Unit Latin");
    CHECK_EQ(resolved.font_file_path, latin_font);
    CHECK_FALSE(resolved.unicode);
}

TEST_CASE("font resolver prefers style-specific explicit mappings") {
    const auto regular_font =
        make_temp_font_file("featherdoc-resolver-regular.ttf");
    const auto bold_font = make_temp_font_file("featherdoc-resolver-bold.ttf");
    const auto italic_font =
        make_temp_font_file("featherdoc-resolver-italic.ttf");
    const auto bold_italic_font =
        make_temp_font_file("featherdoc-resolver-bold-italic.ttf");

    featherdoc::pdf::PdfFontResolver resolver(
        featherdoc::pdf::PdfFontResolverOptions{
            {featherdoc::pdf::PdfFontMapping{"Unit Style", regular_font},
             featherdoc::pdf::PdfFontMapping{"Unit Style", bold_font, true,
                                             false},
             featherdoc::pdf::PdfFontMapping{"Unit Style", italic_font, false,
                                             true},
             featherdoc::pdf::PdfFontMapping{"Unit Style",
                                             bold_italic_font, true, true}},
            {},
            {},
            false,
        });

    CHECK_EQ(resolver.resolve("Unit Style", {}, "regular", false, false)
                 .font_file_path,
             regular_font);
    CHECK_EQ(
        resolver.resolve("Unit Style", {}, "bold", true, false).font_file_path,
        bold_font);
    CHECK_EQ(resolver.resolve("Unit Style", {}, "italic", false, true)
                 .font_file_path,
             italic_font);
    CHECK_EQ(resolver.resolve("Unit Style", {}, "bold italic", true, true)
                 .font_file_path,
             bold_italic_font);
}

TEST_CASE("font resolver prefers style-specific explicit mappings for CJK text") {
    const auto regular_font =
        make_temp_font_file("featherdoc-resolver-cjk-regular.ttf");
    const auto bold_font =
        make_temp_font_file("featherdoc-resolver-cjk-bold.ttf");
    const auto italic_font =
        make_temp_font_file("featherdoc-resolver-cjk-italic.ttf");
    const auto bold_italic_font =
        make_temp_font_file("featherdoc-resolver-cjk-bold-italic.ttf");

    featherdoc::pdf::PdfFontResolver resolver(
        featherdoc::pdf::PdfFontResolverOptions{
            {featherdoc::pdf::PdfFontMapping{"Unit CJK", regular_font},
             featherdoc::pdf::PdfFontMapping{"Unit CJK", bold_font, true,
                                             false},
             featherdoc::pdf::PdfFontMapping{"Unit CJK", italic_font, false,
                                             true},
             featherdoc::pdf::PdfFontMapping{"Unit CJK",
                                             bold_italic_font, true, true}},
            {},
            {},
            false,
        });

    const auto text = utf8_from_u8(u8"合同中文测试");

    const auto regular_resolved = resolver.resolve("Unit Latin", "Unit CJK",
                                                   text);
    CHECK_EQ(regular_resolved.font_family, "Unit CJK");
    CHECK_EQ(regular_resolved.font_file_path, regular_font);
    CHECK(regular_resolved.unicode);

    const auto bold_resolved =
        resolver.resolve("Unit Latin", "Unit CJK", text, true, false);
    CHECK_EQ(bold_resolved.font_family, "Unit CJK");
    CHECK_EQ(bold_resolved.font_file_path, bold_font);
    CHECK(bold_resolved.unicode);

    const auto italic_resolved =
        resolver.resolve("Unit Latin", "Unit CJK", text, false, true);
    CHECK_EQ(italic_resolved.font_family, "Unit CJK");
    CHECK_EQ(italic_resolved.font_file_path, italic_font);
    CHECK(italic_resolved.unicode);

    const auto bold_italic_resolved =
        resolver.resolve("Unit Latin", "Unit CJK", text, true, true);
    CHECK_EQ(bold_italic_resolved.font_family, "Unit CJK");
    CHECK_EQ(bold_italic_resolved.font_file_path, bold_italic_font);
    CHECK(bold_italic_resolved.unicode);
}

TEST_CASE("font resolver falls back to regular mapping when style variant is "
          "missing") {
    const auto regular_font =
        make_temp_font_file("featherdoc-resolver-regular-fallback.ttf");

    featherdoc::pdf::PdfFontResolver resolver({
        {featherdoc::pdf::PdfFontMapping{"Unit Style", regular_font}},
        {},
        {},
        false,
    });

    const auto resolved =
        resolver.resolve("Unit Style", {}, "styled fallback", true, true);

    CHECK_EQ(resolved.font_file_path, regular_font);
}

TEST_CASE("font resolver falls back to regular CJK mapping when style variant "
          "is missing") {
    const auto regular_font =
        make_temp_font_file("featherdoc-resolver-cjk-regular-fallback.ttf");

    featherdoc::pdf::PdfFontResolver resolver({
        {featherdoc::pdf::PdfFontMapping{"Unit CJK", regular_font}},
        {},
        {},
        false,
    });

    const auto resolved = resolver.resolve("Unit Latin", "Unit CJK",
                                           utf8_from_u8(u8"中文"), true, true);

    CHECK_EQ(resolved.font_family, "Unit CJK");
    CHECK_EQ(resolved.font_file_path, regular_font);
    CHECK(resolved.unicode);
}

TEST_CASE("font resolver falls back to configured default font file path") {
    const auto default_font =
        make_temp_font_file("featherdoc-resolver-default-font.ttf");

    featherdoc::pdf::PdfFontResolver resolver({
        {},
        default_font,
        {},
        false,
    });

    const auto resolved = resolver.resolve("Unit Latin", {}, "Contract ABC");

    CHECK_EQ(resolved.font_family, "Unit Latin");
    CHECK_EQ(resolved.font_file_path, default_font);
    CHECK_FALSE(resolved.unicode);
}

TEST_CASE("font resolver ignores missing explicit mapping before default font") {
    const auto default_font =
        make_temp_font_file("featherdoc-resolver-missing-mapping-default.ttf");
    const auto missing_font =
        std::filesystem::current_path() / "featherdoc-resolver-missing.ttf";

    featherdoc::pdf::PdfFontResolver resolver({
        {featherdoc::pdf::PdfFontMapping{"Unit Missing", missing_font}},
        default_font,
        {},
        false,
    });

    const auto resolved = resolver.resolve("Unit Missing", {}, "Contract ABC");

    CHECK_EQ(resolved.font_family, "Unit Missing");
    CHECK_EQ(resolved.font_file_path, default_font);
    CHECK_FALSE(resolved.unicode);
}

TEST_CASE("font resolver falls back to configured default CJK font file path") {
    const auto default_cjk_font =
        make_temp_font_file("featherdoc-resolver-default-cjk-font.ttf");

    featherdoc::pdf::PdfFontResolver resolver({
        {},
        {},
        default_cjk_font,
        false,
    });

    const auto resolved =
        resolver.resolve("Unit Latin", "Missing CJK", utf8_from_u8(u8"中文"));

    CHECK_EQ(resolved.font_family, "Missing CJK");
    CHECK_EQ(resolved.font_file_path, default_cjk_font);
    CHECK(resolved.unicode);
}

TEST_CASE("font resolver reports no font path when CJK fallback is unavailable") {
    featherdoc::pdf::PdfFontResolver resolver({
        {},
        {},
        {},
        false,
    });

    const auto resolved =
        resolver.resolve("Unit Latin", "Missing CJK", utf8_from_u8(u8"中文"));

    CHECK_EQ(resolved.font_family, "Missing CJK");
    CHECK(resolved.font_file_path.empty());
    CHECK(resolved.unicode);
}
