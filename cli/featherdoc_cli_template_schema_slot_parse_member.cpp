#include "featherdoc_cli_template_schema_slot_parse_detail.hpp"

#include "featherdoc_cli_json_parse.hpp"

namespace featherdoc_cli {
namespace {

[[nodiscard]] auto report_duplicate_schema_slot_member(
    std::string_view member_name, std::string &error_message) -> bool {
    error_message = "JSON schema slot member '" + std::string(member_name) +
                    "' must not be duplicated";
    return false;
}

[[nodiscard]] auto parse_schema_slot_string_member(
    std::string_view content, std::size_t &index, std::string_view member_name,
    std::optional<std::string> &target, std::string &error_message) -> bool {
    if (target.has_value()) {
        return report_duplicate_schema_slot_member(member_name, error_message);
    }

    skip_json_patch_whitespace(content, index);
    target.emplace();
    return parse_json_patch_string(content, index, *target, error_message);
}

[[nodiscard]] auto parse_schema_slot_index_member(
    std::string_view content, std::size_t &index, std::string_view member_name,
    std::optional<std::size_t> &target, std::string &error_message) -> bool {
    if (target.has_value()) {
        return report_duplicate_schema_slot_member(member_name, error_message);
    }

    std::size_t parsed_index = 0U;
    if (!parse_json_patch_index_value(content, index, parsed_index,
                                      member_name, error_message)) {
        return false;
    }
    target = parsed_index;
    return true;
}

} // namespace

auto parse_template_schema_json_slot_member(
    std::string_view content, std::size_t &index, std::string_view member_name,
    template_schema_json_slot_members &members, std::string &error_message)
    -> bool {
    if (member_name == "bookmark") {
        return parse_schema_slot_string_member(content, index, member_name,
                                               members.bookmark_value,
                                               error_message);
    }
    if (member_name == "bookmark_name") {
        return parse_schema_slot_string_member(content, index, member_name,
                                               members.bookmark_name_value,
                                               error_message);
    }
    if (member_name == "content_control_tag") {
        return parse_schema_slot_string_member(
            content, index, member_name, members.content_control_tag_value,
            error_message);
    }
    if (member_name == "content_control_alias") {
        return parse_schema_slot_string_member(
            content, index, member_name, members.content_control_alias_value,
            error_message);
    }
    if (member_name == "kind") {
        return parse_schema_slot_string_member(content, index, member_name,
                                               members.kind_value,
                                               error_message);
    }
    if (member_name == "required") {
        if (members.required_value.has_value()) {
            return report_duplicate_schema_slot_member(member_name,
                                                       error_message);
        }
        bool parsed_required = true;
        if (!parse_json_patch_bool_member_value(content, index,
                                                parsed_required, "required",
                                                error_message)) {
            return false;
        }
        members.required_value = parsed_required;
        return true;
    }
    if (member_name == "count") {
        return parse_schema_slot_index_member(content, index, member_name,
                                              members.count_value,
                                              error_message);
    }
    if (member_name == "min") {
        return parse_schema_slot_index_member(content, index, member_name,
                                              members.min_value,
                                              error_message);
    }
    if (member_name == "max") {
        return parse_schema_slot_index_member(content, index, member_name,
                                              members.max_value,
                                              error_message);
    }

    return report_json_input_error("JSON schema file", index,
                                   "unknown slot member", error_message);
}

} // namespace featherdoc_cli
