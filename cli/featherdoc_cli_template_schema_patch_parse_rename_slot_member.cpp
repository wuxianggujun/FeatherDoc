#include "featherdoc_cli_template_schema_patch_parse_rename_slot_detail.hpp"

#include "featherdoc_cli_domain_parse.hpp"
#include "featherdoc_cli_json_parse.hpp"

namespace featherdoc_cli::detail {
namespace {

[[nodiscard]] auto report_duplicate_rename_slot_member(
    std::string_view member_name, std::string &error_message) -> bool {
    error_message = "JSON schema patch member '" + std::string(member_name) +
                    "' must not be duplicated";
    return false;
}

[[nodiscard]] auto parse_string_member(
    std::string_view content, std::size_t &index, std::string_view member_name,
    std::optional<std::string> &target, std::string &error_message) -> bool {
    if (target.has_value()) {
        return report_duplicate_rename_slot_member(member_name, error_message);
    }

    skip_json_patch_whitespace(content, index);
    target.emplace();
    return parse_json_patch_string(content, index, *target, error_message);
}

[[nodiscard]] auto parse_index_member(
    std::string_view content, std::size_t &index, std::string_view member_name,
    std::optional<std::size_t> &target, std::string &error_message) -> bool {
    if (target.has_value()) {
        return report_duplicate_rename_slot_member(member_name, error_message);
    }

    std::size_t parsed_value = 0U;
    if (!parse_json_patch_index_value(content, index, parsed_value, member_name,
                                      error_message)) {
        return false;
    }
    target = parsed_value;
    return true;
}

[[nodiscard]] auto parse_kind_member(
    std::string_view content, std::size_t &index, std::string_view member_name,
    std::optional<featherdoc::section_reference_kind> &target,
    std::string &error_message) -> bool {
    if (target.has_value()) {
        return report_duplicate_rename_slot_member(member_name, error_message);
    }

    featherdoc::section_reference_kind parsed_kind{};
    if (!parse_json_patch_reference_kind_value(content, index, parsed_kind,
                                               error_message)) {
        return false;
    }
    target = parsed_kind;
    return true;
}

[[nodiscard]] auto parse_bool_member(
    std::string_view content, std::size_t &index, std::string_view member_name,
    std::optional<bool> &target, std::string &error_message) -> bool {
    if (target.has_value()) {
        return report_duplicate_rename_slot_member(member_name, error_message);
    }

    bool parsed_value = false;
    if (!parse_json_patch_bool_member_value(content, index, parsed_value,
                                            member_name, error_message)) {
        return false;
    }
    target = parsed_value;
    return true;
}

} // namespace

auto parse_template_schema_patch_rename_slot_member(
    std::string_view content, std::size_t &index, std::string_view member_name,
    template_schema_patch_rename_slot_members &members,
    std::string &error_message) -> bool {
    if (member_name == "part") {
        return parse_string_member(content, index, member_name,
                                   members.part_value, error_message);
    }
    if (member_name == "index") {
        return parse_index_member(content, index, member_name,
                                  members.index_value, error_message);
    }
    if (member_name == "part_index") {
        return parse_index_member(content, index, member_name,
                                  members.part_index_value, error_message);
    }
    if (member_name == "section") {
        return parse_index_member(content, index, member_name,
                                  members.section_value, error_message);
    }
    if (member_name == "kind") {
        return parse_kind_member(content, index, member_name, members.kind_value,
                                 error_message);
    }
    if (member_name == "linked_to_previous") {
        return parse_bool_member(content, index, member_name,
                                 members.linked_to_previous_value,
                                 error_message);
    }
    if (member_name == "resolved_from_section") {
        return parse_index_member(content, index, member_name,
                                  members.resolved_from_section_value,
                                  error_message);
    }
    if (member_name == "bookmark") {
        return parse_string_member(content, index, member_name,
                                   members.bookmark_value, error_message);
    }
    if (member_name == "bookmark_name") {
        return parse_string_member(content, index, member_name,
                                   members.bookmark_name_value, error_message);
    }
    if (member_name == "content_control_tag") {
        return parse_string_member(content, index, member_name,
                                   members.content_control_tag_value,
                                   error_message);
    }
    if (member_name == "content_control_alias") {
        return parse_string_member(content, index, member_name,
                                   members.content_control_alias_value,
                                   error_message);
    }
    if (member_name == "new_bookmark") {
        return parse_string_member(content, index, member_name,
                                   members.new_bookmark_value, error_message);
    }
    if (member_name == "new_bookmark_name") {
        return parse_string_member(content, index, member_name,
                                   members.new_bookmark_name_value,
                                   error_message);
    }
    if (member_name == "new_content_control_tag") {
        return parse_string_member(content, index, member_name,
                                   members.new_content_control_tag_value,
                                   error_message);
    }
    if (member_name == "new_content_control_alias") {
        return parse_string_member(content, index, member_name,
                                   members.new_content_control_alias_value,
                                   error_message);
    }
    if (member_name == "entry_name") {
        std::string ignored;
        skip_json_patch_whitespace(content, index);
        return parse_json_patch_string(content, index, ignored, error_message);
    }

    return report_json_input_error("JSON schema patch file", index,
                                   "unknown rename-slot member",
                                   error_message);
}

} // namespace featherdoc_cli::detail
