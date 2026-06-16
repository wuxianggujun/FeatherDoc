#pragma once

#include <constants.hpp>

#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <iosfwd>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace featherdoc {

enum class bookmark_kind : std::uint8_t {
    text = 0U,
    block,
    table_rows,
    block_range,
    malformed,
    mixed,
};

struct bookmark_summary {
    std::string bookmark_name;
    std::size_t occurrence_count{};
    featherdoc::bookmark_kind kind{featherdoc::bookmark_kind::text};

    [[nodiscard]] bool is_duplicate() const noexcept {
        return this->occurrence_count > 1U;
    }
};

enum class template_slot_kind : std::uint8_t {
    text = 0U,
    table_rows,
    table,
    image,
    floating_image,
    block,
};

enum class template_slot_source_kind : std::uint8_t {
    bookmark = 0U,
    content_control_tag,
    content_control_alias,
};

struct template_slot_requirement {
    std::string bookmark_name;
    featherdoc::template_slot_kind kind{featherdoc::template_slot_kind::text};
    bool required{true};
    std::optional<std::size_t> min_occurrences;
    std::optional<std::size_t> max_occurrences;
    featherdoc::template_slot_source_kind source{
        featherdoc::template_slot_source_kind::bookmark};
};

struct template_kind_mismatch {
    std::string bookmark_name;
    featherdoc::template_slot_kind expected_kind{
        featherdoc::template_slot_kind::text};
    featherdoc::bookmark_kind actual_kind{featherdoc::bookmark_kind::text};
    std::size_t occurrence_count{};
};

struct template_occurrence_mismatch {
    std::string bookmark_name;
    std::size_t actual_occurrences{};
    std::size_t min_occurrences{};
    std::optional<std::size_t> max_occurrences;
};

struct template_validation_result {
    std::vector<std::string> missing_required;
    std::vector<std::string> duplicate_bookmarks;
    std::vector<std::string> malformed_placeholders;
    std::vector<featherdoc::bookmark_summary> unexpected_bookmarks;
    std::vector<featherdoc::template_kind_mismatch> kind_mismatches;
    std::vector<featherdoc::template_occurrence_mismatch> occurrence_mismatches;

    explicit operator bool() const noexcept {
        return this->missing_required.empty() &&
               this->duplicate_bookmarks.empty() &&
               this->malformed_placeholders.empty() &&
               this->unexpected_bookmarks.empty() &&
               this->kind_mismatches.empty() &&
               this->occurrence_mismatches.empty();
    }
};

enum class template_schema_part_kind : std::uint8_t {
    body = 0U,
    header,
    footer,
    section_header,
    section_footer,
};

struct template_schema_part_selector {
    featherdoc::template_schema_part_kind part{
        featherdoc::template_schema_part_kind::body};
    std::optional<std::size_t> part_index;
    std::optional<std::size_t> section_index;
    featherdoc::section_reference_kind reference_kind{
        featherdoc::section_reference_kind::default_reference};
};

struct template_schema_entry {
    featherdoc::template_schema_part_selector target{};
    featherdoc::template_slot_requirement requirement{};
};

struct template_schema {
    std::vector<featherdoc::template_schema_entry> entries;
};

enum class template_schema_scan_target_mode : std::uint8_t {
    related_parts = 0U,
    section_targets,
    resolved_section_targets,
};

struct template_schema_scan_options {
    featherdoc::template_schema_scan_target_mode target_mode{
        featherdoc::template_schema_scan_target_mode::related_parts};
    bool include_bookmark_slots{true};
    bool include_content_control_slots{true};
};

struct template_schema_scan_skipped_bookmark {
    featherdoc::template_schema_part_selector target{};
    std::string entry_name;
    featherdoc::bookmark_summary bookmark;
    std::string reason;
};

struct template_schema_scan_result {
    featherdoc::template_schema schema;
    std::vector<featherdoc::template_schema_scan_skipped_bookmark>
        skipped_bookmarks;

    [[nodiscard]] std::size_t slot_count() const noexcept {
        return this->schema.entries.size();
    }
};

struct template_schema_slot_selector {
    featherdoc::template_schema_part_selector target{};
    std::string bookmark_name;
    featherdoc::template_slot_source_kind source{
        featherdoc::template_slot_source_kind::bookmark};
};

struct template_schema_slot_rename {
    featherdoc::template_schema_slot_selector slot{};
    std::string new_bookmark_name;
};

