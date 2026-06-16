std::optional<std::string>
Document::style_run_text_color(std::string_view style_id) {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before reading style run color",
            std::string{styles_xml_entry});
        return std::nullopt;
    }
    if (style_id.empty()) {
        set_last_error(
            this->last_error_info,
            std::make_error_code(std::errc::invalid_argument),
            "style id must not be empty when reading style run color",
            std::string{styles_xml_entry});
        return std::nullopt;
    }
    if (const auto error = this->ensure_styles_loaded()) {
        return std::nullopt;
    }
    const auto styles_root = this->styles.child("w:styles");
    const auto style = find_style_node(styles_root, style_id);
    if (style == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id '" + std::string{style_id} +
                           "' was not found in word/styles.xml",
                       std::string{styles_xml_entry});
        return std::nullopt;
    }
    this->last_error_info.clear();
    return read_run_text_color(style.child("w:rPr"));
}

std::optional<bool> Document::style_run_bold(std::string_view style_id) {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before reading style run bold",
            std::string{styles_xml_entry});
        return std::nullopt;
    }
    if (style_id.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id must not be empty when reading style run bold",
                       std::string{styles_xml_entry});
        return std::nullopt;
    }
    if (const auto error = this->ensure_styles_loaded()) {
        return std::nullopt;
    }
    const auto style =
        find_style_node(this->styles.child("w:styles"), style_id);
    if (style == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id '" + std::string{style_id} +
                           "' was not found in word/styles.xml",
                       std::string{styles_xml_entry});
        return std::nullopt;
    }
    this->last_error_info.clear();
    return read_run_bold(style.child("w:rPr"));
}

std::optional<bool> Document::style_run_italic(std::string_view style_id) {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before reading style run italic",
            std::string{styles_xml_entry});
        return std::nullopt;
    }
    if (style_id.empty()) {
        set_last_error(
            this->last_error_info,
            std::make_error_code(std::errc::invalid_argument),
            "style id must not be empty when reading style run italic",
            std::string{styles_xml_entry});
        return std::nullopt;
    }
    if (const auto error = this->ensure_styles_loaded()) {
        return std::nullopt;
    }
    const auto style =
        find_style_node(this->styles.child("w:styles"), style_id);
    if (style == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id '" + std::string{style_id} +
                           "' was not found in word/styles.xml",
                       std::string{styles_xml_entry});
        return std::nullopt;
    }
    this->last_error_info.clear();
    return read_run_italic(style.child("w:rPr"));
}

std::optional<bool> Document::style_run_underline(std::string_view style_id) {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before reading style run underline",
            std::string{styles_xml_entry});
        return std::nullopt;
    }
    if (style_id.empty()) {
        set_last_error(
            this->last_error_info,
            std::make_error_code(std::errc::invalid_argument),
            "style id must not be empty when reading style run underline",
            std::string{styles_xml_entry});
        return std::nullopt;
    }
    if (const auto error = this->ensure_styles_loaded()) {
        return std::nullopt;
    }
    const auto style =
        find_style_node(this->styles.child("w:styles"), style_id);
    if (style == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id '" + std::string{style_id} +
                           "' was not found in word/styles.xml",
                       std::string{styles_xml_entry});
        return std::nullopt;
    }
    this->last_error_info.clear();
    return read_run_underline(style.child("w:rPr"));
}

std::optional<bool>
Document::style_run_strikethrough(std::string_view style_id) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before reading style "
                       "run strikethrough",
                       std::string{styles_xml_entry});
        return std::nullopt;
    }
    if (style_id.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id must not be empty when reading style run "
                       "strikethrough",
                       std::string{styles_xml_entry});
        return std::nullopt;
    }
    if (const auto error = this->ensure_styles_loaded()) {
        return std::nullopt;
    }
    const auto style =
        find_style_node(this->styles.child("w:styles"), style_id);
    if (style == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id '" + std::string{style_id} +
                           "' was not found in word/styles.xml",
                       std::string{styles_xml_entry});
        return std::nullopt;
    }
    this->last_error_info.clear();
    return read_run_strikethrough(style.child("w:rPr"));
}

std::optional<double>
Document::style_run_font_size_points(std::string_view style_id) {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before reading style run font size",
            std::string{styles_xml_entry});
        return std::nullopt;
    }
    if (style_id.empty()) {
        set_last_error(
            this->last_error_info,
            std::make_error_code(std::errc::invalid_argument),
            "style id must not be empty when reading style run font size",
            std::string{styles_xml_entry});
        return std::nullopt;
    }
    if (const auto error = this->ensure_styles_loaded()) {
        return std::nullopt;
    }
    const auto style =
        find_style_node(this->styles.child("w:styles"), style_id);
    if (style == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id '" + std::string{style_id} +
                           "' was not found in word/styles.xml",
                       std::string{styles_xml_entry});
        return std::nullopt;
    }
    this->last_error_info.clear();
    return read_run_font_size_points(style.child("w:rPr"));
}

