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

std::optional<std::size_t> TableCell::column_index() const {
    return cell_column_index(this->current);
}

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

featherdoc::cell_vertical_merge TableCell::vertical_merge() const {
    switch (cell_vertical_merge_state_for(this->current)) {
    case cell_vertical_merge_state::restart:
        return featherdoc::cell_vertical_merge::restart;
    case cell_vertical_merge_state::continue_merge:
        return featherdoc::cell_vertical_merge::continue_merge;
    case cell_vertical_merge_state::none:
        return featherdoc::cell_vertical_merge::none;
    }

    return featherdoc::cell_vertical_merge::none;
}

std::size_t TableCell::row_span() const {
    const auto merge_chain = plan_vertical_merge_chain(this->current);
    if (!merge_chain.has_value()) {
        return 1U;
    }
    return std::max<std::size_t>(1U, merge_chain->cells.size());
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

std::optional<featherdoc::border_inspection_summary>
TableCell::border(featherdoc::cell_border_edge edge) const {
    if (this->current == pugi::xml_node{}) {
        return std::nullopt;
    }

    return read_border_inspection_summary(
        this->current.child("w:tcPr")
            .child("w:tcBorders")
            .child(to_xml_border_name(edge)));
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
