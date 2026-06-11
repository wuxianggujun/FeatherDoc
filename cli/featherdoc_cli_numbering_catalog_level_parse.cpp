#include "featherdoc_cli_numbering_catalog_level_parse.hpp"

#include "featherdoc_cli_domain_parse.hpp"
#include "featherdoc_cli_json_parse.hpp"
#include "featherdoc_cli_numbering_catalog_parse.hpp"
#include "featherdoc_cli_numbering_catalog_parse_support.hpp"

#include <optional>
#include <string>
#include <utility>

namespace featherdoc_cli {

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

} // namespace featherdoc_cli
