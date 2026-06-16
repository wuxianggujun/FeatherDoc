std::size_t TemplatePart::replace_content_control_text_by_tag(
    std::string_view tag, std::string_view replacement) {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return 0U;
    }

    return replace_content_control_text_by_tag_or_alias_in_part(
        *this->last_error_info, *this->xml_document, this->entry_name_storage,
        tag, replacement, true);
}

std::size_t TemplatePart::replace_content_control_text_by_alias(
    std::string_view alias, std::string_view replacement) {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return 0U;
    }

    return replace_content_control_text_by_tag_or_alias_in_part(
        *this->last_error_info, *this->xml_document, this->entry_name_storage,
        alias, replacement, false);
}

std::size_t TemplatePart::set_content_control_form_state_by_tag(
    std::string_view tag,
    const featherdoc::content_control_form_state_options &options) {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return 0U;
    }

    return set_content_control_form_state_by_tag_or_alias_in_part(
        *this->last_error_info, *this->xml_document, this->entry_name_storage,
        tag, options, true);
}

std::size_t TemplatePart::set_content_control_form_state_by_alias(
    std::string_view alias,
    const featherdoc::content_control_form_state_options &options) {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return 0U;
    }

    return set_content_control_form_state_by_tag_or_alias_in_part(
        *this->last_error_info, *this->xml_document, this->entry_name_storage,
        alias, options, false);
}

std::size_t TemplatePart::replace_content_control_with_paragraphs_by_tag(
    std::string_view tag, const std::vector<std::string> &paragraphs) {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return 0U;
    }

    return replace_content_control_with_paragraphs_by_tag_or_alias_in_part(
        *this->last_error_info, *this->xml_document, this->entry_name_storage,
        tag, paragraphs, true);
}

std::size_t TemplatePart::replace_content_control_with_paragraphs_by_alias(
    std::string_view alias, const std::vector<std::string> &paragraphs) {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return 0U;
    }

    return replace_content_control_with_paragraphs_by_tag_or_alias_in_part(
        *this->last_error_info, *this->xml_document, this->entry_name_storage,
        alias, paragraphs, false);
}

std::size_t TemplatePart::replace_content_control_with_table_rows_by_tag(
    std::string_view tag, const std::vector<std::vector<std::string>> &rows) {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return 0U;
    }

    return replace_content_control_with_table_rows_by_tag_or_alias_in_part(
        *this->last_error_info, *this->xml_document, this->entry_name_storage,
        tag, rows, true);
}

std::size_t TemplatePart::replace_content_control_with_table_rows_by_alias(
    std::string_view alias, const std::vector<std::vector<std::string>> &rows) {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return 0U;
    }

    return replace_content_control_with_table_rows_by_tag_or_alias_in_part(
        *this->last_error_info, *this->xml_document, this->entry_name_storage,
        alias, rows, false);
}

std::size_t TemplatePart::replace_content_control_with_table_by_tag(
    std::string_view tag, const std::vector<std::vector<std::string>> &rows) {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return 0U;
    }

    return replace_content_control_with_table_by_tag_or_alias_in_part(
        *this->last_error_info, *this->xml_document, this->entry_name_storage,
        tag, rows, true);
}

std::size_t TemplatePart::replace_content_control_with_table_by_alias(
    std::string_view alias, const std::vector<std::vector<std::string>> &rows) {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return 0U;
    }

    return replace_content_control_with_table_by_tag_or_alias_in_part(
        *this->last_error_info, *this->xml_document, this->entry_name_storage,
        alias, rows, false);
}

std::size_t TemplatePart::replace_content_control_with_image_by_tag(
    std::string_view tag, const std::filesystem::path &image_path) {
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

    return this->owner->replace_content_control_with_image_in_part(
        *this->xml_document, this->entry_name_storage, tag, true, image_path,
        std::nullopt);
}

std::size_t TemplatePart::replace_content_control_with_image_by_tag(
    std::string_view tag, const std::filesystem::path &image_path,
    std::uint32_t width_px, std::uint32_t height_px) {
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

    return this->owner->replace_content_control_with_image_in_part(
        *this->xml_document, this->entry_name_storage, tag, true, image_path,
        std::pair<std::uint32_t, std::uint32_t>{width_px, height_px});
}

std::size_t TemplatePart::replace_content_control_with_image_by_alias(
    std::string_view alias, const std::filesystem::path &image_path) {
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

    return this->owner->replace_content_control_with_image_in_part(
        *this->xml_document, this->entry_name_storage, alias, false, image_path,
        std::nullopt);
}

std::size_t TemplatePart::replace_content_control_with_image_by_alias(
    std::string_view alias, const std::filesystem::path &image_path,
    std::uint32_t width_px, std::uint32_t height_px) {
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

    return this->owner->replace_content_control_with_image_in_part(
        *this->xml_document, this->entry_name_storage, alias, false, image_path,
        std::pair<std::uint32_t, std::uint32_t>{width_px, height_px});
}
