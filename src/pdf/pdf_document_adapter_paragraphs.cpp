#include "pdf_document_adapter_paragraphs.hpp"

#include "pdf_document_adapter_render.hpp"

#include <optional>
#include <string>
#include <utility>

namespace featherdoc::pdf::detail {
namespace {

[[nodiscard]] std::optional<std::string>
first_present(std::optional<std::string> first,
              std::optional<std::string> second,
              std::optional<std::string> third) {
    if (first && !first->empty()) {
        return first;
    }
    if (second && !second->empty()) {
        return second;
    }
    if (third && !third->empty()) {
        return third;
    }
    return std::nullopt;
}

[[nodiscard]] std::optional<featherdoc::resolved_style_properties_summary>
resolve_run_style_properties(featherdoc::Document &document,
                             const featherdoc::run_inspection_summary &run) {
    if (!run.style_id || run.style_id->empty()) {
        return std::nullopt;
    }
    return document.resolve_style_properties(*run.style_id);
}

[[nodiscard]] PdfGlyphDirection
shaping_direction_from_rtl(bool rtl) noexcept {
    return rtl ? PdfGlyphDirection::right_to_left
               : PdfGlyphDirection::unknown;
}

[[nodiscard]] ResolvedRunStyle
resolve_plain_text_style(featherdoc::Document &document, std::string_view text,
                         const PdfDocumentAdapterOptions &options,
                         const PdfFontResolver &resolver) {
    const auto font_family = document.default_run_font_family().value_or(
        options.font_family.empty() ? std::string{"Helvetica"}
                                    : options.font_family);
    const auto east_asia_font_family =
        document.default_run_east_asia_font_family().value_or(std::string{});

    return ResolvedRunStyle{
        font_family,
        east_asia_font_family,
        resolver.resolve(font_family, east_asia_font_family, text, false,
                         false),
        options.font_size_points,
        PdfRgbColor{0.0, 0.0, 0.0},
        false,
        false,
        false,
        shaping_direction_from_rtl(document.default_run_rtl().value_or(false)),
        {},
    };
}

[[nodiscard]] std::optional<featherdoc::numbering_level_definition>
find_numbering_level(
    featherdoc::Document &document,
    const featherdoc::paragraph_inspection_summary &paragraph) {
    if (!paragraph.numbering || !paragraph.numbering->definition_id ||
        !paragraph.numbering->level) {
        return std::nullopt;
    }

    const auto definition =
        document.find_numbering_definition(*paragraph.numbering->definition_id);
    if (!definition.has_value()) {
        return std::nullopt;
    }

    for (const auto &level : definition->levels) {
        if (level.level == *paragraph.numbering->level) {
            return level;
        }
    }
    return std::nullopt;
}

[[nodiscard]] std::string
numbering_prefix_text(featherdoc::Document &document,
                      const featherdoc::paragraph_inspection_summary &paragraph,
                      ParagraphNumberingState &numbering_state) {
    if (!paragraph.numbering || !paragraph.numbering->num_id ||
        !paragraph.numbering->level) {
        return {};
    }

    const auto level_definition = find_numbering_level(document, paragraph);
    if (!level_definition.has_value()) {
        return {};
    }

    if (level_definition->kind == featherdoc::list_kind::bullet) {
        return level_definition->text_pattern.empty()
                   ? std::string{"\xE2\x80\xA2\t"}
                   : level_definition->text_pattern + "\t";
    }

    auto &levels = numbering_state.counters[*paragraph.numbering->num_id];
    const auto current_level = *paragraph.numbering->level;
    auto &counter = levels[current_level];
    if (counter == 0U) {
        counter = level_definition->start;
    } else {
        ++counter;
    }
    for (auto iterator = levels.begin(); iterator != levels.end();) {
        if (iterator->first > current_level) {
            iterator = levels.erase(iterator);
        } else {
            ++iterator;
        }
    }

    auto prefix = level_definition->text_pattern.empty()
                      ? std::string{"%1."}
                      : level_definition->text_pattern;
    for (std::uint32_t level = 0U; level <= current_level; ++level) {
        const auto placeholder = "%" + std::to_string(level + 1U);
        const auto counter_entry = levels.find(level);
        const auto value = counter_entry != levels.end()
                               ? counter_entry->second
                               : level_definition->start;
        for (auto position = prefix.find(placeholder);
             position != std::string::npos;
             position = prefix.find(placeholder,
                                    position + std::to_string(value).size())) {
            prefix.replace(position, placeholder.size(), std::to_string(value));
        }
    }
    if (prefix.find('%') != std::string::npos) {
        prefix = std::to_string(counter) + ".";
    }
    return prefix + "\t";
}

void prepend_text_tokens(std::vector<TextToken> &tokens,
                         std::vector<TextToken> prefix_tokens) {
    if (prefix_tokens.empty()) {
        return;
    }

    prefix_tokens.insert(prefix_tokens.end(),
                         std::make_move_iterator(tokens.begin()),
                         std::make_move_iterator(tokens.end()));
    tokens = std::move(prefix_tokens);
}

} // namespace

[[nodiscard]] ResolvedRunStyle
resolve_run_style(featherdoc::Document &document,
                  const featherdoc::run_inspection_summary &run,
                  const PdfDocumentAdapterOptions &options,
                  const PdfFontResolver &resolver) {
    const auto style_properties = resolve_run_style_properties(document, run);
    const auto style_font_family = style_properties
                                       ? style_properties->run_font_family.value
                                       : std::optional<std::string>{};
    const auto style_east_asia_font_family =
        style_properties ? style_properties->run_east_asia_font_family.value
                         : std::optional<std::string>{};

    const auto font_family =
        first_present(run.font_family, style_font_family,
                      document.default_run_font_family())
            .value_or(options.font_family.empty() ? std::string{"Helvetica"}
                                                  : options.font_family);

    const auto east_asia_font_family =
        first_present(run.east_asia_font_family, style_east_asia_font_family,
                      document.default_run_east_asia_font_family())
            .value_or(std::string{});

    const bool bold =
        run.bold.value_or(style_properties && style_properties->run_bold.value
                              ? *style_properties->run_bold.value
                              : false);
    const bool italic = run.italic.value_or(
        style_properties && style_properties->run_italic.value
            ? *style_properties->run_italic.value
            : false);
    const bool underline = run.underline.value_or(
        style_properties && style_properties->run_underline.value
            ? *style_properties->run_underline.value
            : false);
    const bool rtl = run.rtl.value_or(
        style_properties && style_properties->run_rtl.value
            ? *style_properties->run_rtl.value
            : document.default_run_rtl().value_or(false));

    const auto text_color =
        first_present(run.text_color,
                      style_properties ? style_properties->run_text_color.value
                                       : std::optional<std::string>{},
                      std::nullopt);

    return ResolvedRunStyle{
        font_family,
        east_asia_font_family,
        resolver.resolve(font_family, east_asia_font_family, run.text, bold,
                         italic),
        run.font_size_points.value_or(
            style_properties && style_properties->run_font_size_points.value
                ? *style_properties->run_font_size_points.value
                : options.font_size_points),
        text_color ? parse_hex_rgb_color(*text_color)
                         .value_or(PdfRgbColor{0.0, 0.0, 0.0})
                   : PdfRgbColor{0.0, 0.0, 0.0},
        bold,
        italic,
        underline,
        shaping_direction_from_rtl(rtl),
        {},
    };
}

[[nodiscard]] bool lines_contain_text(const std::vector<LineState> &lines) {
    for (const auto &line : lines) {
        for (const auto &fragment : line.fragments) {
            if (!fragment.text.empty()) {
                return true;
            }
        }
    }
    return false;
}

[[nodiscard]] std::vector<LineState>
wrap_plain_text(featherdoc::Document &document, std::string_view text,
                const PdfDocumentAdapterOptions &options,
                const PdfFontResolver &resolver, double max_width_points) {
    if (text.empty()) {
        return {LineState{}};
    }

    const auto style =
        resolve_plain_text_style(document, text, options, resolver);
    const auto tokens = tokenize_run_text(text, style);
    if (tokens.empty()) {
        return {LineState{}};
    }
    return wrap_run_tokens(tokens, max_width_points);
}

[[nodiscard]] std::vector<TextToken>
paragraph_text_tokens(featherdoc::Document &document,
                      const featherdoc::paragraph_inspection_summary &paragraph,
                      const PdfDocumentAdapterOptions &options,
                      const PdfFontResolver &resolver,
                      ParagraphNumberingState &numbering_state) {
    std::vector<TextToken> tokens;
    const auto runs = document.inspect_paragraph_runs(paragraph.index);
    const auto prefix =
        numbering_prefix_text(document, paragraph, numbering_state);
    if (runs.empty()) {
        if (prefix.empty()) {
            return {};
        }

        const auto style =
            resolve_plain_text_style(document, prefix, options, resolver);
        return tokenize_run_text(prefix, style);
    }

    for (const auto &run : runs) {
        if (run.text.empty()) {
            continue;
        }
        const auto style = resolve_run_style(document, run, options, resolver);
        auto run_tokens = tokenize_run_text(run.text, style);
        tokens.insert(tokens.end(), std::make_move_iterator(run_tokens.begin()),
                      std::make_move_iterator(run_tokens.end()));
    }

    if (!prefix.empty()) {
        const auto style =
            resolve_plain_text_style(document, prefix, options, resolver);
        prepend_text_tokens(tokens, tokenize_run_text(prefix, style));
    }

    return tokens;
}

[[nodiscard]] std::vector<LineState>
wrap_paragraph_runs(featherdoc::Document &document,
                    const featherdoc::paragraph_inspection_summary &paragraph,
                    const PdfDocumentAdapterOptions &options,
                    const PdfFontResolver &resolver, double max_width_points,
                    ParagraphNumberingState &numbering_state) {
    const auto tokens = paragraph_text_tokens(document, paragraph, options,
                                             resolver, numbering_state);
    if (tokens.empty()) {
        return {LineState{}};
    }

    return wrap_run_tokens(tokens, max_width_points);
}

} // namespace featherdoc::pdf::detail
