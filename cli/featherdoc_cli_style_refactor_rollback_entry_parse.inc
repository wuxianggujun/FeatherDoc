auto parse_style_refactor_rollback_entry(
    std::string_view content, std::size_t &index,
    featherdoc::style_refactor_rollback_entry &entry,
    std::string &error_message) -> bool {
    skip_json_patch_whitespace(content, index);
    if (index >= content.size() || content[index] != '{') {
        return report_json_input_error("style refactor rollback file", index,
                                       "expected rollback operation object",
                                       error_message);
    }

    ++index;
    skip_json_patch_whitespace(content, index);
    bool saw_action = false;
    bool saw_source = false;
    bool saw_target = false;
    if (index < content.size() && content[index] == '}') {
        return report_json_input_error("style refactor rollback file", index,
                                       "rollback operation object must not be empty",
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
                "expected ':' after rollback operation member", error_message);
        }
        ++index;
        skip_json_patch_whitespace(content, index);

        if (member_name == "action") {
            saw_action = true;
            if (!parse_style_refactor_plan_action(content, index, entry.action,
                                                  error_message)) {
                return false;
            }
        } else if (member_name == "source_style_id") {
            saw_source = true;
            if (!parse_json_patch_string(content, index, entry.source_style_id,
                                         error_message)) {
                return false;
            }
        } else if (member_name == "target_style_id") {
            saw_target = true;
            if (!parse_json_patch_string(content, index, entry.target_style_id,
                                         error_message)) {
                return false;
            }
        } else if (member_name == "automatic") {
            if (!parse_json_bool_value(content, index, entry.automatic,
                                       member_name, error_message)) {
                return false;
            }
        } else if (member_name == "restorable") {
            if (!parse_json_bool_value(content, index, entry.restorable,
                                       member_name, error_message)) {
                return false;
            }
        } else if (member_name == "note") {
            if (!parse_nullable_json_string_value(content, index, entry.note,
                                                  member_name, error_message)) {
                return false;
            }
        } else if (member_name == "source_style_xml") {
            if (!parse_nullable_json_string_value(content, index,
                                                  entry.source_style_xml,
                                                  member_name, error_message)) {
                return false;
            }
        } else if (member_name == "source_usage") {
            skip_json_patch_whitespace(content, index);
            if (content.substr(index, 4U) == "null") {
                if (!consume_json_null_value(content, index, member_name,
                                             error_message)) {
                    return false;
                }
                entry.source_usage.reset();
            } else {
                auto usage = featherdoc::style_usage_summary{};
                if (!parse_style_usage_summary_object(content, index, usage,
                                                      error_message)) {
                    return false;
                }
                entry.source_usage = std::move(usage);
            }
        } else {
            if (!skip_json_patch_value(content, index, error_message)) {
                return false;
            }
        }

        bool closed = false;
        if (!consume_style_refactor_rollback_separator(
                content, index, '}',
                "expected ',' or '}' after rollback operation member", closed,
                error_message)) {
            return false;
        }
        if (closed) {
            break;
        }
    }

    if (!saw_action || !saw_source || !saw_target) {
        error_message = "JSON style refactor rollback operation must contain "
                        "'action', 'source_style_id', and 'target_style_id'";
        return false;
    }

    return true;
}

auto parse_style_refactor_rollback_entries(
    std::string_view content, std::size_t &index,
    std::vector<featherdoc::style_refactor_rollback_entry> &entries,
    std::string &error_message) -> bool {
    skip_json_patch_whitespace(content, index);
    if (index >= content.size() || content[index] != '[') {
        return report_json_input_error("style refactor rollback file", index,
                                       "expected rollback_operations array",
                                       error_message);
    }

    entries.clear();
    ++index;
    skip_json_patch_whitespace(content, index);
    if (index < content.size() && content[index] == ']') {
        ++index;
        return true;
    }

    while (index < content.size()) {
        auto entry = featherdoc::style_refactor_rollback_entry{};
        if (!parse_style_refactor_rollback_entry(content, index, entry,
                                                 error_message)) {
            return false;
        }
        entries.push_back(std::move(entry));

        bool closed = false;
        if (!consume_style_refactor_rollback_separator(
                content, index, ']',
                "expected ',' or ']' after rollback operation", closed,
                error_message)) {
            return false;
        }
        if (closed) {
            return true;
        }
    }

    return report_json_input_error("style refactor rollback file", index,
                                   "unterminated rollback_operations array",
                                   error_message);
}
