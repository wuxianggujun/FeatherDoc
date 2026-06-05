#pragma once

#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace featherdoc_cli {

struct inspect_options {
    bool json_output = false;
};

struct audit_style_numbering_options {
    bool fail_on_issue = false;
    bool json_output = false;
};

struct audit_table_style_regions_options {
    std::optional<std::string> style_id;
    bool fail_on_issue = false;
    bool json_output = false;
};

struct audit_table_style_inheritance_options {
    std::optional<std::string> style_id;
    bool fail_on_issue = false;
    bool json_output = false;
};

struct audit_table_style_quality_options {
    bool fail_on_issue = false;
    bool json_output = false;
};

struct plan_table_style_quality_fixes_options {
    bool fail_on_issue = false;
    bool json_output = false;
};

struct apply_table_style_quality_fixes_options {
    bool look_only = false;
    std::optional<std::filesystem::path> output_path;
    bool json_output = false;
};

struct repair_style_numbering_options {
    bool plan_only = false;
    bool apply = false;
    std::optional<std::filesystem::path> catalog_path;
    std::optional<std::filesystem::path> output_path;
    bool json_output = false;
};

[[nodiscard]] auto parse_inspect_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    inspect_options &options, std::string &error_message) -> bool;

[[nodiscard]] auto parse_audit_style_numbering_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    audit_style_numbering_options &options, std::string &error_message)
    -> bool;

[[nodiscard]] auto parse_audit_table_style_regions_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    audit_table_style_regions_options &options, std::string &error_message)
    -> bool;

[[nodiscard]] auto parse_audit_table_style_inheritance_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    audit_table_style_inheritance_options &options,
    std::string &error_message) -> bool;

[[nodiscard]] auto parse_audit_table_style_quality_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    audit_table_style_quality_options &options, std::string &error_message)
    -> bool;

[[nodiscard]] auto parse_plan_table_style_quality_fixes_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    plan_table_style_quality_fixes_options &options,
    std::string &error_message) -> bool;

[[nodiscard]] auto parse_apply_table_style_quality_fixes_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    apply_table_style_quality_fixes_options &options,
    std::string &error_message) -> bool;

[[nodiscard]] auto parse_repair_style_numbering_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    repair_style_numbering_options &options, std::string &error_message)
    -> bool;

} // namespace featherdoc_cli
