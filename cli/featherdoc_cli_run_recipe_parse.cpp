#include "featherdoc_cli_run_recipe_parse.hpp"

#include "featherdoc_cli_json_parse.hpp"

#include <fstream>
#include <iterator>
#include <utility>

namespace featherdoc_cli {
namespace {

using path_type = std::filesystem::path;

auto read_utf8_text_file(const path_type &path, std::string_view label,
                         std::string &content, std::string &error_message)
    -> bool {
    std::ifstream stream(path, std::ios::binary);
    if (!stream) {
        error_message = "failed to open " + std::string(label) + ": " + path.string();
        return false;
    }

    content.assign(std::istreambuf_iterator<char>(stream),
                   std::istreambuf_iterator<char>());
    if (stream.bad()) {
        error_message = "failed to read " + std::string(label) + ": " + path.string();
        return false;
    }

    return true;
}

auto consume_run_recipe_json_object_separator(std::string_view content,
                                              std::size_t &index,
                                              std::string &error_message,
                                              bool &done) -> bool {
    skip_json_patch_whitespace(content, index);
    if (index >= content.size()) {
        return report_json_patch_error(index,
                                       "unexpected end of object",
                                       error_message);
    }

    if (content[index] == ',') {
        ++index;
        skip_json_patch_whitespace(content, index);
        done = false;
        return true;
    }

    if (content[index] == '}') {
        ++index;
        done = true;
        return true;
    }

    return report_json_patch_error(index,
                                   "expected ',' or '}' after object member",
                                   error_message);
}

auto parse_run_recipe_scalar_value(std::string_view content, std::size_t &index,
                                   std::string &value,
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

auto parse_run_recipe_inputs_object(
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

auto parse_run_recipe_options(const std::vector<std::string_view> &arguments,
                              run_recipe_options &options,
                              std::string &error_message) -> bool {
    options = {};

    for (std::size_t index = 1U; index < arguments.size(); ++index) {
        const auto argument = arguments[index];
        if (argument == "--json") {
            options.json_output = true;
            continue;
        }

        if (argument == "--recipe" || argument == "--input") {
            if (options.recipe_path.has_value()) {
                error_message =
                    "run-recipe recipe path was provided more than once";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = std::string(argument) + " expects a path";
                return false;
            }
            options.recipe_path = path_type(std::string(arguments[++index]));
            continue;
        }

        if (argument == "--inputs") {
            if (options.inputs_path.has_value()) {
                error_message =
                    "run-recipe inputs path was provided more than once";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "--inputs expects a path";
                return false;
            }
            options.inputs_path = path_type(std::string(arguments[++index]));
            continue;
        }

        if (argument == "--output" || argument == "--output-dir") {
            if (options.output_dir.has_value()) {
                error_message =
                    "run-recipe output directory was provided more than once";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = std::string(argument) + " expects a path";
                return false;
            }
            options.output_dir = path_type(std::string(arguments[++index]));
            continue;
        }

        error_message = "unknown run-recipe option: " + std::string(argument);
        return false;
    }

    if (!options.recipe_path.has_value()) {
        error_message = "run-recipe expects --recipe <recipe.json>";
        return false;
    }
    if (!options.inputs_path.has_value()) {
        error_message = "run-recipe expects --inputs <inputs.json>";
        return false;
    }
    if (!options.output_dir.has_value()) {
        error_message = "run-recipe expects --output <output-dir>";
        return false;
    }

    return true;
}

auto read_run_recipe_inputs_file(
    const path_type &inputs_path,
    std::unordered_map<std::string, std::string> &inputs,
    std::string &error_message) -> bool {
    std::string content;
    if (!read_utf8_text_file(inputs_path, "run-recipe inputs", content,
                             error_message)) {
        return false;
    }

    inputs.clear();
    std::size_t index = 0U;
    skip_json_patch_whitespace(content, index);
    if (!parse_run_recipe_inputs_object(content, index, inputs, error_message)) {
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

auto read_run_recipe_recipe_id(const path_type &recipe_path,
                               std::string &recipe_id,
                               std::string &error_message) -> bool {
    std::string content;
    if (!read_utf8_text_file(recipe_path, "run-recipe recipe", content,
                             error_message)) {
        return false;
    }

    std::size_t index = 0U;
    skip_json_patch_whitespace(content, index);
    if (index >= content.size() || content[index] != '{') {
        return report_json_patch_error(index, "expected recipe object",
                                       error_message);
    }

    recipe_id.clear();
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
                return report_json_patch_error(
                    index, "expected ':' after recipe object member",
                    error_message);
            }
            ++index;

            if (member_name == "id") {
                if (!recipe_id.empty()) {
                    error_message = "recipe member 'id' must not be duplicated";
                    return false;
                }
                skip_json_patch_whitespace(content, index);
                if (!parse_json_patch_string(content, index, recipe_id,
                                             error_message)) {
                    return false;
                }
            } else if (!skip_json_patch_value(content, index, error_message)) {
                return false;
            }

            bool done = false;
            if (!consume_run_recipe_json_object_separator(content, index,
                                                          error_message,
                                                          done)) {
                return false;
            }
            if (done) {
                break;
            }
        }
    }

    skip_json_patch_whitespace(content, index);
    if (index != content.size()) {
        return report_json_patch_error(
            index, "unexpected trailing content after recipe object",
            error_message);
    }
    if (recipe_id.empty()) {
        error_message = "recipe JSON must contain a non-empty string id";
        return false;
    }

    return true;
}

} // namespace featherdoc_cli
