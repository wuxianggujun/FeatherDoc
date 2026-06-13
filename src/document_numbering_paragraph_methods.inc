bool Document::set_paragraph_numbering(Paragraph target_paragraph,
                                       std::uint32_t numbering_definition_id,
                                       std::uint32_t level) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before editing paragraph numbering");
        return false;
    }

    if (!target_paragraph.has_next() || target_paragraph.current == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "target paragraph handle is not valid",
                       std::string{document_xml_entry});
        return false;
    }

    if (level > max_list_level) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "numbering level must be in the range [0, 8]",
                       std::string{document_xml_entry});
        return false;
    }

    if (const auto error = this->ensure_numbering_part_attached()) {
        return false;
    }

    auto numbering_root = this->numbering.child("w:numbering");
    if (numbering_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       document_errc::numbering_xml_parse_failed,
                       "word/numbering.xml does not contain the expected w:numbering root",
                       std::string{numbering_xml_entry});
        return false;
    }

    const auto abstract_num =
        find_abstract_numbering_by_id(numbering_root, numbering_definition_id);
    if (abstract_num == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "numbering definition id does not exist",
                       std::string{numbering_xml_entry});
        return false;
    }

    if (find_level_definition(abstract_num, level) == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "requested numbering level is not defined by the target definition",
                       std::string{numbering_xml_entry});
        return false;
    }

    const auto num_id =
        ensure_numbering_instance_for_abstract(numbering_root, numbering_definition_id);
    if (!num_id.has_value()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to create or reuse a numbering instance for the target definition",
                       std::string{numbering_xml_entry});
        return false;
    }

    if (!apply_numbering_to_paragraph(target_paragraph.current, level, *num_id)) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to populate numbering metadata for the target paragraph",
                       std::string{document_xml_entry});
        return false;
    }

    this->numbering_dirty = true;
    this->last_error_info.clear();
    return true;
}

bool Document::set_paragraph_style_numbering(std::string_view style_id,
                                             std::uint32_t numbering_definition_id,
                                             std::uint32_t level) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before editing style numbering",
                       std::string{styles_xml_entry});
        return false;
    }

    if (style_id.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id must not be empty when editing style numbering",
                       std::string{styles_xml_entry});
        return false;
    }

    if (level > max_list_level) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "numbering level must be in the range [0, 8]",
                       std::string{styles_xml_entry});
        return false;
    }

    if (const auto error = this->ensure_styles_part_attached()) {
        return false;
    }
    if (const auto error = this->ensure_numbering_part_attached()) {
        return false;
    }

    auto styles_root = this->styles.child("w:styles");
    if (styles_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info, document_errc::styles_xml_parse_failed,
                       "word/styles.xml does not contain a w:styles root",
                       std::string{styles_xml_entry});
        return false;
    }

    auto style = find_style_node(styles_root, style_id);
    if (style == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id '" + std::string{style_id} +
                           "' was not found in word/styles.xml",
                       std::string{styles_xml_entry});
        return false;
    }

    if (std::string_view{style.attribute("w:type").value()} != "paragraph") {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id '" + std::string{style_id} +
                           "' is not a paragraph style and cannot carry paragraph numbering",
                       std::string{styles_xml_entry});
        return false;
    }

    auto numbering_root = this->numbering.child("w:numbering");
    if (numbering_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       document_errc::numbering_xml_parse_failed,
                       "word/numbering.xml does not contain the expected w:numbering root",
                       std::string{numbering_xml_entry});
        return false;
    }

    const auto abstract_num =
        find_abstract_numbering_by_id(numbering_root, numbering_definition_id);
    if (abstract_num == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "numbering definition id does not exist",
                       std::string{numbering_xml_entry});
        return false;
    }

    if (find_level_definition(abstract_num, level) == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "requested numbering level is not defined by the target definition",
                       std::string{numbering_xml_entry});
        return false;
    }

    const auto num_id =
        ensure_numbering_instance_for_abstract(numbering_root, numbering_definition_id);
    if (!num_id.has_value()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to create or reuse a numbering instance for the target definition",
                       std::string{numbering_xml_entry});
        return false;
    }

    if (!apply_numbering_to_style(style, level, *num_id)) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to populate numbering metadata for the target style",
                       std::string{styles_xml_entry});
        return false;
    }

    this->numbering_dirty = true;
    this->styles_dirty = true;
    this->last_error_info.clear();
    return true;
}

