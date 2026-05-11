#include "pdf_document_adapter_table_borders.hpp"

#include "pdf_document_adapter_render.hpp"

#include <algorithm>
#include <optional>

namespace featherdoc::pdf::detail {
namespace {

[[nodiscard]] bool is_visible_border(
    const featherdoc::border_inspection_summary &border) noexcept {
    return border.style != featherdoc::border_style::none &&
           border.size_eighth_points > 0U;
}

[[nodiscard]] double border_width_points(
    const featherdoc::border_inspection_summary &border) noexcept {
    return std::max(0.25, static_cast<double>(border.size_eighth_points) / 8.0);
}

[[nodiscard]] PdfRgbColor
border_color(const featherdoc::border_inspection_summary &border) noexcept {
    if (border.color.empty() || border.color == "auto") {
        return PdfRgbColor{0.0, 0.0, 0.0};
    }
    return parse_hex_rgb_color(border.color)
        .value_or(PdfRgbColor{0.0, 0.0, 0.0});
}

[[nodiscard]] PdfLine
border_line(PdfPoint start, PdfPoint end,
            const featherdoc::border_inspection_summary &border,
            double line_width_points) noexcept {
    auto line = PdfLine{start, end, border_color(border), line_width_points};
    switch (border.style) {
    case featherdoc::border_style::dashed:
        line.dash_on_points = line_width_points * 3.0;
        line.dash_off_points = line_width_points * 2.0;
        break;
    case featherdoc::border_style::dotted:
        line.dash_on_points = line_width_points;
        line.dash_off_points = line_width_points * 2.0;
        line.line_cap = PdfLineCap::round;
        break;
    case featherdoc::border_style::none:
    case featherdoc::border_style::single:
    case featherdoc::border_style::double_line:
    case featherdoc::border_style::thick:
        break;
    }
    return line;
}

[[nodiscard]] PdfLine
border_line(PdfPoint start, PdfPoint end,
            const featherdoc::border_inspection_summary &border) noexcept {
    return border_line(start, end, border, border_width_points(border));
}

[[nodiscard]] std::optional<featherdoc::border_inspection_summary>
table_border_for_cell_edge(const featherdoc::table_inspection_summary &table,
                           const TableCellLayout &cell,
                           featherdoc::cell_border_edge edge) {
    if (!table.borders) {
        return std::nullopt;
    }

    switch (edge) {
    case featherdoc::cell_border_edge::top:
        if (cell.cell.row_index == 0U) {
            return table.borders->top;
        }
        return table.borders->inside_horizontal;
    case featherdoc::cell_border_edge::left:
        if (cell.cell.column_index == 0U) {
            return table.borders->left;
        }
        return table.borders->inside_vertical;
    case featherdoc::cell_border_edge::bottom:
        if (cell.cell.row_index +
                std::max<std::size_t>(1U, cell.cell.row_span) >=
            table.row_count) {
            return table.borders->bottom;
        }
        return table.borders->inside_horizontal;
    case featherdoc::cell_border_edge::right:
        if (cell.cell.column_index +
                std::max<std::size_t>(1U, cell.cell.column_span) >=
            table.column_count) {
            return table.borders->right;
        }
        return table.borders->inside_vertical;
    }

    return std::nullopt;
}

[[nodiscard]] std::optional<featherdoc::border_inspection_summary>
resolved_cell_border(const featherdoc::table_inspection_summary &table,
                     const TableCellLayout &cell,
                     featherdoc::cell_border_edge edge) {
    switch (edge) {
    case featherdoc::cell_border_edge::top:
        if (cell.cell.border_top) {
            return cell.cell.border_top;
        }
        break;
    case featherdoc::cell_border_edge::left:
        if (cell.cell.border_left) {
            return cell.cell.border_left;
        }
        break;
    case featherdoc::cell_border_edge::bottom:
        if (cell.cell.border_bottom) {
            return cell.cell.border_bottom;
        }
        break;
    case featherdoc::cell_border_edge::right:
        if (cell.cell.border_right) {
            return cell.cell.border_right;
        }
        break;
    }
    return table_border_for_cell_edge(table, cell, edge);
}

[[nodiscard]] PdfPoint offset_border_point(PdfPoint point,
                                           featherdoc::cell_border_edge edge,
                                           double offset_points) noexcept {
    switch (edge) {
    case featherdoc::cell_border_edge::top:
        point.y_points -= offset_points;
        break;
    case featherdoc::cell_border_edge::left:
        point.x_points += offset_points;
        break;
    case featherdoc::cell_border_edge::bottom:
        point.y_points += offset_points;
        break;
    case featherdoc::cell_border_edge::right:
        point.x_points -= offset_points;
        break;
    }
    return point;
}

void emit_border_lines(PdfPageLayout &page, PdfPoint start, PdfPoint end,
                       featherdoc::cell_border_edge edge,
                       const featherdoc::border_inspection_summary &border) {
    if (border.style != featherdoc::border_style::double_line) {
        page.lines.push_back(border_line(start, end, border));
        return;
    }

    const auto requested_width = border_width_points(border);
    const auto line_width = std::max(0.25, requested_width / 3.0);
    const auto offset = line_width;
    page.lines.push_back(border_line(start, end, border, line_width));
    page.lines.push_back(border_line(offset_border_point(start, edge, offset),
                                     offset_border_point(end, edge, offset),
                                     border, line_width));
}

} // namespace

[[nodiscard]] bool
has_resolved_cell_border(const featherdoc::table_inspection_summary &table,
                         const TableCellLayout &cell) {
    return resolved_cell_border(table, cell,
                                featherdoc::cell_border_edge::top) ||
           resolved_cell_border(table, cell,
                                featherdoc::cell_border_edge::left) ||
           resolved_cell_border(table, cell,
                                featherdoc::cell_border_edge::bottom) ||
           resolved_cell_border(table, cell,
                                featherdoc::cell_border_edge::right);
}

void emit_cell_border_line(PdfPageLayout &page,
                           const featherdoc::table_inspection_summary &table,
                           const TableCellLayout &cell,
                           featherdoc::cell_border_edge edge, double row_top,
                           double row_bottom) {
    const auto border = resolved_cell_border(table, cell, edge);
    if (!border || !is_visible_border(*border)) {
        return;
    }

    auto start = PdfPoint{};
    auto end = PdfPoint{};
    switch (edge) {
    case featherdoc::cell_border_edge::top:
        start = PdfPoint{cell.x_points, row_top};
        end = PdfPoint{cell.x_points + cell.width_points, row_top};
        break;
    case featherdoc::cell_border_edge::left:
        start = PdfPoint{cell.x_points, row_bottom};
        end = PdfPoint{cell.x_points, row_top};
        break;
    case featherdoc::cell_border_edge::bottom:
        start = PdfPoint{cell.x_points, row_bottom};
        end = PdfPoint{cell.x_points + cell.width_points, row_bottom};
        break;
    case featherdoc::cell_border_edge::right:
        start = PdfPoint{cell.x_points + cell.width_points, row_bottom};
        end = PdfPoint{cell.x_points + cell.width_points, row_top};
        break;
    }

    emit_border_lines(page, start, end, edge, *border);
}

} // namespace featherdoc::pdf::detail
