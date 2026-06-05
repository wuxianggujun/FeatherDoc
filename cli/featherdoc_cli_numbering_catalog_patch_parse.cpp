#include "featherdoc_cli_numbering_catalog_patch_parse.hpp"

#include "featherdoc_cli_json_parse.hpp"
#include "featherdoc_cli_numbering_catalog_parse.hpp"

#include <cstddef>
#include <filesystem>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace featherdoc_cli {

using path_type = std::filesystem::path;

namespace {

auto consume_json_numbering_catalog_patch_separator(
    std::string_view content, std::size_t &index, char closing_character,
    std::string_view after_item_detail, bool &closed,
    std::string &error_message) -> bool {
    skip_json_patch_whitespace(content, index);
    if (index >= content.size()) {
        return report_json_input_error("numbering catalog patch file", index,
                                       "unterminated JSON container",
                                       error_message);
    }
    if (content[index] == ',') {
        ++index;
        skip_json_patch_whitespace(content, index);
        closed = false;
        return true;
    }
    if (content[index] == closing_character) {
        ++index;
        closed = true;
        return true;
    }

    return report_json_input_error("numbering catalog patch file", index,
                                   after_item_detail, error_message);
}

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

} // namespace

auto read_numbering_catalog_patch_file(
    const path_type &patch_path, numbering_catalog_patch_document &patch,
    std::string &error_message) -> bool {
    std::string content;
    std::size_t index = 0U;
    if (!read_template_table_json_content(patch_path, content, index,
                                          error_message)) {
        if (error_message.rfind("failed to read JSON patch file:", 0U) == 0U) {
            error_message.replace(0U,
                                  std::string("failed to read JSON patch file:").size(),
                                  "failed to read numbering catalog patch file:");
        }
        return false;
    }

    skip_json_patch_whitespace(content, index);
    if (index >= content.size() || content[index] != '{') {
        return report_json_input_error("numbering catalog patch file", index,
                                       "root must be an object", error_message);
    }

    patch = {};
    bool saw_upsert_levels = false;
    bool saw_upsert_overrides = false;
    bool saw_remove_overrides = false;

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
                    "expected ':' after root object member", error_message);
            }

            ++index;
            if (member_name == "upsert_levels") {
                if (saw_upsert_levels) {
                    error_message = "JSON numbering catalog patch root member "
                                    "'upsert_levels' must not be duplicated";
                    return false;
                }
                saw_upsert_levels = true;
                if (!parse_numbering_catalog_level_patch_array(
                        content, index, patch.upsert_levels, member_name,
                        error_message)) {
                    return false;
                }
            } else if (member_name == "upsert_overrides") {
                if (saw_upsert_overrides) {
                    error_message = "JSON numbering catalog patch root member "
                                    "'upsert_overrides' must not be duplicated";
                    return false;
                }
                saw_upsert_overrides = true;
                if (!parse_numbering_catalog_override_patch_array(
                        content, index, patch.upsert_overrides, member_name, true,
                        error_message)) {
                    return false;
                }
            } else if (member_name == "remove_overrides") {
                if (saw_remove_overrides) {
                    error_message = "JSON numbering catalog patch root member "
                                    "'remove_overrides' must not be duplicated";
                    return false;
                }
                saw_remove_overrides = true;
                if (!parse_numbering_catalog_override_patch_array(
                        content, index, patch.remove_overrides, member_name, false,
                        error_message)) {
                    return false;
                }
            } else {
                return report_json_input_error("numbering catalog patch file", index,
                                               "unknown root member", error_message);
            }

            bool closed = false;
            if (!consume_json_numbering_catalog_patch_separator(
                    content, index, '}',
                    "expected ',' or '}' after root object member", closed,
                    error_message)) {
                return false;
            }
            if (closed) {
                break;
            }
        }
    }

    skip_json_patch_whitespace(content, index);
    if (index != content.size()) {
        return report_json_input_error("numbering catalog patch file", index,
                                       "unexpected trailing content after root object",
                                       error_message);
    }

    return true;
}

} // namespace featherdoc_cli
