#include "table_method_dependencies.hpp"

#include <new>

namespace featherdoc {

TableCell::TableCell() = default;

TableCell::TableCell(detail::tracked_xml_node parent, pugi::xml_node current) {
    this->set_parent(std::move(parent));
    this->set_current(current);
}

void TableCell::set_parent(detail::tracked_xml_node node) {
    this->parent = std::move(node);
    this->current = this->parent.child("w:tc");
    this->paragraph.set_parent(this->current);
}

void TableCell::set_current(pugi::xml_node node) {
    this->current = node;
    this->paragraph.set_parent(this->current);
}

bool TableCell::valid() const noexcept { return this->current.has_node(); }

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
    return detail::replace_table_cell_text(this->current, text);
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
    const auto column_count = current_table_column_count(table);
    if (!column_count.has_value() || *column_count == 0U) {
        return false;
    }

    const auto next_cell = detail::next_named_sibling(this->current, "w:tc");
    const auto previous_cell = detail::previous_named_sibling(this->current, "w:tc");
    const auto surviving_cell =
        next_cell != pugi::xml_node{} ? next_cell : previous_cell;
    if (surviving_cell == pugi::xml_node{}) {
        return false;
    }
    auto cells_to_remove = std::vector<pugi::xml_node>{};
    cells_to_remove.reserve(removal_plan->targets.size());
    for (const auto &target : removal_plan->targets) {
        cells_to_remove.push_back(target.cell);
    }

    auto staged_properties = std::vector<staged_cell_properties>{};
    auto staged_layout = std::optional<staged_table_layout>{};
    const auto rollback = [&]() noexcept {
        rollback_staged_cell_properties(staged_properties);
        if (staged_layout.has_value()) {
            rollback_staged_table_layout(*staged_layout);
        }
    };
    try {
        staged_layout = stage_table_layout(
            table, *column_count,
            table_grid_edit{table_grid_edit_kind::remove_column,
                            removal_plan->column_index, 0U});
        if (!staged_layout.has_value() ||
            !stage_fixed_layout_cell_widths(
                table,
                std::span<const pugi::xml_node>{cells_to_remove.data(),
                                                cells_to_remove.size()},
                staged_properties)) {
            rollback();
            return false;
        }

        auto retirement_roots = std::vector<pugi::xml_node>{};
        retirement_roots.reserve(cells_to_remove.size() +
                                 staged_properties.size() + 2U);
        retirement_roots.insert(retirement_roots.end(),
                                cells_to_remove.begin(),
                                cells_to_remove.end());
        for (const auto &staged : staged_properties) {
            if (staged.original != pugi::xml_node{}) {
                retirement_roots.push_back(staged.original);
            }
        }
        if (staged_layout->original_properties != pugi::xml_node{}) {
            retirement_roots.push_back(staged_layout->original_properties);
        }
        if (staged_layout->original_grid != pugi::xml_node{}) {
            retirement_roots.push_back(staged_layout->original_grid);
        }
        if (!this->parent.retire_subtrees(std::span<const pugi::xml_node>{
                retirement_roots.data(), retirement_roots.size()})) {
            rollback();
            return false;
        }
    } catch (...) {
        rollback();
        throw;
    }

    if (!commit_staged_cell_properties(staged_properties) ||
        !commit_staged_table_layout(*staged_layout)) {
        return false;
    }
    for (const auto &target : removal_plan->targets) {
        auto target_row = target.row;
        if (!target_row.remove_child(target.cell)) {
            return false;
        }
    }

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

    if (inserted_current_row_cell == pugi::xml_node{}) {
        rollback_inserted_table_cells(inserted_cells);
        return {};
    }

    auto staged_properties = std::vector<staged_cell_properties>{};
    auto staged_layout = std::optional<staged_table_layout>{};
    const auto rollback = [&]() noexcept {
        rollback_staged_cell_properties(staged_properties);
        if (staged_layout.has_value()) {
            rollback_staged_table_layout(*staged_layout);
        }
        rollback_inserted_table_cells(inserted_cells);
    };
    try {
        staged_layout = stage_table_layout(
            table, insertion_plan->column_count_before_insertion,
            table_grid_edit{table_grid_edit_kind::insert_column,
                            insertion_plan->boundary_column_index,
                            insertion_plan->grid_width_source_column_index});
        if (!staged_layout.has_value() ||
            !stage_fixed_layout_cell_widths(table, {}, staged_properties)) {
            rollback();
            return {};
        }
    } catch (...) {
        rollback();
        throw;
    }
    if (!commit_staged_cell_properties(staged_properties) ||
        !commit_staged_table_layout(*staged_layout)) {
        return {};
    }

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

    if (inserted_current_row_cell == pugi::xml_node{}) {
        rollback_inserted_table_cells(inserted_cells);
        return {};
    }

