#if defined(FEATHERDOC_ENABLE_PDF_FONT_SUBSET)
#include <featherdoc/pdf/pdf_font_subset.hpp>
#endif
#include <featherdoc/pdf/pdf_text_metrics.hpp>
#include <featherdoc/pdf/pdf_writer.hpp>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <limits>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

#include <ft2build.h>
#include FT_FREETYPE_H

extern "C" {
#include <pdfio-content.h>
#include <pdfio.h>
}

namespace featherdoc::pdf {
namespace {

constexpr double kPi = 3.14159265358979323846;

struct PdfioErrorState {
    std::string message;
};

bool pdfio_error_callback(pdfio_file_t *, const char *message, void *data) {
    auto *state = static_cast<PdfioErrorState *>(data);
    if (state != nullptr && message != nullptr) {
        state->message = message;
    }
    return true;
}

struct PdfioFileCloser {
    void operator()(pdfio_file_t *pdf) const {
        if (pdf != nullptr) {
            pdfioFileClose(pdf);
        }
    }
};

struct PdfioStreamCloser {
    void operator()(pdfio_stream_t *stream) const {
        if (stream != nullptr) {
            pdfioStreamClose(stream);
        }
    }
};

struct FontResourceKey {
    std::string family;
    std::filesystem::path file_path;
    bool bold{false};
    bool italic{false};
    bool unicode{false};
    bool shaped_glyphs{false};

    [[nodiscard]] friend bool operator<(const FontResourceKey &left,
                                        const FontResourceKey &right) {
        return std::tie(left.family, left.file_path, left.bold, left.italic,
                        left.unicode, left.shaped_glyphs) <
               std::tie(right.family, right.file_path, right.bold,
                        right.italic, right.unicode, right.shaped_glyphs);
    }
};

struct ImageResourceKey {
    std::filesystem::path file_path;
    bool interpolate{true};

    [[nodiscard]] friend bool operator<(const ImageResourceKey &left,
                                        const ImageResourceKey &right) {
        return std::tie(left.file_path, left.interpolate) <
               std::tie(right.file_path, right.interpolate);
    }
};

struct ImageSourceCleanup {
    std::vector<std::filesystem::path> paths;

    ~ImageSourceCleanup() {
        for (const auto &path : paths) {
            std::error_code error;
            std::filesystem::remove(path, error);
        }
    }
};

struct FontSubsetStorage {
    std::vector<std::vector<unsigned char>> buffers;
};

struct GlyphCidEntry {
    std::uint16_t cid{0U};
    std::uint16_t glyph_id{0U};
    double width_1000{0.0};
    std::string unicode_text;
};

struct GlyphFontResource {
    std::vector<GlyphCidEntry> entries;
};

using GlyphFontResources = std::map<FontResourceKey, GlyphFontResource>;

struct FreeTypeFaceOwner {
    FT_Library library{nullptr};
    FT_Face face{nullptr};

    explicit FreeTypeFaceOwner(const std::filesystem::path &font_file_path) {
        if (FT_Init_FreeType(&library) != 0) {
            library = nullptr;
            return;
        }

        const auto font_path = font_file_path.string();
        if (FT_New_Face(library, font_path.c_str(), 0, &face) != 0) {
            face = nullptr;
        }
    }

    ~FreeTypeFaceOwner() {
        if (face != nullptr) {
            FT_Done_Face(face);
        }
        if (library != nullptr) {
            FT_Done_FreeType(library);
        }
    }

    FreeTypeFaceOwner(const FreeTypeFaceOwner &) = delete;
    FreeTypeFaceOwner &operator=(const FreeTypeFaceOwner &) = delete;

