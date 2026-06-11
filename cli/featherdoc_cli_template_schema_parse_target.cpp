#include "featherdoc_cli_template_schema_parse_detail.hpp"

#include "featherdoc_cli_json_parse.hpp"
#include "featherdoc_cli_validation_part.hpp"

#include <utility>

namespace featherdoc_cli {

auto finalize_template_schema_json_target(
    const template_schema_json_target_members &members,
    exported_template_schema_target &target, std::string &error_message)
    -> bool {
    if (!members.part_value.has_value()) {
        error_message = "JSON schema target must contain 'part'";
        return false;
    }
    if (!members.saw_slots) {
        error_message = "JSON schema target must contain a 'slots' array";
        return false;
    }
    if (members.slots.empty()) {
        error_message = "JSON schema target 'slots' array must not be empty";
        return false;
    }

    target = {};
    if (!parse_validation_part(*members.part_value, target.part)) {
        error_message =
            "JSON schema member 'part' must be one of "
            "'body', 'header', 'footer', 'section-header', or 'section-footer'";
        return false;
    }
    target.part_index = members.index_value;
    target.section_index = members.section_value;
    target.requirements = members.slots;
    target.entry_name = members.entry_name_value.value_or(std::string{});
    target.reference_kind = members.kind_value;
    target.resolved_from_section_index = members.resolved_from_section_value;
    target.linked_to_previous =
        members.linked_to_previous_value.value_or(false);

    return validate_template_part_selection(target.part, target.part_index,
                                            target.section_index,
                                            target.reference_kind.has_value(),
                                            "schema validation",
                                            error_message);
}

auto parse_template_schema_json_target(
    std::string_view content, std::size_t &index,
    exported_template_schema_target &target, std::string &error_message)
    -> bool {
    skip_json_patch_whitespace(content, index);
    if (index >= content.size() || content[index] != '{') {
        return report_json_input_error("JSON schema file", index,
                                       "expected target object",
                                       error_message);
    }

    template_schema_json_target_members members;
    ++index;
    skip_json_patch_whitespace(content, index);
    if (index < content.size() && content[index] == '}') {
        ++index;
    } else {
        while (index < content.size()) {
            std::string member_name;
            if (!parse_json_patch_string(content, index, member_name,
                                         error_message)) {
                return false;
            }

            skip_json_patch_whitespace(content, index);
            if (index >= content.size() || content[index] != ':') {
                return report_json_input_error("JSON schema file", index,
                                               "expected ':' after object member",
                                               error_message);
            }

            ++index;
            if (!parse_template_schema_json_target_member(
                    content, index, member_name, members, error_message)) {
                return false;
            }

            skip_json_patch_whitespace(content, index);
            if (index >= content.size()) {
                break;
            }
            if (content[index] == ',') {
                ++index;
                skip_json_patch_whitespace(content, index);
                continue;
            }
            if (content[index] == '}') {
                ++index;
                break;
            }
            return report_json_input_error("JSON schema file", index,
                                           "expected ',' or '}' after object member",
                                           error_message);
        }
    }

    return finalize_template_schema_json_target(members, target,
                                                error_message);
}

} // namespace featherdoc_cli
