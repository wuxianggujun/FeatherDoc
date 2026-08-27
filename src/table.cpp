#include "table_method_dependencies.hpp"
#include "xml_document_clone_helpers.hpp"

#include <array>
#include <limits>
#include <new>
#include <span>
#include <string_view>

namespace featherdoc {

namespace {

[[nodiscard]] auto appended_row_is_valid(
    pugi::xml_node table, pugi::xml_node row,
    std::size_t expected_cell_count) noexcept -> bool {
    const auto row_column_count = current_table_row_column_count(row);
    if (table == pugi::xml_node{} || row == pugi::xml_node{} ||
        std::string_view{row.name()} != "w:tr" || row.parent() != table ||
        count_named_children(row, "w:tc") != expected_cell_count ||
        !row_column_count.has_value() ||
        *row_column_count != expected_cell_count) {
        return false;
    }

    for (auto cell = row.child("w:tc"); cell != pugi::xml_node{};
         cell = detail::next_named_sibling(cell, "w:tc")) {
        if (std::string_view{cell.name()} != "w:tc" || cell.parent() != row ||
            count_named_children(cell, "w:tcPr") != 1U ||
            count_named_children(cell.child("w:tcPr"), "w:tcW") != 1U ||
            count_named_children(cell, "w:p") != 1U) {
            return false;
        }
    }
    return true;
}

[[nodiscard]] auto staged_layout_is_valid_for_append(
    const staged_table_layout &staged_layout, pugi::xml_node table,
    std::size_t expected_column_count) noexcept -> bool {
    const auto expected_table_properties_count =
        staged_layout.original_properties != pugi::xml_node{} ? 2U : 1U;
    const auto expected_table_grid_count =
        staged_layout.original_grid != pugi::xml_node{} ? 2U : 1U;
    return staged_layout.table == table &&
           staged_layout.replacement_properties != pugi::xml_node{} &&
           std::string_view{staged_layout.replacement_properties.name()} ==
               "w:tblPr" &&
           staged_layout.replacement_properties.parent() == table &&
           staged_layout.replacement_grid != pugi::xml_node{} &&
           std::string_view{staged_layout.replacement_grid.name()} ==
               "w:tblGrid" &&
           staged_layout.replacement_grid.parent() == table &&
           (staged_layout.original_properties == pugi::xml_node{} ||
            staged_layout.original_properties.parent() == table) &&
           (staged_layout.original_grid == pugi::xml_node{} ||
            staged_layout.original_grid.parent() == table) &&
           count_named_children(table, "w:tblPr") ==
               expected_table_properties_count &&
           count_named_children(table, "w:tblGrid") ==
               expected_table_grid_count &&
           count_named_children(staged_layout.replacement_grid, "w:gridCol") ==
               expected_column_count;
}

} // namespace

Table::Table() = default;

Table::Table(detail::tracked_xml_node parent, pugi::xml_node current) {
    this->set_parent(std::move(parent));
    this->set_current(current);
}

void Table::set_owner(Document *document_owner) { this->owner = document_owner; }

void Table::set_parent(detail::tracked_xml_node node) {
    this->parent = std::move(node);
    this->current = this->parent.child("w:tbl");
    this->row.set_parent(this->current);
}

void Table::set_current(pugi::xml_node node) {
    this->current = node;
    this->row.set_parent(this->current);
}

bool Table::valid() const noexcept { return this->current.has_node(); }

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

std::optional<TableCell> Table::find_cell_by_grid_column(std::size_t row_index,
                                                         std::size_t grid_column) {
    auto row_handle = this->find_row(row_index);
    if (!row_handle.has_value()) {
        return std::nullopt;
    }

    return row_handle->find_cell_by_grid_column(grid_column);
}

bool Table::set_cell_text(std::size_t row_index, std::size_t cell_index,
                          const std::string &text) {
    auto cell_handle = this->find_cell(row_index, cell_index);
    if (!cell_handle.has_value()) {
        return false;
    }

    return cell_handle->set_text(text);
}

bool Table::set_cell_text_by_grid_column(std::size_t row_index, std::size_t grid_column,
                                         const std::string &text) {
    auto cell_handle = this->find_cell_by_grid_column(row_index, grid_column);
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

    auto replacements = std::vector<tracked_table_cell_text_replacement>{};
    try {
        auto replacement_count = std::size_t{0U};
        for (const auto &row_texts : rows) {
            if (row_texts.size() >
                std::numeric_limits<std::size_t>::max() - replacement_count) {
                return false;
            }
            replacement_count += row_texts.size();
        }
        replacements.reserve(replacement_count);

        for (std::size_t row_offset = 0U; row_offset < rows.size();
             ++row_offset) {
            if (row_offset >
                std::numeric_limits<std::size_t>::max() - start_row_index) {
                return false;
            }

            auto row_handle = this->find_row(start_row_index + row_offset);
            if (!row_handle.has_value()) {
                return false;
            }

            const auto &row_texts = rows[row_offset];
            auto cell_count = std::size_t{0U};
            for (auto cell = row_handle->current.child("w:tc");
                 cell != pugi::xml_node{};
                 cell = detail::next_named_sibling(cell, "w:tc")) {
                if (cell_count == row_texts.size()) {
                    return false;
                }
                replacements.push_back(tracked_table_cell_text_replacement{
                    this->parent.with_node(cell),
                    row_texts[cell_count].c_str()});
                ++cell_count;
            }
            if (cell_count != row_texts.size()) {
                return false;
            }
        }
    } catch (const std::bad_alloc &) {
        return false;
    }

    const auto replacement_span =
        std::span<const tracked_table_cell_text_replacement>{
            replacements.data(), replacements.size()};
    if (!replace_table_cell_texts(replacement_span)) {
        return false;
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

    auto replacements = std::vector<tracked_table_cell_text_replacement>{};
    try {
        auto replacement_count = std::size_t{0U};
        for (const auto &row_texts : rows) {
            if (row_texts.size() >
                std::numeric_limits<std::size_t>::max() - replacement_count) {
                return false;
            }
            replacement_count += row_texts.size();
        }
        replacements.reserve(replacement_count);

        for (std::size_t row_offset = 0U; row_offset < rows.size();
             ++row_offset) {
            if (row_offset >
                std::numeric_limits<std::size_t>::max() - start_row_index) {
                return false;
            }

            auto row_handle = this->find_row(start_row_index + row_offset);
            if (!row_handle.has_value()) {
                return false;
            }

            const auto &row_texts = rows[row_offset];
            const auto row_cell_count =
                count_named_children(row_handle->current, "w:tc");
            if (start_cell_index > row_cell_count ||
                row_texts.size() > row_cell_count - start_cell_index) {
                return false;
            }
            if (row_texts.empty()) {
                continue;
            }

            auto cell = row_handle->current.child("w:tc");
            for (std::size_t skipped = 0U; skipped < start_cell_index;
                 ++skipped) {
                if (cell == pugi::xml_node{}) {
                    return false;
                }
                cell = detail::next_named_sibling(cell, "w:tc");
            }

            for (std::size_t cell_offset = 0U; cell_offset < row_texts.size();
                 ++cell_offset) {
                if (cell == pugi::xml_node{}) {
                    return false;
                }
                replacements.push_back(tracked_table_cell_text_replacement{
                    this->parent.with_node(cell),
                    row_texts[cell_offset].c_str()});
                cell = detail::next_named_sibling(cell, "w:tc");
            }
        }
    } catch (const std::bad_alloc &) {
        return false;
    }

    const auto replacement_span =
        std::span<const tracked_table_cell_text_replacement>{
            replacements.data(), replacements.size()};
    if (!replace_table_cell_texts(replacement_span)) {
        return false;
    }

    return true;
}

bool Table::set_cell_block_texts(
    std::size_t start_row_index, std::size_t start_cell_index,
    std::initializer_list<std::initializer_list<std::string>> rows) {
    return this->set_cell_block_texts(start_row_index, start_cell_index,
                                      string_matrix_from_initializer_list(rows));
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

    if (row_count == 0U || column_count == 0U ||
        column_count > max_table_grid_columns) {
        return {};
    }

    auto parent_node = this->parent.node();
    const auto table_node = detail::insert_table_node(parent_node, this->current);
    if (table_node == pugi::xml_node{}) {
        return {};
    }
    try {
        for (std::size_t row_index = 0; row_index < row_count; ++row_index) {
            if (append_row_node(table_node, column_count) ==
                pugi::xml_node{}) {
                (void)parent_node.remove_child(table_node);
                return {};
            }
        }
    } catch (...) {
        (void)parent_node.remove_child(table_node);
        throw;
    }

    auto created_table = Table(this->parent, table_node);
    created_table.set_owner(this->owner);
    this->set_current(table_node);
    return created_table;
}

Table Table::insert_table_after(std::size_t row_count, std::size_t column_count) {
    if (this->parent == pugi::xml_node{} || this->current == pugi::xml_node{}) {
        return {};
    }

    if (row_count == 0U || column_count == 0U ||
        column_count > max_table_grid_columns) {
        return {};
    }

    const auto next_sibling = this->current.next_sibling();
    auto parent_node = this->parent.node();
    const auto table_node =
        detail::insert_table_node(parent_node, next_sibling);
    if (table_node == pugi::xml_node{}) {
        return {};
    }
    try {
        for (std::size_t row_index = 0; row_index < row_count; ++row_index) {
            if (append_row_node(table_node, column_count) ==
                pugi::xml_node{}) {
                (void)parent_node.remove_child(table_node);
                return {};
            }
        }
    } catch (...) {
        (void)parent_node.remove_child(table_node);
        throw;
    }

    auto created_table = Table(this->parent, table_node);
    created_table.set_owner(this->owner);
    this->set_current(table_node);
    return created_table;
}

Paragraph Table::insert_paragraph_after(const std::string &text,
                                        featherdoc::formatting_flag formatting) {
    if (this->parent == pugi::xml_node{} || this->current == pugi::xml_node{}) {
        return {};
    }

    auto parent_node = this->parent.node();
    const auto paragraph_node = detail::insert_paragraph_node(
        parent_node, this->current.next_sibling());
    if (paragraph_node == pugi::xml_node{}) {
        return {};
    }
    auto paragraph = Paragraph(this->parent, paragraph_node);
    if (!text.empty() && !paragraph.add_run(text, formatting).has_next()) {
        // The paragraph has not escaped yet, so raw removal safely rolls the
        // complete insertion back without allocating retirement metadata.
        (void)parent_node.remove_child(paragraph_node);
        return {};
    }
    return paragraph;
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
    if (cell_count == 0U || cell_count > max_table_grid_columns) {
        return {};
    }

    auto table_node = this->current.node();
    auto created_table = false;
    if (table_node == pugi::xml_node{} && this->parent != pugi::xml_node{}) {
        table_node = detail::append_table_node(this->parent.node());
        created_table = table_node != pugi::xml_node{};
    }
    if (table_node == pugi::xml_node{}) {
        return {};
    }

    if (!table_geometry_is_valid_for_append(table_node)) {
        if (created_table) {
            (void)this->parent.node().remove_child(table_node);
        }
        return {};
    }

    if (created_table) {
        auto new_row = pugi::xml_node{};
        try {
            new_row = append_row_node(table_node, cell_count);
        } catch (...) {
            (void)this->parent.node().remove_child(table_node);
            throw;
        }
        if (new_row == pugi::xml_node{}) {
            (void)this->parent.node().remove_child(table_node);
            return {};
        }

        this->set_current(table_node);
        this->row.set_current(new_row);
        return TableRow(this->current, new_row);
    }

    if (table_node.parent() != this->parent.node()) {
        return {};
    }

    const auto current_column_count = current_table_column_count(table_node);
    if (!current_column_count.has_value()) {
        return {};
    }
    const auto existing_row_count = count_named_children(table_node, "w:tr");
    const auto required_column_count =
        std::max(*current_column_count, cell_count);

    auto new_row = pugi::xml_node{};
    auto staged_layout = std::optional<staged_table_layout>{};
    const auto rollback = [&]() noexcept {
        if (staged_layout.has_value()) {
            rollback_staged_table_layout(*staged_layout);
        }
        if (new_row != pugi::xml_node{}) {
            (void)table_node.remove_child(new_row);
        }
    };

    try {
        new_row = detail::checked_append_xml_element(table_node, "w:tr");
        if (new_row == pugi::xml_node{}) {
            return {};
        }
        for (std::size_t cell_index = 0U; cell_index < cell_count;
             ++cell_index) {
            if (append_cell_node(new_row) == pugi::xml_node{}) {
                rollback();
                return {};
            }
        }

        staged_layout = stage_table_layout(table_node, required_column_count);
        if (!staged_layout.has_value() ||
            count_named_children(table_node, "w:tr") != existing_row_count + 1U ||
            !appended_row_is_valid(table_node, new_row, cell_count) ||
            !staged_layout_is_valid_for_append(
                *staged_layout, table_node, required_column_count)) {
            rollback();
            return {};
        }

        auto retirement_roots = std::array<pugi::xml_node, 2U>{};
        auto retirement_root_count = std::size_t{0U};
        if (staged_layout->original_properties != pugi::xml_node{}) {
            retirement_roots[retirement_root_count++] =
                staged_layout->original_properties;
        }
        if (staged_layout->original_grid != pugi::xml_node{}) {
            retirement_roots[retirement_root_count++] =
                staged_layout->original_grid;
        }
        if (retirement_root_count > 0U &&
            !this->current.retire_subtrees(std::span<const pugi::xml_node>{
                retirement_roots.data(), retirement_root_count})) {
            rollback();
            return {};
        }
    } catch (...) {
        rollback();
        throw;
    }

    if (!commit_staged_table_layout(*staged_layout)) {
        return {};
    }

    this->row.set_current(new_row);
    return TableRow(this->current, new_row);
}

} // namespace featherdoc