    auto staged_properties = std::vector<staged_cell_properties>{};
    auto staged_layout = std::optional<staged_table_layout>{};
    const auto rollback = [&]() noexcept {
        rollback_staged_cell_properties(staged_properties);
        if (staged_layout.has_value()) {
            rollback_staged_table_layout(*staged_layout);
        }
        rollback_inserted_table_cells(inserted_cells);
    };
    try {
        staged_layout = stage_table_layout(
            table, insertion_plan->column_count_before_insertion,
            table_grid_edit{table_grid_edit_kind::insert_column,
                            insertion_plan->boundary_column_index,
                            insertion_plan->grid_width_source_column_index});
        if (!staged_layout.has_value() ||
            !stage_fixed_layout_cell_widths(table, {}, staged_properties)) {
            rollback();
            return {};
        }
    } catch (...) {
        rollback();
        throw;
    }
    if (!commit_staged_cell_properties(staged_properties) ||
        !commit_staged_table_layout(*staged_layout)) {
        return {};
    }

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

    auto staged_properties = std::vector<staged_cell_properties>{};
    const auto rollback = [&]() noexcept {
        rollback_staged_cell_properties(staged_properties);
    };

    try {
        staged_properties.reserve(1U);
        const auto staged = stage_cell_properties(this->current.node());
        if (!staged.has_value()) {
            return false;
        }
        staged_properties.push_back(*staged);

        const auto width_node = ensure_cell_width_node(this->current.node());
        const auto width_text = std::to_string(width_twips);
        if (width_node == pugi::xml_node{} ||
            std::string_view{width_node.name()} != "w:tcW" ||
            width_node.parent() != staged_properties.front().replacement) {
            rollback();
            return false;
        }
        ensure_attribute_value(width_node, "w:w", width_text.c_str());
        ensure_attribute_value(width_node, "w:type", "dxa");
        if (std::string_view{width_node.attribute("w:w").value()} !=
                width_text ||
            std::string_view{width_node.attribute("w:type").value()} !=
                "dxa") {
            rollback();
            return false;
        }

        if (staged_properties.front().original != pugi::xml_node{} &&
            !this->current.retire_subtree(staged_properties.front().original)) {
            rollback();
            return false;
        }
    } catch (const std::bad_alloc &) {
        rollback();
        return false;
    } catch (...) {
        rollback();
        throw;
    }

    return commit_staged_cell_properties(staged_properties);
}

