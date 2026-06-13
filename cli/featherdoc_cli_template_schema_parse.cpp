#include "featherdoc_cli_template_schema_parse.hpp"

#include "featherdoc_cli_json_parse.hpp"

#include <string>
#include <utility>

namespace featherdoc_cli {
namespace {

using path_type = std::filesystem::path;

[[nodiscard]] auto parse_template_schema_targets_array(
    std::string_view content, std::size_t &index,
    exported_template_schema_result &result, std::string &error_message)
    -> bool {
    skip_json_patch_whitespace(content, index);
    if (index >= content.size() || content[index] != '[') {
        return report_json_input_error("JSON schema file", index,
                                       "expected targets array",
                                       error_message);
    }

    ++index;
    skip_json_patch_whitespace(content, index);
    if (index < content.size() && content[index] == ']') {
        ++index;
        return true;
    }

    while (index < content.size()) {
        exported_template_schema_target target;
        if (!parse_template_schema_json_target(content, index, target,
                                               error_message)) {
            return false;
        }
        result.targets.push_back(std::move(target));

        skip_json_patch_whitespace(content, index);
        if (index >= content.size()) {
            break;
        }
        if (content[index] == ',') {
            ++index;
            skip_json_patch_whitespace(content, index);
            continue;
        }
        if (content[index] == ']') {
            ++index;
            return true;
        }
        return report_json_input_error(
            "JSON schema file", index,
            "expected ',' or ']' after target entry", error_message);
    }

    return report_json_input_error("JSON schema file", index,
                                   "unterminated targets array",
                                   error_message);
}

} // namespace

auto read_template_schema_file(const path_type &schema_path,
                               exported_template_schema_result &result,
                               std::string &error_message) -> bool {
    std::string content;
    std::size_t index = 0U;
    if (!read_template_table_json_content(schema_path, content, index,
                                          error_message)) {
        if (error_message.rfind("failed to read JSON patch file:", 0U) == 0U) {
            error_message.replace(
                0U, std::string("failed to read JSON patch file:").size(),
                "failed to read JSON schema file:");
        }
        return false;
    }

    skip_json_patch_whitespace(content, index);
    if (index >= content.size() || content[index] != '{') {
        return report_json_input_error("JSON schema file", index,
                                       "root must be an object",
                                       error_message);
    }

    result = {};
    bool saw_targets = false;
    ++index;
    skip_json_patch_whitespace(content, index);
    if (index < content.size() && content[index] == '}') {
        ++index;
    } else {
        while (index < content.size()) {
            std::string member_name;
            if (!parse_json_patch_string(content, index, member_name,
                                         error_message)) {
                return false;
            }

            skip_json_patch_whitespace(content, index);
            if (index >= content.size() || content[index] != ':') {
                return report_json_input_error("JSON schema file", index,
                                               "expected ':' after object member",
                                               error_message);
            }

            ++index;
            if (member_name == "targets") {
                if (saw_targets) {
                    error_message =
                        "JSON schema member 'targets' must not be duplicated";
                    return false;
                }
                if (!parse_template_schema_targets_array(
                        content, index, result, error_message)) {
                    return false;
                }
                saw_targets = true;
            } else {
                return report_json_input_error("JSON schema file", index,
                                               "unknown root member",
                                               error_message);
            }

            skip_json_patch_whitespace(content, index);
            if (index >= content.size()) {
                break;
            }
            if (content[index] == ',') {
                ++index;
                skip_json_patch_whitespace(content, index);
                continue;
            }
            if (content[index] == '}') {
                ++index;
                break;
            }
            return report_json_input_error("JSON schema file", index,
                                           "expected ',' or '}' after object member",
                                           error_message);
        }
    }

    skip_json_patch_whitespace(content, index);
    if (index != content.size()) {
        return report_json_input_error(
            "JSON schema file", index,
            "unexpected trailing content after root object", error_message);
    }

    if (!saw_targets) {
        error_message = "JSON schema file must contain a 'targets' array";
        return false;
    }

    return true;
}

} // namespace featherdoc_cli
