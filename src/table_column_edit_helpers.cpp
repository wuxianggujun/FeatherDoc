#include "table_column_edit_helpers.hpp"
#include "table_xml_helpers.hpp"
#include "xml_document_clone_helpers.hpp"
#include "xml_helpers.hpp"

#include <limits>
#include <new>
#include <string>

namespace featherdoc::detail {

namespace {

struct staged_table_child final {
    pugi::xml_node original;
    pugi::xml_node replacement;
};

[[nodiscard]] auto checked_stage_table_child(pugi::xml_node table,
                                             const char *child_name,
                                             pugi::xml_node insertion_anchor)
    -> std::optional<staged_table_child> {
    const auto original = table.child(child_name);
    auto replacement = pugi::xml_node{};
    if (original != pugi::xml_node{}) {
        replacement = table.insert_child_before(original.type(), original);
        if (replacement == pugi::xml_node{}) {
            return std::nullopt;
        }
        try {
            if (!xml_document_clone_detail::copy_node_contents(original,
                                                               replacement)) {
                (void)table.remove_child(replacement);
                return std::nullopt;
            }
            for (auto child = original.first_child();
                 child != pugi::xml_node{}; child = child.next_sibling()) {
                if (checked_append_copy_xml_node(child, replacement) !=
                    xml_document_clone_status::success) {
                    (void)table.remove_child(replacement);
                    return std::nullopt;
                }
            }
        } catch (...) {
            (void)table.remove_child(replacement);
            throw;
        }
    } else if (insertion_anchor != pugi::xml_node{}) {
        replacement =
            checked_insert_xml_element_before(table, child_name,
                                              insertion_anchor);
    } else {
        replacement = checked_append_xml_element(table, child_name);
    }

    if (replacement == pugi::xml_node{} ||
        std::string_view{replacement.name()} != child_name) {
        if (replacement != pugi::xml_node{}) {
            (void)table.remove_child(replacement);
        }
        return std::nullopt;
    }
    return staged_table_child{original, replacement};
}

[[nodiscard]] auto checked_ensure_default_attribute(
    pugi::xml_node node, const char *name, std::string_view value) -> bool {
    return node.attribute(name) != pugi::xml_attribute{} ||
           checked_append_xml_attribute(node, name, value);
}

[[nodiscard]] auto checked_prepare_default_table_properties(
    pugi::xml_node table) -> bool {
    auto table_width = ensure_table_width_node(table);
    if (table_width == pugi::xml_node{} ||
        std::string_view{table_width.name()} != "w:tblW" ||
        !checked_ensure_default_attribute(table_width, "w:w", "0") ||
        !checked_ensure_default_attribute(table_width, "w:type", "auto")) {
        return false;
    }

    auto table_look = ensure_table_look_node(table);
    return table_look != pugi::xml_node{} &&
           std::string_view{table_look.name()} == "w:tblLook" &&
           checked_ensure_default_attribute(table_look, "w:val", "04A0") &&
           checked_ensure_default_attribute(table_look, "w:firstRow", "1") &&
           checked_ensure_default_attribute(table_look, "w:firstColumn", "1") &&
           checked_ensure_default_attribute(table_look, "w:lastRow", "0") &&
           checked_ensure_default_attribute(table_look, "w:lastColumn", "0") &&
           checked_ensure_default_attribute(table_look, "w:noHBand", "0") &&
           checked_ensure_default_attribute(table_look, "w:noVBand", "1");
}

[[nodiscard]] auto checked_append_grid_column(pugi::xml_node table_grid,
                                              std::string_view width)
    -> pugi::xml_node {
    auto grid_column = checked_append_xml_element(table_grid, "w:gridCol");
    if (grid_column == pugi::xml_node{} ||
        !checked_set_xml_attribute_value(grid_column, "w:w", width)) {
        if (grid_column != pugi::xml_node{}) {
            (void)table_grid.remove_child(grid_column);
        }
        return {};
    }
    return grid_column;
}

[[nodiscard]] auto checked_append_grid_column_copy(
    pugi::xml_node table_grid, pugi::xml_node source_column)
    -> pugi::xml_node {
    const auto previous_last_child = table_grid.last_child();
    const auto copy_status =
        checked_append_copy_xml_node(source_column, table_grid);
    const auto copied_column =
        previous_last_child == pugi::xml_node{}
            ? table_grid.first_child()
            : previous_last_child.next_sibling();
    if (copy_status != xml_document_clone_status::success ||
        copied_column == pugi::xml_node{} ||
        std::string_view{copied_column.name()} != "w:gridCol") {
        if (copied_column != pugi::xml_node{}) {
            (void)table_grid.remove_child(copied_column);
        }
        return {};
    }
    return copied_column;
}

[[nodiscard]] auto contains_node(std::span<const pugi::xml_node> nodes,
                                 pugi::xml_node candidate) noexcept -> bool {
    for (const auto node : nodes) {
        if (node == candidate) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] auto find_staged_cell_properties(
    std::vector<staged_cell_properties> &staged_properties,
    pugi::xml_node cell) noexcept -> staged_cell_properties * {
    for (auto &staged : staged_properties) {
        if (staged.cell == cell) {
            return &staged;
        }
    }
    return nullptr;
}

[[nodiscard]] auto stage_cell_properties_once(
    pugi::xml_node cell,
    std::vector<staged_cell_properties> &staged_properties)
    -> staged_cell_properties * {
    if (auto *existing =
            find_staged_cell_properties(staged_properties, cell);
        existing != nullptr) {
        return existing;
    }

    const auto staged = stage_cell_properties(cell);
    if (!staged.has_value()) {
        return nullptr;
    }
    try {
        staged_properties.push_back(*staged);
    } catch (...) {
        (void)cell.remove_child(staged->replacement);
        throw;
    }
    return &staged_properties.back();
}

[[nodiscard]] auto append_staged_table_cell_text(pugi::xml_node parent,
                                                 const char *text)
    -> pugi::xml_node {
    if (parent == pugi::xml_node{} || text == nullptr) {
        return {};
    }

    auto cell = checked_append_xml_element(parent, "w:tc");
    auto paragraph = cell == pugi::xml_node{}
                         ? pugi::xml_node{}
                         : checked_append_xml_element(cell, "w:p");
    if (paragraph == pugi::xml_node{}) {
        return {};
    }

    if (text[0] != '\0' && !append_plain_text_run(paragraph, text)) {
        return {};
    }
    return cell;
}

} // namespace

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
    if (!column_count.has_value() || *column_count == 0U) {
        return;
    }

