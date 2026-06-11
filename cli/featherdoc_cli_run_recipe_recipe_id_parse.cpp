#include "featherdoc_cli_run_recipe_parse.hpp"

#include "featherdoc_cli_json_parse.hpp"
#include "featherdoc_cli_run_recipe_parse_support.hpp"

namespace featherdoc_cli {

auto read_run_recipe_recipe_id(const std::filesystem::path &recipe_path,
                               std::string &recipe_id,
                               std::string &error_message) -> bool {
    std::string content;
    if (!read_run_recipe_utf8_text_file(recipe_path, "run-recipe recipe",
                                        content, error_message)) {
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