struct template_schema_slot_update {
    std::optional<featherdoc::template_slot_kind> kind;
    std::optional<bool> required;
    std::optional<std::size_t> min_occurrences;
    std::optional<std::size_t> max_occurrences;
    bool clear_min_occurrences{false};
    bool clear_max_occurrences{false};
};

struct template_schema_slot_patch_update {
    featherdoc::template_schema_slot_selector slot{};
    featherdoc::template_schema_slot_update update{};
};

struct template_schema_patch {
    std::vector<featherdoc::template_schema_entry> upsert_slots;
    std::vector<featherdoc::template_schema_part_selector> remove_targets;
    std::vector<featherdoc::template_schema_slot_selector> remove_slots;
    std::vector<featherdoc::template_schema_slot_rename> rename_slots;
    std::vector<featherdoc::template_schema_slot_patch_update> update_slots;
};

struct template_schema_normalization_summary {
    std::size_t original_slots{};
    std::size_t final_slots{};
    std::size_t duplicate_slots_removed{};
    bool reordered_or_normalized{false};

    [[nodiscard]] bool changed() const noexcept {
        return this->duplicate_slots_removed > 0U ||
               this->original_slots != this->final_slots ||
               this->reordered_or_normalized;
    }
};

struct template_schema_patch_summary {
    std::size_t removed_targets{};
    std::size_t removed_slots{};
    std::size_t renamed_slots{};
    std::size_t inserted_slots{};
    std::size_t replaced_slots{};

    [[nodiscard]] bool changed() const noexcept {
        return this->removed_targets > 0U || this->removed_slots > 0U ||
               this->renamed_slots > 0U || this->inserted_slots > 0U ||
               this->replaced_slots > 0U;
    }
};

struct template_schema_patch_review_summary {
    std::size_t baseline_slot_count{};
    std::optional<std::size_t> generated_slot_count;
    std::size_t patch_upsert_slot_count{};
    std::size_t patch_remove_target_count{};
    std::size_t patch_remove_slot_count{};
    std::size_t patch_rename_slot_count{};
    std::size_t patch_update_slot_count{};
    featherdoc::template_schema_patch_summary preview{};

    [[nodiscard]] bool changed() const noexcept {
        return this->preview.changed();
    }
};

struct template_schema_part_validation_result {
    featherdoc::template_schema_part_selector target{};
    std::string entry_name;
    bool available{false};
    featherdoc::template_validation_result validation{};

    explicit operator bool() const noexcept {
        return static_cast<bool>(this->validation);
    }
};

struct template_schema_validation_result {
    std::vector<featherdoc::template_schema_part_validation_result>
        part_results;

    explicit operator bool() const noexcept {
        for (const auto &part_result : this->part_results) {
            if (!static_cast<bool>(part_result)) {
                return false;
            }
        }

        return true;
    }
};

enum class template_onboarding_issue_severity : std::uint8_t {
    info = 0U,
    warning,
    error,
};

enum class template_onboarding_next_action_kind : std::uint8_t {
    fix_template_slots = 0U,
    create_schema_baseline,
    review_schema_patch,
    prepare_render_data,
    run_project_template_smoke,
};

struct template_onboarding_issue {
    featherdoc::template_onboarding_issue_severity severity{
        featherdoc::template_onboarding_issue_severity::info};
    std::string code;
    std::string message;
    std::optional<featherdoc::template_schema_part_selector> target;
    std::optional<std::string> slot_name;
};

struct template_onboarding_next_action {
    featherdoc::template_onboarding_next_action_kind kind{
        featherdoc::template_onboarding_next_action_kind::prepare_render_data};
    std::string code;
    std::string message;
    bool blocking{false};
};

struct template_onboarding_options {
    featherdoc::template_schema_scan_options scan_options{};
    std::optional<featherdoc::template_schema> baseline_schema;
    bool validate_scanned_schema{true};
    bool validate_baseline_schema{true};
};

struct template_onboarding_result {
    featherdoc::template_schema_scan_result scan{};
    bool baseline_schema_available{false};
    std::optional<featherdoc::template_schema_validation_result>
        scanned_schema_validation;
    std::optional<featherdoc::template_schema_validation_result>
        baseline_validation;
    std::optional<featherdoc::template_schema_patch> schema_patch;
    std::optional<featherdoc::template_schema_patch_review_summary>
        patch_review;
    std::vector<featherdoc::template_onboarding_issue> issues;
    std::vector<featherdoc::template_onboarding_next_action> next_actions;

