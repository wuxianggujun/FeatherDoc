#pragma once

#include "featherdoc_cli_template_part_selection.hpp"

#include <featherdoc.hpp>

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace featherdoc_cli {

[[nodiscard]] auto resolve_template_table(
    selected_template_part &selected, std::size_t table_index,
    featherdoc::Table &table, std::string_view command, bool json_output,
    std::string_view stage = "inspect") -> bool;

[[nodiscard]] auto resolve_template_table_index(
    featherdoc::Document &doc, selected_template_part &selected,
    const std::optional<std::size_t> &table_index,
    const std::optional<std::string> &bookmark_name,
    std::size_t &resolved_table_index, std::string_view command,
    bool json_output, std::string_view stage = "inspect") -> bool;

[[nodiscard]] auto resolve_template_table_index(
    featherdoc::Document &doc, selected_template_part &selected,
    const featherdoc::template_table_selector &selector,
    std::size_t &resolved_table_index, std::string_view command,
    bool json_output, std::string_view stage = "inspect") -> bool;

[[nodiscard]] auto resolve_template_table_index_for_batch(
    featherdoc::Document &doc, selected_template_part &selected,
    const std::optional<std::size_t> &table_index,
    const std::optional<std::string> &bookmark_name,
    std::size_t &resolved_table_index, std::size_t operation_index,
    std::string_view command, bool json_output,
    std::string_view stage = "mutate") -> bool;

[[nodiscard]] auto resolve_template_table_index_for_batch(
    featherdoc::Document &doc, selected_template_part &selected,
    const featherdoc::template_table_selector &selector,
    std::size_t &resolved_table_index, std::size_t operation_index,
    std::string_view command, bool json_output,
    std::string_view stage = "mutate") -> bool;

[[nodiscard]] auto resolve_template_table_for_batch(
    selected_template_part &selected, std::size_t table_index,
    featherdoc::Table &table, std::size_t operation_index,
    std::string_view command, bool json_output,
    std::string_view stage = "mutate") -> bool;

[[nodiscard]] auto resolve_template_table_row(
    selected_template_part &selected, std::size_t table_index,
    std::size_t row_index, featherdoc::TableRow &row,
    std::string_view command, bool json_output,
    std::string_view stage = "inspect") -> bool;

[[nodiscard]] auto resolve_template_table_cell(
    selected_template_part &selected, std::size_t table_index,
    std::size_t row_index, std::size_t cell_index, featherdoc::TableCell &cell,
    std::string_view command, bool json_output,
    std::string_view stage = "mutate") -> bool;

[[nodiscard]] auto resolve_template_table_cell_by_grid_column(
    selected_template_part &selected, std::size_t table_index,
    std::size_t row_index, std::size_t grid_column,
    featherdoc::TableCell &cell, std::string_view command, bool json_output,
    std::string_view stage = "mutate") -> bool;

[[nodiscard]] auto load_template_table_summary(
    featherdoc::Document &doc, selected_template_part &selected,
    std::size_t table_index, featherdoc::table_inspection_summary &table,
    std::string_view command, bool json_output,
    std::string_view stage = "mutate") -> bool;

[[nodiscard]] auto load_template_table_cell_summary(
    featherdoc::Document &doc, selected_template_part &selected,
    std::size_t table_index, std::size_t row_index, std::size_t cell_index,
    featherdoc::table_cell_inspection_summary &cell, std::string_view command,
    bool json_output, std::string_view stage = "mutate") -> bool;

[[nodiscard]] auto load_template_table_cells_summary(
    featherdoc::Document &doc, selected_template_part &selected,
    std::size_t table_index,
    std::vector<featherdoc::table_cell_inspection_summary> &cells,
    std::string_view command, bool json_output,
    std::string_view stage = "mutate") -> bool;

} // namespace featherdoc_cli
