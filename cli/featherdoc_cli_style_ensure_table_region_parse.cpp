#include "featherdoc_cli_style_ensure_table_region_parse.hpp"

#include "featherdoc_cli_domain_parse.hpp"
#include "featherdoc_cli_parse.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace featherdoc_cli {
namespace {

auto split_colon_fields(std::string_view text) -> std::vector<std::string_view> {
    auto fields = std::vector<std::string_view>{};
    std::size_t start = 0U;
    while (start <= text.size()) {
        const auto separator = text.find(':', start);
        if (separator == std::string_view::npos) {
            fields.push_back(text.substr(start));
            break;
        }

        fields.push_back(text.substr(start, separator - start));
        start = separator + 1U;
    }
    return fields;
}

auto table_style_region_option(
    featherdoc::table_style_definition &definition, std::string_view region_name)
    -> std::optional<featherdoc::table_style_region_definition> * {
    if (region_name == "whole_table" || region_name == "whole-table") {
        return &definition.whole_table;
    }
    if (region_name == "first_row" || region_name == "first-row") {
        return &definition.first_row;
    }
    if (region_name == "last_row" || region_name == "last-row") {
        return &definition.last_row;
    }
    if (region_name == "first_column" || region_name == "first-column") {
        return &definition.first_column;
    }
    if (region_name == "last_column" || region_name == "last-column") {
        return &definition.last_column;
    }
    if (region_name == "banded_rows" || region_name == "banded-rows") {
        return &definition.banded_rows;
    }
    if (region_name == "banded_columns" || region_name == "banded-columns") {
        return &definition.banded_columns;
    }
    if (region_name == "second_banded_rows" || region_name == "second-banded-rows" ||
        region_name == "banded_rows_2" || region_name == "banded-rows-2") {
        return &definition.second_banded_rows;
    }
    if (region_name == "second_banded_columns" ||
        region_name == "second-banded-columns" || region_name == "banded_columns_2" ||
        region_name == "banded-columns-2") {
        return &definition.second_banded_columns;
    }

    return nullptr;
}

auto ensure_table_style_region_option(
    featherdoc::table_style_definition &definition, std::string_view region_name,
    std::string_view option_name, std::string &error_message)
    -> featherdoc::table_style_region_definition * {
    auto *region = table_style_region_option(definition, region_name);
    if (region == nullptr) {
        error_message = "invalid " + std::string{option_name} +
                        " region: " + std::string{region_name};
        return nullptr;
    }
    if (!region->has_value()) {
        region->emplace();
    }
    return &(**region);
}

auto assign_table_style_margin(featherdoc::table_style_margins_definition &margins,
                               featherdoc::cell_margin_edge edge,
                               std::uint32_t value) -> void {
    switch (edge) {
    case featherdoc::cell_margin_edge::top:
        margins.top_twips = value;
        return;
    case featherdoc::cell_margin_edge::left:
        margins.left_twips = value;
        return;
    case featherdoc::cell_margin_edge::bottom:
        margins.bottom_twips = value;
        return;
    case featherdoc::cell_margin_edge::right:
        margins.right_twips = value;
        return;
    }
}

auto assign_table_style_border(featherdoc::table_style_borders_definition &borders,
                               featherdoc::table_border_edge edge,
                               featherdoc::border_definition border) -> void {
    switch (edge) {
    case featherdoc::table_border_edge::top:
        borders.top = border;
        return;
    case featherdoc::table_border_edge::left:
        borders.left = border;
        return;
    case featherdoc::table_border_edge::bottom:
        borders.bottom = border;
        return;
    case featherdoc::table_border_edge::right:
        borders.right = border;
        return;
    case featherdoc::table_border_edge::inside_horizontal:
        borders.inside_horizontal = border;
        return;
    case featherdoc::table_border_edge::inside_vertical:
        borders.inside_vertical = border;
        return;
    }
}

auto ensure_table_style_paragraph_spacing_option(
    featherdoc::table_style_region_definition &region)
    -> featherdoc::table_style_paragraph_spacing_definition & {
    return region.paragraph_spacing.has_value() ? *region.paragraph_spacing
                                                : region.paragraph_spacing.emplace();
}

} // namespace

