std::vector<featherdoc::content_control_summary>
Document::list_content_controls() const {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before listing content controls",
            std::string{document_xml_entry});
        return {};
    }

    return summarize_content_controls_in_part(
        this->last_error_info,
        const_cast<pugi::xml_document &>(this->document));
}

std::optional<featherdoc::custom_xml_data_binding_sync_result>
Document::sync_content_controls_from_custom_xml() {
    try {
        if (!this->is_open()) {
            set_last_error(this->last_error_info,
                           document_errc::document_not_open,
                           "call open() or create_empty() before "
                           "synchronizing content controls from Custom XML",
                           std::string{document_xml_entry});
            return std::nullopt;
        }

        const auto &source_archive_path =
            this->has_source_archive
                ? this->extension_state().source_archive_io_path
                : this->document_path;
        auto custom_xml_parts = load_custom_xml_package_parts(
            source_archive_path, this->opened_archive_limits,
            this->extension_state().opened_validation_mode,
            this->has_source_archive
                ? &this->extension_state().source_archive_snapshot
                : nullptr,
            this->last_error_info);
        if (!custom_xml_parts.has_value()) {
            return std::nullopt;
        }

        // Synchronization is a document-wide operation. Work on isolated
        // copies so a later allocation failure cannot publish an earlier
        // body/header/footer rewrite. checked_clone_xml_document is required
        // because pugixml's ordinary clone path cannot report truncation.
        pugi::xml_document synchronized_document;
        if (featherdoc::detail::checked_clone_xml_document(
                this->document, synchronized_document) !=
            featherdoc::detail::xml_document_clone_status::success) {
            (void)featherdoc::detail::set_xml_document_clone_allocation_failure(
                this->last_error_info, document_xml_entry);
            return std::nullopt;
        }

        std::vector<pugi::xml_document> synchronized_headers;
        synchronized_headers.reserve(this->header_parts.size());
        for (const auto &part : this->header_parts) {
            pugi::xml_document synchronized_part;
            if (featherdoc::detail::checked_clone_xml_document(
                    part->xml, synchronized_part) !=
                featherdoc::detail::xml_document_clone_status::success) {
                (void)featherdoc::detail::
                    set_xml_document_clone_allocation_failure(
                        this->last_error_info, part->entry_name);
                return std::nullopt;
            }
            synchronized_headers.push_back(std::move(synchronized_part));
        }

        std::vector<pugi::xml_document> synchronized_footers;
        synchronized_footers.reserve(this->footer_parts.size());
        for (const auto &part : this->footer_parts) {
            pugi::xml_document synchronized_part;
            if (featherdoc::detail::checked_clone_xml_document(
                    part->xml, synchronized_part) !=
                featherdoc::detail::xml_document_clone_status::success) {
                (void)featherdoc::detail::
                    set_xml_document_clone_allocation_failure(
                        this->last_error_info, part->entry_name);
                return std::nullopt;
            }
            synchronized_footers.push_back(std::move(synchronized_part));
        }

        featherdoc::custom_xml_data_binding_sync_result result;
        if (!sync_content_controls_from_custom_xml_in_part(
                this->last_error_info, synchronized_document,
                document_xml_entry, *custom_xml_parts, result)) {
            return std::nullopt;
        }

        for (std::size_t index = 0U; index < this->header_parts.size();
             ++index) {
            if (!sync_content_controls_from_custom_xml_in_part(
                    this->last_error_info, synchronized_headers[index],
                    this->header_parts[index]->entry_name, *custom_xml_parts,
                    result)) {
                return std::nullopt;
            }
        }

        for (std::size_t index = 0U; index < this->footer_parts.size();
             ++index) {
            if (!sync_content_controls_from_custom_xml_in_part(
                    this->last_error_info, synchronized_footers[index],
                    this->footer_parts[index]->entry_name, *custom_xml_parts,
                    result)) {
                return std::nullopt;
            }
        }

        // Materialize the public return object before the commit boundary. Its
        // move is allocation-free with the standard allocators used by the
        // result vectors, and any unexpected construction failure is still
        // caught while the published DOM is untouched.
        auto synchronized_result =
            std::optional<featherdoc::custom_xml_data_binding_sync_result>{
                std::move(result)};

        if (synchronized_result->synced_content_controls != 0U) {
            // No operation below can allocate: the lifetime token already
            // exists for an open Document, pugixml moves are noexcept in this
            // build, and rebind_internal_xml_handles only refreshes facades.
            // Invalidate before destroying the published DOM so an old facade
            // can never observe freed pugixml nodes.
            this->invalidate_xml_handles();
            this->document = std::move(synchronized_document);
            for (std::size_t index = 0U; index < this->header_parts.size();
                 ++index) {
                this->header_parts[index]->xml =
                    std::move(synchronized_headers[index]);
            }
            for (std::size_t index = 0U; index < this->footer_parts.size();
                 ++index) {
                this->footer_parts[index]->xml =
                    std::move(synchronized_footers[index]);
            }
            this->rebind_internal_xml_handles();
        }

        this->last_error_info.clear();
        return synchronized_result;
    } catch (const std::bad_alloc &) {
        (void)featherdoc::detail::set_xml_document_allocation_failure(
            this->last_error_info,
            "Custom XML synchronization ran out of memory", document_xml_entry);
        return std::nullopt;
    } catch (const std::length_error &) {
        (void)featherdoc::detail::set_xml_document_allocation_failure(
            this->last_error_info,
            "Custom XML synchronization exceeded an allocation limit",
            document_xml_entry);
        return std::nullopt;
    }
}

