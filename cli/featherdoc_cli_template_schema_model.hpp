#pragma once

#include "featherdoc_cli_validation_part.hpp"

#include <featherdoc.hpp>

#include <algorithm>
#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace featherdoc_cli {

struct exported_template_schema_target {
    validation_part_family part = validation_part_family::body;
    std::optional<std::size_t> part_index;
    std::optional<std::size_t> section_index;
    std::optional<featherdoc::section_reference_kind> reference_kind;
    std::optional<std::size_t> resolved_from_section_index;
    bool linked_to_previous = false;
    std::string entry_name;
    std::vector<featherdoc::template_slot_requirement> requirements;
};

struct exported_template_schema_skipped_bookmark {
    validation_part_family part = validation_part_family::body;
    std::optional<std::size_t> part_index;
    std::optional<std::size_t> section_index;
    std::optional<featherdoc::section_reference_kind> reference_kind;
    std::optional<std::size_t> resolved_from_section_index;
    bool linked_to_previous = false;
    std::string entry_name;
    featherdoc::bookmark_summary bookmark;
    std::string reason;
};

struct exported_template_schema_result {
    std::vector<exported_template_schema_target> targets;
    std::vector<exported_template_schema_skipped_bookmark> skipped_bookmarks;

    [[nodiscard]] std::size_t slot_count() const noexcept {
        std::size_t total = 0U;
        for (const auto &target : this->targets) {
            total += target.requirements.size();
        }
        return total;
    }
};

struct template_schema_patch_target_selector {
    validation_part_family part = validation_part_family::body;
    std::optional<std::size_t> part_index;
    std::optional<std::size_t> section_index;
    std::optional<featherdoc::section_reference_kind> reference_kind;
    std::optional<std::size_t> resolved_from_section_index;
    bool linked_to_previous = false;
};

struct template_schema_patch_remove_slot {
    template_schema_patch_target_selector target{};
    std::string bookmark_name;
    featherdoc::template_slot_source_kind source{
        featherdoc::template_slot_source_kind::bookmark};
};

struct template_schema_patch_rename_slot {
    template_schema_patch_target_selector target{};
    std::string bookmark_name;
    std::string new_bookmark_name;
    featherdoc::template_slot_source_kind source{
        featherdoc::template_slot_source_kind::bookmark};
};

struct template_schema_patch_update_slot {
    template_schema_patch_target_selector target{};
    std::string bookmark_name;
    featherdoc::template_slot_source_kind source{
        featherdoc::template_slot_source_kind::bookmark};
    featherdoc::template_schema_slot_update update{};
};

struct template_schema_patch_document {
    std::vector<exported_template_schema_target> upsert_targets;
    std::vector<template_schema_patch_target_selector> remove_targets;
    std::vector<template_schema_patch_remove_slot> remove_slots;
    std::vector<template_schema_patch_rename_slot> rename_slots;
    std::vector<template_schema_patch_update_slot> update_slots;
};

struct merged_template_schema_summary {
    std::size_t input_count = 0U;
    std::size_t updated_target_count = 0U;
    std::size_t replaced_slot_count = 0U;
};

struct patched_template_schema_summary {
    std::size_t upsert_target_count = 0U;
    std::size_t remove_target_count = 0U;
    std::size_t remove_slot_count = 0U;
    std::size_t rename_slot_count = 0U;
    std::size_t update_slot_count = 0U;
    std::size_t updated_target_count = 0U;
    std::size_t replaced_slot_count = 0U;
    std::size_t applied_remove_target_count = 0U;
    std::size_t applied_remove_slot_count = 0U;
    std::size_t applied_rename_slot_count = 0U;
    std::size_t applied_update_slot_count = 0U;
    std::size_t pruned_empty_target_count = 0U;
};

enum class template_schema_lint_issue_kind {
    target_order,
    slot_order,
    duplicate_target_identity,
    duplicate_slot_name,
    entry_name_present,
};

struct template_schema_lint_issue {
    template_schema_lint_issue_kind kind =
        template_schema_lint_issue_kind::target_order;
    std::size_t target_index = 0U;
    template_schema_patch_target_selector target{};
    std::optional<std::size_t> previous_target_index;
    std::optional<std::size_t> slot_index;
    std::optional<std::size_t> previous_slot_index;
    std::string bookmark_name;
    std::string previous_bookmark_name;
    std::string entry_name;
};

struct template_schema_lint_result {
    std::size_t target_count = 0U;
    std::size_t slot_count = 0U;
    std::vector<template_schema_lint_issue> issues;

    [[nodiscard]] bool clean() const noexcept { return this->issues.empty(); }

    [[nodiscard]] std::size_t
    issue_count(const template_schema_lint_issue_kind kind) const noexcept {
        return static_cast<std::size_t>(std::count_if(
            this->issues.begin(), this->issues.end(),
            [&](const template_schema_lint_issue &issue) {
                return issue.kind == kind;
            }));
    }
};

struct repaired_template_schema_summary {
    std::size_t input_target_count = 0U;
    std::size_t input_slot_count = 0U;
    std::size_t merged_duplicate_target_count = 0U;
    std::size_t deduplicated_target_count = 0U;
    std::size_t deduplicated_slot_count = 0U;
    std::size_t stripped_entry_name_count = 0U;
    std::size_t replaced_slot_count = 0U;
    bool changed = false;
};

struct changed_template_schema_target {
    exported_template_schema_target left;
    exported_template_schema_target right;
};

struct template_schema_diff_result {
    std::vector<exported_template_schema_target> added_targets;
    std::vector<exported_template_schema_target> removed_targets;
    std::vector<changed_template_schema_target> changed_targets;

    [[nodiscard]] bool equal() const noexcept {
        return this->added_targets.empty() && this->removed_targets.empty() &&
               this->changed_targets.empty();
    }
};

struct built_template_schema_patch_summary {
    std::size_t added_target_count = 0U;
    std::size_t removed_target_count = 0U;
    std::size_t changed_target_count = 0U;
    std::size_t generated_remove_target_count = 0U;
    std::size_t generated_remove_slot_count = 0U;
    std::size_t generated_rename_slot_count = 0U;
    std::size_t generated_update_slot_count = 0U;
    std::size_t generated_upsert_target_count = 0U;

    [[nodiscard]] bool empty_patch() const noexcept {
        return this->generated_remove_target_count == 0U &&
               this->generated_remove_slot_count == 0U &&
               this->generated_rename_slot_count == 0U &&
               this->generated_update_slot_count == 0U &&
               this->generated_upsert_target_count == 0U;
    }
};

} // namespace featherdoc_cli
