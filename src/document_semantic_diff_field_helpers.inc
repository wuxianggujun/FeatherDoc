struct semantic_field_value {
    std::string path;
    std::string value;
};

auto is_semantic_field_name(std::string_view text) -> bool {
    if (text.empty()) {
        return false;
    }
    for (const auto ch : text) {
        const auto valid = (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') ||
                           (ch >= '0' && ch <= '9') || ch == '_';
        if (!valid) {
            return false;
        }
    }
    return true;
}

auto try_expand_semicolon_fields(std::string_view path, std::string_view value,
                                 std::vector<semantic_field_value> &fields)
    -> bool {
    if (value.find(';') == std::string_view::npos) {
        return false;
    }

    auto expanded = std::vector<semantic_field_value>{};
    std::size_t start = 0U;
    while (start <= value.size()) {
        const auto end = value.find(';', start);
        const auto segment = value.substr(
            start, end == std::string_view::npos ? std::string_view::npos
                                                 : end - start);
        if (segment.empty()) {
            return false;
        }
        const auto equals = segment.find('=');
        if (equals == std::string_view::npos || equals == 0U) {
            return false;
        }
        const auto name = segment.substr(0U, equals);
        if (!is_semantic_field_name(name)) {
            return false;
        }
        auto child_path = std::string{path};
        child_path.push_back('.');
        child_path.append(name.data(), name.size());
        expanded.push_back(semantic_field_value{
            std::move(child_path),
            std::string{segment.substr(equals + 1U)}});
        if (end == std::string_view::npos) {
            break;
        }
        start = end + 1U;
    }

    if (expanded.size() < 2U) {
        return false;
    }
    fields.insert(fields.end(), expanded.begin(), expanded.end());
    return true;
}

auto parse_semantic_field_values(std::string_view value)
    -> std::vector<semantic_field_value> {
    auto top_level = std::vector<semantic_field_value>{};
    std::size_t start = 0U;
    while (start <= value.size()) {
        const auto end = value.find('\n', start);
        const auto line = value.substr(
            start, end == std::string_view::npos ? std::string_view::npos
                                                 : end - start);
        const auto equals = line.find('=');
        if (equals != std::string_view::npos &&
            is_semantic_field_name(line.substr(0U, equals))) {
            top_level.push_back(semantic_field_value{
                std::string{line.substr(0U, equals)},
                std::string{line.substr(equals + 1U)}});
        } else if (!top_level.empty()) {
            top_level.back().value.push_back('\n');
            top_level.back().value.append(line.data(), line.size());
        } else if (!line.empty()) {
            top_level.push_back(semantic_field_value{"value", std::string{line}});
        }
        if (end == std::string_view::npos) {
            break;
        }
        start = end + 1U;
    }

    auto fields = std::vector<semantic_field_value>{};
    for (const auto &field : top_level) {
        if (!try_expand_semicolon_fields(field.path, field.value, fields)) {
            fields.push_back(field);
        }
    }
    return fields;
}

auto find_semantic_field_value(const std::vector<semantic_field_value> &fields,
                               std::string_view path) -> std::optional<std::string> {
    for (const auto &field : fields) {
        if (field.path == path) {
            return field.value;
        }
    }
    return std::nullopt;
}

auto semantic_field_changes(std::string_view left_value, std::string_view right_value)
    -> std::vector<featherdoc::document_semantic_diff_field_change> {
    const auto left_fields = parse_semantic_field_values(left_value);
    const auto right_fields = parse_semantic_field_values(right_value);
    auto changes = std::vector<featherdoc::document_semantic_diff_field_change>{};

    for (const auto &left_field : left_fields) {
        const auto right_field = find_semantic_field_value(right_fields, left_field.path);
        if (!right_field.has_value() || *right_field != left_field.value) {
            changes.push_back(featherdoc::document_semantic_diff_field_change{
                left_field.path, left_field.value,
                right_field.value_or(std::string{})});
        }
    }

    for (const auto &right_field : right_fields) {
        if (!find_semantic_field_value(left_fields, right_field.path).has_value()) {
            changes.push_back(featherdoc::document_semantic_diff_field_change{
                right_field.path, std::string{}, right_field.value});
        }
    }

    return changes;
}
