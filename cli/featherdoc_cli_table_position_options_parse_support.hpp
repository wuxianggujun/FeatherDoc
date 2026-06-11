#pragma once

#include "featherdoc_cli_parse.hpp"
#include "featherdoc_cli_table_position_plan_parse.hpp"

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace featherdoc_cli {

[[nodiscard]] auto parse_table_position_table_option(
    const std::vector<std::string_view> &arguments, std::size_t &index,
    std::vector<std::size_t> &additional_table_indices,
    std::string &error_message) -> option_parse_result;

[[nodiscard]] auto parse_table_position_output_option(
    const std::vector<std::string_view> &arguments, std::size_t &index,
    std::optional<std::filesystem::path> &output_path,
    std::string &error_message) -> option_parse_result;

[[nodiscard]] auto parse_table_position_preset_option(
    const std::vector<std::string_view> &arguments, std::size_t &index,
    std::optional<table_position_preset> &preset,
    std::string &error_message) -> option_parse_result;

} // namespace featherdoc_cli
