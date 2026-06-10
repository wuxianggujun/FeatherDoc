bool Document::clear_style_run_text_color(std::string_view style_id) {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before editing style run color",
            std::string{styles_xml_entry});
        return false;
    }
    if (style_id.empty()) {
        set_last_error(
            this->last_error_info,
            std::make_error_code(std::errc::invalid_argument),
            "style id must not be empty when editing style run color",
            std::string{styles_xml_entry});
        return false;
    }
    if (const auto error = this->ensure_styles_loaded()) {
        return false;
    }
    const auto styles_root = this->styles.child("w:styles");
    auto style = find_style_node(styles_root, style_id);
    if (style == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id '" + std::string{style_id} +
                           "' was not found in word/styles.xml",
                       std::string{styles_xml_entry});
        return false;
    }
    if (clear_run_text_color(style.child("w:rPr"))) {
        remove_empty_style_run_properties(style);
        this->styles_dirty = true;
    }
    this->last_error_info.clear();
    return true;
}

bool Document::clear_style_run_bold(std::string_view style_id) {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before editing style run bold",
            std::string{styles_xml_entry});
        return false;
    }
    if (style_id.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id must not be empty when editing style run bold",
                       std::string{styles_xml_entry});
        return false;
    }
    if (const auto error = this->ensure_styles_loaded()) {
        return false;
    }
    const auto styles_root = this->styles.child("w:styles");
    auto style = find_style_node(styles_root, style_id);
    if (style == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id '" + std::string{style_id} +
                           "' was not found in word/styles.xml",
                       std::string{styles_xml_entry});
        return false;
    }
    if (clear_run_bold(style.child("w:rPr"))) {
        remove_empty_style_run_properties(style);
        this->styles_dirty = true;
    }
    this->last_error_info.clear();
    return true;
}

bool Document::clear_style_run_italic(std::string_view style_id) {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before editing style run italic",
            std::string{styles_xml_entry});
        return false;
    }
    if (style_id.empty()) {
        set_last_error(
            this->last_error_info,
            std::make_error_code(std::errc::invalid_argument),
            "style id must not be empty when editing style run italic",
            std::string{styles_xml_entry});
        return false;
    }
    if (const auto error = this->ensure_styles_loaded()) {
        return false;
    }
    const auto styles_root = this->styles.child("w:styles");
    auto style = find_style_node(styles_root, style_id);
    if (style == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id '" + std::string{style_id} +
                           "' was not found in word/styles.xml",
                       std::string{styles_xml_entry});
        return false;
    }
    if (clear_run_italic(style.child("w:rPr"))) {
        remove_empty_style_run_properties(style);
        this->styles_dirty = true;
    }
    this->last_error_info.clear();
    return true;
}

bool Document::clear_style_run_strikethrough(std::string_view style_id) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before editing style "
                       "run strikethrough",
                       std::string{styles_xml_entry});
        return false;
    }
    if (style_id.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id must not be empty when editing style run "
                       "strikethrough",
                       std::string{styles_xml_entry});
        return false;
    }
    if (const auto error = this->ensure_styles_loaded()) {
        return false;
    }
    const auto styles_root = this->styles.child("w:styles");
    auto style = find_style_node(styles_root, style_id);
    if (style == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id '" + std::string{style_id} +
                           "' was not found in word/styles.xml",
                       std::string{styles_xml_entry});
        return false;
    }
    if (clear_run_strikethrough(style.child("w:rPr"))) {
        remove_empty_style_run_properties(style);
        this->styles_dirty = true;
    }
    this->last_error_info.clear();
    return true;
}

bool Document::clear_style_run_underline(std::string_view style_id) {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before editing style run underline",
            std::string{styles_xml_entry});
        return false;
    }
    if (style_id.empty()) {
        set_last_error(
            this->last_error_info,
            std::make_error_code(std::errc::invalid_argument),
            "style id must not be empty when editing style run underline",
            std::string{styles_xml_entry});
        return false;
    }
    if (const auto error = this->ensure_styles_loaded()) {
        return false;
    }
    const auto styles_root = this->styles.child("w:styles");
    auto style = find_style_node(styles_root, style_id);
    if (style == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id '" + std::string{style_id} +
                           "' was not found in word/styles.xml",
                       std::string{styles_xml_entry});
        return false;
    }
    if (clear_run_underline(style.child("w:rPr"))) {
        remove_empty_style_run_properties(style);
        this->styles_dirty = true;
    }
    this->last_error_info.clear();
    return true;
}

bool Document::clear_style_run_superscript(std::string_view style_id) {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before editing style run "
            "superscript",
            std::string{styles_xml_entry});
        return false;
    }
    if (style_id.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id must not be empty when editing style run "
                       "superscript",
                       std::string{styles_xml_entry});
        return false;
    }
    if (const auto error = this->ensure_styles_loaded()) {
        return false;
    }
    const auto styles_root = this->styles.child("w:styles");
    auto style = find_style_node(styles_root, style_id);
    if (style == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id '" + std::string{style_id} +
                           "' was not found in word/styles.xml",
                       std::string{styles_xml_entry});
        return false;
    }
    if (clear_run_vertical_align(style.child("w:rPr"))) {
        remove_empty_style_run_properties(style);
        this->styles_dirty = true;
    }
    this->last_error_info.clear();
    return true;
}

bool Document::clear_style_run_subscript(std::string_view style_id) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before editing style "
                       "run subscript",
                       std::string{styles_xml_entry});
        return false;
    }
    if (style_id.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id must not be empty when editing style run "
                       "subscript",
                       std::string{styles_xml_entry});
        return false;
    }
    if (const auto error = this->ensure_styles_loaded()) {
        return false;
    }
    const auto styles_root = this->styles.child("w:styles");
    auto style = find_style_node(styles_root, style_id);
    if (style == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id '" + std::string{style_id} +
                           "' was not found in word/styles.xml",
                       std::string{styles_xml_entry});
        return false;
    }
    if (clear_run_vertical_align(style.child("w:rPr"))) {
        remove_empty_style_run_properties(style);
        this->styles_dirty = true;
    }
    this->last_error_info.clear();
    return true;
}

bool Document::clear_style_run_font_size_points(std::string_view style_id) {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before editing style run font size",
            std::string{styles_xml_entry});
        return false;
    }
    if (style_id.empty()) {
        set_last_error(
            this->last_error_info,
            std::make_error_code(std::errc::invalid_argument),
            "style id must not be empty when editing style run font size",
            std::string{styles_xml_entry});
        return false;
    }
    if (const auto error = this->ensure_styles_loaded()) {
        return false;
    }
    const auto styles_root = this->styles.child("w:styles");
    auto style = find_style_node(styles_root, style_id);
    if (style == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id '" + std::string{style_id} +
                           "' was not found in word/styles.xml",
                       std::string{styles_xml_entry});
        return false;
    }
    if (clear_run_font_size_points(style.child("w:rPr"))) {
        remove_empty_style_run_properties(style);
        this->styles_dirty = true;
    }
    this->last_error_info.clear();
    return true;
}
