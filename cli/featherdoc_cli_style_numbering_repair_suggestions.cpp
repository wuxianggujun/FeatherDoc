#include "featherdoc_cli_style_numbering_repair_suggestions.hpp"

#include <algorithm>
#include <string>
#include <string_view>
#include <vector>

namespace featherdoc_cli {
namespace {

auto style_numbering_repair_command_template(
    style_numbering_repair_action action, const featherdoc::style_summary &style)
    -> std::string {
    switch (action) {
    case style_numbering_repair_action::clear_style_numbering:
        return "featherdoc_cli clear-paragraph-style-numbering <input.docx> " +
               style.style_id + " --output <output.docx> --json";
    case style_numbering_repair_action::relink_style_numbering:
        return "featherdoc_cli set-paragraph-style-numbering <input.docx> " +
               style.style_id +
               " --definition-name <name> --numbering-level <level>:<kind>:<start>:<text-pattern> --style-level <level> --output <output.docx> --json";
    case style_numbering_repair_action::add_numbering_level:
        return "featherdoc_cli patch-numbering-catalog numbering-catalog.json --patch-file <patch-with-upsert_levels.json> --output numbering-catalog.patched.json --json";
    case style_numbering_repair_action::rename_numbering_definition:
        return "featherdoc_cli patch-numbering-catalog numbering-catalog.json --patch-file <patch.json> --output numbering-catalog.patched.json --json";
    case style_numbering_repair_action::align_with_based_on_numbering:
        return "featherdoc_cli set-paragraph-style-numbering <input.docx> " +
               style.style_id +
               " --definition-name <based-on-definition-name> --numbering-level <level>:<kind>:<start>:<text-pattern> --style-level <level> --output <output.docx> --json";
    case style_numbering_repair_action::manual_review:
        return "featherdoc_cli inspect-style-numbering <input.docx> --json";
    }

    return "featherdoc_cli inspect-style-numbering <input.docx> --json";
}

auto style_numbering_repair_action_for_issue(
    style_numbering_audit_issue_kind kind) -> style_numbering_repair_action {
    switch (kind) {
    case style_numbering_audit_issue_kind::missing_num_id:
    case style_numbering_audit_issue_kind::missing_level:
    case style_numbering_audit_issue_kind::orphan_instance:
    case style_numbering_audit_issue_kind::missing_definition:
        return style_numbering_repair_action::clear_style_numbering;
    case style_numbering_audit_issue_kind::missing_level_definition:
        return style_numbering_repair_action::add_numbering_level;
    case style_numbering_audit_issue_kind::duplicate_definition_name:
        return style_numbering_repair_action::rename_numbering_definition;
    case style_numbering_audit_issue_kind::based_on_definition_mismatch:
        return style_numbering_repair_action::align_with_based_on_numbering;
    case style_numbering_audit_issue_kind::non_paragraph_numbering:
        return style_numbering_repair_action::manual_review;
    }

    return style_numbering_repair_action::manual_review;
}

auto style_numbering_repair_rationale(
    style_numbering_audit_issue_kind kind) -> std::string_view {
    switch (kind) {
    case style_numbering_audit_issue_kind::missing_num_id:
        return "remove incomplete style numbering or relink the style to a known numbering definition";
    case style_numbering_audit_issue_kind::missing_level:
        return "remove incomplete style numbering or relink the style with an explicit style level";
    case style_numbering_audit_issue_kind::orphan_instance:
        return "clear the orphan numId binding before relinking the style to a valid numbering definition";
    case style_numbering_audit_issue_kind::missing_definition:
        return "clear the missing definition binding before importing or recreating the target numbering catalog";
    case style_numbering_audit_issue_kind::missing_level_definition:
        return "add the missing level to the numbering catalog or relink the style to a defined level";
    case style_numbering_audit_issue_kind::duplicate_definition_name:
        return "rename or merge duplicate numbering definitions before relying on definition_name based automation";
    case style_numbering_audit_issue_kind::based_on_definition_mismatch:
        return "align the style with its based-on numbering definition or intentionally rebase the style";
    case style_numbering_audit_issue_kind::non_paragraph_numbering:
        return "review the non-paragraph style XML before applying paragraph numbering commands";
    }

    return "review the style numbering binding before applying mutations";
}

} // namespace

auto make_style_numbering_repair_target(
    const featherdoc::style_summary::numbering_summary &numbering)
    -> style_numbering_repair_target {
    auto target = style_numbering_repair_target{};
    target.definition_id = numbering.definition_id;
    target.definition_name = numbering.definition_name;
    target.level = numbering.level;
    return target;
}

auto make_style_numbering_repair_target(
    const featherdoc::numbering_definition_summary &definition,
    std::uint32_t level) -> style_numbering_repair_target {
    auto target = style_numbering_repair_target{};
    target.definition_id = definition.definition_id;
    target.definition_name = definition.name;
    target.level = level;
    return target;
}

auto build_style_numbering_repair_suggestion(
    style_numbering_audit_issue_kind kind,
    const featherdoc::style_summary &style,
    const style_numbering_repair_target *target)
    -> style_numbering_repair_suggestion {
    const auto has_relink_target =
        target != nullptr && target->definition_id.has_value() &&
        target->level.has_value();
    auto action = style_numbering_repair_action_for_issue(kind);
    if (kind == style_numbering_audit_issue_kind::missing_level_definition &&
        has_relink_target) {
        action = style_numbering_repair_action::relink_style_numbering;
    }

    auto suggestion = style_numbering_repair_suggestion{};
    suggestion.action = action;
    suggestion.issue_kind = kind;
    suggestion.style_id = style.style_id;
    suggestion.rationale = std::string{style_numbering_repair_rationale(kind)};
    suggestion.command_template =
        style_numbering_repair_command_template(action, style);
    if (target != nullptr) {
        suggestion.target_definition_id = target->definition_id;
        suggestion.target_definition_name = target->definition_name;
        suggestion.target_level = target->level;
    }
    return suggestion;
}

auto style_numbering_repair_action_name(
    style_numbering_repair_action action) -> std::string_view {
    switch (action) {
    case style_numbering_repair_action::clear_style_numbering:
        return "clear_style_numbering";
    case style_numbering_repair_action::relink_style_numbering:
        return "relink_style_numbering";
    case style_numbering_repair_action::add_numbering_level:
        return "add_numbering_level";
    case style_numbering_repair_action::rename_numbering_definition:
        return "rename_numbering_definition";
    case style_numbering_repair_action::align_with_based_on_numbering:
        return "align_with_based_on_numbering";
    case style_numbering_repair_action::manual_review:
        return "manual_review";
    }

    return "manual_review";
}

auto style_numbering_repair_suggestion_applyable(
    const style_numbering_repair_suggestion &suggestion) -> bool {
    switch (suggestion.action) {
    case style_numbering_repair_action::clear_style_numbering:
        return true;
    case style_numbering_repair_action::align_with_based_on_numbering:
    case style_numbering_repair_action::relink_style_numbering:
        return suggestion.target_definition_id.has_value() &&
               suggestion.target_level.has_value();
    case style_numbering_repair_action::add_numbering_level:
    case style_numbering_repair_action::rename_numbering_definition:
    case style_numbering_repair_action::manual_review:
        return false;
    }

    return false;
}

auto collect_applyable_style_numbering_repair_suggestions(
    const std::vector<style_numbering_repair_suggestion> &suggestions)
    -> std::vector<style_numbering_repair_suggestion> {
    auto applyable = std::vector<style_numbering_repair_suggestion>{};
    for (const auto &suggestion : suggestions) {
        if (!style_numbering_repair_suggestion_applyable(suggestion)) {
            continue;
        }

        const auto duplicate = std::find_if(
            applyable.begin(), applyable.end(), [&suggestion](const auto &existing) {
                return existing.action == suggestion.action &&
                       existing.style_id == suggestion.style_id;
            });
        if (duplicate == applyable.end()) {
            applyable.push_back(suggestion);
        }
    }
    return applyable;
}

} // namespace featherdoc_cli
