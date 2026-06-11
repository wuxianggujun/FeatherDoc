#include "featherdoc_cli_numbering_catalog_patch_override_parse.hpp"

#include "featherdoc_cli_json_parse.hpp"
#include "featherdoc_cli_numbering_catalog_parse.hpp"
#include "featherdoc_cli_numbering_catalog_patch_parse_support.hpp"

#include <cstdint>
#include <string>
#include <utility>

namespace featherdoc_cli {
namespace {

auto parse_numbering_catalog_override_patch(
    std::string_view content, std::size_t &index,
    numbering_catalog_override_patch &patch, bool allow_override_values,
    std::string &error_message) -> bool {
    skip_json_patch_whitespace(content, index);
    if (index >= content.size() || content[index] != '{') {
        return report_json_input_error("numbering catalog patch file", index,
                                       "override patch entry must be an object",
                                       error_message);
    }

    bool saw_definition_name = false;
    bool saw_level = false;
    bool saw_instance_index = false;
    bool saw_instance_id = false;

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
                    "expected ':' after override patch member", error_message);
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
            } else if (member_name == "instance_index") {
                if (saw_instance_index) {
                    error_message = "JSON numbering catalog patch member "
                                    "'instance_index' must not be duplicated";
                    return false;
                }
                saw_instance_index = true;
                std::uint32_t parsed_index = 0U;
                if (!parse_json_numbering_catalog_uint32(
                        content, index, parsed_index, member_name,
                        error_message)) {
                    return false;
                }
                patch.instance_index = static_cast<std::size_t>(parsed_index);
            } else if (member_name == "instance_id") {
                if (saw_instance_id) {
                    error_message = "JSON numbering catalog patch member "
                                    "'instance_id' must not be duplicated";
                    return false;
                }
                saw_instance_id = true;
                std::uint32_t parsed_id = 0U;
                if (!parse_json_numbering_catalog_uint32(
                        content, index, parsed_id, member_name, error_message)) {
                    return false;
                }
                patch.instance_id = parsed_id;
            } else if (member_name == "level") {
                if (saw_level) {
                    error_message = "JSON numbering catalog patch member 'level' "
                                    "must not be duplicated";
                    return false;
                }
                saw_level = true;
                if (!parse_json_numbering_catalog_uint32(
                        content, index, patch.level, member_name, error_message)) {
                    return false;
                }
            } else if (member_name == "start_override") {
                if (!allow_override_values) {
                    error_message = "JSON numbering catalog remove override entries "
                                    "must not contain 'start_override'";
                    return false;
                }
                if (patch.saw_start_override) {
                    error_message = "JSON numbering catalog patch member "
                                    "'start_override' must not be duplicated";
                    return false;
                }
                patch.saw_start_override = true;
                if (!parse_json_numbering_catalog_optional_uint32(
                        content, index, patch.start_override, member_name,
                        error_message)) {
                    return false;
                }
            } else if (member_name == "level_definition") {
                if (!allow_override_values) {
                    error_message = "JSON numbering catalog remove override entries "
                                    "must not contain 'level_definition'";
                    return false;
                }
                if (patch.saw_level_definition) {
                    error_message = "JSON numbering catalog patch member "
                                    "'level_definition' must not be duplicated";
                    return false;
                }
                patch.saw_level_definition = true;
                skip_json_patch_whitespace(content, index);
                if (index < content.size() && content.substr(index, 4U) == "null") {
                    index += 4U;
                    patch.level_definition.reset();
                } else {
                    featherdoc::numbering_level_definition parsed_definition;
                    if (!parse_numbering_catalog_level_definition(
                            content, index, parsed_definition, error_message)) {
                        return false;
                    }
                    patch.level_definition = std::move(parsed_definition);
                }
            } else {
                return report_json_input_error("numbering catalog patch file", index,
                                               "unknown override patch member",
                                               error_message);
            }

            bool closed = false;
            if (!consume_json_numbering_catalog_patch_separator(
                    content, index, '}',
                    "expected ',' or '}' after override patch member", closed,
                    error_message)) {
                return false;
            }
            if (closed) {
                break;
            }
        }
    }

    if (!saw_definition_name || !saw_level) {
        error_message = "JSON numbering catalog override patch entries must contain "
                        "'definition_name' and 'level'";
        return false;
    }
    if (patch.definition_name.empty()) {
        error_message =
            "JSON numbering catalog override patch member 'definition_name' must not be empty";
        return false;
    }
    if (patch.instance_index.has_value() == patch.instance_id.has_value()) {
        error_message = "JSON numbering catalog override patch entries must contain "
                        "exactly one of 'instance_index' or 'instance_id'";
        return false;
    }
    if (allow_override_values && !patch.saw_start_override &&
        !patch.saw_level_definition) {
        error_message = "JSON numbering catalog upsert override entries must contain "
                        "'start_override' or 'level_definition'";
        return false;
    }

    return true;
}

} // namespace

auto parse_numbering_catalog_override_patch_array(
    std::string_view content, std::size_t &index,
    std::vector<numbering_catalog_override_patch> &patches,
    std::string_view member_name, bool allow_override_values,
    std::string &error_message) -> bool {
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
        numbering_catalog_override_patch patch;
        if (!parse_numbering_catalog_override_patch(
                content, index, patch, allow_override_values, error_message)) {
            return false;
        }
        patches.push_back(std::move(patch));

        bool closed = false;
        if (!consume_json_numbering_catalog_patch_separator(
                content, index, ']',
                "expected ',' or ']' after override patch entry", closed,
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
