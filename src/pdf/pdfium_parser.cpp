#include <featherdoc/pdf/pdf_parser.hpp>

#include <cstddef>
#include <cstdint>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

extern "C" {
#include <fpdf_edit.h>
#include <fpdf_text.h>
#include <fpdfview.h>
}

#ifndef MINIZ_HEADER_FILE_ONLY
#define MINIZ_HEADER_FILE_ONLY 1
#endif
#include <miniz.h>

namespace featherdoc::pdf {
namespace {

struct PdfiumDocumentCloser {
    void operator()(FPDF_DOCUMENT document) const {
        if (document != nullptr) {
            FPDF_CloseDocument(document);
        }
    }
};

struct PdfiumPageCloser {
    void operator()(FPDF_PAGE page) const {
        if (page != nullptr) {
            FPDF_ClosePage(page);
        }
    }
};

struct PdfiumTextPageCloser {
    void operator()(FPDF_TEXTPAGE text_page) const {
        if (text_page != nullptr) {
            FPDFText_ClosePage(text_page);
        }
    }
};

using PdfiumDocumentPtr =
    std::unique_ptr<std::remove_pointer_t<FPDF_DOCUMENT>, PdfiumDocumentCloser>;
using PdfiumPagePtr =
    std::unique_ptr<std::remove_pointer_t<FPDF_PAGE>, PdfiumPageCloser>;
using PdfiumTextPagePtr =
    std::unique_ptr<std::remove_pointer_t<FPDF_TEXTPAGE>, PdfiumTextPageCloser>;

struct TextCluster {
    std::string text;
    PdfRect bounds;
};

struct CandidateLine {
    std::size_t line_index{0U};
    std::vector<TextCluster> clusters;
    double center_y{0.0};
};

struct PdfIndirectRef {
    unsigned int object_number{0U};
    unsigned int generation{0U};
};

struct PdfiumLibrary {
    PdfiumLibrary() {
        FPDF_InitLibrary();
    }

    ~PdfiumLibrary() {
        FPDF_DestroyLibrary();
    }

    PdfiumLibrary(const PdfiumLibrary &) = delete;
    PdfiumLibrary &operator=(const PdfiumLibrary &) = delete;
};

void ensure_pdfium_initialized() {
    static PdfiumLibrary library;
}

[[nodiscard]] std::string pdfium_error_to_string(unsigned long error_code) {
    switch (error_code) {
    case FPDF_ERR_SUCCESS:
        return "success";
    case FPDF_ERR_UNKNOWN:
        return "unknown error";
    case FPDF_ERR_FILE:
        return "file access or format error";
    case FPDF_ERR_FORMAT:
        return "invalid PDF format";
    case FPDF_ERR_PASSWORD:
        return "password is required or incorrect";
    case FPDF_ERR_SECURITY:
        return "unsupported security scheme";
    case FPDF_ERR_PAGE:
        return "page not found or content error";
    default:
        return "unrecognized PDFium error " + std::to_string(error_code);
    }
}

void append_utf8(std::string &output, std::uint32_t codepoint) {
    if (codepoint <= 0x7F) {
        output.push_back(static_cast<char>(codepoint));
    } else if (codepoint <= 0x7FF) {
        output.push_back(static_cast<char>(0xC0 | ((codepoint >> 6) & 0x1F)));
        output.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
    } else if (codepoint <= 0xFFFF) {
        output.push_back(static_cast<char>(0xE0 | ((codepoint >> 12) & 0x0F)));
        output.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
        output.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
    } else if (codepoint <= 0x10FFFF) {
        output.push_back(static_cast<char>(0xF0 | ((codepoint >> 18) & 0x07)));
        output.push_back(static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F)));
        output.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
        output.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
    }
}

[[nodiscard]] PdfParsedTextSpan parse_text_span(FPDF_TEXTPAGE text_page,
                                                int char_index,
                                                const PdfParseOptions &options) {
    PdfParsedTextSpan span;
    append_utf8(span.text,
                static_cast<std::uint32_t>(
                    FPDFText_GetUnicode(text_page, char_index)));
    span.font_size_points = FPDFText_GetFontSize(text_page, char_index);

    if (options.extract_geometry) {
        double left = 0.0;
        double right = 0.0;
        double bottom = 0.0;
        double top = 0.0;
        if (FPDFText_GetCharBox(text_page, char_index, &left, &right, &bottom,
                                &top)) {
            span.bounds = PdfRect{left, bottom, right - left, top - bottom};
        }
    }

    return span;
}

void append_utf16_to_utf8(std::string &output,
                          const std::vector<FPDF_WCHAR> &text,
                          std::size_t char_count) {
    for (std::size_t index = 0; index < char_count; ++index) {
        const auto value = static_cast<std::uint32_t>(text[index]);
        if (value == 0U) {
            continue;
        }

        if (value >= 0xD800U && value <= 0xDBFFU &&
            index + 1U < char_count) {
            const auto low = static_cast<std::uint32_t>(text[index + 1U]);
            if (low >= 0xDC00U && low <= 0xDFFFU) {
                const auto codepoint =
                    0x10000U + ((value - 0xD800U) << 10U) +
                    (low - 0xDC00U);
                append_utf8(output, codepoint);
                ++index;
                continue;
            }
        }

        append_utf8(output, value);
    }
}

[[nodiscard]] std::size_t visible_text_byte_count(std::string_view text) {
    std::size_t count = 0U;
    for (const unsigned char ch : text) {
        if (!std::isspace(ch)) {
            ++count;
        }
    }

    return count;
}

[[nodiscard]] std::size_t
visible_text_byte_count(const std::vector<PdfParsedTextSpan> &spans) {
    std::size_t count = 0U;
    for (const auto &span : spans) {
        count += visible_text_byte_count(span.text);
    }

    return count;
}

[[nodiscard]] std::string read_pdf_bytes(const std::filesystem::path &path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        return {};
    }

    return {std::istreambuf_iterator<char>(input),
            std::istreambuf_iterator<char>()};
}

[[nodiscard]] bool is_pdf_name_character(unsigned char ch) noexcept {
    return std::isalnum(ch) || ch == '#' || ch == '-' || ch == '_';
}

void skip_pdf_whitespace(std::string_view text, std::size_t &offset) {
    while (offset < text.size() &&
           std::isspace(static_cast<unsigned char>(text[offset]))) {
        ++offset;
    }
}

[[nodiscard]] std::optional<unsigned int>
parse_pdf_unsigned_integer(std::string_view text, std::size_t &offset) {
    skip_pdf_whitespace(text, offset);
    if (offset >= text.size() ||
        !std::isdigit(static_cast<unsigned char>(text[offset]))) {
        return std::nullopt;
    }

    unsigned int value = 0U;
    while (offset < text.size() &&
           std::isdigit(static_cast<unsigned char>(text[offset]))) {
        value = value * 10U +
                static_cast<unsigned int>(text[offset] - '0');
        ++offset;
    }
    return value;
}

[[nodiscard]] std::optional<PdfIndirectRef>
parse_pdf_indirect_ref(std::string_view text, std::size_t &offset) {
    const auto original_offset = offset;
    const auto object_number = parse_pdf_unsigned_integer(text, offset);
    if (!object_number.has_value()) {
        offset = original_offset;
        return std::nullopt;
    }
    const auto generation = parse_pdf_unsigned_integer(text, offset);
    if (!generation.has_value()) {
        offset = original_offset;
        return std::nullopt;
    }

    skip_pdf_whitespace(text, offset);
    if (offset >= text.size() || text[offset] != 'R') {
        offset = original_offset;
        return std::nullopt;
    }
    ++offset;

    return PdfIndirectRef{*object_number, *generation};
}

[[nodiscard]] std::vector<PdfIndirectRef>
extract_page_content_refs(std::string_view page_object) {
    std::vector<PdfIndirectRef> refs;
    const auto contents_pos = page_object.find("/Contents");
    if (contents_pos == std::string_view::npos) {
        return refs;
    }

    std::size_t offset = contents_pos + std::string_view{"/Contents"}.size();
    skip_pdf_whitespace(page_object, offset);
    if (offset >= page_object.size()) {
        return refs;
    }

    if (page_object[offset] != '[') {
        if (auto ref = parse_pdf_indirect_ref(page_object, offset);
            ref.has_value()) {
            refs.push_back(*ref);
        }
        return refs;
    }

    const auto array_end = page_object.find(']', offset + 1U);
    if (array_end == std::string_view::npos) {
        return refs;
    }

    ++offset;
    while (offset < array_end) {
        const auto ref_offset = offset;
        if (auto ref = parse_pdf_indirect_ref(page_object, offset);
            ref.has_value()) {
            refs.push_back(*ref);
            continue;
        }
        offset = ref_offset + 1U;
    }
    return refs;
}

