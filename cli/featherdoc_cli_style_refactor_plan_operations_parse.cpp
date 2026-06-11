#include "featherdoc_cli_style_refactor_plan_operations_parse.hpp"

#include "featherdoc_cli_json_parse.hpp"
#include "featherdoc_cli_style_refactor_plan_parse.hpp"

#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace featherdoc_cli {
namespace {

auto parse_style_refactor_plan_operation(
    std::string_view content, std::size_t &index,
    featherdoc::style_refactor_request &request, std::string &error_message)
    -> bool {
    skip_json_patch_whitespace(content, index);
    if (index >= content.size() || content[index] != '{') {
        return report_json_input_error("style refactor plan file", index,
                                       "expected operation object", error_message);
    }

    ++index;
    skip_json_patch_whitespace(content, index);
    bool saw_action = false;
    bool saw_source = false;
    bool saw_target = false;
    if (index < content.size() && content[index] == '}') {
        return report_json_input_error("style refactor plan file", index,
                                       "operation object must not be empty",
                                       error_message);
    }

    while (index < content.size()) {
        std::string member_name;
        if (!parse_json_patch_string(content, index, member_name, error_message)) {
            return false;
        }
        skip_json_patch_whitespace(content, index);
        if (index >= content.size() || content[index] != ':') {
            return report_json_input_error(
                "style refactor plan file", index,
                "expected ':' after operation object member", error_message);
        }
        ++index;
        skip_json_patch_whitespace(content, index);

        if (member_name == "action") {
            if (saw_action) {
                error_message =
                    "JSON style refactor plan operation member 'action' must not be duplicated";
                return false;
            }
            saw_action = true;
            if (!parse_style_refactor_plan_action(content, index, request.action,
                                                  error_message)) {
                return false;
            }
        } else if (member_name == "source_style_id") {
            if (saw_source) {
                error_message = "JSON style refactor plan operation member "
                                "'source_style_id' must not be duplicated";
                return false;
            }
            saw_source = true;
            if (!parse_json_patch_string(content, index, request.source_style_id,
                                         error_message)) {
                return false;
            }
        } else if (member_name == "target_style_id") {
            if (saw_target) {
                error_message = "JSON style refactor plan operation member "
                                "'target_style_id' must not be duplicated";
                return false;
            }
            saw_target = true;
            if (!parse_json_patch_string(content, index, request.target_style_id,
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
                "expected ',' or '}' after operation object member", closed,
                error_message)) {
            return false;
        }
        if (closed) {
            break;
        }
    }

    if (!saw_action || !saw_source || !saw_target) {
        error_message = "JSON style refactor plan operation must contain 'action', "
                        "'source_style_id', and 'target_style_id'";
        return false;
    }

    return true;
}

} // namespace

auto consume_style_refactor_plan_separator(std::string_view content,
                                           std::size_t &index, char close_char,
                                           std::string_view error_detail,
                                           bool &closed,
                                           std::string &error_message) -> bool {
    skip_json_patch_whitespace(content, index);
    if (index >= content.size()) {
        return report_json_input_error("style refactor plan file", index,
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

    return report_json_input_error("style refactor plan file", index, error_detail,
                                   error_message);
}

auto parse_style_refactor_plan_operations(
    std::string_view content, std::size_t &index,
    std::vector<featherdoc::style_refactor_request> &requests,
    std::string &error_message) -> bool {
    skip_json_patch_whitespace(content, index);
    if (index >= content.size() || content[index] != '[') {
        return report_json_input_error("style refactor plan file", index,
                                       "expected operations array", error_message);
    }

    requests.clear();
    ++index;
    skip_json_patch_whitespace(content, index);
    if (index < content.size() && content[index] == ']') {
        ++index;
        return true;
    }

    while (index < content.size()) {
        auto request = featherdoc::style_refactor_request{};
        if (!parse_style_refactor_plan_operation(content, index, request,
                                                 error_message)) {
            return false;
        }
        requests.push_back(std::move(request));

        bool closed = false;
        if (!consume_style_refactor_plan_separator(
                content, index, ']',
                "expected ',' or ']' after operations array item", closed,
                error_message)) {
            return false;
        }
        if (closed) {
            return true;
        }
    }

    return report_json_input_error("style refactor plan file", index,
                                   "unterminated operations array", error_message);
}

} // namespace featherdoc_cli
