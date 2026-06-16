std::size_t TemplatePart::replace_bookmark_text(const std::string &bookmark_name,
                                                const std::string &replacement) {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return 0U;
    }

    return replace_bookmark_text_in_part(*this->last_error_info, *this->xml_document,
                                         this->entry_name_storage, bookmark_name,
                                         replacement);
}

std::size_t TemplatePart::replace_bookmark_text(const char *bookmark_name,
                                                const char *replacement) {
    if (bookmark_name == nullptr || replacement == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           "bookmark name and replacement text must not be null",
                           this->entry_name_storage);
        }
        return 0U;
    }

    return this->replace_bookmark_text(std::string{bookmark_name},
                                       std::string{replacement});
}

bookmark_fill_result TemplatePart::fill_bookmarks(
    std::span<const bookmark_text_binding> bindings) {
    bookmark_fill_result result;
    result.requested = bindings.size();

    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return result;
    }

    return fill_bookmarks_in_part(*this->last_error_info, *this->xml_document,
                                  this->entry_name_storage, bindings);
}

bookmark_fill_result TemplatePart::fill_bookmarks(
    std::initializer_list<bookmark_text_binding> bindings) {
    return this->fill_bookmarks(
        std::span<const bookmark_text_binding>{bindings.begin(), bindings.size()});
}

std::vector<featherdoc::bookmark_summary> TemplatePart::list_bookmarks() const {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return {};
    }

    return summarize_bookmarks_in_part(*this->last_error_info, *this->xml_document);
}

std::optional<featherdoc::bookmark_summary> TemplatePart::find_bookmark(
    std::string_view bookmark_name) const {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return std::nullopt;
    }

    return find_bookmark_in_part(*this->last_error_info, *this->xml_document,
                                 this->entry_name_storage, bookmark_name);
}

