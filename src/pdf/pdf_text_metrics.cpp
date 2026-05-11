#include <featherdoc/pdf/pdf_text_metrics.hpp>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <initializer_list>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>

#include <ft2build.h>
#include FT_FREETYPE_H

namespace featherdoc::pdf {
namespace {

struct DecodedCodepoint {
    char32_t value{0U};
    std::size_t size{1U};
};

struct FreeTypeLibrary {
    FT_Library library{nullptr};

    FreeTypeLibrary() {
        if (FT_Init_FreeType(&library) != 0) {
            library = nullptr;
        }
    }

    ~FreeTypeLibrary() {
        if (library != nullptr) {
            FT_Done_FreeType(library);
        }
    }

    FreeTypeLibrary(const FreeTypeLibrary &) = delete;
    FreeTypeLibrary &operator=(const FreeTypeLibrary &) = delete;

    [[nodiscard]] bool valid() const noexcept { return library != nullptr; }
};

using FreeTypeLibraryPtr = std::shared_ptr<FreeTypeLibrary>;

struct FreeTypeFaceDeleter {
    void operator()(FT_Face face) const noexcept {
        if (face != nullptr) {
            FT_Done_Face(face);
        }
    }
};

using FreeTypeFacePtr =
    std::unique_ptr<std::remove_pointer_t<FT_Face>, FreeTypeFaceDeleter>;

struct FreeTypeMetrics {
    FreeTypeLibraryPtr library;
    FreeTypeFacePtr face;
};

[[nodiscard]] std::size_t utf8_codepoint_size(unsigned char lead) noexcept {
    if ((lead & 0x80U) == 0U) {
        return 1U;
    }
    if ((lead & 0xE0U) == 0xC0U) {
        return 2U;
    }
    if ((lead & 0xF0U) == 0xE0U) {
        return 3U;
    }
    if ((lead & 0xF8U) == 0xF0U) {
        return 4U;
    }
    return 1U;
}

[[nodiscard]] DecodedCodepoint decode_utf8_codepoint(std::string_view text,
                                                     std::size_t index) noexcept {
    const auto lead = static_cast<unsigned char>(text[index]);
    const auto remaining = text.size() - index;
    const auto expected_size = std::min(utf8_codepoint_size(lead), remaining);

    if (expected_size == 1U) {
        return {static_cast<char32_t>(lead), 1U};
    }

    unsigned char lead_mask = 0x7FU;
    if (expected_size == 2U) {
        lead_mask = 0x1FU;
    } else if (expected_size == 3U) {
        lead_mask = 0x0FU;
    } else if (expected_size == 4U) {
        lead_mask = 0x07U;
    }

    auto value = static_cast<char32_t>(lead & lead_mask);
    for (std::size_t offset = 1U; offset < expected_size; ++offset) {
        const auto byte = static_cast<unsigned char>(text[index + offset]);
        if ((byte & 0xC0U) != 0x80U) {
            return {static_cast<char32_t>(lead), 1U};
        }
        value = static_cast<char32_t>((value << 6U) | (byte & 0x3FU));
    }

    return {value, expected_size};
}

[[nodiscard]] std::string lower_ascii_copy(std::string_view text) {
    std::string lowered(text.begin(), text.end());
    std::transform(lowered.begin(), lowered.end(), lowered.begin(),
                   [](unsigned char value) { return static_cast<char>(std::tolower(value)); });
    return lowered;
}

[[nodiscard]] bool contains_any(std::string_view haystack,
                                std::initializer_list<std::string_view> needles) {
    for (const auto needle : needles) {
        if (haystack.find(needle) != std::string_view::npos) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] bool is_monospaced_family(std::string_view font_family) {
    if (font_family.empty()) {
        return false;
    }

    const auto lowered = lower_ascii_copy(font_family);
    return contains_any(lowered, {"courier", "consolas", "monaco", "menlo",
                                  "liberation mono", "dejavu sans mono",
                                  "droid sans mono", "noto sans mono", "mono"});
}

[[nodiscard]] bool is_wide_codepoint(char32_t codepoint) noexcept {
    return (codepoint >= 0x1100U && codepoint <= 0x115FU) ||
           (codepoint >= 0x2329U && codepoint <= 0x232AU) ||
           (codepoint >= 0x2E80U && codepoint <= 0xA4CFU) ||
           (codepoint >= 0xAC00U && codepoint <= 0xD7A3U) ||
           (codepoint >= 0xF900U && codepoint <= 0xFAFFU) ||
           (codepoint >= 0xFE10U && codepoint <= 0xFE19U) ||
           (codepoint >= 0xFE30U && codepoint <= 0xFE6FU) ||
           (codepoint >= 0xFF00U && codepoint <= 0xFF60U) ||
           (codepoint >= 0xFFE0U && codepoint <= 0xFFE6U) ||
           (codepoint >= 0x1F300U && codepoint <= 0x1FAFFU);
}

[[nodiscard]] bool is_zero_width_codepoint(char32_t codepoint) noexcept {
    return (codepoint >= 0x0300U && codepoint <= 0x036FU) ||
           (codepoint >= 0x1AB0U && codepoint <= 0x1AFFU) ||
           (codepoint >= 0x1DC0U && codepoint <= 0x1DFFU) ||
           (codepoint >= 0x20D0U && codepoint <= 0x20FFU) ||
           (codepoint >= 0xFE00U && codepoint <= 0xFE0FU) ||
           (codepoint >= 0xFE20U && codepoint <= 0xFE2FU) ||
           codepoint == 0x200BU || codepoint == 0x200CU ||
           codepoint == 0x200DU || codepoint == 0x2060U;
}

[[nodiscard]] FreeTypeLibraryPtr acquire_freetype_library() {
    static std::mutex mutex;
    static std::weak_ptr<FreeTypeLibrary> weak_library;

    std::lock_guard<std::mutex> lock(mutex);
    if (auto library = weak_library.lock()) {
        return library;
    }

    auto library = std::make_shared<FreeTypeLibrary>();
    if (!library->valid()) {
        return {};
    }

    weak_library = library;
    return library;
}

[[nodiscard]] std::optional<double>
font_units_to_points_scale(FT_Face face, double font_size_points) noexcept {
    if (face == nullptr || font_size_points <= 0.0) {
        return std::nullopt;
    }

    const auto units_per_em = static_cast<double>(face->units_per_EM);
    if (units_per_em <= 0.0) {
        return std::nullopt;
    }

    return font_size_points / units_per_em;
}

[[nodiscard]] std::optional<double>
load_glyph_advance_em_fraction(FT_Face face, FT_UInt glyph_index) noexcept {
    if (face == nullptr) {
        return std::nullopt;
    }

    if (FT_Load_Glyph(face, glyph_index,
                      FT_LOAD_NO_SCALE | FT_LOAD_NO_HINTING) != 0 ||
        face->glyph == nullptr) {
        return std::nullopt;
    }

    const auto units_per_em = static_cast<double>(face->units_per_EM);
    if (units_per_em <= 0.0) {
        return std::nullopt;
    }

    return static_cast<double>(face->glyph->metrics.horiAdvance) /
           units_per_em;
}

[[nodiscard]] std::optional<FreeTypeMetrics>
load_freetype_metrics(const std::filesystem::path &font_file_path) {
    if (font_file_path.empty() || !std::filesystem::exists(font_file_path)) {
        return std::nullopt;
    }

    const auto library = acquire_freetype_library();
    if (!library) {
        return std::nullopt;
    }

    FT_Face face = nullptr;
    const auto font_path = font_file_path.string();
    if (FT_New_Face(library->library, font_path.c_str(), 0, &face) != 0 ||
        face == nullptr) {
        return std::nullopt;
    }

    return FreeTypeMetrics{library, FreeTypeFacePtr(face)};
}

[[nodiscard]] const FreeTypeMetrics *
find_cached_freetype_metrics(const std::filesystem::path &font_file_path) {
    if (font_file_path.empty()) {
        return nullptr;
    }

    static std::mutex mutex;
    static std::map<std::filesystem::path, std::optional<FreeTypeMetrics>>
        cache;

    const auto normalized_path =
        std::filesystem::absolute(font_file_path).lexically_normal();

    std::lock_guard<std::mutex> lock(mutex);
    auto [entry, inserted] = cache.try_emplace(normalized_path);
    if (inserted) {
        entry->second = load_freetype_metrics(normalized_path);
    }

    return entry->second ? &*entry->second : nullptr;
}

[[nodiscard]] std::optional<double>
measure_freetype_text_width_points(std::string_view text,
                                   double font_size_points,
                                   const FreeTypeMetrics &metrics) noexcept {
    auto *face = metrics.face.get();
    const auto scale = font_units_to_points_scale(face, font_size_points);
    if (!scale) {
        return std::nullopt;
    }

    const bool has_kerning = FT_HAS_KERNING(face) != 0;
    double width_points = 0.0;
    FT_UInt previous_glyph_index = 0U;
    bool has_previous_glyph = false;

    for (std::size_t index = 0U; index < text.size();) {
        const auto decoded = decode_utf8_codepoint(text, index);
        index += decoded.size;

        const auto codepoint = decoded.value;
        if (codepoint == U'\n' || codepoint == U'\r') {
            continue;
        }

        if (codepoint == U'\t') {
            const auto space_index =
                FT_Get_Char_Index(face, static_cast<FT_ULong>(U' '));
            const auto space_width = load_glyph_advance_em_fraction(face, space_index);
            if (!space_width) {
                return std::nullopt;
            }
            width_points += *space_width * font_size_points * 4.0;
            has_previous_glyph = false;
            previous_glyph_index = 0U;
            continue;
        }

        if (is_zero_width_codepoint(codepoint)) {
            continue;
        }

        const auto glyph_index =
            FT_Get_Char_Index(face, static_cast<FT_ULong>(codepoint));
        if (has_previous_glyph && has_kerning && glyph_index != 0U &&
            previous_glyph_index != 0U) {
            FT_Vector kerning{};
            if (FT_Get_Kerning(face, previous_glyph_index, glyph_index,
                               FT_KERNING_UNSCALED, &kerning) == 0) {
                width_points += static_cast<double>(kerning.x) * *scale;
            }
        }

        const auto advance = load_glyph_advance_em_fraction(face, glyph_index);
        if (!advance) {
            return std::nullopt;
        }

        width_points += *advance * font_size_points;
        previous_glyph_index = glyph_index;
        has_previous_glyph = true;
    }

    return width_points;
}

[[nodiscard]] std::optional<double>
line_height_from_freetype_points(double font_size_points,
                                 const FreeTypeMetrics &metrics) noexcept {
    auto *face = metrics.face.get();
    const auto scale = font_units_to_points_scale(face, font_size_points);
    if (!scale) {
        return std::nullopt;
    }

    const double ascent = static_cast<double>(face->ascender) * *scale;
    const double descent =
        std::abs(static_cast<double>(face->descender) * *scale);
    const double height = static_cast<double>(face->height) * *scale;
    const double line_gap = std::max(0.0, height - ascent - descent);
    const double line_height = ascent + descent + line_gap;
    if (line_height <= 0.0) {
        return std::nullopt;
    }

    return line_height;
}

[[nodiscard]] double codepoint_width_units(char32_t codepoint,
                                           bool monospaced_family) noexcept {
    if (codepoint == U'\t') {
        return monospaced_family ? 2.4 : 1.32;
    }
    if (codepoint == U' ') {
        return monospaced_family ? 0.60 : 0.33;
    }
    if (codepoint < 0x80U) {
        const auto ascii = static_cast<unsigned char>(codepoint);
        if (std::iscntrl(ascii) != 0) {
            return 0.0;
        }
        if (std::ispunct(ascii) != 0) {
            return monospaced_family ? 0.60 : 0.35;
        }
        if (std::isdigit(ascii) != 0) {
            return monospaced_family ? 0.60 : 0.52;
        }
        if (std::isupper(ascii) != 0) {
            return monospaced_family ? 0.60 : 0.58;
        }
        return monospaced_family ? 0.60 : 0.55;
    }
    if (codepoint >= 0x0300U && codepoint <= 0x036FU) {
        return 0.0;
    }
    if (is_wide_codepoint(codepoint)) {
        return 1.0;
    }
    return monospaced_family ? 0.60 : 1.0;
}

[[nodiscard]] double fallback_text_width_points(std::string_view text,
                                                double font_size_points,
                                                std::string_view font_family) noexcept {
    const bool monospaced_family = is_monospaced_family(font_family);
    double width_units = 0.0;
    for (std::size_t index = 0U; index < text.size();) {
        const auto decoded = decode_utf8_codepoint(text, index);
        width_units +=
            codepoint_width_units(decoded.value, monospaced_family);
        index += decoded.size;
    }
    return width_units * font_size_points;
}

[[nodiscard]] double fallback_line_height_points(double font_size_points,
                                                 std::string_view font_family) noexcept {
    const bool monospaced_family = is_monospaced_family(font_family);
    const double line_height_scale = monospaced_family ? 1.18 : 1.24;
    return font_size_points * line_height_scale;
}

} // namespace

double measure_text_width_points(std::string_view text, double font_size_points,
                                 std::string_view font_family) noexcept {
    return measure_text_width_points(
        text, font_size_points,
        PdfTextMetricsOptions{font_family, {}});
}

double measure_text_width_points(std::string_view text, double font_size_points,
                                 const PdfTextMetricsOptions &options) noexcept {
    if (text.empty() || font_size_points <= 0.0) {
        return 0.0;
    }

    if (const auto *metrics =
            find_cached_freetype_metrics(options.font_file_path)) {
        if (const auto width = measure_freetype_text_width_points(
                text, font_size_points, *metrics)) {
            return *width;
        }
    }

    return fallback_text_width_points(text, font_size_points,
                                      options.font_family);
}

double estimate_line_height_points(double font_size_points,
                                   std::string_view font_family) noexcept {
    return estimate_line_height_points(
        font_size_points,
        PdfTextMetricsOptions{font_family, {}});
}

double estimate_line_height_points(double font_size_points,
                                   const PdfTextMetricsOptions &options) noexcept {
    if (font_size_points <= 0.0) {
        return 0.0;
    }

    if (const auto *metrics =
            find_cached_freetype_metrics(options.font_file_path)) {
        if (const auto line_height =
                line_height_from_freetype_points(font_size_points, *metrics)) {
            return *line_height;
        }
    }

    return fallback_line_height_points(font_size_points, options.font_family);
}

} // namespace featherdoc::pdf
