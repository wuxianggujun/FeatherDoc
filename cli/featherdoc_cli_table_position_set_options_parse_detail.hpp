#pragma once

#include "featherdoc_cli_parse.hpp"
#include "featherdoc_cli_table_position_options_parse.hpp"

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace featherdoc_cli {

[[nodiscard]] auto parse_table_position_anchor_option(
    const std::vector<std::string_view> &arguments, std::size_t &index,
    table_position_options &options, std::string &error_message)
    -> option_parse_result;

[[nodiscard]] auto parse_table_position_spacing_option(
    const std::vector<std::string_view> &arguments, std::size_t &index,
    table_position_options &options, std::string &error_message)
    -> option_parse_result;

} // namespace featherdoc_cli
