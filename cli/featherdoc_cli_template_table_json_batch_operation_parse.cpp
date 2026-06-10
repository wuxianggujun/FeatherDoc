#include "featherdoc_cli_template_table_json_batch_operation_parse.hpp"

#include "featherdoc_cli_domain_parse.hpp"
#include "featherdoc_cli_json_parse.hpp"
#include "featherdoc_cli_template_inspect_options_parse.hpp"
#include "featherdoc_cli_template_table_json_patch_parse.hpp"
#include "featherdoc_cli_template_table_json_patch_parse_detail.hpp"

#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace featherdoc_cli {

namespace {

auto parse_template_table_json_batch_operation(
    std::string_view content, std::size_t &index,
    template_table_json_batch_operation &operation,
    std::string &error_message) -> bool {
    skip_json_patch_whitespace(content, index);
    if (index >= content.size() || content[index] != '{') {
        return report_json_patch_error(index, "expected operation object",
                                       error_message);
    }

    std::optional<std::string> bookmark_value;
    std::optional<std::string> bookmark_name_value;
    std::optional<std::size_t> table_index_value;
    std::optional<std::string> after_text_value;
    std::optional<std::string> after_paragraph_text_value;
    std::vector<std::string> header_cells_value;
    std::vector<std::string> header_cell_texts_value;
    bool saw_header_cells = false;
    bool saw_header_cell_texts = false;
    std::optional<std::size_t> header_row_value;
    std::optional<std::size_t> header_row_index_value;
    std::optional<std::size_t> occurrence_value;
    std::optional<validation_part_family> part_value;
    std::optional<std::size_t> index_value;
    std::optional<std::size_t> part_index_value;
    std::optional<std::size_t> section_value;
    std::optional<std::size_t> section_index_value;
    std::optional<featherdoc::section_reference_kind> reference_kind_value;
    std::optional<std::string> mode_value;
    std::optional<std::size_t> start_row_value;
    std::optional<std::size_t> start_row_index_value;
    std::optional<std::size_t> start_cell_value;
    std::optional<std::size_t> start_cell_index_value;
    std::vector<std::vector<std::string>> rows;
    bool saw_rows = false;

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
            if (member_name == "bookmark") {
                if (bookmark_value.has_value()) {
                    error_message =
                        "JSON patch member 'bookmark' must not be duplicated";
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
                    error_message = "JSON patch member 'bookmark_name' must not "
                                    "be duplicated";
                    return false;
                }
                skip_json_patch_whitespace(content, index);
                bookmark_name_value.emplace();
                if (!parse_json_patch_string(content, index,
                                             *bookmark_name_value,
                                             error_message)) {
                    return false;
                }
            } else if (member_name == "table_index") {
                if (table_index_value.has_value()) {
                    error_message =
                        "JSON patch member 'table_index' must not be duplicated";
                    return false;
                }
                std::size_t parsed_index = 0U;
                if (!parse_json_patch_index_value(content, index, parsed_index,
                                                  "table_index",
                                                  error_message)) {
                    return false;
                }
                table_index_value = parsed_index;
            } else if (member_name == "after_text") {
                if (after_text_value.has_value()) {
                    error_message =
                        "JSON patch member 'after_text' must not be duplicated";
                    return false;
                }
                skip_json_patch_whitespace(content, index);
                after_text_value.emplace();
                if (!parse_json_patch_string(content, index, *after_text_value,
                                             error_message)) {
                    return false;
                }
            } else if (member_name == "after_paragraph_text") {
                if (after_paragraph_text_value.has_value()) {
                    error_message = "JSON patch member 'after_paragraph_text' "
                                    "must not be duplicated";
                    return false;
                }
                skip_json_patch_whitespace(content, index);
                after_paragraph_text_value.emplace();
                if (!parse_json_patch_string(content, index,
                                             *after_paragraph_text_value,
                                             error_message)) {
                    return false;
                }
            } else if (member_name == "header_cells") {
                if (saw_header_cells) {
                    error_message = "JSON patch member 'header_cells' must not "
                                    "be duplicated";
                    return false;
                }
                if (!parse_json_patch_text_array(content, index,
                                                 header_cells_value,
                                                 "header_cells",
                                                 error_message)) {
                    return false;
                }
                saw_header_cells = true;
            } else if (member_name == "header_cell_texts") {
                if (saw_header_cell_texts) {
                    error_message = "JSON patch member 'header_cell_texts' must "
                                    "not be duplicated";
                    return false;
                }
                if (!parse_json_patch_text_array(content, index,
                                                 header_cell_texts_value,
                                                 "header_cell_texts",
                                                 error_message)) {
                    return false;
                }
                saw_header_cell_texts = true;
            } else if (member_name == "header_row") {
                if (header_row_value.has_value()) {
                    error_message =
                        "JSON patch member 'header_row' must not be duplicated";
                    return false;
                }
                std::size_t parsed_index = 0U;
                if (!parse_json_patch_index_value(content, index, parsed_index,
                                                  "header_row",
                                                  error_message)) {
                    return false;
                }
                header_row_value = parsed_index;
            } else if (member_name == "header_row_index") {
                if (header_row_index_value.has_value()) {
                    error_message = "JSON patch member 'header_row_index' must "
                                    "not be duplicated";
                    return false;
                }
                std::size_t parsed_index = 0U;
                if (!parse_json_patch_index_value(content, index, parsed_index,
                                                  "header_row_index",
                                                  error_message)) {
                    return false;
                }
                header_row_index_value = parsed_index;
            } else if (member_name == "occurrence") {
                if (occurrence_value.has_value()) {
                    error_message =
                        "JSON patch member 'occurrence' must not be duplicated";
                    return false;
                }
                std::size_t parsed_index = 0U;
                if (!parse_json_patch_index_value(content, index, parsed_index,
                                                  "occurrence",
                                                  error_message)) {
                    return false;
                }
                occurrence_value = parsed_index;
            } else if (member_name == "part") {
                if (part_value.has_value()) {
                    error_message =
                        "JSON patch member 'part' must not be duplicated";
                    return false;
                }
                validation_part_family parsed_part =
                    validation_part_family::body;
                if (!parse_json_patch_part_value(content, index, parsed_part,
                                                 error_message)) {
                    return false;
                }
                part_value = parsed_part;
            } else if (member_name == "index") {
                if (index_value.has_value()) {
                    error_message =
                        "JSON patch member 'index' must not be duplicated";
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
                    error_message = "JSON patch member 'part_index' must not be "
                                    "duplicated";
                    return false;
                }
                std::size_t parsed_index = 0U;
                if (!parse_json_patch_index_value(content, index, parsed_index,
                                                  "part_index",
                                                  error_message)) {
                    return false;
                }
                part_index_value = parsed_index;
            } else if (member_name == "section") {
                if (section_value.has_value()) {
                    error_message =
                        "JSON patch member 'section' must not be duplicated";
                    return false;
                }
                std::size_t parsed_index = 0U;
                if (!parse_json_patch_index_value(content, index, parsed_index,
                                                  "section", error_message)) {
                    return false;
                }
                section_value = parsed_index;
            } else if (member_name == "section_index") {
                if (section_index_value.has_value()) {
                    error_message = "JSON patch member 'section_index' must not "
                                    "be duplicated";
                    return false;
                }
                std::size_t parsed_index = 0U;
                if (!parse_json_patch_index_value(content, index, parsed_index,
                                                  "section_index",
                                                  error_message)) {
                    return false;
                }
                section_index_value = parsed_index;
            } else if (member_name == "kind") {
                if (reference_kind_value.has_value()) {
                    error_message =
                        "JSON patch member 'kind' must not be duplicated";
                    return false;
                }
                featherdoc::section_reference_kind parsed_reference_kind =
                    featherdoc::section_reference_kind::default_reference;
                if (!parse_json_patch_reference_kind_value(
                        content, index, parsed_reference_kind, error_message)) {
                    return false;
                }
                reference_kind_value = parsed_reference_kind;
            } else if (member_name == "mode") {
                if (mode_value.has_value()) {
                    error_message =
                        "JSON patch member 'mode' must not be duplicated";
                    return false;
                }
                skip_json_patch_whitespace(content, index);
                mode_value.emplace();
                if (!parse_json_patch_string(content, index, *mode_value,
                                             error_message)) {
                    return false;
                }
            } else if (member_name == "start_row") {
                if (start_row_value.has_value()) {
                    error_message =
                        "JSON patch member 'start_row' must not be duplicated";
                    return false;
                }
                std::size_t parsed_index = 0U;
                if (!parse_json_patch_index_value(content, index, parsed_index,
                                                  "start_row",
                                                  error_message)) {
                    return false;
                }
                start_row_value = parsed_index;
            } else if (member_name == "start_row_index") {
                if (start_row_index_value.has_value()) {
                    error_message =
                        "JSON patch member 'start_row_index' must not be duplicated";
                    return false;
                }
                std::size_t parsed_index = 0U;
                if (!parse_json_patch_index_value(content, index, parsed_index,
                                                  "start_row_index",
                                                  error_message)) {
                    return false;
                }
                start_row_index_value = parsed_index;
            } else if (member_name == "start_cell") {
                if (start_cell_value.has_value()) {
                    error_message =
                        "JSON patch member 'start_cell' must not be duplicated";
                    return false;
                }
                std::size_t parsed_index = 0U;
                if (!parse_json_patch_index_value(content, index, parsed_index,
                                                  "start_cell",
                                                  error_message)) {
                    return false;
                }
                start_cell_value = parsed_index;
            } else if (member_name == "start_cell_index") {
                if (start_cell_index_value.has_value()) {
                    error_message = "JSON patch member 'start_cell_index' must "
                                    "not be duplicated";
                    return false;
                }
                std::size_t parsed_index = 0U;
                if (!parse_json_patch_index_value(content, index, parsed_index,
                                                  "start_cell_index",
                                                  error_message)) {
                    return false;
                }
                start_cell_index_value = parsed_index;
            } else if (member_name == "rows") {
                if (saw_rows) {
                    error_message =
                        "JSON patch member 'rows' must not be duplicated";
                    return false;
                }
                if (!parse_json_patch_rows_matrix(content, index, rows,
                                                  error_message)) {
                    return false;
                }
                saw_rows = true;
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
            return report_json_patch_error(
                index, "expected ',' or '}' after object member",
                error_message);
        }
    }

