#include "document_table_style_helpers.hpp"

#include <algorithm>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

namespace {
constexpr auto styles_xml_entry = std::string_view{"word/styles.xml"};

void ensure_attribute_value(pugi::xml_node node, const char *name,
                            std::string_view value) {
    if (node == pugi::xml_node{}) {
        return;
    }

    auto attribute = node.attribute(name);
    if (attribute == pugi::xml_attribute{}) {
        attribute = node.append_attribute(name);
    }
    attribute.set_value(value.data());
}

void ensure_attribute_value(pugi::xml_node node, const char *name,
                            const std::string &value) {
    ensure_attribute_value(node, name, std::string_view{value});
}

void ensure_attribute_value(pugi::xml_node node, const char *name,
                            const char *value) {
    ensure_attribute_value(node, name, std::string_view{value});
}

auto set_last_error(featherdoc::document_error_info &error_info,
                    std::error_code code, std::string detail,
                    std::string entry_name) -> std::error_code {
    error_info.code = code;
    error_info.detail = std::move(detail);
    error_info.entry_name = std::move(entry_name);
    error_info.xml_offset = std::nullopt;
    return code;
}

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

void set_on_off_value(pugi::xml_node node, bool enabled) {
    if (node == pugi::xml_node{}) {
        return;
    }

    auto attribute = node.attribute("w:val");
    if (attribute == pugi::xml_attribute{}) {
        attribute = node.append_attribute("w:val");
    }
    attribute.set_value(enabled ? "1" : "0");
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

auto ensure_run_fonts_node(pugi::xml_node run_properties) -> pugi::xml_node {
    if (run_properties == pugi::xml_node{}) {
        return {};
    }

    auto run_fonts = run_properties.child("w:rFonts");
    if (run_fonts != pugi::xml_node{}) {
        return run_fonts;
    }

    if (const auto first_child = run_properties.first_child();
        first_child != pugi::xml_node{}) {
        return run_properties.insert_child_before("w:rFonts", first_child);
    }
    return run_properties.append_child("w:rFonts");
}

} // namespace

namespace featherdoc::detail {
auto to_table_style_border_style(featherdoc::border_style style) -> const
    char * {
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

void apply_table_style_border_definition(pugi::xml_node border_node,
                                         featherdoc::border_definition border) {
    if (border_node == pugi::xml_node{}) {
        return;
    }

    const auto size_text = std::to_string(border.size_eighth_points);
    const auto space_text = std::to_string(border.space_points);
    const auto color_text =
        border.color.empty() ? std::string{"auto"} : std::string{border.color};

    ensure_attribute_value(border_node, "w:val",
                           to_table_style_border_style(border.style));
    ensure_attribute_value(border_node, "w:sz", size_text);
    ensure_attribute_value(border_node, "w:space", space_text);
    ensure_attribute_value(border_node, "w:color", color_text);
}

auto ensure_table_style_table_properties_node(pugi::xml_node parent)
    -> pugi::xml_node {
    if (parent == pugi::xml_node{}) {
        return {};
    }

    auto table_properties = parent.child("w:tblPr");
    if (table_properties != pugi::xml_node{}) {
        return table_properties;
    }

    if (const auto cell_properties = parent.child("w:tcPr");
        cell_properties != pugi::xml_node{}) {
        return parent.insert_child_before("w:tblPr", cell_properties);
    }
    if (const auto conditional = parent.child("w:tblStylePr");
        conditional != pugi::xml_node{}) {
        return parent.insert_child_before("w:tblPr", conditional);
    }

    return parent.append_child("w:tblPr");
}

auto ensure_table_style_cell_properties_node(pugi::xml_node parent)
    -> pugi::xml_node {
    if (parent == pugi::xml_node{}) {
        return {};
    }

    auto cell_properties = parent.child("w:tcPr");
    if (cell_properties != pugi::xml_node{}) {
        return cell_properties;
    }

    if (const auto table_properties = parent.child("w:tblPr");
        table_properties != pugi::xml_node{}) {
        return parent.insert_child_after("w:tcPr", table_properties);
    }
    if (const auto run_properties = parent.child("w:rPr");
        run_properties != pugi::xml_node{}) {
        return parent.insert_child_after("w:tcPr", run_properties);
    }
    if (const auto paragraph_properties = parent.child("w:pPr");
        paragraph_properties != pugi::xml_node{}) {
        return parent.insert_child_after("w:tcPr", paragraph_properties);
    }
    if (const auto conditional = parent.child("w:tblStylePr");
        conditional != pugi::xml_node{}) {
        return parent.insert_child_before("w:tcPr", conditional);
    }

    return parent.append_child("w:tcPr");
}

auto ensure_table_style_paragraph_properties_node(pugi::xml_node parent)
    -> pugi::xml_node {
    if (parent == pugi::xml_node{}) {
        return {};
    }

    auto paragraph_properties = parent.child("w:pPr");
    if (paragraph_properties != pugi::xml_node{}) {
        return paragraph_properties;
    }

    if (const auto run_properties = parent.child("w:rPr");
        run_properties != pugi::xml_node{}) {
        return parent.insert_child_before("w:pPr", run_properties);
    }
    if (const auto cell_properties = parent.child("w:tcPr");
        cell_properties != pugi::xml_node{}) {
        return parent.insert_child_after("w:pPr", cell_properties);
    }
    if (const auto table_properties = parent.child("w:tblPr");
        table_properties != pugi::xml_node{}) {
        return parent.insert_child_after("w:pPr", table_properties);
    }
    if (const auto conditional = parent.child("w:tblStylePr");
        conditional != pugi::xml_node{}) {
        return parent.insert_child_before("w:pPr", conditional);
    }

    return parent.append_child("w:pPr");
}

auto ensure_table_style_run_properties_node(pugi::xml_node parent)
    -> pugi::xml_node {
    if (parent == pugi::xml_node{}) {
        return {};
    }

    auto run_properties = parent.child("w:rPr");
    if (run_properties != pugi::xml_node{}) {
        return run_properties;
    }

    if (const auto paragraph_properties = parent.child("w:pPr");
        paragraph_properties != pugi::xml_node{}) {
        return parent.insert_child_after("w:rPr", paragraph_properties);
    }
    if (const auto cell_properties = parent.child("w:tcPr");
        cell_properties != pugi::xml_node{}) {
        return parent.insert_child_after("w:rPr", cell_properties);
    }
    if (const auto table_properties = parent.child("w:tblPr");
        table_properties != pugi::xml_node{}) {
        return parent.insert_child_after("w:rPr", table_properties);
    }
    if (const auto conditional = parent.child("w:tblStylePr");
        conditional != pugi::xml_node{}) {
        return parent.insert_child_before("w:rPr", conditional);
    }

    return parent.append_child("w:rPr");
}

auto ensure_table_style_conditional_node(
    pugi::xml_node style, std::string_view type_name) -> pugi::xml_node {
    if (style == pugi::xml_node{}) {
        return {};
    }

    auto last_conditional = pugi::xml_node{};
    for (auto conditional = style.child("w:tblStylePr");
         conditional != pugi::xml_node{};
         conditional = conditional.next_sibling("w:tblStylePr")) {
        if (std::string_view{conditional.attribute("w:type").value()} ==
            type_name) {
            return conditional;
        }
        last_conditional = conditional;
    }

    auto conditional =
        last_conditional != pugi::xml_node{}
            ? style.insert_child_after("w:tblStylePr", last_conditional)
            : style.append_child("w:tblStylePr");
    if (conditional != pugi::xml_node{}) {
        ensure_attribute_value(conditional, "w:type", type_name);
    }
    return conditional;
}

auto ensure_table_style_shading_node(pugi::xml_node cell_properties)
    -> pugi::xml_node {
    if (cell_properties == pugi::xml_node{}) {
        return {};
    }

    auto shading = cell_properties.child("w:shd");
    if (shading != pugi::xml_node{}) {
        return shading;
    }

    if (const auto margins = cell_properties.child("w:tcMar");
        margins != pugi::xml_node{}) {
        return cell_properties.insert_child_before("w:shd", margins);
    }
    if (const auto vertical_alignment = cell_properties.child("w:vAlign");
        vertical_alignment != pugi::xml_node{}) {
        return cell_properties.insert_child_before("w:shd", vertical_alignment);
    }
    if (const auto borders = cell_properties.child("w:tcBorders");
        borders != pugi::xml_node{}) {
        return cell_properties.insert_child_before("w:shd", borders);
    }
    if (const auto first_child = cell_properties.first_child();
        first_child != pugi::xml_node{}) {
        return cell_properties.insert_child_before("w:shd", first_child);
    }

    return cell_properties.append_child("w:shd");
}

auto ensure_table_style_vertical_alignment_node(pugi::xml_node cell_properties)
    -> pugi::xml_node {
    if (cell_properties == pugi::xml_node{}) {
        return {};
    }

    auto vertical_alignment = cell_properties.child("w:vAlign");
    if (vertical_alignment != pugi::xml_node{}) {
        return vertical_alignment;
    }

    if (const auto margins = cell_properties.child("w:tcMar");
        margins != pugi::xml_node{}) {
        return cell_properties.insert_child_before("w:vAlign", margins);
    }
    if (const auto shading = cell_properties.child("w:shd");
        shading != pugi::xml_node{}) {
        return cell_properties.insert_child_after("w:vAlign", shading);
    }
    if (const auto borders = cell_properties.child("w:tcBorders");
        borders != pugi::xml_node{}) {
        return cell_properties.insert_child_after("w:vAlign", borders);
    }

    return cell_properties.append_child("w:vAlign");
}

auto ensure_table_style_paragraph_alignment_node(
    pugi::xml_node paragraph_properties) -> pugi::xml_node {
    if (paragraph_properties == pugi::xml_node{}) {
        return {};
    }

    auto alignment = paragraph_properties.child("w:jc");
    if (alignment != pugi::xml_node{}) {
        return alignment;
    }

    return paragraph_properties.append_child("w:jc");
}

auto ensure_table_style_paragraph_spacing_node(
    pugi::xml_node paragraph_properties) -> pugi::xml_node {
    if (paragraph_properties == pugi::xml_node{}) {
        return {};
    }

    auto spacing = paragraph_properties.child("w:spacing");
    if (spacing != pugi::xml_node{}) {
        return spacing;
    }

    if (const auto alignment = paragraph_properties.child("w:jc");
        alignment != pugi::xml_node{}) {
        return paragraph_properties.insert_child_before("w:spacing", alignment);
    }

    return paragraph_properties.append_child("w:spacing");
}

auto ensure_table_style_text_direction_node(pugi::xml_node cell_properties)
    -> pugi::xml_node {
    if (cell_properties == pugi::xml_node{}) {
        return {};
    }

    auto text_direction = cell_properties.child("w:textDirection");
    if (text_direction != pugi::xml_node{}) {
        return text_direction;
    }

    if (const auto vertical_alignment = cell_properties.child("w:vAlign");
        vertical_alignment != pugi::xml_node{}) {
        return cell_properties.insert_child_before("w:textDirection",
                                                   vertical_alignment);
    }
    if (const auto margins = cell_properties.child("w:tcMar");
        margins != pugi::xml_node{}) {
        return cell_properties.insert_child_after("w:textDirection", margins);
    }
    if (const auto shading = cell_properties.child("w:shd");
        shading != pugi::xml_node{}) {
        return cell_properties.insert_child_after("w:textDirection", shading);
    }
    if (const auto borders = cell_properties.child("w:tcBorders");
        borders != pugi::xml_node{}) {
        return cell_properties.insert_child_after("w:textDirection", borders);
    }
    if (const auto first_child = cell_properties.first_child();
        first_child != pugi::xml_node{}) {
        return cell_properties.insert_child_before("w:textDirection",
                                                   first_child);
    }

    return cell_properties.append_child("w:textDirection");
}

auto ensure_table_style_margins_node(
    pugi::xml_node properties, const char *container_name) -> pugi::xml_node {
    if (properties == pugi::xml_node{}) {
        return {};
    }

    auto margins = properties.child(container_name);
    if (margins != pugi::xml_node{}) {
        return margins;
    }

    return properties.append_child(container_name);
}

auto ensure_table_style_borders_node(
    pugi::xml_node properties, const char *container_name) -> pugi::xml_node {
    if (properties == pugi::xml_node{}) {
        return {};
    }

    auto borders = properties.child(container_name);
    if (borders != pugi::xml_node{}) {
        return borders;
    }

    return properties.append_child(container_name);
}

auto to_table_style_cell_vertical_alignment_value(
    featherdoc::cell_vertical_alignment alignment) -> const char * {
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

auto to_table_style_paragraph_alignment_value(
    featherdoc::paragraph_alignment alignment) -> const char * {
    switch (alignment) {
    case featherdoc::paragraph_alignment::left:
        return "left";
    case featherdoc::paragraph_alignment::center:
        return "center";
    case featherdoc::paragraph_alignment::right:
        return "right";
    case featherdoc::paragraph_alignment::justified:
        return "both";
    case featherdoc::paragraph_alignment::distribute:
        return "distribute";
    }

    return "left";
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

auto to_table_style_paragraph_line_spacing_rule_value(
    featherdoc::paragraph_line_spacing_rule rule) -> const char * {
    switch (rule) {
    case featherdoc::paragraph_line_spacing_rule::automatic:
        return "auto";
    case featherdoc::paragraph_line_spacing_rule::at_least:
        return "atLeast";
    case featherdoc::paragraph_line_spacing_rule::exact:
        return "exact";
    }

    return "auto";
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

auto to_table_style_cell_text_direction_value(
    featherdoc::cell_text_direction direction) -> const char * {
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

auto ensure_table_style_text_color_node(pugi::xml_node run_properties)
    -> pugi::xml_node {
    if (run_properties == pugi::xml_node{}) {
        return {};
    }

    auto color = run_properties.child("w:color");
    if (color != pugi::xml_node{}) {
        return color;
    }

    return run_properties.append_child("w:color");
}

auto ensure_table_style_bold_node(pugi::xml_node run_properties)
    -> pugi::xml_node {
    if (run_properties == pugi::xml_node{}) {
        return {};
    }

    auto bold = run_properties.child("w:b");
    if (bold != pugi::xml_node{}) {
        return bold;
    }

    if (const auto italic = run_properties.child("w:i");
        italic != pugi::xml_node{}) {
        return run_properties.insert_child_before("w:b", italic);
    }
    if (const auto color = run_properties.child("w:color");
        color != pugi::xml_node{}) {
        return run_properties.insert_child_before("w:b", color);
    }

    return run_properties.append_child("w:b");
}

auto ensure_table_style_italic_node(pugi::xml_node run_properties)
    -> pugi::xml_node {
    if (run_properties == pugi::xml_node{}) {
        return {};
    }

    auto italic = run_properties.child("w:i");
    if (italic != pugi::xml_node{}) {
        return italic;
    }

    if (const auto bold = run_properties.child("w:b");
        bold != pugi::xml_node{}) {
        return run_properties.insert_child_after("w:i", bold);
    }
    if (const auto color = run_properties.child("w:color");
        color != pugi::xml_node{}) {
        return run_properties.insert_child_before("w:i", color);
    }
    if (const auto size = run_properties.child("w:sz");
        size != pugi::xml_node{}) {
        return run_properties.insert_child_before("w:i", size);
    }

    return run_properties.append_child("w:i");
}

auto ensure_table_style_font_size_node(
    pugi::xml_node run_properties, const char *child_name) -> pugi::xml_node {
    if (run_properties == pugi::xml_node{}) {
        return {};
    }

    auto font_size = run_properties.child(child_name);
    if (font_size != pugi::xml_node{}) {
        return font_size;
    }

    const auto name = std::string_view{child_name};
    if (name == "w:szCs") {
        if (const auto primary_size = run_properties.child("w:sz");
            primary_size != pugi::xml_node{}) {
            return run_properties.insert_child_after(child_name, primary_size);
        }
    } else if (const auto complex_size = run_properties.child("w:szCs");
               complex_size != pugi::xml_node{}) {
        return run_properties.insert_child_before(child_name, complex_size);
    }

    if (const auto color = run_properties.child("w:color");
        color != pugi::xml_node{}) {
        return run_properties.insert_child_after(child_name, color);
    }
    if (const auto italic = run_properties.child("w:i");
        italic != pugi::xml_node{}) {
        return run_properties.insert_child_after(child_name, italic);
    }
    if (const auto bold = run_properties.child("w:b");
        bold != pugi::xml_node{}) {
        return run_properties.insert_child_after(child_name, bold);
    }

    return run_properties.append_child(child_name);
}

auto apply_table_style_margin(
    pugi::xml_node margins, featherdoc::cell_margin_edge edge,
    const std::optional<std::uint32_t> &value) -> bool {
    if (!value.has_value()) {
        return true;
    }

    auto margin = margins.child(to_table_style_margin_name(edge));
    if (margin == pugi::xml_node{}) {
        margin = margins.append_child(to_table_style_margin_name(edge));
    }
    if (margin == pugi::xml_node{}) {
        return false;
    }

    ensure_attribute_value(margin, "w:w", std::to_string(*value));
    ensure_attribute_value(margin, "w:type", "dxa");
    return true;
}

auto apply_table_style_margins(
    pugi::xml_node properties, const char *container_name,
    const featherdoc::table_style_margins_definition &margins) -> bool {
    const auto margins_node =
        ensure_table_style_margins_node(properties, container_name);
    if (margins_node == pugi::xml_node{}) {
        return false;
    }

    return apply_table_style_margin(margins_node,
                                    featherdoc::cell_margin_edge::top,
                                    margins.top_twips) &&
           apply_table_style_margin(margins_node,
                                    featherdoc::cell_margin_edge::left,
                                    margins.left_twips) &&
           apply_table_style_margin(margins_node,
                                    featherdoc::cell_margin_edge::bottom,
                                    margins.bottom_twips) &&
           apply_table_style_margin(margins_node,
                                    featherdoc::cell_margin_edge::right,
                                    margins.right_twips);
}

auto apply_table_style_border(
    pugi::xml_node borders, featherdoc::table_border_edge edge,
    const std::optional<featherdoc::border_definition> &value) -> bool {
    if (!value.has_value()) {
        return true;
    }

    auto border = borders.child(to_table_style_border_name(edge));
    if (border == pugi::xml_node{}) {
        border = borders.append_child(to_table_style_border_name(edge));
    }
    if (border == pugi::xml_node{}) {
        return false;
    }

    apply_table_style_border_definition(border, *value);
    return true;
}

auto apply_table_style_borders(
    pugi::xml_node properties, const char *container_name,
    const featherdoc::table_style_borders_definition &borders) -> bool {
    const auto borders_node =
        ensure_table_style_borders_node(properties, container_name);
    if (borders_node == pugi::xml_node{}) {
        return false;
    }

    return apply_table_style_border(
               borders_node, featherdoc::table_border_edge::top, borders.top) &&
           apply_table_style_border(borders_node,
                                    featherdoc::table_border_edge::left,
                                    borders.left) &&
           apply_table_style_border(borders_node,
                                    featherdoc::table_border_edge::bottom,
                                    borders.bottom) &&
           apply_table_style_border(borders_node,
                                    featherdoc::table_border_edge::right,
                                    borders.right) &&
           apply_table_style_border(
               borders_node, featherdoc::table_border_edge::inside_horizontal,
               borders.inside_horizontal) &&
           apply_table_style_border(
               borders_node, featherdoc::table_border_edge::inside_vertical,
               borders.inside_vertical);
}

auto apply_table_style_paragraph_spacing(
    pugi::xml_node paragraph_properties,
    const featherdoc::table_style_paragraph_spacing_definition &spacing)
    -> bool {
    const auto spacing_node =
        ensure_table_style_paragraph_spacing_node(paragraph_properties);
    if (spacing_node == pugi::xml_node{}) {
        return false;
    }

    if (spacing.before_twips.has_value()) {
        ensure_attribute_value(spacing_node, "w:before",
                               std::to_string(*spacing.before_twips));
    }
    if (spacing.after_twips.has_value()) {
        ensure_attribute_value(spacing_node, "w:after",
                               std::to_string(*spacing.after_twips));
    }
    if (spacing.line_twips.has_value()) {
        ensure_attribute_value(spacing_node, "w:line",
                               std::to_string(*spacing.line_twips));
    }
    if (spacing.line_rule.has_value()) {
        ensure_attribute_value(spacing_node, "w:lineRule",
                               to_table_style_paragraph_line_spacing_rule_value(
                                   *spacing.line_rule));
    }

    return true;
}

auto apply_table_style_region(
    pugi::xml_node parent,
    const featherdoc::table_style_region_definition &region,
    bool whole_table) -> bool {
    if (region.fill_color.has_value()) {
        const auto cell_properties =
            ensure_table_style_cell_properties_node(parent);
        const auto shading = ensure_table_style_shading_node(cell_properties);
        if (shading == pugi::xml_node{}) {
            return false;
        }

        ensure_attribute_value(shading, "w:val", "clear");
        ensure_attribute_value(shading, "w:color", "auto");
        ensure_attribute_value(shading, "w:fill", *region.fill_color);
    }

    if (region.cell_margins.has_value()) {
        const auto properties =
            whole_table ? ensure_table_style_table_properties_node(parent)
                        : ensure_table_style_cell_properties_node(parent);
        if (!apply_table_style_margins(properties,
                                       whole_table ? "w:tblCellMar" : "w:tcMar",
                                       *region.cell_margins)) {
            return false;
        }
    }

    if (region.cell_vertical_alignment.has_value()) {
        const auto cell_properties =
            ensure_table_style_cell_properties_node(parent);
        const auto vertical_alignment =
            ensure_table_style_vertical_alignment_node(cell_properties);
        if (vertical_alignment == pugi::xml_node{}) {
            return false;
        }
        ensure_attribute_value(vertical_alignment, "w:val",
                               to_table_style_cell_vertical_alignment_value(
                                   *region.cell_vertical_alignment));
    }

    if (region.borders.has_value()) {
        const auto properties =
            whole_table ? ensure_table_style_table_properties_node(parent)
                        : ensure_table_style_cell_properties_node(parent);
        if (!apply_table_style_borders(
                properties, whole_table ? "w:tblBorders" : "w:tcBorders",
                *region.borders)) {
            return false;
        }
    }

    if (region.cell_text_direction.has_value()) {
        const auto cell_properties =
            ensure_table_style_cell_properties_node(parent);
        const auto text_direction =
            ensure_table_style_text_direction_node(cell_properties);
        if (text_direction == pugi::xml_node{}) {
            return false;
        }
        ensure_attribute_value(text_direction, "w:val",
                               to_table_style_cell_text_direction_value(
                                   *region.cell_text_direction));
    }

    if (region.paragraph_alignment.has_value()) {
        const auto paragraph_properties =
            ensure_table_style_paragraph_properties_node(parent);
        const auto alignment =
            ensure_table_style_paragraph_alignment_node(paragraph_properties);
        if (alignment == pugi::xml_node{}) {
            return false;
        }
        ensure_attribute_value(alignment, "w:val",
                               to_table_style_paragraph_alignment_value(
                                   *region.paragraph_alignment));
    }

    if (region.paragraph_spacing.has_value()) {
        const auto paragraph_properties =
            ensure_table_style_paragraph_properties_node(parent);
        if (!apply_table_style_paragraph_spacing(paragraph_properties,
                                                 *region.paragraph_spacing)) {
            return false;
        }
    }

    if (region.text_color.has_value()) {
        const auto run_properties =
            ensure_table_style_run_properties_node(parent);
        const auto color = ensure_table_style_text_color_node(run_properties);
        if (color == pugi::xml_node{}) {
            return false;
        }

        ensure_attribute_value(color, "w:val", *region.text_color);
    }

    if (region.bold.has_value()) {
        const auto run_properties =
            ensure_table_style_run_properties_node(parent);
        const auto bold = ensure_table_style_bold_node(run_properties);
        if (bold == pugi::xml_node{}) {
            return false;
        }

        set_on_off_value(bold, *region.bold);
    }

    if (region.italic.has_value()) {
        const auto run_properties =
            ensure_table_style_run_properties_node(parent);
        const auto italic = ensure_table_style_italic_node(run_properties);
        if (italic == pugi::xml_node{}) {
            return false;
        }

        set_on_off_value(italic, *region.italic);
    }

    if (region.font_size_points.has_value()) {
        const auto run_properties =
            ensure_table_style_run_properties_node(parent);
        const auto primary_size =
            ensure_table_style_font_size_node(run_properties, "w:sz");
        const auto complex_size =
            ensure_table_style_font_size_node(run_properties, "w:szCs");
        if (primary_size == pugi::xml_node{} ||
            complex_size == pugi::xml_node{}) {
            return false;
        }

        const auto half_points = std::to_string(*region.font_size_points * 2U);
        ensure_attribute_value(primary_size, "w:val", half_points);
        ensure_attribute_value(complex_size, "w:val", half_points);
    }

    if (region.font_family.has_value() ||
        region.east_asia_font_family.has_value()) {
        const auto run_properties =
            ensure_table_style_run_properties_node(parent);
        const auto run_fonts = ensure_run_fonts_node(run_properties);
        if (run_fonts == pugi::xml_node{}) {
            return false;
        }

        if (region.font_family.has_value()) {
            ensure_attribute_value(run_fonts, "w:ascii", *region.font_family);
            ensure_attribute_value(run_fonts, "w:hAnsi", *region.font_family);
            ensure_attribute_value(run_fonts, "w:cs", *region.font_family);
        }
        if (region.east_asia_font_family.has_value()) {
            ensure_attribute_value(run_fonts, "w:eastAsia",
                                   *region.east_asia_font_family);
        }
    }

    return true;
}

auto apply_table_style_conditional_region(
    pugi::xml_node style, std::string_view type_name,
    const std::optional<featherdoc::table_style_region_definition> &region)
    -> bool {
    if (!region.has_value()) {
        return true;
    }

    const auto conditional =
        ensure_table_style_conditional_node(style, type_name);
    return conditional != pugi::xml_node{} &&
           apply_table_style_region(conditional, *region, false);
}

auto apply_table_style_regions(
    pugi::xml_node style,
    const featherdoc::table_style_definition &definition) -> bool {
    if (definition.whole_table.has_value() &&
        !apply_table_style_region(style, *definition.whole_table, true)) {
        return false;
    }

    return apply_table_style_conditional_region(style, "firstRow",
                                                definition.first_row) &&
           apply_table_style_conditional_region(style, "lastRow",
                                                definition.last_row) &&
           apply_table_style_conditional_region(style, "firstCol",
                                                definition.first_column) &&
           apply_table_style_conditional_region(style, "lastCol",
                                                definition.last_column) &&
           apply_table_style_conditional_region(style, "band1Horz",
                                                definition.banded_rows) &&
           apply_table_style_conditional_region(style, "band1Vert",
                                                definition.banded_columns) &&
           apply_table_style_conditional_region(
               style, "band2Horz", definition.second_banded_rows) &&
           apply_table_style_conditional_region(
               style, "band2Vert", definition.second_banded_columns);
}

auto validate_table_style_region(
    featherdoc::document_error_info &last_error_info,
    std::string_view region_name,
    const std::optional<featherdoc::table_style_region_definition> &region)
    -> bool {
    if (!region.has_value()) {
        return true;
    }

    if (region->fill_color.has_value() && region->fill_color->empty()) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "table style " + std::string{region_name} +
                           " fill color must not be empty",
                       std::string{styles_xml_entry});
        return false;
    }

    if (region->text_color.has_value() && region->text_color->empty()) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "table style " + std::string{region_name} +
                           " text color must not be empty",
                       std::string{styles_xml_entry});
        return false;
    }

    if (region->font_size_points.has_value() &&
        *region->font_size_points == 0U) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "table style " + std::string{region_name} +
                           " font size points must be greater than zero",
                       std::string{styles_xml_entry});
        return false;
    }

