#include "featherdoc_cli_template_schema_patch_parse_update_slot_finalize_detail.hpp"

#include <string>
#include <utility>

namespace featherdoc_cli::detail {

auto finalize_template_schema_patch_update_slot(
    const template_schema_patch_update_slot_parse_state &state,
    template_schema_patch_update_slot &operation,
    std::string &error_message) -> bool {
    operation = {};
    if (!finalize_template_schema_patch_selector(
            state.part_value, state.index_value, state.part_index_value,
            state.section_value, state.kind_value,
            state.resolved_from_section_value, state.linked_to_previous_value,
            operation.target, error_message)) {
        return false;
    }

    auto source = featherdoc::template_slot_source_kind::bookmark;
    std::string resolved_slot_name;
    if (!resolve_update_slot_selector_name(state, source, resolved_slot_name,
                                           error_message)) {
        return false;
    }

    std::optional<std::size_t> resolved_min;
    std::optional<std::size_t> resolved_max;
    std::optional<bool> resolved_clear_min;
    std::optional<bool> resolved_clear_max;
    if (!resolve_update_slot_occurrence_aliases(
            state, resolved_min, resolved_max, resolved_clear_min,
            resolved_clear_max, error_message) ||
        !validate_update_slot_occurrence_options(
            state, resolved_min, resolved_max, resolved_clear_min,
            resolved_clear_max, error_message)) {
        return false;
    }

    operation.bookmark_name = std::move(resolved_slot_name);
    operation.source = source;
    operation.update.kind = state.slot_kind_value;
    operation.update.required = state.required_value;
    if (state.count_value.has_value()) {
        operation.update.min_occurrences = *state.count_value;
        operation.update.max_occurrences = *state.count_value;
    } else {
        operation.update.min_occurrences = resolved_min;
        operation.update.max_occurrences = resolved_max;
    }
    operation.update.clear_min_occurrences = resolved_clear_min.value_or(false);
    operation.update.clear_max_occurrences = resolved_clear_max.value_or(false);

    if (!operation.update.kind.has_value() &&
        !operation.update.required.has_value() &&
        !operation.update.min_occurrences.has_value() &&
        !operation.update.max_occurrences.has_value() &&
        !operation.update.clear_min_occurrences &&
        !operation.update.clear_max_occurrences) {
        error_message = "JSON schema patch update-slot object must contain at "
                        "least one update field";
        return false;
    }

    return true;
}

} // namespace featherdoc_cli::detail