    std::optional<std::string> resolved_bookmark_name;
    if (!resolve_json_patch_string_member(bookmark_value, bookmark_name_value,
                                          "bookmark", "bookmark_name",
                                          resolved_bookmark_name,
                                          error_message)) {
        return false;
    }

    std::optional<std::string> resolved_after_text;
    if (!resolve_json_patch_string_member(after_text_value,
                                          after_paragraph_text_value,
                                          "after_text",
                                          "after_paragraph_text",
                                          resolved_after_text,
                                          error_message)) {
        return false;
    }

    std::vector<std::string> resolved_header_cells;
    if (saw_header_cells && saw_header_cell_texts &&
        header_cells_value != header_cell_texts_value) {
        error_message = "JSON patch members 'header_cells' and "
                        "'header_cell_texts' must match when both are present";
        return false;
    }
    if (saw_header_cells) {
        resolved_header_cells = std::move(header_cells_value);
    } else if (saw_header_cell_texts) {
        resolved_header_cells = std::move(header_cell_texts_value);
    }

    std::optional<std::size_t> resolved_header_row_index;
    if (!resolve_json_patch_index_member(header_row_value,
                                         header_row_index_value, "header_row",
                                         "header_row_index",
                                         resolved_header_row_index,
                                         error_message)) {
        return false;
    }