auto parse_table_style_fill_option(std::string_view text,
                                   featherdoc::table_style_definition &definition,
                                   std::string &error_message) -> bool {
    const auto fields = split_colon_fields(text);
    if (fields.size() != 2U || fields[0].empty() || fields[1].empty()) {
        error_message = "invalid --style-fill value: expected <region>:<color>";
        return false;
    }

    auto *region = ensure_table_style_region_option(definition, fields[0],
                                                    "--style-fill", error_message);
    if (region == nullptr) {
        return false;
    }
    region->fill_color = std::string{fields[1]};
    return true;
}

auto parse_table_style_text_color_option(
    std::string_view text, featherdoc::table_style_definition &definition,
    std::string &error_message) -> bool {
    const auto fields = split_colon_fields(text);
    if (fields.size() != 2U || fields[0].empty() || fields[1].empty()) {
        error_message = "invalid --style-text-color value: expected <region>:<color>";
        return false;
    }

    auto *region = ensure_table_style_region_option(
        definition, fields[0], "--style-text-color", error_message);
    if (region == nullptr) {
        return false;
    }
    region->text_color = std::string{fields[1]};
    return true;
}

auto parse_table_style_bold_option(std::string_view text,
                                   featherdoc::table_style_definition &definition,
                                   std::string &error_message) -> bool {
    const auto fields = split_colon_fields(text);
    if (fields.size() != 2U || fields[0].empty() || fields[1].empty()) {
        error_message = "invalid --style-bold value: expected <region>:true|false";
        return false;
    }

    auto *region = ensure_table_style_region_option(definition, fields[0],
                                                    "--style-bold", error_message);
    if (region == nullptr) {
        return false;
    }

    auto value = false;
    if (!parse_bool(fields[1], value)) {
        error_message = "invalid --style-bold value: " + std::string{fields[1]};
        return false;
    }

    region->bold = value;
    return true;
}

auto parse_table_style_italic_option(std::string_view text,
                                     featherdoc::table_style_definition &definition,
                                     std::string &error_message) -> bool {
    const auto fields = split_colon_fields(text);
    if (fields.size() != 2U || fields[0].empty() || fields[1].empty()) {
        error_message = "invalid --style-italic value: expected <region>:true|false";
        return false;
    }

    auto *region = ensure_table_style_region_option(definition, fields[0],
                                                    "--style-italic", error_message);
    if (region == nullptr) {
        return false;
    }

    auto value = false;
    if (!parse_bool(fields[1], value)) {
        error_message = "invalid --style-italic value: " + std::string{fields[1]};
        return false;
    }

    region->italic = value;
    return true;
}

auto parse_table_style_font_size_option(
    std::string_view text, featherdoc::table_style_definition &definition,
    std::string &error_message) -> bool {
    const auto fields = split_colon_fields(text);
    if (fields.size() != 2U || fields[0].empty() || fields[1].empty()) {
        error_message = "invalid --style-font-size value: expected <region>:<points>";
        return false;
    }

    auto *region = ensure_table_style_region_option(definition, fields[0],
                                                    "--style-font-size", error_message);
    if (region == nullptr) {
        return false;
    }

    auto value = std::uint32_t{};
    if (!parse_uint32(fields[1], value) || value == 0U) {
        error_message = "invalid --style-font-size points: " + std::string{fields[1]};
        return false;
    }

    region->font_size_points = value;
    return true;
}

auto parse_table_style_font_family_option(
    std::string_view text, featherdoc::table_style_definition &definition,
    bool east_asia, std::string &error_message) -> bool {
    const auto fields = split_colon_fields(text);
    const auto option_name = east_asia ? "--style-east-asia-font-family"
                                      : "--style-font-family";
    if (fields.size() != 2U || fields[0].empty() || fields[1].empty()) {
        error_message = std::string{"invalid "} + option_name +
                        " value: expected <region>:<name>";
        return false;
    }

    auto *region = ensure_table_style_region_option(definition, fields[0],
                                                    option_name, error_message);
    if (region == nullptr) {
        return false;
    }

    if (east_asia) {
        region->east_asia_font_family = std::string{fields[1]};
    } else {
        region->font_family = std::string{fields[1]};
    }
    return true;
}

