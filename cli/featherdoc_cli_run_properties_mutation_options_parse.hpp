#pragma once

#include "featherdoc_cli_run_properties_options_parse.hpp"

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace featherdoc_cli {

[[nodiscard]] auto parse_set_run_properties_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    set_default_run_properties_options &options, std::string &error_message,
    std::string_view required_option_message) -> bool;

[[nodiscard]] auto parse_set_run_properties_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    set_style_run_properties_options &options, std::string &error_message,
    std::string_view required_option_message) -> bool;

[[nodiscard]] auto parse_clear_run_properties_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    clear_default_run_properties_options &options, std::string &error_message,
    std::string_view required_option_message) -> bool;

[[nodiscard]] auto parse_clear_run_properties_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    clear_style_run_properties_options &options, std::string &error_message,
    std::string_view required_option_message) -> bool;

} // namespace featherdoc_cli
