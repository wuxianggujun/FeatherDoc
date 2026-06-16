auto read_style_refactor_rollback_content(const path_type &rollback_path,
                                          std::string &content,
                                          std::string &error_message) -> bool {
    std::ifstream stream(rollback_path, std::ios::binary);
    if (!stream.good()) {
        error_message = "failed to read style refactor rollback file: " +
                        rollback_path.string();
        return false;
    }

    content.assign(std::istreambuf_iterator<char>(stream),
                   std::istreambuf_iterator<char>());
    return true;
}

auto read_style_refactor_rollback_file(
    const path_type &rollback_path, const std::vector<std::size_t> &entry_indexes,
    const std::vector<std::string> &source_style_ids,
    const std::vector<std::string> &target_style_ids,
    std::vector<featherdoc::style_refactor_rollback_entry> &entries,
    std::string &error_message) -> bool {
    std::string content;
    if (!read_style_refactor_rollback_content(rollback_path, content,
                                              error_message)) {
        return false;
    }

    std::size_t index = 0U;
    skip_json_patch_whitespace(content, index);
    if (index >= content.size() || content[index] != '{') {
        return report_json_input_error("style refactor rollback file", index,
                                       "expected root object", error_message);
    }

    auto parsed_entries = std::vector<featherdoc::style_refactor_rollback_entry>{};
    ++index;
    skip_json_patch_whitespace(content, index);
    bool saw_rollback_operations = false;
    if (index < content.size() && content[index] == '}') {
        return report_json_input_error("style refactor rollback file", index,
                                       "root object must not be empty",
                                       error_message);
    }

    while (index < content.size()) {
        std::string member_name;
        if (!parse_json_patch_string(content, index, member_name, error_message)) {
            return false;
        }
        skip_json_patch_whitespace(content, index);
        if (index >= content.size() || content[index] != ':') {
            return report_json_input_error(
                "style refactor rollback file", index,
                "expected ':' after root object member", error_message);
        }
        ++index;
        skip_json_patch_whitespace(content, index);

        if (member_name == "rollback_operations") {
            if (saw_rollback_operations) {
                error_message = "JSON style refactor rollback root member "
                                "'rollback_operations' must not be duplicated";
                return false;
            }
            saw_rollback_operations = true;
            if (!parse_style_refactor_rollback_entries(content, index,
                                                       parsed_entries,
                                                       error_message)) {
                return false;
            }
        } else {
            if (!skip_json_patch_value(content, index, error_message)) {
                return false;
            }
        }

        bool closed = false;
        if (!consume_style_refactor_rollback_separator(
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
        return report_json_input_error("style refactor rollback file", index,
                                       "unexpected trailing content after root object",
                                       error_message);
    }
    if (!saw_rollback_operations) {
        error_message = "JSON style refactor rollback file must contain a "
                        "'rollback_operations' array";
        return false;
    }

    const auto matches_style_selection = [&](const auto &entry) {
        if (!source_style_ids.empty() &&
            std::find(source_style_ids.begin(), source_style_ids.end(),
                      entry.source_style_id) == source_style_ids.end()) {
            return false;
        }
        if (!target_style_ids.empty() &&
            std::find(target_style_ids.begin(), target_style_ids.end(),
                      entry.target_style_id) == target_style_ids.end()) {
            return false;
        }
        return true;
    };

    entries.clear();
    if (!entry_indexes.empty()) {
        for (const auto entry_index : entry_indexes) {
            if (entry_index >= parsed_entries.size()) {
                error_message = "style refactor rollback --entry index is out of range";
                return false;
            }
            if (parsed_entries[entry_index].action !=
                featherdoc::style_refactor_action::merge) {
                error_message = "style refactor rollback --entry must reference a "
                                "merge rollback operation";
                return false;
            }
            entries.push_back(std::move(parsed_entries[entry_index]));
        }
    } else {
        for (auto &entry : parsed_entries) {
            if (entry.action == featherdoc::style_refactor_action::merge &&
                matches_style_selection(entry)) {
                entries.push_back(std::move(entry));
            }
        }
    }

    if (entries.empty()) {
        if (!source_style_ids.empty() || !target_style_ids.empty()) {
            error_message = "JSON style refactor rollback file does not contain any "
                            "merge rollback operations matching restore selection";
        } else {
            error_message = "JSON style refactor rollback file does not contain any "
                            "merge rollback operations to restore";
        }
        return false;
    }

    return true;
}
