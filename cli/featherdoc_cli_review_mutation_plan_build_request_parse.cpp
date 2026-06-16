#include "featherdoc_cli_review_mutation_plan_parse.hpp"

#include "featherdoc_cli_json_parse.hpp"
#include "featherdoc_cli_review_mutation_plan_build_request_operation_parse.hpp"
#include "featherdoc_cli_review_mutation_plan_build_request_parse_support.hpp"

#include <filesystem>
#include <string>
#include <vector>

namespace featherdoc_cli {

using path_type = std::filesystem::path;

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

} // namespace featherdoc_cli
