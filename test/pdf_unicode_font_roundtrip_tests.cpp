#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include <featherdoc/pdf/pdf_parser.hpp>
#include <featherdoc/pdf/pdf_text_shaper.hpp>
#include <featherdoc/pdf/pdf_writer.hpp>

#include <algorithm>
#include <array>
#include <cstdlib>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <zlib.h>

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

struct ExpectedToUnicodeEntry {
    std::uint16_t cid{0U};
    std::string unicode_text;
};

void append_hex_byte(std::string &output, unsigned int value) {
    constexpr char kHex[] = "0123456789ABCDEF";
    output.push_back(kHex[(value >> 4U) & 0x0FU]);
    output.push_back(kHex[value & 0x0FU]);
}

void append_utf16be_hex_unit(std::string &output, std::uint16_t value) {
    append_hex_byte(output, static_cast<unsigned int>(value >> 8U));
    append_hex_byte(output, static_cast<unsigned int>(value & 0xFFU));
}

[[nodiscard]] std::string utf16be_hex_unit(std::uint16_t value) {
    std::string output;
    append_utf16be_hex_unit(output, value);
    return output;
}

[[nodiscard]] std::uint32_t decode_utf8_codepoint(std::string_view text,
                                                  std::size_t &index) noexcept {
    const auto byte = static_cast<unsigned char>(text[index++]);
    if (byte < 0x80U) {
        return byte;
    }
    if ((byte >> 5U) == 0x06U && index < text.size()) {
        const auto next = static_cast<unsigned char>(text[index++]);
        return ((byte & 0x1FU) << 6U) | (next & 0x3FU);
    }
    if ((byte >> 4U) == 0x0EU && index + 1U < text.size()) {
        const auto second = static_cast<unsigned char>(text[index++]);
        const auto third = static_cast<unsigned char>(text[index++]);
        return ((byte & 0x0FU) << 12U) | ((second & 0x3FU) << 6U) |
               (third & 0x3FU);
    }
    if ((byte >> 3U) == 0x1EU && index + 2U < text.size()) {
        const auto second = static_cast<unsigned char>(text[index++]);
        const auto third = static_cast<unsigned char>(text[index++]);
        const auto fourth = static_cast<unsigned char>(text[index++]);
        return ((byte & 0x07U) << 18U) | ((second & 0x3FU) << 12U) |
               ((third & 0x3FU) << 6U) | (fourth & 0x3FU);
    }
    return 0xFFFDU;
}

[[nodiscard]] bool has_multiple_codepoints(std::string_view text) {
    std::size_t count = 0U;
    for (std::size_t index = 0U; index < text.size() && count < 2U;) {
        (void)decode_utf8_codepoint(text, index);
        ++count;
    }
    return count > 1U;
}

[[nodiscard]] std::string
utf16be_hex_payload_from_utf8(std::string_view text) {
    std::string hex;
    for (std::size_t index = 0U; index < text.size();) {
        auto codepoint = decode_utf8_codepoint(text, index);
        if (codepoint <= 0xFFFFU) {
            append_utf16be_hex_unit(hex, static_cast<std::uint16_t>(codepoint));
        } else {
            codepoint -= 0x10000U;
            append_utf16be_hex_unit(
                hex, static_cast<std::uint16_t>(0xD800U + (codepoint >> 10U)));
            append_utf16be_hex_unit(
                hex, static_cast<std::uint16_t>(0xDC00U + (codepoint & 0x3FFU)));
        }
    }
    return hex;
}

[[nodiscard]] std::vector<ExpectedToUnicodeEntry>
expected_to_unicode_entries(
    const featherdoc::pdf::PdfGlyphRun &glyph_run) {
    std::vector<ExpectedToUnicodeEntry> entries;
    for (std::size_t glyph_index = 0U; glyph_index < glyph_run.glyphs.size();
         ++glyph_index) {
        const auto cluster = glyph_run.glyphs[glyph_index].cluster;
        if (cluster >= glyph_run.text.size()) {
            continue;
        }

        bool already_mapped = false;
        for (std::size_t previous = 0U; previous < glyph_index; ++previous) {
            if (glyph_run.glyphs[previous].cluster == cluster) {
                already_mapped = true;
                break;
            }
        }
        if (already_mapped) {
            continue;
        }

        auto next_cluster = static_cast<std::uint32_t>(glyph_run.text.size());
        for (const auto &glyph : glyph_run.glyphs) {
            if (glyph.cluster > cluster && glyph.cluster < next_cluster) {
                next_cluster = glyph.cluster;
            }
        }
        if (next_cluster <= cluster || next_cluster > glyph_run.text.size()) {
            continue;
        }

        entries.push_back(ExpectedToUnicodeEntry{
            static_cast<std::uint16_t>(glyph_index + 1U),
            glyph_run.text.substr(cluster, next_cluster - cluster),
        });
    }
    return entries;
}