[[nodiscard]] std::vector<std::vector<PdfIndirectRef>>
collect_page_content_refs(std::string_view pdf_bytes) {
    std::vector<std::vector<PdfIndirectRef>> pages;

    constexpr std::string_view kTypePage = "/Type/Page";
    std::size_t offset = 0U;
    while ((offset = pdf_bytes.find(kTypePage, offset)) !=
           std::string_view::npos) {
        const auto after_type = offset + kTypePage.size();
        if (after_type < pdf_bytes.size() &&
            is_pdf_name_character(
                static_cast<unsigned char>(pdf_bytes[after_type]))) {
            offset = after_type;
            continue;
        }

        const auto obj_keyword = pdf_bytes.rfind(" obj", offset);
        const auto end_obj = pdf_bytes.find("endobj", offset);
        if (obj_keyword == std::string_view::npos ||
            end_obj == std::string_view::npos) {
            offset = after_type;
            continue;
        }

        const auto object_start = pdf_bytes.rfind('\n', obj_keyword);
        const auto page_object_start =
            object_start == std::string_view::npos ? 0U : object_start + 1U;
        auto refs = extract_page_content_refs(
            pdf_bytes.substr(page_object_start, end_obj - page_object_start));
        if (!refs.empty()) {
            pages.push_back(std::move(refs));
        }
        offset = end_obj + std::string_view{"endobj"}.size();
    }

    return pages;
}

[[nodiscard]] std::optional<std::string_view>
find_pdf_stream_payload(std::string_view pdf_bytes, PdfIndirectRef ref) {
    const auto object_header = std::to_string(ref.object_number) + " " +
                               std::to_string(ref.generation) + " obj";
    std::size_t object_pos = std::string_view::npos;
    for (std::size_t search_offset = 0U;;) {
        const auto candidate = pdf_bytes.find(object_header, search_offset);
        if (candidate == std::string_view::npos) {
            break;
        }
        const bool starts_at_boundary =
            candidate == 0U ||
            std::isspace(
                static_cast<unsigned char>(pdf_bytes[candidate - 1U]));
        const auto after_header = candidate + object_header.size();
        const bool ends_at_boundary =
            after_header >= pdf_bytes.size() ||
            std::isspace(
                static_cast<unsigned char>(pdf_bytes[after_header]));
        if (starts_at_boundary && ends_at_boundary) {
            object_pos = candidate;
            break;
        }
        search_offset = candidate + 1U;
    }
    if (object_pos == std::string_view::npos) {
        return std::nullopt;
    }

    const auto end_obj = pdf_bytes.find("endobj", object_pos);
    const auto stream_pos = pdf_bytes.find("stream", object_pos);
    if (end_obj == std::string_view::npos ||
        stream_pos == std::string_view::npos || stream_pos > end_obj) {
        return std::nullopt;
    }

    auto data_start = stream_pos + std::string_view{"stream"}.size();
    if (data_start + 1U < pdf_bytes.size() && pdf_bytes[data_start] == '\r' &&
        pdf_bytes[data_start + 1U] == '\n') {
        data_start += 2U;
    } else if (data_start < pdf_bytes.size() && pdf_bytes[data_start] == '\n') {
        ++data_start;
    }

    auto data_end = pdf_bytes.find("endstream", data_start);
    if (data_end == std::string_view::npos || data_end > end_obj) {
        return std::nullopt;
    }
    while (data_end > data_start &&
           (pdf_bytes[data_end - 1U] == '\r' ||
            pdf_bytes[data_end - 1U] == '\n')) {
        --data_end;
    }

    return pdf_bytes.substr(data_start, data_end - data_start);
}

[[nodiscard]] std::optional<std::string>
inflate_pdf_stream(std::string_view stream_payload) {
    size_t output_size = 0U;
    void *output = tinfl_decompress_mem_to_heap(
        stream_payload.data(), stream_payload.size(), &output_size,
        TINFL_FLAG_PARSE_ZLIB_HEADER);
    if (output == nullptr) {
        return std::nullopt;
    }

    std::string inflated(static_cast<const char *>(output), output_size);
    mz_free(output);
    return inflated;
}

[[nodiscard]] std::optional<unsigned int>
parse_hex_byte(std::string_view hex, std::size_t offset) {
    if (offset + 1U >= hex.size()) {
        return std::nullopt;
    }

    auto parse_digit = [](char ch) -> std::optional<unsigned int> {
        if (ch >= '0' && ch <= '9') {
            return static_cast<unsigned int>(ch - '0');
        }
        if (ch >= 'A' && ch <= 'F') {
            return static_cast<unsigned int>(ch - 'A' + 10);
        }
        if (ch >= 'a' && ch <= 'f') {
            return static_cast<unsigned int>(ch - 'a' + 10);
        }
        return std::nullopt;
    };

    const auto high = parse_digit(hex[offset]);
    const auto low = parse_digit(hex[offset + 1U]);
    if (!high.has_value() || !low.has_value()) {
        return std::nullopt;
    }

    return (*high << 4U) | *low;
}

[[nodiscard]] std::optional<std::string>
decode_actual_text_hex(std::string_view hex) {
    if (hex.size() % 2U != 0U) {
        return std::nullopt;
    }

    std::vector<unsigned char> bytes;
    bytes.reserve(hex.size() / 2U);
    for (std::size_t offset = 0U; offset < hex.size(); offset += 2U) {
        const auto byte = parse_hex_byte(hex, offset);
        if (!byte.has_value()) {
            return std::nullopt;
        }
        bytes.push_back(static_cast<unsigned char>(*byte));
    }

    bool little_endian = false;
    std::size_t byte_offset = 0U;
    if (bytes.size() >= 2U && bytes[0] == 0xFEU && bytes[1] == 0xFFU) {
        byte_offset = 2U;
    } else if (bytes.size() >= 2U && bytes[0] == 0xFFU && bytes[1] == 0xFEU) {
        little_endian = true;
        byte_offset = 2U;
    }

    if ((bytes.size() - byte_offset) % 2U != 0U) {
        return std::nullopt;
    }

    std::vector<FPDF_WCHAR> units;
    units.reserve((bytes.size() - byte_offset) / 2U);
    for (; byte_offset + 1U < bytes.size(); byte_offset += 2U) {
        const auto high = little_endian ? bytes[byte_offset + 1U]
                                        : bytes[byte_offset];
        const auto low = little_endian ? bytes[byte_offset]
                                       : bytes[byte_offset + 1U];
        units.push_back(
            static_cast<FPDF_WCHAR>((static_cast<unsigned int>(high) << 8U) |
                                    static_cast<unsigned int>(low)));
    }

    std::string output;
    append_utf16_to_utf8(output, units, units.size());
    return output;
}

[[nodiscard]] std::vector<std::string>
extract_actual_text_spans(std::string_view content_stream) {
    std::vector<std::string> spans;
    constexpr std::string_view kActualTextMarker = "/Span <</ActualText <";
    constexpr std::string_view kActualTextClose = ">>> BDC";

    std::size_t offset = 0U;
    while ((offset = content_stream.find(kActualTextMarker, offset)) !=
           std::string_view::npos) {
        const auto hex_start = offset + kActualTextMarker.size();
        const auto hex_end = content_stream.find('>', hex_start);
        if (hex_end == std::string_view::npos) {
            break;
        }

        if (content_stream.substr(hex_end, kActualTextClose.size()) ==
            kActualTextClose) {
            if (auto decoded = decode_actual_text_hex(
                    content_stream.substr(hex_start, hex_end - hex_start));
                decoded.has_value() && !decoded->empty()) {
                spans.push_back(std::move(*decoded));
            }
        }
        offset = hex_end + 1U;
    }

    return spans;
}

