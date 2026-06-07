#include "featherdoc_cli_template_schema_patch_parse_detail.hpp"

#include "featherdoc_cli_domain_parse.hpp"
#include "featherdoc_cli_json_parse.hpp"
#include "featherdoc_cli_template_schema_parse.hpp"
#include "featherdoc_cli_template_slot_parse.hpp"
#include "featherdoc_cli_validation_part.hpp"

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace featherdoc_cli {
namespace detail {

void rewrite_template_schema_patch_error(std::string &error_message) {
    constexpr std::string_view schema_prefix = "JSON schema ";
    constexpr std::string_view patch_prefix = "JSON schema patch ";
    std::size_t position = 0U;
    while ((position = error_message.find(schema_prefix, position)) !=
           std::string::npos) {
        error_message.replace(position, schema_prefix.size(), patch_prefix);
        position += patch_prefix.size();
    }
}

auto finalize_template_schema_patch_selector(
    const std::optional<std::string> &part_value,
    const std::optional<std::size_t> &index_value,
    const std::optional<std::size_t> &part_index_value,
    const std::optional<std::size_t> &section_value,
    const std::optional<featherdoc::section_reference_kind> &kind_value,
    const std::optional<std::size_t> &resolved_from_section_value,
    const std::optional<bool> &linked_to_previous_value,
    template_schema_patch_target_selector &selector, std::string &error_message)
    -> bool {
    if (!part_value.has_value()) {
        error_message = "JSON schema patch selector must contain 'part'";
        return false;
    }

    selector = {};
    if (!parse_validation_part(*part_value, selector.part)) {
        error_message =
            "JSON schema patch member 'part' must be one of "
            "'body', 'header', 'footer', 'section-header', or 'section-footer'";
        return false;
    }

    std::optional<std::size_t> resolved_part_index;
    if (!resolve_json_patch_index_member(index_value, part_index_value, "index",
                                         "part_index", resolved_part_index,
                                         error_message)) {
        return false;
    }

    selector.part_index = resolved_part_index;
    selector.section_index = section_value;
    selector.reference_kind = kind_value;
    selector.resolved_from_section_index = resolved_from_section_value;
    selector.linked_to_previous = linked_to_previous_value.value_or(false);

    return validate_template_part_selection(selector.part, selector.part_index,
                                            selector.section_index,
                                            selector.reference_kind.has_value(),
                                            "schema patch selection",
                                            error_message);
}

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
                if (!parse_json_patch_index_value(content, index, parsed_index, "index",
                                                  error_message)) {
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
                if (!parse_json_patch_string(content, index, ignored, error_message)) {
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

auto parse_template_schema_patch_remove_slot(
    std::string_view content, std::size_t &index,
    template_schema_patch_remove_slot &operation, std::string &error_message)
    -> bool {
    skip_json_patch_whitespace(content, index);
    if (index >= content.size() || content[index] != '{') {
        return report_json_input_error("JSON schema patch file", index,
                                       "expected remove-slot object", error_message);
    }

    std::optional<std::string> part_value;
    std::optional<std::size_t> index_value;
    std::optional<std::size_t> part_index_value;
    std::optional<std::size_t> section_value;
    std::optional<featherdoc::section_reference_kind> kind_value;
    std::optional<std::size_t> resolved_from_section_value;
    std::optional<bool> linked_to_previous_value;
    std::optional<std::string> bookmark_value;
    std::optional<std::string> bookmark_name_value;
    std::optional<std::string> content_control_tag_value;
    std::optional<std::string> content_control_alias_value;

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
                if (!parse_json_patch_index_value(content, index, parsed_index, "index",
                                                  error_message)) {
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
            } else if (member_name == "bookmark") {
                if (bookmark_value.has_value()) {
                    error_message = "JSON schema patch member 'bookmark' must not "
                                    "be duplicated";
                    return false;
                }
                skip_json_patch_whitespace(content, index);
                bookmark_value.emplace();
                if (!parse_json_patch_string(content, index, *bookmark_value,
                                             error_message)) {
                    return false;
                }
            } else if (member_name == "bookmark_name") {
                if (bookmark_name_value.has_value()) {
                    error_message = "JSON schema patch member 'bookmark_name' "
                                    "must not be duplicated";
                    return false;
                }
                skip_json_patch_whitespace(content, index);
                bookmark_name_value.emplace();
                if (!parse_json_patch_string(content, index, *bookmark_name_value,
                                             error_message)) {
                    return false;
                }
            } else if (member_name == "content_control_tag") {
                if (content_control_tag_value.has_value()) {
                    error_message = "JSON schema patch member 'content_control_tag' "
                                    "must not be duplicated";
                    return false;
                }
                skip_json_patch_whitespace(content, index);
                content_control_tag_value.emplace();
                if (!parse_json_patch_string(content, index,
                                             *content_control_tag_value,
                                             error_message)) {
                    return false;
                }
            } else if (member_name == "content_control_alias") {
                if (content_control_alias_value.has_value()) {
                    error_message = "JSON schema patch member 'content_control_alias' "
                                    "must not be duplicated";
                    return false;
                }
                skip_json_patch_whitespace(content, index);
                content_control_alias_value.emplace();
                if (!parse_json_patch_string(content, index,
                                             *content_control_alias_value,
                                             error_message)) {
                    return false;
                }
            } else if (member_name == "entry_name") {
                std::string ignored;
                skip_json_patch_whitespace(content, index);
                if (!parse_json_patch_string(content, index, ignored, error_message)) {
                    return false;
                }
            } else {
                return report_json_input_error("JSON schema patch file", index,
                                               "unknown remove-slot member",
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

    operation = {};
    if (!finalize_template_schema_patch_selector(
            part_value, index_value, part_index_value, section_value, kind_value,
            resolved_from_section_value, linked_to_previous_value, operation.target,
            error_message)) {
        return false;
    }

    std::optional<std::string> resolved_bookmark_name;
    if (!resolve_json_patch_string_member(bookmark_value, bookmark_name_value,
                                          "bookmark", "bookmark_name",
                                          resolved_bookmark_name, error_message)) {
        return false;
    }
    std::size_t selector_count = 0U;
    selector_count += resolved_bookmark_name.has_value() ? 1U : 0U;
    selector_count += content_control_tag_value.has_value() ? 1U : 0U;
    selector_count += content_control_alias_value.has_value() ? 1U : 0U;
    if (selector_count != 1U) {
        error_message =
            "JSON schema patch remove-slot object must contain exactly one of "
            "'bookmark', 'bookmark_name', 'content_control_tag', or "
            "'content_control_alias'";
        return false;
    }

    auto source = featherdoc::template_slot_source_kind::bookmark;
    std::string resolved_slot_name;
    if (resolved_bookmark_name.has_value()) {
        resolved_slot_name = *resolved_bookmark_name;
    } else if (content_control_tag_value.has_value()) {
        source = featherdoc::template_slot_source_kind::content_control_tag;
        resolved_slot_name = *content_control_tag_value;
    } else {
        source = featherdoc::template_slot_source_kind::content_control_alias;
        resolved_slot_name = *content_control_alias_value;
    }
    if (resolved_slot_name.empty()) {
        error_message = "JSON schema patch remove-slot selector must not be empty";
        return false;
    }

    operation.bookmark_name = std::move(resolved_slot_name);
    operation.source = source;
    return true;
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

auto parse_template_schema_patch_remove_slots_array(
    std::string_view content, std::size_t &index,
    std::vector<template_schema_patch_remove_slot> &operations,
    std::string &error_message) -> bool {
    skip_json_patch_whitespace(content, index);
    if (index >= content.size() || content[index] != '[') {
        return report_json_input_error("JSON schema patch file", index,
                                       "expected remove_slots array",
                                       error_message);
    }

    operations.clear();
    ++index;
    skip_json_patch_whitespace(content, index);
    if (index < content.size() && content[index] == ']') {
        ++index;
        return true;
    }

    while (index < content.size()) {
        template_schema_patch_remove_slot operation;
        if (!parse_template_schema_patch_remove_slot(content, index, operation,
                                                     error_message)) {
            return false;
        }
        operations.push_back(std::move(operation));

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
        return report_json_input_error(
            "JSON schema patch file", index,
            "expected ',' or ']' after remove-slot entry", error_message);
    }

    return report_json_input_error("JSON schema patch file", index,
                                   "unterminated remove_slots array",
                                   error_message);
}


} // namespace detail
} // namespace featherdoc_cli