    if (region->font_size_points.has_value() &&
        *region->font_size_points >
            std::numeric_limits<std::uint32_t>::max() / 2U) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "table style " + std::string{region_name} +
                           " font size points is too large",
                       std::string{styles_xml_entry});
        return false;
    }

    if (region->font_family.has_value() && region->font_family->empty()) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "table style " + std::string{region_name} +
                           " font family must not be empty",
                       std::string{styles_xml_entry});
        return false;
    }

    if (region->east_asia_font_family.has_value() &&
        region->east_asia_font_family->empty()) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "table style " + std::string{region_name} +
                           " eastAsia font family must not be empty",
                       std::string{styles_xml_entry});
        return false;
    }

    return true;
}

auto validate_table_style_regions(
    featherdoc::document_error_info &last_error_info,
    const featherdoc::table_style_definition &definition) -> bool {
    return validate_table_style_region(last_error_info, "whole_table",
                                       definition.whole_table) &&
           validate_table_style_region(last_error_info, "first_row",
                                       definition.first_row) &&
           validate_table_style_region(last_error_info, "last_row",
                                       definition.last_row) &&
           validate_table_style_region(last_error_info, "first_column",
                                       definition.first_column) &&
           validate_table_style_region(last_error_info, "last_column",
                                       definition.last_column) &&
           validate_table_style_region(last_error_info, "banded_rows",
                                       definition.banded_rows) &&
           validate_table_style_region(last_error_info, "banded_columns",
                                       definition.banded_columns) &&
           validate_table_style_region(last_error_info, "second_banded_rows",
                                       definition.second_banded_rows) &&
           validate_table_style_region(last_error_info, "second_banded_columns",
                                       definition.second_banded_columns);
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

