#include "table_xml_helpers.hpp"
#include "table_column_edit_helpers.hpp"
#include "xml_document_clone_helpers.hpp"
#include "xml_helpers.hpp"

#include <algorithm>
#include <charconv>
#include <system_error>

namespace featherdoc::detail {

namespace {

[[nodiscard]] auto checked_insert_copy_xml_node(
    pugi::xml_node parent, pugi::xml_node source, pugi::xml_node anchor,
    xml_clone_exception_policy exception_policy =
        xml_clone_exception_policy::return_failure)
    -> pugi::xml_node {
    if (parent == pugi::xml_node{} || source == pugi::xml_node{} ||
        source.type() == pugi::node_document ||
        (anchor != pugi::xml_node{} && anchor.parent() != parent)) {
        return {};
    }

    auto inserted = pugi::xml_node{};
    const auto rollback = [&]() noexcept {
        if (inserted != pugi::xml_node{}) {
            (void)parent.remove_child(inserted);
        }
    };
    try {
        inserted = anchor != pugi::xml_node{}
                       ? parent.insert_child_before(source.type(), anchor)
                       : parent.append_child(source.type());
        if (inserted == pugi::xml_node{}) {
            return {};
        }

        if (!xml_document_clone_detail::copy_node_contents(source, inserted)) {
            rollback();
            return {};
        }
        for (auto child = source.first_child(); child != pugi::xml_node{};
             child = child.next_sibling()) {
            if (checked_append_copy_xml_node(child, inserted,
                                             exception_policy) !=
                xml_document_clone_status::success) {
                rollback();
                return {};
            }
        }
    } catch (...) {
        rollback();
        if (exception_policy == xml_clone_exception_policy::propagate) {
            throw;
        }
        return {};
    }

    return inserted;
}

auto parse_table_cell_column_span(pugi::xml_node cell)
    -> std::optional<std::size_t> {
    if (cell == pugi::xml_node{}) {
        return std::nullopt;
    }

    const auto span_node = cell.child("w:tcPr").child("w:gridSpan");
    if (span_node == pugi::xml_node{}) {
        return 1U;
    }

    const auto span_attribute = span_node.attribute("w:val");
    if (span_attribute == pugi::xml_attribute{}) {
        return std::nullopt;
    }
    const auto span_text = std::string_view{span_attribute.value()};

    auto span = std::size_t{0U};
    const auto [end, error] =
        std::from_chars(span_text.data(), span_text.data() + span_text.size(), span);
    if (error != std::errc{} || end != span_text.data() + span_text.size() ||
        span == 0U || span > max_table_grid_columns) {
        return std::nullopt;
    }

    return span;
}

} // namespace

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

auto current_table_row_column_count(pugi::xml_node row)
    -> std::optional<std::size_t> {
    if (row == pugi::xml_node{}) {
        return std::nullopt;
    }

    auto column_count = std::size_t{0U};
    for (auto cell = row.child("w:tc"); cell != pugi::xml_node{};
         cell = detail::next_named_sibling(cell, "w:tc")) {
        const auto span = parse_table_cell_column_span(cell);
        if (!span.has_value() ||
            *span > max_table_grid_columns - column_count) {
            return std::nullopt;
        }
        column_count += *span;
    }

    return column_count;
}

auto current_table_column_count(pugi::xml_node table)
    -> std::optional<std::size_t> {
    if (table == pugi::xml_node{}) {
        return std::nullopt;
    }

    auto column_count =
        count_named_children(table.child("w:tblGrid"), "w:gridCol");
    if (column_count > max_table_grid_columns) {
        return std::nullopt;
    }
    for (auto row = table.child("w:tr"); row != pugi::xml_node{};
         row = detail::next_named_sibling(row, "w:tr")) {
        const auto row_column_count = current_table_row_column_count(row);
        if (!row_column_count.has_value()) {
            return std::nullopt;
        }
        column_count = std::max(column_count, *row_column_count);
    }

    return column_count;
}

auto table_geometry_is_valid_for_append(pugi::xml_node table) -> bool {
    if (table == pugi::xml_node{} ||
        std::string_view{table.name()} != "w:tbl" ||
        count_named_children(table, "w:tblPr") > 1U ||
        count_named_children(table, "w:tblGrid") > 1U) {
        return false;
    }

    for (auto row = table.child("w:tr"); row != pugi::xml_node{};
         row = detail::next_named_sibling(row, "w:tr")) {
        for (auto cell = row.child("w:tc"); cell != pugi::xml_node{};
             cell = detail::next_named_sibling(cell, "w:tc")) {
            if (count_named_children(cell, "w:tcPr") > 1U) {
                return false;
            }
            const auto properties = cell.child("w:tcPr");
            if (count_named_children(properties, "w:gridSpan") > 1U) {
                return false;
            }
        }
        if (!current_table_row_column_count(row).has_value()) {
            return false;
        }
    }

    return true;
}

auto ensure_table_grid_columns(pugi::xml_node table, std::size_t column_count)
    -> bool {
    if (table == pugi::xml_node{} || column_count > max_table_grid_columns) {
        return false;
    }

    const auto existing_grid = table.child("w:tblGrid");
    if (count_named_children(existing_grid, "w:gridCol") >
        max_table_grid_columns) {
        return false;
    }

    ensure_default_table_properties(table);
    auto table_grid = ensure_table_grid_node(table);
    if (table_grid == pugi::xml_node{}) {
        return false;
    }

    auto existing_columns = count_named_children(table_grid, "w:gridCol");
    while (existing_columns < column_count) {
        auto grid_column = table_grid.append_child("w:gridCol");
        if (grid_column == pugi::xml_node{}) {
            return false;
        }
        ensure_attribute_value(grid_column, "w:w", "0");
        ++existing_columns;
    }
    return true;
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

    // Legacy readers treat malformed spans as one cell. Mutation paths call
    // current_table_column_count() first and reject the same input before edits.
    return parse_table_cell_column_span(cell).value_or(1U);
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
    if (table == pugi::xml_node{} || source_row == pugi::xml_node{} ||
        source_row.parent() != table ||
        (merge_guard_row != pugi::xml_node{} &&
         merge_guard_row.parent() != table)) {
        return {};
    }

    const auto table_column_count = current_table_column_count(table);
    const auto source_row_column_count =
        current_table_row_column_count(source_row);
    if (!table_column_count.has_value() || *table_column_count == 0U ||
        !source_row_column_count.has_value() ||
        *source_row_column_count == 0U) {
        return {};
    }

    if (row_contains_vertical_merge_cells(source_row) ||
        row_contains_vertical_merge_cells(merge_guard_row)) {
        return {};
    }

    const auto anchor = insert_after ? source_row.next_sibling() : source_row;
    auto inserted_row = checked_insert_copy_xml_node(
        table, source_row, anchor, xml_clone_exception_policy::propagate);
    if (inserted_row == pugi::xml_node{}) {
        return {};
    }

    const auto rollback = [&]() noexcept {
        (void)table.remove_child(inserted_row);
    };
    try {
        for (auto row_cell = inserted_row.child("w:tc");
             row_cell != pugi::xml_node{};
             row_cell = detail::next_named_sibling(row_cell, "w:tc")) {
            if (!replace_table_cell_text(row_cell, "")) {
                rollback();
                return {};
            }
        }
    } catch (...) {
        rollback();
        throw;
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
            if (!replace_table_cell_text(cell, "")) {
                return false;
            }
        }
    }

