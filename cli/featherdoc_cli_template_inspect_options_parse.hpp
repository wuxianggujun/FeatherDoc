#pragma once

#include "featherdoc_cli_parse.hpp"
#include "featherdoc_cli_validation_part.hpp"

#include <featherdoc.hpp>

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace featherdoc_cli {

struct template_inspect_paragraphs_options {
    validation_part_family part = validation_part_family::body;
    std::optional<std::size_t> part_index;
    std::optional<std::size_t> section_index;
    featherdoc::section_reference_kind reference_kind =
        featherdoc::section_reference_kind::default_reference;
    std::optional<std::size_t> paragraph_index;
    bool has_part = false;
    bool has_kind = false;
    bool json_output = false;
};

struct template_inspect_runs_options {
    validation_part_family part = validation_part_family::body;
    std::optional<std::size_t> part_index;
    std::optional<std::size_t> section_index;
    featherdoc::section_reference_kind reference_kind =
        featherdoc::section_reference_kind::default_reference;
    std::optional<std::size_t> run_index;
    bool has_part = false;
    bool has_kind = false;
    bool json_output = false;
};

struct template_inspect_tables_options {
    validation_part_family part = validation_part_family::body;
    std::optional<std::size_t> part_index;
    std::optional<std::size_t> section_index;
    featherdoc::section_reference_kind reference_kind =
        featherdoc::section_reference_kind::default_reference;
    std::optional<std::size_t> table_index;
    std::optional<std::string> bookmark_name;
    bool has_part = false;
    bool has_kind = false;
    bool json_output = false;
};

struct template_inspect_table_cells_options {
    validation_part_family part = validation_part_family::body;
    std::optional<std::size_t> part_index;
    std::optional<std::size_t> section_index;
    featherdoc::section_reference_kind reference_kind =
        featherdoc::section_reference_kind::default_reference;
    std::optional<std::size_t> row_index;
    std::optional<std::size_t> cell_index;
    std::optional<std::size_t> grid_column;
    std::optional<std::string> bookmark_name;
    bool has_part = false;
    bool has_kind = false;
    bool json_output = false;
};

struct template_inspect_table_rows_options {
    validation_part_family part = validation_part_family::body;
    std::optional<std::size_t> part_index;
    std::optional<std::size_t> section_index;
    featherdoc::section_reference_kind reference_kind =
        featherdoc::section_reference_kind::default_reference;
    std::optional<std::size_t> row_index;
    featherdoc::template_table_selector selector;
    bool has_header_row_index = false;
    bool has_occurrence = false;
    bool has_part = false;
    bool has_kind = false;
    bool json_output = false;
};

[[nodiscard]] auto parse_template_part_selection_option(
    const std::vector<std::string_view> &arguments, std::size_t &index,
    validation_part_family &part, std::optional<std::size_t> &part_index,
    std::optional<std::size_t> &section_index,
    featherdoc::section_reference_kind &reference_kind, bool &has_part,
    bool &has_kind, std::string &error_message) -> option_parse_result;

[[nodiscard]] auto parse_template_table_bookmark_option(
    const std::vector<std::string_view> &arguments, std::size_t &index,
    std::optional<std::string> &bookmark_name, std::string &error_message)
    -> option_parse_result;

[[nodiscard]] auto template_table_selector_uses_text_matching(
    const featherdoc::template_table_selector &selector) -> bool;

[[nodiscard]] auto validate_template_table_selector(
    const std::optional<std::size_t> &table_index,
    const std::optional<std::string> &bookmark_name, bool allow_none,
    std::string &error_message) -> bool;

[[nodiscard]] auto validate_template_table_selector(
    const featherdoc::template_table_selector &selector, bool allow_none,
    bool has_header_row_index, bool has_occurrence,
    std::string &error_message) -> bool;

[[nodiscard]] auto parse_template_table_selector_option(
    const std::vector<std::string_view> &arguments, std::size_t &index,
    featherdoc::template_table_selector &selector, bool &has_header_row_index,
    bool &has_occurrence, std::string &error_message) -> option_parse_result;

[[nodiscard]] auto parse_template_inspect_paragraphs_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    template_inspect_paragraphs_options &options, std::string &error_message)
    -> bool;

[[nodiscard]] auto parse_template_inspect_runs_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    template_inspect_runs_options &options, std::string &error_message) -> bool;

[[nodiscard]] auto parse_template_inspect_tables_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    template_inspect_tables_options &options, std::string &error_message) -> bool;

[[nodiscard]] auto parse_template_inspect_table_cells_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    template_inspect_table_cells_options &options,
    std::string &error_message) -> bool;

[[nodiscard]] auto parse_template_inspect_table_rows_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    template_inspect_table_rows_options &options, std::string &error_message)
    -> bool;

} // namespace featherdoc_cli
