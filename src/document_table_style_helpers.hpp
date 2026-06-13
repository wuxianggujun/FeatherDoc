#pragma once

#include "featherdoc.hpp"

#include <cstddef>
#include <optional>
#include <string_view>

namespace featherdoc::detail {

[[nodiscard]] auto validate_table_style_regions(
    document_error_info &last_error_info,
    const table_style_definition &definition) -> bool;

[[nodiscard]] auto apply_table_style_regions(
    pugi::xml_node style, const table_style_definition &definition) -> bool;

[[nodiscard]] auto read_table_style_region(pugi::xml_node parent,
                                           bool whole_table)
    -> std::optional<table_style_region_summary>;

[[nodiscard]] auto read_table_style_conditional_region(
    pugi::xml_node style, std::string_view type_name)
    -> std::optional<table_style_region_summary>;

[[nodiscard]] auto find_table_style_conditional_node(
    pugi::xml_node style, std::string_view type_name) -> pugi::xml_node;

[[nodiscard]] auto table_style_region_property_count(
    const table_style_region_summary &region) -> std::size_t;

[[nodiscard]] auto table_style_whole_region_declared(pugi::xml_node style)
    -> bool;

} // namespace featherdoc::detail
