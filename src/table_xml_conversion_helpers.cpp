#include "table_xml_helpers.hpp"

#include <string>

namespace featherdoc::detail {

auto to_xml_border_style(featherdoc::border_style style) -> const char * {
    switch (style) {
    case featherdoc::border_style::none:
        return "nil";
    case featherdoc::border_style::single:
        return "single";
    case featherdoc::border_style::double_line:
        return "double";
    case featherdoc::border_style::dashed:
        return "dashed";
    case featherdoc::border_style::dotted:
        return "dotted";
    case featherdoc::border_style::thick:
        return "thick";
    }

    return "single";
}

auto parse_border_style(std::string_view style) -> featherdoc::border_style {
    if (style == "nil" || style == "none") {
        return featherdoc::border_style::none;
    }
    if (style == "double") {
        return featherdoc::border_style::double_line;
    }
    if (style == "dashed") {
        return featherdoc::border_style::dashed;
    }
    if (style == "dotted") {
        return featherdoc::border_style::dotted;
    }
    if (style == "thick") {
        return featherdoc::border_style::thick;
    }
    return featherdoc::border_style::single;
}

auto to_xml_table_layout_mode(featherdoc::table_layout_mode layout_mode) -> const char * {
    switch (layout_mode) {
    case featherdoc::table_layout_mode::autofit:
        return "autofit";
    case featherdoc::table_layout_mode::fixed:
        return "fixed";
    }

    return "autofit";
}

auto to_xml_table_alignment(featherdoc::table_alignment alignment) -> const char * {
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

auto to_xml_table_position_horizontal_reference(
    featherdoc::table_position_horizontal_reference reference) -> const char * {
    switch (reference) {
    case featherdoc::table_position_horizontal_reference::margin:
        return "margin";
    case featherdoc::table_position_horizontal_reference::page:
        return "page";
    case featherdoc::table_position_horizontal_reference::column:
        return "text";
    }

    return "margin";
}

auto to_xml_table_position_horizontal_spec(
    featherdoc::table_position_horizontal_spec spec) -> const char * {
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

auto to_xml_table_position_vertical_reference(
    featherdoc::table_position_vertical_reference reference) -> const char * {
    switch (reference) {
    case featherdoc::table_position_vertical_reference::margin:
        return "margin";
    case featherdoc::table_position_vertical_reference::page:
        return "page";
    case featherdoc::table_position_vertical_reference::paragraph:
        return "text";
    }

    return "text";
}

auto to_xml_table_position_vertical_spec(
    featherdoc::table_position_vertical_spec spec) -> const char * {
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

auto to_xml_table_overlap(featherdoc::table_overlap overlap) -> const char * {
    switch (overlap) {
    case featherdoc::table_overlap::allow:
        return "overlap";
    case featherdoc::table_overlap::never:
        return "never";
    }

    return "overlap";
}

auto to_xml_row_height_rule(featherdoc::row_height_rule height_rule) -> const char * {
    switch (height_rule) {
    case featherdoc::row_height_rule::automatic:
        return "auto";
    case featherdoc::row_height_rule::at_least:
        return "atLeast";
    case featherdoc::row_height_rule::exact:
        return "exact";
    }

    return "auto";
}

auto parse_table_layout_mode(std::string_view layout_type)
    -> std::optional<featherdoc::table_layout_mode> {
    if (layout_type == "autofit") {
        return featherdoc::table_layout_mode::autofit;
    }

    if (layout_type == "fixed") {
        return featherdoc::table_layout_mode::fixed;
    }

    return std::nullopt;
}

auto parse_table_position_horizontal_reference(std::string_view reference)
    -> std::optional<featherdoc::table_position_horizontal_reference> {
    if (reference == "margin") {
        return featherdoc::table_position_horizontal_reference::margin;
    }

    if (reference == "page") {
        return featherdoc::table_position_horizontal_reference::page;
    }

    if (reference == "text" || reference == "column") {
        return featherdoc::table_position_horizontal_reference::column;
    }

    return std::nullopt;
}

auto parse_table_position_horizontal_spec(std::string_view spec)
    -> std::optional<featherdoc::table_position_horizontal_spec> {
    if (spec == "left") {
        return featherdoc::table_position_horizontal_spec::left;
    }

    if (spec == "center") {
        return featherdoc::table_position_horizontal_spec::center;
    }

    if (spec == "right") {
        return featherdoc::table_position_horizontal_spec::right;
    }

    if (spec == "inside") {
        return featherdoc::table_position_horizontal_spec::inside;
    }

    if (spec == "outside") {
        return featherdoc::table_position_horizontal_spec::outside;
    }

    return std::nullopt;
}

auto parse_table_position_vertical_reference(std::string_view reference)
    -> std::optional<featherdoc::table_position_vertical_reference> {
    if (reference == "margin") {
        return featherdoc::table_position_vertical_reference::margin;
    }

    if (reference == "page") {
        return featherdoc::table_position_vertical_reference::page;
    }

    if (reference == "text" || reference == "paragraph") {
        return featherdoc::table_position_vertical_reference::paragraph;
    }

    return std::nullopt;
}

auto parse_table_position_vertical_spec(std::string_view spec)
    -> std::optional<featherdoc::table_position_vertical_spec> {
    if (spec == "top") {
        return featherdoc::table_position_vertical_spec::top;
    }

    if (spec == "center") {
        return featherdoc::table_position_vertical_spec::center;
    }

    if (spec == "bottom") {
        return featherdoc::table_position_vertical_spec::bottom;
    }

    if (spec == "inside") {
        return featherdoc::table_position_vertical_spec::inside;
    }

    if (spec == "outside") {
        return featherdoc::table_position_vertical_spec::outside;
    }

    return std::nullopt;
}

auto parse_table_overlap(std::string_view overlap)
    -> std::optional<featherdoc::table_overlap> {
    if (overlap == "overlap" || overlap == "allow") {
        return featherdoc::table_overlap::allow;
    }

    if (overlap == "never") {
        return featherdoc::table_overlap::never;
    }

    return std::nullopt;
}

auto parse_table_alignment(std::string_view alignment)
    -> std::optional<featherdoc::table_alignment> {
    if (alignment == "left" || alignment == "start") {
        return featherdoc::table_alignment::left;
    }

    if (alignment == "center") {
        return featherdoc::table_alignment::center;
    }

    if (alignment == "right" || alignment == "end") {
        return featherdoc::table_alignment::right;
    }

    return std::nullopt;
}

auto parse_row_height_rule(std::string_view height_rule)
    -> std::optional<featherdoc::row_height_rule> {
    if (height_rule == "auto") {
        return featherdoc::row_height_rule::automatic;
    }

    if (height_rule == "atLeast") {
        return featherdoc::row_height_rule::at_least;
    }

    if (height_rule == "exact") {
        return featherdoc::row_height_rule::exact;
    }

    return std::nullopt;
}

auto to_xml_cell_vertical_alignment(featherdoc::cell_vertical_alignment alignment)
    -> const char * {
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

auto parse_cell_vertical_alignment(std::string_view alignment)
    -> std::optional<featherdoc::cell_vertical_alignment> {
    if (alignment == "top") {
        return featherdoc::cell_vertical_alignment::top;
    }

    if (alignment == "center") {
        return featherdoc::cell_vertical_alignment::center;
    }

    if (alignment == "bottom") {
        return featherdoc::cell_vertical_alignment::bottom;
    }

    if (alignment == "both") {
        return featherdoc::cell_vertical_alignment::both;
    }

    return std::nullopt;
}

auto to_xml_cell_text_direction(featherdoc::cell_text_direction direction) -> const char * {
    switch (direction) {
    case featherdoc::cell_text_direction::left_to_right_top_to_bottom:
        return "lrTb";
    case featherdoc::cell_text_direction::top_to_bottom_right_to_left:
        return "tbRl";
    case featherdoc::cell_text_direction::bottom_to_top_left_to_right:
        return "btLr";
    case featherdoc::cell_text_direction::left_to_right_top_to_bottom_rotated:
        return "lrTbV";
    case featherdoc::cell_text_direction::top_to_bottom_right_to_left_rotated:
        return "tbRlV";
    case featherdoc::cell_text_direction::top_to_bottom_left_to_right_rotated:
        return "tbLrV";
    }

    return "lrTb";
}

auto parse_cell_text_direction(std::string_view direction)
    -> std::optional<featherdoc::cell_text_direction> {
    if (direction == "lrTb") {
        return featherdoc::cell_text_direction::left_to_right_top_to_bottom;
    }

    if (direction == "tbRl") {
        return featherdoc::cell_text_direction::top_to_bottom_right_to_left;
    }

    if (direction == "btLr") {
        return featherdoc::cell_text_direction::bottom_to_top_left_to_right;
    }

    if (direction == "lrTbV") {
        return featherdoc::cell_text_direction::left_to_right_top_to_bottom_rotated;
    }

    if (direction == "tbRlV") {
        return featherdoc::cell_text_direction::top_to_bottom_right_to_left_rotated;
    }

    if (direction == "tbLrV") {
        return featherdoc::cell_text_direction::top_to_bottom_left_to_right_rotated;
    }

    return std::nullopt;
}

auto to_xml_border_name(featherdoc::cell_border_edge edge) -> const char * {
    switch (edge) {
    case featherdoc::cell_border_edge::top:
        return "w:top";
    case featherdoc::cell_border_edge::left:
        return "w:left";
    case featherdoc::cell_border_edge::bottom:
        return "w:bottom";
    case featherdoc::cell_border_edge::right:
        return "w:right";
    }

    return "w:top";
}

auto to_xml_border_name(featherdoc::table_border_edge edge) -> const char * {
    switch (edge) {
    case featherdoc::table_border_edge::top:
        return "w:top";
    case featherdoc::table_border_edge::left:
        return "w:left";
    case featherdoc::table_border_edge::bottom:
        return "w:bottom";
    case featherdoc::table_border_edge::right:
        return "w:right";
    case featherdoc::table_border_edge::inside_horizontal:
        return "w:insideH";
    case featherdoc::table_border_edge::inside_vertical:
        return "w:insideV";
    }

    return "w:top";
}

auto to_xml_margin_name(featherdoc::cell_margin_edge edge) -> const char * {
    switch (edge) {
    case featherdoc::cell_margin_edge::top:
        return "w:top";
    case featherdoc::cell_margin_edge::left:
        return "w:left";
    case featherdoc::cell_margin_edge::bottom:
        return "w:bottom";
    case featherdoc::cell_margin_edge::right:
        return "w:right";
    }

    return "w:top";
}

void apply_border_definition(pugi::xml_node border_node,
                             featherdoc::border_definition border) {
    if (border_node == pugi::xml_node{}) {
        return;
    }

    const auto size_text = std::to_string(border.size_eighth_points);
    const auto space_text = std::to_string(border.space_points);
    const auto color_text =
        border.color.empty() ? std::string{"auto"} : std::string{border.color};

    ensure_attribute_value(border_node, "w:val", to_xml_border_style(border.style));
    ensure_attribute_value(border_node, "w:sz", size_text.c_str());
    ensure_attribute_value(border_node, "w:space", space_text.c_str());
    ensure_attribute_value(border_node, "w:color", color_text.c_str());
}

auto read_border_inspection_summary(pugi::xml_node border_node)
    -> std::optional<featherdoc::border_inspection_summary> {
    if (border_node == pugi::xml_node{}) {
        return std::nullopt;
    }

    auto summary = featherdoc::border_inspection_summary{};
    summary.style = parse_border_style(
        std::string_view{border_node.attribute("w:val").value()});
    summary.size_eighth_points =
        parse_unsigned_attribute(border_node, "w:sz").value_or(4U);
    if (const auto color = std::string_view{
            border_node.attribute("w:color").value()};
        !color.empty()) {
        summary.color = std::string{color};
    }
    summary.space_points =
        parse_unsigned_attribute(border_node, "w:space").value_or(0U);
    return summary;
}

} // namespace featherdoc::detail
