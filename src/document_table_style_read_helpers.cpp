#include "document_table_style_helpers.hpp"

#include <charconv>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>

namespace {

auto read_font_family_attribute(pugi::xml_node run_fonts,
                                const char *attribute_name)
    -> std::optional<std::string> {
    if (run_fonts == pugi::xml_node{}) {
        return std::nullopt;
    }

    const auto value = std::string_view{run_fonts.attribute(attribute_name).value()};
    if (value.empty()) {
        return std::nullopt;
    }
    return std::string{value};
}


auto read_primary_font_family(pugi::xml_node run_properties)
    -> std::optional<std::string> {
    const auto run_fonts = run_properties.child("w:rFonts");
    if (auto ascii = read_font_family_attribute(run_fonts, "w:ascii")) {
        return ascii;
    }
    if (auto high_ansi = read_font_family_attribute(run_fonts, "w:hAnsi")) {
        return high_ansi;
    }
    return read_font_family_attribute(run_fonts, "w:cs");
}


auto read_east_asia_font_family(pugi::xml_node run_properties)
    -> std::optional<std::string> {
    return read_font_family_attribute(run_properties.child("w:rFonts"),
                                      "w:eastAsia");
}


auto read_on_off_value(pugi::xml_node node) -> std::optional<bool> {
    if (node == pugi::xml_node{}) {
        return std::nullopt;
    }

    const auto value = std::string_view{node.attribute("w:val").value()};
    if (value.empty() || value == "true" || value == "1" || value == "on") {
        return true;
    }
    if (value == "false" || value == "0" || value == "off") {
        return false;
    }
    return std::nullopt;
}

auto parse_u32_attribute_value(const char *text)
    -> std::optional<std::uint32_t> {
    if (text == nullptr || text[0] == '\0') {
        return std::nullopt;
    }

    const auto *first = text;
    const auto *last = text + std::char_traits<char>::length(text);
    auto value = std::uint32_t{0U};
    const auto [ptr, ec] = std::from_chars(first, last, value);
    if (ec != std::errc{} || ptr != last) {
        return std::nullopt;
    }
    return value;
}

auto to_table_style_border_name(featherdoc::table_border_edge edge) -> const
    char * {
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

auto to_table_style_margin_name(featherdoc::cell_margin_edge edge) -> const
    char * {
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

auto read_table_style_cell_vertical_alignment(pugi::xml_node node)
    -> std::optional<featherdoc::cell_vertical_alignment> {
    const auto value = std::string_view{node.attribute("w:val").value()};
    if (value == "top") {
        return featherdoc::cell_vertical_alignment::top;
    }
    if (value == "center") {
        return featherdoc::cell_vertical_alignment::center;
    }
    if (value == "bottom") {
        return featherdoc::cell_vertical_alignment::bottom;
    }
    if (value == "both") {
        return featherdoc::cell_vertical_alignment::both;
    }

    return std::nullopt;
}

auto read_table_style_paragraph_alignment(pugi::xml_node node)
    -> std::optional<featherdoc::paragraph_alignment> {
    const auto value = std::string_view{node.attribute("w:val").value()};
    if (value == "left" || value == "start") {
        return featherdoc::paragraph_alignment::left;
    }
    if (value == "center") {
        return featherdoc::paragraph_alignment::center;
    }
    if (value == "right" || value == "end") {
        return featherdoc::paragraph_alignment::right;
    }
    if (value == "both" || value == "mediumKashida" || value == "highKashida" ||
        value == "lowKashida") {
        return featherdoc::paragraph_alignment::justified;
    }
    if (value == "distribute") {
        return featherdoc::paragraph_alignment::distribute;
    }

    return std::nullopt;
}

auto read_table_style_paragraph_line_spacing_rule(pugi::xml_node node)
    -> std::optional<featherdoc::paragraph_line_spacing_rule> {
    const auto value = std::string_view{node.attribute("w:lineRule").value()};
    if (value == "auto") {
        return featherdoc::paragraph_line_spacing_rule::automatic;
    }
    if (value == "atLeast") {
        return featherdoc::paragraph_line_spacing_rule::at_least;
    }
    if (value == "exact") {
        return featherdoc::paragraph_line_spacing_rule::exact;
    }

    return std::nullopt;
}

auto read_table_style_cell_text_direction(pugi::xml_node node)
    -> std::optional<featherdoc::cell_text_direction> {
    const auto value = std::string_view{node.attribute("w:val").value()};
    if (value == "lrTb") {
        return featherdoc::cell_text_direction::left_to_right_top_to_bottom;
    }
    if (value == "tbRl") {
        return featherdoc::cell_text_direction::top_to_bottom_right_to_left;
    }
    if (value == "btLr") {
        return featherdoc::cell_text_direction::bottom_to_top_left_to_right;
    }
    if (value == "lrTbV") {
        return featherdoc::cell_text_direction::
            left_to_right_top_to_bottom_rotated;
    }
    if (value == "tbRlV") {
        return featherdoc::cell_text_direction::
            top_to_bottom_right_to_left_rotated;
    }
    if (value == "tbLrV") {
        return featherdoc::cell_text_direction::
            top_to_bottom_left_to_right_rotated;
    }

    return std::nullopt;
}

auto read_non_empty_xml_attribute(pugi::xml_node node, const char *name)
    -> std::optional<std::string> {
    const auto attribute = node.attribute(name);
    if (attribute == pugi::xml_attribute{} || attribute.value()[0] == '\0') {
        return std::nullopt;
    }

    return std::string{attribute.value()};
}

auto read_table_style_margin(pugi::xml_node margins,
                             featherdoc::cell_margin_edge edge)
    -> std::optional<std::uint32_t> {
    return parse_u32_attribute_value(
        margins.child(to_table_style_margin_name(edge))
            .attribute("w:w")
            .value());
}

auto read_table_style_margins(pugi::xml_node properties,
                              const char *container_name)
    -> std::optional<featherdoc::table_style_margins_definition> {
    const auto margins = properties.child(container_name);
    if (margins == pugi::xml_node{}) {
        return std::nullopt;
    }

    auto summary = featherdoc::table_style_margins_definition{};
    summary.top_twips =
        read_table_style_margin(margins, featherdoc::cell_margin_edge::top);
    summary.left_twips =
        read_table_style_margin(margins, featherdoc::cell_margin_edge::left);
    summary.bottom_twips =
        read_table_style_margin(margins, featherdoc::cell_margin_edge::bottom);
    summary.right_twips =
        read_table_style_margin(margins, featherdoc::cell_margin_edge::right);
    return summary;
}

auto read_table_style_border(pugi::xml_node borders,
                             featherdoc::table_border_edge edge)
    -> std::optional<featherdoc::table_style_border_summary> {
    const auto border = borders.child(to_table_style_border_name(edge));
    if (border == pugi::xml_node{}) {
        return std::nullopt;
    }

    auto summary = featherdoc::table_style_border_summary{};
    summary.style = read_non_empty_xml_attribute(border, "w:val");
    summary.size_eighth_points =
        parse_u32_attribute_value(border.attribute("w:sz").value());
    summary.color = read_non_empty_xml_attribute(border, "w:color");
    summary.space_points =
        parse_u32_attribute_value(border.attribute("w:space").value());
    return summary;
}

auto read_table_style_borders(pugi::xml_node properties,
                              const char *container_name)
    -> std::optional<featherdoc::table_style_borders_summary> {
    const auto borders = properties.child(container_name);
    if (borders == pugi::xml_node{}) {
        return std::nullopt;
    }

    auto summary = featherdoc::table_style_borders_summary{};
    summary.top =
        read_table_style_border(borders, featherdoc::table_border_edge::top);
    summary.left =
        read_table_style_border(borders, featherdoc::table_border_edge::left);
    summary.bottom =
        read_table_style_border(borders, featherdoc::table_border_edge::bottom);
    summary.right =
        read_table_style_border(borders, featherdoc::table_border_edge::right);
    summary.inside_horizontal = read_table_style_border(
        borders, featherdoc::table_border_edge::inside_horizontal);
    summary.inside_vertical = read_table_style_border(
        borders, featherdoc::table_border_edge::inside_vertical);
    return summary;
}

auto read_table_style_paragraph_spacing(pugi::xml_node paragraph_properties)
    -> std::optional<featherdoc::table_style_paragraph_spacing_definition> {
    const auto spacing = paragraph_properties.child("w:spacing");
    if (spacing == pugi::xml_node{}) {
        return std::nullopt;
    }

    auto summary = featherdoc::table_style_paragraph_spacing_definition{};
    summary.before_twips =
        parse_u32_attribute_value(spacing.attribute("w:before").value());
    summary.after_twips =
        parse_u32_attribute_value(spacing.attribute("w:after").value());
    summary.line_twips =
        parse_u32_attribute_value(spacing.attribute("w:line").value());
    summary.line_rule = read_table_style_paragraph_line_spacing_rule(spacing);
    return summary;
}

auto table_style_border_property_count(
    const featherdoc::table_style_border_summary &border) -> std::size_t {
    return static_cast<std::size_t>(border.style.has_value()) +
           static_cast<std::size_t>(border.size_eighth_points.has_value()) +
           static_cast<std::size_t>(border.color.has_value()) +
           static_cast<std::size_t>(border.space_points.has_value());
}

auto table_style_borders_property_count(
    const featherdoc::table_style_borders_summary &borders) -> std::size_t {
    auto count = std::size_t{0U};
    if (borders.top.has_value()) {
        count += table_style_border_property_count(*borders.top);
    }
    if (borders.left.has_value()) {
        count += table_style_border_property_count(*borders.left);
    }
    if (borders.bottom.has_value()) {
        count += table_style_border_property_count(*borders.bottom);
    }
    if (borders.right.has_value()) {
        count += table_style_border_property_count(*borders.right);
    }
    if (borders.inside_horizontal.has_value()) {
        count += table_style_border_property_count(*borders.inside_horizontal);
    }
    if (borders.inside_vertical.has_value()) {
        count += table_style_border_property_count(*borders.inside_vertical);
    }
    return count;
}

auto table_style_margins_property_count(
    const featherdoc::table_style_margins_definition &margins) -> std::size_t {
    return static_cast<std::size_t>(margins.top_twips.has_value()) +
           static_cast<std::size_t>(margins.left_twips.has_value()) +
           static_cast<std::size_t>(margins.bottom_twips.has_value()) +
           static_cast<std::size_t>(margins.right_twips.has_value());
}

auto table_style_paragraph_spacing_property_count(
    const featherdoc::table_style_paragraph_spacing_definition &spacing)
    -> std::size_t {
    return static_cast<std::size_t>(spacing.before_twips.has_value()) +
           static_cast<std::size_t>(spacing.after_twips.has_value()) +
           static_cast<std::size_t>(spacing.line_twips.has_value()) +
           static_cast<std::size_t>(spacing.line_rule.has_value());
}

} // namespace

namespace featherdoc::detail {

auto read_table_style_region(pugi::xml_node parent, bool whole_table)
    -> std::optional<featherdoc::table_style_region_summary> {
    auto summary = featherdoc::table_style_region_summary{};

    const auto cell_properties = parent.child("w:tcPr");
    if (cell_properties != pugi::xml_node{}) {
        summary.fill_color = read_non_empty_xml_attribute(
            cell_properties.child("w:shd"), "w:fill");
        summary.cell_vertical_alignment =
            read_table_style_cell_vertical_alignment(
                cell_properties.child("w:vAlign"));
        summary.cell_text_direction = read_table_style_cell_text_direction(
            cell_properties.child("w:textDirection"));
    }

    const auto paragraph_properties = parent.child("w:pPr");
    if (paragraph_properties != pugi::xml_node{}) {
        summary.paragraph_alignment = read_table_style_paragraph_alignment(
            paragraph_properties.child("w:jc"));
        summary.paragraph_spacing =
            read_table_style_paragraph_spacing(paragraph_properties);
    }

    const auto run_properties = parent.child("w:rPr");
    if (run_properties != pugi::xml_node{}) {
        summary.text_color = read_non_empty_xml_attribute(
            run_properties.child("w:color"), "w:val");
        summary.bold = read_on_off_value(run_properties.child("w:b"));
        summary.italic = read_on_off_value(run_properties.child("w:i"));
        if (const auto half_points = parse_u32_attribute_value(
                run_properties.child("w:sz").attribute("w:val").value())) {
            summary.font_size_points = *half_points / 2U;
        }
        summary.font_family = read_primary_font_family(run_properties);
        summary.east_asia_font_family =
            read_east_asia_font_family(run_properties);
    }

    const auto properties =
        whole_table ? parent.child("w:tblPr") : cell_properties;
    if (properties != pugi::xml_node{}) {
        summary.cell_margins = read_table_style_margins(
            properties, whole_table ? "w:tblCellMar" : "w:tcMar");
        summary.borders = read_table_style_borders(
            properties, whole_table ? "w:tblBorders" : "w:tcBorders");
    }

    if (!summary.fill_color.has_value() && !summary.text_color.has_value() &&
        !summary.bold.has_value() && !summary.italic.has_value() &&
        !summary.font_size_points.has_value() &&
        !summary.font_family.has_value() &&
        !summary.east_asia_font_family.has_value() &&
        !summary.cell_vertical_alignment.has_value() &&
        !summary.cell_text_direction.has_value() &&
        !summary.paragraph_alignment.has_value() &&
        !summary.paragraph_spacing.has_value() &&
        !summary.cell_margins.has_value() && !summary.borders.has_value()) {
        return std::nullopt;
    }

    return summary;
}

auto find_table_style_conditional_node(
    pugi::xml_node style, std::string_view type_name) -> pugi::xml_node {
    for (auto conditional = style.child("w:tblStylePr");
         conditional != pugi::xml_node{};
         conditional = conditional.next_sibling("w:tblStylePr")) {
        if (std::string_view{conditional.attribute("w:type").value()} ==
            type_name) {
            return conditional;
        }
    }

    return {};
}

auto read_table_style_conditional_region(pugi::xml_node style,
                                         std::string_view type_name)
    -> std::optional<featherdoc::table_style_region_summary> {
    const auto conditional =
        find_table_style_conditional_node(style, type_name);
    if (conditional == pugi::xml_node{}) {
        return std::nullopt;
    }

    return read_table_style_region(conditional, false);
}

auto table_style_region_property_count(
    const featherdoc::table_style_region_summary &region) -> std::size_t {
    auto count = std::size_t{0U};
    count += static_cast<std::size_t>(region.fill_color.has_value());
    count += static_cast<std::size_t>(region.text_color.has_value());
    count += static_cast<std::size_t>(region.bold.has_value());
    count += static_cast<std::size_t>(region.italic.has_value());
    count += static_cast<std::size_t>(region.font_size_points.has_value());
    count += static_cast<std::size_t>(region.font_family.has_value());
    count += static_cast<std::size_t>(region.east_asia_font_family.has_value());
    count +=
        static_cast<std::size_t>(region.cell_vertical_alignment.has_value());
    count += static_cast<std::size_t>(region.cell_text_direction.has_value());
    count += static_cast<std::size_t>(region.paragraph_alignment.has_value());
    if (region.paragraph_spacing.has_value()) {
        count += table_style_paragraph_spacing_property_count(
            *region.paragraph_spacing);
    }
    if (region.cell_margins.has_value()) {
        count += table_style_margins_property_count(*region.cell_margins);
    }
    if (region.borders.has_value()) {
        count += table_style_borders_property_count(*region.borders);
    }
    return count;
}

auto table_style_whole_region_declared(pugi::xml_node style) -> bool {
    return style.child("w:tblPr") != pugi::xml_node{} ||
           style.child("w:tcPr") != pugi::xml_node{} ||
           style.child("w:pPr") != pugi::xml_node{} ||
           style.child("w:rPr") != pugi::xml_node{};
}

} // namespace featherdoc::detail
