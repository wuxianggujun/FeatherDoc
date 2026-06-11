#include "featherdoc_cli_review_mutation_plan_build_request_operation_parse_detail.hpp"

#include "featherdoc_cli_json_parse.hpp"
#include "featherdoc_cli_review_mutation_plan_build_request_parse_support.hpp"

#include <utility>

namespace featherdoc_cli {
namespace {

[[nodiscard]] auto parse_review_mutation_plan_build_request_operation(
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

    review_mutation_plan_build_request_operation_members members;
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

        if (!parse_review_mutation_plan_build_request_operation_member(
                content, index, member_name, operation, members,
                error_message)) {
            return false;
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

    return validate_review_mutation_plan_build_request_operation(
        operation, members, error_message);
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
