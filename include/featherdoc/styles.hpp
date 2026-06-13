#pragma once

#include <constants.hpp>
#include <featherdoc/numbering.hpp>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace featherdoc {

enum class style_kind : std::uint8_t {
    paragraph = 0U,
    character,
    table,
    numbering,
    unknown,
};

struct style_summary {
    struct numbering_summary {
        std::optional<std::uint32_t> num_id;
        std::optional<std::uint32_t> level;
        std::optional<std::uint32_t> definition_id;
        std::optional<std::string> definition_name;
        std::optional<featherdoc::numbering_instance_summary> instance;
    };

    std::string style_id;
    std::string name;
    std::optional<std::string> based_on;
    featherdoc::style_kind kind{featherdoc::style_kind::unknown};
    std::string type_name;
    std::optional<numbering_summary> numbering;
    bool is_default{false};
    bool is_custom{false};
    bool is_semi_hidden{false};
    bool is_unhide_when_used{false};
    bool is_quick_format{false};
};

struct resolved_style_string_property {
    std::optional<std::string> value;
    std::optional<std::string> source_style_id;
};

struct resolved_style_bool_property {
    std::optional<bool> value;
    std::optional<std::string> source_style_id;
};

struct resolved_style_double_property {
    std::optional<double> value;
    std::optional<std::string> source_style_id;
};

struct resolved_style_properties_summary {
    std::string style_id;
    std::string type_name;
    std::optional<std::string> based_on;
    featherdoc::style_kind kind{featherdoc::style_kind::unknown};
    std::vector<std::string> inheritance_chain;
    featherdoc::resolved_style_string_property run_font_family{};
    featherdoc::resolved_style_string_property run_east_asia_font_family{};
    featherdoc::resolved_style_string_property run_text_color{};
    featherdoc::resolved_style_bool_property run_bold{};
    featherdoc::resolved_style_bool_property run_italic{};
    featherdoc::resolved_style_bool_property run_strikethrough{};
    featherdoc::resolved_style_bool_property run_underline{};
    featherdoc::resolved_style_bool_property run_superscript{};
    featherdoc::resolved_style_bool_property run_subscript{};
    featherdoc::resolved_style_double_property run_font_size_points{};
    featherdoc::resolved_style_string_property run_language{};
    featherdoc::resolved_style_string_property run_east_asia_language{};
    featherdoc::resolved_style_string_property run_bidi_language{};
    featherdoc::resolved_style_bool_property run_rtl{};
    featherdoc::resolved_style_bool_property paragraph_bidi{};
};

struct style_usage_breakdown {
    std::size_t paragraph_count{0};
    std::size_t run_count{0};
    std::size_t table_count{0};

    [[nodiscard]] std::size_t total_count() const {
        return paragraph_count + run_count + table_count;
    }
};

enum class style_usage_part_kind { body, header, footer };

enum class style_usage_hit_kind { paragraph, run, table };

struct style_usage_hit_reference {
    std::size_t section_index{0};
    featherdoc::section_reference_kind reference_kind{
        featherdoc::section_reference_kind::default_reference};
};

struct style_usage_hit {
    featherdoc::style_usage_part_kind part{
        featherdoc::style_usage_part_kind::body};
    featherdoc::style_usage_hit_kind kind{
        featherdoc::style_usage_hit_kind::paragraph};
    std::string entry_name;
    std::size_t ordinal{0};
    std::size_t node_ordinal{0};
    std::optional<std::size_t> section_index;
    std::vector<featherdoc::style_usage_hit_reference> references{};
};

struct style_usage_summary {
    std::string style_id;
    std::size_t paragraph_count{0};
    std::size_t run_count{0};
    std::size_t table_count{0};
    featherdoc::style_usage_breakdown body{};
    featherdoc::style_usage_breakdown header{};
    featherdoc::style_usage_breakdown footer{};
    std::vector<featherdoc::style_usage_hit> hits{};

    [[nodiscard]] std::size_t total_count() const {
        return paragraph_count + run_count + table_count;
    }
};

struct style_usage_report_entry {
    featherdoc::style_summary style{};
    featherdoc::style_usage_summary usage{};
};

struct style_usage_report {
    std::size_t style_count{0};
    std::size_t used_style_count{0};
    std::size_t unused_style_count{0};
    std::size_t total_reference_count{0};
    std::vector<featherdoc::style_usage_report_entry> entries{};
};

enum class style_refactor_action { rename, merge };

