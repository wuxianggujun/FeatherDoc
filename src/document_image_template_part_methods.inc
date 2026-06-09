bool TemplatePart::append_image(const std::filesystem::path &image_path) {
    if (this->xml_document == nullptr || this->last_error_info == nullptr ||
        this->owner == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           "template part is not available", this->entry_name_storage);
        }
        return false;
    }

    featherdoc::detail::image_file_info image_info;
    auto error_code = featherdoc::document_errc::success;
    std::string detail;
    if (!featherdoc::detail::load_image_file(image_path, image_info, error_code, detail)) {
        set_last_error(*this->last_error_info, error_code, std::move(detail),
                       image_path.string());
        return false;
    }

    if (this->entry_name_storage == document_xml_entry) {
        return this->owner->append_inline_image_part(std::move(image_info.data),
                                                     std::move(image_info.extension),
                                                     std::move(image_info.content_type),
                                                     image_path.filename().string(),
                                                     image_info.width_px,
                                                     image_info.height_px);
    }

    auto *part = this->owner->find_related_part_state(this->entry_name_storage);
    if (part == nullptr) {
        set_last_error(*this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "template part is not attached to this document",
                       this->entry_name_storage);
        return false;
    }

    return this->owner->append_inline_image_part(
        part->xml, this->entry_name_storage, part->relationships,
        part->relationships_entry_name, part->has_relationships_part,
        part->relationships_dirty, part->xml.document_element(), {},
        std::move(image_info.data), std::move(image_info.extension),
        std::move(image_info.content_type), image_path.filename().string(),
        image_info.width_px, image_info.height_px);
}

bool TemplatePart::append_image(const std::filesystem::path &image_path,
                                std::uint32_t width_px,
                                std::uint32_t height_px) {
    if (this->xml_document == nullptr || this->last_error_info == nullptr ||
        this->owner == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           "template part is not available", this->entry_name_storage);
        }
        return false;
    }

    if (width_px == 0U || height_px == 0U) {
        set_last_error(*this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "image width and height must both be greater than zero",
                       image_path.string());
        return false;
    }

    featherdoc::detail::image_file_info image_info;
    auto error_code = featherdoc::document_errc::success;
    std::string detail;
    if (!featherdoc::detail::load_image_file(image_path, image_info, error_code, detail)) {
        set_last_error(*this->last_error_info, error_code, std::move(detail),
                       image_path.string());
        return false;
    }

    if (this->entry_name_storage == document_xml_entry) {
        return this->owner->append_inline_image_part(std::move(image_info.data),
                                                     std::move(image_info.extension),
                                                     std::move(image_info.content_type),
                                                     image_path.filename().string(),
                                                     width_px, height_px);
    }

    auto *part = this->owner->find_related_part_state(this->entry_name_storage);
    if (part == nullptr) {
        set_last_error(*this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "template part is not attached to this document",
                       this->entry_name_storage);
        return false;
    }

    return this->owner->append_inline_image_part(
        part->xml, this->entry_name_storage, part->relationships,
        part->relationships_entry_name, part->has_relationships_part,
        part->relationships_dirty, part->xml.document_element(), {},
        std::move(image_info.data), std::move(image_info.extension),
        std::move(image_info.content_type), image_path.filename().string(), width_px,
        height_px);
}

bool TemplatePart::append_floating_image(
    const std::filesystem::path &image_path,
    featherdoc::floating_image_options options) {
    if (this->xml_document == nullptr || this->last_error_info == nullptr ||
        this->owner == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           "template part is not available", this->entry_name_storage);
        }
        return false;
    }

    featherdoc::detail::image_file_info image_info;
    auto error_code = featherdoc::document_errc::success;
    std::string detail;
    if (!featherdoc::detail::load_image_file(image_path, image_info, error_code, detail)) {
        set_last_error(*this->last_error_info, error_code, std::move(detail),
                       image_path.string());
        return false;
    }

    if (this->entry_name_storage == document_xml_entry) {
        return this->owner->append_floating_image_part(
            std::move(image_info.data), std::move(image_info.extension),
            std::move(image_info.content_type), image_path.filename().string(),
            image_info.width_px, image_info.height_px, std::move(options));
    }

    auto *part = this->owner->find_related_part_state(this->entry_name_storage);
    if (part == nullptr) {
        set_last_error(*this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "template part is not attached to this document",
                       this->entry_name_storage);
        return false;
    }

    return this->owner->append_floating_image_part(
        part->xml, this->entry_name_storage, part->relationships,
        part->relationships_entry_name, part->has_relationships_part,
        part->relationships_dirty, part->xml.document_element(), {},
        std::move(image_info.data), std::move(image_info.extension),
        std::move(image_info.content_type), image_path.filename().string(),
        image_info.width_px, image_info.height_px, std::move(options));
}

