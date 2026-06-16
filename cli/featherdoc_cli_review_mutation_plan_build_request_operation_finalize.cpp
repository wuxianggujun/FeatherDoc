#include "featherdoc_cli_review_mutation_plan_build_request_operation_parse_detail.hpp"

namespace featherdoc_cli {
namespace {

[[nodiscard]] auto operation_uses_direct_comment_index(
    review_mutation_plan_operation_kind kind) -> bool {
    return kind == review_mutation_plan_operation_kind::append_comment_reply ||
           kind == review_mutation_plan_operation_kind::replace_comment ||
           kind == review_mutation_plan_operation_kind::remove_comment ||
           kind == review_mutation_plan_operation_kind::set_comment_resolved ||
           kind == review_mutation_plan_operation_kind::set_comment_metadata;
}

[[nodiscard]] auto validate_comment_operation(
    const review_mutation_plan_build_request_operation &operation,
    const review_mutation_plan_build_request_operation_members &members,
    std::string &error_message) -> bool {
    if (members.saw_insert_after_match) {
        error_message =
            "JSON review mutation plan build request comment operation does not accept 'insert_after_match'";
        return false;
    }
    if (!members.saw_comment_text) {
        error_message =
            "JSON review mutation plan build request comment operation must contain 'comment_text'";
        return false;
    }
    if (operation.text.empty()) {
        error_message =
            "JSON review mutation plan build request comment text must not be empty";
        return false;
    }
    if (members.saw_text) {
        error_message =
            "JSON review mutation plan build request comment operation does not accept 'text'";
        return false;
    }
    return true;
}

[[nodiscard]] auto validate_insert_operation(
    const review_mutation_plan_build_request_operation &operation,
    const review_mutation_plan_build_request_operation_members &members,
    std::string &error_message) -> bool {
    if (members.saw_comment_text || members.saw_initials) {
        error_message =
            "JSON review mutation plan build request insert operation does not accept 'comment_text' or 'initials'";
        return false;
    }
    if (!members.saw_text) {
        error_message =
            "JSON review mutation plan build request insert operation must contain 'text'";
        return false;
    }
    if (operation.text.empty()) {
        error_message =
            "JSON review mutation plan build request insertion text must not be empty";
        return false;
    }
    return true;
}

[[nodiscard]] auto validate_delete_operation(
    const review_mutation_plan_build_request_operation_members &members,
    std::string &error_message) -> bool {
    if (members.saw_insert_after_match) {
        error_message =
            "JSON review mutation plan build request delete operation does not accept 'insert_after_match'";
        return false;
    }
    if (members.saw_text) {
        error_message =
            "JSON review mutation plan build request delete operation does not accept 'text'";
        return false;
    }
    if (members.saw_comment_text || members.saw_initials) {
        error_message =
            "JSON review mutation plan build request delete operation does not accept 'comment_text' or 'initials'";
        return false;
    }
    return true;
}

[[nodiscard]] auto validate_replace_operation(
    const review_mutation_plan_build_request_operation &operation,
    const review_mutation_plan_build_request_operation_members &members,
    std::string &error_message) -> bool {
    if (members.saw_insert_after_match) {
        error_message =
            "JSON review mutation plan build request replace operation does not accept 'insert_after_match'";
        return false;
    }
    if (members.saw_comment_text || members.saw_initials) {
        error_message =
            "JSON review mutation plan build request replace operation does not accept 'comment_text' or 'initials'";
        return false;
    }
    if (!members.saw_text) {
        error_message =
            "JSON review mutation plan build request replace operation must contain 'text'";
        return false;
    }
    if (operation.text.empty()) {
        error_message =
            "JSON review mutation plan build request replacement text must not be empty";
        return false;
    }
    return true;
}

} // namespace

auto validate_review_mutation_plan_build_request_operation(
    const review_mutation_plan_build_request_operation &operation,
    const review_mutation_plan_build_request_operation_members &members,
    std::string &error_message) -> bool {
    if (!members.saw_kind) {
        error_message =
            "JSON review mutation plan build request operation must contain 'kind'";
        return false;
    }

    if (operation_uses_direct_comment_index(operation.kind)) {
        error_message =
            "JSON review mutation plan build request does not support direct comment-index operations; use a direct review mutation plan operation";
        return false;
    }

    if (!members.saw_find_text) {
        error_message =
            "JSON review mutation plan build request operation must contain 'kind' and 'find_text'";
        return false;
    }

    switch (operation.kind) {
    case review_mutation_plan_operation_kind::append_paragraph_text_comment:
    case review_mutation_plan_operation_kind::append_text_range_comment:
        return validate_comment_operation(operation, members, error_message);
    case review_mutation_plan_operation_kind::insert_paragraph_text_revision:
    case review_mutation_plan_operation_kind::insert_text_range_revision:
        return validate_insert_operation(operation, members, error_message);
    case review_mutation_plan_operation_kind::delete_paragraph_text_revision:
    case review_mutation_plan_operation_kind::delete_text_range_revision:
        return validate_delete_operation(members, error_message);
    case review_mutation_plan_operation_kind::replace_paragraph_text_revision:
    case review_mutation_plan_operation_kind::replace_text_range_revision:
        return validate_replace_operation(operation, members, error_message);
    case review_mutation_plan_operation_kind::append_comment_reply:
    case review_mutation_plan_operation_kind::replace_comment:
    case review_mutation_plan_operation_kind::remove_comment:
    case review_mutation_plan_operation_kind::set_comment_resolved:
    case review_mutation_plan_operation_kind::set_comment_metadata:
        break;
    }

    return false;
}

} // namespace featherdoc_cli