    [[nodiscard]] bool valid() const noexcept {
        return library != nullptr && face != nullptr;
    }
};

[[nodiscard]] std::string build_error(std::string fallback,
                                      const PdfioErrorState &state) {
    if (!state.message.empty()) {
        fallback += ": ";
        fallback += state.message;
    }
    return fallback;
}

[[nodiscard]] bool
can_write_shaped_glyph_run(const PdfTextRun &text_run) noexcept {
    constexpr double kPositionTolerance = 0.0001;
    if (text_run.text.empty() || text_run.font_file_path.empty() ||
        text_run.font_size_points <= 0.0 ||
        !text_run.glyph_run.used_harfbuzz ||
        !text_run.glyph_run.error_message.empty() ||
        text_run.glyph_run.glyphs.empty() ||
        text_run.glyph_run.text != text_run.text ||
        text_run.glyph_run.font_file_path != text_run.font_file_path) {
        return false;
    }

    for (const auto &glyph : text_run.glyph_run.glyphs) {
        if (glyph.glyph_id == 0U ||
            glyph.glyph_id > std::numeric_limits<std::uint16_t>::max() ||
            std::abs(glyph.x_offset_points) > kPositionTolerance ||
            std::abs(glyph.y_offset_points) > kPositionTolerance ||
            std::abs(glyph.y_advance_points) > kPositionTolerance) {
            return false;
        }
    }

    return true;
}

[[nodiscard]] FontResourceKey make_font_key(const PdfTextRun &text_run) {
    return FontResourceKey{
        text_run.font_family.empty() ? "Helvetica" : text_run.font_family,
        text_run.font_file_path,
        text_run.bold,
        text_run.italic,
        text_run.unicode,
        can_write_shaped_glyph_run(text_run),
    };
}

[[nodiscard]] std::string base_font_family_name(const FontResourceKey &key) {
    if (key.family == "Helvetica") {
        if (key.bold && key.italic) {
            return "Helvetica-BoldOblique";
        }
        if (key.bold) {
            return "Helvetica-Bold";
        }
        if (key.italic) {
            return "Helvetica-Oblique";
        }
    } else if (key.family == "Times" || key.family == "Times-Roman") {
        if (key.bold && key.italic) {
            return "Times-BoldItalic";
        }
        if (key.bold) {
            return "Times-Bold";
        }
        if (key.italic) {
            return "Times-Italic";
        }
        return "Times-Roman";
    } else if (key.family == "Courier") {
        if (key.bold && key.italic) {
            return "Courier-BoldOblique";
        }
        if (key.bold) {
            return "Courier-Bold";
        }
        if (key.italic) {
            return "Courier-Oblique";
        }
    }

    return key.family;
}

[[nodiscard]] std::string font_label(const FontResourceKey &key) {
    const std::string suffix = key.shaped_glyphs ? " shaped glyphs" : "";
    if (!key.file_path.empty()) {
        return key.file_path.string() + suffix;
    }
    return base_font_family_name(key) + suffix;
}

[[nodiscard]] std::string image_label(const ImageResourceKey &key) {
    return key.file_path.string();
}

[[nodiscard]] std::uint32_t decode_utf8_codepoint(std::string_view text,
                                                  std::size_t &index) noexcept;

[[nodiscard]] pdfio_obj_t *
create_pdfio_font(pdfio_file_t *pdf, const FontResourceKey &key,
                  const std::string &used_text, bool subset_unicode_fonts,
                  FontSubsetStorage &subset_storage) {
    if (!key.file_path.empty()) {
#if defined(FEATHERDOC_ENABLE_PDF_FONT_SUBSET)
        if (subset_unicode_fonts && key.unicode && !used_text.empty()) {
            std::set<std::uint32_t> unique_codepoints;
            for (std::size_t index = 0U; index < used_text.size();) {
                unique_codepoints.insert(
                    decode_utf8_codepoint(used_text, index));
            }
            const auto subset_result = subset_font_file_for_codepoints(
                key.file_path,
                std::vector<std::uint32_t>(unique_codepoints.begin(),
                                           unique_codepoints.end()));
            if (subset_result.success && !subset_result.font_data.empty()) {
                subset_storage.buffers.push_back(
                    std::move(subset_result.font_data));
                const auto &font_data = subset_storage.buffers.back();
                pdfio_obj_t *font = pdfioFileCreateFontObjFromData(
                    pdf, font_data.data(), font_data.size(), key.unicode);
                if (font != nullptr) {
                    return font;
                }
                subset_storage.buffers.pop_back();
            }
        }
#else
        (void)used_text;
        (void)subset_unicode_fonts;
        (void)subset_storage;
#endif
        const auto font_path = key.file_path.string();
        return pdfioFileCreateFontObjFromFile(pdf, font_path.c_str(),
                                              key.unicode);
    }

    const auto family = base_font_family_name(key);
    return pdfioFileCreateFontObjFromBase(pdf, family.c_str());
}

[[nodiscard]] const char *make_pdfio_resource_name(pdfio_file_t *pdf,
                                                   const std::string &name,
                                                   std::string &error_message) {
    const auto *resource_name = pdfioStringCreate(pdf, name.c_str());
    if (resource_name == nullptr) {
        error_message = "Unable to allocate PDF resource name: " + name;
    }
    return resource_name;
}

[[nodiscard]] pdfio_linecap_t to_pdfio_line_cap(PdfLineCap line_cap) noexcept {
    switch (line_cap) {
    case PdfLineCap::butt:
        return PDFIO_LINECAP_BUTT;
    case PdfLineCap::round:
        return PDFIO_LINECAP_ROUND;
    case PdfLineCap::square:
        return PDFIO_LINECAP_SQUARE;
    }

    return PDFIO_LINECAP_BUTT;
}

void append_utf16be_hex_unit(std::string &output, std::uint16_t value) {
    constexpr char digits[] = "0123456789ABCDEF";
    output.push_back(digits[(value >> 12U) & 0x0FU]);
    output.push_back(digits[(value >> 8U) & 0x0FU]);
    output.push_back(digits[(value >> 4U) & 0x0FU]);
    output.push_back(digits[value & 0x0FU]);
}

[[nodiscard]] std::uint32_t decode_utf8_codepoint(std::string_view text,
                                                  std::size_t &index) noexcept {
    if (index >= text.size()) {
        return 0xFFFDU;
    }

    const auto first = static_cast<unsigned char>(text[index++]);
    if (first < 0x80U) {
        return first;
    }

    auto continuation = [&](std::uint32_t &value) noexcept {
        if (index >= text.size()) {
            return false;
        }
        const auto next = static_cast<unsigned char>(text[index]);
        if ((next & 0xC0U) != 0x80U) {
            return false;
        }
        ++index;
        value = (value << 6U) | (next & 0x3FU);
        return true;
    };

    if ((first & 0xE0U) == 0xC0U) {
        std::uint32_t value = first & 0x1FU;
        return continuation(value) ? value : 0xFFFDU;
    }
    if ((first & 0xF0U) == 0xE0U) {
        std::uint32_t value = first & 0x0FU;
        return continuation(value) && continuation(value) ? value : 0xFFFDU;
    }
    if ((first & 0xF8U) == 0xF0U) {
        std::uint32_t value = first & 0x07U;
        return continuation(value) && continuation(value) && continuation(value)
                   ? value
                   : 0xFFFDU;
    }

    return 0xFFFDU;
}

[[nodiscard]] std::string utf16be_hex_from_utf8(std::string_view text) {
    std::string hex{"FEFF"};
    for (std::size_t index = 0U; index < text.size();) {
        auto codepoint = decode_utf8_codepoint(text, index);
        if (codepoint > 0x10FFFFU ||
            (codepoint >= 0xD800U && codepoint <= 0xDFFFU)) {
            codepoint = 0xFFFDU;
        }

        if (codepoint <= 0xFFFFU) {
            append_utf16be_hex_unit(hex, static_cast<std::uint16_t>(codepoint));
        } else {
            codepoint -= 0x10000U;
            append_utf16be_hex_unit(
                hex, static_cast<std::uint16_t>(0xD800U +
                                                ((codepoint >> 10U) & 0x3FFU)));
            append_utf16be_hex_unit(hex, static_cast<std::uint16_t>(
                                             0xDC00U + (codepoint & 0x3FFU)));
        }
    }
    return hex;
}

[[nodiscard]] std::string utf16be_hex_payload_from_utf8(std::string_view text) {
    std::string hex;
    for (std::size_t index = 0U; index < text.size();) {
        auto codepoint = decode_utf8_codepoint(text, index);
        if (codepoint > 0x10FFFFU ||
            (codepoint >= 0xD800U && codepoint <= 0xDFFFU)) {
            codepoint = 0xFFFDU;
        }

        if (codepoint <= 0xFFFFU) {
            append_utf16be_hex_unit(hex, static_cast<std::uint16_t>(codepoint));
        } else {
            codepoint -= 0x10000U;
            append_utf16be_hex_unit(
                hex, static_cast<std::uint16_t>(0xD800U +
                                                ((codepoint >> 10U) & 0x3FFU)));
            append_utf16be_hex_unit(hex, static_cast<std::uint16_t>(
                                             0xDC00U + (codepoint & 0x3FFU)));
        }
    }
    return hex;
}

[[nodiscard]] std::vector<unsigned char>
read_binary_file(const std::filesystem::path &path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        return {};
    }

