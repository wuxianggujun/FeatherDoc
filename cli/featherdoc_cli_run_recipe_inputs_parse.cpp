#include "featherdoc_cli_run_recipe_parse.hpp"

#include "featherdoc_cli_json_parse.hpp"
#include "featherdoc_cli_run_recipe_parse_support.hpp"

#include <utility>

namespace featherdoc_cli {
namespace {

[[nodiscard]] auto parse_run_recipe_scalar_value(
    std::string_view content, std::size_t &index, std::string &value,
    std::string &error_message) -> bool {
    skip_json_patch_whitespace(content, index);
    if (index >= content.size()) {
        return report_json_patch_error(index, "expected input value",
                                       error_message);
    }

    if (content[index] == '{' || content[index] == '[') {
        return report_json_patch_error(
            index,
            "recipe input values must be strings, numbers, booleans, or null",
            error_message);
    }

    if (content[index] == '"') {
        return parse_json_patch_string(content, index, value, error_message);
    }
    if (content[index] == 't') {
        if (content.substr(index, 4U) != "true") {
            return report_json_patch_error(index, "expected 'true'",
                                           error_message);
        }
        value = "true";
        index += 4U;
        return true;
    }
    if (content[index] == 'f') {
        if (content.substr(index, 5U) != "false") {
            return report_json_patch_error(index, "expected 'false'",
                                           error_message);
        }
        value = "false";
        index += 5U;
        return true;
    }
    if (content[index] == 'n') {
        if (content.substr(index, 4U) != "null") {
            return report_json_patch_error(index, "expected 'null'",
                                           error_message);
        }
        value.clear();
        index += 4U;
        return true;
    }
    if (content[index] == '-' ||
        (content[index] >= '0' && content[index] <= '9')) {
        return parse_json_patch_number(content, index, value, error_message);
    }

    return report_json_patch_error(index,
                                   "table cell values must be strings, numbers, "
                                   "booleans, or null",
                                   error_message);
}

[[nodiscard]] auto parse_run_recipe_inputs_object(
    std::string_view content, std::size_t &index,
    std::unordered_map<std::string, std::string> &inputs,
    std::string &error_message) -> bool {
    if (index >= content.size() || content[index] != '{') {
        return report_json_patch_error(index, "expected inputs object",
                                       error_message);
    }

    ++index;
    skip_json_patch_whitespace(content, index);
    if (index < content.size() && content[index] == '}') {
        ++index;
        return true;
    }

    while (index < content.size()) {
        std::string member_name;
        if (!parse_json_patch_string(content, index, member_name,
                                     error_message)) {
            return false;
        }

        skip_json_patch_whitespace(content, index);
        if (index >= content.size() || content[index] != ':') {
            return report_json_patch_error(index,
                                           "expected ':' after object member",
                                           error_message);
        }
        ++index;

        if (member_name == "inputs") {
            skip_json_patch_whitespace(content, index);
            if (index >= content.size() || content[index] != '{') {
                return report_json_patch_error(
                    index, "run-recipe inputs member must be an object",
                    error_message);
            }
            if (!parse_run_recipe_inputs_object(content, index, inputs,
                                                error_message)) {
                return false;
            }
        } else {
            std::string value;
            if (!parse_run_recipe_scalar_value(content, index, value,
                                               error_message)) {
                return false;
            }
            if (!inputs.emplace(member_name, std::move(value)).second) {
                error_message =
                    "run-recipe input member '" + member_name + "' is duplicated";
                return false;
            }
        }

        bool done = false;
        if (!consume_run_recipe_json_object_separator(content, index,
                                                      error_message, done)) {
            return false;
        }
        if (done) {
            return true;
        }
    }

    return report_json_patch_error(index, "unexpected end of inputs object",
                                   error_message);
}

} // namespace

auto read_run_recipe_inputs_file(
    const std::filesystem::path &inputs_path,
    std::unordered_map<std::string, std::string> &inputs,
    std::string &error_message) -> bool {
    std::string content;
    if (!read_run_recipe_utf8_text_file(inputs_path, "run-recipe inputs",
                                        content, error_message)) {
        return false;
    }

    inputs.clear();
    std::size_t index = 0U;
    skip_json_patch_whitespace(content, index);
    if (!parse_run_recipe_inputs_object(content, index, inputs,
                                        error_message)) {
        return false;
    }

    skip_json_patch_whitespace(content, index);
    if (index != content.size()) {
        return report_json_patch_error(
            index, "unexpected trailing content after inputs object",
            error_message);
    }

    return true;
}

} // namespace featherdoc_cli
