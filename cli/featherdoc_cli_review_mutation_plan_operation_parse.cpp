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

#include "featherdoc_cli_review_mutation_plan_operation_index_members.inc"
#include "featherdoc_cli_review_mutation_plan_operation_text_members.inc"
#include "featherdoc_cli_review_mutation_plan_operation_expected_members.inc"
#include "featherdoc_cli_review_mutation_plan_operation_metadata_members.inc"

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

#include "featherdoc_cli_review_mutation_plan_operation_validation.inc"

}
} // namespace featherdoc_cli
