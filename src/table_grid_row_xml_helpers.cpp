#include "table_xml_helpers.hpp"
#include "xml_helpers.hpp"

#include <algorithm>
#include <charconv>
#include <system_error>

namespace featherdoc::detail {

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
        static_cast<void>(append_cell_node(row));
    }
    return row;
}

} // namespace featherdoc::detail
