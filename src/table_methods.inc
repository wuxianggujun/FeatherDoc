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

#include "table_property_methods.inc"

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

Paragraph Table::insert_paragraph_after(const std::string &text,
                                        featherdoc::formatting_flag formatting) {
    if (this->parent == pugi::xml_node{} || this->current == pugi::xml_node{}) {
        return {};
    }

    const auto paragraph_node =
        detail::insert_paragraph_node(this->parent, this->current.next_sibling());
    auto paragraph = Paragraph(this->parent, paragraph_node);
    if (!text.empty() && !paragraph.add_run(text, formatting).has_next()) {
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
