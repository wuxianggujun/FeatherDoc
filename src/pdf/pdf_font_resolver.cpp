#include <featherdoc/pdf/pdf_font_resolver.hpp>

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

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

struct FreeTypeFaceHandle {
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

[[nodiscard]] DecodedCodepoint
decode_utf8_codepoint(std::string_view text, std::size_t index) noexcept {
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
                   [](unsigned char value) {
                       return static_cast<char>(std::tolower(value));
                   });
    return lowered;
}

[[nodiscard]] std::string normalized_family_key(std::string_view family) {
    while (!family.empty() &&
           std::isspace(static_cast<unsigned char>(family.front())) != 0) {
        family.remove_prefix(1U);
    }
    while (!family.empty() &&
           std::isspace(static_cast<unsigned char>(family.back())) != 0) {
        family.remove_suffix(1U);
    }
    return lower_ascii_copy(family);
}

[[nodiscard]] bool path_exists(const std::filesystem::path &path) {
    if (path.empty()) {
        return false;
    }

    std::error_code error;
    return std::filesystem::is_regular_file(path, error);
}

[[nodiscard]] std::optional<std::filesystem::path>
first_existing_path(const std::vector<std::filesystem::path> &candidates) {
    for (const auto &candidate : candidates) {
        if (path_exists(candidate)) {
            return candidate;
        }
    }
    return std::nullopt;
}

[[nodiscard]] std::string environment_value(const char *name) {
#if defined(_WIN32)
    char *value = nullptr;
    std::size_t size = 0;
    if (::_dupenv_s(&value, &size, name) != 0 || value == nullptr) {
        return {};
    }

    std::string result(value, size > 0U ? size - 1U : 0U);
    std::free(value);
    return result;
#else
    const char *value = std::getenv(name);
    return value == nullptr ? std::string{} : std::string{value};
#endif
}

