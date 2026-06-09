#include "table_xml_helpers.hpp"

#include <charconv>
#include <system_error>

namespace featherdoc::detail {

auto count_named_children(pugi::xml_node parent, std::string_view child_name)
    -> std::size_t {
    std::size_t count = 0;
    for (auto child = parent.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        if (std::string_view{child.name()} == child_name) {
            ++count;
        }
    }
    return count;
}

auto string_matrix_from_initializer_list(
    std::initializer_list<std::initializer_list<std::string>> rows)
    -> std::vector<std::vector<std::string>> {
    auto result = std::vector<std::vector<std::string>>{};
    result.reserve(rows.size());
    for (const auto &row : rows) {
        result.emplace_back(row.begin(), row.end());
    }
    return result;
}

void ensure_attribute_value(pugi::xml_node node, const char *name, const char *value) {
    if (node == pugi::xml_node{}) {
        return;
    }

    auto attribute = node.attribute(name);
    if (attribute == pugi::xml_attribute{}) {
        attribute = node.append_attribute(name);
    }
    attribute.set_value(value);
}

void ensure_default_attribute_value(pugi::xml_node node, const char *name, const char *value) {
    if (node == pugi::xml_node{}) {
        return;
    }

    auto attribute = node.attribute(name);
    if (attribute == pugi::xml_attribute{}) {
        attribute = node.append_attribute(name);
        attribute.set_value(value);
        return;
    }

    if (std::string_view{attribute.value()}.empty()) {
        attribute.set_value(value);
    }
}

auto ensure_table_properties_node(pugi::xml_node table) -> pugi::xml_node {
    if (table == pugi::xml_node{}) {
        return {};
    }

    auto table_properties = table.child("w:tblPr");
    if (table_properties != pugi::xml_node{}) {
        return table_properties;
    }

    if (const auto first_child = table.first_child(); first_child != pugi::xml_node{}) {
        return table.insert_child_before("w:tblPr", first_child);
    }

    return table.append_child("w:tblPr");
}

auto ensure_table_look_node(pugi::xml_node table) -> pugi::xml_node {
    auto table_properties = ensure_table_properties_node(table);
    if (table_properties == pugi::xml_node{}) {
        return {};
    }

    auto table_look = table_properties.child("w:tblLook");
    if (table_look != pugi::xml_node{}) {
        return table_look;
    }

    if (const auto margins = table_properties.child("w:tblCellMar");
        margins != pugi::xml_node{}) {
        return table_properties.insert_child_after("w:tblLook", margins);
    }

    if (const auto table_layout = table_properties.child("w:tblLayout");
        table_layout != pugi::xml_node{}) {
        return table_properties.insert_child_after("w:tblLook", table_layout);
    }

    if (const auto table_borders = table_properties.child("w:tblBorders");
        table_borders != pugi::xml_node{}) {
        return table_properties.insert_child_after("w:tblLook", table_borders);
    }

    if (const auto spacing = table_properties.child("w:tblCellSpacing");
        spacing != pugi::xml_node{}) {
        return table_properties.insert_child_after("w:tblLook", spacing);
    }

    if (const auto table_indent = table_properties.child("w:tblInd");
        table_indent != pugi::xml_node{}) {
        return table_properties.insert_child_after("w:tblLook", table_indent);
    }

    if (const auto alignment = table_properties.child("w:jc");
        alignment != pugi::xml_node{}) {
        return table_properties.insert_child_after("w:tblLook", alignment);
    }

    if (const auto table_width = table_properties.child("w:tblW");
        table_width != pugi::xml_node{}) {
        return table_properties.insert_child_after("w:tblLook", table_width);
    }

    if (const auto table_style = table_properties.child("w:tblStyle");
        table_style != pugi::xml_node{}) {
        return table_properties.insert_child_after("w:tblLook", table_style);
    }

    if (const auto first_child = table_properties.first_child(); first_child != pugi::xml_node{}) {
        return table_properties.insert_child_before("w:tblLook", first_child);
    }

    return table_properties.append_child("w:tblLook");
}

auto encode_table_style_look(featherdoc::table_style_look style_look) -> std::uint16_t {
    auto value = std::uint16_t{0U};
    if (style_look.first_row) {
        value = static_cast<std::uint16_t>(value | table_style_look_first_row_bit);
    }
    if (style_look.last_row) {
        value = static_cast<std::uint16_t>(value | table_style_look_last_row_bit);
    }
    if (style_look.first_column) {
        value = static_cast<std::uint16_t>(value | table_style_look_first_column_bit);
    }
    if (style_look.last_column) {
        value = static_cast<std::uint16_t>(value | table_style_look_last_column_bit);
    }
    if (!style_look.banded_rows) {
        value = static_cast<std::uint16_t>(value | table_style_look_no_hband_bit);
    }
    if (!style_look.banded_columns) {
        value = static_cast<std::uint16_t>(value | table_style_look_no_vband_bit);
    }
    return value;
}

auto format_short_hex(std::uint16_t value) -> std::string {
    constexpr auto digits = std::string_view{"0123456789ABCDEF"};
    auto result = std::string(4U, '0');
    for (auto index = std::size_t{0U}; index < result.size(); ++index) {
        const auto shift = static_cast<unsigned>((result.size() - index - 1U) * 4U);
        result[index] = digits[(value >> shift) & 0x0FU];
    }
    return result;
}

auto parse_xml_on_off_value(std::string_view value) -> std::optional<bool> {
    if (value.empty()) {
        return std::nullopt;
    }

    if (value == "1" || value == "true" || value == "on" || value == "TRUE" ||
        value == "ON") {
        return true;
    }

    if (value == "0" || value == "false" || value == "off" || value == "FALSE" ||
        value == "OFF") {
        return false;
    }

    return std::nullopt;
}

auto parse_short_hex_value(std::string_view value) -> std::optional<std::uint16_t> {
    if (value.empty()) {
        return std::nullopt;
    }

    auto parsed = std::uint16_t{0U};
    const auto *begin = value.data();
    const auto *end = begin + value.size();
    const auto [ptr, ec] = std::from_chars(begin, end, parsed, 16);
    if (ec != std::errc{} || ptr != end) {
        return std::nullopt;
    }

    return parsed;
}

auto decode_table_style_look_flag(std::optional<bool> attribute_value,
                                  std::optional<std::uint16_t> encoded_value,
                                  std::uint16_t bit, bool default_value,
                                  bool inverted) -> bool {
    if (attribute_value.has_value()) {
        return inverted ? !*attribute_value : *attribute_value;
    }

    if (encoded_value.has_value()) {
        const auto raw_value = (*encoded_value & bit) != 0U;
        return inverted ? !raw_value : raw_value;
    }

    return default_value;
}

void ensure_default_table_properties(pugi::xml_node table) {
    auto table_properties = ensure_table_properties_node(table);
    if (table_properties == pugi::xml_node{}) {
        return;
    }

    auto table_width = table_properties.child("w:tblW");
    if (table_width == pugi::xml_node{}) {
        table_width = table_properties.append_child("w:tblW");
    }
    ensure_default_attribute_value(table_width, "w:w", "0");
    ensure_default_attribute_value(table_width, "w:type", "auto");

    auto table_look = ensure_table_look_node(table);
    ensure_default_attribute_value(table_look, "w:val", "04A0");
    ensure_default_attribute_value(table_look, "w:firstRow", "1");
    ensure_default_attribute_value(table_look, "w:firstColumn", "1");
    ensure_default_attribute_value(table_look, "w:lastRow", "0");
    ensure_default_attribute_value(table_look, "w:lastColumn", "0");
    ensure_default_attribute_value(table_look, "w:noHBand", "0");
    ensure_default_attribute_value(table_look, "w:noVBand", "1");
}

auto ensure_table_width_node(pugi::xml_node table) -> pugi::xml_node {
    auto table_properties = ensure_table_properties_node(table);
    if (table_properties == pugi::xml_node{}) {
        return {};
    }

    auto table_width = table_properties.child("w:tblW");
    if (table_width != pugi::xml_node{}) {
        return table_width;
    }

    if (const auto table_style = table_properties.child("w:tblStyle");
        table_style != pugi::xml_node{}) {
        return table_properties.insert_child_after("w:tblW", table_style);
    }

    if (const auto first_child = table_properties.first_child(); first_child != pugi::xml_node{}) {
        return table_properties.insert_child_before("w:tblW", first_child);
    }

    return table_properties.append_child("w:tblW");
}

auto ensure_table_layout_node(pugi::xml_node table) -> pugi::xml_node {
    auto table_properties = ensure_table_properties_node(table);
    if (table_properties == pugi::xml_node{}) {
        return {};
    }

    auto table_layout = table_properties.child("w:tblLayout");
    if (table_layout != pugi::xml_node{}) {
        return table_layout;
    }

    if (const auto table_width = table_properties.child("w:tblW");
        table_width != pugi::xml_node{}) {
        return table_properties.insert_child_after("w:tblLayout", table_width);
    }

    if (const auto table_style = table_properties.child("w:tblStyle");
        table_style != pugi::xml_node{}) {
        return table_properties.insert_child_after("w:tblLayout", table_style);
    }

    if (const auto first_child = table_properties.first_child(); first_child != pugi::xml_node{}) {
        return table_properties.insert_child_before("w:tblLayout", first_child);
    }

    return table_properties.append_child("w:tblLayout");
}

auto ensure_table_alignment_node(pugi::xml_node table) -> pugi::xml_node {
    auto table_properties = ensure_table_properties_node(table);
    if (table_properties == pugi::xml_node{}) {
        return {};
    }

    auto alignment = table_properties.child("w:jc");
    if (alignment != pugi::xml_node{}) {
        return alignment;
    }

    if (const auto table_layout = table_properties.child("w:tblLayout");
        table_layout != pugi::xml_node{}) {
        return table_properties.insert_child_before("w:jc", table_layout);
    }

    if (const auto table_look = table_properties.child("w:tblLook");
        table_look != pugi::xml_node{}) {
        return table_properties.insert_child_before("w:jc", table_look);
    }

    if (const auto table_width = table_properties.child("w:tblW");
        table_width != pugi::xml_node{}) {
        return table_properties.insert_child_after("w:jc", table_width);
    }

    if (const auto table_style = table_properties.child("w:tblStyle");
        table_style != pugi::xml_node{}) {
        return table_properties.insert_child_after("w:jc", table_style);
    }

    if (const auto first_child = table_properties.first_child(); first_child != pugi::xml_node{}) {
        return table_properties.insert_child_before("w:jc", first_child);
    }

    return table_properties.append_child("w:jc");
}

auto ensure_table_indent_node(pugi::xml_node table) -> pugi::xml_node {
    auto table_properties = ensure_table_properties_node(table);
    if (table_properties == pugi::xml_node{}) {
        return {};
    }

    auto table_indent = table_properties.child("w:tblInd");
    if (table_indent != pugi::xml_node{}) {
        return table_indent;
    }

    if (const auto table_layout = table_properties.child("w:tblLayout");
        table_layout != pugi::xml_node{}) {
        return table_properties.insert_child_before("w:tblInd", table_layout);
    }

    if (const auto table_look = table_properties.child("w:tblLook");
        table_look != pugi::xml_node{}) {
        return table_properties.insert_child_before("w:tblInd", table_look);
    }

    if (const auto alignment = table_properties.child("w:jc"); alignment != pugi::xml_node{}) {
        return table_properties.insert_child_after("w:tblInd", alignment);
    }

    if (const auto table_width = table_properties.child("w:tblW");
        table_width != pugi::xml_node{}) {
        return table_properties.insert_child_after("w:tblInd", table_width);
    }

    if (const auto table_style = table_properties.child("w:tblStyle");
        table_style != pugi::xml_node{}) {
        return table_properties.insert_child_after("w:tblInd", table_style);
    }

    if (const auto first_child = table_properties.first_child(); first_child != pugi::xml_node{}) {
        return table_properties.insert_child_before("w:tblInd", first_child);
    }

    return table_properties.append_child("w:tblInd");
}

auto ensure_table_cell_spacing_node(pugi::xml_node table) -> pugi::xml_node {
    auto table_properties = ensure_table_properties_node(table);
    if (table_properties == pugi::xml_node{}) {
        return {};
    }

    auto spacing = table_properties.child("w:tblCellSpacing");
    if (spacing != pugi::xml_node{}) {
        return spacing;
    }

    if (const auto table_indent = table_properties.child("w:tblInd");
        table_indent != pugi::xml_node{}) {
        return table_properties.insert_child_before("w:tblCellSpacing", table_indent);
    }

    if (const auto table_borders = table_properties.child("w:tblBorders");
        table_borders != pugi::xml_node{}) {
        return table_properties.insert_child_before("w:tblCellSpacing", table_borders);
    }

    if (const auto table_layout = table_properties.child("w:tblLayout");
        table_layout != pugi::xml_node{}) {
        return table_properties.insert_child_before("w:tblCellSpacing", table_layout);
    }

    if (const auto margins = table_properties.child("w:tblCellMar");
        margins != pugi::xml_node{}) {
        return table_properties.insert_child_before("w:tblCellSpacing", margins);
    }

    if (const auto table_look = table_properties.child("w:tblLook");
        table_look != pugi::xml_node{}) {
        return table_properties.insert_child_before("w:tblCellSpacing", table_look);
    }

    if (const auto alignment = table_properties.child("w:jc"); alignment != pugi::xml_node{}) {
        return table_properties.insert_child_after("w:tblCellSpacing", alignment);
    }

    if (const auto table_width = table_properties.child("w:tblW");
        table_width != pugi::xml_node{}) {
        return table_properties.insert_child_after("w:tblCellSpacing", table_width);
    }

    if (const auto table_style = table_properties.child("w:tblStyle");
        table_style != pugi::xml_node{}) {
        return table_properties.insert_child_after("w:tblCellSpacing", table_style);
    }

    if (const auto first_child = table_properties.first_child(); first_child != pugi::xml_node{}) {
        return table_properties.insert_child_before("w:tblCellSpacing", first_child);
    }

    return table_properties.append_child("w:tblCellSpacing");
}

auto ensure_table_style_node(pugi::xml_node table) -> pugi::xml_node {
    auto table_properties = ensure_table_properties_node(table);
    if (table_properties == pugi::xml_node{}) {
        return {};
    }

    auto table_style = table_properties.child("w:tblStyle");
    if (table_style != pugi::xml_node{}) {
        return table_style;
    }

    if (const auto table_width = table_properties.child("w:tblW");
        table_width != pugi::xml_node{}) {
        return table_properties.insert_child_before("w:tblStyle", table_width);
    }

    if (const auto first_child = table_properties.first_child(); first_child != pugi::xml_node{}) {
        return table_properties.insert_child_before("w:tblStyle", first_child);
    }

    return table_properties.append_child("w:tblStyle");
}

auto ensure_table_position_node(pugi::xml_node table) -> pugi::xml_node {
    auto table_properties = ensure_table_properties_node(table);
    if (table_properties == pugi::xml_node{}) {
        return {};
    }

    auto position = table_properties.child("w:tblpPr");
    if (position != pugi::xml_node{}) {
        return position;
    }

    if (const auto table_look = table_properties.child("w:tblLook");
        table_look != pugi::xml_node{}) {
        return table_properties.insert_child_before("w:tblpPr", table_look);
    }

    if (const auto margins = table_properties.child("w:tblCellMar");
        margins != pugi::xml_node{}) {
        return table_properties.insert_child_before("w:tblpPr", margins);
    }

    if (const auto table_layout = table_properties.child("w:tblLayout");
        table_layout != pugi::xml_node{}) {
        return table_properties.insert_child_after("w:tblpPr", table_layout);
    }

    if (const auto table_borders = table_properties.child("w:tblBorders");
        table_borders != pugi::xml_node{}) {
        return table_properties.insert_child_after("w:tblpPr", table_borders);
    }

    if (const auto spacing = table_properties.child("w:tblCellSpacing");
        spacing != pugi::xml_node{}) {
        return table_properties.insert_child_after("w:tblpPr", spacing);
    }

    if (const auto table_indent = table_properties.child("w:tblInd");
        table_indent != pugi::xml_node{}) {
        return table_properties.insert_child_after("w:tblpPr", table_indent);
    }

    if (const auto alignment = table_properties.child("w:jc");
        alignment != pugi::xml_node{}) {
        return table_properties.insert_child_after("w:tblpPr", alignment);
    }

    if (const auto table_width = table_properties.child("w:tblW");
        table_width != pugi::xml_node{}) {
        return table_properties.insert_child_after("w:tblpPr", table_width);
    }

    if (const auto table_style = table_properties.child("w:tblStyle");
        table_style != pugi::xml_node{}) {
        return table_properties.insert_child_after("w:tblpPr", table_style);
    }

    if (const auto first_child = table_properties.first_child(); first_child != pugi::xml_node{}) {
        return table_properties.insert_child_before("w:tblpPr", first_child);
    }

    return table_properties.append_child("w:tblpPr");
}

auto ensure_table_cell_margins_node(pugi::xml_node table) -> pugi::xml_node {
    auto table_properties = ensure_table_properties_node(table);
    if (table_properties == pugi::xml_node{}) {
        return {};
    }

    auto margins = table_properties.child("w:tblCellMar");
    if (margins != pugi::xml_node{}) {
        return margins;
    }

    if (const auto table_look = table_properties.child("w:tblLook");
        table_look != pugi::xml_node{}) {
        return table_properties.insert_child_before("w:tblCellMar", table_look);
    }

    if (const auto table_layout = table_properties.child("w:tblLayout");
        table_layout != pugi::xml_node{}) {
        return table_properties.insert_child_after("w:tblCellMar", table_layout);
    }

    if (const auto table_borders = table_properties.child("w:tblBorders");
        table_borders != pugi::xml_node{}) {
        return table_properties.insert_child_after("w:tblCellMar", table_borders);
    }

    if (const auto spacing = table_properties.child("w:tblCellSpacing");
        spacing != pugi::xml_node{}) {
        return table_properties.insert_child_after("w:tblCellMar", spacing);
    }

    if (const auto table_indent = table_properties.child("w:tblInd");
        table_indent != pugi::xml_node{}) {
        return table_properties.insert_child_after("w:tblCellMar", table_indent);
    }

    if (const auto alignment = table_properties.child("w:jc"); alignment != pugi::xml_node{}) {
        return table_properties.insert_child_after("w:tblCellMar", alignment);
    }

    if (const auto table_width = table_properties.child("w:tblW");
        table_width != pugi::xml_node{}) {
        return table_properties.insert_child_after("w:tblCellMar", table_width);
    }

    if (const auto table_style = table_properties.child("w:tblStyle");
        table_style != pugi::xml_node{}) {
        return table_properties.insert_child_after("w:tblCellMar", table_style);
    }

    if (const auto first_child = table_properties.first_child(); first_child != pugi::xml_node{}) {
        return table_properties.insert_child_before("w:tblCellMar", first_child);
    }

    return table_properties.append_child("w:tblCellMar");
}

auto ensure_table_cell_margin_node(pugi::xml_node table, const char *margin_name)
    -> pugi::xml_node {
    auto margins = ensure_table_cell_margins_node(table);
    if (margins == pugi::xml_node{}) {
        return {};
    }

    auto margin = margins.child(margin_name);
    if (margin != pugi::xml_node{}) {
        return margin;
    }

    return margins.append_child(margin_name);
}

auto ensure_table_borders_node(pugi::xml_node table) -> pugi::xml_node {
    auto table_properties = ensure_table_properties_node(table);
    if (table_properties == pugi::xml_node{}) {
        return {};
    }

    auto table_borders = table_properties.child("w:tblBorders");
    if (table_borders != pugi::xml_node{}) {
        return table_borders;
    }

    if (const auto table_look = table_properties.child("w:tblLook");
        table_look != pugi::xml_node{}) {
        return table_properties.insert_child_after("w:tblBorders", table_look);
    }

    if (const auto table_width = table_properties.child("w:tblW");
        table_width != pugi::xml_node{}) {
        return table_properties.insert_child_after("w:tblBorders", table_width);
    }

    if (const auto table_style = table_properties.child("w:tblStyle");
        table_style != pugi::xml_node{}) {
        return table_properties.insert_child_after("w:tblBorders", table_style);
    }

    if (const auto first_child = table_properties.first_child(); first_child != pugi::xml_node{}) {
        return table_properties.insert_child_before("w:tblBorders", first_child);
    }

    return table_properties.append_child("w:tblBorders");
}

} // namespace featherdoc::detail