[[nodiscard]] std::vector<std::vector<std::string>>
extract_page_actual_text_spans(const std::filesystem::path &input_path) {
    const auto pdf_bytes = read_pdf_bytes(input_path);
    if (pdf_bytes.empty()) {
        return {};
    }

    std::vector<std::vector<std::string>> page_actual_texts;
    const auto page_refs = collect_page_content_refs(pdf_bytes);
    page_actual_texts.reserve(page_refs.size());
    for (const auto &refs : page_refs) {
        std::vector<std::string> actual_texts;
        for (const auto &ref : refs) {
            const auto payload = find_pdf_stream_payload(pdf_bytes, ref);
            if (!payload.has_value()) {
                continue;
            }
            const auto inflated = inflate_pdf_stream(*payload);
            if (!inflated.has_value()) {
                continue;
            }
            auto spans = extract_actual_text_spans(*inflated);
            actual_texts.insert(actual_texts.end(),
                                std::make_move_iterator(spans.begin()),
                                std::make_move_iterator(spans.end()));
        }
        page_actual_texts.push_back(std::move(actual_texts));
    }

    return page_actual_texts;
}

[[nodiscard]] std::string
collect_span_text(const std::vector<PdfParsedTextSpan> &spans) {
    std::string text;
    for (const auto &span : spans) {
        text += span.text;
    }
    return text;
}

[[nodiscard]] bool missing_actual_text_span(
    const std::vector<PdfParsedTextSpan> &spans,
    const std::vector<std::string> &actual_text_spans) {
    const auto text = collect_span_text(spans);
    return std::any_of(actual_text_spans.begin(), actual_text_spans.end(),
                       [&text](const std::string &actual_text) {
                           return visible_text_byte_count(actual_text) > 0U &&
                                  text.find(actual_text) == std::string::npos;
                       });
}

[[nodiscard]] std::size_t count_text_occurrences(std::string_view haystack,
                                                 std::string_view needle) {
    if (needle.empty()) {
        return 0U;
    }

    std::size_t count = 0U;
    std::size_t offset = 0U;
    while ((offset = haystack.find(needle, offset)) !=
           std::string_view::npos) {
        ++count;
        offset += needle.size();
    }
    return count;
}

[[nodiscard]] bool should_prefer_actual_text_spans(
    const std::vector<PdfParsedTextSpan> &spans,
    const std::vector<std::string> &actual_text_spans) {
    const auto text = collect_span_text(spans);
    return std::any_of(actual_text_spans.begin(), actual_text_spans.end(),
                       [&text](const std::string &actual_text) {
                           return visible_text_byte_count(actual_text) > 0U &&
                                  count_text_occurrences(text, actual_text) !=
                                      1U;
                       });
}

[[nodiscard]] std::vector<PdfParsedTextSpan> build_actual_text_spans(
    const std::vector<PdfParsedTextSpan> &object_spans,
    const std::vector<std::string> &actual_text_spans) {
    std::vector<PdfParsedTextSpan> spans;
    spans.reserve(actual_text_spans.size());
    for (std::size_t index = 0U; index < actual_text_spans.size(); ++index) {
        const auto &actual_text = actual_text_spans[index];
        if (visible_text_byte_count(actual_text) == 0U) {
            continue;
        }

        PdfParsedTextSpan span;
        span.text = actual_text;
        if (index < object_spans.size()) {
            span.bounds = object_spans[index].bounds;
            span.font_name = object_spans[index].font_name;
            span.font_size_points = object_spans[index].font_size_points;
        }
        spans.push_back(std::move(span));
    }

    if (spans.size() == 1U && !object_spans.empty()) {
        PdfRect bounds;
        for (const auto &object_span : object_spans) {
            if (bounds.width_points <= 0.0 && bounds.height_points <= 0.0) {
                bounds = object_span.bounds;
                continue;
            }

            const double left =
                (std::min)(bounds.x_points, object_span.bounds.x_points);
            const double bottom =
                (std::min)(bounds.y_points, object_span.bounds.y_points);
            const double right = (std::max)(
                bounds.x_points + bounds.width_points,
                object_span.bounds.x_points + object_span.bounds.width_points);
            const double top = (std::max)(
                bounds.y_points + bounds.height_points,
                object_span.bounds.y_points + object_span.bounds.height_points);
            bounds = PdfRect{left, bottom, right - left, top - bottom};
        }
        spans.front().bounds = bounds;
    }

    return spans;
}

[[nodiscard]] bool should_prefer_text_object_spans(
    const std::vector<PdfParsedTextSpan> &char_spans,
    const std::vector<PdfParsedTextSpan> &object_spans) {
    const auto object_visible = visible_text_byte_count(object_spans);
    if (object_visible == 0U) {
        return false;
    }

    const auto char_visible = visible_text_byte_count(char_spans);
    if (char_visible == 0U) {
        return true;
    }

    return object_visible >= char_visible + 8U &&
           object_visible >= char_visible * 2U;
}

[[nodiscard]] std::vector<std::string> split_utf8_characters(
    std::string_view text) {
    std::vector<std::string> characters;
    characters.reserve(text.size());

    for (std::size_t index = 0U; index < text.size();) {
        const auto lead = static_cast<unsigned char>(text[index]);
        std::size_t length = 1U;
        if ((lead & 0xE0U) == 0xC0U) {
            length = 2U;
        } else if ((lead & 0xF0U) == 0xE0U) {
            length = 3U;
        } else if ((lead & 0xF8U) == 0xF0U) {
            length = 4U;
        }
        if (index + length > text.size()) {
            length = 1U;
        }

        characters.emplace_back(text.substr(index, length));
        index += length;
    }

    return characters;
}

[[nodiscard]] std::string read_text_object_text(FPDF_PAGEOBJECT text_object,
                                                FPDF_TEXTPAGE text_page) {
    const unsigned long byte_count =
        FPDFTextObj_GetText(text_object, text_page, nullptr, 0);
    if (byte_count <= sizeof(FPDF_WCHAR)) {
        return {};
    }

    const auto char_capacity =
        static_cast<std::size_t>((byte_count + sizeof(FPDF_WCHAR) - 1U) /
                                 sizeof(FPDF_WCHAR));
    std::vector<FPDF_WCHAR> buffer(char_capacity, 0);
    const unsigned long copied =
        FPDFTextObj_GetText(text_object, text_page, buffer.data(), byte_count);
    if (copied <= sizeof(FPDF_WCHAR)) {
        return {};
    }

    const auto copied_chars =
        static_cast<std::size_t>(copied / sizeof(FPDF_WCHAR));
    std::string output;
    output.reserve(copied_chars);
    append_utf16_to_utf8(output, buffer, copied_chars);
    return output;
}

[[nodiscard]] std::string
read_mark_string_value(FPDF_PAGEOBJECTMARK mark, FPDF_BYTESTRING key) {
    unsigned long byte_count = 0;
    if (!FPDFPageObjMark_GetParamStringValue(mark, key, nullptr, 0,
                                             &byte_count) ||
        byte_count <= sizeof(FPDF_WCHAR)) {
        return {};
    }

    const auto char_capacity =
        static_cast<std::size_t>((byte_count + sizeof(FPDF_WCHAR) - 1U) /
                                 sizeof(FPDF_WCHAR));
    std::vector<FPDF_WCHAR> buffer(char_capacity, 0);
    unsigned long copied = 0;
    if (!FPDFPageObjMark_GetParamStringValue(mark, key, buffer.data(),
                                             byte_count, &copied) ||
        copied <= sizeof(FPDF_WCHAR)) {
        return {};
    }

    const auto copied_chars =
        static_cast<std::size_t>(copied / sizeof(FPDF_WCHAR));
    std::string output;
    output.reserve(copied_chars);
    append_utf16_to_utf8(output, buffer, copied_chars);
    return output;
}

[[nodiscard]] std::string read_text_object_actual_text(
    FPDF_PAGEOBJECT text_object) {
    const int mark_count = FPDFPageObj_CountMarks(text_object);
    for (int mark_index = 0; mark_index < mark_count; ++mark_index) {
        auto *mark = FPDFPageObj_GetMark(
            text_object, static_cast<unsigned long>(mark_index));
        if (mark == nullptr) {
            continue;
        }

        auto actual_text = read_mark_string_value(mark, "ActualText");
        if (!actual_text.empty()) {
            return actual_text;
        }
    }

    return {};
}

