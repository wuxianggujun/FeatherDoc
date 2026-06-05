#pragma once

#include "featherdoc_cli_table_position_plan_parse.hpp"

#include <featherdoc.hpp>

#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace featherdoc_cli {

struct table_position_options {
    featherdoc::table_position position{};
    std::optional<table_position_preset> preset;
    std::vector<std::size_t> additional_table_indices;
    bool has_horizontal_reference = false;
    bool has_horizontal_offset = false;
    bool has_horizontal_spec = false;
    bool has_vertical_reference = false;
    bool has_vertical_offset = false;
    bool has_vertical_spec = false;
    std::optional<std::filesystem::path> output_path;
    bool json_output = false;
};

struct table_position_target_options {
    std::vector<std::size_t> additional_table_indices;
    std::optional<std::filesystem::path> output_path;
    bool json_output = false;
};

struct plan_table_position_presets_options {
    std::optional<table_position_preset> preset;
    std::optional<std::filesystem::path> input_path;
    std::optional<std::filesystem::path> output_path;
    std::optional<std::filesystem::path> output_plan_path;
    bool replace_positioned = false;
    bool fail_on_change = false;
    bool json_output = false;
};

struct apply_table_position_plan_options {
    std::optional<std::filesystem::path> output_path;
    bool dry_run = false;
    bool json_output = false;
};

void apply_table_position_preset_defaults(table_position_options &options);

[[nodiscard]] auto parse_table_position_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    table_position_options &options, std::string &error_message) -> bool;

[[nodiscard]] auto parse_table_position_target_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    table_position_target_options &options, std::string &error_message) -> bool;

[[nodiscard]] auto parse_apply_table_position_plan_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    apply_table_position_plan_options &options, std::string &error_message)
    -> bool;

[[nodiscard]] auto parse_plan_table_position_presets_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    plan_table_position_presets_options &options, std::string &error_message)
    -> bool;

} // namespace featherdoc_cli
