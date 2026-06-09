#include "featherdoc_cli_numbering_catalog_parse.hpp"

#include "featherdoc_cli_domain_parse.hpp"
#include "featherdoc_cli_json_parse.hpp"
#include "featherdoc_cli_numbering_catalog_instance_parse.hpp"
#include "featherdoc_cli_parse.hpp"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace featherdoc_cli {

using path_type = std::filesystem::path;

auto parse_json_numbering_catalog_uint32(
    std::string_view content, std::size_t &index, std::uint32_t &value,
    std::string_view member_name, std::string &error_message) -> bool {
    std::string token;
    skip_json_patch_whitespace(content, index);
    if (index >= content.size()) {
        return report_json_input_error("numbering catalog file", index,
                                       "expected unsigned integer value",
                                       error_message);
    }

    if (content[index] == '"') {
        if (!parse_json_patch_string(content, index, token, error_message)) {
            return false;
        }
    } else if (content[index] >= '0' && content[index] <= '9') {
        if (!parse_json_patch_number(content, index, token, error_message)) {
            return false;
        }
    } else {
        return report_json_input_error("numbering catalog file", index,
                                       "expected unsigned integer value",
                                       error_message);
    }

    if (!parse_uint32(token, value)) {
        error_message = "JSON numbering catalog member '" +
                        std::string(member_name) +
                        "' must be an unsigned integer";
        return false;
    }

    return true;
}

auto parse_json_numbering_catalog_optional_uint32(
    std::string_view content, std::size_t &index,
    std::optional<std::uint32_t> &value, std::string_view member_name,
    std::string &error_message) -> bool {
    skip_json_patch_whitespace(content, index);
    if (index < content.size() && content.substr(index, 4U) == "null") {
        index += 4U;
        value.reset();
        return true;
    }

    std::uint32_t parsed_value = 0U;
    if (!parse_json_numbering_catalog_uint32(content, index, parsed_value,
                                             member_name, error_message)) {
        return false;
    }
    value = parsed_value;
    return true;
}

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

} // namespace

auto parse_numbering_catalog_level_definition(
    std::string_view content, std::size_t &index,
    featherdoc::numbering_level_definition &definition,
    std::string &error_message) -> bool {
    skip_json_patch_whitespace(content, index);
    if (index >= content.size() || content[index] != '{') {
        return report_json_input_error("numbering catalog file", index,
                                       "numbering level definition must be an object",
                                       error_message);
    }

    std::optional<std::uint32_t> level;
    std::optional<featherdoc::list_kind> kind;
    std::optional<std::uint32_t> start;
    std::optional<std::string> text_pattern;

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
                    "expected ':' after numbering level member", error_message);
            }

            ++index;
            if (member_name == "level") {
                if (level.has_value()) {
                    error_message =
                        "JSON numbering catalog level member 'level' must not be duplicated";
                    return false;
                }
                std::uint32_t parsed_level = 0U;
                if (!parse_json_numbering_catalog_uint32(
                        content, index, parsed_level, member_name, error_message)) {
                    return false;
                }
                level = parsed_level;
            } else if (member_name == "kind") {
                if (kind.has_value()) {
                    error_message =
                        "JSON numbering catalog level member 'kind' must not be duplicated";
                    return false;
                }
                std::string kind_text;
                skip_json_patch_whitespace(content, index);
                if (!parse_json_patch_string(content, index, kind_text,
                                             error_message)) {
                    return false;
                }
                featherdoc::list_kind parsed_kind{};
                if (!parse_list_kind(kind_text, parsed_kind)) {
                    error_message =
                        "JSON numbering catalog level member 'kind' must be 'bullet' or 'decimal'";
                    return false;
                }
                kind = parsed_kind;
            } else if (member_name == "start") {
                if (start.has_value()) {
                    error_message =
                        "JSON numbering catalog level member 'start' must not be duplicated";
                    return false;
                }
                std::uint32_t parsed_start = 0U;
                if (!parse_json_numbering_catalog_uint32(
                        content, index, parsed_start, member_name, error_message)) {
                    return false;
                }
                start = parsed_start;
            } else if (member_name == "text_pattern") {
                if (text_pattern.has_value()) {
                    error_message = "JSON numbering catalog level member 'text_pattern' "
                                    "must not be duplicated";
                    return false;
                }
                text_pattern.emplace();
                skip_json_patch_whitespace(content, index);
                if (!parse_json_patch_string(content, index, *text_pattern,
                                             error_message)) {
                    return false;
                }
            } else {
                return report_json_input_error("numbering catalog file", index,
                                               "unknown numbering level member",
                                               error_message);
            }

            bool closed = false;
            if (!consume_json_numbering_catalog_separator(
                    content, index, '}',
                    "expected ',' or '}' after numbering level member", closed,
                    error_message)) {
                return false;
            }
            if (closed) {
                break;
            }
        }
    }

    if (!level.has_value() || !kind.has_value() || !start.has_value() ||
        !text_pattern.has_value()) {
        error_message = "JSON numbering catalog level definition must contain "
                        "'level', 'kind', 'start', and 'text_pattern'";
        return false;
    }

    definition.level = *level;
    definition.kind = *kind;
    definition.start = *start;
    definition.text_pattern = std::move(*text_pattern);
    return true;
}

