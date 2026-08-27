bool Document::ensure_review_notes_part(featherdoc::review_note_kind kind,
                                        bool create_if_missing) {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before editing review notes",
            std::string{document_xml_entry});
        return false;
    }
    if (kind != featherdoc::review_note_kind::footnote &&
        kind != featherdoc::review_note_kind::endnote) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "review note kind must be footnote or endnote",
                       std::string{document_xml_entry});
        return false;
    }
    if (create_if_missing) {
        if (const auto error = this->ensure_content_types_loaded()) {
            return false;
        }
    }

    const bool footnote = kind == featherdoc::review_note_kind::footnote;
    auto &notes_document = footnote ? this->footnotes : this->endnotes;
    auto &has_notes_part =
        footnote ? this->has_footnotes_part : this->has_endnotes_part;
    auto &notes_loaded =
        footnote ? this->footnotes_loaded : this->endnotes_loaded;
    auto &notes_dirty = footnote ? this->footnotes_dirty : this->endnotes_dirty;
    auto &part_entry = footnote ? this->extension_state().singleton_part_entries.footnotes
                                : this->extension_state().singleton_part_entries.endnotes;
    const auto relationship_type =
        footnote ? footnotes_relationship_type : endnotes_relationship_type;
    const auto content_type =
        footnote ? footnotes_content_type : endnotes_content_type;
    const char *root_name = footnote ? "w:footnotes" : "w:endnotes";

    if (this->ignored_singleton_relationship_type(relationship_type)) {
        set_last_error(this->last_error_info,
                       document_errc::invalid_package_structure,
                       "cannot access review notes through an invalid source "
                       "relationship",
                       std::string{document_relationships_xml_entry});
        return false;
    }

    if (!notes_loaded) {
        bool loaded_existing_part = false;
        const auto &source_archive_path =
            this->extension_state().source_archive_io_path;
        if (this->has_source_archive && !source_archive_path.empty()) {
            const auto entry_lookup = related_part_entry_for_type(
                this->document_relationships, relationship_type, part_entry,
                this->ignored_singleton_relationship_type(relationship_type),
                this->last_error_info);
            if (!entry_lookup.valid || !entry_lookup.entry_name.has_value()) {
                return false;
            }
            if (!create_if_missing) {
                std::string candidate_part_entry;
                try {
                    candidate_part_entry = *entry_lookup.entry_name;
                    pugi::xml_document candidate_document;
                    if (!load_optional_docx_xml_part(
                            source_archive_path, candidate_part_entry,
                            candidate_document, this->last_error_info,
                            &this->opened_archive_limits,
                            this->extension_state().opened_validation_mode,
                            this->has_source_archive
                                ? &this->extension_state()
                                       .source_archive_snapshot
                                : nullptr,
                            footnote
                                ? featherdoc::detail::
                                      wordprocessingml_part_kind::footnotes
                                : featherdoc::detail::
                                      wordprocessingml_part_kind::endnotes)) {
                        return false;
                    }
                    loaded_existing_part =
                        candidate_document.document_element() !=
                        pugi::xml_node{};
                    if (loaded_existing_part) {
                        part_entry = candidate_part_entry;
                        notes_document = std::move(candidate_document);
                        has_notes_part = true;
                        notes_loaded = true;
                    }
                } catch (...) {
                    const auto &error_entry = candidate_part_entry.empty()
                                                  ? part_entry
                                                  : candidate_part_entry;
                    (void)featherdoc::detail::
                        set_xml_document_allocation_failure(
                            this->last_error_info,
                            "review notes part load ran out of memory",
                            error_entry);
                    return false;
                }
            } else {
                part_entry = *entry_lookup.entry_name;
                if (!load_optional_docx_xml_part(
                        source_archive_path, part_entry, notes_document,
                        this->last_error_info, &this->opened_archive_limits,
                        this->extension_state().opened_validation_mode,
                        this->has_source_archive
                            ? &this->extension_state().source_archive_snapshot
                            : nullptr,
                        footnote ? featherdoc::detail::
                                       wordprocessingml_part_kind::footnotes
                                 : featherdoc::detail::
                                       wordprocessingml_part_kind::endnotes)) {
                    return false;
                }
                loaded_existing_part =
                    notes_document.document_element() != pugi::xml_node{};
            }
        }
        if (!loaded_existing_part) {
            if (!create_if_missing) {
                this->last_error_info.clear();
                return true;
            }
            if (!initialize_review_notes_document(notes_document, kind)) {
                set_last_error(
                    this->last_error_info,
                    std::make_error_code(std::errc::not_enough_memory),
                    "failed to initialize review notes part",
                    std::string{part_entry});
                return false;
            }
            notes_loaded = true;
        } else if (!notes_loaded) {
            notes_loaded = true;
        }
    }

    auto root = notes_document.child(root_name);
    if (root == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "review notes part has an unsupported root",
                       std::string{part_entry});
        return false;
    }
    if (!create_if_missing) {
        this->last_error_info.clear();
        return true;
    }
    if (!ensure_attribute_value(root, "xmlns:w",
                                wordprocessingml_namespace_uri)) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to prepare review notes namespace",
                       std::string{part_entry});
        return false;
    }

    const auto relationship_target =
        featherdoc::detail::make_package_relationship_target(document_xml_entry,
                                                             part_entry);

    if (!ensure_document_relationship(
            this->document_relationships, this->has_document_relationships_part,
            this->document_relationships_dirty, this->last_error_info,
            relationship_type, relationship_target) ||
        !review_document_relationship_matches(
            this->document_relationships, relationship_type,
            relationship_target)) {
        if (!this->last_error_info) {
            set_last_error(
                this->last_error_info,
                std::make_error_code(std::errc::not_enough_memory),
                "failed to prepare review notes relationship",
                std::string{document_relationships_xml_entry});
        }
        return false;
    }
    if (!add_content_type_override(
            this->content_types, this->content_types_dirty,
            this->last_error_info, part_entry, content_type) ||
        !review_content_type_override_matches(this->content_types, part_entry,
                                              content_type)) {
        if (!this->last_error_info) {
            set_last_error(this->last_error_info,
                           std::make_error_code(std::errc::not_enough_memory),
                           "failed to prepare review notes content type",
                           "[Content_Types].xml");
        }
        return false;
    }

    has_notes_part = true;
    notes_dirty = true;
    this->last_error_info.clear();
    return true;
}