    return {std::istreambuf_iterator<char>(input),
            std::istreambuf_iterator<char>()};
}

[[nodiscard]] double font_unit_to_1000(FT_Face face, FT_Pos value) noexcept {
    if (face == nullptr || face->units_per_EM <= 0) {
        return 0.0;
    }

    return static_cast<double>(value) * 1000.0 /
           static_cast<double>(face->units_per_EM);
}

[[nodiscard]] std::string sanitized_pdf_font_name(std::string_view value) {
    std::string name;
    name.reserve(value.size());
    for (const auto byte : value) {
        const auto ch = static_cast<unsigned char>(byte);
        if (std::isalnum(ch) != 0 || ch == '-' || ch == '_') {
            name.push_back(static_cast<char>(ch));
        } else if (!name.empty() && name.back() != '-') {
            name.push_back('-');
        }
    }

    while (!name.empty() && name.back() == '-') {
        name.pop_back();
    }
    if (name.empty()) {
        name = "Font";
    }
    return "FeatherDocGlyph-" + name;
}

[[nodiscard]] std::string base_font_name_for(FT_Face face,
                                             const FontResourceKey &key) {
    if (face != nullptr) {
        if (const char *postscript_name = FT_Get_Postscript_Name(face);
            postscript_name != nullptr && postscript_name[0] != '\0') {
            return sanitized_pdf_font_name(postscript_name);
        }
        if (face->family_name != nullptr && face->family_name[0] != '\0') {
            return sanitized_pdf_font_name(face->family_name);
        }
    }

    return sanitized_pdf_font_name(base_font_family_name(key));
}

[[nodiscard]] std::string glyph_cluster_text(const PdfGlyphRun &glyph_run,
                                             std::size_t glyph_index) {
    if (glyph_index >= glyph_run.glyphs.size()) {
        return {};
    }

    const auto cluster = glyph_run.glyphs[glyph_index].cluster;
    if (cluster >= glyph_run.text.size()) {
        return {};
    }

    for (std::size_t index = 0U; index < glyph_index; ++index) {
        if (glyph_run.glyphs[index].cluster == cluster) {
            return {};
        }
    }

    auto next_cluster = static_cast<std::uint32_t>(glyph_run.text.size());
    for (const auto &glyph : glyph_run.glyphs) {
        if (glyph.cluster > cluster && glyph.cluster < next_cluster) {
            next_cluster = glyph.cluster;
        }
    }

    if (next_cluster <= cluster || next_cluster > glyph_run.text.size()) {
        return {};
    }
    return glyph_run.text.substr(cluster, next_cluster - cluster);
}

[[nodiscard]] bool collect_shaped_glyph_font_resources(
    const PdfPageLayout &page, GlyphFontResources &glyph_resources,
    std::string &error_message) {
    constexpr auto kMaxCid = std::numeric_limits<std::uint16_t>::max();
    for (const auto &text_run : page.text_runs) {
        const auto font_key = make_font_key(text_run);
        if (!font_key.shaped_glyphs) {
            continue;
        }

        auto &resource = glyph_resources[font_key];
        for (std::size_t index = 0U; index < text_run.glyph_run.glyphs.size();
             ++index) {
            if (resource.entries.size() >= kMaxCid) {
                error_message =
                    "Too many shaped PDF glyphs for one font resource: " +
                    font_label(font_key);
                return false;
            }

            const auto &glyph = text_run.glyph_run.glyphs[index];
            const double width_1000 =
                std::isfinite(glyph.x_advance_points)
                    ? std::max(0.0, glyph.x_advance_points * 1000.0 /
                                        text_run.font_size_points)
                    : 0.0;
            resource.entries.push_back(GlyphCidEntry{
                static_cast<std::uint16_t>(resource.entries.size() + 1U),
                static_cast<std::uint16_t>(glyph.glyph_id),
                width_1000,
                glyph_cluster_text(text_run.glyph_run, index),
            });
        }
    }

    return true;
}

[[nodiscard]] pdfio_obj_t *
create_stream_object_from_bytes(pdfio_file_t *pdf, pdfio_dict_t *dict,
                                const unsigned char *data, std::size_t size,
                                std::string_view label,
                                std::string &error_message) {
    if (dict == nullptr) {
        error_message = "Unable to create PDF stream dictionary: " +
                        std::string{label};
        return nullptr;
    }

    pdfio_obj_t *object = pdfioFileCreateObj(pdf, dict);
    if (object == nullptr) {
        error_message =
            "Unable to create PDF stream object: " + std::string{label};
        return nullptr;
    }

    std::unique_ptr<pdfio_stream_t, PdfioStreamCloser> stream(
        pdfioObjCreateStream(object, PDFIO_FILTER_FLATE));
    if (!stream) {
        error_message =
            "Unable to open PDF stream object: " + std::string{label};
        return nullptr;
    }

    if (size > 0U && !pdfioStreamWrite(stream.get(), data, size)) {
        error_message =
            "Unable to write PDF stream object: " + std::string{label};
        return nullptr;
    }

    if (!pdfioStreamClose(stream.release())) {
        error_message =
            "Unable to close PDF stream object: " + std::string{label};
        return nullptr;
    }

    return object;
}

[[nodiscard]] pdfio_obj_t *
create_stream_object_from_text(pdfio_file_t *pdf, pdfio_dict_t *dict,
                               const std::string &text,
                               std::string_view label,
                               std::string &error_message) {
    return create_stream_object_from_bytes(
        pdf, dict, reinterpret_cast<const unsigned char *>(text.data()),
        text.size(), label, error_message);
}

[[nodiscard]] pdfio_obj_t *
create_font_file_object(pdfio_file_t *pdf, const FontResourceKey &key,
                        std::string &error_message) {
    auto font_data = read_binary_file(key.file_path);
    if (font_data.empty()) {
        error_message = "Unable to read PDF glyph font file: " +
                        key.file_path.string();
        return nullptr;
    }

    pdfio_dict_t *dict = pdfioDictCreate(pdf);
    if (dict != nullptr) {
        pdfioDictSetName(dict, "Filter", "FlateDecode");
    }
    return create_stream_object_from_bytes(pdf, dict, font_data.data(),
                                           font_data.size(), font_label(key),
                                           error_message);
}

[[nodiscard]] pdfio_obj_t *
create_cid_to_gid_map_object(pdfio_file_t *pdf,
                             const GlyphFontResource &resource,
                             std::string_view label,
                             std::string &error_message) {
    const std::size_t byte_count = (resource.entries.size() + 1U) * 2U;
    std::vector<unsigned char> cid_to_gid(byte_count, 0U);
    for (const auto &entry : resource.entries) {
        const auto offset = static_cast<std::size_t>(entry.cid) * 2U;
        cid_to_gid[offset] = static_cast<unsigned char>(entry.glyph_id >> 8U);
        cid_to_gid[offset + 1U] =
            static_cast<unsigned char>(entry.glyph_id & 0xFFU);
    }

    pdfio_dict_t *dict = pdfioDictCreate(pdf);
    if (dict != nullptr) {
        pdfioDictSetName(dict, "Filter", "FlateDecode");
    }
    return create_stream_object_from_bytes(pdf, dict, cid_to_gid.data(),
                                           cid_to_gid.size(), label,
                                           error_message);
}

[[nodiscard]] pdfio_obj_t *
create_to_unicode_object(pdfio_file_t *pdf, const GlyphFontResource &resource,
                         std::string_view label,
                         std::string &error_message) {
    std::string cmap;
    cmap += "/CIDInit /ProcSet findresource begin\n";
    cmap += "12 dict begin\n";
    cmap += "begincmap\n";
    cmap += "/CIDSystemInfo << /Registry (Adobe) /Ordering (UCS) "
            "/Supplement 0 >> def\n";
    cmap += "/CMapName /FeatherDocGlyphToUnicode def\n";
    cmap += "/CMapType 2 def\n";
    cmap += "1 begincodespacerange\n";
    cmap += "<0000> <FFFF>\n";
    cmap += "endcodespacerange\n";

    std::vector<const GlyphCidEntry *> unicode_entries;
    unicode_entries.reserve(resource.entries.size());
    for (const auto &entry : resource.entries) {
        if (entry.unicode_text.empty()) {
            continue;
        }
        unicode_entries.push_back(&entry);
    }

    constexpr std::size_t kEntriesPerChunk = 100U;
    for (std::size_t offset = 0U; offset < unicode_entries.size();
         offset += kEntriesPerChunk) {
        const auto end =
            std::min(unicode_entries.size(), offset + kEntriesPerChunk);
        cmap += std::to_string(end - offset);
        cmap += " beginbfchar\n";

        for (std::size_t index = offset; index < end; ++index) {
            const auto &entry = *unicode_entries[index];
            cmap += "<";
            append_utf16be_hex_unit(cmap, entry.cid);
            cmap += "> <";
            cmap += utf16be_hex_payload_from_utf8(entry.unicode_text);
            cmap += ">\n";
        }
        cmap += "endbfchar\n";
    }

    cmap += "endcmap\n";
    cmap += "CMapName currentdict /CMap defineresource pop\n";
    cmap += "end\n";
    cmap += "end\n";

    pdfio_dict_t *dict = pdfioDictCreate(pdf);
    if (dict != nullptr) {
        pdfioDictSetName(dict, "Type", "CMap");
        pdfioDictSetName(dict, "CMapName", "FeatherDocGlyphToUnicode");
        pdfioDictSetName(dict, "Filter", "FlateDecode");
    }
    return create_stream_object_from_text(pdf, dict, cmap, label,
                                          error_message);
}

[[nodiscard]] pdfio_array_t *
create_shaped_width_array(pdfio_file_t *pdf,
                          const GlyphFontResource &resource) {
    pdfio_array_t *widths = pdfioArrayCreate(pdf);
    if (widths == nullptr) {
        return nullptr;
    }

    constexpr std::size_t kWidthsPerChunk = 256U;
    for (std::size_t offset = 0U; offset < resource.entries.size();
         offset += kWidthsPerChunk) {
        pdfioArrayAppendNumber(widths, resource.entries[offset].cid);

        pdfio_array_t *chunk = pdfioArrayCreate(pdf);
        if (chunk == nullptr) {
            return nullptr;
        }

        const auto end =
            std::min(resource.entries.size(), offset + kWidthsPerChunk);
        for (std::size_t index = offset; index < end; ++index) {
            pdfioArrayAppendNumber(chunk, resource.entries[index].width_1000);
        }
        pdfioArrayAppendArray(widths, chunk);
    }

    return widths;
}

[[nodiscard]] pdfio_obj_t *
create_shaped_glyph_font(pdfio_file_t *pdf, const FontResourceKey &key,
                         const GlyphFontResource &resource,
                         std::string &error_message) {
    if (resource.entries.empty()) {
        error_message = "Shaped PDF glyph font has no glyphs: " +
                        font_label(key);
        return nullptr;
    }

    FreeTypeFaceOwner face_owner(key.file_path);
    if (!face_owner.valid()) {
        error_message =
            "Unable to load shaped PDF glyph font: " + key.file_path.string();
        return nullptr;
    }

    pdfio_obj_t *font_file = create_font_file_object(pdf, key, error_message);
    if (font_file == nullptr) {
        return nullptr;
    }

    const auto base_font = base_font_name_for(face_owner.face, key);
    pdfio_array_t *bbox = pdfioArrayCreate(pdf);
    if (bbox == nullptr) {
        error_message = "Unable to create shaped PDF font bounding box";
        return nullptr;
    }
    pdfioArrayAppendNumber(bbox, font_unit_to_1000(face_owner.face,
                                                   face_owner.face->bbox.xMin));
    pdfioArrayAppendNumber(bbox, font_unit_to_1000(face_owner.face,
                                                   face_owner.face->bbox.yMin));
    pdfioArrayAppendNumber(bbox, font_unit_to_1000(face_owner.face,
                                                   face_owner.face->bbox.xMax));
    pdfioArrayAppendNumber(bbox, font_unit_to_1000(face_owner.face,
                                                   face_owner.face->bbox.yMax));

    pdfio_dict_t *descriptor = pdfioDictCreate(pdf);
    if (descriptor == nullptr) {
        error_message = "Unable to create shaped PDF font descriptor";
        return nullptr;
    }
    pdfioDictSetName(descriptor, "Type", "FontDescriptor");
    pdfioDictSetName(descriptor, "FontName", base_font.c_str());
    pdfioDictSetObj(descriptor, "FontFile2", font_file);
    pdfioDictSetNumber(descriptor, "Flags", key.italic ? 0x60 : 0x20);
    pdfioDictSetArray(descriptor, "FontBBox", bbox);
    pdfioDictSetNumber(descriptor, "ItalicAngle", key.italic ? -12.0 : 0.0);
    pdfioDictSetNumber(descriptor, "Ascent",
                       font_unit_to_1000(face_owner.face,
                                         face_owner.face->ascender));
    pdfioDictSetNumber(descriptor, "Descent",
                       font_unit_to_1000(face_owner.face,
                                         face_owner.face->descender));
    pdfioDictSetNumber(descriptor, "CapHeight",
                       font_unit_to_1000(face_owner.face,
                                         face_owner.face->ascender));
    pdfioDictSetNumber(descriptor, "StemV", key.bold ? 120.0 : 80.0);

    pdfio_obj_t *descriptor_obj = pdfioFileCreateObj(pdf, descriptor);
    if (descriptor_obj == nullptr || !pdfioObjClose(descriptor_obj)) {
        error_message = "Unable to create shaped PDF font descriptor object";
        return nullptr;
    }

    pdfio_obj_t *cid_to_gid =
        create_cid_to_gid_map_object(pdf, resource, font_label(key),
                                     error_message);
    if (cid_to_gid == nullptr) {
        return nullptr;
    }

    pdfio_obj_t *to_unicode =
        create_to_unicode_object(pdf, resource, font_label(key), error_message);
    if (to_unicode == nullptr) {
        return nullptr;
    }

    pdfio_array_t *widths = create_shaped_width_array(pdf, resource);
    if (widths == nullptr) {
        error_message = "Unable to create shaped PDF glyph widths";
        return nullptr;
    }

    pdfio_dict_t *system_info = pdfioDictCreate(pdf);
    if (system_info == nullptr) {
        error_message = "Unable to create shaped PDF CID system info";
        return nullptr;
    }
    pdfioDictSetString(system_info, "Registry", "Adobe");
    pdfioDictSetString(system_info, "Ordering", "Identity");
    pdfioDictSetNumber(system_info, "Supplement", 0.0);

    pdfio_dict_t *cid_font = pdfioDictCreate(pdf);
    if (cid_font == nullptr) {
        error_message = "Unable to create shaped PDF CID font";
        return nullptr;
    }
    pdfioDictSetName(cid_font, "Type", "Font");
    pdfioDictSetName(cid_font, "Subtype", "CIDFontType2");
    pdfioDictSetName(cid_font, "BaseFont", base_font.c_str());
    pdfioDictSetDict(cid_font, "CIDSystemInfo", system_info);
    pdfioDictSetObj(cid_font, "CIDToGIDMap", cid_to_gid);
    pdfioDictSetObj(cid_font, "FontDescriptor", descriptor_obj);
    pdfioDictSetArray(cid_font, "W", widths);

    pdfio_obj_t *cid_font_obj = pdfioFileCreateObj(pdf, cid_font);
    if (cid_font_obj == nullptr || !pdfioObjClose(cid_font_obj)) {
        error_message = "Unable to create shaped PDF CID font object";
        return nullptr;
    }

    pdfio_array_t *descendants = pdfioArrayCreate(pdf);
    if (descendants == nullptr ||
        !pdfioArrayAppendObj(descendants, cid_font_obj)) {
        error_message = "Unable to create shaped PDF descendant font list";
        return nullptr;
    }

    pdfio_dict_t *type0 = pdfioDictCreate(pdf);
    if (type0 == nullptr) {
        error_message = "Unable to create shaped PDF Type0 font";
        return nullptr;
    }
    pdfioDictSetName(type0, "Type", "Font");
    pdfioDictSetName(type0, "Subtype", "Type0");
    pdfioDictSetName(type0, "BaseFont", base_font.c_str());
    pdfioDictSetArray(type0, "DescendantFonts", descendants);
    pdfioDictSetName(type0, "Encoding", "Identity-H");
    pdfioDictSetObj(type0, "ToUnicode", to_unicode);

    pdfio_obj_t *font = pdfioFileCreateObj(pdf, type0);
    if (font != nullptr && pdfioObjClose(font)) {
        return font;
    }

    error_message = "Unable to create shaped PDF Type0 font object";
    return nullptr;
}

[[nodiscard]] bool write_actual_text_begin(pdfio_stream_t *stream,
                                           std::string_view text,
                                           std::string &error_message) {
    const auto hex = utf16be_hex_from_utf8(text);
    const auto command =
        std::string{"/Span <</ActualText <"} + hex + ">>> BDC\n";
    if (!pdfioStreamPuts(stream, command.c_str())) {
        error_message = "Unable to write PDF ActualText marker";
        return false;
    }
    return true;
}

[[nodiscard]] bool write_actual_text_end(pdfio_stream_t *stream,
                                         std::string &error_message) {
    if (!pdfioStreamPuts(stream, "EMC\n")) {
        error_message = "Unable to close PDF ActualText marker";
        return false;
    }
    return true;
}

[[nodiscard]] bool write_text_underline(pdfio_stream_t *stream,
                                        const PdfTextRun &text_run,
                                        std::string &error_message) {
    if (!text_run.underline || text_run.text.empty() ||
        text_run.font_size_points <= 0.0 ||
        std::abs(text_run.rotation_degrees) > 0.0001) {
        return true;
    }

    const auto underline_width = measure_text_width_points(
        text_run.text, text_run.font_size_points,
        PdfTextMetricsOptions{text_run.font_family, text_run.font_file_path});
    if (underline_width <= 0.0) {
        return true;
    }

    const double underline_y =
        text_run.baseline_origin.y_points - text_run.font_size_points * 0.12;
    const double line_width = std::max(0.5, text_run.font_size_points / 16.0);

    if (!pdfioContentSetStrokeColorDeviceRGB(stream, text_run.fill_color.red,
                                             text_run.fill_color.green,
                                             text_run.fill_color.blue) ||
        !pdfioContentSetLineWidth(stream, line_width) ||
        !pdfioContentSetLineCap(stream, PDFIO_LINECAP_BUTT) ||
        !pdfioContentSetDashPattern(stream, 0.0, 0.0, 0.0) ||
        !pdfioContentPathMoveTo(stream, text_run.baseline_origin.x_points,
                                underline_y) ||
        !pdfioContentPathLineTo(
            stream, text_run.baseline_origin.x_points + underline_width,
            underline_y) ||
        !pdfioContentStroke(stream)) {
        error_message = "Unable to write PDF text underline";
        return false;
    }

    return true;
}

[[nodiscard]] bool set_text_run_matrix(pdfio_stream_t *stream,
                                       const PdfTextRun &text_run) {
    if (std::abs(text_run.rotation_degrees) <= 0.0001) {
        if (text_run.synthetic_italic && text_run.italic) {
            constexpr double kSyntheticItalicDegrees = 12.0;
            const auto skew =
                std::tan(kSyntheticItalicDegrees * kPi / 180.0);
            pdfio_matrix_t matrix = {
                {1.0, 0.0},
                {skew, 1.0},
                {text_run.baseline_origin.x_points,
                 text_run.baseline_origin.y_points},
            };
            return pdfioContentSetTextMatrix(stream, matrix);
        }
        return pdfioContentTextMoveTo(stream,
                                      text_run.baseline_origin.x_points,
                                      text_run.baseline_origin.y_points);
    }

    const auto radians = text_run.rotation_degrees * kPi / 180.0;
    const auto cosine = std::cos(radians);
    const auto sine = std::sin(radians);
    pdfio_matrix_t matrix = {
        {cosine, sine},
        {-sine, cosine},
        {text_run.baseline_origin.x_points,
         text_run.baseline_origin.y_points},
    };
    return pdfioContentSetTextMatrix(stream, matrix);
}

[[nodiscard]] bool set_text_rendering_style(pdfio_stream_t *stream,
                                            const PdfTextRun &text_run) {
    const bool synthetic_bold = text_run.synthetic_bold && text_run.bold;
    if (!pdfioContentSetTextRenderingMode(
            stream, synthetic_bold ? PDFIO_TEXTRENDERING_FILL_AND_STROKE
                                   : PDFIO_TEXTRENDERING_FILL)) {
        return false;
    }

    if (!synthetic_bold) {
        return true;
    }

    const double stroke_width =
        std::max(0.2, text_run.font_size_points / 48.0);
    return pdfioContentSetStrokeColorDeviceRGB(
               stream, text_run.fill_color.red, text_run.fill_color.green,
               text_run.fill_color.blue) &&
           pdfioContentSetLineWidth(stream, stroke_width);
}

[[nodiscard]] bool write_shaped_glyph_text_show(
    pdfio_stream_t *stream, const PdfTextRun &text_run,
    const GlyphFontResources &glyph_resources,
    std::map<FontResourceKey, std::size_t> &glyph_offsets,
    std::string &error_message) {
    const auto font_key = make_font_key(text_run);
    const auto resource = glyph_resources.find(font_key);
    if (resource == glyph_resources.end()) {
        error_message =
            "Missing shaped PDF glyph resource: " + font_label(font_key);
        return false;
    }

    auto &offset = glyph_offsets[font_key];
    const auto glyph_count = text_run.glyph_run.glyphs.size();
    if (offset + glyph_count > resource->second.entries.size()) {
        error_message =
            "Shaped PDF glyph resource is shorter than its text run: " +
            font_label(font_key);
        return false;
    }

    std::string command;
    command.reserve(2U + glyph_count * 4U + 5U);
    command.push_back('<');
    for (std::size_t index = 0U; index < glyph_count; ++index) {
        const auto &entry = resource->second.entries[offset + index];
        const auto &glyph = text_run.glyph_run.glyphs[index];
        if (entry.glyph_id != glyph.glyph_id) {
            error_message =
                "Shaped PDF glyph resource no longer matches text run: " +
                font_label(font_key);
            return false;
        }
        append_utf16be_hex_unit(command, entry.cid);
    }
    command += "> Tj\n";
    offset += glyph_count;

    if (!pdfioStreamPuts(stream, command.c_str())) {
        error_message = "Unable to write shaped PDF glyph text";
        return false;
    }
    return true;
}

[[nodiscard]] bool write_text_run_content(
    pdfio_stream_t *stream, const PdfTextRun &text_run,
    const std::string &font_resource_name,
    const GlyphFontResources &glyph_resources,
    std::map<FontResourceKey, std::size_t> &glyph_offsets,
    std::string &error_message) {
    const auto font_key = make_font_key(text_run);
    if (!pdfioContentTextBegin(stream) ||
        !pdfioContentSetFillColorDeviceRGB(
            stream, text_run.fill_color.red, text_run.fill_color.green,
            text_run.fill_color.blue) ||
        !pdfioContentSetTextFont(stream, font_resource_name.c_str(),
                                 text_run.font_size_points) ||
        !set_text_rendering_style(stream, text_run) ||
        !set_text_run_matrix(stream, text_run)) {
        error_message = "Unable to begin PDF text run";
        return false;
    }

    const bool wrote_text =
        font_key.shaped_glyphs
            ? write_shaped_glyph_text_show(stream, text_run, glyph_resources,
                                           glyph_offsets, error_message)
            : pdfioContentTextShow(stream, text_run.unicode,
                                   text_run.text.c_str());
    if (!wrote_text) {
        if (error_message.empty()) {
            error_message = "Unable to write PDF text run";
        }
        return false;
    }

    if (!pdfioContentTextEnd(stream)) {
        error_message = "Unable to end PDF text run";
        return false;
    }
    return true;
}

[[nodiscard]] pdfio_rect_t
page_bounds_rect(const PdfPageSize &page_size) noexcept {
    return pdfio_rect_t{0.0, 0.0, page_size.width_points,
                        page_size.height_points};
}

[[nodiscard]] bool set_page_bounds(pdfio_dict_t *page_dict,
                                   const PdfPageLayout &page,
                                   std::string &error_message) {
    auto media_box = page_bounds_rect(page.size);
    auto crop_box = media_box;
    if (!pdfioDictSetRect(page_dict, "MediaBox", &media_box) ||
        !pdfioDictSetRect(page_dict, "CropBox", &crop_box)) {
        error_message = "Unable to set PDF page bounds";
        return false;
    }

    return true;
}

[[nodiscard]] bool has_image_crop(const PdfImage &image) noexcept {
    return image.crop_left_per_mille != 0U ||
           image.crop_top_per_mille != 0U ||
           image.crop_right_per_mille != 0U ||
           image.crop_bottom_per_mille != 0U;
}

[[nodiscard]] bool image_crop_is_valid(const PdfImage &image) noexcept {
    constexpr std::uint32_t max_crop_per_mille = 1000U;
    if (image.crop_left_per_mille > max_crop_per_mille ||
        image.crop_top_per_mille > max_crop_per_mille ||
        image.crop_right_per_mille > max_crop_per_mille ||
        image.crop_bottom_per_mille > max_crop_per_mille) {
        return false;
    }

    return image.crop_left_per_mille + image.crop_right_per_mille <
               max_crop_per_mille &&
           image.crop_top_per_mille + image.crop_bottom_per_mille <
               max_crop_per_mille;
}

[[nodiscard]] bool write_pdf_image(pdfio_stream_t *stream,
                                   const PdfImage &image,
                                   const std::string &resource_name,
                                   std::string &error_message) {
    if (!has_image_crop(image)) {
        if (!pdfioContentDrawImage(
                stream, resource_name.c_str(), image.bounds.x_points,
                image.bounds.y_points, image.bounds.width_points,
                image.bounds.height_points)) {
            error_message =
                "Unable to draw PDF image: " + image.source_path.string();
            return false;
        }
        return true;
    }

    if (!image_crop_is_valid(image)) {
        error_message =
            "Invalid PDF image crop: " + image.source_path.string();
        return false;
    }

    constexpr double crop_scale = 1000.0;
    const double crop_left =
        static_cast<double>(image.crop_left_per_mille) / crop_scale;
    const double crop_top =
        static_cast<double>(image.crop_top_per_mille) / crop_scale;
    const double crop_right =
        static_cast<double>(image.crop_right_per_mille) / crop_scale;
    const double crop_bottom =
        static_cast<double>(image.crop_bottom_per_mille) / crop_scale;
    const double visible_width = 1.0 - crop_left - crop_right;
    const double visible_height = 1.0 - crop_top - crop_bottom;
    const double draw_width = image.bounds.width_points / visible_width;
    const double draw_height = image.bounds.height_points / visible_height;

    const double draw_x = image.bounds.x_points - crop_left * draw_width;
    const double draw_y = image.bounds.y_points - crop_bottom * draw_height;

    if (!pdfioContentSave(stream) ||
        !pdfioContentPathRect(stream, image.bounds.x_points,
                              image.bounds.y_points,
                              image.bounds.width_points,
                              image.bounds.height_points) ||
        !pdfioContentClip(stream, false) ||
        !pdfioStreamPuts(stream, "n\n")) {
        error_message =
            "Unable to set PDF image crop: " + image.source_path.string();
        pdfioContentRestore(stream);
        return false;
    }

    if (!pdfioContentDrawImage(stream, resource_name.c_str(), draw_x, draw_y,
                               draw_width, draw_height)) {
        error_message =
            "Unable to draw cropped PDF image: " + image.source_path.string();
        pdfioContentRestore(stream);
        return false;
    }

    if (!pdfioContentRestore(stream)) {
        error_message =
            "Unable to restore PDF image crop: " + image.source_path.string();
        return false;
    }

    return true;
}

[[nodiscard]] bool write_page_images(
    pdfio_stream_t *stream, const PdfPageLayout &page,
    const std::map<ImageResourceKey, std::string> &image_resources,
    bool draw_behind_text, std::string &error_message) {
    for (const auto &image : page.images) {
        if (image.draw_behind_text != draw_behind_text) {
            continue;
        }

        const auto image_key =
            ImageResourceKey{image.source_path, image.interpolate};
        const auto image_resource = image_resources.find(image_key);
        if (image_resource == image_resources.end()) {
            error_message =
                "Missing PDF image resource: " + image.source_path.string();
            return false;
        }

        if (!write_pdf_image(stream, image, image_resource->second,
                             error_message)) {
            return false;
        }
    }

    return true;
}

[[nodiscard]] bool write_probe_page(pdfio_file_t *pdf,
                                    const PdfPageLayout &page,
                                    const PdfWriterOptions &options,
                                    FontSubsetStorage &subset_storage,
                                    std::string &error_message) {
    pdfio_dict_t *page_dict = pdfioDictCreate(pdf);
    if (page_dict == nullptr) {
        error_message = "Unable to create PDF page dictionary";
        return false;
    }

    if (!set_page_bounds(page_dict, page, error_message)) {
        return false;
    }

    GlyphFontResources glyph_font_resources;
    if (!collect_shaped_glyph_font_resources(page, glyph_font_resources,
                                             error_message)) {
        return false;
    }

    std::map<FontResourceKey, std::string> used_text_by_font;
    for (const auto &text_run : page.text_runs) {
        const auto font_key = make_font_key(text_run);
        if (!font_key.shaped_glyphs) {
            used_text_by_font[font_key] += text_run.text;
        }
    }

    std::map<FontResourceKey, std::string> font_resources;
    for (const auto &text_run : page.text_runs) {
        const auto font_key = make_font_key(text_run);
        if (font_resources.find(font_key) != font_resources.end()) {
            continue;
        }

        const std::string resource_name =
            "F" + std::to_string(font_resources.size() + 1);
        const auto *pdf_resource_name =
            make_pdfio_resource_name(pdf, resource_name, error_message);
        if (pdf_resource_name == nullptr) {
            return false;
        }
        if (font_key.shaped_glyphs) {
            const auto glyph_resource = glyph_font_resources.find(font_key);
            if (glyph_resource == glyph_font_resources.end()) {
                error_message =
                    "Missing shaped PDF glyph resource: " + font_label(font_key);
                return false;
            }
            pdfio_obj_t *font = create_shaped_glyph_font(
                pdf, font_key, glyph_resource->second, error_message);
            if (font == nullptr) {
                if (error_message.empty()) {
                    error_message =
                        "Unable to create shaped PDF glyph font: " +
                        font_label(font_key);
                }
                return false;
            }

            if (!pdfioPageDictAddFont(page_dict, pdf_resource_name, font)) {
                error_message =
                    "Unable to add PDF font resource: " + font_label(font_key);
                return false;
            }

            font_resources.emplace(font_key, resource_name);
            continue;
        }

        if (font_key.unicode && font_key.file_path.empty()) {
            error_message =
                "Unicode PDF text requires an embedded font file: " +
                font_label(font_key);
            return false;
        }
        const auto used_text = used_text_by_font.find(font_key);
        pdfio_obj_t *font = create_pdfio_font(
            pdf, font_key,
            used_text == used_text_by_font.end() ? std::string{}
                                                 : used_text->second,
            options.subset_unicode_fonts, subset_storage);
        if (font == nullptr) {
            error_message =
                "Unable to create PDF font: " + font_label(font_key);
            return false;
        }

        if (!pdfioPageDictAddFont(page_dict, pdf_resource_name, font)) {
            error_message =
                "Unable to add PDF font resource: " + font_label(font_key);
            return false;
        }

        font_resources.emplace(font_key, resource_name);
    }

    std::map<ImageResourceKey, std::string> image_resources;
    for (const auto &image : page.images) {
        if (image.source_path.empty()) {
            error_message = "PDF image source path is empty";
            return false;
        }

        const auto image_key =
            ImageResourceKey{image.source_path, image.interpolate};
        if (image_resources.find(image_key) != image_resources.end()) {
            continue;
        }

        const std::string resource_name =
            "Im" + std::to_string(image_resources.size() + 1);
        const auto *pdf_resource_name =
            make_pdfio_resource_name(pdf, resource_name, error_message);
        if (pdf_resource_name == nullptr) {
            return false;
        }
        const auto image_path = image.source_path.string();
        pdfio_obj_t *image_object = pdfioFileCreateImageObjFromFile(
            pdf, image_path.c_str(), image.interpolate);
        if (image_object == nullptr) {
            error_message =
                "Unable to create PDF image: " + image_label(image_key);
            return false;
        }

        if (!pdfioPageDictAddImage(page_dict, pdf_resource_name,
                                   image_object)) {
            error_message =
                "Unable to add PDF image resource: " + image_label(image_key);
            return false;
        }

        image_resources.emplace(image_key, resource_name);
    }

    std::unique_ptr<pdfio_stream_t, PdfioStreamCloser> stream(
        pdfioFileCreatePage(pdf, page_dict));
    if (!stream) {
        error_message = "Unable to create PDF page stream";
        return false;
    }

    for (const auto &rectangle : page.rectangles) {
        if (!rectangle.stroke && !rectangle.fill) {
            continue;
        }

        if (rectangle.fill &&
            !pdfioContentSetFillColorDeviceRGB(
                stream.get(), rectangle.fill_color.red,
                rectangle.fill_color.green, rectangle.fill_color.blue)) {
            error_message = "Unable to set PDF rectangle fill color";
            return false;
        }

        if (rectangle.stroke &&
            (!pdfioContentSetStrokeColorDeviceRGB(
                 stream.get(), rectangle.stroke_color.red,
                 rectangle.stroke_color.green, rectangle.stroke_color.blue) ||
             !pdfioContentSetLineWidth(stream.get(),
                                       rectangle.line_width_points))) {
            error_message = "Unable to set PDF rectangle stroke";
            return false;
        }

        if (!pdfioContentPathRect(stream.get(), rectangle.bounds.x_points,
                                  rectangle.bounds.y_points,
                                  rectangle.bounds.width_points,
                                  rectangle.bounds.height_points)) {
            error_message = "Unable to write PDF rectangle path";
            return false;
        }

        if (rectangle.fill && rectangle.stroke) {
            if (!pdfioContentFillAndStroke(stream.get(), false)) {
                error_message = "Unable to fill and stroke PDF rectangle";
                return false;
            }
        } else if (rectangle.fill) {
            if (!pdfioContentFill(stream.get(), false)) {
                error_message = "Unable to fill PDF rectangle";
                return false;
            }
        } else if (!pdfioContentStroke(stream.get())) {
            error_message = "Unable to stroke PDF rectangle";
            return false;
        }
    }

    for (const auto &line : page.lines) {
        if (!pdfioContentSetStrokeColorDeviceRGB(
                stream.get(), line.stroke_color.red, line.stroke_color.green,
                line.stroke_color.blue) ||
            !pdfioContentSetLineWidth(stream.get(), line.line_width_points) ||
            !pdfioContentSetLineCap(stream.get(),
                                    to_pdfio_line_cap(line.line_cap)) ||
            !pdfioContentSetDashPattern(stream.get(), line.dash_phase_points,
                                        line.dash_on_points,
                                        line.dash_off_points) ||
            !pdfioContentPathMoveTo(stream.get(), line.start.x_points,
                                    line.start.y_points) ||
            !pdfioContentPathLineTo(stream.get(), line.end.x_points,
                                    line.end.y_points) ||
            !pdfioContentStroke(stream.get())) {
            error_message = "Unable to write PDF line";
            return false;
        }
    }

    if (!write_page_images(stream.get(), page, image_resources, true,
                           error_message)) {
        return false;
    }

    std::map<FontResourceKey, std::size_t> glyph_resource_offsets;
    for (const auto &text_run : page.text_runs) {
        const auto font_key = make_font_key(text_run);
        const auto font_resource = font_resources.find(font_key);
        if (font_resource == font_resources.end()) {
            error_message =
                "Missing PDF font resource: " + font_label(font_key);
            return false;
        }

        if (!write_actual_text_begin(stream.get(), text_run.text,
                                     error_message)) {
            return false;
        }

        if (!write_text_run_content(stream.get(), text_run,
                                    font_resource->second,
                                    glyph_font_resources,
                                    glyph_resource_offsets, error_message)) {
            return false;
        }

        if (!write_actual_text_end(stream.get(), error_message)) {
            return false;
        }

        if (!write_text_underline(stream.get(), text_run, error_message)) {
            return false;
        }
    }

    if (!write_page_images(stream.get(), page, image_resources, false,
                           error_message)) {
        return false;
    }

    if (!pdfioStreamClose(stream.release())) {
        error_message = "Unable to close PDF page stream";
        return false;
    }

    return true;
}

} // namespace