    featherdoc::template_table_selector selector{};
    selector.table_index = table_index_value;
    selector.bookmark_name = std::move(resolved_bookmark_name);
    selector.after_paragraph_text = std::move(resolved_after_text);
    selector.header_cell_texts = std::move(resolved_header_cells);
    if (resolved_header_row_index.has_value()) {
        selector.header_row_index = *resolved_header_row_index;
    }
    if (occurrence_value.has_value()) {
        selector.occurrence = *occurrence_value;
    }

    if (!validate_template_table_selector(selector, false,
                                          resolved_header_row_index.has_value(),
                                          occurrence_value.has_value(),
                                          error_message)) {
        return false;
    }

    std::optional<std::size_t> resolved_part_index;
    if (!resolve_json_patch_index_member(index_value, part_index_value, "index",
                                         "part_index", resolved_part_index,
                                         error_message)) {
        return false;
    }

    std::optional<std::size_t> resolved_section_index;
    if (!resolve_json_patch_index_member(section_value, section_index_value,
                                         "section", "section_index",
                                         resolved_section_index,
                                         error_message)) {
        return false;
    }

    template_table_json_patch patch;
    if (!finalize_template_table_json_patch(
            mode_value, start_row_value, start_row_index_value,
            start_cell_value, start_cell_index_value, std::move(rows), saw_rows,
            patch, error_message)) {
        return false;
    }

    operation.selector = std::move(selector);
    operation.part = part_value;
    operation.part_index = resolved_part_index;
    operation.section_index = resolved_section_index;
    operation.reference_kind = reference_kind_value;
    operation.patch = std::move(patch);
    return true;
}

} // namespace

auto parse_template_table_json_batch_operations_array(
    std::string_view content, std::size_t &index,
    std::vector<template_table_json_batch_operation> &operations,
    std::string &error_message) -> bool {
    skip_json_patch_whitespace(content, index);
    if (index >= content.size() || content[index] != '[') {
        return report_json_patch_error(index, "expected operations array",
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
        template_table_json_batch_operation operation;
        if (!parse_template_table_json_batch_operation(content, index,
                                                       operation,
                                                       error_message)) {
            error_message = prefix_template_table_json_batch_message(
                operations.size(), error_message);
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
        return report_json_patch_error(index,
                                       "expected ',' or ']' after operation",
                                       error_message);
    }

    return report_json_patch_error(index, "unterminated operations array",
                                   error_message);
}

} // namespace featherdoc_cli
