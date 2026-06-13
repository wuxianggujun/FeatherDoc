#include "featherdoc_cli_review_mutation_plan_parse.hpp"

#include "featherdoc_cli_json_parse.hpp"

#include <string>
#include <string_view>

namespace featherdoc_cli {

auto review_mutation_plan_operation_kind_name(
    review_mutation_plan_operation_kind kind) -> std::string_view {
    switch (kind) {
    case review_mutation_plan_operation_kind::append_paragraph_text_comment:
        return "append_paragraph_text_comment";
    case review_mutation_plan_operation_kind::append_comment_reply:
        return "append_comment_reply";
    case review_mutation_plan_operation_kind::replace_comment:
        return "replace_comment";
    case review_mutation_plan_operation_kind::remove_comment:
        return "remove_comment";
    case review_mutation_plan_operation_kind::set_comment_resolved:
        return "set_comment_resolved";
    case review_mutation_plan_operation_kind::set_comment_metadata:
        return "set_comment_metadata";
    case review_mutation_plan_operation_kind::insert_paragraph_text_revision:
        return "insert_paragraph_text_revision";
    case review_mutation_plan_operation_kind::delete_paragraph_text_revision:
        return "delete_paragraph_text_revision";
    case review_mutation_plan_operation_kind::replace_paragraph_text_revision:
        return "replace_paragraph_text_revision";
    case review_mutation_plan_operation_kind::append_text_range_comment:
        return "append_text_range_comment";
    case review_mutation_plan_operation_kind::insert_text_range_revision:
        return "insert_text_range_revision";
    case review_mutation_plan_operation_kind::delete_text_range_revision:
        return "delete_text_range_revision";
    case review_mutation_plan_operation_kind::replace_text_range_revision:
        return "replace_text_range_revision";
    }

    return "delete_paragraph_text_revision";
}

auto parse_review_mutation_plan_operation_kind(
    std::string_view content, std::size_t &index,
    review_mutation_plan_operation_kind &kind, std::string &error_message)
    -> bool {
    std::string token;
    if (!parse_json_patch_string(content, index, token, error_message)) {
        return false;
    }

    if (token == "append_paragraph_text_comment" ||
        token == "append-paragraph-text-comment") {
        kind =
            review_mutation_plan_operation_kind::append_paragraph_text_comment;
        return true;
    }
    if (token == "append_comment_reply" ||
        token == "append-comment-reply") {
        kind = review_mutation_plan_operation_kind::append_comment_reply;
        return true;
    }
    if (token == "replace_comment" || token == "replace-comment") {
        kind = review_mutation_plan_operation_kind::replace_comment;
        return true;
    }
    if (token == "remove_comment" || token == "remove-comment") {
        kind = review_mutation_plan_operation_kind::remove_comment;
        return true;
    }
    if (token == "set_comment_resolved" ||
        token == "set-comment-resolved") {
        kind = review_mutation_plan_operation_kind::set_comment_resolved;
        return true;
    }
    if (token == "set_comment_metadata" ||
        token == "set-comment-metadata") {
        kind = review_mutation_plan_operation_kind::set_comment_metadata;
        return true;
    }
    if (token == "insert_paragraph_text_revision" ||
        token == "insert-paragraph-text-revision") {
        kind =
            review_mutation_plan_operation_kind::insert_paragraph_text_revision;
        return true;
    }
    if (token == "delete_paragraph_text_revision" ||
        token == "delete-paragraph-text-revision") {
        kind =
            review_mutation_plan_operation_kind::delete_paragraph_text_revision;
        return true;
    }
    if (token == "replace_paragraph_text_revision" ||
        token == "replace-paragraph-text-revision") {
        kind =
            review_mutation_plan_operation_kind::replace_paragraph_text_revision;
        return true;
    }
    if (token == "append_text_range_comment" ||
        token == "append-text-range-comment") {
        kind = review_mutation_plan_operation_kind::append_text_range_comment;
        return true;
    }
    if (token == "insert_text_range_revision" ||
        token == "insert-text-range-revision") {
        kind = review_mutation_plan_operation_kind::insert_text_range_revision;
        return true;
    }
    if (token == "delete_text_range_revision" ||
        token == "delete-text-range-revision") {
        kind = review_mutation_plan_operation_kind::delete_text_range_revision;
        return true;
    }
    if (token == "replace_text_range_revision" ||
        token == "replace-text-range-revision") {
        kind = review_mutation_plan_operation_kind::replace_text_range_revision;
        return true;
    }

    return report_json_input_error(
        "review mutation plan file", index,
        "operation kind must be one of "
        "'append_paragraph_text_comment', "
        "'append_comment_reply', "
        "'replace_comment', "
        "'remove_comment', "
        "'set_comment_resolved', "
        "'set_comment_metadata', "
        "'insert_paragraph_text_revision', "
        "'delete_paragraph_text_revision', "
        "'replace_paragraph_text_revision', "
        "'append_text_range_comment', "
        "'insert_text_range_revision', "
        "'delete_text_range_revision', or "
        "'replace_text_range_revision' (CLI hyphenated names are also accepted)",
        error_message);
}

} // namespace featherdoc_cli
