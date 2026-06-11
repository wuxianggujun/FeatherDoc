#pragma once

#include "featherdoc_cli_table_cell_options_parse.hpp"

#include <featherdoc.hpp>

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace featherdoc_cli {

struct table_column_target {
    std::size_t table_index = 0U;
    std::size_t row_index = 0U;
    std::size_t cell_index = 0U;
};

auto report_body_table_column_failure(std::string_view command,
                                      std::string_view summary,
                                      std::string detail, bool json_output)
    -> bool;

auto parse_body_table_column_target(
    std::string_view command, const std::vector<std::string_view> &arguments,
    std::string_view usage_message, table_column_target &target,
    table_cell_style_options &options) -> bool;

auto resolve_body_table_cell_for_column(std::string_view command,
                                        featherdoc::Document &doc,
                                        const table_column_target &target,
                                        featherdoc::TableCell &cell,
                                        bool json_output) -> bool;

auto load_body_table_summary_for_column(
    std::string_view command, featherdoc::Document &doc,
    std::size_t table_index, featherdoc::table_inspection_summary &table,
    bool json_output) -> bool;

auto load_body_table_cell_summary_for_column(
    std::string_view command, featherdoc::Document &doc,
    const table_column_target &target,
    featherdoc::table_cell_inspection_summary &cell, bool json_output) -> bool;

auto load_body_table_cells_summary_for_column(
    std::string_view command, featherdoc::Document &doc,
    std::size_t table_index,
    std::vector<featherdoc::table_cell_inspection_summary> &cells,
    bool json_output) -> bool;

} // namespace featherdoc_cli
