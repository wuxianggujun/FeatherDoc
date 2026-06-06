#pragma once

#include <constants.hpp>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace featherdoc {

struct numbering_level_definition {
    featherdoc::list_kind kind{};
    std::uint32_t start{1U};
    std::uint32_t level{0U};
    std::string text_pattern;
};

struct numbering_definition {
    std::string name;
    std::vector<featherdoc::numbering_level_definition> levels;
};

struct paragraph_style_numbering_link {
    std::string style_id;
    std::uint32_t level{0U};
};

struct numbering_level_override_summary {
    std::uint32_t level{};
    std::optional<std::uint32_t> start_override;
    std::optional<featherdoc::numbering_level_definition> level_definition;
};

struct numbering_instance_summary {
    std::uint32_t instance_id{};
    std::vector<featherdoc::numbering_level_override_summary> level_overrides;
};

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

struct table_style_margins_definition {
    std::optional<std::uint32_t> top_twips;
    std::optional<std::uint32_t> left_twips;
    std::optional<std::uint32_t> bottom_twips;
    std::optional<std::uint32_t> right_twips;
};

struct table_style_borders_definition {
    std::optional<featherdoc::border_definition> top;
    std::optional<featherdoc::border_definition> left;
    std::optional<featherdoc::border_definition> bottom;
    std::optional<featherdoc::border_definition> right;
    std::optional<featherdoc::border_definition> inside_horizontal;
    std::optional<featherdoc::border_definition> inside_vertical;
};

struct table_style_paragraph_spacing_definition {
    std::optional<std::uint32_t> before_twips;
    std::optional<std::uint32_t> after_twips;
    std::optional<std::uint32_t> line_twips;
    std::optional<featherdoc::paragraph_line_spacing_rule> line_rule;
};

struct table_style_region_definition {
    std::optional<std::string> fill_color;
    std::optional<std::string> text_color;
    std::optional<bool> bold;
    std::optional<bool> italic;
    std::optional<std::uint32_t> font_size_points;
    std::optional<std::string> font_family;
    std::optional<std::string> east_asia_font_family;
    std::optional<featherdoc::cell_vertical_alignment> cell_vertical_alignment;
    std::optional<featherdoc::cell_text_direction> cell_text_direction;
    std::optional<featherdoc::paragraph_alignment> paragraph_alignment;
    std::optional<featherdoc::table_style_paragraph_spacing_definition>
        paragraph_spacing;
    std::optional<featherdoc::table_style_margins_definition> cell_margins;
    std::optional<featherdoc::table_style_borders_definition> borders;
};

struct table_style_definition {
    std::string name;
    std::optional<std::string> based_on;
    bool is_custom{true};
    bool is_semi_hidden{false};
    bool is_unhide_when_used{false};
    bool is_quick_format{false};
    std::optional<featherdoc::table_style_region_definition> whole_table;
    std::optional<featherdoc::table_style_region_definition> first_row;
    std::optional<featherdoc::table_style_region_definition> last_row;
    std::optional<featherdoc::table_style_region_definition> first_column;
    std::optional<featherdoc::table_style_region_definition> last_column;
    std::optional<featherdoc::table_style_region_definition> banded_rows;
    std::optional<featherdoc::table_style_region_definition> banded_columns;
    std::optional<featherdoc::table_style_region_definition> second_banded_rows;
    std::optional<featherdoc::table_style_region_definition>
        second_banded_columns;
};

struct table_style_border_summary {
    std::optional<std::string> style;
    std::optional<std::uint32_t> size_eighth_points;
    std::optional<std::string> color;
    std::optional<std::uint32_t> space_points;
};

struct table_style_borders_summary {
    std::optional<featherdoc::table_style_border_summary> top;
    std::optional<featherdoc::table_style_border_summary> left;
    std::optional<featherdoc::table_style_border_summary> bottom;
    std::optional<featherdoc::table_style_border_summary> right;
    std::optional<featherdoc::table_style_border_summary> inside_horizontal;
    std::optional<featherdoc::table_style_border_summary> inside_vertical;
};

struct table_style_region_summary {
    std::optional<std::string> fill_color;
    std::optional<std::string> text_color;
    std::optional<bool> bold;
    std::optional<bool> italic;
    std::optional<std::uint32_t> font_size_points;
    std::optional<std::string> font_family;
    std::optional<std::string> east_asia_font_family;
    std::optional<featherdoc::cell_vertical_alignment> cell_vertical_alignment;
    std::optional<featherdoc::cell_text_direction> cell_text_direction;
    std::optional<featherdoc::paragraph_alignment> paragraph_alignment;
    std::optional<featherdoc::table_style_paragraph_spacing_definition>
        paragraph_spacing;
    std::optional<featherdoc::table_style_margins_definition> cell_margins;
    std::optional<featherdoc::table_style_borders_summary> borders;
};

struct table_style_definition_summary {
    featherdoc::style_summary style;
    std::optional<featherdoc::table_style_region_summary> whole_table;
    std::optional<featherdoc::table_style_region_summary> first_row;
    std::optional<featherdoc::table_style_region_summary> last_row;
    std::optional<featherdoc::table_style_region_summary> first_column;
    std::optional<featherdoc::table_style_region_summary> last_column;
    std::optional<featherdoc::table_style_region_summary> banded_rows;
    std::optional<featherdoc::table_style_region_summary> banded_columns;
    std::optional<featherdoc::table_style_region_summary> second_banded_rows;
    std::optional<featherdoc::table_style_region_summary> second_banded_columns;
};