bool TemplatePart::append_floating_image(
    const std::filesystem::path &image_path, std::uint32_t width_px,
    std::uint32_t height_px, featherdoc::floating_image_options options) {
    if (this->xml_document == nullptr || this->last_error_info == nullptr ||
        this->owner == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           "template part is not available", this->entry_name_storage);
        }
        return false;
    }

    if (width_px == 0U || height_px == 0U) {
        set_last_error(*this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "image width and height must both be greater than zero",
                       image_path.string());
        return false;
    }

    featherdoc::detail::image_file_info image_info;
    auto error_code = featherdoc::document_errc::success;
    std::string detail;
    if (!featherdoc::detail::load_image_file(image_path, image_info, error_code, detail)) {
        set_last_error(*this->last_error_info, error_code, std::move(detail),
                       image_path.string());
        return false;
    }

    if (this->entry_name_storage == document_xml_entry) {
        return this->owner->append_floating_image_part(
            std::move(image_info.data), std::move(image_info.extension),
            std::move(image_info.content_type), image_path.filename().string(), width_px,
            height_px, std::move(options));
    }

    auto *part = this->owner->find_related_part_state(this->entry_name_storage);
    if (part == nullptr) {
        set_last_error(*this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "template part is not attached to this document",
                       this->entry_name_storage);
        return false;
    }

    return this->owner->append_floating_image_part(
        part->xml, this->entry_name_storage, part->relationships,
        part->relationships_entry_name, part->has_relationships_part,
        part->relationships_dirty, part->xml.document_element(), {},
        std::move(image_info.data), std::move(image_info.extension),
        std::move(image_info.content_type), image_path.filename().string(), width_px,
        height_px, std::move(options));
}

std::vector<drawing_image_info> TemplatePart::drawing_images() const {
    if (this->xml_document == nullptr || this->last_error_info == nullptr ||
        this->owner == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           "template part is not available", this->entry_name_storage);
        }
        return {};
    }

    return this->owner->drawing_images_in_part(this->entry_name_storage);
}

bool TemplatePart::extract_drawing_image(
    std::size_t image_index, const std::filesystem::path &output_path) const {
    if (this->xml_document == nullptr || this->last_error_info == nullptr ||
        this->owner == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           "template part is not available", this->entry_name_storage);
        }
        return false;
    }

    return this->owner->extract_drawing_image_from_part(this->entry_name_storage,
                                                        image_index, output_path);
}

bool TemplatePart::remove_drawing_image(std::size_t image_index) {
    if (this->xml_document == nullptr || this->last_error_info == nullptr ||
        this->owner == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           "template part is not available", this->entry_name_storage);
        }
        return false;
    }

    return this->owner->remove_drawing_image_in_part(this->entry_name_storage,
                                                     image_index);
}

bool TemplatePart::replace_drawing_image(std::size_t image_index,
                                         const std::filesystem::path &image_path) {
    if (this->xml_document == nullptr || this->last_error_info == nullptr ||
        this->owner == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           "template part is not available", this->entry_name_storage);
        }
        return false;
    }

    return this->owner->replace_drawing_image_in_part(this->entry_name_storage,
                                                      image_index, image_path);
}

std::vector<inline_image_info> TemplatePart::inline_images() const {
    if (this->xml_document == nullptr || this->last_error_info == nullptr ||
        this->owner == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           "template part is not available", this->entry_name_storage);
        }
        return {};
    }

    return this->owner->inline_images_in_part(this->entry_name_storage);
}

bool TemplatePart::extract_inline_image(
    std::size_t image_index, const std::filesystem::path &output_path) const {
    if (this->xml_document == nullptr || this->last_error_info == nullptr ||
        this->owner == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           "template part is not available", this->entry_name_storage);
        }
        return false;
    }

    return this->owner->extract_inline_image_from_part(this->entry_name_storage,
                                                       image_index, output_path);
}

bool TemplatePart::remove_inline_image(std::size_t image_index) {
    if (this->xml_document == nullptr || this->last_error_info == nullptr ||
        this->owner == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           "template part is not available", this->entry_name_storage);
        }
        return false;
    }

    return this->owner->remove_inline_image_in_part(this->entry_name_storage,
                                                    image_index);
}

bool TemplatePart::replace_inline_image(std::size_t image_index,
                                        const std::filesystem::path &image_path) {
    if (this->xml_document == nullptr || this->last_error_info == nullptr ||
        this->owner == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           "template part is not available", this->entry_name_storage);
        }
        return false;
    }

    return this->owner->replace_inline_image_in_part(this->entry_name_storage,
                                                     image_index, image_path);
}