bool Document::ensure_comments_part(bool create_if_missing) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before editing comments",
                       std::string{document_xml_entry});
        return false;
    }
    if (this->ignored_singleton_relationship_type(comments_relationship_type)) {
        set_last_error(
            this->last_error_info, document_errc::invalid_package_structure,
            "cannot access comments through an invalid source relationship",
            std::string{document_relationships_xml_entry});
        return false;
    }
    if (create_if_missing) {
        if (const auto error = this->ensure_content_types_loaded()) {
            return false;
        }
    }

    if (!this->comments_loaded) {
        bool loaded_existing_part = false;
        const auto &source_archive_path =
            this->extension_state().source_archive_io_path;
        if (this->has_source_archive && !source_archive_path.empty()) {
            const auto entry_lookup = related_part_entry_for_type(
                this->document_relationships, comments_relationship_type,
                this->extension_state().singleton_part_entries.comments,
                this->ignored_singleton_relationship_type(
                    comments_relationship_type),
                this->last_error_info);
            if (!entry_lookup.valid || !entry_lookup.entry_name.has_value()) {
                return false;
            }
            if (!create_if_missing) {
                std::string candidate_part_entry;
                try {
                    candidate_part_entry = *entry_lookup.entry_name;
                    pugi::xml_document candidate_document;
                    if (!load_optional_docx_xml_part(
                            source_archive_path, candidate_part_entry,
                            candidate_document, this->last_error_info,
                            &this->opened_archive_limits,
                            this->extension_state().opened_validation_mode,
                            this->has_source_archive
                                ? &this->extension_state()
                                       .source_archive_snapshot
                                : nullptr,
                            featherdoc::detail::
                                wordprocessingml_part_kind::comments)) {
                        return false;
                    }
                    loaded_existing_part =
                        candidate_document.document_element() !=
                        pugi::xml_node{};
                    if (loaded_existing_part) {
                        this->extension_state()
                            .singleton_part_entries.comments =
                            candidate_part_entry;
                        this->comments = std::move(candidate_document);
                        this->has_comments_part = true;
                        this->comments_loaded = true;
                    }
                } catch (...) {
                    const auto &error_entry =
                        candidate_part_entry.empty()
                            ? this->extension_state()
                                  .singleton_part_entries.comments
                            : candidate_part_entry;
                    (void)featherdoc::detail::
                        set_xml_document_allocation_failure(
                            this->last_error_info,
                            "comments part load ran out of memory",
                            error_entry);
                    return false;
                }
            } else {
                this->extension_state().singleton_part_entries.comments =
                    *entry_lookup.entry_name;
                if (!load_optional_docx_xml_part(
                        source_archive_path,
                        this->extension_state().singleton_part_entries.comments,
                        this->comments, this->last_error_info,
                        &this->opened_archive_limits,
                        this->extension_state().opened_validation_mode,
                        this->has_source_archive
                            ? &this->extension_state().source_archive_snapshot
                            : nullptr,
                        featherdoc::detail::
                            wordprocessingml_part_kind::comments)) {
                    return false;
                }
                loaded_existing_part =
                    this->comments.document_element() != pugi::xml_node{};
            }
        }
        if (!loaded_existing_part) {
            if (!create_if_missing) {
                this->last_error_info.clear();
                return true;
            }
            if (!initialize_comments_document(this->comments)) {
                set_last_error(
                    this->last_error_info,
                    std::make_error_code(std::errc::not_enough_memory),
                    "failed to initialize comments part",
                    this->extension_state().singleton_part_entries.comments);
                return false;
            }
            this->comments_loaded = true;
        } else if (!this->comments_loaded) {
            this->comments_loaded = true;
        }
    }

    auto root = this->comments.child("w:comments");
    if (root == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "comments part has an unsupported root",
                       this->extension_state().singleton_part_entries.comments);
        return false;
    }
    if (!create_if_missing) {
        this->last_error_info.clear();
        return true;
    }
    if (!ensure_attribute_value(root, "xmlns:w",
                                wordprocessingml_namespace_uri)) {
        set_last_error(
            this->last_error_info,
            std::make_error_code(std::errc::not_enough_memory),
            "failed to prepare comments namespace",
            this->extension_state().singleton_part_entries.comments);
        return false;
    }

    const auto relationship_target =
        featherdoc::detail::make_package_relationship_target(
            document_xml_entry, this->extension_state().singleton_part_entries.comments);

    if (!ensure_document_relationship(
            this->document_relationships, this->has_document_relationships_part,
            this->document_relationships_dirty, this->last_error_info,
            comments_relationship_type, relationship_target) ||
        !review_document_relationship_matches(
            this->document_relationships, comments_relationship_type,
            relationship_target)) {
        if (!this->last_error_info) {
            set_last_error(
                this->last_error_info,
                std::make_error_code(std::errc::not_enough_memory),
                "failed to prepare comments relationship",
                std::string{document_relationships_xml_entry});
        }
        return false;
    }
    if (!add_content_type_override(
            this->content_types, this->content_types_dirty,
            this->last_error_info, this->extension_state().singleton_part_entries.comments,
            comments_content_type) ||
        !review_content_type_override_matches(
            this->content_types,
            this->extension_state().singleton_part_entries.comments,
            comments_content_type)) {
        if (!this->last_error_info) {
            set_last_error(this->last_error_info,
                           std::make_error_code(std::errc::not_enough_memory),
                           "failed to prepare comments content type",
                           "[Content_Types].xml");
        }
        return false;
    }

    this->has_comments_part = true;
    this->comments_dirty = true;
    this->last_error_info.clear();
    return true;
}

