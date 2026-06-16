#include "featherdoc_cli_domain_names.hpp"

namespace featherdoc_cli {

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

} // namespace featherdoc_cli