PdfDocumentLayout make_pdfio_probe_layout(const PdfWriterOptions &options) {
    PdfDocumentLayout layout;
    layout.metadata.title = options.title;
    layout.metadata.creator = options.creator;

    PdfPageLayout page;
    page.size = options.page_size;

    const double width = std::max(page.size.width_points, 144.0);
    const double height = std::max(page.size.height_points, 144.0);

    page.rectangles.push_back(PdfRectangle{
        PdfRect{36.0, 36.0, width - 72.0, height - 72.0},
        PdfRgbColor{0.12, 0.16, 0.22},
        PdfRgbColor{1.0, 1.0, 1.0},
        1.0,
        true,
        false,
    });

    page.text_runs.push_back(PdfTextRun{
        PdfPoint{72.0, height - 96.0},
        options.title,
        "Helvetica",
        {},
        18.0,
        PdfRgbColor{0.0, 0.0, 0.0},
        false,
        false,
        false,
        false,
    });
    page.text_runs.push_back(PdfTextRun{
        PdfPoint{72.0, height - 120.0},
        "Generated by the FeatherDoc IPdfGenerator/PDFio prototype.",
        "Helvetica",
        {},
        10.0,
        PdfRgbColor{0.0, 0.0, 0.0},
        false,
        false,
        false,
        false,
    });

    layout.pages.push_back(std::move(page));
    return layout;
}

