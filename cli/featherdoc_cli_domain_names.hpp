#pragma once

#include <featherdoc.hpp>

#include <string>
#include <string_view>

namespace featherdoc_cli {

[[nodiscard]] auto style_kind_name(featherdoc::style_kind kind) -> const char *;
[[nodiscard]] auto list_kind_name(featherdoc::list_kind kind) -> const char *;
[[nodiscard]] auto bookmark_kind_name(featherdoc::bookmark_kind kind)
    -> const char *;
[[nodiscard]] auto content_control_kind_name(
    featherdoc::content_control_kind kind) -> const char *;
[[nodiscard]] auto content_control_form_kind_name(
    featherdoc::content_control_form_kind kind) -> const char *;
[[nodiscard]] auto review_note_kind_name(featherdoc::review_note_kind kind)
    -> const char *;
[[nodiscard]] auto revision_kind_name(featherdoc::revision_kind kind)
    -> const char *;
[[nodiscard]] auto template_slot_kind_name(featherdoc::template_slot_kind kind)
    -> const char *;
[[nodiscard]] auto template_slot_source_json_name(
    featherdoc::template_slot_source_kind source) -> const char *;
[[nodiscard]] auto template_slot_source_text_name(
    featherdoc::template_slot_source_kind source) -> const char *;
[[nodiscard]] auto template_slot_source_new_json_name(
    featherdoc::template_slot_source_kind source) -> std::string;
[[nodiscard]] auto drawing_image_placement_name(
    featherdoc::drawing_image_placement placement) -> const char *;
[[nodiscard]] auto floating_image_horizontal_reference_name(
    featherdoc::floating_image_horizontal_reference reference) -> const char *;
[[nodiscard]] auto floating_image_vertical_reference_name(
    featherdoc::floating_image_vertical_reference reference) -> const char *;
[[nodiscard]] auto floating_image_wrap_mode_name(
    featherdoc::floating_image_wrap_mode mode) -> const char *;
[[nodiscard]] auto page_orientation_name(
    featherdoc::page_orientation orientation) -> std::string_view;
[[nodiscard]] auto style_usage_part_kind_name(
    featherdoc::style_usage_part_kind part_kind) -> const char *;
[[nodiscard]] auto style_usage_hit_kind_name(
    featherdoc::style_usage_hit_kind hit_kind) -> const char *;
[[nodiscard]] auto table_layout_mode_name(
    featherdoc::table_layout_mode layout_mode) noexcept -> std::string_view;
[[nodiscard]] auto table_alignment_name(
    featherdoc::table_alignment alignment) noexcept -> std::string_view;
[[nodiscard]] auto cell_margin_edge_name(
    featherdoc::cell_margin_edge edge) noexcept -> std::string_view;
[[nodiscard]] auto cell_border_edge_name(
    featherdoc::cell_border_edge edge) noexcept -> std::string_view;
[[nodiscard]] auto table_border_edge_name(
    featherdoc::table_border_edge edge) noexcept -> std::string_view;
[[nodiscard]] auto border_style_name(
    featherdoc::border_style style) noexcept -> std::string_view;
[[nodiscard]] auto row_height_rule_name(
    featherdoc::row_height_rule height_rule) noexcept -> std::string_view;
[[nodiscard]] auto cell_text_direction_name(
    featherdoc::cell_text_direction direction) noexcept -> std::string_view;
[[nodiscard]] auto table_position_horizontal_reference_name(
    featherdoc::table_position_horizontal_reference reference) noexcept
    -> std::string_view;
[[nodiscard]] auto table_position_vertical_reference_name(
    featherdoc::table_position_vertical_reference reference) noexcept
    -> std::string_view;
[[nodiscard]] auto table_position_horizontal_spec_name(
    featherdoc::table_position_horizontal_spec spec) noexcept
    -> std::string_view;
[[nodiscard]] auto table_position_vertical_spec_name(
    featherdoc::table_position_vertical_spec spec) noexcept
    -> std::string_view;
[[nodiscard]] auto table_overlap_name(
    featherdoc::table_overlap overlap) noexcept -> std::string_view;

} // namespace featherdoc_cli