auto parse_table_style_cell_vertical_alignment_option(
    std::string_view text, featherdoc::table_style_definition &definition,
    std::string &error_message) -> bool {
    const auto fields = split_colon_fields(text);
    if (fields.size() != 2U || fields[0].empty() || fields[1].empty()) {
        error_message = "invalid --style-cell-vertical-alignment value: expected "
                        "<region>:<top|center|bottom|both>";
        return false;
    }

    auto *region = ensure_table_style_region_option(
        definition, fields[0], "--style-cell-vertical-alignment", error_message);
    if (region == nullptr) {
        return false;
    }

    auto alignment = featherdoc::cell_vertical_alignment::top;
    if (!parse_table_style_cell_vertical_alignment_text(fields[1], alignment)) {
        error_message =
            "invalid --style-cell-vertical-alignment value: " + std::string{fields[1]};
        return false;
    }

    region->cell_vertical_alignment = alignment;
    return true;
}

auto parse_table_style_cell_text_direction_option(
    std::string_view text, featherdoc::table_style_definition &definition,
    std::string &error_message) -> bool {
    const auto fields = split_colon_fields(text);
    if (fields.size() != 2U || fields[0].empty() || fields[1].empty()) {
        error_message = "invalid --style-cell-text-direction value: expected "
                        "<region>:<direction>";
        return false;
    }

    auto *region = ensure_table_style_region_option(
        definition, fields[0], "--style-cell-text-direction", error_message);
    if (region == nullptr) {
        return false;
    }

    auto direction = featherdoc::cell_text_direction::left_to_right_top_to_bottom;
    if (!parse_table_style_cell_text_direction_text(fields[1], direction)) {
        error_message =
            "invalid --style-cell-text-direction value: " + std::string{fields[1]};
        return false;
    }

    region->cell_text_direction = direction;
    return true;
}

auto parse_table_style_paragraph_alignment_option(
    std::string_view text, featherdoc::table_style_definition &definition,
    std::string &error_message) -> bool {
    const auto fields = split_colon_fields(text);
    if (fields.size() != 2U || fields[0].empty() || fields[1].empty()) {
        error_message = "invalid --style-paragraph-alignment value: expected "
                        "<region>:<left|center|right|justified|distribute>";
        return false;
    }

    auto *region = ensure_table_style_region_option(
        definition, fields[0], "--style-paragraph-alignment", error_message);
    if (region == nullptr) {
        return false;
    }

    auto alignment = featherdoc::paragraph_alignment::left;
    if (!parse_table_style_paragraph_alignment_text(fields[1], alignment)) {
        error_message =
            "invalid --style-paragraph-alignment value: " + std::string{fields[1]};
        return false;
    }

    region->paragraph_alignment = alignment;
    return true;
}

auto parse_table_style_paragraph_spacing_before_option(
    std::string_view text, featherdoc::table_style_definition &definition,
    std::string &error_message) -> bool {
    const auto fields = split_colon_fields(text);
    if (fields.size() != 2U || fields[0].empty() || fields[1].empty()) {
        error_message = "invalid --style-paragraph-spacing-before value: expected "
                        "<region>:<twips>";
        return false;
    }

    auto *region = ensure_table_style_region_option(
        definition, fields[0], "--style-paragraph-spacing-before", error_message);
    if (region == nullptr) {
        return false;
    }

    auto value = std::uint32_t{};
    if (!parse_uint32(fields[1], value)) {
        error_message = "invalid --style-paragraph-spacing-before twips: " +
                        std::string{fields[1]};
        return false;
    }

    ensure_table_style_paragraph_spacing_option(*region).before_twips = value;
    return true;
}

auto parse_table_style_paragraph_spacing_after_option(
    std::string_view text, featherdoc::table_style_definition &definition,
    std::string &error_message) -> bool {
    const auto fields = split_colon_fields(text);
    if (fields.size() != 2U || fields[0].empty() || fields[1].empty()) {
        error_message = "invalid --style-paragraph-spacing-after value: expected "
                        "<region>:<twips>";
        return false;
    }

    auto *region = ensure_table_style_region_option(
        definition, fields[0], "--style-paragraph-spacing-after", error_message);
    if (region == nullptr) {
        return false;
    }

    auto value = std::uint32_t{};
    if (!parse_uint32(fields[1], value)) {
        error_message = "invalid --style-paragraph-spacing-after twips: " +
                        std::string{fields[1]};
        return false;
    }

    ensure_table_style_paragraph_spacing_option(*region).after_twips = value;
    return true;
}

