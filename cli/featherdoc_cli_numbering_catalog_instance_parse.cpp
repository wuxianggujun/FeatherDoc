#include "featherdoc_cli_numbering_catalog_instance_parse.hpp"

#include "featherdoc_cli_json_parse.hpp"
#include "featherdoc_cli_numbering_catalog_parse.hpp"

#include <cstdint>
#include <optional>
#include <utility>

namespace featherdoc_cli {
namespace {

auto consume_json_numbering_catalog_separator(
    std::string_view content, std::size_t &index, char closing_character,
    std::string_view after_item_detail, bool &closed,
    std::string &error_message) -> bool {
    skip_json_patch_whitespace(content, index);
    if (index >= content.size()) {
        return report_json_input_error("numbering catalog file", index,
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

    return report_json_input_error("numbering catalog file", index,
                                   after_item_detail, error_message);
}

auto parse_numbering_catalog_level_override(
    std::string_view content, std::size_t &index,
    featherdoc::numbering_level_override_summary &level_override,
    std::string &error_message) -> bool {
    skip_json_patch_whitespace(content, index);
    if (index >= content.size() || content[index] != '{') {
        return report_json_input_error("numbering catalog file", index,
                                       "numbering level override must be an object",
                                       error_message);
    }

    std::optional<std::uint32_t> level;
    std::optional<std::uint32_t> start_override;
    bool saw_start_override = false;
    std::optional<featherdoc::numbering_level_definition> level_definition;
    bool saw_level_definition = false;

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
                    "expected ':' after numbering override member", error_message);
            }

            ++index;
            if (member_name == "level") {
                if (level.has_value()) {
                    error_message = "JSON numbering catalog override member 'level' "
                                    "must not be duplicated";
                    return false;
                }
                std::uint32_t parsed_level = 0U;
                if (!parse_json_numbering_catalog_uint32(
                        content, index, parsed_level, member_name, error_message)) {
                    return false;
                }
                level = parsed_level;
            } else if (member_name == "start_override") {
                if (saw_start_override) {
                    error_message = "JSON numbering catalog override member "
                                    "'start_override' must not be duplicated";
                    return false;
                }
                saw_start_override = true;
                if (!parse_json_numbering_catalog_optional_uint32(
                        content, index, start_override, member_name,
                        error_message)) {
                    return false;
                }
            } else if (member_name == "level_definition") {
                if (saw_level_definition) {
                    error_message = "JSON numbering catalog override member "
                                    "'level_definition' must not be duplicated";
                    return false;
                }
                saw_level_definition = true;
                skip_json_patch_whitespace(content, index);
                if (index < content.size() && content.substr(index, 4U) == "null") {
                    index += 4U;
                    level_definition.reset();
                } else {
                    featherdoc::numbering_level_definition parsed_definition;
                    if (!parse_numbering_catalog_level_definition(
                            content, index, parsed_definition, error_message)) {
                        return false;
                    }
                    level_definition = std::move(parsed_definition);
                }
            } else {
                return report_json_input_error("numbering catalog file", index,
                                               "unknown numbering override member",
                                               error_message);
            }

            bool closed = false;
            if (!consume_json_numbering_catalog_separator(
                    content, index, '}',
                    "expected ',' or '}' after numbering override member", closed,
                    error_message)) {
                return false;
            }
            if (closed) {
                break;
            }
        }
    }

    if (!level.has_value()) {
        error_message =
            "JSON numbering catalog level override must contain 'level'";
        return false;
    }

    level_override.level = *level;
    level_override.start_override = start_override;
    level_override.level_definition = std::move(level_definition);
    return true;
}

