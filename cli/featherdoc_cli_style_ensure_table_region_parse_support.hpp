#pragma once

#include <featherdoc.hpp>

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace featherdoc_cli {

[[nodiscard]] auto split_table_style_region_option_fields(std::string_view text)
    -> std::vector<std::string_view>;

[[nodiscard]] auto ensure_table_style_region_option(
    featherdoc::table_style_definition &definition, std::string_view region_name,
    std::string_view option_name, std::string &error_message)
    -> featherdoc::table_style_region_definition *;

auto assign_table_style_margin(featherdoc::table_style_margins_definition &margins,
                               featherdoc::cell_margin_edge edge,
                               std::uint32_t value) -> void;

auto assign_table_style_border(featherdoc::table_style_borders_definition &borders,
                               featherdoc::table_border_edge edge,
                               featherdoc::border_definition border) -> void;

[[nodiscard]] auto ensure_table_style_paragraph_spacing_option(
    featherdoc::table_style_region_definition &region)
    -> featherdoc::table_style_paragraph_spacing_definition &;

} // namespace featherdoc_cli
