bool Document::set_paragraph_style(Paragraph paragraph_handle,
                                   std::string_view style_id) {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before editing paragraph styles");
        return false;
    }

    if (style_id.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "paragraph style id must not be empty",
                       std::string{document_xml_entry});
        return false;
    }

    if (!paragraph_handle.has_next() ||
        paragraph_handle.current == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "target paragraph handle is not valid",
                       std::string{document_xml_entry});
        return false;
    }

    if (const auto error = this->ensure_styles_part_attached()) {
        return false;
    }

    auto paragraph_properties =
        ensure_paragraph_properties_node(paragraph_handle.current);
    if (paragraph_properties == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to create w:pPr for the target paragraph",
                       std::string{document_xml_entry});
        return false;
    }

    auto style_node = paragraph_properties.child("w:pStyle");
    if (style_node == pugi::xml_node{}) {
        if (const auto first_child = paragraph_properties.first_child();
            first_child != pugi::xml_node{}) {
            style_node = paragraph_properties.insert_child_before("w:pStyle",
                                                                  first_child);
        } else {
            style_node = paragraph_properties.append_child("w:pStyle");
        }
    }
    if (style_node == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to create w:pStyle for the target paragraph",
                       std::string{document_xml_entry});
        return false;
    }

    ensure_attribute_value(style_node, "w:val", style_id);
    this->last_error_info.clear();
    return true;
}

bool Document::clear_paragraph_style(Paragraph paragraph_handle) {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before editing paragraph styles");
        return false;
    }

    if (!paragraph_handle.has_next() ||
        paragraph_handle.current == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "target paragraph handle is not valid",
                       std::string{document_xml_entry});
        return false;
    }

    if (auto paragraph_properties = paragraph_handle.current.child("w:pPr");
        paragraph_properties != pugi::xml_node{}) {
        paragraph_properties.remove_child("w:pStyle");
        remove_empty_paragraph_properties(paragraph_handle.current);
    }

    this->last_error_info.clear();
    return true;
}

bool Document::set_run_style(Run run_handle, std::string_view style_id) {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before editing run styles");
        return false;
    }

    if (style_id.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "run style id must not be empty",
                       std::string{document_xml_entry});
        return false;
    }

    if (!run_handle.has_next() || run_handle.current == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "target run handle is not valid",
                       std::string{document_xml_entry});
        return false;
    }

    if (const auto error = this->ensure_styles_part_attached()) {
        return false;
    }

    auto run_properties = ensure_run_properties_node(run_handle.current);
    if (run_properties == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to create w:rPr for the target run",
                       std::string{document_xml_entry});
        return false;
    }

    auto style_node = run_properties.child("w:rStyle");
    if (style_node == pugi::xml_node{}) {
        if (const auto first_child = run_properties.first_child();
            first_child != pugi::xml_node{}) {
            style_node =
                run_properties.insert_child_before("w:rStyle", first_child);
        } else {
            style_node = run_properties.append_child("w:rStyle");
        }
    }
    if (style_node == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to create w:rStyle for the target run",
                       std::string{document_xml_entry});
        return false;
    }

    ensure_attribute_value(style_node, "w:val", style_id);
    this->last_error_info.clear();
    return true;
}

bool Document::clear_run_style(Run run_handle) {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before editing run styles");
        return false;
    }

    if (!run_handle.has_next() || run_handle.current == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "target run handle is not valid",
                       std::string{document_xml_entry});
        return false;
    }

    if (auto run_properties = run_handle.current.child("w:rPr");
        run_properties != pugi::xml_node{}) {
        run_properties.remove_child("w:rStyle");
        remove_empty_run_properties(run_handle.current);
    }

    this->last_error_info.clear();
    return true;
}
