#pragma once

#include "featherdoc_cli_run_properties_options_parse.hpp"

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace featherdoc_cli {

[[nodiscard]] auto parse_json_only_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    inspect_default_run_properties_options &options, std::string &error_message)
    -> bool;

[[nodiscard]] auto parse_json_only_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    inspect_style_run_properties_options &options, std::string &error_message)
    -> bool;

[[nodiscard]] auto parse_json_only_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    inspect_paragraph_style_properties_options &options,
    std::string &error_message) -> bool;

[[nodiscard]] auto parse_output_json_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    materialize_style_run_properties_options &options,
    std::string &error_message) -> bool;

[[nodiscard]] auto parse_output_json_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    rebase_style_based_on_options &options, std::string &error_message) -> bool;

} // namespace featherdoc_cli
