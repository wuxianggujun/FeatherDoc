#include "featherdoc_cli_template_schema_patch_ops.hpp"

#include "featherdoc_cli_template_schema_ops.hpp"

#include <algorithm>

namespace featherdoc_cli {

void normalize_template_schema_result(exported_template_schema_result &result) {
    for (auto &target : result.targets) {
        std::sort(target.requirements.begin(), target.requirements.end(),
                  [](const featherdoc::template_slot_requirement &left,
                     const featherdoc::template_slot_requirement &right) {
                      return compare_template_slot_requirement(left, right) < 0;
                  });
        target.entry_name.clear();
    }

    std::sort(result.targets.begin(), result.targets.end(),
              [](const exported_template_schema_target &left,
                 const exported_template_schema_target &right) {
                  return compare_template_schema_target(left, right) < 0;
              });
}

void merge_template_schema_result(exported_template_schema_result &base,
                                  const exported_template_schema_result &overlay,
                                  merged_template_schema_summary &summary) {
    ++summary.input_count;
    for (const auto &overlay_target : overlay.targets) {
        auto existing_it =
            std::find_if(base.targets.begin(), base.targets.end(),
                         [&](const exported_template_schema_target &candidate) {
                             return compare_template_schema_target_identity(
                                        candidate, overlay_target) == 0;
                         });
        if (existing_it == base.targets.end()) {
            base.targets.push_back(overlay_target);
            existing_it = base.targets.end() - 1;
            existing_it->requirements.clear();
        } else {
            ++summary.updated_target_count;
        }

        if (!overlay_target.entry_name.empty()) {
            existing_it->entry_name = overlay_target.entry_name;
        }

        for (const auto &requirement : overlay_target.requirements) {
            auto requirement_it = std::find_if(
                existing_it->requirements.begin(), existing_it->requirements.end(),
                [&](const featherdoc::template_slot_requirement &candidate) {
                    return candidate.source == requirement.source &&
                           candidate.bookmark_name == requirement.bookmark_name;
                });
            if (requirement_it == existing_it->requirements.end()) {
                existing_it->requirements.push_back(requirement);
                continue;
            }

            if (compare_template_slot_requirement(*requirement_it, requirement) != 0) {
                ++summary.replaced_slot_count;
                *requirement_it = requirement;
            }
        }
    }
}

auto template_schema_patch_selector_matches_target(
    const template_schema_patch_target_selector &selector,
    const exported_template_schema_target &target) -> bool {
    return selector.part == target.part &&
           selector.part_index == target.part_index &&
           selector.section_index == target.section_index &&
           selector.reference_kind == target.reference_kind &&
           selector.resolved_from_section_index ==
               target.resolved_from_section_index &&
           selector.linked_to_previous == target.linked_to_previous;
}

auto has_template_schema_resolved_target_metadata(
    const exported_template_schema_result &result) -> bool {
    return std::any_of(result.targets.begin(), result.targets.end(),
                       [](const exported_template_schema_target &target) {
                           return target.resolved_from_section_index.has_value() ||
                                  target.linked_to_previous;
                       });
}

auto has_template_schema_resolved_target_metadata(
    const template_schema_patch_document &patch) -> bool {
    const auto selector_has_metadata = [](const auto &selector) {
        return selector.resolved_from_section_index.has_value() ||
               selector.linked_to_previous;
    };

    return has_template_schema_resolved_target_metadata(
               exported_template_schema_result{patch.upsert_targets, {}}) ||
           std::any_of(patch.remove_targets.begin(), patch.remove_targets.end(),
                       selector_has_metadata) ||
           std::any_of(patch.remove_slots.begin(), patch.remove_slots.end(),
                       [&](const template_schema_patch_remove_slot &slot) {
                           return selector_has_metadata(slot.target);
                       }) ||
           std::any_of(patch.rename_slots.begin(), patch.rename_slots.end(),
                       [&](const template_schema_patch_rename_slot &slot) {
                           return selector_has_metadata(slot.target);
                       }) ||
           std::any_of(patch.update_slots.begin(), patch.update_slots.end(),
                       [&](const template_schema_patch_update_slot &slot) {
                           return selector_has_metadata(slot.target);
                       });
}

} // namespace featherdoc_cli
