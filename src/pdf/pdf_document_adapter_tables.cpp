#include "pdf_document_adapter_tables.hpp"

#include "pdf_document_adapter_render.hpp"
#include "pdf_document_adapter_table_borders.hpp"
#include "pdf_document_adapter_table_layout.hpp"

#include <cstddef>
#include <optional>
#include <vector>

namespace featherdoc::pdf::detail {
namespace {

void emit_rotated_cell_lines(PdfPageLayout &page, const TableCellLayout &cell,
                             double row_top, double cell_bottom,
                             double rotation_degrees,
                             double line_height_points,
                             double default_font_size_points) {
    if (rotation_degrees > 0.0) {
        auto baseline_x =
            cell.x_points + cell.width_points - cell.padding_right_points;
        const auto minimum_x = cell.x_points + cell.padding_left_points;
        const auto baseline_y = cell_bottom + cell.padding_bottom_points;
        const auto maximum_y = row_top - cell.padding_top_points;
        for (const auto &line : cell.lines) {
            if (baseline_x < minimum_x || baseline_y + line.width_points >
                                              maximum_y + 0.01) {
                break;
            }
            emit_line_at(page, line, baseline_x, baseline_y, rotation_degrees);
            baseline_x -= line_height_points_for(line, line_height_points,
                                                 default_font_size_points);
        }
        return;
    }

    auto baseline_x = cell.x_points + cell.padding_left_points;
    const auto maximum_x =
        cell.x_points + cell.width_points - cell.padding_right_points;
    const auto baseline_y = row_top - cell.padding_top_points;
    const auto minimum_y = cell_bottom + cell.padding_bottom_points;
    for (const auto &line : cell.lines) {
        if (baseline_x > maximum_x ||
            baseline_y - line.width_points < minimum_y - 0.01) {
            break;
        }
        emit_line_at(page, line, baseline_x, baseline_y, rotation_degrees);
        baseline_x += line_height_points_for(line, line_height_points,
                                             default_font_size_points);
    }
}

[[nodiscard]] bool row_overflows_page_area(
    const TableRowLayout &row, double row_top,
    const PdfDocumentAdapterOptions &options) noexcept {
    return row_top - row.height_points < options.margin_bottom_points;
}

[[nodiscard]] std::size_t repeated_header_row_count(
    const std::vector<TableRowLayout> &rows) noexcept {
    auto count = std::size_t{0U};
    for (const auto &row : rows) {
        if (!row.repeats_header) {
            break;
        }
        ++count;
    }
    return count;
}

[[nodiscard]] double repeated_header_block_height(
    const std::vector<TableRowLayout> &rows, std::size_t header_row_count,
    double cell_spacing_points) noexcept {
    auto height = 0.0;
    for (std::size_t row_index = 0U; row_index < header_row_count; ++row_index) {
        height += rows[row_index].height_points + cell_spacing_points;
    }
    return height;
}

[[nodiscard]] bool repeated_headers_fit_with_row(
    const std::vector<TableRowLayout> &rows, std::size_t header_row_count,
    std::size_t row_index, double header_block_height,
    const PdfDocumentAdapterOptions &options) noexcept {
    if (header_row_count == 0U || row_index < header_row_count) {
        return false;
    }

    const auto fresh_page_top = first_baseline_y(options) + options.font_size_points;
    const auto repeated_group_height =
        header_block_height + rows[row_index].height_points;
    return fresh_page_top - repeated_group_height >=
           options.margin_bottom_points - 0.01;
}

[[nodiscard]] double emit_table_row(
    PdfPageLayout &page, const std::vector<TableRowLayout> &rows,
    std::size_t row_index, double row_top,
    const featherdoc::table_inspection_summary &table,
    const PdfDocumentAdapterOptions &options, double line_height_points,
    double cell_spacing_points) {
    const auto &row = rows[row_index];
    const auto row_bottom = row_top - row.height_points;
    for (const auto &cell : row.cells) {
        if (is_vertical_merge_continuation(cell.cell)) {
            continue;
        }
        const auto cell_bottom =
            spanned_row_bottom(rows, row_index, cell, row_top, cell_spacing_points);
        page.rectangles.push_back(PdfRectangle{
            PdfRect{cell.x_points, cell_bottom, cell.width_points,
                    row_top - cell_bottom},
            PdfRgbColor{0.0, 0.0, 0.0},
            cell.fill_color.value_or(PdfRgbColor{1.0, 1.0, 1.0}),
            0.5,
            !has_resolved_cell_border(table, cell),
            cell.fill_color.has_value(),
        });
        emit_cell_border_line(page, table, cell,
                              featherdoc::cell_border_edge::top, row_top,
                              cell_bottom);
        emit_cell_border_line(page, table, cell,
                              featherdoc::cell_border_edge::left, row_top,
                              cell_bottom);
        emit_cell_border_line(page, table, cell,
                              featherdoc::cell_border_edge::bottom, row_top,
                              cell_bottom);
        emit_cell_border_line(page, table, cell,
                              featherdoc::cell_border_edge::right, row_top,
                              cell_bottom);
    }

    for (const auto &cell : row.cells) {
        if (is_vertical_merge_continuation(cell.cell)) {
            continue;
        }
        const auto cell_bottom =
            spanned_row_bottom(rows, row_index, cell, row_top, cell_spacing_points);
        const auto rotation_degrees = cell_text_rotation_degrees(cell.cell);
        if (rotation_degrees.has_value()) {
            emit_rotated_cell_lines(page, cell, row_top, cell_bottom,
                                    *rotation_degrees, line_height_points,
                                    options.font_size_points);
            continue;
        }

        auto line_top = cell_content_top(cell, row_top, cell_bottom);
        const auto minimum_baseline_y =
            cell_bottom + cell.padding_bottom_points;
        for (const auto &line : cell.lines) {
            const auto line_height = line_height_points_for(
                line, line_height_points, options.font_size_points);
            const auto baseline_y =
                line_top -
                line_baseline_offset_points_for(line, options.font_size_points);
            if (baseline_y < minimum_baseline_y) {
                break;
            }
            emit_line_at(page, line, cell.x_points + cell.padding_left_points,
                         baseline_y);
            line_top -= line_height;
        }
    }

    return row_bottom;
}

void emit_repeated_header_rows(
    PdfPageLayout &page, double &current_y,
    const std::vector<TableRowLayout> &rows, std::size_t header_row_count,
    const featherdoc::table_inspection_summary &table,
    const PdfDocumentAdapterOptions &options, double line_height_points,
    double cell_spacing_points) {
    for (std::size_t header_index = 0U; header_index < header_row_count;
         ++header_index) {
        const auto header_top = current_y + options.font_size_points;
        const auto header_bottom =
            emit_table_row(page, rows, header_index, header_top, table, options,
                           line_height_points, cell_spacing_points);
        current_y =
            header_bottom - options.font_size_points - cell_spacing_points;
    }
}

} // namespace

void emit_table(featherdoc::Document &document, PdfPageLayout *&page,
                double &current_y,
                const featherdoc::table_inspection_summary &table,
                const PdfDocumentAdapterOptions &options,
                double max_width_points, double line_height_points,
                const TableRenderContext &context) {
    constexpr auto cell_padding_points = 4.0;
    const auto cells = document.inspect_table_cells(table.index);
    if (cells.empty() && table.row_count == 0U) {
        return;
    }

    auto table_handle = find_table_handle(document, table.index);
    const auto column_count = table_column_count(table, cells);
    const auto column_widths =
        build_table_column_widths(table, column_count, max_width_points);
    const auto rows =
        build_table_rows(table, table_handle ? &*table_handle : nullptr, cells,
                         column_widths, options, max_width_points,
                          line_height_points, cell_padding_points, context);

    const auto cell_spacing_points = table_cell_spacing_points(table);
    const auto header_row_count = repeated_header_row_count(rows);
    const auto header_block_height = repeated_header_block_height(
        rows, header_row_count, cell_spacing_points);
    for (std::size_t row_index = 0U; row_index < rows.size(); ++row_index) {
        const auto &row = rows[row_index];
        const auto anchor_top = current_y + options.font_size_points;
        auto row_top = anchor_top;
        const auto positioned_row_top =
            row_index == 0U ? positioned_table_row_top_points(
                                  table, anchor_top, options)
                            : std::optional<double>{};
        if (positioned_row_top.has_value()) {
            row_top = *positioned_row_top;
        }
        const auto fresh_page_baseline = first_baseline_y(options);
        const auto at_fresh_page = current_y >= fresh_page_baseline - 0.01;
        if (!positioned_row_top.has_value() && !at_fresh_page &&
            row_overflows_page_area(row, row_top, options)) {
            page = &context.new_page();
            current_y = fresh_page_baseline;
            row_top = current_y + options.font_size_points;
            if (repeated_headers_fit_with_row(rows, header_row_count, row_index,
                                              header_block_height, options)) {
                emit_repeated_header_rows(*page, current_y, rows,
                                          header_row_count, table, options,
                                          line_height_points,
                                          cell_spacing_points);
                row_top = current_y + options.font_size_points;
            }
        }

        const auto row_bottom = emit_table_row(*page, rows, row_index, row_top,
                                               table, options,
                                               line_height_points,
                                               cell_spacing_points);
        current_y = row_bottom - options.font_size_points - cell_spacing_points;
    }

    current_y -= options.paragraph_spacing_after_points;
}

} // namespace featherdoc::pdf::detail
