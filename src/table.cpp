#include "featherdoc.hpp"
#include "table_xml_helpers.hpp"
#include "xml_helpers.hpp"

#include <algorithm>
#include <limits>

namespace featherdoc {

using detail::append_cell_node;
using detail::append_row_node;
using detail::apply_border_definition;
using detail::cell_column_span;
using detail::cell_vertical_merge_state;
using detail::cell_vertical_merge_state_for;
using detail::clear_cell_width_node;
using detail::count_named_children;
using detail::current_table_column_count;
using detail::decode_table_style_look_flag;
using detail::encode_table_style_look;
using detail::ensure_attribute_value;
using detail::ensure_cell_borders_node;
using detail::ensure_cell_grid_span_node;
using detail::ensure_cell_margin_node;
using detail::ensure_cell_margins_node;
using detail::ensure_cell_properties_node;
using detail::ensure_cell_shading_node;
using detail::ensure_cell_text_direction_node;
using detail::ensure_cell_vertical_alignment_node;
using detail::ensure_cell_vertical_merge_node;
using detail::ensure_cell_width_node;
using detail::ensure_default_attribute_value;
using detail::ensure_default_cell_properties;
using detail::ensure_default_table_properties;
using detail::ensure_row_cant_split_node;
using detail::ensure_row_header_node;
using detail::ensure_row_height_node;
using detail::ensure_row_properties_node;
using detail::ensure_table_alignment_node;
using detail::ensure_table_borders_node;
using detail::ensure_table_cell_margin_node;
using detail::ensure_table_cell_margins_node;
using detail::ensure_table_cell_spacing_node;
using detail::ensure_table_grid_columns;
using detail::ensure_table_grid_node;
using detail::ensure_table_indent_node;
using detail::ensure_table_layout_node;
using detail::ensure_table_look_node;
using detail::ensure_table_position_node;
using detail::ensure_table_properties_node;
using detail::ensure_table_style_node;
using detail::ensure_table_width_node;
using detail::find_table_grid_column;
using detail::format_short_hex;
using detail::insert_empty_clone_row;
using detail::insert_empty_clone_table;
using detail::on_off_node_enabled;
using detail::parse_border_style;
using detail::parse_cell_text_direction;
using detail::parse_cell_vertical_alignment;
using detail::parse_row_height_rule;
using detail::parse_short_hex_value;
using detail::parse_signed_attribute;
using detail::parse_table_alignment;
using detail::parse_table_layout_mode;
using detail::parse_table_overlap;
using detail::parse_table_position_horizontal_reference;
using detail::parse_table_position_horizontal_spec;
using detail::parse_table_position_vertical_reference;
using detail::parse_table_position_vertical_spec;
using detail::parse_unsigned_attribute;
using detail::parse_xml_on_off_value;
using detail::read_border_inspection_summary;
using detail::remove_empty_container;
using detail::string_matrix_from_initializer_list;
using detail::table_style_look_first_column_bit;
using detail::table_style_look_first_row_bit;
using detail::table_style_look_last_column_bit;
using detail::table_style_look_last_row_bit;
using detail::table_style_look_no_hband_bit;
using detail::table_style_look_no_vband_bit;
using detail::to_xml_border_name;
using detail::to_xml_border_style;
using detail::to_xml_cell_text_direction;
using detail::to_xml_cell_vertical_alignment;
using detail::to_xml_margin_name;
using detail::to_xml_row_height_rule;
using detail::to_xml_table_alignment;
using detail::to_xml_table_layout_mode;
using detail::to_xml_table_overlap;
using detail::to_xml_table_position_horizontal_reference;
using detail::to_xml_table_position_horizontal_spec;
using detail::to_xml_table_position_vertical_reference;
using detail::to_xml_table_position_vertical_spec;

namespace {

#include "table_column_edit_helpers.inc"

} // namespace
#include "table_cell_methods.inc"

#include "table_row_methods.inc"

#include "table_methods.inc"

} // namespace featherdoc