namespace {

auto parse_numbering_catalog_level_definitions(
    std::string_view content, std::size_t &index,
    std::vector<featherdoc::numbering_level_definition> &levels,
    std::string &error_message) -> bool {
    skip_json_patch_whitespace(content, index);
    if (index >= content.size() || content[index] != '[') {
        return report_json_input_error("numbering catalog file", index,
                                       "expected levels array", error_message);
    }

    levels.clear();
    ++index;
    skip_json_patch_whitespace(content, index);
    if (index < content.size() && content[index] == ']') {
        ++index;
        return true;
    }

    while (index < content.size()) {
        featherdoc::numbering_level_definition level;
        if (!parse_numbering_catalog_level_definition(content, index, level,
                                                      error_message)) {
            return false;
        }
        levels.push_back(std::move(level));

        bool closed = false;
        if (!consume_json_numbering_catalog_separator(
                content, index, ']', "expected ',' or ']' after level definition",
                closed, error_message)) {
            return false;
        }
        if (closed) {
            return true;
        }
    }

    return report_json_input_error("numbering catalog file", index,
                                   "unterminated levels array", error_message);
}

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

} // namespace

auto read_numbering_catalog_file(const path_type &catalog_path,
                                 featherdoc::numbering_catalog &catalog,
                                 std::string &error_message) -> bool {
    std::string content;
    std::size_t index = 0U;
    if (!read_template_table_json_content(catalog_path, content, index,
                                          error_message)) {
        if (error_message.rfind("failed to read JSON patch file:", 0U) == 0U) {
            error_message.replace(0U,
                                  std::string("failed to read JSON patch file:").size(),
                                  "failed to read numbering catalog file:");
        }
        return false;
    }

    skip_json_patch_whitespace(content, index);
    if (index >= content.size() || content[index] != '{') {
        return report_json_input_error("numbering catalog file", index,
                                       "root must be an object", error_message);
    }

    bool saw_definition_count = false;
    bool saw_instance_count = false;
    bool saw_definitions = false;

    catalog.definitions.clear();
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
                    "expected ':' after root object member", error_message);
            }

            ++index;
            if (member_name == "definition_count") {
                if (saw_definition_count) {
                    error_message = "JSON numbering catalog root member "
                                    "'definition_count' must not be duplicated";
                    return false;
                }
                saw_definition_count = true;
                std::uint32_t ignored_count = 0U;
                if (!parse_json_numbering_catalog_uint32(
                        content, index, ignored_count, member_name, error_message)) {
                    return false;
                }
            } else if (member_name == "instance_count") {
                if (saw_instance_count) {
                    error_message = "JSON numbering catalog root member "
                                    "'instance_count' must not be duplicated";
                    return false;
                }
                saw_instance_count = true;
                std::uint32_t ignored_count = 0U;
                if (!parse_json_numbering_catalog_uint32(
                        content, index, ignored_count, member_name, error_message)) {
                    return false;
                }
            } else if (member_name == "definitions") {
                if (saw_definitions) {
                    error_message = "JSON numbering catalog root member 'definitions' "
                                    "must not be duplicated";
                    return false;
                }
                saw_definitions = true;
                if (!parse_numbering_catalog_definitions(
                        content, index, catalog.definitions, error_message)) {
                    return false;
                }
            } else {
                return report_json_input_error("numbering catalog file", index,
                                               "unknown root member", error_message);
            }

            bool closed = false;
            if (!consume_json_numbering_catalog_separator(
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
        return report_json_input_error("numbering catalog file", index,
                                       "unexpected trailing content after root object",
                                       error_message);
    }

    if (!saw_definitions) {
        error_message =
            "JSON numbering catalog file must contain a 'definitions' array";
        return false;
    }

    return true;
}

} // namespace featherdoc_cli