auto parse_table_style_paragraph_line_spacing_option(
    std::string_view text, featherdoc::table_style_definition &definition,
    std::string &error_message) -> bool {
    const auto fields = split_colon_fields(text);
    if (fields.size() != 3U || fields[0].empty() || fields[1].empty() ||
        fields[2].empty()) {
        error_message = "invalid --style-paragraph-line-spacing value: expected "
                        "<region>:<twips>:<auto|at_least|exact>";
        return false;
    }

    auto *region = ensure_table_style_region_option(
        definition, fields[0], "--style-paragraph-line-spacing", error_message);
    if (region == nullptr) {
        return false;
    }

    auto line_twips = std::uint32_t{};
    if (!parse_uint32(fields[1], line_twips)) {
        error_message = "invalid --style-paragraph-line-spacing twips: " +
                        std::string{fields[1]};
        return false;
    }

    auto line_rule = featherdoc::paragraph_line_spacing_rule::automatic;
    if (!parse_table_style_paragraph_line_spacing_rule_text(fields[2], line_rule)) {
        error_message = "invalid --style-paragraph-line-spacing rule: " +
                        std::string{fields[2]};
        return false;
    }

    auto &spacing = ensure_table_style_paragraph_spacing_option(*region);
    spacing.line_twips = line_twips;
    spacing.line_rule = line_rule;
    return true;
}

auto parse_table_style_margin_option(std::string_view text,
                                     featherdoc::table_style_definition &definition,
                                     std::string &error_message) -> bool {
    const auto fields = split_colon_fields(text);
    if (fields.size() != 3U || fields[0].empty() || fields[1].empty() ||
        fields[2].empty()) {
        error_message =
            "invalid --style-margin value: expected <region>:<edge>:<twips>";
        return false;
    }

    auto *region = ensure_table_style_region_option(definition, fields[0],
                                                    "--style-margin", error_message);
    if (region == nullptr) {
        return false;
    }

    auto edge = featherdoc::cell_margin_edge::top;
    if (!parse_table_style_margin_edge_text(fields[1], edge)) {
        error_message = "invalid --style-margin edge: " + std::string{fields[1]};
        return false;
    }

    auto value = std::uint32_t{};
    if (!parse_uint32(fields[2], value)) {
        error_message = "invalid --style-margin twips: " + std::string{fields[2]};
        return false;
    }

    auto &margins = region->cell_margins.has_value() ? *region->cell_margins
                                                     : region->cell_margins.emplace();
    assign_table_style_margin(margins, edge, value);
    return true;
}

auto parse_table_style_border_option(std::string_view text,
                                     featherdoc::table_style_definition &definition,
                                     std::string &error_message) -> bool {
    const auto fields = split_colon_fields(text);
    if ((fields.size() != 5U && fields.size() != 6U) || fields[0].empty() ||
        fields[1].empty() || fields[2].empty() || fields[3].empty() ||
        fields[4].empty()) {
        error_message = "invalid --style-border value: expected "
                        "<region>:<edge>:<style>:<size>:<color>[:space]";
        return false;
    }

    auto *region = ensure_table_style_region_option(definition, fields[0],
                                                    "--style-border", error_message);
    if (region == nullptr) {
        return false;
    }

    auto edge = featherdoc::table_border_edge::top;
    if (!parse_table_style_border_edge_text(fields[1], edge)) {
        error_message = "invalid --style-border edge: " + std::string{fields[1]};
        return false;
    }

    auto style = featherdoc::border_style::single;
    if (!parse_table_style_border_style_text(fields[2], style)) {
        error_message = "invalid --style-border style: " + std::string{fields[2]};
        return false;
    }

    auto size = std::uint32_t{};
    if (!parse_uint32(fields[3], size)) {
        error_message = "invalid --style-border size: " + std::string{fields[3]};
        return false;
    }

    auto space = std::uint32_t{0U};
    if (fields.size() == 6U && !parse_uint32(fields[5], space)) {
        error_message = "invalid --style-border space: " + std::string{fields[5]};
        return false;
    }

    auto &borders = region->borders.has_value() ? *region->borders
                                                : region->borders.emplace();
    assign_table_style_border(
        borders, edge,
        featherdoc::border_definition{style, size, fields[4], space});
    return true;
}

} // namespace featherdoc_cli
