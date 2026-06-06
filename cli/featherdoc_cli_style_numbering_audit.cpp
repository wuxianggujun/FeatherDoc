#include "featherdoc_cli_style_numbering_audit.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace featherdoc_cli {
namespace {

struct style_numbering_repair_target {
    std::optional<std::uint32_t> definition_id;
    std::optional<std::string> definition_name;
    std::optional<std::uint32_t> level;
};

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

auto build_style_numbering_repair_suggestion(
    style_numbering_audit_issue_kind kind,
    const featherdoc::style_summary &style,
    const style_numbering_repair_target *target = nullptr)
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

auto find_numbering_definition_summary_by_id(
    const std::vector<featherdoc::numbering_definition_summary> &definitions,
    std::uint32_t definition_id)
    -> const featherdoc::numbering_definition_summary * {
    const auto found = std::find_if(
        definitions.begin(), definitions.end(),
        [definition_id](const auto &definition) {
            return definition.definition_id == definition_id;
        });
    return found == definitions.end() ? nullptr : &(*found);
}

auto find_style_summary_by_id(
    const std::vector<featherdoc::style_summary> &styles,
    std::string_view style_id) -> const featherdoc::style_summary * {
    const auto found = std::find_if(styles.begin(), styles.end(),
                                    [style_id](const auto &style) {
                                        return std::string_view{style.style_id} ==
                                               style_id;
                                    });
    return found == styles.end() ? nullptr : &(*found);
}

auto numbering_definition_has_level(
    const featherdoc::numbering_definition_summary &definition,
    std::uint32_t level) -> bool {
    return std::find_if(definition.levels.begin(), definition.levels.end(),
                        [level](const auto &candidate) {
                            return candidate.level == level;
                        }) != definition.levels.end();
}

style_numbering_repair_target make_style_numbering_repair_target(
    const featherdoc::style_summary::numbering_summary &numbering) {
    auto target = style_numbering_repair_target{};
    target.definition_id = numbering.definition_id;
    target.definition_name = numbering.definition_name;
    target.level = numbering.level;
    return target;
}

style_numbering_repair_target make_style_numbering_repair_target(
    const featherdoc::numbering_definition_summary &definition,
    std::uint32_t level) {
    auto target = style_numbering_repair_target{};
    target.definition_id = definition.definition_id;
    target.definition_name = definition.name;
    target.level = level;
    return target;
}

auto find_unique_numbering_definition_summary_by_name_and_level(
    const std::vector<featherdoc::numbering_definition_summary> &definitions,
    std::string_view name, std::uint32_t level,
    std::optional<std::uint32_t> excluded_definition_id)
    -> const featherdoc::numbering_definition_summary * {
    const auto *candidate =
        static_cast<const featherdoc::numbering_definition_summary *>(nullptr);
    for (const auto &definition : definitions) {
        if (excluded_definition_id.has_value() &&
            definition.definition_id == *excluded_definition_id) {
            continue;
        }
        if (std::string_view{definition.name} != name ||
            !numbering_definition_has_level(definition, level)) {
            continue;
        }
        if (candidate != nullptr) {
            return nullptr;
        }
        candidate = &definition;
    }
    return candidate;
}

auto numbering_instance_has_level_definition_override(
    const std::optional<featherdoc::numbering_instance_summary> &instance,
    std::uint32_t level) -> bool {
    if (!instance.has_value()) {
        return false;
    }

    return std::find_if(
               instance->level_overrides.begin(), instance->level_overrides.end(),
               [level](const auto &level_override) {
                   return level_override.level == level &&
                          level_override.level_definition.has_value();
               }) != instance->level_overrides.end();
}

auto numbering_definition_name_is_duplicate(
    const std::vector<featherdoc::numbering_definition_summary> &definitions,
    std::string_view name) -> bool {
    if (name.empty()) {
        return false;
    }

    std::size_t count = 0U;
    for (const auto &definition : definitions) {
        if (std::string_view{definition.name} == name) {
            ++count;
            if (count > 1U) {
                return true;
            }
        }
    }
    return false;
}

void append_style_numbering_audit_issue(
    style_numbering_audit_result &result,
    style_numbering_audit_issue_kind kind,
    const featherdoc::style_summary &style, std::string message,
    const style_numbering_repair_target *target = nullptr) {
    auto issue = style_numbering_audit_issue{};
    issue.kind = kind;
    issue.style_id = style.style_id;
    issue.style_name = style.name;
    issue.based_on = style.based_on;
    issue.message = std::move(message);
    if (style.numbering.has_value()) {
        issue.num_id = style.numbering->num_id;
        issue.level = style.numbering->level;
        issue.definition_id = style.numbering->definition_id;
        issue.definition_name = style.numbering->definition_name;
    }
    result.issues.push_back(std::move(issue));
    result.suggestions.push_back(
        build_style_numbering_repair_suggestion(kind, style, target));
}

} // namespace