bool Document::ensure_comments_extended_loaded() {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before loading comment metadata",
            std::string{document_xml_entry});
        return false;
    }
    if (this->comments_extended_loaded) {
        return true;
    }

    this->comments_extended.reset();
    this->has_comments_extended_part = false;
    bool loaded_existing_part = false;
    const auto &source_archive_path =
        this->extension_state().source_archive_io_path;
    if (this->has_source_archive && !source_archive_path.empty()) {
        const auto entry_lookup = optional_related_part_entry_for_type(
            this->document_relationships, comments_extended_relationship_type,
            this->ignored_singleton_relationship_type(
                comments_extended_relationship_type),
            this->last_error_info);
        if (!entry_lookup.valid) {
            return false;
        }
        if (entry_lookup.entry_name.has_value()) {
            this->extension_state().singleton_part_entries.comments_extended =
                *entry_lookup.entry_name;
            if (!load_optional_docx_xml_part(
                    source_archive_path,
                    this->extension_state().singleton_part_entries.comments_extended,
                    this->comments_extended, this->last_error_info,
                    &this->opened_archive_limits,
                    this->extension_state().opened_validation_mode,
                    this->has_source_archive
                        ? &this->extension_state().source_archive_snapshot
                        : nullptr, std::nullopt)) {
                return false;
            }
            loaded_existing_part =
                this->comments_extended.document_element() != pugi::xml_node{};
        }
    }

    if (!loaded_existing_part &&
        !initialize_comments_extended_document(this->comments_extended)) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to initialize extended comments part",
                       this->extension_state().singleton_part_entries.comments_extended);
        return false;
    }

    this->comments_extended_loaded = true;
    this->has_comments_extended_part = loaded_existing_part;
    this->last_error_info.clear();
    return true;
}

