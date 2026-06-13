std::size_t Document::replace_bookmark_text(const std::string &bookmark_name,
                                            const std::string &replacement) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before filling bookmark text");
        return 0U;
    }

    return replace_bookmark_text_in_part(this->last_error_info, this->document,
                                         document_xml_entry, bookmark_name,
                                         replacement);
}

std::size_t Document::replace_bookmark_text(const char *bookmark_name,
                                            const char *replacement) {
    if (bookmark_name == nullptr || replacement == nullptr) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "bookmark name and replacement text must not be null",
                       std::string{document_xml_entry});
        return 0U;
    }

    return this->replace_bookmark_text(std::string{bookmark_name},
                                       std::string{replacement});
}

bookmark_fill_result Document::fill_bookmarks(
    std::span<const bookmark_text_binding> bindings) {
    bookmark_fill_result result;
    result.requested = bindings.size();

    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before filling bookmarks");
        return result;
    }

    return fill_bookmarks_in_part(this->last_error_info, this->document,
                                  document_xml_entry, bindings);
}

bookmark_fill_result Document::fill_bookmarks(
    std::initializer_list<bookmark_text_binding> bindings) {
    return this->fill_bookmarks(
        std::span<const bookmark_text_binding>{bindings.begin(), bindings.size()});
}

std::vector<featherdoc::bookmark_summary> Document::list_bookmarks() const {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before listing bookmarks",
                       std::string{document_xml_entry});
        return {};
    }

    return summarize_bookmarks_in_part(this->last_error_info,
                                       const_cast<pugi::xml_document &>(this->document));
}

std::optional<featherdoc::bookmark_summary> Document::find_bookmark(
    std::string_view bookmark_name) const {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before reading bookmark metadata",
                       std::string{document_xml_entry});
        return std::nullopt;
    }

    return find_bookmark_in_part(this->last_error_info,
                                 const_cast<pugi::xml_document &>(this->document),
                                 document_xml_entry, bookmark_name);
}