void append_text_object_span(FPDF_PAGEOBJECT text_object,
                             FPDF_TEXTPAGE text_page,
                             PdfParsedPage &page,
                             const PdfParseOptions &options,
                             bool split_multichar_text) {
    auto text = read_text_object_text(text_object, text_page);
    auto actual_text = read_text_object_actual_text(text_object);
    if (visible_text_byte_count(text) == 0U &&
        visible_text_byte_count(actual_text) > 0U) {
        text = std::move(actual_text);
    }
    if (text.empty()) {
        return;
    }

    PdfParsedTextSpan span;
    span.text = std::move(text);

    float font_size = 0.0F;
    if (FPDFTextObj_GetFontSize(text_object, &font_size)) {
        span.font_size_points = static_cast<double>(font_size);
    }

    if (options.extract_geometry) {
        float left = 0.0F;
        float right = 0.0F;
        float bottom = 0.0F;
        float top = 0.0F;
        if (FPDFPageObj_GetBounds(text_object, &left, &bottom, &right,
                                  &top)) {
            span.bounds =
                PdfRect{left, bottom, right - left, top - bottom};
        }
    }

    if (!split_multichar_text || !options.extract_geometry ||
        span.bounds.width_points <= 0.0 || span.text.size() <= 1U) {
        page.text_spans.push_back(std::move(span));
        return;
    }

    const auto characters = split_utf8_characters(span.text);
    if (characters.size() <= 1U) {
        page.text_spans.push_back(std::move(span));
        return;
    }

    const double character_width =
        span.bounds.width_points / static_cast<double>(characters.size());
    for (std::size_t index = 0U; index < characters.size(); ++index) {
        PdfParsedTextSpan character_span;
        character_span.text = characters[index];
        character_span.font_size_points = span.font_size_points;
        character_span.bounds =
            PdfRect{span.bounds.x_points +
                        character_width * static_cast<double>(index),
                    span.bounds.y_points, character_width,
                    span.bounds.height_points};
        page.text_spans.push_back(std::move(character_span));
    }
}

void append_text_object_spans(FPDF_PAGEOBJECT page_object,
                              FPDF_TEXTPAGE text_page,
                              PdfParsedPage &page,
                              const PdfParseOptions &options,
                              bool split_multichar_text,
                              int depth = 0) {
    if (page_object == nullptr || depth > 8) {
        return;
    }

    const int object_type = FPDFPageObj_GetType(page_object);
    if (object_type == FPDF_PAGEOBJ_TEXT) {
        append_text_object_span(page_object, text_page, page, options,
                                split_multichar_text);
        return;
    }

    if (object_type != FPDF_PAGEOBJ_FORM) {
        return;
    }

    const int child_count = FPDFFormObj_CountObjects(page_object);
    for (int child_index = 0; child_index < child_count; ++child_index) {
        append_text_object_spans(
            FPDFFormObj_GetObject(
                page_object, static_cast<unsigned long>(child_index)),
            text_page, page, options, split_multichar_text, depth + 1);
    }
}

void append_page_object_text_spans(FPDF_PAGE page,
                                   FPDF_TEXTPAGE text_page,
                                   PdfParsedPage &parsed_page,
                                   const PdfParseOptions &options,
                                   bool split_multichar_text) {
    const int object_count = FPDFPage_CountObjects(page);
    for (int object_index = 0; object_index < object_count; ++object_index) {
        append_text_object_spans(FPDFPage_GetObject(page, object_index),
                                 text_page, parsed_page, options,
                                 split_multichar_text);
    }
}

[[nodiscard]] double rect_bottom(const PdfRect &rect) noexcept {
    return rect.y_points;
}

[[nodiscard]] double rect_top(const PdfRect &rect) noexcept {
    return rect.y_points + rect.height_points;
}

[[nodiscard]] double rect_right(const PdfRect &rect) noexcept {
    return rect.x_points + rect.width_points;
}

[[nodiscard]] double span_center_y(const PdfParsedTextSpan &span) noexcept {
    return span.bounds.y_points + span.bounds.height_points / 2.0;
}

[[nodiscard]] double rect_area(const PdfRect &rect) noexcept {
    return (std::max)(0.0, rect.width_points) *
           (std::max)(0.0, rect.height_points);
}

[[nodiscard]] bool rect_contains_point(const PdfRect &rect, double x_points,
                                       double y_points) noexcept {
    return x_points >= rect.x_points && x_points <= rect_right(rect) &&
           y_points >= rect.y_points &&
           y_points <= rect.y_points + rect.height_points;
}

[[nodiscard]] double rect_intersection_area(const PdfRect &left,
                                            const PdfRect &right) noexcept {
    const double intersection_left =
        (std::max)(left.x_points, right.x_points);
    const double intersection_bottom =
        (std::max)(left.y_points, right.y_points);
    const double intersection_right =
        (std::min)(rect_right(left), rect_right(right));
    const double intersection_top = (std::min)(
        left.y_points + left.height_points, right.y_points + right.height_points);
    if (intersection_right <= intersection_left ||
        intersection_top <= intersection_bottom) {
        return 0.0;
    }

    return (intersection_right - intersection_left) *
           (intersection_top - intersection_bottom);
}

[[nodiscard]] bool rect_meaningfully_overlaps_rect(const PdfRect &subject,
                                                   const PdfRect &rect) noexcept {
    const double subject_area = rect_area(subject);
    if (subject_area <= 0.0) {
        return false;
    }

    const double center_x = subject.x_points + subject.width_points / 2.0;
    const double center_y = subject.y_points + subject.height_points / 2.0;
    const bool center_inside_rect =
        rect_contains_point(rect, center_x, center_y);
    const double overlap_ratio =
        rect_intersection_area(subject, rect) / subject_area;
    return center_inside_rect || overlap_ratio >= 0.5;
}

void expand_rect(PdfRect &target, const PdfRect &candidate) {
    if (target.width_points <= 0.0 && target.height_points <= 0.0) {
        target = candidate;
        return;
    }

    const double left = (std::min)(target.x_points, candidate.x_points);
    const double bottom = (std::min)(rect_bottom(target), rect_bottom(candidate));
    const double right = (std::max)(rect_right(target), rect_right(candidate));
    const double top = (std::max)(rect_top(target), rect_top(candidate));
    target = PdfRect{left, bottom, right - left, top - bottom};
}

void append_span_to_line(PdfParsedTextLine &line, const PdfParsedTextSpan &span) {
    if (!line.text.empty() && !span.text.empty() &&
        !std::isspace(static_cast<unsigned char>(line.text.back())) &&
        !std::isspace(static_cast<unsigned char>(span.text.front()))) {
        const double horizontal_gap = span.bounds.x_points - rect_right(line.bounds);
        const double line_height = (std::max)(line.bounds.height_points, 1.0);
        const double gap_threshold = (std::max)(3.0, line_height * 0.25);
        if (horizontal_gap > gap_threshold) {
            line.text.push_back(' ');
        }
    }
    line.text += span.text;
    expand_rect(line.bounds, span.bounds);
    line.text_spans.push_back(span);
}

void append_span_to_cluster(TextCluster &cluster,
                            const PdfParsedTextSpan &span) {
    cluster.text += span.text;
    expand_rect(cluster.bounds, span.bounds);
}

struct VisualTextRow {
    std::vector<PdfParsedTextSpan> spans;
    double center_y{0.0};
    double max_height{0.0};
};

[[nodiscard]] bool belongs_to_visual_text_row(const VisualTextRow &row,
                                              const PdfParsedTextSpan &span) {
    const double tolerance =
        (std::max)(4.0, (std::max)(row.max_height,
                                   span.bounds.height_points) *
                            0.65);
    return std::abs(span_center_y(span) - row.center_y) <= tolerance;
}

void append_span_to_visual_text_row(VisualTextRow &row,
                                    const PdfParsedTextSpan &span) {
    const auto previous_size = row.spans.size();
    row.spans.push_back(span);
    row.center_y =
        ((row.center_y * static_cast<double>(previous_size)) +
         span_center_y(span)) /
        static_cast<double>(previous_size + 1U);
    row.max_height = (std::max)(row.max_height, span.bounds.height_points);
}

