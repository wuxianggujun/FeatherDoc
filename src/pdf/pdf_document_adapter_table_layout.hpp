#ifndef FEATHERDOC_PDF_DOCUMENT_ADAPTER_TABLE_LAYOUT_HPP
#define FEATHERDOC_PDF_DOCUMENT_ADAPTER_TABLE_LAYOUT_HPP

#include <featherdoc.hpp>
#include <featherdoc/pdf/pdf_document_adapter.hpp>

#include "pdf_document_adapter_tables.hpp"
#include "pdf_document_adapter_text.hpp"

#include <cstddef>
#include <optional>
#include <vector>

namespace featherdoc::pdf::detail {

struct TableCellLayout {
    featherdoc::table_cell_inspection_summary cell;
    double x_points{0.0};
    double width_points{0.0};
    double padding_top_points{4.0};
    double padding_left_points{4.0};
    double padding_bottom_points{4.0};
    double padding_right_points{4.0};
    double content_height_points{0.0};
    std::optional<PdfRgbColor> fill_color;
    std::vector<LineState> lines;
};

struct TableRowLayout {
    std::vector<TableCellLayout> cells;
    double height_points{0.0};
    bool cant_split{false};
    bool repeats_header{false};
};

[[nodiscard]] std::optional<featherdoc::Table>
find_table_handle(featherdoc::Document &document, std::size_t table_index);

[[nodiscard]] std::size_t table_column_count(
    const featherdoc::table_inspection_summary &table,
    const std::vector<featherdoc::table_cell_inspection_summary> &cells);

[[nodiscard]] std::vector<double>
build_table_column_widths(const featherdoc::table_inspection_summary &table,
                          std::size_t column_count, double max_width_points);

[[nodiscard]] double table_cell_spacing_points(
    const featherdoc::table_inspection_summary &table) noexcept;

[[nodiscard]] std::optional<double> positioned_table_row_top_points(
    const featherdoc::table_inspection_summary &table, double anchor_top_points,
    const PdfDocumentAdapterOptions &options) noexcept;

[[nodiscard]] bool is_vertical_merge_continuation(
    const featherdoc::table_cell_inspection_summary &cell) noexcept;

[[nodiscard]] std::optional<double> cell_text_rotation_degrees(
    const featherdoc::table_cell_inspection_summary &cell) noexcept;

[[nodiscard]] std::vector<TableRowLayout> build_table_rows(
    const featherdoc::table_inspection_summary &table,
    featherdoc::Table *table_handle,
    const std::vector<featherdoc::table_cell_inspection_summary> &cells,
    const std::vector<double> &column_widths,
    const PdfDocumentAdapterOptions &options, double max_width_points,
    double line_height_points, double cell_padding_points,
    const TableRenderContext &context);

[[nodiscard]] double cell_content_top(const TableCellLayout &cell,
                                      double row_top,
                                      double row_bottom) noexcept;

[[nodiscard]] double cell_minimum_baseline_y(const TableCellLayout &cell,
                                             double row_top,
                                             double row_bottom) noexcept;

[[nodiscard]] double spanned_row_bottom(const std::vector<TableRowLayout> &rows,
                                        std::size_t row_index,
                                        const TableCellLayout &cell,
                                        double row_top,
                                        double cell_spacing_points) noexcept;

} // namespace featherdoc::pdf::detail

#endif // FEATHERDOC_PDF_DOCUMENT_ADAPTER_TABLE_LAYOUT_HPP
