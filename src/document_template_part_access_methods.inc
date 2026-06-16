TemplatePart Document::body_template() {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before accessing the body template");
        return {this, nullptr, &this->last_error_info, {}};
    }

    this->last_error_info.clear();
    return {this, &this->document, &this->last_error_info,
            std::string{document_xml_entry}};
}

TemplatePart Document::header_template(std::size_t index) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before accessing a header template");
        return {this, nullptr, &this->last_error_info, {}};
    }

    if (index >= this->header_parts.size()) {
        this->last_error_info.clear();
        return {this, nullptr, &this->last_error_info, {}};
    }

    this->last_error_info.clear();
    return {this, &this->header_parts[index]->xml, &this->last_error_info,
            this->header_parts[index]->entry_name};
}

TemplatePart Document::footer_template(std::size_t index) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before accessing a footer template");
        return {this, nullptr, &this->last_error_info, {}};
    }

    if (index >= this->footer_parts.size()) {
        this->last_error_info.clear();
        return {this, nullptr, &this->last_error_info, {}};
    }

    this->last_error_info.clear();
    return {this, &this->footer_parts[index]->xml, &this->last_error_info,
            this->footer_parts[index]->entry_name};
}

TemplatePart Document::section_header_template(
    std::size_t section_index, featherdoc::section_reference_kind reference_kind) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before accessing a section header "
                       "template");
        return {this, nullptr, &this->last_error_info, {}};
    }

    auto *part = this->section_related_part_state(section_index, reference_kind,
                                                  this->header_parts,
                                                  "w:headerReference");
    if (part == nullptr) {
        this->last_error_info.clear();
        return {this, nullptr, &this->last_error_info, {}};
    }

    this->last_error_info.clear();
    return {this, &part->xml, &this->last_error_info, part->entry_name};
}

TemplatePart Document::section_footer_template(
    std::size_t section_index, featherdoc::section_reference_kind reference_kind) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before accessing a section footer "
                       "template");
        return {this, nullptr, &this->last_error_info, {}};
    }

    auto *part = this->section_related_part_state(section_index, reference_kind,
                                                  this->footer_parts,
                                                  "w:footerReference");
    if (part == nullptr) {
        this->last_error_info.clear();
        return {this, nullptr, &this->last_error_info, {}};
    }

    this->last_error_info.clear();
    return {this, &part->xml, &this->last_error_info, part->entry_name};
}