[[nodiscard]] std::vector<PdfParsedTextSpan>
order_spans_by_visual_rows(std::vector<PdfParsedTextSpan> spans) {
    std::stable_sort(spans.begin(), spans.end(),
                     [](const PdfParsedTextSpan &left,
                        const PdfParsedTextSpan &right) {
                         const auto left_center =
                             std::llround(span_center_y(left) * 100.0);
                         const auto right_center =
                             std::llround(span_center_y(right) * 100.0);
                         if (left_center != right_center) {
                             return left_center > right_center;
                         }

                         const auto left_x =
                             std::llround(left.bounds.x_points * 100.0);
                         const auto right_x =
                             std::llround(right.bounds.x_points * 100.0);
                         if (left_x != right_x) {
                             return left_x < right_x;
                         }

                         return false;
                     });

    std::vector<VisualTextRow> rows;
    for (const auto &span : spans) {
        if (rows.empty() ||
            !belongs_to_visual_text_row(rows.back(), span)) {
            rows.push_back(VisualTextRow{});
        }
        append_span_to_visual_text_row(rows.back(), span);
    }

    std::vector<PdfParsedTextSpan> ordered;
    ordered.reserve(spans.size());
    for (auto &row : rows) {
        std::stable_sort(row.spans.begin(), row.spans.end(),
                         [](const PdfParsedTextSpan &left,
                            const PdfParsedTextSpan &right) {
                             const auto left_x =
                                 std::llround(left.bounds.x_points * 100.0);
                             const auto right_x =
                                 std::llround(right.bounds.x_points * 100.0);
                             if (left_x != right_x) {
                                 return left_x < right_x;
                             }

                             return span_center_y(left) > span_center_y(right);
                         });
        ordered.insert(ordered.end(), row.spans.begin(), row.spans.end());
    }

    return ordered;
}

[[nodiscard]] bool should_start_new_line(const PdfParsedTextLine &line,
                                         const PdfParsedTextSpan &span) {
    if (line.text_spans.empty()) {
        return false;
    }

    const double line_height = (std::max)(line.bounds.height_points, 1.0);
    const double span_center_y = span.bounds.y_points + span.bounds.height_points / 2.0;
    const double line_center_y = line.bounds.y_points + line.bounds.height_points / 2.0;
    if (std::abs(span_center_y - line_center_y) > line_height * 0.65) {
        return true;
    }

    const double x_backtrack = line.bounds.x_points - span.bounds.x_points;
    const double x_backtrack_threshold = (std::max)(12.0, line_height * 1.5);
    return x_backtrack > x_backtrack_threshold;
}

[[nodiscard]] std::vector<PdfParsedTextLine>
group_text_spans_into_lines(const std::vector<PdfParsedTextSpan> &spans) {
    std::vector<PdfParsedTextLine> lines;

    std::vector<PdfParsedTextSpan> ordered_spans = spans;
    const bool has_explicit_line_break = std::any_of(
        ordered_spans.begin(), ordered_spans.end(),
        [](const PdfParsedTextSpan &span) {
            return span.text == "\r" || span.text == "\n";
        });
    if (!has_explicit_line_break) {
        ordered_spans = order_spans_by_visual_rows(std::move(ordered_spans));
    }

    for (const auto &span : ordered_spans) {
        if (span.text == "\r" || span.text == "\n") {
            if (!lines.empty() && !lines.back().text.empty()) {
                lines.emplace_back();
            }
            continue;
        }
        if (span.text.empty()) {
            continue;
        }

        if (lines.empty() || should_start_new_line(lines.back(), span)) {
            lines.emplace_back();
        }
        append_span_to_line(lines.back(), span);
    }

    lines.erase(std::remove_if(lines.begin(), lines.end(),
                               [](const PdfParsedTextLine &line) {
                                   return line.text.empty();
                               }),
                lines.end());
    return lines;
}

[[nodiscard]] bool should_start_new_paragraph(
    const PdfParsedTextLine &previous, const PdfParsedTextLine &current,
    const std::optional<std::size_t> &previous_table_candidate_index,
    const std::optional<std::size_t> &current_table_candidate_index) {
    if (previous_table_candidate_index != current_table_candidate_index) {
        return true;
    }

    const double previous_height = (std::max)(previous.bounds.height_points, 1.0);
    const double vertical_gap = previous.bounds.y_points - rect_top(current.bounds);
    if (vertical_gap > previous_height * 0.85) {
        return true;
    }

    const double indent_delta = current.bounds.x_points - previous.bounds.x_points;
    return indent_delta > previous_height * 1.5;
}

void append_line_to_paragraph(PdfParsedParagraph &paragraph,
                              const PdfParsedTextLine &line) {
    if (!paragraph.text.empty()) {
        paragraph.text.push_back('\n');
    }
    paragraph.text += line.text;
    expand_rect(paragraph.bounds, line.bounds);
    paragraph.lines.push_back(line);
}

[[nodiscard]] bool should_start_new_cell_cluster(
    const TextCluster &cluster, const PdfParsedTextSpan &span,
    double line_height) {
    if (cluster.text.empty()) {
        return false;
    }

    const double horizontal_gap = span.bounds.x_points - rect_right(cluster.bounds);
    const double gap_threshold = (std::max)(18.0, line_height * 1.5);
    return horizontal_gap > gap_threshold;
}

[[nodiscard]] std::vector<TextCluster>
split_line_into_cell_clusters(const PdfParsedTextLine &line) {
    std::vector<TextCluster> clusters;
    const double line_height = (std::max)(line.bounds.height_points, 1.0);

    for (const auto &span : line.text_spans) {
        if (span.text == "\r" || span.text == "\n" || span.text.empty()) {
            continue;
        }

        if (clusters.empty() ||
            should_start_new_cell_cluster(clusters.back(), span, line_height)) {
            clusters.emplace_back();
        }
        append_span_to_cluster(clusters.back(), span);
    }

    clusters.erase(std::remove_if(clusters.begin(), clusters.end(),
                                  [](const TextCluster &cluster) {
                                      return cluster.text.empty();
                                  }),
                   clusters.end());
    return clusters;
}

[[nodiscard]] double line_center_y(const PdfParsedTextLine &line) noexcept {
    return line.bounds.y_points + line.bounds.height_points / 2.0;
}

[[nodiscard]] double line_top_y(const PdfParsedTextLine &line) noexcept {
    return rect_top(line.bounds);
}

[[nodiscard]] double line_left_x(const PdfParsedTextLine &line) noexcept {
    return line.bounds.x_points;
}

void sort_lines_by_reading_order(std::vector<PdfParsedTextLine> &lines) {
    std::stable_sort(lines.begin(), lines.end(),
                     [](const PdfParsedTextLine &left,
                        const PdfParsedTextLine &right) {
                         const auto left_top_key =
                             std::llround(line_top_y(left) * 100.0);
                         const auto right_top_key =
                             std::llround(line_top_y(right) * 100.0);
                         if (left_top_key != right_top_key) {
                             return left_top_key > right_top_key;
                         }

                         const auto left_x_key =
                             std::llround(line_left_x(left) * 100.0);
                         const auto right_x_key =
                             std::llround(line_left_x(right) * 100.0);
                         if (left_x_key != right_x_key) {
                             return left_x_key < right_x_key;
                         }

                         return false;
                     });
}

[[nodiscard]] std::vector<CandidateLine>
collect_candidate_lines(const std::vector<PdfParsedTextLine> &lines) {
    std::vector<CandidateLine> candidate_lines;
    candidate_lines.reserve(lines.size());

    for (std::size_t line_index = 0U; line_index < lines.size(); ++line_index) {
        const auto &line = lines[line_index];
        auto clusters = split_line_into_cell_clusters(line);
        if (clusters.empty()) {
            continue;
        }

        candidate_lines.push_back(
            CandidateLine{line_index, std::move(clusters), line_center_y(line)});
    }

    return candidate_lines;
}

[[nodiscard]] std::vector<double> build_column_anchors(
    const std::vector<CandidateLine> &rows) {
    std::vector<double> anchors;
    std::vector<std::size_t> anchor_counts;
    std::vector<double> cluster_positions;

    std::size_t max_cluster_count = 0U;
    for (const auto &row : rows) {
        max_cluster_count = (std::max)(max_cluster_count, row.clusters.size());
    }
    const auto full_width_row_count = static_cast<std::size_t>(std::count_if(
        rows.begin(), rows.end(), [max_cluster_count](const CandidateLine &row) {
            return row.clusters.size() == max_cluster_count;
        }));
    const bool has_full_table_width_rows =
        max_cluster_count >= 3U && full_width_row_count >= 2U;

    for (const auto &row : rows) {
        if (has_full_table_width_rows &&
            row.clusters.size() < max_cluster_count) {
            continue;
        }
        for (const auto &cluster : row.clusters) {
            cluster_positions.push_back(cluster.bounds.x_points);
        }
    }

    std::sort(cluster_positions.begin(), cluster_positions.end());
    for (const double position : cluster_positions) {
        if (anchors.empty() || position - anchors.back() > 24.0) {
            anchors.push_back(position);
            anchor_counts.push_back(1U);
            continue;
        }

        const auto last_index = anchors.size() - 1U;
        const auto count = anchor_counts[last_index];
        anchors[last_index] =
            ((anchors[last_index] * static_cast<double>(count)) + position) /
            static_cast<double>(count + 1U);
        anchor_counts[last_index] = count + 1U;
    }

    return anchors;
}

