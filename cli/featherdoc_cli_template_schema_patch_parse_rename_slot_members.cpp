#include "featherdoc_cli_template_schema_patch_parse_rename_slot_detail.hpp"

#include "featherdoc_cli_json_parse.hpp"

namespace featherdoc_cli::detail {

auto parse_template_schema_patch_rename_slot_members(
    std::string_view content, std::size_t &index,
    template_schema_patch_rename_slot_members &members,
    std::string &error_message) -> bool {
    skip_json_patch_whitespace(content, index);
    if (index >= content.size() || content[index] != '{') {
        return report_json_input_error("JSON schema patch file", index,
                                       "expected rename-slot object",
                                       error_message);
    }

    members = {};
    ++index;
    skip_json_patch_whitespace(content, index);
    if (index < content.size() && content[index] == '}') {
        ++index;
        return true;
    }

    while (index < content.size()) {
        std::string member_name;
        if (!parse_json_patch_string(content, index, member_name,
                                     error_message)) {
            return false;
        }

        skip_json_patch_whitespace(content, index);
        if (index >= content.size() || content[index] != ':') {
            return report_json_input_error("JSON schema patch file", index,
                                           "expected ':' after object member",
                                           error_message);
        }

        ++index;
        if (!parse_template_schema_patch_rename_slot_member(
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
        return report_json_input_error("JSON schema patch file", index,
                                       "expected ',' or '}' after object member",
                                       error_message);
    }

    return true;
}

} // namespace featherdoc_cli::detail
