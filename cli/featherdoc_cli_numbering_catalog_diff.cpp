#include "featherdoc_cli_numbering_catalog_diff.hpp"

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <utility>
#include <vector>

namespace featherdoc_cli {
namespace {

auto compare_numbering_catalog_level_definition(
    const featherdoc::numbering_level_definition &left,
    const featherdoc::numbering_level_definition &right) -> int {
    if (left.level != right.level) {
        return left.level < right.level ? -1 : 1;
    }
    if (left.kind != right.kind) {
        return static_cast<int>(left.kind) < static_cast<int>(right.kind) ? -1 : 1;
    }
    if (left.start != right.start) {
        return left.start < right.start ? -1 : 1;
    }
    if (left.text_pattern != right.text_pattern) {
        return left.text_pattern < right.text_pattern ? -1 : 1;
    }
    return 0;
}

auto compare_numbering_catalog_override(
    const featherdoc::numbering_level_override_summary &left,
    const featherdoc::numbering_level_override_summary &right) -> int {
    if (left.level != right.level) {
        return left.level < right.level ? -1 : 1;
    }
    if (left.start_override != right.start_override) {
        return left.start_override < right.start_override ? -1 : 1;
    }
    if (left.level_definition.has_value() != right.level_definition.has_value()) {
        return left.level_definition.has_value() ? 1 : -1;
    }
    if (left.level_definition.has_value()) {
        return compare_numbering_catalog_level_definition(*left.level_definition,
                                                          *right.level_definition);
    }
    return 0;
}

auto sorted_numbering_catalog_levels(
    const std::vector<featherdoc::numbering_level_definition> &levels)
    -> std::vector<featherdoc::numbering_level_definition> {
    auto sorted_levels = levels;
    std::sort(sorted_levels.begin(), sorted_levels.end(),
              [](const auto &left, const auto &right) {
                  return left.level < right.level;
              });
    return sorted_levels;
}

auto sorted_numbering_catalog_overrides(
    const std::vector<featherdoc::numbering_level_override_summary> &overrides)
    -> std::vector<featherdoc::numbering_level_override_summary> {
    auto sorted_overrides = overrides;
    std::sort(sorted_overrides.begin(), sorted_overrides.end(),
              [](const auto &left, const auto &right) {
                  return left.level < right.level;
              });
    return sorted_overrides;
}

auto diff_numbering_catalog_levels(
    const std::vector<featherdoc::numbering_level_definition> &left,
    const std::vector<featherdoc::numbering_level_definition> &right,
    changed_numbering_catalog_definition &definition_diff) -> void {
    const auto left_levels = sorted_numbering_catalog_levels(left);
    const auto right_levels = sorted_numbering_catalog_levels(right);
    std::size_t left_index = 0U;
    std::size_t right_index = 0U;
    while (left_index < left_levels.size() && right_index < right_levels.size()) {
        if (left_levels[left_index].level < right_levels[right_index].level) {
            definition_diff.removed_levels.push_back(left_levels[left_index++]);
            continue;
        }
        if (left_levels[left_index].level > right_levels[right_index].level) {
            definition_diff.added_levels.push_back(right_levels[right_index++]);
            continue;
        }
        if (compare_numbering_catalog_level_definition(left_levels[left_index],
                                                       right_levels[right_index]) != 0) {
            definition_diff.changed_levels.push_back(
                {left_levels[left_index], right_levels[right_index]});
        }
        ++left_index;
        ++right_index;
    }
    definition_diff.removed_levels.insert(definition_diff.removed_levels.end(),
                                          left_levels.begin() +
                                              static_cast<std::ptrdiff_t>(left_index),
                                          left_levels.end());
    definition_diff.added_levels.insert(definition_diff.added_levels.end(),
                                        right_levels.begin() +
                                            static_cast<std::ptrdiff_t>(right_index),
                                        right_levels.end());
}

auto diff_numbering_catalog_overrides(
    const std::vector<featherdoc::numbering_level_override_summary> &left,
    const std::vector<featherdoc::numbering_level_override_summary> &right,
    std::size_t instance_index) -> numbering_catalog_instance_diff_result {
    auto result = numbering_catalog_instance_diff_result{};
    result.instance_index = instance_index;
    const auto left_overrides = sorted_numbering_catalog_overrides(left);
    const auto right_overrides = sorted_numbering_catalog_overrides(right);
    std::size_t left_index = 0U;
    std::size_t right_index = 0U;
    while (left_index < left_overrides.size() && right_index < right_overrides.size()) {
        if (left_overrides[left_index].level < right_overrides[right_index].level) {
            result.removed_overrides.push_back(left_overrides[left_index++]);
            continue;
        }
        if (left_overrides[left_index].level > right_overrides[right_index].level) {
            result.added_overrides.push_back(right_overrides[right_index++]);
            continue;
        }
        if (compare_numbering_catalog_override(left_overrides[left_index],
                                               right_overrides[right_index]) != 0) {
            result.changed_overrides.push_back(
                {left_overrides[left_index], right_overrides[right_index]});
        }
        ++left_index;
        ++right_index;
    }
    result.removed_overrides.insert(result.removed_overrides.end(),
                                    left_overrides.begin() +
                                        static_cast<std::ptrdiff_t>(left_index),
                                    left_overrides.end());
    result.added_overrides.insert(result.added_overrides.end(),
                                  right_overrides.begin() +
                                      static_cast<std::ptrdiff_t>(right_index),
                                  right_overrides.end());
    return result;
}

auto diff_numbering_catalog_instances(
    const std::vector<featherdoc::numbering_instance_summary> &left,
    const std::vector<featherdoc::numbering_instance_summary> &right,
    changed_numbering_catalog_definition &definition_diff) -> void {
    const auto common_count = std::min(left.size(), right.size());
    for (std::size_t index = 0U; index < common_count; ++index) {
        auto instance_diff = diff_numbering_catalog_overrides(
            left[index].level_overrides, right[index].level_overrides, index);
        if (!instance_diff.equal()) {
            definition_diff.changed_instances.push_back(std::move(instance_diff));
        }
    }
    if (left.size() > common_count) {
        definition_diff.removed_instances.insert(
            definition_diff.removed_instances.end(),
            left.begin() + static_cast<std::ptrdiff_t>(common_count), left.end());
    }
    if (right.size() > common_count) {
        definition_diff.added_instances.insert(
            definition_diff.added_instances.end(),
            right.begin() + static_cast<std::ptrdiff_t>(common_count), right.end());
    }
}

} // namespace

auto diff_numbering_catalogs(const featherdoc::numbering_catalog &left,
                             const featherdoc::numbering_catalog &right)
    -> numbering_catalog_diff_result {
    auto result = numbering_catalog_diff_result{};
    std::vector<bool> matched_right(right.definitions.size(), false);

    for (const auto &left_definition : left.definitions) {
        const auto right_it = std::find_if(
            right.definitions.begin(), right.definitions.end(),
            [&](const auto &right_definition) {
                return right_definition.definition.name ==
                       left_definition.definition.name;
            });
        if (right_it == right.definitions.end()) {
            result.removed_definitions.push_back(left_definition);
            continue;
        }

        const auto right_index = static_cast<std::size_t>(
            std::distance(right.definitions.begin(), right_it));
        matched_right[right_index] = true;

        auto definition_diff = changed_numbering_catalog_definition{};
        definition_diff.name = left_definition.definition.name;
        diff_numbering_catalog_levels(left_definition.definition.levels,
                                      right_it->definition.levels,
                                      definition_diff);
        diff_numbering_catalog_instances(left_definition.instances,
                                         right_it->instances, definition_diff);
        if (!definition_diff.equal()) {
            result.changed_definitions.push_back(std::move(definition_diff));
        }
    }

    for (std::size_t index = 0U; index < right.definitions.size(); ++index) {
        if (!matched_right[index]) {
            result.added_definitions.push_back(right.definitions[index]);
        }
    }

    return result;
}

} // namespace featherdoc_cli