bool Document::ensure_comments_extended_part() {
    if (this->ignored_singleton_relationship_type(
            comments_extended_relationship_type)) {
        set_last_error(
            this->last_error_info, document_errc::invalid_package_structure,
            "cannot attach extended comments while its source relationship is "
            "invalid",
            std::string{document_relationships_xml_entry});
        return false;
    }
    if (const auto error = this->ensure_content_types_loaded()) {
        return false;
    }
    if (!this->ensure_comments_extended_loaded()) {
        return false;
    }

    auto root = this->comments_extended.child("w15:commentsEx");
    if (root == pugi::xml_node{}) {
        if (!initialize_comments_extended_document(this->comments_extended)) {
            set_last_error(this->last_error_info,
                           std::make_error_code(std::errc::not_enough_memory),
                           "failed to initialize extended comments part",
                           this->extension_state().singleton_part_entries.comments_extended);
            return false;
        }
        root = this->comments_extended.child("w15:commentsEx");
    }
    if (root == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "extended comments part has an unsupported root",
                       this->extension_state().singleton_part_entries.comments_extended);
        return false;
    }
    ensure_wordprocessingml_2012_namespace(root);
    if (std::string_view{root.attribute("xmlns:w15").value()} !=
            wordprocessingml_2012_namespace_uri ||
        !checked_ensure_review_ignorable_namespace(root, "w15")) {
        set_last_error(
            this->last_error_info,
            std::make_error_code(std::errc::not_enough_memory),
            "failed to prepare extended comments namespace",
            this->extension_state().singleton_part_entries.comments_extended);
        return false;
    }

    const auto relationship_target =
        featherdoc::detail::make_package_relationship_target(
            document_xml_entry, this->extension_state().singleton_part_entries.comments_extended);

    if (!ensure_document_relationship(
            this->document_relationships, this->has_document_relationships_part,
            this->document_relationships_dirty, this->last_error_info,
            comments_extended_relationship_type, relationship_target) ||
        !review_document_relationship_matches(
            this->document_relationships, comments_extended_relationship_type,
            relationship_target)) {
        if (!this->last_error_info) {
            set_last_error(
                this->last_error_info,
                std::make_error_code(std::errc::not_enough_memory),
                "failed to prepare extended comments relationship",
                std::string{document_relationships_xml_entry});
        }
        return false;
    }
    if (!add_content_type_override(
            this->content_types, this->content_types_dirty,
            this->last_error_info,
            this->extension_state().singleton_part_entries.comments_extended,
            comments_extended_content_type) ||
        !review_content_type_override_matches(
            this->content_types,
            this->extension_state().singleton_part_entries.comments_extended,
            comments_extended_content_type)) {
        if (!this->last_error_info) {
            set_last_error(this->last_error_info,
                           std::make_error_code(std::errc::not_enough_memory),
                           "failed to prepare extended comments content type",
                           "[Content_Types].xml");
        }
        return false;
    }

    this->has_comments_extended_part = true;
    this->comments_extended_dirty = true;
    this->last_error_info.clear();
    return true;
}

bool Document::ensure_review_notes_part(featherdoc::review_note_kind kind) {
    return ensure_review_notes_part(kind, true);
}

bool Document::ensure_comments_part() { return ensure_comments_part(true); }
