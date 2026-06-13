#include "featherdoc_cli_template_schema_patch_ops.hpp"

#include "featherdoc_cli_template_schema_ops.hpp"

#include <algorithm>
#include <cstddef>
#include <utility>

namespace featherdoc_cli {

void apply_template_schema_patch(exported_template_schema_result &result,
                                 const template_schema_patch_document &patch,
                                 patched_template_schema_summary &summary) {
    summary.upsert_target_count = patch.upsert_targets.size();
    summary.remove_target_count = patch.remove_targets.size();
    summary.remove_slot_count = patch.remove_slots.size();
    summary.rename_slot_count = patch.rename_slots.size();
    summary.update_slot_count = patch.update_slots.size();

    for (const auto &selector : patch.remove_targets) {
        const auto previous_size = result.targets.size();
        result.targets.erase(
            std::remove_if(result.targets.begin(), result.targets.end(),
                           [&](const exported_template_schema_target &target) {
                               return template_schema_patch_selector_matches_target(
                                   selector, target);
                           }),
            result.targets.end());
        summary.applied_remove_target_count += previous_size - result.targets.size();
    }

    for (const auto &remove_slot : patch.remove_slots) {
        for (auto target_it = result.targets.begin(); target_it != result.targets.end();) {
            if (!template_schema_patch_selector_matches_target(remove_slot.target,
                                                               *target_it)) {
                ++target_it;
                continue;
            }

            const auto previous_slot_count = target_it->requirements.size();
            target_it->requirements.erase(
                std::remove_if(
                    target_it->requirements.begin(), target_it->requirements.end(),
                    [&](const featherdoc::template_slot_requirement &requirement) {
                        return requirement.source == remove_slot.source &&
                               requirement.bookmark_name ==
                                   remove_slot.bookmark_name;
                    }),
                target_it->requirements.end());
            summary.applied_remove_slot_count +=
                previous_slot_count - target_it->requirements.size();

            if (target_it->requirements.empty()) {
                target_it = result.targets.erase(target_it);
                ++summary.pruned_empty_target_count;
                continue;
            }

            ++target_it;
        }
    }

    for (const auto &rename_slot : patch.rename_slots) {
        for (auto &target : result.targets) {
            if (!template_schema_patch_selector_matches_target(rename_slot.target,
                                                               target)) {
                continue;
            }

            for (auto &requirement : target.requirements) {
                if (requirement.source != rename_slot.source ||
                    requirement.bookmark_name != rename_slot.bookmark_name ||
                    requirement.bookmark_name == rename_slot.new_bookmark_name) {
                    continue;
                }
                requirement.bookmark_name = rename_slot.new_bookmark_name;
                ++summary.applied_rename_slot_count;
            }
        }
    }

    for (const auto &update_slot : patch.update_slots) {
        for (auto &target : result.targets) {
            if (!template_schema_patch_selector_matches_target(update_slot.target,
                                                               target)) {
                continue;
            }
            for (auto &requirement : target.requirements) {
                if (requirement.source != update_slot.source ||
                    requirement.bookmark_name != update_slot.bookmark_name) {
                    continue;
                }
                auto updated_requirement = requirement;
                if (update_slot.update.kind.has_value()) {
                    updated_requirement.kind = *update_slot.update.kind;
                }
                if (update_slot.update.required.has_value()) {
                    updated_requirement.required = *update_slot.update.required;
                }
                if (update_slot.update.clear_min_occurrences) {
                    updated_requirement.min_occurrences.reset();
                }
                if (update_slot.update.min_occurrences.has_value()) {
                    updated_requirement.min_occurrences =
                        *update_slot.update.min_occurrences;
                }
                if (update_slot.update.clear_max_occurrences) {
                    updated_requirement.max_occurrences.reset();
                }
                if (update_slot.update.max_occurrences.has_value()) {
                    updated_requirement.max_occurrences =
                        *update_slot.update.max_occurrences;
                }
                if (compare_template_slot_requirement(requirement,
                                                      updated_requirement) == 0) {
                    continue;
                }
                requirement = std::move(updated_requirement);
                ++summary.applied_update_slot_count;
            }
        }
    }

    if (!patch.upsert_targets.empty()) {
        exported_template_schema_result overlay{};
        overlay.targets = patch.upsert_targets;
        merged_template_schema_summary merge_summary{};
        merge_template_schema_result(result, overlay, merge_summary);
        summary.updated_target_count = merge_summary.updated_target_count;
        summary.replaced_slot_count = merge_summary.replaced_slot_count;
    }
}

} // namespace featherdoc_cli
