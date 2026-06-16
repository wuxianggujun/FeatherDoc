#include "featherdoc_cli_table_structure_validation.hpp"

namespace featherdoc_cli {

auto insertion_boundary_intersects_horizontal_merge(
    const std::vector<featherdoc::table_cell_inspection_summary> &cells,
    std::size_t boundary_column_index) -> bool {
    for (const auto &cell : cells) {
        if (cell.column_span <= 1U) {
            continue;
        }

        const auto cell_end_column = cell.column_index + cell.column_span;
        if (cell.column_index < boundary_column_index &&
            boundary_column_index < cell_end_column) {
            return true;
        }
    }

    return false;
}

auto column_intersects_horizontal_merge(
    const std::vector<featherdoc::table_cell_inspection_summary> &cells,
    std::size_t column_index) -> bool {
    for (const auto &cell : cells) {
        if (cell.column_span <= 1U) {
            continue;
        }

        const auto cell_end_column = cell.column_index + cell.column_span;
        if (cell.column_index <= column_index && column_index < cell_end_column) {
            return true;
        }
    }

    return false;
}

} // namespace featherdoc_cli
