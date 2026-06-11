#include "featherdoc_cli_style_refactor_plan_parse.hpp"

#include "featherdoc_cli_json_parse.hpp"
#include "featherdoc_cli_style_refactor_plan_operations_parse.hpp"

#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

namespace featherdoc_cli {
namespace {

using path_type = std::filesystem::path;

auto read_style_refactor_plan_content(const path_type &plan_path,
                                      std::string &content,
                                      std::string &error_message) -> bool {
    std::ifstream stream(plan_path, std::ios::binary);
    if (!stream.good()) {
        error_message = "failed to read style refactor plan file: " +
                        plan_path.string();
        return false;
    }

    content.assign(std::istreambuf_iterator<char>(stream),
                   std::istreambuf_iterator<char>());
    return true;
}

} // namespace

auto read_style_refactor_plan_file(
    const path_type &plan_path,
    std::vector<featherdoc::style_refactor_request> &requests,
    std::string &error_message) -> bool {
    std::string content;
    if (!read_style_refactor_plan_content(plan_path, content, error_message)) {
        return false;
    }

    std::size_t index = 0U;
    skip_json_patch_whitespace(content, index);
    if (index >= content.size() || content[index] != '{') {
        return report_json_input_error("style refactor plan file", index,
                                       "expected root object", error_message);
    }

    ++index;
    skip_json_patch_whitespace(content, index);
    bool saw_operations = false;
    if (index < content.size() && content[index] == '}') {
        return report_json_input_error("style refactor plan file", index,
                                       "root object must not be empty",
                                       error_message);
    }

    while (index < content.size()) {
        std::string member_name;
        if (!parse_json_patch_string(content, index, member_name, error_message)) {
            return false;
        }
        skip_json_patch_whitespace(content, index);
        if (index >= content.size() || content[index] != ':') {
            return report_json_input_error("style refactor plan file", index,
                                           "expected ':' after root object member",
                                           error_message);
        }
        ++index;
        skip_json_patch_whitespace(content, index);

        if (member_name == "operations") {
            if (saw_operations) {
                error_message = "JSON style refactor plan root member 'operations' "
                                "must not be duplicated";
                return false;
            }
            saw_operations = true;
            if (!parse_style_refactor_plan_operations(content, index, requests,
                                                      error_message)) {
                return false;
            }
        } else {
            if (!skip_json_patch_value(content, index, error_message)) {
                return false;
            }
        }

        bool closed = false;
        if (!consume_style_refactor_plan_separator(
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
        return report_json_input_error("style refactor plan file", index,
                                       "unexpected trailing content after root object",
                                       error_message);
    }

    if (!saw_operations) {
        error_message =
            "JSON style refactor plan file must contain an 'operations' array";
        return false;
    }
    if (requests.empty()) {
        error_message =
            "JSON style refactor plan file must contain at least one operation";
        return false;
    }

    return true;
}

} // namespace featherdoc_cli
