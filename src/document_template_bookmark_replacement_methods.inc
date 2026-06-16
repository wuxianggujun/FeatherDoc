std::size_t Document::replace_bookmark_with_paragraphs(
    std::string_view bookmark_name, const std::vector<std::string> &paragraphs) {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before replacing a bookmark with paragraphs");
        return 0U;
    }

    return replace_bookmark_with_paragraphs_in_part(
        this->last_error_info, this->document, document_xml_entry, bookmark_name,
        paragraphs);
}

std::size_t Document::replace_bookmark_with_table_rows(
    std::string_view bookmark_name, const std::vector<std::vector<std::string>> &rows) {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before replacing a bookmark with table rows");
        return 0U;
    }

    return replace_bookmark_with_table_rows_in_part(
        this->last_error_info, this->document, document_xml_entry, bookmark_name, rows);
}

std::size_t Document::replace_bookmark_with_table(
    std::string_view bookmark_name, const std::vector<std::vector<std::string>> &rows) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before replacing a bookmark with a table");
        return 0U;
    }

    return replace_bookmark_with_table_in_part(this->last_error_info, this->document,
                                               document_xml_entry, bookmark_name, rows);
}

std::size_t Document::replace_bookmark_with_image_in_part(
    pugi::xml_document &xml_document, std::string_view entry_name,
    std::string_view bookmark_name, const std::filesystem::path &image_path,
    std::optional<std::pair<std::uint32_t, std::uint32_t>> dimensions) {
    featherdoc::detail::image_file_info image_info;
    auto error_code = featherdoc::document_errc::success;
    std::string detail;

    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before replacing a bookmark with an image");
        return 0U;
    }

    if (bookmark_name.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "bookmark name must not be empty", std::string{entry_name});
        return 0U;
    }

    if (dimensions.has_value() &&
        (dimensions->first == 0U || dimensions->second == 0U)) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "image width and height must both be greater than zero",
                       image_path.string());
        return 0U;
    }

    std::vector<block_bookmark_placeholder> placeholders;
    if (!collect_block_bookmark_placeholders(this->last_error_info, xml_document, entry_name,
                                             bookmark_name, placeholders)) {
        return 0U;
    }

    if (placeholders.empty()) {
        this->last_error_info.clear();
        return 0U;
    }

    if (!featherdoc::detail::load_image_file(image_path, image_info, error_code, detail)) {
        set_last_error(this->last_error_info, error_code, std::move(detail),
                       image_path.string());
        return 0U;
    }

    pugi::xml_document *relationships_document = nullptr;
    std::string_view relationships_entry_name;
    bool *has_relationships_part = nullptr;
    bool *relationships_dirty = nullptr;

    if (entry_name == document_xml_entry) {
        relationships_document = &this->document_relationships;
        relationships_entry_name = document_relationships_xml_entry;
        has_relationships_part = &this->has_document_relationships_part;
        relationships_dirty = &this->document_relationships_dirty;
    } else if (auto *part = this->find_related_part_state(entry_name); part != nullptr) {
        relationships_document = &part->relationships;
        relationships_entry_name = part->relationships_entry_name;
        has_relationships_part = &part->has_relationships_part;
        relationships_dirty = &part->relationships_dirty;
    } else {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "template part is not attached to this document",
                       std::string{entry_name});
        return 0U;
    }

    const auto replacement_width =
        dimensions.has_value() ? dimensions->first : image_info.width_px;
    const auto replacement_height =
        dimensions.has_value() ? dimensions->second : image_info.height_px;

    std::size_t replaced = 0U;
    for (const auto &placeholder : placeholders) {
        auto parent = placeholder.paragraph.parent();
        if (!this->append_inline_image_part(
                xml_document, entry_name, *relationships_document,
                relationships_entry_name, *has_relationships_part, *relationships_dirty,
                parent, placeholder.paragraph, std::string{image_info.data},
                std::string{image_info.extension}, std::string{image_info.content_type},
                image_path.filename().string(), replacement_width,
                replacement_height)) {
            return 0U;
        }

        if (!parent.remove_child(placeholder.paragraph)) {
            set_last_error(this->last_error_info,
                           std::make_error_code(std::errc::not_enough_memory),
                           "failed to remove the bookmark placeholder paragraph after image "
                           "replacement",
                           std::string{entry_name});
            return 0U;
        }

        ++replaced;
    }

    this->last_error_info.clear();
    return replaced;
}