[[nodiscard]] std::optional<std::string>
inflate_zlib_payload(std::string_view payload) {
    z_stream stream{};
    stream.next_in = reinterpret_cast<Bytef *>(
        const_cast<char *>(payload.data()));
    stream.avail_in = static_cast<uInt>(payload.size());

    if (inflateInit(&stream) != Z_OK) {
        return std::nullopt;
    }

    std::string output;
    std::array<char, 4096> buffer{};
    int status = Z_OK;
    do {
        stream.next_out = reinterpret_cast<Bytef *>(buffer.data());
        stream.avail_out = static_cast<uInt>(buffer.size());
        status = inflate(&stream, Z_NO_FLUSH);
        if (status != Z_OK && status != Z_STREAM_END) {
            inflateEnd(&stream);
            return std::nullopt;
        }
        output.append(buffer.data(), buffer.size() - stream.avail_out);
    } while (status != Z_STREAM_END && stream.avail_in > 0U);

    inflateEnd(&stream);
    if (status != Z_STREAM_END) {
        return std::nullopt;
    }
    return output;
}

[[nodiscard]] std::vector<std::string>
inflated_pdf_streams(std::string_view pdf_bytes) {
    std::vector<std::string> streams;
    constexpr std::string_view kStreamMarker = "stream";
    constexpr std::string_view kEndStreamMarker = "endstream";
    std::size_t offset = 0U;
    while ((offset = pdf_bytes.find(kStreamMarker, offset)) !=
           std::string_view::npos) {
        auto data_start = offset + kStreamMarker.size();
        if (data_start + 1U < pdf_bytes.size() &&
            pdf_bytes[data_start] == '\r' && pdf_bytes[data_start + 1U] == '\n') {
            data_start += 2U;
        } else if (data_start < pdf_bytes.size() &&
                   pdf_bytes[data_start] == '\n') {
            data_start += 1U;
        }

        const auto data_end = pdf_bytes.find(kEndStreamMarker, data_start);
        if (data_end == std::string_view::npos) {
            break;
        }
        if (const auto inflated =
                inflate_zlib_payload(pdf_bytes.substr(data_start,
                                                       data_end - data_start));
            inflated.has_value()) {
            streams.push_back(*inflated);
        }
        offset = data_end + kEndStreamMarker.size();
    }
    return streams;
}

[[nodiscard]] std::string
find_shaped_to_unicode_cmap(const std::vector<std::string> &streams) {
    for (const auto &stream : streams) {
        if (stream.find("/CMapName /FeatherDocGlyphToUnicode def") !=
            std::string::npos) {
            return stream;
        }
    }
    return {};
}

