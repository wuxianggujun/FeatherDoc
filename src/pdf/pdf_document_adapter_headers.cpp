#include "pdf_document_adapter_headers.hpp"

#include "pdf_document_adapter_render.hpp"

#include <algorithm>
#include <cstddef>
#include <optional>
#include <string>
#include <utility>

namespace featherdoc::pdf::detail {
namespace {

[[nodiscard]] PdfDocumentAdapterOptions
header_footer_options(const PdfDocumentAdapterOptions &options) {
    auto header_footer_options = options;
    header_footer_options.font_size_points =
        options.header_footer_font_size_points > 0.0
            ? options.header_footer_font_size_points
            : options.font_size_points;
    header_footer_options.line_height_points =
        std::max(header_footer_options.font_size_points * 1.2,
                 header_footer_options.font_size_points + 2.0);
    header_footer_options.paragraph_spacing_after_points = 0.0;
    return header_footer_options;
}

void replace_all(std::string &text, std::string_view needle,
                 std::string_view replacement) {
    if (needle.empty()) {
        return;
    }

    for (auto position = text.find(needle); position != std::string::npos;
         position = text.find(needle, position + replacement.size())) {
        text.replace(position, needle.size(), replacement.data(),
                     replacement.size());
    }
}

[[nodiscard]] std::string expand_page_placeholders(
    std::string text, std::size_t document_page_index,
    std::size_t document_page_count, std::size_t section_page_index,
    std::size_t section_page_count) {
    replace_all(text, "{{page}}", std::to_string(document_page_index + 1U));
    replace_all(text, "{{total_pages}}", std::to_string(document_page_count));
    replace_all(text, "{{section_page}}",
                std::to_string(section_page_index + 1U));
    replace_all(text, "{{section_total_pages}}",
                std::to_string(section_page_count));
    return text;
}

[[nodiscard]] LineState expand_line_page_placeholders(
    LineState line, std::size_t document_page_index,
    std::size_t document_page_count, std::size_t section_page_index,
    std::size_t section_page_count) {
    for (auto &fragment : line.fragments) {
        fragment.text = expand_page_placeholders(
            std::move(fragment.text), document_page_index, document_page_count,
            section_page_index, section_page_count);
    }
    return line;
}

[[nodiscard]] double paragraph_alignment_offset_points(
    featherdoc::paragraph_alignment alignment, double line_width_points,
    double max_width_points) noexcept {
    const auto extra_width = std::max(0.0, max_width_points - line_width_points);
    if (alignment == featherdoc::paragraph_alignment::center) {
        return extra_width / 2.0;
    }
    if (alignment == featherdoc::paragraph_alignment::right) {
        return extra_width;
    }
    return 0.0;
}

[[nodiscard]] std::vector<HeaderFooterLineLayout> wrap_header_footer_paragraphs(
    featherdoc::Paragraph paragraph, const PdfDocumentAdapterOptions &options,
    double max_width_points, const HeaderFooterRenderContext &context) {
    if (!paragraph.has_next() || !context.wrap_paragraph) {
        return {};
    }

    std::vector<HeaderFooterLineLayout> lines;
    for (; paragraph.has_next(); paragraph.next()) {
        auto paragraph_lines = context.wrap_paragraph(
            paragraph, header_footer_options(options), max_width_points);
        if (paragraph_lines.empty()) {
            paragraph_lines.push_back(LineState{});
        }
        const auto alignment =
            paragraph.alignment().value_or(featherdoc::paragraph_alignment::left);
        for (auto &line : paragraph_lines) {
            const auto line_width_points = line.width_points;
            lines.push_back(HeaderFooterLineLayout{
                std::move(line),
                options.margin_left_points +
                    paragraph_alignment_offset_points(
                        alignment, line_width_points,
                        max_width_points),
            });
        }
    }
    return lines;
}

[[nodiscard]] HeaderFooterPageLayout build_header_footer_page_layout(
    featherdoc::Document &document,
    std::optional<std::size_t> header_section_index,
    std::optional<std::size_t> footer_section_index,
    featherdoc::section_reference_kind reference_kind,
    const PdfDocumentAdapterOptions &options, double max_width_points,
    const HeaderFooterRenderContext &context) {
    HeaderFooterPageLayout layout;

    if (header_section_index.has_value()) {
        auto header = document.section_header_paragraphs(*header_section_index,
                                                         reference_kind);
        layout.header_lines = wrap_header_footer_paragraphs(
            header, options, max_width_points, context);
    }

    if (footer_section_index.has_value()) {
        auto footer = document.section_footer_paragraphs(*footer_section_index,
                                                         reference_kind);
        layout.footer_lines = wrap_header_footer_paragraphs(
            footer, options, max_width_points, context);
    }

    return layout;
}

void emit_header_footer_lines(PdfPageLayout &page,
                              const std::vector<HeaderFooterLineLayout> &lines,
                              double start_baseline_y, double step_points,
                              const PdfDocumentAdapterOptions &options,
                              std::size_t document_page_index,
                              std::size_t document_page_count,
                              std::size_t section_page_count) {
    auto baseline_y = start_baseline_y;
    for (const auto &line_layout : lines) {
        if (!line_layout.line.empty()) {
            if (options.expand_header_footer_page_placeholders) {
                emit_line_at(page,
                             expand_line_page_placeholders(
                                 line_layout.line, document_page_index,
                                 document_page_count,
                                 page.section_page_index, section_page_count),
                             line_layout.start_x_points, baseline_y);
            } else {
                emit_line_at(page, line_layout.line, line_layout.start_x_points,
                             baseline_y);
            }
        }
        baseline_y += step_points;
    }
}

[[nodiscard]] const HeaderFooterPageLayout &
select_header_footer_page_layout(const HeaderFooterLayout &header_footer,
                                 const PdfPageLayout &page,
                                 std::size_t document_page_index) {
    if (header_footer.different_first_page_enabled &&
        page.section_page_index == 0U) {
        return header_footer.first_page;
    }

    const auto is_document_even_page = (document_page_index + 1U) % 2U == 0U;
    if (header_footer.even_and_odd_headers_enabled && is_document_even_page) {
        return header_footer.even_page;
    }

    return header_footer.default_page;
}

} // namespace

[[nodiscard]] double
header_footer_line_height_points(const PdfDocumentAdapterOptions &options) {
    return header_footer_options(options).line_height_points;
}

[[nodiscard]] HeaderFooterLayout build_header_footer_layout(
    featherdoc::Document &document, std::size_t section_index,
    const PdfDocumentAdapterOptions &options, double max_width_points,
    const HeaderFooterRenderContext &context) {
    HeaderFooterLayout layout;
    if (!options.render_headers_and_footers ||
        section_index >= document.section_count()) {
        return layout;
    }

    const auto section = document.inspect_section(section_index);
    if (!section.has_value()) {
        return layout;
    }

    layout.different_first_page_enabled = section->different_first_page_enabled;
    layout.even_and_odd_headers_enabled =
        section->even_and_odd_headers_enabled.value_or(false);
    layout.default_page = build_header_footer_page_layout(
        document, section->header.resolved_default_section_index,
        section->footer.resolved_default_section_index,
        featherdoc::section_reference_kind::default_reference, options,
        max_width_points, context);
    if (layout.different_first_page_enabled) {
        layout.first_page = build_header_footer_page_layout(
            document, section->header.resolved_first_section_index,
            section->footer.resolved_first_section_index,
            featherdoc::section_reference_kind::first_page, options,
            max_width_points, context);
    }
    if (layout.even_and_odd_headers_enabled) {
        layout.even_page = build_header_footer_page_layout(
            document, section->header.resolved_even_section_index,
            section->footer.resolved_even_section_index,
            featherdoc::section_reference_kind::even_page, options,
            max_width_points, context);
    }

    return layout;
}

void emit_headers_and_footers(
    PdfDocumentLayout &layout, const std::vector<HeaderFooterLayout> &headers,
    const std::vector<PdfDocumentAdapterOptions> &section_options,
    const std::vector<double> &line_height_points) {
    if (headers.empty() || section_options.empty() ||
        line_height_points.empty()) {
        return;
    }

    std::vector<std::size_t> section_page_counts(section_options.size(), 0U);
    for (const auto &page : layout.pages) {
        const auto section_index =
            std::min(page.section_index, section_options.size() - 1U);
        ++section_page_counts[section_index];
    }
    const auto document_page_count = layout.pages.size();

    for (std::size_t page_index = 0U; page_index < layout.pages.size();
         ++page_index) {
        auto &page = layout.pages[page_index];
        const auto section_index =
            std::min(page.section_index, section_options.size() - 1U);
        const auto &options = section_options[section_index];
        if (!options.render_headers_and_footers) {
            continue;
        }

        const auto &header_footer =
            headers[std::min(section_index, headers.size() - 1U)];
        const auto &page_layout =
            select_header_footer_page_layout(header_footer, page, page_index);
        const auto step_points = line_height_points[std::min(
            section_index, line_height_points.size() - 1U)];
        const auto header_y =
            page.size.height_points - options.header_y_offset_points;
        const auto footer_y = options.footer_y_offset_points;
        const auto section_page_count =
            section_page_counts[std::min(section_index,
                                         section_page_counts.size() - 1U)];
        emit_header_footer_lines(page, page_layout.header_lines, header_y,
                                 -step_points, options, page_index,
                                 document_page_count, section_page_count);
        emit_header_footer_lines(page, page_layout.footer_lines, footer_y,
                                 step_points, options, page_index,
                                 document_page_count, section_page_count);
    }
}

} // namespace featherdoc::pdf::detail