[[nodiscard]] std::optional<std::filesystem::path>
find_mapped_font_path(const std::vector<PdfFontMapping> &mappings,
                      std::string_view font_family, bool bold, bool italic) {
    const auto requested = normalized_family_key(font_family);
    if (requested.empty()) {
        return std::nullopt;
    }

    for (const auto &mapping : mappings) {
        if (normalized_family_key(mapping.font_family) == requested &&
            mapping.bold == bold && mapping.italic == italic &&
            path_exists(mapping.font_file_path)) {
            return mapping.font_file_path;
        }
    }

    for (const auto &mapping : mappings) {
        if (normalized_family_key(mapping.font_family) == requested &&
            !mapping.bold && !mapping.italic &&
            path_exists(mapping.font_file_path)) {
            return mapping.font_file_path;
        }
    }

    return std::nullopt;
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

[[nodiscard]] std::optional<FreeTypeFaceHandle>
load_freetype_face(const std::filesystem::path &font_file_path) {
    if (font_file_path.empty() || !path_exists(font_file_path)) {
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

    return FreeTypeFaceHandle{library, FreeTypeFacePtr(face)};
}

[[nodiscard]] const FreeTypeFaceHandle *
find_cached_freetype_face(const std::filesystem::path &font_file_path) {
    if (font_file_path.empty()) {
        return nullptr;
    }

    static std::mutex mutex;
    static std::map<std::filesystem::path, std::optional<FreeTypeFaceHandle>>
        cache;

    const auto normalized_path =
        std::filesystem::absolute(font_file_path).lexically_normal();

    std::lock_guard<std::mutex> lock(mutex);
    auto [entry, inserted] = cache.try_emplace(normalized_path);
    if (inserted) {
        entry->second = load_freetype_face(normalized_path);
    }

    return entry->second ? &*entry->second : nullptr;
}

[[nodiscard]] std::vector<std::filesystem::path>
system_font_candidates_for_family(std::string_view font_family, bool bold,
                                  bool italic) {
    const auto key = normalized_family_key(font_family);
    std::vector<std::filesystem::path> candidates;

#if defined(_WIN32)
    const std::filesystem::path fonts_dir{"C:/Windows/Fonts"};
    if (key == "arial" || key == "helvetica") {
        if (bold && italic) {
            candidates.push_back(fonts_dir / "arialbi.ttf");
        } else if (bold) {
            candidates.push_back(fonts_dir / "arialbd.ttf");
        } else if (italic) {
            candidates.push_back(fonts_dir / "ariali.ttf");
        }
        candidates.push_back(fonts_dir / "arial.ttf");
    } else if (key == "segoe ui") {
        if (bold && italic) {
            candidates.push_back(fonts_dir / "segoeuiz.ttf");
        } else if (bold) {
            candidates.push_back(fonts_dir / "segoeuib.ttf");
        } else if (italic) {
            candidates.push_back(fonts_dir / "segoeuii.ttf");
        }
        candidates.push_back(fonts_dir / "segoeui.ttf");
    } else if (key == "calibri") {
        if (bold && italic) {
            candidates.push_back(fonts_dir / "calibriz.ttf");
        } else if (bold) {
            candidates.push_back(fonts_dir / "calibrib.ttf");
        } else if (italic) {
            candidates.push_back(fonts_dir / "calibrii.ttf");
        }
        candidates.push_back(fonts_dir / "calibri.ttf");
    } else if (key == "times new roman") {
        if (bold && italic) {
            candidates.push_back(fonts_dir / "timesbi.ttf");
        } else if (bold) {
            candidates.push_back(fonts_dir / "timesbd.ttf");
        } else if (italic) {
            candidates.push_back(fonts_dir / "timesi.ttf");
        }
        candidates.push_back(fonts_dir / "times.ttf");
    } else if (key == "courier new") {
        if (bold && italic) {
            candidates.push_back(fonts_dir / "courbi.ttf");
        } else if (bold) {
            candidates.push_back(fonts_dir / "courbd.ttf");
        } else if (italic) {
            candidates.push_back(fonts_dir / "couri.ttf");
        }
        candidates.push_back(fonts_dir / "cour.ttf");
    } else if (key == "dengxian" || key == "deng" || key == "deng xian") {
        if (bold) {
            candidates.push_back(fonts_dir / "Dengb.ttf");
        }
        candidates.push_back(fonts_dir / "Deng.ttf");
    } else if (key == "microsoft yahei" || key == "microsoft yahei ui") {
        if (bold) {
            candidates.push_back(fonts_dir / "msyhbd.ttc");
        }
        candidates.push_back(fonts_dir / "msyh.ttc");
    } else if (key == "simsun" || key == "nsimsun") {
        candidates.push_back(fonts_dir / "simsun.ttc");
    } else if (key == "simhei") {
        candidates.push_back(fonts_dir / "simhei.ttf");
    } else if (key == "noto sans sc" || key == "noto sans cjk sc") {
        candidates.push_back(fonts_dir / "NotoSansSC-VF.ttf");
    }
#else
    if (key == "arial" || key == "helvetica") {
        if (bold && italic) {
            candidates.emplace_back("/usr/share/fonts/truetype/liberation2/"
                                    "LiberationSans-BoldItalic.ttf");
            candidates.emplace_back(
                "/usr/share/fonts/truetype/dejavu/DejaVuSans-BoldOblique.ttf");
        } else if (bold) {
            candidates.emplace_back("/usr/share/fonts/truetype/liberation2/"
                                    "LiberationSans-Bold.ttf");
            candidates.emplace_back(
                "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf");
        } else if (italic) {
            candidates.emplace_back("/usr/share/fonts/truetype/liberation2/"
                                    "LiberationSans-Italic.ttf");
            candidates.emplace_back(
                "/usr/share/fonts/truetype/dejavu/DejaVuSans-Oblique.ttf");
        }
        candidates.emplace_back(
            "/usr/share/fonts/truetype/liberation2/LiberationSans-Regular.ttf");
        candidates.emplace_back(
            "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf");
    } else if (key == "courier new") {
        if (bold && italic) {
            candidates.emplace_back("/usr/share/fonts/truetype/liberation2/"
                                    "LiberationMono-BoldItalic.ttf");
            candidates.emplace_back("/usr/share/fonts/truetype/dejavu/"
                                    "DejaVuSansMono-BoldOblique.ttf");
        } else if (bold) {
            candidates.emplace_back("/usr/share/fonts/truetype/liberation2/"
                                    "LiberationMono-Bold.ttf");
            candidates.emplace_back(
                "/usr/share/fonts/truetype/dejavu/DejaVuSansMono-Bold.ttf");
        } else if (italic) {
            candidates.emplace_back("/usr/share/fonts/truetype/liberation2/"
                                    "LiberationMono-Italic.ttf");
            candidates.emplace_back(
                "/usr/share/fonts/truetype/dejavu/DejaVuSansMono-Oblique.ttf");
        }
        candidates.emplace_back(
            "/usr/share/fonts/truetype/liberation2/LiberationMono-Regular.ttf");
        candidates.emplace_back(
            "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf");
    } else if (key == "times new roman") {
        if (bold && italic) {
            candidates.emplace_back("/usr/share/fonts/truetype/liberation2/"
                                    "LiberationSerif-BoldItalic.ttf");
        } else if (bold) {
            candidates.emplace_back("/usr/share/fonts/truetype/liberation2/"
                                    "LiberationSerif-Bold.ttf");
        } else if (italic) {
            candidates.emplace_back("/usr/share/fonts/truetype/liberation2/"
                                    "LiberationSerif-Italic.ttf");
        }
        candidates.emplace_back("/usr/share/fonts/truetype/liberation2/"
                                "LiberationSerif-Regular.ttf");
    } else if (key == "noto sans sc" || key == "noto sans cjk sc") {
        candidates.emplace_back(
            "/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc");
    }
#endif

    return candidates;
}

[[nodiscard]] std::vector<std::filesystem::path> cjk_fallback_candidates() {
    std::vector<std::filesystem::path> candidates;

    if (const auto configured = environment_value("FEATHERDOC_PDF_CJK_FONT");
        !configured.empty()) {
        candidates.emplace_back(configured);
    }
    if (const auto configured = environment_value("FEATHERDOC_TEST_CJK_FONT");
        !configured.empty()) {
        candidates.emplace_back(configured);
    }

#if defined(_WIN32)
    const std::filesystem::path fonts_dir{"C:/Windows/Fonts"};
    candidates.push_back(fonts_dir / "Deng.ttf");
    candidates.push_back(fonts_dir / "Dengb.ttf");
    candidates.push_back(fonts_dir / "NotoSansSC-VF.ttf");
    candidates.push_back(fonts_dir / "msyh.ttc");
    candidates.push_back(fonts_dir / "msyhbd.ttc");
    candidates.push_back(fonts_dir / "simhei.ttf");
    candidates.push_back(fonts_dir / "simsun.ttc");
#else
    candidates.emplace_back(
        "/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc");
    candidates.emplace_back("/usr/share/fonts/truetype/arphic/uming.ttc");
#endif

    return candidates;
}

[[nodiscard]] std::vector<std::filesystem::path> latin_fallback_candidates() {
    std::vector<std::filesystem::path> candidates;

    if (const auto configured = environment_value("FEATHERDOC_PDF_FONT");
        !configured.empty()) {
        candidates.emplace_back(configured);
    }
    if (const auto configured =
            environment_value("FEATHERDOC_TEST_PROPORTIONAL_FONT");
        !configured.empty()) {
        candidates.emplace_back(configured);
    }

#if defined(_WIN32)
    const std::filesystem::path fonts_dir{"C:/Windows/Fonts"};
    candidates.push_back(fonts_dir / "arial.ttf");
    candidates.push_back(fonts_dir / "segoeui.ttf");
    candidates.push_back(fonts_dir / "calibri.ttf");
#else
    candidates.emplace_back(
        "/usr/share/fonts/truetype/liberation2/LiberationSans-Regular.ttf");
    candidates.emplace_back("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf");
#endif

    return candidates;
}

[[nodiscard]] bool is_cjk_codepoint(char32_t codepoint) noexcept {
    return (codepoint >= 0x1100U && codepoint <= 0x11FFU) ||
           (codepoint >= 0x2E80U && codepoint <= 0x2EFFU) ||
           (codepoint >= 0x3000U && codepoint <= 0x303FU) ||
           (codepoint >= 0x3040U && codepoint <= 0x309FU) ||
           (codepoint >= 0x30A0U && codepoint <= 0x30FFU) ||
           (codepoint >= 0x3100U && codepoint <= 0x312FU) ||
           (codepoint >= 0x3130U && codepoint <= 0x318FU) ||
           (codepoint >= 0x31A0U && codepoint <= 0x31BFU) ||
           (codepoint >= 0x31F0U && codepoint <= 0x31FFU) ||
           (codepoint >= 0x3200U && codepoint <= 0x33FFU) ||
           (codepoint >= 0x3400U && codepoint <= 0x4DBFU) ||
           (codepoint >= 0x4E00U && codepoint <= 0x9FFFU) ||
           (codepoint >= 0xA960U && codepoint <= 0xA97FU) ||
           (codepoint >= 0xAC00U && codepoint <= 0xD7A3U) ||
           (codepoint >= 0xD7B0U && codepoint <= 0xD7FFU) ||
           (codepoint >= 0xF900U && codepoint <= 0xFAFFU) ||
           (codepoint >= 0xFE10U && codepoint <= 0xFE1FU) ||
           (codepoint >= 0xFE30U && codepoint <= 0xFE4FU) ||
           (codepoint >= 0xFF00U && codepoint <= 0xFFEFU) ||
           (codepoint >= 0x20000U && codepoint <= 0x2FA1FU);
}

[[nodiscard]] bool is_ignorable_font_codepoint(char32_t codepoint) noexcept {
    return codepoint == U'\n' || codepoint == U'\r' || codepoint == U'\t' ||
           codepoint == U' ' || codepoint == 0x00A0U || codepoint == 0x200BU ||
           codepoint == 0x200CU || codepoint == 0x200DU ||
           codepoint == 0x2060U;
}

[[nodiscard]] std::optional<std::filesystem::path>
find_system_font_path(std::string_view font_family, bool bold, bool italic);

[[nodiscard]] std::optional<bool>
font_path_supports_text(const std::filesystem::path &font_file_path,
                        std::string_view text) {
    const auto *handle = find_cached_freetype_face(font_file_path);
    if (handle == nullptr || handle->face == nullptr) {
        return std::nullopt;
    }

    auto *face = handle->face.get();
    for (std::size_t index = 0U; index < text.size();) {
        const auto decoded = decode_utf8_codepoint(text, index);
        index += decoded.size;

        if (is_ignorable_font_codepoint(decoded.value)) {
            continue;
        }

        if (FT_Get_Char_Index(face, static_cast<FT_ULong>(decoded.value)) ==
            0U) {
            return false;
        }
    }

    return true;
}

[[nodiscard]] std::optional<std::filesystem::path>
resolve_unicode_fallback_path(const PdfFontResolverOptions &options,
                              std::string_view east_asia_font_family, bool bold,
                              bool italic) {
    if (!east_asia_font_family.empty()) {
        auto resolved_path = find_mapped_font_path(options.font_mappings,
                                                   east_asia_font_family, bold,
                                                   italic);
        if (!resolved_path && options.use_system_font_fallbacks) {
            resolved_path =
                find_system_font_path(east_asia_font_family, bold, italic);
        }
        if (resolved_path) {
            return resolved_path;
        }
    }

    if (path_exists(options.default_cjk_font_file_path)) {
        return options.default_cjk_font_file_path;
    }

    if (options.use_system_font_fallbacks) {
        return first_existing_path(cjk_fallback_candidates());
    }

    return std::nullopt;
}

[[nodiscard]] std::optional<std::filesystem::path>
find_system_font_path(std::string_view font_family, bool bold, bool italic) {
    return first_existing_path(
        system_font_candidates_for_family(font_family, bold, italic));
}

} // namespace

PdfFontResolver::PdfFontResolver(PdfFontResolverOptions options)
    : options_(std::move(options)) {}

const PdfFontResolverOptions &PdfFontResolver::options() const noexcept {
    return this->options_;
}

PdfResolvedFont PdfFontResolver::resolve(std::string_view font_family,
                                         std::string_view east_asia_font_family,
                                         std::string_view text) const {
    return this->resolve(font_family, east_asia_font_family, text, false,
                         false);
}

PdfResolvedFont PdfFontResolver::resolve(std::string_view font_family,
                                         std::string_view east_asia_font_family,
                                         std::string_view text, bool bold,
                                         bool italic) const {
    const bool cjk_text = pdf_text_contains_cjk(text);
    const bool unicode_text = pdf_text_requires_unicode(text);
    std::string resolved_family{cjk_text && !east_asia_font_family.empty()
                                    ? east_asia_font_family
                                    : (font_family.empty()
                                           ? std::string_view{"Helvetica"}
                                           : font_family)};

    std::optional<std::filesystem::path> resolved_path;

    if (cjk_text) {
        if (!east_asia_font_family.empty()) {
            resolved_path =
                find_mapped_font_path(this->options_.font_mappings,
                                      east_asia_font_family, bold, italic);
            if (!resolved_path && this->options_.use_system_font_fallbacks) {
                resolved_path =
                    find_system_font_path(east_asia_font_family, bold, italic);
            }
        }

        if (!resolved_path &&
            path_exists(this->options_.default_cjk_font_file_path)) {
            resolved_path = this->options_.default_cjk_font_file_path;
        }

        if (!resolved_path && this->options_.use_system_font_fallbacks) {
            resolved_path = first_existing_path(cjk_fallback_candidates());
        }

        if (!resolved_path && !font_family.empty()) {
            resolved_path = find_mapped_font_path(this->options_.font_mappings,
                                                  font_family, bold, italic);
            if (!resolved_path && this->options_.use_system_font_fallbacks) {
                resolved_path =
                    find_system_font_path(font_family, bold, italic);
            }
        }
    } else {
        resolved_path = find_mapped_font_path(this->options_.font_mappings,
                                              font_family, bold, italic);
        if (!resolved_path && this->options_.use_system_font_fallbacks) {
            resolved_path = find_system_font_path(font_family, bold, italic);
        }

        if (unicode_text && !resolved_path &&
            !east_asia_font_family.empty()) {
            resolved_path =
                find_mapped_font_path(this->options_.font_mappings,
                                      east_asia_font_family, bold, italic);
            if (!resolved_path && this->options_.use_system_font_fallbacks) {
                resolved_path =
                    find_system_font_path(east_asia_font_family, bold, italic);
            }
            if (resolved_path) {
                resolved_family = std::string{east_asia_font_family};
            }
        }

        if (unicode_text && !resolved_path &&
            path_exists(this->options_.default_cjk_font_file_path)) {
            resolved_path = this->options_.default_cjk_font_file_path;
            if (!east_asia_font_family.empty()) {
                resolved_family = std::string{east_asia_font_family};
            }
        }

        if (unicode_text && !resolved_path &&
            this->options_.use_system_font_fallbacks) {
            resolved_path = first_existing_path(cjk_fallback_candidates());
            if (resolved_path && !east_asia_font_family.empty()) {
                resolved_family = std::string{east_asia_font_family};
            }
        }
    }

    if (!resolved_path && path_exists(this->options_.default_font_file_path)) {
        resolved_path = this->options_.default_font_file_path;
    }

    if (!resolved_path && this->options_.use_system_font_fallbacks &&
        !cjk_text) {
        resolved_path = first_existing_path(latin_fallback_candidates());
    }

    if (unicode_text && resolved_path) {
        const auto current_supports_text =
            font_path_supports_text(*resolved_path, text);
        if (current_supports_text.has_value() && !*current_supports_text) {
            if (const auto unicode_fallback =
                    resolve_unicode_fallback_path(this->options_,
                                                  east_asia_font_family, bold,
                                                  italic);
                unicode_fallback &&
                *unicode_fallback != *resolved_path) {
                const auto fallback_supports_text =
                    font_path_supports_text(*unicode_fallback, text);
                if (fallback_supports_text.has_value() &&
                    *fallback_supports_text) {
                    resolved_path = unicode_fallback;
                    if (!east_asia_font_family.empty()) {
                        resolved_family = std::string{east_asia_font_family};
                    }
                }
            }
        }
    }

    if (resolved_path && resolved_family == "Helvetica" && cjk_text &&
        !east_asia_font_family.empty()) {
        resolved_family = std::string{east_asia_font_family};
    }

    return PdfResolvedFont{
        resolved_family,
        resolved_path.value_or(std::filesystem::path{}),
        unicode_text,
    };
}

bool pdf_text_requires_unicode(std::string_view text) noexcept {
    return std::any_of(text.begin(), text.end(), [](unsigned char value) {
        return (value & 0x80U) != 0U;
    });
}

bool pdf_text_contains_cjk(std::string_view text) noexcept {
    for (std::size_t index = 0U; index < text.size();) {
        const auto decoded = decode_utf8_codepoint(text, index);
        if (is_cjk_codepoint(decoded.value)) {
            return true;
        }
        index += decoded.size;
    }
    return false;
}

} // namespace featherdoc::pdf
