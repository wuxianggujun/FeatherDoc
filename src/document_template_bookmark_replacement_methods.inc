std::size_t Document::replace_bookmark_with_paragraphs(
    std::string_view bookmark_name, const std::vector<std::string> &paragraphs) {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before replacing a bookmark with paragraphs");
        return 0U;
    }

    return replace_bookmark_with_paragraphs_in_part(
        this->last_error_info, *this->xml_handle_lifetime, this->document,
        document_xml_entry, bookmark_name, paragraphs);
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
        this->last_error_info, *this->xml_handle_lifetime, this->document,
        document_xml_entry, bookmark_name, rows);
}

std::size_t Document::replace_bookmark_with_table(
    std::string_view bookmark_name, const std::vector<std::vector<std::string>> &rows) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before replacing a bookmark with a table");
        return 0U;
    }

    return replace_bookmark_with_table_in_part(
        this->last_error_info, *this->xml_handle_lifetime, this->document,
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
                       featherdoc::detail::path_to_utf8(image_path));
        return 0U;
    }

    std::vector<block_bookmark_placeholder> placeholders;
    if (!collect_bookmark_mutation_inputs(
            this->last_error_info, entry_name,
            "bookmark image replacement ran out of memory while collecting targets",
            [&]() {
                return collect_block_bookmark_placeholders(
                    this->last_error_info, xml_document, entry_name,
                    bookmark_name, placeholders);
            })) {
        return 0U;
    }

    if (placeholders.empty()) {
        this->last_error_info.clear();
        return 0U;
    }

    if (!featherdoc::detail::load_image_file(image_path, image_info, error_code, detail)) {
        set_last_error(this->last_error_info, error_code, std::move(detail),
                       featherdoc::detail::path_to_utf8(image_path));
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

    if (const auto error = this->ensure_content_types_loaded()) {
        return 0U;
    }

    const auto part_root = xml_document.document_element();
    if (part_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "bookmark image replacement requires a valid part root",
                       std::string{entry_name});
        return 0U;
    }
    const auto relationships_namespace = part_root.attribute("xmlns:r");
    if (relationships_namespace != pugi::xml_attribute{} &&
        std::string_view{relationships_namespace.value()} !=
            office_document_relationships_namespace_uri) {
        set_last_error(
            this->last_error_info,
            std::make_error_code(std::errc::invalid_argument),
            "bookmark image replacement found a conflicting xmlns:r namespace",
            std::string{entry_name});
        return 0U;
    }

    const auto replacement_width =
        dimensions.has_value() ? dimensions->first : image_info.width_px;
    const auto replacement_height =
        dimensions.has_value() ? dimensions->second : image_info.height_px;

    auto retired_paragraphs = std::vector<pugi::xml_node>{};
    auto prepared_image_paragraphs = std::vector<pugi::xml_node>{};
    auto image_inputs = std::vector<bookmark_image_append_input>{};
    auto image_display_name = std::string{};
    try {
        retired_paragraphs.reserve(placeholders.size());
        prepared_image_paragraphs.reserve(placeholders.size());
        image_inputs.reserve(placeholders.size());
        image_display_name =
            featherdoc::detail::path_to_utf8(image_path.filename());
        for (const auto &placeholder : placeholders) {
            append_outermost_removal_root(retired_paragraphs,
                                          placeholder.paragraph);
            image_inputs.push_back(
                {image_info.data, image_info.extension,
                 image_info.content_type, image_display_name});
        }
    } catch (const std::bad_alloc &) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to prepare bookmark image replacement",
                       std::string{entry_name});
        return 0U;
    }

    bookmark_image_package_transaction package_transaction{
        *relationships_document, this->content_types, part_root,
        *has_relationships_part, *relationships_dirty,
        this->content_types_dirty, this->image_parts};
    if (!package_transaction.prepare(this->last_error_info, entry_name,
                                     placeholders.size())) {
        return 0U;
    }

    const auto rollback_prepared_images = [&]() noexcept {
        for (const auto paragraph : prepared_image_paragraphs) {
            auto parent = paragraph.parent();
            if (parent != pugi::xml_node{}) {
                (void)parent.remove_child(paragraph);
            }
        }
    };

    for (std::size_t placeholder_index = 0U;
         placeholder_index < placeholders.size(); ++placeholder_index) {
        const auto &placeholder = placeholders[placeholder_index];
        auto parent = placeholder.paragraph.parent();
        const auto previous_sibling = placeholder.paragraph.previous_sibling();
        const auto rollback_current_image = [&]() noexcept {
            auto node = previous_sibling != pugi::xml_node{}
                            ? previous_sibling.next_sibling()
                            : parent.first_child();
            while (node != pugi::xml_node{} &&
                   node != placeholder.paragraph) {
                const auto next = node.next_sibling();
                (void)parent.remove_child(node);
                node = next;
            }
        };

        bool appended = false;
        try {
            auto &input = image_inputs[placeholder_index];
            appended = this->append_inline_image_part(
                xml_document, entry_name, *relationships_document,
                relationships_entry_name, *has_relationships_part,
                *relationships_dirty, parent, placeholder.paragraph,
                std::move(input.image_data), std::move(input.extension),
                std::move(input.content_type), std::move(input.display_name),
                replacement_width, replacement_height);
        } catch (...) {
            rollback_current_image();
            rollback_prepared_images();
            (void)featherdoc::detail::set_xml_document_allocation_failure(
                this->last_error_info,
                "bookmark image replacement ran out of memory while staging XML",
                entry_name);
            return 0U;
        }
        if (!appended) {
            rollback_current_image();
            rollback_prepared_images();
            return 0U;
        }

        const auto prepared_paragraph = placeholder.paragraph.previous_sibling();
        if (prepared_paragraph == pugi::xml_node{} ||
            prepared_paragraph == previous_sibling) {
            rollback_current_image();
            rollback_prepared_images();
            set_last_error(this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           "image replacement did not publish a paragraph before the "
                           "bookmark placeholder",
                           std::string{entry_name});
            return 0U;
        }
        if (this->image_parts.size() !=
                package_transaction.initial_image_part_count() +
                    placeholder_index + 1U ||
            std::string_view{part_root.attribute("xmlns:r").value()} !=
                office_document_relationships_namespace_uri ||
            !bookmark_image_drawing_is_complete(
                prepared_paragraph, false, std::nullopt, image_display_name,
                image_info.extension, image_info.content_type,
                *relationships_document, this->content_types)) {
            rollback_current_image();
            rollback_prepared_images();
            (void)featherdoc::detail::set_xml_document_allocation_failure(
                this->last_error_info,
                "bookmark image replacement produced incomplete XML",
                entry_name);
            return 0U;
        }
        prepared_image_paragraphs.push_back(prepared_paragraph);
    }

    if (!retire_bookmark_subtrees(
            this->last_error_info, *this->xml_handle_lifetime,
            retired_paragraphs, entry_name, "bookmark image replacement")) {
        rollback_prepared_images();
        return 0U;
    }

    for (const auto retired_paragraph : retired_paragraphs) {
        auto parent = retired_paragraph.parent();
        if (parent != pugi::xml_node{}) {
            (void)parent.remove_child(retired_paragraph);
        }
    }

    package_transaction.commit();

    this->last_error_info.clear();
    return placeholders.size();
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
                       featherdoc::detail::path_to_utf8(image_path));
        return 0U;
    }

    std::vector<block_bookmark_placeholder> placeholders;
    if (!collect_bookmark_mutation_inputs(
            this->last_error_info, entry_name,
            "floating bookmark image replacement ran out of memory while collecting targets",
            [&]() {
                return collect_block_bookmark_placeholders(
                    this->last_error_info, xml_document, entry_name,
                    bookmark_name, placeholders);
            })) {
        return 0U;
    }

    if (placeholders.empty()) {
        this->last_error_info.clear();
        return 0U;
    }

    if (!featherdoc::detail::load_image_file(image_path, image_info, error_code, detail)) {
        set_last_error(this->last_error_info, error_code, std::move(detail),
                       featherdoc::detail::path_to_utf8(image_path));
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

    if (const auto error = this->ensure_content_types_loaded()) {
        return 0U;
    }

    const auto part_root = xml_document.document_element();
    if (part_root == pugi::xml_node{}) {
        set_last_error(
            this->last_error_info,
            std::make_error_code(std::errc::invalid_argument),
            "floating bookmark image replacement requires a valid part root",
            std::string{entry_name});
        return 0U;
    }
    const auto relationships_namespace = part_root.attribute("xmlns:r");
    if (relationships_namespace != pugi::xml_attribute{} &&
        std::string_view{relationships_namespace.value()} !=
            office_document_relationships_namespace_uri) {
        set_last_error(
            this->last_error_info,
            std::make_error_code(std::errc::invalid_argument),
            "floating bookmark image replacement found a conflicting xmlns:r namespace",
            std::string{entry_name});
        return 0U;
    }

    const auto replacement_width =
        dimensions.has_value() ? dimensions->first : image_info.width_px;
    const auto replacement_height =
        dimensions.has_value() ? dimensions->second : image_info.height_px;

    auto retired_paragraphs = std::vector<pugi::xml_node>{};
    auto prepared_image_paragraphs = std::vector<pugi::xml_node>{};
    auto image_inputs = std::vector<bookmark_image_append_input>{};
    auto image_display_name = std::string{};
    try {
        retired_paragraphs.reserve(placeholders.size());
        prepared_image_paragraphs.reserve(placeholders.size());
        image_inputs.reserve(placeholders.size());
        image_display_name =
            featherdoc::detail::path_to_utf8(image_path.filename());
        for (const auto &placeholder : placeholders) {
            append_outermost_removal_root(retired_paragraphs,
                                          placeholder.paragraph);
            image_inputs.push_back(
                {image_info.data, image_info.extension,
                 image_info.content_type, image_display_name});
        }
    } catch (const std::bad_alloc &) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to prepare floating bookmark image replacement",
                       std::string{entry_name});
        return 0U;
    }

    bookmark_image_package_transaction package_transaction{
        *relationships_document, this->content_types, part_root,
        *has_relationships_part, *relationships_dirty,
        this->content_types_dirty, this->image_parts};
    if (!package_transaction.prepare(this->last_error_info, entry_name,
                                     placeholders.size())) {
        return 0U;
    }

    const auto rollback_prepared_images = [&]() noexcept {
        for (const auto paragraph : prepared_image_paragraphs) {
            auto parent = paragraph.parent();
            if (parent != pugi::xml_node{}) {
                (void)parent.remove_child(paragraph);
            }
        }
    };

    for (std::size_t placeholder_index = 0U;
         placeholder_index < placeholders.size(); ++placeholder_index) {
        const auto &placeholder = placeholders[placeholder_index];
        auto parent = placeholder.paragraph.parent();
        const auto previous_sibling = placeholder.paragraph.previous_sibling();
        const auto rollback_current_image = [&]() noexcept {
            auto node = previous_sibling != pugi::xml_node{}
                            ? previous_sibling.next_sibling()
                            : parent.first_child();
            while (node != pugi::xml_node{} &&
                   node != placeholder.paragraph) {
                const auto next = node.next_sibling();
                (void)parent.remove_child(node);
                node = next;
            }
        };

        bool appended = false;
        try {
            auto &input = image_inputs[placeholder_index];
            appended = this->append_floating_image_part(
                xml_document, entry_name, *relationships_document,
                relationships_entry_name, *has_relationships_part,
                *relationships_dirty, parent, placeholder.paragraph,
                std::move(input.image_data), std::move(input.extension),
                std::move(input.content_type), std::move(input.display_name),
                replacement_width, replacement_height, options);
        } catch (...) {
            rollback_current_image();
            rollback_prepared_images();
            (void)featherdoc::detail::set_xml_document_allocation_failure(
                this->last_error_info,
                "floating bookmark image replacement ran out of memory while staging XML",
                entry_name);
            return 0U;
        }
        if (!appended) {
            rollback_current_image();
            rollback_prepared_images();
            return 0U;
        }

        const auto prepared_paragraph = placeholder.paragraph.previous_sibling();
        if (prepared_paragraph == pugi::xml_node{} ||
            prepared_paragraph == previous_sibling) {
            rollback_current_image();
            rollback_prepared_images();
            set_last_error(this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           "floating image replacement did not publish a paragraph before "
                           "the bookmark placeholder",
                           std::string{entry_name});
            return 0U;
        }
        if (this->image_parts.size() !=
                package_transaction.initial_image_part_count() +
                    placeholder_index + 1U ||
            std::string_view{part_root.attribute("xmlns:r").value()} !=
                office_document_relationships_namespace_uri ||
            !bookmark_image_drawing_is_complete(
                prepared_paragraph, true, options, image_display_name,
                image_info.extension, image_info.content_type,
                *relationships_document, this->content_types)) {
            rollback_current_image();
            rollback_prepared_images();
            (void)featherdoc::detail::set_xml_document_allocation_failure(
                this->last_error_info,
                "floating bookmark image replacement produced incomplete XML",
                entry_name);
            return 0U;
        }
        prepared_image_paragraphs.push_back(prepared_paragraph);
    }

    if (!retire_bookmark_subtrees(
            this->last_error_info, *this->xml_handle_lifetime,
            retired_paragraphs, entry_name,
            "floating bookmark image replacement")) {
        rollback_prepared_images();
        return 0U;
    }

    for (const auto retired_paragraph : retired_paragraphs) {
        auto parent = retired_paragraph.parent();
        if (parent != pugi::xml_node{}) {
            (void)parent.remove_child(retired_paragraph);
        }
    }

    package_transaction.commit();

    this->last_error_info.clear();
    return placeholders.size();
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

    return remove_bookmark_block_in_part(
        this->last_error_info, *this->xml_handle_lifetime, this->document,
        document_xml_entry, bookmark_name);
}

std::size_t Document::set_bookmark_block_visibility(std::string_view bookmark_name,
                                                    bool visible) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before changing bookmark block visibility");
        return 0U;
    }

    return set_bookmark_block_visibility_in_part(
        this->last_error_info, *this->xml_handle_lifetime, this->document,
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

    return apply_bookmark_block_visibility_in_part(
        this->last_error_info, *this->xml_handle_lifetime, this->document,
        document_xml_entry, bindings);
}

bookmark_block_visibility_result Document::apply_bookmark_block_visibility(
    std::initializer_list<bookmark_block_visibility_binding> bindings) {
    return this->apply_bookmark_block_visibility(
        std::span<const bookmark_block_visibility_binding>{bindings.begin(),
                                                           bindings.size()});
}