[[nodiscard]] bool has_regular_column_spacing(
    const std::vector<double> &anchors) {
    if (anchors.size() < 3U) {
        return true;
    }

    double min_gap = 0.0;
    double max_gap = 0.0;
    double total_gap = 0.0;
    bool has_gap = false;

    for (std::size_t index = 1U; index < anchors.size(); ++index) {
        const double gap = anchors[index] - anchors[index - 1U];
        if (!has_gap) {
            min_gap = gap;
            max_gap = gap;
            has_gap = true;
        } else {
            min_gap = (std::min)(min_gap, gap);
            max_gap = (std::max)(max_gap, gap);
        }
        total_gap += gap;
    }

    if (!has_gap) {
        return false;
    }

    const double average_gap =
        total_gap / static_cast<double>(anchors.size() - 1U);
    return (max_gap - min_gap) <= (std::max)(24.0, average_gap * 0.25);
}

[[nodiscard]] bool is_short_header_label(const std::string &text) {
    if (text.empty() || text.size() > 24U) {
        return false;
    }

    bool has_alpha = false;
    for (unsigned char ch : text) {
        if (std::isdigit(ch)) {
            return false;
        }
        if (std::isalpha(ch)) {
            has_alpha = true;
        }
    }

    return has_alpha;
}

[[nodiscard]] bool is_data_like_cell_text(const std::string &text) {
    if (text.empty()) {
        return false;
    }

    for (unsigned char ch : text) {
        if (std::isdigit(ch)) {
            return true;
        }
    }

    return !is_short_header_label(text);
}

[[nodiscard]] bool is_conservative_two_row_table(
    const std::vector<CandidateLine> &rows) {
    if (rows.size() != 2U) {
        return false;
    }

    const auto &header_row = rows.front().clusters;
    const auto &data_row = rows.back().clusters;
    if (header_row.size() < 3U || data_row.size() < 3U) {
        return false;
    }

    if (!std::all_of(header_row.begin(), header_row.end(),
                     [](const TextCluster &cluster) {
                         return is_short_header_label(cluster.text);
                     })) {
        return false;
    }

    const auto data_like_count = static_cast<std::size_t>(std::count_if(
        data_row.begin(), data_row.end(), [](const TextCluster &cluster) {
            return is_data_like_cell_text(cluster.text);
        }));
    return data_like_count >= 1U;
}

[[nodiscard]] bool is_conservative_two_column_key_value_table(
    const std::vector<CandidateLine> &rows) {
    if (rows.size() < 3U) {
        return false;
    }

    std::size_t data_like_value_count = 0U;
    for (const auto &row : rows) {
        if (row.clusters.size() != 2U) {
            return false;
        }

        if (!is_short_header_label(row.clusters[0].text)) {
            return false;
        }

        if (is_data_like_cell_text(row.clusters[1].text)) {
            ++data_like_value_count;
        }
    }

    const auto minimum_data_values =
        (std::max)(std::size_t{2U}, (rows.size() + 1U) / 2U);
    return data_like_value_count >= minimum_data_values;
}

[[nodiscard]] bool is_conservative_irregular_header_table(
    const std::vector<CandidateLine> &rows,
    const std::vector<double> &anchors) {
    if (rows.size() < 3U || anchors.size() < 3U) {
        return false;
    }

    const auto &header_row = rows.front().clusters;
    if (header_row.size() != anchors.size()) {
        return false;
    }
    if (!std::all_of(header_row.begin(), header_row.end(),
                     [](const TextCluster &cluster) {
                         return is_short_header_label(cluster.text);
                     })) {
        return false;
    }

    const auto minimum_data_cells = (anchors.size() + 1U) / 2U;
    for (std::size_t row_index = 1U; row_index < rows.size(); ++row_index) {
        const auto &row = rows[row_index].clusters;
        if (row.size() != anchors.size()) {
            return false;
        }

        const auto data_like_count = static_cast<std::size_t>(std::count_if(
            row.begin(), row.end(), [](const TextCluster &cluster) {
                return is_data_like_cell_text(cluster.text);
            }));
        if (data_like_count < minimum_data_cells) {
            return false;
        }
    }

    return true;
}

[[nodiscard]] std::size_t nearest_column_index(
    const std::vector<double> &anchors, double x_points) {
    std::size_t best_index = 0U;
    double best_distance = std::abs(x_points - anchors.front());

    for (std::size_t index = 1U; index < anchors.size(); ++index) {
        const double distance = std::abs(x_points - anchors[index]);
        if (distance < best_distance) {
            best_distance = distance;
            best_index = index;
        }
    }

    return best_index;
}

[[nodiscard]] double average_column_gap(
    const std::vector<double> &anchors) noexcept {
    if (anchors.size() < 2U) {
        return 0.0;
    }

    return (anchors.back() - anchors.front()) /
           static_cast<double>(anchors.size() - 1U);
}

[[nodiscard]] std::size_t detect_column_span(
    const std::vector<double> &anchors, const TextCluster &cluster,
    std::size_t column_index) {
    if (anchors.size() < 2U || column_index >= anchors.size()) {
        return 1U;
    }

    const double average_gap = average_column_gap(anchors);
    if (average_gap <= 0.0 ||
        cluster.bounds.width_points < average_gap * 1.35) {
        return 1U;
    }

    // 仅对明显宽于单列的文本簇做最小跨列推断，避免把普通长单元格过度提升成合并单元格。
    const auto estimated_span = static_cast<std::size_t>(
        std::ceil(cluster.bounds.width_points / average_gap));
    if (estimated_span <= 1U) {
        return 1U;
    }

    return (std::min)(anchors.size() - column_index, estimated_span);
}

[[nodiscard]] double cluster_center_x(const TextCluster &cluster) noexcept {
    return cluster.bounds.x_points + cluster.bounds.width_points / 2.0;
}

struct GroupHeaderSpan final {
    std::size_t column_index{0U};
    std::size_t column_span{1U};
};

[[nodiscard]] std::optional<GroupHeaderSpan>
infer_group_header_span(const std::vector<double> &anchors,
                        const CandidateLine &row,
                        std::size_t cluster_index) {
    if (anchors.size() < 3U || row.clusters.empty() ||
        row.clusters.size() >= anchors.size() ||
        cluster_index >= row.clusters.size()) {
        return std::nullopt;
    }

    const double average_gap = average_column_gap(anchors);
    if (average_gap <= 0.0) {
        return std::nullopt;
    }

    const auto &cluster = row.clusters[cluster_index];
    const double cluster_center = cluster_center_x(cluster);
    const double table_center = (anchors.front() + anchors.back()) / 2.0;
    if (row.clusters.size() == 1U) {
        const double center_tolerance = (std::max)(36.0, average_gap * 0.4);
        if (std::abs(cluster_center - table_center) > center_tolerance) {
            return std::nullopt;
        }
    }

    double left_boundary = anchors.front() - average_gap;
    if (cluster_index > 0U) {
        left_boundary =
            (cluster_center_x(row.clusters[cluster_index - 1U]) +
             cluster_center) /
            2.0;
    }

    double right_boundary = anchors.back() + average_gap;
    if (cluster_index + 1U < row.clusters.size()) {
        right_boundary =
            (cluster_center +
             cluster_center_x(row.clusters[cluster_index + 1U])) /
            2.0;
    }

    std::optional<std::size_t> first_column;
    std::size_t covered_columns = 0U;
    const double boundary_tolerance = (std::max)(4.0, average_gap * 0.05);
    for (std::size_t column_index = 0U; column_index < anchors.size();
         ++column_index) {
        const double anchor = anchors[column_index];
        if (anchor + boundary_tolerance < left_boundary ||
            anchor - boundary_tolerance > right_boundary) {
            continue;
        }

        if (!first_column.has_value()) {
            first_column = column_index;
        }
        ++covered_columns;
    }

    if (!first_column.has_value() || covered_columns <= 1U) {
        return std::nullopt;
    }

    return GroupHeaderSpan{*first_column, covered_columns};
}

