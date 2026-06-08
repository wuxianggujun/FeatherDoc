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

std::optional<TableCell> TableRow::find_cell_by_grid_column(std::size_t grid_column) {
    const auto match = find_row_cell_covering_column(this->current, grid_column);
    if (match.cell == pugi::xml_node{}) {
        return std::nullopt;
    }

    return TableCell(this->current, match.cell);
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
