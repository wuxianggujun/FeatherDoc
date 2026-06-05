#include "featherdoc_cli_domain_parse.hpp"

#include "featherdoc_cli_json_parse.hpp"

#include <string>

namespace featherdoc_cli {

auto parse_page_orientation(std::string_view text,
                            featherdoc::page_orientation &orientation) -> bool {
    if (text == "portrait") {
        orientation = featherdoc::page_orientation::portrait;
        return true;
    }
    if (text == "landscape") {
        orientation = featherdoc::page_orientation::landscape;
        return true;
    }

    return false;
}

auto parse_list_kind(std::string_view text, featherdoc::list_kind &kind)
    -> bool {
    if (text == "bullet") {
        kind = featherdoc::list_kind::bullet;
        return true;
    }
    if (text == "decimal") {
        kind = featherdoc::list_kind::decimal;
        return true;
    }

    return false;
}

auto parse_reference_kind(std::string_view text,
                          featherdoc::section_reference_kind &reference_kind)
    -> bool {
    if (text == "default") {
        reference_kind = featherdoc::section_reference_kind::default_reference;
        return true;
    }
    if (text == "first") {
        reference_kind = featherdoc::section_reference_kind::first_page;
        return true;
    }
    if (text == "even") {
        reference_kind = featherdoc::section_reference_kind::even_page;
        return true;
    }

    return false;
}

auto parse_json_patch_reference_kind_value(
    std::string_view text, std::size_t &index,
    featherdoc::section_reference_kind &reference_kind,
    std::string &error_message) -> bool {
    skip_json_patch_whitespace(text, index);
    std::string token;
    if (!parse_json_patch_string(text, index, token, error_message)) {
        return false;
    }

    if (!parse_reference_kind(token, reference_kind)) {
        error_message =
            "JSON patch member 'kind' must be 'default', 'first', or 'even'";
        return false;
    }

    return true;
}

auto parse_floating_image_horizontal_reference(
    std::string_view text,
    featherdoc::floating_image_horizontal_reference &reference) -> bool {
    if (text == "page") {
        reference = featherdoc::floating_image_horizontal_reference::page;
        return true;
    }
    if (text == "margin") {
        reference = featherdoc::floating_image_horizontal_reference::margin;
        return true;
    }
    if (text == "column") {
        reference = featherdoc::floating_image_horizontal_reference::column;
        return true;
    }
    if (text == "character") {
        reference = featherdoc::floating_image_horizontal_reference::character;
        return true;
    }

    return false;
}

auto parse_floating_image_vertical_reference(
    std::string_view text,
    featherdoc::floating_image_vertical_reference &reference) -> bool {
    if (text == "page") {
        reference = featherdoc::floating_image_vertical_reference::page;
        return true;
    }
    if (text == "margin") {
        reference = featherdoc::floating_image_vertical_reference::margin;
        return true;
    }
    if (text == "paragraph") {
        reference = featherdoc::floating_image_vertical_reference::paragraph;
        return true;
    }
    if (text == "line") {
        reference = featherdoc::floating_image_vertical_reference::line;
        return true;
    }

    return false;
}

auto parse_floating_image_wrap_mode(
    std::string_view text, featherdoc::floating_image_wrap_mode &mode) -> bool {
    if (text == "none") {
        mode = featherdoc::floating_image_wrap_mode::none;
        return true;
    }
    if (text == "square") {
        mode = featherdoc::floating_image_wrap_mode::square;
        return true;
    }
    if (text == "top_bottom" || text == "top-bottom") {
        mode = featherdoc::floating_image_wrap_mode::top_bottom;
        return true;
    }

    return false;
}

auto parse_table_style_margin_edge_text(
    std::string_view text, featherdoc::cell_margin_edge &edge) -> bool {
    if (text == "top") {
        edge = featherdoc::cell_margin_edge::top;
        return true;
    }
    if (text == "left") {
        edge = featherdoc::cell_margin_edge::left;
        return true;
    }
    if (text == "bottom") {
        edge = featherdoc::cell_margin_edge::bottom;
        return true;
    }
    if (text == "right") {
        edge = featherdoc::cell_margin_edge::right;
        return true;
    }

    return false;
}

auto parse_cell_margin_edge_text(std::string_view text,
                                 featherdoc::cell_margin_edge &edge) -> bool {
    return parse_table_style_margin_edge_text(text, edge);
}

auto parse_cell_border_edge_text(std::string_view text,
                                 featherdoc::cell_border_edge &edge) -> bool {
    if (text == "top") {
        edge = featherdoc::cell_border_edge::top;
        return true;
    }
    if (text == "left") {
        edge = featherdoc::cell_border_edge::left;
        return true;
    }
    if (text == "bottom") {
        edge = featherdoc::cell_border_edge::bottom;
        return true;
    }
    if (text == "right") {
        edge = featherdoc::cell_border_edge::right;
        return true;
    }

    return false;
}

auto parse_table_style_border_edge_text(
    std::string_view text, featherdoc::table_border_edge &edge) -> bool {
    if (text == "top") {
        edge = featherdoc::table_border_edge::top;
        return true;
    }
    if (text == "left") {
        edge = featherdoc::table_border_edge::left;
        return true;
    }
    if (text == "bottom") {
        edge = featherdoc::table_border_edge::bottom;
        return true;
    }
    if (text == "right") {
        edge = featherdoc::table_border_edge::right;
        return true;
    }
    if (text == "inside_horizontal" || text == "inside-horizontal") {
        edge = featherdoc::table_border_edge::inside_horizontal;
        return true;
    }
    if (text == "inside_vertical" || text == "inside-vertical") {
        edge = featherdoc::table_border_edge::inside_vertical;
        return true;
    }

    return false;
}

auto parse_table_border_edge_text(
    std::string_view text, featherdoc::table_border_edge &edge) -> bool {
    return parse_table_style_border_edge_text(text, edge);
}

auto parse_table_style_border_style_text(
    std::string_view text, featherdoc::border_style &style) -> bool {
    if (text == "none") {
        style = featherdoc::border_style::none;
        return true;
    }
    if (text == "single") {
        style = featherdoc::border_style::single;
        return true;
    }
    if (text == "double_line" || text == "double-line") {
        style = featherdoc::border_style::double_line;
        return true;
    }
    if (text == "dashed") {
        style = featherdoc::border_style::dashed;
        return true;
    }
    if (text == "dotted") {
        style = featherdoc::border_style::dotted;
        return true;
    }
    if (text == "thick") {
        style = featherdoc::border_style::thick;
        return true;
    }

    return false;
}

auto parse_border_style_text(std::string_view text,
                             featherdoc::border_style &style) -> bool {
    return parse_table_style_border_style_text(text, style);
}

auto parse_table_style_cell_vertical_alignment_text(
    std::string_view text,
    featherdoc::cell_vertical_alignment &alignment) -> bool {
    if (text == "top") {
        alignment = featherdoc::cell_vertical_alignment::top;
        return true;
    }
    if (text == "center") {
        alignment = featherdoc::cell_vertical_alignment::center;
        return true;
    }
    if (text == "bottom") {
        alignment = featherdoc::cell_vertical_alignment::bottom;
        return true;
    }
    if (text == "both") {
        alignment = featherdoc::cell_vertical_alignment::both;
        return true;
    }

    return false;
}

auto parse_cell_vertical_alignment_text(
    std::string_view text, featherdoc::cell_vertical_alignment &alignment)
    -> bool {
    return parse_table_style_cell_vertical_alignment_text(text, alignment);
}

auto parse_table_style_cell_text_direction_text(
    std::string_view text, featherdoc::cell_text_direction &direction) -> bool {
    if (text == "left_to_right_top_to_bottom") {
        direction = featherdoc::cell_text_direction::left_to_right_top_to_bottom;
        return true;
    }
    if (text == "top_to_bottom_right_to_left") {
        direction = featherdoc::cell_text_direction::top_to_bottom_right_to_left;
        return true;
    }
    if (text == "bottom_to_top_left_to_right") {
        direction = featherdoc::cell_text_direction::bottom_to_top_left_to_right;
        return true;
    }
    if (text == "left_to_right_top_to_bottom_rotated") {
        direction =
            featherdoc::cell_text_direction::left_to_right_top_to_bottom_rotated;
        return true;
    }
    if (text == "top_to_bottom_right_to_left_rotated") {
        direction =
            featherdoc::cell_text_direction::top_to_bottom_right_to_left_rotated;
        return true;
    }
    if (text == "top_to_bottom_left_to_right_rotated") {
        direction =
            featherdoc::cell_text_direction::top_to_bottom_left_to_right_rotated;
        return true;
    }

    return false;
}

auto parse_cell_text_direction_text(
    std::string_view text, featherdoc::cell_text_direction &direction) -> bool {
    return parse_table_style_cell_text_direction_text(text, direction);
}

auto parse_table_style_paragraph_alignment_text(
    std::string_view text, featherdoc::paragraph_alignment &alignment) -> bool {
    if (text == "left") {
        alignment = featherdoc::paragraph_alignment::left;
        return true;
    }
    if (text == "center") {
        alignment = featherdoc::paragraph_alignment::center;
        return true;
    }
    if (text == "right") {
        alignment = featherdoc::paragraph_alignment::right;
        return true;
    }
    if (text == "justified" || text == "both") {
        alignment = featherdoc::paragraph_alignment::justified;
        return true;
    }
    if (text == "distribute") {
        alignment = featherdoc::paragraph_alignment::distribute;
        return true;
    }

    return false;
}

auto parse_table_style_paragraph_line_spacing_rule_text(
    std::string_view text, featherdoc::paragraph_line_spacing_rule &rule)
    -> bool {
    if (text == "auto" || text == "automatic") {
        rule = featherdoc::paragraph_line_spacing_rule::automatic;
        return true;
    }
    if (text == "at_least" || text == "at-least") {
        rule = featherdoc::paragraph_line_spacing_rule::at_least;
        return true;
    }
    if (text == "exact") {
        rule = featherdoc::paragraph_line_spacing_rule::exact;
        return true;
    }

    return false;
}

auto parse_row_height_rule_text(std::string_view text,
                                featherdoc::row_height_rule &height_rule)
    -> bool {
    if (text == "automatic") {
        height_rule = featherdoc::row_height_rule::automatic;
        return true;
    }
    if (text == "at_least") {
        height_rule = featherdoc::row_height_rule::at_least;
        return true;
    }
    if (text == "exact") {
        height_rule = featherdoc::row_height_rule::exact;
        return true;
    }

    return false;
}

auto parse_table_layout_mode_text(std::string_view text,
                                  featherdoc::table_layout_mode &layout_mode)
    -> bool {
    if (text == "autofit") {
        layout_mode = featherdoc::table_layout_mode::autofit;
        return true;
    }
    if (text == "fixed") {
        layout_mode = featherdoc::table_layout_mode::fixed;
        return true;
    }

    return false;
}

auto parse_table_alignment_text(std::string_view text,
                                featherdoc::table_alignment &alignment) -> bool {
    if (text == "left") {
        alignment = featherdoc::table_alignment::left;
        return true;
    }
    if (text == "center") {
        alignment = featherdoc::table_alignment::center;
        return true;
    }
    if (text == "right") {
        alignment = featherdoc::table_alignment::right;
        return true;
    }

    return false;
}

auto parse_table_position_horizontal_reference(
    std::string_view text,
    featherdoc::table_position_horizontal_reference &reference) -> bool {
    if (text == "margin") {
        reference = featherdoc::table_position_horizontal_reference::margin;
        return true;
    }
    if (text == "page") {
        reference = featherdoc::table_position_horizontal_reference::page;
        return true;
    }
    if (text == "column") {
        reference = featherdoc::table_position_horizontal_reference::column;
        return true;
    }

    return false;
}

auto parse_table_position_vertical_reference(
    std::string_view text,
    featherdoc::table_position_vertical_reference &reference) -> bool {
    if (text == "margin") {
        reference = featherdoc::table_position_vertical_reference::margin;
        return true;
    }
    if (text == "page") {
        reference = featherdoc::table_position_vertical_reference::page;
        return true;
    }
    if (text == "paragraph") {
        reference = featherdoc::table_position_vertical_reference::paragraph;
        return true;
    }

    return false;
}

auto parse_table_position_horizontal_spec(
    std::string_view text,
    featherdoc::table_position_horizontal_spec &spec) -> bool {
    if (text == "left") {
        spec = featherdoc::table_position_horizontal_spec::left;
        return true;
    }
    if (text == "center") {
        spec = featherdoc::table_position_horizontal_spec::center;
        return true;
    }
    if (text == "right") {
        spec = featherdoc::table_position_horizontal_spec::right;
        return true;
    }
    if (text == "inside") {
        spec = featherdoc::table_position_horizontal_spec::inside;
        return true;
    }
    if (text == "outside") {
        spec = featherdoc::table_position_horizontal_spec::outside;
        return true;
    }

    return false;
}

auto parse_table_position_vertical_spec(
    std::string_view text, featherdoc::table_position_vertical_spec &spec)
    -> bool {
    if (text == "top") {
        spec = featherdoc::table_position_vertical_spec::top;
        return true;
    }
    if (text == "center") {
        spec = featherdoc::table_position_vertical_spec::center;
        return true;
    }
    if (text == "bottom") {
        spec = featherdoc::table_position_vertical_spec::bottom;
        return true;
    }
    if (text == "inside") {
        spec = featherdoc::table_position_vertical_spec::inside;
        return true;
    }
    if (text == "outside") {
        spec = featherdoc::table_position_vertical_spec::outside;
        return true;
    }

    return false;
}

auto parse_table_overlap_text(std::string_view text,
                              featherdoc::table_overlap &overlap) -> bool {
    if (text == "allow" || text == "overlap") {
        overlap = featherdoc::table_overlap::allow;
        return true;
    }
    if (text == "never") {
        overlap = featherdoc::table_overlap::never;
        return true;
    }

    return false;
}

} // namespace featherdoc_cli
