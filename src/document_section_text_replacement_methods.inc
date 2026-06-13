bool Document::replace_section_header_text(
    std::size_t section_index, std::string_view replacement_text,
    featherdoc::section_reference_kind reference_kind) {
    this->ensure_section_header_paragraphs(section_index, reference_kind);

    auto *part = this->section_related_part_state(section_index, reference_kind,
                                                  this->header_parts,
                                                  "w:headerReference");
    if (part == nullptr) {
        if (!this->last_error_info) {
            set_last_error(this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           "failed to resolve the requested section header part",
                           std::string{document_xml_entry});
        }
        return false;
    }

    auto root = part->xml.child("w:hdr");
    if (root == pugi::xml_node{}) {
        set_last_error(this->last_error_info, document_errc::related_part_parse_failed,
                       "header part does not contain the expected w:hdr root",
                       part->entry_name);
        return false;
    }

    root.remove_children();
    for (const auto &paragraph_text : split_plain_text_paragraphs(replacement_text)) {
        if (!featherdoc::detail::append_plain_text_paragraph(root, paragraph_text)) {
            set_last_error(this->last_error_info,
                           std::make_error_code(std::errc::not_enough_memory),
                           "failed to rebuild the requested section header paragraphs",
                           part->entry_name);
            return false;
        }
    }

    part->paragraph.set_parent(root);
    this->last_error_info.clear();
    return true;
}

bool Document::replace_section_footer_text(
    std::size_t section_index, std::string_view replacement_text,
    featherdoc::section_reference_kind reference_kind) {
    this->ensure_section_footer_paragraphs(section_index, reference_kind);

    auto *part = this->section_related_part_state(section_index, reference_kind,
                                                  this->footer_parts,
                                                  "w:footerReference");
    if (part == nullptr) {
        if (!this->last_error_info) {
            set_last_error(this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           "failed to resolve the requested section footer part",
                           std::string{document_xml_entry});
        }
        return false;
    }

    auto root = part->xml.child("w:ftr");
    if (root == pugi::xml_node{}) {
        set_last_error(this->last_error_info, document_errc::related_part_parse_failed,
                       "footer part does not contain the expected w:ftr root",
                       part->entry_name);
        return false;
    }

    root.remove_children();
    for (const auto &paragraph_text : split_plain_text_paragraphs(replacement_text)) {
        if (!featherdoc::detail::append_plain_text_paragraph(root, paragraph_text)) {
            set_last_error(this->last_error_info,
                           std::make_error_code(std::errc::not_enough_memory),
                           "failed to rebuild the requested section footer paragraphs",
                           part->entry_name);
            return false;
        }
    }

    part->paragraph.set_parent(root);
    this->last_error_info.clear();
    return true;
}