auto parse_numbering_catalog_level_overrides(
    std::string_view content, std::size_t &index,
    std::vector<featherdoc::numbering_level_override_summary> &level_overrides,
    std::string &error_message) -> bool {
    skip_json_patch_whitespace(content, index);
    if (index >= content.size() || content[index] != '[') {
        return report_json_input_error("numbering catalog file", index,
                                       "expected level_overrides array",
                                       error_message);
    }

    level_overrides.clear();
    ++index;
    skip_json_patch_whitespace(content, index);
    if (index < content.size() && content[index] == ']') {
        ++index;
        return true;
    }

    while (index < content.size()) {
        featherdoc::numbering_level_override_summary level_override;
        if (!parse_numbering_catalog_level_override(content, index, level_override,
                                                    error_message)) {
            return false;
        }
        level_overrides.push_back(std::move(level_override));

        bool closed = false;
        if (!consume_json_numbering_catalog_separator(
                content, index, ']',
                "expected ',' or ']' after numbering override", closed,
                error_message)) {
            return false;
        }
        if (closed) {
            return true;
        }
    }

    return report_json_input_error("numbering catalog file", index,
                                   "unterminated level_overrides array",
                                   error_message);
}

auto parse_numbering_catalog_instance(
    std::string_view content, std::size_t &index,
    featherdoc::numbering_instance_summary &instance,
    std::string &error_message) -> bool {
    skip_json_patch_whitespace(content, index);
    if (index >= content.size() || content[index] != '{') {
        return report_json_input_error("numbering catalog file", index,
                                       "numbering instance must be an object",
                                       error_message);
    }

    bool saw_instance_id = false;
    bool saw_level_overrides = false;

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
                    "expected ':' after numbering instance member", error_message);
            }

            ++index;
            if (member_name == "instance_id") {
                if (saw_instance_id) {
                    error_message = "JSON numbering catalog instance member "
                                    "'instance_id' must not be duplicated";
                    return false;
                }
                saw_instance_id = true;
                if (!parse_json_numbering_catalog_uint32(
                        content, index, instance.instance_id, member_name,
                        error_message)) {
                    return false;
                }
            } else if (member_name == "level_overrides") {
                if (saw_level_overrides) {
                    error_message = "JSON numbering catalog instance member "
                                    "'level_overrides' must not be duplicated";
                    return false;
                }
                saw_level_overrides = true;
                if (!parse_numbering_catalog_level_overrides(
                        content, index, instance.level_overrides, error_message)) {
                    return false;
                }
            } else {
                return report_json_input_error("numbering catalog file", index,
                                               "unknown numbering instance member",
                                               error_message);
            }

            bool closed = false;
            if (!consume_json_numbering_catalog_separator(
                    content, index, '}',
                    "expected ',' or '}' after numbering instance member", closed,
                    error_message)) {
                return false;
            }
            if (closed) {
                break;
            }
        }
    }

    if (!saw_level_overrides) {
        error_message =
            "JSON numbering catalog instance must contain 'level_overrides'";
        return false;
    }

    return true;
}

} // namespace

auto parse_numbering_catalog_instances(
    std::string_view content, std::size_t &index,
    std::vector<featherdoc::numbering_instance_summary> &instances,
    std::string &error_message) -> bool {
    skip_json_patch_whitespace(content, index);
    if (index >= content.size() || content[index] != '[') {
        return report_json_input_error("numbering catalog file", index,
                                       "expected instances array", error_message);
    }

    instances.clear();
    ++index;
    skip_json_patch_whitespace(content, index);
    if (index < content.size() && content[index] == ']') {
        ++index;
        return true;
    }

    while (index < content.size()) {
        featherdoc::numbering_instance_summary instance;
        if (!parse_numbering_catalog_instance(content, index, instance,
                                             error_message)) {
            return false;
        }
        instances.push_back(std::move(instance));

        bool closed = false;
        if (!consume_json_numbering_catalog_separator(
                content, index, ']',
                "expected ',' or ']' after numbering instance", closed,
                error_message)) {
            return false;
        }
        if (closed) {
            return true;
        }
    }

    return report_json_input_error("numbering catalog file", index,
                                   "unterminated instances array", error_message);
}

} // namespace featherdoc_cli
