#include "featherdoc_cli_review_mutation_plan_build_request_operation_parse_detail.hpp"

#include "featherdoc_cli_json_parse.hpp"
#include "featherdoc_cli_review_mutation_plan_build_request_parse_support.hpp"

#include <optional>
#include <utility>

namespace featherdoc_cli {
namespace {

[[nodiscard]] auto report_duplicate_operation_member(
    std::string_view member_name, std::string &error_message) -> bool {
    error_message =
        "JSON review mutation plan build request operation member '" +
        std::string(member_name) + "' must not be duplicated";
    return false;
}

[[nodiscard]] auto parse_operation_string(
    std::string_view content, std::size_t &index, std::string &target,
    std::string_view member_name, bool require_non_empty,
    std::string &error_message) -> bool {
    std::string parsed_value;
    if (!parse_json_patch_string(content, index, parsed_value, error_message)) {
        return false;
    }
    if (require_non_empty && parsed_value.empty()) {
        error_message =
            "JSON review mutation plan build request operation member '" +
            std::string(member_name) + "' must not be empty";
        return false;
    }

    target = std::move(parsed_value);
    return true;
}

[[nodiscard]] auto parse_operation_optional_string(
    std::string_view content, std::size_t &index,
    std::optional<std::string> &target, std::string_view member_name,
    bool require_non_empty, std::string &error_message) -> bool {
    std::string parsed_value;
    if (!parse_operation_string(content, index, parsed_value, member_name,
                                require_non_empty, error_message)) {
        return false;
    }

    target = std::move(parsed_value);
    return true;
}

} // namespace

auto parse_review_mutation_plan_build_request_operation_member(
    std::string_view content, std::size_t &index, std::string_view member_name,
    review_mutation_plan_build_request_operation &operation,
    review_mutation_plan_build_request_operation_members &members,
    std::string &error_message) -> bool {
    if (member_name == "kind") {
        if (members.saw_kind) {
            return report_duplicate_operation_member(member_name, error_message);
        }
        members.saw_kind = true;
        return parse_review_mutation_plan_operation_kind(
            content, index, operation.kind, error_message);
    }
    if (member_name == "find_text") {
        if (members.saw_find_text) {
            return report_duplicate_operation_member(member_name, error_message);
        }
        members.saw_find_text = true;
        return parse_operation_string(content, index, operation.find_text,
                                      member_name, true, error_message);
    }
    if (member_name == "occurrence") {
        if (members.saw_occurrence) {
            return report_duplicate_operation_member(member_name, error_message);
        }
        members.saw_occurrence = true;
        return parse_json_patch_index_value(content, index, operation.occurrence,
                                            "occurrence", error_message);
    }
    if (member_name == "before_text") {
        if (members.saw_before_text) {
            return report_duplicate_operation_member(member_name, error_message);
        }
        members.saw_before_text = true;
        return parse_operation_optional_string(content, index,
                                               operation.before_text,
                                               member_name, true,
                                               error_message);
    }
    if (member_name == "after_text") {
        if (members.saw_after_text) {
            return report_duplicate_operation_member(member_name, error_message);
        }
        members.saw_after_text = true;
        return parse_operation_optional_string(content, index,
                                               operation.after_text,
                                               member_name, true,
                                               error_message);
    }
    if (member_name == "require_unique") {
        if (members.saw_require_unique) {
            return report_duplicate_operation_member(member_name, error_message);
        }
        members.saw_require_unique = true;
        return parse_review_mutation_plan_build_request_bool_value(
            content, index, operation.require_unique, member_name,
            error_message);
    }
    if (member_name == "insert_after_match") {
        if (members.saw_insert_after_match) {
            return report_duplicate_operation_member(member_name, error_message);
        }
        members.saw_insert_after_match = true;
        return parse_review_mutation_plan_build_request_bool_value(
            content, index, operation.insert_after_match, member_name,
            error_message);
    }
    if (member_name == "text") {
        if (members.saw_text) {
            return report_duplicate_operation_member(member_name, error_message);
        }
        members.saw_text = true;
        return parse_operation_string(content, index, operation.text,
                                      member_name, false, error_message);
    }
    if (member_name == "comment_text") {
        if (members.saw_comment_text) {
            return report_duplicate_operation_member(member_name, error_message);
        }
        members.saw_comment_text = true;
        return parse_operation_string(content, index, operation.text,
                                      member_name, false, error_message);
    }
    if (member_name == "author") {
        if (members.saw_author) {
            return report_duplicate_operation_member(member_name, error_message);
        }
        members.saw_author = true;
        return parse_operation_optional_string(content, index, operation.author,
                                               member_name, false,
                                               error_message);
    }
    if (member_name == "initials") {
        if (members.saw_initials) {
            return report_duplicate_operation_member(member_name, error_message);
        }
        members.saw_initials = true;
        return parse_operation_optional_string(content, index, operation.initials,
                                               member_name, false,
                                               error_message);
    }
    if (member_name == "date") {
        if (members.saw_date) {
            return report_duplicate_operation_member(member_name, error_message);
        }
        members.saw_date = true;
        return parse_operation_optional_string(content, index, operation.date,
                                               member_name, false,
                                               error_message);
    }

    return skip_json_patch_value(content, index, error_message);
}

} // namespace featherdoc_cli