struct table_style_region_audit_issue {
    std::string style_id;
    std::string style_name;
    std::string region;
    std::string issue_type;
    std::size_t property_count{0};
    std::string suggestion;
};

struct table_style_region_audit_report {
    std::size_t table_style_count{0};
    std::size_t region_count{0};
    std::vector<featherdoc::table_style_region_audit_issue> issues;

    [[nodiscard]] std::size_t issue_count() const noexcept {
        return this->issues.size();
    }

    [[nodiscard]] bool ok() const noexcept { return this->issues.empty(); }
};

struct table_style_inheritance_audit_issue {
    std::string style_id;
    std::string style_name;
    std::string based_on_style_id;
    std::string based_on_style_kind;
    std::string issue_type;
    std::vector<std::string> inheritance_chain;
    std::string suggestion;
};

struct table_style_inheritance_audit_report {
    std::size_t table_style_count{0};
    std::vector<featherdoc::table_style_inheritance_audit_issue> issues;

    [[nodiscard]] std::size_t issue_count() const noexcept {
        return this->issues.size();
    }

    [[nodiscard]] bool ok() const noexcept { return this->issues.empty(); }
};

struct numbering_definition_summary {
    std::uint32_t definition_id{};
    std::string name;
    std::vector<featherdoc::numbering_level_definition> levels;
    std::vector<std::uint32_t> instance_ids;
    std::vector<featherdoc::numbering_instance_summary> instances;
};

struct numbering_instance_lookup_summary {
    std::uint32_t definition_id{};
    std::string definition_name;
    featherdoc::numbering_instance_summary instance;
};

struct numbering_catalog_definition {
    featherdoc::numbering_definition definition{};
    std::vector<featherdoc::numbering_instance_summary> instances;
};

struct numbering_catalog {
    std::vector<featherdoc::numbering_catalog_definition> definitions;
};

struct imported_numbering_definition_summary {
    std::string name;
    std::uint32_t definition_id{};
    std::vector<std::uint32_t> instance_ids;
};

struct numbering_catalog_import_summary {
    std::size_t input_definition_count{};
    std::size_t imported_definition_count{};
    std::size_t imported_instance_count{};
    std::vector<featherdoc::imported_numbering_definition_summary> definitions;

    explicit operator bool() const noexcept {
        return this->input_definition_count == this->imported_definition_count;
    }
};

struct table_style_look_consistency_issue {
    std::size_t table_index{0};
    std::string style_id;
    std::string issue_type;
    std::string region;
    std::string required_style_look_flag;
    std::optional<bool> actual_value;
    bool expected_value{true};
    std::string suggestion;
};

struct table_style_look_consistency_report {
    std::size_t table_count{0};
    std::vector<featherdoc::table_style_look_consistency_issue> issues;

    [[nodiscard]] std::size_t issue_count() const noexcept {
        return this->issues.size();
    }

    [[nodiscard]] bool ok() const noexcept { return this->issues.empty(); }
};

struct table_style_look_repair_report {
    featherdoc::table_style_look_consistency_report before;
    featherdoc::table_style_look_consistency_report after;
    std::size_t changed_table_count{0};

    [[nodiscard]] bool changed() const noexcept {
        return this->changed_table_count > 0U;
    }

    [[nodiscard]] bool ok() const noexcept { return this->after.ok(); }
};

struct table_style_quality_audit_report {
    featherdoc::table_style_region_audit_report region_audit;
    featherdoc::table_style_inheritance_audit_report inheritance_audit;
    featherdoc::table_style_look_consistency_report style_look;

    [[nodiscard]] std::size_t issue_count() const noexcept {
        return this->region_audit.issue_count() +
               this->inheritance_audit.issue_count() +
               this->style_look.issue_count();
    }

    [[nodiscard]] bool ok() const noexcept { return this->issue_count() == 0U; }
};

struct table_style_quality_fix_item {
    std::string source;
    std::string issue_type;
    std::optional<std::size_t> table_index;
    std::string style_id;
    std::string style_name;
    std::string region;
    std::string action;
    bool automatic{false};
    std::string command;
    std::string suggestion;
};

struct table_style_quality_fix_plan {
    featherdoc::table_style_quality_audit_report audit;
    std::vector<featherdoc::table_style_quality_fix_item> items;

    [[nodiscard]] std::size_t issue_count() const noexcept {
        return this->audit.issue_count();
    }

    [[nodiscard]] std::size_t automatic_fix_count() const noexcept {
        auto count = std::size_t{0U};
        for (const auto &item : this->items) {
            if (item.automatic) {
                ++count;
            }
        }
        return count;
    }

    [[nodiscard]] std::size_t manual_fix_count() const noexcept {
        return this->items.size() - this->automatic_fix_count();
    }

    [[nodiscard]] bool ok() const noexcept { return this->audit.ok(); }
};

} // namespace featherdoc
