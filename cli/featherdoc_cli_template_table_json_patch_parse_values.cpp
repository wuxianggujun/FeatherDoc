#include "featherdoc_cli_template_table_json_patch_parse_detail.hpp"

#include "featherdoc_cli_json_parse.hpp"

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

} // namespace featherdoc_cli
