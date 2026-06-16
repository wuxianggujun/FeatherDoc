#pragma once

#include "featherdoc_cli_table_format_support.hpp"

#include <featherdoc.hpp>

#include <cstddef>
#include <iosfwd>
#include <string>
#include <string_view>
#include <vector>

namespace featherdoc_cli {

[[nodiscard]] auto open_table_cell_for_style(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc, std::size_t table_index,
    std::size_t row_index, std::size_t cell_index,
    const table_cell_style_options &options, featherdoc::TableCell &cell)
    -> bool;

auto report_invalid_table_cell_target(
    std::string_view command, std::string_view summary, std::string detail,
    bool json_output) -> bool;

[[nodiscard]] auto run_set_table_cell_fill_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int;

[[nodiscard]] auto run_clear_table_cell_fill_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int;

[[nodiscard]] auto run_set_table_cell_vertical_alignment_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int;

[[nodiscard]] auto run_clear_table_cell_vertical_alignment_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int;

[[nodiscard]] auto run_set_table_cell_text_direction_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int;

[[nodiscard]] auto run_clear_table_cell_text_direction_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int;

[[nodiscard]] auto run_set_table_cell_width_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int;

[[nodiscard]] auto run_clear_table_cell_width_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int;

} // namespace featherdoc_cli
