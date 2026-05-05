#include <featherdoc/pdf/pdf_document_adapter.hpp>

#include <algorithm>
#include <cctype>
#include <iterator>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace featherdoc::pdf {
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

[[nodiscard]] bool contains_non_ascii(std::string_view text) noexcept {
    return std::any_of(text.begin(), text.end(), [](unsigned char value) {
        return (value & 0x80U) != 0U;
    });
}

[[nodiscard]] double estimate_text_width_points(std::string_view text,
                                                double font_size_points) {
    double units = 0.0;
    for (std::size_t index = 0U; index < text.size();) {
        const auto value = static_cast<unsigned char>(text[index]);
        const auto codepoint_size = utf8_codepoint_size(value);
        if (codepoint_size > 1U) {
            units += 1.0;
            index += std::min(codepoint_size, text.size() - index);
            continue;
        }

        if (value == ' ') {
            units += 0.33;
        } else if (std::isdigit(value) != 0) {
            units += 0.52;
        } else if (std::ispunct(value) != 0) {
            units += 0.35;
        } else {
            units += 0.55;
        }
        ++index;
    }
    return units * font_size_points;
}

[[nodiscard]] std::vector<std::string> split_words(std::string_view text) {
    std::vector<std::string> words;
    std::size_t index = 0U;
    while (index < text.size()) {
        while (index < text.size() &&
               is_ascii_space(static_cast<unsigned char>(text[index]))) {
            ++index;
        }

        const auto word_start = index;
        while (index < text.size() &&
               !is_ascii_space(static_cast<unsigned char>(text[index]))) {
            index +=
                utf8_codepoint_size(static_cast<unsigned char>(text[index]));
        }

        if (word_start < index) {
            words.emplace_back(text.substr(word_start, index - word_start));
        }
    }
    return words;
}

[[nodiscard]] std::vector<std::string> wrap_long_word(std::string_view word,
                                                      double max_width_points,
                                                      double font_size_points) {
    std::vector<std::string> lines;
    std::string current;
    for (std::size_t index = 0U; index < word.size();) {
        const auto codepoint_size = std::min(
            utf8_codepoint_size(static_cast<unsigned char>(word[index])),
            word.size() - index);
        auto candidate = current;
        candidate.append(word.substr(index, codepoint_size));
        if (!current.empty() &&
            estimate_text_width_points(candidate, font_size_points) >
                max_width_points) {
            lines.push_back(std::move(current));
            current.clear();
        }
        current.append(word.substr(index, codepoint_size));
        index += codepoint_size;
    }

    if (!current.empty()) {
        lines.push_back(std::move(current));
    }
    return lines;
}

[[nodiscard]] std::vector<std::string>
wrap_paragraph_text(std::string_view text, double max_width_points,
                    double font_size_points) {
    if (text.empty()) {
        return {""};
    }

    std::vector<std::string> lines;
    std::string current;
    for (const auto &word : split_words(text)) {
        const auto word_width =
            estimate_text_width_points(word, font_size_points);
        if (word_width > max_width_points) {
            if (!current.empty()) {
                lines.push_back(std::move(current));
                current.clear();
            }
            auto broken =
                wrap_long_word(word, max_width_points, font_size_points);
            lines.insert(lines.end(), std::make_move_iterator(broken.begin()),
                         std::make_move_iterator(broken.end()));
            continue;
        }

        auto candidate = current.empty() ? word : current + " " + word;
        if (!current.empty() &&
            estimate_text_width_points(candidate, font_size_points) >
                max_width_points) {
            lines.push_back(std::move(current));
            current = word;
            continue;
        }

        current = std::move(candidate);
    }

    if (!current.empty()) {
        lines.push_back(std::move(current));
    }
    if (lines.empty()) {
        lines.push_back("");
    }
    return lines;
}

[[nodiscard]] double content_width(const PdfDocumentAdapterOptions &options) {
    const auto width = options.page_size.width_points -
                       options.margin_left_points - options.margin_right_points;
    return std::max(1.0, width);
}

[[nodiscard]] double
first_baseline_y(const PdfDocumentAdapterOptions &options) {
    return options.page_size.height_points - options.margin_top_points -
           options.font_size_points;
}

} // namespace

PdfDocumentLayout
layout_document_paragraphs(featherdoc::Document &document,
                           const PdfDocumentAdapterOptions &options) {
    PdfDocumentLayout layout;
    layout.metadata = options.metadata;

    auto new_page = [&layout, &options]() -> PdfPageLayout & {
        PdfPageLayout page;
        page.size = options.page_size;
        layout.pages.push_back(std::move(page));
        return layout.pages.back();
    };

    auto *page = &new_page();
    auto current_y = first_baseline_y(options);
    const auto max_width = content_width(options);

    const auto paragraphs = document.inspect_paragraphs();
    for (const auto &paragraph : paragraphs) {
        const auto lines = wrap_paragraph_text(paragraph.text, max_width,
                                               options.font_size_points);
        for (const auto &line : lines) {
            if (current_y < options.margin_bottom_points) {
                page = &new_page();
                current_y = first_baseline_y(options);
            }

            if (!line.empty()) {
                page->text_runs.push_back(PdfTextRun{
                    PdfPoint{options.margin_left_points, current_y},
                    line,
                    options.font_family,
                    options.font_size_points,
                    PdfRgbColor{0.0, 0.0, 0.0},
                    contains_non_ascii(line),
                });
            }
            current_y -= options.line_height_points;
        }
        current_y -= options.paragraph_spacing_after_points;
    }

    return layout;
}

} // namespace featherdoc::pdf
