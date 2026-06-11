#include "featherdoc_cli_numbering_catalog_patch_level_parse.hpp"

#include "featherdoc_cli_json_parse.hpp"
#include "featherdoc_cli_numbering_catalog_parse.hpp"
#include "featherdoc_cli_numbering_catalog_patch_parse_support.hpp"

#include <string>
#include <utility>

namespace featherdoc_cli {
namespace {

auto parse_numbering_catalog_level_patch(
    std::string_view content, std::size_t &index,
    numbering_catalog_level_patch &patch, std::string &error_message) -> bool {
    skip_json_patch_whitespace(content, index);
    if (index >= content.size() || content[index] != '{') {
        return report_json_input_error("numbering catalog patch file", index,
                                       "level patch entry must be an object",
                                       error_message);
    }

    bool saw_definition_name = false;
    bool saw_level = false;

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
                return report_json_input_error(
                    "numbering catalog patch file", index,
                    "expected ':' after level patch member", error_message);
            }

            ++index;
            if (member_name == "definition_name") {
                if (saw_definition_name) {
                    error_message = "JSON numbering catalog patch member "
                                    "'definition_name' must not be duplicated";
                    return false;
                }
                saw_definition_name = true;
                skip_json_patch_whitespace(content, index);
                if (!parse_json_patch_string(content, index, patch.definition_name,
                                             error_message)) {
                    return false;
                }
            } else if (member_name == "level") {
                if (saw_level) {
                    error_message = "JSON numbering catalog patch member 'level' "
                                    "must not be duplicated";
                    return false;
                }
                saw_level = true;
                if (!parse_numbering_catalog_level_definition(
                        content, index, patch.level_definition, error_message)) {
                    return false;
                }
            } else {
                return report_json_input_error("numbering catalog patch file", index,
                                               "unknown level patch member",
                                               error_message);
            }

            bool closed = false;
            if (!consume_json_numbering_catalog_patch_separator(
                    content, index, '}',
                    "expected ',' or '}' after level patch member", closed,
                    error_message)) {
                return false;
            }
            if (closed) {
                break;
            }
        }
    }

    if (!saw_definition_name || !saw_level) {
        error_message = "JSON numbering catalog level patch entries must contain "
                        "'definition_name' and 'level'";
        return false;
    }
    if (patch.definition_name.empty()) {
        error_message =
            "JSON numbering catalog level patch member 'definition_name' must not be empty";
        return false;
    }

    return true;
}

} // namespace

auto parse_numbering_catalog_level_patch_array(
    std::string_view content, std::size_t &index,
    std::vector<numbering_catalog_level_patch> &patches,
    std::string_view member_name, std::string &error_message) -> bool {
    skip_json_patch_whitespace(content, index);
    if (index >= content.size() || content[index] != '[') {
        return report_json_input_error(
            "numbering catalog patch file", index,
            "expected " + std::string(member_name) + " array", error_message);
    }

    patches.clear();
    ++index;
    skip_json_patch_whitespace(content, index);
    if (index < content.size() && content[index] == ']') {
        ++index;
        return true;
    }

    while (index < content.size()) {
        numbering_catalog_level_patch patch;
        if (!parse_numbering_catalog_level_patch(content, index, patch,
                                                 error_message)) {
            return false;
        }
        patches.push_back(std::move(patch));

        bool closed = false;
        if (!consume_json_numbering_catalog_patch_separator(
                content, index, ']',
                "expected ',' or ']' after level patch entry", closed,
                error_message)) {
            return false;
        }
        if (closed) {
            return true;
        }
    }

    return report_json_input_error(
        "numbering catalog patch file", index,
        "unterminated " + std::string(member_name) + " array", error_message);
}

} // namespace featherdoc_cli