auto style_numbering_audit_clean(
    const style_numbering_audit_result &result) -> bool {
    return result.issues.empty();
}

auto style_numbering_audit_issue_kind_name(
    style_numbering_audit_issue_kind kind) -> std::string_view {
    switch (kind) {
    case style_numbering_audit_issue_kind::non_paragraph_numbering:
        return "non_paragraph_numbering";
    case style_numbering_audit_issue_kind::missing_num_id:
        return "missing_num_id";
    case style_numbering_audit_issue_kind::missing_level:
        return "missing_level";
    case style_numbering_audit_issue_kind::orphan_instance:
        return "orphan_instance";
    case style_numbering_audit_issue_kind::missing_definition:
        return "missing_definition";
    case style_numbering_audit_issue_kind::missing_level_definition:
        return "missing_level_definition";
    case style_numbering_audit_issue_kind::duplicate_definition_name:
        return "duplicate_definition_name";
    case style_numbering_audit_issue_kind::based_on_definition_mismatch:
        return "based_on_definition_mismatch";
    }

    return "unknown";
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

auto audit_style_numbering(
    const std::vector<featherdoc::style_summary> &styles,
    const std::vector<featherdoc::numbering_definition_summary> &definitions)
    -> style_numbering_audit_result {
    auto result = style_numbering_audit_result{};
    for (const auto &style : styles) {
        if (style.kind == featherdoc::style_kind::paragraph) {
            ++result.paragraph_style_count;
            if (style.numbering.has_value()) {
                result.numbered_styles.push_back(style);
            }
        }

        if (!style.numbering.has_value()) {
            continue;
        }

        if (style.kind != featherdoc::style_kind::paragraph) {
            append_style_numbering_audit_issue(
                result, style_numbering_audit_issue_kind::non_paragraph_numbering,
                style, "non-paragraph style carries paragraph numbering metadata");
            continue;
        }

        const auto &numbering = *style.numbering;
        if (!numbering.num_id.has_value()) {
            append_style_numbering_audit_issue(
                result, style_numbering_audit_issue_kind::missing_num_id, style,
                "style numbering has w:numPr but no valid w:numId");
        }
        if (!numbering.level.has_value()) {
            append_style_numbering_audit_issue(
                result, style_numbering_audit_issue_kind::missing_level, style,
                "style numbering has w:numPr but no valid w:ilvl");
        }
        if (numbering.num_id.has_value() && !numbering.instance.has_value()) {
            append_style_numbering_audit_issue(
                result, style_numbering_audit_issue_kind::orphan_instance, style,
                "style numbering references a numId that is not present in word/numbering.xml");
        }

        if (numbering.definition_id.has_value()) {
            const auto *definition = find_numbering_definition_summary_by_id(
                definitions, *numbering.definition_id);
            if (definition == nullptr) {
                append_style_numbering_audit_issue(
                    result, style_numbering_audit_issue_kind::missing_definition,
                    style,
                    "style numbering instance resolves to a missing numbering definition");
            } else if (numbering.level.has_value() &&
                       !numbering_definition_has_level(*definition,
                                                       *numbering.level) &&
                       !numbering_instance_has_level_definition_override(
                           numbering.instance, *numbering.level)) {
                auto relink_target = std::optional<style_numbering_repair_target>{};
                if (!definition->name.empty()) {
                    const auto *target_definition =
                        find_unique_numbering_definition_summary_by_name_and_level(
                            definitions, definition->name, *numbering.level,
                            numbering.definition_id);
                    if (target_definition != nullptr) {
                        relink_target = make_style_numbering_repair_target(
                            *target_definition, *numbering.level);
                    }
                }
                append_style_numbering_audit_issue(
                    result,
                    style_numbering_audit_issue_kind::missing_level_definition,
                    style,
                    "style numbering level is not defined by its numbering definition or lvlOverride",
                    relink_target.has_value() ? &(*relink_target) : nullptr);
            }
        }

        if (numbering.definition_name.has_value() &&
            numbering_definition_name_is_duplicate(definitions,
                                                   *numbering.definition_name)) {
            append_style_numbering_audit_issue(
                result,
                style_numbering_audit_issue_kind::duplicate_definition_name,
                style,
                "style uses a numbering definition name that appears on multiple definitions");
        }

        if (style.based_on.has_value() && numbering.definition_id.has_value()) {
            const auto *base_style = find_style_summary_by_id(styles, *style.based_on);
            if (base_style != nullptr && base_style->numbering.has_value() &&
                base_style->numbering->definition_id.has_value() &&
                *base_style->numbering->definition_id != *numbering.definition_id) {
                const auto target =
                    make_style_numbering_repair_target(*base_style->numbering);
                append_style_numbering_audit_issue(
                    result,
                    style_numbering_audit_issue_kind::based_on_definition_mismatch,
                    style,
                    "style and based-on style use different numbering definitions",
                    &target);
            }
        }
    }

    return result;
}

} // namespace featherdoc_cli
