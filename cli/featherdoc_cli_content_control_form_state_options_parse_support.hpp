#pragma once

#include "featherdoc_cli_content_control_form_state_options_parse.hpp"
#include "featherdoc_cli_parse.hpp"

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace featherdoc_cli {

[[nodiscard]] auto parse_content_control_form_state_selector_option(
    std::string_view argument, const std::vector<std::string_view> &arguments,
    std::size_t &index, set_content_control_form_state_options &options,
    std::string &error_message) -> option_parse_result;
[[nodiscard]] auto parse_content_control_form_state_value_option(
    std::string_view argument, const std::vector<std::string_view> &arguments,
    std::size_t &index, set_content_control_form_state_options &options,
    std::string &error_message) -> option_parse_result;
[[nodiscard]] auto parse_content_control_form_state_output_option(
    std::string_view argument, const std::vector<std::string_view> &arguments,
    std::size_t &index, set_content_control_form_state_options &options,
    std::string &error_message) -> option_parse_result;
[[nodiscard]] auto validate_content_control_form_state_options(
    const set_content_control_form_state_options &options,
    std::string &error_message) -> bool;

} // namespace featherdoc_cli
