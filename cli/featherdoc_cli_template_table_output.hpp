#pragma once

#include "featherdoc_cli_command_support.hpp"
#include "featherdoc_cli_template_part_selection.hpp"
#include "featherdoc_cli_template_table_options_parse.hpp"

#include <featherdoc.hpp>

#include <cstddef>
#include <iosfwd>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace featherdoc_cli {

struct applied_template_table_json_batch_operation {
    std::size_t operation_index = 0U;
    selected_template_part selected;
    std::size_t table_index = 0U;
    featherdoc::template_table_selector selector;
    template_table_json_patch patch;
};

void write_json_template_table_row_texts_result(
    std::ostream &stream, const selected_template_part &selected,
    std::size_t table_index, std::size_t start_row_index,
    const std::vector<std::vector<std::string>> &rows,
    const std::optional<std::string> &bookmark_name);

void print_template_table_row_texts_result(
    const selected_template_part &selected, std::size_t table_index,
    std::size_t start_row_index, const std::vector<std::vector<std::string>> &rows,
    const std::optional<std::string> &bookmark_name,
    const std::optional<path_type> &output_path);

void write_json_template_table_cell_block_texts_result(
    std::ostream &stream, const selected_template_part &selected,
    std::size_t table_index, std::size_t start_row_index,
    std::size_t start_cell_index, const std::vector<std::vector<std::string>> &rows,
    const std::optional<std::string> &bookmark_name);

void print_template_table_cell_block_texts_result(
    const selected_template_part &selected, std::size_t table_index,
    std::size_t start_row_index, std::size_t start_cell_index,
    const std::vector<std::vector<std::string>> &rows,
    const std::optional<std::string> &bookmark_name,
    const std::optional<path_type> &output_path);

[[nodiscard]] auto
template_table_json_patch_mode_name(template_table_json_patch_mode mode)
    -> std::string_view;

void write_json_template_table_selector_fields(
    std::ostream &stream, const featherdoc::template_table_selector &selector);

void print_template_table_selector_fields(
    std::ostream &stream, const featherdoc::template_table_selector &selector);

void write_json_template_table_from_json_result(
    std::ostream &stream, const selected_template_part &selected,
    std::size_t table_index, const template_table_json_patch &patch,
    const featherdoc::template_table_selector &selector);

void print_template_table_from_json_result(
    const selected_template_part &selected, std::size_t table_index,
    const template_table_json_patch &patch,
    const featherdoc::template_table_selector &selector,
    const std::optional<path_type> &output_path);

void write_json_template_tables_from_json_result(
    std::ostream &stream,
    const std::vector<applied_template_table_json_batch_operation> &operations);

void print_template_tables_from_json_result(
    const std::vector<applied_template_table_json_batch_operation> &operations,
    const std::optional<path_type> &output_path);

} // namespace featherdoc_cli
