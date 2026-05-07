#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include <featherdoc/pdf/pdf_text_metrics.hpp>

#include <cstdlib>
#include <filesystem>
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

[[nodiscard]] std::vector<std::filesystem::path> candidate_proportional_fonts() {
    std::vector<std::filesystem::path> candidates;

    if (const auto configured =
            environment_value("FEATHERDOC_TEST_PROPORTIONAL_FONT");
        !configured.empty()) {
        candidates.emplace_back(configured);
    }

#if defined(_WIN32)
    candidates.emplace_back("C:/Windows/Fonts/arial.ttf");
    candidates.emplace_back("C:/Windows/Fonts/NotoSans-Regular.ttf");
    candidates.emplace_back("C:/Windows/Fonts/Deng.ttf");
#endif

    return candidates;
}

[[nodiscard]] std::filesystem::path find_proportional_font() {
    for (const auto &candidate : candidate_proportional_fonts()) {
        if (std::filesystem::exists(candidate)) {
            return candidate;
        }
    }
    return {};
}

} // namespace

TEST_CASE("text metrics treat CJK text as wider than short latin text") {
    const auto latin_width =
        featherdoc::pdf::measure_text_width_points("AB", 12.0, "Helvetica");
    const auto cjk_width = featherdoc::pdf::measure_text_width_points(
        utf8_from_u8(u8"中文"), 12.0, "Helvetica");

    CHECK_GT(cjk_width, latin_width);
}

TEST_CASE("text metrics keep monospaced families consistent") {
    const auto first = featherdoc::pdf::measure_text_width_points(
        "iiii", 12.0, "Courier New");
    const auto second = featherdoc::pdf::measure_text_width_points(
        "WWWW", 12.0, "Courier New");

    CHECK(first == doctest::Approx(second));
}

TEST_CASE("text metrics estimate a usable line height") {
    const auto line_height =
        featherdoc::pdf::estimate_line_height_points(12.0, "Helvetica");

    CHECK_GT(line_height, 12.0);
}

TEST_CASE("text metrics use FreeType metrics when a font file is provided") {
    const auto font_path = find_proportional_font();
    if (font_path.empty()) {
        MESSAGE("skipping FreeType metrics test: set "
                "FEATHERDOC_TEST_PROPORTIONAL_FONT to run it");
        return;
    }

    const featherdoc::pdf::PdfTextMetricsOptions options{
        "Proportional Test Font",
        font_path,
    };

    const auto narrow =
        featherdoc::pdf::measure_text_width_points("iiii", 12.0, options);
    const auto wide =
        featherdoc::pdf::measure_text_width_points("WWWW", 12.0, options);

    CHECK_GT(wide, narrow);
    CHECK_GT(narrow, 1.0);
    CHECK_GT(wide, 12.0);

    const auto line_height =
        featherdoc::pdf::estimate_line_height_points(12.0, options);
    CHECK_GT(line_height, 0.0);
}

TEST_CASE("text metrics fall back when a font file cannot be loaded") {
    const featherdoc::pdf::PdfTextMetricsOptions options{
        "Courier New",
        "missing-font-file.ttf",
    };

    const auto first =
        featherdoc::pdf::measure_text_width_points("iiii", 12.0, options);
    const auto second =
        featherdoc::pdf::measure_text_width_points("WWWW", 12.0, options);

    CHECK(first == doctest::Approx(second));
}