bool TableCell::clear_width() {
    if (this->current == pugi::xml_node{}) {
        return false;
    }

    const auto cell_properties = this->current.child("w:tcPr");
    if (cell_properties == pugi::xml_node{}) {
        return true;
    }

    const auto width_node = cell_properties.child("w:tcW");
    if (width_node == pugi::xml_node{}) {
        return true;
    }

    auto staged_properties = std::vector<staged_cell_properties>{};
    const auto rollback = [&]() noexcept {
        rollback_staged_cell_properties(staged_properties);
    };

    try {
        staged_properties.reserve(1U);
        const auto staged = stage_cell_properties(this->current.node());
        if (!staged.has_value()) {
            return false;
        }
        staged_properties.push_back(*staged);

        const auto replacement_width =
            staged_properties.front().replacement.child("w:tcW");
        if (replacement_width == pugi::xml_node{} ||
            replacement_width.parent() !=
                staged_properties.front().replacement ||
            !staged_properties.front().replacement.remove_child(
                replacement_width)) {
            rollback();
            return false;
        }

        if (staged_properties.front().original != pugi::xml_node{} &&
            !this->current.retire_subtree(staged_properties.front().original)) {
            rollback();
            return false;
        }
    } catch (const std::bad_alloc &) {
        rollback();
        return false;
    } catch (...) {
        rollback();
        throw;
    }

    return commit_staged_cell_properties(staged_properties);
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

    const auto column_count = current_table_column_count(table);
    if (!column_count.has_value()) {
        return false;
    }

    std::vector<pugi::xml_node> cells_to_remove;

    std::size_t added_span = 0U;
    auto next_cell = detail::next_named_sibling(this->current, "w:tc");
    for (std::size_t i = 0; i < additional_cells; ++i) {
        if (next_cell == pugi::xml_node{}) {
            return false;
        }

        const auto next_span = cell_column_span(next_cell);
        if (next_span > max_table_grid_columns - added_span) {
            return false;
        }
        added_span += next_span;
        cells_to_remove.push_back(next_cell);
        next_cell = detail::next_named_sibling(next_cell, "w:tc");
    }

    const auto current_span = cell_column_span(this->current);
    if (current_span > max_table_grid_columns - added_span) {
        return false;
    }

    const auto merged_span = std::to_string(current_span + added_span);
    auto staged_properties = std::vector<staged_cell_properties>{};
    staged_properties.reserve(1U);
    auto staged_layout = std::optional<staged_table_layout>{};
    const auto rollback = [&]() noexcept {
        rollback_staged_cell_properties(staged_properties);
        if (staged_layout.has_value()) {
            rollback_staged_table_layout(*staged_layout);
        }
    };
    try {
        const auto staged_anchor = stage_cell_properties(this->current.node());
        if (!staged_anchor.has_value()) {
            return false;
        }
        staged_properties.push_back(*staged_anchor);

        const auto grid_span_node = ensure_cell_grid_span_node(this->current);
        if (grid_span_node == pugi::xml_node{} ||
            std::string_view{grid_span_node.name()} != "w:gridSpan") {
            rollback();
            return false;
        }
        ensure_attribute_value(grid_span_node, "w:val", merged_span.c_str());
        if (const auto span_attribute = grid_span_node.attribute("w:val");
            span_attribute == pugi::xml_attribute{} ||
            std::string_view{span_attribute.value()} != merged_span) {
            rollback();
            return false;
        }

        staged_layout = stage_table_layout(table, *column_count);
        if (!staged_layout.has_value() ||
            !stage_fixed_layout_cell_widths(
                table,
                std::span<const pugi::xml_node>{cells_to_remove.data(),
                                                cells_to_remove.size()},
                staged_properties)) {
            rollback();
            return false;
        }

        auto retirement_roots = std::vector<pugi::xml_node>{};
        retirement_roots.reserve(cells_to_remove.size() +
                                 staged_properties.size() + 2U);
        retirement_roots.insert(retirement_roots.end(),
                                cells_to_remove.begin(),
                                cells_to_remove.end());
        for (const auto &staged : staged_properties) {
            if (staged.original != pugi::xml_node{}) {
                retirement_roots.push_back(staged.original);
            }
        }
        if (staged_layout->original_properties != pugi::xml_node{}) {
            retirement_roots.push_back(staged_layout->original_properties);
        }
        if (staged_layout->original_grid != pugi::xml_node{}) {
            retirement_roots.push_back(staged_layout->original_grid);
        }
        if (!this->parent.retire_subtrees(std::span<const pugi::xml_node>{
                retirement_roots.data(), retirement_roots.size()})) {
            rollback();
            return false;
        }
    } catch (...) {
        rollback();
        throw;
    }
    if (!commit_staged_cell_properties(staged_properties) ||
        !commit_staged_table_layout(*staged_layout)) {
        return false;
    }
    auto row_node = this->parent.node();
    for (const auto cell : cells_to_remove) {
        if (!row_node.remove_child(cell)) {
            return false;
        }
    }

    return true;
}