[[nodiscard]] std::string
find_actual_text_content_stream(const std::vector<std::string> &streams) {
    for (const auto &stream : streams) {
        if (stream.find("/Span <</ActualText <") != std::string::npos &&
            stream.find(" Tj") != std::string::npos) {
            return stream;
        }
    }
    return {};
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

TEST_CASE("PDFio positions shaped glyph CIDs when glyph offsets are present") {
    const auto font_path = find_latin_font();
    if (font_path.empty()) {
        MESSAGE("skipping shaped glyph positioned writer smoke: configure "
                "test Latin font");
        return;
    }
    if (!featherdoc::pdf::pdf_text_shaper_has_harfbuzz()) {
        MESSAGE("skipping shaped glyph positioned writer smoke: HarfBuzz "
                "unavailable");
        return;
    }

    const std::string expected_text = "offset office";
    constexpr double font_size = 18.0;
    auto glyph_run = featherdoc::pdf::shape_pdf_text(
        expected_text,
        featherdoc::pdf::PdfTextShaperOptions{font_path, font_size});
    if (!glyph_run.used_harfbuzz || !glyph_run.error_message.empty() ||
        glyph_run.glyphs.size() < 2U) {
        MESSAGE("skipping shaped glyph positioned writer smoke: shaping "
                "failed");
        return;
    }

    glyph_run.glyphs[0].x_offset_points += 1.25;
    glyph_run.glyphs[0].y_offset_points += 2.0;
    glyph_run.glyphs[0].y_advance_points += 0.5;

    const auto glyph_count = glyph_run.glyphs.size();
    const auto output_path =
        std::filesystem::current_path() /
        "featherdoc-shaped-glyph-positioned.pdf";

    featherdoc::pdf::PdfDocumentLayout layout;
    layout.metadata.title = "FeatherDoc shaped glyph positioned";
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

    const auto streams = inflated_pdf_streams(pdf_bytes);
    REQUIRE_FALSE(streams.empty());
    const auto content_stream = find_actual_text_content_stream(streams);
    REQUIRE_MESSAGE(!content_stream.empty(),
                    "shaped glyph positioned content stream not found");
    CHECK_GE(count_occurrences(content_stream, " Tm\n"), glyph_count);
    CHECK_EQ(count_occurrences(content_stream, "> Tj\n"), glyph_count);

    featherdoc::pdf::PdfiumParser parser;
    const auto parse_result = parser.parse(output_path, {});
    REQUIRE_MESSAGE(parse_result.success, parse_result.error_message);

    const auto extracted_text = collect_text(parse_result.document);
    CHECK_EQ(count_occurrences(extracted_text, expected_text), 1U);
}

TEST_CASE("PDFio maps shaped glyph clusters in ToUnicode CMap") {
    if (!featherdoc::pdf::pdf_text_shaper_has_harfbuzz()) {
        MESSAGE("skipping shaped glyph ToUnicode smoke: HarfBuzz unavailable");
        return;
    }

    const std::string expected_text = "office affinity efficient";
    constexpr double font_size = 18.0;
    std::filesystem::path font_path;
    featherdoc::pdf::PdfGlyphRun glyph_run;
    std::vector<ExpectedToUnicodeEntry> expected_entries;
    for (const auto &candidate : candidate_latin_fonts()) {
        if (!std::filesystem::exists(candidate)) {
            continue;
        }

        auto candidate_run = featherdoc::pdf::shape_pdf_text(
            expected_text,
            featherdoc::pdf::PdfTextShaperOptions{candidate, font_size});
        if (!candidate_run.used_harfbuzz ||
            !candidate_run.error_message.empty() ||
            candidate_run.glyphs.empty()) {
            continue;
        }

        auto candidate_entries = expected_to_unicode_entries(candidate_run);
        const bool has_multi_codepoint_cluster =
            std::any_of(candidate_entries.begin(), candidate_entries.end(),
                        [](const ExpectedToUnicodeEntry &entry) {
                            return has_multiple_codepoints(entry.unicode_text);
                        });
        if (has_multi_codepoint_cluster) {
            font_path = candidate;
            glyph_run = std::move(candidate_run);
            expected_entries = std::move(candidate_entries);
            break;
        }
    }

    if (font_path.empty()) {
        MESSAGE("skipping shaped glyph ToUnicode smoke: no Latin font formed "
                "a multi-codepoint cluster");
        return;
    }
    REQUIRE_FALSE(expected_entries.empty());

    const auto output_path =
        std::filesystem::current_path() /
        "featherdoc-shaped-glyph-tounicode.pdf";

    featherdoc::pdf::PdfDocumentLayout layout;
    layout.metadata.title = "FeatherDoc shaped glyph ToUnicode";
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

    const auto streams = inflated_pdf_streams(pdf_bytes);
    REQUIRE_FALSE(streams.empty());
    const auto cmap = find_shaped_to_unicode_cmap(streams);
    REQUIRE_MESSAGE(!cmap.empty(), "shaped glyph ToUnicode CMap not found");

    std::size_t checked_entries = 0U;
    bool checked_multi_codepoint_cluster = false;
    for (const auto &entry : expected_entries) {
        const auto expected_cmap_entry =
            "<" + utf16be_hex_unit(entry.cid) + "> <" +
            utf16be_hex_payload_from_utf8(entry.unicode_text) + ">";
        CHECK_NE(cmap.find(expected_cmap_entry), std::string::npos);
        ++checked_entries;
        checked_multi_codepoint_cluster =
            checked_multi_codepoint_cluster ||
            has_multiple_codepoints(entry.unicode_text);
    }

    CHECK_GT(checked_entries, 0U);
    CHECK(checked_multi_codepoint_cluster);
}

TEST_CASE("PDFio maps only the first shaped glyph for repeated clusters") {
    const auto font_path = find_latin_font();
    if (font_path.empty()) {
        MESSAGE("skipping repeated shaped cluster smoke: configure test Latin "
                "font");
        return;
    }

    const auto expected_text = utf8_from_u8(u8"a\u0301b");
    constexpr double font_size = 18.0;
    featherdoc::pdf::PdfGlyphRun glyph_run;
    glyph_run.text = expected_text;
    glyph_run.font_file_path = font_path;
    glyph_run.font_size_points = font_size;
    glyph_run.used_harfbuzz = true;
    glyph_run.glyphs = {
        featherdoc::pdf::PdfGlyphPosition{1U, 0U, 6.0, 0.0, 0.0, 0.0},
        featherdoc::pdf::PdfGlyphPosition{2U, 0U, 0.0, 0.0, 0.0, 0.0},
        featherdoc::pdf::PdfGlyphPosition{3U, 3U, 6.0, 0.0, 0.0, 0.0},
    };

    const auto output_path =
        std::filesystem::current_path() /
        "featherdoc-shaped-glyph-repeated-cluster.pdf";

    featherdoc::pdf::PdfDocumentLayout layout;
    layout.metadata.title = "FeatherDoc repeated shaped cluster";
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

    const auto streams = inflated_pdf_streams(pdf_bytes);
    REQUIRE_FALSE(streams.empty());
    const auto cmap = find_shaped_to_unicode_cmap(streams);
    REQUIRE_MESSAGE(!cmap.empty(), "shaped glyph ToUnicode CMap not found");

    CHECK_NE(cmap.find("<0001> <00610301>"), std::string::npos);
    CHECK_EQ(cmap.find("<0002> <"), std::string::npos);
    CHECK_NE(cmap.find("<0003> <0062>"), std::string::npos);

    featherdoc::pdf::PdfiumParser parser;
    const auto parse_result = parser.parse(output_path, {});
    REQUIRE_MESSAGE(parse_result.success, parse_result.error_message);

    const auto extracted_text = collect_text(parse_result.document);
    CHECK_EQ(count_occurrences(extracted_text, expected_text), 1U);
}

TEST_CASE("PDFio falls back for shaped glyph clusters inside UTF-8 codepoints") {
    const auto font_path = find_latin_font();
    if (font_path.empty()) {
        MESSAGE("skipping shaped glyph UTF-8 boundary smoke: configure test "
                "Latin font");
        return;
    }

    const auto expected_text = utf8_from_u8(u8"a\u0301b");
    constexpr double font_size = 18.0;
    featherdoc::pdf::PdfGlyphRun glyph_run;
    glyph_run.text = expected_text;
    glyph_run.font_file_path = font_path;
    glyph_run.font_size_points = font_size;
    glyph_run.used_harfbuzz = true;
    glyph_run.glyphs = {
        featherdoc::pdf::PdfGlyphPosition{1U, 0U, 6.0, 0.0, 0.0, 0.0},
        featherdoc::pdf::PdfGlyphPosition{2U, 2U, 6.0, 0.0, 0.0, 0.0},
        featherdoc::pdf::PdfGlyphPosition{3U, 3U, 6.0, 0.0, 0.0, 0.0},
    };

    const auto output_path =
        std::filesystem::current_path() /
        "featherdoc-shaped-glyph-utf8-boundary-fallback.pdf";

    featherdoc::pdf::PdfDocumentLayout layout;
    layout.metadata.title = "FeatherDoc shaped glyph UTF-8 boundary";
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
    CHECK_EQ(pdf_bytes.find("/FeatherDocGlyph"), std::string::npos);

    featherdoc::pdf::PdfiumParser parser;
    const auto parse_result = parser.parse(output_path, {});
    REQUIRE_MESSAGE(parse_result.success, parse_result.error_message);

    const auto extracted_text = collect_text(parse_result.document);
    CHECK_NE(extracted_text.find(expected_text), std::string::npos);
}

TEST_CASE("PDFio falls back for non-forward shaped glyph clusters") {
    const auto font_path = find_latin_font();
    if (font_path.empty()) {
        MESSAGE("skipping shaped glyph fallback smoke: configure test Latin "
                "font");
        return;
    }

    const std::string expected_text = "abcdef";
    constexpr double font_size = 18.0;
    featherdoc::pdf::PdfGlyphRun glyph_run;
    glyph_run.text = expected_text;
    glyph_run.font_file_path = font_path;
    glyph_run.font_size_points = font_size;
    glyph_run.used_harfbuzz = true;
    glyph_run.glyphs = {
        featherdoc::pdf::PdfGlyphPosition{1U, 4U, 6.0, 0.0, 0.0, 0.0},
        featherdoc::pdf::PdfGlyphPosition{2U, 2U, 6.0, 0.0, 0.0, 0.0},
        featherdoc::pdf::PdfGlyphPosition{3U, 0U, 6.0, 0.0, 0.0, 0.0},
    };

    const auto output_path =
        std::filesystem::current_path() /
        "featherdoc-shaped-glyph-cluster-fallback.pdf";

    featherdoc::pdf::PdfDocumentLayout layout;
    layout.metadata.title = "FeatherDoc shaped glyph fallback";
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
    CHECK_EQ(pdf_bytes.find("/FeatherDocGlyph"), std::string::npos);

    featherdoc::pdf::PdfiumParser parser;
    const auto parse_result = parser.parse(output_path, {});
    REQUIRE_MESSAGE(parse_result.success, parse_result.error_message);

    const auto extracted_text = collect_text(parse_result.document);
    CHECK_NE(extracted_text.find(expected_text), std::string::npos);
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
