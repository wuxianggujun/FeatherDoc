#include "pdf_document_adapter_text.hpp"

#include <algorithm>
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
                              bool italic, bool underline) {
    return same_font(fragment.font, font) &&
           fragment.font_size_points == font_size_points &&
           fragment.fill_color.red == fill_color.red &&
           fragment.fill_color.green == fill_color.green &&
           fragment.fill_color.blue == fill_color.blue &&
           fragment.bold == bold && fragment.italic == italic &&
           fragment.underline == underline;
}

void append_fragment(LineState &line, std::string_view text,
                     const PdfResolvedFont &font, double font_size_points,
                     const PdfRgbColor &fill_color, bool bold, bool italic,
                     bool underline) {
    if (text.empty()) {
        return;
    }

    if (!line.fragments.empty() &&
        same_style(line.fragments.back(), font, font_size_points, fill_color,
                   bold, italic, underline)) {
        line.fragments.back().text.append(text);
    } else {
        line.fragments.push_back(TextFragment{
            std::string{text},
            font,
            font_size_points,
            fill_color,
            bold,
            italic,
            underline,
        });
    }
    line.width_points += measure_text(text, font_size_points, font);
    line.height_points =
        std::max(line.height_points,
                 estimate_line_height_points(font_size_points,
                                             metrics_options_for(font)));
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
    bool italic, bool underline) {
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
                        bold, italic, underline);
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

std::vector<TextToken> tokenize_run_text(std::string_view text,
                                         const ResolvedRunStyle &style) {
    std::vector<TextToken> tokens;
    for (std::size_t index = 0U; index < text.size();) {
        const auto value = static_cast<unsigned char>(text[index]);
        if (value == '\n' || value == '\r') {
            tokens.push_back(TextToken{
                {},
                false,
                true,
                style.font,
                style.font_size_points,
                style.fill_color,
                style.bold,
                style.italic,
                style.underline,
            });
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
            tokens.push_back(TextToken{
                std::string{text.substr(token_start, index - token_start)},
                true,
                false,
                style.font,
                style.font_size_points,
                style.fill_color,
                style.bold,
                style.italic,
                style.underline,
            });
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
        tokens.push_back(TextToken{
            std::string{text.substr(token_start, index - token_start)},
            false,
            false,
            style.font,
            style.font_size_points,
            style.fill_color,
            style.bold,
            style.italic,
            style.underline,
        });
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
                               token.underline);
            pending_spaces.clear();
            continue;
        }

        if (!current.empty()) {
            for (const auto &space : pending_spaces) {
                append_fragment(current, space.text, space.font,
                                space.font_size_points, space.fill_color,
                                space.bold, space.italic, space.underline);
            }
        }
        pending_spaces.clear();
        append_fragment(current, token.text, token.font, token.font_size_points,
                        token.fill_color, token.bold, token.italic,
                        token.underline);
    }

    if (!current.empty() || lines.empty()) {
        lines.push_back(std::move(current));
    }

    return lines;
}

} // namespace featherdoc::pdf::detail