[[nodiscard]] std::vector<std::optional<GroupHeaderSpan>>
detect_group_header_row_spans(const std::vector<double> &anchors,
                              const CandidateLine &row) {
    std::vector<std::optional<GroupHeaderSpan>> spans;
    spans.reserve(row.clusters.size());

    std::size_t next_column = 0U;
    for (std::size_t cluster_index = 0U; cluster_index < row.clusters.size();
         ++cluster_index) {
        auto span = infer_group_header_span(anchors, row, cluster_index);
        if (!span.has_value() || span->column_span <= 1U ||
            span->column_index != next_column ||
            span->column_index + span->column_span > anchors.size()) {
            return {};
        }

        next_column = span->column_index + span->column_span;
        spans.push_back(span);
    }

    if (next_column != anchors.size()) {
        return {};
    }

    return spans;
}

[[nodiscard]] bool is_summary_label_text(std::string_view text) {
    std::string normalized;
    normalized.reserve(text.size());
    for (unsigned char ch : text) {
        if (std::isalnum(ch)) {
            normalized.push_back(
                static_cast<char>(std::tolower(ch)));
        }
    }

    return normalized.find("total") != std::string::npos;
}

[[nodiscard]] std::optional<GroupHeaderSpan>
detect_summary_label_span(const std::vector<double> &anchors,
                          const CandidateLine &row,
                          std::size_t cluster_index,
                          std::size_t first_complete_row_index,
                          std::size_t row_index) {
    if (anchors.size() < 3U || row_index <= first_complete_row_index ||
        row.clusters.size() != 2U || cluster_index != 0U) {
        return std::nullopt;
    }

    const auto first_column =
        nearest_column_index(anchors, row.clusters.front().bounds.x_points);
    const auto last_column =
        nearest_column_index(anchors, row.clusters.back().bounds.x_points);
    if (first_column >= last_column) {
        return std::nullopt;
    }

    if (!is_summary_label_text(row.clusters.front().text) ||
        !is_short_header_label(row.clusters.front().text) ||
        !is_data_like_cell_text(row.clusters.back().text)) {
        return std::nullopt;
    }

    return GroupHeaderSpan{first_column, last_column - first_column};
}

[[nodiscard]] std::size_t count_text_cells(const PdfParsedTableRow &row) {
    return static_cast<std::size_t>(std::count_if(
        row.cells.begin(), row.cells.end(),
        [](const PdfParsedTableCell &cell) { return cell.has_text; }));
}

[[nodiscard]] bool is_column_covered_by_text_span(
    const PdfParsedTableRow &row, std::size_t column_index) {
    return std::any_of(row.cells.begin(), row.cells.end(),
                       [column_index](const PdfParsedTableCell &cell) {
                           return cell.has_text &&
                                  cell.column_index < column_index &&
                                  cell.column_index + cell.column_span >
                                      column_index;
                       });
}

[[nodiscard]] bool row_has_text_span(const PdfParsedTableRow &row) {
    return std::any_of(row.cells.begin(), row.cells.end(),
                       [](const PdfParsedTableCell &cell) {
                           return cell.has_text && cell.column_span > 1U;
                       });
}

[[nodiscard]] std::size_t detect_row_span(
    const std::vector<PdfParsedTableRow> &rows, std::size_t row_index,
    std::size_t column_index) {
    if (row_index >= rows.size()) {
        return 1U;
    }

    const auto &current_row = rows[row_index];
    if (current_row.cells.size() <= column_index) {
        return 1U;
    }

    const auto &anchor_cell = current_row.cells[column_index];
    if (!anchor_cell.has_text || count_text_cells(current_row) < 2U) {
        return 1U;
    }

    std::size_t row_span = 1U;
    for (std::size_t next_row_index = row_index + 1U;
         next_row_index < rows.size(); ++next_row_index) {
        const auto &next_row = rows[next_row_index];
        if (next_row.cells.size() <= column_index) {
            break;
        }
        if (count_text_cells(next_row) < 2U) {
            break;
        }

        const auto &continuation_cell = next_row.cells[column_index];
        if (continuation_cell.has_text) {
            break;
        }
        if (row_has_text_span(next_row)) {
            break;
        }
        if (is_column_covered_by_text_span(next_row, column_index)) {
            break;
        }

        ++row_span;
    }

    return row_span;
}

[[nodiscard]] bool build_table_candidate(
    const std::vector<CandidateLine> &rows, PdfParsedTableCandidate &candidate) {
    const bool is_two_row_table = is_conservative_two_row_table(rows);
    const bool is_two_column_key_value_table =
        is_conservative_two_column_key_value_table(rows);

    if (rows.size() < 3U && !is_two_row_table) {
        return false;
    }

    const auto anchors = build_column_anchors(rows);
    const bool has_regular_spacing = has_regular_column_spacing(anchors);
    const bool is_irregular_header_table =
        !has_regular_spacing &&
        is_conservative_irregular_header_table(rows, anchors);
    if (anchors.size() < 3U &&
        !(anchors.size() == 2U && is_two_column_key_value_table)) {
        return false;
    }
    if (!has_regular_spacing && !is_irregular_header_table) {
        return false;
    }

    candidate.column_anchor_x_points = anchors;
    candidate.rows.reserve(rows.size());

    const auto first_complete_column_row = std::find_if(
        rows.begin(), rows.end(), [&anchors](const CandidateLine &row) {
            return row.clusters.size() == anchors.size();
        });
    const std::optional<std::size_t> first_complete_column_row_index =
        first_complete_column_row == rows.end()
            ? std::nullopt
            : std::make_optional(static_cast<std::size_t>(
                  std::distance(rows.begin(), first_complete_column_row)));
    std::vector<std::vector<std::optional<GroupHeaderSpan>>> group_header_spans(
        rows.size());
    if (first_complete_column_row_index.has_value()) {
        for (std::size_t row_index = 0U;
             row_index < *first_complete_column_row_index;
             ++row_index) {
            auto row_spans =
                detect_group_header_row_spans(anchors, rows[row_index]);
            if (row_spans.empty()) {
                break;
            }
            group_header_spans[row_index] = std::move(row_spans);
        }
    }

    for (std::size_t source_row_index = 0U; source_row_index < rows.size();
         ++source_row_index) {
        const auto &source_row = rows[source_row_index];
        PdfParsedTableRow row;
        row.cells.reserve(anchors.size());
        for (std::size_t column_index = 0U; column_index < anchors.size();
             ++column_index) {
            PdfParsedTableCell cell;
            cell.column_index = column_index;
            row.cells.push_back(cell);
        }

        for (std::size_t cluster_index = 0U;
             cluster_index < source_row.clusters.size(); ++cluster_index) {
            const auto &cluster = source_row.clusters[cluster_index];
            std::optional<GroupHeaderSpan> group_header_span;
            if (source_row_index < group_header_spans.size() &&
                cluster_index < group_header_spans[source_row_index].size()) {
                group_header_span =
                    group_header_spans[source_row_index][cluster_index];
            }
            if (!group_header_span.has_value() &&
                first_complete_column_row_index.has_value()) {
                group_header_span = detect_summary_label_span(
                    anchors, source_row, cluster_index,
                    *first_complete_column_row_index, source_row_index);
            }
            const auto column_index =
                group_header_span.has_value()
                    ? group_header_span->column_index
                    : nearest_column_index(anchors, cluster.bounds.x_points);
            auto &cell = row.cells[column_index];
            cell.column_span =
                group_header_span.has_value()
                    ? group_header_span->column_span
                    : detect_column_span(anchors, cluster, column_index);
            if (!cell.has_text) {
                cell.text = cluster.text;
                cell.bounds = cluster.bounds;
                cell.has_text = true;
            } else {
                cell.text.push_back('\n');
                cell.text += cluster.text;
                expand_rect(cell.bounds, cluster.bounds);
            }

            expand_rect(row.bounds, cluster.bounds);
            expand_rect(candidate.bounds, cluster.bounds);
        }

        candidate.rows.push_back(std::move(row));
    }

    for (std::size_t row_index = 0U; row_index < candidate.rows.size();
         ++row_index) {
        auto &row = candidate.rows[row_index];
        for (std::size_t column_index = 0U; column_index < row.cells.size();
             ++column_index) {
            row.cells[column_index].row_span =
                detect_row_span(candidate.rows, row_index, column_index);
        }
    }

    return !candidate.rows.empty();
}