std::size_t Document::replace_bookmark_with_floating_image_in_part(
    pugi::xml_document &xml_document, std::string_view entry_name,
    std::string_view bookmark_name, const std::filesystem::path &image_path,
    std::optional<std::pair<std::uint32_t, std::uint32_t>> dimensions,
    featherdoc::floating_image_options options) {
    featherdoc::detail::image_file_info image_info;
    auto error_code = featherdoc::document_errc::success;
    std::string detail;

    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before replacing a bookmark with a floating image");
        return 0U;
    }

    if (bookmark_name.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "bookmark name must not be empty", std::string{entry_name});
        return 0U;
    }

    if (dimensions.has_value() &&
        (dimensions->first == 0U || dimensions->second == 0U)) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "image width and height must both be greater than zero",
                       image_path.string());
        return 0U;
    }

    std::vector<block_bookmark_placeholder> placeholders;
    if (!collect_block_bookmark_placeholders(this->last_error_info, xml_document, entry_name,
                                             bookmark_name, placeholders)) {
        return 0U;
    }

    if (placeholders.empty()) {
        this->last_error_info.clear();
        return 0U;
    }

    if (!featherdoc::detail::load_image_file(image_path, image_info, error_code, detail)) {
        set_last_error(this->last_error_info, error_code, std::move(detail),
                       image_path.string());
        return 0U;
    }

    pugi::xml_document *relationships_document = nullptr;
    std::string_view relationships_entry_name;
    bool *has_relationships_part = nullptr;
    bool *relationships_dirty = nullptr;

    if (entry_name == document_xml_entry) {
        relationships_document = &this->document_relationships;
        relationships_entry_name = document_relationships_xml_entry;
        has_relationships_part = &this->has_document_relationships_part;
        relationships_dirty = &this->document_relationships_dirty;
    } else if (auto *part = this->find_related_part_state(entry_name); part != nullptr) {
        relationships_document = &part->relationships;
        relationships_entry_name = part->relationships_entry_name;
        has_relationships_part = &part->has_relationships_part;
        relationships_dirty = &part->relationships_dirty;
    } else {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "template part is not attached to this document",
                       std::string{entry_name});
        return 0U;
    }

    const auto replacement_width =
        dimensions.has_value() ? dimensions->first : image_info.width_px;
    const auto replacement_height =
        dimensions.has_value() ? dimensions->second : image_info.height_px;

    std::size_t replaced = 0U;
    for (const auto &placeholder : placeholders) {
        auto parent = placeholder.paragraph.parent();
        if (!this->append_floating_image_part(
                xml_document, entry_name, *relationships_document,
                relationships_entry_name, *has_relationships_part, *relationships_dirty,
                parent, placeholder.paragraph, std::string{image_info.data},
                std::string{image_info.extension}, std::string{image_info.content_type},
                image_path.filename().string(), replacement_width,
                replacement_height, options)) {
            return 0U;
        }

        if (!parent.remove_child(placeholder.paragraph)) {
            set_last_error(this->last_error_info,
                           std::make_error_code(std::errc::not_enough_memory),
                           "failed to remove the bookmark placeholder paragraph after floating "
                           "image replacement",
                           std::string{entry_name});
            return 0U;
        }

        ++replaced;
    }

    this->last_error_info.clear();
    return replaced;
}

std::size_t Document::replace_bookmark_with_image(
    std::string_view bookmark_name, const std::filesystem::path &image_path) {
    return this->replace_bookmark_with_image_in_part(
        this->document, document_xml_entry, bookmark_name, image_path, std::nullopt);
}

std::size_t Document::replace_bookmark_with_image(
    std::string_view bookmark_name, const std::filesystem::path &image_path,
    std::uint32_t width_px, std::uint32_t height_px) {
    return this->replace_bookmark_with_image_in_part(
        this->document, document_xml_entry, bookmark_name, image_path,
        std::pair<std::uint32_t, std::uint32_t>{width_px, height_px});
}

std::size_t Document::replace_bookmark_with_floating_image(
    std::string_view bookmark_name, const std::filesystem::path &image_path,
    featherdoc::floating_image_options options) {
    return this->replace_bookmark_with_floating_image_in_part(
        this->document, document_xml_entry, bookmark_name, image_path, std::nullopt,
        std::move(options));
}

std::size_t Document::replace_bookmark_with_floating_image(
    std::string_view bookmark_name, const std::filesystem::path &image_path,
    std::uint32_t width_px, std::uint32_t height_px,
    featherdoc::floating_image_options options) {
    return this->replace_bookmark_with_floating_image_in_part(
        this->document, document_xml_entry, bookmark_name, image_path,
        std::pair<std::uint32_t, std::uint32_t>{width_px, height_px},
        std::move(options));
}

std::size_t Document::remove_bookmark_block(std::string_view bookmark_name) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before removing a bookmark block");
        return 0U;
    }

    return remove_bookmark_block_in_part(this->last_error_info, this->document,
                                         document_xml_entry, bookmark_name);
}

std::size_t Document::set_bookmark_block_visibility(std::string_view bookmark_name,
                                                    bool visible) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before changing bookmark block visibility");
        return 0U;
    }

    return set_bookmark_block_visibility_in_part(this->last_error_info, this->document,
                                                 document_xml_entry, bookmark_name, visible);
}

bookmark_block_visibility_result Document::apply_bookmark_block_visibility(
    std::span<const bookmark_block_visibility_binding> bindings) {
    bookmark_block_visibility_result result;
    result.requested = bindings.size();

    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before applying bookmark block visibility");
        return result;
    }

    return apply_bookmark_block_visibility_in_part(this->last_error_info, this->document,
                                                   document_xml_entry, bindings);
}

bookmark_block_visibility_result Document::apply_bookmark_block_visibility(
    std::initializer_list<bookmark_block_visibility_binding> bindings) {
    return this->apply_bookmark_block_visibility(
        std::span<const bookmark_block_visibility_binding>{bindings.begin(),
                                                           bindings.size()});
}
