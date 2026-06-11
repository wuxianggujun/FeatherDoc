#include "featherdoc_cli_domain_names.hpp"

namespace featherdoc_cli {

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