bool Document::clear_paragraph_style_numbering(std::string_view style_id) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before editing style numbering",
                       std::string{styles_xml_entry});
        return false;
    }

    if (style_id.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id must not be empty when editing style numbering",
                       std::string{styles_xml_entry});
        return false;
    }

    if (const auto error = this->ensure_styles_part_attached()) {
        return false;
    }

    auto styles_root = this->styles.child("w:styles");
    if (styles_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info, document_errc::styles_xml_parse_failed,
                       "word/styles.xml does not contain a w:styles root",
                       std::string{styles_xml_entry});
        return false;
    }

    auto style = find_style_node(styles_root, style_id);
    if (style == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id '" + std::string{style_id} +
                           "' was not found in word/styles.xml",
                       std::string{styles_xml_entry});
        return false;
    }

    if (std::string_view{style.attribute("w:type").value()} != "paragraph") {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id '" + std::string{style_id} +
                           "' is not a paragraph style and cannot carry paragraph numbering",
                       std::string{styles_xml_entry});
        return false;
    }

    if (auto paragraph_properties = style.child("w:pPr"); paragraph_properties != pugi::xml_node{}) {
        paragraph_properties.remove_child("w:numPr");
        remove_empty_style_paragraph_properties(style);
        this->styles_dirty = true;
    }

    this->last_error_info.clear();
    return true;
}

bool Document::set_paragraph_list(Paragraph target_paragraph, featherdoc::list_kind kind,
                                  std::uint32_t level) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before editing paragraph lists");
        return false;
    }

    if (!target_paragraph.has_next() || target_paragraph.current == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "target paragraph handle is not valid",
                       std::string{document_xml_entry});
        return false;
    }

    if (level > max_list_level) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "list level must be in the range [0, 8]",
                       std::string{document_xml_entry});
        return false;
    }

    if (const auto error = this->ensure_numbering_part_attached()) {
        return false;
    }

    auto numbering_root = this->numbering.child("w:numbering");
    if (numbering_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       document_errc::numbering_xml_parse_failed,
                       "word/numbering.xml does not contain the expected w:numbering root",
                       std::string{numbering_xml_entry});
        return false;
    }

    const auto num_id =
        apply_list_numbering(target_paragraph.current, numbering_root, kind, level, false);
    if (!num_id.has_value()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to create or reuse a managed numbering definition",
                       std::string{numbering_xml_entry});
        return false;
    }

    if (!apply_numbering_to_paragraph(target_paragraph.current, level, *num_id)) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to populate numbering metadata for the target paragraph",
                       std::string{document_xml_entry});
        return false;
    }

    this->numbering_dirty = true;
    this->last_error_info.clear();
    return true;
}

bool Document::restart_paragraph_list(Paragraph target_paragraph, featherdoc::list_kind kind,
                                      std::uint32_t level) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before editing paragraph lists");
        return false;
    }

    if (!target_paragraph.has_next() || target_paragraph.current == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "target paragraph handle is not valid",
                       std::string{document_xml_entry});
        return false;
    }

    if (level > max_list_level) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "list level must be in the range [0, 8]",
                       std::string{document_xml_entry});
        return false;
    }

    if (const auto error = this->ensure_numbering_part_attached()) {
        return false;
    }

    auto numbering_root = this->numbering.child("w:numbering");
    if (numbering_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       document_errc::numbering_xml_parse_failed,
                       "word/numbering.xml does not contain the expected w:numbering root",
                       std::string{numbering_xml_entry});
        return false;
    }

    const auto num_id =
        apply_list_numbering(target_paragraph.current, numbering_root, kind, level, true);
    if (!num_id.has_value()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to create a restarted managed numbering definition",
                       std::string{numbering_xml_entry});
        return false;
    }

    if (!apply_numbering_to_paragraph(target_paragraph.current, level, *num_id)) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to populate numbering metadata for the target paragraph",
                       std::string{document_xml_entry});
        return false;
    }

    this->numbering_dirty = true;
    this->last_error_info.clear();
    return true;
}

bool Document::clear_paragraph_list(Paragraph target_paragraph) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before editing paragraph lists");
        return false;
    }

    if (!target_paragraph.has_next() || target_paragraph.current == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "target paragraph handle is not valid",
                       std::string{document_xml_entry});
        return false;
    }

    if (auto paragraph_properties = target_paragraph.current.child("w:pPr");
        paragraph_properties != pugi::xml_node{}) {
        paragraph_properties.remove_child("w:numPr");
        remove_empty_paragraph_properties(target_paragraph.current);
    }

    this->last_error_info.clear();
    return true;
}
