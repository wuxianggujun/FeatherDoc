#include "pdf_document_adapter_text.hpp"

#include <algorithm>
#include <cmath>
#include <cctype>
#include <cstdint>
#include <functional>
#include <utility>

namespace featherdoc::pdf::detail {
namespace {

[[nodiscard]] bool is_ascii_space(unsigned char value) noexcept {
    return std::isspace(value) != 0;
}

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

[[nodiscard]] std::uint32_t
decode_utf8_codepoint(std::string_view text, std::size_t offset,
                      std::size_t codepoint_size) noexcept {
    if (offset >= text.size()) {
        return 0U;
    }
    const auto remaining = text.size() - offset;
    const auto size = std::min(codepoint_size, remaining);
    const auto lead = static_cast<unsigned char>(text[offset]);
    if (size == 1U) {
        return lead;
    }
    if (size == 2U) {
        return (static_cast<std::uint32_t>(lead & 0x1FU) << 6U) |
               static_cast<std::uint32_t>(
                   static_cast<unsigned char>(text[offset + 1U]) & 0x3FU);
    }
    if (size == 3U) {
        return (static_cast<std::uint32_t>(lead & 0x0FU) << 12U) |
               (static_cast<std::uint32_t>(
                    static_cast<unsigned char>(text[offset + 1U]) & 0x3FU)
                << 6U) |
               static_cast<std::uint32_t>(
                   static_cast<unsigned char>(text[offset + 2U]) & 0x3FU);
    }
    if (size == 4U) {
        return (static_cast<std::uint32_t>(lead & 0x07U) << 18U) |
               (static_cast<std::uint32_t>(
                    static_cast<unsigned char>(text[offset + 1U]) & 0x3FU)
                << 12U) |
               (static_cast<std::uint32_t>(
                    static_cast<unsigned char>(text[offset + 2U]) & 0x3FU)
                << 6U) |
               static_cast<std::uint32_t>(
                   static_cast<unsigned char>(text[offset + 3U]) & 0x3FU);
    }
    return lead;
}

[[nodiscard]] bool is_rtl_codepoint(std::uint32_t codepoint) noexcept {
    return (codepoint >= 0x0590U && codepoint <= 0x08FFU) ||
           (codepoint >= 0xFB1DU && codepoint <= 0xFDFFU) ||
           (codepoint >= 0xFE70U && codepoint <= 0xFEFFU) ||
           (codepoint >= 0x10800U && codepoint <= 0x10FFFD);
}

[[nodiscard]] bool contains_rtl_text(std::string_view text) noexcept {
    for (std::size_t index = 0U; index < text.size();) {
        const auto codepoint_size = std::min(
            utf8_codepoint_size(static_cast<unsigned char>(text[index])),
            text.size() - index);
        if (is_rtl_codepoint(
                decode_utf8_codepoint(text, index, codepoint_size))) {
            return true;
        }
        index += codepoint_size;
    }
    return false;
}

[[nodiscard]] bool same_font(const PdfResolvedFont &left,
                             const PdfResolvedFont &right) {
    return left.font_family == right.font_family &&
           left.font_file_path == right.font_file_path &&
           left.unicode == right.unicode;
}

[[nodiscard]] bool same_style(const TextFragment &fragment,
                              const PdfResolvedFont &font,
                              double font_size_points,
                              const PdfRgbColor &fill_color, bool bold,
                              bool italic, bool underline,
                              double vertical_shift_points,
                              bool strikethrough, bool rtl) {
    return same_font(fragment.font, font) &&
           fragment.font_size_points == font_size_points &&
           fragment.fill_color.red == fill_color.red &&
           fragment.fill_color.green == fill_color.green &&
           fragment.fill_color.blue == fill_color.blue &&
            fragment.bold == bold && fragment.italic == italic &&
           fragment.underline == underline &&
           fragment.vertical_shift_points == vertical_shift_points &&
           fragment.strikethrough == strikethrough && fragment.rtl == rtl;
}

void append_fragment(LineState &line, std::string_view text,
                     const PdfResolvedFont &font, double font_size_points,
                     const PdfRgbColor &fill_color, bool bold, bool italic,
                     bool underline, double vertical_shift_points,
                     bool strikethrough, bool rtl) {
    if (text.empty()) {
        return;
    }

    if (!line.fragments.empty() &&
        same_style(line.fragments.back(), font, font_size_points, fill_color,
                   bold, italic, underline, vertical_shift_points,
                   strikethrough, rtl)) {
        line.fragments.back().text.append(text);
    } else {
        auto fragment = TextFragment{
            std::string{text},
            font,
            font_size_points,
            fill_color,
            bold,
            italic,
            underline,
            vertical_shift_points,
        };
        fragment.strikethrough = strikethrough;
        fragment.rtl = rtl;
        line.fragments.push_back(std::move(fragment));
    }
    line.width_points += measure_text(text, font_size_points, font);
    line.height_points =
        std::max(line.height_points,
                 estimate_line_height_points(font_size_points,
                                             metrics_options_for(font)) +
                     std::abs(vertical_shift_points));
    line.baseline_offset_points =
        std::max(line.baseline_offset_points, font_size_points);
}

[[nodiscard]] double safe_line_width(
    const std::function<double(std::size_t)> &max_width_for_line,
    std::size_t line_index) {
    return std::max(1.0, max_width_for_line(line_index));
}

void append_broken_word(
    std::vector<LineState> &lines, std::string_view word,
    const PdfResolvedFont &font,
    const std::function<double(std::size_t)> &max_width_for_line,
    double font_size_points, const PdfRgbColor &fill_color, bool bold,
    bool italic, bool underline, double vertical_shift_points,
    bool strikethrough, bool rtl) {
    LineState current;
    for (std::size_t index = 0U; index < word.size();) {
        const auto codepoint_size = std::min(
            utf8_codepoint_size(static_cast<unsigned char>(word[index])),
            word.size() - index);
        const auto codepoint = word.substr(index, codepoint_size);
        const auto max_width_points =
            safe_line_width(max_width_for_line, lines.size());
        const auto candidate_width =
            current.width_points +
            measure_text(codepoint, font_size_points, font);
        if (!current.empty() && candidate_width > max_width_points) {
            lines.push_back(std::move(current));
            current = {};
        }
        append_fragment(current, codepoint, font, font_size_points, fill_color,
                        bold, italic, underline, vertical_shift_points,
                        strikethrough, rtl);
        index += codepoint_size;
    }

    if (!current.empty()) {
        lines.push_back(std::move(current));
    }
}

} // namespace

PdfTextMetricsOptions metrics_options_for(const PdfResolvedFont &font) {
    return PdfTextMetricsOptions{
        font.font_family,
        font.font_file_path,
    };
}

double measure_text(std::string_view text, double font_size_points,
                    const PdfResolvedFont &font) {
    return measure_text_width_points(text, font_size_points,
                                     metrics_options_for(font));
}

double line_height_points_for(const LineState &line, double fallback_points,
                              double default_font_size_points) {
    if (line.baseline_offset_points <= default_font_size_points ||
        line.height_points <= 0.0) {
        return fallback_points;
    }
    return std::max(fallback_points, line.height_points);
}

double line_baseline_offset_points_for(
    const LineState &line, double default_font_size_points) noexcept {
    if (line.baseline_offset_points <= 0.0) {
        return default_font_size_points;
    }
    return std::max(default_font_size_points, line.baseline_offset_points);
}

double content_height_points_for(const std::vector<LineState> &lines,
                                 double fallback_line_height_points,
                                 double default_font_size_points) {
    auto height = 0.0;
    for (const auto &line : lines) {
        height += line_height_points_for(line, fallback_line_height_points,
                                         default_font_size_points);
    }
    return height;
}

bool line_contains_rtl_fragments(const LineState &line) noexcept {
    return std::any_of(line.fragments.begin(), line.fragments.end(),
                       [](const TextFragment &fragment) {
                           return fragment.rtl;
                       });
}

std::vector<TextToken> tokenize_run_text(std::string_view text,
                                         const ResolvedRunStyle &style) {
    std::vector<TextToken> tokens;
    for (std::size_t index = 0U; index < text.size();) {
        const auto value = static_cast<unsigned char>(text[index]);
        if (value == '\n' || value == '\r') {
            auto token = TextToken{
                {},
                false,
                true,
                style.font,
                style.font_size_points,
                style.fill_color,
                style.bold,
                style.italic,
                style.underline,
                style.vertical_shift_points,
            };
            token.strikethrough = style.strikethrough;
            token.rtl = style.rtl;
            tokens.push_back(std::move(token));
            ++index;
            if (value == '\r' && index < text.size() && text[index] == '\n') {
                ++index;
            }
            continue;
        }

        const auto token_start = index;
        if (is_ascii_space(value)) {
            while (index < text.size()) {
                const auto current = static_cast<unsigned char>(text[index]);
                if (!is_ascii_space(current) || current == '\n' ||
                    current == '\r') {
                    break;
                }
                ++index;
            }
            auto token = TextToken{
                std::string{text.substr(token_start, index - token_start)},
                true,
                false,
                style.font,
                style.font_size_points,
                style.fill_color,
                style.bold,
                style.italic,
                style.underline,
                style.vertical_shift_points,
            };
            token.strikethrough = style.strikethrough;
            token.rtl = style.rtl;
            tokens.push_back(std::move(token));
            continue;
        }

        while (index < text.size()) {
            const auto current = static_cast<unsigned char>(text[index]);
            if (is_ascii_space(current)) {
                break;
            }
            index +=
                std::min(utf8_codepoint_size(current), text.size() - index);
        }
        auto token = TextToken{
            std::string{text.substr(token_start, index - token_start)},
            false,
            false,
            style.font,
            style.font_size_points,
            style.fill_color,
            style.bold,
            style.italic,
            style.underline,
            style.vertical_shift_points,
        };
        token.strikethrough = style.strikethrough;
        token.rtl = style.rtl || contains_rtl_text(token.text);
        tokens.push_back(std::move(token));
    }
    return tokens;
}

std::vector<LineState> wrap_run_tokens(const std::vector<TextToken> &tokens,
                                       double max_width_points) {
    return wrap_run_tokens_with_line_widths(
        tokens, [max_width_points](std::size_t) { return max_width_points; });
}

std::vector<LineState> wrap_run_tokens_with_line_widths(
    const std::vector<TextToken> &tokens,
    const std::function<double(std::size_t)> &max_width_for_line) {
    std::vector<LineState> lines;
    LineState current;
    std::vector<TextToken> pending_spaces;

    auto flush_current = [&]() {
        lines.push_back(std::move(current));
        current = {};
        pending_spaces.clear();
    };

    for (const auto &token : tokens) {
        if (token.hard_break) {
            flush_current();
            continue;
        }

        if (token.space) {
            if (!current.empty()) {
                pending_spaces.push_back(token);
            }
            continue;
        }

        const auto token_width =
            measure_text(token.text, token.font_size_points, token.font);
        double pending_width = 0.0;
        for (const auto &space : pending_spaces) {
            pending_width +=
                measure_text(space.text, space.font_size_points, space.font);
        }

        const auto max_width_points =
            safe_line_width(max_width_for_line, lines.size());
        if (!current.empty() &&
            current.width_points + pending_width + token_width >
                max_width_points) {
            flush_current();
        }

        if (token_width > max_width_points) {
            if (!current.empty()) {
                flush_current();
            }
            append_broken_word(lines, token.text, token.font,
                               max_width_for_line, token.font_size_points,
                               token.fill_color, token.bold, token.italic,
                               token.underline, token.vertical_shift_points,
                               token.strikethrough, token.rtl);
            pending_spaces.clear();
            continue;
        }

        if (!current.empty()) {
            for (const auto &space : pending_spaces) {
                append_fragment(current, space.text, space.font,
                                space.font_size_points, space.fill_color,
                                space.bold, space.italic, space.underline,
                                space.vertical_shift_points,
                                space.strikethrough, space.rtl);
            }
        }
        pending_spaces.clear();
        append_fragment(current, token.text, token.font, token.font_size_points,
                        token.fill_color, token.bold, token.italic,
                        token.underline, token.vertical_shift_points,
                        token.strikethrough, token.rtl);
    }

    if (!current.empty() || lines.empty()) {
        lines.push_back(std::move(current));
    }

    return lines;
}

} // namespace featherdoc::pdf::detail
