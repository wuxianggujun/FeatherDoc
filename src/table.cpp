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
        if (target_column_index < column_index + span) {
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

auto to_xml_border_style(featherdoc::border_style style) -> const char * {
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

auto to_xml_table_layout_mode(featherdoc::table_layout_mode layout_mode) -> const char * {
    switch (layout_mode) {
    case featherdoc::table_layout_mode::autofit:
        return "autofit";
    case featherdoc::table_layout_mode::fixed:
        return "fixed";
    }

    return "autofit";
}

auto to_xml_table_alignment(featherdoc::table_alignment alignment) -> const char * {
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

auto to_xml_table_position_horizontal_reference(
    featherdoc::table_position_horizontal_reference reference) -> const char * {
    switch (reference) {
    case featherdoc::table_position_horizontal_reference::margin:
        return "margin";
    case featherdoc::table_position_horizontal_reference::page:
        return "page";
    case featherdoc::table_position_horizontal_reference::column:
        return "text";
    }

    return "margin";
}

auto to_xml_table_position_vertical_reference(
    featherdoc::table_position_vertical_reference reference) -> const char * {
    switch (reference) {
    case featherdoc::table_position_vertical_reference::margin:
        return "margin";
    case featherdoc::table_position_vertical_reference::page:
        return "page";
    case featherdoc::table_position_vertical_reference::paragraph:
        return "text";
    }

    return "text";
}

auto to_xml_table_overlap(featherdoc::table_overlap overlap) -> const char * {
    switch (overlap) {
    case featherdoc::table_overlap::allow:
        return "overlap";
    case featherdoc::table_overlap::never:
        return "never";
    }

    return "overlap";
}

auto to_xml_row_height_rule(featherdoc::row_height_rule height_rule) -> const char * {
    switch (height_rule) {
    case featherdoc::row_height_rule::automatic:
        return "auto";
    case featherdoc::row_height_rule::at_least:
        return "atLeast";
    case featherdoc::row_height_rule::exact:
        return "exact";
    }

    return "auto";
}

auto parse_table_layout_mode(std::string_view layout_type)
    -> std::optional<featherdoc::table_layout_mode> {
    if (layout_type == "autofit") {
        return featherdoc::table_layout_mode::autofit;
    }

    if (layout_type == "fixed") {
        return featherdoc::table_layout_mode::fixed;
    }

    return std::nullopt;
}

auto parse_table_position_horizontal_reference(std::string_view reference)
    -> std::optional<featherdoc::table_position_horizontal_reference> {
    if (reference == "margin") {
        return featherdoc::table_position_horizontal_reference::margin;
    }

    if (reference == "page") {
        return featherdoc::table_position_horizontal_reference::page;
    }

    if (reference == "text" || reference == "column") {
        return featherdoc::table_position_horizontal_reference::column;
    }

    return std::nullopt;
}

auto parse_table_position_vertical_reference(std::string_view reference)
    -> std::optional<featherdoc::table_position_vertical_reference> {
    if (reference == "margin") {
        return featherdoc::table_position_vertical_reference::margin;
    }

    if (reference == "page") {
        return featherdoc::table_position_vertical_reference::page;
    }

    if (reference == "text" || reference == "paragraph") {
        return featherdoc::table_position_vertical_reference::paragraph;
    }

    return std::nullopt;
}

auto parse_table_overlap(std::string_view overlap)
    -> std::optional<featherdoc::table_overlap> {
    if (overlap == "overlap" || overlap == "allow") {
        return featherdoc::table_overlap::allow;
    }

    if (overlap == "never") {
        return featherdoc::table_overlap::never;
    }

    return std::nullopt;
}

auto parse_table_alignment(std::string_view alignment)
    -> std::optional<featherdoc::table_alignment> {
    if (alignment == "left" || alignment == "start") {
        return featherdoc::table_alignment::left;
    }

    if (alignment == "center") {
        return featherdoc::table_alignment::center;
    }

    if (alignment == "right" || alignment == "end") {
        return featherdoc::table_alignment::right;
    }

    return std::nullopt;
}

auto parse_row_height_rule(std::string_view height_rule)
    -> std::optional<featherdoc::row_height_rule> {
    if (height_rule == "auto") {
        return featherdoc::row_height_rule::automatic;
    }

    if (height_rule == "atLeast") {
        return featherdoc::row_height_rule::at_least;
    }

    if (height_rule == "exact") {
        return featherdoc::row_height_rule::exact;
    }

    return std::nullopt;
}

auto to_xml_cell_vertical_alignment(featherdoc::cell_vertical_alignment alignment)
    -> const char * {
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

auto parse_cell_vertical_alignment(std::string_view alignment)
    -> std::optional<featherdoc::cell_vertical_alignment> {
    if (alignment == "top") {
        return featherdoc::cell_vertical_alignment::top;
    }

    if (alignment == "center") {
        return featherdoc::cell_vertical_alignment::center;
    }

    if (alignment == "bottom") {
        return featherdoc::cell_vertical_alignment::bottom;
    }

    if (alignment == "both") {
        return featherdoc::cell_vertical_alignment::both;
    }

    return std::nullopt;
}

auto to_xml_cell_text_direction(featherdoc::cell_text_direction direction) -> const char * {
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

auto parse_cell_text_direction(std::string_view direction)
    -> std::optional<featherdoc::cell_text_direction> {
    if (direction == "lrTb") {
        return featherdoc::cell_text_direction::left_to_right_top_to_bottom;
    }

    if (direction == "tbRl") {
        return featherdoc::cell_text_direction::top_to_bottom_right_to_left;
    }

    if (direction == "btLr") {
        return featherdoc::cell_text_direction::bottom_to_top_left_to_right;
    }

    if (direction == "lrTbV") {
        return featherdoc::cell_text_direction::left_to_right_top_to_bottom_rotated;
    }

    if (direction == "tbRlV") {
        return featherdoc::cell_text_direction::top_to_bottom_right_to_left_rotated;
    }

    if (direction == "tbLrV") {
        return featherdoc::cell_text_direction::top_to_bottom_left_to_right_rotated;
    }

    return std::nullopt;
}

auto to_xml_border_name(featherdoc::cell_border_edge edge) -> const char * {
    switch (edge) {
    case featherdoc::cell_border_edge::top:
        return "w:top";
    case featherdoc::cell_border_edge::left:
        return "w:left";
    case featherdoc::cell_border_edge::bottom:
        return "w:bottom";
    case featherdoc::cell_border_edge::right:
        return "w:right";
    }

    return "w:top";
}

auto to_xml_border_name(featherdoc::table_border_edge edge) -> const char * {
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

auto to_xml_margin_name(featherdoc::cell_margin_edge edge) -> const char * {
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

void apply_border_definition(pugi::xml_node border_node,
                             featherdoc::border_definition border) {
    if (border_node == pugi::xml_node{}) {
        return;
    }

    const auto size_text = std::to_string(border.size_eighth_points);
    const auto space_text = std::to_string(border.space_points);
    const auto color_text =
        border.color.empty() ? std::string{"auto"} : std::string{border.color};

    ensure_attribute_value(border_node, "w:val", to_xml_border_style(border.style));
    ensure_attribute_value(border_node, "w:sz", size_text.c_str());
    ensure_attribute_value(border_node, "w:space", space_text.c_str());
    ensure_attribute_value(border_node, "w:color", color_text.c_str());
}

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

TableCell::TableCell() = default;

TableCell::TableCell(pugi::xml_node parent, pugi::xml_node current) {
    this->set_parent(parent);
    this->set_current(current);
}

void TableCell::set_parent(pugi::xml_node node) {
    this->parent = node;
    this->current = this->parent.child("w:tc");
    this->paragraph.set_parent(this->current);
}

void TableCell::set_current(pugi::xml_node node) {
    this->current = node;
    this->paragraph.set_parent(this->current);
}

Paragraph &TableCell::paragraphs() {
    this->paragraph.set_parent(this->current);
    return this->paragraph;
}

std::string TableCell::get_text() const {
    if (this->current == pugi::xml_node{}) {
        return {};
    }

    std::string text;
    bool first_paragraph = true;
    for (auto paragraph_node = this->current.child("w:p"); paragraph_node != pugi::xml_node{};
         paragraph_node = detail::next_named_sibling(paragraph_node, "w:p")) {
        if (!first_paragraph) {
            text.push_back('\n');
        }
        first_paragraph = false;
        text += detail::collect_plain_text_from_xml(paragraph_node);
    }

    return text;
}

bool TableCell::set_text(const std::string &text) const { return this->set_text(text.c_str()); }

bool TableCell::set_text(const char *text) const {
    if (this->current == pugi::xml_node{} || text == nullptr) {
        return false;
    }

    auto cell_node = this->current;
    for (auto child = cell_node.first_child(); child != pugi::xml_node{};) {
        const auto next_child = child.next_sibling();
        if (std::string_view{child.name()} != "w:tcPr") {
            cell_node.remove_child(child);
        }
        child = next_child;
    }

    return detail::append_plain_text_paragraph(cell_node, text);
}

bool TableCell::remove() {
    if (this->current == pugi::xml_node{} || this->parent == pugi::xml_node{}) {
        return false;
    }

    const auto table = this->parent.parent();
    if (table == pugi::xml_node{}) {
        return false;
    }

    auto removal_plan = plan_table_column_removal(this->current);
    if (!removal_plan.has_value()) {
        return false;
    }

    const auto next_cell = detail::next_named_sibling(this->current, "w:tc");
    const auto previous_cell = detail::previous_named_sibling(this->current, "w:tc");
    const auto surviving_cell =
        next_cell != pugi::xml_node{} ? next_cell : previous_cell;
    if (surviving_cell == pugi::xml_node{}) {
        return false;
    }

    for (auto &target : removal_plan->targets) {
        if (target.cell == this->current) {
            continue;
        }
        if (!target.row.remove_child(target.cell)) {
            return false;
        }
    }

    if (!remove_table_grid_column(table, removal_plan->column_index)) {
        return false;
    }

    if (!this->parent.remove_child(this->current)) {
        return false;
    }

    synchronize_fixed_layout_cell_widths_from_grid(table);
    this->set_current(surviving_cell);
    return true;
}

TableCell TableCell::insert_cell_before() {
    if (this->current == pugi::xml_node{} || this->parent == pugi::xml_node{}) {
        return {};
    }

    const auto table = this->parent.parent();
    if (table == pugi::xml_node{}) {
        return {};
    }

    const auto insertion_plan = plan_table_column_insertion(this->current, false);
    if (!insertion_plan.has_value()) {
        return {};
    }

    pugi::xml_node inserted_current_row_cell;
    auto inserted_cells = std::vector<pugi::xml_node>{};
    inserted_cells.reserve(insertion_plan->targets.size());
    for (const auto &target : insertion_plan->targets) {
        const auto inserted_cell =
            insert_empty_clone_cell(target.row, target.clone_source, target.insert_before);
        if (inserted_cell == pugi::xml_node{}) {
            rollback_inserted_table_cells(inserted_cells);
            return {};
        }
        inserted_cells.push_back(inserted_cell);
        if (target.row == this->parent) {
            inserted_current_row_cell = inserted_cell;
        }
    }

    if (inserted_current_row_cell == pugi::xml_node{} ||
        !insert_table_grid_column(table, insertion_plan->boundary_column_index,
                                  insertion_plan->column_count_before_insertion,
                                  insertion_plan->grid_width_source_column_index)) {
        rollback_inserted_table_cells(inserted_cells);
        return {};
    }

    synchronize_fixed_layout_cell_widths_from_grid(table);
    this->set_current(inserted_current_row_cell);
    return TableCell(this->parent, inserted_current_row_cell);
}

TableCell TableCell::insert_cell_after() {
    if (this->current == pugi::xml_node{} || this->parent == pugi::xml_node{}) {
        return {};
    }

    const auto table = this->parent.parent();
    if (table == pugi::xml_node{}) {
        return {};
    }

    const auto insertion_plan = plan_table_column_insertion(this->current, true);
    if (!insertion_plan.has_value()) {
        return {};
    }

    pugi::xml_node inserted_current_row_cell;
    auto inserted_cells = std::vector<pugi::xml_node>{};
    inserted_cells.reserve(insertion_plan->targets.size());
    for (const auto &target : insertion_plan->targets) {
        const auto inserted_cell =
            insert_empty_clone_cell(target.row, target.clone_source, target.insert_before);
        if (inserted_cell == pugi::xml_node{}) {
            rollback_inserted_table_cells(inserted_cells);
            return {};
        }
        inserted_cells.push_back(inserted_cell);
        if (target.row == this->parent) {
            inserted_current_row_cell = inserted_cell;
        }
    }

    if (inserted_current_row_cell == pugi::xml_node{} ||
        !insert_table_grid_column(table, insertion_plan->boundary_column_index,
                                  insertion_plan->column_count_before_insertion,
                                  insertion_plan->grid_width_source_column_index)) {
        rollback_inserted_table_cells(inserted_cells);
        return {};
    }

    synchronize_fixed_layout_cell_widths_from_grid(table);
    this->set_current(inserted_current_row_cell);
    return TableCell(this->parent, inserted_current_row_cell);
}

std::optional<std::uint32_t> TableCell::width_twips() const {
    if (this->current == pugi::xml_node{}) {
        return std::nullopt;
    }

    const auto width_node = this->current.child("w:tcPr").child("w:tcW");
    if (width_node == pugi::xml_node{} ||
        std::string_view{width_node.attribute("w:type").value()} != "dxa") {
        return std::nullopt;
    }

    return parse_unsigned_attribute(width_node, "w:w");
}

bool TableCell::set_width_twips(std::uint32_t width_twips) {
    if (this->current == pugi::xml_node{}) {
        return false;
    }

    const auto width_node = ensure_cell_width_node(this->current);
    if (width_node == pugi::xml_node{}) {
        return false;
    }

    const auto width_text = std::to_string(width_twips);
    ensure_attribute_value(width_node, "w:w", width_text.c_str());
    ensure_attribute_value(width_node, "w:type", "dxa");
    return true;
}

bool TableCell::clear_width() {
    if (this->current == pugi::xml_node{}) {
        return false;
    }

    auto cell_properties = this->current.child("w:tcPr");
    if (cell_properties == pugi::xml_node{}) {
        return true;
    }

    const auto width_node = cell_properties.child("w:tcW");
    return width_node == pugi::xml_node{} || cell_properties.remove_child(width_node);
}

std::size_t TableCell::column_span() const { return cell_column_span(this->current); }

bool TableCell::merge_right(std::size_t additional_cells) {
    if (this->current == pugi::xml_node{} || this->parent == pugi::xml_node{}) {
        return false;
    }

    const auto table = this->parent.parent();
    if (table == pugi::xml_node{}) {
        return false;
    }

    if (additional_cells == 0U) {
        return true;
    }

    std::vector<pugi::xml_node> cells_to_remove;
    cells_to_remove.reserve(additional_cells);

    std::size_t added_span = 0U;
    auto next_cell = detail::next_named_sibling(this->current, "w:tc");
    for (std::size_t i = 0; i < additional_cells; ++i) {
        if (next_cell == pugi::xml_node{}) {
            return false;
        }

        added_span += cell_column_span(next_cell);
        cells_to_remove.push_back(next_cell);
        next_cell = detail::next_named_sibling(next_cell, "w:tc");
    }

    const auto grid_span_node = ensure_cell_grid_span_node(this->current);
    if (grid_span_node == pugi::xml_node{}) {
        return false;
    }

    const auto merged_span = std::to_string(this->column_span() + added_span);
    ensure_attribute_value(grid_span_node, "w:val", merged_span.c_str());

    for (const auto cell : cells_to_remove) {
        if (!this->parent.remove_child(cell)) {
            return false;
        }
    }

    synchronize_fixed_layout_cell_widths_from_grid(table);
    return true;
}

bool TableCell::merge_down(std::size_t additional_rows) {
    if (this->current == pugi::xml_node{} || this->parent == pugi::xml_node{}) {
        return false;
    }

    if (additional_rows == 0U) {
        return true;
    }

    const auto current_merge_state = cell_vertical_merge_state_for(this->current);
    if (current_merge_state == cell_vertical_merge_state::continue_merge) {
        return false;
    }

    const auto column_index = cell_column_index(this->current);
    if (!column_index.has_value()) {
        return false;
    }

    const auto column_span = cell_column_span(this->current);
    auto anchor_row = this->parent;
    if (current_merge_state == cell_vertical_merge_state::restart) {
        while (true) {
            const auto next_row = detail::next_named_sibling(anchor_row, "w:tr");
            if (next_row == pugi::xml_node{}) {
                break;
            }

            const auto continued_cell =
                find_row_cell_at_columns(next_row, *column_index, column_span);
            if (continued_cell == pugi::xml_node{} ||
                cell_vertical_merge_state_for(continued_cell) !=
                    cell_vertical_merge_state::continue_merge) {
                break;
            }

            anchor_row = next_row;
        }
    }

    std::vector<pugi::xml_node> target_cells;
    target_cells.reserve(additional_rows);

    auto row_cursor = anchor_row;
    for (std::size_t i = 0; i < additional_rows; ++i) {
        row_cursor = detail::next_named_sibling(row_cursor, "w:tr");
        if (row_cursor == pugi::xml_node{}) {
            return false;
        }

        const auto target_cell =
            find_row_cell_at_columns(row_cursor, *column_index, column_span);
        if (target_cell == pugi::xml_node{} ||
            cell_vertical_merge_state_for(target_cell) != cell_vertical_merge_state::none) {
            return false;
        }

        target_cells.push_back(target_cell);
    }

    const auto vertical_merge_node = ensure_cell_vertical_merge_node(this->current);
    if (vertical_merge_node == pugi::xml_node{}) {
        return false;
    }

    std::vector<pugi::xml_node> target_merge_nodes;
    target_merge_nodes.reserve(target_cells.size());
    for (const auto target_cell : target_cells) {
        const auto target_merge_node = ensure_cell_vertical_merge_node(target_cell);
        if (target_merge_node == pugi::xml_node{}) {
            return false;
        }
        target_merge_nodes.push_back(target_merge_node);
    }

    ensure_attribute_value(vertical_merge_node, "w:val", "restart");
    for (std::size_t i = 0; i < target_merge_nodes.size(); ++i) {
        ensure_attribute_value(target_merge_nodes[i], "w:val", "continue");
        clear_cell_contents_for_vertical_merge(target_cells[i]);
    }

    return true;
}

bool TableCell::unmerge_right() {
    if (this->current == pugi::xml_node{} || this->parent == pugi::xml_node{}) {
        return false;
    }

    const auto table = this->parent.parent();
    if (table == pugi::xml_node{}) {
        return false;
    }

    if (cell_vertical_merge_state_for(this->current) != cell_vertical_merge_state::none) {
        return false;
    }

    const auto span = cell_column_span(this->current);
    if (span <= 1U) {
        return false;
    }

    const auto insert_before = detail::next_named_sibling(this->current, "w:tc");
    auto inserted_cells = std::vector<pugi::xml_node>{};
    inserted_cells.reserve(span - 1U);
    for (std::size_t i = 0U; i + 1U < span; ++i) {
        const auto inserted_cell =
            insert_empty_clone_cell(this->parent, this->current, insert_before);
        if (inserted_cell == pugi::xml_node{}) {
            rollback_inserted_table_cells(inserted_cells);
            return false;
        }
        inserted_cells.push_back(inserted_cell);
    }

    auto cell_properties = this->current.child("w:tcPr");
    if (cell_properties == pugi::xml_node{}) {
        rollback_inserted_table_cells(inserted_cells);
        return false;
    }

    const auto grid_span = cell_properties.child("w:gridSpan");
    if (grid_span == pugi::xml_node{} || !cell_properties.remove_child(grid_span)) {
        rollback_inserted_table_cells(inserted_cells);
        return false;
    }

    remove_empty_cell_properties(this->current);
    synchronize_fixed_layout_cell_widths_from_grid(table);
    return true;
}

bool TableCell::unmerge_down() {
    if (this->current == pugi::xml_node{}) {
        return false;
    }

    const auto merge_chain = plan_vertical_merge_chain(this->current);
    if (!merge_chain.has_value()) {
        return false;
    }

    for (const auto cell : merge_chain->cells) {
        auto cell_properties = cell.child("w:tcPr");
        if (cell_properties == pugi::xml_node{}) {
            return false;
        }

        const auto vertical_merge = cell_properties.child("w:vMerge");
        if (vertical_merge == pugi::xml_node{} || !cell_properties.remove_child(vertical_merge)) {
            return false;
        }

        remove_empty_cell_properties(cell);
    }

    return true;
}

std::optional<featherdoc::cell_vertical_alignment> TableCell::vertical_alignment() const {
    if (this->current == pugi::xml_node{}) {
        return std::nullopt;
    }

    const auto vertical_alignment = this->current.child("w:tcPr").child("w:vAlign");
    const auto alignment_text =
        std::string_view{vertical_alignment.attribute("w:val").value()};
    if (alignment_text.empty()) {
        return std::nullopt;
    }

    return parse_cell_vertical_alignment(alignment_text);
}

bool TableCell::set_vertical_alignment(featherdoc::cell_vertical_alignment alignment) {
    if (this->current == pugi::xml_node{}) {
        return false;
    }

    const auto vertical_alignment = ensure_cell_vertical_alignment_node(this->current);
    if (vertical_alignment == pugi::xml_node{}) {
        return false;
    }

    ensure_attribute_value(vertical_alignment, "w:val",
                           to_xml_cell_vertical_alignment(alignment));
    return true;
}

bool TableCell::clear_vertical_alignment() {
    if (this->current == pugi::xml_node{}) {
        return false;
    }

    auto cell_properties = this->current.child("w:tcPr");
    if (cell_properties == pugi::xml_node{}) {
        return true;
    }

    const auto vertical_alignment = cell_properties.child("w:vAlign");
    return vertical_alignment == pugi::xml_node{} ||
           cell_properties.remove_child(vertical_alignment);
}

std::optional<featherdoc::cell_text_direction> TableCell::text_direction() const {
    if (this->current == pugi::xml_node{}) {
        return std::nullopt;
    }

    const auto text_direction = this->current.child("w:tcPr").child("w:textDirection");
    const auto direction_text = std::string_view{text_direction.attribute("w:val").value()};
    if (direction_text.empty()) {
        return std::nullopt;
    }

    return parse_cell_text_direction(direction_text);
}

bool TableCell::set_text_direction(featherdoc::cell_text_direction direction) {
    if (this->current == pugi::xml_node{}) {
        return false;
    }

    const auto text_direction = ensure_cell_text_direction_node(this->current);
    if (text_direction == pugi::xml_node{}) {
        return false;
    }

    ensure_attribute_value(text_direction, "w:val", to_xml_cell_text_direction(direction));
    return true;
}

bool TableCell::clear_text_direction() {
    if (this->current == pugi::xml_node{}) {
        return false;
    }

    auto cell_properties = this->current.child("w:tcPr");
    if (cell_properties == pugi::xml_node{}) {
        return true;
    }

    const auto text_direction = cell_properties.child("w:textDirection");
    return text_direction == pugi::xml_node{} || cell_properties.remove_child(text_direction);
}

std::optional<std::string> TableCell::fill_color() const {
    if (this->current == pugi::xml_node{}) {
        return std::nullopt;
    }

    const auto shading = this->current.child("w:tcPr").child("w:shd");
    const auto fill_text = std::string_view{shading.attribute("w:fill").value()};
    if (fill_text.empty()) {
        return std::nullopt;
    }

    return std::string{fill_text};
}

bool TableCell::set_fill_color(std::string_view fill_color) {
    if (this->current == pugi::xml_node{} || fill_color.empty()) {
        return false;
    }

    const auto shading = ensure_cell_shading_node(this->current);
    if (shading == pugi::xml_node{}) {
        return false;
    }

    const auto fill_text = std::string{fill_color};
    ensure_attribute_value(shading, "w:val", "clear");
    ensure_attribute_value(shading, "w:color", "auto");
    ensure_attribute_value(shading, "w:fill", fill_text.c_str());
    return true;
}

bool TableCell::clear_fill_color() {
    if (this->current == pugi::xml_node{}) {
        return false;
    }

    auto cell_properties = this->current.child("w:tcPr");
    if (cell_properties == pugi::xml_node{}) {
        return true;
    }

    const auto shading = cell_properties.child("w:shd");
    return shading == pugi::xml_node{} || cell_properties.remove_child(shading);
}

std::optional<std::uint32_t> TableCell::margin_twips(featherdoc::cell_margin_edge edge) const {
    if (this->current == pugi::xml_node{}) {
        return std::nullopt;
    }

    const auto margin =
        this->current.child("w:tcPr").child("w:tcMar").child(to_xml_margin_name(edge));
    if (margin == pugi::xml_node{}) {
        return std::nullopt;
    }

    if (const auto margin_type = std::string_view{margin.attribute("w:type").value()};
        !margin_type.empty() && margin_type != "dxa") {
        return std::nullopt;
    }

    return parse_unsigned_attribute(margin, "w:w");
}

bool TableCell::set_margin_twips(featherdoc::cell_margin_edge edge, std::uint32_t margin_twips) {
    if (this->current == pugi::xml_node{}) {
        return false;
    }

    const auto margin = ensure_cell_margin_node(this->current, to_xml_margin_name(edge));
    if (margin == pugi::xml_node{}) {
        return false;
    }

    const auto margin_text = std::to_string(margin_twips);
    ensure_attribute_value(margin, "w:w", margin_text.c_str());
    ensure_attribute_value(margin, "w:type", "dxa");
    return true;
}

bool TableCell::clear_margin(featherdoc::cell_margin_edge edge) {
    if (this->current == pugi::xml_node{}) {
        return false;
    }

    auto cell_properties = this->current.child("w:tcPr");
    if (cell_properties == pugi::xml_node{}) {
        return true;
    }

    auto margins = cell_properties.child("w:tcMar");
    if (margins == pugi::xml_node{}) {
        return true;
    }

    if (const auto margin = margins.child(to_xml_margin_name(edge)); margin != pugi::xml_node{}) {
        margins.remove_child(margin);
    }

    remove_empty_container(cell_properties, "w:tcMar");
    return true;
}

bool TableCell::set_border(featherdoc::cell_border_edge edge,
                           featherdoc::border_definition border) {
    if (this->current == pugi::xml_node{}) {
        return false;
    }

    auto cell_borders = ensure_cell_borders_node(this->current);
    if (cell_borders == pugi::xml_node{}) {
        return false;
    }

    const auto border_name = to_xml_border_name(edge);
    auto border_node = cell_borders.child(border_name);
    if (border_node == pugi::xml_node{}) {
        border_node = cell_borders.append_child(border_name);
    }
    if (border_node == pugi::xml_node{}) {
        return false;
    }

    apply_border_definition(border_node, border);
    return true;
}

bool TableCell::clear_border(featherdoc::cell_border_edge edge) {
    if (this->current == pugi::xml_node{}) {
        return false;
    }

    auto cell_properties = this->current.child("w:tcPr");
    if (cell_properties == pugi::xml_node{}) {
        return true;
    }

    auto cell_borders = cell_properties.child("w:tcBorders");
    if (cell_borders == pugi::xml_node{}) {
        return true;
    }

    if (const auto border_node = cell_borders.child(to_xml_border_name(edge));
        border_node != pugi::xml_node{}) {
        cell_borders.remove_child(border_node);
    }

    remove_empty_container(cell_properties, "w:tcBorders");
    return true;
}

TableCell &TableCell::next() {
    this->current = detail::next_named_sibling(this->current, "w:tc");
    return *this;
}

bool TableCell::has_next() const { return this->current != pugi::xml_node{}; }

TableRow::TableRow() = default;

TableRow::TableRow(pugi::xml_node parent, pugi::xml_node current) {
    this->set_parent(parent);
    this->set_current(current);
}

void TableRow::set_parent(pugi::xml_node node) {
    this->parent = node;
    this->current = this->parent.child("w:tr");
    this->cell.set_parent(this->current);
}

void TableRow::set_current(pugi::xml_node node) {
    this->current = node;
    this->cell.set_parent(this->current);
}

TableCell &TableRow::cells() {
    this->cell.set_parent(this->current);
    return this->cell;
}

std::optional<std::uint32_t> TableRow::height_twips() const {
    if (this->current == pugi::xml_node{}) {
        return std::nullopt;
    }

    return parse_unsigned_attribute(this->current.child("w:trPr").child("w:trHeight"),
                                    "w:val");
}

std::optional<featherdoc::row_height_rule> TableRow::height_rule() const {
    if (this->current == pugi::xml_node{}) {
        return std::nullopt;
    }

    const auto row_height = this->current.child("w:trPr").child("w:trHeight");
    const auto height_rule_text = std::string_view{row_height.attribute("w:hRule").value()};
    if (height_rule_text.empty()) {
        return std::nullopt;
    }

    return parse_row_height_rule(height_rule_text);
}

bool TableRow::set_height_twips(std::uint32_t height_twips,
                                featherdoc::row_height_rule height_rule) {
    if (this->current == pugi::xml_node{}) {
        return false;
    }

    const auto row_height = ensure_row_height_node(this->current);
    if (row_height == pugi::xml_node{}) {
        return false;
    }

    const auto height_text = std::to_string(height_twips);
    ensure_attribute_value(row_height, "w:val", height_text.c_str());
    ensure_attribute_value(row_height, "w:hRule", to_xml_row_height_rule(height_rule));
    return true;
}

bool TableRow::clear_height() {
    if (this->current == pugi::xml_node{}) {
        return false;
    }

    auto row_properties = this->current.child("w:trPr");
    if (row_properties == pugi::xml_node{}) {
        return true;
    }

    if (const auto row_height = row_properties.child("w:trHeight");
        row_height != pugi::xml_node{}) {
        row_properties.remove_child(row_height);
    }

    remove_empty_container(this->current, "w:trPr");
    return true;
}

bool TableRow::cant_split() const {
    if (this->current == pugi::xml_node{}) {
        return false;
    }

    return on_off_node_enabled(this->current.child("w:trPr").child("w:cantSplit"));
}

bool TableRow::set_cant_split() {
    if (this->current == pugi::xml_node{}) {
        return false;
    }

    const auto cant_split = ensure_row_cant_split_node(this->current);
    if (cant_split == pugi::xml_node{}) {
        return false;
    }

    ensure_attribute_value(cant_split, "w:val", "1");
    return true;
}

bool TableRow::clear_cant_split() {
    if (this->current == pugi::xml_node{}) {
        return false;
    }

    auto row_properties = this->current.child("w:trPr");
    if (row_properties == pugi::xml_node{}) {
        return true;
    }

    if (const auto cant_split = row_properties.child("w:cantSplit");
        cant_split != pugi::xml_node{}) {
        row_properties.remove_child(cant_split);
    }

    remove_empty_container(this->current, "w:trPr");
    return true;
}

bool TableRow::repeats_header() const {
    if (this->current == pugi::xml_node{}) {
        return false;
    }

    return on_off_node_enabled(this->current.child("w:trPr").child("w:tblHeader"));
}

bool TableRow::set_repeats_header() {
    if (this->current == pugi::xml_node{}) {
        return false;
    }

    const auto table_header = ensure_row_header_node(this->current);
    if (table_header == pugi::xml_node{}) {
        return false;
    }

    ensure_attribute_value(table_header, "w:val", "1");
    return true;
}

bool TableRow::clear_repeats_header() {
    if (this->current == pugi::xml_node{}) {
        return false;
    }

    auto row_properties = this->current.child("w:trPr");
    if (row_properties == pugi::xml_node{}) {
        return true;
    }

    if (const auto table_header = row_properties.child("w:tblHeader");
        table_header != pugi::xml_node{}) {
        row_properties.remove_child(table_header);
    }

    remove_empty_container(this->current, "w:trPr");
    return true;
}

bool TableRow::remove() {
    if (this->parent == pugi::xml_node{} || this->current == pugi::xml_node{}) {
        return false;
    }

    if (count_named_children(this->parent, "w:tr") <= 1U) {
        return false;
    }

    const auto promotions = successor_vertical_merge_promotions_for_row_removal(this->current);
    for (const auto &[source_cell, target_cell] : promotions) {
        const auto target_merge = ensure_cell_vertical_merge_node(target_cell);
        if (target_merge == pugi::xml_node{}) {
            return false;
        }

        ensure_attribute_value(target_merge, "w:val", "restart");
        replace_cell_body_contents(target_cell, source_cell);
    }

    const auto next_row = detail::next_named_sibling(this->current, "w:tr");
    const auto previous_row = detail::previous_named_sibling(this->current, "w:tr");
    if (!this->parent.remove_child(this->current)) {
        return false;
    }

    this->current = next_row != pugi::xml_node{} ? next_row : previous_row;
    this->cell.set_parent(this->current);
    return true;
}

TableRow TableRow::insert_row_before() {
    if (this->parent == pugi::xml_node{} || this->current == pugi::xml_node{}) {
        return {};
    }

    auto inserted_row = insert_empty_clone_row(this->parent, this->current, {}, false);
    if (inserted_row == pugi::xml_node{}) {
        return {};
    }

    this->current = inserted_row;
    this->cell.set_parent(this->current);
    return TableRow(this->parent, inserted_row);
}

TableRow TableRow::insert_row_after() {
    if (this->parent == pugi::xml_node{} || this->current == pugi::xml_node{}) {
        return {};
    }

    const auto next_row = detail::next_named_sibling(this->current, "w:tr");
    auto inserted_row = insert_empty_clone_row(this->parent, this->current, next_row, true);
    if (inserted_row == pugi::xml_node{}) {
        return {};
    }

    this->current = inserted_row;
    this->cell.set_parent(this->current);
    return TableRow(this->parent, inserted_row);
}

TableCell TableRow::append_cell() {
    if (this->current == pugi::xml_node{} && this->parent != pugi::xml_node{}) {
        this->current = this->parent.append_child("w:tr");
    }

    if (this->current == pugi::xml_node{}) {
        return {};
    }

    const auto required_columns =
        std::max(current_table_column_count(this->parent),
                 count_named_children(this->current, "w:tc") + 1U);
    ensure_table_grid_columns(this->parent, required_columns);

    const auto new_cell = append_cell_node(this->current);
    this->cell.set_parent(this->current);
    this->cell.set_current(new_cell);
    return TableCell(this->current, new_cell);
}

bool TableRow::has_next() const { return this->current != pugi::xml_node{}; }

TableRow &TableRow::next() {
    this->current = detail::next_named_sibling(this->current, "w:tr");
    return *this;
}

std::optional<TableCell> TableRow::find_cell(std::size_t cell_index) {
    auto cell_handle = this->cells();
    for (std::size_t current_index = 0U;
         current_index < cell_index && cell_handle.has_next(); ++current_index) {
        cell_handle.next();
    }

    if (!cell_handle.has_next()) {
        return std::nullopt;
    }

    return cell_handle;
}

bool TableRow::set_texts(const std::vector<std::string> &texts) {
    if (this->current == pugi::xml_node{}) {
        return false;
    }

    const auto cell_count = count_named_children(this->current, "w:tc");
    if (texts.size() != cell_count) {
        return false;
    }

    auto cell_handle = this->cells();
    for (std::size_t index = 0U; index < texts.size(); ++index) {
        if (!cell_handle.has_next() || !cell_handle.set_text(texts[index])) {
            return false;
        }

        if (index + 1U < texts.size()) {
            cell_handle.next();
        }
    }

    return true;
}

bool TableRow::set_texts(std::initializer_list<std::string> texts) {
    return this->set_texts(std::vector<std::string>{texts});
}

Table::Table() = default;

Table::Table(pugi::xml_node parent, pugi::xml_node current) {
    this->set_parent(parent);
    this->set_current(current);
}

void Table::set_owner(Document *document_owner) { this->owner = document_owner; }

void Table::set_parent(pugi::xml_node node) {
    this->parent = node;
    this->current = this->parent.child("w:tbl");
    this->row.set_parent(this->current);
}

void Table::set_current(pugi::xml_node node) {
    this->current = node;
    this->row.set_parent(this->current);
}

Table &Table::next() {
    this->current = detail::next_named_sibling(this->current, "w:tbl");
    this->row.set_parent(this->current);
    return *this;
}

bool Table::has_next() const { return this->current != pugi::xml_node{}; }

TableRow &Table::rows() {
    this->row.set_parent(this->current);
    return this->row;
}

std::optional<TableRow> Table::find_row(std::size_t row_index) {
    auto row_handle = this->rows();
    for (std::size_t current_index = 0U;
         current_index < row_index && row_handle.has_next(); ++current_index) {
        row_handle.next();
    }

    if (!row_handle.has_next()) {
        return std::nullopt;
    }

    return row_handle;
}

std::optional<TableCell> Table::find_cell(std::size_t row_index, std::size_t cell_index) {
    auto row_handle = this->find_row(row_index);
    if (!row_handle.has_value()) {
        return std::nullopt;
    }

    return row_handle->find_cell(cell_index);
}

bool Table::set_cell_text(std::size_t row_index, std::size_t cell_index,
                          const std::string &text) {
    auto cell_handle = this->find_cell(row_index, cell_index);
    if (!cell_handle.has_value()) {
        return false;
    }

    return cell_handle->set_text(text);
}

bool Table::set_row_texts(std::size_t row_index, const std::vector<std::string> &texts) {
    auto row_handle = this->find_row(row_index);
    if (!row_handle.has_value()) {
        return false;
    }

    return row_handle->set_texts(texts);
}

bool Table::set_row_texts(std::size_t row_index, std::initializer_list<std::string> texts) {
    return this->set_row_texts(row_index, std::vector<std::string>{texts});
}

bool Table::set_rows_texts(std::size_t start_row_index,
                           const std::vector<std::vector<std::string>> &rows) {
    if (rows.empty()) {
        return true;
    }

    const auto count_row_cells = [](TableRow row_handle) -> std::size_t {
        auto count = std::size_t{0U};
        for (auto cell_handle = row_handle.cells(); cell_handle.has_next();
             cell_handle.next()) {
            ++count;
        }
        return count;
    };

    auto row_handles = std::vector<TableRow>{};
    row_handles.reserve(rows.size());
    for (std::size_t row_offset = 0U; row_offset < rows.size(); ++row_offset) {
        auto row_handle = this->find_row(start_row_index + row_offset);
        if (!row_handle.has_value()) {
            return false;
        }

        if (count_row_cells(*row_handle) != rows[row_offset].size()) {
            return false;
        }

        row_handles.push_back(*row_handle);
    }

    for (std::size_t row_offset = 0U; row_offset < rows.size(); ++row_offset) {
        if (!row_handles[row_offset].set_texts(rows[row_offset])) {
            return false;
        }
    }

    return true;
}

bool Table::set_rows_texts(
    std::size_t start_row_index,
    std::initializer_list<std::initializer_list<std::string>> rows) {
    return this->set_rows_texts(start_row_index,
                                string_matrix_from_initializer_list(rows));
}

bool Table::set_cell_block_texts(
    std::size_t start_row_index, std::size_t start_cell_index,
    const std::vector<std::vector<std::string>> &rows) {
    if (rows.empty()) {
        return true;
    }

    const auto count_row_cells = [](TableRow row_handle) -> std::size_t {
        auto count = std::size_t{0U};
        for (auto cell_handle = row_handle.cells(); cell_handle.has_next();
             cell_handle.next()) {
            ++count;
        }
        return count;
    };

    auto row_handles = std::vector<TableRow>{};
    row_handles.reserve(rows.size());
    for (std::size_t row_offset = 0U; row_offset < rows.size(); ++row_offset) {
        auto row_handle = this->find_row(start_row_index + row_offset);
        if (!row_handle.has_value()) {
            return false;
        }

        const auto row_cell_count = count_row_cells(*row_handle);
        if (start_cell_index > row_cell_count ||
            rows[row_offset].size() > row_cell_count - start_cell_index) {
            return false;
        }

        row_handles.push_back(*row_handle);
    }

    for (std::size_t row_offset = 0U; row_offset < rows.size(); ++row_offset) {
        if (rows[row_offset].empty()) {
            continue;
        }

        auto cell_handle = row_handles[row_offset].find_cell(start_cell_index);
        if (!cell_handle.has_value()) {
            return false;
        }

        for (std::size_t cell_offset = 0U; cell_offset < rows[row_offset].size();
             ++cell_offset) {
            if (!cell_handle->set_text(rows[row_offset][cell_offset])) {
                return false;
            }

            if (cell_offset + 1U < rows[row_offset].size()) {
                cell_handle->next();
                if (!cell_handle->has_next()) {
                    return false;
                }
            }
        }
    }

    return true;
}

bool Table::set_cell_block_texts(
    std::size_t start_row_index, std::size_t start_cell_index,
    std::initializer_list<std::initializer_list<std::string>> rows) {
    return this->set_cell_block_texts(start_row_index, start_cell_index,
                                      string_matrix_from_initializer_list(rows));
}

std::optional<std::uint32_t> Table::width_twips() const {
    if (this->current == pugi::xml_node{}) {
        return std::nullopt;
    }

    const auto width_node = this->current.child("w:tblPr").child("w:tblW");
    if (width_node == pugi::xml_node{} ||
        std::string_view{width_node.attribute("w:type").value()} != "dxa") {
        return std::nullopt;
    }

    return parse_unsigned_attribute(width_node, "w:w");
}

bool Table::set_width_twips(std::uint32_t width_twips) {
    if (this->current == pugi::xml_node{}) {
        return false;
    }

    const auto width_node = ensure_table_width_node(this->current);
    if (width_node == pugi::xml_node{}) {
        return false;
    }

    const auto width_text = std::to_string(width_twips);
    ensure_attribute_value(width_node, "w:w", width_text.c_str());
    ensure_attribute_value(width_node, "w:type", "dxa");
    return true;
}

bool Table::clear_width() {
    if (this->current == pugi::xml_node{}) {
        return false;
    }

    auto table_properties = this->current.child("w:tblPr");
    if (table_properties == pugi::xml_node{}) {
        return true;
    }

    const auto width_node = table_properties.child("w:tblW");
    return width_node == pugi::xml_node{} || table_properties.remove_child(width_node);
}

std::optional<std::uint32_t> Table::column_width_twips(std::size_t column_index) const {
    if (this->current == pugi::xml_node{}) {
        return std::nullopt;
    }

    const auto column_count = current_table_column_count(this->current);
    if (column_index >= column_count) {
        return std::nullopt;
    }

    const auto grid_column = find_table_grid_column(this->current, column_index);
    if (grid_column == pugi::xml_node{} || grid_column.attribute("w:w") == pugi::xml_attribute{}) {
        return std::nullopt;
    }

    return parse_unsigned_attribute(grid_column, "w:w");
}

bool Table::set_column_width_twips(std::size_t column_index, std::uint32_t width_twips) {
    if (this->current == pugi::xml_node{}) {
        return false;
    }

    const auto column_count = current_table_column_count(this->current);
    if (column_index >= column_count) {
        return false;
    }

    ensure_table_grid_columns(this->current, column_count);
    const auto grid_column = find_table_grid_column(this->current, column_index);
    if (grid_column == pugi::xml_node{}) {
        return false;
    }

    const auto width_text = std::to_string(width_twips);
    ensure_attribute_value(grid_column, "w:w", width_text.c_str());
    synchronize_fixed_layout_cell_widths_from_grid(this->current);
    return true;
}

bool Table::clear_column_width(std::size_t column_index) {
    if (this->current == pugi::xml_node{}) {
        return false;
    }

    const auto column_count = current_table_column_count(this->current);
    if (column_index >= column_count) {
        return false;
    }

    auto grid_column = find_table_grid_column(this->current, column_index);
    if (grid_column == pugi::xml_node{} || grid_column.attribute("w:w") == pugi::xml_attribute{}) {
        return true;
    }

    if (!grid_column.remove_attribute("w:w")) {
        return false;
    }

    clear_fixed_layout_cell_widths_covering_column(this->current, column_index);
    return true;
}

std::optional<featherdoc::table_layout_mode> Table::layout_mode() const {
    if (this->current == pugi::xml_node{}) {
        return std::nullopt;
    }

    const auto layout_node = this->current.child("w:tblPr").child("w:tblLayout");
    const auto layout_type = std::string_view{layout_node.attribute("w:type").value()};
    if (layout_type.empty()) {
        return std::nullopt;
    }

    return parse_table_layout_mode(layout_type);
}

bool Table::set_layout_mode(featherdoc::table_layout_mode layout_mode) {
    if (this->current == pugi::xml_node{}) {
        return false;
    }

    const auto layout_node = ensure_table_layout_node(this->current);
    if (layout_node == pugi::xml_node{}) {
        return false;
    }

    ensure_attribute_value(layout_node, "w:type", to_xml_table_layout_mode(layout_mode));
    if (layout_mode == featherdoc::table_layout_mode::fixed) {
        synchronize_fixed_layout_cell_widths_from_grid(this->current);
    }
    return true;
}

bool Table::clear_layout_mode() {
    if (this->current == pugi::xml_node{}) {
        return false;
    }

    auto table_properties = this->current.child("w:tblPr");
    if (table_properties == pugi::xml_node{}) {
        return true;
    }

    const auto layout_node = table_properties.child("w:tblLayout");
    return layout_node == pugi::xml_node{} || table_properties.remove_child(layout_node);
}

std::optional<featherdoc::table_alignment> Table::alignment() const {
    if (this->current == pugi::xml_node{}) {
        return std::nullopt;
    }

    const auto alignment_node = this->current.child("w:tblPr").child("w:jc");
    const auto alignment_text = std::string_view{alignment_node.attribute("w:val").value()};
    if (alignment_text.empty()) {
        return std::nullopt;
    }

    return parse_table_alignment(alignment_text);
}

bool Table::set_alignment(featherdoc::table_alignment alignment) {
    if (this->current == pugi::xml_node{}) {
        return false;
    }

    const auto alignment_node = ensure_table_alignment_node(this->current);
    if (alignment_node == pugi::xml_node{}) {
        return false;
    }

    ensure_attribute_value(alignment_node, "w:val", to_xml_table_alignment(alignment));
    return true;
}

bool Table::clear_alignment() {
    if (this->current == pugi::xml_node{}) {
        return false;
    }

    auto table_properties = this->current.child("w:tblPr");
    if (table_properties == pugi::xml_node{}) {
        return true;
    }

    const auto alignment_node = table_properties.child("w:jc");
    return alignment_node == pugi::xml_node{} || table_properties.remove_child(alignment_node);
}

std::optional<std::uint32_t> Table::indent_twips() const {
    if (this->current == pugi::xml_node{}) {
        return std::nullopt;
    }

    const auto indent_node = this->current.child("w:tblPr").child("w:tblInd");
    if (indent_node == pugi::xml_node{}) {
        return std::nullopt;
    }

    if (const auto indent_type = std::string_view{indent_node.attribute("w:type").value()};
        !indent_type.empty() && indent_type != "dxa") {
        return std::nullopt;
    }

    return parse_unsigned_attribute(indent_node, "w:w");
}

bool Table::set_indent_twips(std::uint32_t indent_twips) {
    if (this->current == pugi::xml_node{}) {
        return false;
    }

    const auto indent_node = ensure_table_indent_node(this->current);
    if (indent_node == pugi::xml_node{}) {
        return false;
    }

    const auto indent_text = std::to_string(indent_twips);
    ensure_attribute_value(indent_node, "w:w", indent_text.c_str());
    ensure_attribute_value(indent_node, "w:type", "dxa");
    return true;
}

bool Table::clear_indent() {
    if (this->current == pugi::xml_node{}) {
        return false;
    }

    auto table_properties = this->current.child("w:tblPr");
    if (table_properties == pugi::xml_node{}) {
        return true;
    }

    const auto indent_node = table_properties.child("w:tblInd");
    return indent_node == pugi::xml_node{} || table_properties.remove_child(indent_node);
}

std::optional<std::uint32_t> Table::cell_spacing_twips() const {
    if (this->current == pugi::xml_node{}) {
        return std::nullopt;
    }

    const auto spacing = this->current.child("w:tblPr").child("w:tblCellSpacing");
    if (spacing == pugi::xml_node{}) {
        return std::nullopt;
    }

    if (const auto spacing_type = std::string_view{spacing.attribute("w:type").value()};
        !spacing_type.empty() && spacing_type != "dxa") {
        return std::nullopt;
    }

    return parse_unsigned_attribute(spacing, "w:w");
}

bool Table::set_cell_spacing_twips(std::uint32_t spacing_twips) {
    if (this->current == pugi::xml_node{}) {
        return false;
    }

    const auto spacing = ensure_table_cell_spacing_node(this->current);
    if (spacing == pugi::xml_node{}) {
        return false;
    }

    const auto spacing_text = std::to_string(spacing_twips);
    ensure_attribute_value(spacing, "w:w", spacing_text.c_str());
    ensure_attribute_value(spacing, "w:type", "dxa");
    return true;
}

bool Table::clear_cell_spacing() {
    if (this->current == pugi::xml_node{}) {
        return false;
    }

    auto table_properties = this->current.child("w:tblPr");
    if (table_properties == pugi::xml_node{}) {
        return true;
    }

    const auto spacing = table_properties.child("w:tblCellSpacing");
    return spacing == pugi::xml_node{} || table_properties.remove_child(spacing);
}

void set_optional_unsigned_attribute(pugi::xml_node node, const char *name,
                                     std::optional<std::uint32_t> value) {
    if (node == pugi::xml_node{}) {
        return;
    }

    if (value.has_value()) {
        const auto text = std::to_string(*value);
        ensure_attribute_value(node, name, text.c_str());
        return;
    }

    if (node.attribute(name) != pugi::xml_attribute{}) {
        node.remove_attribute(name);
    }
}

std::optional<featherdoc::table_position> Table::position() const {
    if (this->current == pugi::xml_node{}) {
        return std::nullopt;
    }

    const auto position_node = this->current.child("w:tblPr").child("w:tblpPr");
    if (position_node == pugi::xml_node{}) {
        return std::nullopt;
    }

    auto position = featherdoc::table_position{};
    if (const auto horizontal_reference = parse_table_position_horizontal_reference(
            std::string_view{position_node.attribute("w:horzAnchor").value()})) {
        position.horizontal_reference = *horizontal_reference;
    }
    if (const auto horizontal_offset = parse_signed_attribute(position_node, "w:tblpX")) {
        position.horizontal_offset_twips = *horizontal_offset;
    }
    if (const auto vertical_reference = parse_table_position_vertical_reference(
            std::string_view{position_node.attribute("w:vertAnchor").value()})) {
        position.vertical_reference = *vertical_reference;
    }
    if (const auto vertical_offset = parse_signed_attribute(position_node, "w:tblpY")) {
        position.vertical_offset_twips = *vertical_offset;
    }
    position.left_from_text_twips =
        parse_unsigned_attribute(position_node, "w:leftFromText");
    position.right_from_text_twips =
        parse_unsigned_attribute(position_node, "w:rightFromText");
    position.top_from_text_twips =
        parse_unsigned_attribute(position_node, "w:topFromText");
    position.bottom_from_text_twips =
        parse_unsigned_attribute(position_node, "w:bottomFromText");
    if (const auto overlap = parse_table_overlap(
            std::string_view{position_node.attribute("w:tblOverlap").value()})) {
        position.overlap = *overlap;
    }

    return position;
}

bool Table::set_position(featherdoc::table_position position) {
    if (this->current == pugi::xml_node{}) {
        return false;
    }

    auto position_node = ensure_table_position_node(this->current);
    if (position_node == pugi::xml_node{}) {
        return false;
    }

    const auto horizontal_offset = std::to_string(position.horizontal_offset_twips);
    const auto vertical_offset = std::to_string(position.vertical_offset_twips);
    ensure_attribute_value(position_node, "w:horzAnchor",
                           to_xml_table_position_horizontal_reference(
                               position.horizontal_reference));
    ensure_attribute_value(position_node, "w:tblpX", horizontal_offset.c_str());
    ensure_attribute_value(position_node, "w:vertAnchor",
                           to_xml_table_position_vertical_reference(
                               position.vertical_reference));
    ensure_attribute_value(position_node, "w:tblpY", vertical_offset.c_str());
    set_optional_unsigned_attribute(position_node, "w:leftFromText",
                                    position.left_from_text_twips);
    set_optional_unsigned_attribute(position_node, "w:rightFromText",
                                    position.right_from_text_twips);
    set_optional_unsigned_attribute(position_node, "w:topFromText",
                                    position.top_from_text_twips);
    set_optional_unsigned_attribute(position_node, "w:bottomFromText",
                                    position.bottom_from_text_twips);
    if (position.overlap.has_value()) {
        ensure_attribute_value(position_node, "w:tblOverlap",
                               to_xml_table_overlap(*position.overlap));
    } else if (position_node.attribute("w:tblOverlap") != pugi::xml_attribute{}) {
        position_node.remove_attribute("w:tblOverlap");
    }
    return true;
}

bool Table::clear_position() {
    if (this->current == pugi::xml_node{}) {
        return false;
    }

    auto table_properties = this->current.child("w:tblPr");
    if (table_properties == pugi::xml_node{}) {
        return true;
    }

    const auto position_node = table_properties.child("w:tblpPr");
    return position_node == pugi::xml_node{} ||
           table_properties.remove_child(position_node);
}

std::optional<std::uint32_t> Table::cell_margin_twips(
    featherdoc::cell_margin_edge edge) const {
    if (this->current == pugi::xml_node{}) {
        return std::nullopt;
    }

    const auto margin =
        this->current.child("w:tblPr").child("w:tblCellMar").child(to_xml_margin_name(edge));
    if (margin == pugi::xml_node{}) {
        return std::nullopt;
    }

    if (const auto margin_type = std::string_view{margin.attribute("w:type").value()};
        !margin_type.empty() && margin_type != "dxa") {
        return std::nullopt;
    }

    return parse_unsigned_attribute(margin, "w:w");
}

bool Table::set_cell_margin_twips(featherdoc::cell_margin_edge edge,
                                  std::uint32_t margin_twips) {
    if (this->current == pugi::xml_node{}) {
        return false;
    }

    const auto margin = ensure_table_cell_margin_node(this->current, to_xml_margin_name(edge));
    if (margin == pugi::xml_node{}) {
        return false;
    }

    const auto margin_text = std::to_string(margin_twips);
    ensure_attribute_value(margin, "w:w", margin_text.c_str());
    ensure_attribute_value(margin, "w:type", "dxa");
    return true;
}

bool Table::clear_cell_margin(featherdoc::cell_margin_edge edge) {
    if (this->current == pugi::xml_node{}) {
        return false;
    }

    auto table_properties = this->current.child("w:tblPr");
    if (table_properties == pugi::xml_node{}) {
        return true;
    }

    auto margins = table_properties.child("w:tblCellMar");
    if (margins == pugi::xml_node{}) {
        return true;
    }

    if (const auto margin = margins.child(to_xml_margin_name(edge)); margin != pugi::xml_node{}) {
        margins.remove_child(margin);
    }

    remove_empty_container(table_properties, "w:tblCellMar");
    return true;
}

std::optional<std::string> Table::style_id() const {
    if (this->current == pugi::xml_node{}) {
        return std::nullopt;
    }

    const auto style_node = this->current.child("w:tblPr").child("w:tblStyle");
    const auto style_text = std::string_view{style_node.attribute("w:val").value()};
    if (style_text.empty()) {
        return std::nullopt;
    }

    return std::string{style_text};
}

bool Table::set_style_id(std::string_view style_id) {
    if (this->current == pugi::xml_node{} || style_id.empty()) {
        return false;
    }

    if (this->owner != nullptr && this->owner->ensure_styles_part_attached()) {
        return false;
    }

    const auto style_node = ensure_table_style_node(this->current);
    if (style_node == pugi::xml_node{}) {
        return false;
    }

    const auto style_text = std::string{style_id};
    ensure_attribute_value(style_node, "w:val", style_text.c_str());
    return true;
}

bool Table::clear_style_id() {
    if (this->current == pugi::xml_node{}) {
        return false;
    }

    auto table_properties = this->current.child("w:tblPr");
    if (table_properties == pugi::xml_node{}) {
        return true;
    }

    const auto style_node = table_properties.child("w:tblStyle");
    return style_node == pugi::xml_node{} || table_properties.remove_child(style_node);
}

std::optional<featherdoc::table_style_look> Table::style_look() const {
    if (this->current == pugi::xml_node{}) {
        return std::nullopt;
    }

    const auto table_look_node = this->current.child("w:tblPr").child("w:tblLook");
    if (table_look_node == pugi::xml_node{}) {
        return std::nullopt;
    }

    const auto encoded_value =
        parse_short_hex_value(std::string_view{table_look_node.attribute("w:val").value()});
    auto style_look = featherdoc::table_style_look{};
    style_look.first_row = decode_table_style_look_flag(
        parse_xml_on_off_value(std::string_view{table_look_node.attribute("w:firstRow").value()}),
        encoded_value, table_style_look_first_row_bit, style_look.first_row);
    style_look.last_row = decode_table_style_look_flag(
        parse_xml_on_off_value(std::string_view{table_look_node.attribute("w:lastRow").value()}),
        encoded_value, table_style_look_last_row_bit, style_look.last_row);
    style_look.first_column = decode_table_style_look_flag(
        parse_xml_on_off_value(
            std::string_view{table_look_node.attribute("w:firstColumn").value()}),
        encoded_value, table_style_look_first_column_bit, style_look.first_column);
    style_look.last_column = decode_table_style_look_flag(
        parse_xml_on_off_value(
            std::string_view{table_look_node.attribute("w:lastColumn").value()}),
        encoded_value, table_style_look_last_column_bit, style_look.last_column);
    style_look.banded_rows = decode_table_style_look_flag(
        parse_xml_on_off_value(std::string_view{table_look_node.attribute("w:noHBand").value()}),
        encoded_value, table_style_look_no_hband_bit, style_look.banded_rows, true);
    style_look.banded_columns = decode_table_style_look_flag(
        parse_xml_on_off_value(std::string_view{table_look_node.attribute("w:noVBand").value()}),
        encoded_value, table_style_look_no_vband_bit, style_look.banded_columns, true);
    return style_look;
}

bool Table::set_style_look(featherdoc::table_style_look style_look) {
    if (this->current == pugi::xml_node{}) {
        return false;
    }

    const auto table_look_node = ensure_table_look_node(this->current);
    if (table_look_node == pugi::xml_node{}) {
        return false;
    }

    const auto encoded_value = format_short_hex(encode_table_style_look(style_look));
    ensure_attribute_value(table_look_node, "w:val", encoded_value.c_str());
    ensure_attribute_value(table_look_node, "w:firstRow", style_look.first_row ? "1" : "0");
    ensure_attribute_value(table_look_node, "w:lastRow", style_look.last_row ? "1" : "0");
    ensure_attribute_value(table_look_node, "w:firstColumn",
                           style_look.first_column ? "1" : "0");
    ensure_attribute_value(table_look_node, "w:lastColumn",
                           style_look.last_column ? "1" : "0");
    ensure_attribute_value(table_look_node, "w:noHBand", style_look.banded_rows ? "0" : "1");
    ensure_attribute_value(table_look_node, "w:noVBand",
                           style_look.banded_columns ? "0" : "1");
    return true;
}

bool Table::clear_style_look() {
    if (this->current == pugi::xml_node{}) {
        return false;
    }

    auto table_properties = this->current.child("w:tblPr");
    if (table_properties == pugi::xml_node{}) {
        return true;
    }

    const auto table_look_node = table_properties.child("w:tblLook");
    return table_look_node == pugi::xml_node{} || table_properties.remove_child(table_look_node);
}

bool Table::set_border(featherdoc::table_border_edge edge,
                       featherdoc::border_definition border) {
    if (this->current == pugi::xml_node{}) {
        return false;
    }

    auto table_borders = ensure_table_borders_node(this->current);
    if (table_borders == pugi::xml_node{}) {
        return false;
    }

    const auto border_name = to_xml_border_name(edge);
    auto border_node = table_borders.child(border_name);
    if (border_node == pugi::xml_node{}) {
        border_node = table_borders.append_child(border_name);
    }
    if (border_node == pugi::xml_node{}) {
        return false;
    }

    apply_border_definition(border_node, border);
    return true;
}

bool Table::clear_border(featherdoc::table_border_edge edge) {
    if (this->current == pugi::xml_node{}) {
        return false;
    }

    auto table_properties = this->current.child("w:tblPr");
    if (table_properties == pugi::xml_node{}) {
        return true;
    }

    auto table_borders = table_properties.child("w:tblBorders");
    if (table_borders == pugi::xml_node{}) {
        return true;
    }

    if (const auto border_node = table_borders.child(to_xml_border_name(edge));
        border_node != pugi::xml_node{}) {
        table_borders.remove_child(border_node);
    }

    remove_empty_container(table_properties, "w:tblBorders");
    return true;
}

bool Table::remove() {
    if (this->parent == pugi::xml_node{} || this->current == pugi::xml_node{}) {
        return false;
    }

    if (detail::parent_requires_nonempty_block_content(this->parent) &&
        detail::count_remaining_block_children(this->parent, this->current) == 0U) {
        return false;
    }

    const auto next_table = detail::next_named_sibling(this->current, "w:tbl");
    const auto previous_table = detail::previous_named_sibling(this->current, "w:tbl");
    if (!this->parent.remove_child(this->current)) {
        return false;
    }

    this->current =
        next_table != pugi::xml_node{} ? next_table : previous_table;
    this->row.set_parent(this->current);
    return true;
}

Table Table::insert_table_before(std::size_t row_count, std::size_t column_count) {
    if (this->parent == pugi::xml_node{} || this->current == pugi::xml_node{}) {
        return {};
    }

    const auto table_node = detail::insert_table_node(this->parent, this->current);
    auto created_table = Table(this->parent, table_node);
    created_table.set_owner(this->owner);

    for (std::size_t row_index = 0; row_index < row_count; ++row_index) {
        created_table.append_row(column_count);
    }

    this->current = table_node;
    this->row.set_parent(this->current);
    return created_table;
}

Table Table::insert_table_after(std::size_t row_count, std::size_t column_count) {
    if (this->parent == pugi::xml_node{} || this->current == pugi::xml_node{}) {
        return {};
    }

    const auto next_table = detail::next_named_sibling(this->current, "w:tbl");
    const auto table_node = detail::insert_table_node(this->parent, next_table);
    auto created_table = Table(this->parent, table_node);
    created_table.set_owner(this->owner);

    for (std::size_t row_index = 0; row_index < row_count; ++row_index) {
        created_table.append_row(column_count);
    }

    this->current = table_node;
    this->row.set_parent(this->current);
    return created_table;
}

Table Table::insert_table_like_before() {
    if (this->parent == pugi::xml_node{} || this->current == pugi::xml_node{}) {
        return {};
    }

    const auto table_node = insert_empty_clone_table(this->parent, this->current, false);
    if (table_node == pugi::xml_node{}) {
        return {};
    }

    auto created_table = Table(this->parent, table_node);
    created_table.set_owner(this->owner);

    this->current = table_node;
    this->row.set_parent(this->current);
    return created_table;
}

Table Table::insert_table_like_after() {
    if (this->parent == pugi::xml_node{} || this->current == pugi::xml_node{}) {
        return {};
    }

    const auto table_node = insert_empty_clone_table(this->parent, this->current, true);
    if (table_node == pugi::xml_node{}) {
        return {};
    }

    auto created_table = Table(this->parent, table_node);
    created_table.set_owner(this->owner);

    this->current = table_node;
    this->row.set_parent(this->current);
    return created_table;
}

TableRow Table::append_row(std::size_t cell_count) {
    if (this->current == pugi::xml_node{} && this->parent != pugi::xml_node{}) {
        this->current = detail::append_table_node(this->parent);
    }

    if (this->current == pugi::xml_node{}) {
        return {};
    }

    const auto new_row = append_row_node(this->current, cell_count);
    this->row.set_parent(this->current);
    this->row.set_current(new_row);
    return TableRow(this->current, new_row);
}

} // namespace featherdoc
