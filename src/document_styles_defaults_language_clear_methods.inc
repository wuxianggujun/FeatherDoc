bool Document::clear_default_run_primary_language() {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before editing default run language",
            std::string{styles_xml_entry});
        return false;
    }

    if (const auto error = this->ensure_styles_loaded()) {
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

    if (clear_language_attribute(styles_root.child("w:docDefaults")
                                     .child("w:rPrDefault")
                                     .child("w:rPr"),
                                 "w:val")) {
        this->styles_dirty = true;
    }

    this->last_error_info.clear();
    return true;
}

bool Document::clear_default_run_language() {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before editing default run language",
            std::string{styles_xml_entry});
        return false;
    }

    if (const auto error = this->ensure_styles_loaded()) {
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

    if (clear_language_attributes(styles_root.child("w:docDefaults")
                                      .child("w:rPrDefault")
                                      .child("w:rPr"))) {
        this->styles_dirty = true;
    }

    this->last_error_info.clear();
    return true;
}

bool Document::clear_default_run_east_asia_language() {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before editing default run language",
            std::string{styles_xml_entry});
        return false;
    }

    if (const auto error = this->ensure_styles_loaded()) {
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

    if (clear_language_attribute(styles_root.child("w:docDefaults")
                                     .child("w:rPrDefault")
                                     .child("w:rPr"),
                                 "w:eastAsia")) {
        this->styles_dirty = true;
    }

    this->last_error_info.clear();
    return true;
}

bool Document::clear_default_run_bidi_language() {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before editing default run language",
            std::string{styles_xml_entry});
        return false;
    }

    if (const auto error = this->ensure_styles_loaded()) {
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

    if (clear_language_attribute(styles_root.child("w:docDefaults")
                                     .child("w:rPrDefault")
                                     .child("w:rPr"),
                                 "w:bidi")) {
        this->styles_dirty = true;
    }

    this->last_error_info.clear();
    return true;
}
