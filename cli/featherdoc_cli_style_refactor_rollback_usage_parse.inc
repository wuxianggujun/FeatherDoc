auto parse_style_usage_hit_object(
    std::string_view content, std::size_t &index,
    featherdoc::style_usage_hit &hit, std::string &error_message) -> bool {
    skip_json_patch_whitespace(content, index);
    if (index >= content.size() || content[index] != '{') {
        return report_json_input_error("style refactor rollback file", index,
                                       "expected style usage hit object",
                                       error_message);
    }

    ++index;
    skip_json_patch_whitespace(content, index);
    bool saw_part = false;
    bool saw_entry_name = false;
    bool saw_ordinal = false;
    bool saw_kind = false;
    bool saw_node_ordinal = false;

    if (index < content.size() && content[index] == '}') {
        return report_json_input_error("style refactor rollback file", index,
                                       "style usage hit object must not be empty",
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
                "expected ':' after style usage hit member", error_message);
        }
        ++index;
        skip_json_patch_whitespace(content, index);

        if (member_name == "part") {
            saw_part = true;
            if (!parse_style_usage_part_kind_value(content, index, hit.part,
                                                   error_message)) {
                return false;
            }
        } else if (member_name == "entry_name") {
            saw_entry_name = true;
            if (!parse_json_patch_string(content, index, hit.entry_name,
                                         error_message)) {
                return false;
            }
        } else if (member_name == "section_index") {
            if (!parse_style_usage_hit_section_index(content, index,
                                                     hit.section_index,
                                                     error_message)) {
                return false;
            }
        } else if (member_name == "ordinal") {
            saw_ordinal = true;
            if (!parse_json_patch_index_value(content, index, hit.ordinal,
                                              "ordinal", error_message)) {
                return false;
            }
        } else if (member_name == "node_ordinal") {
            saw_node_ordinal = true;
            if (!parse_json_patch_index_value(content, index, hit.node_ordinal,
                                              "node_ordinal", error_message)) {
                return false;
            }
        } else if (member_name == "kind") {
            saw_kind = true;
            if (!parse_style_usage_hit_kind_value(content, index, hit.kind,
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
                "expected ',' or '}' after style usage hit member", closed,
                error_message)) {
            return false;
        }
        if (closed) {
            break;
        }
    }

    if (!saw_part || !saw_entry_name || !saw_ordinal || !saw_kind) {
        error_message = "JSON style usage hit must contain 'part', 'entry_name', "
                        "'ordinal', and 'kind'";
        return false;
    }
    if (!saw_node_ordinal) {
        hit.node_ordinal = hit.ordinal;
    }

    return true;
}

auto parse_style_usage_hits_array(
    std::string_view content, std::size_t &index,
    std::vector<featherdoc::style_usage_hit> &hits,
    std::string &error_message) -> bool {
    skip_json_patch_whitespace(content, index);
    if (index >= content.size() || content[index] != '[') {
        return report_json_input_error("style refactor rollback file", index,
                                       "expected style usage hits array",
                                       error_message);
    }

    hits.clear();
    ++index;
    skip_json_patch_whitespace(content, index);
    if (index < content.size() && content[index] == ']') {
        ++index;
        return true;
    }

    while (index < content.size()) {
        auto hit = featherdoc::style_usage_hit{};
        if (!parse_style_usage_hit_object(content, index, hit, error_message)) {
            return false;
        }
        hits.push_back(std::move(hit));

        bool closed = false;
        if (!consume_style_refactor_rollback_separator(
                content, index, ']',
                "expected ',' or ']' after style usage hit", closed,
                error_message)) {
            return false;
        }
        if (closed) {
            return true;
        }
    }

    return report_json_input_error("style refactor rollback file", index,
                                   "unterminated style usage hits array",
                                   error_message);
}

auto parse_style_usage_summary_object(
    std::string_view content, std::size_t &index,
    featherdoc::style_usage_summary &usage,
    std::string &error_message) -> bool {
    skip_json_patch_whitespace(content, index);
    if (index >= content.size() || content[index] != '{') {
        return report_json_input_error("style refactor rollback file", index,
                                       "expected style usage object",
                                       error_message);
    }

    ++index;
    skip_json_patch_whitespace(content, index);
    bool saw_style_id = false;
    bool saw_hits = false;
    if (index < content.size() && content[index] == '}') {
        return report_json_input_error("style refactor rollback file", index,
                                       "style usage object must not be empty",
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
                "expected ':' after style usage object member", error_message);
        }
        ++index;
        skip_json_patch_whitespace(content, index);

        if (member_name == "style_id") {
            saw_style_id = true;
            if (!parse_json_patch_string(content, index, usage.style_id,
                                         error_message)) {
                return false;
            }
        } else if (member_name == "paragraph_count") {
            if (!parse_json_patch_index_value(content, index,
                                              usage.paragraph_count,
                                              "paragraph_count",
                                              error_message)) {
                return false;
            }
        } else if (member_name == "run_count") {
            if (!parse_json_patch_index_value(content, index, usage.run_count,
                                              "run_count", error_message)) {
                return false;
            }
        } else if (member_name == "table_count") {
            if (!parse_json_patch_index_value(content, index, usage.table_count,
                                              "table_count", error_message)) {
                return false;
            }
        } else if (member_name == "hits") {
            saw_hits = true;
            if (!parse_style_usage_hits_array(content, index, usage.hits,
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
                "expected ',' or '}' after style usage object member", closed,
                error_message)) {
            return false;
        }
        if (closed) {
            break;
        }
    }

    if (!saw_style_id || !saw_hits) {
        error_message = "JSON style usage object must contain 'style_id' and 'hits'";
        return false;
    }

    return true;
}
