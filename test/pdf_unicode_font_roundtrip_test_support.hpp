#pragma once

#include "doctest.h"

#include <featherdoc.hpp>
#include <featherdoc/pdf/pdf_document_adapter.hpp>
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
