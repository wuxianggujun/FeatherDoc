#pragma once

#include "featherdoc_cli_command_support.hpp"
#include "featherdoc_cli_template_schema_model.hpp"

#include <featherdoc.hpp>

#include <cstddef>
#include <iosfwd>
#include <optional>
#include <string>

namespace featherdoc_cli {

struct previewed_template_schema_patch_summary {
    std::size_t left_slot_count = 0U;
    std::optional<path_type> output_patch_path;
    std::optional<path_type> review_json_path;
    std::optional<std::size_t> right_slot_count;
    std::optional<std::size_t> upsert_slot_count;
    std::optional<std::size_t> remove_target_count;
    std::optional<std::size_t> remove_slot_count;
    std::optional<std::size_t> rename_slot_count;
    std::optional<std::size_t> update_slot_count;
    featherdoc::template_schema_patch_summary applied{};
};

void write_json_exported_template_schema(
    std::ostream &stream, const exported_template_schema_result &result);
void write_json_template_schema_patch_document(
    std::ostream &stream, const template_schema_patch_document &document);

void print_exported_template_schema_summary(
    const exported_template_schema_result &result,
    const std::optional<path_type> &output_path, bool json_output);
void print_normalized_template_schema_summary(
    const exported_template_schema_result &result,
    const std::optional<path_type> &output_path, bool json_output);
void print_linted_template_schema_result(
    const template_schema_lint_result &result, bool json_output);
void print_repaired_template_schema_summary(
    const exported_template_schema_result &result,
    const repaired_template_schema_summary &summary,
    const std::optional<path_type> &output_path, bool json_output);
void print_merged_template_schema_summary(
    const exported_template_schema_result &result,
    const merged_template_schema_summary &summary,
    const std::optional<path_type> &output_path, bool json_output);
void print_patched_template_schema_summary(
    const exported_template_schema_result &result,
    const patched_template_schema_summary &summary,
    const std::optional<path_type> &output_path, bool json_output);
void print_previewed_template_schema_patch_summary(
    const previewed_template_schema_patch_summary &summary, bool json_output);
void print_built_template_schema_patch_summary(
    const built_template_schema_patch_summary &summary,
    const std::optional<path_type> &output_path,
    const std::optional<path_type> &review_json_path, bool json_output);
void print_template_schema_diff_result(
    const template_schema_diff_result &result, bool json_output);
void print_checked_template_schema_result(
    const path_type &schema_path, const template_schema_diff_result &result,
    const std::optional<path_type> &output_path, bool json_output);

[[nodiscard]] auto write_exported_template_schema_file(
    const path_type &output_path, const exported_template_schema_result &result,
    std::string &error_message) -> bool;
[[nodiscard]] auto write_template_schema_patch_file(
    const path_type &output_path, const template_schema_patch_document &patch,
    std::string &error_message) -> bool;
[[nodiscard]] auto write_template_schema_patch_review_file(
    const path_type &output_path,
    const featherdoc::template_schema_patch_review_summary &summary,
    std::string &error_message) -> bool;

} // namespace featherdoc_cli
