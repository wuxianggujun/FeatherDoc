#include <featherdoc/pdf/pdf_document_adapter.hpp>
#include <featherdoc/pdf/pdf_text_metrics.hpp>

#include "pdf_document_adapter_headers.hpp"
#include "pdf_document_adapter_images.hpp"
#include "pdf_document_adapter_paragraphs.hpp"
#include "pdf_document_adapter_render.hpp"
#include "pdf_document_adapter_tables.hpp"
#include "pdf_document_adapter_text.hpp"

#include <algorithm>
#include <optional>
#include <string_view>
#include <utility>
#include <vector>

namespace featherdoc::pdf {
namespace {

using detail::content_width;
using detail::emit_line_at;
using detail::first_baseline_y;
using detail::line_baseline_offset_points_for;
using detail::line_height_points_for;
using detail::LineState;
using detail::metrics_options_for;

struct ParagraphLineLayout {
    LineState line;
    double start_x_points{0.0};
    double max_width_points{1.0};
};

struct FloatingLineShape {
    double start_x_points{0.0};
    double max_width_points{1.0};
};

void emit_line(PdfPageLayout &page, const LineState &line, double baseline_y,
               const PdfDocumentAdapterOptions &options) {
    emit_line_at(page, line, options.margin_left_points, baseline_y);
}

void emit_or_advance_blank_line(PdfPageLayout &page, const LineState &line,
                                double start_x_points, double baseline_y) {
    if (!line.empty()) {
        emit_line_at(page, line, start_x_points, baseline_y);
    }
}

[[nodiscard]] bool
line_layouts_contain_text(const std::vector<ParagraphLineLayout> &layouts) {
    for (const auto &layout : layouts) {
        for (const auto &fragment : layout.line.fragments) {
            if (!fragment.text.empty()) {
                return true;
            }
        }
    }
    return false;
}

[[nodiscard]] bool line_overlaps_exclusion(
    double line_top, double line_height,
    const detail::FloatingTextExclusion &exclusion) {
    const auto line_bottom = line_top - line_height;
    const auto exclusion_top = exclusion.bounds.y_points +
                               exclusion.bounds.height_points +
                               exclusion.distance_top_points;
    const auto exclusion_bottom =
        exclusion.bounds.y_points - exclusion.distance_bottom_points;
    return line_bottom < exclusion_top && line_top > exclusion_bottom;
}

[[nodiscard]] FloatingLineShape line_shape_for_exclusions(
    double line_top, double line_height, double content_left,
    double content_right,
    const std::vector<detail::FloatingTextExclusion> &exclusions) {
    FloatingLineShape shape{content_left,
                            std::max(1.0, content_right - content_left)};

    for (const auto &exclusion : exclusions) {
        if (exclusion.wrap_mode != featherdoc::floating_image_wrap_mode::square ||
            !line_overlaps_exclusion(line_top, line_height, exclusion)) {
            continue;
        }

        const auto excluded_left =
            std::max(content_left,
                     exclusion.bounds.x_points -
                         exclusion.distance_left_points);
        const auto excluded_right =
            std::min(content_right,
                     exclusion.bounds.x_points +
                         exclusion.bounds.width_points +
                         exclusion.distance_right_points);
        if (excluded_left >= content_right || excluded_right <= content_left ||
            excluded_left >= excluded_right) {
            continue;
        }

        const auto left_width =
            std::max(0.0, excluded_left - shape.start_x_points);
        const auto right_start = std::max(shape.start_x_points, excluded_right);
        const auto right_width = std::max(0.0, content_right - right_start);
        if (right_width >= left_width) {
            shape.start_x_points = right_start;
            shape.max_width_points = right_width;
        } else {
            shape.max_width_points = left_width;
        }
    }

    shape.max_width_points = std::max(1.0, shape.max_width_points);
    return shape;
}

[[nodiscard]] double optional_twips_to_points(
    const std::optional<std::uint32_t> &twips) noexcept {
    return twips ? detail::twips_to_points(*twips) : 0.0;
}

[[nodiscard]] double paragraph_first_line_offset_points(
    const featherdoc::paragraph_inspection_summary &paragraph) noexcept {
    if (paragraph.first_line_indent_twips.has_value()) {
        return detail::twips_to_points(*paragraph.first_line_indent_twips);
    }
    if (paragraph.hanging_indent_twips.has_value()) {
        return -detail::twips_to_points(*paragraph.hanging_indent_twips);
    }
    return 0.0;
}

[[nodiscard]] FloatingLineShape paragraph_line_shape_for_index(
    const featherdoc::paragraph_inspection_summary &paragraph,
    const PdfDocumentAdapterOptions &options, double max_width_points,
    std::size_t line_index, double line_top, double line_height,
    const std::vector<detail::FloatingTextExclusion> &exclusions) {
    const auto paragraph_left =
        options.margin_left_points +
        optional_twips_to_points(paragraph.indent_left_twips);
    auto paragraph_right =
        options.margin_left_points + max_width_points -
        optional_twips_to_points(paragraph.indent_right_twips);
    paragraph_right = std::max(paragraph_left + 1.0, paragraph_right);

    auto line_left = paragraph_left;
    if (line_index == 0U) {
        line_left += paragraph_first_line_offset_points(paragraph);
    }
    line_left = std::clamp(line_left, options.margin_left_points,
                           paragraph_right - 1.0);

    return line_shape_for_exclusions(line_top, line_height, line_left,
                                     paragraph_right, exclusions);
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

[[nodiscard]] double apply_top_bottom_text_exclusions(
    double current_y, const PdfDocumentAdapterOptions &options,
    const std::vector<detail::FloatingTextExclusion> &exclusions) {
    auto adjusted_y = current_y;
    for (const auto &exclusion : exclusions) {
        if (exclusion.wrap_mode !=
            featherdoc::floating_image_wrap_mode::top_bottom) {
            continue;
        }

        const auto line_top = adjusted_y + options.font_size_points;
        const auto exclusion_top = exclusion.bounds.y_points +
                                   exclusion.bounds.height_points +
                                   exclusion.distance_top_points;
        const auto exclusion_bottom =
            exclusion.bounds.y_points - exclusion.distance_bottom_points;
        if (line_top <= exclusion_top && line_top > exclusion_bottom) {
            adjusted_y =
                std::min(adjusted_y,
                         exclusion_bottom - options.font_size_points);
        }
    }
    return adjusted_y;
}

[[nodiscard]] std::vector<ParagraphLineLayout> layout_paragraph_lines(
    featherdoc::Document &document,
    const featherdoc::paragraph_inspection_summary &paragraph,
    const PdfDocumentAdapterOptions &options, const PdfFontResolver &resolver,
    double max_width_points, double current_y, double resolved_line_height,
    detail::ParagraphNumberingState &numbering_state,
    const std::vector<detail::FloatingTextExclusion> &exclusions) {
    const auto tokens = detail::paragraph_text_tokens(
        document, paragraph, options, resolver, numbering_state);
    if (tokens.empty()) {
        return {ParagraphLineLayout{LineState{}, options.margin_left_points,
                                    max_width_points}};
    }

    const auto line_top_for_index = [&](std::size_t line_index) {
        return current_y + options.font_size_points -
               static_cast<double>(line_index) * resolved_line_height;
    };
    const auto shape_for_index = [&](std::size_t line_index) {
        return paragraph_line_shape_for_index(
            paragraph, options, max_width_points, line_index,
            line_top_for_index(line_index), resolved_line_height, exclusions);
    };

    const auto lines = detail::wrap_run_tokens_with_line_widths(
        tokens, [&](std::size_t line_index) {
            return shape_for_index(line_index).max_width_points;
        });

    std::vector<ParagraphLineLayout> layouts;
    layouts.reserve(lines.size());
    for (std::size_t index = 0U; index < lines.size(); ++index) {
        const auto shape = shape_for_index(index);
        const auto alignment =
            paragraph.alignment.value_or(featherdoc::paragraph_alignment::left);
        layouts.push_back(ParagraphLineLayout{
            lines[index],
            shape.start_x_points +
                paragraph_alignment_offset_points(
                    alignment, lines[index].width_points,
                    shape.max_width_points),
            shape.max_width_points,
        });
    }
    return layouts;
}

[[nodiscard]] PdfFontResolver
make_font_resolver(const PdfDocumentAdapterOptions &options) {
    return PdfFontResolver(PdfFontResolverOptions{
        options.font_mappings,
        options.font_file_path,
        options.cjk_font_file_path,
        options.use_system_font_fallbacks,
    });
}

[[nodiscard]] double
default_line_height(const PdfDocumentAdapterOptions &options,
                    const PdfFontResolver &resolver) {
    if (options.line_height_points > 0.0) {
        return options.line_height_points;
    }

    const auto resolved_font = resolver.resolve(options.font_family, {}, "Ag");
    return estimate_line_height_points(options.font_size_points,
                                       metrics_options_for(resolved_font));
}

[[nodiscard]] PdfPageSize
page_size_from_section_setup(const featherdoc::section_page_setup &setup) {
    return PdfPageSize{
        detail::twips_to_points(setup.width_twips),
        detail::twips_to_points(setup.height_twips),
    };
}

void apply_section_page_setup(PdfDocumentAdapterOptions &options,
                              const featherdoc::section_page_setup &setup) {
    if (setup.width_twips > 0U && setup.height_twips > 0U) {
        options.page_size = page_size_from_section_setup(setup);
    }

    options.margin_top_points =
        detail::twips_to_points(setup.margins.top_twips);
    options.margin_bottom_points =
        detail::twips_to_points(setup.margins.bottom_twips);
    options.margin_left_points =
        detail::twips_to_points(setup.margins.left_twips);
    options.margin_right_points =
        detail::twips_to_points(setup.margins.right_twips);
    options.header_y_offset_points =
        detail::twips_to_points(setup.margins.header_twips);
    options.footer_y_offset_points =
        detail::twips_to_points(setup.margins.footer_twips);
}

[[nodiscard]] std::vector<PdfDocumentAdapterOptions>
layout_options_for_document_sections(featherdoc::Document &document,
                                     const PdfDocumentAdapterOptions &options) {
    auto section_count = document.section_count();
    if (section_count == 0U) {
        section_count = 1U;
    }

    auto section_options =
        std::vector<PdfDocumentAdapterOptions>(section_count, options);
    for (std::size_t section_index = 0U; section_index < section_count;
         ++section_index) {
        const auto page_setup = document.get_section_page_setup(section_index);
        if (page_setup.has_value()) {
            apply_section_page_setup(section_options[section_index],
                                     *page_setup);
        }
    }

    return section_options;
}

[[nodiscard]] const PdfDocumentAdapterOptions &
options_for_section(const std::vector<PdfDocumentAdapterOptions> &options,
                    std::size_t section_index) {
    return options[std::min(section_index, options.size() - 1U)];
}

} // namespace

PdfDocumentLayout
layout_document_paragraphs(featherdoc::Document &document,
                           const PdfDocumentAdapterOptions &options) {
    const auto section_options =
        layout_options_for_document_sections(document, options);
    const auto body_blocks = document.inspect_body_blocks();

    PdfDocumentLayout layout;
    layout.metadata = section_options.front().metadata;

    std::vector<std::size_t> section_page_counts(section_options.size(), 0U);
    auto new_page = [&layout, &section_options, &section_page_counts](
                        std::size_t section_index) -> PdfPageLayout & {
        const auto normalized_section_index =
            std::min(section_index, section_options.size() - 1U);
        const auto &page_options = section_options[normalized_section_index];
        PdfPageLayout page;
        page.size = page_options.page_size;
        page.section_index = section_index;
        page.section_page_index =
            section_page_counts[normalized_section_index]++;
        layout.pages.push_back(std::move(page));
        return layout.pages.back();
    };

    auto current_section_index =
        body_blocks.empty() ? 0U : body_blocks.front().section_index;
    auto *page = &new_page(current_section_index);
    auto current_y = first_baseline_y(
        options_for_section(section_options, current_section_index));
    const auto resolver = make_font_resolver(section_options.front());
    const detail::HeaderFooterRenderContext header_footer_context{
        [&](std::string_view text,
            const PdfDocumentAdapterOptions &header_options,
            double width_points) {
            return detail::wrap_plain_text(document, text, header_options,
                                           resolver, width_points);
        },
    };

    std::vector<detail::HeaderFooterLayout> header_footer_layouts;
    std::vector<double> header_footer_line_heights;
    header_footer_layouts.reserve(section_options.size());
    header_footer_line_heights.reserve(section_options.size());
    for (std::size_t section_index = 0U; section_index < section_options.size();
         ++section_index) {
        const auto &section_option = section_options[section_index];
        header_footer_layouts.push_back(detail::build_header_footer_layout(
            document, section_index, section_option,
            content_width(section_option), header_footer_context));
        header_footer_line_heights.push_back(
            detail::header_footer_line_height_points(section_option));
    }

    const auto inline_images =
        section_options.front().render_inline_images
            ? document.inline_images()
            : std::vector<featherdoc::inline_image_info>{};
    const auto drawing_images =
        section_options.front().render_inline_images
            ? document.drawing_images()
            : std::vector<featherdoc::drawing_image_info>{};
    const auto body_block_images =
        detail::group_inline_images_by_body_block(inline_images);
    const auto fallback_images =
        detail::unpositioned_inline_images(inline_images);
    const auto body_block_floating_images =
        detail::group_floating_images_by_body_block(drawing_images);
    const auto fallback_floating_images =
        detail::unpositioned_floating_images(drawing_images);
    detail::ParagraphNumberingState numbering_state;
    std::vector<detail::FloatingTextExclusion> active_floating_text_exclusions;
    const detail::InlineImageRenderContext image_context{
        [&]() -> PdfPageLayout & { return new_page(current_section_index); },
    };
    const detail::TableRenderContext table_context{
        [&](const featherdoc::run_inspection_summary &run) {
            return detail::resolve_run_style(
                document, run,
                options_for_section(section_options, current_section_index),
                resolver);
        },
        [&](std::string_view text, double width_points) {
            return detail::wrap_plain_text(
                document, text,
                options_for_section(section_options, current_section_index),
                resolver, width_points);
        },
        [&]() -> PdfPageLayout & { return new_page(current_section_index); },
    };

    for (const auto &block : body_blocks) {
        if (block.section_index != current_section_index) {
            current_section_index = block.section_index;
            page = &new_page(current_section_index);
            current_y = first_baseline_y(
                options_for_section(section_options, current_section_index));
            active_floating_text_exclusions.clear();
        }

        const auto &layout_options =
            options_for_section(section_options, current_section_index);
        const auto max_width = content_width(layout_options);
        const auto resolved_line_height =
            default_line_height(layout_options, resolver);
        const auto block_anchor_top =
            current_y + layout_options.font_size_points;

        const auto floating_images_for_block =
            body_block_floating_images.find(block.block_index);
        std::vector<detail::FloatingTextExclusion> floating_text_exclusions;
        if (floating_images_for_block != body_block_floating_images.end()) {
            floating_text_exclusions = detail::floating_image_text_exclusions(
                floating_images_for_block->second, block_anchor_top,
                layout_options, max_width);
            active_floating_text_exclusions.insert(
                active_floating_text_exclusions.end(),
                floating_text_exclusions.begin(),
                floating_text_exclusions.end());
            detail::emit_floating_images(
                document, floating_images_for_block->second, *page,
                block_anchor_top, layout_options, max_width);
        }

        if (block.kind == featherdoc::body_block_kind::table) {
            const auto table = document.inspect_table(block.item_index);
            if (table.has_value()) {
                detail::emit_table(document, page, current_y, *table,
                                   layout_options, max_width,
                                   resolved_line_height, table_context);
            }
            continue;
        }

        const auto paragraph = document.inspect_paragraph(block.item_index);
        if (!paragraph.has_value()) {
            continue;
        }

        current_y = apply_top_bottom_text_exclusions(
            current_y, layout_options, active_floating_text_exclusions);
        const auto line_layouts = layout_paragraph_lines(
            document, *paragraph, layout_options, resolver, max_width,
            current_y, resolved_line_height, numbering_state,
            active_floating_text_exclusions);
        const auto images_for_block = body_block_images.find(block.block_index);
        const auto has_block_images =
            images_for_block != body_block_images.end();
        if (block.kind != featherdoc::body_block_kind::table &&
            (!has_block_images || line_layouts_contain_text(line_layouts))) {
            auto line_top = current_y + layout_options.font_size_points;
            for (const auto &line_layout : line_layouts) {
                const auto current_line_height =
                    line_height_points_for(line_layout.line,
                                           resolved_line_height,
                                           layout_options.font_size_points);
                auto start_x = line_layout.start_x_points;
                if (line_top - current_line_height <
                    layout_options.margin_bottom_points) {
                    page = &new_page(current_section_index);
                    current_y = first_baseline_y(layout_options);
                    line_top = current_y + layout_options.font_size_points;
                }

                const auto baseline_y =
                    line_top - line_baseline_offset_points_for(
                                   line_layout.line,
                                   layout_options.font_size_points);
                emit_or_advance_blank_line(*page, line_layout.line, start_x,
                                           baseline_y);
                line_top -= current_line_height;
                current_y = line_top - layout_options.font_size_points;
            }
        }

        if (has_block_images) {
            detail::emit_inline_images(document, images_for_block->second, page,
                                       current_y, layout_options, max_width,
                                       image_context);
        }
        current_y -= layout_options.paragraph_spacing_after_points;
    }

    const auto &layout_options =
        options_for_section(section_options, current_section_index);
    const auto max_width = content_width(layout_options);
    detail::emit_inline_images(document, fallback_images, page, current_y,
                               layout_options, max_width, image_context);
    detail::emit_floating_images(document, fallback_floating_images, *page,
                                 current_y + layout_options.font_size_points,
                                 layout_options, max_width);
    detail::emit_headers_and_footers(layout, header_footer_layouts,
                                     section_options,
                                     header_footer_line_heights);
    return layout;
}

} // namespace featherdoc::pdf
