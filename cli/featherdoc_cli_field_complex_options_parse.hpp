#pragma once

#include "featherdoc_cli_field_parse.hpp"

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace featherdoc_cli {

enum class field_complex_option_parse_result {
    not_handled,
    handled,
    error
};

[[nodiscard]] auto parse_complex_field_option(
    std::string_view command, std::string_view argument,
    const std::vector<std::string_view> &arguments, std::size_t &index,
    append_field_options &options, bool is_complex,
    std::string &error_message) -> field_complex_option_parse_result;

[[nodiscard]] auto validate_complex_field_options(
    const append_field_options &options, std::string &error_message) -> bool;

} // namespace featherdoc_cli
