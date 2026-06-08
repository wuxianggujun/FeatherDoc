#include "featherdoc_cli_template_table_json_patch_parse_detail.hpp"

#include "featherdoc_cli_json_parse.hpp"
#include "featherdoc_cli_validation_part.hpp"

#include <utility>

namespace featherdoc_cli {

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

} // namespace featherdoc_cli