std::vector<featherdoc::content_control_summary>
Document::find_content_controls_by_tag(std::string_view tag) const {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before reading content controls",
            std::string{document_xml_entry});
        return {};
    }

    return filter_content_controls_by_tag_or_alias(
        this->last_error_info, const_cast<pugi::xml_document &>(this->document),
        document_xml_entry, tag, true);
}

std::vector<featherdoc::content_control_summary>
Document::find_content_controls_by_alias(std::string_view alias) const {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before reading content controls",
            std::string{document_xml_entry});
        return {};
    }

    return filter_content_controls_by_tag_or_alias(
        this->last_error_info, const_cast<pugi::xml_document &>(this->document),
        document_xml_entry, alias, false);
}

std::size_t
Document::replace_content_control_text_by_tag(std::string_view tag,
                                              std::string_view replacement) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before replacing content "
                       "control text",
                       std::string{document_xml_entry});
        return 0U;
    }

    return replace_content_control_text_by_tag_or_alias_in_part(
        this->last_error_info, *this->xml_handle_lifetime, this->document,
        document_xml_entry, tag, replacement, true);
}

std::size_t
Document::replace_content_control_text_by_alias(std::string_view alias,
                                                std::string_view replacement) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before replacing content "
                       "control text",
                       std::string{document_xml_entry});
        return 0U;
    }

    return replace_content_control_text_by_tag_or_alias_in_part(
        this->last_error_info, *this->xml_handle_lifetime, this->document,
        document_xml_entry, alias, replacement, false);
}

std::size_t Document::set_content_control_form_state_by_tag(
    std::string_view tag,
    const featherdoc::content_control_form_state_options &options) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before setting content "
                       "control form state",
                       std::string{document_xml_entry});
        return 0U;
    }

    return set_content_control_form_state_by_tag_or_alias_in_part(
        this->last_error_info, this->document, document_xml_entry, tag, options,
        true);
}

std::size_t Document::set_content_control_form_state_by_alias(
    std::string_view alias,
    const featherdoc::content_control_form_state_options &options) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before setting content "
                       "control form state",
                       std::string{document_xml_entry});
        return 0U;
    }

    return set_content_control_form_state_by_tag_or_alias_in_part(
        this->last_error_info, this->document, document_xml_entry, alias,
        options, false);
}

