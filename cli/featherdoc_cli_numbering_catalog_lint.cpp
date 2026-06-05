#include "featherdoc_cli_numbering_catalog_lint.hpp"

#include <algorithm>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace featherdoc_cli {

auto numbering_catalog_lint_issue_name(
    const numbering_catalog_lint_issue_kind kind) -> std::string_view {
    switch (kind) {
    case numbering_catalog_lint_issue_kind::empty_definition_name:
        return "empty_definition_name";
    case numbering_catalog_lint_issue_kind::duplicate_definition_name:
        return "duplicate_definition_name";
    case numbering_catalog_lint_issue_kind::empty_levels:
        return "empty_levels";
    case numbering_catalog_lint_issue_kind::duplicate_level:
        return "duplicate_level";
    case numbering_catalog_lint_issue_kind::invalid_level:
        return "invalid_level";
    case numbering_catalog_lint_issue_kind::invalid_start:
        return "invalid_start";
    case numbering_catalog_lint_issue_kind::empty_text_pattern:
        return "empty_text_pattern";
    case numbering_catalog_lint_issue_kind::duplicate_instance_id:
        return "duplicate_instance_id";
    case numbering_catalog_lint_issue_kind::duplicate_override_level:
        return "duplicate_override_level";
    case numbering_catalog_lint_issue_kind::invalid_override_level:
        return "invalid_override_level";
    case numbering_catalog_lint_issue_kind::invalid_override_start:
        return "invalid_override_start";
    case numbering_catalog_lint_issue_kind::invalid_override_definition:
        return "invalid_override_definition";
    }

    return "unknown";
}

void add_numbering_catalog_lint_issue(
    numbering_catalog_lint_result &result,
    numbering_catalog_lint_issue_kind kind, std::size_t definition_index,
    std::string definition_name, std::string detail,
    std::optional<std::size_t> instance_index = std::nullopt,
    std::optional<std::uint32_t> instance_id = std::nullopt,
    std::optional<std::size_t> level_index = std::nullopt,
    std::optional<std::size_t> override_index = std::nullopt,
    std::optional<std::uint32_t> level = std::nullopt) {
    auto issue = numbering_catalog_lint_issue{};
    issue.kind = kind;
    issue.definition_index = definition_index;
    issue.definition_name = std::move(definition_name);
    issue.detail = std::move(detail);
    issue.instance_index = instance_index;
    issue.instance_id = instance_id;
    issue.level_index = level_index;
    issue.override_index = override_index;
    issue.level = level;
    result.issues.push_back(std::move(issue));
}

