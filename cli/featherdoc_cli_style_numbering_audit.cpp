#include "featherdoc_cli_style_numbering_audit.hpp"

#include "featherdoc_cli_style_numbering_audit_support.hpp"
#include "featherdoc_cli_style_numbering_repair_suggestions.hpp"

#include <optional>
#include <string_view>

namespace featherdoc_cli {

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
