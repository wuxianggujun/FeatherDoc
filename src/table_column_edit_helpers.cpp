#include "table_column_edit_helpers.hpp"
#include "table_xml_helpers.hpp"
#include "xml_helpers.hpp"

#include <limits>
#include <string>

namespace featherdoc::detail {

auto cell_column_index(pugi::xml_node cell) -> std::optional<std::size_t> {
    if (cell == pugi::xml_node{}) {
        return std::nullopt;
    }

    const auto row = cell.parent();
    if (row == pugi::xml_node{}) {
        return std::nullopt;
    }

    std::size_t column_index = 0U;
    for (auto candidate = row.child("w:tc"); candidate != pugi::xml_node{};
         candidate = detail::next_named_sibling(candidate, "w:tc")) {
        if (candidate == cell) {
            return column_index;
        }
        column_index += cell_column_span(candidate);
    }

    return std::nullopt;
}

auto table_uses_fixed_layout(pugi::xml_node table) -> bool {
    if (table == pugi::xml_node{}) {
        return false;
    }

    return std::string_view{
               table.child("w:tblPr").child("w:tblLayout").attribute("w:type").value()} ==
           "fixed";
}

auto grid_column_width_twips(pugi::xml_node table, std::size_t column_index)
    -> std::optional<std::uint32_t> {
    if (table == pugi::xml_node{}) {
        return std::nullopt;
    }

    const auto grid_column = find_table_grid_column(table, column_index);
    if (grid_column == pugi::xml_node{} || grid_column.attribute("w:w") == pugi::xml_attribute{}) {
        return std::nullopt;
    }

    return parse_unsigned_attribute(grid_column, "w:w");
}

auto summed_grid_width_twips(pugi::xml_node table, std::size_t column_index,
                             std::size_t column_span) -> std::optional<std::uint32_t> {
    if (table == pugi::xml_node{} || column_span == 0U) {
        return std::nullopt;
    }

    std::uint64_t total_width = 0U;
    for (std::size_t offset = 0U; offset < column_span; ++offset) {
        const auto column_width = grid_column_width_twips(table, column_index + offset);
        if (!column_width.has_value()) {
            return std::nullopt;
        }

        total_width += *column_width;
        if (total_width > std::numeric_limits<std::uint32_t>::max()) {
            return std::nullopt;
        }
    }

    return static_cast<std::uint32_t>(total_width);
}

void synchronize_fixed_layout_cell_widths_from_grid(pugi::xml_node table) {
    if (table == pugi::xml_node{} || !table_uses_fixed_layout(table)) {
        return;
    }

    const auto column_count = current_table_column_count(table);
    if (column_count == 0U) {
        return;
    }

    ensure_table_grid_columns(table, column_count);
    for (std::size_t column_index = 0U; column_index < column_count; ++column_index) {
        if (!grid_column_width_twips(table, column_index).has_value()) {
            return;
        }
    }

    for (auto row = table.child("w:tr"); row != pugi::xml_node{};
         row = detail::next_named_sibling(row, "w:tr")) {
        std::size_t column_index = 0U;
        for (auto cell = row.child("w:tc"); cell != pugi::xml_node{};
             cell = detail::next_named_sibling(cell, "w:tc")) {
            const auto column_span = cell_column_span(cell);
            const auto cell_width = summed_grid_width_twips(table, column_index, column_span);
            if (cell_width.has_value()) {
                const auto width_node = ensure_cell_width_node(cell);
                if (width_node != pugi::xml_node{}) {
                    const auto width_text = std::to_string(*cell_width);
                    ensure_attribute_value(width_node, "w:w", width_text.c_str());
                    ensure_attribute_value(width_node, "w:type", "dxa");
                }
            }

            column_index += column_span;
        }
    }
}

void clear_fixed_layout_cell_widths_covering_column(pugi::xml_node table,
                                                    std::size_t target_column_index) {
    if (table == pugi::xml_node{} || !table_uses_fixed_layout(table)) {
        return;
    }

    for (auto row = table.child("w:tr"); row != pugi::xml_node{};
         row = detail::next_named_sibling(row, "w:tr")) {
        std::size_t column_index = 0U;
        for (auto cell = row.child("w:tc"); cell != pugi::xml_node{};
             cell = detail::next_named_sibling(cell, "w:tc")) {
            const auto column_span = cell_column_span(cell);
            const auto next_column_index = column_index + column_span;
            if (target_column_index >= column_index && target_column_index < next_column_index) {
                clear_cell_width_node(cell);
            }

            column_index = next_column_index;
        }
    }
}

auto find_row_cell_at_columns(pugi::xml_node row, std::size_t target_column_index,
                              std::size_t target_column_span) -> pugi::xml_node {
    if (row == pugi::xml_node{}) {
        return {};
    }

    std::size_t column_index = 0U;
    for (auto cell = row.child("w:tc"); cell != pugi::xml_node{};
         cell = detail::next_named_sibling(cell, "w:tc")) {
        const auto span = cell_column_span(cell);
        if (column_index == target_column_index && span == target_column_span) {
            return cell;
        }

        column_index += span;
        if (column_index > target_column_index) {
            return {};
        }
    }

    return {};
}


auto find_row_cell_covering_column(pugi::xml_node row, std::size_t target_column_index)
    -> row_cell_cover_result {
    if (row == pugi::xml_node{}) {
        return {};
    }

    std::size_t column_index = 0U;
    for (auto cell = row.child("w:tc"); cell != pugi::xml_node{};
         cell = detail::next_named_sibling(cell, "w:tc")) {
        const auto span = cell_column_span(cell);
        if (target_column_index >= column_index &&
            target_column_index - column_index < span) {
            return {cell, column_index, span};
        }

        column_index += span;
    }

    return {};
}






auto plan_table_column_removal(pugi::xml_node cell) -> std::optional<table_column_removal_plan> {
    if (cell == pugi::xml_node{}) {
        return std::nullopt;
    }

    const auto row = cell.parent();
    const auto table = row.parent();
    if (row == pugi::xml_node{} || table == pugi::xml_node{}) {
        return std::nullopt;
    }

    if (cell_column_span(cell) != 1U) {
        return std::nullopt;
    }

    const auto column_index = cell_column_index(cell);
    if (!column_index.has_value()) {
        return std::nullopt;
    }

    if (current_table_column_count(table) <= 1U) {
        return std::nullopt;
    }

    auto plan = table_column_removal_plan{};
    plan.column_index = *column_index;

    for (auto row_cursor = table.child("w:tr"); row_cursor != pugi::xml_node{};
         row_cursor = detail::next_named_sibling(row_cursor, "w:tr")) {
        if (count_named_children(row_cursor, "w:tc") <= 1U) {
            return std::nullopt;
        }

        const auto match = find_row_cell_covering_column(row_cursor, *column_index);
        if (match.cell == pugi::xml_node{} || match.span != 1U) {
            return std::nullopt;
        }

        plan.targets.push_back({row_cursor, match.cell});
    }

    if (plan.targets.empty()) {
        return std::nullopt;
    }

    return plan;
}

struct row_column_insertion_result final {
    pugi::xml_node clone_source;
    pugi::xml_node insert_before;
};

auto find_row_column_insertion_target(pugi::xml_node row, std::size_t boundary_column_index,
                                      bool insert_after)
    -> std::optional<row_column_insertion_result> {
    if (row == pugi::xml_node{}) {
        return std::nullopt;
    }

    auto previous_cell = pugi::xml_node{};
    std::size_t column_index = 0U;
    for (auto cell = row.child("w:tc"); cell != pugi::xml_node{};
         cell = detail::next_named_sibling(cell, "w:tc")) {
        const auto span = cell_column_span(cell);
        const auto cell_end = column_index + span;

        if (boundary_column_index == column_index) {
            const auto clone_source = insert_after ? previous_cell : cell;
            if (clone_source == pugi::xml_node{}) {
                return std::nullopt;
            }
            return row_column_insertion_result{clone_source, cell};
        }

        if (boundary_column_index > column_index && boundary_column_index < cell_end) {
            return std::nullopt;
        }

        previous_cell = cell;
        column_index = cell_end;
        if (insert_after && boundary_column_index == column_index) {
            return row_column_insertion_result{
                cell, detail::next_named_sibling(cell, "w:tc")};
        }
    }

    return std::nullopt;
}

auto plan_table_column_insertion(pugi::xml_node cell, bool insert_after)
    -> std::optional<table_column_insertion_plan> {
    if (cell == pugi::xml_node{}) {
        return std::nullopt;
    }

    const auto row = cell.parent();
    const auto table = row.parent();
    if (row == pugi::xml_node{} || table == pugi::xml_node{}) {
        return std::nullopt;
    }

    const auto column_index = cell_column_index(cell);
    if (!column_index.has_value()) {
        return std::nullopt;
    }

    const auto boundary_column_index =
        *column_index + (insert_after ? cell_column_span(cell) : 0U);
    auto plan = table_column_insertion_plan{};
    plan.boundary_column_index = boundary_column_index;
    plan.column_count_before_insertion = current_table_column_count(table);
    if (plan.column_count_before_insertion == 0U) {
        return std::nullopt;
    }
    plan.grid_width_source_column_index =
        insert_after ? boundary_column_index - 1U : *column_index;
    if (plan.grid_width_source_column_index >= plan.column_count_before_insertion) {
        return std::nullopt;
    }

    for (auto row_cursor = table.child("w:tr"); row_cursor != pugi::xml_node{};
         row_cursor = detail::next_named_sibling(row_cursor, "w:tr")) {
        const auto row_target =
            find_row_column_insertion_target(row_cursor, boundary_column_index, insert_after);
        if (!row_target.has_value()) {
            return std::nullopt;
        }

        plan.targets.push_back({row_cursor, row_target->clone_source, row_target->insert_before});
    }

    if (plan.targets.empty()) {
        return std::nullopt;
    }

    return plan;
}

auto plan_vertical_merge_chain(pugi::xml_node cell) -> std::optional<vertical_merge_chain_plan> {
    if (cell == pugi::xml_node{}) {
        return std::nullopt;
    }

    const auto merge_state = cell_vertical_merge_state_for(cell);
    if (merge_state == cell_vertical_merge_state::none) {
        return std::nullopt;
    }

    const auto row = cell.parent();
    if (row == pugi::xml_node{}) {
        return std::nullopt;
    }

    const auto column_index = cell_column_index(cell);
    if (!column_index.has_value()) {
        return std::nullopt;
    }

    const auto column_span = cell_column_span(cell);
    auto anchor_row = row;
    auto anchor_cell = cell;
    if (merge_state == cell_vertical_merge_state::continue_merge) {
        while (true) {
            const auto previous_row = detail::previous_named_sibling(anchor_row, "w:tr");
            if (previous_row == pugi::xml_node{}) {
                return std::nullopt;
            }

            const auto previous_cell =
                find_row_cell_at_columns(previous_row, *column_index, column_span);
            if (previous_cell == pugi::xml_node{}) {
                return std::nullopt;
            }

            const auto previous_state = cell_vertical_merge_state_for(previous_cell);
            if (previous_state == cell_vertical_merge_state::restart) {
                anchor_row = previous_row;
                anchor_cell = previous_cell;
                break;
            }

            if (previous_state != cell_vertical_merge_state::continue_merge) {
                return std::nullopt;
            }

            anchor_row = previous_row;
            anchor_cell = previous_cell;
        }
    }

    auto plan = vertical_merge_chain_plan{};
    plan.anchor_cell = anchor_cell;
    plan.cells.push_back(anchor_cell);

    auto row_cursor = anchor_row;
    while (true) {
        const auto next_row = detail::next_named_sibling(row_cursor, "w:tr");
        if (next_row == pugi::xml_node{}) {
            break;
        }

        const auto next_cell = find_row_cell_at_columns(next_row, *column_index, column_span);
        if (next_cell == pugi::xml_node{} ||
            cell_vertical_merge_state_for(next_cell) !=
                cell_vertical_merge_state::continue_merge) {
            break;
        }

        plan.cells.push_back(next_cell);
        row_cursor = next_row;
    }

    return plan;
}

bool remove_table_grid_column(pugi::xml_node table, std::size_t target_column_index) {
    if (table == pugi::xml_node{}) {
        return false;
    }

    const auto column_count = current_table_column_count(table);
    if (column_count == 0U || target_column_index >= column_count) {
        return false;
    }

    ensure_table_grid_columns(table, column_count);
    auto table_grid = table.child("w:tblGrid");
    if (table_grid == pugi::xml_node{}) {
        return false;
    }

    auto grid_column = table_grid.child("w:gridCol");
    for (std::size_t index = 0U; index < target_column_index && grid_column != pugi::xml_node{};
         ++index) {
        grid_column = detail::next_named_sibling(grid_column, "w:gridCol");
    }

    return grid_column != pugi::xml_node{} && table_grid.remove_child(grid_column);
}

bool insert_table_grid_column(pugi::xml_node table, std::size_t boundary_column_index,
                              std::size_t column_count_before_insertion,
                              std::size_t source_column_index) {
    if (table == pugi::xml_node{}) {
        return false;
    }

    const auto column_count = column_count_before_insertion;
    if (boundary_column_index > column_count) {
        return false;
    }

    ensure_table_grid_columns(table, column_count);
    auto table_grid = table.child("w:tblGrid");
    if (table_grid == pugi::xml_node{}) {
        return false;
    }

    if (column_count == 0U) {
        auto grid_column = table_grid.append_child("w:gridCol");
        if (grid_column == pugi::xml_node{}) {
            return false;
        }
        ensure_attribute_value(grid_column, "w:w", "0");
        return true;
    }

    if (source_column_index >= column_count) {
        return false;
    }

    const auto source_column = find_table_grid_column(table, source_column_index);
    if (source_column == pugi::xml_node{}) {
        return false;
    }

    auto anchor_column = table_grid.child("w:gridCol");
    for (std::size_t index = 0U; index < boundary_column_index && anchor_column != pugi::xml_node{};
         ++index) {
        anchor_column = detail::next_named_sibling(anchor_column, "w:gridCol");
    }

    if (anchor_column != pugi::xml_node{}) {
        return table_grid.insert_copy_before(source_column, anchor_column) != pugi::xml_node{};
    }

    const auto last_column = find_table_grid_column(table, column_count - 1U);

    return last_column != pugi::xml_node{} &&
           table_grid.insert_copy_after(source_column, last_column) != pugi::xml_node{};
}

void remove_empty_cell_properties(pugi::xml_node cell) {
    if (cell == pugi::xml_node{}) {
        return;
    }

    const auto cell_properties = cell.child("w:tcPr");
    if (cell_properties != pugi::xml_node{} &&
        cell_properties.first_child() == pugi::xml_node{} &&
        cell_properties.first_attribute() == pugi::xml_attribute{}) {
        cell.remove_child(cell_properties);
    }
}

void normalize_inserted_table_cell(pugi::xml_node cell) {
    if (cell == pugi::xml_node{}) {
        return;
    }

    auto cell_properties = cell.child("w:tcPr");
    if (cell_properties != pugi::xml_node{}) {
        if (const auto grid_span = cell_properties.child("w:gridSpan");
            grid_span != pugi::xml_node{}) {
            cell_properties.remove_child(grid_span);
        }
        if (const auto vertical_merge = cell_properties.child("w:vMerge");
            vertical_merge != pugi::xml_node{}) {
            cell_properties.remove_child(vertical_merge);
        }
    }

    remove_empty_cell_properties(cell);
}

auto insert_empty_clone_cell(pugi::xml_node row, pugi::xml_node source_cell,
                             pugi::xml_node insert_before) -> pugi::xml_node {
    if (row == pugi::xml_node{} || source_cell == pugi::xml_node{}) {
        return {};
    }

    auto inserted_cell = pugi::xml_node{};
    if (insert_before != pugi::xml_node{}) {
        inserted_cell = row.insert_copy_before(source_cell, insert_before);
    } else {
        inserted_cell = row.insert_copy_after(source_cell, source_cell);
    }
    if (inserted_cell == pugi::xml_node{}) {
        return {};
    }

    normalize_inserted_table_cell(inserted_cell);
    if (!TableCell(row, inserted_cell).set_text("")) {
        row.remove_child(inserted_cell);
        return {};
    }

    return inserted_cell;
}

void rollback_inserted_table_cells(const std::vector<pugi::xml_node> &inserted_cells) {
    for (auto it = inserted_cells.rbegin(); it != inserted_cells.rend(); ++it) {
        const auto inserted_cell = *it;
        if (inserted_cell != pugi::xml_node{}) {
            auto parent = inserted_cell.parent();
            if (parent != pugi::xml_node{}) {
                parent.remove_child(inserted_cell);
            }
        }
    }
}

void clear_cell_contents_for_vertical_merge(pugi::xml_node cell) {
    if (cell == pugi::xml_node{}) {
        return;
    }

    const auto cell_properties = cell.child("w:tcPr");
    for (auto child = cell.first_child(); child != pugi::xml_node{};) {
        const auto next_child = child.next_sibling();
        if (child != cell_properties) {
            cell.remove_child(child);
        }
        child = next_child;
    }

    if (cell.child("w:p") == pugi::xml_node{}) {
        cell.append_child("w:p");
    }
}

void replace_cell_body_contents(pugi::xml_node target_cell, pugi::xml_node source_cell) {
    if (target_cell == pugi::xml_node{} || source_cell == pugi::xml_node{}) {
        return;
    }

    const auto target_properties = target_cell.child("w:tcPr");
    for (auto child = target_cell.first_child(); child != pugi::xml_node{};) {
        const auto next_child = child.next_sibling();
        if (child != target_properties) {
            target_cell.remove_child(child);
        }
        child = next_child;
    }

    const auto source_properties = source_cell.child("w:tcPr");
    for (auto child = source_cell.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        if (child != source_properties) {
            target_cell.append_copy(child);
        }
    }

    if (target_cell.child("w:p") == pugi::xml_node{}) {
        target_cell.append_child("w:p");
    }
}

auto successor_vertical_merge_promotions_for_row_removal(pugi::xml_node row)
    -> std::vector<std::pair<pugi::xml_node, pugi::xml_node>> {
    std::vector<std::pair<pugi::xml_node, pugi::xml_node>> promotions;
    if (row == pugi::xml_node{}) {
        return promotions;
    }

    const auto next_row = detail::next_named_sibling(row, "w:tr");
    if (next_row == pugi::xml_node{}) {
        return promotions;
    }

    for (auto cell = row.child("w:tc"); cell != pugi::xml_node{};
         cell = detail::next_named_sibling(cell, "w:tc")) {
        if (cell_vertical_merge_state_for(cell) != cell_vertical_merge_state::restart) {
            continue;
        }

        const auto column_index = cell_column_index(cell);
        if (!column_index.has_value()) {
            promotions.clear();
            return promotions;
        }

        const auto next_cell =
            find_row_cell_at_columns(next_row, *column_index, cell_column_span(cell));
        if (next_cell == pugi::xml_node{} ||
            cell_vertical_merge_state_for(next_cell) !=
                cell_vertical_merge_state::continue_merge) {
            continue;
        }

        promotions.emplace_back(cell, next_cell);
    }

    return promotions;
}

} // namespace featherdoc::detail