std::size_t Document::replace_content_control_with_paragraphs_by_tag(
    std::string_view tag, const std::vector<std::string> &paragraphs) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before replacing content "
                       "control content",
                       std::string{document_xml_entry});
        return 0U;
    }

    return replace_content_control_with_paragraphs_by_tag_or_alias_in_part(
        this->last_error_info, this->document, document_xml_entry, tag,
        paragraphs, true);
}

std::size_t Document::replace_content_control_with_paragraphs_by_alias(
    std::string_view alias, const std::vector<std::string> &paragraphs) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before replacing content "
                       "control content",
                       std::string{document_xml_entry});
        return 0U;
    }

    return replace_content_control_with_paragraphs_by_tag_or_alias_in_part(
        this->last_error_info, this->document, document_xml_entry, alias,
        paragraphs, false);
}

std::size_t Document::replace_content_control_with_table_rows_by_tag(
    std::string_view tag, const std::vector<std::vector<std::string>> &rows) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before replacing content "
                       "control content",
                       std::string{document_xml_entry});
        return 0U;
    }

    return replace_content_control_with_table_rows_by_tag_or_alias_in_part(
        this->last_error_info, this->document, document_xml_entry, tag, rows,
        true);
}

std::size_t Document::replace_content_control_with_table_rows_by_alias(
    std::string_view alias, const std::vector<std::vector<std::string>> &rows) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before replacing content "
                       "control content",
                       std::string{document_xml_entry});
        return 0U;
    }

    return replace_content_control_with_table_rows_by_tag_or_alias_in_part(
        this->last_error_info, this->document, document_xml_entry, alias, rows,
        false);
}

std::size_t Document::replace_content_control_with_table_by_tag(
    std::string_view tag, const std::vector<std::vector<std::string>> &rows) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before replacing content "
                       "control content",
                       std::string{document_xml_entry});
        return 0U;
    }

    return replace_content_control_with_table_by_tag_or_alias_in_part(
        this->last_error_info, this->document, document_xml_entry, tag, rows,
        true);
}

std::size_t Document::replace_content_control_with_table_by_alias(
    std::string_view alias, const std::vector<std::vector<std::string>> &rows) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before replacing content "
                       "control content",
                       std::string{document_xml_entry});
        return 0U;
    }

    return replace_content_control_with_table_by_tag_or_alias_in_part(
        this->last_error_info, this->document, document_xml_entry, alias, rows,
        false);
}

std::size_t Document::replace_content_control_with_image_by_tag(
    std::string_view tag, const std::filesystem::path &image_path) {
    return this->replace_content_control_with_image_in_part(
        this->document, document_xml_entry, tag, true, image_path,
        std::nullopt);
}

std::size_t Document::replace_content_control_with_image_by_tag(
    std::string_view tag, const std::filesystem::path &image_path,
    std::uint32_t width_px, std::uint32_t height_px) {
    return this->replace_content_control_with_image_in_part(
        this->document, document_xml_entry, tag, true, image_path,
        std::pair<std::uint32_t, std::uint32_t>{width_px, height_px});
}

std::size_t Document::replace_content_control_with_image_by_alias(
    std::string_view alias, const std::filesystem::path &image_path) {
    return this->replace_content_control_with_image_in_part(
        this->document, document_xml_entry, alias, false, image_path,
        std::nullopt);
}

std::size_t Document::replace_content_control_with_image_by_alias(
    std::string_view alias, const std::filesystem::path &image_path,
    std::uint32_t width_px, std::uint32_t height_px) {
    return this->replace_content_control_with_image_in_part(
        this->document, document_xml_entry, alias, false, image_path,
        std::pair<std::uint32_t, std::uint32_t>{width_px, height_px});
}

