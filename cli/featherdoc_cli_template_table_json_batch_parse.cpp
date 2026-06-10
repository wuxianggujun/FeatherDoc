#include "featherdoc_cli_template_table_json_patch_parse.hpp"

#include "featherdoc_cli_json_parse.hpp"
#include "featherdoc_cli_template_inspect_options_parse.hpp"
#include "featherdoc_cli_template_table_json_batch_operation_parse.hpp"

#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace featherdoc_cli {

using path_type = std::filesystem::path;

auto read_template_table_json_batch(
    const path_type &patch_path,
    std::vector<template_table_json_batch_operation> &operations,
    std::string &error_message) -> bool {
    std::string content;
    std::size_t index = 0U;
    if (!read_template_table_json_content(patch_path, content, index,
                                          error_message)) {
        return false;
    }

    skip_json_patch_whitespace(content, index);
    if (index >= content.size() || content[index] != '{') {
        return report_json_patch_error(index,
                                       "JSON patch root must be an object",
                                       error_message);
    }

    bool saw_operations = false;
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
                return report_json_patch_error(
                    index, "expected ':' after object member", error_message);
            }

            ++index;
            if (member_name == "operations") {
                if (saw_operations) {
                    error_message =
                        "JSON patch member 'operations' must not be duplicated";
                    return false;
                }
                if (!parse_template_table_json_batch_operations_array(
                        content, index, operations, error_message)) {
                    return false;
                }
                saw_operations = true;
            } else if (!skip_json_patch_value(content, index, error_message)) {
                return false;
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
            return report_json_patch_error(index,
                                           "expected ',' or '}' after object "
                                           "member",
                                           error_message);
        }
    }

    skip_json_patch_whitespace(content, index);
    if (index != content.size()) {
        return report_json_patch_error(index,
                                       "unexpected trailing content after JSON "
                                       "patch object",
                                       error_message);
    }

    if (!saw_operations) {
        error_message = "JSON patch must contain an 'operations' array";
        return false;
    }
    if (operations.empty()) {
        error_message =
            "JSON patch member 'operations' must contain at least one operation";
        return false;
    }

    return true;
}

auto resolve_template_table_json_batch_operation_selection(
    const template_table_json_batch_options &options,
    const template_table_json_batch_operation &operation,
    validation_part_family &part, std::optional<std::size_t> &part_index,
    std::optional<std::size_t> &section_index,
    featherdoc::section_reference_kind &reference_kind, bool &has_kind,
    featherdoc::template_table_selector &selector, std::string &error_message)
    -> bool {
    part = operation.part.value_or(options.part);
    part_index =
        operation.part_index.has_value() ? operation.part_index : options.part_index;
    section_index = operation.section_index.has_value()
                        ? operation.section_index
                        : options.section_index;
    reference_kind = operation.reference_kind.value_or(options.reference_kind);
    has_kind = operation.reference_kind.has_value() || options.has_kind;
    selector = operation.selector;

    if (!validate_template_table_selector(selector, false, false, false,
                                          error_message)) {
        return false;
    }

    return validate_template_part_selection(part, part_index, section_index,
                                            has_kind, "mutation", error_message);
}

} // namespace featherdoc_cli
