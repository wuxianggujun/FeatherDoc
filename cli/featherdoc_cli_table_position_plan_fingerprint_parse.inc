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