PdfWriteResult write_pdfio_document(const std::filesystem::path &output_path,
                                    const PdfDocumentLayout &layout,
                                    const PdfWriterOptions &options) {
    PdfioErrorState error_state;
    PdfWriteResult result;
    if (layout.pages.empty()) {
        result.error_message = "PDF layout does not contain any pages";
        return result;
    }

    ImageSourceCleanup image_cleanup;
    for (const auto &page : layout.pages) {
        for (const auto &image : page.images) {
            if (image.cleanup_source_after_write) {
                image_cleanup.paths.push_back(image.source_path);
            }
        }
    }

    const auto output_string = output_path.string();
    const PdfPageSize first_page_size = layout.pages.front().size;
    pdfio_rect_t media_box{0.0, 0.0, first_page_size.width_points,
                           first_page_size.height_points};

    std::unique_ptr<pdfio_file_t, PdfioFileCloser> pdf(
        pdfioFileCreate(output_string.c_str(), "1.7", &media_box, &media_box,
                        pdfio_error_callback, &error_state));
    if (!pdf) {
        result.error_message =
            build_error("Unable to create PDF file", error_state);
        return result;
    }

    const std::string creator = layout.metadata.creator.empty()
                                    ? options.creator
                                    : layout.metadata.creator;
    const std::string title =
        layout.metadata.title.empty() ? options.title : layout.metadata.title;

    pdfioFileSetCreator(pdf.get(), creator.c_str());
    pdfioFileSetTitle(pdf.get(), title.c_str());

    FontSubsetStorage font_subset_storage;
    for (const auto &page : layout.pages) {
        std::string page_error;
        if (!write_probe_page(pdf.get(), page, options, font_subset_storage,
                              page_error)) {
            result.error_message = build_error(page_error, error_state);
            return result;
        }
    }

    if (!pdfioFileClose(pdf.release())) {
        result.error_message =
            build_error("Unable to close PDF file", error_state);
        return result;
    }

    result.success = true;
    if (std::filesystem::exists(output_path)) {
        result.bytes_written = std::filesystem::file_size(output_path);
    }
    return result;
}

PdfWriteResult PdfioGenerator::write(const PdfDocumentLayout &layout,
                                     const std::filesystem::path &output_path,
                                     const PdfWriterOptions &options) {
    return write_pdfio_document(output_path, layout, options);
}

PdfWriteResult
write_pdfio_probe_document(const std::filesystem::path &output_path,
                           const PdfWriterOptions &options) {
    PdfioGenerator generator;
    return generator.write(make_pdfio_probe_layout(options), output_path,
                           options);
}

} // namespace featherdoc::pdf
