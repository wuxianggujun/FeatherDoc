#include "featherdoc_cli_domain_names.hpp"

namespace featherdoc_cli {

auto style_kind_name(featherdoc::style_kind kind) -> const char * {
    switch (kind) {
    case featherdoc::style_kind::paragraph:
        return "paragraph";
    case featherdoc::style_kind::character:
        return "character";
    case featherdoc::style_kind::table:
        return "table";
    case featherdoc::style_kind::numbering:
        return "numbering";
    case featherdoc::style_kind::unknown:
        return "unknown";
    }

    return "unknown";
}

auto list_kind_name(featherdoc::list_kind kind) -> const char * {
    switch (kind) {
    case featherdoc::list_kind::bullet:
        return "bullet";
    case featherdoc::list_kind::decimal:
        return "decimal";
    }

    return "bullet";
}

auto bookmark_kind_name(featherdoc::bookmark_kind kind) -> const char * {
    switch (kind) {
    case featherdoc::bookmark_kind::text:
        return "text";
    case featherdoc::bookmark_kind::block:
        return "block";
    case featherdoc::bookmark_kind::table_rows:
        return "table_rows";
    case featherdoc::bookmark_kind::block_range:
        return "block_range";
    case featherdoc::bookmark_kind::malformed:
        return "malformed";
    case featherdoc::bookmark_kind::mixed:
        return "mixed";
    }

    return "mixed";
}

auto content_control_kind_name(featherdoc::content_control_kind kind)
    -> const char * {
    switch (kind) {
    case featherdoc::content_control_kind::block:
        return "block";
    case featherdoc::content_control_kind::run:
        return "run";
    case featherdoc::content_control_kind::table_row:
        return "table_row";
    case featherdoc::content_control_kind::table_cell:
        return "table_cell";
    case featherdoc::content_control_kind::unknown:
        return "unknown";
    }

    return "unknown";
}

auto content_control_form_kind_name(
    featherdoc::content_control_form_kind kind) -> const char * {
    switch (kind) {
    case featherdoc::content_control_form_kind::rich_text:
        return "rich_text";
    case featherdoc::content_control_form_kind::plain_text:
        return "plain_text";
    case featherdoc::content_control_form_kind::picture:
        return "picture";
    case featherdoc::content_control_form_kind::checkbox:
        return "checkbox";
    case featherdoc::content_control_form_kind::drop_down_list:
        return "drop_down_list";
    case featherdoc::content_control_form_kind::combo_box:
        return "combo_box";
    case featherdoc::content_control_form_kind::date:
        return "date";
    case featherdoc::content_control_form_kind::repeating_section:
        return "repeating_section";
    case featherdoc::content_control_form_kind::group:
        return "group";
    case featherdoc::content_control_form_kind::unknown:
        return "unknown";
    }

    return "unknown";
}

auto review_note_kind_name(featherdoc::review_note_kind kind) -> const char * {
    switch (kind) {
    case featherdoc::review_note_kind::footnote:
        return "footnote";
    case featherdoc::review_note_kind::endnote:
        return "endnote";
    case featherdoc::review_note_kind::comment:
        return "comment";
    }

    return "footnote";
}

auto revision_kind_name(featherdoc::revision_kind kind) -> const char * {
    switch (kind) {
    case featherdoc::revision_kind::insertion:
        return "insertion";
    case featherdoc::revision_kind::deletion:
        return "deletion";
    case featherdoc::revision_kind::move_from:
        return "move_from";
    case featherdoc::revision_kind::move_to:
        return "move_to";
    case featherdoc::revision_kind::paragraph_property_change:
        return "paragraph_property_change";
    case featherdoc::revision_kind::run_property_change:
        return "run_property_change";
    case featherdoc::revision_kind::unknown:
        return "unknown";
    }

    return "unknown";
}

auto template_slot_kind_name(featherdoc::template_slot_kind kind)
    -> const char * {
    switch (kind) {
    case featherdoc::template_slot_kind::text:
        return "text";
    case featherdoc::template_slot_kind::table_rows:
        return "table_rows";
    case featherdoc::template_slot_kind::table:
        return "table";
    case featherdoc::template_slot_kind::image:
        return "image";
    case featherdoc::template_slot_kind::floating_image:
        return "floating_image";
    case featherdoc::template_slot_kind::block:
        return "block";
    }

    return "text";
}

auto template_slot_source_json_name(
    featherdoc::template_slot_source_kind source) -> const char * {
    switch (source) {
    case featherdoc::template_slot_source_kind::bookmark:
        return "bookmark";
    case featherdoc::template_slot_source_kind::content_control_tag:
        return "content_control_tag";
    case featherdoc::template_slot_source_kind::content_control_alias:
        return "content_control_alias";
    }

    return "bookmark";
}

auto template_slot_source_text_name(
    featherdoc::template_slot_source_kind source) -> const char * {
    switch (source) {
    case featherdoc::template_slot_source_kind::bookmark:
        return "bookmark";
    case featherdoc::template_slot_source_kind::content_control_tag:
        return "content-control-tag";
    case featherdoc::template_slot_source_kind::content_control_alias:
        return "content-control-alias";
    }

    return "bookmark";
}

auto template_slot_source_new_json_name(
    featherdoc::template_slot_source_kind source) -> std::string {
    if (source == featherdoc::template_slot_source_kind::bookmark) {
        return "new_bookmark";
    }

    auto name = std::string{"new_"};
    name += template_slot_source_json_name(source);
    return name;
}

auto drawing_image_placement_name(
    featherdoc::drawing_image_placement placement) -> const char * {
    switch (placement) {
    case featherdoc::drawing_image_placement::inline_object:
        return "inline";
    case featherdoc::drawing_image_placement::anchored_object:
        return "anchored";
    }

    return "inline";
}

auto floating_image_horizontal_reference_name(
    featherdoc::floating_image_horizontal_reference reference) -> const char * {
    switch (reference) {
    case featherdoc::floating_image_horizontal_reference::page:
        return "page";
    case featherdoc::floating_image_horizontal_reference::margin:
        return "margin";
    case featherdoc::floating_image_horizontal_reference::column:
        return "column";
    case featherdoc::floating_image_horizontal_reference::character:
        return "character";
    }

    return "column";
}

auto floating_image_vertical_reference_name(
    featherdoc::floating_image_vertical_reference reference) -> const char * {
    switch (reference) {
    case featherdoc::floating_image_vertical_reference::page:
        return "page";
    case featherdoc::floating_image_vertical_reference::margin:
        return "margin";
    case featherdoc::floating_image_vertical_reference::paragraph:
        return "paragraph";
    case featherdoc::floating_image_vertical_reference::line:
        return "line";
    }

    return "paragraph";
}

auto floating_image_wrap_mode_name(featherdoc::floating_image_wrap_mode mode)
    -> const char * {
    switch (mode) {
    case featherdoc::floating_image_wrap_mode::none:
        return "none";
    case featherdoc::floating_image_wrap_mode::square:
        return "square";
    case featherdoc::floating_image_wrap_mode::top_bottom:
        return "top_bottom";
    }

    return "none";
}

auto page_orientation_name(featherdoc::page_orientation orientation)
    -> std::string_view {
    switch (orientation) {
    case featherdoc::page_orientation::portrait:
        return "portrait";
    case featherdoc::page_orientation::landscape:
        return "landscape";
    }

    return "unknown";
}

auto style_usage_part_kind_name(featherdoc::style_usage_part_kind part_kind)
    -> const char * {
    switch (part_kind) {
    case featherdoc::style_usage_part_kind::body:
        return "body";
    case featherdoc::style_usage_part_kind::header:
        return "header";
    case featherdoc::style_usage_part_kind::footer:
        return "footer";
    }

    return "unknown";
}

auto style_usage_hit_kind_name(featherdoc::style_usage_hit_kind hit_kind)
    -> const char * {
    switch (hit_kind) {
    case featherdoc::style_usage_hit_kind::paragraph:
        return "paragraph";
    case featherdoc::style_usage_hit_kind::run:
        return "run";
    case featherdoc::style_usage_hit_kind::table:
        return "table";
    }

    return "unknown";
}

auto table_layout_mode_name(featherdoc::table_layout_mode layout_mode) noexcept
    -> std::string_view {
    switch (layout_mode) {
    case featherdoc::table_layout_mode::autofit:
        return "autofit";
    case featherdoc::table_layout_mode::fixed:
        return "fixed";
    }

    return "autofit";
}

auto table_alignment_name(featherdoc::table_alignment alignment) noexcept
    -> std::string_view {
    switch (alignment) {
    case featherdoc::table_alignment::left:
        return "left";
    case featherdoc::table_alignment::center:
        return "center";
    case featherdoc::table_alignment::right:
        return "right";
    }

    return "left";
}

auto cell_margin_edge_name(featherdoc::cell_margin_edge edge) noexcept
    -> std::string_view {
    switch (edge) {
    case featherdoc::cell_margin_edge::top:
        return "top";
    case featherdoc::cell_margin_edge::left:
        return "left";
    case featherdoc::cell_margin_edge::bottom:
        return "bottom";
    case featherdoc::cell_margin_edge::right:
        return "right";
    }

    return "top";
}

auto cell_border_edge_name(featherdoc::cell_border_edge edge) noexcept
    -> std::string_view {
    switch (edge) {
    case featherdoc::cell_border_edge::top:
        return "top";
    case featherdoc::cell_border_edge::left:
        return "left";
    case featherdoc::cell_border_edge::bottom:
        return "bottom";
    case featherdoc::cell_border_edge::right:
        return "right";
    }

    return "top";
}

auto table_border_edge_name(featherdoc::table_border_edge edge) noexcept
    -> std::string_view {
    switch (edge) {
    case featherdoc::table_border_edge::top:
        return "top";
    case featherdoc::table_border_edge::left:
        return "left";
    case featherdoc::table_border_edge::bottom:
        return "bottom";
    case featherdoc::table_border_edge::right:
        return "right";
    case featherdoc::table_border_edge::inside_horizontal:
        return "inside_horizontal";
    case featherdoc::table_border_edge::inside_vertical:
        return "inside_vertical";
    }

    return "top";
}

auto border_style_name(featherdoc::border_style style) noexcept
    -> std::string_view {
    switch (style) {
    case featherdoc::border_style::none:
        return "none";
    case featherdoc::border_style::single:
        return "single";
    case featherdoc::border_style::double_line:
        return "double_line";
    case featherdoc::border_style::dashed:
        return "dashed";
    case featherdoc::border_style::dotted:
        return "dotted";
    case featherdoc::border_style::thick:
        return "thick";
    }

    return "single";
}

auto row_height_rule_name(featherdoc::row_height_rule height_rule) noexcept
    -> std::string_view {
    switch (height_rule) {
    case featherdoc::row_height_rule::automatic:
        return "automatic";
    case featherdoc::row_height_rule::at_least:
        return "at_least";
    case featherdoc::row_height_rule::exact:
        return "exact";
    }

    return "automatic";
}

auto cell_text_direction_name(featherdoc::cell_text_direction direction) noexcept
    -> std::string_view {
    switch (direction) {
    case featherdoc::cell_text_direction::left_to_right_top_to_bottom:
        return "left_to_right_top_to_bottom";
    case featherdoc::cell_text_direction::top_to_bottom_right_to_left:
        return "top_to_bottom_right_to_left";
    case featherdoc::cell_text_direction::bottom_to_top_left_to_right:
        return "bottom_to_top_left_to_right";
    case featherdoc::cell_text_direction::left_to_right_top_to_bottom_rotated:
        return "left_to_right_top_to_bottom_rotated";
    case featherdoc::cell_text_direction::top_to_bottom_right_to_left_rotated:
        return "top_to_bottom_right_to_left_rotated";
    case featherdoc::cell_text_direction::top_to_bottom_left_to_right_rotated:
        return "top_to_bottom_left_to_right_rotated";
    }

    return "left_to_right_top_to_bottom";
}

auto cell_vertical_alignment_name(
    featherdoc::cell_vertical_alignment alignment) noexcept -> std::string_view {
    switch (alignment) {
    case featherdoc::cell_vertical_alignment::top:
        return "top";
    case featherdoc::cell_vertical_alignment::center:
        return "center";
    case featherdoc::cell_vertical_alignment::bottom:
        return "bottom";
    case featherdoc::cell_vertical_alignment::both:
        return "both";
    }

    return "top";
}

auto table_style_cell_vertical_alignment_name(
    featherdoc::cell_vertical_alignment alignment) noexcept -> std::string_view {
    return cell_vertical_alignment_name(alignment);
}

auto table_style_cell_text_direction_name(
    featherdoc::cell_text_direction direction) noexcept -> std::string_view {
    return cell_text_direction_name(direction);
}

auto table_style_paragraph_alignment_name(
    featherdoc::paragraph_alignment alignment) noexcept -> std::string_view {
    switch (alignment) {
    case featherdoc::paragraph_alignment::left:
        return "left";
    case featherdoc::paragraph_alignment::center:
        return "center";
    case featherdoc::paragraph_alignment::right:
        return "right";
    case featherdoc::paragraph_alignment::justified:
        return "justified";
    case featherdoc::paragraph_alignment::distribute:
        return "distribute";
    }

    return "left";
}

auto table_style_paragraph_line_spacing_rule_name(
    featherdoc::paragraph_line_spacing_rule rule) noexcept -> std::string_view {
    switch (rule) {
    case featherdoc::paragraph_line_spacing_rule::automatic:
        return "auto";
    case featherdoc::paragraph_line_spacing_rule::at_least:
        return "at_least";
    case featherdoc::paragraph_line_spacing_rule::exact:
        return "exact";
    }

    return "auto";
}

auto table_position_horizontal_reference_name(
    featherdoc::table_position_horizontal_reference reference) noexcept
    -> std::string_view {
    switch (reference) {
    case featherdoc::table_position_horizontal_reference::margin:
        return "margin";
    case featherdoc::table_position_horizontal_reference::page:
        return "page";
    case featherdoc::table_position_horizontal_reference::column:
        return "column";
    }

    return "margin";
}

auto table_position_vertical_reference_name(
    featherdoc::table_position_vertical_reference reference) noexcept
    -> std::string_view {
    switch (reference) {
    case featherdoc::table_position_vertical_reference::margin:
        return "margin";
    case featherdoc::table_position_vertical_reference::page:
        return "page";
    case featherdoc::table_position_vertical_reference::paragraph:
        return "paragraph";
    }

    return "paragraph";
}

auto table_position_horizontal_spec_name(
    featherdoc::table_position_horizontal_spec spec) noexcept
    -> std::string_view {
    switch (spec) {
    case featherdoc::table_position_horizontal_spec::left:
        return "left";
    case featherdoc::table_position_horizontal_spec::center:
        return "center";
    case featherdoc::table_position_horizontal_spec::right:
        return "right";
    case featherdoc::table_position_horizontal_spec::inside:
        return "inside";
    case featherdoc::table_position_horizontal_spec::outside:
        return "outside";
    }

    return "left";
}

auto table_position_vertical_spec_name(
    featherdoc::table_position_vertical_spec spec) noexcept -> std::string_view {
    switch (spec) {
    case featherdoc::table_position_vertical_spec::top:
        return "top";
    case featherdoc::table_position_vertical_spec::center:
        return "center";
    case featherdoc::table_position_vertical_spec::bottom:
        return "bottom";
    case featherdoc::table_position_vertical_spec::inside:
        return "inside";
    case featherdoc::table_position_vertical_spec::outside:
        return "outside";
    }

    return "top";
}

auto table_overlap_name(featherdoc::table_overlap overlap) noexcept
    -> std::string_view {
    switch (overlap) {
    case featherdoc::table_overlap::allow:
        return "allow";
    case featherdoc::table_overlap::never:
        return "never";
    }

    return "allow";
}

} // namespace featherdoc_cli
