#include "featherdoc_cli_domain_parse.hpp"

namespace featherdoc_cli {

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
