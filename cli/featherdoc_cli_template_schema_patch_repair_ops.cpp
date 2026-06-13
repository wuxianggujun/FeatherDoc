#include "featherdoc_cli_template_schema_patch_ops.hpp"

#include "featherdoc_cli_template_schema_ops.hpp"

#include <algorithm>
#include <cstddef>

namespace featherdoc_cli {
namespace {

[[nodiscard]] auto exported_template_schema_results_equal_exact(
    const exported_template_schema_result &left,
    const exported_template_schema_result &right) -> bool {
    if (left.targets.size() != right.targets.size()) {
        return false;
    }

    for (std::size_t index = 0U; index < left.targets.size(); ++index) {
        if (compare_template_schema_target(left.targets[index],
                                           right.targets[index]) != 0 ||
            left.targets[index].entry_name != right.targets[index].entry_name) {
            return false;
        }
    }

    return true;
}

} // namespace

void repair_template_schema_result(const exported_template_schema_result &input,
                                   exported_template_schema_result &result,
                                   repaired_template_schema_summary &summary) {
    summary = {};
    summary.input_target_count = input.targets.size();
    summary.input_slot_count = input.slot_count();
    summary.stripped_entry_name_count = static_cast<std::size_t>(std::count_if(
        input.targets.begin(), input.targets.end(),
        [](const exported_template_schema_target &target) {
            return !target.entry_name.empty();
        }));

    result = {};
    merged_template_schema_summary merge_summary{};
    merge_template_schema_result(result, input, merge_summary);
    normalize_template_schema_result(result);

    summary.merged_duplicate_target_count = merge_summary.updated_target_count;
    summary.replaced_slot_count = merge_summary.replaced_slot_count;
    summary.deduplicated_target_count =
        summary.input_target_count - result.targets.size();
    summary.deduplicated_slot_count =
        summary.input_slot_count - result.slot_count();
    summary.changed = !exported_template_schema_results_equal_exact(input, result);
}

} // namespace featherdoc_cli
