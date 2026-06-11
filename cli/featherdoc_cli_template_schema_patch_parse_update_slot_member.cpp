#include "featherdoc_cli_template_schema_patch_parse_update_slot_state.hpp"

#include "featherdoc_cli_json_parse.hpp"
#include "featherdoc_cli_template_schema_patch_parse_update_slot_member_helpers.hpp"

#include <string>

namespace featherdoc_cli::detail {

auto parse_template_schema_patch_update_slot_member(
    std::string_view content, std::size_t &index, std::string_view member_name,
    template_schema_patch_update_slot_parse_state &state,
    std::string &error_message) -> bool {
    if (member_name == "part") {
        return parse_update_slot_string_member(content, index, member_name,
                                               state.part_value, error_message);
    }
    if (member_name == "index") {
        return parse_update_slot_selector_index_member(
            content, index, member_name, state.index_value, error_message);
    }
    if (member_name == "part_index") {
        return parse_update_slot_selector_index_member(
            content, index, member_name, state.part_index_value, error_message);
    }
    if (member_name == "section") {
        return parse_update_slot_selector_index_member(
            content, index, member_name, state.section_value, error_message);
    }
    if (member_name == "kind") {
        return parse_update_slot_kind_member(content, index, state, error_message);
    }
    if (member_name == "target_kind" || member_name == "reference_kind") {
        return parse_update_slot_reference_kind_member(
            content, index, member_name, state, error_message);
    }
    if (member_name == "slot_kind") {
        return parse_update_slot_slot_kind_member(content, index, state,
                                                  error_message);
    }
    if (member_name == "linked_to_previous") {
        return parse_update_slot_selector_bool_member(
            content, index, member_name, state.linked_to_previous_value,
            error_message);
    }
    if (member_name == "resolved_from_section") {
        return parse_update_slot_selector_index_member(
            content, index, member_name, state.resolved_from_section_value,
            error_message);
    }
    if (member_name == "bookmark") {
        return parse_update_slot_string_member(content, index, member_name,
                                               state.bookmark_value, error_message);
    }
    if (member_name == "bookmark_name") {
        return parse_update_slot_string_member(content, index, member_name,
                                               state.bookmark_name_value,
                                               error_message);
    }
    if (member_name == "content_control_tag") {
        return parse_update_slot_string_member(content, index, member_name,
                                               state.content_control_tag_value,
                                               error_message);
    }
    if (member_name == "content_control_alias") {
        return parse_update_slot_string_member(content, index, member_name,
                                               state.content_control_alias_value,
                                               error_message);
    }
    if (member_name == "required") {
        return parse_update_slot_bool_member(content, index, member_name,
                                             state.required_value, error_message);
    }
    if (member_name == "count") {
        return parse_update_slot_index_member(content, index, member_name,
                                              state.count_value, error_message);
    }
    if (member_name == "min") {
        return parse_update_slot_index_member(content, index, member_name,
                                              state.min_value, error_message);
    }
    if (member_name == "min_occurrences") {
        return parse_update_slot_index_member(content, index, member_name,
                                              state.min_occurrences_value,
                                              error_message);
    }
    if (member_name == "max") {
        return parse_update_slot_index_member(content, index, member_name,
                                              state.max_value, error_message);
    }
    if (member_name == "max_occurrences") {
        return parse_update_slot_index_member(content, index, member_name,
                                              state.max_occurrences_value,
                                              error_message);
    }
    if (member_name == "clear_min") {
        return parse_update_slot_bool_member(content, index, member_name,
                                             state.clear_min_value, error_message);
    }
    if (member_name == "clear_min_occurrences") {
        return parse_update_slot_bool_member(content, index, member_name,
                                             state.clear_min_occurrences_value,
                                             error_message);
    }
    if (member_name == "clear_max") {
        return parse_update_slot_bool_member(content, index, member_name,
                                             state.clear_max_value, error_message);
    }
    if (member_name == "clear_max_occurrences") {
        return parse_update_slot_bool_member(content, index, member_name,
                                             state.clear_max_occurrences_value,
                                             error_message);
    }
    if (member_name == "entry_name") {
        std::string ignored;
        skip_json_patch_whitespace(content, index);
        return parse_json_patch_string(content, index, ignored, error_message);
    }

    return report_json_input_error("JSON schema patch file", index,
                                   "unknown update-slot member", error_message);
}

} // namespace featherdoc_cli::detail