struct style_refactor_request {
    featherdoc::style_refactor_action action{
        featherdoc::style_refactor_action::rename};
    std::string source_style_id;
    std::string target_style_id;
};

struct style_refactor_issue {
    std::string code;
    std::string message;
};

struct style_refactor_suggestion {
    std::string reason_code;
    std::string reason;
    std::uint32_t confidence{0};
    std::vector<std::string> evidence{};
    std::vector<std::string> differences{};
};

struct style_refactor_suggestion_confidence_summary {
    std::size_t suggestion_count{0};
    std::size_t exact_xml_match_count{0};
    std::size_t xml_difference_count{0};
    std::optional<std::uint32_t> min_confidence{};
    std::optional<std::uint32_t> max_confidence{};
    std::optional<std::uint32_t> recommended_min_confidence{};
    std::string recommendation;
};

struct style_refactor_operation_plan {
    featherdoc::style_refactor_action action{
        featherdoc::style_refactor_action::rename};
    std::string source_style_id;
    std::string target_style_id;
    std::optional<featherdoc::style_summary> source_style{};
    std::optional<featherdoc::style_summary> target_style{};
    std::optional<featherdoc::style_usage_summary> source_usage{};
    std::optional<featherdoc::style_refactor_suggestion> suggestion{};
    std::vector<featherdoc::style_refactor_issue> issues{};
    bool applyable{false};
};

struct style_refactor_plan {
    std::size_t operation_count{0};
    std::size_t applyable_count{0};
    std::size_t issue_count{0};
    std::vector<featherdoc::style_refactor_operation_plan> operations{};

    [[nodiscard]] featherdoc::style_refactor_suggestion_confidence_summary
    suggestion_confidence_summary() const {
        auto summary =
            featherdoc::style_refactor_suggestion_confidence_summary{};
        auto exact_xml_min_confidence = std::optional<std::uint32_t>{};

        for (const auto &operation : operations) {
            if (!operation.suggestion.has_value()) {
                continue;
            }

            const auto &suggestion = *operation.suggestion;
            ++summary.suggestion_count;
            if (!summary.min_confidence.has_value() ||
                suggestion.confidence < *summary.min_confidence) {
                summary.min_confidence = suggestion.confidence;
            }
            if (!summary.max_confidence.has_value() ||
                suggestion.confidence > *summary.max_confidence) {
                summary.max_confidence = suggestion.confidence;
            }

            if (suggestion.reason_code == "matching_style_signature_and_xml") {
                ++summary.exact_xml_match_count;
                if (!exact_xml_min_confidence.has_value() ||
                    suggestion.confidence < *exact_xml_min_confidence) {
                    exact_xml_min_confidence = suggestion.confidence;
                }
            } else if (suggestion.reason_code ==
                       "matching_resolved_style_signature") {
                ++summary.xml_difference_count;
            }
        }

        if (summary.suggestion_count == 0U) {
            summary.recommendation =
                "No automatic style merge suggestions are present.";
            return summary;
        }

        if (summary.exact_xml_match_count != 0U) {
            summary.recommended_min_confidence = exact_xml_min_confidence;
            summary.recommendation =
                summary.exact_xml_match_count == summary.suggestion_count
                    ? "All suggestions are exact style XML matches; use the "
                      "recommended minimum confidence for automation gates."
                    : "Use the recommended minimum confidence to keep exact "
                      "style XML matches and review lower-confidence XML "
                      "differences manually.";
            return summary;
        }

        summary.recommended_min_confidence = summary.max_confidence;
        summary.recommendation =
            "No exact style XML matches are present; keep these suggestions in "
            "manual review workflows.";
        return summary;
    }

    [[nodiscard]] bool clean() const noexcept { return issue_count == 0U; }
};

struct style_refactor_rollback_entry {
    featherdoc::style_refactor_action action{
        featherdoc::style_refactor_action::rename};
    std::string source_style_id;
    std::string target_style_id;
    bool automatic{false};
    bool restorable{false};
    std::string note;
    std::string source_style_xml;
    std::optional<featherdoc::style_usage_summary> source_usage{};
};

struct style_refactor_apply_result {
    featherdoc::style_refactor_plan plan{};
    std::size_t requested_count{0};
    std::size_t applied_count{0};
    std::vector<featherdoc::style_refactor_rollback_entry> rollback_entries{};
    bool changed{false};

    [[nodiscard]] std::size_t skipped_count() const noexcept {
        return requested_count >= applied_count
                   ? requested_count - applied_count
                   : 0U;
    }

