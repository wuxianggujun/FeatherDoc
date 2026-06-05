#pragma once

#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace featherdoc_cli {

struct check_table_style_look_options {
    bool fail_on_issue = false;
    bool json_output = false;
};

struct repair_table_style_look_options {
    bool plan_only = false;
    bool apply = false;
    std::optional<std::filesystem::path> output_path;
    bool json_output = false;
};

struct table_style_look_options {
    std::optional<bool> first_row;
    std::optional<bool> last_row;
    std::optional<bool> first_column;
    std::optional<bool> last_column;
    std::optional<bool> banded_rows;
    std::optional<bool> banded_columns;
    std::optional<std::filesystem::path> output_path;
    bool json_output = false;
};

[[nodiscard]] auto parse_check_table_style_look_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    check_table_style_look_options &options, std::string &error_message) -> bool;

[[nodiscard]] auto parse_repair_table_style_look_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    repair_table_style_look_options &options, std::string &error_message) -> bool;

[[nodiscard]] auto table_style_look_options_have_flag(
    const table_style_look_options &options) -> bool;

[[nodiscard]] auto parse_table_style_look_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    table_style_look_options &options, std::string &error_message) -> bool;

} // namespace featherdoc_cli
