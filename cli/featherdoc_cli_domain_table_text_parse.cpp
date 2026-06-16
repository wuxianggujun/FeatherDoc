#include "featherdoc_cli_domain_parse.hpp"

namespace featherdoc_cli {

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

} // namespace featherdoc_cli
