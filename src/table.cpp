#include "featherdoc.hpp"
#include "xml_helpers.hpp"

#include <algorithm>
#include <charconv>
#include <limits>

namespace featherdoc {
namespace {

constexpr auto table_style_look_first_row_bit = std::uint16_t{0x0020};
constexpr auto table_style_look_last_row_bit = std::uint16_t{0x0040};
constexpr auto table_style_look_first_column_bit = std::uint16_t{0x0080};
constexpr auto table_style_look_last_column_bit = std::uint16_t{0x0100};
constexpr auto table_style_look_no_hband_bit = std::uint16_t{0x0200};
constexpr auto table_style_look_no_vband_bit = std::uint16_t{0x0400};

enum class cell_vertical_merge_state {
    none = 0,
    restart,
    continue_merge,
};

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
                                  bool inverted = false) -> bool {
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

auto ensure_table_grid_node(pugi::xml_node table) -> pugi::xml_node {
    if (table == pugi::xml_node{}) {
        return {};
    }

    auto table_grid = table.child("w:tblGrid");
    if (table_grid != pugi::xml_node{}) {
        return table_grid;
    }

    const auto table_properties = ensure_table_properties_node(table);
    if (const auto first_row = table.child("w:tr"); first_row != pugi::xml_node{}) {
        return table.insert_child_before("w:tblGrid", first_row);
    }

    if (table_properties != pugi::xml_node{}) {
        return table.insert_child_after("w:tblGrid", table_properties);
    }

    return table.prepend_child("w:tblGrid");
}

auto ensure_row_properties_node(pugi::xml_node row) -> pugi::xml_node {
    if (row == pugi::xml_node{}) {
        return {};
    }

    auto row_properties = row.child("w:trPr");
    if (row_properties != pugi::xml_node{}) {
        return row_properties;
    }

    if (const auto first_cell = row.child("w:tc"); first_cell != pugi::xml_node{}) {
        return row.insert_child_before("w:trPr", first_cell);
    }

    if (const auto first_child = row.first_child(); first_child != pugi::xml_node{}) {
        return row.insert_child_before("w:trPr", first_child);
    }

    return row.append_child("w:trPr");
}

auto ensure_row_height_node(pugi::xml_node row) -> pugi::xml_node {
    auto row_properties = ensure_row_properties_node(row);
    if (row_properties == pugi::xml_node{}) {
        return {};
    }

    auto row_height = row_properties.child("w:trHeight");
    if (row_height != pugi::xml_node{}) {
        return row_height;
    }

    if (const auto cant_split = row_properties.child("w:cantSplit");
        cant_split != pugi::xml_node{}) {
        return row_properties.insert_child_after("w:trHeight", cant_split);
    }

    if (const auto table_header = row_properties.child("w:tblHeader");
        table_header != pugi::xml_node{}) {
        return row_properties.insert_child_before("w:trHeight", table_header);
    }

    if (const auto first_child = row_properties.first_child(); first_child != pugi::xml_node{}) {
        return row_properties.insert_child_before("w:trHeight", first_child);
    }

    return row_properties.append_child("w:trHeight");
}

auto ensure_row_cant_split_node(pugi::xml_node row) -> pugi::xml_node {
    auto row_properties = ensure_row_properties_node(row);
    if (row_properties == pugi::xml_node{}) {
        return {};
    }

    auto cant_split = row_properties.child("w:cantSplit");
    if (cant_split != pugi::xml_node{}) {
        return cant_split;
    }

    if (const auto row_height = row_properties.child("w:trHeight");
        row_height != pugi::xml_node{}) {
        return row_properties.insert_child_before("w:cantSplit", row_height);
    }

    if (const auto table_header = row_properties.child("w:tblHeader");
        table_header != pugi::xml_node{}) {
        return row_properties.insert_child_before("w:cantSplit", table_header);
    }

    if (const auto first_child = row_properties.first_child(); first_child != pugi::xml_node{}) {
        return row_properties.insert_child_before("w:cantSplit", first_child);
    }

    return row_properties.append_child("w:cantSplit");
}

auto ensure_row_header_node(pugi::xml_node row) -> pugi::xml_node {
    auto row_properties = ensure_row_properties_node(row);
    if (row_properties == pugi::xml_node{}) {
        return {};
    }

    auto table_header = row_properties.child("w:tblHeader");
    if (table_header != pugi::xml_node{}) {
        return table_header;
    }

    if (const auto row_height = row_properties.child("w:trHeight");
        row_height != pugi::xml_node{}) {
        return row_properties.insert_child_after("w:tblHeader", row_height);
    }

    if (const auto cant_split = row_properties.child("w:cantSplit");
        cant_split != pugi::xml_node{}) {
        return row_properties.insert_child_after("w:tblHeader", cant_split);
    }

    if (const auto first_child = row_properties.first_child(); first_child != pugi::xml_node{}) {
        return row_properties.insert_child_before("w:tblHeader", first_child);
    }

    return row_properties.append_child("w:tblHeader");
}

auto current_table_column_count(pugi::xml_node table) -> std::size_t {
    if (table == pugi::xml_node{}) {
        return 0U;
    }

    const auto effective_cell_column_span = [](pugi::xml_node cell) -> std::size_t {
        if (cell == pugi::xml_node{}) {
            return 0U;
        }

        const auto span_node = cell.child("w:tcPr").child("w:gridSpan");
        const auto span_text = std::string_view{span_node.attribute("w:val").value()};
        if (span_text.empty()) {
            return 1U;
        }

        std::size_t span = 0U;
        const auto [end, error] =
            std::from_chars(span_text.data(), span_text.data() + span_text.size(), span);
        if (error != std::errc{} || end != span_text.data() + span_text.size() || span == 0U) {
            return 1U;
        }

        return span;
    };

    std::size_t column_count = count_named_children(table.child("w:tblGrid"), "w:gridCol");
    for (auto row = table.child("w:tr"); row != pugi::xml_node{};
         row = detail::next_named_sibling(row, "w:tr")) {
        std::size_t row_column_count = 0U;
        for (auto cell = row.child("w:tc"); cell != pugi::xml_node{};
             cell = detail::next_named_sibling(cell, "w:tc")) {
            row_column_count += effective_cell_column_span(cell);
        }
        column_count = std::max(column_count, row_column_count);
    }

    return column_count;
}

void ensure_table_grid_columns(pugi::xml_node table, std::size_t column_count) {
    if (table == pugi::xml_node{}) {
        return;
    }

    ensure_default_table_properties(table);
    auto table_grid = ensure_table_grid_node(table);
    if (table_grid == pugi::xml_node{}) {
        return;
    }

    auto existing_columns = count_named_children(table_grid, "w:gridCol");
    while (existing_columns < column_count) {
        auto grid_column = table_grid.append_child("w:gridCol");
        ensure_attribute_value(grid_column, "w:w", "0");
        ++existing_columns;
    }
}

auto find_table_grid_column(pugi::xml_node table, std::size_t column_index) -> pugi::xml_node {
    if (table == pugi::xml_node{}) {
        return {};
    }

    auto grid_column = table.child("w:tblGrid").child("w:gridCol");
    for (std::size_t index = 0U; index < column_index && grid_column != pugi::xml_node{};
         ++index) {
        grid_column = detail::next_named_sibling(grid_column, "w:gridCol");
    }

    return grid_column;
}

auto ensure_cell_properties_node(pugi::xml_node cell) -> pugi::xml_node {
    if (cell == pugi::xml_node{}) {
        return {};
    }

    auto cell_properties = cell.child("w:tcPr");
    if (cell_properties != pugi::xml_node{}) {
        return cell_properties;
    }

    if (const auto first_paragraph = cell.child("w:p"); first_paragraph != pugi::xml_node{}) {
        return cell.insert_child_before("w:tcPr", first_paragraph);
    }

    if (const auto first_child = cell.first_child(); first_child != pugi::xml_node{}) {
        return cell.insert_child_before("w:tcPr", first_child);
    }

    return cell.append_child("w:tcPr");
}

void ensure_default_cell_properties(pugi::xml_node cell) {
    auto cell_properties = ensure_cell_properties_node(cell);
    if (cell_properties == pugi::xml_node{}) {
        return;
    }

    auto cell_width = cell_properties.child("w:tcW");
    if (cell_width == pugi::xml_node{}) {
        cell_width = cell_properties.append_child("w:tcW");
    }
    ensure_attribute_value(cell_width, "w:w", "0");
    ensure_attribute_value(cell_width, "w:type", "auto");
}

auto ensure_cell_width_node(pugi::xml_node cell) -> pugi::xml_node {
    auto cell_properties = ensure_cell_properties_node(cell);
    if (cell_properties == pugi::xml_node{}) {
        return {};
    }

    auto cell_width = cell_properties.child("w:tcW");
    if (cell_width != pugi::xml_node{}) {
        return cell_width;
    }

    if (const auto first_child = cell_properties.first_child(); first_child != pugi::xml_node{}) {
        return cell_properties.insert_child_before("w:tcW", first_child);
    }

    return cell_properties.append_child("w:tcW");
}

bool clear_cell_width_node(pugi::xml_node cell) {
    if (cell == pugi::xml_node{}) {
        return false;
    }

    auto cell_properties = cell.child("w:tcPr");
    if (cell_properties == pugi::xml_node{}) {
        return true;
    }

    const auto width_node = cell_properties.child("w:tcW");
    return width_node == pugi::xml_node{} || cell_properties.remove_child(width_node);
}

auto ensure_cell_vertical_merge_node(pugi::xml_node cell) -> pugi::xml_node {
    auto cell_properties = ensure_cell_properties_node(cell);
    if (cell_properties == pugi::xml_node{}) {
        return {};
    }

    auto vertical_merge = cell_properties.child("w:vMerge");
    if (vertical_merge != pugi::xml_node{}) {
        return vertical_merge;
    }

    if (const auto grid_span = cell_properties.child("w:gridSpan"); grid_span != pugi::xml_node{}) {
        return cell_properties.insert_child_after("w:vMerge", grid_span);
    }

    if (const auto cell_width = cell_properties.child("w:tcW"); cell_width != pugi::xml_node{}) {
        return cell_properties.insert_child_after("w:vMerge", cell_width);
    }

    if (const auto first_child = cell_properties.first_child(); first_child != pugi::xml_node{}) {
        return cell_properties.insert_child_before("w:vMerge", first_child);
    }

    return cell_properties.append_child("w:vMerge");
}

auto ensure_cell_borders_node(pugi::xml_node cell) -> pugi::xml_node {
    auto cell_properties = ensure_cell_properties_node(cell);
    if (cell_properties == pugi::xml_node{}) {
        return {};
    }

    auto cell_borders = cell_properties.child("w:tcBorders");
    if (cell_borders != pugi::xml_node{}) {
        return cell_borders;
    }

    if (const auto vertical_merge = cell_properties.child("w:vMerge");
        vertical_merge != pugi::xml_node{}) {
        return cell_properties.insert_child_after("w:tcBorders", vertical_merge);
    }

    if (const auto grid_span = cell_properties.child("w:gridSpan"); grid_span != pugi::xml_node{}) {
        return cell_properties.insert_child_after("w:tcBorders", grid_span);
    }

    if (const auto cell_width = cell_properties.child("w:tcW"); cell_width != pugi::xml_node{}) {
        return cell_properties.insert_child_after("w:tcBorders", cell_width);
    }

    if (const auto first_child = cell_properties.first_child(); first_child != pugi::xml_node{}) {
        return cell_properties.insert_child_before("w:tcBorders", first_child);
    }

    return cell_properties.append_child("w:tcBorders");
}

auto ensure_cell_vertical_alignment_node(pugi::xml_node cell) -> pugi::xml_node {
    auto cell_properties = ensure_cell_properties_node(cell);
    if (cell_properties == pugi::xml_node{}) {
        return {};
    }

    auto vertical_alignment = cell_properties.child("w:vAlign");
    if (vertical_alignment != pugi::xml_node{}) {
        return vertical_alignment;
    }

    if (const auto margins = cell_properties.child("w:tcMar"); margins != pugi::xml_node{}) {
        return cell_properties.insert_child_after("w:vAlign", margins);
    }

    if (const auto shading = cell_properties.child("w:shd"); shading != pugi::xml_node{}) {
        return cell_properties.insert_child_after("w:vAlign", shading);
    }

    if (const auto borders = cell_properties.child("w:tcBorders"); borders != pugi::xml_node{}) {
        return cell_properties.insert_child_after("w:vAlign", borders);
    }

    if (const auto vertical_merge = cell_properties.child("w:vMerge");
        vertical_merge != pugi::xml_node{}) {
        return cell_properties.insert_child_after("w:vAlign", vertical_merge);
    }

    if (const auto grid_span = cell_properties.child("w:gridSpan"); grid_span != pugi::xml_node{}) {
        return cell_properties.insert_child_after("w:vAlign", grid_span);
    }

    if (const auto cell_width = cell_properties.child("w:tcW"); cell_width != pugi::xml_node{}) {
        return cell_properties.insert_child_after("w:vAlign", cell_width);
    }

    if (const auto first_child = cell_properties.first_child(); first_child != pugi::xml_node{}) {
        return cell_properties.insert_child_before("w:vAlign", first_child);
    }

    return cell_properties.append_child("w:vAlign");
}

auto ensure_cell_text_direction_node(pugi::xml_node cell) -> pugi::xml_node {
    auto cell_properties = ensure_cell_properties_node(cell);
    if (cell_properties == pugi::xml_node{}) {
        return {};
    }

    auto text_direction = cell_properties.child("w:textDirection");
    if (text_direction != pugi::xml_node{}) {
        return text_direction;
    }

    if (const auto vertical_alignment = cell_properties.child("w:vAlign");
        vertical_alignment != pugi::xml_node{}) {
        return cell_properties.insert_child_before("w:textDirection", vertical_alignment);
    }

    if (const auto margins = cell_properties.child("w:tcMar"); margins != pugi::xml_node{}) {
        return cell_properties.insert_child_after("w:textDirection", margins);
    }

    if (const auto shading = cell_properties.child("w:shd"); shading != pugi::xml_node{}) {
        return cell_properties.insert_child_after("w:textDirection", shading);
    }

    if (const auto borders = cell_properties.child("w:tcBorders"); borders != pugi::xml_node{}) {
        return cell_properties.insert_child_after("w:textDirection", borders);
    }

    if (const auto vertical_merge = cell_properties.child("w:vMerge");
        vertical_merge != pugi::xml_node{}) {
        return cell_properties.insert_child_after("w:textDirection", vertical_merge);
    }

    if (const auto grid_span = cell_properties.child("w:gridSpan"); grid_span != pugi::xml_node{}) {
        return cell_properties.insert_child_after("w:textDirection", grid_span);
    }

    if (const auto cell_width = cell_properties.child("w:tcW"); cell_width != pugi::xml_node{}) {
        return cell_properties.insert_child_after("w:textDirection", cell_width);
    }

    if (const auto first_child = cell_properties.first_child(); first_child != pugi::xml_node{}) {
        return cell_properties.insert_child_before("w:textDirection", first_child);
    }

    return cell_properties.append_child("w:textDirection");
}

auto ensure_cell_shading_node(pugi::xml_node cell) -> pugi::xml_node {
    auto cell_properties = ensure_cell_properties_node(cell);
    if (cell_properties == pugi::xml_node{}) {
        return {};
    }

    auto shading = cell_properties.child("w:shd");
    if (shading != pugi::xml_node{}) {
        return shading;
    }

    if (const auto borders = cell_properties.child("w:tcBorders"); borders != pugi::xml_node{}) {
        return cell_properties.insert_child_after("w:shd", borders);
    }

    if (const auto vertical_merge = cell_properties.child("w:vMerge");
        vertical_merge != pugi::xml_node{}) {
        return cell_properties.insert_child_after("w:shd", vertical_merge);
    }

    if (const auto grid_span = cell_properties.child("w:gridSpan"); grid_span != pugi::xml_node{}) {
        return cell_properties.insert_child_after("w:shd", grid_span);
    }

    if (const auto cell_width = cell_properties.child("w:tcW"); cell_width != pugi::xml_node{}) {
        return cell_properties.insert_child_after("w:shd", cell_width);
    }

    if (const auto first_child = cell_properties.first_child(); first_child != pugi::xml_node{}) {
        return cell_properties.insert_child_before("w:shd", first_child);
    }

    return cell_properties.append_child("w:shd");
}

auto ensure_cell_margins_node(pugi::xml_node cell) -> pugi::xml_node {
    auto cell_properties = ensure_cell_properties_node(cell);
    if (cell_properties == pugi::xml_node{}) {
        return {};
    }

    auto margins = cell_properties.child("w:tcMar");
    if (margins != pugi::xml_node{}) {
        return margins;
    }

    if (const auto shading = cell_properties.child("w:shd"); shading != pugi::xml_node{}) {
        return cell_properties.insert_child_after("w:tcMar", shading);
    }

    if (const auto borders = cell_properties.child("w:tcBorders"); borders != pugi::xml_node{}) {
        return cell_properties.insert_child_after("w:tcMar", borders);
    }

    if (const auto vertical_merge = cell_properties.child("w:vMerge");
        vertical_merge != pugi::xml_node{}) {
        return cell_properties.insert_child_after("w:tcMar", vertical_merge);
    }

    if (const auto grid_span = cell_properties.child("w:gridSpan"); grid_span != pugi::xml_node{}) {
        return cell_properties.insert_child_after("w:tcMar", grid_span);
    }

    if (const auto cell_width = cell_properties.child("w:tcW"); cell_width != pugi::xml_node{}) {
        return cell_properties.insert_child_after("w:tcMar", cell_width);
    }

    if (const auto first_child = cell_properties.first_child(); first_child != pugi::xml_node{}) {
        return cell_properties.insert_child_before("w:tcMar", first_child);
    }

    return cell_properties.append_child("w:tcMar");
}

auto ensure_cell_margin_node(pugi::xml_node cell, const char *margin_name) -> pugi::xml_node {
    auto margins = ensure_cell_margins_node(cell);
    if (margins == pugi::xml_node{}) {
        return {};
    }

    auto margin = margins.child(margin_name);
    if (margin != pugi::xml_node{}) {
        return margin;
    }

    return margins.append_child(margin_name);
}

auto ensure_cell_grid_span_node(pugi::xml_node cell) -> pugi::xml_node {
    auto cell_properties = ensure_cell_properties_node(cell);
    if (cell_properties == pugi::xml_node{}) {
        return {};
    }

    auto grid_span = cell_properties.child("w:gridSpan");
    if (grid_span != pugi::xml_node{}) {
        return grid_span;
    }

    if (const auto cell_width = cell_properties.child("w:tcW"); cell_width != pugi::xml_node{}) {
        return cell_properties.insert_child_after("w:gridSpan", cell_width);
    }

    if (const auto first_child = cell_properties.first_child(); first_child != pugi::xml_node{}) {
        return cell_properties.insert_child_before("w:gridSpan", first_child);
    }

    return cell_properties.append_child("w:gridSpan");
}

auto parse_signed_attribute(pugi::xml_node node, const char *attribute_name)
    -> std::optional<std::int32_t> {
    if (node == pugi::xml_node{}) {
        return std::nullopt;
    }

    const auto value_text = std::string_view{node.attribute(attribute_name).value()};
    if (value_text.empty()) {
        return std::nullopt;
    }

    std::int32_t value = 0;
    const auto [end, error] =
        std::from_chars(value_text.data(), value_text.data() + value_text.size(), value);
    if (error != std::errc{} || end != value_text.data() + value_text.size()) {
        return std::nullopt;
    }

    return value;
}

auto parse_unsigned_attribute(pugi::xml_node node, const char *attribute_name)
    -> std::optional<std::uint32_t> {
    if (node == pugi::xml_node{}) {
        return std::nullopt;
    }

    const auto value_text = std::string_view{node.attribute(attribute_name).value()};
    if (value_text.empty()) {
        return std::nullopt;
    }

    std::uint32_t value = 0U;
    const auto [end, error] =
        std::from_chars(value_text.data(), value_text.data() + value_text.size(), value);
    if (error != std::errc{} || end != value_text.data() + value_text.size()) {
        return std::nullopt;
    }

    return value;
}

auto on_off_node_enabled(pugi::xml_node node) -> bool {
    if (node == pugi::xml_node{}) {
        return false;
    }

    const auto value = std::string_view{node.attribute("w:val").value()};
    if (value.empty()) {
        return true;
    }

    return value != "0" && value != "false" && value != "off";
}

auto cell_column_span(pugi::xml_node cell) -> std::size_t {
    if (cell == pugi::xml_node{}) {
        return 0U;
    }

    const auto span = parse_unsigned_attribute(cell.child("w:tcPr").child("w:gridSpan"), "w:val");
    if (!span.has_value() || *span == 0U) {
        return 1U;
    }

    return *span;
}

auto cell_vertical_merge_state_for(pugi::xml_node cell) -> cell_vertical_merge_state {
    const auto vertical_merge = cell.child("w:tcPr").child("w:vMerge");
    if (vertical_merge == pugi::xml_node{}) {
        return cell_vertical_merge_state::none;
    }

    const auto merge_value = std::string_view{vertical_merge.attribute("w:val").value()};
    if (merge_value == "restart") {
        return cell_vertical_merge_state::restart;
    }

    return cell_vertical_merge_state::continue_merge;
}

auto row_contains_vertical_merge_cells(pugi::xml_node row) -> bool {
    if (row == pugi::xml_node{}) {
        return false;
    }

    for (auto cell = row.child("w:tc"); cell != pugi::xml_node{};
         cell = detail::next_named_sibling(cell, "w:tc")) {
        if (cell_vertical_merge_state_for(cell) != cell_vertical_merge_state::none) {
            return true;
        }
    }

    return false;
}

auto insert_empty_clone_row(pugi::xml_node table, pugi::xml_node source_row,
                            pugi::xml_node merge_guard_row, bool insert_after)
    -> pugi::xml_node {
    if (table == pugi::xml_node{} || source_row == pugi::xml_node{}) {
        return {};
    }

    if (row_contains_vertical_merge_cells(source_row) ||
        row_contains_vertical_merge_cells(merge_guard_row)) {
        return {};
    }

    auto inserted_row = insert_after ? table.insert_copy_after(source_row, source_row)
                                     : table.insert_copy_before(source_row, source_row);
    if (inserted_row == pugi::xml_node{}) {
        return {};
    }

    for (auto row_cell = inserted_row.child("w:tc"); row_cell != pugi::xml_node{};
         row_cell = detail::next_named_sibling(row_cell, "w:tc")) {
        if (!TableCell(inserted_row, row_cell).set_text("")) {
            table.remove_child(inserted_row);
            return {};
        }
    }

    return inserted_row;
}

auto clear_table_cell_contents(pugi::xml_node table) -> bool {
    if (table == pugi::xml_node{}) {
        return false;
    }

    for (auto row = table.child("w:tr"); row != pugi::xml_node{};
         row = detail::next_named_sibling(row, "w:tr")) {
        for (auto cell = row.child("w:tc"); cell != pugi::xml_node{};
             cell = detail::next_named_sibling(cell, "w:tc")) {
            if (!TableCell(row, cell).set_text("")) {
                return false;
            }
        }
    }

    return true;
}

auto insert_empty_clone_table(pugi::xml_node parent, pugi::xml_node source_table,
                              bool insert_after) -> pugi::xml_node {
    if (parent == pugi::xml_node{} || source_table == pugi::xml_node{}) {
        return {};
    }

    auto inserted_table = insert_after ? parent.insert_copy_after(source_table, source_table)
                                       : parent.insert_copy_before(source_table, source_table);
    if (inserted_table == pugi::xml_node{}) {
        return {};
    }

    if (!clear_table_cell_contents(inserted_table)) {
        parent.remove_child(inserted_table);
        return {};
    }

    return inserted_table;
}

auto cell_column_index(pugi::xml_node cell) -> std::optional<std::size_t> {
    if (cell == pugi::xml_node{}) {
        return std::nullopt;
    }

    const auto row = cell.parent();
    if (row == pugi::xml_node{}) {
        return std::nullopt;
    }

    std::size_t column_index = 0U;
    for (auto candidate = row.child("w:tc"); candidate != pugi::xml_node{};
         candidate = detail::next_named_sibling(candidate, "w:tc")) {
        if (candidate == cell) {
            return column_index;
        }
        column_index += cell_column_span(candidate);
    }

    return std::nullopt;
}

auto table_uses_fixed_layout(pugi::xml_node table) -> bool {
    if (table == pugi::xml_node{}) {
        return false;
    }

    return std::string_view{
               table.child("w:tblPr").child("w:tblLayout").attribute("w:type").value()} ==
           "fixed";
}

auto grid_column_width_twips(pugi::xml_node table, std::size_t column_index)
    -> std::optional<std::uint32_t> {
    if (table == pugi::xml_node{}) {
        return std::nullopt;
    }

    const auto grid_column = find_table_grid_column(table, column_index);
    if (grid_column == pugi::xml_node{} || grid_column.attribute("w:w") == pugi::xml_attribute{}) {
        return std::nullopt;
    }

    return parse_unsigned_attribute(grid_column, "w:w");
}

auto summed_grid_width_twips(pugi::xml_node table, std::size_t column_index,
                             std::size_t column_span) -> std::optional<std::uint32_t> {
    if (table == pugi::xml_node{} || column_span == 0U) {
        return std::nullopt;
    }

    std::uint64_t total_width = 0U;
    for (std::size_t offset = 0U; offset < column_span; ++offset) {
        const auto column_width = grid_column_width_twips(table, column_index + offset);
        if (!column_width.has_value()) {
            return std::nullopt;
        }

        total_width += *column_width;
        if (total_width > std::numeric_limits<std::uint32_t>::max()) {
            return std::nullopt;
        }
    }

    return static_cast<std::uint32_t>(total_width);
}

void synchronize_fixed_layout_cell_widths_from_grid(pugi::xml_node table) {
    if (table == pugi::xml_node{} || !table_uses_fixed_layout(table)) {
        return;
    }

    const auto column_count = current_table_column_count(table);
    if (column_count == 0U) {
        return;
    }

    ensure_table_grid_columns(table, column_count);
    for (std::size_t column_index = 0U; column_index < column_count; ++column_index) {
        if (!grid_column_width_twips(table, column_index).has_value()) {
            return;
        }
    }

    for (auto row = table.child("w:tr"); row != pugi::xml_node{};
         row = detail::next_named_sibling(row, "w:tr")) {
        std::size_t column_index = 0U;
        for (auto cell = row.child("w:tc"); cell != pugi::xml_node{};
             cell = detail::next_named_sibling(cell, "w:tc")) {
            const auto column_span = cell_column_span(cell);
            const auto cell_width = summed_grid_width_twips(table, column_index, column_span);
            if (cell_width.has_value()) {
                const auto width_node = ensure_cell_width_node(cell);
                if (width_node != pugi::xml_node{}) {
                    const auto width_text = std::to_string(*cell_width);
                    ensure_attribute_value(width_node, "w:w", width_text.c_str());
                    ensure_attribute_value(width_node, "w:type", "dxa");
                }
            }

            column_index += column_span;
        }
    }
}

void clear_fixed_layout_cell_widths_covering_column(pugi::xml_node table,
                                                    std::size_t target_column_index) {
    if (table == pugi::xml_node{} || !table_uses_fixed_layout(table)) {
        return;
    }

    for (auto row = table.child("w:tr"); row != pugi::xml_node{};
         row = detail::next_named_sibling(row, "w:tr")) {
        std::size_t column_index = 0U;
        for (auto cell = row.child("w:tc"); cell != pugi::xml_node{};
             cell = detail::next_named_sibling(cell, "w:tc")) {
            const auto column_span = cell_column_span(cell);
            const auto next_column_index = column_index + column_span;
            if (target_column_index >= column_index && target_column_index < next_column_index) {
                clear_cell_width_node(cell);
            }

            column_index = next_column_index;
        }
    }
}

auto find_row_cell_at_columns(pugi::xml_node row, std::size_t target_column_index,
                              std::size_t target_column_span) -> pugi::xml_node {
    if (row == pugi::xml_node{}) {
        return {};
    }

    std::size_t column_index = 0U;
    for (auto cell = row.child("w:tc"); cell != pugi::xml_node{};
         cell = detail::next_named_sibling(cell, "w:tc")) {
        const auto span = cell_column_span(cell);
        if (column_index == target_column_index && span == target_column_span) {
            return cell;
        }

        column_index += span;
        if (column_index > target_column_index) {
            return {};
        }
    }

    return {};
}

struct row_cell_cover_result final {
    pugi::xml_node cell;
    std::size_t start_column{};
    std::size_t span{};
};

auto find_row_cell_covering_column(pugi::xml_node row, std::size_t target_column_index)
    -> row_cell_cover_result {
    if (row == pugi::xml_node{}) {
        return {};
    }

    std::size_t column_index = 0U;
    for (auto cell = row.child("w:tc"); cell != pugi::xml_node{};
         cell = detail::next_named_sibling(cell, "w:tc")) {
        const auto span = cell_column_span(cell);
        if (target_column_index >= column_index &&
            target_column_index - column_index < span) {
            return {cell, column_index, span};
        }

        column_index += span;
    }

    return {};
}

struct table_column_removal_target final {
    pugi::xml_node row;
    pugi::xml_node cell;
};

struct table_column_removal_plan final {
    std::size_t column_index{};
    std::vector<table_column_removal_target> targets;
};

struct table_column_insertion_target final {
    pugi::xml_node row;
    pugi::xml_node clone_source;
    pugi::xml_node insert_before;
};

struct table_column_insertion_plan final {
    std::size_t boundary_column_index{};
    std::size_t column_count_before_insertion{};
    std::size_t grid_width_source_column_index{};
    std::vector<table_column_insertion_target> targets;
};

struct vertical_merge_chain_plan final {
    pugi::xml_node anchor_cell;
    std::vector<pugi::xml_node> cells;
};

auto plan_table_column_removal(pugi::xml_node cell) -> std::optional<table_column_removal_plan> {
    if (cell == pugi::xml_node{}) {
        return std::nullopt;
    }

    const auto row = cell.parent();
    const auto table = row.parent();
    if (row == pugi::xml_node{} || table == pugi::xml_node{}) {
        return std::nullopt;
    }

    if (cell_column_span(cell) != 1U) {
        return std::nullopt;
    }

    const auto column_index = cell_column_index(cell);
    if (!column_index.has_value()) {
        return std::nullopt;
    }

    if (current_table_column_count(table) <= 1U) {
        return std::nullopt;
    }

    auto plan = table_column_removal_plan{};
    plan.column_index = *column_index;

    for (auto row_cursor = table.child("w:tr"); row_cursor != pugi::xml_node{};
         row_cursor = detail::next_named_sibling(row_cursor, "w:tr")) {
        if (count_named_children(row_cursor, "w:tc") <= 1U) {
            return std::nullopt;
        }

        const auto match = find_row_cell_covering_column(row_cursor, *column_index);
        if (match.cell == pugi::xml_node{} || match.span != 1U) {
            return std::nullopt;
        }

        plan.targets.push_back({row_cursor, match.cell});
    }

    if (plan.targets.empty()) {
        return std::nullopt;
    }

    return plan;
}

struct row_column_insertion_result final {
    pugi::xml_node clone_source;
    pugi::xml_node insert_before;
};

auto find_row_column_insertion_target(pugi::xml_node row, std::size_t boundary_column_index,
                                      bool insert_after)
    -> std::optional<row_column_insertion_result> {
    if (row == pugi::xml_node{}) {
        return std::nullopt;
    }

    auto previous_cell = pugi::xml_node{};
    std::size_t column_index = 0U;
    for (auto cell = row.child("w:tc"); cell != pugi::xml_node{};
         cell = detail::next_named_sibling(cell, "w:tc")) {
        const auto span = cell_column_span(cell);
        const auto cell_end = column_index + span;

        if (boundary_column_index == column_index) {
            const auto clone_source = insert_after ? previous_cell : cell;
            if (clone_source == pugi::xml_node{}) {
                return std::nullopt;
            }
            return row_column_insertion_result{clone_source, cell};
        }

        if (boundary_column_index > column_index && boundary_column_index < cell_end) {
            return std::nullopt;
        }

        previous_cell = cell;
        column_index = cell_end;
        if (insert_after && boundary_column_index == column_index) {
            return row_column_insertion_result{
                cell, detail::next_named_sibling(cell, "w:tc")};
        }
    }

    return std::nullopt;
}

auto plan_table_column_insertion(pugi::xml_node cell, bool insert_after)
    -> std::optional<table_column_insertion_plan> {
    if (cell == pugi::xml_node{}) {
        return std::nullopt;
    }

    const auto row = cell.parent();
    const auto table = row.parent();
    if (row == pugi::xml_node{} || table == pugi::xml_node{}) {
        return std::nullopt;
    }

    const auto column_index = cell_column_index(cell);
    if (!column_index.has_value()) {
        return std::nullopt;
    }

    const auto boundary_column_index =
        *column_index + (insert_after ? cell_column_span(cell) : 0U);
    auto plan = table_column_insertion_plan{};
    plan.boundary_column_index = boundary_column_index;
    plan.column_count_before_insertion = current_table_column_count(table);
    if (plan.column_count_before_insertion == 0U) {
        return std::nullopt;
    }
    plan.grid_width_source_column_index =
        insert_after ? boundary_column_index - 1U : *column_index;
    if (plan.grid_width_source_column_index >= plan.column_count_before_insertion) {
        return std::nullopt;
    }

    for (auto row_cursor = table.child("w:tr"); row_cursor != pugi::xml_node{};
         row_cursor = detail::next_named_sibling(row_cursor, "w:tr")) {
        const auto row_target =
            find_row_column_insertion_target(row_cursor, boundary_column_index, insert_after);
        if (!row_target.has_value()) {
            return std::nullopt;
        }

        plan.targets.push_back({row_cursor, row_target->clone_source, row_target->insert_before});
    }

    if (plan.targets.empty()) {
        return std::nullopt;
    }

    return plan;
}

auto plan_vertical_merge_chain(pugi::xml_node cell) -> std::optional<vertical_merge_chain_plan> {
    if (cell == pugi::xml_node{}) {
        return std::nullopt;
    }

    const auto merge_state = cell_vertical_merge_state_for(cell);
    if (merge_state == cell_vertical_merge_state::none) {
        return std::nullopt;
    }

    const auto row = cell.parent();
    if (row == pugi::xml_node{}) {
        return std::nullopt;
    }

    const auto column_index = cell_column_index(cell);
    if (!column_index.has_value()) {
        return std::nullopt;
    }

    const auto column_span = cell_column_span(cell);
    auto anchor_row = row;
    auto anchor_cell = cell;
    if (merge_state == cell_vertical_merge_state::continue_merge) {
        while (true) {
            const auto previous_row = detail::previous_named_sibling(anchor_row, "w:tr");
            if (previous_row == pugi::xml_node{}) {
                return std::nullopt;
            }

            const auto previous_cell =
                find_row_cell_at_columns(previous_row, *column_index, column_span);
            if (previous_cell == pugi::xml_node{}) {
                return std::nullopt;
            }

            const auto previous_state = cell_vertical_merge_state_for(previous_cell);
            if (previous_state == cell_vertical_merge_state::restart) {
                anchor_row = previous_row;
                anchor_cell = previous_cell;
                break;
            }

            if (previous_state != cell_vertical_merge_state::continue_merge) {
                return std::nullopt;
            }

            anchor_row = previous_row;
            anchor_cell = previous_cell;
        }
    }

    auto plan = vertical_merge_chain_plan{};
    plan.anchor_cell = anchor_cell;
    plan.cells.push_back(anchor_cell);

    auto row_cursor = anchor_row;
    while (true) {
        const auto next_row = detail::next_named_sibling(row_cursor, "w:tr");
        if (next_row == pugi::xml_node{}) {
            break;
        }

        const auto next_cell = find_row_cell_at_columns(next_row, *column_index, column_span);
        if (next_cell == pugi::xml_node{} ||
            cell_vertical_merge_state_for(next_cell) !=
                cell_vertical_merge_state::continue_merge) {
            break;
        }

        plan.cells.push_back(next_cell);
        row_cursor = next_row;
    }

    return plan;
}

bool remove_table_grid_column(pugi::xml_node table, std::size_t target_column_index) {
    if (table == pugi::xml_node{}) {
        return false;
    }

    const auto column_count = current_table_column_count(table);
    if (column_count == 0U || target_column_index >= column_count) {
        return false;
    }

    ensure_table_grid_columns(table, column_count);
    auto table_grid = table.child("w:tblGrid");
    if (table_grid == pugi::xml_node{}) {
        return false;
    }

    auto grid_column = table_grid.child("w:gridCol");
    for (std::size_t index = 0U; index < target_column_index && grid_column != pugi::xml_node{};
         ++index) {
        grid_column = detail::next_named_sibling(grid_column, "w:gridCol");
    }

    return grid_column != pugi::xml_node{} && table_grid.remove_child(grid_column);
}

bool insert_table_grid_column(pugi::xml_node table, std::size_t boundary_column_index,
                              std::size_t column_count_before_insertion,
                              std::size_t source_column_index) {
    if (table == pugi::xml_node{}) {
        return false;
    }

    const auto column_count = column_count_before_insertion;
    if (boundary_column_index > column_count) {
        return false;
    }

    ensure_table_grid_columns(table, column_count);
    auto table_grid = table.child("w:tblGrid");
    if (table_grid == pugi::xml_node{}) {
        return false;
    }

    if (column_count == 0U) {
        auto grid_column = table_grid.append_child("w:gridCol");
        if (grid_column == pugi::xml_node{}) {
            return false;
        }
        ensure_attribute_value(grid_column, "w:w", "0");
        return true;
    }

    if (source_column_index >= column_count) {
        return false;
    }

    const auto source_column = find_table_grid_column(table, source_column_index);
    if (source_column == pugi::xml_node{}) {
        return false;
    }

    auto anchor_column = table_grid.child("w:gridCol");
    for (std::size_t index = 0U; index < boundary_column_index && anchor_column != pugi::xml_node{};
         ++index) {
        anchor_column = detail::next_named_sibling(anchor_column, "w:gridCol");
    }

    if (anchor_column != pugi::xml_node{}) {
        return table_grid.insert_copy_before(source_column, anchor_column) != pugi::xml_node{};
    }

    const auto last_column = find_table_grid_column(table, column_count - 1U);

    return last_column != pugi::xml_node{} &&
           table_grid.insert_copy_after(source_column, last_column) != pugi::xml_node{};
}

void remove_empty_cell_properties(pugi::xml_node cell) {
    if (cell == pugi::xml_node{}) {
        return;
    }

    const auto cell_properties = cell.child("w:tcPr");
    if (cell_properties != pugi::xml_node{} &&
        cell_properties.first_child() == pugi::xml_node{} &&
        cell_properties.first_attribute() == pugi::xml_attribute{}) {
        cell.remove_child(cell_properties);
    }
}

void normalize_inserted_table_cell(pugi::xml_node cell) {
    if (cell == pugi::xml_node{}) {
        return;
    }

    auto cell_properties = cell.child("w:tcPr");
    if (cell_properties != pugi::xml_node{}) {
        if (const auto grid_span = cell_properties.child("w:gridSpan");
            grid_span != pugi::xml_node{}) {
            cell_properties.remove_child(grid_span);
        }
        if (const auto vertical_merge = cell_properties.child("w:vMerge");
            vertical_merge != pugi::xml_node{}) {
            cell_properties.remove_child(vertical_merge);
        }
    }

    remove_empty_cell_properties(cell);
}

auto insert_empty_clone_cell(pugi::xml_node row, pugi::xml_node source_cell,
                             pugi::xml_node insert_before) -> pugi::xml_node {
    if (row == pugi::xml_node{} || source_cell == pugi::xml_node{}) {
        return {};
    }

    auto inserted_cell = pugi::xml_node{};
    if (insert_before != pugi::xml_node{}) {
        inserted_cell = row.insert_copy_before(source_cell, insert_before);
    } else {
        inserted_cell = row.insert_copy_after(source_cell, source_cell);
    }
    if (inserted_cell == pugi::xml_node{}) {
        return {};
    }

    normalize_inserted_table_cell(inserted_cell);
    if (!TableCell(row, inserted_cell).set_text("")) {
        row.remove_child(inserted_cell);
        return {};
    }

    return inserted_cell;
}

void rollback_inserted_table_cells(const std::vector<pugi::xml_node> &inserted_cells) {
    for (auto it = inserted_cells.rbegin(); it != inserted_cells.rend(); ++it) {
        const auto inserted_cell = *it;
        if (inserted_cell != pugi::xml_node{}) {
            auto parent = inserted_cell.parent();
            if (parent != pugi::xml_node{}) {
                parent.remove_child(inserted_cell);
            }
        }
    }
}

void clear_cell_contents_for_vertical_merge(pugi::xml_node cell) {
    if (cell == pugi::xml_node{}) {
        return;
    }

    const auto cell_properties = cell.child("w:tcPr");
    for (auto child = cell.first_child(); child != pugi::xml_node{};) {
        const auto next_child = child.next_sibling();
        if (child != cell_properties) {
            cell.remove_child(child);
        }
        child = next_child;
    }

    if (cell.child("w:p") == pugi::xml_node{}) {
        cell.append_child("w:p");
    }
}

void replace_cell_body_contents(pugi::xml_node target_cell, pugi::xml_node source_cell) {
    if (target_cell == pugi::xml_node{} || source_cell == pugi::xml_node{}) {
        return;
    }

    const auto target_properties = target_cell.child("w:tcPr");
    for (auto child = target_cell.first_child(); child != pugi::xml_node{};) {
        const auto next_child = child.next_sibling();
        if (child != target_properties) {
            target_cell.remove_child(child);
        }
        child = next_child;
    }

    const auto source_properties = source_cell.child("w:tcPr");
    for (auto child = source_cell.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        if (child != source_properties) {
            target_cell.append_copy(child);
        }
    }

    if (target_cell.child("w:p") == pugi::xml_node{}) {
        target_cell.append_child("w:p");
    }
}

auto successor_vertical_merge_promotions_for_row_removal(pugi::xml_node row)
    -> std::vector<std::pair<pugi::xml_node, pugi::xml_node>> {
    std::vector<std::pair<pugi::xml_node, pugi::xml_node>> promotions;
    if (row == pugi::xml_node{}) {
        return promotions;
    }

    const auto next_row = detail::next_named_sibling(row, "w:tr");
    if (next_row == pugi::xml_node{}) {
        return promotions;
    }

    for (auto cell = row.child("w:tc"); cell != pugi::xml_node{};
         cell = detail::next_named_sibling(cell, "w:tc")) {
        if (cell_vertical_merge_state_for(cell) != cell_vertical_merge_state::restart) {
            continue;
        }

        const auto column_index = cell_column_index(cell);
        if (!column_index.has_value()) {
            promotions.clear();
            return promotions;
        }

        const auto next_cell =
            find_row_cell_at_columns(next_row, *column_index, cell_column_span(cell));
        if (next_cell == pugi::xml_node{} ||
            cell_vertical_merge_state_for(next_cell) !=
                cell_vertical_merge_state::continue_merge) {
            continue;
        }

        promotions.emplace_back(cell, next_cell);
    }

    return promotions;
}

#include "table_xml_conversion_helpers.inc"

void remove_empty_container(pugi::xml_node properties, const char *container_name) {
    if (properties == pugi::xml_node{}) {
        return;
    }

    const auto container = properties.child(container_name);
    if (container != pugi::xml_node{} && container.first_child() == pugi::xml_node{} &&
        container.first_attribute() == pugi::xml_attribute{}) {
        properties.remove_child(container);
    }
}

auto append_cell_node(pugi::xml_node row) -> pugi::xml_node {
    if (row == pugi::xml_node{}) {
        return {};
    }

    auto cell = row.append_child("w:tc");
    ensure_default_cell_properties(cell);
    cell.append_child("w:p");
    return cell;
}

auto append_row_node(pugi::xml_node table, std::size_t cell_count) -> pugi::xml_node {
    if (table == pugi::xml_node{}) {
        return {};
    }

    ensure_table_grid_columns(table,
                              std::max(current_table_column_count(table), cell_count));

    auto row = table.append_child("w:tr");
    for (std::size_t i = 0; i < cell_count; ++i) {
        append_cell_node(row);
    }
    return row;
}

} // namespace

#include "table_cell_methods.inc"

#include "table_row_methods.inc"

#include "table_methods.inc"

} // namespace featherdoc
