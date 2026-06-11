#pragma once

#include "featherdoc_cli_table_cell_options_parse.hpp"

#include <featherdoc.hpp>

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace featherdoc_cli {

auto parse_table_structure_index_arg(
    std::string_view command, const std::vector<std::string_view> &arguments,
    std::size_t argument_index, std::string_view label, std::size_t &value,
    bool json_output) -> bool;

auto parse_table_structure_table_index(
    std::string_view command, const std::vector<std::string_view> &arguments,
    std::size_t &table_index, bool json_output) -> bool;

auto parse_table_structure_style_options(
    std::string_view command, const std::vector<std::string_view> &arguments,
    std::size_t option_start, table_cell_style_options &options,
    bool json_output) -> bool;

auto report_table_structure_failure(std::string_view command,
                                    std::string_view summary,
                                    std::string detail, bool json_output)
    -> bool;

auto report_invalid_table_structure_target(std::string_view command,
                                           std::string_view summary,
                                           bool json_output) -> bool;

auto load_body_table_summary_for_structure(
    std::string_view command, featherdoc::Document &doc,
    std::size_t table_index, featherdoc::table_inspection_summary &table,
    bool json_output) -> bool;

auto validate_table_structure_column_index(
    std::string_view command, std::size_t table_index,
    std::size_t column_index,
    const featherdoc::table_inspection_summary &table, bool json_output)
    -> bool;

auto parse_table_column_width_target(
    std::string_view command, const std::vector<std::string_view> &arguments,
    std::size_t minimum_argument_count, std::string_view usage_message,
    std::size_t &table_index, std::size_t &column_index, bool json_output)
    -> bool;

} // namespace featherdoc_cli
