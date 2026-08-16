#include "table_method_dependencies.hpp"

#include <new>

namespace featherdoc {

TableRow::TableRow() = default;

TableRow::TableRow(detail::tracked_xml_node parent, pugi::xml_node current) {
    this->set_parent(std::move(parent));
    this->set_current(current);
}

void TableRow::set_parent(detail::tracked_xml_node node) {
    this->parent = std::move(node);
    this->current = this->parent.child("w:tr");
    this->cell.set_parent(this->current);
}

void TableRow::set_current(pugi::xml_node node) {
    this->current = node;
    this->cell.set_parent(this->current);
}

bool TableRow::valid() const noexcept { return this->current.has_node(); }

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

    auto staged_properties = std::optional<staged_table_child>{};
    const auto rollback = [&]() noexcept {
        if (staged_properties.has_value()) {
            rollback_staged_table_child(*staged_properties);
        }
    };

    try {
        const auto row = this->current.node();
        staged_properties =
            stage_table_child(row, "w:trPr", row.first_child());
        if (!staged_properties.has_value()) {
            return false;
        }

        const auto height_text = std::to_string(height_twips);
        const auto height_rule_text = to_xml_row_height_rule(height_rule);
        const auto row_height = ensure_row_height_node(row);
        ensure_attribute_value(row_height, "w:val", height_text.c_str());
        ensure_attribute_value(row_height, "w:hRule", height_rule_text);
        if (row_height == pugi::xml_node{} ||
            std::string_view{row_height.name()} != "w:trHeight" ||
            row_height.parent() != staged_properties->replacement ||
            count_named_children(staged_properties->replacement,
                                 "w:trHeight") != 1U ||
            std::string_view{row_height.attribute("w:val").value()} !=
                height_text ||
            std::string_view{row_height.attribute("w:hRule").value()} !=
                height_rule_text) {
            rollback();
            return false;
        }

        if (staged_properties->original != pugi::xml_node{} &&
            !this->current.retire_subtree(staged_properties->original)) {
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

    return commit_staged_table_child(*staged_properties);
}

bool TableRow::clear_height() {
    if (this->current == pugi::xml_node{}) {
        return false;
    }

    const auto row_properties = this->current.child("w:trPr");
    if (row_properties == pugi::xml_node{}) {
        return true;
    }

    const auto row_height = row_properties.child("w:trHeight");
    if (row_height == pugi::xml_node{}) {
        return true;
    }

    auto staged_properties = std::optional<staged_table_child>{};
    const auto rollback = [&]() noexcept {
        if (staged_properties.has_value()) {
            rollback_staged_table_child(*staged_properties);
        }
    };

    try {
        const auto row = this->current.node();
        staged_properties =
            stage_table_child(row, "w:trPr", row.first_child());
        if (!staged_properties.has_value()) {
            return false;
        }

        const auto replacement_height =
            staged_properties->replacement.child("w:trHeight");
        if (replacement_height == pugi::xml_node{} ||
            std::string_view{replacement_height.name()} != "w:trHeight" ||
            replacement_height.parent() != staged_properties->replacement ||
            !staged_properties->replacement.remove_child(replacement_height)) {
            rollback();
            return false;
        }

        if (staged_properties->replacement.first_child() == pugi::xml_node{} &&
            staged_properties->replacement.first_attribute() ==
                pugi::xml_attribute{}) {
            if (!row.remove_child(staged_properties->replacement)) {
                rollback();
                return false;
            }
            staged_properties->replacement = {};
        }

        if (staged_properties->original != pugi::xml_node{} &&
            !this->current.retire_subtree(staged_properties->original)) {
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

    return commit_staged_table_child(*staged_properties);
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

    auto staged_properties = std::optional<staged_table_child>{};
    const auto rollback = [&]() noexcept {
        if (staged_properties.has_value()) {
            rollback_staged_table_child(*staged_properties);
        }
    };

    try {
        const auto row = this->current.node();
        staged_properties =
            stage_table_child(row, "w:trPr", row.first_child());
        if (!staged_properties.has_value()) {
            return false;
        }

        const auto cant_split = ensure_row_cant_split_node(row);
        ensure_attribute_value(cant_split, "w:val", "1");
        if (cant_split == pugi::xml_node{} ||
            std::string_view{cant_split.name()} != "w:cantSplit" ||
            cant_split.parent() != staged_properties->replacement ||
            count_named_children(staged_properties->replacement,
                                 "w:cantSplit") != 1U ||
            std::string_view{cant_split.attribute("w:val").value()} != "1") {
            rollback();
            return false;
        }

        if (staged_properties->original != pugi::xml_node{} &&
            !this->current.retire_subtree(staged_properties->original)) {
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

    return commit_staged_table_child(*staged_properties);
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

    auto staged_properties = std::optional<staged_table_child>{};
    const auto rollback = [&]() noexcept {
        if (staged_properties.has_value()) {
            rollback_staged_table_child(*staged_properties);
        }
    };

    try {
        const auto row = this->current.node();
        staged_properties =
            stage_table_child(row, "w:trPr", row.first_child());
        if (!staged_properties.has_value()) {
            return false;
        }

        const auto table_header = ensure_row_header_node(row);
        ensure_attribute_value(table_header, "w:val", "1");
        if (table_header == pugi::xml_node{} ||
            std::string_view{table_header.name()} != "w:tblHeader" ||
            table_header.parent() != staged_properties->replacement ||
            count_named_children(staged_properties->replacement,
                                 "w:tblHeader") != 1U ||
            std::string_view{table_header.attribute("w:val").value()} != "1") {
            rollback();
            return false;
        }

        if (staged_properties->original != pugi::xml_node{} &&
            !this->current.retire_subtree(staged_properties->original)) {
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

    return commit_staged_table_child(*staged_properties);
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

    const auto removed_row = this->current.node();
    const auto next_row = detail::next_named_sibling(removed_row, "w:tr");
    const auto previous_row = detail::previous_named_sibling(removed_row, "w:tr");
    const auto promotions =
        successor_vertical_merge_promotions_for_row_removal(removed_row);
    auto replacements = std::vector<tracked_cell_body_replacement>{};
    replacements.reserve(promotions.size());
    auto staged_properties = std::vector<staged_cell_properties>{};
    staged_properties.reserve(promotions.size());
    auto additional_retirement_roots = std::vector<pugi::xml_node>{};
    additional_retirement_roots.reserve(promotions.size() + 1U);
    additional_retirement_roots.push_back(removed_row);
    try {
        for (const auto &[source_cell, target_cell] : promotions) {
            const auto staged = stage_cell_properties(target_cell);
            if (!staged.has_value()) {
                rollback_staged_cell_properties(staged_properties);
                return false;
            }
            staged_properties.push_back(*staged);
            if (staged->original != pugi::xml_node{}) {
                additional_retirement_roots.push_back(staged->original);
            }

            const auto target_merge =
                ensure_cell_vertical_merge_node(target_cell);
            if (target_merge == pugi::xml_node{} ||
                std::string_view{target_merge.name()} != "w:vMerge") {
                rollback_staged_cell_properties(staged_properties);
                return false;
            }

            ensure_attribute_value(target_merge, "w:val", "restart");
            const auto value_attribute = target_merge.attribute("w:val");
            if (value_attribute == pugi::xml_attribute{} ||
                std::string_view{value_attribute.value()} != "restart") {
                rollback_staged_cell_properties(staged_properties);
                return false;
            }
            replacements.push_back(tracked_cell_body_replacement{
                this->parent.with_node(target_cell), source_cell});
        }

        if (!replace_cell_body_contents(
                this->parent, replacements,
                std::span<const pugi::xml_node>{
                    additional_retirement_roots.data(),
                    additional_retirement_roots.size()})) {
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

    auto table_node = this->parent.node();
    if (!table_node.remove_child(removed_row)) {
        return false;
    }

    this->set_current(next_row != pugi::xml_node{} ? next_row : previous_row);
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
    if (this->parent == pugi::xml_node{}) {
        return {};
    }

    const auto table_column_count = current_table_column_count(this->parent);
    if (!table_column_count.has_value()) {
        return {};
    }

    if (this->current == pugi::xml_node{}) {
        const auto new_row = append_row_node(this->parent.node(), 1U);
        if (new_row == pugi::xml_node{}) {
            return {};
        }
        const auto new_cell = new_row.child("w:tc");
        this->set_current(new_row);
        return TableCell(this->current, new_cell);
    }

    const auto existing_row_column_count =
        current_table_row_column_count(this->current);
    if (!existing_row_column_count.has_value()) {
        return {};
    }
    const auto row_column_count = *existing_row_column_count;

    if (row_column_count >= max_table_grid_columns) {
        return {};
    }

    const auto required_columns =
        std::max(*table_column_count, row_column_count + 1U);
    auto row_node = this->current.node();
    const auto new_cell = append_cell_node(row_node);
    if (new_cell == pugi::xml_node{}) {
        return {};
    }
    try {
        const auto staged_layout =
            stage_table_layout(this->parent.node(), required_columns);
        if (!staged_layout.has_value()) {
            (void)row_node.remove_child(new_cell);
            return {};
        }
        if (!commit_staged_table_layout(*staged_layout)) {
            (void)row_node.remove_child(new_cell);
            return {};
        }
    } catch (...) {
        (void)row_node.remove_child(new_cell);
        throw;
    }

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

    auto replacements = std::vector<tracked_table_cell_text_replacement>{};
    try {
        replacements.reserve(texts.size());
        auto cell_node = this->current.child("w:tc");
        for (std::size_t index = 0U; index < texts.size(); ++index) {
            if (cell_node == pugi::xml_node{}) {
                return false;
            }
            replacements.push_back(tracked_table_cell_text_replacement{
                this->parent.with_node(cell_node), texts[index].c_str()});
            cell_node = detail::next_named_sibling(cell_node, "w:tc");
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

bool TableRow::set_texts(std::initializer_list<std::string> texts) {
    return this->set_texts(std::vector<std::string>{texts});
}

} // namespace featherdoc
