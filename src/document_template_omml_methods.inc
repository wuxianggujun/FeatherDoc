std::vector<featherdoc::omml_summary> Document::list_omml() const {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before listing OMML equations",
                       std::string{document_xml_entry});
        return {};
    }

    return summarize_omml_in_part(
        this->last_error_info, const_cast<pugi::xml_document &>(this->document));
}

bool Document::append_omml(std::string_view omml_xml) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before appending OMML",
                       std::string{document_xml_entry});
        return false;
    }

    return append_omml_to_part(this->last_error_info, this->document,
                               document_xml_entry, omml_xml);
}

bool Document::replace_omml(std::size_t omml_index, std::string_view omml_xml) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before replacing OMML",
                       std::string{document_xml_entry});
        return false;
    }

    return replace_omml_in_part(this->last_error_info, this->document,
                                document_xml_entry, omml_index, omml_xml);
}

bool Document::remove_omml(std::size_t omml_index) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before removing OMML",
                       std::string{document_xml_entry});
        return false;
    }

    return remove_omml_in_part(this->last_error_info, this->document,
                               document_xml_entry, omml_index);
}