std::size_t Document::replace_content_control_with_image_in_part(
    pugi::xml_document &xml_document, std::string_view entry_name,
    std::string_view value, bool match_tag,
    const std::filesystem::path &image_path,
    std::optional<std::pair<std::uint32_t, std::uint32_t>> dimensions) {
    featherdoc::detail::image_file_info image_info;
    auto error_code = featherdoc::document_errc::success;
    std::string detail;

    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before replacing a "
                       "content control with an image");
        return 0U;
    }

    if (value.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       match_tag ? "content control tag must not be empty"
                                 : "content control alias must not be empty",
                       std::string{entry_name});
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

    const auto matches = filter_content_controls_by_tag_or_alias(
        this->last_error_info, xml_document, entry_name, value, match_tag);
    if (matches.empty()) {
        this->last_error_info.clear();
        return 0U;
    }

    if (!featherdoc::detail::load_image_file(image_path, image_info, error_code,
                                             detail)) {
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
    } else if (auto *part = this->find_related_part_state(entry_name);
               part != nullptr) {
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

    if (!this->image_relationships_allow_mutation(entry_name)) {
        return 0U;
    }

    const auto replacement_width =
        dimensions.has_value() ? dimensions->first : image_info.width_px;
    const auto replacement_height =
        dimensions.has_value() ? dimensions->second : image_info.height_px;

    std::size_t replaced = 0U;
    const auto rewrite =
        [&](featherdoc::document_error_info &current_last_error,
            std::string_view current_entry_name,
            pugi::xml_node content_control) {
            auto content = ensure_content_control_content_node(content_control);
            if (content == pugi::xml_node{}) {
                set_last_error(
                    current_last_error,
                    std::make_error_code(std::errc::not_enough_memory),
                    "failed to resolve content control content for image "
                    "replacement",
                    std::string{current_entry_name});
                return false;
            }

            const auto kind = content_control_kind_from_node(content_control);
            if (kind == featherdoc::content_control_kind::table_row) {
                set_content_control_replacement_error(
                    current_last_error, current_entry_name,
                    "image replacement requires a block, run, or table-cell "
                    "content control");
                return false;
            }

            clear_node_children(content);

            auto image_parent = pugi::xml_node{};
            switch (kind) {
            case featherdoc::content_control_kind::run: {
                auto run = content.append_child("w:r");
                if (run == pugi::xml_node{}) {
                    set_last_error(
                        current_last_error,
                        std::make_error_code(std::errc::not_enough_memory),
                        "failed to append a replacement image run",
                        std::string{current_entry_name});
                    return false;
                }
                image_parent = run;
                break;
            }
            case featherdoc::content_control_kind::table_cell: {
                auto cell = content.append_child("w:tc");
                if (cell == pugi::xml_node{}) {
                    set_last_error(
                        current_last_error,
                        std::make_error_code(std::errc::not_enough_memory),
                        "failed to append a replacement image cell",
                        std::string{current_entry_name});
                    return false;
                }
                image_parent = cell;
                break;
            }
            case featherdoc::content_control_kind::block:
            case featherdoc::content_control_kind::unknown:
                image_parent = content;
                break;
            case featherdoc::content_control_kind::table_row:
                return false;
            }

            if (!this->append_inline_image_part(
                    xml_document, current_entry_name, *relationships_document,
                    relationships_entry_name, *has_relationships_part,
                    *relationships_dirty, image_parent, {},
                    std::string{image_info.data},
                    std::string{image_info.extension},
                    std::string{image_info.content_type},
                    featherdoc::detail::path_to_utf8(image_path.filename()),
                    replacement_width, replacement_height)) {
                return false;
            }

            clear_content_control_placeholder_flag(content_control);
            return true;
        };

    if (!replace_content_control_by_tag_or_alias_in_node(
            this->last_error_info, xml_document, entry_name, value, match_tag,
            replaced, rewrite)) {
        return replaced;
    }

    this->last_error_info.clear();
    return replaced;
}