auto lint_numbering_catalog(const featherdoc::numbering_catalog &catalog)
    -> numbering_catalog_lint_result {
    constexpr std::uint32_t max_numbering_level = 8U;
    auto result = numbering_catalog_lint_result{};
    result.definition_count = catalog.definitions.size();

    std::vector<std::string> seen_definition_names;
    for (std::size_t definition_index = 0U;
         definition_index < catalog.definitions.size(); ++definition_index) {
        const auto &definition = catalog.definitions[definition_index];
        const auto &definition_name = definition.definition.name;

        result.level_count += definition.definition.levels.size();
        result.instance_count += definition.instances.size();

        if (definition_name.empty()) {
            add_numbering_catalog_lint_issue(
                result, numbering_catalog_lint_issue_kind::empty_definition_name,
                definition_index, definition_name,
                "numbering catalog definition name must not be empty");
        } else if (std::find(seen_definition_names.begin(),
                            seen_definition_names.end(),
                            definition_name) != seen_definition_names.end()) {
            add_numbering_catalog_lint_issue(
                result,
                numbering_catalog_lint_issue_kind::duplicate_definition_name,
                definition_index, definition_name,
                "numbering catalog definition names must be unique");
        } else {
            seen_definition_names.push_back(definition_name);
        }

        if (definition.definition.levels.empty()) {
            add_numbering_catalog_lint_issue(
                result, numbering_catalog_lint_issue_kind::empty_levels,
                definition_index, definition_name,
                "numbering catalog definition must contain at least one level");
        }

        std::vector<std::uint32_t> seen_levels;
        for (std::size_t level_index = 0U;
             level_index < definition.definition.levels.size(); ++level_index) {
            const auto &level_definition = definition.definition.levels[level_index];
            if (level_definition.level > max_numbering_level) {
                add_numbering_catalog_lint_issue(
                    result, numbering_catalog_lint_issue_kind::invalid_level,
                    definition_index, definition_name,
                    "numbering catalog level must be in the range [0, 8]",
                    std::nullopt, std::nullopt, level_index, std::nullopt,
                    level_definition.level);
            }
            if (std::find(seen_levels.begin(), seen_levels.end(),
                          level_definition.level) != seen_levels.end()) {
                add_numbering_catalog_lint_issue(
                    result, numbering_catalog_lint_issue_kind::duplicate_level,
                    definition_index, definition_name,
                    "numbering catalog definition levels must be unique",
                    std::nullopt, std::nullopt, level_index, std::nullopt,
                    level_definition.level);
            } else {
                seen_levels.push_back(level_definition.level);
            }
            if (level_definition.start == 0U) {
                add_numbering_catalog_lint_issue(
                    result, numbering_catalog_lint_issue_kind::invalid_start,
                    definition_index, definition_name,
                    "numbering catalog level start must be greater than 0",
                    std::nullopt, std::nullopt, level_index, std::nullopt,
                    level_definition.level);
            }
            if (level_definition.text_pattern.empty()) {
                add_numbering_catalog_lint_issue(
                    result, numbering_catalog_lint_issue_kind::empty_text_pattern,
                    definition_index, definition_name,
                    "numbering catalog level text_pattern must not be empty",
                    std::nullopt, std::nullopt, level_index, std::nullopt,
                    level_definition.level);
            }
        }

        std::vector<std::uint32_t> seen_instance_ids;
        for (std::size_t instance_index = 0U;
             instance_index < definition.instances.size(); ++instance_index) {
            const auto &instance = definition.instances[instance_index];
            result.override_count += instance.level_overrides.size();

            if (instance.instance_id != 0U &&
                std::find(seen_instance_ids.begin(), seen_instance_ids.end(),
                          instance.instance_id) != seen_instance_ids.end()) {
                add_numbering_catalog_lint_issue(
                    result,
                    numbering_catalog_lint_issue_kind::duplicate_instance_id,
                    definition_index, definition_name,
                    "numbering catalog instance ids should be unique per definition",
                    instance_index, instance.instance_id);
            } else if (instance.instance_id != 0U) {
                seen_instance_ids.push_back(instance.instance_id);
            }

            std::vector<std::uint32_t> seen_override_levels;
            for (std::size_t override_index = 0U;
                 override_index < instance.level_overrides.size(); ++override_index) {
                const auto &level_override = instance.level_overrides[override_index];
                if (level_override.level > max_numbering_level) {
                    add_numbering_catalog_lint_issue(
                        result,
                        numbering_catalog_lint_issue_kind::invalid_override_level,
                        definition_index, definition_name,
                        "numbering catalog override level must be in the range [0, 8]",
                        instance_index, instance.instance_id, std::nullopt,
                        override_index, level_override.level);
                }
                if (std::find(seen_override_levels.begin(),
                              seen_override_levels.end(),
                              level_override.level) != seen_override_levels.end()) {
                    add_numbering_catalog_lint_issue(
                        result,
                        numbering_catalog_lint_issue_kind::duplicate_override_level,
                        definition_index, definition_name,
                        "numbering catalog override levels must be unique per instance",
                        instance_index, instance.instance_id, std::nullopt,
                        override_index, level_override.level);
                } else {
                    seen_override_levels.push_back(level_override.level);
                }
                if (level_override.start_override.has_value() &&
                    *level_override.start_override == 0U) {
                    add_numbering_catalog_lint_issue(
                        result,
                        numbering_catalog_lint_issue_kind::invalid_override_start,
                        definition_index, definition_name,
                        "numbering catalog start override must be greater than 0",
                        instance_index, instance.instance_id, std::nullopt,
                        override_index, level_override.level);
                }
                if (level_override.level_definition.has_value()) {
                    const auto &override_definition = *level_override.level_definition;
                    if (override_definition.level > max_numbering_level ||
                        override_definition.start == 0U ||
                        override_definition.text_pattern.empty()) {
                        add_numbering_catalog_lint_issue(
                            result,
                            numbering_catalog_lint_issue_kind::invalid_override_definition,
                            definition_index, definition_name,
                            "numbering catalog override level definition is invalid",
                            instance_index, instance.instance_id, std::nullopt,
                            override_index, level_override.level);
                    }
                }
            }
        }
    }

    return result;
}

} // namespace featherdoc_cli