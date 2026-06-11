#include "featherdoc_cli_template_schema_patch_parse_update_slot_state.hpp"

#include "featherdoc_cli_json_parse.hpp"

#include <string>
#include <utility>

namespace featherdoc_cli::detail {
namespace {

auto resolve_json_patch_bool_alias(const std::optional<bool> &primary,
                                   const std::optional<bool> &alias,
                                   std::string_view primary_name,
                                   std::string_view alias_name,
                                   std::optional<bool> &resolved,
                                   std::string &error_message) -> bool {
    if (primary.has_value() && alias.has_value() && *primary != *alias) {
        error_message = "JSON patch members '" + std::string(primary_name) +
                        "' and '" + std::string(alias_name) +
                        "' must match when both are present";
        return false;
    }
    resolved = primary.has_value() ? primary : alias;
    return true;
}

auto resolve_update_slot_selector_name(
    const template_schema_patch_update_slot_parse_state &state,
    featherdoc::template_slot_source_kind &source, std::string &slot_name,
    std::string &error_message) -> bool {
    std::optional<std::string> resolved_bookmark_name;
    if (!resolve_json_patch_string_member(state.bookmark_value,
                                          state.bookmark_name_value, "bookmark",
                                          "bookmark_name",
                                          resolved_bookmark_name,
                                          error_message)) {
        return false;
    }

    std::size_t selector_count = 0U;
    selector_count += resolved_bookmark_name.has_value() ? 1U : 0U;
    selector_count += state.content_control_tag_value.has_value() ? 1U : 0U;
    selector_count += state.content_control_alias_value.has_value() ? 1U : 0U;
    if (selector_count != 1U) {
        error_message =
            "JSON schema patch update-slot object must contain exactly one of "
            "'bookmark', 'bookmark_name', 'content_control_tag', or "
            "'content_control_alias'";
        return false;
    }

    source = featherdoc::template_slot_source_kind::bookmark;
    if (resolved_bookmark_name.has_value()) {
        slot_name = *resolved_bookmark_name;
    } else if (state.content_control_tag_value.has_value()) {
        source = featherdoc::template_slot_source_kind::content_control_tag;
        slot_name = *state.content_control_tag_value;
    } else {
        source = featherdoc::template_slot_source_kind::content_control_alias;
        slot_name = *state.content_control_alias_value;
    }

    if (slot_name.empty()) {
        error_message = "JSON schema patch update-slot selector must not be empty";
        return false;
    }
    return true;
}

auto resolve_update_slot_occurrence_aliases(
    const template_schema_patch_update_slot_parse_state &state,
    std::optional<std::size_t> &resolved_min,
    std::optional<std::size_t> &resolved_max,
    std::optional<bool> &resolved_clear_min,
    std::optional<bool> &resolved_clear_max,
    std::string &error_message) -> bool {
    if (!resolve_json_patch_index_member(state.min_value,
                                         state.min_occurrences_value, "min",
                                         "min_occurrences", resolved_min,
                                         error_message)) {
        return false;
    }
    if (!resolve_json_patch_index_member(state.max_value,
                                         state.max_occurrences_value, "max",
                                         "max_occurrences", resolved_max,
                                         error_message)) {
        return false;
    }
    return resolve_json_patch_bool_alias(
               state.clear_min_value, state.clear_min_occurrences_value,
               "clear_min", "clear_min_occurrences", resolved_clear_min,
               error_message) &&
           resolve_json_patch_bool_alias(
               state.clear_max_value, state.clear_max_occurrences_value,
               "clear_max", "clear_max_occurrences", resolved_clear_max,
               error_message);
}

auto validate_update_slot_occurrence_options(
    const template_schema_patch_update_slot_parse_state &state,
    const std::optional<std::size_t> &resolved_min,
    const std::optional<std::size_t> &resolved_max,
    const std::optional<bool> &resolved_clear_min,
    const std::optional<bool> &resolved_clear_max,
    std::string &error_message) -> bool {
    if (state.count_value.has_value() &&
        (resolved_min.has_value() || resolved_max.has_value())) {
        error_message =
            "JSON schema patch update-slot member 'count' conflicts with 'min'/'max'";
        return false;
    }
    if (state.count_value.has_value() &&
        (resolved_clear_min.value_or(false) || resolved_clear_max.value_or(false))) {
        error_message = "JSON schema patch update-slot member 'count' conflicts "
                        "with clear occurrence flags";
        return false;
    }
    if (resolved_min.has_value() && resolved_clear_min.value_or(false)) {
        error_message = "JSON schema patch update-slot member 'min' conflicts with "
                        "'clear_min_occurrences'";
        return false;
    }
    if (resolved_max.has_value() && resolved_clear_max.value_or(false)) {
        error_message = "JSON schema patch update-slot member 'max' conflicts with "
                        "'clear_max_occurrences'";
        return false;
    }
    if (resolved_min.has_value() && resolved_max.has_value() &&
        *resolved_max < *resolved_min) {
        error_message = "JSON schema patch update-slot occurrence range is invalid: "
                        "max must be greater than or equal to min";
        return false;
    }
    return true;
}

} // namespace

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
