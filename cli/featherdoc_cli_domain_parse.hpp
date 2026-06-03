#pragma once

#include <featherdoc.hpp>

#include <string_view>

namespace featherdoc_cli {

[[nodiscard]] auto parse_page_orientation(
    std::string_view text, featherdoc::page_orientation &orientation) -> bool;
[[nodiscard]] auto parse_list_kind(std::string_view text,
                                   featherdoc::list_kind &kind) -> bool;
[[nodiscard]] auto parse_reference_kind(
    std::string_view text, featherdoc::section_reference_kind &reference_kind)
    -> bool;
[[nodiscard]] auto parse_floating_image_horizontal_reference(
    std::string_view text,
    featherdoc::floating_image_horizontal_reference &reference) -> bool;
[[nodiscard]] auto parse_floating_image_vertical_reference(
    std::string_view text,
    featherdoc::floating_image_vertical_reference &reference) -> bool;
[[nodiscard]] auto parse_floating_image_wrap_mode(
    std::string_view text, featherdoc::floating_image_wrap_mode &mode) -> bool;
[[nodiscard]] auto parse_table_style_margin_edge_text(
    std::string_view text, featherdoc::cell_margin_edge &edge) -> bool;
[[nodiscard]] auto parse_table_style_border_edge_text(
    std::string_view text, featherdoc::table_border_edge &edge) -> bool;
[[nodiscard]] auto parse_table_style_border_style_text(
    std::string_view text, featherdoc::border_style &style) -> bool;
[[nodiscard]] auto parse_table_style_cell_vertical_alignment_text(
    std::string_view text, featherdoc::cell_vertical_alignment &alignment)
    -> bool;
[[nodiscard]] auto parse_table_style_cell_text_direction_text(
    std::string_view text, featherdoc::cell_text_direction &direction) -> bool;
[[nodiscard]] auto parse_table_style_paragraph_alignment_text(
    std::string_view text, featherdoc::paragraph_alignment &alignment) -> bool;
[[nodiscard]] auto parse_table_style_paragraph_line_spacing_rule_text(
    std::string_view text, featherdoc::paragraph_line_spacing_rule &rule)
    -> bool;

} // namespace featherdoc_cli
