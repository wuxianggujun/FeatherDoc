#include "featherdoc_cli_review_mutation_plan_build_request_operation_parse.hpp"

#include "featherdoc_cli_json_parse.hpp"
#include "featherdoc_cli_review_mutation_plan_build_request_parse_support.hpp"

#include <string>
#include <utility>

namespace featherdoc_cli {
namespace {

auto parse_review_mutation_plan_build_request_operation(
    std::string_view content, std::size_t &index,
    review_mutation_plan_build_request_operation &operation,
    std::string &error_message) -> bool {
    skip_json_patch_whitespace(content, index);
    if (index >= content.size() || content[index] != '{') {
        return report_json_input_error(
            "review mutation plan build request file", index,
            "expected operation object", error_message);
    }

    ++index;
    skip_json_patch_whitespace(content, index);
    if (index < content.size() && content[index] == '}') {
        return report_json_input_error(
            "review mutation plan build request file", index,
            "operation object must not be empty", error_message);
    }

    bool saw_kind = false;
    bool saw_find_text = false;
    bool saw_occurrence = false;
    bool saw_before_text = false;
    bool saw_after_text = false;
    bool saw_require_unique = false;
    bool saw_insert_after_match = false;
    bool saw_text = false;
    bool saw_comment_text = false;
    bool saw_author = false;
    bool saw_initials = false;
    bool saw_date = false;

    while (index < content.size()) {
        std::string member_name;
        if (!parse_json_patch_string(content, index, member_name,
                                     error_message)) {
            return false;
        }
        skip_json_patch_whitespace(content, index);
        if (index >= content.size() || content[index] != ':') {
            return report_json_input_error(
                "review mutation plan build request file", index,
                "expected ':' after operation object member", error_message);
        }
        ++index;
        skip_json_patch_whitespace(content, index);

        if (member_name == "kind") {
            if (saw_kind) {
                error_message =
                    "JSON review mutation plan build request operation member 'kind' must not be duplicated";
                return false;
            }
            saw_kind = true;
            if (!parse_review_mutation_plan_operation_kind(
                    content, index, operation.kind, error_message)) {
                return false;
            }
        } else if (member_name == "find_text") {
            if (saw_find_text) {
                error_message =
                    "JSON review mutation plan build request operation member 'find_text' must not be duplicated";
                return false;
            }
            saw_find_text = true;
            if (!parse_json_patch_string(content, index, operation.find_text,
                                         error_message)) {
                return false;
            }
            if (operation.find_text.empty()) {
                error_message =
                    "JSON review mutation plan build request operation member 'find_text' must not be empty";
                return false;
            }
        } else if (member_name == "occurrence") {
            if (saw_occurrence) {
                error_message =
                    "JSON review mutation plan build request operation member 'occurrence' must not be duplicated";
                return false;
            }
            saw_occurrence = true;
            if (!parse_json_patch_index_value(content, index,
                                              operation.occurrence,
                                              "occurrence", error_message)) {
                return false;
            }
        } else if (member_name == "before_text") {
            if (saw_before_text) {
                error_message =
                    "JSON review mutation plan build request operation member 'before_text' must not be duplicated";
                return false;
            }
            saw_before_text = true;
            std::string before_text;
            if (!parse_json_patch_string(content, index, before_text,
                                         error_message)) {
                return false;
            }
            if (before_text.empty()) {
                error_message =
                    "JSON review mutation plan build request operation member 'before_text' must not be empty";
                return false;
            }
            operation.before_text = std::move(before_text);
        } else if (member_name == "after_text") {
            if (saw_after_text) {
                error_message =
                    "JSON review mutation plan build request operation member 'after_text' must not be duplicated";
                return false;
            }
            saw_after_text = true;
            std::string after_text;
            if (!parse_json_patch_string(content, index, after_text,
                                         error_message)) {
                return false;
            }
            if (after_text.empty()) {
                error_message =
                    "JSON review mutation plan build request operation member 'after_text' must not be empty";
                return false;
            }
            operation.after_text = std::move(after_text);
        } else if (member_name == "require_unique") {
            if (saw_require_unique) {
                error_message =
                    "JSON review mutation plan build request operation member 'require_unique' must not be duplicated";
                return false;
            }
            saw_require_unique = true;
            if (!parse_review_mutation_plan_build_request_bool_value(
                    content, index, operation.require_unique, member_name,
                    error_message)) {
                return false;
            }
        } else if (member_name == "insert_after_match") {
            if (saw_insert_after_match) {
                error_message =
                    "JSON review mutation plan build request operation member 'insert_after_match' must not be duplicated";
                return false;
            }
            saw_insert_after_match = true;
            if (!parse_review_mutation_plan_build_request_bool_value(
                    content, index, operation.insert_after_match, member_name,
                    error_message)) {
                return false;
            }
        } else if (member_name == "text") {
            if (saw_text) {
                error_message =
                    "JSON review mutation plan build request operation member 'text' must not be duplicated";
                return false;
            }
            saw_text = true;
            if (!parse_json_patch_string(content, index, operation.text,
                                         error_message)) {
                return false;
            }
        } else if (member_name == "comment_text") {
            if (saw_comment_text) {
                error_message =
                    "JSON review mutation plan build request operation member 'comment_text' must not be duplicated";
                return false;
            }
            saw_comment_text = true;
            if (!parse_json_patch_string(content, index, operation.text,
                                         error_message)) {
                return false;
            }
        } else if (member_name == "author") {
            if (saw_author) {
                error_message =
                    "JSON review mutation plan build request operation member 'author' must not be duplicated";
                return false;
            }
            saw_author = true;
            std::string author;
            if (!parse_json_patch_string(content, index, author,
                                         error_message)) {
                return false;
            }
            operation.author = std::move(author);
        } else if (member_name == "initials") {
            if (saw_initials) {
                error_message =
                    "JSON review mutation plan build request operation member 'initials' must not be duplicated";
                return false;
            }
            saw_initials = true;
            std::string initials;
            if (!parse_json_patch_string(content, index, initials,
                                         error_message)) {
                return false;
            }
            operation.initials = std::move(initials);
        } else if (member_name == "date") {
            if (saw_date) {
                error_message =
                    "JSON review mutation plan build request operation member 'date' must not be duplicated";
                return false;
            }
            saw_date = true;
            std::string date;
            if (!parse_json_patch_string(content, index, date, error_message)) {
                return false;
            }
            operation.date = std::move(date);
        } else {
            if (!skip_json_patch_value(content, index, error_message)) {
                return false;
            }
        }

        bool closed = false;
        if (!consume_review_mutation_plan_build_request_separator(
                content, index, '}',
                "expected ',' or '}' after operation object member", closed,
                error_message)) {
            return false;
        }
        if (closed) {
            break;
        }
    }

    if (!saw_kind) {
        error_message =
            "JSON review mutation plan build request operation must contain 'kind'";
        return false;
    }

    if (operation.kind ==
            review_mutation_plan_operation_kind::append_comment_reply ||
        operation.kind ==
            review_mutation_plan_operation_kind::replace_comment ||
        operation.kind ==
            review_mutation_plan_operation_kind::remove_comment ||
        operation.kind ==
            review_mutation_plan_operation_kind::set_comment_resolved ||
        operation.kind ==
            review_mutation_plan_operation_kind::set_comment_metadata) {
        error_message =
            "JSON review mutation plan build request does not support direct comment-index operations; use a direct review mutation plan operation";
        return false;
    }

    if (!saw_find_text) {
        error_message =
            "JSON review mutation plan build request operation must contain 'kind' and 'find_text'";
        return false;
    }

    switch (operation.kind) {
    case review_mutation_plan_operation_kind::append_paragraph_text_comment:
    case review_mutation_plan_operation_kind::append_text_range_comment:
        if (saw_insert_after_match) {
            error_message =
                "JSON review mutation plan build request comment operation does not accept 'insert_after_match'";
            return false;
        }
        if (!saw_comment_text) {
            error_message =
                "JSON review mutation plan build request comment operation must contain 'comment_text'";
            return false;
        }
        if (operation.text.empty()) {
            error_message =
                "JSON review mutation plan build request comment text must not be empty";
            return false;
        }
        if (saw_text) {
            error_message =
                "JSON review mutation plan build request comment operation does not accept 'text'";
            return false;
        }
        return true;
    case review_mutation_plan_operation_kind::insert_paragraph_text_revision:
    case review_mutation_plan_operation_kind::insert_text_range_revision:
        if (saw_comment_text || saw_initials) {
            error_message =
                "JSON review mutation plan build request insert operation does not accept 'comment_text' or 'initials'";
            return false;
        }
        if (!saw_text) {
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
    case review_mutation_plan_operation_kind::delete_paragraph_text_revision:
    case review_mutation_plan_operation_kind::delete_text_range_revision:
        if (saw_insert_after_match) {
            error_message =
                "JSON review mutation plan build request delete operation does not accept 'insert_after_match'";
            return false;
        }
        if (saw_text) {
            error_message =
                "JSON review mutation plan build request delete operation does not accept 'text'";
            return false;
        }
        if (saw_comment_text || saw_initials) {
            error_message =
                "JSON review mutation plan build request delete operation does not accept 'comment_text' or 'initials'";
            return false;
        }
        return true;
    case review_mutation_plan_operation_kind::replace_paragraph_text_revision:
    case review_mutation_plan_operation_kind::replace_text_range_revision:
        if (saw_insert_after_match) {
            error_message =
                "JSON review mutation plan build request replace operation does not accept 'insert_after_match'";
            return false;
        }
        if (saw_comment_text || saw_initials) {
            error_message =
                "JSON review mutation plan build request replace operation does not accept 'comment_text' or 'initials'";
            return false;
        }
        if (!saw_text) {
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

    return false;
}

} // namespace

auto parse_review_mutation_plan_build_request_operations(
    std::string_view content, std::size_t &index,
    std::vector<review_mutation_plan_build_request_operation> &operations,
    std::string &error_message) -> bool {
    skip_json_patch_whitespace(content, index);
    if (index >= content.size() || content[index] != '[') {
        return report_json_input_error(
            "review mutation plan build request file", index,
            "expected operations array", error_message);
    }

    operations.clear();
    ++index;
    skip_json_patch_whitespace(content, index);
    if (index < content.size() && content[index] == ']') {
        ++index;
        return true;
    }

    while (index < content.size()) {
        review_mutation_plan_build_request_operation operation;
        if (!parse_review_mutation_plan_build_request_operation(
                content, index, operation, error_message)) {
            return false;
        }
        operations.push_back(std::move(operation));

        bool closed = false;
        if (!consume_review_mutation_plan_build_request_separator(
                content, index, ']',
                "expected ',' or ']' after operations array item", closed,
                error_message)) {
            return false;
        }
        if (closed) {
            return true;
        }
    }

    return report_json_input_error("review mutation plan build request file",
                                   index, "unterminated operations array",
                                   error_message);
}

} // namespace featherdoc_cli
