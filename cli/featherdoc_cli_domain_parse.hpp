#pragma once

#include <featherdoc.hpp>

#include <cstddef>
#include <string>
#include <string_view>

namespace featherdoc_cli {

[[nodiscard]] auto parse_page_orientation(
    std::string_view text, featherdoc::page_orientation &orientation) -> bool;
[[nodiscard]] auto parse_list_kind(std::string_view text,
                                   featherdoc::list_kind &kind) -> bool;
[[nodiscard]] auto parse_reference_kind(
    std::string_view text, featherdoc::section_reference_kind &reference_kind)
    -> bool;
[[nodiscard]] auto parse_json_patch_reference_kind_value(
    std::string_view text, std::size_t &index,
    featherdoc::section_reference_kind &reference_kind,
    std::string &error_message) -> bool;
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
[[nodiscard]] auto parse_cell_margin_edge_text(
    std::string_view text, featherdoc::cell_margin_edge &edge) -> bool;
[[nodiscard]] auto parse_cell_border_edge_text(
    std::string_view text, featherdoc::cell_border_edge &edge) -> bool;
[[nodiscard]] auto parse_table_style_border_edge_text(
    std::string_view text, featherdoc::table_border_edge &edge) -> bool;
[[nodiscard]] auto parse_table_border_edge_text(
    std::string_view text, featherdoc::table_border_edge &edge) -> bool;
[[nodiscard]] auto parse_table_style_border_style_text(
    std::string_view text, featherdoc::border_style &style) -> bool;
[[nodiscard]] auto parse_border_style_text(
    std::string_view text, featherdoc::border_style &style) -> bool;
[[nodiscard]] auto parse_table_style_cell_vertical_alignment_text(
    std::string_view text, featherdoc::cell_vertical_alignment &alignment)
    -> bool;
[[nodiscard]] auto parse_cell_vertical_alignment_text(
    std::string_view text, featherdoc::cell_vertical_alignment &alignment)
    -> bool;
[[nodiscard]] auto parse_table_style_cell_text_direction_text(
    std::string_view text, featherdoc::cell_text_direction &direction) -> bool;
[[nodiscard]] auto parse_cell_text_direction_text(
    std::string_view text, featherdoc::cell_text_direction &direction) -> bool;
[[nodiscard]] auto parse_table_style_paragraph_alignment_text(
    std::string_view text, featherdoc::paragraph_alignment &alignment) -> bool;
[[nodiscard]] auto parse_table_style_paragraph_line_spacing_rule_text(
    std::string_view text, featherdoc::paragraph_line_spacing_rule &rule)
    -> bool;
[[nodiscard]] auto parse_row_height_rule_text(
    std::string_view text, featherdoc::row_height_rule &height_rule) -> bool;
[[nodiscard]] auto parse_table_layout_mode_text(
    std::string_view text, featherdoc::table_layout_mode &layout_mode) -> bool;
[[nodiscard]] auto parse_table_alignment_text(
    std::string_view text, featherdoc::table_alignment &alignment) -> bool;
[[nodiscard]] auto parse_table_position_horizontal_reference(
    std::string_view text,
    featherdoc::table_position_horizontal_reference &reference) -> bool;
[[nodiscard]] auto parse_table_position_vertical_reference(
    std::string_view text,
    featherdoc::table_position_vertical_reference &reference) -> bool;
[[nodiscard]] auto parse_table_position_horizontal_spec(
    std::string_view text, featherdoc::table_position_horizontal_spec &spec)
    -> bool;
[[nodiscard]] auto parse_table_position_vertical_spec(
    std::string_view text, featherdoc::table_position_vertical_spec &spec)
    -> bool;
[[nodiscard]] auto parse_table_overlap_text(
    std::string_view text, featherdoc::table_overlap &overlap) -> bool;

} // namespace featherdoc_cli
