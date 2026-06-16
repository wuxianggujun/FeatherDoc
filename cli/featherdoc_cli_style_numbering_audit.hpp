#pragma once

#include <featherdoc.hpp>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace featherdoc_cli {

enum class style_numbering_audit_issue_kind {
    non_paragraph_numbering,
    missing_num_id,
    missing_level,
    orphan_instance,
    missing_definition,
    missing_level_definition,
    duplicate_definition_name,
    based_on_definition_mismatch,
};

struct style_numbering_audit_issue {
    style_numbering_audit_issue_kind kind{};
    std::string style_id;
    std::string style_name;
    std::optional<std::string> based_on;
    std::optional<std::uint32_t> num_id;
    std::optional<std::uint32_t> level;
    std::optional<std::uint32_t> definition_id;
    std::optional<std::string> definition_name;
    std::string message;
};

enum class style_numbering_repair_action {
    clear_style_numbering,
    relink_style_numbering,
    add_numbering_level,
    rename_numbering_definition,
    align_with_based_on_numbering,
    manual_review,
};

struct style_numbering_repair_suggestion {
    style_numbering_repair_action action{};
    style_numbering_audit_issue_kind issue_kind{};
    std::string style_id;
    std::string rationale;
    std::string command_template;
    std::optional<std::uint32_t> target_definition_id;
    std::optional<std::string> target_definition_name;
    std::optional<std::uint32_t> target_level;
};

struct style_numbering_audit_result {
    std::size_t paragraph_style_count{};
    std::vector<featherdoc::style_summary> numbered_styles;
    std::vector<style_numbering_audit_issue> issues;
    std::vector<style_numbering_repair_suggestion> suggestions;
};

struct style_numbering_repair_result {
    bool apply = false;
    std::optional<std::filesystem::path> catalog_path;
    std::optional<featherdoc::numbering_catalog_import_summary> catalog_import;
    std::optional<std::filesystem::path> output_path;
    style_numbering_audit_result before;
    std::optional<style_numbering_audit_result> after;
    std::vector<style_numbering_repair_suggestion> applyable_suggestions;
    std::size_t applied_count{};
};

[[nodiscard]] auto style_numbering_audit_clean(
    const style_numbering_audit_result &result) -> bool;
[[nodiscard]] auto style_numbering_audit_issue_kind_name(
    style_numbering_audit_issue_kind kind) -> std::string_view;
[[nodiscard]] auto style_numbering_repair_action_name(
    style_numbering_repair_action action) -> std::string_view;
[[nodiscard]] auto style_numbering_repair_suggestion_applyable(
    const style_numbering_repair_suggestion &suggestion) -> bool;
[[nodiscard]] auto collect_applyable_style_numbering_repair_suggestions(
    const std::vector<style_numbering_repair_suggestion> &suggestions)
    -> std::vector<style_numbering_repair_suggestion>;
[[nodiscard]] auto audit_style_numbering(
    const std::vector<featherdoc::style_summary> &styles,
    const std::vector<featherdoc::numbering_definition_summary> &definitions)
    -> style_numbering_audit_result;

} // namespace featherdoc_cli
