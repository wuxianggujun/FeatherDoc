#include "pdf_document_adapter_table_layout.hpp"

#include "pdf_document_adapter_render.hpp"

#include <algorithm>
#include <cstdint>
#include <iterator>
#include <string>
#include <utility>

namespace featherdoc::pdf::detail {
namespace {

[[nodiscard]] double
cell_margin_points(const std::optional<std::uint32_t> &margin_twips,
                   double fallback_points) noexcept {
    return margin_twips ? twips_to_points(*margin_twips) : fallback_points;
}

[[nodiscard]] double resolved_cell_padding_points(
    const std::optional<std::uint32_t> &cell_margin_twips,
    const std::optional<std::uint32_t> &table_margin_twips,
    double fallback_points) noexcept {
    return cell_margin_points(
        cell_margin_twips,
        cell_margin_points(table_margin_twips, fallback_points));
}

[[nodiscard]] featherdoc::run_inspection_summary
summarize_run_handle(const featherdoc::Run &run_handle, std::size_t run_index) {
    auto summary = featherdoc::run_inspection_summary{};
    summary.index = run_index;
    summary.text = run_handle.get_text();
    summary.style_id = run_handle.style_id();
    summary.font_family = run_handle.font_family();
    summary.east_asia_font_family = run_handle.east_asia_font_family();
    summary.text_color = run_handle.text_color();
    summary.bold = run_handle.bold();
    summary.italic = run_handle.italic();
    summary.underline = run_handle.underline();
    summary.font_size_points = run_handle.font_size_points();
    summary.language = run_handle.language();
    summary.east_asia_language = run_handle.east_asia_language();
    summary.bidi_language = run_handle.bidi_language();
    summary.rtl = run_handle.rtl();
    return summary;
}

void append_hard_break_token(std::vector<TextToken> &tokens) {
    auto token = TextToken{};
    token.hard_break = true;
    tokens.push_back(std::move(token));
}

[[nodiscard]] std::optional<std::vector<LineState>>
wrap_table_cell_runs(featherdoc::Table *table_handle,
                     const featherdoc::table_cell_inspection_summary &cell,
                     double max_width_points,
                     const TableRenderContext &context) {
    if (table_handle == nullptr || !context.resolve_run_style) {
        return std::nullopt;
    }

    auto cell_handle = table_handle->find_cell(cell.row_index, cell.cell_index);
    if (!cell_handle.has_value()) {
        return std::nullopt;
    }

    std::vector<TextToken> tokens;
    auto paragraph = cell_handle->paragraphs();
    auto saw_paragraph = false;
    for (; paragraph.has_next(); paragraph.next()) {
        if (saw_paragraph) {
            append_hard_break_token(tokens);
        }
        saw_paragraph = true;

        auto run = paragraph.runs();
        for (std::size_t run_index = 0U; run.has_next();
             ++run_index, run.next()) {
            auto summary = summarize_run_handle(run, run_index);
            if (summary.text.empty()) {
                continue;
            }

            const auto style = context.resolve_run_style(summary);
            auto run_tokens = tokenize_run_text(summary.text, style);
            tokens.insert(tokens.end(),
                          std::make_move_iterator(run_tokens.begin()),
                          std::make_move_iterator(run_tokens.end()));
        }
    }

    if (!saw_paragraph || tokens.empty()) {
        return std::vector<LineState>{LineState{}};
    }

    return wrap_run_tokens(tokens, max_width_points);
}

[[nodiscard]] std::size_t table_row_count(
    const featherdoc::table_inspection_summary &table,
    const std::vector<featherdoc::table_cell_inspection_summary> &cells) {
    auto row_count = table.row_count;
    for (const auto &cell : cells) {
        row_count = std::max(row_count, cell.row_index + 1U);
    }
    return row_count;
}

[[nodiscard]] double resolved_table_row_height_points(
    const featherdoc::table_inspection_summary &table, std::size_t row_index,
    double content_height_points) {
    if (row_index >= table.row_height_twips.size() ||
        !table.row_height_twips[row_index]) {
        return content_height_points;
    }

    const auto requested_height =
        twips_to_points(*table.row_height_twips[row_index]);
    auto rule = featherdoc::row_height_rule::at_least;
    if (row_index < table.row_height_rules.size() &&
        table.row_height_rules[row_index]) {
        rule = *table.row_height_rules[row_index];
    }

    if (rule == featherdoc::row_height_rule::exact) {
        return requested_height;
    }
    return std::max(content_height_points, requested_height);
}

[[nodiscard]] double sum_column_widths(const std::vector<double> &column_widths,
                                       std::size_t start_column,
                                       std::size_t column_span,
                                       double cell_spacing_points) {
    auto width = 0.0;
    const auto end_column =
        std::min(column_widths.size(),
                 start_column + std::max<std::size_t>(1U, column_span));
    for (auto index = start_column; index < end_column; ++index) {
        width += column_widths[index];
    }
    if (end_column > start_column + 1U) {
        width += cell_spacing_points *
                 static_cast<double>(end_column - start_column - 1U);
    }
    return std::max(1.0, width);
}

[[nodiscard]] std::size_t
clamped_row_span(const featherdoc::table_cell_inspection_summary &cell,
                 std::size_t row_count) noexcept {
    if (cell.vertical_merge != featherdoc::cell_vertical_merge::restart) {
        return 1U;
    }
    if (cell.row_index >= row_count) {
        return 1U;
    }
    return std::max<std::size_t>(
        1U, std::min(cell.row_span, row_count - cell.row_index));
}

[[nodiscard]] double
table_width_points(const std::vector<double> &column_widths,
                   double cell_spacing_points) noexcept {
    auto width = 0.0;
    for (const auto column_width : column_widths) {
        width += column_width;
    }
    if (column_widths.size() > 1U) {
        width += cell_spacing_points *
                 static_cast<double>(column_widths.size() - 1U);
    }
    return width;
}

[[nodiscard]] double signed_twips_to_points(std::int32_t twips) noexcept {
    return static_cast<double>(twips) / 20.0;
}

[[nodiscard]] std::optional<double> positioned_table_origin_x_points(
    const featherdoc::table_position &position,
    const PdfDocumentAdapterOptions &options) noexcept {
    const auto offset_points =
        signed_twips_to_points(position.horizontal_offset_twips);

    switch (position.horizontal_reference) {
    case featherdoc::table_position_horizontal_reference::page:
        return offset_points;
    case featherdoc::table_position_horizontal_reference::margin:
    case featherdoc::table_position_horizontal_reference::column:
        return options.margin_left_points + offset_points;
    }

    return std::nullopt;
}

[[nodiscard]] double
table_origin_x_points(const featherdoc::table_inspection_summary &table,
                      const std::vector<double> &column_widths,
                      const PdfDocumentAdapterOptions &options,
                      double max_width_points) noexcept {
    if (table.position.has_value()) {
        if (const auto positioned_origin =
                positioned_table_origin_x_points(*table.position, options)) {
            return *positioned_origin;
        }
    }

    auto origin = options.margin_left_points;
    const auto cell_spacing = table_cell_spacing_points(table);
    const auto width = table_width_points(column_widths, cell_spacing);
    const auto free_width = std::max(0.0, max_width_points - width);

    if (table.alignment == featherdoc::table_alignment::center) {
        origin += free_width / 2.0;
    } else if (table.alignment == featherdoc::table_alignment::right) {
        origin += free_width;
    } else if (table.indent_twips && *table.indent_twips > 0U) {
        origin += std::min(free_width, twips_to_points(*table.indent_twips));
    }

    return origin;
}

[[nodiscard]] double rotated_content_height_points_for(
    const std::vector<LineState> &lines, double fallback_line_height_points) {
    auto height = 0.0;
    for (const auto &line : lines) {
        height = std::max(height, line.width_points);
    }
    return height > 0.0 ? height : fallback_line_height_points;
}

} // namespace

[[nodiscard]] std::optional<featherdoc::Table>
find_table_handle(featherdoc::Document &document, std::size_t table_index) {
    auto table_handle = document.tables();
    for (std::size_t current_index = 0U;
         current_index < table_index && table_handle.has_next();
         ++current_index) {
        table_handle.next();
    }

    if (!table_handle.has_next()) {
        return std::nullopt;
    }

    return table_handle;
}

[[nodiscard]] std::size_t table_column_count(
    const featherdoc::table_inspection_summary &table,
    const std::vector<featherdoc::table_cell_inspection_summary> &cells) {
    auto column_count = table.column_count;
    for (const auto &cell : cells) {
        column_count = std::max(
            column_count,
            cell.column_index + std::max<std::size_t>(1U, cell.column_span));
    }
    return std::max<std::size_t>(1U, column_count);
}

[[nodiscard]] std::vector<double>
build_table_column_widths(const featherdoc::table_inspection_summary &table,
                          std::size_t column_count, double max_width_points) {
    std::vector<double> widths(column_count, 0.0);
    if (column_count == 0U) {
        return widths;
    }

    auto target_width = table.width_twips ? twips_to_points(*table.width_twips)
                                          : max_width_points;
    if (target_width <= 0.0 || target_width > max_width_points) {
        target_width = max_width_points;
    }

    auto known_width = 0.0;
    auto missing_count = std::size_t{0U};
    for (std::size_t index = 0U; index < column_count; ++index) {
        if (index < table.column_widths.size() &&
            table.column_widths[index].has_value() &&
            *table.column_widths[index] > 0U) {
            widths[index] = twips_to_points(*table.column_widths[index]);
            known_width += widths[index];
        } else {
            ++missing_count;
        }
    }

    if (missing_count == column_count) {
        const auto equal_width =
            target_width / static_cast<double>(column_count);
        std::fill(widths.begin(), widths.end(), equal_width);
    } else if (missing_count > 0U) {
        const auto fallback_width =
            known_width < target_width
                ? (target_width - known_width) /
                      static_cast<double>(missing_count)
                : target_width / static_cast<double>(column_count);
        for (auto &width : widths) {
            if (width <= 0.0) {
                width = std::max(1.0, fallback_width);
            }
        }
    }

    auto total_width = 0.0;
    for (const auto width : widths) {
        total_width += width;
    }
    if (total_width <= 0.0) {
        const auto equal_width =
            max_width_points / static_cast<double>(column_count);
        std::fill(widths.begin(), widths.end(), equal_width);
        return widths;
    }
    if (total_width > max_width_points) {
        const auto scale = max_width_points / total_width;
        for (auto &width : widths) {
            width *= scale;
        }
    }

    return widths;
}

[[nodiscard]] double table_cell_spacing_points(
    const featherdoc::table_inspection_summary &table) noexcept {
    return table.cell_spacing_twips ? twips_to_points(*table.cell_spacing_twips)
                                    : 0.0;
}

[[nodiscard]] std::optional<double> positioned_table_row_top_points(
    const featherdoc::table_inspection_summary &table, double anchor_top_points,
    const PdfDocumentAdapterOptions &options) noexcept {
    if (!table.position.has_value()) {
        return std::nullopt;
    }

    const auto offset_points =
        signed_twips_to_points(table.position->vertical_offset_twips);

    switch (table.position->vertical_reference) {
    case featherdoc::table_position_vertical_reference::page:
        return options.page_size.height_points - offset_points;
    case featherdoc::table_position_vertical_reference::margin:
        return options.page_size.height_points - options.margin_top_points -
               offset_points;
    case featherdoc::table_position_vertical_reference::paragraph:
        return anchor_top_points - offset_points;
    }

    return std::nullopt;
}

[[nodiscard]] bool is_vertical_merge_continuation(
    const featherdoc::table_cell_inspection_summary &cell) noexcept {
    return cell.vertical_merge ==
           featherdoc::cell_vertical_merge::continue_merge;
}

[[nodiscard]] std::optional<double> cell_text_rotation_degrees(
    const featherdoc::table_cell_inspection_summary &cell) noexcept {
    if (!cell.text_direction.has_value()) {
        return std::nullopt;
    }

    switch (*cell.text_direction) {
    case featherdoc::cell_text_direction::top_to_bottom_right_to_left:
    case featherdoc::cell_text_direction::left_to_right_top_to_bottom_rotated:
    case featherdoc::cell_text_direction::top_to_bottom_right_to_left_rotated:
        return 90.0;
    case featherdoc::cell_text_direction::bottom_to_top_left_to_right:
    case featherdoc::cell_text_direction::top_to_bottom_left_to_right_rotated:
        return -90.0;
    case featherdoc::cell_text_direction::left_to_right_top_to_bottom:
        return std::nullopt;
    }

    return std::nullopt;
}

[[nodiscard]] std::vector<TableRowLayout> build_table_rows(
    const featherdoc::table_inspection_summary &table,
    featherdoc::Table *table_handle,
    const std::vector<featherdoc::table_cell_inspection_summary> &cells,
    const std::vector<double> &column_widths,
    const PdfDocumentAdapterOptions &options, double max_width_points,
    double line_height_points, double cell_padding_points,
    const TableRenderContext &context) {
    const auto row_count = table_row_count(table, cells);
    std::vector<std::vector<featherdoc::table_cell_inspection_summary>>
        grouped_cells(row_count);
    for (const auto &cell : cells) {
        if (cell.row_index < grouped_cells.size()) {
            grouped_cells[cell.row_index].push_back(cell);
        }
    }

    std::vector<double> column_offsets(column_widths.size(), 0.0);
    auto current_x =
        table_origin_x_points(table, column_widths, options, max_width_points);
    const auto cell_spacing = table_cell_spacing_points(table);
    for (std::size_t index = 0U; index < column_widths.size(); ++index) {
        column_offsets[index] = current_x;
        current_x += column_widths[index] + cell_spacing;
    }

    std::vector<TableRowLayout> rows;
    rows.reserve(row_count);
    const auto empty_row_height =
        line_height_points + cell_padding_points * 2.0;
    for (std::size_t row_index = 0U; row_index < grouped_cells.size();
         ++row_index) {
        auto &row_cells = grouped_cells[row_index];
        std::sort(row_cells.begin(), row_cells.end(),
                  [](const auto &left, const auto &right) {
                      if (left.column_index != right.column_index) {
                          return left.column_index < right.column_index;
                      }
                      return left.cell_index < right.cell_index;
                  });

        auto row_layout = TableRowLayout{};
        if (row_index < table.row_cant_split.size()) {
            row_layout.cant_split = table.row_cant_split[row_index];
        }
        if (row_index < table.row_repeats_header.size()) {
            row_layout.repeats_header = table.row_repeats_header[row_index];
        }
        for (const auto &cell : row_cells) {
            const auto start_column =
                std::min(cell.column_index, column_widths.size() - 1U);
            const auto cell_width = sum_column_widths(
                column_widths, start_column, cell.column_span, cell_spacing);
            const auto padding_top = resolved_cell_padding_points(
                cell.margin_top_twips, table.cell_margin_top_twips,
                cell_padding_points);
            const auto padding_left = resolved_cell_padding_points(
                cell.margin_left_twips, table.cell_margin_left_twips,
                cell_padding_points);
            const auto padding_bottom = resolved_cell_padding_points(
                cell.margin_bottom_twips, table.cell_margin_bottom_twips,
                cell_padding_points);
            const auto padding_right = resolved_cell_padding_points(
                cell.margin_right_twips, table.cell_margin_right_twips,
                cell_padding_points);
            const auto text_width =
                std::max(1.0, cell_width - padding_left - padding_right);
            auto rich_lines =
                wrap_table_cell_runs(table_handle, cell, text_width, context);
            auto lines = rich_lines.value_or(
                context.wrap_plain_text
                    ? context.wrap_plain_text(cell.text, text_width)
                    : std::vector<LineState>{LineState{}});
            const auto content_height = cell_text_rotation_degrees(cell)
                                            ? rotated_content_height_points_for(
                                                  lines, line_height_points)
                                            : content_height_points_for(
                                                  lines, line_height_points,
                                                  options.font_size_points);
            row_layout.height_points =
                std::max(row_layout.height_points,
                         content_height + padding_top + padding_bottom);
            row_layout.cells.push_back(TableCellLayout{
                cell,
                column_offsets[start_column],
                cell_width,
                padding_top,
                padding_left,
                padding_bottom,
                padding_right,
                content_height,
                cell.fill_color ? parse_hex_rgb_color(*cell.fill_color)
                                : std::optional<PdfRgbColor>{},
                std::move(lines),
            });
        }
        if (row_layout.cells.empty()) {
            row_layout.height_points = empty_row_height;
        }
        row_layout.height_points = resolved_table_row_height_points(
            table, row_index, row_layout.height_points);
        rows.push_back(std::move(row_layout));
    }

    return rows;
}

[[nodiscard]] double cell_content_top(const TableCellLayout &cell,
                                      double row_top,
                                      double row_bottom) noexcept {
    const auto available_height =
        std::max(0.0, row_top - row_bottom - cell.padding_top_points -
                          cell.padding_bottom_points);
    const auto extra_height =
        std::max(0.0, available_height - cell.content_height_points);

    if (cell.cell.vertical_alignment ==
        featherdoc::cell_vertical_alignment::bottom) {
        return row_top - cell.padding_top_points - extra_height;
    }
    if (cell.cell.vertical_alignment ==
            featherdoc::cell_vertical_alignment::center ||
        cell.cell.vertical_alignment ==
            featherdoc::cell_vertical_alignment::both) {
        return row_top - cell.padding_top_points - extra_height / 2.0;
    }
    return row_top - cell.padding_top_points;
}

[[nodiscard]] double spanned_row_bottom(const std::vector<TableRowLayout> &rows,
                                        std::size_t row_index,
                                        const TableCellLayout &cell,
                                        double row_top,
                                        double cell_spacing_points) noexcept {
    const auto row_span = clamped_row_span(cell.cell, rows.size());
    auto row_bottom = row_top;
    for (std::size_t offset = 0U; offset < row_span; ++offset) {
        row_bottom -= rows[row_index + offset].height_points;
        if (offset + 1U < row_span) {
            row_bottom -= cell_spacing_points;
        }
    }
    return row_bottom;
}

} // namespace featherdoc::pdf::detail
