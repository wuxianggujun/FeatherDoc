#include "featherdoc_cli_template_schema_parse_detail.hpp"

#include "featherdoc_cli_domain_parse.hpp"
#include "featherdoc_cli_json_parse.hpp"
#include "featherdoc_cli_template_schema_slot_parse.hpp"

namespace featherdoc_cli {
namespace {

[[nodiscard]] auto report_duplicate_template_schema_member(
    std::string_view member_name, std::string &error_message) -> bool {
    error_message = "JSON schema member '" + std::string(member_name) +
                    "' must not be duplicated";
    return false;
}

[[nodiscard]] auto parse_template_schema_string_member(
    std::string_view content, std::size_t &index, std::string_view member_name,
    std::optional<std::string> &target, std::string &error_message) -> bool {
    if (target.has_value()) {
        return report_duplicate_template_schema_member(member_name,
                                                       error_message);
    }

    skip_json_patch_whitespace(content, index);
    target.emplace();
    return parse_json_patch_string(content, index, *target, error_message);
}

[[nodiscard]] auto parse_template_schema_index_member(
    std::string_view content, std::size_t &index, std::string_view member_name,
    std::optional<std::size_t> &target, std::string &error_message) -> bool {
    if (target.has_value()) {
        return report_duplicate_template_schema_member(member_name,
                                                       error_message);
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

auto parse_template_schema_json_target_member(
    std::string_view content, std::size_t &index, std::string_view member_name,
    template_schema_json_target_members &members,
    std::string &error_message) -> bool {
    if (member_name == "part") {
        return parse_template_schema_string_member(
            content, index, member_name, members.part_value, error_message);
    }
    if (member_name == "index") {
        return parse_template_schema_index_member(
            content, index, member_name, members.index_value, error_message);
    }
    if (member_name == "section") {
        return parse_template_schema_index_member(
            content, index, member_name, members.section_value, error_message);
    }
    if (member_name == "kind") {
        if (members.kind_value.has_value()) {
            return report_duplicate_template_schema_member(member_name,
                                                           error_message);
        }
        featherdoc::section_reference_kind parsed_kind{};
        if (!parse_json_patch_reference_kind_value(content, index, parsed_kind,
                                                   error_message)) {
            return false;
        }
        members.kind_value = parsed_kind;
        return true;
    }
    if (member_name == "slots") {
        if (members.saw_slots) {
            return report_duplicate_template_schema_member(member_name,
                                                           error_message);
        }
        if (!parse_template_schema_json_slots(content, index, members.slots,
                                              error_message)) {
            return false;
        }
        members.saw_slots = true;
        return true;
    }
    if (member_name == "linked_to_previous") {
        if (members.linked_to_previous_value.has_value()) {
            return report_duplicate_template_schema_member(member_name,
                                                           error_message);
        }
        bool parsed_value = false;
        if (!parse_json_patch_bool_member_value(content, index, parsed_value,
                                                "linked_to_previous",
                                                error_message)) {
            return false;
        }
        members.linked_to_previous_value = parsed_value;
        return true;
    }
    if (member_name == "resolved_from_section") {
        return parse_template_schema_index_member(
            content, index, member_name, members.resolved_from_section_value,
            error_message);
    }
    if (member_name == "entry_name") {
        return parse_template_schema_string_member(
            content, index, member_name, members.entry_name_value,
            error_message);
    }

    return report_json_input_error("JSON schema file", index,
                                   "unknown target member", error_message);
}

} // namespace featherdoc_cli
