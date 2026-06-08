#include "featherdoc_cli_template_schema_patch_parse_slot_mutation.hpp"

#include "featherdoc_cli_domain_parse.hpp"

#include "featherdoc_cli_json_parse.hpp"
#include "featherdoc_cli_template_slot_parse.hpp"

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace featherdoc_cli {
namespace detail {
auto parse_template_schema_patch_update_slot(
    std::string_view content, std::size_t &index,
    template_schema_patch_update_slot &operation, std::string &error_message)
    -> bool {
    skip_json_patch_whitespace(content, index);
    if (index >= content.size() || content[index] != '{') {
        return report_json_input_error("JSON schema patch file", index,
                                       "expected update-slot object", error_message);
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
    std::optional<featherdoc::template_slot_kind> slot_kind_value;
    std::optional<bool> required_value;
    std::optional<std::size_t> count_value;
    std::optional<std::size_t> min_value;
    std::optional<std::size_t> min_occurrences_value;
    std::optional<std::size_t> max_value;
    std::optional<std::size_t> max_occurrences_value;
    std::optional<bool> clear_min_value;
    std::optional<bool> clear_min_occurrences_value;
    std::optional<bool> clear_max_value;
    std::optional<bool> clear_max_occurrences_value;

    const auto parse_string_member = [&](std::string_view member_name,
                                         std::optional<std::string> &value) -> bool {
        if (value.has_value()) {
            error_message = "JSON schema patch member '" + std::string(member_name) +
                            "' must not be duplicated";
            return false;
        }
        skip_json_patch_whitespace(content, index);
        value.emplace();
        return parse_json_patch_string(content, index, *value, error_message);
    };
    const auto parse_index_member = [&](std::string_view member_name,
                                        std::optional<std::size_t> &value) -> bool {
        if (value.has_value()) {
            error_message = "JSON schema patch update-slot member '" +
                            std::string(member_name) + "' must not be duplicated";
            return false;
        }
        std::size_t parsed_index = 0U;
        if (!parse_json_patch_index_value(content, index, parsed_index, member_name,
                                          error_message)) {
            return false;
        }
        value = parsed_index;
        return true;
    };
    const auto parse_clear_bool = [&](std::string_view member_name,
                                      std::optional<bool> &value) -> bool {
        if (value.has_value()) {
            error_message = "JSON schema patch update-slot member '" +
                            std::string(member_name) + "' must not be duplicated";
            return false;
        }
        bool parsed_value = false;
        if (!parse_json_patch_bool_member_value(content, index, parsed_value,
                                                member_name, error_message)) {
            return false;
        }
        value = parsed_value;
        return true;
    };

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
                if (!parse_string_member(member_name, part_value)) {
                    return false;
                }
            } else if (member_name == "index") {
                if (index_value.has_value()) {
                    error_message = "JSON schema patch member 'index' must not be duplicated";
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
                    error_message = "JSON schema patch member 'part_index' must not be duplicated";
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
                    error_message = "JSON schema patch member 'section' must not be duplicated";
                    return false;
                }
                std::size_t parsed_index = 0U;
                if (!parse_json_patch_index_value(content, index, parsed_index,
                                                  "section", error_message)) {
                    return false;
                }
                section_value = parsed_index;
            } else if (member_name == "kind") {
                skip_json_patch_whitespace(content, index);
                std::string token;
                if (!parse_json_patch_string(content, index, token, error_message)) {
                    return false;
                }
                featherdoc::section_reference_kind parsed_reference_kind{};
                featherdoc::template_slot_kind parsed_slot_kind{};
                if (parse_reference_kind(token, parsed_reference_kind)) {
                    if (kind_value.has_value()) {
                        error_message = "JSON schema patch member 'kind' must not be duplicated";
                        return false;
                    }
                    kind_value = parsed_reference_kind;
                } else if (parse_template_slot_kind(token, parsed_slot_kind)) {
                    if (slot_kind_value.has_value()) {
                        error_message = "JSON schema patch update-slot member 'kind' must not be duplicated";
                        return false;
                    }
                    slot_kind_value = parsed_slot_kind;
                } else {
                    error_message = "JSON schema patch update-slot member 'kind' must be a section reference kind or template slot kind";
                    return false;
                }
            } else if (member_name == "target_kind" || member_name == "reference_kind") {
                if (kind_value.has_value()) {
                    error_message = "JSON schema patch member '" + member_name + "' must not be duplicated";
                    return false;
                }
                featherdoc::section_reference_kind parsed_kind{};
                if (!parse_json_patch_reference_kind_value(content, index, parsed_kind,
                                                           error_message)) {
                    return false;
                }
                kind_value = parsed_kind;
            } else if (member_name == "slot_kind") {
                if (slot_kind_value.has_value()) {
                    error_message = "JSON schema patch update-slot member 'slot_kind' must not be duplicated";
                    return false;
                }
                std::string token;
                skip_json_patch_whitespace(content, index);
                if (!parse_json_patch_string(content, index, token, error_message)) {
                    return false;
                }
                featherdoc::template_slot_kind parsed_kind{};
                if (!parse_template_slot_kind(token, parsed_kind)) {
                    error_message = "JSON schema patch update-slot member 'slot_kind' must be one of 'text', 'table_rows', 'table', 'image', 'floating_image', or 'block'";
                    return false;
                }
                slot_kind_value = parsed_kind;
            } else if (member_name == "linked_to_previous") {
                if (linked_to_previous_value.has_value()) {
                    error_message = "JSON schema patch member 'linked_to_previous' must not be duplicated";
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
                    error_message = "JSON schema patch member 'resolved_from_section' must not be duplicated";
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
                if (!parse_string_member(member_name, bookmark_value)) {
                    return false;
                }
            } else if (member_name == "bookmark_name") {
                if (!parse_string_member(member_name, bookmark_name_value)) {
                    return false;
                }
            } else if (member_name == "content_control_tag") {
                if (!parse_string_member(member_name, content_control_tag_value)) {
                    return false;
                }
            } else if (member_name == "content_control_alias") {
                if (!parse_string_member(member_name, content_control_alias_value)) {
                    return false;
                }
            } else if (member_name == "required") {
                if (required_value.has_value()) {
                    error_message = "JSON schema patch update-slot member 'required' must not be duplicated";
                    return false;
                }
                bool parsed_required = true;
                if (!parse_json_patch_bool_member_value(content, index,
                                                        parsed_required,
                                                        "required",
                                                        error_message)) {
                    return false;
                }
                required_value = parsed_required;
            } else if (member_name == "count") {
                if (!parse_index_member(member_name, count_value)) {
                    return false;
                }
            } else if (member_name == "min") {
                if (!parse_index_member(member_name, min_value)) {
                    return false;
                }
            } else if (member_name == "min_occurrences") {
                if (!parse_index_member(member_name, min_occurrences_value)) {
                    return false;
                }
            } else if (member_name == "max") {
                if (!parse_index_member(member_name, max_value)) {
                    return false;
                }
            } else if (member_name == "max_occurrences") {
                if (!parse_index_member(member_name, max_occurrences_value)) {
                    return false;
                }
            } else if (member_name == "clear_min") {
                if (!parse_clear_bool(member_name, clear_min_value)) {
                    return false;
                }
            } else if (member_name == "clear_min_occurrences") {
                if (!parse_clear_bool(member_name, clear_min_occurrences_value)) {
                    return false;
                }
            } else if (member_name == "clear_max") {
                if (!parse_clear_bool(member_name, clear_max_value)) {
                    return false;
                }
            } else if (member_name == "clear_max_occurrences") {
                if (!parse_clear_bool(member_name, clear_max_occurrences_value)) {
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
                                               "unknown update-slot member",
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
            "JSON schema patch update-slot object must contain exactly one of "
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
        error_message = "JSON schema patch update-slot selector must not be empty";
        return false;
    }

    std::optional<std::size_t> resolved_min;
    if (!resolve_json_patch_index_member(min_value, min_occurrences_value,
                                         "min", "min_occurrences", resolved_min,
                                         error_message)) {
        return false;
    }
    std::optional<std::size_t> resolved_max;
    if (!resolve_json_patch_index_member(max_value, max_occurrences_value,
                                         "max", "max_occurrences", resolved_max,
                                         error_message)) {
        return false;
    }
    const auto resolve_bool_alias = [&](const std::optional<bool> &primary,
                                        const std::optional<bool> &alias,
                                        std::string_view primary_name,
                                        std::string_view alias_name,
                                        std::optional<bool> &resolved) -> bool {
        if (primary.has_value() && alias.has_value() && *primary != *alias) {
            error_message = "JSON patch members '" + std::string(primary_name) +
                            "' and '" + std::string(alias_name) +
                            "' must match when both are present";
            return false;
        }
        resolved = primary.has_value() ? primary : alias;
        return true;
    };
    std::optional<bool> resolved_clear_min;
    std::optional<bool> resolved_clear_max;
    if (!resolve_bool_alias(clear_min_value, clear_min_occurrences_value,
                            "clear_min", "clear_min_occurrences",
                            resolved_clear_min) ||
        !resolve_bool_alias(clear_max_value, clear_max_occurrences_value,
                            "clear_max", "clear_max_occurrences",
                            resolved_clear_max)) {
        return false;
    }

    if (count_value.has_value() &&
        (resolved_min.has_value() || resolved_max.has_value())) {
        error_message = "JSON schema patch update-slot member 'count' conflicts with 'min'/'max'";
        return false;
    }
    if (count_value.has_value() &&
        (resolved_clear_min.value_or(false) || resolved_clear_max.value_or(false))) {
        error_message = "JSON schema patch update-slot member 'count' conflicts with clear occurrence flags";
        return false;
    }
    if (resolved_min.has_value() && resolved_clear_min.value_or(false)) {
        error_message = "JSON schema patch update-slot member 'min' conflicts with 'clear_min_occurrences'";
        return false;
    }
    if (resolved_max.has_value() && resolved_clear_max.value_or(false)) {
        error_message = "JSON schema patch update-slot member 'max' conflicts with 'clear_max_occurrences'";
        return false;
    }
    if (resolved_min.has_value() && resolved_max.has_value() &&
        *resolved_max < *resolved_min) {
        error_message =
            "JSON schema patch update-slot occurrence range is invalid: max must be greater than or equal to min";
        return false;
    }

    operation.bookmark_name = std::move(resolved_slot_name);
    operation.source = source;
    operation.update.kind = slot_kind_value;
    operation.update.required = required_value;
    if (count_value.has_value()) {
        operation.update.min_occurrences = *count_value;
        operation.update.max_occurrences = *count_value;
    } else {
        operation.update.min_occurrences = resolved_min;
        operation.update.max_occurrences = resolved_max;
    }
    operation.update.clear_min_occurrences = resolved_clear_min.value_or(false);
    operation.update.clear_max_occurrences = resolved_clear_max.value_or(false);

    if (!operation.update.kind.has_value() &&
        !operation.update.required.has_value() &&
        !operation.update.min_occurrences.has_value() &&
        !operation.update.max_occurrences.has_value() &&
        !operation.update.clear_min_occurrences &&
        !operation.update.clear_max_occurrences) {
        error_message = "JSON schema patch update-slot object must contain at least one update field";
        return false;
    }

    return true;
}

auto parse_template_schema_patch_update_slots_array(
    std::string_view content, std::size_t &index,
    std::vector<template_schema_patch_update_slot> &operations,
    std::string &error_message) -> bool {
    skip_json_patch_whitespace(content, index);
    if (index >= content.size() || content[index] != '[') {
        return report_json_input_error("JSON schema patch file", index,
                                       "expected update_slots array",
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
        template_schema_patch_update_slot operation;
        if (!parse_template_schema_patch_update_slot(content, index, operation,
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
            "expected ',' or ']' after update-slot entry", error_message);
    }

    return report_json_input_error("JSON schema patch file", index,
                                   "unterminated update_slots array",
                                   error_message);
}

} // namespace detail
} // namespace featherdoc_cli