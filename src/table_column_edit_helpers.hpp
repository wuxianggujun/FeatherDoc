#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <utility>
#include <vector>

#include <featherdoc/detail/xml_handle.hpp>
#include <pugixml.hpp>

namespace featherdoc::detail {

struct row_cell_cover_result final {
    pugi::xml_node cell;
    std::size_t start_column{};
    std::size_t span{};
};

struct table_column_removal_target final {
    pugi::xml_node row;
    pugi::xml_node cell;
};

struct table_column_removal_plan final {
    std::size_t column_index{};
    std::vector<table_column_removal_target> targets;
};

struct table_column_insertion_target final {
    pugi::xml_node row;
    pugi::xml_node clone_source;
    pugi::xml_node insert_before;
};

struct table_column_insertion_plan final {
    std::size_t boundary_column_index{};
    std::size_t column_count_before_insertion{};
    std::size_t grid_width_source_column_index{};
    std::vector<table_column_insertion_target> targets;
};

struct vertical_merge_chain_plan final {
    pugi::xml_node anchor_cell;
    std::vector<pugi::xml_node> cells;
};

struct tracked_cell_body_replacement final {
    tracked_xml_node target_cell;
    pugi::xml_node source_cell;
};

struct tracked_table_cell_text_replacement final {
    tracked_xml_node target_cell;
    const char *text{};
};

struct staged_cell_properties final {
    pugi::xml_node cell;
    pugi::xml_node original;
    pugi::xml_node replacement;
};

struct staged_table_child final {
    pugi::xml_node table;
    pugi::xml_node original;
    pugi::xml_node replacement;
};

enum class table_grid_edit_kind {
    normalize = 0,
    insert_column,
    remove_column,
};

struct table_grid_edit final {
    table_grid_edit_kind kind{table_grid_edit_kind::normalize};
    std::size_t column_index{};
    std::size_t source_column_index{};
};

struct staged_table_layout final {
    pugi::xml_node table;
    pugi::xml_node original_properties;
    pugi::xml_node replacement_properties;
    pugi::xml_node original_grid;
    pugi::xml_node replacement_grid;
};

[[nodiscard]] auto cell_column_index(pugi::xml_node cell)
    -> std::optional<std::size_t>;
void synchronize_fixed_layout_cell_widths_from_grid(pugi::xml_node table);
void clear_fixed_layout_cell_widths_covering_column(
    pugi::xml_node table, std::size_t target_column_index);
[[nodiscard]] auto find_row_cell_at_columns(pugi::xml_node row,
                                            std::size_t target_column_index,
                                            std::size_t target_column_span)
    -> pugi::xml_node;
[[nodiscard]] auto find_row_cell_covering_column(
    pugi::xml_node row, std::size_t target_column_index) -> row_cell_cover_result;
[[nodiscard]] auto plan_table_column_removal(pugi::xml_node cell)
    -> std::optional<table_column_removal_plan>;
[[nodiscard]] auto plan_table_column_insertion(pugi::xml_node cell,
                                               bool insert_after)
    -> std::optional<table_column_insertion_plan>;
[[nodiscard]] auto plan_vertical_merge_chain(pugi::xml_node cell)
    -> std::optional<vertical_merge_chain_plan>;
[[nodiscard]] auto remove_table_grid_column(pugi::xml_node table,
                                            std::size_t target_column_index)
    -> bool;
[[nodiscard]] auto insert_table_grid_column(
    pugi::xml_node table, std::size_t boundary_column_index,
    std::size_t column_count_before_insertion, std::size_t source_column_index)
    -> bool;
void remove_empty_cell_properties(pugi::xml_node cell);
[[nodiscard]] auto insert_empty_clone_cell(pugi::xml_node row,
                                           pugi::xml_node source_cell,
                                           pugi::xml_node insert_before)
    -> pugi::xml_node;
void rollback_inserted_table_cells(
    const std::vector<pugi::xml_node> &inserted_cells);
[[nodiscard]] auto stage_cell_properties(pugi::xml_node cell)
    -> std::optional<staged_cell_properties>;
void rollback_staged_cell_properties(
    const std::vector<staged_cell_properties> &staged_properties) noexcept;
[[nodiscard]] auto commit_staged_cell_properties(
    const std::vector<staged_cell_properties> &staged_properties) noexcept
    -> bool;
[[nodiscard]] auto stage_table_child(pugi::xml_node table,
                                     const char *child_name,
                                     pugi::xml_node insertion_anchor)
    -> std::optional<staged_table_child>;
void rollback_staged_table_child(
    const staged_table_child &staged_child) noexcept;
[[nodiscard]] auto
commit_staged_table_child(const staged_table_child &staged_child) noexcept
    -> bool;
[[nodiscard]] auto stage_table_layout(pugi::xml_node table,
                                      std::size_t normalized_column_count,
                                      table_grid_edit grid_edit = {})
    -> std::optional<staged_table_layout>;
void rollback_staged_table_layout(
    const staged_table_layout &staged_layout) noexcept;
[[nodiscard]] auto commit_staged_table_layout(
    const staged_table_layout &staged_layout) noexcept -> bool;
[[nodiscard]] auto stage_fixed_layout_cell_widths(
    pugi::xml_node table, std::span<const pugi::xml_node> excluded_cells,
    std::vector<staged_cell_properties> &staged_properties) -> bool;
[[nodiscard]] auto stage_cleared_fixed_layout_cell_widths_covering_column(
    pugi::xml_node table, std::size_t target_column_index,
    std::vector<staged_cell_properties> &staged_properties) -> bool;
[[nodiscard]] auto
clear_cell_contents_for_vertical_merge(
    const std::vector<tracked_xml_node> &cells,
    std::span<const pugi::xml_node> additional_retirement_roots = {}) -> bool;
[[nodiscard]] auto replace_cell_body_contents(
    const tracked_xml_node &retirement_anchor,
    const std::vector<tracked_cell_body_replacement> &replacements,
    std::span<const pugi::xml_node> additional_retirement_roots = {}) -> bool;
[[nodiscard]] auto replace_table_cell_texts(
    std::span<const tracked_table_cell_text_replacement> replacements) -> bool;
[[nodiscard]] auto successor_vertical_merge_promotions_for_row_removal(
    pugi::xml_node row) -> std::vector<std::pair<pugi::xml_node, pugi::xml_node>>;

} // namespace featherdoc::detail
