#include "featherdoc_cli_template_schema_patch_ops.hpp"

#include "featherdoc_cli_template_schema_ops.hpp"

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <optional>
#include <utility>
#include <vector>

namespace featherdoc_cli {
namespace {

void append_changed_target_template_schema_patch(
    const exported_template_schema_target &left,
    const exported_template_schema_target &right,
    template_schema_patch_document &patch) {
    const auto selector = make_template_schema_patch_target_selector(left);
    if (compare_template_schema_target_identity(left, right) != 0) {
        patch.remove_targets.push_back(selector);
        patch.upsert_targets.push_back(right);
        return;
    }

    std::vector<const featherdoc::template_slot_requirement *> removed_requirements;
    std::vector<const featherdoc::template_slot_requirement *> added_requirements;
    exported_template_schema_target upsert_target = right;
    upsert_target.requirements.clear();

    std::vector<bool> right_matched(right.requirements.size(), false);
    for (const auto &left_requirement : left.requirements) {
        const auto right_it = std::find_if(
            right.requirements.begin(), right.requirements.end(),
            [&](const featherdoc::template_slot_requirement &candidate) {
                return candidate.source == left_requirement.source &&
                       candidate.bookmark_name == left_requirement.bookmark_name;
            });
        if (right_it == right.requirements.end()) {
            removed_requirements.push_back(&left_requirement);
            continue;
        }

        const auto right_index =
            static_cast<std::size_t>(std::distance(right.requirements.begin(), right_it));
        right_matched[right_index] = true;
        if (compare_template_slot_requirement(left_requirement, *right_it) != 0) {
            template_schema_patch_update_slot update_slot{};
            update_slot.target = selector;
            update_slot.bookmark_name = left_requirement.bookmark_name;
            update_slot.source = left_requirement.source;
            update_slot.update =
                make_template_schema_slot_update(left_requirement, *right_it);
            patch.update_slots.push_back(std::move(update_slot));
        }
    }

    for (std::size_t right_index = 0U; right_index < right.requirements.size();
         ++right_index) {
        if (!right_matched[right_index]) {
            added_requirements.push_back(&right.requirements[right_index]);
        }
    }

    std::vector<bool> removed_consumed(removed_requirements.size(), false);
    std::vector<bool> added_consumed(added_requirements.size(), false);
    const auto find_unique_renamed_slot = [&](std::size_t removed_index,
                                              int match_level)
        -> std::optional<std::size_t> {
        const auto matches = [&](const featherdoc::template_slot_requirement &removed,
                                 const featherdoc::template_slot_requirement &added) {
            if (removed.source != added.source) {
                return false;
            }
            if (match_level == 0) {
                return compare_template_slot_requirement_shape(removed, added) == 0;
            }
            if (match_level == 1) {
                return removed.kind == added.kind;
            }
            return true;
        };

        std::optional<std::size_t> matched_added_index;
        std::size_t matched_added_count = 0U;
        for (std::size_t added_index = 0U; added_index < added_requirements.size();
             ++added_index) {
            if (added_consumed[added_index] ||
                !matches(*removed_requirements[removed_index],
                         *added_requirements[added_index])) {
                continue;
            }
            matched_added_index = added_index;
            ++matched_added_count;
        }
        if (matched_added_count != 1U || !matched_added_index.has_value()) {
            return std::nullopt;
        }

        std::size_t matched_removed_count = 0U;
        for (std::size_t candidate_removed_index = 0U;
             candidate_removed_index < removed_requirements.size();
             ++candidate_removed_index) {
            if (removed_consumed[candidate_removed_index] ||
                !matches(*removed_requirements[candidate_removed_index],
                         *added_requirements[*matched_added_index])) {
                continue;
            }
            ++matched_removed_count;
        }
        if (matched_removed_count != 1U) {
            return std::nullopt;
        }
        return matched_added_index;
    };

    for (std::size_t removed_index = 0U; removed_index < removed_requirements.size();
         ++removed_index) {
        auto matched_added_index = find_unique_renamed_slot(removed_index, 0);
        if (!matched_added_index.has_value()) {
            matched_added_index = find_unique_renamed_slot(removed_index, 1);
        }
        if (!matched_added_index.has_value()) {
            matched_added_index = find_unique_renamed_slot(removed_index, 2);
        }
        if (!matched_added_index.has_value()) {
            continue;
        }

        template_schema_patch_rename_slot rename_slot{};
        rename_slot.target = selector;
        rename_slot.bookmark_name =
            removed_requirements[removed_index]->bookmark_name;
        rename_slot.new_bookmark_name =
            added_requirements[*matched_added_index]->bookmark_name;
        rename_slot.source = removed_requirements[removed_index]->source;
        patch.rename_slots.push_back(std::move(rename_slot));

        if (compare_template_slot_requirement_shape(
                *removed_requirements[removed_index],
                *added_requirements[*matched_added_index]) != 0) {
            template_schema_patch_update_slot update_slot{};
            update_slot.target = selector;
            update_slot.bookmark_name =
                added_requirements[*matched_added_index]->bookmark_name;
            update_slot.source = removed_requirements[removed_index]->source;
            update_slot.update = make_template_schema_slot_update(
                *removed_requirements[removed_index],
                *added_requirements[*matched_added_index]);
            patch.update_slots.push_back(std::move(update_slot));
        }

        removed_consumed[removed_index] = true;
        added_consumed[*matched_added_index] = true;
    }

    for (std::size_t removed_index = 0U; removed_index < removed_requirements.size();
         ++removed_index) {
        if (removed_consumed[removed_index]) {
            continue;
        }
        template_schema_patch_remove_slot remove_slot{};
        remove_slot.target = selector;
        remove_slot.bookmark_name =
            removed_requirements[removed_index]->bookmark_name;
        remove_slot.source = removed_requirements[removed_index]->source;
        patch.remove_slots.push_back(std::move(remove_slot));
    }

    for (std::size_t added_index = 0U; added_index < added_requirements.size();
         ++added_index) {
        if (added_consumed[added_index]) {
            continue;
        }
        upsert_target.requirements.push_back(*added_requirements[added_index]);
    }

    if (!upsert_target.requirements.empty()) {
        patch.upsert_targets.push_back(std::move(upsert_target));
    }
}

} // namespace

auto build_template_schema_patch_document(
    const template_schema_diff_result &diff,
    built_template_schema_patch_summary &summary)
    -> template_schema_patch_document {
    template_schema_patch_document patch;
    summary = {};
    summary.added_target_count = diff.added_targets.size();
    summary.removed_target_count = diff.removed_targets.size();
    summary.changed_target_count = diff.changed_targets.size();

    patch.remove_targets.reserve(diff.removed_targets.size() +
                                 diff.changed_targets.size());
    patch.upsert_targets.reserve(diff.added_targets.size() +
                                 diff.changed_targets.size());
    for (const auto &target : diff.removed_targets) {
        patch.remove_targets.push_back(
            make_template_schema_patch_target_selector(target));
    }
    for (const auto &target_pair : diff.changed_targets) {
        append_changed_target_template_schema_patch(target_pair.left,
                                                   target_pair.right, patch);
    }

    patch.upsert_targets.insert(patch.upsert_targets.end(), diff.added_targets.begin(),
                                diff.added_targets.end());

    summary.generated_remove_target_count = patch.remove_targets.size();
    summary.generated_remove_slot_count = patch.remove_slots.size();
    summary.generated_rename_slot_count = patch.rename_slots.size();
    summary.generated_update_slot_count = patch.update_slots.size();
    summary.generated_upsert_target_count = patch.upsert_targets.size();
    return patch;
}

auto diff_template_schema_results(const exported_template_schema_result &left,
                                  const exported_template_schema_result &right)
    -> template_schema_diff_result {
    template_schema_diff_result result;

    std::size_t left_index = 0U;
    std::size_t right_index = 0U;
    while (left_index < left.targets.size() && right_index < right.targets.size()) {
        const auto selector_compare = compare_template_schema_target_selector(
            left.targets[left_index], right.targets[right_index]);
        if (selector_compare < 0) {
            result.removed_targets.push_back(left.targets[left_index]);
            ++left_index;
            continue;
        }
        if (selector_compare > 0) {
            result.added_targets.push_back(right.targets[right_index]);
            ++right_index;
            continue;
        }

        std::size_t left_group_end = left_index + 1U;
        while (left_group_end < left.targets.size() &&
               compare_template_schema_target_selector(left.targets[left_index],
                                                       left.targets[left_group_end]) == 0) {
            ++left_group_end;
        }
        std::size_t right_group_end = right_index + 1U;
        while (right_group_end < right.targets.size() &&
               compare_template_schema_target_selector(right.targets[right_index],
                                                       right.targets[right_group_end]) == 0) {
            ++right_group_end;
        }

        const auto left_group_size = left_group_end - left_index;
        const auto right_group_size = right_group_end - right_index;
        if (left_group_size == 1U && right_group_size == 1U) {
            if (compare_template_schema_target(left.targets[left_index],
                                               right.targets[right_index]) != 0) {
                result.changed_targets.push_back(
                    {left.targets[left_index], right.targets[right_index]});
            }
        } else {
            const bool groups_equal =
                left_group_size == right_group_size &&
                std::equal(
                    left.targets.begin() + static_cast<std::ptrdiff_t>(left_index),
                    left.targets.begin() +
                        static_cast<std::ptrdiff_t>(left_group_end),
                    right.targets.begin() + static_cast<std::ptrdiff_t>(right_index),
                    [](const exported_template_schema_target &left_target,
                       const exported_template_schema_target &right_target) {
                        return compare_template_schema_target(left_target,
                                                              right_target) == 0;
                    });
            if (!groups_equal) {
                result.removed_targets.insert(
                    result.removed_targets.end(),
                    left.targets.begin() + static_cast<std::ptrdiff_t>(left_index),
                    left.targets.begin() + static_cast<std::ptrdiff_t>(left_group_end));
                result.added_targets.insert(
                    result.added_targets.end(),
                    right.targets.begin() + static_cast<std::ptrdiff_t>(right_index),
                    right.targets.begin() + static_cast<std::ptrdiff_t>(right_group_end));
            }
        }

        left_index = left_group_end;
        right_index = right_group_end;
    }

    result.removed_targets.insert(
        result.removed_targets.end(),
        left.targets.begin() + static_cast<std::ptrdiff_t>(left_index),
        left.targets.end());
    result.added_targets.insert(
        result.added_targets.end(),
        right.targets.begin() + static_cast<std::ptrdiff_t>(right_index),
        right.targets.end());
    return result;
}

} // namespace featherdoc_cli
