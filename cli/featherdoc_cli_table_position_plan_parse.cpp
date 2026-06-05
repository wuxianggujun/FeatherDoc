#include "featherdoc_cli_table_position_plan_parse.hpp"

#include "featherdoc_cli_json_parse.hpp"
#include "featherdoc_cli_parse.hpp"

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace featherdoc_cli {

using path_type = std::filesystem::path;

auto table_position_preset_name(table_position_preset preset) noexcept
    -> std::string_view {
    switch (preset) {
    case table_position_preset::paragraph_callout:
        return "paragraph-callout";
    case table_position_preset::page_corner:
        return "page-corner";
    case table_position_preset::margin_anchor:
        return "margin-anchor";
    }

    return "paragraph-callout";
}

auto parse_table_position_preset(std::string_view text,
                                 table_position_preset &preset) -> bool {
    if (text == "paragraph-callout") {
        preset = table_position_preset::paragraph_callout;
        return true;
    }
    if (text == "page-corner") {
        preset = table_position_preset::page_corner;
        return true;
    }
    if (text == "margin-anchor") {
        preset = table_position_preset::margin_anchor;
        return true;
    }

    return false;
}

auto consume_table_position_plan_separator(std::string_view content,
                                           std::size_t &index, char close_char,
                                           std::string_view error_detail,
                                           bool &closed,
                                           std::string &error_message) -> bool {
    skip_json_patch_whitespace(content, index);
    if (index >= content.size()) {
        return report_json_input_error("table position plan file", index,
                                       "unexpected end of JSON", error_message);
    }
    if (content[index] == ',') {
        ++index;
        skip_json_patch_whitespace(content, index);
        closed = false;
        return true;
    }
    if (content[index] == close_char) {
        ++index;
        closed = true;
        return true;
    }

    return report_json_input_error("table position plan file", index,
                                   error_detail, error_message);
}

auto parse_table_position_plan_size_array(
    std::string_view content, std::size_t &index,
    std::vector<std::size_t> &values, std::string_view member_name,
    std::string &error_message) -> bool {
    skip_json_patch_whitespace(content, index);
    if (index >= content.size() || content[index] != '[') {
        return report_json_input_error("table position plan file", index,
                                       "expected array for '" +
                                           std::string(member_name) + "'",
                                       error_message);
    }

    values.clear();
    ++index;
    skip_json_patch_whitespace(content, index);
    if (index < content.size() && content[index] == ']') {
        ++index;
        return true;
    }

    while (index < content.size()) {
        std::size_t value = 0U;
        if (!parse_json_patch_index_value(content, index, value, member_name,
                                          error_message)) {
            return false;
        }
        values.push_back(value);

        bool closed = false;
        if (!consume_table_position_plan_separator(
                content, index, ']',
                "expected ',' or ']' after array item", closed,
                error_message)) {
            return false;
        }
        if (closed) {
            return true;
        }
    }

    return report_json_input_error("table position plan file", index,
                                   "unterminated array", error_message);
}

auto parse_table_position_plan_optional_path(
    std::string_view content, std::size_t &index,
    std::optional<path_type> &value, std::string_view member_name,
    std::string &error_message) -> bool {
    skip_json_patch_whitespace(content, index);
    if (index < content.size() && content.substr(index, 4U) == "null") {
        value.reset();
        index += 4U;
        return true;
    }

    std::string path_text;
    if (!parse_json_patch_string(content, index, path_text, error_message)) {
        return false;
    }
    if (path_text.empty()) {
        return report_json_input_error("table position plan file", index,
                                       "'" + std::string(member_name) +
                                           "' must not be empty",
                                       error_message);
    }
    value = path_type(std::move(path_text));
    return true;
}

auto parse_table_position_plan_uint32(
    std::string_view content, std::size_t &index, std::uint32_t &value,
    std::string_view member_name, std::string &error_message) -> bool {
    std::string token;
    skip_json_patch_whitespace(content, index);
    if (index >= content.size()) {
        return report_json_input_error("table position plan file", index,
                                       "expected unsigned integer value",
                                       error_message);
    }

    if (content[index] == '"') {
        if (!parse_json_patch_string(content, index, token, error_message)) {
            return false;
        }
    } else if (content[index] >= '0' && content[index] <= '9') {
        if (!parse_json_patch_number(content, index, token, error_message)) {
            return false;
        }
    } else {
        return report_json_input_error("table position plan file", index,
                                       "expected unsigned integer value",
                                       error_message);
    }

    if (!parse_uint32(token, value)) {
        error_message = "JSON table position plan member '" +
                        std::string(member_name) +
                        "' must be an unsigned integer";
        return false;
    }

    return true;
}

auto parse_table_position_plan_optional_uint32(
    std::string_view content, std::size_t &index,
    std::optional<std::uint32_t> &value, std::string_view member_name,
    std::string &error_message) -> bool {
    skip_json_patch_whitespace(content, index);
    if (index < content.size() && content.substr(index, 4U) == "null") {
        value.reset();
        index += 4U;
        return true;
    }

    std::uint32_t parsed_value = 0U;
    if (!parse_table_position_plan_uint32(content, index, parsed_value,
                                          member_name, error_message)) {
        return false;
    }
    value = parsed_value;
    return true;
}

auto parse_table_position_plan_optional_string(
    std::string_view content, std::size_t &index,
    std::optional<std::string> &value, std::string_view member_name,
    std::string &error_message) -> bool {
    skip_json_patch_whitespace(content, index);
    if (index < content.size() && content.substr(index, 4U) == "null") {
        value.reset();
        index += 4U;
        return true;
    }
    if (index < content.size() && content[index] == '"') {
        std::string parsed_value;
        if (!parse_json_patch_string(content, index, parsed_value,
                                     error_message)) {
            return false;
        }
        value = std::move(parsed_value);
        return true;
    }

    return report_json_input_error("table position plan file", index,
                                   "expected string or null for '" +
                                       std::string(member_name) + "'",
                                   error_message);
}

auto parse_table_position_plan_optional_uint32_array(
    std::string_view content, std::size_t &index,
    std::vector<std::optional<std::uint32_t>> &values,
    std::string_view member_name, std::string &error_message) -> bool {
    skip_json_patch_whitespace(content, index);
    if (index >= content.size() || content[index] != '[') {
        return report_json_input_error("table position plan file", index,
                                       "expected array for '" +
                                           std::string(member_name) + "'",
                                       error_message);
    }

    values.clear();
    ++index;
    skip_json_patch_whitespace(content, index);
    if (index < content.size() && content[index] == ']') {
        ++index;
        return true;
    }

    while (index < content.size()) {
        std::optional<std::uint32_t> value;
        if (!parse_table_position_plan_optional_uint32(
                content, index, value, member_name, error_message)) {
            return false;
        }
        values.push_back(value);

        bool closed = false;
        if (!consume_table_position_plan_separator(
                content, index, ']',
                "expected ',' or ']' after fingerprint array item", closed,
                error_message)) {
            return false;
        }
        if (closed) {
            return true;
        }
    }

    return report_json_input_error("table position plan file", index,
                                   "unterminated array", error_message);
}

auto parse_table_position_table_fingerprint(
    std::string_view content, std::size_t &index,
    table_position_table_fingerprint &fingerprint,
    std::string &error_message) -> bool {
    skip_json_patch_whitespace(content, index);
    if (index >= content.size() || content[index] != '{') {
        return report_json_input_error("table position plan file", index,
                                       "expected table fingerprint object",
                                       error_message);
    }

    fingerprint = table_position_table_fingerprint{};
    bool saw_table_index = false;
    bool saw_style_id = false;
    bool saw_width_twips = false;
    bool saw_row_count = false;
    bool saw_column_count = false;
    bool saw_column_widths = false;
    bool saw_text = false;

    ++index;
    skip_json_patch_whitespace(content, index);
    bool closed = false;
    if (index < content.size() && content[index] == '}') {
        ++index;
        closed = true;
    }

    while (!closed && index < content.size()) {
        std::string member_name;
        if (!parse_json_patch_string(content, index, member_name,
                                     error_message)) {
            return false;
        }
        skip_json_patch_whitespace(content, index);
        if (index >= content.size() || content[index] != ':') {
            return report_json_input_error(
                "table position plan file", index,
                "expected ':' after table fingerprint member", error_message);
        }
        ++index;
        skip_json_patch_whitespace(content, index);

        if (member_name == "table_index") {
            if (saw_table_index) {
                error_message = "JSON table position plan fingerprint member "
                                "'table_index' must not be duplicated";
                return false;
            }
            saw_table_index = true;
            if (!parse_json_patch_index_value(content, index,
                                              fingerprint.table_index,
                                              "table_index", error_message)) {
                return false;
            }
        } else if (member_name == "style_id") {
            if (saw_style_id) {
                error_message = "JSON table position plan fingerprint member "
                                "'style_id' must not be duplicated";
                return false;
            }
            saw_style_id = true;
            if (!parse_table_position_plan_optional_string(
                    content, index, fingerprint.style_id, "style_id",
                    error_message)) {
                return false;
            }
        } else if (member_name == "width_twips") {
            if (saw_width_twips) {
                error_message = "JSON table position plan fingerprint member "
                                "'width_twips' must not be duplicated";
                return false;
            }
            saw_width_twips = true;
            if (!parse_table_position_plan_optional_uint32(
                    content, index, fingerprint.width_twips, "width_twips",
                    error_message)) {
                return false;
            }
        } else if (member_name == "row_count") {
            if (saw_row_count) {
                error_message = "JSON table position plan fingerprint member "
                                "'row_count' must not be duplicated";
                return false;
            }
            saw_row_count = true;
            if (!parse_json_patch_index_value(content, index,
                                              fingerprint.row_count,
                                              "row_count", error_message)) {
                return false;
            }
        } else if (member_name == "column_count") {
            if (saw_column_count) {
                error_message = "JSON table position plan fingerprint member "
                                "'column_count' must not be duplicated";
                return false;
            }
            saw_column_count = true;
            if (!parse_json_patch_index_value(content, index,
                                              fingerprint.column_count,
                                              "column_count", error_message)) {
                return false;
            }
        } else if (member_name == "column_widths") {
            if (saw_column_widths) {
                error_message = "JSON table position plan fingerprint member "
                                "'column_widths' must not be duplicated";
                return false;
            }
            saw_column_widths = true;
            if (!parse_table_position_plan_optional_uint32_array(
                    content, index, fingerprint.column_widths, "column_widths",
                    error_message)) {
                return false;
            }
        } else if (member_name == "text") {
            if (saw_text) {
                error_message = "JSON table position plan fingerprint member "
                                "'text' must not be duplicated";
                return false;
            }
            saw_text = true;
            if (!parse_json_patch_string(content, index, fingerprint.text,
                                         error_message)) {
                return false;
            }
        } else if (!skip_json_patch_value(content, index, error_message)) {
            return false;
        }

        if (!consume_table_position_plan_separator(
                content, index, '}',
                "expected ',' or '}' after table fingerprint member", closed,
                error_message)) {
            return false;
        }
    }

    if (!closed) {
        return report_json_input_error("table position plan file", index,
                                       "unterminated table fingerprint object",
                                       error_message);
    }
    if (!saw_table_index || !saw_style_id || !saw_width_twips ||
        !saw_row_count || !saw_column_count || !saw_column_widths ||
        !saw_text) {
        error_message = "JSON table position plan table fingerprint must contain "
                        "'table_index', 'style_id', 'width_twips', "
                        "'row_count', 'column_count', 'column_widths', "
                        "and 'text'";
        return false;
    }

    return true;
}

auto parse_table_position_table_fingerprints(
    std::string_view content, std::size_t &index,
    std::vector<table_position_table_fingerprint> &fingerprints,
    std::string &error_message) -> bool {
    skip_json_patch_whitespace(content, index);
    if (index >= content.size() || content[index] != '[') {
        return report_json_input_error(
            "table position plan file", index,
            "expected array for 'table_fingerprints'", error_message);
    }

    fingerprints.clear();
    ++index;
    skip_json_patch_whitespace(content, index);
    if (index < content.size() && content[index] == ']') {
        ++index;
        return true;
    }

    while (index < content.size()) {
        table_position_table_fingerprint fingerprint;
        if (!parse_table_position_table_fingerprint(content, index, fingerprint,
                                                    error_message)) {
            return false;
        }
        fingerprints.push_back(std::move(fingerprint));

        bool closed = false;
        if (!consume_table_position_plan_separator(
                content, index, ']',
                "expected ',' or ']' after table fingerprint", closed,
                error_message)) {
            return false;
        }
        if (closed) {
            return true;
        }
    }

    return report_json_input_error("table position plan file", index,
                                   "unterminated table_fingerprints array",
                                   error_message);
}
auto read_table_position_plan_content(const path_type &plan_path,
                                      std::string &content,
                                      std::string &error_message) -> bool {
    std::ifstream stream(plan_path, std::ios::binary);
    if (!stream.good()) {
        error_message = "failed to read table position plan file: " +
                        plan_path.string();
        return false;
    }

    content.assign(std::istreambuf_iterator<char>(stream),
                   std::istreambuf_iterator<char>());
    return true;
}

auto read_table_position_plan_file(
    const path_type &plan_path, parsed_table_position_plan_file &plan,
    std::string &error_message) -> bool {
    std::string content;
    if (!read_table_position_plan_content(plan_path, content, error_message)) {
        return false;
    }

    std::size_t index = 0U;
    skip_json_patch_whitespace(content, index);
    if (index >= content.size() || content[index] != '{') {
        return report_json_input_error("table position plan file", index,
                                       "expected root object", error_message);
    }

    bool saw_command = false;
    bool saw_input_path = false;
    bool saw_preset = false;
    bool saw_table_count = false;
    bool saw_automatic_indices = false;
    bool saw_table_fingerprints = false;

    ++index;
    skip_json_patch_whitespace(content, index);
    if (index < content.size() && content[index] == '}') {
        return report_json_input_error("table position plan file", index,
                                       "root object must not be empty",
                                       error_message);
    }

    while (index < content.size()) {
        std::string member_name;
        if (!parse_json_patch_string(content, index, member_name,
                                     error_message)) {
            return false;
        }
        skip_json_patch_whitespace(content, index);
        if (index >= content.size() || content[index] != ':') {
            return report_json_input_error("table position plan file", index,
                                           "expected ':' after root object member",
                                           error_message);
        }
        ++index;
        skip_json_patch_whitespace(content, index);

        if (member_name == "command") {
            if (saw_command) {
                error_message = "JSON table position plan root member 'command' "
                                "must not be duplicated";
                return false;
            }
            saw_command = true;
            std::string command_name;
            if (!parse_json_patch_string(content, index, command_name,
                                         error_message)) {
                return false;
            }
            if (command_name != "plan-table-position-presets") {
                return report_json_input_error(
                    "table position plan file", index,
                    "command must be 'plan-table-position-presets'",
                    error_message);
            }
        } else if (member_name == "input_path") {
            if (saw_input_path) {
                error_message = "JSON table position plan root member 'input_path' "
                                "must not be duplicated";
                return false;
            }
            saw_input_path = true;
            std::string input_path_text;
            if (!parse_json_patch_string(content, index, input_path_text,
                                         error_message)) {
                return false;
            }
            if (input_path_text.empty()) {
                return report_json_input_error("table position plan file", index,
                                               "'input_path' must not be empty",
                                               error_message);
            }
            plan.input_path = path_type(std::move(input_path_text));
        } else if (member_name == "preset") {
            if (saw_preset) {
                error_message = "JSON table position plan root member 'preset' "
                                "must not be duplicated";
                return false;
            }
            saw_preset = true;
            std::string preset_name;
            if (!parse_json_patch_string(content, index, preset_name,
                                         error_message)) {
                return false;
            }
            if (!parse_table_position_preset(preset_name, plan.preset)) {
                return report_json_input_error("table position plan file", index,
                                               "invalid table position preset",
                                               error_message);
            }
        } else if (member_name == "table_count") {
            if (saw_table_count) {
                error_message = "JSON table position plan root member 'table_count' "
                                "must not be duplicated";
                return false;
            }
            saw_table_count = true;
            if (!parse_json_patch_index_value(content, index, plan.table_count,
                                              "table_count", error_message)) {
                return false;
            }
        } else if (member_name == "automatic_table_indices") {
            if (saw_automatic_indices) {
                error_message = "JSON table position plan root member "
                                "'automatic_table_indices' must not be duplicated";
                return false;
            }
            saw_automatic_indices = true;
            if (!parse_table_position_plan_size_array(
                    content, index, plan.automatic_table_indices,
                    "automatic_table_indices", error_message)) {
                return false;
            }
        } else if (member_name == "table_fingerprints") {
            if (saw_table_fingerprints) {
                error_message = "JSON table position plan root member "
                                "'table_fingerprints' must not be duplicated";
                return false;
            }
            saw_table_fingerprints = true;
            if (!parse_table_position_table_fingerprints(
                    content, index, plan.table_fingerprints, error_message)) {
                return false;
            }
        } else if (member_name == "resolved_output_path") {
            if (!parse_table_position_plan_optional_path(
                    content, index, plan.resolved_output_path,
                    "resolved_output_path", error_message)) {
                return false;
            }
        } else {
            if (!skip_json_patch_value(content, index, error_message)) {
                return false;
            }
        }

        bool closed = false;
        if (!consume_table_position_plan_separator(
                content, index, '}',
                "expected ',' or '}' after root object member", closed,
                error_message)) {
            return false;
        }
        if (closed) {
            break;
        }
    }

    skip_json_patch_whitespace(content, index);
    if (index != content.size()) {
        return report_json_input_error("table position plan file", index,
                                       "unexpected trailing content after root object",
                                       error_message);
    }

    if (!saw_command || !saw_input_path || !saw_preset || !saw_table_count ||
        !saw_automatic_indices || !saw_table_fingerprints) {
        error_message = "JSON table position plan file must contain 'command', "
                        "'input_path', 'preset', 'table_count', "
                        "'automatic_table_indices', and "
                        "'table_fingerprints'";
        return false;
    }
    if (plan.automatic_table_indices.empty()) {
        error_message = "JSON table position plan file has no automatic table "
                        "indices to apply";
        return false;
    }

    std::vector<std::size_t> seen_indices;
    seen_indices.reserve(plan.automatic_table_indices.size());
    for (const auto table_index : plan.automatic_table_indices) {
        if (table_index >= plan.table_count) {
            error_message = "JSON table position plan automatic table index '" +
                            std::to_string(table_index) + "' is out of range";
            return false;
        }
        if (std::find(seen_indices.begin(), seen_indices.end(), table_index) !=
            seen_indices.end()) {
            error_message = "JSON table position plan automatic table index '" +
                            std::to_string(table_index) + "' is duplicated";
            return false;
        }
        seen_indices.push_back(table_index);
    }

    if (plan.table_fingerprints.size() > plan.table_count) {
        error_message = "JSON table position plan contains more table "
                        "fingerprints than planned tables";
        return false;
    }

    std::vector<std::size_t> seen_fingerprint_indices;
    seen_fingerprint_indices.reserve(plan.table_fingerprints.size());
    for (const auto &fingerprint : plan.table_fingerprints) {
        if (fingerprint.table_index >= plan.table_count) {
            error_message = "JSON table position plan table fingerprint "
                            "index '" +
                            std::to_string(fingerprint.table_index) +
                            "' is out of range";
            return false;
        }
        if (std::find(seen_fingerprint_indices.begin(),
                      seen_fingerprint_indices.end(),
                      fingerprint.table_index) !=
            seen_fingerprint_indices.end()) {
            error_message = "JSON table position plan table fingerprint "
                            "index '" +
                            std::to_string(fingerprint.table_index) +
                            "' is duplicated";
            return false;
        }
        seen_fingerprint_indices.push_back(fingerprint.table_index);
    }

    for (const auto table_index : plan.automatic_table_indices) {
        if (std::find(seen_fingerprint_indices.begin(),
                      seen_fingerprint_indices.end(), table_index) ==
            seen_fingerprint_indices.end()) {
            error_message = "JSON table position plan is missing table "
                            "fingerprint for automatic table index '" +
                            std::to_string(table_index) + "'";
            return false;
        }
    }

    return true;
}
} // namespace featherdoc_cli
