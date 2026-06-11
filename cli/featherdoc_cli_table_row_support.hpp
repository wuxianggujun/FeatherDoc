#pragma once

#include "featherdoc_cli_table_cell_options_parse.hpp"

#include <featherdoc.hpp>

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace featherdoc_cli {

struct table_row_target {
    std::size_t table_index = 0U;
    std::size_t row_index = 0U;
};

auto report_body_table_row_failure(std::string_view command,
                                   std::string_view summary,
                                   std::string detail, bool json_output)
    -> bool;

[[nodiscard]] auto parse_body_table_index(
    std::string_view command, const std::vector<std::string_view> &arguments,
    std::size_t argument_index, std::string_view label, std::size_t &value,
    bool json_output) -> bool;

[[nodiscard]] auto parse_body_table_row_target(
    std::string_view command, const std::vector<std::string_view> &arguments,
    std::size_t minimum_argument_count, std::string_view usage_message,
    table_row_target &target, bool json_output) -> bool;

[[nodiscard]] auto parse_body_table_row_mutation_options(
    std::string_view command, const std::vector<std::string_view> &arguments,
    std::size_t options_start_index, table_cell_style_options &options,
    bool json_output) -> bool;

[[nodiscard]] auto resolve_body_table_for_row(std::string_view command,
                                              featherdoc::Document &doc,
                                              std::size_t table_index,
                                              featherdoc::Table &table,
                                              bool json_output) -> bool;

[[nodiscard]] auto resolve_body_table_row_for_row(
    std::string_view command, featherdoc::Document &doc,
    std::size_t table_index, std::size_t row_index, featherdoc::TableRow &row,
    bool json_output) -> bool;

[[nodiscard]] auto load_body_table_summary(
    std::string_view command, featherdoc::Document &doc,
    std::size_t table_index, featherdoc::table_inspection_summary &table,
    bool json_output) -> bool;

[[nodiscard]] auto load_mutable_body_table_row(
    std::string_view command, const std::vector<std::string_view> &arguments,
    const table_cell_style_options &options, const table_row_target &target,
    featherdoc::Document &doc, featherdoc::TableRow &row) -> bool;

} // namespace featherdoc_cli