    [[nodiscard]] std::size_t slot_count() const noexcept {
        return this->scan.slot_count();
    }

    [[nodiscard]] bool has_errors() const noexcept {
        for (const auto &issue : this->issues) {
            if (issue.severity ==
                featherdoc::template_onboarding_issue_severity::error) {
                return true;
            }
        }

        return false;
    }

    [[nodiscard]] bool has_warnings() const noexcept {
        for (const auto &issue : this->issues) {
            if (issue.severity ==
                featherdoc::template_onboarding_issue_severity::warning) {
                return true;
            }
        }

        return false;
    }

    [[nodiscard]] bool requires_schema_review() const noexcept {
        return this->patch_review.has_value() && this->patch_review->changed();
    }

    [[nodiscard]] bool ready_for_render_data() const noexcept {
        return !this->has_errors() && !this->requires_schema_review();
    }

    [[nodiscard]] bool ready_for_project_smoke() const noexcept {
        return this->baseline_schema_available && this->ready_for_render_data();
    }

    explicit operator bool() const noexcept { return !this->has_errors(); }
};

[[nodiscard]] template_schema_normalization_summary
normalize_template_schema(featherdoc::template_schema &schema);
[[nodiscard]] template_schema_patch_summary
merge_template_schema(featherdoc::template_schema &base,
                      const featherdoc::template_schema &overlay);
[[nodiscard]] template_schema_patch_summary
apply_template_schema_patch(featherdoc::template_schema &schema,
                            const featherdoc::template_schema_patch &patch);
[[nodiscard]] template_schema_patch_summary
preview_template_schema_patch(const featherdoc::template_schema &schema,
                              const featherdoc::template_schema_patch &patch);
[[nodiscard]] template_schema_patch_summary
preview_template_schema_patch(const featherdoc::template_schema &left,
                              const featherdoc::template_schema &right);
[[nodiscard]] featherdoc::template_schema_patch
build_template_schema_patch(const featherdoc::template_schema &left,
                            const featherdoc::template_schema &right);
[[nodiscard]] featherdoc::template_schema_patch_review_summary
make_template_schema_patch_review_summary(
    const featherdoc::template_schema &baseline,
    const featherdoc::template_schema_patch &patch);
[[nodiscard]] featherdoc::template_schema_patch_review_summary
make_template_schema_patch_review_summary(
    const featherdoc::template_schema &baseline,
    const featherdoc::template_schema &generated);
void write_template_schema_patch_review_json(
    std::ostream &stream,
    const featherdoc::template_schema_patch_review_summary &summary);
[[nodiscard]] std::string template_schema_patch_review_json(
    const featherdoc::template_schema_patch_review_summary &summary);
[[nodiscard]] template_schema_patch_summary replace_template_schema_target(
    featherdoc::template_schema &schema,
    const featherdoc::template_schema_part_selector &target,
    std::span<const featherdoc::template_schema_entry> entries);
[[nodiscard]] template_schema_patch_summary replace_template_schema_target(
    featherdoc::template_schema &schema,
    const featherdoc::template_schema_part_selector &target,
    std::initializer_list<featherdoc::template_schema_entry> entries);
[[nodiscard]] template_schema_patch_summary
upsert_template_schema_slot(featherdoc::template_schema &schema,
                            const featherdoc::template_schema_entry &entry);
[[nodiscard]] template_schema_patch_summary remove_template_schema_target(
    featherdoc::template_schema &schema,
    const featherdoc::template_schema_part_selector &target);
[[nodiscard]] template_schema_patch_summary rename_template_schema_target(
    featherdoc::template_schema &schema,
    const featherdoc::template_schema_part_selector &source_target,
    const featherdoc::template_schema_part_selector &target);
[[nodiscard]] template_schema_patch_summary remove_template_schema_slot(
    featherdoc::template_schema &schema,
    const featherdoc::template_schema_slot_selector &slot);
[[nodiscard]] template_schema_patch_summary rename_template_schema_slot(
    featherdoc::template_schema &schema,
    const featherdoc::template_schema_slot_selector &slot,
    std::string_view new_bookmark_name);
[[nodiscard]] template_schema_patch_summary update_template_schema_slot(
    featherdoc::template_schema &schema,
    const featherdoc::template_schema_slot_selector &slot,
    const featherdoc::template_schema_slot_update &update);

} // namespace featherdoc
