#include "featherdoc_cli_style_numbering_audit_support.hpp"

#include <algorithm>
#include <cstddef>
#include <string_view>
#include <utility>

namespace featherdoc_cli {

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
    const style_numbering_repair_target *target) {
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

} // namespace featherdoc_cli
