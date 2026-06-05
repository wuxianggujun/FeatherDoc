#pragma once

#include "featherdoc_cli_template_inspect_options_parse.hpp"

#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace featherdoc_cli {

enum class template_table_json_patch_mode {
    rows,
    block,
};

struct template_table_json_patch {
    template_table_json_patch_mode mode = template_table_json_patch_mode::rows;
    std::size_t start_row_index = 0U;
    std::size_t start_cell_index = 0U;
    std::vector<std::vector<std::string>> rows;
};

struct template_table_json_patch_options {
    validation_part_family part = validation_part_family::body;
    std::optional<std::size_t> part_index;
    std::optional<std::size_t> section_index;
    featherdoc::section_reference_kind reference_kind =
        featherdoc::section_reference_kind::default_reference;
    featherdoc::template_table_selector selector;
    std::optional<std::filesystem::path> patch_file;
    std::optional<std::filesystem::path> output_path;
    bool has_header_row_index = false;
    bool has_occurrence = false;
    bool has_part = false;
    bool has_kind = false;
    bool json_output = false;
};

struct template_table_json_batch_operation {
    std::optional<validation_part_family> part;
    std::optional<std::size_t> part_index;
    std::optional<std::size_t> section_index;
    std::optional<featherdoc::section_reference_kind> reference_kind;
    featherdoc::template_table_selector selector;
    template_table_json_patch patch;
};

struct template_table_json_batch_options {
    validation_part_family part = validation_part_family::body;
    std::optional<std::size_t> part_index;
    std::optional<std::size_t> section_index;
    featherdoc::section_reference_kind reference_kind =
        featherdoc::section_reference_kind::default_reference;
    std::optional<std::filesystem::path> patch_file;
    std::optional<std::filesystem::path> output_path;
    bool has_part = false;
    bool has_kind = false;
    bool json_output = false;
};

struct parsed_template_table_selector_target {
    featherdoc::template_table_selector selector;
    std::size_t options_start_index = 0U;
    bool has_header_row_index = false;
    bool has_occurrence = false;
};

struct parsed_template_table_selector_row_target
    : parsed_template_table_selector_target {
    std::size_t row_index = 0U;
};

struct parsed_template_table_selector_cell_target
    : parsed_template_table_selector_row_target {
    std::size_t cell_index = 0U;
};

struct parsed_template_table_selector_optional_cell_target
    : parsed_template_table_selector_row_target {
    std::optional<std::size_t> cell_index;
};

[[nodiscard]] auto parse_template_table_target_arguments(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    std::optional<std::size_t> &table_index,
    std::optional<std::string> &bookmark_name,
    std::size_t &options_start_index, std::string &error_message) -> bool;

[[nodiscard]] auto parse_template_table_selector_target_arguments(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    parsed_template_table_selector_target &target, bool allow_empty_target,
    std::string &error_message) -> bool;

[[nodiscard]] auto parse_template_table_selector_row_target_arguments(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    parsed_template_table_selector_row_target &target,
    std::string &error_message) -> bool;

[[nodiscard]] auto parse_template_table_selector_cell_target_arguments(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    parsed_template_table_selector_cell_target &target,
    std::string &error_message) -> bool;

[[nodiscard]] auto parse_template_table_selector_optional_cell_target_arguments(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    parsed_template_table_selector_optional_cell_target &target,
    std::string &error_message) -> bool;

[[nodiscard]] auto parse_template_table_json_patch_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    template_table_json_patch_options &options, std::string &error_message)
    -> bool;

[[nodiscard]] auto parse_template_table_json_batch_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    template_table_json_batch_options &options, std::string &error_message)
    -> bool;

} // namespace featherdoc_cli
