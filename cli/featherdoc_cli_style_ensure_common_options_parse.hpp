#pragma once

#include "featherdoc_cli_parse.hpp"

#include <featherdoc.hpp>

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace featherdoc_cli {

[[nodiscard]] auto parse_style_catalog_option(
    const std::vector<std::string_view> &arguments, std::size_t &index,
    featherdoc::paragraph_style_definition &definition,
    std::string &error_message) -> option_parse_result;

[[nodiscard]] auto parse_style_catalog_option(
    const std::vector<std::string_view> &arguments, std::size_t &index,
    featherdoc::character_style_definition &definition,
    std::string &error_message) -> option_parse_result;

[[nodiscard]] auto parse_style_catalog_option(
    const std::vector<std::string_view> &arguments, std::size_t &index,
    featherdoc::table_style_definition &definition,
    std::string &error_message) -> option_parse_result;

[[nodiscard]] auto parse_run_style_option(
    const std::vector<std::string_view> &arguments, std::size_t &index,
    featherdoc::paragraph_style_definition &definition,
    std::string &error_message) -> option_parse_result;

[[nodiscard]] auto parse_run_style_option(
    const std::vector<std::string_view> &arguments, std::size_t &index,
    featherdoc::character_style_definition &definition,
    std::string &error_message) -> option_parse_result;

} // namespace featherdoc_cli
