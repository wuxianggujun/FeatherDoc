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
