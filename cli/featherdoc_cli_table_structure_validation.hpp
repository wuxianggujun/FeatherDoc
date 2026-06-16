#pragma once

#include <featherdoc.hpp>

#include <cstddef>
#include <vector>

namespace featherdoc_cli {

[[nodiscard]] auto insertion_boundary_intersects_horizontal_merge(
    const std::vector<featherdoc::table_cell_inspection_summary> &cells,
    std::size_t boundary_column_index) -> bool;

[[nodiscard]] auto column_intersects_horizontal_merge(
    const std::vector<featherdoc::table_cell_inspection_summary> &cells,
    std::size_t column_index) -> bool;

} // namespace featherdoc_cli
