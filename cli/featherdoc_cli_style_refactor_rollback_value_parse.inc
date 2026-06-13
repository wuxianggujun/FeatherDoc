auto consume_style_refactor_rollback_separator(std::string_view content,
                                                std::size_t &index,
                                                char close_char,
                                                std::string_view error_detail,
                                                bool &closed,
                                                std::string &error_message)
    -> bool {
    skip_json_patch_whitespace(content, index);
    if (index >= content.size()) {
        return report_json_input_error("style refactor rollback file", index,
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

    return report_json_input_error("style refactor rollback file", index,
                                   error_detail, error_message);
}

auto parse_json_bool_value(std::string_view content, std::size_t &index,
                           bool &value, std::string_view member_name,
                           std::string &error_message) -> bool {
    skip_json_patch_whitespace(content, index);
    if (content.substr(index, 4U) == "true") {
        value = true;
        index += 4U;
        return true;
    }
    if (content.substr(index, 5U) == "false") {
        value = false;
        index += 5U;
        return true;
    }
    if (index < content.size() && content[index] == '"') {
        std::string token;
        if (!parse_json_patch_string(content, index, token, error_message)) {
            return false;
        }
        if (parse_bool(token, value)) {
            return true;
        }
    }

    error_message = "JSON style refactor rollback member '" +
                    std::string(member_name) + "' must be a boolean";
    return false;
}

auto consume_json_null_value(std::string_view content, std::size_t &index,
                             std::string_view member_name,
                             std::string &error_message) -> bool {
    skip_json_patch_whitespace(content, index);
    if (content.substr(index, 4U) == "null") {
        index += 4U;
        return true;
    }

    error_message = "JSON style refactor rollback member '" +
                    std::string(member_name) + "' must be null";
    return false;
}

auto parse_nullable_json_string_value(std::string_view content,
                                      std::size_t &index,
                                      std::string &value,
                                      std::string_view member_name,
                                      std::string &error_message) -> bool {
    skip_json_patch_whitespace(content, index);
    if (content.substr(index, 4U) == "null") {
        value.clear();
        index += 4U;
        return true;
    }

    if (index < content.size() && content[index] == '"') {
        return parse_json_patch_string(content, index, value, error_message);
    }

    error_message = "JSON style refactor rollback member '" +
                    std::string(member_name) + "' must be a string or null";
    return false;
}

auto parse_style_usage_part_kind_value(
    std::string_view content, std::size_t &index,
    featherdoc::style_usage_part_kind &part,
    std::string &error_message) -> bool {
    std::string token;
    skip_json_patch_whitespace(content, index);
    if (!parse_json_patch_string(content, index, token, error_message)) {
        return false;
    }

    if (token == "body") {
        part = featherdoc::style_usage_part_kind::body;
        return true;
    }
    if (token == "header") {
        part = featherdoc::style_usage_part_kind::header;
        return true;
    }
    if (token == "footer") {
        part = featherdoc::style_usage_part_kind::footer;
        return true;
    }

    error_message = "JSON style usage hit member 'part' must be 'body', "
                    "'header', or 'footer'";
    return false;
}

auto parse_style_usage_hit_kind_value(
    std::string_view content, std::size_t &index,
    featherdoc::style_usage_hit_kind &kind,
    std::string &error_message) -> bool {
    std::string token;
    skip_json_patch_whitespace(content, index);
    if (!parse_json_patch_string(content, index, token, error_message)) {
        return false;
    }

    if (token == "paragraph") {
        kind = featherdoc::style_usage_hit_kind::paragraph;
        return true;
    }
    if (token == "run") {
        kind = featherdoc::style_usage_hit_kind::run;
        return true;
    }
    if (token == "table") {
        kind = featherdoc::style_usage_hit_kind::table;
        return true;
    }

    error_message = "JSON style usage hit member 'kind' must be 'paragraph', "
                    "'run', or 'table'";
    return false;
}

auto parse_style_usage_hit_section_index(
    std::string_view content, std::size_t &index,
    std::optional<std::size_t> &section_index,
    std::string &error_message) -> bool {
    skip_json_patch_whitespace(content, index);
    if (content.substr(index, 4U) == "null") {
        section_index.reset();
        index += 4U;
        return true;
    }

    std::size_t parsed_index = 0U;
    if (!parse_json_patch_index_value(content, index, parsed_index,
                                      "section_index", error_message)) {
        return false;
    }
    section_index = parsed_index;
    return true;
}