std::optional<std::string>
Document::style_run_font_family(std::string_view style_id) {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before reading style run fonts");
        return std::nullopt;
    }

    if (style_id.empty()) {
        set_last_error(
            this->last_error_info,
            std::make_error_code(std::errc::invalid_argument),
            "style id must not be empty when reading style run fonts",
            std::string{styles_xml_entry});
        return std::nullopt;
    }

    if (const auto error = this->ensure_styles_loaded()) {
        return std::nullopt;
    }

    const auto styles_root = this->styles.child("w:styles");
    if (styles_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       document_errc::styles_xml_parse_failed,
                       "word/styles.xml does not contain a w:styles root",
                       std::string{styles_xml_entry});
        return std::nullopt;
    }

    auto style = find_style_node(styles_root, style_id);
    if (style == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id '" + std::string{style_id} +
                           "' was not found in word/styles.xml",
                       std::string{styles_xml_entry});
        return std::nullopt;
    }

    this->last_error_info.clear();
    return read_primary_font_family(style.child("w:rPr"));
}

std::optional<std::string>
Document::style_run_east_asia_font_family(std::string_view style_id) {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before reading style run fonts");
        return std::nullopt;
    }

    if (style_id.empty()) {
        set_last_error(
            this->last_error_info,
            std::make_error_code(std::errc::invalid_argument),
            "style id must not be empty when reading style run fonts",
            std::string{styles_xml_entry});
        return std::nullopt;
    }

    if (const auto error = this->ensure_styles_loaded()) {
        return std::nullopt;
    }

    const auto styles_root = this->styles.child("w:styles");
    if (styles_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       document_errc::styles_xml_parse_failed,
                       "word/styles.xml does not contain a w:styles root",
                       std::string{styles_xml_entry});
        return std::nullopt;
    }

    auto style = find_style_node(styles_root, style_id);
    if (style == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id '" + std::string{style_id} +
                           "' was not found in word/styles.xml",
                       std::string{styles_xml_entry});
        return std::nullopt;
    }

    this->last_error_info.clear();
    return read_east_asia_font_family(style.child("w:rPr"));
}

std::optional<std::string>
Document::style_run_language(std::string_view style_id) {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before reading style run language",
            std::string{styles_xml_entry});
        return std::nullopt;
    }

    if (style_id.empty()) {
        set_last_error(
            this->last_error_info,
            std::make_error_code(std::errc::invalid_argument),
            "style id must not be empty when reading style run language",
            std::string{styles_xml_entry});
        return std::nullopt;
    }

    if (const auto error = this->ensure_styles_loaded()) {
        return std::nullopt;
    }

    const auto styles_root = this->styles.child("w:styles");
    if (styles_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       document_errc::styles_xml_parse_failed,
                       "word/styles.xml does not contain a w:styles root",
                       std::string{styles_xml_entry});
        return std::nullopt;
    }

    auto style = find_style_node(styles_root, style_id);
    if (style == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id '" + std::string{style_id} +
                           "' was not found in word/styles.xml",
                       std::string{styles_xml_entry});
        return std::nullopt;
    }

    this->last_error_info.clear();
    return read_primary_language(style.child("w:rPr"));
}

std::optional<std::string>
Document::style_run_east_asia_language(std::string_view style_id) {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before reading style run language",
            std::string{styles_xml_entry});
        return std::nullopt;
    }

    if (style_id.empty()) {
        set_last_error(
            this->last_error_info,
            std::make_error_code(std::errc::invalid_argument),
            "style id must not be empty when reading style run language",
            std::string{styles_xml_entry});
        return std::nullopt;
    }

    if (const auto error = this->ensure_styles_loaded()) {
        return std::nullopt;
    }

    const auto styles_root = this->styles.child("w:styles");
    if (styles_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       document_errc::styles_xml_parse_failed,
                       "word/styles.xml does not contain a w:styles root",
                       std::string{styles_xml_entry});
        return std::nullopt;
    }

    auto style = find_style_node(styles_root, style_id);
    if (style == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id '" + std::string{style_id} +
                           "' was not found in word/styles.xml",
                       std::string{styles_xml_entry});
        return std::nullopt;
    }

    this->last_error_info.clear();
    return read_east_asia_language(style.child("w:rPr"));
}

std::optional<std::string>
Document::style_run_bidi_language(std::string_view style_id) {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before reading style run language",
            std::string{styles_xml_entry});
        return std::nullopt;
    }

    if (style_id.empty()) {
        set_last_error(
            this->last_error_info,
            std::make_error_code(std::errc::invalid_argument),
            "style id must not be empty when reading style run language",
            std::string{styles_xml_entry});
        return std::nullopt;
    }

    if (const auto error = this->ensure_styles_loaded()) {
        return std::nullopt;
    }

    const auto styles_root = this->styles.child("w:styles");
    if (styles_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       document_errc::styles_xml_parse_failed,
                       "word/styles.xml does not contain a w:styles root",
                       std::string{styles_xml_entry});
        return std::nullopt;
    }

    auto style = find_style_node(styles_root, style_id);
    if (style == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id '" + std::string{style_id} +
                           "' was not found in word/styles.xml",
                       std::string{styles_xml_entry});
        return std::nullopt;
    }

    this->last_error_info.clear();
    return read_bidi_language(style.child("w:rPr"));
}

