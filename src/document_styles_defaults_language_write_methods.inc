bool Document::set_default_run_language(std::string_view language) {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before editing default run language",
            this->extension_state().singleton_part_entries.styles);
        return false;
    }

    if (language.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "default run language must not be empty",
                       this->extension_state().singleton_part_entries.styles);
        return false;
    }

    if (const auto error = this->ensure_styles_part_attached()) {
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

    const auto run_properties =
        ensure_doc_defaults_run_properties_node(styles_root);
    if (run_properties == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to create docDefaults/w:rPrDefault/w:rPr",
                       this->extension_state().singleton_part_entries.styles);
        return false;
    }

    const auto run_language = ensure_run_language_node(run_properties);
    if (run_language == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to create docDefaults default w:lang",
                       this->extension_state().singleton_part_entries.styles);
        return false;
    }

    ensure_attribute_value(run_language, "w:val", language);
    this->styles_dirty = true;
    this->last_error_info.clear();
    return true;
}

bool Document::set_default_run_east_asia_language(std::string_view language) {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before editing default run language",
            this->extension_state().singleton_part_entries.styles);
        return false;
    }

    if (language.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "default eastAsia language must not be empty",
                       this->extension_state().singleton_part_entries.styles);
        return false;
    }

    if (const auto error = this->ensure_styles_part_attached()) {
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

    const auto run_properties =
        ensure_doc_defaults_run_properties_node(styles_root);
    if (run_properties == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to create docDefaults/w:rPrDefault/w:rPr",
                       this->extension_state().singleton_part_entries.styles);
        return false;
    }

    const auto run_language = ensure_run_language_node(run_properties);
    if (run_language == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to create docDefaults default w:lang",
                       this->extension_state().singleton_part_entries.styles);
        return false;
    }

    ensure_attribute_value(run_language, "w:eastAsia", language);
    this->styles_dirty = true;
    this->last_error_info.clear();
    return true;
}

bool Document::set_default_run_bidi_language(std::string_view language) {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before editing default run language",
            this->extension_state().singleton_part_entries.styles);
        return false;
    }

    if (language.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "default bidi language must not be empty",
                       this->extension_state().singleton_part_entries.styles);
        return false;
    }

    if (const auto error = this->ensure_styles_part_attached()) {
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

    const auto run_properties =
        ensure_doc_defaults_run_properties_node(styles_root);
    if (run_properties == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to create docDefaults/w:rPrDefault/w:rPr",
                       this->extension_state().singleton_part_entries.styles);
        return false;
    }

    const auto run_language = ensure_run_language_node(run_properties);
    if (run_language == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to create docDefaults default w:lang",
                       this->extension_state().singleton_part_entries.styles);
        return false;
    }

    ensure_attribute_value(run_language, "w:bidi", language);
    this->styles_dirty = true;
    this->last_error_info.clear();
    return true;
}
