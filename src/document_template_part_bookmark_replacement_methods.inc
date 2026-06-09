std::size_t TemplatePart::replace_bookmark_with_paragraphs(
    std::string_view bookmark_name, const std::vector<std::string> &paragraphs) {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return 0U;
    }

    return replace_bookmark_with_paragraphs_in_part(
        *this->last_error_info, *this->xml_document, this->entry_name_storage,
        bookmark_name, paragraphs);
}

std::size_t TemplatePart::replace_bookmark_with_table_rows(
    std::string_view bookmark_name, const std::vector<std::vector<std::string>> &rows) {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return 0U;
    }

    return replace_bookmark_with_table_rows_in_part(
        *this->last_error_info, *this->xml_document, this->entry_name_storage,
        bookmark_name, rows);
}

std::size_t TemplatePart::replace_bookmark_with_table(
    std::string_view bookmark_name, const std::vector<std::vector<std::string>> &rows) {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return 0U;
    }

    return replace_bookmark_with_table_in_part(*this->last_error_info, *this->xml_document,
                                               this->entry_name_storage, bookmark_name,
                                               rows);
}

std::size_t TemplatePart::replace_bookmark_with_image(
    std::string_view bookmark_name, const std::filesystem::path &image_path) {
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

    return this->owner->replace_bookmark_with_image_in_part(
        *this->xml_document, this->entry_name_storage, bookmark_name, image_path,
        std::nullopt);
}

std::size_t TemplatePart::replace_bookmark_with_image(
    std::string_view bookmark_name, const std::filesystem::path &image_path,
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

    return this->owner->replace_bookmark_with_image_in_part(
        *this->xml_document, this->entry_name_storage, bookmark_name, image_path,
        std::pair<std::uint32_t, std::uint32_t>{width_px, height_px});
}

std::size_t TemplatePart::replace_bookmark_with_floating_image(
    std::string_view bookmark_name, const std::filesystem::path &image_path,
    featherdoc::floating_image_options options) {
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

    return this->owner->replace_bookmark_with_floating_image_in_part(
        *this->xml_document, this->entry_name_storage, bookmark_name, image_path,
        std::nullopt, std::move(options));
}

std::size_t TemplatePart::replace_bookmark_with_floating_image(
    std::string_view bookmark_name, const std::filesystem::path &image_path,
    std::uint32_t width_px, std::uint32_t height_px,
    featherdoc::floating_image_options options) {
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

    return this->owner->replace_bookmark_with_floating_image_in_part(
        *this->xml_document, this->entry_name_storage, bookmark_name, image_path,
        std::pair<std::uint32_t, std::uint32_t>{width_px, height_px},
        std::move(options));
}

std::size_t TemplatePart::remove_bookmark_block(std::string_view bookmark_name) {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return 0U;
    }

    return remove_bookmark_block_in_part(*this->last_error_info, *this->xml_document,
                                         this->entry_name_storage, bookmark_name);
}

std::size_t TemplatePart::set_bookmark_block_visibility(
    std::string_view bookmark_name, bool visible) {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return 0U;
    }

    return set_bookmark_block_visibility_in_part(*this->last_error_info, *this->xml_document,
                                                 this->entry_name_storage, bookmark_name,
                                                 visible);
}

bookmark_block_visibility_result TemplatePart::apply_bookmark_block_visibility(
    std::span<const bookmark_block_visibility_binding> bindings) {
    bookmark_block_visibility_result result;
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

    return apply_bookmark_block_visibility_in_part(*this->last_error_info,
                                                   *this->xml_document,
                                                   this->entry_name_storage, bindings);
}

bookmark_block_visibility_result TemplatePart::apply_bookmark_block_visibility(
    std::initializer_list<bookmark_block_visibility_binding> bindings) {
    return this->apply_bookmark_block_visibility(
        std::span<const bookmark_block_visibility_binding>{bindings.begin(),
                                                           bindings.size()});
}
