#pragma once

#include "featherdoc_cli_domain_parse.hpp"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace featherdoc_cli {

enum class table_merge_direction {
    right,
    down,
};

struct table_cell_text_options {
    std::optional<std::filesystem::path> output_path;
    std::optional<std::string> text;
    std::optional<std::filesystem::path> text_file;
    std::optional<std::size_t> grid_column;
    bool json_output = false;
};

struct merge_table_cells_options {
    table_merge_direction direction = table_merge_direction::right;
    bool has_direction = false;
    std::size_t count = 1U;
    bool has_count = false;
    std::optional<std::filesystem::path> output_path;
    bool json_output = false;
};

struct unmerge_table_cells_options {
    table_merge_direction direction = table_merge_direction::right;
    bool has_direction = false;
    std::optional<std::filesystem::path> output_path;
    bool json_output = false;
};

struct table_cell_style_options {
    std::optional<std::filesystem::path> output_path;
    bool json_output = false;
};

struct append_table_row_options {
    std::optional<std::size_t> cell_count;
    std::optional<std::filesystem::path> output_path;
    bool json_output = false;
};

struct table_cell_border_options {
    std::optional<featherdoc::border_style> style;
    std::optional<std::uint32_t> size_eighth_points;
    std::optional<std::string> color;
    std::optional<std::uint32_t> space_points;
    std::optional<std::filesystem::path> output_path;
    bool json_output = false;
};

[[nodiscard]] auto parse_table_merge_direction(
    std::string_view text, table_merge_direction &direction) -> bool;

[[nodiscard]] auto table_merge_direction_name(table_merge_direction direction)
    -> const char *;

[[nodiscard]] auto parse_table_cell_text_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    table_cell_text_options &options, std::string &error_message) -> bool;

[[nodiscard]] auto parse_merge_table_cells_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    merge_table_cells_options &options, std::string &error_message) -> bool;

[[nodiscard]] auto parse_unmerge_table_cells_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    unmerge_table_cells_options &options, std::string &error_message) -> bool;

[[nodiscard]] auto parse_table_cell_style_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    table_cell_style_options &options, std::string &error_message) -> bool;

[[nodiscard]] auto parse_append_table_row_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    append_table_row_options &options, std::string &error_message) -> bool;

[[nodiscard]] auto parse_table_cell_border_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    table_cell_border_options &options, std::string &error_message) -> bool;

} // namespace featherdoc_cli