    [[nodiscard]] bool applied() const noexcept {
        return plan.clean() && applied_count == requested_count;
    }
};

struct style_refactor_restore_issue {
    std::string code;
    std::string message;
    std::string suggestion;
};

struct style_refactor_restore_issue_summary {
    std::string code;
    std::size_t count{0};
    std::string suggestion;
};

struct style_refactor_restore_operation_result {
    featherdoc::style_refactor_action action{
        featherdoc::style_refactor_action::merge};
    std::string source_style_id;
    std::string target_style_id;
    bool restorable{false};
    bool restored{false};
    bool style_restored{false};
    std::size_t restored_reference_count{0};
    std::vector<featherdoc::style_refactor_restore_issue> issues{};
};

struct style_refactor_restore_result {
    std::size_t requested_count{0};
    std::size_t restored_count{0};
    std::size_t restored_style_count{0};
    std::size_t restored_reference_count{0};
    std::vector<featherdoc::style_refactor_restore_operation_result>
        operations{};
    bool changed{false};
    bool dry_run{false};

    [[nodiscard]] std::size_t skipped_count() const noexcept {
        return requested_count >= restored_count
                   ? requested_count - restored_count
                   : 0U;
    }

    [[nodiscard]] std::size_t issue_count() const noexcept {
        auto count = std::size_t{0};
        for (const auto &operation : operations) {
            count += operation.issues.size();
        }
        return count;
    }

    [[nodiscard]] std::vector<featherdoc::style_refactor_restore_issue_summary>
    issue_summary() const {
        auto summaries =
            std::vector<featherdoc::style_refactor_restore_issue_summary>{};
        for (const auto &operation : operations) {
            for (const auto &issue : operation.issues) {
                auto matched = false;
                for (auto &summary : summaries) {
                    if (summary.code == issue.code) {
                        ++summary.count;
                        matched = true;
                        break;
                    }
                }
                if (!matched) {
                    summaries.push_back({issue.code, 1U, issue.suggestion});
                }
            }
        }
        return summaries;
    }

    [[nodiscard]] bool clean() const noexcept { return issue_count() == 0U; }

    [[nodiscard]] bool restored() const noexcept {
        return clean() && restored_count == requested_count;
    }
};

struct style_prune_plan {
    std::size_t scanned_style_count{0};
    std::size_t protected_style_count{0};
    std::vector<std::string> removable_style_ids{};

    [[nodiscard]] bool changed() const noexcept {
        return !removable_style_ids.empty();
    }
};

struct style_prune_summary {
    std::size_t scanned_style_count{0};
    std::size_t protected_style_count{0};
    std::vector<std::string> removed_style_ids{};

    [[nodiscard]] bool changed() const noexcept {
        return !removed_style_ids.empty();
    }
};

struct paragraph_style_definition {
    std::string name;
    std::optional<std::string> based_on;
    std::optional<std::string> next_style;
    bool is_custom{true};
    bool is_semi_hidden{false};
    bool is_unhide_when_used{false};
    bool is_quick_format{false};
    std::optional<std::string> run_text_color;
    std::optional<bool> run_bold;
    std::optional<bool> run_italic;
    std::optional<bool> run_strikethrough;
    std::optional<bool> run_underline;
    std::optional<double> run_font_size_points;
    std::optional<std::string> run_font_family;
    std::optional<std::string> run_east_asia_font_family;
    std::optional<std::string> run_language;
    std::optional<std::string> run_east_asia_language;
    std::optional<std::string> run_bidi_language;
    std::optional<bool> run_rtl;
    std::optional<bool> paragraph_bidi;
    std::optional<std::uint32_t> outline_level;
};

struct character_style_definition {
    std::string name;
    std::optional<std::string> based_on;
    bool is_custom{true};
    bool is_semi_hidden{false};
    bool is_unhide_when_used{false};
    bool is_quick_format{false};
    std::optional<std::string> run_text_color;
    std::optional<bool> run_bold;
    std::optional<bool> run_italic;
    std::optional<bool> run_strikethrough;
    std::optional<bool> run_underline;
    std::optional<bool> run_superscript;
    std::optional<bool> run_subscript;
    std::optional<double> run_font_size_points;
    std::optional<std::string> run_font_family;
    std::optional<std::string> run_east_asia_font_family;
    std::optional<std::string> run_language;
    std::optional<std::string> run_east_asia_language;
    std::optional<std::string> run_bidi_language;
    std::optional<bool> run_rtl;
};

} // namespace featherdoc
