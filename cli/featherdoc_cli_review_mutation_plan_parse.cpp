#include "featherdoc_cli_review_mutation_plan_parse.hpp"

#include "featherdoc_cli_json.hpp"
#include "featherdoc_cli_json_parse.hpp"
#include "featherdoc_cli_parse.hpp"

#include <filesystem>
#include <fstream>
#include <iterator>
#include <ostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace featherdoc_cli {

using path_type = std::filesystem::path;

auto write_json_bool(bool value) noexcept -> const char * {
    return value ? "true" : "false";
}

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

auto read_review_mutation_plan_content(const path_type &plan_path,
                                       std::string &content,
                                       std::string &error_message) -> bool {
    std::ifstream stream(plan_path, std::ios::binary);
    if (!stream.good()) {
        error_message = "failed to read review mutation plan file: " +
                        plan_path.string();
        return false;
    }

    content.assign(std::istreambuf_iterator<char>(stream),
                   std::istreambuf_iterator<char>());
    return true;
}

auto consume_review_mutation_plan_separator(std::string_view content,
                                            std::size_t &index,
                                            char close_char,
                                            std::string_view error_detail,
                                            bool &closed,
                                            std::string &error_message)
    -> bool {
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
    if (content[index] == close_char) {
        ++index;
        closed = true;
        return true;
    }

    return report_json_input_error("review mutation plan file", index,
                                   error_detail, error_message);
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
        if (!consume_review_mutation_plan_separator(
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

auto parse_review_mutation_plan_operations(
    std::string_view content, std::size_t &index,
    std::vector<review_mutation_plan_operation> &operations,
    std::string &error_message) -> bool {
    skip_json_patch_whitespace(content, index);
    if (index >= content.size() || content[index] != '[') {
        return report_json_input_error("review mutation plan file", index,
                                       "expected operations array",
                                       error_message);
    }

    operations.clear();
    ++index;
    skip_json_patch_whitespace(content, index);
    if (index < content.size() && content[index] == ']') {
        ++index;
        return true;
    }

    while (index < content.size()) {
        review_mutation_plan_operation operation;
        if (!parse_review_mutation_plan_operation(content, index, operation,
                                                  error_message)) {
            return false;
        }
        operations.push_back(std::move(operation));

        bool closed = false;
        if (!consume_review_mutation_plan_separator(
                content, index, ']',
                "expected ',' or ']' after operations array item", closed,
                error_message)) {
            return false;
        }
        if (closed) {
            return true;
        }
    }

    return report_json_input_error("review mutation plan file", index,
                                   "unterminated operations array",
                                   error_message);
}

auto read_review_mutation_plan_file(
    const path_type &plan_path,
    std::vector<review_mutation_plan_operation> &operations,
    std::string &error_message) -> bool {
    std::string content;
    if (!read_review_mutation_plan_content(plan_path, content, error_message)) {
        return false;
    }

    std::size_t index = 0U;
    skip_json_patch_whitespace(content, index);
    if (index >= content.size() || content[index] != '{') {
        return report_json_input_error("review mutation plan file", index,
                                       "expected root object", error_message);
    }

    ++index;
    skip_json_patch_whitespace(content, index);
    if (index < content.size() && content[index] == '}') {
        return report_json_input_error("review mutation plan file", index,
                                       "root object must not be empty",
                                       error_message);
    }

    bool saw_operations = false;
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
                "expected ':' after root object member", error_message);
        }
        ++index;
        skip_json_patch_whitespace(content, index);

        if (member_name == "operations") {
            if (saw_operations) {
                error_message =
                    "JSON review mutation plan root member 'operations' must not be duplicated";
                return false;
            }
            saw_operations = true;
            if (!parse_review_mutation_plan_operations(
                    content, index, operations, error_message)) {
                return false;
            }
        } else {
            if (!skip_json_patch_value(content, index, error_message)) {
                return false;
            }
        }

        bool closed = false;
        if (!consume_review_mutation_plan_separator(
                content, index, '}',
                "expected ',' or '}' after root object member", closed,
                error_message)) {
            return false;
        }
        if (closed) {
            break;
        }
    }

    skip_json_patch_whitespace(content, index);
    if (index != content.size()) {
        return report_json_input_error("review mutation plan file", index,
                                       "unexpected trailing content after root object",
                                       error_message);
    }

    if (!saw_operations) {
        error_message =
            "JSON review mutation plan file must contain an 'operations' array";
        return false;
    }
    if (operations.empty()) {
        error_message =
            "JSON review mutation plan file must contain at least one operation";
        return false;
    }

    return true;
}