    if (!ensure_table_grid_columns(table, *column_count)) {
        return;
    }
    for (std::size_t column_index = 0U; column_index < *column_count; ++column_index) {
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

    const auto column_count = current_table_column_count(table);
    if (!column_count.has_value() || *column_count <= 1U) {
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
                cell, cell.next_sibling()};
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

    const auto column_count = current_table_column_count(table);
    if (!column_count.has_value() || *column_count == 0U ||
        *column_count >= max_table_grid_columns) {
        return std::nullopt;
    }

    const auto insertion_offset = insert_after ? cell_column_span(cell) : 0U;
    if (*column_index > *column_count ||
        insertion_offset > *column_count - *column_index) {
        return std::nullopt;
    }
    const auto boundary_column_index = *column_index + insertion_offset;

    auto plan = table_column_insertion_plan{};
    plan.boundary_column_index = boundary_column_index;
    plan.column_count_before_insertion = *column_count;
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
    if (!column_count.has_value() || *column_count == 0U ||
        target_column_index >= *column_count) {
        return false;
    }

    if (!ensure_table_grid_columns(table, *column_count)) {
        return false;
    }
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
    if (column_count >= max_table_grid_columns ||
        boundary_column_index > column_count) {
        return false;
    }

    if (!ensure_table_grid_columns(table, column_count)) {
        return false;
    }
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
    if (row == pugi::xml_node{} || source_cell == pugi::xml_node{} ||
        source_cell.parent() != row ||
        (insert_before != pugi::xml_node{} && insert_before.parent() != row)) {
        return {};
    }

    auto inserted_cell = pugi::xml_node{};
    if (insert_before != pugi::xml_node{}) {
        inserted_cell = row.insert_child_before(source_cell.type(),
                                                insert_before);
    } else {
        inserted_cell = row.insert_child_after(source_cell.type(), source_cell);
    }
    if (inserted_cell == pugi::xml_node{}) {
        return {};
    }

    try {
        if (!xml_document_clone_detail::copy_node_contents(source_cell,
                                                           inserted_cell)) {
            (void)row.remove_child(inserted_cell);
            return {};
        }
        for (auto child = source_cell.first_child();
             child != pugi::xml_node{}; child = child.next_sibling()) {
            if (checked_append_copy_xml_node(child, inserted_cell) !=
                xml_document_clone_status::success) {
                (void)row.remove_child(inserted_cell);
                return {};
            }
        }
    } catch (...) {
        (void)row.remove_child(inserted_cell);
        throw;
    }

    normalize_inserted_table_cell(inserted_cell);
    if (!replace_table_cell_text(inserted_cell, "")) {
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

auto stage_cell_properties(pugi::xml_node cell)
    -> std::optional<staged_cell_properties> {
    if (cell == pugi::xml_node{}) {
        return std::nullopt;
    }

    const auto original = cell.child("w:tcPr");
    auto replacement = pugi::xml_node{};
    if (original != pugi::xml_node{}) {
        replacement = cell.insert_child_before(original.type(), original);
        if (replacement == pugi::xml_node{}) {
            return std::nullopt;
        }

        try {
            if (!xml_document_clone_detail::copy_node_contents(
                    original, replacement)) {
                (void)cell.remove_child(replacement);
                return std::nullopt;
            }
            for (auto child = original.first_child();
                 child != pugi::xml_node{}; child = child.next_sibling()) {
                if (checked_append_copy_xml_node(child, replacement) !=
                    xml_document_clone_status::success) {
                    (void)cell.remove_child(replacement);
                    return std::nullopt;
                }
            }
        } catch (...) {
            (void)cell.remove_child(replacement);
            throw;
        }
    } else if (const auto first_child = cell.first_child();
               first_child != pugi::xml_node{}) {
        replacement =
            checked_insert_xml_element_before(cell, "w:tcPr", first_child);
    } else {
        replacement = checked_append_xml_element(cell, "w:tcPr");
    }
    if (replacement == pugi::xml_node{}) {
        return std::nullopt;
    }

    return staged_cell_properties{cell, original, replacement};
}

void rollback_staged_cell_properties(
    const std::vector<staged_cell_properties> &staged_properties) noexcept {
    for (auto iterator = staged_properties.rbegin();
         iterator != staged_properties.rend(); ++iterator) {
        auto cell = iterator->cell;
        (void)cell.remove_child(iterator->replacement);
    }
}

auto commit_staged_cell_properties(
    const std::vector<staged_cell_properties> &staged_properties) noexcept
    -> bool {
    for (const auto &staged : staged_properties) {
        if (staged.original != pugi::xml_node{}) {
            auto cell = staged.cell;
            if (!cell.remove_child(staged.original)) {
                return false;
            }
        }
    }
    return true;
}

auto stage_table_layout(pugi::xml_node table,
                        std::size_t normalized_column_count,
                        table_grid_edit grid_edit)
    -> std::optional<staged_table_layout> {
    if (table == pugi::xml_node{} ||
        normalized_column_count > max_table_grid_columns) {
        return std::nullopt;
    }
    if ((grid_edit.kind == table_grid_edit_kind::insert_column &&
         (normalized_column_count == 0U ||
          normalized_column_count >= max_table_grid_columns ||
          grid_edit.column_index > normalized_column_count ||
          grid_edit.source_column_index >= normalized_column_count)) ||
        (grid_edit.kind == table_grid_edit_kind::remove_column &&
         (normalized_column_count == 0U ||
          grid_edit.column_index >= normalized_column_count))) {
        return std::nullopt;
    }

    auto staged_properties = std::optional<staged_table_child>{};
    auto staged_grid = std::optional<staged_table_child>{};
    const auto rollback = [&]() noexcept {
        if (staged_grid.has_value()) {
            (void)table.remove_child(staged_grid->replacement);
        }
        if (staged_properties.has_value()) {
            (void)table.remove_child(staged_properties->replacement);
        }
    };

    try {
        staged_properties = checked_stage_table_child(
            table, "w:tblPr", table.first_child());
        if (!staged_properties.has_value() ||
            !checked_prepare_default_table_properties(table)) {
            rollback();
            return std::nullopt;
        }

        staged_grid = checked_stage_table_child(
            table, "w:tblGrid", table.child("w:tr"));
        if (!staged_grid.has_value()) {
            rollback();
            return std::nullopt;
        }

        auto existing_column_count =
            count_named_children(staged_grid->replacement, "w:gridCol");
        if (existing_column_count > normalized_column_count) {
            rollback();
            return std::nullopt;
        }
        while (existing_column_count < normalized_column_count) {
            if (checked_append_grid_column(staged_grid->replacement, "0") ==
                pugi::xml_node{}) {
                rollback();
                return std::nullopt;
            }
            ++existing_column_count;
        }

        switch (grid_edit.kind) {
        case table_grid_edit_kind::normalize:
            break;
        case table_grid_edit_kind::insert_column: {
            const auto source_column = find_table_grid_column(
                table, grid_edit.source_column_index);
            if (source_column == pugi::xml_node{}) {
                rollback();
                return std::nullopt;
            }
            auto inserted_column = checked_append_grid_column_copy(
                staged_grid->replacement, source_column);
            if (inserted_column == pugi::xml_node{}) {
                rollback();
                return std::nullopt;
            }
            auto anchor_column =
                grid_edit.column_index < normalized_column_count
                    ? find_table_grid_column(table, grid_edit.column_index)
                    : pugi::xml_node{};
            if (anchor_column != pugi::xml_node{} &&
                staged_grid->replacement.insert_move_before(inserted_column,
                                                             anchor_column) ==
                    pugi::xml_node{}) {
                rollback();
                return std::nullopt;
            }
            break;
        }
        case table_grid_edit_kind::remove_column: {
            const auto removed_column =
                find_table_grid_column(table, grid_edit.column_index);
            if (removed_column == pugi::xml_node{} ||
                !staged_grid->replacement.remove_child(removed_column)) {
                rollback();
                return std::nullopt;
            }
            break;
        }
        }

        const auto expected_column_count =
            grid_edit.kind == table_grid_edit_kind::insert_column
                ? normalized_column_count + 1U
                : grid_edit.kind == table_grid_edit_kind::remove_column
                      ? normalized_column_count - 1U
                      : normalized_column_count;
        if (count_named_children(staged_grid->replacement, "w:gridCol") !=
            expected_column_count) {
            rollback();
            return std::nullopt;
        }
    } catch (...) {
        rollback();
        throw;
    }

    return staged_table_layout{
        table,
        staged_properties->original,
        staged_properties->replacement,
        staged_grid->original,
        staged_grid->replacement,
    };
}

void rollback_staged_table_layout(
    const staged_table_layout &staged_layout) noexcept {
    auto table = staged_layout.table;
    if (table != pugi::xml_node{}) {
        (void)table.remove_child(staged_layout.replacement_grid);
        (void)table.remove_child(staged_layout.replacement_properties);
    }
}

auto commit_staged_table_layout(
    const staged_table_layout &staged_layout) noexcept -> bool {
    auto table = staged_layout.table;
    if (table == pugi::xml_node{}) {
        return false;
    }
    if (staged_layout.original_grid != pugi::xml_node{} &&
        !table.remove_child(staged_layout.original_grid)) {
        return false;
    }
    return staged_layout.original_properties == pugi::xml_node{} ||
           table.remove_child(staged_layout.original_properties);
}

auto stage_fixed_layout_cell_widths(
    pugi::xml_node table, std::span<const pugi::xml_node> excluded_cells,
    std::vector<staged_cell_properties> &staged_properties) -> bool {
    if (table == pugi::xml_node{} || !table_uses_fixed_layout(table)) {
        return table != pugi::xml_node{};
    }

    const auto column_count =
        count_named_children(table.child("w:tblGrid"), "w:gridCol");
    if (column_count == 0U || column_count > max_table_grid_columns) {
        return false;
    }
    for (std::size_t column_index = 0U; column_index < column_count;
         ++column_index) {
        if (!grid_column_width_twips(table, column_index).has_value()) {
            return true;
        }
    }

    for (auto row = table.child("w:tr"); row != pugi::xml_node{};
         row = detail::next_named_sibling(row, "w:tr")) {
        auto column_index = std::size_t{0U};
        for (auto cell = row.child("w:tc"); cell != pugi::xml_node{};
             cell = detail::next_named_sibling(cell, "w:tc")) {
            if (contains_node(excluded_cells, cell)) {
                continue;
            }

            const auto column_span = cell_column_span(cell);
            const auto cell_width =
                summed_grid_width_twips(table, column_index, column_span);
            if (cell_width.has_value()) {
                const auto width_text = std::to_string(*cell_width);
                const auto current_width =
                    cell.child("w:tcPr").child("w:tcW");
                const auto already_synchronized =
                    current_width != pugi::xml_node{} &&
                    std::string_view{current_width.attribute("w:w").value()} ==
                        width_text &&
                    std::string_view{
                        current_width.attribute("w:type").value()} == "dxa";
                if (!already_synchronized) {
                    if (stage_cell_properties_once(cell, staged_properties) ==
                        nullptr) {
                        return false;
                    }
                    const auto width_node = ensure_cell_width_node(cell);
                    if (width_node == pugi::xml_node{} ||
                        std::string_view{width_node.name()} != "w:tcW" ||
                        !checked_set_xml_attribute_value(width_node, "w:w",
                                                         width_text) ||
                        !checked_set_xml_attribute_value(width_node, "w:type",
                                                         "dxa")) {
                        return false;
                    }
                }
            }

            if (column_span >
                std::numeric_limits<std::size_t>::max() - column_index) {
                return false;
            }
            column_index += column_span;
        }
    }
    return true;
}

auto clear_cell_contents_for_vertical_merge(
    const std::vector<tracked_xml_node> &cells,
    std::span<const pugi::xml_node> additional_retirement_roots) -> bool {
    if (cells.empty()) {
        return true;
    }

    auto old_body_count = std::size_t{0U};
    for (const auto &cell : cells) {
        const auto cell_node = cell.node();
        if (cell_node == pugi::xml_node{}) {
            return false;
        }
        for (auto child = cell_node.first_child();
             child != pugi::xml_node{}; child = child.next_sibling()) {
            if (std::string_view{child.name()} == "w:tcPr") {
                continue;
            }
            if (old_body_count == std::numeric_limits<std::size_t>::max()) {
                return false;
            }
            ++old_body_count;
        }
    }

    if (additional_retirement_roots.size() >
        std::numeric_limits<std::size_t>::max() - old_body_count) {
        return false;
    }
    auto retirement_roots = std::vector<pugi::xml_node>{};
    retirement_roots.reserve(old_body_count +
                             additional_retirement_roots.size());
    for (const auto &cell : cells) {
        const auto cell_node = cell.node();
        for (auto child = cell_node.first_child();
             child != pugi::xml_node{}; child = child.next_sibling()) {
            if (std::string_view{child.name()} != "w:tcPr") {
                retirement_roots.push_back(child);
            }
        }
    }
    const auto old_body_root_count = retirement_roots.size();
    retirement_roots.insert(retirement_roots.end(),
                            additional_retirement_roots.begin(),
                            additional_retirement_roots.end());

    auto replacement_paragraphs = std::vector<pugi::xml_node>{};
    replacement_paragraphs.reserve(cells.size());
    const auto rollback_replacement_paragraphs = [&]() noexcept {
        for (auto iterator = replacement_paragraphs.rbegin();
             iterator != replacement_paragraphs.rend(); ++iterator) {
            auto parent = iterator->parent();
            (void)parent.remove_child(*iterator);
        }
    };

    // Allocate every replacement before retiring any public handle. Batch
    // retirement then makes publishing a sequence of no-allocation unlinks.
    for (const auto &cell : cells) {
        auto cell_node = cell.node();
        const auto replacement_paragraph =
            checked_append_xml_element(cell_node, "w:p");
        if (replacement_paragraph == pugi::xml_node{}) {
            rollback_replacement_paragraphs();
            return false;
        }
        replacement_paragraphs.push_back(replacement_paragraph);
    }

    try {
        if (!cells.front().retire_subtrees(std::span<const pugi::xml_node>{
                retirement_roots.data(), retirement_roots.size()})) {
            rollback_replacement_paragraphs();
            return false;
        }
    } catch (...) {
        rollback_replacement_paragraphs();
        throw;
    }

    for (std::size_t index = 0U; index < old_body_root_count; ++index) {
        const auto child = retirement_roots[index];
        auto parent = child.parent();
        if (!parent.remove_child(child)) {
            return false;
        }
    }
    return true;
}

auto replace_cell_body_contents(
    const tracked_xml_node &retirement_anchor,
    const std::vector<tracked_cell_body_replacement> &replacements,
    std::span<const pugi::xml_node> additional_retirement_roots) -> bool {
    auto old_body_count = std::size_t{0U};
    auto replacement_child_count = std::size_t{0U};
    for (const auto &replacement : replacements) {
        const auto target_node = replacement.target_cell.node();
        if (target_node == pugi::xml_node{} ||
            replacement.source_cell == pugi::xml_node{}) {
            return false;
        }

        for (auto child = target_node.first_child();
             child != pugi::xml_node{}; child = child.next_sibling()) {
            if (std::string_view{child.name()} == "w:tcPr") {
                continue;
            }
            if (old_body_count == std::numeric_limits<std::size_t>::max()) {
                return false;
            }
            ++old_body_count;
        }

        auto source_has_paragraph = false;
        for (auto child = replacement.source_cell.first_child();
             child != pugi::xml_node{}; child = child.next_sibling()) {
            if (std::string_view{child.name()} == "w:tcPr") {
                continue;
            }
            if (replacement_child_count ==
                std::numeric_limits<std::size_t>::max()) {
                return false;
            }
            ++replacement_child_count;
            source_has_paragraph = source_has_paragraph ||
                                   std::string_view{child.name()} == "w:p";
        }
        if (!source_has_paragraph) {
            if (replacement_child_count ==
                std::numeric_limits<std::size_t>::max()) {
                return false;
            }
            ++replacement_child_count;
        }
    }

    if (additional_retirement_roots.size() >
        std::numeric_limits<std::size_t>::max() - old_body_count) {
        return false;
    }
    auto retirement_roots = std::vector<pugi::xml_node>{};
    retirement_roots.reserve(old_body_count +
                             additional_retirement_roots.size());
    for (const auto &replacement : replacements) {
        const auto target_node = replacement.target_cell.node();
        for (auto child = target_node.first_child();
             child != pugi::xml_node{}; child = child.next_sibling()) {
            if (std::string_view{child.name()} != "w:tcPr") {
                retirement_roots.push_back(child);
            }
        }
    }
    const auto old_body_root_count = retirement_roots.size();
    retirement_roots.insert(retirement_roots.end(),
                            additional_retirement_roots.begin(),
                            additional_retirement_roots.end());

    auto replacement_children = std::vector<pugi::xml_node>{};
    replacement_children.reserve(replacement_child_count);
    const auto rollback_replacement_children = [&]() noexcept {
        for (auto iterator = replacement_children.rbegin();
             iterator != replacement_children.rend(); ++iterator) {
            auto parent = iterator->parent();
            (void)parent.remove_child(*iterator);
        }
    };

    for (const auto &replacement : replacements) {
        auto target_node = replacement.target_cell.node();
        auto source_has_paragraph = false;
        for (auto child = replacement.source_cell.first_child();
             child != pugi::xml_node{}; child = child.next_sibling()) {
            if (std::string_view{child.name()} == "w:tcPr") {
                continue;
            }
            const auto previous_last_child = target_node.last_child();
            const auto copy_status =
                checked_append_copy_xml_node(child, target_node);
            const auto replacement_child =
                previous_last_child == pugi::xml_node{}
                    ? target_node.first_child()
                    : previous_last_child.next_sibling();
            if (replacement_child != pugi::xml_node{}) {
                replacement_children.push_back(replacement_child);
            }
            if (copy_status != xml_document_clone_status::success ||
                replacement_child == pugi::xml_node{}) {
                rollback_replacement_children();
                return false;
            }
            source_has_paragraph = source_has_paragraph ||
                                   std::string_view{child.name()} == "w:p";
        }
        if (!source_has_paragraph) {
            const auto replacement_paragraph =
                checked_append_xml_element(target_node, "w:p");
            if (replacement_paragraph == pugi::xml_node{}) {
                rollback_replacement_children();
                return false;
            }
            replacement_children.push_back(replacement_paragraph);
        }
    }

    try {
        if (!retirement_anchor.retire_subtrees(
                std::span<const pugi::xml_node>{retirement_roots.data(),
                                                retirement_roots.size()})) {
            rollback_replacement_children();
            return false;
        }
    } catch (...) {
        rollback_replacement_children();
        throw;
    }

    for (std::size_t index = 0U; index < old_body_root_count; ++index) {
        const auto child = retirement_roots[index];
        auto parent = child.parent();
        if (!parent.remove_child(child)) {
            return false;
        }
    }
    return true;
}

auto replace_table_cell_texts(
    std::span<const tracked_table_cell_text_replacement> replacements) -> bool {
    if (replacements.empty()) {
        return true;
    }

    try {
        pugi::xml_document staging_document;
        const auto staging_root = checked_append_xml_element(
            staging_document, "featherdoc-table-cell-text-replacements");
        if (staging_root == pugi::xml_node{}) {
            return false;
        }

        auto body_replacements = std::vector<tracked_cell_body_replacement>{};
        body_replacements.reserve(replacements.size());
        for (const auto &replacement : replacements) {
            if (!replacement.target_cell.has_node() ||
                replacement.text == nullptr) {
                return false;
            }

            const auto source_cell =
                append_staged_table_cell_text(staging_root, replacement.text);
            if (source_cell == pugi::xml_node{}) {
                return false;
            }
            body_replacements.push_back(tracked_cell_body_replacement{
                replacement.target_cell, source_cell});
        }

        return replace_cell_body_contents(replacements.front().target_cell,
                                          body_replacements);
    } catch (const std::bad_alloc &) {
        return false;
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
