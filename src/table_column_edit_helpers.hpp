#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

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
void clear_cell_contents_for_vertical_merge(pugi::xml_node cell);
void replace_cell_body_contents(pugi::xml_node target_cell,
                                pugi::xml_node source_cell);
[[nodiscard]] auto successor_vertical_merge_promotions_for_row_removal(
    pugi::xml_node row) -> std::vector<std::pair<pugi::xml_node, pugi::xml_node>>;

} // namespace featherdoc::detail