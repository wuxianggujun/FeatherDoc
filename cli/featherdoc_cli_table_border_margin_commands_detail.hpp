#pragma once

#include <featherdoc.hpp>

#include <string_view>
#include <vector>

namespace featherdoc_cli {

[[nodiscard]] auto run_set_table_default_cell_margin_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int;

[[nodiscard]] auto run_clear_table_default_cell_margin_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int;

[[nodiscard]] auto run_set_table_border_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int;

[[nodiscard]] auto run_clear_table_border_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int;

[[nodiscard]] auto run_set_table_cell_margin_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int;

[[nodiscard]] auto run_clear_table_cell_margin_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int;

[[nodiscard]] auto run_set_table_cell_border_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int;

[[nodiscard]] auto run_clear_table_cell_border_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int;

} // namespace featherdoc_cli
