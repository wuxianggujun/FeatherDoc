#include "featherdoc_cli_template_schema_patch_parse_detail.hpp"

#include "featherdoc_cli_domain_parse.hpp"
#include "featherdoc_cli_json_parse.hpp"

#include <cstddef>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace featherdoc_cli::detail {

auto parse_template_schema_patch_target_selector(
    std::string_view content, std::size_t &index,
    template_schema_patch_target_selector &selector, std::string &error_message)
    -> bool {
    skip_json_patch_whitespace(content, index);
    if (index >= content.size() || content[index] != '{') {
        return report_json_input_error("JSON schema patch file", index,
                                       "expected selector object", error_message);
    }

    std::optional<std::string> part_value;
    std::optional<std::size_t> index_value;
    std::optional<std::size_t> part_index_value;
    std::optional<std::size_t> section_value;
    std::optional<featherdoc::section_reference_kind> kind_value;
    std::optional<std::size_t> resolved_from_section_value;
    std::optional<bool> linked_to_previous_value;

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
                return report_json_input_error("JSON schema patch file", index,
                                               "expected ':' after object member",
                                               error_message);
            }

            ++index;
            if (member_name == "part") {
                if (part_value.has_value()) {
                    error_message =
                        "JSON schema patch member 'part' must not be duplicated";
                    return false;
                }
                skip_json_patch_whitespace(content, index);
                part_value.emplace();
                if (!parse_json_patch_string(content, index, *part_value,
                                             error_message)) {
                    return false;
                }
            } else if (member_name == "index") {
                if (index_value.has_value()) {
                    error_message =
                        "JSON schema patch member 'index' must not be duplicated";
                    return false;
                }
                std::size_t parsed_index = 0U;
                if (!parse_json_patch_index_value(content, index, parsed_index,
                                                  "index", error_message)) {
                    return false;
                }
                index_value = parsed_index;
            } else if (member_name == "part_index") {
                if (part_index_value.has_value()) {
                    error_message = "JSON schema patch member 'part_index' must "
                                    "not be duplicated";
                    return false;
                }
                std::size_t parsed_index = 0U;
                if (!parse_json_patch_index_value(content, index, parsed_index,
                                                  "part_index", error_message)) {
                    return false;
                }
                part_index_value = parsed_index;
            } else if (member_name == "section") {
                if (section_value.has_value()) {
                    error_message =
                        "JSON schema patch member 'section' must not be duplicated";
                    return false;
                }
                std::size_t parsed_index = 0U;
                if (!parse_json_patch_index_value(content, index, parsed_index,
                                                  "section", error_message)) {
                    return false;
                }
                section_value = parsed_index;
            } else if (member_name == "kind") {
                if (kind_value.has_value()) {
                    error_message =
                        "JSON schema patch member 'kind' must not be duplicated";
                    return false;
                }
                featherdoc::section_reference_kind parsed_kind{};
                if (!parse_json_patch_reference_kind_value(content, index,
                                                           parsed_kind,
                                                           error_message)) {
                    return false;
                }
                kind_value = parsed_kind;
            } else if (member_name == "linked_to_previous") {
                if (linked_to_previous_value.has_value()) {
                    error_message = "JSON schema patch member "
                                    "'linked_to_previous' must not be duplicated";
                    return false;
                }
                bool parsed_value = false;
                if (!parse_json_patch_bool_member_value(content, index, parsed_value,
                                                        "linked_to_previous",
                                                        error_message)) {
                    return false;
                }
                linked_to_previous_value = parsed_value;
            } else if (member_name == "resolved_from_section") {
                if (resolved_from_section_value.has_value()) {
                    error_message = "JSON schema patch member "
                                    "'resolved_from_section' must not be duplicated";
                    return false;
                }
                std::size_t parsed_value = 0U;
                if (!parse_json_patch_index_value(content, index, parsed_value,
                                                  "resolved_from_section",
                                                  error_message)) {
                    return false;
                }
                resolved_from_section_value = parsed_value;
            } else if (member_name == "entry_name") {
                std::string ignored;
                skip_json_patch_whitespace(content, index);
                if (!parse_json_patch_string(content, index, ignored,
                                             error_message)) {
                    return false;
                }
            } else {
                return report_json_input_error("JSON schema patch file", index,
                                               "unknown selector member",
                                               error_message);
            }

            skip_json_patch_whitespace(content, index);
            if (index >= content.size()) {
                break;
            }
            if (content[index] == ',') {
                ++index;
                skip_json_patch_whitespace(content, index);
                continue;
            }
            if (content[index] == '}') {
                ++index;
                break;
            }
            return report_json_input_error("JSON schema patch file", index,
                                           "expected ',' or '}' after object member",
                                           error_message);
        }
    }

    return finalize_template_schema_patch_selector(
        part_value, index_value, part_index_value, section_value, kind_value,
        resolved_from_section_value, linked_to_previous_value, selector,
        error_message);
}

auto parse_template_schema_patch_target_selectors_array(
    std::string_view content, std::size_t &index,
    std::vector<template_schema_patch_target_selector> &selectors,
    std::string &error_message) -> bool {
    skip_json_patch_whitespace(content, index);
    if (index >= content.size() || content[index] != '[') {
        return report_json_input_error("JSON schema patch file", index,
                                       "expected selector array", error_message);
    }

    selectors.clear();
    ++index;
    skip_json_patch_whitespace(content, index);
    if (index < content.size() && content[index] == ']') {
        ++index;
        return true;
    }

    while (index < content.size()) {
        template_schema_patch_target_selector selector;
        if (!parse_template_schema_patch_target_selector(content, index, selector,
                                                         error_message)) {
            return false;
        }
        selectors.push_back(std::move(selector));

        skip_json_patch_whitespace(content, index);
        if (index >= content.size()) {
            break;
        }
        if (content[index] == ',') {
            ++index;
            skip_json_patch_whitespace(content, index);
            continue;
        }
        if (content[index] == ']') {
            ++index;
            return true;
        }
        return report_json_input_error("JSON schema patch file", index,
                                       "expected ',' or ']' after selector entry",
                                       error_message);
    }

    return report_json_input_error("JSON schema patch file", index,
                                   "unterminated selector array", error_message);
}

} // namespace featherdoc_cli::detail