std::optional<bool> Document::style_run_rtl(std::string_view style_id) {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before reading style run direction",
            std::string{styles_xml_entry});
        return std::nullopt;
    }

    if (style_id.empty()) {
        set_last_error(
            this->last_error_info,
            std::make_error_code(std::errc::invalid_argument),
            "style id must not be empty when reading style run direction",
            std::string{styles_xml_entry});
        return std::nullopt;
    }

    if (const auto error = this->ensure_styles_loaded()) {
        return std::nullopt;
    }

    const auto styles_root = this->styles.child("w:styles");
    if (styles_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       document_errc::styles_xml_parse_failed,
                       "word/styles.xml does not contain a w:styles root",
                       std::string{styles_xml_entry});
        return std::nullopt;
    }

    const auto style = find_style_node(styles_root, style_id);
    if (style == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id '" + std::string{style_id} +
                           "' was not found in word/styles.xml",
                       std::string{styles_xml_entry});
        return std::nullopt;
    }

    this->last_error_info.clear();
    return read_run_rtl(style.child("w:rPr"));
}

std::optional<bool> Document::style_paragraph_bidi(std::string_view style_id) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before reading style "
                       "paragraph direction",
                       std::string{styles_xml_entry});
        return std::nullopt;
    }

    if (style_id.empty()) {
        set_last_error(
            this->last_error_info,
            std::make_error_code(std::errc::invalid_argument),
            "style id must not be empty when reading style paragraph direction",
            std::string{styles_xml_entry});
        return std::nullopt;
    }

    if (const auto error = this->ensure_styles_loaded()) {
        return std::nullopt;
    }

    const auto styles_root = this->styles.child("w:styles");
    if (styles_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       document_errc::styles_xml_parse_failed,
                       "word/styles.xml does not contain a w:styles root",
                       std::string{styles_xml_entry});
        return std::nullopt;
    }

    const auto style = find_style_node(styles_root, style_id);
    if (style == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id '" + std::string{style_id} +
                           "' was not found in word/styles.xml",
                       std::string{styles_xml_entry});
        return std::nullopt;
    }

    this->last_error_info.clear();
    return read_paragraph_bidi(style.child("w:pPr"));
}

std::optional<std::string>
Document::paragraph_style_next_style(std::string_view style_id) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before reading paragraph "
                       "style next style",
                       std::string{styles_xml_entry});
        return std::nullopt;
    }

    if (style_id.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id must not be empty when reading paragraph "
                       "style next style",
                       std::string{styles_xml_entry});
        return std::nullopt;
    }

    if (const auto error = this->ensure_styles_loaded()) {
        return std::nullopt;
    }

    const auto styles_root = this->styles.child("w:styles");
    if (styles_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       document_errc::styles_xml_parse_failed,
                       "word/styles.xml does not contain a w:styles root",
                       std::string{styles_xml_entry});
        return std::nullopt;
    }

    const auto style = find_style_node(styles_root, style_id);
    if (style == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id '" + std::string{style_id} +
                           "' was not found in word/styles.xml",
                       std::string{styles_xml_entry});
        return std::nullopt;
    }

    const auto type_name = std::string_view{style.attribute("w:type").value()};
    if (!type_name.empty() && type_name != "paragraph") {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id '" + std::string{style_id} +
                           "' is not a paragraph style",
                       std::string{styles_xml_entry});
        return std::nullopt;
    }

    if (const auto next = style.child("w:next").attribute("w:val");
        next != pugi::xml_attribute{} && next.value()[0] != '\0') {
        this->last_error_info.clear();
        return std::string{next.value()};
    }

    this->last_error_info.clear();
    return std::nullopt;
}

std::optional<std::uint32_t>
Document::paragraph_style_outline_level(std::string_view style_id) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before reading paragraph "
                       "style outline level",
                       std::string{styles_xml_entry});
        return std::nullopt;
    }

    if (style_id.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id must not be empty when reading paragraph "
                       "style outline level",
                       std::string{styles_xml_entry});
        return std::nullopt;
    }

    if (const auto error = this->ensure_styles_loaded()) {
        return std::nullopt;
    }

    const auto styles_root = this->styles.child("w:styles");
    if (styles_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       document_errc::styles_xml_parse_failed,
                       "word/styles.xml does not contain a w:styles root",
                       std::string{styles_xml_entry});
        return std::nullopt;
    }

    const auto style = find_style_node(styles_root, style_id);
    if (style == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id '" + std::string{style_id} +
                           "' was not found in word/styles.xml",
                       std::string{styles_xml_entry});
        return std::nullopt;
    }

    const auto type_name = std::string_view{style.attribute("w:type").value()};
    if (!type_name.empty() && type_name != "paragraph") {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id '" + std::string{style_id} +
                           "' is not a paragraph style",
                       std::string{styles_xml_entry});
        return std::nullopt;
    }

    this->last_error_info.clear();
    return parse_u32_attribute_value(
        style.child("w:pPr").child("w:outlineLvl").attribute("w:val").value());
}
