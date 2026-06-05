#pragma once

#include "featherdoc_cli_table_cell_options_parse.hpp"
#include "featherdoc_cli_template_inspect_options_parse.hpp"

#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace featherdoc_cli {

struct template_table_cell_text_options : table_cell_text_options {
    validation_part_family part = validation_part_family::body;
    std::optional<std::size_t> part_index;
    std::optional<std::size_t> section_index;
    featherdoc::section_reference_kind reference_kind =
        featherdoc::section_reference_kind::default_reference;
    std::optional<std::string> bookmark_name;
    bool has_part = false;
    bool has_kind = false;
};

struct template_table_cell_mutation_options : table_cell_style_options {
    validation_part_family part = validation_part_family::body;
    std::optional<std::size_t> part_index;
    std::optional<std::size_t> section_index;
    featherdoc::section_reference_kind reference_kind =
        featherdoc::section_reference_kind::default_reference;
    std::optional<std::string> bookmark_name;
    bool has_part = false;
    bool has_kind = false;
};

struct template_append_table_row_options : append_table_row_options {
    validation_part_family part = validation_part_family::body;
    std::optional<std::size_t> part_index;
    std::optional<std::size_t> section_index;
    featherdoc::section_reference_kind reference_kind =
        featherdoc::section_reference_kind::default_reference;
    std::optional<std::string> bookmark_name;
    bool has_part = false;
    bool has_kind = false;
};

struct template_table_row_mutation_options : table_cell_style_options {
    validation_part_family part = validation_part_family::body;
    std::optional<std::size_t> part_index;
    std::optional<std::size_t> section_index;
    featherdoc::section_reference_kind reference_kind =
        featherdoc::section_reference_kind::default_reference;
    std::optional<std::string> bookmark_name;
    bool has_part = false;
    bool has_kind = false;
};

struct template_table_row_texts_options {
    validation_part_family part = validation_part_family::body;
    std::optional<std::size_t> part_index;
    std::optional<std::size_t> section_index;
    featherdoc::section_reference_kind reference_kind =
        featherdoc::section_reference_kind::default_reference;
    std::vector<std::vector<std::string>> rows;
    std::optional<std::string> bookmark_name;
    std::optional<std::filesystem::path> output_path;
    bool has_part = false;
    bool has_kind = false;
    bool json_output = false;
};

struct template_merge_table_cells_options : merge_table_cells_options {
    validation_part_family part = validation_part_family::body;
    std::optional<std::size_t> part_index;
    std::optional<std::size_t> section_index;
    featherdoc::section_reference_kind reference_kind =
        featherdoc::section_reference_kind::default_reference;
    std::optional<std::string> bookmark_name;
    bool has_part = false;
    bool has_kind = false;
};

struct template_unmerge_table_cells_options : unmerge_table_cells_options {
    validation_part_family part = validation_part_family::body;
    std::optional<std::size_t> part_index;
    std::optional<std::size_t> section_index;
    featherdoc::section_reference_kind reference_kind =
        featherdoc::section_reference_kind::default_reference;
    std::optional<std::string> bookmark_name;
    bool has_part = false;
    bool has_kind = false;
};

[[nodiscard]] auto parse_template_table_cell_text_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    template_table_cell_text_options &options, std::string &error_message)
    -> bool;

[[nodiscard]] auto parse_template_table_cell_mutation_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    template_table_cell_mutation_options &options, std::string &error_message)
    -> bool;

[[nodiscard]] auto parse_template_table_row_mutation_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    template_table_row_mutation_options &options, std::string &error_message)
    -> bool;

[[nodiscard]] auto parse_template_table_row_texts_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    template_table_row_texts_options &options, std::string &error_message)
    -> bool;

[[nodiscard]] auto parse_template_append_table_row_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    template_append_table_row_options &options, std::string &error_message)
    -> bool;

[[nodiscard]] auto parse_template_merge_table_cells_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    template_merge_table_cells_options &options, std::string &error_message)
    -> bool;

[[nodiscard]] auto parse_template_unmerge_table_cells_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    template_unmerge_table_cells_options &options, std::string &error_message)
    -> bool;

} // namespace featherdoc_cli
