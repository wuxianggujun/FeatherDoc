#pragma once

#include "featherdoc_cli_table_row_summary.hpp"

#include <featherdoc.hpp>

#include <cstddef>
#include <optional>
#include <vector>

namespace featherdoc_cli {

void inspect_tables(
    const std::vector<featherdoc::table_inspection_summary> &tables,
    bool json_output);

void inspect_table(const featherdoc::table_inspection_summary &table,
                   bool json_output);

void inspect_table_cells(
    std::size_t table_index, std::optional<std::size_t> row_index,
    const std::vector<featherdoc::table_cell_inspection_summary> &cells,
    bool json_output);

void inspect_table_cell(std::size_t table_index,
                        const featherdoc::table_cell_inspection_summary &cell,
                        bool json_output);

void inspect_table_rows(std::size_t table_index,
                        const std::vector<table_row_inspection_summary> &rows,
                        bool json_output);

void inspect_table_row(std::size_t table_index,
                       const table_row_inspection_summary &row,
                       bool json_output);

} // namespace featherdoc_cli