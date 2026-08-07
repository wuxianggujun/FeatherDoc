#include "table_method_dependencies.hpp"
#include "xml_document_clone_helpers.hpp"

#include <array>
#include <new>
#include <span>
#include <vector>

namespace featherdoc {

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

    auto staged_properties = std::optional<staged_table_child>{};
    const auto rollback = [&]() noexcept {
        if (staged_properties.has_value()) {
            rollback_staged_table_child(*staged_properties);
        }
    };

    try {
        const auto table = this->current.node();
        staged_properties =
            stage_table_child(table, "w:tblPr", table.first_child());
        if (!staged_properties.has_value()) {
            return false;
        }

        const auto width_node = ensure_table_width_node(table);
        const auto width_text = std::to_string(width_twips);
        if (width_node == pugi::xml_node{} ||
            std::string_view{width_node.name()} != "w:tblW" ||
            width_node.parent() != staged_properties->replacement ||
            !detail::checked_set_xml_attribute_value(width_node, "w:w",
                                                     width_text) ||
            !detail::checked_set_xml_attribute_value(width_node, "w:type",
                                                     "dxa")) {
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
    if (!column_count.has_value() || column_index >= *column_count) {
        return std::nullopt;
    }

    const auto grid_column = find_table_grid_column(this->current, column_index);
    if (grid_column == pugi::xml_node{} || grid_column.attribute("w:w") == pugi::xml_attribute{}) {
        return std::nullopt;
    }

    return parse_unsigned_attribute(grid_column, "w:w");
}

bool Table::set_column_width_twips(std::size_t column_index,
                                   std::uint32_t width_twips) {
    if (this->current == pugi::xml_node{}) {
        return false;
    }

    const auto column_count = current_table_column_count(this->current);
    if (!column_count.has_value() || column_index >= *column_count) {
        return false;
    }

    auto staged_layout = std::optional<staged_table_layout>{};
    auto staged_properties = std::vector<staged_cell_properties>{};
    const auto rollback = [&]() noexcept {
        rollback_staged_cell_properties(staged_properties);
        if (staged_layout.has_value()) {
            rollback_staged_table_layout(*staged_layout);
        }
    };

    try {
        staged_layout = stage_table_layout(this->current.node(), *column_count);
        if (!staged_layout.has_value()) {
            rollback();
            return false;
        }

        const auto grid_column =
            find_table_grid_column(this->current.node(), column_index);
        const auto width_text = std::to_string(width_twips);
        if (grid_column == pugi::xml_node{} ||
            grid_column.parent() != staged_layout->replacement_grid ||
            !detail::checked_set_xml_attribute_value(grid_column, "w:w",
                                                     width_text) ||
            !stage_fixed_layout_cell_widths(this->current.node(), {},
                                            staged_properties)) {
            rollback();
            return false;
        }

        auto retirement_roots = std::vector<pugi::xml_node>{};
        retirement_roots.reserve(staged_properties.size() + 2U);
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
        if (!this->current.retire_subtrees(std::span<const pugi::xml_node>{
                retirement_roots.data(), retirement_roots.size()})) {
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

    return commit_staged_cell_properties(staged_properties) &&
           commit_staged_table_layout(*staged_layout);
}

bool Table::clear_column_width(std::size_t column_index) {
    if (this->current == pugi::xml_node{}) {
        return false;
    }

    const auto column_count = current_table_column_count(this->current);
    if (!column_count.has_value() || column_index >= *column_count) {
        return false;
    }

    const auto grid_column =
        find_table_grid_column(this->current, column_index);
    if (grid_column == pugi::xml_node{} ||
        grid_column.attribute("w:w") == pugi::xml_attribute{}) {
        return true;
    }

    auto staged_grid = std::optional<staged_table_child>{};
    auto staged_properties = std::vector<staged_cell_properties>{};
    const auto rollback = [&]() noexcept {
        rollback_staged_cell_properties(staged_properties);
        if (staged_grid.has_value()) {
            rollback_staged_table_child(*staged_grid);
        }
    };

    try {
        const auto table = this->current.node();
        staged_grid = stage_table_child(table, "w:tblGrid", {});
        if (!staged_grid.has_value()) {
            rollback();
            return false;
        }

        auto replacement_grid_column =
            find_table_grid_column(table, column_index);
        if (replacement_grid_column == pugi::xml_node{} ||
            replacement_grid_column.parent() != staged_grid->replacement ||
            !replacement_grid_column.remove_attribute("w:w") ||
            !stage_cleared_fixed_layout_cell_widths_covering_column(
                table, column_index, staged_properties)) {
            rollback();
            return false;
        }

        auto retirement_roots = std::vector<pugi::xml_node>{};
        retirement_roots.reserve(staged_properties.size() + 1U);
        for (const auto &staged : staged_properties) {
            if (staged.original != pugi::xml_node{}) {
                retirement_roots.push_back(staged.original);
            }
        }
        if (staged_grid->original != pugi::xml_node{}) {
            retirement_roots.push_back(staged_grid->original);
        }
        if (!this->current.retire_subtrees(std::span<const pugi::xml_node>{
                retirement_roots.data(), retirement_roots.size()})) {
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

    return commit_staged_cell_properties(staged_properties) &&
           commit_staged_table_child(*staged_grid);
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

    const auto column_count = current_table_column_count(this->current);
    const auto synchronize_cell_widths =
        layout_mode == featherdoc::table_layout_mode::fixed &&
        column_count.has_value() && *column_count > 0U;
    auto staged_layout = std::optional<staged_table_layout>{};
    auto staged_table_properties = std::optional<staged_table_child>{};
    auto staged_cells = std::vector<staged_cell_properties>{};
    const auto rollback = [&]() noexcept {
        rollback_staged_cell_properties(staged_cells);
        if (staged_layout.has_value()) {
            rollback_staged_table_layout(*staged_layout);
        }
        if (staged_table_properties.has_value()) {
            rollback_staged_table_child(*staged_table_properties);
        }
    };

    try {
        const auto table = this->current.node();
        if (synchronize_cell_widths) {
            staged_layout = stage_table_layout(table, *column_count);
        } else {
            staged_table_properties =
                stage_table_child(table, "w:tblPr", table.first_child());
        }
        if (!staged_layout.has_value() &&
            !staged_table_properties.has_value()) {
            rollback();
            return false;
        }

        auto replacement_table_properties = pugi::xml_node{};
        if (staged_layout.has_value()) {
            replacement_table_properties =
                staged_layout->replacement_properties;
        } else {
            replacement_table_properties = staged_table_properties->replacement;
        }
        const auto layout_node = ensure_table_layout_node(table);
        if (layout_node == pugi::xml_node{} ||
            std::string_view{layout_node.name()} != "w:tblLayout" ||
            layout_node.parent() != replacement_table_properties ||
            !detail::checked_set_xml_attribute_value(
                layout_node, "w:type", to_xml_table_layout_mode(layout_mode)) ||
            (synchronize_cell_widths &&
             !stage_fixed_layout_cell_widths(table, {}, staged_cells))) {
            rollback();
            return false;
        }

        auto retirement_roots = std::vector<pugi::xml_node>{};
        retirement_roots.reserve(staged_cells.size() + 2U);
        for (const auto &staged : staged_cells) {
            if (staged.original != pugi::xml_node{}) {
                retirement_roots.push_back(staged.original);
            }
        }
        if (staged_layout.has_value()) {
            if (staged_layout->original_properties != pugi::xml_node{}) {
                retirement_roots.push_back(staged_layout->original_properties);
            }
            if (staged_layout->original_grid != pugi::xml_node{}) {
                retirement_roots.push_back(staged_layout->original_grid);
            }
        } else if (staged_table_properties->original != pugi::xml_node{}) {
            retirement_roots.push_back(staged_table_properties->original);
        }
        if (!this->current.retire_subtrees(std::span<const pugi::xml_node>{
                retirement_roots.data(), retirement_roots.size()})) {
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

    if (!commit_staged_cell_properties(staged_cells)) {
        return false;
    }
    return staged_layout.has_value()
               ? commit_staged_table_layout(*staged_layout)
               : commit_staged_table_child(*staged_table_properties);
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

    auto staged_properties = std::optional<staged_table_child>{};
    const auto rollback = [&]() noexcept {
        if (staged_properties.has_value()) {
            rollback_staged_table_child(*staged_properties);
        }
    };

    try {
        const auto table = this->current.node();
        staged_properties =
            stage_table_child(table, "w:tblPr", table.first_child());
        if (!staged_properties.has_value()) {
            return false;
        }

        const auto alignment_node = ensure_table_alignment_node(table);
        if (alignment_node == pugi::xml_node{} ||
            std::string_view{alignment_node.name()} != "w:jc" ||
            alignment_node.parent() != staged_properties->replacement ||
            !detail::checked_set_xml_attribute_value(
                alignment_node, "w:val", to_xml_table_alignment(alignment))) {
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

    auto staged_properties = std::optional<staged_table_child>{};
    const auto rollback = [&]() noexcept {
        if (staged_properties.has_value()) {
            rollback_staged_table_child(*staged_properties);
        }
    };

    try {
        const auto table = this->current.node();
        staged_properties =
            stage_table_child(table, "w:tblPr", table.first_child());
        if (!staged_properties.has_value()) {
            return false;
        }

        const auto indent_node = ensure_table_indent_node(table);
        const auto indent_text = std::to_string(indent_twips);
        if (indent_node == pugi::xml_node{} ||
            std::string_view{indent_node.name()} != "w:tblInd" ||
            indent_node.parent() != staged_properties->replacement ||
            !detail::checked_set_xml_attribute_value(indent_node, "w:w",
                                                     indent_text) ||
            !detail::checked_set_xml_attribute_value(indent_node, "w:type",
                                                     "dxa")) {
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

    auto staged_properties = std::optional<staged_table_child>{};
    const auto rollback = [&]() noexcept {
        if (staged_properties.has_value()) {
            rollback_staged_table_child(*staged_properties);
        }
    };

    try {
        const auto table = this->current.node();
        staged_properties =
            stage_table_child(table, "w:tblPr", table.first_child());
        if (!staged_properties.has_value()) {
            return false;
        }

        const auto spacing = ensure_table_cell_spacing_node(table);
        const auto spacing_text = std::to_string(spacing_twips);
        if (spacing == pugi::xml_node{} ||
            std::string_view{spacing.name()} != "w:tblCellSpacing" ||
            spacing.parent() != staged_properties->replacement ||
            !detail::checked_set_xml_attribute_value(spacing, "w:w",
                                                     spacing_text) ||
            !detail::checked_set_xml_attribute_value(spacing, "w:type",
                                                     "dxa")) {
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

namespace {

struct staged_table_position final {
    pugi::xml_node original_position;
    pugi::xml_node replacement_position;
    pugi::xml_node original_overlap;
    pugi::xml_node replacement_overlap;
};

[[nodiscard]] auto checked_remove_xml_attribute(pugi::xml_node node,
                                                const char *name) -> bool {
    const auto attribute = node.attribute(name);
    return attribute == pugi::xml_attribute{} ||
           node.remove_attribute(attribute);
}

[[nodiscard]] auto
checked_set_optional_unsigned_attribute(pugi::xml_node node, const char *name,
                                        std::optional<std::uint32_t> value)
    -> bool {
    if (!value.has_value()) {
        return checked_remove_xml_attribute(node, name);
    }

    const auto text = std::to_string(*value);
    return detail::checked_set_xml_attribute_value(node, name, text);
}

[[nodiscard]] auto
checked_insert_table_position_node(pugi::xml_node table_properties)
    -> pugi::xml_node {
    const auto insert_before = [&](pugi::xml_node anchor) {
        return detail::checked_insert_xml_element_before(table_properties,
                                                         "w:tblpPr", anchor);
    };
    const auto insert_after = [&](pugi::xml_node anchor) {
        const auto next = anchor.next_sibling();
        return next != pugi::xml_node{} ? insert_before(next)
                                        : detail::checked_append_xml_element(
                                              table_properties, "w:tblpPr");
    };

    if (const auto table_style = table_properties.child("w:tblStyle");
        table_style != pugi::xml_node{}) {
        return insert_after(table_style);
    }
    if (const auto first_child = table_properties.first_child();
        first_child != pugi::xml_node{}) {
        return insert_before(first_child);
    }
    return detail::checked_append_xml_element(table_properties, "w:tblpPr");
}

[[nodiscard]] auto
checked_insert_table_overlap_node(pugi::xml_node table_properties,
                                  pugi::xml_node position_node)
    -> pugi::xml_node {
    const auto next = position_node.next_sibling();
    if (next != pugi::xml_node{}) {
        return detail::checked_insert_xml_element_before(table_properties,
                                                         "w:tblOverlap", next);
    }
    return detail::checked_append_xml_element(table_properties, "w:tblOverlap");
}

[[nodiscard]] auto checked_copy_xml_node_contents(pugi::xml_node source,
                                                  pugi::xml_node destination)
    -> bool {
    if (!detail::xml_document_clone_detail::copy_node_contents(source,
                                                               destination)) {
        return false;
    }
    for (auto child = source.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        if (detail::checked_append_copy_xml_node(child, destination) !=
            detail::xml_document_clone_status::success) {
            return false;
        }
    }
    return true;
}

[[nodiscard]] auto has_duplicate_child(pugi::xml_node parent, const char *name)
    -> bool {
    const auto first = parent.child(name);
    return first != pugi::xml_node{} &&
           first.next_sibling(name) != pugi::xml_node{};
}

[[nodiscard]] auto stage_table_position_nodes(pugi::xml_node table_properties,
                                              bool stage_overlap)
    -> std::optional<staged_table_position> {
    if (has_duplicate_child(table_properties, "w:tblpPr") ||
        has_duplicate_child(table_properties, "w:tblOverlap")) {
        return std::nullopt;
    }

    auto staged = staged_table_position{table_properties.child("w:tblpPr"),
                                        {},
                                        table_properties.child("w:tblOverlap"),
                                        {}};
    const auto rollback = [&]() noexcept {
        if (staged.replacement_overlap != pugi::xml_node{}) {
            (void)table_properties.remove_child(staged.replacement_overlap);
        }
        if (staged.replacement_position != pugi::xml_node{}) {
            (void)table_properties.remove_child(staged.replacement_position);
        }
    };

    try {
        staged.replacement_position =
            checked_insert_table_position_node(table_properties);
        if (staged.replacement_position != pugi::xml_node{} &&
            staged.original_position != pugi::xml_node{} &&
            !checked_copy_xml_node_contents(staged.original_position,
                                            staged.replacement_position)) {
            rollback();
            return std::nullopt;
        }
        if (staged.replacement_position == pugi::xml_node{}) {
            rollback();
            return std::nullopt;
        }

        if (stage_overlap) {
            staged.replacement_overlap = checked_insert_table_overlap_node(
                table_properties, staged.replacement_position);
            if (staged.replacement_overlap == pugi::xml_node{}) {
                rollback();
                return std::nullopt;
            }
            if (staged.original_overlap != pugi::xml_node{} &&
                !checked_copy_xml_node_contents(staged.original_overlap,
                                                staged.replacement_overlap)) {
                rollback();
                return std::nullopt;
            }
        }
    } catch (...) {
        rollback();
        throw;
    }

    return staged;
}

[[nodiscard]] auto
apply_table_position_attributes(pugi::xml_node position_node,
                                const featherdoc::table_position &position)
    -> bool {
    const auto horizontal_offset =
        std::to_string(position.horizontal_offset_twips);
    const auto vertical_offset = std::to_string(position.vertical_offset_twips);

    if (!detail::checked_set_xml_attribute_value(
            position_node, "w:horzAnchor",
            to_xml_table_position_horizontal_reference(
                position.horizontal_reference)) ||
        !detail::checked_set_xml_attribute_value(position_node, "w:tblpX",
                                                 horizontal_offset)) {
        return false;
    }
    if (position.horizontal_spec.has_value()) {
        if (!detail::checked_set_xml_attribute_value(
                position_node, "w:tblpXSpec",
                to_xml_table_position_horizontal_spec(
                    *position.horizontal_spec))) {
            return false;
        }
    } else if (!checked_remove_xml_attribute(position_node, "w:tblpXSpec")) {
        return false;
    }

    if (!detail::checked_set_xml_attribute_value(
            position_node, "w:vertAnchor",
            to_xml_table_position_vertical_reference(
                position.vertical_reference)) ||
        !detail::checked_set_xml_attribute_value(position_node, "w:tblpY",
                                                 vertical_offset)) {
        return false;
    }
    if (position.vertical_spec.has_value()) {
        if (!detail::checked_set_xml_attribute_value(
                position_node, "w:tblpYSpec",
                to_xml_table_position_vertical_spec(*position.vertical_spec))) {
            return false;
        }
    } else if (!checked_remove_xml_attribute(position_node, "w:tblpYSpec")) {
        return false;
    }

    if (!checked_set_optional_unsigned_attribute(
            position_node, "w:leftFromText", position.left_from_text_twips) ||
        !checked_set_optional_unsigned_attribute(
            position_node, "w:rightFromText", position.right_from_text_twips) ||
        !checked_set_optional_unsigned_attribute(
            position_node, "w:topFromText", position.top_from_text_twips) ||
        !checked_set_optional_unsigned_attribute(
            position_node, "w:bottomFromText",
            position.bottom_from_text_twips)) {
        return false;
    }

    return checked_remove_xml_attribute(position_node, "w:tblOverlap");
}

[[nodiscard]] auto
apply_table_overlap_attribute(pugi::xml_node overlap_node,
                              featherdoc::table_overlap overlap) -> bool {
    return overlap_node != pugi::xml_node{} &&
           detail::checked_set_xml_attribute_value(
               overlap_node, "w:val", to_xml_table_overlap(overlap));
}

[[nodiscard]] auto checked_ensure_table_properties_node(pugi::xml_node table,
                                                        bool &created)
    -> pugi::xml_node {
    auto table_properties = table.child("w:tblPr");
    if (table_properties != pugi::xml_node{}) {
        created = false;
        return table_properties;
    }

    created = true;
    if (const auto first_child = table.first_child();
        first_child != pugi::xml_node{}) {
        return detail::checked_insert_xml_element_before(table, "w:tblPr",
                                                         first_child);
    }
    return detail::checked_append_xml_element(table, "w:tblPr");
}

} // namespace

std::optional<featherdoc::table_position> Table::position() const {
    if (this->current == pugi::xml_node{}) {
        return std::nullopt;
    }

    const auto position_node = this->current.child("w:tblPr").child("w:tblpPr");
    if (position_node == pugi::xml_node{}) {
        return std::nullopt;
    }

    auto position = featherdoc::table_position{};
    if (const auto horizontal_reference =
            parse_table_position_horizontal_reference(std::string_view{
                position_node.attribute("w:horzAnchor").value()})) {
        position.horizontal_reference = *horizontal_reference;
    }
    if (const auto horizontal_offset =
            parse_signed_attribute(position_node, "w:tblpX")) {
        position.horizontal_offset_twips = *horizontal_offset;
    }
    if (const auto horizontal_spec = parse_table_position_horizontal_spec(
            std::string_view{position_node.attribute("w:tblpXSpec").value()})) {
        position.horizontal_spec = *horizontal_spec;
    }
    if (const auto vertical_reference =
            parse_table_position_vertical_reference(std::string_view{
                position_node.attribute("w:vertAnchor").value()})) {
        position.vertical_reference = *vertical_reference;
    }
    if (const auto vertical_offset =
            parse_signed_attribute(position_node, "w:tblpY")) {
        position.vertical_offset_twips = *vertical_offset;
    }
    if (const auto vertical_spec = parse_table_position_vertical_spec(
            std::string_view{position_node.attribute("w:tblpYSpec").value()})) {
        position.vertical_spec = *vertical_spec;
    }
    position.left_from_text_twips =
        parse_unsigned_attribute(position_node, "w:leftFromText");
    position.right_from_text_twips =
        parse_unsigned_attribute(position_node, "w:rightFromText");
    position.top_from_text_twips =
        parse_unsigned_attribute(position_node, "w:topFromText");
    position.bottom_from_text_twips =
        parse_unsigned_attribute(position_node, "w:bottomFromText");
    const auto overlap_node = position_node.parent().child("w:tblOverlap");
    auto overlap = overlap_node != pugi::xml_node{}
                       ? parse_table_overlap(std::string_view{
                             overlap_node.attribute("w:val").value()})
                       : std::nullopt;
    if (!overlap.has_value()) {
        overlap = parse_table_overlap(
            std::string_view{position_node.attribute("w:tblOverlap").value()});
    }
    if (overlap.has_value()) {
        position.overlap = *overlap;
    }

    return position;
}

bool Table::set_position(featherdoc::table_position position) {
    if (this->current == pugi::xml_node{}) {
        return false;
    }

    auto table = this->current.node();
    auto created_table_properties = false;
    auto table_properties =
        checked_ensure_table_properties_node(table, created_table_properties);
    if (table_properties == pugi::xml_node{}) {
        return false;
    }
    auto tracked_table_properties = this->current.with_node(table_properties);

    auto staged = std::optional<staged_table_position>{};
    const auto rollback = [&]() noexcept {
        if (staged.has_value()) {
            if (staged->replacement_overlap != pugi::xml_node{}) {
                (void)table_properties.remove_child(
                    staged->replacement_overlap);
            }
            if (staged->replacement_position != pugi::xml_node{}) {
                (void)table_properties.remove_child(
                    staged->replacement_position);
            }
        }
        if (created_table_properties) {
            (void)table.remove_child(table_properties);
        }
    };

    try {
        staged = stage_table_position_nodes(table_properties,
                                            position.overlap.has_value());
        if (!staged.has_value() ||
            !apply_table_position_attributes(staged->replacement_position,
                                             position) ||
            (position.overlap.has_value() &&
             !apply_table_overlap_attribute(staged->replacement_overlap,
                                            *position.overlap))) {
            rollback();
            return false;
        }

        const auto retirement_roots =
            std::array{staged->original_position, staged->original_overlap};
        if (!tracked_table_properties.retire_subtrees(
                std::span<const pugi::xml_node>{retirement_roots})) {
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

    if (staged->original_position != pugi::xml_node{}) {
        (void)table_properties.remove_child(staged->original_position);
    }
    if (staged->original_overlap != pugi::xml_node{}) {
        (void)table_properties.remove_child(staged->original_overlap);
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

    auto table_properties_node = table_properties.node();
    if (has_duplicate_child(table_properties_node, "w:tblpPr") ||
        has_duplicate_child(table_properties_node, "w:tblOverlap")) {
        return false;
    }

    const auto retirement_roots =
        std::array{table_properties_node.child("w:tblpPr"),
                   table_properties_node.child("w:tblOverlap")};
    if (retirement_roots[0] == pugi::xml_node{} &&
        retirement_roots[1] == pugi::xml_node{}) {
        return true;
    }

    try {
        if (!table_properties.retire_subtrees(
                std::span<const pugi::xml_node>{retirement_roots})) {
            return false;
        }
    } catch (const std::bad_alloc &) {
        return false;
    }

    for (const auto node : retirement_roots) {
        if (node != pugi::xml_node{}) {
            (void)table_properties_node.remove_child(node);
        }
    }
    return true;
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

    auto staged_properties = std::optional<staged_table_child>{};
    const auto rollback = [&]() noexcept {
        if (staged_properties.has_value()) {
            rollback_staged_table_child(*staged_properties);
        }
    };

    try {
        const auto table = this->current.node();
        staged_properties =
            stage_table_child(table, "w:tblPr", table.first_child());
        if (!staged_properties.has_value()) {
            return false;
        }

        const auto margin_name = to_xml_margin_name(edge);
        const auto margin = ensure_table_cell_margin_node(table, margin_name);
        const auto margins = margin.parent();
        const auto margin_text = std::to_string(margin_twips);
        if (margin == pugi::xml_node{} ||
            std::string_view{margin.name()} != margin_name ||
            margins == pugi::xml_node{} ||
            std::string_view{margins.name()} != "w:tblCellMar" ||
            margins.parent() != staged_properties->replacement ||
            !detail::checked_set_xml_attribute_value(margin, "w:w",
                                                     margin_text) ||
            !detail::checked_set_xml_attribute_value(margin, "w:type", "dxa")) {
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

    auto staged_properties = std::optional<staged_table_child>{};
    const auto rollback = [&]() noexcept {
        if (staged_properties.has_value()) {
            rollback_staged_table_child(*staged_properties);
        }
    };

    try {
        if (this->owner != nullptr &&
            this->owner->ensure_styles_part_attached()) {
            return false;
        }

        const auto table = this->current.node();
        staged_properties =
            stage_table_child(table, "w:tblPr", table.first_child());
        if (!staged_properties.has_value()) {
            return false;
        }

        const auto style_node = ensure_table_style_node(table);
        const auto style_text = std::string{style_id};
        if (style_node == pugi::xml_node{} ||
            std::string_view{style_node.name()} != "w:tblStyle" ||
            style_node.parent() != staged_properties->replacement ||
            !detail::checked_set_xml_attribute_value(style_node, "w:val",
                                                     style_text.c_str())) {
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

    auto staged_properties = std::optional<staged_table_child>{};
    const auto rollback = [&]() noexcept {
        if (staged_properties.has_value()) {
            rollback_staged_table_child(*staged_properties);
        }
    };

    try {
        const auto table = this->current.node();
        staged_properties =
            stage_table_child(table, "w:tblPr", table.first_child());
        if (!staged_properties.has_value()) {
            return false;
        }

        const auto table_look_node = ensure_table_look_node(table);
        const auto encoded_value =
            format_short_hex(encode_table_style_look(style_look));
        if (table_look_node == pugi::xml_node{} ||
            std::string_view{table_look_node.name()} != "w:tblLook" ||
            table_look_node.parent() != staged_properties->replacement ||
            !detail::checked_set_xml_attribute_value(table_look_node, "w:val",
                                                     encoded_value) ||
            !detail::checked_set_xml_attribute_value(
                table_look_node, "w:firstRow",
                style_look.first_row ? "1" : "0") ||
            !detail::checked_set_xml_attribute_value(
                table_look_node, "w:lastRow",
                style_look.last_row ? "1" : "0") ||
            !detail::checked_set_xml_attribute_value(
                table_look_node, "w:firstColumn",
                style_look.first_column ? "1" : "0") ||
            !detail::checked_set_xml_attribute_value(
                table_look_node, "w:lastColumn",
                style_look.last_column ? "1" : "0") ||
            !detail::checked_set_xml_attribute_value(
                table_look_node, "w:noHBand",
                style_look.banded_rows ? "0" : "1") ||
            !detail::checked_set_xml_attribute_value(
                table_look_node, "w:noVBand",
                style_look.banded_columns ? "0" : "1")) {
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

std::optional<featherdoc::border_inspection_summary>
Table::border(featherdoc::table_border_edge edge) const {
    if (this->current == pugi::xml_node{}) {
        return std::nullopt;
    }

    return read_border_inspection_summary(
        this->current.child("w:tblPr")
            .child("w:tblBorders")
            .child(to_xml_border_name(edge)));
}

bool Table::set_border(featherdoc::table_border_edge edge,
                       featherdoc::border_definition border) {
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
        const auto table = this->current.node();
        staged_properties =
            stage_table_child(table, "w:tblPr", table.first_child());
        if (!staged_properties.has_value()) {
            return false;
        }

        const auto border_name = to_xml_border_name(edge);
        auto table_borders = ensure_table_borders_node(table);
        auto border_node = table_borders.child(border_name);
        if (border_node == pugi::xml_node{}) {
            border_node = table_borders.append_child(border_name);
        }

        const auto expected_size = std::to_string(border.size_eighth_points);
        const auto expected_space = std::to_string(border.space_points);
        const auto expected_color = border.color.empty()
                                        ? std::string{"auto"}
                                        : std::string{border.color};
        apply_border_definition(border_node, border);
        if (table_borders == pugi::xml_node{} ||
            std::string_view{table_borders.name()} != "w:tblBorders" ||
            table_borders.parent() != staged_properties->replacement ||
            border_node == pugi::xml_node{} ||
            std::string_view{border_node.name()} != border_name ||
            border_node.parent() != table_borders ||
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

} // namespace featherdoc
