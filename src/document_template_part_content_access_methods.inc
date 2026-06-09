std::vector<featherdoc::content_control_summary>
TemplatePart::list_content_controls() const {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return {};
    }

    return summarize_content_controls_in_part(*this->last_error_info,
                                              *this->xml_document);
}

std::vector<featherdoc::hyperlink_summary> TemplatePart::list_hyperlinks() const {
    if (this->xml_document == nullptr || this->last_error_info == nullptr ||
        this->owner == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return {};
    }

    return this->owner->list_hyperlinks_in_part(*this->xml_document,
                                                this->entry_name_storage);
}

std::size_t TemplatePart::append_hyperlink(std::string_view text,
                                           std::string_view target) {
    if (this->xml_document == nullptr || this->last_error_info == nullptr ||
        this->owner == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return 0U;
    }

    return this->owner->append_hyperlink_in_part(
        *this->xml_document, this->entry_name_storage, text, target);
}

bool TemplatePart::replace_hyperlink(std::size_t hyperlink_index,
                                      std::string_view text,
                                      std::string_view target) {
    if (this->xml_document == nullptr || this->last_error_info == nullptr ||
        this->owner == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return false;
    }

    return this->owner->replace_hyperlink_in_part(
        *this->xml_document, this->entry_name_storage, hyperlink_index, text, target);
}

bool TemplatePart::remove_hyperlink(std::size_t hyperlink_index) {
    if (this->xml_document == nullptr || this->last_error_info == nullptr ||
        this->owner == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return false;
    }

    return this->owner->remove_hyperlink_in_part(
        *this->xml_document, this->entry_name_storage, hyperlink_index);
}

std::vector<featherdoc::content_control_summary>
TemplatePart::find_content_controls_by_tag(std::string_view tag) const {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return {};
    }

    return filter_content_controls_by_tag_or_alias(
        *this->last_error_info, *this->xml_document, this->entry_name_storage,
        tag, true);
}

std::vector<featherdoc::content_control_summary>
TemplatePart::find_content_controls_by_alias(std::string_view alias) const {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return {};
    }

    return filter_content_controls_by_tag_or_alias(
        *this->last_error_info, *this->xml_document, this->entry_name_storage,
        alias, false);
}

