#pragma once

#include "featherdoc_cli_template_schema_model.hpp"

namespace featherdoc_cli {

void normalize_template_schema_result(exported_template_schema_result &result);

void merge_template_schema_result(exported_template_schema_result &base,
                                  const exported_template_schema_result &overlay,
                                  merged_template_schema_summary &summary);

[[nodiscard]] auto template_schema_patch_selector_matches_target(
    const template_schema_patch_target_selector &selector,
    const exported_template_schema_target &target) -> bool;

void apply_template_schema_patch(exported_template_schema_result &result,
                                 const template_schema_patch_document &patch,
                                 patched_template_schema_summary &summary);

[[nodiscard]] auto has_template_schema_resolved_target_metadata(
    const exported_template_schema_result &result) -> bool;

[[nodiscard]] auto has_template_schema_resolved_target_metadata(
    const template_schema_patch_document &patch) -> bool;

void repair_template_schema_result(const exported_template_schema_result &input,
                                   exported_template_schema_result &result,
                                   repaired_template_schema_summary &summary);

[[nodiscard]] auto build_template_schema_patch_document(
    const template_schema_diff_result &diff, built_template_schema_patch_summary &summary)
    -> template_schema_patch_document;

[[nodiscard]] auto diff_template_schema_results(
    const exported_template_schema_result &left,
    const exported_template_schema_result &right) -> template_schema_diff_result;

} // namespace featherdoc_cli
