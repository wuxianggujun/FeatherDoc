#include "featherdoc_cli_template_table_json_patch_parse.hpp"

#include "featherdoc_cli_json_parse.hpp"
#include "featherdoc_cli_template_table_json_patch_parse_detail.hpp"

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

} // namespace featherdoc_cli