[[nodiscard]] std::vector<PdfParsedTableCandidate>
detect_table_candidates(std::vector<PdfParsedTextLine> &lines) {
    const auto candidate_lines = collect_candidate_lines(lines);
    std::vector<PdfParsedTableCandidate> tables;

    if (candidate_lines.size() < 3U) {
        return tables;
    }

    for (std::size_t gap_index = 0U; gap_index + 1U < candidate_lines.size();
         ++gap_index) {
        const double first_gap =
            candidate_lines[gap_index].center_y -
            candidate_lines[gap_index + 1U].center_y;
        if (first_gap <= 0.0) {
            continue;
        }

        const double gap_tolerance = (std::max)(4.0, first_gap * 0.15);
        std::size_t run_end = gap_index;
        while (run_end + 1U < candidate_lines.size() - 1U) {
            const double next_gap =
                candidate_lines[run_end + 1U].center_y -
                candidate_lines[run_end + 2U].center_y;
            if (std::abs(next_gap - first_gap) > gap_tolerance) {
                break;
            }
            ++run_end;
        }

        std::vector<CandidateLine> rows;
        rows.reserve(run_end - gap_index + 2U);
        for (std::size_t row_index = gap_index; row_index <= run_end + 1U;
             ++row_index) {
            rows.push_back(candidate_lines[row_index]);
        }

        PdfParsedTableCandidate table;
        if (build_table_candidate(rows, table)) {
            const auto table_index = tables.size();
            for (const auto &row : rows) {
                lines[row.line_index].table_candidate_index = table_index;
            }
            tables.push_back(std::move(table));
            gap_index = run_end;
        }
    }

    return tables;
}

[[nodiscard]] std::vector<PdfParsedParagraph>
group_lines_into_paragraphs(const std::vector<PdfParsedTextLine> &lines) {
    std::vector<PdfParsedParagraph> paragraphs;
    std::optional<std::size_t> previous_table_candidate_index;
    for (const auto &line : lines) {
        const auto current_table_candidate_index = line.table_candidate_index;
        if (paragraphs.empty() ||
            should_start_new_paragraph(paragraphs.back().lines.back(), line,
                                       previous_table_candidate_index,
                                       current_table_candidate_index)) {
            paragraphs.emplace_back();
        }
        paragraphs.back().table_candidate_index = current_table_candidate_index;
        append_line_to_paragraph(paragraphs.back(), line);
        previous_table_candidate_index = current_table_candidate_index;
    }
    return paragraphs;
}

[[nodiscard]] std::vector<PdfParsedContentBlock> build_content_blocks(
    const PdfParsedPage &page) {
    std::vector<PdfParsedContentBlock> blocks;
    blocks.reserve(page.paragraphs.size() + page.table_candidates.size());

    std::vector<bool> emitted_tables(page.table_candidates.size(), false);
    for (std::size_t paragraph_index = 0U;
         paragraph_index < page.paragraphs.size(); ++paragraph_index) {
        const auto &paragraph = page.paragraphs[paragraph_index];
        if (paragraph.table_candidate_index.has_value() &&
            *paragraph.table_candidate_index < page.table_candidates.size()) {
            const auto table_index = *paragraph.table_candidate_index;
            if (!emitted_tables[table_index]) {
                blocks.push_back(PdfParsedContentBlock{
                    PdfParsedContentBlockKind::table_candidate,
                    page.table_candidates[table_index].bounds, table_index});
                emitted_tables[table_index] = true;
            }
            continue;
        }

        blocks.push_back(PdfParsedContentBlock{
            PdfParsedContentBlockKind::paragraph, paragraph.bounds,
            paragraph_index});
    }

    for (std::size_t table_index = 0U; table_index < page.table_candidates.size();
         ++table_index) {
        if (emitted_tables[table_index]) {
            continue;
        }

        blocks.push_back(PdfParsedContentBlock{
            PdfParsedContentBlockKind::table_candidate,
            page.table_candidates[table_index].bounds, table_index});
    }

    return blocks;
}

void enrich_text_structure(PdfParsedPage &page) {
    page.text_lines = group_text_spans_into_lines(page.text_spans);
    sort_lines_by_reading_order(page.text_lines);
    page.table_candidates = detect_table_candidates(page.text_lines);
    page.paragraphs = group_lines_into_paragraphs(page.text_lines);
    page.content_blocks = build_content_blocks(page);
}

}  // namespace

PdfParseResult PdfiumParser::parse(const std::filesystem::path &input_path,
                                   const PdfParseOptions &options) {
    PdfParseResult result;

    if (!std::filesystem::exists(input_path)) {
        result.error_message = "PDF input does not exist: " + input_path.string();
        return result;
    }

    ensure_pdfium_initialized();

    const auto input_string = input_path.string();
    PdfiumDocumentPtr document(FPDF_LoadDocument(input_string.c_str(), nullptr));
    if (!document) {
        result.error_message =
            "Unable to load PDF: " + pdfium_error_to_string(FPDF_GetLastError());
        return result;
    }

    const int page_count = FPDF_GetPageCount(document.get());
    result.document.pages.reserve(static_cast<std::size_t>(page_count));
    const auto actual_text_pages =
        options.extract_text ? extract_page_actual_text_spans(input_path)
                             : std::vector<std::vector<std::string>>{};

    for (int page_index = 0; page_index < page_count; ++page_index) {
        PdfiumPagePtr page(FPDF_LoadPage(document.get(), page_index));
        if (!page) {
            result.error_message =
                "Unable to load PDF page " + std::to_string(page_index);
            return result;
        }

        PdfParsedPage parsed_page;
        parsed_page.size =
            PdfPageSize{FPDF_GetPageWidth(page.get()), FPDF_GetPageHeight(page.get())};

        if (options.extract_text) {
            PdfiumTextPagePtr text_page(FPDFText_LoadPage(page.get()));
            if (!text_page) {
                result.error_message =
                    "Unable to load PDF text page " + std::to_string(page_index);
                return result;
            }

            const int char_count = FPDFText_CountChars(text_page.get());
            parsed_page.text_spans.reserve(static_cast<std::size_t>(char_count));
            for (int char_index = 0; char_index < char_count; ++char_index) {
                if (FPDFText_GetUnicode(text_page.get(), char_index) == 0) {
                    continue;
                }
                parsed_page.text_spans.push_back(
                    parse_text_span(text_page.get(), char_index, options));
            }
            if (parsed_page.text_spans.empty() && char_count > 0) {
                std::vector<unsigned short> page_text(
                    static_cast<std::size_t>(char_count) + 1U, 0U);
                const int copied = FPDFText_GetText(
                    text_page.get(), 0, char_count, page_text.data());
                if (copied > 1) {
                    PdfParsedTextSpan span;
                    for (int text_index = 0; text_index < copied - 1;
                         ++text_index) {
                        append_utf8(
                            span.text,
                            static_cast<std::uint32_t>(page_text[
                                static_cast<std::size_t>(text_index)]));
                    }
                    if (!span.text.empty()) {
                        parsed_page.text_spans.push_back(std::move(span));
                    }
                }
            }
            PdfParsedPage object_text_page;
            const auto *actual_text_spans =
                static_cast<std::size_t>(page_index) < actual_text_pages.size()
                    ? &actual_text_pages[static_cast<std::size_t>(page_index)]
                    : nullptr;
            append_page_object_text_spans(
                page.get(), text_page.get(), object_text_page, options,
                actual_text_spans == nullptr || actual_text_spans->empty());
            if (actual_text_spans != nullptr &&
                should_prefer_actual_text_spans(object_text_page.text_spans,
                                                *actual_text_spans)) {
                object_text_page.text_spans = build_actual_text_spans(
                    object_text_page.text_spans, *actual_text_spans);
            }
            bool use_object_spans = should_prefer_text_object_spans(
                parsed_page.text_spans, object_text_page.text_spans);
            if (!use_object_spans && actual_text_spans != nullptr &&
                missing_actual_text_span(parsed_page.text_spans,
                                         *actual_text_spans) &&
                !missing_actual_text_span(object_text_page.text_spans,
                                          *actual_text_spans)) {
                use_object_spans = true;
            }
            if (use_object_spans) {
                parsed_page.text_spans =
                    std::move(object_text_page.text_spans);
            }
            if (options.extract_geometry) {
                enrich_text_structure(parsed_page);
            }
        }

        result.document.pages.push_back(std::move(parsed_page));
    }

    result.success = true;
    return result;
}

}  // namespace featherdoc::pdf
