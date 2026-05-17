#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include <featherdoc.hpp>
#include <featherdoc/pdf/pdf_document_adapter.hpp>
#include <featherdoc/pdf/pdf_document_importer.hpp>
#include <featherdoc/pdf/pdf_parser.hpp>
#include <featherdoc/pdf/pdf_text_shaper.hpp>
#include <featherdoc/pdf/pdf_writer.hpp>

#include <pdfio.h>
#include <pdfio-content.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <zlib.h>

namespace featherdoc::pdf::detail {
bool normalize_rtl_table_cell_text_for_testing(std::string &text);
}

namespace {

auto utf8_from_u8(std::u8string_view text) -> std::string {
    return {reinterpret_cast<const char *>(text.data()), text.size()};
}

[[nodiscard]] std::string hebrew_shalom_logical() {
    return utf8_from_u8(u8"\u05E9\u05DC\u05D5\u05DD");
}

[[nodiscard]] std::string hebrew_shalom_visual() {
    return utf8_from_u8(u8"\u05DD\u05D5\u05DC\u05E9");
}

[[nodiscard]] std::string hebrew_bedika_logical() {
    return utf8_from_u8(u8"\u05D1\u05D3\u05D9\u05E7\u05D4");
}

[[nodiscard]] std::string hebrew_bedika_visual() {
    return utf8_from_u8(u8"\u05D4\u05E7\u05D9\u05D3\u05D1");
}

[[nodiscard]] std::string arabic_salam_logical() {
    return utf8_from_u8(u8"\u0633\u0644\u0627\u0645");
}

[[nodiscard]] std::string arabic_salam_visual() {
    return utf8_from_u8(u8"\u0645\u0627\u0644\u0633");
}

[[nodiscard]] std::string utf8_from_codepoint(std::uint32_t codepoint) {
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
        text.push_back(
            static_cast<char>(0x80U | ((codepoint >> 12U) & 0x3FU)));
        text.push_back(
            static_cast<char>(0x80U | ((codepoint >> 6U) & 0x3FU)));
        text.push_back(static_cast<char>(0x80U | (codepoint & 0x3FU)));
    }
    return text;
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

[[nodiscard]] std::filesystem::path find_rtl_font() {
    for (const auto &candidate : candidate_rtl_fonts()) {
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

[[nodiscard]] std::string collect_paragraph_text(
    const featherdoc::pdf::PdfParsedDocument &document) {
    std::string text;
    for (const auto &page : document.pages) {
        for (const auto &paragraph : page.paragraphs) {
            text += paragraph.text;
            text.push_back('\n');
        }
    }
    return text;
}

[[nodiscard]] std::string collect_line_text(
    const featherdoc::pdf::PdfParsedDocument &document) {
    std::string text;
    for (const auto &page : document.pages) {
        for (const auto &line : page.text_lines) {
            text += line.text;
            text.push_back('\n');
        }
    }
    return text;
}

[[nodiscard]] std::string collect_document_text(featherdoc::Document &document) {
    std::string text;
    for (auto paragraph = document.paragraphs(); paragraph.has_next();
         paragraph.next()) {
        for (auto run = paragraph.runs(); run.has_next(); run.next()) {
            text += run.get_text();
        }
        text.push_back('\n');
    }
    return text;
}

[[nodiscard]] featherdoc::pdf::PdfTextRun make_unicode_text_run(
    double x_points,
    double y_points,
    std::string text,
    const std::filesystem::path &font_path) {
    return featherdoc::pdf::PdfTextRun{
        featherdoc::pdf::PdfPoint{x_points, y_points},
        std::move(text),
        "Hebrew RTL Test Font",
        font_path,
        16.0,
        featherdoc::pdf::PdfRgbColor{0.0, 0.0, 0.0},
        false,
        false,
        false,
        true,
    };
}

void append_test_grid(featherdoc::pdf::PdfPageLayout &page,
                      double left,
                      double top,
                      double cell_width,
                      double cell_height,
                      std::size_t column_count,
                      std::size_t row_count) {
    const double right = left + cell_width * static_cast<double>(column_count);
    const double bottom = top - cell_height * static_cast<double>(row_count);

    for (std::size_t column = 0U; column <= column_count; ++column) {
        const double x = left + cell_width * static_cast<double>(column);
        page.lines.push_back(featherdoc::pdf::PdfLine{
            featherdoc::pdf::PdfPoint{x, bottom},
            featherdoc::pdf::PdfPoint{x, top},
            featherdoc::pdf::PdfRgbColor{0.12, 0.16, 0.22},
            0.75,
        });
    }
    for (std::size_t row = 0U; row <= row_count; ++row) {
        const double y = top - cell_height * static_cast<double>(row);
        page.lines.push_back(featherdoc::pdf::PdfLine{
            featherdoc::pdf::PdfPoint{left, y},
            featherdoc::pdf::PdfPoint{right, y},
            featherdoc::pdf::PdfRgbColor{0.12, 0.16, 0.22},
            0.75,
        });
    }
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

struct TestGlyphCidEntry {
    std::uint16_t cid{0U};
    std::uint16_t glyph_id{0U};
    double width_1000{0.0};
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

[[nodiscard]] bool span_has_valid_geometry(
    const featherdoc::pdf::PdfParsedTextSpan &span) {
    INFO("raw span text: " << span.text);
    CHECK_GT(span.font_size_points, 0.0);
    CHECK(std::isfinite(span.bounds.x_points));
    CHECK(std::isfinite(span.bounds.y_points));
    CHECK(std::isfinite(span.bounds.width_points));
    CHECK(std::isfinite(span.bounds.height_points));
    CHECK_GE(span.bounds.width_points, 0.0);
    CHECK_GE(span.bounds.height_points, 0.0);
    return span.font_size_points > 0.0 &&
           std::isfinite(span.bounds.x_points) &&
           std::isfinite(span.bounds.y_points) &&
           std::isfinite(span.bounds.width_points) &&
           std::isfinite(span.bounds.height_points) &&
           span.bounds.width_points >= 0.0 &&
           span.bounds.height_points >= 0.0;
}

[[nodiscard]] std::optional<std::pair<std::size_t, std::size_t>>
find_span_sequence_from(
    const std::vector<featherdoc::pdf::PdfParsedTextSpan> &spans,
    std::string_view expected_sequence,
    std::size_t first_start) {
    for (std::size_t start = first_start; start < spans.size(); ++start) {
        std::string candidate;
        for (std::size_t end = start; end < spans.size(); ++end) {
            candidate += spans[end].text;
            if (candidate.size() > expected_sequence.size() ||
                expected_sequence.compare(0U, candidate.size(), candidate) !=
                    0) {
                break;
            }

            if (candidate == expected_sequence) {
                return std::make_pair(start, end);
            }
        }
    }

    return std::nullopt;
}

[[nodiscard]] std::optional<std::pair<std::size_t, std::size_t>>
find_span_sequence(const std::vector<featherdoc::pdf::PdfParsedTextSpan> &spans,
                   std::string_view expected_sequence) {
    return find_span_sequence_from(spans, expected_sequence, 0U);
}

[[nodiscard]] bool has_span_sequence_with_geometry(
    const std::vector<featherdoc::pdf::PdfParsedTextSpan> &spans,
    std::string_view expected_sequence) {
    CAPTURE(std::string{expected_sequence});
    REQUIRE_FALSE(expected_sequence.empty());

    const auto sequence = find_span_sequence(spans, expected_sequence);
    if (!sequence.has_value()) {
        return false;
    }

    for (std::size_t index = sequence->first; index <= sequence->second;
         ++index) {
        (void)span_has_valid_geometry(spans[index]);
    }

    return true;
}

void check_span_sequences_appear_in_order(
    const std::vector<featherdoc::pdf::PdfParsedTextSpan> &spans,
    const std::vector<std::string> &expected_sequences) {
    std::size_t search_begin = 0U;
    for (const auto &expected_sequence : expected_sequences) {
        INFO("expected ordered raw span sequence: " << expected_sequence);
        const auto sequence =
            find_span_sequence_from(spans, expected_sequence, search_begin);
        REQUIRE_MESSAGE(sequence.has_value(),
                        "Expected ordered raw span sequence was not found.");
        for (std::size_t index = sequence->first; index <= sequence->second;
             ++index) {
            (void)span_has_valid_geometry(spans[index]);
        }
        search_begin = sequence->second + 1U;
    }
}

void check_raw_text_spans_unchanged(
    const std::vector<featherdoc::pdf::PdfParsedTextSpan> &normalized_spans,
    const std::vector<featherdoc::pdf::PdfParsedTextSpan> &raw_spans) {
    REQUIRE_EQ(normalized_spans.size(), raw_spans.size());

    for (std::size_t index = 0U; index < raw_spans.size(); ++index) {
        INFO("raw span index: " << index);
        const auto &normalized_span = normalized_spans[index];
        const auto &raw_span = raw_spans[index];
        CHECK_EQ(normalized_span.text, raw_span.text);
        CHECK_EQ(normalized_span.font_name, raw_span.font_name);
        CHECK_EQ(normalized_span.font_size_points,
                 doctest::Approx(raw_span.font_size_points));
        CHECK_EQ(normalized_span.bounds.x_points,
                 doctest::Approx(raw_span.bounds.x_points));
        CHECK_EQ(normalized_span.bounds.y_points,
                 doctest::Approx(raw_span.bounds.y_points));
        CHECK_EQ(normalized_span.bounds.width_points,
                 doctest::Approx(raw_span.bounds.width_points));
        CHECK_EQ(normalized_span.bounds.height_points,
                 doctest::Approx(raw_span.bounds.height_points));
    }
}

void check_rect_unchanged(const featherdoc::pdf::PdfRect &normalized_rect,
                          const featherdoc::pdf::PdfRect &raw_rect) {
    CHECK_EQ(normalized_rect.x_points,
             doctest::Approx(raw_rect.x_points));
    CHECK_EQ(normalized_rect.y_points,
             doctest::Approx(raw_rect.y_points));
    CHECK_EQ(normalized_rect.width_points,
             doctest::Approx(raw_rect.width_points));
    CHECK_EQ(normalized_rect.height_points,
             doctest::Approx(raw_rect.height_points));
}

void check_table_candidate_geometry_unchanged(
    const featherdoc::pdf::PdfParsedTableCandidate &normalized_table,
    const featherdoc::pdf::PdfParsedTableCandidate &raw_table) {
    check_rect_unchanged(normalized_table.bounds, raw_table.bounds);

    REQUIRE_EQ(normalized_table.column_anchor_x_points.size(),
               raw_table.column_anchor_x_points.size());
    for (std::size_t column_index = 0U;
         column_index < raw_table.column_anchor_x_points.size();
         ++column_index) {
        INFO("table column anchor index: " << column_index);
        CHECK_EQ(normalized_table.column_anchor_x_points[column_index],
                 doctest::Approx(
                     raw_table.column_anchor_x_points[column_index]));
    }

    REQUIRE_EQ(normalized_table.rows.size(), raw_table.rows.size());
    for (std::size_t row_index = 0U; row_index < raw_table.rows.size();
         ++row_index) {
        INFO("table row index: " << row_index);
        const auto &normalized_row = normalized_table.rows[row_index];
        const auto &raw_row = raw_table.rows[row_index];
        check_rect_unchanged(normalized_row.bounds, raw_row.bounds);

        REQUIRE_EQ(normalized_row.cells.size(), raw_row.cells.size());
        for (std::size_t column_index = 0U;
             column_index < raw_row.cells.size();
             ++column_index) {
            INFO("table cell column index: " << column_index);
            const auto &normalized_cell = normalized_row.cells[column_index];
            const auto &raw_cell = raw_row.cells[column_index];
            CHECK_EQ(normalized_cell.column_index, raw_cell.column_index);
            CHECK_EQ(normalized_cell.column_span, raw_cell.column_span);
            CHECK_EQ(normalized_cell.row_span, raw_cell.row_span);
            CHECK_EQ(normalized_cell.has_text, raw_cell.has_text);
            check_rect_unchanged(normalized_cell.bounds, raw_cell.bounds);
            check_raw_text_spans_unchanged(normalized_cell.text_spans,
                                           raw_cell.text_spans);
        }
    }
}

void check_table_cell_text_unchanged_except_column_rows(
    const featherdoc::pdf::PdfParsedTableCandidate &normalized_table,
    const featherdoc::pdf::PdfParsedTableCandidate &raw_table,
    std::size_t exempt_column_index,
    std::size_t first_exempt_row,
    std::size_t exempt_row_count) {
    REQUIRE_EQ(normalized_table.rows.size(), raw_table.rows.size());
    const auto last_exempt_row = first_exempt_row + exempt_row_count;

    for (std::size_t row_index = 0U; row_index < raw_table.rows.size();
         ++row_index) {
        INFO("table text row index: " << row_index);
        const auto &normalized_row = normalized_table.rows[row_index];
        const auto &raw_row = raw_table.rows[row_index];
        REQUIRE_EQ(normalized_row.cells.size(), raw_row.cells.size());

        for (std::size_t column_index = 0U;
             column_index < raw_row.cells.size();
             ++column_index) {
            INFO("table text column index: " << column_index);
            const bool is_exempt_cell =
                column_index == exempt_column_index &&
                row_index >= first_exempt_row && row_index < last_exempt_row;
            if (is_exempt_cell) {
                continue;
            }

            CHECK_EQ(normalized_row.cells[column_index].text,
                     raw_row.cells[column_index].text);
        }
    }
}

[[nodiscard]] double span_center_x(
    const featherdoc::pdf::PdfParsedTextSpan &span) noexcept {
    return span.bounds.x_points + span.bounds.width_points / 2.0;
}

[[nodiscard]] double span_center_y(
    const featherdoc::pdf::PdfParsedTextSpan &span) noexcept {
    return span.bounds.y_points + span.bounds.height_points / 2.0;
}

[[nodiscard]] bool spans_have_nearby_centers(
    const featherdoc::pdf::PdfParsedTextSpan &left,
    const featherdoc::pdf::PdfParsedTextSpan &right) noexcept {
    const double line_height =
        (std::max)({left.bounds.height_points,
                    right.bounds.height_points,
                    left.font_size_points,
                    right.font_size_points,
                    1.0});
    const double max_center_y_delta = line_height;
    const double max_center_x_delta = line_height * 2.0;
    const bool close_y =
        std::abs(span_center_y(left) - span_center_y(right)) <=
        max_center_y_delta;
    const bool close_x =
        std::abs(span_center_x(left) - span_center_x(right)) <=
        max_center_x_delta;
    return close_x && close_y;
}

[[nodiscard]] bool combining_mark_span_has_adjacent_base_geometry(
    const std::vector<featherdoc::pdf::PdfParsedTextSpan> &spans,
    std::size_t mark_index,
    std::size_t sequence_begin,
    std::size_t sequence_end) {
    const auto &mark = spans[mark_index];
    (void)span_has_valid_geometry(mark);

    for (const auto neighbor_index :
         {mark_index > sequence_begin ? std::make_optional(mark_index - 1U)
                                      : std::optional<std::size_t>{},
          mark_index < sequence_end ? std::make_optional(mark_index + 1U)
                                    : std::optional<std::size_t>{}}) {
        if (!neighbor_index.has_value()) {
            continue;
        }
        const auto &neighbor = spans[*neighbor_index];
        if (neighbor.text.empty()) {
            continue;
        }

        if (spans_have_nearby_centers(mark, neighbor)) {
            return true;
        }
    }

    return false;
}

struct ExpectedCombiningMarkNeighbors {
    std::string left_text;
    std::string mark_text;
    std::string right_text;
};

void check_combining_mark_neighbor_geometry(
    const std::vector<featherdoc::pdf::PdfParsedTextSpan> &spans,
    std::string_view expected_sequence,
    const std::vector<ExpectedCombiningMarkNeighbors> &expected_neighbors) {
    CAPTURE(std::string{expected_sequence});
    const auto sequence = find_span_sequence(spans, expected_sequence);
    REQUIRE_MESSAGE(sequence.has_value(),
                    "Expected raw span sequence was not found.");

    for (const auto &expected : expected_neighbors) {
        INFO("expected mark neighbor triple: " << expected.left_text
                                             << " | " << expected.mark_text
                                             << " | " << expected.right_text);
        std::size_t match_count = 0U;
        for (std::size_t index = sequence->first + 1U;
             index < sequence->second; ++index) {
            const auto &left = spans[index - 1U];
            const auto &mark = spans[index];
            const auto &right = spans[index + 1U];
            if (left.text != expected.left_text ||
                mark.text != expected.mark_text ||
                right.text != expected.right_text) {
                continue;
            }

            ++match_count;
            (void)span_has_valid_geometry(left);
            (void)span_has_valid_geometry(mark);
            (void)span_has_valid_geometry(right);
            const bool mark_is_near_candidate_base =
                spans_have_nearby_centers(mark, left) ||
                spans_have_nearby_centers(mark, right);
            CHECK(mark_is_near_candidate_base);
        }
        CHECK_EQ(match_count, 1U);
    }
}

void check_combining_marks_have_adjacent_base_geometry(
    const std::vector<featherdoc::pdf::PdfParsedTextSpan> &spans,
    std::string_view expected_sequence,
    std::string_view combining_mark) {
    CAPTURE(std::string{expected_sequence});
    CAPTURE(std::string{combining_mark});
    const auto sequence = find_span_sequence(spans, expected_sequence);
    REQUIRE_MESSAGE(sequence.has_value(),
                    "Expected raw span sequence was not found.");

    std::size_t mark_count = 0U;
    for (std::size_t index = sequence->first; index <= sequence->second;
         ++index) {
        if (spans[index].text != combining_mark) {
            continue;
        }
        ++mark_count;
        CHECK(combining_mark_span_has_adjacent_base_geometry(
            spans, index, sequence->first, sequence->second));
    }
    CHECK_GT(mark_count, 0U);
}

void check_raw_span_sequence_has_geometry(
    const featherdoc::pdf::PdfParsedDocument &document,
    std::string_view expected_sequence) {
    for (const auto &page : document.pages) {
        if (has_span_sequence_with_geometry(page.text_spans,
                                            expected_sequence)) {
            return;
        }
    }

    FAIL("Expected raw span sequence was not found.");
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

[[nodiscard]] std::string
glyph_cluster_text_for_index(const featherdoc::pdf::PdfGlyphRun &glyph_run,
                             std::size_t glyph_index) {
    if (glyph_index >= glyph_run.glyphs.size()) {
        return {};
    }

    const auto cluster = glyph_run.glyphs[glyph_index].cluster;
    if (cluster >= glyph_run.text.size()) {
        return {};
    }

    for (std::size_t previous = 0U; previous < glyph_index; ++previous) {
        if (glyph_run.glyphs[previous].cluster == cluster) {
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

[[nodiscard]] std::vector<TestGlyphCidEntry>
glyph_cid_entries_from_run(const featherdoc::pdf::PdfGlyphRun &glyph_run) {
    std::vector<TestGlyphCidEntry> entries;
    entries.reserve(glyph_run.glyphs.size());
    for (std::size_t index = 0U; index < glyph_run.glyphs.size(); ++index) {
        const auto &glyph = glyph_run.glyphs[index];
        const double width_1000 =
            std::isfinite(glyph.x_advance_points) &&
                    glyph_run.font_size_points > 0.0
                ? std::max(0.0, glyph.x_advance_points * 1000.0 /
                                     glyph_run.font_size_points)
                : 0.0;
        entries.push_back(TestGlyphCidEntry{
            static_cast<std::uint16_t>(index + 1U),
            static_cast<std::uint16_t>(glyph.glyph_id),
            width_1000,
            glyph_cluster_text_for_index(glyph_run, index),
        });
    }
    return entries;
}

[[nodiscard]] std::vector<TestGlyphCidEntry>
logical_unicode_entries_for_visual_cids(
    const featherdoc::pdf::PdfGlyphRun &glyph_run) {
    struct ClusterText {
        std::uint32_t cluster{0U};
        std::string text;
    };

    std::vector<ClusterText> logical_clusters;
    logical_clusters.reserve(glyph_run.glyphs.size());
    for (std::size_t index = 0U; index < glyph_run.glyphs.size(); ++index) {
        auto text = glyph_cluster_text_for_index(glyph_run, index);
        if (!text.empty()) {
            logical_clusters.push_back(
                ClusterText{glyph_run.glyphs[index].cluster, std::move(text)});
        }
    }
    std::sort(logical_clusters.begin(), logical_clusters.end(),
              [](const ClusterText &left, const ClusterText &right) {
                  return left.cluster < right.cluster;
              });

    auto entries = glyph_cid_entries_from_run(glyph_run);
    for (auto &entry : entries) {
        entry.unicode_text.clear();
    }

    const auto mapped_count = std::min(entries.size(), logical_clusters.size());
    for (std::size_t index = 0U; index < mapped_count; ++index) {
        entries[index].unicode_text = logical_clusters[index].text;
    }
    return entries;
}

[[nodiscard]] std::string cid_tj_command(std::size_t glyph_count) {
    std::string command;
    command.reserve(2U + glyph_count * 4U + 5U);
    command.push_back('<');
    for (std::size_t index = 0U; index < glyph_count; ++index) {
        append_utf16be_hex_unit(command, static_cast<std::uint16_t>(index + 1U));
    }
    command += "> Tj\n";
    return command;
}

[[nodiscard]] std::string single_cid_tj_command(std::uint16_t cid) {
    std::string command;
    command.reserve(11U);
    command.push_back('<');
    append_utf16be_hex_unit(command, cid);
    command += "> Tj\n";
    return command;
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
            stream.find("Tj") != std::string::npos) {
            return stream;
        }
    }
    return {};
}

void check_hebrew_fallback_extraction_boundary(
    const std::filesystem::path &output_path,
    std::string_view actual_text,
    std::string_view shown_text,
    std::string_view logical_text,
    std::string_view visual_order_text,
    std::size_t expected_logical_occurrences,
    std::size_t expected_visual_order_occurrences) {
    const auto pdf_bytes = read_file_bytes(output_path);
    CHECK_EQ(pdf_bytes.find("/FeatherDocGlyph"), std::string::npos);
    CHECK_NE(pdf_bytes.find("/ToUnicode"), std::string::npos);
    CHECK_NE(pdf_bytes.find("/Identity-H"), std::string::npos);

    const auto streams = inflated_pdf_streams(pdf_bytes);
    const auto content_stream = find_actual_text_content_stream(streams);
    REQUIRE_FALSE(content_stream.empty());

    const auto expected_actual_text =
        std::string{"/ActualText <FEFF"} +
        utf16be_hex_payload_from_utf8(actual_text) + ">";
    const auto expected_tj =
        std::string{"<"} + utf16be_hex_payload_from_utf8(shown_text) + ">Tj";
    CHECK_NE(content_stream.find(expected_actual_text), std::string::npos);
    CHECK_NE(content_stream.find(expected_tj), std::string::npos);

    featherdoc::pdf::PdfiumParser parser;
    const auto parse_result = parser.parse(output_path, {});
    REQUIRE_MESSAGE(parse_result.success, parse_result.error_message);

    const auto extracted_text = collect_text(parse_result.document);
    CHECK_EQ(count_occurrences(extracted_text, logical_text),
             expected_logical_occurrences);
    CHECK_EQ(count_occurrences(extracted_text, visual_order_text),
             expected_visual_order_occurrences);
}

struct PdfioFileCloser {
    void operator()(pdfio_file_t *file) const {
        if (file != nullptr) {
            (void)pdfioFileClose(file);
        }
    }
};

struct PdfioStreamCloser {
    void operator()(pdfio_stream_t *stream) const {
        if (stream != nullptr) {
            (void)pdfioStreamClose(stream);
        }
    }
};

bool capture_pdfio_error(pdfio_file_t *, const char *message, void *data) {
    if (data != nullptr && message != nullptr) {
        auto *error_message = static_cast<std::string *>(data);
        *error_message = message;
    }
    return false;
}

struct TestFreeTypeFaceOwner {
    FT_Library library{nullptr};
    FT_Face face{nullptr};

    explicit TestFreeTypeFaceOwner(const std::filesystem::path &font_path) {
        if (FT_Init_FreeType(&library) != 0) {
            library = nullptr;
            return;
        }

        const auto path = font_path.string();
        if (FT_New_Face(library, path.c_str(), 0, &face) != 0) {
            face = nullptr;
        }
    }

    ~TestFreeTypeFaceOwner() {
        if (face != nullptr) {
            FT_Done_Face(face);
        }
        if (library != nullptr) {
            FT_Done_FreeType(library);
        }
    }

    [[nodiscard]] bool valid() const noexcept {
        return library != nullptr && face != nullptr;
    }
};

[[nodiscard]] double test_font_unit_to_1000(FT_Face face,
                                            FT_Pos value) noexcept {
    if (face == nullptr || face->units_per_EM <= 0) {
        return 0.0;
    }
    return static_cast<double>(value) * 1000.0 /
           static_cast<double>(face->units_per_EM);
}

[[nodiscard]] std::string sanitized_test_pdf_font_name(std::string_view value) {
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

[[nodiscard]] std::string test_base_font_name_for(FT_Face face) {
    if (face != nullptr) {
        if (const char *postscript_name = FT_Get_Postscript_Name(face);
            postscript_name != nullptr && postscript_name[0] != '\0') {
            return sanitized_test_pdf_font_name(postscript_name);
        }
        if (face->family_name != nullptr && face->family_name[0] != '\0') {
            return sanitized_test_pdf_font_name(face->family_name);
        }
    }
    return "FeatherDocGlyph-TestFont";
}

[[nodiscard]] pdfio_obj_t *create_test_stream_object_from_bytes(
    pdfio_file_t *pdf,
    pdfio_dict_t *dict,
    const unsigned char *data,
    std::size_t size,
    std::string_view label,
    std::string &error_message) {
    if (dict == nullptr) {
        error_message =
            "Unable to create PDF stream dictionary: " + std::string{label};
        return nullptr;
    }

    auto *object = pdfioFileCreateObj(pdf, dict);
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
create_test_stream_object_from_text(pdfio_file_t *pdf,
                                    pdfio_dict_t *dict,
                                    const std::string &text,
                                    std::string_view label,
                                    std::string &error_message) {
    return create_test_stream_object_from_bytes(
        pdf,
        dict,
        reinterpret_cast<const unsigned char *>(text.data()),
        text.size(),
        label,
        error_message);
}

[[nodiscard]] pdfio_obj_t *
create_test_font_file_object(pdfio_file_t *pdf,
                             const std::filesystem::path &font_path,
                             std::string_view label,
                             std::string &error_message) {
    const auto font_bytes = read_file_bytes(font_path);
    if (font_bytes.empty()) {
        error_message = "Unable to read PDF glyph font file";
        return nullptr;
    }

    auto *dict = pdfioDictCreate(pdf);
    if (dict != nullptr) {
        pdfioDictSetName(dict, "Filter", "FlateDecode");
    }
    return create_test_stream_object_from_bytes(
        pdf,
        dict,
        reinterpret_cast<const unsigned char *>(font_bytes.data()),
        font_bytes.size(),
        label,
        error_message);
}

[[nodiscard]] pdfio_obj_t *create_test_cid_to_gid_map_object(
    pdfio_file_t *pdf,
    const std::vector<TestGlyphCidEntry> &entries,
    std::string_view label,
    std::string &error_message) {
    const std::size_t byte_count = (entries.size() + 1U) * 2U;
    std::vector<unsigned char> cid_to_gid(byte_count, 0U);
    for (const auto &entry : entries) {
        const auto offset = static_cast<std::size_t>(entry.cid) * 2U;
        cid_to_gid[offset] = static_cast<unsigned char>(entry.glyph_id >> 8U);
        cid_to_gid[offset + 1U] =
            static_cast<unsigned char>(entry.glyph_id & 0xFFU);
    }

    auto *dict = pdfioDictCreate(pdf);
    if (dict != nullptr) {
        pdfioDictSetName(dict, "Filter", "FlateDecode");
    }
    return create_test_stream_object_from_bytes(
        pdf, dict, cid_to_gid.data(), cid_to_gid.size(), label, error_message);
}

[[nodiscard]] pdfio_obj_t *create_test_to_unicode_object(
    pdfio_file_t *pdf,
    const std::vector<TestGlyphCidEntry> &entries,
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

    std::vector<const TestGlyphCidEntry *> unicode_entries;
    unicode_entries.reserve(entries.size());
    for (const auto &entry : entries) {
        if (!entry.unicode_text.empty()) {
            unicode_entries.push_back(&entry);
        }
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

    auto *dict = pdfioDictCreate(pdf);
    if (dict != nullptr) {
        pdfioDictSetName(dict, "Type", "CMap");
        pdfioDictSetName(dict, "CMapName", "FeatherDocGlyphToUnicode");
        pdfioDictSetName(dict, "Filter", "FlateDecode");
    }
    return create_test_stream_object_from_text(pdf, dict, cmap, label,
                                               error_message);
}

[[nodiscard]] pdfio_array_t *create_test_width_array(
    pdfio_file_t *pdf,
    const std::vector<TestGlyphCidEntry> &entries) {
    auto *widths = pdfioArrayCreate(pdf);
    if (widths == nullptr || entries.empty()) {
        return widths;
    }

    pdfioArrayAppendNumber(widths, entries.front().cid);
    auto *chunk = pdfioArrayCreate(pdf);
    if (chunk == nullptr) {
        return nullptr;
    }
    for (const auto &entry : entries) {
        pdfioArrayAppendNumber(chunk, entry.width_1000);
    }
    pdfioArrayAppendArray(widths, chunk);
    return widths;
}

[[nodiscard]] pdfio_obj_t *create_test_glyph_cid_font(
    pdfio_file_t *pdf,
    const std::filesystem::path &font_path,
    const std::vector<TestGlyphCidEntry> &entries,
    std::string &error_message) {
    if (entries.empty()) {
        error_message = "RTL glyph CID fixture has no glyph entries";
        return nullptr;
    }

    TestFreeTypeFaceOwner face_owner(font_path);
    if (!face_owner.valid()) {
        error_message = "Unable to load RTL glyph CID fixture font";
        return nullptr;
    }

    const auto base_font = test_base_font_name_for(face_owner.face);
    auto *font_file =
        create_test_font_file_object(pdf, font_path, base_font, error_message);
    if (font_file == nullptr) {
        return nullptr;
    }

    auto *bbox = pdfioArrayCreate(pdf);
    if (bbox == nullptr) {
        error_message = "Unable to create RTL glyph CID font bbox";
        return nullptr;
    }
    pdfioArrayAppendNumber(bbox, test_font_unit_to_1000(
                                     face_owner.face, face_owner.face->bbox.xMin));
    pdfioArrayAppendNumber(bbox, test_font_unit_to_1000(
                                     face_owner.face, face_owner.face->bbox.yMin));
    pdfioArrayAppendNumber(bbox, test_font_unit_to_1000(
                                     face_owner.face, face_owner.face->bbox.xMax));
    pdfioArrayAppendNumber(bbox, test_font_unit_to_1000(
                                     face_owner.face, face_owner.face->bbox.yMax));

    auto *descriptor = pdfioDictCreate(pdf);
    if (descriptor == nullptr) {
        error_message = "Unable to create RTL glyph CID font descriptor";
        return nullptr;
    }
    pdfioDictSetName(descriptor, "Type", "FontDescriptor");
    pdfioDictSetName(descriptor, "FontName", base_font.c_str());
    pdfioDictSetObj(descriptor, "FontFile2", font_file);
    pdfioDictSetNumber(descriptor, "Flags", 0x20);
    pdfioDictSetArray(descriptor, "FontBBox", bbox);
    pdfioDictSetNumber(descriptor, "ItalicAngle", 0.0);
    pdfioDictSetNumber(descriptor, "Ascent",
                       test_font_unit_to_1000(face_owner.face,
                                              face_owner.face->ascender));
    pdfioDictSetNumber(descriptor, "Descent",
                       test_font_unit_to_1000(face_owner.face,
                                              face_owner.face->descender));
    pdfioDictSetNumber(descriptor, "CapHeight",
                       test_font_unit_to_1000(face_owner.face,
                                              face_owner.face->ascender));
    pdfioDictSetNumber(descriptor, "StemV", 80.0);

    auto *descriptor_obj = pdfioFileCreateObj(pdf, descriptor);
    if (descriptor_obj == nullptr || !pdfioObjClose(descriptor_obj)) {
        error_message = "Unable to create RTL glyph CID descriptor object";
        return nullptr;
    }

    auto *cid_to_gid =
        create_test_cid_to_gid_map_object(pdf, entries, base_font,
                                          error_message);
    if (cid_to_gid == nullptr) {
        return nullptr;
    }

    auto *to_unicode =
        create_test_to_unicode_object(pdf, entries, base_font, error_message);
    if (to_unicode == nullptr) {
        return nullptr;
    }

    auto *widths = create_test_width_array(pdf, entries);
    if (widths == nullptr) {
        error_message = "Unable to create RTL glyph CID widths";
        return nullptr;
    }

    auto *system_info = pdfioDictCreate(pdf);
    if (system_info == nullptr) {
        error_message = "Unable to create RTL glyph CID system info";
        return nullptr;
    }
    pdfioDictSetString(system_info, "Registry", "Adobe");
    pdfioDictSetString(system_info, "Ordering", "Identity");
    pdfioDictSetNumber(system_info, "Supplement", 0.0);

    auto *cid_font = pdfioDictCreate(pdf);
    if (cid_font == nullptr) {
        error_message = "Unable to create RTL glyph CID descendant font";
        return nullptr;
    }
    pdfioDictSetName(cid_font, "Type", "Font");
    pdfioDictSetName(cid_font, "Subtype", "CIDFontType2");
    pdfioDictSetName(cid_font, "BaseFont", base_font.c_str());
    pdfioDictSetDict(cid_font, "CIDSystemInfo", system_info);
    pdfioDictSetObj(cid_font, "CIDToGIDMap", cid_to_gid);
    pdfioDictSetObj(cid_font, "FontDescriptor", descriptor_obj);
    pdfioDictSetArray(cid_font, "W", widths);

    auto *cid_font_obj = pdfioFileCreateObj(pdf, cid_font);
    if (cid_font_obj == nullptr || !pdfioObjClose(cid_font_obj)) {
        error_message = "Unable to create RTL glyph CID descendant font object";
        return nullptr;
    }

    auto *descendants = pdfioArrayCreate(pdf);
    if (descendants == nullptr ||
        !pdfioArrayAppendObj(descendants, cid_font_obj)) {
        error_message = "Unable to create RTL glyph CID descendant list";
        return nullptr;
    }

    auto *type0 = pdfioDictCreate(pdf);
    if (type0 == nullptr) {
        error_message = "Unable to create RTL glyph CID Type0 font";
        return nullptr;
    }
    pdfioDictSetName(type0, "Type", "Font");
    pdfioDictSetName(type0, "Subtype", "Type0");
    pdfioDictSetName(type0, "BaseFont", base_font.c_str());
    pdfioDictSetArray(type0, "DescendantFonts", descendants);
    pdfioDictSetName(type0, "Encoding", "Identity-H");
    pdfioDictSetObj(type0, "ToUnicode", to_unicode);

    auto *font = pdfioFileCreateObj(pdf, type0);
    if (font != nullptr && pdfioObjClose(font)) {
        return font;
    }

    error_message = "Unable to create RTL glyph CID Type0 font object";
    return nullptr;
}

[[nodiscard]] bool write_hebrew_actual_text_fixture(
    const std::filesystem::path &output_path,
    const std::filesystem::path &font_path,
    std::string_view actual_text,
    std::string_view shown_text,
    std::string &error_message) {
    auto output = output_path.string();
    pdfio_rect_t media_box{0.0, 0.0, 595.275590551, 841.88976378};
    std::unique_ptr<pdfio_file_t, PdfioFileCloser> pdf(pdfioFileCreate(
        output.c_str(), "1.7", &media_box, &media_box, capture_pdfio_error,
        &error_message));
    if (!pdf) {
        if (error_message.empty()) {
            error_message = "Unable to create PDF fixture";
        }
        return false;
    }

    auto font_file = font_path.string();
    auto *font =
        pdfioFileCreateFontObjFromFile(pdf.get(), font_file.c_str(), true);
    if (font == nullptr) {
        if (error_message.empty()) {
            error_message = "Unable to create Unicode font fixture";
        }
        return false;
    }

    auto *page_dict = pdfioDictCreate(pdf.get());
    if (page_dict == nullptr ||
        !pdfioPageDictAddFont(page_dict, "F1", font)) {
        if (error_message.empty()) {
            error_message = "Unable to create PDF page font resource";
        }
        return false;
    }

    std::unique_ptr<pdfio_stream_t, PdfioStreamCloser> stream(
        pdfioFileCreatePage(pdf.get(), page_dict));
    if (!stream) {
        if (error_message.empty()) {
            error_message = "Unable to create PDF page stream";
        }
        return false;
    }

    const auto actual_text_command =
        std::string{"/Span <</ActualText <FEFF"} +
        utf16be_hex_payload_from_utf8(actual_text) + ">>> BDC\n";
    const auto shown_text_string = std::string{shown_text};
    if (!pdfioStreamPuts(stream.get(), actual_text_command.c_str()) ||
        !pdfioContentTextBegin(stream.get()) ||
        !pdfioContentSetFillColorDeviceRGB(stream.get(), 0.0, 0.0, 0.0) ||
        !pdfioContentSetTextFont(stream.get(), "F1", 18.0) ||
        !pdfioContentTextMoveTo(stream.get(), 72.0, 720.0) ||
        !pdfioContentTextShow(stream.get(), true, shown_text_string.c_str()) ||
        !pdfioContentTextEnd(stream.get()) ||
        !pdfioStreamPuts(stream.get(), "EMC\n")) {
        if (error_message.empty()) {
            error_message =
                "Unable to write PDF ActualText extraction fixture";
        }
        return false;
    }

    if (!pdfioStreamClose(stream.release())) {
        if (error_message.empty()) {
            error_message = "Unable to close PDF page stream";
        }
        return false;
    }
    if (!pdfioFileClose(pdf.release())) {
        if (error_message.empty()) {
            error_message = "Unable to close PDF fixture";
        }
        return false;
    }
    return true;
}

struct RtlActualTextNormalizationCase {
    std::string suffix;
    std::string logical_order_text;
    std::string visual_order_text;
    std::vector<std::string> expected_raw_fragments;
    std::vector<std::string> unexpected_normalized_fragments;
};

void check_hebrew_actual_text_normalization_case(
    const std::filesystem::path &rtl_font,
    std::string_view fixture_group,
    const RtlActualTextNormalizationCase &fixture_case) {
    const auto group = std::string{fixture_group};
    CAPTURE(group);
    CAPTURE(fixture_case.suffix);

    const auto output_path =
        std::filesystem::current_path() /
        ("featherdoc-hebrew-rtl-" + group + "-" + fixture_case.suffix +
         "-boundary.pdf");

    std::string error_message;
    REQUIRE_MESSAGE(write_hebrew_actual_text_fixture(
                        output_path, rtl_font, fixture_case.logical_order_text,
                        fixture_case.visual_order_text, error_message),
                    error_message);

    featherdoc::pdf::PdfiumParser parser;

    featherdoc::pdf::PdfParseOptions raw_options;
    raw_options.normalize_rtl_text = false;
    const auto raw_parse_result = parser.parse(output_path, raw_options);
    REQUIRE_MESSAGE(raw_parse_result.success, raw_parse_result.error_message);
    const auto raw_paragraph_text =
        collect_paragraph_text(raw_parse_result.document);
    CHECK_EQ(count_occurrences(raw_paragraph_text,
                               fixture_case.logical_order_text),
             0U);
    for (const auto &fragment : fixture_case.expected_raw_fragments) {
        CAPTURE(fragment);
        CHECK_NE(raw_paragraph_text.find(fragment), std::string::npos);
    }

    const auto normalized_parse_result = parser.parse(output_path, {});
    REQUIRE_MESSAGE(normalized_parse_result.success,
                    normalized_parse_result.error_message);
    const auto normalized_paragraph_text =
        collect_paragraph_text(normalized_parse_result.document);
    CHECK_EQ(count_occurrences(normalized_paragraph_text,
                               fixture_case.logical_order_text),
             1U);
    for (const auto &fragment :
         fixture_case.unexpected_normalized_fragments) {
        CAPTURE(fragment);
        CHECK_EQ(count_occurrences(normalized_paragraph_text, fragment), 0U);
    }

    const auto imported_docx_path =
        std::filesystem::current_path() /
        ("featherdoc-hebrew-rtl-" + group + "-" + fixture_case.suffix +
         "-boundary-import.docx");
    std::filesystem::remove(imported_docx_path);

    featherdoc::Document imported_document(imported_docx_path);
    const auto import_result =
        featherdoc::pdf::import_pdf_text_document(output_path,
                                                  imported_document);
    REQUIRE_MESSAGE(import_result.success, import_result.error_message);
    CHECK_EQ(import_result.paragraphs_imported, 1U);
    CHECK_EQ(collect_document_text(imported_document),
             fixture_case.logical_order_text + "\n");
}

struct RtlActualTextRawPreservationCase {
    std::string suffix;
    std::string logical_order_text;
    std::string visual_order_text;
    std::vector<std::string> expected_raw_fragments;
    std::vector<std::string> unexpected_normalized_fragments;
    bool verify_raw_span_geometry{false};
};

void check_rtl_actual_text_raw_preservation_case(
    const std::filesystem::path &rtl_font,
    std::string_view fixture_group,
    const RtlActualTextRawPreservationCase &fixture_case) {
    const auto group = std::string{fixture_group};
    CAPTURE(group);
    CAPTURE(fixture_case.suffix);

    const auto output_path =
        std::filesystem::current_path() /
        ("featherdoc-" + group + "-" + fixture_case.suffix + "-boundary.pdf");

    std::string error_message;
    REQUIRE_MESSAGE(write_hebrew_actual_text_fixture(
                        output_path, rtl_font, fixture_case.logical_order_text,
                        fixture_case.visual_order_text, error_message),
                    error_message);

    featherdoc::pdf::PdfiumParser parser;

    featherdoc::pdf::PdfParseOptions raw_options;
    raw_options.normalize_rtl_text = false;
    const auto raw_parse_result = parser.parse(output_path, raw_options);
    REQUIRE_MESSAGE(raw_parse_result.success, raw_parse_result.error_message);
    const auto raw_paragraph_text =
        collect_paragraph_text(raw_parse_result.document);
    REQUIRE_FALSE(raw_paragraph_text.empty());
    INFO("raw RTL preservation paragraph text: " << raw_paragraph_text);
    CHECK_EQ(count_occurrences(raw_paragraph_text,
                               fixture_case.logical_order_text),
             0U);
    for (const auto &fragment : fixture_case.expected_raw_fragments) {
        CAPTURE(fragment);
        CHECK_NE(raw_paragraph_text.find(fragment), std::string::npos);
        if (fixture_case.verify_raw_span_geometry) {
            check_raw_span_sequence_has_geometry(raw_parse_result.document,
                                                 fragment);
        }
    }

    const auto normalized_parse_result = parser.parse(output_path, {});
    REQUIRE_MESSAGE(normalized_parse_result.success,
                    normalized_parse_result.error_message);
    const auto normalized_paragraph_text =
        collect_paragraph_text(normalized_parse_result.document);
    CHECK_EQ(normalized_paragraph_text, raw_paragraph_text);
    for (const auto &fragment :
         fixture_case.unexpected_normalized_fragments) {
        CAPTURE(fragment);
        CHECK_EQ(count_occurrences(normalized_paragraph_text, fragment), 0U);
    }

    const auto imported_docx_path =
        std::filesystem::current_path() /
        ("featherdoc-" + group + "-" + fixture_case.suffix +
         "-boundary-import.docx");
    std::filesystem::remove(imported_docx_path);

    featherdoc::Document imported_document(imported_docx_path);
    const auto import_result =
        featherdoc::pdf::import_pdf_text_document(output_path,
                                                  imported_document);
    REQUIRE_MESSAGE(import_result.success, import_result.error_message);
    CHECK_EQ(import_result.paragraphs_imported, 1U);
    CHECK_EQ(collect_document_text(imported_document), raw_paragraph_text);
}

[[nodiscard]] bool write_rtl_split_actual_text_fixture(
    const std::filesystem::path &output_path,
    const std::filesystem::path &font_path,
    std::string_view actual_text,
    const std::vector<std::pair<double, std::string>> &shown_segments,
    std::string &error_message) {
    auto output = output_path.string();
    pdfio_rect_t media_box{0.0, 0.0, 595.275590551, 841.88976378};
    std::unique_ptr<pdfio_file_t, PdfioFileCloser> pdf(pdfioFileCreate(
        output.c_str(), "1.7", &media_box, &media_box, capture_pdfio_error,
        &error_message));
    if (!pdf) {
        if (error_message.empty()) {
            error_message = "Unable to create split RTL PDF fixture";
        }
        return false;
    }

    auto font_file = font_path.string();
    auto *font =
        pdfioFileCreateFontObjFromFile(pdf.get(), font_file.c_str(), true);
    if (font == nullptr) {
        if (error_message.empty()) {
            error_message = "Unable to create split RTL Unicode font fixture";
        }
        return false;
    }

    auto *page_dict = pdfioDictCreate(pdf.get());
    if (page_dict == nullptr ||
        !pdfioPageDictAddFont(page_dict, "F1", font)) {
        if (error_message.empty()) {
            error_message = "Unable to create split RTL page font resource";
        }
        return false;
    }

    std::unique_ptr<pdfio_stream_t, PdfioStreamCloser> stream(
        pdfioFileCreatePage(pdf.get(), page_dict));
    if (!stream) {
        if (error_message.empty()) {
            error_message = "Unable to create split RTL page stream";
        }
        return false;
    }

    const auto actual_text_command =
        std::string{"/Span <</ActualText <FEFF"} +
        utf16be_hex_payload_from_utf8(actual_text) + ">>> BDC\n";
    if (!pdfioStreamPuts(stream.get(), actual_text_command.c_str()) ||
        !pdfioContentTextBegin(stream.get()) ||
        !pdfioContentSetFillColorDeviceRGB(stream.get(), 0.0, 0.0, 0.0) ||
        !pdfioContentSetTextFont(stream.get(), "F1", 18.0)) {
        if (error_message.empty()) {
            error_message = "Unable to begin split RTL content stream";
        }
        return false;
    }

    for (const auto &[x_points, text] : shown_segments) {
        pdfio_matrix_t matrix{
            {1.0, 0.0},
            {0.0, 1.0},
            {x_points, 720.0},
        };
        if (!pdfioContentSetTextMatrix(stream.get(), matrix) ||
            !pdfioContentTextShow(stream.get(), true, text.c_str())) {
            if (error_message.empty()) {
                error_message = "Unable to write split RTL text segment";
            }
            return false;
        }
    }

    if (!pdfioContentTextEnd(stream.get()) ||
        !pdfioStreamPuts(stream.get(), "EMC\n")) {
        if (error_message.empty()) {
            error_message = "Unable to finish split RTL content stream";
        }
        return false;
    }

    if (!pdfioStreamClose(stream.release())) {
        if (error_message.empty()) {
            error_message = "Unable to close split RTL page stream";
        }
        return false;
    }
    if (!pdfioFileClose(pdf.release())) {
        if (error_message.empty()) {
            error_message = "Unable to close split RTL PDF fixture";
        }
        return false;
    }

    return true;
}

[[nodiscard]] bool write_hebrew_rtl_glyph_cid_fixture(
    const std::filesystem::path &output_path,
    const std::filesystem::path &font_path,
    const featherdoc::pdf::PdfGlyphRun &glyph_run,
    const std::vector<TestGlyphCidEntry> &entries,
    std::string_view actual_text,
    bool use_reversed_chars_marker,
    std::string &error_message) {
    if (glyph_run.glyphs.empty()) {
        error_message = "RTL glyph CID fixture has no shaped glyphs";
        return false;
    }
    if (glyph_run.glyphs.size() >
        static_cast<std::size_t>(std::numeric_limits<std::uint16_t>::max())) {
        error_message = "RTL glyph CID fixture has too many glyphs";
        return false;
    }
    for (const auto &glyph : glyph_run.glyphs) {
        if (glyph.glyph_id == 0U ||
            glyph.glyph_id >
                static_cast<std::uint32_t>(
                    std::numeric_limits<std::uint16_t>::max())) {
            error_message = "RTL glyph CID fixture has unsupported glyph id";
            return false;
        }
    }
    if (entries.size() != glyph_run.glyphs.size()) {
        error_message = "RTL glyph CID fixture entry count mismatch";
        return false;
    }

    auto output = output_path.string();
    pdfio_rect_t media_box{0.0, 0.0, 595.275590551, 841.88976378};
    std::unique_ptr<pdfio_file_t, PdfioFileCloser> pdf(pdfioFileCreate(
        output.c_str(), "1.7", &media_box, &media_box, capture_pdfio_error,
        &error_message));
    if (!pdf) {
        if (error_message.empty()) {
            error_message = "Unable to create RTL glyph CID PDF fixture";
        }
        return false;
    }

    auto *font =
        create_test_glyph_cid_font(pdf.get(), font_path, entries, error_message);
    if (font == nullptr) {
        if (error_message.empty()) {
            error_message = "Unable to create RTL glyph CID font fixture";
        }
        return false;
    }

    auto *page_dict = pdfioDictCreate(pdf.get());
    if (page_dict == nullptr ||
        !pdfioPageDictAddFont(page_dict, "F1", font)) {
        if (error_message.empty()) {
            error_message = "Unable to create RTL glyph CID page font resource";
        }
        return false;
    }

    std::unique_ptr<pdfio_stream_t, PdfioStreamCloser> stream(
        pdfioFileCreatePage(pdf.get(), page_dict));
    if (!stream) {
        if (error_message.empty()) {
            error_message = "Unable to create RTL glyph CID page stream";
        }
        return false;
    }

    const auto actual_text_command =
        std::string{"/Span <</ActualText <FEFF"} +
        utf16be_hex_payload_from_utf8(actual_text) + ">>> BDC\n";
    const auto glyph_show_command = cid_tj_command(glyph_run.glyphs.size());
    if (!pdfioStreamPuts(stream.get(), actual_text_command.c_str()) ||
        !pdfioContentTextBegin(stream.get()) ||
        !pdfioContentSetFillColorDeviceRGB(stream.get(), 0.0, 0.0, 0.0) ||
        !pdfioContentSetTextFont(stream.get(), "F1", 18.0) ||
        !pdfioContentTextMoveTo(stream.get(), 72.0, 720.0)) {
        if (error_message.empty()) {
            error_message = "Unable to begin RTL glyph CID content stream";
        }
        return false;
    }

    if (use_reversed_chars_marker &&
        !pdfioStreamPuts(stream.get(), "/ReversedChars BMC\n")) {
        error_message = "Unable to write RTL glyph CID ReversedChars marker";
        return false;
    }

    if (!pdfioStreamPuts(stream.get(), glyph_show_command.c_str())) {
        error_message = "Unable to write RTL glyph CID text";
        return false;
    }

    if (use_reversed_chars_marker &&
        !pdfioStreamPuts(stream.get(), "EMC\n")) {
        error_message = "Unable to close RTL glyph CID ReversedChars marker";
        return false;
    }

    if (!pdfioContentTextEnd(stream.get()) ||
        !pdfioStreamPuts(stream.get(), "EMC\n")) {
        if (error_message.empty()) {
            error_message = "Unable to finish RTL glyph CID content stream";
        }
        return false;
    }

    if (!pdfioStreamClose(stream.release())) {
        if (error_message.empty()) {
            error_message = "Unable to close RTL glyph CID page stream";
        }
        return false;
    }
    if (!pdfioFileClose(pdf.release())) {
        if (error_message.empty()) {
            error_message = "Unable to close RTL glyph CID PDF fixture";
        }
        return false;
    }
    return true;
}

[[nodiscard]] bool write_hebrew_rtl_glyph_cid_positioned_logical_fixture(
    const std::filesystem::path &output_path,
    const std::filesystem::path &font_path,
    const featherdoc::pdf::PdfGlyphRun &glyph_run,
    const std::vector<TestGlyphCidEntry> &entries,
    std::string_view actual_text,
    std::string &error_message) {
    if (glyph_run.glyphs.empty()) {
        error_message = "RTL positioned glyph CID fixture has no shaped glyphs";
        return false;
    }
    if (entries.size() != glyph_run.glyphs.size()) {
        error_message = "RTL positioned glyph CID fixture entry count mismatch";
        return false;
    }

    std::vector<double> visual_x_positions;
    visual_x_positions.reserve(glyph_run.glyphs.size());
    double pen_x = 72.0;
    for (const auto &glyph : glyph_run.glyphs) {
        if (glyph.glyph_id == 0U ||
            glyph.glyph_id >
                static_cast<std::uint32_t>(
                    std::numeric_limits<std::uint16_t>::max()) ||
            !std::isfinite(glyph.x_advance_points) ||
            !std::isfinite(glyph.x_offset_points)) {
            error_message =
                "RTL positioned glyph CID fixture has unsupported glyph data";
            return false;
        }
        visual_x_positions.push_back(pen_x + glyph.x_offset_points);
        pen_x += glyph.x_advance_points;
    }

    auto output = output_path.string();
    pdfio_rect_t media_box{0.0, 0.0, 595.275590551, 841.88976378};
    std::unique_ptr<pdfio_file_t, PdfioFileCloser> pdf(pdfioFileCreate(
        output.c_str(), "1.7", &media_box, &media_box, capture_pdfio_error,
        &error_message));
    if (!pdf) {
        if (error_message.empty()) {
            error_message =
                "Unable to create RTL positioned glyph CID PDF fixture";
        }
        return false;
    }

    auto *font =
        create_test_glyph_cid_font(pdf.get(), font_path, entries, error_message);
    if (font == nullptr) {
        if (error_message.empty()) {
            error_message =
                "Unable to create RTL positioned glyph CID font fixture";
        }
        return false;
    }

    auto *page_dict = pdfioDictCreate(pdf.get());
    if (page_dict == nullptr ||
        !pdfioPageDictAddFont(page_dict, "F1", font)) {
        if (error_message.empty()) {
            error_message =
                "Unable to create RTL positioned glyph CID page resource";
        }
        return false;
    }

    std::unique_ptr<pdfio_stream_t, PdfioStreamCloser> stream(
        pdfioFileCreatePage(pdf.get(), page_dict));
    if (!stream) {
        if (error_message.empty()) {
            error_message =
                "Unable to create RTL positioned glyph CID page stream";
        }
        return false;
    }

    const auto actual_text_command =
        std::string{"/Span <</ActualText <FEFF"} +
        utf16be_hex_payload_from_utf8(actual_text) + ">>> BDC\n";
    if (!pdfioStreamPuts(stream.get(), actual_text_command.c_str()) ||
        !pdfioContentTextBegin(stream.get()) ||
        !pdfioContentSetFillColorDeviceRGB(stream.get(), 0.0, 0.0, 0.0) ||
        !pdfioContentSetTextFont(stream.get(), "F1", 18.0)) {
        if (error_message.empty()) {
            error_message =
                "Unable to begin RTL positioned glyph CID content stream";
        }
        return false;
    }

    for (std::size_t reverse_index = glyph_run.glyphs.size(); reverse_index > 0U;
         --reverse_index) {
        const auto visual_index = reverse_index - 1U;
        const auto cid = static_cast<std::uint16_t>(visual_index + 1U);
        if (!pdfioStreamPrintf(stream.get(), "1 0 0 1 %.4f %.4f Tm\n",
                               visual_x_positions[visual_index], 720.0) ||
            !pdfioStreamPuts(stream.get(), single_cid_tj_command(cid).c_str())) {
            if (error_message.empty()) {
                error_message =
                    "Unable to write RTL positioned glyph CID text";
            }
            return false;
        }
    }

    if (!pdfioContentTextEnd(stream.get()) ||
        !pdfioStreamPuts(stream.get(), "EMC\n")) {
        if (error_message.empty()) {
            error_message =
                "Unable to finish RTL positioned glyph CID content stream";
        }
        return false;
    }

    if (!pdfioStreamClose(stream.release())) {
        if (error_message.empty()) {
            error_message = "Unable to close RTL positioned glyph CID stream";
        }
        return false;
    }
    if (!pdfioFileClose(pdf.release())) {
        if (error_message.empty()) {
            error_message = "Unable to close RTL positioned glyph CID PDF";
        }
        return false;
    }
    return true;
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

TEST_CASE("PDFio falls back for non-LTR shaped glyph directions") {
    const auto font_path = find_latin_font();
    if (font_path.empty()) {
        MESSAGE("skipping shaped glyph direction fallback smoke: configure "
                "test Latin font");
        return;
    }

    struct DirectionFallbackCase {
        featherdoc::pdf::PdfGlyphDirection direction{
            featherdoc::pdf::PdfGlyphDirection::unknown};
        std::string label;
    };

    const std::array<DirectionFallbackCase, 4U> fallback_cases{{
        {featherdoc::pdf::PdfGlyphDirection::right_to_left, "rtl"},
        {featherdoc::pdf::PdfGlyphDirection::top_to_bottom, "ttb"},
        {featherdoc::pdf::PdfGlyphDirection::bottom_to_top, "btt"},
        {featherdoc::pdf::PdfGlyphDirection::unknown, "unknown"},
    }};

    const std::string expected_text = "abc";
    constexpr double font_size = 18.0;

    for (const auto &fallback_case : fallback_cases) {
        CAPTURE(fallback_case.label);

        featherdoc::pdf::PdfGlyphRun glyph_run;
        glyph_run.text = expected_text;
        glyph_run.font_file_path = font_path;
        glyph_run.font_size_points = font_size;
        glyph_run.direction = fallback_case.direction;
        glyph_run.used_harfbuzz = true;
        glyph_run.glyphs = {
            featherdoc::pdf::PdfGlyphPosition{1U, 0U, 6.0, 0.0, 0.0, 0.0},
            featherdoc::pdf::PdfGlyphPosition{2U, 1U, 6.0, 0.0, 0.0, 0.0},
            featherdoc::pdf::PdfGlyphPosition{3U, 2U, 6.0, 0.0, 0.0, 0.0},
        };

        const auto output_path =
            std::filesystem::current_path() /
            ("featherdoc-shaped-glyph-direction-fallback-" +
             fallback_case.label + ".pdf");

        featherdoc::pdf::PdfDocumentLayout layout;
        layout.metadata.title = "FeatherDoc shaped glyph direction fallback";
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
        const auto write_result = generator.write(
            layout, output_path, featherdoc::pdf::PdfWriterOptions{});
        REQUIRE_MESSAGE(write_result.success, write_result.error_message);
        CHECK_GT(write_result.bytes_written, 0U);

        const auto pdf_bytes = read_file_bytes(output_path);
        CHECK_EQ(pdf_bytes.find("/FeatherDocGlyph"), std::string::npos);

        featherdoc::pdf::PdfiumParser parser;
        const auto parse_result = parser.parse(output_path, {});
        REQUIRE_MESSAGE(parse_result.success, parse_result.error_message);

        const auto extracted_text = collect_text(parse_result.document);
        CHECK_EQ(count_occurrences(extracted_text, expected_text), 1U);
    }
}

TEST_CASE("PDFio falls back for document adapter RTL shaped runs") {
    const auto font_path = find_latin_font();
    if (font_path.empty()) {
        MESSAGE("skipping document RTL fallback smoke: configure test Latin "
                "font");
        return;
    }
    if (!featherdoc::pdf::pdf_text_shaper_has_harfbuzz()) {
        MESSAGE("skipping document RTL fallback smoke: HarfBuzz unavailable");
        return;
    }

    const std::string expected_text = "abc";

    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());

    auto paragraph = document.paragraphs();
    REQUIRE(paragraph.has_next());
    auto rtl_run = paragraph.add_run(expected_text);
    REQUIRE(rtl_run.has_next());
    CHECK(rtl_run.set_font_family("Latin RTL Fallback"));
    CHECK(rtl_run.set_rtl());

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.font_mappings = {
        featherdoc::pdf::PdfFontMapping{"Latin RTL Fallback", font_path},
    };
    options.use_system_font_fallbacks = false;
    options.font_size_points = 18.0;

    auto layout = featherdoc::pdf::layout_document_paragraphs(document, options);
    REQUIRE_EQ(layout.pages.size(), 1U);
    REQUIRE_EQ(layout.pages.front().text_runs.size(), 1U);

    const auto &text_run = layout.pages.front().text_runs.front();
    CHECK_EQ(text_run.text, expected_text);
    CHECK_EQ(text_run.font_file_path, font_path);
    CHECK(text_run.glyph_run.used_harfbuzz);
    CHECK(text_run.glyph_run.error_message.empty());
    CHECK_EQ(text_run.glyph_run.direction,
             featherdoc::pdf::PdfGlyphDirection::right_to_left);
    CHECK_EQ(text_run.glyph_run.script_tag, "Latn");

    const auto output_path =
        std::filesystem::current_path() /
        "featherdoc-document-rtl-glyph-fallback.pdf";

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
    CHECK_EQ(count_occurrences(extracted_text, expected_text), 1U);
}

TEST_CASE("PDFio records natural RTL fallback extraction boundary") {
    const auto rtl_font = find_rtl_font();
    if (rtl_font.empty()) {
        MESSAGE("skipping natural RTL fallback boundary smoke: configure test "
                "RTL font");
        return;
    }
    if (!featherdoc::pdf::pdf_text_shaper_has_harfbuzz()) {
        MESSAGE("skipping natural RTL fallback boundary smoke: HarfBuzz "
                "unavailable");
        return;
    }

    const auto expected_text = hebrew_shalom_logical();
    const auto visual_order_text = hebrew_shalom_visual();

    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());

    auto paragraph = document.paragraphs();
    REQUIRE(paragraph.has_next());
    auto rtl_run = paragraph.add_run(expected_text);
    REQUIRE(rtl_run.has_next());
    CHECK(rtl_run.set_font_family("Hebrew RTL Fallback"));

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.font_mappings = {
        featherdoc::pdf::PdfFontMapping{"Hebrew RTL Fallback", rtl_font},
    };
    options.use_system_font_fallbacks = false;
    options.font_size_points = 18.0;

    auto layout = featherdoc::pdf::layout_document_paragraphs(document, options);
    REQUIRE_EQ(layout.pages.size(), 1U);
    REQUIRE_EQ(layout.pages.front().text_runs.size(), 1U);

    const auto &text_run = layout.pages.front().text_runs.front();
    CHECK_EQ(text_run.text, expected_text);
    CHECK_EQ(text_run.font_file_path, rtl_font);
    CHECK(text_run.glyph_run.used_harfbuzz);
    CHECK(text_run.glyph_run.error_message.empty());
    CHECK_EQ(text_run.glyph_run.direction,
             featherdoc::pdf::PdfGlyphDirection::right_to_left);
    CHECK_EQ(text_run.glyph_run.script_tag, "Hebr");
    REQUIRE_FALSE(text_run.glyph_run.glyphs.empty());

    const auto output_path =
        std::filesystem::current_path() /
        "featherdoc-document-natural-rtl-glyph-fallback.pdf";

    featherdoc::pdf::PdfioGenerator generator;
    const auto write_result =
        generator.write(layout, output_path, featherdoc::pdf::PdfWriterOptions{});
    REQUIRE_MESSAGE(write_result.success, write_result.error_message);
    CHECK_GT(write_result.bytes_written, 0U);

    check_hebrew_fallback_extraction_boundary(
        output_path,
        expected_text,
        expected_text,
        expected_text,
        visual_order_text,
        0U,
        1U);

    featherdoc::pdf::PdfiumParser parser;

    featherdoc::pdf::PdfParseOptions raw_options;
    raw_options.normalize_rtl_text = false;
    const auto raw_parse_result = parser.parse(output_path, raw_options);
    REQUIRE_MESSAGE(raw_parse_result.success, raw_parse_result.error_message);
    const auto raw_span_text = collect_text(raw_parse_result.document);
    CHECK_EQ(count_occurrences(raw_span_text, expected_text), 0U);
    CHECK_EQ(count_occurrences(raw_span_text, visual_order_text), 1U);
    const auto raw_line_text = collect_line_text(raw_parse_result.document);
    CHECK_EQ(count_occurrences(raw_line_text, expected_text), 0U);
    CHECK_EQ(count_occurrences(raw_line_text, visual_order_text), 1U);
    const auto raw_paragraph_text =
        collect_paragraph_text(raw_parse_result.document);
    CHECK_EQ(count_occurrences(raw_paragraph_text, expected_text), 0U);
    CHECK_EQ(count_occurrences(raw_paragraph_text, visual_order_text), 1U);

    featherdoc::pdf::PdfParseOptions normalized_options;
    normalized_options.normalize_rtl_text = true;
    const auto normalized_parse_result =
        parser.parse(output_path, normalized_options);
    REQUIRE_MESSAGE(normalized_parse_result.success,
                    normalized_parse_result.error_message);
    const auto normalized_span_text =
        collect_text(normalized_parse_result.document);
    CHECK_EQ(count_occurrences(normalized_span_text, expected_text), 0U);
    CHECK_EQ(count_occurrences(normalized_span_text, visual_order_text), 1U);
    const auto normalized_line_text =
        collect_line_text(normalized_parse_result.document);
    CHECK_EQ(count_occurrences(normalized_line_text, expected_text), 1U);
    CHECK_EQ(count_occurrences(normalized_line_text, visual_order_text), 0U);
    const auto normalized_paragraph_text =
        collect_paragraph_text(normalized_parse_result.document);
    CHECK_EQ(count_occurrences(normalized_paragraph_text, expected_text), 1U);
    CHECK_EQ(count_occurrences(normalized_paragraph_text, visual_order_text), 0U);

    const auto imported_docx_path =
        std::filesystem::current_path() /
        "featherdoc-document-natural-rtl-glyph-fallback-import.docx";
    std::filesystem::remove(imported_docx_path);

    featherdoc::Document imported_document(imported_docx_path);
    const auto import_result =
        featherdoc::pdf::import_pdf_text_document(output_path, imported_document);
    REQUIRE_MESSAGE(import_result.success, import_result.error_message);
    CHECK_EQ(import_result.paragraphs_imported, 1U);
    CHECK_EQ(collect_document_text(imported_document), expected_text + "\n");
    REQUIRE_FALSE(imported_document.save());

    featherdoc::Document reopened_document(imported_docx_path);
    REQUIRE_FALSE(reopened_document.open());
    CHECK_EQ(collect_document_text(reopened_document), expected_text + "\n");

    const auto raw_imported_docx_path =
        std::filesystem::current_path() /
        "featherdoc-document-natural-rtl-glyph-fallback-raw-import.docx";
    std::filesystem::remove(raw_imported_docx_path);

    featherdoc::Document raw_imported_document(raw_imported_docx_path);
    featherdoc::pdf::PdfDocumentImportOptions raw_import_options;
    raw_import_options.parse_options.normalize_rtl_text = false;
    const auto raw_import_result = featherdoc::pdf::import_pdf_text_document(
        output_path, raw_imported_document, raw_import_options);
    REQUIRE_MESSAGE(raw_import_result.success,
                    raw_import_result.error_message);
    CHECK_EQ(raw_import_result.paragraphs_imported, 1U);
    CHECK_EQ(collect_document_text(raw_imported_document), raw_paragraph_text);
}

TEST_CASE("PDFium RTL normalization leaves digit mixes raw") {
    const auto rtl_font = find_rtl_font();
    if (rtl_font.empty()) {
        MESSAGE("skipping Hebrew digit-mix normalization boundary smoke: "
                "configure test RTL font");
        return;
    }

    const auto logical_text = hebrew_shalom_logical() + " 123";
    const auto visual_order_text = hebrew_shalom_visual() + " 123";

    check_rtl_actual_text_raw_preservation_case(
        rtl_font,
        "hebrew-rtl",
        RtlActualTextRawPreservationCase{
            "digit-mix", logical_text, visual_order_text, {"123"}, {"321"}});
}

TEST_CASE("PDFium RTL normalization leaves LTR mixes raw") {
    const auto rtl_font = find_rtl_font();
    if (rtl_font.empty()) {
        MESSAGE("skipping Hebrew LTR-mix normalization boundary smoke: "
                "configure test RTL font");
        return;
    }

    const auto logical_rtl_text = hebrew_shalom_logical();
    const auto visual_rtl_text = hebrew_shalom_visual();
    const auto logical_text = std::string{"Status "} + logical_rtl_text;
    const auto visual_order_text = std::string{"Status "} + visual_rtl_text;

    check_rtl_actual_text_raw_preservation_case(
        rtl_font,
        "hebrew-rtl",
        RtlActualTextRawPreservationCase{
            "ltr-mix",
            logical_text,
            visual_order_text,
            {"Status", visual_rtl_text},
            {}});
}

TEST_CASE("PDFium RTL normalization applies to pure Arabic text") {
    const auto rtl_font = find_rtl_font();
    if (rtl_font.empty()) {
        MESSAGE("skipping Arabic RTL normalization smoke: configure test RTL "
                "font");
        return;
    }

    const auto logical_text = arabic_salam_logical();
    const auto visual_order_text = arabic_salam_visual();

    const auto output_path =
        std::filesystem::current_path() /
        "featherdoc-arabic-rtl-normalization-boundary.pdf";

    std::string error_message;
    REQUIRE_MESSAGE(write_hebrew_actual_text_fixture(output_path, rtl_font,
                                                     logical_text,
                                                     visual_order_text,
                                                     error_message),
                    error_message);

    featherdoc::pdf::PdfiumParser parser;

    featherdoc::pdf::PdfParseOptions raw_options;
    raw_options.normalize_rtl_text = false;
    const auto raw_parse_result = parser.parse(output_path, raw_options);
    REQUIRE_MESSAGE(raw_parse_result.success, raw_parse_result.error_message);
    const auto raw_paragraph_text =
        collect_paragraph_text(raw_parse_result.document);
    CHECK_EQ(count_occurrences(raw_paragraph_text, logical_text), 0U);
    CHECK_EQ(count_occurrences(raw_paragraph_text, visual_order_text), 1U);

    const auto normalized_parse_result = parser.parse(output_path, {});
    REQUIRE_MESSAGE(normalized_parse_result.success,
                    normalized_parse_result.error_message);
    const auto normalized_paragraph_text =
        collect_paragraph_text(normalized_parse_result.document);
    CHECK_EQ(count_occurrences(normalized_paragraph_text, logical_text), 1U);
    CHECK_EQ(count_occurrences(normalized_paragraph_text, visual_order_text), 0U);

    const auto imported_docx_path =
        std::filesystem::current_path() /
        "featherdoc-arabic-rtl-normalization-boundary-import.docx";
    std::filesystem::remove(imported_docx_path);

    featherdoc::Document imported_document(imported_docx_path);
    const auto import_result =
        featherdoc::pdf::import_pdf_text_document(output_path, imported_document);
    REQUIRE_MESSAGE(import_result.success, import_result.error_message);
    CHECK_EQ(import_result.paragraphs_imported, 1U);
    CHECK_EQ(collect_document_text(imported_document), logical_text + "\n");
}

TEST_CASE("PDFium RTL normalization recovers Arabic lam-alef clusters") {
    const auto rtl_font = find_rtl_font();
    if (rtl_font.empty()) {
        MESSAGE("skipping Arabic presentation-form normalization smoke: "
                "configure test RTL font");
        return;
    }

    struct LamAlefVariant {
        const char *suffix;
        std::uint32_t presentation_form;
        std::uint32_t base_alef;
    };
    const std::array<LamAlefVariant, 4U> variants{{
        {"plain", 0xFEFCU, 0x0627U},
        {"madda", 0xFEF6U, 0x0622U},
        {"hamza-above", 0xFEF8U, 0x0623U},
        {"hamza-below", 0xFEFAU, 0x0625U},
    }};

    featherdoc::pdf::PdfiumParser parser;

    for (const auto &variant : variants) {
        CAPTURE(variant.suffix);
        const auto presentation_lam_alef =
            utf8_from_codepoint(variant.presentation_form);
        const auto base_alef = utf8_from_codepoint(variant.base_alef);

        const auto logical_text = utf8_from_u8(u8"\uFEB3") +
                                  presentation_lam_alef +
                                  utf8_from_u8(u8"\uFEE1");
        const auto visual_order_text = utf8_from_u8(u8"\uFEE1") +
                                       presentation_lam_alef +
                                       utf8_from_u8(u8"\uFEB3");
        const auto pdfium_decomposed_visual_text =
            utf8_from_u8(u8"\u0645\u0644") + base_alef +
            utf8_from_u8(u8"\u0633");
        const auto normalized_text =
            utf8_from_u8(u8"\u0633\u0644") + base_alef +
            utf8_from_u8(u8"\u0645");
        const auto incorrect_simple_reversal =
            utf8_from_u8(u8"\u0633") + base_alef +
            utf8_from_u8(u8"\u0644\u0645");

        const auto output_path = std::filesystem::current_path() /
                                 ("featherdoc-arabic-presentation-form-" +
                                  std::string{variant.suffix} +
                                  "-normalization-boundary.pdf");

        std::string error_message;
        REQUIRE_MESSAGE(write_hebrew_actual_text_fixture(output_path, rtl_font,
                                                         logical_text,
                                                         visual_order_text,
                                                         error_message),
                        error_message);

        featherdoc::pdf::PdfParseOptions raw_options;
        raw_options.normalize_rtl_text = false;
        const auto raw_parse_result = parser.parse(output_path, raw_options);
        REQUIRE_MESSAGE(raw_parse_result.success, raw_parse_result.error_message);
        const auto raw_paragraph_text =
            collect_paragraph_text(raw_parse_result.document);
        CHECK_EQ(count_occurrences(raw_paragraph_text, logical_text), 0U);
        CHECK_EQ(count_occurrences(raw_paragraph_text, visual_order_text), 0U);
        CHECK_EQ(count_occurrences(raw_paragraph_text,
                                   pdfium_decomposed_visual_text),
                 1U);

        const auto normalized_parse_result = parser.parse(output_path, {});
        REQUIRE_MESSAGE(normalized_parse_result.success,
                        normalized_parse_result.error_message);
        const auto normalized_paragraph_text =
            collect_paragraph_text(normalized_parse_result.document);
        CHECK_EQ(count_occurrences(normalized_paragraph_text, normalized_text),
                 1U);
        CHECK_EQ(count_occurrences(normalized_paragraph_text,
                                   incorrect_simple_reversal),
                 0U);

        const auto imported_docx_path =
            std::filesystem::current_path() /
            ("featherdoc-arabic-presentation-form-" +
             std::string{variant.suffix} +
             "-normalization-boundary-import.docx");
        std::filesystem::remove(imported_docx_path);

        featherdoc::Document imported_document(imported_docx_path);
        const auto import_result = featherdoc::pdf::import_pdf_text_document(
            output_path, imported_document);
        REQUIRE_MESSAGE(import_result.success, import_result.error_message);
        CHECK_EQ(import_result.paragraphs_imported, 1U);
        CHECK_EQ(collect_document_text(imported_document), normalized_text + "\n");
    }
}

TEST_CASE("PDFium RTL normalization applies across split punctuation spans") {
    const auto rtl_font = find_rtl_font();
    if (rtl_font.empty()) {
        MESSAGE("skipping Hebrew split punctuation normalization smoke: "
                "configure test RTL font");
        return;
    }

    const auto logical_text = hebrew_shalom_logical() + "!";
    const auto visual_rtl_text = hebrew_shalom_visual();
    const auto visual_order_text = std::string{"!"} + visual_rtl_text;

    const auto output_path =
        std::filesystem::current_path() /
        "featherdoc-hebrew-rtl-split-punctuation-boundary.pdf";

    std::string error_message;
    REQUIRE_MESSAGE(write_rtl_split_actual_text_fixture(
                        output_path, rtl_font, logical_text,
                        {{72.0, "!"}, {84.0, visual_rtl_text}}, error_message),
                    error_message);

    featherdoc::pdf::PdfiumParser parser;

    featherdoc::pdf::PdfParseOptions raw_options;
    raw_options.normalize_rtl_text = false;
    const auto raw_parse_result = parser.parse(output_path, raw_options);
    REQUIRE_MESSAGE(raw_parse_result.success, raw_parse_result.error_message);
    const auto raw_paragraph_text =
        collect_paragraph_text(raw_parse_result.document);
    CHECK_EQ(count_occurrences(raw_paragraph_text, logical_text), 0U);
    CHECK_EQ(count_occurrences(raw_paragraph_text, visual_order_text), 1U);

    const auto normalized_parse_result = parser.parse(output_path, {});
    REQUIRE_MESSAGE(normalized_parse_result.success,
                    normalized_parse_result.error_message);
    const auto normalized_paragraph_text =
        collect_paragraph_text(normalized_parse_result.document);
    CHECK_EQ(count_occurrences(normalized_paragraph_text, logical_text), 1U);
    CHECK_EQ(count_occurrences(normalized_paragraph_text, visual_order_text), 0U);

    const auto imported_docx_path =
        std::filesystem::current_path() /
        "featherdoc-hebrew-rtl-split-punctuation-boundary-import.docx";
    std::filesystem::remove(imported_docx_path);

    featherdoc::Document imported_document(imported_docx_path);
    const auto import_result =
        featherdoc::pdf::import_pdf_text_document(output_path, imported_document);
    REQUIRE_MESSAGE(import_result.success, import_result.error_message);
    CHECK_EQ(import_result.paragraphs_imported, 1U);
    CHECK_EQ(collect_document_text(imported_document), logical_text + "\n");
}

TEST_CASE("PDFium RTL normalization leaves split mixed bidi runs raw") {
    const auto rtl_font = find_rtl_font();
    if (rtl_font.empty()) {
        MESSAGE("skipping Hebrew split mixed-bidi normalization boundary smoke: "
                "configure test RTL font");
        return;
    }

    const auto logical_rtl_text = hebrew_shalom_logical();
    const auto visual_rtl_text = hebrew_shalom_visual();
    const auto logical_text = std::string{"Status "} + logical_rtl_text +
                              ": 123";

    const auto output_path =
        std::filesystem::current_path() /
        "featherdoc-hebrew-rtl-split-mixed-bidi-boundary.pdf";

    std::string error_message;
    REQUIRE_MESSAGE(write_rtl_split_actual_text_fixture(
                        output_path,
                        rtl_font,
                        logical_text,
                        {{72.0, "Status "},
                         {142.0, visual_rtl_text},
                         {190.0, ": 123"}},
                        error_message),
                    error_message);

    featherdoc::pdf::PdfiumParser parser;

    featherdoc::pdf::PdfParseOptions raw_options;
    raw_options.normalize_rtl_text = false;
    const auto raw_parse_result = parser.parse(output_path, raw_options);
    REQUIRE_MESSAGE(raw_parse_result.success, raw_parse_result.error_message);
    const auto raw_paragraph_text =
        collect_paragraph_text(raw_parse_result.document);
    REQUIRE_FALSE(raw_paragraph_text.empty());
    INFO("raw split mixed-bidi paragraph text: " << raw_paragraph_text);
    CHECK_EQ(count_occurrences(raw_paragraph_text, logical_text), 0U);
    CHECK_EQ(count_occurrences(raw_paragraph_text, "Status"), 1U);
    CHECK_EQ(count_occurrences(raw_paragraph_text, visual_rtl_text), 1U);
    CHECK_EQ(count_occurrences(raw_paragraph_text, "123"), 1U);
    check_raw_span_sequence_has_geometry(raw_parse_result.document, "Status");
    check_raw_span_sequence_has_geometry(raw_parse_result.document,
                                         visual_rtl_text);
    check_raw_span_sequence_has_geometry(raw_parse_result.document, "123");

    const auto normalized_parse_result = parser.parse(output_path, {});
    REQUIRE_MESSAGE(normalized_parse_result.success,
                    normalized_parse_result.error_message);
    const auto normalized_paragraph_text =
        collect_paragraph_text(normalized_parse_result.document);
    CHECK_EQ(normalized_paragraph_text, raw_paragraph_text);
    CHECK_EQ(count_occurrences(normalized_paragraph_text, logical_text), 0U);
    CHECK_EQ(count_occurrences(normalized_paragraph_text, "Status"), 1U);
    CHECK_EQ(count_occurrences(normalized_paragraph_text, visual_rtl_text), 1U);
    CHECK_EQ(count_occurrences(normalized_paragraph_text, "123"), 1U);

    const auto imported_docx_path =
        std::filesystem::current_path() /
        "featherdoc-hebrew-rtl-split-mixed-bidi-boundary-import.docx";
    std::filesystem::remove(imported_docx_path);

    featherdoc::Document imported_document(imported_docx_path);
    const auto import_result =
        featherdoc::pdf::import_pdf_text_document(output_path, imported_document);
    REQUIRE_MESSAGE(import_result.success, import_result.error_message);
    CHECK_EQ(import_result.paragraphs_imported, 1U);
    CHECK_EQ(collect_document_text(imported_document), raw_paragraph_text);
}

TEST_CASE("PDFium RTL normalization recovers edge bracket pairs") {
    const auto rtl_font = find_rtl_font();
    if (rtl_font.empty()) {
        MESSAGE("skipping Hebrew bracket normalization smoke: configure test RTL "
                "font");
        return;
    }

    const auto logical_rtl_text = hebrew_shalom_logical();
    const auto visual_rtl_text = hebrew_shalom_visual();
    const auto logical_text = std::string{"("} + logical_rtl_text + ")";
    const auto visual_ltr_pair_text =
        std::string{"("} + visual_rtl_text + ")";
    const auto visual_mirrored_pair_text =
        std::string{")"} + visual_rtl_text + "(";
    const auto incorrect_reversed_pair =
        std::string{")"} + logical_rtl_text + "(";

    const std::array<RtlActualTextNormalizationCase, 2U> variants{{
        {"ltr-pair",
         logical_text,
         visual_ltr_pair_text,
         {visual_rtl_text},
         {incorrect_reversed_pair}},
        {"mirrored-pair",
         logical_text,
         visual_mirrored_pair_text,
         {visual_rtl_text},
         {incorrect_reversed_pair}},
    }};

    for (const auto &variant : variants) {
        check_hebrew_actual_text_normalization_case(rtl_font, "bracket",
                                                    variant);
    }
}

TEST_CASE("PDFium RTL normalization recovers angle bracket pairs") {
    const auto rtl_font = find_rtl_font();
    if (rtl_font.empty()) {
        MESSAGE("skipping Hebrew angle-bracket normalization smoke: configure "
                "test RTL font");
        return;
    }

    const auto logical_rtl_text = hebrew_shalom_logical();
    const auto visual_rtl_text = hebrew_shalom_visual();
    const auto logical_text = std::string{"<"} + logical_rtl_text + ">";
    const auto visual_ltr_pair_text =
        std::string{"<"} + visual_rtl_text + ">";
    const auto visual_mirrored_pair_text =
        std::string{">"} + visual_rtl_text + "<";
    const auto incorrect_reversed_pair =
        std::string{">"} + logical_rtl_text + "<";

    const std::array<RtlActualTextNormalizationCase, 2U> variants{{
        {"angle-ltr-pair",
         logical_text,
         visual_ltr_pair_text,
         {visual_rtl_text},
         {incorrect_reversed_pair}},
        {"angle-mirrored-pair",
         logical_text,
         visual_mirrored_pair_text,
         {visual_rtl_text},
         {incorrect_reversed_pair}},
    }};

    for (const auto &variant : variants) {
        check_hebrew_actual_text_normalization_case(rtl_font, "bracket",
                                                    variant);
    }
}

TEST_CASE("PDFium RTL normalization recovers middle bracket pairs") {
    const auto rtl_font = find_rtl_font();
    if (rtl_font.empty()) {
        MESSAGE("skipping Hebrew middle-bracket normalization smoke: configure "
                "test RTL font");
        return;
    }

    const auto logical_prefix_text = hebrew_shalom_logical();
    const auto visual_prefix_text = hebrew_shalom_visual();
    const auto logical_inner_text = hebrew_bedika_logical();
    const auto visual_inner_text = hebrew_bedika_visual();
    const auto logical_text =
        logical_prefix_text + " (" + logical_inner_text + ")";
    const auto visual_ltr_pair_text =
        std::string{"("} + visual_inner_text + ") " + visual_prefix_text;
    const auto visual_mirrored_pair_text =
        std::string{")"} + visual_inner_text + "( " + visual_prefix_text;
    const auto incorrect_reversed_pair =
        logical_prefix_text + " )" + logical_inner_text + "(";

    const std::array<RtlActualTextNormalizationCase, 2U> variants{{
        {"middle-ltr-pair",
         logical_text,
         visual_ltr_pair_text,
         {visual_prefix_text, visual_inner_text},
         {incorrect_reversed_pair}},
        {"middle-mirrored-pair",
         logical_text,
         visual_mirrored_pair_text,
         {visual_prefix_text, visual_inner_text},
         {incorrect_reversed_pair}},
    }};

    for (const auto &variant : variants) {
        check_hebrew_actual_text_normalization_case(rtl_font, "bracket",
                                                    variant);
    }
}

TEST_CASE("PDFium RTL normalization preserves quoted RTL text") {
    const auto rtl_font = find_rtl_font();
    if (rtl_font.empty()) {
        MESSAGE("skipping Hebrew quote normalization smoke: configure test RTL "
                "font");
        return;
    }

    const auto logical_text = hebrew_shalom_logical();
    const auto visual_text = hebrew_shalom_visual();
    const auto logical_inner_text = hebrew_bedika_logical();
    const auto visual_inner_text = hebrew_bedika_visual();

    const std::array<RtlActualTextNormalizationCase, 4U> variants{{
        {"outer-double-quote",
         "\"" + logical_text + "\"",
         "\"" + visual_text + "\"",
         {visual_text},
         {"\"" + visual_text + "\""}},
        {"middle-double-quote",
         logical_text + " \"" + logical_inner_text + "\"",
         "\"" + visual_inner_text + "\" " + visual_text,
         {visual_text},
         {logical_text + " \"" + visual_inner_text + "\""}},
        {"outer-single-quote",
         "'" + logical_text + "'",
         "'" + visual_text + "'",
         {visual_text},
         {"'" + visual_text + "'"}},
        {"middle-single-quote",
         logical_text + " '" + logical_inner_text + "'",
         "'" + visual_inner_text + "' " + visual_text,
         {visual_text},
         {logical_text + " '" + visual_inner_text + "'"}},
    }};

    for (const auto &variant : variants) {
        check_hebrew_actual_text_normalization_case(rtl_font, "quote",
                                                    variant);
    }
}

TEST_CASE("PDFium RTL normalization preserves short neutral runs") {
    const auto rtl_font = find_rtl_font();
    if (rtl_font.empty()) {
        MESSAGE("skipping Hebrew neutral-run normalization smoke: configure "
                "test RTL font");
        return;
    }

    const auto logical_left_text = hebrew_shalom_logical();
    const auto visual_left_text = hebrew_shalom_visual();
    const auto logical_right_text = hebrew_bedika_logical();
    const auto visual_right_text = hebrew_bedika_visual();

    const std::array<RtlActualTextNormalizationCase, 6U> variants{{
        {"hyphen-separator",
         logical_left_text + " - " + logical_right_text,
         visual_right_text + " - " + visual_left_text,
         {visual_left_text, visual_right_text},
         {visual_right_text + " - " + visual_left_text}},
        {"slash-separator",
         logical_left_text + "/" + logical_right_text,
         visual_right_text + "/" + visual_left_text,
         {visual_left_text, visual_right_text},
         {visual_right_text + "/" + visual_left_text}},
        {"colon-separator",
         logical_left_text + ": " + logical_right_text,
         visual_right_text + " :" + visual_left_text,
         {visual_left_text, visual_right_text},
         {visual_right_text + " :" + visual_left_text}},
        {"semicolon-separator",
         logical_left_text + "; " + logical_right_text,
         visual_right_text + " ;" + visual_left_text,
         {visual_left_text, visual_right_text},
         {visual_right_text + " ;" + visual_left_text}},
        {"comma-separator",
         logical_left_text + ", " + logical_right_text,
         visual_right_text + " ," + visual_left_text,
         {visual_left_text, visual_right_text},
         {visual_right_text + " ," + visual_left_text}},
        {"period-separator",
         logical_left_text + ". " + logical_right_text,
         visual_right_text + " ." + visual_left_text,
         {visual_left_text, visual_right_text},
         {visual_right_text + " ." + visual_left_text}},
    }};

    for (const auto &variant : variants) {
        check_hebrew_actual_text_normalization_case(rtl_font, "neutral",
                                                    variant);
    }
}

TEST_CASE("PDFium RTL normalization leaves Hebrew combining marks raw") {
    const auto rtl_font = find_rtl_font();
    if (rtl_font.empty()) {
        MESSAGE("skipping Hebrew combining-mark normalization smoke: configure "
                "test RTL font");
        return;
    }

    const auto logical_text =
        utf8_from_u8(u8"\u05E9\u05B8\u05DC\u05D5\u05B9\u05DD");
    const auto qamats = utf8_from_u8(u8"\u05B8");
    const auto holam = utf8_from_u8(u8"\u05B9");
    const auto visual_order_text =
        utf8_from_u8(u8"\u05DD\u05D5\u05B9\u05DC\u05E9\u05B8");
    const auto pdfium_raw_text =
        utf8_from_u8(u8"\u05DD\u05B9\u05D5\u05DC\u05B8\u05E9");

    check_rtl_actual_text_raw_preservation_case(
        rtl_font,
        "hebrew-rtl",
        RtlActualTextRawPreservationCase{
            "combining-mark",
            logical_text,
            visual_order_text,
            {pdfium_raw_text, qamats, holam},
            {logical_text},
            true});
}

TEST_CASE("PDFium RTL normalization leaves Arabic combining marks raw") {
    const auto rtl_font = find_rtl_font();
    if (rtl_font.empty()) {
        MESSAGE("skipping Arabic diacritic normalization smoke: configure test "
                "RTL font");
        return;
    }

    const auto logical_text = utf8_from_u8(
        u8"\u0633\u064E\u0644\u064E\u0627\u0645");
    const auto visual_order_text = utf8_from_u8(
        u8"\u0645\u0627\u0644\u064E\u0633\u064E");
    const auto fatha = utf8_from_u8(u8"\u064E");
    const auto pdfium_raw_text = utf8_from_u8(
        u8"\u0645\u0627\u064E\u0644\u064E\u0633");

    check_rtl_actual_text_raw_preservation_case(
        rtl_font,
        "arabic-rtl",
        RtlActualTextRawPreservationCase{
            "diacritic-normalization",
            logical_text,
            visual_order_text,
            {pdfium_raw_text, fatha},
            {logical_text},
            true});
}

struct RtlTableCellBoundaryCase {
    std::string owner;
    std::string logical_text;
    std::string raw_text;
};

[[nodiscard]] std::string rtl_table_boundary_due_date(std::size_t index) {
    const auto day = index + 1U;
    return std::string{"2026-06-"} + (day < 10U ? "0" : "") +
           std::to_string(day);
}

void append_rtl_table_cell_boundary_table(
    featherdoc::pdf::PdfDocumentLayout &layout,
    const std::filesystem::path &rtl_font,
    std::string title,
    const std::vector<RtlTableCellBoundaryCase> &cases) {
    layout.metadata.title = std::move(title);
    layout.metadata.creator = "FeatherDoc test";

    featherdoc::pdf::PdfPageLayout page;
    page.size = featherdoc::pdf::PdfPageSize::a4_portrait();
    append_test_grid(page,
                     72.0,
                     700.0,
                     132.0,
                     32.0,
                     3U,
                     static_cast<std::uint32_t>(cases.size() + 1U));
    page.text_runs.push_back(
        make_unicode_text_run(86.0, 678.0, "Item", rtl_font));
    page.text_runs.push_back(
        make_unicode_text_run(218.0, 678.0, "Owner", rtl_font));
    page.text_runs.push_back(
        make_unicode_text_run(350.0, 678.0, "Due", rtl_font));

    for (std::size_t index = 0U; index < cases.size(); ++index) {
        const auto y = 646.0 - static_cast<double>(index) * 32.0;
        const auto due_date = rtl_table_boundary_due_date(index);
        page.text_runs.push_back(
            make_unicode_text_run(86.0, y, cases[index].logical_text, rtl_font));
        page.text_runs.push_back(
            make_unicode_text_run(218.0, y, cases[index].owner, rtl_font));
        page.text_runs.push_back(make_unicode_text_run(
            350.0, y, due_date, rtl_font));
    }

    layout.pages.push_back(std::move(page));
}

const featherdoc::pdf::PdfParsedTableCandidate &require_single_table_candidate(
    const featherdoc::pdf::PdfParseResult &parse_result,
    std::size_t expected_rows) {
    REQUIRE_EQ(parse_result.document.pages.size(), 1U);
    REQUIRE_EQ(parse_result.document.pages.front().table_candidates.size(),
               1U);
    const auto &table =
        parse_result.document.pages.front().table_candidates.front();
    REQUIRE_EQ(table.rows.size(), expected_rows);
    return table;
}

void check_raw_table_boundary_cells(
    const featherdoc::pdf::PdfParsedTableCandidate &table,
    const std::vector<RtlTableCellBoundaryCase> &cases) {
    for (std::size_t index = 0U; index < cases.size(); ++index) {
        const auto row_index = index + 1U;
        INFO("raw RTL table boundary case: " << cases[index].owner);
        const auto &cell = table.rows[row_index].cells[0];
        REQUIRE(cell.has_text);
        INFO("raw RTL table boundary cell text: "
             << cell.text);
        CHECK_NE(cell.text.find(cases[index].raw_text),
                 std::string::npos);
        REQUIRE_FALSE(cell.text_spans.empty());
        CHECK(has_span_sequence_with_geometry(cell.text_spans,
                                              cases[index].raw_text));
    }
}

void check_normalized_table_boundary_cells(
    const featherdoc::pdf::PdfParsedTableCandidate &table,
    const featherdoc::pdf::PdfParsedTableCandidate &raw_table,
    const std::vector<RtlTableCellBoundaryCase> &cases) {
    for (std::size_t index = 0U; index < cases.size(); ++index) {
        const auto row_index = index + 1U;
        INFO("normalized RTL table boundary case: " << cases[index].owner);
        const auto &cell = table.rows[row_index].cells[0];
        const auto &raw_cell = raw_table.rows[row_index].cells[0];
        REQUIRE(cell.has_text);
        REQUIRE(raw_cell.has_text);
        CHECK_EQ(cell.text, cases[index].logical_text);
        CHECK_EQ(cell.text.find(cases[index].raw_text),
                 std::string::npos);
        REQUIRE_FALSE(cell.text_spans.empty());
        CHECK(has_span_sequence_with_geometry(cell.text_spans,
                                              cases[index].raw_text));
        check_raw_text_spans_unchanged(cell.text_spans, raw_cell.text_spans);
    }
}

[[nodiscard]] std::string imported_table_cell_text(
    featherdoc::Document &document,
    std::size_t row_index,
    std::size_t column_index) {
    const auto imported_cell =
        document.inspect_table_cell(0U, row_index, column_index);
    REQUIRE(imported_cell.has_value());
    return imported_cell->text;
}

void check_imported_table_cell_contains_text(featherdoc::Document &document,
                                             std::size_t row_index,
                                             std::size_t column_index,
                                             std::string_view expected_text) {
    REQUIRE_FALSE(expected_text.empty());
    const auto text = imported_table_cell_text(
        document, row_index, column_index);
    CHECK_NE(text.find(std::string{expected_text}), std::string::npos);
}

void check_imported_four_row_table_static_cells(featherdoc::Document &document,
                                                std::string_view first_owner,
                                                std::string_view first_due,
                                                std::string_view second_due,
                                                std::string_view third_due) {
    check_imported_table_cell_contains_text(document, 0U, 0U, "Item");
    check_imported_table_cell_contains_text(document, 0U, 1U, "Owner");
    check_imported_table_cell_contains_text(document, 0U, 2U, "Due");

    check_imported_table_cell_contains_text(document, 1U, 1U, first_owner);
    check_imported_table_cell_contains_text(document, 1U, 2U, first_due);

    check_imported_table_cell_contains_text(document, 2U, 0U, "Alpha");
    check_imported_table_cell_contains_text(document, 2U, 1U, "QA");
    check_imported_table_cell_contains_text(document, 2U, 2U, second_due);

    check_imported_table_cell_contains_text(document, 3U, 0U, "Beta");
    check_imported_table_cell_contains_text(document, 3U, 1U, "Docs");
    check_imported_table_cell_contains_text(document, 3U, 2U, third_due);
}

void check_imported_four_row_table_cells_unchanged(
    featherdoc::Document &normalized_document,
    featherdoc::Document &raw_document) {
    for (std::size_t row_index = 0U; row_index < 4U; ++row_index) {
        INFO("four-row import row index: " << row_index);
        for (std::size_t column_index = 0U; column_index < 3U;
             ++column_index) {
            INFO("four-row import column index: " << column_index);
            CHECK_EQ(imported_table_cell_text(
                         normalized_document, row_index, column_index),
                     imported_table_cell_text(
                         raw_document, row_index, column_index));
        }
    }
}

void check_imported_four_row_table_cells_unchanged_except(
    featherdoc::Document &normalized_document,
    featherdoc::Document &raw_document,
    std::size_t except_row_index,
    std::size_t except_column_index) {
    for (std::size_t row_index = 0U; row_index < 4U; ++row_index) {
        INFO("four-row import row index: " << row_index);
        for (std::size_t column_index = 0U; column_index < 3U;
             ++column_index) {
            INFO("four-row import column index: " << column_index);
            if (row_index == except_row_index &&
                column_index == except_column_index) {
                continue;
            }
            CHECK_EQ(imported_table_cell_text(
                         normalized_document, row_index, column_index),
                     imported_table_cell_text(
                         raw_document, row_index, column_index));
        }
    }
}

void check_imported_table_boundary_cells(
    featherdoc::Document &document,
    const std::vector<RtlTableCellBoundaryCase> &cases) {
    for (std::size_t index = 0U; index < cases.size(); ++index) {
        INFO("imported RTL table boundary case: " << cases[index].owner);
        const auto imported_cell =
            document.inspect_table_cell(0U, index + 1U, 0U);
        REQUIRE(imported_cell.has_value());
        CHECK_EQ(imported_cell->text, cases[index].logical_text);
        CHECK_EQ(imported_cell->text.find(cases[index].raw_text),
                 std::string::npos);
    }
}

void check_imported_table_static_boundary_cells(
    featherdoc::Document &document,
    const std::vector<RtlTableCellBoundaryCase> &cases) {
    check_imported_table_cell_contains_text(document, 0U, 0U, "Item");
    check_imported_table_cell_contains_text(document, 0U, 1U, "Owner");
    check_imported_table_cell_contains_text(document, 0U, 2U, "Due");

    for (std::size_t index = 0U; index < cases.size(); ++index) {
        INFO("static imported RTL table boundary case: "
             << cases[index].owner);
        const auto row_index = index + 1U;
        check_imported_table_cell_contains_text(
            document, row_index, 1U, cases[index].owner);
        check_imported_table_cell_contains_text(
            document, row_index, 2U, rtl_table_boundary_due_date(index));
    }
}

void check_imported_table_static_boundary_cells_unchanged(
    featherdoc::Document &normalized_document,
    featherdoc::Document &raw_document,
    const std::vector<RtlTableCellBoundaryCase> &cases) {
    for (const auto [row_index, column_index] :
         {std::pair<std::size_t, std::size_t>{0U, 0U},
          std::pair<std::size_t, std::size_t>{0U, 1U},
          std::pair<std::size_t, std::size_t>{0U, 2U}}) {
        INFO("static imported header row index: " << row_index);
        INFO("static imported header column index: " << column_index);
        CHECK_EQ(imported_table_cell_text(
                     normalized_document, row_index, column_index),
                 imported_table_cell_text(
                     raw_document, row_index, column_index));
    }

    for (std::size_t index = 0U; index < cases.size(); ++index) {
        INFO("unchanged imported RTL table boundary case: "
             << cases[index].owner);
        const auto row_index = index + 1U;
        for (const auto column_index : {1U, 2U}) {
            INFO("static imported column index: " << column_index);
            CHECK_EQ(imported_table_cell_text(
                         normalized_document, row_index, column_index),
                     imported_table_cell_text(
                         raw_document, row_index, column_index));
        }
    }
}

void check_raw_imported_table_boundary_cells(
    featherdoc::Document &document,
    const std::vector<RtlTableCellBoundaryCase> &cases) {
    for (std::size_t index = 0U; index < cases.size(); ++index) {
        INFO("raw imported RTL table boundary case: " << cases[index].owner);
        const auto imported_cell =
            document.inspect_table_cell(0U, index + 1U, 0U);
        REQUIRE(imported_cell.has_value());
        CHECK_NE(imported_cell->text.find(cases[index].raw_text),
                 std::string::npos);
        CHECK_EQ(imported_cell->text.find(cases[index].logical_text),
                 std::string::npos);
    }
}

TEST_CASE("PDFium RTL normalization applies to pure RTL table cells") {
    const auto rtl_font = find_rtl_font();
    if (rtl_font.empty()) {
        MESSAGE("skipping Hebrew table cell normalization smoke: configure "
                "test RTL font");
        return;
    }

    const auto logical_text = hebrew_shalom_logical();
    const auto visual_order_text = hebrew_shalom_visual();
    const auto arabic_indic_digits = utf8_from_u8(u8"١٢٣");
    const auto arabic_logical_text = arabic_salam_logical();
    const auto arabic_visual_order_text = arabic_salam_visual();
    const std::vector<RtlTableCellBoundaryCase> cases = {
        RtlTableCellBoundaryCase{"QA Team", logical_text, visual_order_text},
        RtlTableCellBoundaryCase{
            "Ops",
            logical_text + " 123",
            std::string{"123 "} + visual_order_text},
        RtlTableCellBoundaryCase{
            "Support",
            logical_text + " " + arabic_indic_digits,
            arabic_indic_digits + " " + visual_order_text},
        RtlTableCellBoundaryCase{
            "Review",
            logical_text + ": 123",
            std::string{"123 :"} + visual_order_text},
        RtlTableCellBoundaryCase{
            "Audit",
            logical_text + "/123",
            std::string{"/123"} + visual_order_text},
        RtlTableCellBoundaryCase{
            "Release",
            logical_text + "-123",
            std::string{"-123"} + visual_order_text},
        RtlTableCellBoundaryCase{
            "Legal",
            logical_text + ";123",
            std::string{"123;"} + visual_order_text},
        RtlTableCellBoundaryCase{
            "Docs",
            logical_text + ",123",
            std::string{",123"} + visual_order_text},
        RtlTableCellBoundaryCase{
            "Archive",
            logical_text + ".123",
            std::string{".123"} + visual_order_text},
        RtlTableCellBoundaryCase{
            "Locale",
            arabic_logical_text,
            arabic_visual_order_text},
        RtlTableCellBoundaryCase{
            "Metrics",
            arabic_logical_text + " 123",
            std::string{"123 "} + arabic_visual_order_text},
        RtlTableCellBoundaryCase{
            "Intl",
            logical_text + "/" + arabic_indic_digits,
            std::string{"/"} + arabic_indic_digits + visual_order_text},
        RtlTableCellBoundaryCase{
            "Export",
            logical_text + ";" + arabic_indic_digits,
            arabic_indic_digits + ";" + visual_order_text},
        RtlTableCellBoundaryCase{
            "Locale QA",
            logical_text + ": " + arabic_indic_digits,
            visual_order_text + ": " + arabic_indic_digits},
        RtlTableCellBoundaryCase{
            "Fallback",
            logical_text + "-" + arabic_indic_digits,
            std::string{"-"} + arabic_indic_digits + visual_order_text},
        RtlTableCellBoundaryCase{
            "Comma AN",
            logical_text + "," + arabic_indic_digits,
            visual_order_text + "," + arabic_indic_digits},
        RtlTableCellBoundaryCase{
            "Period AN",
            logical_text + "." + arabic_indic_digits,
            visual_order_text + "." + arabic_indic_digits},
    };
    const auto expected_hebrew_raw_count = static_cast<std::size_t>(
        std::count_if(cases.begin(),
                      cases.end(),
                      [&](const RtlTableCellBoundaryCase &boundary_case) {
                          return boundary_case.raw_text.find(
                                     visual_order_text) != std::string::npos;
                      }));
    const auto expected_arabic_raw_count = static_cast<std::size_t>(
        std::count_if(cases.begin(),
                      cases.end(),
                      [&](const RtlTableCellBoundaryCase &boundary_case) {
                          return boundary_case.raw_text.find(
                                     arabic_visual_order_text) !=
                                 std::string::npos;
                      }));

    featherdoc::pdf::PdfDocumentLayout layout;
    append_rtl_table_cell_boundary_table(
        layout,
        rtl_font,
        "FeatherDoc Hebrew table cell normalization",
        cases);

    const auto output_path =
        std::filesystem::current_path() /
        "featherdoc-hebrew-rtl-table-cell-boundary.pdf";

    featherdoc::pdf::PdfioGenerator generator;
    const auto write_result =
        generator.write(layout, output_path, featherdoc::pdf::PdfWriterOptions{});
    REQUIRE_MESSAGE(write_result.success, write_result.error_message);

    featherdoc::pdf::PdfiumParser parser;
    featherdoc::pdf::PdfParseOptions raw_options;
    raw_options.normalize_rtl_text = false;
    const auto raw_parse_result = parser.parse(output_path, raw_options);
    REQUIRE_MESSAGE(raw_parse_result.success, raw_parse_result.error_message);
    const auto raw_span_text = collect_text(raw_parse_result.document);
    CHECK_EQ(count_occurrences(raw_span_text, logical_text), 0U);
    CHECK_EQ(count_occurrences(raw_span_text, visual_order_text),
             expected_hebrew_raw_count);
    CHECK_EQ(count_occurrences(raw_span_text, arabic_logical_text), 0U);
    CHECK_EQ(count_occurrences(raw_span_text, arabic_visual_order_text),
             expected_arabic_raw_count);
    const auto raw_line_text = collect_line_text(raw_parse_result.document);
    CHECK_EQ(count_occurrences(raw_line_text, logical_text), 0U);
    CHECK_EQ(count_occurrences(raw_line_text, visual_order_text),
             expected_hebrew_raw_count);
    CHECK_EQ(count_occurrences(raw_line_text, arabic_logical_text), 0U);
    CHECK_EQ(count_occurrences(raw_line_text, arabic_visual_order_text),
             expected_arabic_raw_count);
    const auto &raw_table =
        require_single_table_candidate(raw_parse_result, cases.size() + 1U);
    check_raw_table_boundary_cells(raw_table, cases);

    const auto normalized_parse_result = parser.parse(output_path, {});
    REQUIRE_MESSAGE(normalized_parse_result.success,
                    normalized_parse_result.error_message);
    const auto normalized_span_text =
        collect_text(normalized_parse_result.document);
    CHECK_EQ(count_occurrences(normalized_span_text, logical_text), 0U);
    CHECK_EQ(count_occurrences(normalized_span_text, visual_order_text),
             expected_hebrew_raw_count);
    CHECK_EQ(count_occurrences(normalized_span_text, arabic_logical_text), 0U);
    CHECK_EQ(count_occurrences(normalized_span_text, arabic_visual_order_text),
             expected_arabic_raw_count);
    const auto normalized_line_text =
        collect_line_text(normalized_parse_result.document);
    CHECK_EQ(count_occurrences(normalized_line_text, logical_text), 0U);
    CHECK_EQ(count_occurrences(normalized_line_text, visual_order_text),
             expected_hebrew_raw_count);
    CHECK_EQ(count_occurrences(normalized_line_text, arabic_logical_text), 1U);
    CHECK_EQ(count_occurrences(normalized_line_text, arabic_visual_order_text),
             1U);
    const auto &normalized_table =
        require_single_table_candidate(normalized_parse_result,
                                       cases.size() + 1U);
    check_table_candidate_geometry_unchanged(normalized_table, raw_table);
    check_table_cell_text_unchanged_except_column_rows(
        normalized_table, raw_table, 0U, 1U, cases.size());
    check_normalized_table_boundary_cells(normalized_table, raw_table, cases);

    const auto imported_docx_path =
        std::filesystem::current_path() /
        "featherdoc-hebrew-rtl-table-cell-boundary-import.docx";
    std::filesystem::remove(imported_docx_path);

    featherdoc::Document imported_document(imported_docx_path);
    featherdoc::pdf::PdfDocumentImportOptions import_options;
    import_options.import_table_candidates_as_tables = true;
    const auto import_result = featherdoc::pdf::import_pdf_text_document(
        output_path, imported_document, import_options);
    REQUIRE_MESSAGE(import_result.success, import_result.error_message);
    CHECK_EQ(import_result.tables_imported, 1U);
    check_imported_table_boundary_cells(imported_document, cases);
    check_imported_table_static_boundary_cells(imported_document, cases);

    const auto raw_imported_docx_path =
        std::filesystem::current_path() /
        "featherdoc-hebrew-rtl-table-cell-boundary-raw-import.docx";
    std::filesystem::remove(raw_imported_docx_path);

    featherdoc::Document raw_imported_document(raw_imported_docx_path);
    featherdoc::pdf::PdfDocumentImportOptions raw_import_options;
    raw_import_options.import_table_candidates_as_tables = true;
    raw_import_options.parse_options.normalize_rtl_text = false;
    const auto raw_import_result = featherdoc::pdf::import_pdf_text_document(
        output_path, raw_imported_document, raw_import_options);
    REQUIRE_MESSAGE(raw_import_result.success,
                    raw_import_result.error_message);
    CHECK_EQ(raw_import_result.tables_imported, 1U);
    check_raw_imported_table_boundary_cells(raw_imported_document, cases);
    check_imported_table_static_boundary_cells(raw_imported_document, cases);
    check_imported_table_static_boundary_cells_unchanged(
        imported_document, raw_imported_document, cases);
}

TEST_CASE("PDFium RTL normalization covers Extended Arabic-Indic table cell "
          "digits") {
    const auto rtl_font = find_rtl_font();
    if (rtl_font.empty()) {
        MESSAGE("skipping Persian digit table cell normalization smoke: "
                "configure test RTL font");
        return;
    }

    const auto logical_text = hebrew_shalom_logical();
    const auto visual_order_text = hebrew_shalom_visual();
    const auto persian_digits = utf8_from_u8(u8"\u06F1\u06F2\u06F3");
    const std::vector<RtlTableCellBoundaryCase> cases = {
        RtlTableCellBoundaryCase{
            "P Space",
            logical_text + " " + persian_digits,
            visual_order_text + " " + persian_digits + " "},
        RtlTableCellBoundaryCase{
            "P Slash",
            logical_text + "/" + persian_digits,
            visual_order_text + "/" + persian_digits + " "},
        RtlTableCellBoundaryCase{
            "P Semi",
            logical_text + ";" + persian_digits,
            visual_order_text + ";" + persian_digits + " "},
        RtlTableCellBoundaryCase{
            "P Colon",
            logical_text + ": " + persian_digits,
            visual_order_text + ": " + persian_digits},
        RtlTableCellBoundaryCase{
            "P Hyph",
            logical_text + "-" + persian_digits,
            visual_order_text + "-" + persian_digits + " "},
        RtlTableCellBoundaryCase{
            "P Comma",
            logical_text + "," + persian_digits,
            visual_order_text + "," + persian_digits + " "},
        RtlTableCellBoundaryCase{
            "P Dot",
            logical_text + "." + persian_digits,
            visual_order_text + "." + persian_digits + " "},
    };

    featherdoc::pdf::PdfDocumentLayout layout;
    append_rtl_table_cell_boundary_table(
        layout,
        rtl_font,
        "FeatherDoc Hebrew table cell Persian digit normalization",
        cases);

    const auto output_path =
        std::filesystem::current_path() /
        "featherdoc-hebrew-rtl-table-cell-persian-digit-boundary.pdf";

    featherdoc::pdf::PdfioGenerator generator;
    const auto write_result =
        generator.write(layout, output_path, featherdoc::pdf::PdfWriterOptions{});
    REQUIRE_MESSAGE(write_result.success, write_result.error_message);

    featherdoc::pdf::PdfiumParser parser;
    featherdoc::pdf::PdfParseOptions raw_options;
    raw_options.normalize_rtl_text = false;
    const auto raw_parse_result = parser.parse(output_path, raw_options);
    REQUIRE_MESSAGE(raw_parse_result.success, raw_parse_result.error_message);
    const auto raw_span_text = collect_text(raw_parse_result.document);
    CHECK_EQ(count_occurrences(raw_span_text, logical_text), 0U);
    CHECK_EQ(count_occurrences(raw_span_text, visual_order_text),
             cases.size());
    const auto raw_line_text = collect_line_text(raw_parse_result.document);
    CHECK_EQ(count_occurrences(raw_line_text, logical_text), 0U);
    CHECK_EQ(count_occurrences(raw_line_text, visual_order_text),
             cases.size());
    const auto &raw_table =
        require_single_table_candidate(raw_parse_result, cases.size() + 1U);
    check_raw_table_boundary_cells(raw_table, cases);

    const auto normalized_parse_result = parser.parse(output_path, {});
    REQUIRE_MESSAGE(normalized_parse_result.success,
                    normalized_parse_result.error_message);
    const auto normalized_span_text =
        collect_text(normalized_parse_result.document);
    CHECK_EQ(count_occurrences(normalized_span_text, logical_text), 0U);
    CHECK_EQ(count_occurrences(normalized_span_text, visual_order_text),
             cases.size());
    const auto normalized_line_text =
        collect_line_text(normalized_parse_result.document);
    CHECK_EQ(count_occurrences(normalized_line_text, logical_text), 0U);
    CHECK_EQ(count_occurrences(normalized_line_text, visual_order_text),
             cases.size());
    const auto &normalized_table =
        require_single_table_candidate(normalized_parse_result,
                                       cases.size() + 1U);
    check_table_candidate_geometry_unchanged(normalized_table, raw_table);
    check_table_cell_text_unchanged_except_column_rows(
        normalized_table, raw_table, 0U, 1U, cases.size());
    check_normalized_table_boundary_cells(normalized_table, raw_table, cases);

    const auto imported_docx_path =
        std::filesystem::current_path() /
        "featherdoc-hebrew-rtl-table-cell-persian-digit-boundary-import.docx";
    std::filesystem::remove(imported_docx_path);

    featherdoc::Document imported_document(imported_docx_path);
    featherdoc::pdf::PdfDocumentImportOptions import_options;
    import_options.import_table_candidates_as_tables = true;
    const auto import_result = featherdoc::pdf::import_pdf_text_document(
        output_path, imported_document, import_options);
    REQUIRE_MESSAGE(import_result.success, import_result.error_message);
    CHECK_EQ(import_result.tables_imported, 1U);
    check_imported_table_boundary_cells(imported_document, cases);
    check_imported_table_static_boundary_cells(imported_document, cases);

    const auto raw_imported_docx_path =
        std::filesystem::current_path() /
        "featherdoc-hebrew-rtl-table-cell-persian-digit-boundary-raw-import.docx";
    std::filesystem::remove(raw_imported_docx_path);

    featherdoc::Document raw_imported_document(raw_imported_docx_path);
    featherdoc::pdf::PdfDocumentImportOptions raw_import_options;
    raw_import_options.import_table_candidates_as_tables = true;
    raw_import_options.parse_options.normalize_rtl_text = false;
    const auto raw_import_result = featherdoc::pdf::import_pdf_text_document(
        output_path, raw_imported_document, raw_import_options);
    REQUIRE_MESSAGE(raw_import_result.success,
                    raw_import_result.error_message);
    CHECK_EQ(raw_import_result.tables_imported, 1U);
    check_raw_imported_table_boundary_cells(raw_imported_document, cases);
    check_imported_table_static_boundary_cells(raw_imported_document, cases);
    check_imported_table_static_boundary_cells_unchanged(
        imported_document, raw_imported_document, cases);
}

TEST_CASE("PDFium RTL table cell raw spans preserve split mixed bidi runs") {
    const auto rtl_font = find_rtl_font();
    if (rtl_font.empty()) {
        MESSAGE("skipping Hebrew split mixed-bidi table cell smoke: configure "
                "test RTL font");
        return;
    }

    const auto logical_rtl_text = hebrew_shalom_logical();
    const auto visual_rtl_text = hebrew_shalom_visual();
    const auto logical_target_text =
        std::string{"Status "} + logical_rtl_text + ": 123";

    featherdoc::pdf::PdfDocumentLayout layout;
    layout.metadata.title = "FeatherDoc split mixed bidi table cell";
    layout.metadata.creator = "FeatherDoc test";

    featherdoc::pdf::PdfPageLayout page;
    page.size = featherdoc::pdf::PdfPageSize::a4_portrait();
    append_test_grid(page, 72.0, 700.0, 132.0, 32.0, 3U, 4U);
    page.text_runs.push_back(
        make_unicode_text_run(86.0, 678.0, "Item", rtl_font));
    page.text_runs.push_back(
        make_unicode_text_run(218.0, 678.0, "Owner", rtl_font));
    page.text_runs.push_back(
        make_unicode_text_run(350.0, 678.0, "Due", rtl_font));

    page.text_runs.push_back(
        make_unicode_text_run(80.0, 646.0, "Status ", rtl_font));
    page.text_runs.push_back(
        make_unicode_text_run(122.0, 646.0, logical_rtl_text, rtl_font));
    page.text_runs.push_back(
        make_unicode_text_run(154.0, 646.0, ": 123", rtl_font));
    page.text_runs.push_back(
        make_unicode_text_run(218.0, 646.0, "Ops", rtl_font));
    page.text_runs.push_back(
        make_unicode_text_run(350.0, 646.0, "2026-06-21", rtl_font));

    page.text_runs.push_back(
        make_unicode_text_run(86.0, 614.0, "Alpha", rtl_font));
    page.text_runs.push_back(
        make_unicode_text_run(218.0, 614.0, "QA", rtl_font));
    page.text_runs.push_back(
        make_unicode_text_run(350.0, 614.0, "2026-06-22", rtl_font));

    page.text_runs.push_back(
        make_unicode_text_run(86.0, 582.0, "Beta", rtl_font));
    page.text_runs.push_back(
        make_unicode_text_run(218.0, 582.0, "Docs", rtl_font));
    page.text_runs.push_back(
        make_unicode_text_run(350.0, 582.0, "2026-06-23", rtl_font));
    layout.pages.push_back(std::move(page));

    const auto output_path =
        std::filesystem::current_path() /
        "featherdoc-hebrew-rtl-table-cell-split-mixed-bidi-boundary.pdf";

    featherdoc::pdf::PdfioGenerator generator;
    const auto write_result =
        generator.write(layout, output_path, featherdoc::pdf::PdfWriterOptions{});
    REQUIRE_MESSAGE(write_result.success, write_result.error_message);

    featherdoc::pdf::PdfiumParser parser;
    featherdoc::pdf::PdfParseOptions raw_options;
    raw_options.normalize_rtl_text = false;
    const auto raw_parse_result = parser.parse(output_path, raw_options);
    REQUIRE_MESSAGE(raw_parse_result.success, raw_parse_result.error_message);
    const auto &raw_table = require_single_table_candidate(raw_parse_result, 4U);
    REQUIRE(raw_table.rows[1].cells[0].has_text);
    const auto &raw_target_cell = raw_table.rows[1].cells[0];
    INFO("raw split mixed-bidi table cell text: " << raw_target_cell.text);
    CHECK_EQ(count_occurrences(raw_target_cell.text, logical_target_text), 0U);
    CHECK_EQ(count_occurrences(raw_target_cell.text, "Status"), 1U);
    CHECK_EQ(count_occurrences(raw_target_cell.text, visual_rtl_text), 1U);
    CHECK_EQ(count_occurrences(raw_target_cell.text, "123"), 1U);
    REQUIRE_FALSE(raw_target_cell.text_spans.empty());
    CHECK(has_span_sequence_with_geometry(raw_target_cell.text_spans,
                                          "Status"));
    CHECK(has_span_sequence_with_geometry(raw_target_cell.text_spans,
                                          visual_rtl_text));
    CHECK(has_span_sequence_with_geometry(raw_target_cell.text_spans, "123"));
    check_span_sequences_appear_in_order(
        raw_target_cell.text_spans, {"Status", visual_rtl_text, "123"});

    const auto normalized_parse_result = parser.parse(output_path, {});
    REQUIRE_MESSAGE(normalized_parse_result.success,
                    normalized_parse_result.error_message);
    const auto &normalized_table =
        require_single_table_candidate(normalized_parse_result, 4U);
    check_table_candidate_geometry_unchanged(normalized_table, raw_table);
    check_table_cell_text_unchanged_except_column_rows(
        normalized_table, raw_table, 0U, 0U, 0U);
    REQUIRE(normalized_table.rows[1].cells[0].has_text);
    const auto &normalized_target_cell = normalized_table.rows[1].cells[0];
    CHECK_EQ(normalized_target_cell.text, raw_target_cell.text);
    CHECK_EQ(count_occurrences(normalized_target_cell.text,
                               logical_target_text),
             0U);
    REQUIRE_FALSE(normalized_target_cell.text_spans.empty());
    CHECK(has_span_sequence_with_geometry(normalized_target_cell.text_spans,
                                          "Status"));
    CHECK(has_span_sequence_with_geometry(normalized_target_cell.text_spans,
                                          visual_rtl_text));
    CHECK(has_span_sequence_with_geometry(normalized_target_cell.text_spans,
                                          "123"));
    check_span_sequences_appear_in_order(normalized_target_cell.text_spans,
                                         {"Status",
                                          visual_rtl_text,
                                          "123"});
    check_raw_text_spans_unchanged(normalized_target_cell.text_spans,
                                   raw_target_cell.text_spans);

    const auto imported_docx_path =
        std::filesystem::current_path() /
        "featherdoc-hebrew-rtl-table-cell-split-mixed-bidi-boundary-import.docx";
    std::filesystem::remove(imported_docx_path);

    featherdoc::Document imported_document(imported_docx_path);
    featherdoc::pdf::PdfDocumentImportOptions import_options;
    import_options.import_table_candidates_as_tables = true;
    const auto import_result = featherdoc::pdf::import_pdf_text_document(
        output_path, imported_document, import_options);
    REQUIRE_MESSAGE(import_result.success, import_result.error_message);
    CHECK_EQ(import_result.tables_imported, 1U);
    const auto imported_cell = imported_document.inspect_table_cell(0U, 1U, 0U);
    REQUIRE(imported_cell.has_value());
    CHECK_EQ(imported_cell->text, normalized_target_cell.text);
    check_imported_four_row_table_static_cells(
        imported_document, "Ops", "2026-06-21", "2026-06-22", "2026-06-23");

    const auto raw_imported_docx_path =
        std::filesystem::current_path() /
        "featherdoc-hebrew-rtl-table-cell-split-mixed-bidi-boundary-raw-import.docx";
    std::filesystem::remove(raw_imported_docx_path);

    featherdoc::Document raw_imported_document(raw_imported_docx_path);
    featherdoc::pdf::PdfDocumentImportOptions raw_import_options;
    raw_import_options.import_table_candidates_as_tables = true;
    raw_import_options.parse_options.normalize_rtl_text = false;
    const auto raw_import_result = featherdoc::pdf::import_pdf_text_document(
        output_path, raw_imported_document, raw_import_options);
    REQUIRE_MESSAGE(raw_import_result.success,
                    raw_import_result.error_message);
    CHECK_EQ(raw_import_result.tables_imported, 1U);
    const auto raw_imported_cell =
        raw_imported_document.inspect_table_cell(0U, 1U, 0U);
    REQUIRE(raw_imported_cell.has_value());
    CHECK_EQ(raw_imported_cell->text, raw_target_cell.text);
    check_imported_four_row_table_static_cells(
        raw_imported_document,
        "Ops",
        "2026-06-21",
        "2026-06-22",
        "2026-06-23");
    check_imported_four_row_table_cells_unchanged(
        imported_document, raw_imported_document);
}

TEST_CASE("PDFium RTL table cell normalization leaves split pure RTL runs raw") {
    const auto rtl_font = find_rtl_font();
    if (rtl_font.empty()) {
        MESSAGE("skipping Hebrew split pure-RTL table cell smoke: configure "
                "test RTL font");
        return;
    }

    const auto logical_prefix = utf8_from_u8(u8"\u05E9\u05DC");
    const auto logical_suffix = utf8_from_u8(u8"\u05D5\u05DD");
    const auto visual_prefix = utf8_from_u8(u8"\u05DC\u05E9");
    const auto visual_suffix = utf8_from_u8(u8"\u05DD\u05D5");
    const auto logical_target_text = logical_prefix + logical_suffix;
    const auto pdfium_raw_text = visual_prefix + "\n" + visual_suffix;

    featherdoc::pdf::PdfDocumentLayout layout;
    layout.metadata.title = "FeatherDoc split pure RTL table cell";
    layout.metadata.creator = "FeatherDoc test";

    featherdoc::pdf::PdfPageLayout page;
    page.size = featherdoc::pdf::PdfPageSize::a4_portrait();
    append_test_grid(page, 72.0, 700.0, 132.0, 32.0, 3U, 4U);
    page.text_runs.push_back(
        make_unicode_text_run(86.0, 678.0, "Item", rtl_font));
    page.text_runs.push_back(
        make_unicode_text_run(218.0, 678.0, "Owner", rtl_font));
    page.text_runs.push_back(
        make_unicode_text_run(350.0, 678.0, "Due", rtl_font));

    page.text_runs.push_back(
        make_unicode_text_run(86.0, 646.0, logical_prefix, rtl_font));
    page.text_runs.push_back(
        make_unicode_text_run(104.0, 646.0, logical_suffix, rtl_font));
    page.text_runs.push_back(
        make_unicode_text_run(218.0, 646.0, "Split", rtl_font));
    page.text_runs.push_back(
        make_unicode_text_run(350.0, 646.0, "2026-06-30", rtl_font));

    page.text_runs.push_back(
        make_unicode_text_run(86.0, 614.0, "Alpha", rtl_font));
    page.text_runs.push_back(
        make_unicode_text_run(218.0, 614.0, "QA", rtl_font));
    page.text_runs.push_back(
        make_unicode_text_run(350.0, 614.0, "2026-07-01", rtl_font));

    page.text_runs.push_back(
        make_unicode_text_run(86.0, 582.0, "Beta", rtl_font));
    page.text_runs.push_back(
        make_unicode_text_run(218.0, 582.0, "Docs", rtl_font));
    page.text_runs.push_back(
        make_unicode_text_run(350.0, 582.0, "2026-07-02", rtl_font));
    layout.pages.push_back(std::move(page));

    const auto output_path =
        std::filesystem::current_path() /
        "featherdoc-hebrew-rtl-table-cell-split-pure-rtl-boundary.pdf";

    featherdoc::pdf::PdfioGenerator generator;
    const auto write_result =
        generator.write(layout, output_path, featherdoc::pdf::PdfWriterOptions{});
    REQUIRE_MESSAGE(write_result.success, write_result.error_message);

    featherdoc::pdf::PdfiumParser parser;
    featherdoc::pdf::PdfParseOptions raw_options;
    raw_options.normalize_rtl_text = false;
    const auto raw_parse_result = parser.parse(output_path, raw_options);
    REQUIRE_MESSAGE(raw_parse_result.success, raw_parse_result.error_message);
    const auto &raw_table = require_single_table_candidate(raw_parse_result, 4U);
    REQUIRE(raw_table.rows[1].cells[0].has_text);
    const auto &raw_target_cell = raw_table.rows[1].cells[0];
    INFO("raw split pure-RTL table cell text: " << raw_target_cell.text);
    CHECK_EQ(raw_target_cell.text, pdfium_raw_text);
    CHECK_EQ(count_occurrences(raw_target_cell.text, logical_target_text), 0U);
    CHECK_EQ(count_occurrences(raw_target_cell.text, visual_prefix), 1U);
    CHECK_EQ(count_occurrences(raw_target_cell.text, visual_suffix), 1U);
    REQUIRE_FALSE(raw_target_cell.text_spans.empty());
    check_span_sequences_appear_in_order(raw_target_cell.text_spans,
                                         {visual_prefix, visual_suffix});

    const auto normalized_parse_result = parser.parse(output_path, {});
    REQUIRE_MESSAGE(normalized_parse_result.success,
                    normalized_parse_result.error_message);
    const auto &normalized_table =
        require_single_table_candidate(normalized_parse_result, 4U);
    check_table_candidate_geometry_unchanged(normalized_table, raw_table);
    check_table_cell_text_unchanged_except_column_rows(
        normalized_table, raw_table, 0U, 0U, 0U);
    REQUIRE(normalized_table.rows[1].cells[0].has_text);
    const auto &normalized_target_cell = normalized_table.rows[1].cells[0];
    CHECK_EQ(normalized_target_cell.text, raw_target_cell.text);
    check_span_sequences_appear_in_order(normalized_target_cell.text_spans,
                                         {visual_prefix, visual_suffix});
    check_raw_text_spans_unchanged(normalized_target_cell.text_spans,
                                   raw_target_cell.text_spans);

    const auto imported_docx_path =
        std::filesystem::current_path() /
        "featherdoc-hebrew-rtl-table-cell-split-pure-rtl-boundary-import.docx";
    std::filesystem::remove(imported_docx_path);

    featherdoc::Document imported_document(imported_docx_path);
    featherdoc::pdf::PdfDocumentImportOptions import_options;
    import_options.import_table_candidates_as_tables = true;
    const auto import_result = featherdoc::pdf::import_pdf_text_document(
        output_path, imported_document, import_options);
    REQUIRE_MESSAGE(import_result.success, import_result.error_message);
    CHECK_EQ(import_result.tables_imported, 1U);
    const auto imported_cell = imported_document.inspect_table_cell(0U, 1U, 0U);
    REQUIRE(imported_cell.has_value());
    CHECK_EQ(imported_cell->text, raw_target_cell.text);
    check_imported_table_cell_contains_text(
        imported_document, 1U, 0U, visual_prefix);
    check_imported_table_cell_contains_text(
        imported_document, 1U, 0U, visual_suffix);
    check_imported_four_row_table_static_cells(
        imported_document, "Split", "2026-06-30", "2026-07-01", "2026-07-02");

    const auto raw_imported_docx_path =
        std::filesystem::current_path() /
        "featherdoc-hebrew-rtl-table-cell-split-pure-rtl-boundary-raw-import.docx";
    std::filesystem::remove(raw_imported_docx_path);

    featherdoc::Document raw_imported_document(raw_imported_docx_path);
    featherdoc::pdf::PdfDocumentImportOptions raw_import_options;
    raw_import_options.import_table_candidates_as_tables = true;
    raw_import_options.parse_options.normalize_rtl_text = false;
    const auto raw_import_result = featherdoc::pdf::import_pdf_text_document(
        output_path, raw_imported_document, raw_import_options);
    REQUIRE_MESSAGE(raw_import_result.success,
                    raw_import_result.error_message);
    CHECK_EQ(raw_import_result.tables_imported, 1U);
    const auto raw_imported_cell =
        raw_imported_document.inspect_table_cell(0U, 1U, 0U);
    REQUIRE(raw_imported_cell.has_value());
    CHECK_EQ(raw_imported_cell->text, raw_target_cell.text);
    check_imported_table_cell_contains_text(
        raw_imported_document, 1U, 0U, visual_prefix);
    check_imported_table_cell_contains_text(
        raw_imported_document, 1U, 0U, visual_suffix);
    check_imported_four_row_table_static_cells(raw_imported_document,
                                               "Split",
                                               "2026-06-30",
                                               "2026-07-01",
                                               "2026-07-02");
    check_imported_four_row_table_cells_unchanged(
        imported_document, raw_imported_document);
}

TEST_CASE("PDFium RTL table cell normalization applies to Arabic split pure "
          "RTL runs when PDFium coalesces them") {
    const auto rtl_font = find_rtl_font();
    if (rtl_font.empty()) {
        MESSAGE("skipping Arabic split pure-RTL table cell smoke: configure "
                "test RTL font");
        return;
    }

    const auto logical_prefix = utf8_from_u8(u8"\u0633\u0644");
    const auto logical_suffix = utf8_from_u8(u8"\u0627\u0645");
    const auto visual_prefix = utf8_from_u8(u8"\u0644\u0633");
    const auto visual_suffix = utf8_from_u8(u8"\u0645\u0627");
    const auto logical_target_text = logical_prefix + logical_suffix;
    const auto pdfium_raw_core = visual_suffix + visual_prefix;
    const auto pdfium_raw_text = pdfium_raw_core + " ";

    featherdoc::pdf::PdfDocumentLayout layout;
    layout.metadata.title = "FeatherDoc Arabic split pure RTL table cell";
    layout.metadata.creator = "FeatherDoc test";

    featherdoc::pdf::PdfPageLayout page;
    page.size = featherdoc::pdf::PdfPageSize::a4_portrait();
    append_test_grid(page, 72.0, 700.0, 132.0, 32.0, 3U, 4U);
    page.text_runs.push_back(
        make_unicode_text_run(86.0, 678.0, "Item", rtl_font));
    page.text_runs.push_back(
        make_unicode_text_run(218.0, 678.0, "Owner", rtl_font));
    page.text_runs.push_back(
        make_unicode_text_run(350.0, 678.0, "Due", rtl_font));

    page.text_runs.push_back(
        make_unicode_text_run(86.0, 646.0, logical_prefix, rtl_font));
    page.text_runs.push_back(
        make_unicode_text_run(104.0, 646.0, logical_suffix, rtl_font));
    page.text_runs.push_back(
        make_unicode_text_run(218.0, 646.0, "Arabic Split", rtl_font));
    page.text_runs.push_back(
        make_unicode_text_run(350.0, 646.0, "2026-07-03", rtl_font));

    page.text_runs.push_back(
        make_unicode_text_run(86.0, 614.0, "Alpha", rtl_font));
    page.text_runs.push_back(
        make_unicode_text_run(218.0, 614.0, "QA", rtl_font));
    page.text_runs.push_back(
        make_unicode_text_run(350.0, 614.0, "2026-07-04", rtl_font));

    page.text_runs.push_back(
        make_unicode_text_run(86.0, 582.0, "Beta", rtl_font));
    page.text_runs.push_back(
        make_unicode_text_run(218.0, 582.0, "Docs", rtl_font));
    page.text_runs.push_back(
        make_unicode_text_run(350.0, 582.0, "2026-07-05", rtl_font));
    layout.pages.push_back(std::move(page));

    const auto output_path =
        std::filesystem::current_path() /
        "featherdoc-arabic-rtl-table-cell-split-pure-rtl-boundary.pdf";

    featherdoc::pdf::PdfioGenerator generator;
    const auto write_result =
        generator.write(layout, output_path, featherdoc::pdf::PdfWriterOptions{});
    REQUIRE_MESSAGE(write_result.success, write_result.error_message);

    featherdoc::pdf::PdfiumParser parser;
    featherdoc::pdf::PdfParseOptions raw_options;
    raw_options.normalize_rtl_text = false;
    const auto raw_parse_result = parser.parse(output_path, raw_options);
    REQUIRE_MESSAGE(raw_parse_result.success, raw_parse_result.error_message);
    const auto &raw_table = require_single_table_candidate(raw_parse_result, 4U);
    REQUIRE(raw_table.rows[1].cells[0].has_text);
    const auto &raw_target_cell = raw_table.rows[1].cells[0];
    INFO("raw Arabic split pure-RTL table cell text: "
         << raw_target_cell.text);
    CHECK_EQ(raw_target_cell.text, pdfium_raw_text);
    CHECK_EQ(count_occurrences(raw_target_cell.text, logical_target_text), 0U);
    CHECK_EQ(count_occurrences(raw_target_cell.text, visual_prefix), 1U);
    CHECK_EQ(count_occurrences(raw_target_cell.text, visual_suffix), 1U);
    REQUIRE_FALSE(raw_target_cell.text_spans.empty());
    check_span_sequences_appear_in_order(raw_target_cell.text_spans,
                                         {visual_suffix, visual_prefix});

    const auto normalized_parse_result = parser.parse(output_path, {});
    REQUIRE_MESSAGE(normalized_parse_result.success,
                    normalized_parse_result.error_message);
    const auto &normalized_table =
        require_single_table_candidate(normalized_parse_result, 4U);
    check_table_candidate_geometry_unchanged(normalized_table, raw_table);
    check_table_cell_text_unchanged_except_column_rows(
        normalized_table, raw_table, 0U, 1U, 1U);
    REQUIRE(normalized_table.rows[1].cells[0].has_text);
    const auto &normalized_target_cell = normalized_table.rows[1].cells[0];
    CHECK_EQ(normalized_target_cell.text, logical_target_text);
    check_span_sequences_appear_in_order(normalized_target_cell.text_spans,
                                         {visual_suffix, visual_prefix});
    check_raw_text_spans_unchanged(normalized_target_cell.text_spans,
                                   raw_target_cell.text_spans);

    const auto imported_docx_path =
        std::filesystem::current_path() /
        "featherdoc-arabic-rtl-table-cell-split-pure-rtl-boundary-import.docx";
    std::filesystem::remove(imported_docx_path);

    featherdoc::Document imported_document(imported_docx_path);
    featherdoc::pdf::PdfDocumentImportOptions import_options;
    import_options.import_table_candidates_as_tables = true;
    const auto import_result = featherdoc::pdf::import_pdf_text_document(
        output_path, imported_document, import_options);
    REQUIRE_MESSAGE(import_result.success, import_result.error_message);
    CHECK_EQ(import_result.tables_imported, 1U);
    const auto imported_cell = imported_document.inspect_table_cell(0U, 1U, 0U);
    REQUIRE(imported_cell.has_value());
    CHECK_EQ(imported_cell->text, normalized_target_cell.text);
    check_imported_table_cell_contains_text(
        imported_document, 1U, 0U, logical_target_text);
    check_imported_four_row_table_static_cells(imported_document,
                                               "Arabic Split",
                                               "2026-07-03",
                                               "2026-07-04",
                                               "2026-07-05");

    const auto raw_imported_docx_path =
        std::filesystem::current_path() /
        "featherdoc-arabic-rtl-table-cell-split-pure-rtl-boundary-raw-import.docx";
    std::filesystem::remove(raw_imported_docx_path);

    featherdoc::Document raw_imported_document(raw_imported_docx_path);
    featherdoc::pdf::PdfDocumentImportOptions raw_import_options;
    raw_import_options.import_table_candidates_as_tables = true;
    raw_import_options.parse_options.normalize_rtl_text = false;
    const auto raw_import_result = featherdoc::pdf::import_pdf_text_document(
        output_path, raw_imported_document, raw_import_options);
    REQUIRE_MESSAGE(raw_import_result.success,
                    raw_import_result.error_message);
    CHECK_EQ(raw_import_result.tables_imported, 1U);
    const auto raw_imported_cell =
        raw_imported_document.inspect_table_cell(0U, 1U, 0U);
    REQUIRE(raw_imported_cell.has_value());
    CHECK_EQ(raw_imported_cell->text.find(logical_target_text),
             std::string::npos);
    check_imported_table_cell_contains_text(
        raw_imported_document, 1U, 0U, pdfium_raw_core);
    check_imported_four_row_table_static_cells(raw_imported_document,
                                               "Arabic Split",
                                               "2026-07-03",
                                               "2026-07-04",
                                               "2026-07-05");
    check_imported_four_row_table_cells_unchanged_except(
        imported_document, raw_imported_document, 1U, 0U);
}

TEST_CASE("PDFium RTL table cell raw spans preserve Hebrew combining marks") {
    const auto rtl_font = find_rtl_font();
    if (rtl_font.empty()) {
        MESSAGE("skipping Hebrew combining-mark table cell smoke: configure "
                "test RTL font");
        return;
    }

    const auto logical_text =
        utf8_from_u8(u8"\u05E9\u05B8\u05DC\u05D5\u05B9\u05DD");
    const auto pdfium_raw_text =
        utf8_from_u8(u8"\u05DD\u05B9\u05D5\u05DC\u05B8\u05E9");
    const auto qamats = utf8_from_u8(u8"\u05B8");
    const auto holam = utf8_from_u8(u8"\u05B9");
    const auto final_mem = utf8_from_u8(u8"\u05DD");
    const auto vav = utf8_from_u8(u8"\u05D5");
    const auto lamed = utf8_from_u8(u8"\u05DC");
    const auto shin = utf8_from_u8(u8"\u05E9");
    const std::vector<ExpectedCombiningMarkNeighbors> mark_neighbors = {
        ExpectedCombiningMarkNeighbors{final_mem, holam, vav},
        ExpectedCombiningMarkNeighbors{lamed, qamats, shin}};

    featherdoc::pdf::PdfDocumentLayout layout;
    layout.metadata.title = "FeatherDoc Hebrew combining mark table cell";
    layout.metadata.creator = "FeatherDoc test";

    featherdoc::pdf::PdfPageLayout page;
    page.size = featherdoc::pdf::PdfPageSize::a4_portrait();
    append_test_grid(page, 72.0, 700.0, 132.0, 32.0, 3U, 4U);
    page.text_runs.push_back(
        make_unicode_text_run(86.0, 678.0, "Item", rtl_font));
    page.text_runs.push_back(
        make_unicode_text_run(218.0, 678.0, "Owner", rtl_font));
    page.text_runs.push_back(
        make_unicode_text_run(350.0, 678.0, "Due", rtl_font));

    page.text_runs.push_back(
        make_unicode_text_run(86.0, 646.0, logical_text, rtl_font));
    page.text_runs.push_back(
        make_unicode_text_run(218.0, 646.0, "Niqqud", rtl_font));
    page.text_runs.push_back(
        make_unicode_text_run(350.0, 646.0, "2026-06-24", rtl_font));

    page.text_runs.push_back(
        make_unicode_text_run(86.0, 614.0, "Alpha", rtl_font));
    page.text_runs.push_back(
        make_unicode_text_run(218.0, 614.0, "QA", rtl_font));
    page.text_runs.push_back(
        make_unicode_text_run(350.0, 614.0, "2026-06-25", rtl_font));

    page.text_runs.push_back(
        make_unicode_text_run(86.0, 582.0, "Beta", rtl_font));
    page.text_runs.push_back(
        make_unicode_text_run(218.0, 582.0, "Docs", rtl_font));
    page.text_runs.push_back(
        make_unicode_text_run(350.0, 582.0, "2026-06-26", rtl_font));
    layout.pages.push_back(std::move(page));

    const auto output_path =
        std::filesystem::current_path() /
        "featherdoc-hebrew-rtl-table-cell-combining-mark-boundary.pdf";

    featherdoc::pdf::PdfioGenerator generator;
    const auto write_result =
        generator.write(layout, output_path, featherdoc::pdf::PdfWriterOptions{});
    REQUIRE_MESSAGE(write_result.success, write_result.error_message);

    featherdoc::pdf::PdfiumParser parser;
    featherdoc::pdf::PdfParseOptions raw_options;
    raw_options.normalize_rtl_text = false;
    const auto raw_parse_result = parser.parse(output_path, raw_options);
    REQUIRE_MESSAGE(raw_parse_result.success, raw_parse_result.error_message);
    const auto &raw_table = require_single_table_candidate(raw_parse_result, 4U);
    REQUIRE(raw_table.rows[1].cells[0].has_text);
    const auto &raw_target_cell = raw_table.rows[1].cells[0];
    INFO("raw Hebrew combining-mark table cell text: "
         << raw_target_cell.text);
    CHECK_EQ(count_occurrences(raw_target_cell.text, logical_text), 0U);
    CHECK_EQ(count_occurrences(raw_target_cell.text, pdfium_raw_text), 1U);
    REQUIRE_FALSE(raw_target_cell.text_spans.empty());
    CHECK(has_span_sequence_with_geometry(raw_target_cell.text_spans,
                                          pdfium_raw_text));
    CHECK(has_span_sequence_with_geometry(raw_target_cell.text_spans, qamats));
    CHECK(has_span_sequence_with_geometry(raw_target_cell.text_spans, holam));
    check_combining_marks_have_adjacent_base_geometry(
        raw_target_cell.text_spans, pdfium_raw_text, qamats);
    check_combining_marks_have_adjacent_base_geometry(
        raw_target_cell.text_spans, pdfium_raw_text, holam);
    check_combining_mark_neighbor_geometry(raw_target_cell.text_spans,
                                           pdfium_raw_text,
                                           mark_neighbors);

    const auto normalized_parse_result = parser.parse(output_path, {});
    REQUIRE_MESSAGE(normalized_parse_result.success,
                    normalized_parse_result.error_message);
    const auto &normalized_table =
        require_single_table_candidate(normalized_parse_result, 4U);
    check_table_candidate_geometry_unchanged(normalized_table, raw_table);
    check_table_cell_text_unchanged_except_column_rows(
        normalized_table, raw_table, 0U, 0U, 0U);
    REQUIRE(normalized_table.rows[1].cells[0].has_text);
    const auto &normalized_target_cell = normalized_table.rows[1].cells[0];
    CHECK_EQ(normalized_target_cell.text, raw_target_cell.text);
    CHECK_EQ(count_occurrences(normalized_target_cell.text, logical_text), 0U);
    CHECK_EQ(count_occurrences(normalized_target_cell.text, pdfium_raw_text),
             1U);
    REQUIRE_FALSE(normalized_target_cell.text_spans.empty());
    CHECK(has_span_sequence_with_geometry(normalized_target_cell.text_spans,
                                          pdfium_raw_text));
    check_raw_text_spans_unchanged(normalized_target_cell.text_spans,
                                   raw_target_cell.text_spans);
    check_combining_marks_have_adjacent_base_geometry(
        normalized_target_cell.text_spans, pdfium_raw_text, qamats);
    check_combining_marks_have_adjacent_base_geometry(
        normalized_target_cell.text_spans, pdfium_raw_text, holam);
    check_combining_mark_neighbor_geometry(normalized_target_cell.text_spans,
                                           pdfium_raw_text,
                                           mark_neighbors);

    const auto imported_docx_path =
        std::filesystem::current_path() /
        "featherdoc-hebrew-rtl-table-cell-combining-mark-boundary-import.docx";
    std::filesystem::remove(imported_docx_path);

    featherdoc::Document imported_document(imported_docx_path);
    featherdoc::pdf::PdfDocumentImportOptions import_options;
    import_options.import_table_candidates_as_tables = true;
    const auto import_result = featherdoc::pdf::import_pdf_text_document(
        output_path, imported_document, import_options);
    REQUIRE_MESSAGE(import_result.success, import_result.error_message);
    CHECK_EQ(import_result.tables_imported, 1U);
    const auto imported_cell = imported_document.inspect_table_cell(0U, 1U, 0U);
    REQUIRE(imported_cell.has_value());
    CHECK_EQ(imported_cell->text, raw_target_cell.text);
    check_imported_four_row_table_static_cells(
        imported_document,
        "Niqqud",
        "2026-06-24",
        "2026-06-25",
        "2026-06-26");

    const auto raw_imported_docx_path =
        std::filesystem::current_path() /
        "featherdoc-hebrew-rtl-table-cell-combining-mark-boundary-raw-import.docx";
    std::filesystem::remove(raw_imported_docx_path);

    featherdoc::Document raw_imported_document(raw_imported_docx_path);
    featherdoc::pdf::PdfDocumentImportOptions raw_import_options;
    raw_import_options.import_table_candidates_as_tables = true;
    raw_import_options.parse_options.normalize_rtl_text = false;
    const auto raw_import_result = featherdoc::pdf::import_pdf_text_document(
        output_path, raw_imported_document, raw_import_options);
    REQUIRE_MESSAGE(raw_import_result.success,
                    raw_import_result.error_message);
    CHECK_EQ(raw_import_result.tables_imported, 1U);
    const auto raw_imported_cell =
        raw_imported_document.inspect_table_cell(0U, 1U, 0U);
    REQUIRE(raw_imported_cell.has_value());
    CHECK_EQ(raw_imported_cell->text, raw_target_cell.text);
    check_imported_four_row_table_static_cells(
        raw_imported_document,
        "Niqqud",
        "2026-06-24",
        "2026-06-25",
        "2026-06-26");
    check_imported_four_row_table_cells_unchanged(
        imported_document, raw_imported_document);
}

TEST_CASE("PDFium RTL table cell raw spans preserve Arabic combining marks") {
    const auto rtl_font = find_rtl_font();
    if (rtl_font.empty()) {
        MESSAGE("skipping Arabic combining-mark table cell smoke: configure "
                "test RTL font");
        return;
    }

    const auto logical_text = utf8_from_u8(
        u8"\u0633\u064E\u0644\u064E\u0627\u0645");
    const auto pdfium_raw_text = utf8_from_u8(
        u8"\u0645\u0627\u064E\u0644\u064E\u0633");
    const auto fatha = utf8_from_u8(u8"\u064E");
    const auto alef = utf8_from_u8(u8"\u0627");
    const auto lam = utf8_from_u8(u8"\u0644");
    const auto seen = utf8_from_u8(u8"\u0633");
    const std::vector<ExpectedCombiningMarkNeighbors> mark_neighbors = {
        ExpectedCombiningMarkNeighbors{alef, fatha, lam},
        ExpectedCombiningMarkNeighbors{lam, fatha, seen}};

    featherdoc::pdf::PdfDocumentLayout layout;
    layout.metadata.title = "FeatherDoc Arabic combining mark table cell";
    layout.metadata.creator = "FeatherDoc test";

    featherdoc::pdf::PdfPageLayout page;
    page.size = featherdoc::pdf::PdfPageSize::a4_portrait();
    append_test_grid(page, 72.0, 700.0, 132.0, 32.0, 3U, 4U);
    page.text_runs.push_back(
        make_unicode_text_run(86.0, 678.0, "Item", rtl_font));
    page.text_runs.push_back(
        make_unicode_text_run(218.0, 678.0, "Owner", rtl_font));
    page.text_runs.push_back(
        make_unicode_text_run(350.0, 678.0, "Due", rtl_font));

    page.text_runs.push_back(
        make_unicode_text_run(86.0, 646.0, logical_text, rtl_font));
    page.text_runs.push_back(
        make_unicode_text_run(218.0, 646.0, "Harakat", rtl_font));
    page.text_runs.push_back(
        make_unicode_text_run(350.0, 646.0, "2026-06-27", rtl_font));

    page.text_runs.push_back(
        make_unicode_text_run(86.0, 614.0, "Alpha", rtl_font));
    page.text_runs.push_back(
        make_unicode_text_run(218.0, 614.0, "QA", rtl_font));
    page.text_runs.push_back(
        make_unicode_text_run(350.0, 614.0, "2026-06-28", rtl_font));

    page.text_runs.push_back(
        make_unicode_text_run(86.0, 582.0, "Beta", rtl_font));
    page.text_runs.push_back(
        make_unicode_text_run(218.0, 582.0, "Docs", rtl_font));
    page.text_runs.push_back(
        make_unicode_text_run(350.0, 582.0, "2026-06-29", rtl_font));
    layout.pages.push_back(std::move(page));

    const auto output_path =
        std::filesystem::current_path() /
        "featherdoc-arabic-rtl-table-cell-combining-mark-boundary.pdf";

    featherdoc::pdf::PdfioGenerator generator;
    const auto write_result =
        generator.write(layout, output_path, featherdoc::pdf::PdfWriterOptions{});
    REQUIRE_MESSAGE(write_result.success, write_result.error_message);

    featherdoc::pdf::PdfiumParser parser;
    featherdoc::pdf::PdfParseOptions raw_options;
    raw_options.normalize_rtl_text = false;
    const auto raw_parse_result = parser.parse(output_path, raw_options);
    REQUIRE_MESSAGE(raw_parse_result.success, raw_parse_result.error_message);
    const auto &raw_table = require_single_table_candidate(raw_parse_result, 4U);
    REQUIRE(raw_table.rows[1].cells[0].has_text);
    const auto &raw_target_cell = raw_table.rows[1].cells[0];
    INFO("raw Arabic combining-mark table cell text: "
         << raw_target_cell.text);
    CHECK_EQ(count_occurrences(raw_target_cell.text, logical_text), 0U);
    CHECK_EQ(count_occurrences(raw_target_cell.text, pdfium_raw_text), 1U);
    REQUIRE_FALSE(raw_target_cell.text_spans.empty());
    CHECK(has_span_sequence_with_geometry(raw_target_cell.text_spans,
                                          pdfium_raw_text));
    CHECK(has_span_sequence_with_geometry(raw_target_cell.text_spans, fatha));
    check_combining_marks_have_adjacent_base_geometry(
        raw_target_cell.text_spans, pdfium_raw_text, fatha);
    check_combining_mark_neighbor_geometry(raw_target_cell.text_spans,
                                           pdfium_raw_text,
                                           mark_neighbors);

    const auto normalized_parse_result = parser.parse(output_path, {});
    REQUIRE_MESSAGE(normalized_parse_result.success,
                    normalized_parse_result.error_message);
    const auto &normalized_table =
        require_single_table_candidate(normalized_parse_result, 4U);
    check_table_candidate_geometry_unchanged(normalized_table, raw_table);
    check_table_cell_text_unchanged_except_column_rows(
        normalized_table, raw_table, 0U, 0U, 0U);
    REQUIRE(normalized_table.rows[1].cells[0].has_text);
    const auto &normalized_target_cell = normalized_table.rows[1].cells[0];
    CHECK_EQ(normalized_target_cell.text, raw_target_cell.text);
    CHECK_EQ(count_occurrences(normalized_target_cell.text, logical_text), 0U);
    CHECK_EQ(count_occurrences(normalized_target_cell.text, pdfium_raw_text),
             1U);
    REQUIRE_FALSE(normalized_target_cell.text_spans.empty());
    CHECK(has_span_sequence_with_geometry(normalized_target_cell.text_spans,
                                          pdfium_raw_text));
    check_raw_text_spans_unchanged(normalized_target_cell.text_spans,
                                   raw_target_cell.text_spans);
    check_combining_marks_have_adjacent_base_geometry(
        normalized_target_cell.text_spans, pdfium_raw_text, fatha);
    check_combining_mark_neighbor_geometry(normalized_target_cell.text_spans,
                                           pdfium_raw_text,
                                           mark_neighbors);

    const auto imported_docx_path =
        std::filesystem::current_path() /
        "featherdoc-arabic-rtl-table-cell-combining-mark-boundary-import.docx";
    std::filesystem::remove(imported_docx_path);

    featherdoc::Document imported_document(imported_docx_path);
    featherdoc::pdf::PdfDocumentImportOptions import_options;
    import_options.import_table_candidates_as_tables = true;
    const auto import_result = featherdoc::pdf::import_pdf_text_document(
        output_path, imported_document, import_options);
    REQUIRE_MESSAGE(import_result.success, import_result.error_message);
    CHECK_EQ(import_result.tables_imported, 1U);
    const auto imported_cell = imported_document.inspect_table_cell(0U, 1U, 0U);
    REQUIRE(imported_cell.has_value());
    CHECK_EQ(imported_cell->text, raw_target_cell.text);
    check_imported_four_row_table_static_cells(
        imported_document,
        "Harakat",
        "2026-06-27",
        "2026-06-28",
        "2026-06-29");

    const auto raw_imported_docx_path =
        std::filesystem::current_path() /
        "featherdoc-arabic-rtl-table-cell-combining-mark-boundary-raw-import.docx";
    std::filesystem::remove(raw_imported_docx_path);

    featherdoc::Document raw_imported_document(raw_imported_docx_path);
    featherdoc::pdf::PdfDocumentImportOptions raw_import_options;
    raw_import_options.import_table_candidates_as_tables = true;
    raw_import_options.parse_options.normalize_rtl_text = false;
    const auto raw_import_result = featherdoc::pdf::import_pdf_text_document(
        output_path, raw_imported_document, raw_import_options);
    REQUIRE_MESSAGE(raw_import_result.success,
                    raw_import_result.error_message);
    CHECK_EQ(raw_import_result.tables_imported, 1U);
    const auto raw_imported_cell =
        raw_imported_document.inspect_table_cell(0U, 1U, 0U);
    REQUIRE(raw_imported_cell.has_value());
    CHECK_EQ(raw_imported_cell->text, raw_target_cell.text);
    check_imported_four_row_table_static_cells(
        raw_imported_document,
        "Harakat",
        "2026-06-27",
        "2026-06-28",
        "2026-06-29");
    check_imported_four_row_table_cells_unchanged(
        imported_document, raw_imported_document);
}

TEST_CASE("PDFium RTL table cell normalization helper branches stay "
          "disjoint") {
    const auto hebrew_logical_text = hebrew_shalom_logical();
    const auto hebrew_visual_order_text = hebrew_shalom_visual();
    const auto arabic_logical_text = arabic_salam_logical();
    const auto arabic_visual_order_text = arabic_salam_visual();
    const auto arabic_indic_digits = utf8_from_u8(u8"١٢٣");
    const auto persian_digits = utf8_from_u8(u8"\u06F1\u06F2\u06F3");
    const auto hebrew_combining_mark_visual_order_text =
        utf8_from_u8(u8"\u05DD\u05D5\u05B9\u05DC\u05E9\u05B8");
    const auto arabic_combining_mark_visual_order_text = utf8_from_u8(
        u8"\u0645\u0627\u0644\u064E\u0633\u064E");

    const std::vector<RtlTableCellBoundaryCase> cases = {
        RtlTableCellBoundaryCase{
            "pure-rtl",
            hebrew_logical_text,
            hebrew_visual_order_text},
        RtlTableCellBoundaryCase{
            "digit-prefix",
            hebrew_logical_text + " 123",
            std::string{"123 "} + hebrew_visual_order_text},
        RtlTableCellBoundaryCase{
            "leading-separator",
            hebrew_logical_text + "/123",
            std::string{"/123"} + hebrew_visual_order_text},
        RtlTableCellBoundaryCase{
            "trailing-digit",
            arabic_logical_text + " " + arabic_indic_digits,
            arabic_visual_order_text + " " + arabic_indic_digits},
        RtlTableCellBoundaryCase{
            "trailing-separator",
            hebrew_logical_text + ": " + arabic_indic_digits,
            hebrew_visual_order_text + ": " + arabic_indic_digits},
        RtlTableCellBoundaryCase{
            "persian-trailing-digit",
            hebrew_logical_text + " " + persian_digits,
            hebrew_visual_order_text + " " + persian_digits + " "},
        RtlTableCellBoundaryCase{
            "persian-slash",
            hebrew_logical_text + "/" + persian_digits,
            hebrew_visual_order_text + "/" + persian_digits + " "},
        RtlTableCellBoundaryCase{
            "persian-semicolon",
            hebrew_logical_text + ";" + persian_digits,
            hebrew_visual_order_text + ";" + persian_digits + " "},
        RtlTableCellBoundaryCase{
            "persian-colon",
            hebrew_logical_text + ": " + persian_digits,
            hebrew_visual_order_text + ": " + persian_digits},
        RtlTableCellBoundaryCase{
            "persian-hyphen",
            hebrew_logical_text + "-" + persian_digits,
            hebrew_visual_order_text + "-" + persian_digits + " "},
        RtlTableCellBoundaryCase{
            "persian-comma",
            hebrew_logical_text + "," + persian_digits,
            hebrew_visual_order_text + "," + persian_digits + " "},
        RtlTableCellBoundaryCase{
            "persian-period",
            hebrew_logical_text + "." + persian_digits,
            hebrew_visual_order_text + "." + persian_digits + " "},
    };

    for (const auto &boundary_case : cases) {
        INFO("RTL table cell helper boundary case: " << boundary_case.owner);
        auto text = boundary_case.raw_text;
        CHECK(featherdoc::pdf::detail::
                  normalize_rtl_table_cell_text_for_testing(text));
        CHECK_EQ(text, boundary_case.logical_text);
    }

    auto mixed_ltr_rtl_text = std::string{"Status "} + hebrew_visual_order_text;
    CHECK_FALSE(featherdoc::pdf::detail::
                    normalize_rtl_table_cell_text_for_testing(
                        mixed_ltr_rtl_text));
    CHECK_EQ(mixed_ltr_rtl_text, std::string{"Status "} + hebrew_visual_order_text);

    auto split_pure_rtl_text =
        utf8_from_u8(u8"\u05DC\u05E9\n\u05DD\u05D5");
    CHECK_FALSE(featherdoc::pdf::detail::
                    normalize_rtl_table_cell_text_for_testing(
                        split_pure_rtl_text));
    CHECK_EQ(split_pure_rtl_text,
             utf8_from_u8(u8"\u05DC\u05E9\n\u05DD\u05D5"));

    auto split_pure_rtl_carriage_return_text =
        utf8_from_u8(u8"\u05DC\u05E9\r\u05DD\u05D5");
    CHECK_FALSE(featherdoc::pdf::detail::
                    normalize_rtl_table_cell_text_for_testing(
                        split_pure_rtl_carriage_return_text));
    CHECK_EQ(split_pure_rtl_carriage_return_text,
             utf8_from_u8(u8"\u05DC\u05E9\r\u05DD\u05D5"));

    auto arabic_split_pure_rtl_text =
        utf8_from_u8(u8"\u0644\u0633\n\u0645\u0627");
    CHECK_FALSE(featherdoc::pdf::detail::
                    normalize_rtl_table_cell_text_for_testing(
                        arabic_split_pure_rtl_text));
    CHECK_EQ(arabic_split_pure_rtl_text,
             utf8_from_u8(u8"\u0644\u0633\n\u0645\u0627"));

    auto arabic_split_pure_rtl_carriage_return_text =
        utf8_from_u8(u8"\u0644\u0633\r\u0645\u0627");
    CHECK_FALSE(featherdoc::pdf::detail::
                    normalize_rtl_table_cell_text_for_testing(
                        arabic_split_pure_rtl_carriage_return_text));
    CHECK_EQ(arabic_split_pure_rtl_carriage_return_text,
             utf8_from_u8(u8"\u0644\u0633\r\u0645\u0627"));

    auto hebrew_combining_mark_text = hebrew_combining_mark_visual_order_text;
    CHECK_FALSE(featherdoc::pdf::detail::
                    normalize_rtl_table_cell_text_for_testing(
                        hebrew_combining_mark_text));
    CHECK_EQ(hebrew_combining_mark_text,
             hebrew_combining_mark_visual_order_text);

    auto arabic_combining_mark_text = arabic_combining_mark_visual_order_text;
    CHECK_FALSE(featherdoc::pdf::detail::
                    normalize_rtl_table_cell_text_for_testing(
                        arabic_combining_mark_text));
    CHECK_EQ(arabic_combining_mark_text,
             arabic_combining_mark_visual_order_text);
}

TEST_CASE("PDFium RTL normalization applies to Arabic table cell digit "
          "boundaries") {
    const auto rtl_font = find_rtl_font();
    if (rtl_font.empty()) {
        MESSAGE("skipping Arabic table cell normalization smoke: configure "
                "test RTL font");
        return;
    }

    const auto logical_text = arabic_salam_logical();
    const auto visual_order_text = arabic_salam_visual();
    const auto arabic_indic_digits = utf8_from_u8(u8"\u0661\u0662\u0663");
    const auto persian_digits = utf8_from_u8(u8"\u06F1\u06F2\u06F3");

    const std::vector<RtlTableCellBoundaryCase> cases = {
        RtlTableCellBoundaryCase{
            "Arabic",
            logical_text,
            visual_order_text,
        },
        RtlTableCellBoundaryCase{
            "Arabic AN",
            logical_text + " " + arabic_indic_digits,
            visual_order_text + " " + arabic_indic_digits,
        },
        RtlTableCellBoundaryCase{
            "Arabic Slash",
            logical_text + "/" + arabic_indic_digits,
            visual_order_text + "/" + arabic_indic_digits,
        },
        RtlTableCellBoundaryCase{
            "Arabic Semi",
            logical_text + ";" + arabic_indic_digits,
            visual_order_text + ";" + arabic_indic_digits,
        },
        RtlTableCellBoundaryCase{
            "Arabic Colon",
            logical_text + ": " + arabic_indic_digits,
            visual_order_text + ": " + arabic_indic_digits,
        },
        RtlTableCellBoundaryCase{
            "Arabic Hyphen",
            logical_text + "-" + arabic_indic_digits,
            visual_order_text + "-" + arabic_indic_digits,
        },
        RtlTableCellBoundaryCase{
            "Arabic Comma",
            logical_text + "," + arabic_indic_digits,
            visual_order_text + "," + arabic_indic_digits,
        },
        RtlTableCellBoundaryCase{
            "Arabic Period",
            logical_text + "." + arabic_indic_digits,
            visual_order_text + "." + arabic_indic_digits,
        },
        RtlTableCellBoundaryCase{
            "ArP AN",
            logical_text + " " + persian_digits,
            visual_order_text + " " + persian_digits,
        },
        RtlTableCellBoundaryCase{
            "ArP Slash",
            logical_text + "/" + persian_digits,
            visual_order_text + "/" + persian_digits,
        },
        RtlTableCellBoundaryCase{
            "ArP Semi",
            logical_text + ";" + persian_digits,
            visual_order_text + ";" + persian_digits,
        },
        RtlTableCellBoundaryCase{
            "ArP Colon",
            logical_text + ": " + persian_digits,
            visual_order_text + ": " + persian_digits,
        },
        RtlTableCellBoundaryCase{
            "ArP Hyph",
            logical_text + "-" + persian_digits,
            visual_order_text + "-" + persian_digits,
        },
        RtlTableCellBoundaryCase{
            "ArP Comma",
            logical_text + "," + persian_digits,
            visual_order_text + "," + persian_digits,
        },
        RtlTableCellBoundaryCase{
            "ArP Dot",
            logical_text + "." + persian_digits,
            visual_order_text + "." + persian_digits,
        },
    };

    featherdoc::pdf::PdfDocumentLayout layout;
    append_rtl_table_cell_boundary_table(
        layout,
        rtl_font,
        "FeatherDoc Arabic table cell normalization",
        cases);

    const auto output_path =
        std::filesystem::current_path() /
        "featherdoc-arabic-rtl-table-cell-boundary.pdf";

    featherdoc::pdf::PdfioGenerator generator;
    const auto write_result =
        generator.write(layout, output_path, featherdoc::pdf::PdfWriterOptions{});
    REQUIRE_MESSAGE(write_result.success, write_result.error_message);

    featherdoc::pdf::PdfiumParser parser;
    featherdoc::pdf::PdfParseOptions raw_options;
    raw_options.normalize_rtl_text = false;
    const auto raw_parse_result = parser.parse(output_path, raw_options);
    REQUIRE_MESSAGE(raw_parse_result.success, raw_parse_result.error_message);
    const auto &raw_table =
        require_single_table_candidate(raw_parse_result, cases.size() + 1U);
    check_raw_table_boundary_cells(raw_table, cases);

    const auto normalized_parse_result = parser.parse(output_path, {});
    REQUIRE_MESSAGE(normalized_parse_result.success,
                    normalized_parse_result.error_message);
    const auto &normalized_table =
        require_single_table_candidate(normalized_parse_result,
                                       cases.size() + 1U);
    check_table_candidate_geometry_unchanged(normalized_table, raw_table);
    check_table_cell_text_unchanged_except_column_rows(
        normalized_table, raw_table, 0U, 1U, cases.size());
    check_normalized_table_boundary_cells(normalized_table, raw_table, cases);

    const auto imported_docx_path =
        std::filesystem::current_path() /
        "featherdoc-arabic-rtl-table-cell-boundary-import.docx";
    std::filesystem::remove(imported_docx_path);

    featherdoc::Document imported_document(imported_docx_path);
    featherdoc::pdf::PdfDocumentImportOptions import_options;
    import_options.import_table_candidates_as_tables = true;
    const auto import_result = featherdoc::pdf::import_pdf_text_document(
        output_path, imported_document, import_options);
    REQUIRE_MESSAGE(import_result.success, import_result.error_message);
    CHECK_EQ(import_result.tables_imported, 1U);
    check_imported_table_boundary_cells(imported_document, cases);
    check_imported_table_static_boundary_cells(imported_document, cases);

    const auto raw_imported_docx_path =
        std::filesystem::current_path() /
        "featherdoc-arabic-rtl-table-cell-boundary-raw-import.docx";
    std::filesystem::remove(raw_imported_docx_path);

    featherdoc::Document raw_imported_document(raw_imported_docx_path);
    featherdoc::pdf::PdfDocumentImportOptions raw_import_options;
    raw_import_options.import_table_candidates_as_tables = true;
    raw_import_options.parse_options.normalize_rtl_text = false;
    const auto raw_import_result = featherdoc::pdf::import_pdf_text_document(
        output_path, raw_imported_document, raw_import_options);
    REQUIRE_MESSAGE(raw_import_result.success,
                    raw_import_result.error_message);
    CHECK_EQ(raw_import_result.tables_imported, 1U);
    check_raw_imported_table_boundary_cells(raw_imported_document, cases);
    check_imported_table_static_boundary_cells(raw_imported_document, cases);
    check_imported_table_static_boundary_cells_unchanged(
        imported_document, raw_imported_document, cases);
}

TEST_CASE("PDFium Hebrew extraction depends on fallback content order") {
    const auto rtl_font = find_rtl_font();
    if (rtl_font.empty()) {
        MESSAGE("skipping Hebrew content order extraction smoke: configure "
                "test RTL font");
        return;
    }

    const auto logical_text = hebrew_shalom_logical();
    const auto visual_order_text = hebrew_shalom_visual();

    featherdoc::pdf::PdfDocumentLayout layout;
    layout.metadata.title = "FeatherDoc Hebrew content order extraction";
    layout.metadata.creator = "FeatherDoc test";

    featherdoc::pdf::PdfPageLayout page;
    page.size = featherdoc::pdf::PdfPageSize::a4_portrait();
    page.text_runs.push_back(featherdoc::pdf::PdfTextRun{
        featherdoc::pdf::PdfPoint{72.0, 720.0},
        visual_order_text,
        "Hebrew Test Font",
        rtl_font,
        18.0,
        featherdoc::pdf::PdfRgbColor{0.0, 0.0, 0.0},
        false,
        false,
        false,
        true,
    });
    layout.pages.push_back(std::move(page));

    const auto output_path =
        std::filesystem::current_path() /
        "featherdoc-hebrew-visual-order-fallback.pdf";

    featherdoc::pdf::PdfioGenerator generator;
    const auto write_result =
        generator.write(layout, output_path, featherdoc::pdf::PdfWriterOptions{});
    REQUIRE_MESSAGE(write_result.success, write_result.error_message);
    CHECK_GT(write_result.bytes_written, 0U);

    check_hebrew_fallback_extraction_boundary(
        output_path,
        visual_order_text,
        visual_order_text,
        logical_text,
        visual_order_text,
        1U,
        0U);
}

TEST_CASE("PDFium Hebrew extraction with logical ActualText and visual Tj is "
          "documented") {
    const auto rtl_font = find_rtl_font();
    if (rtl_font.empty()) {
        MESSAGE("skipping Hebrew logical ActualText visual Tj smoke: configure "
                "test RTL font");
        return;
    }

    const auto logical_text = hebrew_shalom_logical();
    const auto visual_order_text = hebrew_shalom_visual();

    const auto output_path =
        std::filesystem::current_path() /
        "featherdoc-hebrew-logical-actual-visual-show.pdf";

    std::string error_message;
    REQUIRE_MESSAGE(write_hebrew_actual_text_fixture(output_path, rtl_font,
                                                     logical_text,
                                                     visual_order_text,
                                                     error_message),
                    error_message);

    check_hebrew_fallback_extraction_boundary(
        output_path,
        logical_text,
        visual_order_text,
        logical_text,
        visual_order_text,
        0U,
        1U);
}

TEST_CASE("PDFium Hebrew extraction for RTL glyph-id CID fixture is documented") {
    const auto rtl_font = find_rtl_font();
    if (rtl_font.empty()) {
        MESSAGE("skipping Hebrew RTL glyph CID smoke: configure test RTL font");
        return;
    }
    if (!featherdoc::pdf::pdf_text_shaper_has_harfbuzz()) {
        MESSAGE("skipping Hebrew RTL glyph CID smoke: HarfBuzz unavailable");
        return;
    }

    const auto logical_text = hebrew_shalom_logical();
    const auto visual_order_text = hebrew_shalom_visual();
    constexpr double font_size = 18.0;
    const auto glyph_run = featherdoc::pdf::shape_pdf_text(
        logical_text,
        featherdoc::pdf::PdfTextShaperOptions{rtl_font, font_size});
    if (!glyph_run.used_harfbuzz || !glyph_run.error_message.empty() ||
        glyph_run.glyphs.empty()) {
        MESSAGE("skipping Hebrew RTL glyph CID smoke: shaping failed");
        return;
    }

    CHECK_EQ(glyph_run.direction,
             featherdoc::pdf::PdfGlyphDirection::right_to_left);
    CHECK_EQ(glyph_run.script_tag, "Hebr");

    const auto expected_entries = expected_to_unicode_entries(glyph_run);
    REQUIRE_FALSE(expected_entries.empty());

    std::string mapped_text_in_cid_order;
    for (const auto &entry : expected_entries) {
        mapped_text_in_cid_order += entry.unicode_text;
    }
    CHECK_EQ(mapped_text_in_cid_order, visual_order_text);

    const auto output_path =
        std::filesystem::current_path() /
        "featherdoc-hebrew-rtl-glyph-cid-fixture.pdf";

    const auto cid_entries = glyph_cid_entries_from_run(glyph_run);
    std::string error_message;
    REQUIRE_MESSAGE(write_hebrew_rtl_glyph_cid_fixture(
                        output_path, rtl_font, glyph_run, cid_entries,
                        logical_text, false, error_message),
                    error_message);

    const auto pdf_bytes = read_file_bytes(output_path);
    CHECK_NE(pdf_bytes.find("/FeatherDocGlyph"), std::string::npos);
    CHECK_NE(pdf_bytes.find("/CIDToGIDMap"), std::string::npos);
    CHECK_NE(pdf_bytes.find("/ToUnicode"), std::string::npos);
    CHECK_NE(pdf_bytes.find("/Identity-H"), std::string::npos);

    const auto streams = inflated_pdf_streams(pdf_bytes);
    const auto content_stream = find_actual_text_content_stream(streams);
    REQUIRE_FALSE(content_stream.empty());
    CHECK_NE(content_stream.find("/ActualText <FEFF" +
                                 utf16be_hex_payload_from_utf8(logical_text) +
                                 ">"),
             std::string::npos);
    CHECK_NE(content_stream.find(cid_tj_command(glyph_run.glyphs.size())),
             std::string::npos);

    const auto cmap = find_shaped_to_unicode_cmap(streams);
    REQUIRE_MESSAGE(!cmap.empty(), "RTL glyph CID ToUnicode CMap not found");
    for (const auto &entry : expected_entries) {
        const auto expected_cmap_entry =
            "<" + utf16be_hex_unit(entry.cid) + "> <" +
            utf16be_hex_payload_from_utf8(entry.unicode_text) + ">";
        CHECK_NE(cmap.find(expected_cmap_entry), std::string::npos);
    }

    featherdoc::pdf::PdfiumParser parser;
    const auto parse_result = parser.parse(output_path, {});
    REQUIRE_MESSAGE(parse_result.success, parse_result.error_message);

    const auto extracted_text = collect_text(parse_result.document);
    CHECK_EQ(count_occurrences(extracted_text, logical_text), 0U);
    CHECK_EQ(count_occurrences(extracted_text, visual_order_text), 1U);
}

TEST_CASE("PDFium Hebrew extraction for RTL glyph-id CID with logical "
          "ToUnicode is documented") {
    const auto rtl_font = find_rtl_font();
    if (rtl_font.empty()) {
        MESSAGE("skipping Hebrew RTL glyph CID logical ToUnicode smoke: "
                "configure test RTL font");
        return;
    }
    if (!featherdoc::pdf::pdf_text_shaper_has_harfbuzz()) {
        MESSAGE("skipping Hebrew RTL glyph CID logical ToUnicode smoke: "
                "HarfBuzz unavailable");
        return;
    }

    const auto logical_text = hebrew_shalom_logical();
    const auto visual_order_text = hebrew_shalom_visual();
    constexpr double font_size = 18.0;
    const auto glyph_run = featherdoc::pdf::shape_pdf_text(
        logical_text,
        featherdoc::pdf::PdfTextShaperOptions{rtl_font, font_size});
    if (!glyph_run.used_harfbuzz || !glyph_run.error_message.empty() ||
        glyph_run.glyphs.empty()) {
        MESSAGE("skipping Hebrew RTL glyph CID logical ToUnicode smoke: "
                "shaping failed");
        return;
    }

    CHECK_EQ(glyph_run.direction,
             featherdoc::pdf::PdfGlyphDirection::right_to_left);
    CHECK_EQ(glyph_run.script_tag, "Hebr");

    const auto logical_entries =
        logical_unicode_entries_for_visual_cids(glyph_run);
    REQUIRE_FALSE(logical_entries.empty());

    std::string mapped_text_in_cid_order;
    for (const auto &entry : logical_entries) {
        mapped_text_in_cid_order += entry.unicode_text;
    }
    CHECK_EQ(mapped_text_in_cid_order, logical_text);

    const auto output_path =
        std::filesystem::current_path() /
        "featherdoc-hebrew-rtl-glyph-cid-logical-tounicode.pdf";

    std::string error_message;
    REQUIRE_MESSAGE(write_hebrew_rtl_glyph_cid_fixture(
                        output_path, rtl_font, glyph_run, logical_entries,
                        logical_text, false, error_message),
                    error_message);

    const auto pdf_bytes = read_file_bytes(output_path);
    CHECK_NE(pdf_bytes.find("/FeatherDocGlyph"), std::string::npos);
    CHECK_NE(pdf_bytes.find("/CIDToGIDMap"), std::string::npos);
    CHECK_NE(pdf_bytes.find("/ToUnicode"), std::string::npos);
    CHECK_NE(pdf_bytes.find("/Identity-H"), std::string::npos);

    const auto streams = inflated_pdf_streams(pdf_bytes);
    const auto content_stream = find_actual_text_content_stream(streams);
    REQUIRE_FALSE(content_stream.empty());
    CHECK_NE(content_stream.find("/ActualText <FEFF" +
                                 utf16be_hex_payload_from_utf8(logical_text) +
                                 ">"),
             std::string::npos);
    CHECK_NE(content_stream.find(cid_tj_command(glyph_run.glyphs.size())),
             std::string::npos);

    const auto cmap = find_shaped_to_unicode_cmap(streams);
    REQUIRE_MESSAGE(!cmap.empty(),
                    "RTL glyph CID logical ToUnicode CMap not found");
    for (const auto &entry : logical_entries) {
        if (entry.unicode_text.empty()) {
            continue;
        }
        const auto expected_cmap_entry =
            "<" + utf16be_hex_unit(entry.cid) + "> <" +
            utf16be_hex_payload_from_utf8(entry.unicode_text) + ">";
        CHECK_NE(cmap.find(expected_cmap_entry), std::string::npos);
    }

    featherdoc::pdf::PdfiumParser parser;
    const auto parse_result = parser.parse(output_path, {});
    REQUIRE_MESSAGE(parse_result.success, parse_result.error_message);

    const auto extracted_text = collect_text(parse_result.document);
    CHECK_EQ(count_occurrences(extracted_text, logical_text), 0U);
    CHECK_EQ(count_occurrences(extracted_text, visual_order_text), 1U);
}

TEST_CASE("PDFium Hebrew extraction for RTL glyph-id CID with ReversedChars is "
          "documented") {
    const auto rtl_font = find_rtl_font();
    if (rtl_font.empty()) {
        MESSAGE("skipping Hebrew RTL glyph CID ReversedChars smoke: configure "
                "test RTL font");
        return;
    }
    if (!featherdoc::pdf::pdf_text_shaper_has_harfbuzz()) {
        MESSAGE("skipping Hebrew RTL glyph CID ReversedChars smoke: HarfBuzz "
                "unavailable");
        return;
    }

    const auto logical_text = hebrew_shalom_logical();
    const auto visual_order_text = hebrew_shalom_visual();
    constexpr double font_size = 18.0;
    const auto glyph_run = featherdoc::pdf::shape_pdf_text(
        logical_text,
        featherdoc::pdf::PdfTextShaperOptions{rtl_font, font_size});
    if (!glyph_run.used_harfbuzz || !glyph_run.error_message.empty() ||
        glyph_run.glyphs.empty()) {
        MESSAGE("skipping Hebrew RTL glyph CID ReversedChars smoke: shaping "
                "failed");
        return;
    }

    CHECK_EQ(glyph_run.direction,
             featherdoc::pdf::PdfGlyphDirection::right_to_left);
    CHECK_EQ(glyph_run.script_tag, "Hebr");

    const auto cid_entries = glyph_cid_entries_from_run(glyph_run);
    const auto output_path =
        std::filesystem::current_path() /
        "featherdoc-hebrew-rtl-glyph-cid-reversedchars.pdf";

    std::string error_message;
    REQUIRE_MESSAGE(write_hebrew_rtl_glyph_cid_fixture(
                        output_path, rtl_font, glyph_run, cid_entries,
                        logical_text, true, error_message),
                    error_message);

    const auto pdf_bytes = read_file_bytes(output_path);
    CHECK_NE(pdf_bytes.find("/FeatherDocGlyph"), std::string::npos);
    CHECK_NE(pdf_bytes.find("/CIDToGIDMap"), std::string::npos);
    CHECK_NE(pdf_bytes.find("/ToUnicode"), std::string::npos);
    CHECK_NE(pdf_bytes.find("/Identity-H"), std::string::npos);

    const auto streams = inflated_pdf_streams(pdf_bytes);
    const auto content_stream = find_actual_text_content_stream(streams);
    REQUIRE_FALSE(content_stream.empty());
    CHECK_NE(content_stream.find("/ReversedChars BMC"), std::string::npos);
    CHECK_NE(content_stream.find("/ActualText <FEFF" +
                                 utf16be_hex_payload_from_utf8(logical_text) +
                                 ">"),
             std::string::npos);
    CHECK_NE(content_stream.find(cid_tj_command(glyph_run.glyphs.size())),
             std::string::npos);

    featherdoc::pdf::PdfiumParser parser;
    const auto parse_result = parser.parse(output_path, {});
    REQUIRE_MESSAGE(parse_result.success, parse_result.error_message);

    const auto extracted_text = collect_text(parse_result.document);
    CHECK_EQ(count_occurrences(extracted_text, logical_text), 0U);
    CHECK_EQ(count_occurrences(extracted_text, visual_order_text), 1U);
}

TEST_CASE("PDFium Hebrew extraction for positioned RTL glyph-id CID is "
          "documented") {
    const auto rtl_font = find_rtl_font();
    if (rtl_font.empty()) {
        MESSAGE("skipping positioned Hebrew RTL glyph CID smoke: configure "
                "test RTL font");
        return;
    }
    if (!featherdoc::pdf::pdf_text_shaper_has_harfbuzz()) {
        MESSAGE("skipping positioned Hebrew RTL glyph CID smoke: HarfBuzz "
                "unavailable");
        return;
    }

    const auto logical_text = hebrew_shalom_logical();
    const auto visual_order_text = hebrew_shalom_visual();
    constexpr double font_size = 18.0;
    const auto glyph_run = featherdoc::pdf::shape_pdf_text(
        logical_text,
        featherdoc::pdf::PdfTextShaperOptions{rtl_font, font_size});
    if (!glyph_run.used_harfbuzz || !glyph_run.error_message.empty() ||
        glyph_run.glyphs.empty()) {
        MESSAGE("skipping positioned Hebrew RTL glyph CID smoke: shaping "
                "failed");
        return;
    }

    CHECK_EQ(glyph_run.direction,
             featherdoc::pdf::PdfGlyphDirection::right_to_left);
    CHECK_EQ(glyph_run.script_tag, "Hebr");

    const auto cid_entries = glyph_cid_entries_from_run(glyph_run);
    const auto output_path =
        std::filesystem::current_path() /
        "featherdoc-hebrew-rtl-glyph-cid-positioned-logical.pdf";

    std::string error_message;
    REQUIRE_MESSAGE(write_hebrew_rtl_glyph_cid_positioned_logical_fixture(
                        output_path, rtl_font, glyph_run, cid_entries,
                        logical_text, error_message),
                    error_message);

    const auto pdf_bytes = read_file_bytes(output_path);
    CHECK_NE(pdf_bytes.find("/FeatherDocGlyph"), std::string::npos);
    CHECK_NE(pdf_bytes.find("/CIDToGIDMap"), std::string::npos);
    CHECK_NE(pdf_bytes.find("/ToUnicode"), std::string::npos);
    CHECK_NE(pdf_bytes.find("/Identity-H"), std::string::npos);

    const auto streams = inflated_pdf_streams(pdf_bytes);
    const auto content_stream = find_actual_text_content_stream(streams);
    REQUIRE_FALSE(content_stream.empty());
    CHECK_NE(content_stream.find("/ActualText <FEFF" +
                                 utf16be_hex_payload_from_utf8(logical_text) +
                                 ">"),
             std::string::npos);
    CHECK_GE(count_occurrences(content_stream, " Tm\n"),
             glyph_run.glyphs.size());
    CHECK_EQ(count_occurrences(content_stream, "> Tj\n"),
             glyph_run.glyphs.size());

    const auto cmap = find_shaped_to_unicode_cmap(streams);
    REQUIRE_MESSAGE(!cmap.empty(),
                    "positioned RTL glyph CID ToUnicode CMap not found");

    featherdoc::pdf::PdfiumParser parser;
    const auto parse_result = parser.parse(output_path, {});
    REQUIRE_MESSAGE(parse_result.success, parse_result.error_message);

    const auto extracted_text = collect_text(parse_result.document);
    CHECK_EQ(count_occurrences(extracted_text, logical_text), 0U);
    CHECK_EQ(count_occurrences(extracted_text, visual_order_text), 1U);
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
