#include "featherdoc_cli_template_table_json_patch_parse.hpp"

#include "featherdoc_cli_domain_parse.hpp"
#include "featherdoc_cli_json_parse.hpp"
#include "featherdoc_cli_template_inspect_options_parse.hpp"
#include "featherdoc_cli_validation_part.hpp"

#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace featherdoc_cli {

using path_type = std::filesystem::path;

namespace {

auto format_template_table_json_batch_operation_label(
    std::size_t operation_index) -> std::string {
    return "operation[" + std::to_string(operation_index) + "]";
}

} // namespace

auto prefix_template_table_json_batch_message(
    std::optional<std::size_t> operation_index, std::string_view message)
    -> std::string {
    if (!operation_index.has_value()) {
        return std::string(message);
    }

    return format_template_table_json_batch_operation_label(*operation_index) +
           ": " + std::string(message);
}

namespace {

auto parse_json_patch_cell_text(std::string_view text, std::size_t &index,
                                std::string &cell_text,
                                std::string &error_message) -> bool {
    skip_json_patch_whitespace(text, index);
    if (index >= text.size()) {
        return report_json_patch_error(index, "expected cell value",
                                       error_message);
    }

    if (text[index] == '"') {
        return parse_json_patch_string(text, index, cell_text, error_message);
    }
    if (text[index] == 't') {
        if (text.substr(index, 4U) != "true") {
            return report_json_patch_error(index, "expected 'true'",
                                           error_message);
        }
        cell_text = "true";
        index += 4U;
        return true;
    }
    if (text[index] == 'f') {
        if (text.substr(index, 5U) != "false") {
            return report_json_patch_error(index, "expected 'false'",
                                           error_message);
        }
        cell_text = "false";
        index += 5U;
        return true;
    }
    if (text[index] == 'n') {
        if (text.substr(index, 4U) != "null") {
            return report_json_patch_error(index, "expected 'null'",
                                           error_message);
        }
        cell_text.clear();
        index += 4U;
        return true;
    }
    if (text[index] == '-' || (text[index] >= '0' && text[index] <= '9')) {
        return parse_json_patch_number(text, index, cell_text, error_message);
    }

    return report_json_patch_error(index,
                                   "table cell values must be strings, numbers, "
                                   "booleans, or null",
                                   error_message);
}

auto parse_json_patch_rows_matrix(std::string_view text, std::size_t &index,
                                  std::vector<std::vector<std::string>> &rows,
                                  std::string &error_message) -> bool {
    skip_json_patch_whitespace(text, index);
    if (index >= text.size() || text[index] != '[') {
        return report_json_patch_error(index, "expected rows array",
                                       error_message);
    }

    rows.clear();
    ++index;
    skip_json_patch_whitespace(text, index);
    if (index < text.size() && text[index] == ']') {
        ++index;
        return true;
    }

    while (index < text.size()) {
        if (text[index] != '[') {
            return report_json_patch_error(index,
                                           "expected row array inside rows",
                                           error_message);
        }

        ++index;
        std::vector<std::string> row;
        skip_json_patch_whitespace(text, index);
        if (index < text.size() && text[index] == ']') {
            ++index;
            rows.push_back(row);
        } else {
            while (index < text.size()) {
                std::string cell_text;
                if (!parse_json_patch_cell_text(text, index, cell_text,
                                                error_message)) {
                    return false;
                }
                row.push_back(std::move(cell_text));

                skip_json_patch_whitespace(text, index);
                if (index >= text.size()) {
                    break;
                }
                if (text[index] == ',') {
                    ++index;
                    skip_json_patch_whitespace(text, index);
                    continue;
                }
                if (text[index] == ']') {
                    ++index;
                    rows.push_back(std::move(row));
                    break;
                }
                return report_json_patch_error(
                    index, "expected ',' or ']' inside row array",
                    error_message);
            }
        }

        skip_json_patch_whitespace(text, index);
        if (index >= text.size()) {
            break;
        }
        if (text[index] == ',') {
            ++index;
            skip_json_patch_whitespace(text, index);
            continue;
        }
        if (text[index] == ']') {
            ++index;
            return true;
        }
        return report_json_patch_error(index,
                                       "expected ',' or ']' after row array",
                                       error_message);
    }

    return report_json_patch_error(index, "unterminated rows array",
                                   error_message);
}

auto parse_json_patch_text_array(std::string_view text, std::size_t &index,
                                 std::vector<std::string> &values,
                                 std::string_view member_name,
                                 std::string &error_message) -> bool {
    skip_json_patch_whitespace(text, index);
    if (index >= text.size() || text[index] != '[') {
        return report_json_patch_error(index,
                                       "expected " + std::string(member_name) +
                                           " array",
                                       error_message);
    }

    values.clear();
    ++index;
    skip_json_patch_whitespace(text, index);
    if (index < text.size() && text[index] == ']') {
        ++index;
        return true;
    }

    while (index < text.size()) {
        std::string value;
        if (!parse_json_patch_cell_text(text, index, value, error_message)) {
            return false;
        }
        values.push_back(std::move(value));

        skip_json_patch_whitespace(text, index);
        if (index >= text.size()) {
            break;
        }
        if (text[index] == ',') {
            ++index;
            skip_json_patch_whitespace(text, index);
            continue;
        }
        if (text[index] == ']') {
            ++index;
            return true;
        }
        return report_json_patch_error(index,
                                       "expected ',' or ']' after " +
                                           std::string(member_name) + " item",
                                       error_message);
    }

    return report_json_patch_error(index,
                                   "unterminated " +
                                       std::string(member_name) + " array",
                                   error_message);
}

auto parse_json_patch_part_value(std::string_view text, std::size_t &index,
                                 validation_part_family &part,
                                 std::string &error_message) -> bool {
    skip_json_patch_whitespace(text, index);
    std::string token;
    if (!parse_json_patch_string(text, index, token, error_message)) {
        return false;
    }

    if (!parse_validation_part(token, part)) {
        error_message =
            "JSON patch member 'part' must be 'body', 'header', 'footer', "
            "'section-header', or 'section-footer'";
        return false;
    }

    return true;
}

auto finalize_template_table_json_patch(
    const std::optional<std::string> &mode_value,
    const std::optional<std::size_t> &start_row_value,
    const std::optional<std::size_t> &start_row_index_value,
    const std::optional<std::size_t> &start_cell_value,
    const std::optional<std::size_t> &start_cell_index_value,
    std::vector<std::vector<std::string>> rows, bool saw_rows,
    template_table_json_patch &patch, std::string &error_message) -> bool {
    std::optional<std::size_t> resolved_start_row_index;
    if (!resolve_json_patch_index_member(start_row_value, start_row_index_value,
                                         "start_row", "start_row_index",
                                         resolved_start_row_index,
                                         error_message)) {
        return false;
    }
    if (!resolved_start_row_index.has_value()) {
        error_message =
            "JSON patch must contain 'start_row' or 'start_row_index'";
        return false;
    }

    std::optional<std::size_t> resolved_start_cell_index;
    if (!resolve_json_patch_index_member(start_cell_value, start_cell_index_value,
                                         "start_cell", "start_cell_index",
                                         resolved_start_cell_index,
                                         error_message)) {
        return false;
    }

    if (!saw_rows) {
        error_message = "JSON patch must contain a 'rows' array";
        return false;
    }
    if (rows.empty()) {
        error_message =
            "JSON patch member 'rows' must contain at least one row";
        return false;
    }
    for (std::size_t row_offset = 0U; row_offset < rows.size(); ++row_offset) {
        if (rows[row_offset].empty()) {
            error_message = "JSON patch row at offset '" +
                            std::to_string(row_offset) +
                            "' must contain at least one cell";
            return false;
        }
    }

    if (mode_value.has_value()) {
        if (*mode_value == "rows") {
            patch.mode = template_table_json_patch_mode::rows;
        } else if (*mode_value == "block") {
            patch.mode = template_table_json_patch_mode::block;
        } else {
            error_message = "JSON patch member 'mode' must be 'rows' or 'block'";
            return false;
        }
    } else if (resolved_start_cell_index.has_value()) {
        patch.mode = template_table_json_patch_mode::block;
    } else {
        patch.mode = template_table_json_patch_mode::rows;
    }

    if (patch.mode == template_table_json_patch_mode::block) {
        if (!resolved_start_cell_index.has_value()) {
            error_message =
                "JSON block patch must contain 'start_cell' or 'start_cell_index'";
            return false;
        }
        patch.start_cell_index = *resolved_start_cell_index;
    } else if (resolved_start_cell_index.has_value()) {
        error_message =
            "JSON rows patch must not contain 'start_cell' or 'start_cell_index'";
        return false;
    }

    patch.start_row_index = *resolved_start_row_index;
    patch.rows = std::move(rows);
    return true;
}

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
                if (!parse_json_patch_text_array(content, index, header_cells_value,
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
                validation_part_family parsed_part = validation_part_family::body;
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
            return report_json_patch_error(index,
                                           "expected ',' or '}' after object "
                                           "member",
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
                                          "after_text", "after_paragraph_text",
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
    if (!resolve_json_patch_index_member(header_row_value, header_row_index_value,
                                         "header_row", "header_row_index",
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
            mode_value, start_row_value, start_row_index_value, start_cell_value,
            start_cell_index_value, std::move(rows), saw_rows, patch,
            error_message)) {
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
        if (!parse_template_table_json_batch_operation(content, index, operation,
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

} // namespace

auto read_template_table_json_patch(const path_type &patch_path,
                                    template_table_json_patch &patch,
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
            if (member_name == "mode") {
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
                    error_message =
                        "JSON patch member 'start_cell_index' must not be duplicated";
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

    return finalize_template_table_json_patch(
        mode_value, start_row_value, start_row_index_value, start_cell_value,
        start_cell_index_value, std::move(rows), saw_rows, patch, error_message);
}

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
