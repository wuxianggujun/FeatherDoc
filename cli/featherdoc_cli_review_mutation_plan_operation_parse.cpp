#include "featherdoc_cli_review_mutation_plan_operation_parse.hpp"

#include "featherdoc_cli_json_parse.hpp"
#include "featherdoc_cli_parse.hpp"

#include <cstddef>
#include <string>
#include <string_view>
#include <utility>

namespace featherdoc_cli {
namespace {

auto consume_review_mutation_plan_operation_separator(
    std::string_view content, std::size_t &index, bool &closed,
    std::string &error_message) -> bool {
    skip_json_patch_whitespace(content, index);
    if (index >= content.size()) {
        return report_json_input_error("review mutation plan file", index,
                                       "unexpected end of JSON", error_message);
    }
    if (content[index] == ',') {
        ++index;
        skip_json_patch_whitespace(content, index);
        closed = false;
        return true;
    }
    if (content[index] == '}') {
        ++index;
        closed = true;
        return true;
    }

    return report_json_input_error(
        "review mutation plan file", index,
        "expected ',' or '}' after operation object member", error_message);
}

auto parse_review_mutation_plan_bool_value(std::string_view content,
                                           std::size_t &index, bool &value,
                                           std::string_view member_name,
                                           std::string &error_message)
    -> bool {
    skip_json_patch_whitespace(content, index);
    if (content.substr(index, 4U) == "true") {
        value = true;
        index += 4U;
        return true;
    }
    if (content.substr(index, 5U) == "false") {
        value = false;
        index += 5U;
        return true;
    }
    if (index < content.size() && content[index] == '"') {
        std::string token;
        if (!parse_json_patch_string(content, index, token, error_message)) {
            return false;
        }
        if (parse_bool(token, value)) {
            return true;
        }
    }

    error_message = "JSON review mutation plan operation member '" +
                    std::string(member_name) + "' must be a boolean";
    return false;
}

} // namespace

auto parse_review_mutation_plan_operation(
    std::string_view content, std::size_t &index,
    review_mutation_plan_operation &operation, std::string &error_message)
    -> bool {
    skip_json_patch_whitespace(content, index);
    if (index >= content.size() || content[index] != '{') {
        return report_json_input_error("review mutation plan file", index,
                                       "expected operation object",
                                       error_message);
    }

    ++index;
    skip_json_patch_whitespace(content, index);
    if (index < content.size() && content[index] == '}') {
        return report_json_input_error("review mutation plan file", index,
                                       "operation object must not be empty",
                                       error_message);
    }

    bool saw_kind = false;
    bool saw_paragraph_index = false;
    bool saw_text_offset = false;
    bool saw_text_length = false;
    bool saw_start_paragraph_index = false;
    bool saw_start_text_offset = false;
    bool saw_end_paragraph_index = false;
    bool saw_end_text_offset = false;
    bool saw_comment_index = false;
    bool saw_resolved = false;
    bool saw_text = false;
    bool saw_comment_text = false;
    bool saw_expected_text = false;
    bool saw_expected_comment_text = false;
    bool saw_expected_resolved = false;
    bool saw_expected_parent_index = false;
    bool saw_author = false;
    bool saw_initials = false;
    bool saw_date = false;
    bool saw_clear_author = false;
    bool saw_clear_initials = false;
    bool saw_clear_date = false;

    while (index < content.size()) {
        std::string member_name;
        if (!parse_json_patch_string(content, index, member_name,
                                     error_message)) {
            return false;
        }
        skip_json_patch_whitespace(content, index);
        if (index >= content.size() || content[index] != ':') {
            return report_json_input_error(
                "review mutation plan file", index,
                "expected ':' after operation object member", error_message);
        }
        ++index;
        skip_json_patch_whitespace(content, index);

        if (member_name == "kind") {
            if (saw_kind) {
                error_message =
                    "JSON review mutation plan operation member 'kind' must not be duplicated";
                return false;
            }
            saw_kind = true;
            if (!parse_review_mutation_plan_operation_kind(
                    content, index, operation.kind, error_message)) {
                return false;
            }
        } else if (member_name == "paragraph_index") {
            if (saw_paragraph_index) {
                error_message =
                    "JSON review mutation plan operation member 'paragraph_index' must not be duplicated";
                return false;
            }
            saw_paragraph_index = true;
            if (!parse_json_patch_index_value(content, index,
                                              operation.paragraph_index,
                                              "paragraph_index",
                                              error_message)) {
                return false;
            }
        } else if (member_name == "text_offset") {
            if (saw_text_offset) {
                error_message =
                    "JSON review mutation plan operation member 'text_offset' must not be duplicated";
                return false;
            }
            saw_text_offset = true;
            if (!parse_json_patch_index_value(content, index,
                                              operation.text_offset,
                                              "text_offset",
                                              error_message)) {
                return false;
            }
        } else if (member_name == "text_length") {
            if (saw_text_length) {
                error_message =
                    "JSON review mutation plan operation member 'text_length' must not be duplicated";
                return false;
            }
            saw_text_length = true;
            if (!parse_json_patch_index_value(content, index,
                                              operation.text_length,
                                              "text_length",
                                              error_message)) {
                return false;
            }
        } else if (member_name == "start_paragraph_index") {
            if (saw_start_paragraph_index) {
                error_message =
                    "JSON review mutation plan operation member 'start_paragraph_index' must not be duplicated";
                return false;
            }
            saw_start_paragraph_index = true;
            if (!parse_json_patch_index_value(content, index,
                                              operation.start_paragraph_index,
                                              "start_paragraph_index",
                                              error_message)) {
                return false;
            }
        } else if (member_name == "start_text_offset") {
            if (saw_start_text_offset) {
                error_message =
                    "JSON review mutation plan operation member 'start_text_offset' must not be duplicated";
                return false;
            }
            saw_start_text_offset = true;
            if (!parse_json_patch_index_value(content, index,
                                              operation.start_text_offset,
                                              "start_text_offset",
                                              error_message)) {
                return false;
            }
        } else if (member_name == "end_paragraph_index") {
            if (saw_end_paragraph_index) {
                error_message =
                    "JSON review mutation plan operation member 'end_paragraph_index' must not be duplicated";
                return false;
            }
            saw_end_paragraph_index = true;
            if (!parse_json_patch_index_value(content, index,
                                              operation.end_paragraph_index,
                                              "end_paragraph_index",
                                              error_message)) {
                return false;
            }
        } else if (member_name == "end_text_offset") {
            if (saw_end_text_offset) {
                error_message =
                    "JSON review mutation plan operation member 'end_text_offset' must not be duplicated";
                return false;
            }
            saw_end_text_offset = true;
            if (!parse_json_patch_index_value(content, index,
                                              operation.end_text_offset,
                                              "end_text_offset",
                                              error_message)) {
                return false;
            }
        } else if (member_name == "comment_index") {
            if (saw_comment_index) {
                error_message =
                    "JSON review mutation plan operation member 'comment_index' must not be duplicated";
                return false;
            }
            saw_comment_index = true;
            if (!parse_json_patch_index_value(content, index,
                                              operation.comment_index,
                                              "comment_index",
                                              error_message)) {
                return false;
            }
        } else if (member_name == "resolved") {
            if (saw_resolved) {
                error_message =
                    "JSON review mutation plan operation member 'resolved' must not be duplicated";
                return false;
            }
            saw_resolved = true;
            if (!parse_review_mutation_plan_bool_value(
                    content, index, operation.resolved, "resolved",
                    error_message)) {
                return false;
            }
        } else if (member_name == "text") {
            if (saw_text) {
                error_message =
                    "JSON review mutation plan operation member 'text' must not be duplicated";
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
                    "JSON review mutation plan operation member 'comment_text' must not be duplicated";
                return false;
            }
            saw_comment_text = true;
            if (!parse_json_patch_string(content, index, operation.text,
                                         error_message)) {
                return false;
            }
        } else if (member_name == "expected_text") {
            if (saw_expected_text) {
                error_message =
                    "JSON review mutation plan operation member 'expected_text' must not be duplicated";
                return false;
            }
            saw_expected_text = true;
            std::string expected_text;
            if (!parse_json_patch_string(content, index, expected_text,
                                         error_message)) {
                return false;
            }
            operation.expected_text = std::move(expected_text);
        } else if (member_name == "expected_comment_text") {
            if (saw_expected_comment_text) {
                error_message =
                    "JSON review mutation plan operation member 'expected_comment_text' must not be duplicated";
                return false;
            }
            saw_expected_comment_text = true;
            std::string expected_comment_text;
            if (!parse_json_patch_string(content, index,
                                         expected_comment_text,
                                         error_message)) {
                return false;
            }
            operation.expected_comment_text =
                std::move(expected_comment_text);
        } else if (member_name == "expected_resolved") {
            if (saw_expected_resolved) {
                error_message =
                    "JSON review mutation plan operation member 'expected_resolved' must not be duplicated";
                return false;
            }
            saw_expected_resolved = true;
            bool expected_resolved = false;
            if (!parse_review_mutation_plan_bool_value(
                    content, index, expected_resolved, "expected_resolved",
                    error_message)) {
                return false;
            }
            operation.expected_resolved = expected_resolved;
        } else if (member_name == "expected_parent_index") {
            if (saw_expected_parent_index) {
                error_message =
                    "JSON review mutation plan operation member 'expected_parent_index' must not be duplicated";
                return false;
            }
            saw_expected_parent_index = true;
            std::size_t expected_parent_index = 0U;
            if (!parse_json_patch_index_value(
                    content, index, expected_parent_index,
                    "expected_parent_index", error_message)) {
                return false;
            }
            operation.expected_parent_index = expected_parent_index;
        } else if (member_name == "author") {
            if (saw_author) {
                error_message =
                    "JSON review mutation plan operation member 'author' must not be duplicated";
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
                    "JSON review mutation plan operation member 'initials' must not be duplicated";
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
                    "JSON review mutation plan operation member 'date' must not be duplicated";
                return false;
            }
            saw_date = true;
            std::string date;
            if (!parse_json_patch_string(content, index, date, error_message)) {
                return false;
            }
            operation.date = std::move(date);
        } else if (member_name == "clear_author") {
            if (saw_clear_author) {
                error_message =
                    "JSON review mutation plan operation member 'clear_author' must not be duplicated";
                return false;
            }
            saw_clear_author = true;
            if (!parse_review_mutation_plan_bool_value(
                    content, index, operation.clear_author, "clear_author",
                    error_message)) {
                return false;
            }
        } else if (member_name == "clear_initials") {
            if (saw_clear_initials) {
                error_message =
                    "JSON review mutation plan operation member 'clear_initials' must not be duplicated";
                return false;
            }
            saw_clear_initials = true;
            if (!parse_review_mutation_plan_bool_value(
                    content, index, operation.clear_initials,
                    "clear_initials", error_message)) {
                return false;
            }
        } else if (member_name == "clear_date") {
            if (saw_clear_date) {
                error_message =
                    "JSON review mutation plan operation member 'clear_date' must not be duplicated";
                return false;
            }
            saw_clear_date = true;
            if (!parse_review_mutation_plan_bool_value(
                    content, index, operation.clear_date, "clear_date",
                    error_message)) {
                return false;
            }
        } else {
            if (!skip_json_patch_value(content, index, error_message)) {
                return false;
            }
        }

        bool closed = false;
        if (!consume_review_mutation_plan_operation_separator(
                content, index, closed, error_message)) {
            return false;
        }
        if (closed) {
            break;
        }
    }

    if (!saw_kind) {
        error_message =
            "JSON review mutation plan operation must contain 'kind'";
        return false;
    }

    const auto paragraph_fields_ok =
        saw_paragraph_index && saw_text_offset && saw_text_length;
    const auto text_range_fields_ok =
        saw_start_paragraph_index && saw_start_text_offset &&
        saw_end_paragraph_index && saw_end_text_offset;
    const auto text_range_insertion_fields_ok =
        saw_start_paragraph_index && saw_start_text_offset;
    const auto clear_metadata_fields_seen =
        saw_clear_author || saw_clear_initials || saw_clear_date;
    const auto has_comment_metadata_update =
        saw_author || saw_initials || saw_date || operation.clear_author ||
        operation.clear_initials || operation.clear_date;

    switch (operation.kind) {
    case review_mutation_plan_operation_kind::append_paragraph_text_comment:
        if (!paragraph_fields_ok || !saw_comment_text) {
            error_message =
                "JSON review mutation plan append_paragraph_text_comment operation must contain 'paragraph_index', 'text_offset', 'text_length', and 'comment_text'";
            return false;
        }
        if (operation.text.empty()) {
            error_message =
                "JSON review mutation plan append_paragraph_text_comment operation member 'comment_text' must not be empty";
            return false;
        }
        if (saw_text) {
            error_message =
                "JSON review mutation plan append_paragraph_text_comment operation does not accept 'text'";
            return false;
        }
        if (saw_comment_index || saw_resolved || saw_expected_resolved ||
            saw_expected_comment_text || saw_expected_parent_index ||
            clear_metadata_fields_seen) {
            error_message =
                "JSON review mutation plan append_paragraph_text_comment operation does not accept 'comment_index', 'resolved', 'expected_resolved', 'expected_comment_text', 'expected_parent_index', or clear metadata fields";
            return false;
        }
        return true;
    case review_mutation_plan_operation_kind::append_comment_reply:
        if (!saw_comment_index || !saw_comment_text) {
            error_message =
                "JSON review mutation plan append_comment_reply operation must contain 'comment_index' and 'comment_text'";
            return false;
        }
        if (operation.text.empty()) {
            error_message =
                "JSON review mutation plan append_comment_reply operation member 'comment_text' must not be empty";
            return false;
        }
        if (saw_paragraph_index || saw_text_offset || saw_text_length ||
            saw_start_paragraph_index || saw_start_text_offset ||
            saw_end_paragraph_index || saw_end_text_offset || saw_text ||
            saw_resolved || clear_metadata_fields_seen) {
            error_message =
                "JSON review mutation plan append_comment_reply operation only accepts 'comment_index', 'comment_text', 'expected_text', 'expected_comment_text', 'expected_resolved', 'expected_parent_index', 'author', 'initials', and 'date'";
            return false;
        }
        return true;
    case review_mutation_plan_operation_kind::replace_comment:
        if (!saw_comment_index || !saw_comment_text) {
            error_message =
                "JSON review mutation plan replace_comment operation must contain 'comment_index' and 'comment_text'";
            return false;
        }
        if (operation.text.empty()) {
            error_message =
                "JSON review mutation plan replace_comment operation member 'comment_text' must not be empty";
            return false;
        }
        if (saw_paragraph_index || saw_text_offset || saw_text_length ||
            saw_start_paragraph_index || saw_start_text_offset ||
            saw_end_paragraph_index || saw_end_text_offset || saw_text ||
            saw_resolved || saw_author || saw_initials || saw_date ||
            clear_metadata_fields_seen) {
            error_message =
                "JSON review mutation plan replace_comment operation only accepts 'comment_index', 'comment_text', 'expected_text', 'expected_comment_text', 'expected_resolved', and 'expected_parent_index'";
            return false;
        }
        return true;
    case review_mutation_plan_operation_kind::remove_comment:
        if (!saw_comment_index) {
            error_message =
                "JSON review mutation plan remove_comment operation must contain 'comment_index'";
            return false;
        }
        if (saw_paragraph_index || saw_text_offset || saw_text_length ||
            saw_start_paragraph_index || saw_start_text_offset ||
            saw_end_paragraph_index || saw_end_text_offset || saw_text ||
            saw_comment_text || saw_resolved || saw_author || saw_initials ||
            saw_date || clear_metadata_fields_seen) {
            error_message =
                "JSON review mutation plan remove_comment operation only accepts 'comment_index', 'expected_text', 'expected_comment_text', 'expected_resolved', and 'expected_parent_index'";
            return false;
        }
        return true;
    case review_mutation_plan_operation_kind::set_comment_resolved:
        if (!saw_comment_index || !saw_resolved) {
            error_message =
                "JSON review mutation plan set_comment_resolved operation must contain 'comment_index' and 'resolved'";
            return false;
        }
        if (saw_paragraph_index || saw_text_offset || saw_text_length ||
            saw_start_paragraph_index || saw_start_text_offset ||
            saw_end_paragraph_index || saw_end_text_offset || saw_text ||
            saw_comment_text || saw_author || saw_initials || saw_date ||
            clear_metadata_fields_seen) {
            error_message =
                "JSON review mutation plan set_comment_resolved operation only accepts 'comment_index', 'resolved', 'expected_text', 'expected_comment_text', 'expected_resolved', and 'expected_parent_index'";
            return false;
        }
        return true;
    case review_mutation_plan_operation_kind::set_comment_metadata:
        if (!saw_comment_index) {
            error_message =
                "JSON review mutation plan set_comment_metadata operation must contain 'comment_index'";
            return false;
        }
        if (!has_comment_metadata_update) {
            error_message =
                "JSON review mutation plan set_comment_metadata operation must contain at least one metadata update field";
            return false;
        }
        if ((saw_author && operation.clear_author) ||
            (saw_initials && operation.clear_initials) ||
            (saw_date && operation.clear_date)) {
            error_message =
                "JSON review mutation plan set_comment_metadata operation cannot set and clear the same metadata field";
            return false;
        }
        if (saw_paragraph_index || saw_text_offset || saw_text_length ||
            saw_start_paragraph_index || saw_start_text_offset ||
            saw_end_paragraph_index || saw_end_text_offset || saw_text ||
            saw_comment_text || saw_resolved) {
            error_message =
                "JSON review mutation plan set_comment_metadata operation only accepts 'comment_index', metadata update fields, 'expected_text', 'expected_comment_text', 'expected_resolved', and 'expected_parent_index'";
            return false;
        }
        return true;
    case review_mutation_plan_operation_kind::insert_paragraph_text_revision:
        if (!saw_paragraph_index || !saw_text_offset || !saw_text) {
            error_message =
                "JSON review mutation plan insert_paragraph_text_revision operation must contain 'paragraph_index', 'text_offset', and 'text'";
            return false;
        }
        if (saw_text_length || saw_comment_text || saw_expected_text ||
            saw_expected_comment_text || saw_expected_resolved ||
            saw_expected_parent_index || saw_comment_index || saw_resolved ||
            saw_initials || clear_metadata_fields_seen) {
            error_message =
                "JSON review mutation plan insert_paragraph_text_revision operation does not accept comment, expected comment, resolved, initials, or clear metadata fields";
            return false;
        }
        return true;
    case review_mutation_plan_operation_kind::delete_paragraph_text_revision:
        if (!paragraph_fields_ok) {
            error_message =
                "JSON review mutation plan delete_paragraph_text_revision operation must contain 'paragraph_index', 'text_offset', and 'text_length'";
            return false;
        }
        if (saw_text || saw_comment_text || saw_expected_comment_text ||
            saw_expected_resolved || saw_expected_parent_index ||
            saw_comment_index || saw_resolved || saw_initials ||
            clear_metadata_fields_seen) {
            error_message =
                "JSON review mutation plan delete_paragraph_text_revision operation does not accept text, comment, expected comment, resolved, initials, or clear metadata fields";
            return false;
        }
        return true;
    case review_mutation_plan_operation_kind::replace_paragraph_text_revision:
        if (!paragraph_fields_ok || !saw_text) {
            error_message =
                "JSON review mutation plan replace_paragraph_text_revision operation must contain 'paragraph_index', 'text_offset', 'text_length', and 'text'";
            return false;
        }
        if (saw_comment_text || saw_expected_comment_text ||
            saw_expected_resolved || saw_expected_parent_index ||
            saw_comment_index || saw_resolved || saw_initials ||
            clear_metadata_fields_seen) {
            error_message =
                "JSON review mutation plan replace_paragraph_text_revision operation does not accept comment, expected comment, resolved, initials, or clear metadata fields";
            return false;
        }
        return true;
    case review_mutation_plan_operation_kind::append_text_range_comment:
        if (!text_range_fields_ok || !saw_comment_text) {
            error_message =
                "JSON review mutation plan append_text_range_comment operation must contain 'start_paragraph_index', 'start_text_offset', 'end_paragraph_index', 'end_text_offset', and 'comment_text'";
            return false;
        }
        if (operation.text.empty()) {
            error_message =
                "JSON review mutation plan append_text_range_comment operation member 'comment_text' must not be empty";
            return false;
        }
        if (saw_text) {
            error_message =
                "JSON review mutation plan append_text_range_comment operation does not accept 'text'";
            return false;
        }
        if (saw_comment_index || saw_resolved || saw_expected_resolved ||
            saw_expected_comment_text || saw_expected_parent_index ||
            clear_metadata_fields_seen) {
            error_message =
                "JSON review mutation plan append_text_range_comment operation does not accept 'comment_index', 'resolved', 'expected_resolved', 'expected_comment_text', 'expected_parent_index', or clear metadata fields";
            return false;
        }
        return true;
    case review_mutation_plan_operation_kind::insert_text_range_revision:
        if (!text_range_insertion_fields_ok || !saw_text) {
            error_message =
                "JSON review mutation plan insert_text_range_revision operation must contain 'start_paragraph_index', 'start_text_offset', and 'text'";
            return false;
        }
        if (saw_end_paragraph_index || saw_end_text_offset ||
            saw_comment_text || saw_expected_text ||
            saw_expected_comment_text || saw_expected_resolved ||
            saw_expected_parent_index || saw_comment_index || saw_resolved ||
            saw_initials || clear_metadata_fields_seen) {
            error_message =
                "JSON review mutation plan insert_text_range_revision operation does not accept range end, comment, expected text/comment, resolved, initials, or clear metadata fields";
            return false;
        }
        return true;
    case review_mutation_plan_operation_kind::delete_text_range_revision:
        if (!text_range_fields_ok) {
            error_message =
                "JSON review mutation plan delete_text_range_revision operation must contain 'start_paragraph_index', 'start_text_offset', 'end_paragraph_index', and 'end_text_offset'";
            return false;
        }
        if (saw_text || saw_comment_text || saw_expected_comment_text ||
            saw_expected_resolved || saw_expected_parent_index ||
            saw_comment_index || saw_resolved || saw_initials ||
            clear_metadata_fields_seen) {
            error_message =
                "JSON review mutation plan delete_text_range_revision operation does not accept text, comment, expected comment, resolved, initials, or clear metadata fields";
            return false;
        }
        return true;
    case review_mutation_plan_operation_kind::replace_text_range_revision:
        if (!text_range_fields_ok || !saw_text) {
            error_message =
                "JSON review mutation plan replace_text_range_revision operation must contain 'start_paragraph_index', 'start_text_offset', 'end_paragraph_index', 'end_text_offset', and 'text'";
            return false;
        }
        if (saw_comment_text || saw_expected_comment_text ||
            saw_expected_resolved || saw_expected_parent_index ||
            saw_comment_index || saw_resolved || saw_initials ||
            clear_metadata_fields_seen) {
            error_message =
                "JSON review mutation plan replace_text_range_revision operation does not accept comment, expected comment, resolved, initials, or clear metadata fields";
            return false;
        }
        return true;
    }

    return false;
}
} // namespace featherdoc_cli