    return true;
}

auto insert_empty_clone_table(pugi::xml_node parent, pugi::xml_node source_table,
                              bool insert_after) -> pugi::xml_node {
    if (parent == pugi::xml_node{} || source_table == pugi::xml_node{} ||
        source_table.parent() != parent) {
        return {};
    }

    const auto table_column_count = current_table_column_count(source_table);
    if (!table_column_count.has_value() || *table_column_count == 0U) {
        return {};
    }

    auto source_row = source_table.child("w:tr");
    if (source_row == pugi::xml_node{}) {
        return {};
    }
    for (; source_row != pugi::xml_node{};
         source_row = detail::next_named_sibling(source_row, "w:tr")) {
        const auto row_column_count =
            current_table_row_column_count(source_row);
        if (!row_column_count.has_value() || *row_column_count == 0U) {
            return {};
        }
    }

    const auto anchor =
        insert_after ? source_table.next_sibling() : source_table;
    auto inserted_table =
        checked_insert_copy_xml_node(parent, source_table, anchor);
    if (inserted_table == pugi::xml_node{}) {
        return {};
    }

    if (!clear_table_cell_contents(inserted_table)) {
        (void)parent.remove_child(inserted_table);
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

    auto cell = checked_append_xml_element(row, "w:tc");
    if (cell == pugi::xml_node{}) {
        return {};
    }
    const auto rollback = [&]() noexcept { (void)row.remove_child(cell); };
    auto cell_properties = checked_append_xml_element(cell, "w:tcPr");
    auto cell_width = checked_append_xml_element(cell_properties, "w:tcW");
    if (cell_properties == pugi::xml_node{} ||
        cell_width == pugi::xml_node{} ||
        !checked_set_xml_attribute_value(cell_width, "w:w", "0") ||
        !checked_set_xml_attribute_value(cell_width, "w:type", "auto") ||
        checked_append_xml_element(cell, "w:p") == pugi::xml_node{}) {
        rollback();
        return {};
    }
    return cell;
}

auto append_row_node(pugi::xml_node table, std::size_t cell_count) -> pugi::xml_node {
    if (table == pugi::xml_node{} || cell_count == 0U ||
        cell_count > max_table_grid_columns) {
        return {};
    }

    const auto current_column_count = current_table_column_count(table);
    if (!current_column_count.has_value()) {
        return {};
    }
    const auto required_column_count =
        std::max(*current_column_count, cell_count);

    auto row = checked_append_xml_element(table, "w:tr");
    if (row == pugi::xml_node{}) {
        return {};
    }
    for (std::size_t i = 0; i < cell_count; ++i) {
        if (append_cell_node(row) == pugi::xml_node{}) {
            (void)table.remove_child(row);
            return {};
        }
    }

    try {
        const auto staged_layout =
            stage_table_layout(table, required_column_count);
        if (!staged_layout.has_value()) {
            (void)table.remove_child(row);
            return {};
        }
        if (!commit_staged_table_layout(*staged_layout)) {
            (void)table.remove_child(row);
            return {};
        }
    } catch (...) {
        (void)table.remove_child(row);
        throw;
    }
    return row;
}

} // namespace featherdoc::detail
