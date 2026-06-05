#pragma once

#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace featherdoc_cli {

struct export_template_schema_options {
    std::optional<std::filesystem::path> output_path;
    bool section_targets = false;
    bool resolved_section_targets = false;
    bool json_output = false;
};

struct normalize_template_schema_options {
    std::optional<std::filesystem::path> output_path;
    bool json_output = false;
};

struct lint_template_schema_options {
    bool json_output = false;
};

struct repair_template_schema_options {
    std::optional<std::filesystem::path> output_path;
    bool json_output = false;
};

struct merge_template_schema_options {
    std::vector<std::filesystem::path> schema_paths;
    std::optional<std::filesystem::path> output_path;
    bool json_output = false;
};

struct patch_template_schema_options {
    std::optional<std::filesystem::path> patch_path;
    std::optional<std::filesystem::path> output_path;
    bool json_output = false;
};

struct preview_template_schema_patch_options {
    std::optional<std::filesystem::path> patch_path;
    std::optional<std::filesystem::path> output_patch_path;
    std::optional<std::filesystem::path> review_json_path;
    std::optional<std::filesystem::path> right_schema_path;
    bool json_output = false;
};

struct build_template_schema_patch_options {
    std::optional<std::filesystem::path> output_path;
    std::optional<std::filesystem::path> review_json_path;
    bool json_output = false;
};

struct diff_template_schema_options {
    bool json_output = false;
    bool fail_on_diff = false;
};

struct check_template_schema_options {
    std::optional<std::filesystem::path> schema_path;
    std::optional<std::filesystem::path> output_path;
    bool section_targets = false;
    bool resolved_section_targets = false;
    bool json_output = false;
};

[[nodiscard]] auto parse_export_template_schema_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    export_template_schema_options &options, std::string &error_message) -> bool;

[[nodiscard]] auto parse_normalize_template_schema_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    normalize_template_schema_options &options, std::string &error_message)
    -> bool;

[[nodiscard]] auto parse_lint_template_schema_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    lint_template_schema_options &options, std::string &error_message) -> bool;

[[nodiscard]] auto parse_repair_template_schema_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    repair_template_schema_options &options, std::string &error_message) -> bool;

[[nodiscard]] auto parse_merge_template_schema_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    merge_template_schema_options &options, std::string &error_message) -> bool;

[[nodiscard]] auto parse_patch_template_schema_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    patch_template_schema_options &options, std::string &error_message) -> bool;

[[nodiscard]] auto parse_preview_template_schema_patch_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    preview_template_schema_patch_options &options,
    std::string &error_message) -> bool;

[[nodiscard]] auto parse_diff_template_schema_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    diff_template_schema_options &options, std::string &error_message) -> bool;

[[nodiscard]] auto parse_build_template_schema_patch_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    build_template_schema_patch_options &options, std::string &error_message)
    -> bool;

[[nodiscard]] auto parse_check_template_schema_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    check_template_schema_options &options, std::string &error_message) -> bool;

} // namespace featherdoc_cli
