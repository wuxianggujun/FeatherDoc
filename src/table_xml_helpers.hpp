#pragma once

#include "featherdoc.hpp"

#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <pugixml.hpp>

namespace featherdoc::detail {

inline constexpr auto table_style_look_first_row_bit = std::uint16_t{0x0020};
inline constexpr auto table_style_look_last_row_bit = std::uint16_t{0x0040};
inline constexpr auto table_style_look_first_column_bit = std::uint16_t{0x0080};
inline constexpr auto table_style_look_last_column_bit = std::uint16_t{0x0100};
inline constexpr auto table_style_look_no_hband_bit = std::uint16_t{0x0200};
inline constexpr auto table_style_look_no_vband_bit = std::uint16_t{0x0400};

enum class cell_vertical_merge_state {
    none = 0,
    restart,
    continue_merge,
};

[[nodiscard]] auto count_named_children(pugi::xml_node parent,
                                        std::string_view child_name)
    -> std::size_t;
[[nodiscard]] auto string_matrix_from_initializer_list(
    std::initializer_list<std::initializer_list<std::string>> rows)
    -> std::vector<std::vector<std::string>>;
void ensure_attribute_value(pugi::xml_node node, const char *name,
                            const char *value);
void ensure_default_attribute_value(pugi::xml_node node, const char *name,
                                    const char *value);
[[nodiscard]] auto ensure_table_properties_node(pugi::xml_node table)
    -> pugi::xml_node;
[[nodiscard]] auto ensure_table_look_node(pugi::xml_node table)
    -> pugi::xml_node;
[[nodiscard]] auto encode_table_style_look(featherdoc::table_style_look style_look)
    -> std::uint16_t;
[[nodiscard]] auto format_short_hex(std::uint16_t value) -> std::string;
[[nodiscard]] auto parse_xml_on_off_value(std::string_view value)
    -> std::optional<bool>;
[[nodiscard]] auto parse_short_hex_value(std::string_view value)
    -> std::optional<std::uint16_t>;
[[nodiscard]] auto decode_table_style_look_flag(
    std::optional<bool> attribute_value, std::optional<std::uint16_t> encoded_value,
    std::uint16_t bit, bool default_value, bool inverted = false) -> bool;
void ensure_default_table_properties(pugi::xml_node table);
[[nodiscard]] auto ensure_table_width_node(pugi::xml_node table)
    -> pugi::xml_node;
[[nodiscard]] auto ensure_table_layout_node(pugi::xml_node table)
    -> pugi::xml_node;
[[nodiscard]] auto ensure_table_alignment_node(pugi::xml_node table)
    -> pugi::xml_node;
[[nodiscard]] auto ensure_table_indent_node(pugi::xml_node table)
    -> pugi::xml_node;
[[nodiscard]] auto ensure_table_cell_spacing_node(pugi::xml_node table)
    -> pugi::xml_node;
[[nodiscard]] auto ensure_table_style_node(pugi::xml_node table)
    -> pugi::xml_node;
[[nodiscard]] auto ensure_table_position_node(pugi::xml_node table)
    -> pugi::xml_node;
[[nodiscard]] auto ensure_table_cell_margins_node(pugi::xml_node table)
    -> pugi::xml_node;
[[nodiscard]] auto ensure_table_cell_margin_node(pugi::xml_node table,
                                                 const char *margin_name)
    -> pugi::xml_node;
[[nodiscard]] auto ensure_table_borders_node(pugi::xml_node table)
    -> pugi::xml_node;
[[nodiscard]] auto ensure_table_grid_node(pugi::xml_node table)
    -> pugi::xml_node;
[[nodiscard]] auto ensure_row_properties_node(pugi::xml_node row)
    -> pugi::xml_node;
[[nodiscard]] auto ensure_row_height_node(pugi::xml_node row)
    -> pugi::xml_node;
[[nodiscard]] auto ensure_row_cant_split_node(pugi::xml_node row)
    -> pugi::xml_node;
[[nodiscard]] auto ensure_row_header_node(pugi::xml_node row)
    -> pugi::xml_node;
[[nodiscard]] auto current_table_column_count(pugi::xml_node table)
    -> std::size_t;
void ensure_table_grid_columns(pugi::xml_node table, std::size_t column_count);
[[nodiscard]] auto find_table_grid_column(pugi::xml_node table,
                                          std::size_t column_index)
    -> pugi::xml_node;
[[nodiscard]] auto ensure_cell_properties_node(pugi::xml_node cell)
    -> pugi::xml_node;
void ensure_default_cell_properties(pugi::xml_node cell);
[[nodiscard]] auto ensure_cell_width_node(pugi::xml_node cell)
    -> pugi::xml_node;
auto clear_cell_width_node(pugi::xml_node cell) -> bool;
[[nodiscard]] auto ensure_cell_vertical_merge_node(pugi::xml_node cell)
    -> pugi::xml_node;
[[nodiscard]] auto ensure_cell_borders_node(pugi::xml_node cell)
    -> pugi::xml_node;
[[nodiscard]] auto ensure_cell_vertical_alignment_node(pugi::xml_node cell)
    -> pugi::xml_node;
[[nodiscard]] auto ensure_cell_text_direction_node(pugi::xml_node cell)
    -> pugi::xml_node;
[[nodiscard]] auto ensure_cell_shading_node(pugi::xml_node cell)
    -> pugi::xml_node;
[[nodiscard]] auto ensure_cell_margins_node(pugi::xml_node cell)
    -> pugi::xml_node;
[[nodiscard]] auto ensure_cell_margin_node(pugi::xml_node cell,
                                           const char *margin_name)
    -> pugi::xml_node;
[[nodiscard]] auto ensure_cell_grid_span_node(pugi::xml_node cell)
    -> pugi::xml_node;
[[nodiscard]] auto parse_signed_attribute(pugi::xml_node node,
                                          const char *attribute_name)
    -> std::optional<std::int32_t>;
[[nodiscard]] auto parse_unsigned_attribute(pugi::xml_node node,
                                            const char *attribute_name)
    -> std::optional<std::uint32_t>;
[[nodiscard]] auto on_off_node_enabled(pugi::xml_node node) -> bool;
[[nodiscard]] auto cell_column_span(pugi::xml_node cell) -> std::size_t;
[[nodiscard]] auto cell_vertical_merge_state_for(pugi::xml_node cell)
    -> cell_vertical_merge_state;
[[nodiscard]] auto row_contains_vertical_merge_cells(pugi::xml_node row) -> bool;
[[nodiscard]] auto insert_empty_clone_row(pugi::xml_node table,
                                          pugi::xml_node source_row,
                                          pugi::xml_node merge_guard_row,
                                          bool insert_after) -> pugi::xml_node;
[[nodiscard]] auto insert_empty_clone_table(pugi::xml_node parent,
                                            pugi::xml_node source_table,
                                            bool insert_after)
    -> pugi::xml_node;
void remove_empty_container(pugi::xml_node properties, const char *container_name);
[[nodiscard]] auto append_cell_node(pugi::xml_node row) -> pugi::xml_node;
[[nodiscard]] auto append_row_node(pugi::xml_node table, std::size_t cell_count)
    -> pugi::xml_node;

[[nodiscard]] auto to_xml_border_style(featherdoc::border_style style)
    -> const char *;
[[nodiscard]] auto parse_border_style(std::string_view style)
    -> featherdoc::border_style;
[[nodiscard]] auto to_xml_table_layout_mode(featherdoc::table_layout_mode layout_mode)
    -> const char *;
[[nodiscard]] auto to_xml_table_alignment(featherdoc::table_alignment alignment)
    -> const char *;
[[nodiscard]] auto to_xml_table_position_horizontal_reference(
    featherdoc::table_position_horizontal_reference reference) -> const char *;
[[nodiscard]] auto to_xml_table_position_horizontal_spec(
    featherdoc::table_position_horizontal_spec spec) -> const char *;
[[nodiscard]] auto to_xml_table_position_vertical_reference(
    featherdoc::table_position_vertical_reference reference) -> const char *;
[[nodiscard]] auto to_xml_table_position_vertical_spec(
    featherdoc::table_position_vertical_spec spec) -> const char *;
[[nodiscard]] auto to_xml_table_overlap(featherdoc::table_overlap overlap)
    -> const char *;
[[nodiscard]] auto to_xml_row_height_rule(featherdoc::row_height_rule height_rule)
    -> const char *;
[[nodiscard]] auto parse_table_layout_mode(std::string_view layout_type)
    -> std::optional<featherdoc::table_layout_mode>;
[[nodiscard]] auto parse_table_position_horizontal_reference(std::string_view reference)
    -> std::optional<featherdoc::table_position_horizontal_reference>;
[[nodiscard]] auto parse_table_position_horizontal_spec(std::string_view spec)
    -> std::optional<featherdoc::table_position_horizontal_spec>;
[[nodiscard]] auto parse_table_position_vertical_reference(std::string_view reference)
    -> std::optional<featherdoc::table_position_vertical_reference>;
[[nodiscard]] auto parse_table_position_vertical_spec(std::string_view spec)
    -> std::optional<featherdoc::table_position_vertical_spec>;
[[nodiscard]] auto parse_table_overlap(std::string_view overlap)
    -> std::optional<featherdoc::table_overlap>;
[[nodiscard]] auto parse_table_alignment(std::string_view alignment)
    -> std::optional<featherdoc::table_alignment>;
[[nodiscard]] auto parse_row_height_rule(std::string_view height_rule)
    -> std::optional<featherdoc::row_height_rule>;
[[nodiscard]] auto to_xml_cell_vertical_alignment(
    featherdoc::cell_vertical_alignment alignment) -> const char *;
[[nodiscard]] auto parse_cell_vertical_alignment(std::string_view alignment)
    -> std::optional<featherdoc::cell_vertical_alignment>;
[[nodiscard]] auto to_xml_cell_text_direction(
    featherdoc::cell_text_direction direction) -> const char *;
[[nodiscard]] auto parse_cell_text_direction(std::string_view direction)
    -> std::optional<featherdoc::cell_text_direction>;
[[nodiscard]] auto to_xml_border_name(featherdoc::cell_border_edge edge)
    -> const char *;
[[nodiscard]] auto to_xml_border_name(featherdoc::table_border_edge edge)
    -> const char *;
[[nodiscard]] auto to_xml_margin_name(featherdoc::cell_margin_edge edge)
    -> const char *;
void apply_border_definition(pugi::xml_node border_node,
                             featherdoc::border_definition border);
[[nodiscard]] auto read_border_inspection_summary(pugi::xml_node border_node)
    -> std::optional<featherdoc::border_inspection_summary>;

} // namespace featherdoc::detail