bool TableCell::merge_down(std::size_t additional_rows) {
    if (this->current == pugi::xml_node{} || this->parent == pugi::xml_node{}) {
        return false;
    }

    if (additional_rows == 0U) {
        return true;
    }

    const auto table = this->parent.parent();
    if (table == pugi::xml_node{} ||
        !current_table_column_count(table).has_value()) {
        return false;
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

    auto tracked_target_cells = std::vector<detail::tracked_xml_node>{};
    tracked_target_cells.reserve(target_cells.size());
    for (const auto target_cell : target_cells) {
        tracked_target_cells.push_back(this->parent.with_node(target_cell));
    }

    auto staged_properties = std::vector<staged_cell_properties>{};
    staged_properties.reserve(target_cells.size() + 1U);
    const auto stage_vertical_merge_value =
        [&](pugi::xml_node cell, const char *value) -> bool {
        const auto staged = stage_cell_properties(cell);
        if (!staged.has_value()) {
            return false;
        }
        staged_properties.push_back(*staged);
        const auto vertical_merge_node = ensure_cell_vertical_merge_node(cell);
        if (vertical_merge_node == pugi::xml_node{} ||
            std::string_view{vertical_merge_node.name()} != "w:vMerge") {
            return false;
        }
        ensure_attribute_value(vertical_merge_node, "w:val", value);
        const auto value_attribute = vertical_merge_node.attribute("w:val");
        return value_attribute != pugi::xml_attribute{} &&
               std::string_view{value_attribute.value()} == value;
    };

    if (!stage_vertical_merge_value(this->current.node(), "restart")) {
        rollback_staged_cell_properties(staged_properties);
        return false;
    }
    for (const auto target_cell : target_cells) {
        if (!stage_vertical_merge_value(target_cell, "continue")) {
            rollback_staged_cell_properties(staged_properties);
            return false;
        }
    }

    try {
        auto property_retirement_roots = std::vector<pugi::xml_node>{};
        property_retirement_roots.reserve(staged_properties.size());
        for (const auto &staged : staged_properties) {
            if (staged.original != pugi::xml_node{}) {
                property_retirement_roots.push_back(staged.original);
            }
        }
        if (!clear_cell_contents_for_vertical_merge(
                tracked_target_cells,
                std::span<const pugi::xml_node>{
                    property_retirement_roots.data(),
                    property_retirement_roots.size()})) {
            rollback_staged_cell_properties(staged_properties);
            return false;
        }
    } catch (...) {
        rollback_staged_cell_properties(staged_properties);
        throw;
    }
    if (!commit_staged_cell_properties(staged_properties)) {
        return false;
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

    const auto column_count = current_table_column_count(table);
    if (!column_count.has_value()) {
        return false;
    }

    if (cell_vertical_merge_state_for(this->current) != cell_vertical_merge_state::none) {
        return false;
    }

    const auto span = cell_column_span(this->current);
    if (span <= 1U) {
        return false;
    }

    const auto insert_before = this->current.next_sibling();
    auto inserted_cells = std::vector<pugi::xml_node>{};
    auto staged_properties = std::vector<staged_cell_properties>{};
    auto staged_layout = std::optional<staged_table_layout>{};
    const auto rollback = [&]() noexcept {
        rollback_staged_cell_properties(staged_properties);
        if (staged_layout.has_value()) {
            rollback_staged_table_layout(*staged_layout);
        }
        rollback_inserted_table_cells(inserted_cells);
    };

    try {
        inserted_cells.reserve(span - 1U);
        for (std::size_t i = 0U; i + 1U < span; ++i) {
            const auto inserted_cell = insert_empty_clone_cell(
                this->parent, this->current, insert_before);
            if (inserted_cell == pugi::xml_node{}) {
                rollback();
                return false;
            }
            inserted_cells.push_back(inserted_cell);
        }

        staged_properties.reserve(inserted_cells.size() + 1U);
        const auto staged_anchor =
            stage_cell_properties(this->current.node());
        if (!staged_anchor.has_value()) {
            rollback();
            return false;
        }
        staged_properties.push_back(*staged_anchor);

        auto replacement_properties =
            staged_properties.front().replacement;
        const auto replacement_grid_span =
            replacement_properties.child("w:gridSpan");
        if (std::string_view{replacement_properties.name()} != "w:tcPr" ||
            replacement_properties.parent() != this->current.node() ||
            replacement_grid_span == pugi::xml_node{} ||
            std::string_view{replacement_grid_span.name()} != "w:gridSpan" ||
            replacement_grid_span.parent() != replacement_properties ||
            !replacement_properties.remove_child(replacement_grid_span)) {
            rollback();
            return false;
        }

        if (detail::table_uses_fixed_layout(table)) {
            const auto layout_column_count = std::max(
                *column_count,
                count_named_children(table.child("w:tblGrid"),
                                     "w:gridCol"));
            staged_layout = stage_table_layout(table, layout_column_count);
            if (!staged_layout.has_value()) {
                rollback();
                return false;
            }
        }
        if (!stage_fixed_layout_cell_widths(table, {}, staged_properties)) {
            rollback();
            return false;
        }

        replacement_properties = staged_properties.front().replacement;
        if (replacement_properties.first_child() == pugi::xml_node{} &&
            replacement_properties.first_attribute() ==
                pugi::xml_attribute{}) {
            auto anchor_cell = this->current.node();
            if (!anchor_cell.remove_child(replacement_properties)) {
                rollback();
                return false;
            }
            staged_properties.front().replacement = {};
        }

        auto retirement_roots = std::vector<pugi::xml_node>{};
        retirement_roots.reserve(staged_properties.size() + 2U);
        for (const auto &staged : staged_properties) {
            if (staged.original != pugi::xml_node{}) {
                retirement_roots.push_back(staged.original);
            }
        }
        if (staged_layout.has_value()) {
            if (staged_layout->original_properties != pugi::xml_node{}) {
                retirement_roots.push_back(
                    staged_layout->original_properties);
            }
            if (staged_layout->original_grid != pugi::xml_node{}) {
                retirement_roots.push_back(staged_layout->original_grid);
            }
        }
        if (!this->current.retire_subtrees(
                std::span<const pugi::xml_node>{retirement_roots.data(),
                                                retirement_roots.size()})) {
            rollback();
            return false;
        }
    } catch (...) {
        rollback();
        throw;
    }

    return commit_staged_cell_properties(staged_properties) &&
           (!staged_layout.has_value() ||
            commit_staged_table_layout(*staged_layout));
}

bool TableCell::unmerge_down() {
    if (this->current == pugi::xml_node{}) {
        return false;
    }

    const auto row = this->current.parent();
    const auto table = row.parent();
    if (row == pugi::xml_node{} || table == pugi::xml_node{} ||
        !current_table_column_count(table).has_value()) {
        return false;
    }

    auto staged_properties = std::vector<staged_cell_properties>{};
    const auto rollback = [&]() noexcept {
        rollback_staged_cell_properties(staged_properties);
    };

    try {
        const auto merge_chain = plan_vertical_merge_chain(this->current);
        if (!merge_chain.has_value()) {
            return false;
        }

        staged_properties.reserve(merge_chain->cells.size());
        for (const auto cell : merge_chain->cells) {
            const auto staged = stage_cell_properties(cell);
            if (!staged.has_value()) {
                rollback();
                return false;
            }
            staged_properties.push_back(*staged);

            auto replacement_properties =
                staged_properties.back().replacement;
            const auto replacement_vertical_merge =
                replacement_properties.child("w:vMerge");
            if (std::string_view{replacement_properties.name()} != "w:tcPr" ||
                replacement_properties.parent() != cell ||
                count_named_children(replacement_properties, "w:vMerge") !=
                    1U ||
                replacement_vertical_merge == pugi::xml_node{} ||
                std::string_view{replacement_vertical_merge.name()} !=
                    "w:vMerge" ||
                replacement_vertical_merge.parent() !=
                    replacement_properties ||
                !replacement_properties.remove_child(
                    replacement_vertical_merge)) {
                rollback();
                return false;
            }

            if (replacement_properties.first_child() == pugi::xml_node{} &&
                replacement_properties.first_attribute() ==
                    pugi::xml_attribute{}) {
                auto mutable_cell = cell;
                if (!mutable_cell.remove_child(replacement_properties)) {
                    rollback();
                    return false;
                }
                staged_properties.back().replacement = {};
            }
        }

        auto retirement_roots = std::vector<pugi::xml_node>{};
        retirement_roots.reserve(staged_properties.size());
        for (const auto &staged : staged_properties) {
            if (staged.original != pugi::xml_node{}) {
                retirement_roots.push_back(staged.original);
            }
        }
        if (!this->current.retire_subtrees(
                std::span<const pugi::xml_node>{retirement_roots.data(),
                                                retirement_roots.size()})) {
            rollback();
            return false;
        }
    } catch (...) {
        rollback();
        throw;
    }

    return commit_staged_cell_properties(staged_properties);
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

    auto staged_properties = std::vector<staged_cell_properties>{};
    const auto rollback = [&]() noexcept {
        rollback_staged_cell_properties(staged_properties);
    };

    try {
        staged_properties.reserve(1U);
        const auto staged = stage_cell_properties(this->current.node());
        if (!staged.has_value()) {
            return false;
        }
        staged_properties.push_back(*staged);

        const auto alignment_text = to_xml_cell_vertical_alignment(alignment);
        const auto vertical_alignment =
            ensure_cell_vertical_alignment_node(this->current.node());
        if (vertical_alignment == pugi::xml_node{} ||
            std::string_view{vertical_alignment.name()} != "w:vAlign" ||
            vertical_alignment.parent() !=
                staged_properties.front().replacement) {
            rollback();
            return false;
        }
        ensure_attribute_value(vertical_alignment, "w:val", alignment_text);
        if (std::string_view{vertical_alignment.attribute("w:val").value()} !=
            alignment_text) {
            rollback();
            return false;
        }

        if (staged_properties.front().original != pugi::xml_node{} &&
            !this->current.retire_subtree(staged_properties.front().original)) {
            rollback();
            return false;
        }
    } catch (const std::bad_alloc &) {
        rollback();
        return false;
    } catch (...) {
        rollback();
        throw;
    }

    return commit_staged_cell_properties(staged_properties);
}

bool TableCell::clear_vertical_alignment() {
    if (this->current == pugi::xml_node{}) {
        return false;
    }

    const auto cell_properties = this->current.child("w:tcPr");
    if (cell_properties == pugi::xml_node{}) {
        return true;
    }

    const auto vertical_alignment = cell_properties.child("w:vAlign");
    if (vertical_alignment == pugi::xml_node{}) {
        return true;
    }

    auto staged_properties = std::vector<staged_cell_properties>{};
    const auto rollback = [&]() noexcept {
        rollback_staged_cell_properties(staged_properties);
    };

    try {
        staged_properties.reserve(1U);
        const auto staged = stage_cell_properties(this->current.node());
        if (!staged.has_value()) {
            return false;
        }
        staged_properties.push_back(*staged);

        const auto replacement_alignment =
            staged_properties.front().replacement.child("w:vAlign");
        if (replacement_alignment == pugi::xml_node{} ||
            replacement_alignment.parent() !=
                staged_properties.front().replacement ||
            !staged_properties.front().replacement.remove_child(
                replacement_alignment)) {
            rollback();
            return false;
        }

        if (staged_properties.front().original != pugi::xml_node{} &&
            !this->current.retire_subtree(staged_properties.front().original)) {
            rollback();
            return false;
        }
    } catch (const std::bad_alloc &) {
        rollback();
        return false;
    } catch (...) {
        rollback();
        throw;
    }

    return commit_staged_cell_properties(staged_properties);
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

    auto staged_properties = std::vector<staged_cell_properties>{};
    const auto rollback = [&]() noexcept {
        rollback_staged_cell_properties(staged_properties);
    };

    try {
        staged_properties.reserve(1U);
        const auto staged = stage_cell_properties(this->current.node());
        if (!staged.has_value()) {
            return false;
        }
        staged_properties.push_back(*staged);

        const auto direction_text = to_xml_cell_text_direction(direction);
        const auto text_direction =
            ensure_cell_text_direction_node(this->current.node());
        if (text_direction == pugi::xml_node{} ||
            std::string_view{text_direction.name()} != "w:textDirection" ||
            text_direction.parent() != staged_properties.front().replacement) {
            rollback();
            return false;
        }
        ensure_attribute_value(text_direction, "w:val", direction_text);
        if (std::string_view{text_direction.attribute("w:val").value()} !=
            direction_text) {
            rollback();
            return false;
        }

        if (staged_properties.front().original != pugi::xml_node{} &&
            !this->current.retire_subtree(staged_properties.front().original)) {
            rollback();
            return false;
        }
    } catch (const std::bad_alloc &) {
        rollback();
        return false;
    } catch (...) {
        rollback();
        throw;
    }

    return commit_staged_cell_properties(staged_properties);
}

bool TableCell::clear_text_direction() {
    if (this->current == pugi::xml_node{}) {
        return false;
    }

    const auto cell_properties = this->current.child("w:tcPr");
    if (cell_properties == pugi::xml_node{}) {
        return true;
    }

    const auto text_direction = cell_properties.child("w:textDirection");
    if (text_direction == pugi::xml_node{}) {
        return true;
    }

    auto staged_properties = std::vector<staged_cell_properties>{};
    const auto rollback = [&]() noexcept {
        rollback_staged_cell_properties(staged_properties);
    };

    try {
        staged_properties.reserve(1U);
        const auto staged = stage_cell_properties(this->current.node());
        if (!staged.has_value()) {
            return false;
        }
        staged_properties.push_back(*staged);

        const auto replacement_direction =
            staged_properties.front().replacement.child("w:textDirection");
        if (replacement_direction == pugi::xml_node{} ||
            replacement_direction.parent() !=
                staged_properties.front().replacement ||
            !staged_properties.front().replacement.remove_child(
                replacement_direction)) {
            rollback();
            return false;
        }

        if (staged_properties.front().original != pugi::xml_node{} &&
            !this->current.retire_subtree(staged_properties.front().original)) {
            rollback();
            return false;
        }
    } catch (const std::bad_alloc &) {
        rollback();
        return false;
    } catch (...) {
        rollback();
        throw;
    }

    return commit_staged_cell_properties(staged_properties);
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

    auto staged_properties = std::vector<staged_cell_properties>{};
    const auto rollback = [&]() noexcept {
        rollback_staged_cell_properties(staged_properties);
    };

    try {
        staged_properties.reserve(1U);
        const auto staged = stage_cell_properties(this->current.node());
        if (!staged.has_value()) {
            return false;
        }
        staged_properties.push_back(*staged);

        const auto fill_text = std::string{fill_color};
        const auto shading = ensure_cell_shading_node(this->current.node());
        ensure_attribute_value(shading, "w:val", "clear");
        ensure_attribute_value(shading, "w:color", "auto");
        ensure_attribute_value(shading, "w:fill", fill_text.c_str());
        if (shading == pugi::xml_node{} ||
            std::string_view{shading.name()} != "w:shd" ||
            shading.parent() != staged_properties.front().replacement ||
            std::string_view{shading.attribute("w:val").value()} != "clear" ||
            std::string_view{shading.attribute("w:color").value()} != "auto" ||
            std::string_view{shading.attribute("w:fill").value()} !=
                fill_text) {
            rollback();
            return false;
        }

        if (staged_properties.front().original != pugi::xml_node{} &&
            !this->current.retire_subtree(staged_properties.front().original)) {
            rollback();
            return false;
        }
    } catch (const std::bad_alloc &) {
        rollback();
        return false;
    } catch (...) {
        rollback();
        throw;
    }

    return commit_staged_cell_properties(staged_properties);
}

bool TableCell::clear_fill_color() {
    if (this->current == pugi::xml_node{}) {
        return false;
    }

    const auto cell_properties = this->current.child("w:tcPr");
    if (cell_properties == pugi::xml_node{}) {
        return true;
    }

    const auto shading = cell_properties.child("w:shd");
    if (shading == pugi::xml_node{}) {
        return true;
    }

    auto staged_properties = std::vector<staged_cell_properties>{};
    const auto rollback = [&]() noexcept {
        rollback_staged_cell_properties(staged_properties);
    };

    try {
        staged_properties.reserve(1U);
        const auto staged = stage_cell_properties(this->current.node());
        if (!staged.has_value()) {
            return false;
        }
        staged_properties.push_back(*staged);

        const auto replacement_shading =
            staged_properties.front().replacement.child("w:shd");
        if (replacement_shading == pugi::xml_node{} ||
            replacement_shading.parent() !=
                staged_properties.front().replacement ||
            !staged_properties.front().replacement.remove_child(
                replacement_shading)) {
            rollback();
            return false;
        }

        if (staged_properties.front().original != pugi::xml_node{} &&
            !this->current.retire_subtree(staged_properties.front().original)) {
            rollback();
            return false;
        }
    } catch (const std::bad_alloc &) {
        rollback();
        return false;
    } catch (...) {
        rollback();
        throw;
    }

    return commit_staged_cell_properties(staged_properties);
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

    auto staged_properties = std::vector<staged_cell_properties>{};
    const auto rollback = [&]() noexcept {
        rollback_staged_cell_properties(staged_properties);
    };

    try {
        staged_properties.reserve(1U);
        const auto staged = stage_cell_properties(this->current.node());
        if (!staged.has_value()) {
            return false;
        }
        staged_properties.push_back(*staged);

        const auto margin_name = to_xml_margin_name(edge);
        const auto margin =
            ensure_cell_margin_node(this->current.node(), margin_name);
        const auto margins = margin.parent();
        if (margins == pugi::xml_node{} ||
            std::string_view{margins.name()} != "w:tcMar" ||
            margins.parent() != staged_properties.front().replacement ||
            margin == pugi::xml_node{} ||
            std::string_view{margin.name()} != margin_name ||
            margin.parent() != margins) {
            rollback();
            return false;
        }

        const auto margin_text = std::to_string(margin_twips);
        ensure_attribute_value(margin, "w:w", margin_text.c_str());
        ensure_attribute_value(margin, "w:type", "dxa");
        if (std::string_view{margin.attribute("w:w").value()} != margin_text ||
            std::string_view{margin.attribute("w:type").value()} != "dxa") {
            rollback();
            return false;
        }

        if (staged_properties.front().original != pugi::xml_node{} &&
            !this->current.retire_subtree(staged_properties.front().original)) {
            rollback();
            return false;
        }
    } catch (const std::bad_alloc &) {
        rollback();
        return false;
    } catch (...) {
        rollback();
        throw;
    }

    return commit_staged_cell_properties(staged_properties);
}

bool TableCell::clear_margin(featherdoc::cell_margin_edge edge) {
    if (this->current == pugi::xml_node{}) {
        return false;
    }

    const auto cell_properties = this->current.child("w:tcPr");
    if (cell_properties == pugi::xml_node{}) {
        return true;
    }

    const auto margins = cell_properties.child("w:tcMar");
    if (margins == pugi::xml_node{}) {
        return true;
    }

    const auto margin_name = to_xml_margin_name(edge);
    if (margins.child(margin_name) == pugi::xml_node{}) {
        return true;
    }

    auto staged_properties = std::vector<staged_cell_properties>{};
    const auto rollback = [&]() noexcept {
        rollback_staged_cell_properties(staged_properties);
    };

    try {
        staged_properties.reserve(1U);
        const auto staged = stage_cell_properties(this->current.node());
        if (!staged.has_value()) {
            return false;
        }
        staged_properties.push_back(*staged);

        auto replacement_properties = staged_properties.front().replacement;
        auto replacement_margins = replacement_properties.child("w:tcMar");
        const auto replacement_margin =
            replacement_margins.child(margin_name);
        if (std::string_view{replacement_properties.name()} != "w:tcPr" ||
            replacement_properties.parent() != this->current.node() ||
            replacement_margins == pugi::xml_node{} ||
            std::string_view{replacement_margins.name()} != "w:tcMar" ||
            replacement_margins.parent() != replacement_properties ||
            replacement_margin == pugi::xml_node{} ||
            std::string_view{replacement_margin.name()} != margin_name ||
            replacement_margin.parent() != replacement_margins ||
            !replacement_margins.remove_child(replacement_margin)) {
            rollback();
            return false;
        }

        if (replacement_margins.first_child() == pugi::xml_node{} &&
            replacement_margins.first_attribute() == pugi::xml_attribute{} &&
            !replacement_properties.remove_child(replacement_margins)) {
            rollback();
            return false;
        }

        if (staged_properties.front().original != pugi::xml_node{} &&
            !this->current.retire_subtree(staged_properties.front().original)) {
            rollback();
            return false;
        }
    } catch (const std::bad_alloc &) {
        rollback();
        return false;
    } catch (...) {
        rollback();
        throw;
    }

    return commit_staged_cell_properties(staged_properties);
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

    auto staged_properties = std::vector<staged_cell_properties>{};
    const auto rollback = [&]() noexcept {
        rollback_staged_cell_properties(staged_properties);
    };

    try {
        staged_properties.reserve(1U);
        const auto staged = stage_cell_properties(this->current.node());
        if (!staged.has_value()) {
            return false;
        }
        staged_properties.push_back(*staged);

        const auto border_name = to_xml_border_name(edge);
        auto cell_borders = ensure_cell_borders_node(this->current.node());
        auto border_node = cell_borders.child(border_name);
        if (border_node == pugi::xml_node{}) {
            border_node = cell_borders.append_child(border_name);
        }

        const auto expected_size = std::to_string(border.size_eighth_points);
        const auto expected_space = std::to_string(border.space_points);
        const auto expected_color = border.color.empty()
                                        ? std::string{"auto"}
                                        : std::string{border.color};
        apply_border_definition(border_node, border);
        if (cell_borders == pugi::xml_node{} ||
            std::string_view{cell_borders.name()} != "w:tcBorders" ||
            cell_borders.parent() != staged_properties.front().replacement ||
            border_node == pugi::xml_node{} ||
            std::string_view{border_node.name()} != border_name ||
            border_node.parent() != cell_borders ||
            std::string_view{border_node.attribute("w:val").value()} !=
                to_xml_border_style(border.style) ||
            std::string_view{border_node.attribute("w:sz").value()} !=
                expected_size ||
            std::string_view{border_node.attribute("w:space").value()} !=
                expected_space ||
            std::string_view{border_node.attribute("w:color").value()} !=
                expected_color) {
            rollback();
            return false;
        }

        if (staged_properties.front().original != pugi::xml_node{} &&
            !this->current.retire_subtree(staged_properties.front().original)) {
            rollback();
            return false;
        }
    } catch (const std::bad_alloc &) {
        rollback();
        return false;
    } catch (...) {
        rollback();
        throw;
    }

    return commit_staged_cell_properties(staged_properties);
}

bool TableCell::clear_border(featherdoc::cell_border_edge edge) {
    if (this->current == pugi::xml_node{}) {
        return false;
    }

    const auto cell_properties = this->current.child("w:tcPr");
    if (cell_properties == pugi::xml_node{}) {
        return true;
    }

    const auto cell_borders = cell_properties.child("w:tcBorders");
    if (cell_borders == pugi::xml_node{}) {
        return true;
    }

    const auto border_name = to_xml_border_name(edge);
    if (cell_borders.child(border_name) == pugi::xml_node{}) {
        return true;
    }

    auto staged_properties = std::vector<staged_cell_properties>{};
    const auto rollback = [&]() noexcept {
        rollback_staged_cell_properties(staged_properties);
    };

    try {
        staged_properties.reserve(1U);
        const auto staged = stage_cell_properties(this->current.node());
        if (!staged.has_value()) {
            return false;
        }
        staged_properties.push_back(*staged);

        auto replacement_properties = staged_properties.front().replacement;
        auto replacement_borders =
            replacement_properties.child("w:tcBorders");
        const auto replacement_border =
            replacement_borders.child(border_name);
        if (std::string_view{replacement_properties.name()} != "w:tcPr" ||
            replacement_properties.parent() != this->current.node() ||
            replacement_borders == pugi::xml_node{} ||
            std::string_view{replacement_borders.name()} != "w:tcBorders" ||
            replacement_borders.parent() != replacement_properties ||
            replacement_border == pugi::xml_node{} ||
            std::string_view{replacement_border.name()} != border_name ||
            replacement_border.parent() != replacement_borders ||
            !replacement_borders.remove_child(replacement_border)) {
            rollback();
            return false;
        }

        if (replacement_borders.first_child() == pugi::xml_node{} &&
            replacement_borders.first_attribute() == pugi::xml_attribute{} &&
            !replacement_properties.remove_child(replacement_borders)) {
            rollback();
            return false;
        }

        if (staged_properties.front().original != pugi::xml_node{} &&
            !this->current.retire_subtree(staged_properties.front().original)) {
            rollback();
            return false;
        }
    } catch (const std::bad_alloc &) {
        rollback();
        return false;
    } catch (...) {
        rollback();
        throw;
    }

    return commit_staged_cell_properties(staged_properties);
}

TableCell &TableCell::next() {
    this->current = detail::next_named_sibling(this->current, "w:tc");
    return *this;
}

bool TableCell::has_next() const { return this->current != pugi::xml_node{}; }

} // namespace featherdoc
