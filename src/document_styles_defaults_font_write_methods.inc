bool Document::set_default_run_font_family(std::string_view font_family) {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before editing default run fonts");
        return false;
    }

    if (font_family.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "default run font family must not be empty",
                       std::string{styles_xml_entry});
        return false;
    }

    if (const auto error = this->ensure_styles_part_attached()) {
        return false;
    }

    const auto styles_root = this->styles.child("w:styles");
    if (styles_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       document_errc::styles_xml_parse_failed,
                       "word/styles.xml does not contain a w:styles root",
                       std::string{styles_xml_entry});
        return false;
    }

    const auto run_properties =
        ensure_doc_defaults_run_properties_node(styles_root);
    if (run_properties == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to create docDefaults/w:rPrDefault/w:rPr",
                       std::string{styles_xml_entry});
        return false;
    }

    const auto run_fonts = ensure_run_fonts_node(run_properties);
    if (run_fonts == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to create docDefaults default w:rFonts",
                       std::string{styles_xml_entry});
        return false;
    }

    ensure_attribute_value(run_fonts, "w:ascii", font_family);
    ensure_attribute_value(run_fonts, "w:hAnsi", font_family);
    ensure_attribute_value(run_fonts, "w:cs", font_family);
    this->styles_dirty = true;
    this->last_error_info.clear();
    return true;
}

bool Document::set_default_run_east_asia_font_family(
    std::string_view font_family) {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before editing default run fonts");
        return false;
    }

    if (font_family.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "default eastAsia font family must not be empty",
                       std::string{styles_xml_entry});
        return false;
    }

    if (const auto error = this->ensure_styles_part_attached()) {
        return false;
    }

    const auto styles_root = this->styles.child("w:styles");
    if (styles_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       document_errc::styles_xml_parse_failed,
                       "word/styles.xml does not contain a w:styles root",
                       std::string{styles_xml_entry});
        return false;
    }

    const auto run_properties =
        ensure_doc_defaults_run_properties_node(styles_root);
    if (run_properties == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to create docDefaults/w:rPrDefault/w:rPr",
                       std::string{styles_xml_entry});
        return false;
    }

    const auto run_fonts = ensure_run_fonts_node(run_properties);
    if (run_fonts == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to create docDefaults default w:rFonts",
                       std::string{styles_xml_entry});
        return false;
    }

    ensure_attribute_value(run_fonts, "w:eastAsia", font_family);
    this->styles_dirty = true;
    this->last_error_info.clear();
    return true;
}
