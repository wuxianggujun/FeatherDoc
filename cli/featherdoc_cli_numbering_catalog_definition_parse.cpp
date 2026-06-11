#include "featherdoc_cli_numbering_catalog_definition_parse.hpp"

#include "featherdoc_cli_json_parse.hpp"
#include "featherdoc_cli_numbering_catalog_instance_parse.hpp"
#include "featherdoc_cli_numbering_catalog_level_parse.hpp"
#include "featherdoc_cli_numbering_catalog_parse_support.hpp"

#include <string>
#include <utility>

namespace featherdoc_cli {
namespace {

auto parse_numbering_catalog_definition(
    std::string_view content, std::size_t &index,
    featherdoc::numbering_catalog_definition &catalog_definition,
    std::string &error_message) -> bool {
    skip_json_patch_whitespace(content, index);
    if (index >= content.size() || content[index] != '{') {
        return report_json_input_error("numbering catalog file", index,
                                       "numbering catalog definition must be an object",
                                       error_message);
    }

    bool saw_name = false;
    bool saw_levels = false;
    bool saw_instances = false;

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
                    "numbering catalog file", index,
                    "expected ':' after numbering definition member", error_message);
            }

            ++index;
            if (member_name == "name") {
                if (saw_name) {
                    error_message = "JSON numbering catalog definition member 'name' "
                                    "must not be duplicated";
                    return false;
                }
                saw_name = true;
                skip_json_patch_whitespace(content, index);
                if (!parse_json_patch_string(
                        content, index, catalog_definition.definition.name,
                        error_message)) {
                    return false;
                }
            } else if (member_name == "levels") {
                if (saw_levels) {
                    error_message = "JSON numbering catalog definition member 'levels' "
                                    "must not be duplicated";
                    return false;
                }
                saw_levels = true;
                if (!parse_numbering_catalog_level_definitions(
                        content, index, catalog_definition.definition.levels,
                        error_message)) {
                    return false;
                }
            } else if (member_name == "instances") {
                if (saw_instances) {
                    error_message = "JSON numbering catalog definition member "
                                    "'instances' must not be duplicated";
                    return false;
                }
                saw_instances = true;
                if (!parse_numbering_catalog_instances(
                        content, index, catalog_definition.instances,
                        error_message)) {
                    return false;
                }
            } else {
                return report_json_input_error("numbering catalog file", index,
                                               "unknown numbering definition member",
                                               error_message);
            }

            bool closed = false;
            if (!consume_json_numbering_catalog_separator(
                    content, index, '}',
                    "expected ',' or '}' after numbering definition member", closed,
                    error_message)) {
                return false;
            }
            if (closed) {
                break;
            }
        }
    }

    if (!saw_name || !saw_levels) {
        error_message =
            "JSON numbering catalog definition must contain 'name' and 'levels'";
        return false;
    }

    return true;
}

} // namespace

auto parse_numbering_catalog_definitions(
    std::string_view content, std::size_t &index,
    std::vector<featherdoc::numbering_catalog_definition> &definitions,
    std::string &error_message) -> bool {
    skip_json_patch_whitespace(content, index);
    if (index >= content.size() || content[index] != '[') {
        return report_json_input_error("numbering catalog file", index,
                                       "expected definitions array", error_message);
    }

    definitions.clear();
    ++index;
    skip_json_patch_whitespace(content, index);
    if (index < content.size() && content[index] == ']') {
        ++index;
        return true;
    }

    while (index < content.size()) {
        featherdoc::numbering_catalog_definition definition;
        if (!parse_numbering_catalog_definition(content, index, definition,
                                                error_message)) {
            return false;
        }
        definitions.push_back(std::move(definition));

        bool closed = false;
        if (!consume_json_numbering_catalog_separator(
                content, index, ']',
                "expected ',' or ']' after numbering definition", closed,
                error_message)) {
            return false;
        }
        if (closed) {
            return true;
        }
    }

    return report_json_input_error("numbering catalog file", index,
                                   "unterminated definitions array", error_message);
}

} // namespace featherdoc_cli