auto read_review_mutation_plan_build_request_content(
    const path_type &request_path, std::string &content,
    std::string &error_message) -> bool {
    std::ifstream stream(request_path, std::ios::binary);
    if (!stream.good()) {
        error_message = "failed to read review mutation plan build request file: " +
                        request_path.string();
        return false;
    }

    content.assign(std::istreambuf_iterator<char>(stream),
                   std::istreambuf_iterator<char>());
    return true;
}

auto consume_review_mutation_plan_build_request_separator(
    std::string_view content, std::size_t &index, char close_char,
    std::string_view error_detail, bool &closed, std::string &error_message)
    -> bool {
    skip_json_patch_whitespace(content, index);
    if (index >= content.size()) {
        return report_json_input_error(
            "review mutation plan build request file", index,
            "unexpected end of JSON", error_message);
    }
    if (content[index] == ',') {
        ++index;
        skip_json_patch_whitespace(content, index);
        closed = false;
        return true;
    }
    if (content[index] == close_char) {
        ++index;
        closed = true;
        return true;
    }

    return report_json_input_error("review mutation plan build request file",
                                   index, error_detail, error_message);
}

auto parse_review_mutation_plan_build_request_bool_value(
    std::string_view content, std::size_t &index, bool &value,
    std::string_view member_name, std::string &error_message) -> bool {
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

    error_message =
        "JSON review mutation plan build request operation member '" +
        std::string(member_name) + "' must be a boolean";
    return false;
}

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

auto read_review_mutation_plan_build_request_file(
    const path_type &request_path,
    std::vector<review_mutation_plan_build_request_operation> &operations,
    std::string &error_message) -> bool {
    std::string content;
    if (!read_review_mutation_plan_build_request_content(
            request_path, content, error_message)) {
        return false;
    }

    std::size_t index = 0U;
    skip_json_patch_whitespace(content, index);
    if (index >= content.size() || content[index] != '{') {
        return report_json_input_error(
            "review mutation plan build request file", index,
            "expected root object", error_message);
    }

    ++index;
    skip_json_patch_whitespace(content, index);
    if (index < content.size() && content[index] == '}') {
        return report_json_input_error(
            "review mutation plan build request file", index,
            "root object must not be empty", error_message);
    }

    bool saw_operations = false;
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
                "expected ':' after root object member", error_message);
        }
        ++index;
        skip_json_patch_whitespace(content, index);

        if (member_name == "operations") {
            if (saw_operations) {
                error_message =
                    "JSON review mutation plan build request root member 'operations' must not be duplicated";
                return false;
            }
            saw_operations = true;
            if (!parse_review_mutation_plan_build_request_operations(
                    content, index, operations, error_message)) {
                return false;
            }
        } else {
            if (!skip_json_patch_value(content, index, error_message)) {
                return false;
            }
        }

        bool closed = false;
        if (!consume_review_mutation_plan_build_request_separator(
                content, index, '}',
                "expected ',' or '}' after root object member", closed,
                error_message)) {
            return false;
        }
        if (closed) {
            break;
        }
    }

    skip_json_patch_whitespace(content, index);
    if (index != content.size()) {
        return report_json_input_error(
            "review mutation plan build request file", index,
            "unexpected trailing content after root object", error_message);
    }

    if (!saw_operations) {
        error_message =
            "JSON review mutation plan build request file must contain an 'operations' array";
        return false;
    }
    if (operations.empty()) {
        error_message =
            "JSON review mutation plan build request file must contain at least one operation";
        return false;
    }

    return true;
}

