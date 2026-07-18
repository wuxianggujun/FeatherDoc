bool Document::clear_default_run_rtl() {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before editing default "
                       "run direction",
                       this->extension_state().singleton_part_entries.styles);
        return false;
    }

    if (const auto error = this->ensure_styles_loaded()) {
        return false;
    }

    const auto styles_root = this->styles.child("w:styles");
    if (styles_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       document_errc::styles_xml_parse_failed,
                       "'" + this->extension_state().singleton_part_entries.styles + "' does not contain a w:styles root",
                       this->extension_state().singleton_part_entries.styles);
        return false;
    }

    if (clear_on_off_child(styles_root.child("w:docDefaults")
                               .child("w:rPrDefault")
                               .child("w:rPr"),
                           "w:rtl")) {
        this->styles_dirty = true;
    }

    this->last_error_info.clear();
    return true;
}

bool Document::clear_default_paragraph_bidi() {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before editing default "
                       "paragraph direction",
                       this->extension_state().singleton_part_entries.styles);
        return false;
    }

    if (const auto error = this->ensure_styles_loaded()) {
        return false;
    }

    const auto styles_root = this->styles.child("w:styles");
    if (styles_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       document_errc::styles_xml_parse_failed,
                       "'" + this->extension_state().singleton_part_entries.styles + "' does not contain a w:styles root",
                       this->extension_state().singleton_part_entries.styles);
        return false;
    }

    if (clear_on_off_child(styles_root.child("w:docDefaults")
                               .child("w:pPrDefault")
                               .child("w:pPr"),
                           "w:bidi")) {
        this->styles_dirty = true;
    }

    this->last_error_info.clear();
    return true;
}
