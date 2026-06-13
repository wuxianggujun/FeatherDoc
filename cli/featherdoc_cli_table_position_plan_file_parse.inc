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
