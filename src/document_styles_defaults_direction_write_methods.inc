bool Document::set_default_run_rtl(bool enabled) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before editing default "
                       "run direction",
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

    const auto rtl = ensure_run_rtl_node(run_properties);
    if (rtl == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to create docDefaults default w:rtl",
                       std::string{styles_xml_entry});
        return false;
    }

    set_on_off_value(rtl, enabled);
    this->styles_dirty = true;
    this->last_error_info.clear();
    return true;
}

bool Document::set_default_paragraph_bidi(bool enabled) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before editing default "
                       "paragraph direction",
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

    const auto paragraph_properties =
        ensure_doc_defaults_paragraph_properties_node(styles_root);
    if (paragraph_properties == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to create docDefaults/w:pPrDefault/w:pPr",
                       std::string{styles_xml_entry});
        return false;
    }

    const auto bidi = ensure_paragraph_bidi_node(paragraph_properties);
    if (bidi == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to create docDefaults default w:bidi",
                       std::string{styles_xml_entry});
        return false;
    }

    set_on_off_value(bidi, enabled);
    this->styles_dirty = true;
    this->last_error_info.clear();
    return true;
}
