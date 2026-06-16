bool Document::ensure_review_notes_part(featherdoc::review_note_kind kind) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
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
    if (const auto error = this->ensure_content_types_loaded()) {
        return false;
    }

    const bool footnote = kind == featherdoc::review_note_kind::footnote;
    auto &notes_document = footnote ? this->footnotes : this->endnotes;
    auto &has_notes_part = footnote ? this->has_footnotes_part : this->has_endnotes_part;
    auto &notes_loaded = footnote ? this->footnotes_loaded : this->endnotes_loaded;
    auto &notes_dirty = footnote ? this->footnotes_dirty : this->endnotes_dirty;
    const auto part_entry = footnote ? footnotes_xml_entry : endnotes_xml_entry;
    const auto relationship_type = footnote ? footnotes_relationship_type
                                            : endnotes_relationship_type;
    const auto relationship_target = footnote ? footnotes_relationship_target
                                              : endnotes_relationship_target;
    const auto content_type = footnote ? footnotes_content_type : endnotes_content_type;
    const char *root_name = footnote ? "w:footnotes" : "w:endnotes";

    if (!notes_loaded) {
        bool loaded_existing_part = false;
        if (this->has_source_archive && !this->document_path.empty() &&
            std::filesystem::exists(this->document_path)) {
            const auto entry_name = related_part_entry_for_type(
                this->document_relationships, relationship_type, part_entry);
            if (!load_optional_docx_xml_part(this->document_path, entry_name,
                                             notes_document, this->last_error_info)) {
                return false;
            }
            loaded_existing_part = notes_document.document_element() != pugi::xml_node{};
        }
        if (!loaded_existing_part && !initialize_review_notes_document(notes_document, kind)) {
            set_last_error(this->last_error_info,
                           std::make_error_code(std::errc::not_enough_memory),
                           "failed to initialize review notes part",
                           std::string{part_entry});
            return false;
        }
        notes_loaded = true;
    }

    auto root = notes_document.child(root_name);
    if (root == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "review notes part has an unsupported root",
                       std::string{part_entry});
        return false;
    }
    ensure_wordprocessingml_namespace(root);

    if (!ensure_document_relationship(this->document_relationships,
                                      this->has_document_relationships_part,
                                      this->document_relationships_dirty,
                                      this->last_error_info, relationship_type,
                                      relationship_target)) {
        return false;
    }
    if (!add_content_type_override(this->content_types, this->content_types_dirty,
                                   this->last_error_info, part_entry, content_type)) {
        return false;
    }

    has_notes_part = true;
    notes_dirty = true;
    this->last_error_info.clear();
    return true;
}

bool Document::ensure_comments_part() {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before editing comments",
                       std::string{document_xml_entry});
        return false;
    }
    if (const auto error = this->ensure_content_types_loaded()) {
        return false;
    }

    if (!this->comments_loaded) {
        bool loaded_existing_part = false;
        if (this->has_source_archive && !this->document_path.empty() &&
            std::filesystem::exists(this->document_path)) {
            const auto entry_name = related_part_entry_for_type(
                this->document_relationships, comments_relationship_type,
                comments_xml_entry);
            if (!load_optional_docx_xml_part(this->document_path, entry_name,
                                             this->comments, this->last_error_info)) {
                return false;
            }
            loaded_existing_part = this->comments.document_element() != pugi::xml_node{};
        }
        if (!loaded_existing_part && !initialize_comments_document(this->comments)) {
            set_last_error(this->last_error_info,
                           std::make_error_code(std::errc::not_enough_memory),
                           "failed to initialize comments part",
                           std::string{comments_xml_entry});
            return false;
        }
        this->comments_loaded = true;
    }

    auto root = this->comments.child("w:comments");
    if (root == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "comments part has an unsupported root",
                       std::string{comments_xml_entry});
        return false;
    }
    ensure_wordprocessingml_namespace(root);

    if (!ensure_document_relationship(this->document_relationships,
                                      this->has_document_relationships_part,
                                      this->document_relationships_dirty,
                                      this->last_error_info, comments_relationship_type,
                                      comments_relationship_target)) {
        return false;
    }
    if (!add_content_type_override(this->content_types, this->content_types_dirty,
                                   this->last_error_info, comments_xml_entry,
                                   comments_content_type)) {
        return false;
    }

    this->has_comments_part = true;
    this->comments_dirty = true;
    this->last_error_info.clear();
    return true;
}

bool Document::ensure_comments_extended_loaded() {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
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
    if (this->has_source_archive && !this->document_path.empty() &&
        std::filesystem::exists(this->document_path)) {
        const auto entry_name = optional_related_part_entry_for_type(
            this->document_relationships, comments_extended_relationship_type);
        if (entry_name.has_value()) {
            if (!load_optional_docx_xml_part(
                    this->document_path, *entry_name, this->comments_extended,
                    this->last_error_info)) {
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
                       std::string{comments_extended_xml_entry});
        return false;
    }

    this->comments_extended_loaded = true;
    this->has_comments_extended_part = loaded_existing_part;
    this->last_error_info.clear();
    return true;
}

bool Document::ensure_comments_extended_part() {
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
                           std::string{comments_extended_xml_entry});
            return false;
        }
        root = this->comments_extended.child("w15:commentsEx");
    }
    if (root == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "extended comments part has an unsupported root",
                       std::string{comments_extended_xml_entry});
        return false;
    }
    ensure_wordprocessingml_2012_namespace(root);

    if (!ensure_document_relationship(
            this->document_relationships, this->has_document_relationships_part,
            this->document_relationships_dirty, this->last_error_info,
            comments_extended_relationship_type,
            comments_extended_relationship_target)) {
        return false;
    }
    if (!add_content_type_override(this->content_types, this->content_types_dirty,
                                   this->last_error_info,
                                   comments_extended_xml_entry,
                                   comments_extended_content_type)) {
        return false;
    }

    this->has_comments_extended_part = true;
    this->comments_extended_dirty = true;
    this->last_error_info.clear();
    return true;
}
