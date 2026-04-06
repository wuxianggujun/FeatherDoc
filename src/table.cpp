#include "featherdoc.hpp"
#include "xml_helpers.hpp"

#include <algorithm>
#include <charconv>

namespace featherdoc {
namespace {

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

void ensure_default_table_properties(pugi::xml_node table) {
    auto table_properties = ensure_table_properties_node(table);
    if (table_properties == pugi::xml_node{}) {
        return;
    }

    auto table_width = table_properties.child("w:tblW");
    if (table_width == pugi::xml_node{}) {
        table_width = table_properties.append_child("w:tblW");
    }
    ensure_attribute_value(table_width, "w:w", "0");
    ensure_attribute_value(table_width, "w:type", "auto");

    auto table_look = table_properties.child("w:tblLook");
    if (table_look == pugi::xml_node{}) {
        table_look = table_properties.append_child("w:tblLook");
    }
    ensure_attribute_value(table_look, "w:val", "04A0");
    ensure_attribute_value(table_look, "w:firstRow", "1");
    ensure_attribute_value(table_look, "w:firstColumn", "1");
    ensure_attribute_value(table_look, "w:lastRow", "0");
    ensure_attribute_value(table_look, "w:lastColumn", "0");
    ensure_attribute_value(table_look, "w:noHBand", "0");
    ensure_attribute_value(table_look, "w:noVBand", "1");
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

Table::Table() = default;

Table::Table(pugi::xml_node parent, pugi::xml_node current) {
    this->set_parent(parent);
    this->set_current(current);
}

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