void write_json_review_mutation_plan_operation(
    std::ostream &stream, const review_mutation_plan_operation &operation) {
    stream << "{\"kind\":";
    write_json_string(stream, review_mutation_plan_operation_kind_name(operation.kind));
    switch (operation.kind) {
    case review_mutation_plan_operation_kind::append_comment_reply:
    case review_mutation_plan_operation_kind::replace_comment:
    case review_mutation_plan_operation_kind::remove_comment:
        stream << ",\"comment_index\":" << operation.comment_index;
        break;
    case review_mutation_plan_operation_kind::set_comment_resolved:
        stream << ",\"comment_index\":" << operation.comment_index
               << ",\"resolved\":" << write_json_bool(operation.resolved);
        break;
    case review_mutation_plan_operation_kind::set_comment_metadata:
        stream << ",\"comment_index\":" << operation.comment_index;
        break;
    case review_mutation_plan_operation_kind::append_paragraph_text_comment:
    case review_mutation_plan_operation_kind::insert_paragraph_text_revision:
    case review_mutation_plan_operation_kind::delete_paragraph_text_revision:
    case review_mutation_plan_operation_kind::replace_paragraph_text_revision:
        stream << ",\"paragraph_index\":" << operation.paragraph_index
               << ",\"text_offset\":" << operation.text_offset;
        if (operation.kind !=
            review_mutation_plan_operation_kind::insert_paragraph_text_revision) {
            stream << ",\"text_length\":" << operation.text_length;
        }
        break;
    case review_mutation_plan_operation_kind::append_text_range_comment:
    case review_mutation_plan_operation_kind::insert_text_range_revision:
    case review_mutation_plan_operation_kind::delete_text_range_revision:
    case review_mutation_plan_operation_kind::replace_text_range_revision:
        stream << ",\"start_paragraph_index\":"
               << operation.start_paragraph_index
               << ",\"start_text_offset\":" << operation.start_text_offset;
        if (operation.kind !=
            review_mutation_plan_operation_kind::insert_text_range_revision) {
            stream << ",\"end_paragraph_index\":"
                   << operation.end_paragraph_index
                   << ",\"end_text_offset\":" << operation.end_text_offset;
        }
        break;
    }
    if (operation.kind ==
            review_mutation_plan_operation_kind::append_paragraph_text_comment ||
        operation.kind ==
            review_mutation_plan_operation_kind::append_comment_reply ||
        operation.kind ==
            review_mutation_plan_operation_kind::replace_comment ||
        operation.kind ==
            review_mutation_plan_operation_kind::append_text_range_comment) {
        stream << ",\"comment_text\":";
        write_json_string(stream, operation.text);
    } else if (operation.kind ==
            review_mutation_plan_operation_kind::insert_paragraph_text_revision ||
        operation.kind ==
            review_mutation_plan_operation_kind::replace_paragraph_text_revision ||
        operation.kind ==
            review_mutation_plan_operation_kind::insert_text_range_revision ||
        operation.kind ==
            review_mutation_plan_operation_kind::replace_text_range_revision) {
        stream << ",\"text\":";
        write_json_string(stream, operation.text);
    }
    if (operation.expected_text.has_value()) {
        stream << ",\"expected_text\":";
        write_json_string(stream, *operation.expected_text);
    }
    if (operation.expected_comment_text.has_value()) {
        stream << ",\"expected_comment_text\":";
        write_json_string(stream, *operation.expected_comment_text);
    }
    if (operation.expected_resolved.has_value()) {
        stream << ",\"expected_resolved\":"
               << write_json_bool(*operation.expected_resolved);
    }
    if (operation.expected_parent_index.has_value()) {
        stream << ",\"expected_parent_index\":"
               << *operation.expected_parent_index;
    }
    if (operation.author.has_value()) {
        stream << ",\"author\":";
        write_json_string(stream, *operation.author);
    }
    if (operation.initials.has_value()) {
        stream << ",\"initials\":";
        write_json_string(stream, *operation.initials);
    }
    if (operation.date.has_value()) {
        stream << ",\"date\":";
        write_json_string(stream, *operation.date);
    }
    if (operation.clear_author) {
        stream << ",\"clear_author\":true";
    }
    if (operation.clear_initials) {
        stream << ",\"clear_initials\":true";
    }
    if (operation.clear_date) {
        stream << ",\"clear_date\":true";
    }
    stream << '}';
}

void write_json_review_mutation_plan_document(
    std::ostream &stream,
    const std::vector<review_mutation_plan_operation> &operations) {
    stream << "{\"operations\":[";
    for (std::size_t index = 0U; index < operations.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        write_json_review_mutation_plan_operation(stream, operations[index]);
    }
    stream << "]}";
}

auto write_review_mutation_plan_file(
    const path_type &plan_path,
    const std::vector<review_mutation_plan_operation> &operations,
    std::string &error_message) -> bool {
    std::ofstream stream(plan_path, std::ios::binary);
    if (!stream.good()) {
        error_message = "failed to write review mutation plan file: " +
                        plan_path.string();
        return false;
    }

    write_json_review_mutation_plan_document(stream, operations);
    stream << '\n';
    if (!stream.good()) {
        error_message = "failed to write review mutation plan file: " +
                        plan_path.string();
        return false;
    }
    return true;
}
} // namespace featherdoc_cli
