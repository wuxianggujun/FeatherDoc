#include "featherdoc_cli_review_mutation_plan_parse.hpp"

#include "featherdoc_cli_review_mutation_plan_operation_parse.hpp"

#include "featherdoc_cli_json_parse.hpp"

#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace featherdoc_cli {

using path_type = std::filesystem::path;

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

} // namespace featherdoc_cli
