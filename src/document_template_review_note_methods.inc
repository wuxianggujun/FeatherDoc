std::vector<featherdoc::review_note_summary> Document::list_footnotes() const {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before listing footnotes",
                       std::string{document_xml_entry});
        return {};
    }
    if (this->footnotes_loaded) {
        auto summaries = summarize_notes_from_part(
            const_cast<pugi::xml_document &>(this->footnotes),
            featherdoc::review_note_kind::footnote);
        this->last_error_info.clear();
        return summaries;
    }
    const auto &source_archive_path =
        this->has_source_archive
            ? this->extension_state().source_archive_io_path
            : this->document_path;
    if (source_archive_path.empty() ||
        (!this->has_source_archive &&
         !std::filesystem::exists(source_archive_path))) {
        this->last_error_info.clear();
        return {};
    }

    const auto entry_lookup = related_part_entry_for_type(
        this->document_relationships, footnotes_relationship_type,
        footnotes_xml_entry,
        this->ignored_singleton_relationship_type(footnotes_relationship_type),
        this->last_error_info);
    if (!entry_lookup.valid || !entry_lookup.entry_name.has_value()) {
        return {};
    }
    pugi::xml_document footnotes_document;
    if (!load_optional_docx_xml_part(
            source_archive_path, *entry_lookup.entry_name, footnotes_document,
            this->last_error_info, &this->opened_archive_limits,
            this->extension_state().opened_validation_mode,
            this->has_source_archive
                ? &this->extension_state().source_archive_snapshot
                : nullptr,
            featherdoc::detail::wordprocessingml_part_kind::footnotes)) {
        return {};
    }

    auto summaries = summarize_notes_from_part(
        footnotes_document, featherdoc::review_note_kind::footnote);
    this->last_error_info.clear();
    return summaries;
}

std::size_t Document::append_footnote(std::string_view reference_text,
                                      std::string_view note_text) {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before appending a footnote",
            std::string{document_xml_entry});
        return 0U;
    }
    if (note_text.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "footnote text must not be empty",
                       this->extension_state().singleton_part_entries.footnotes);
        return 0U;
    }
    try {
        review_package_transaction transaction(
            {&this->document_relationships, &this->content_types,
             &this->footnotes},
            {&this->has_document_relationships_part,
             &this->document_relationships_dirty,
             &this->content_types_loaded, &this->content_types_dirty,
             &this->has_footnotes_part, &this->footnotes_loaded,
             &this->footnotes_dirty},
            {&this->extension_state().singleton_part_entries.footnotes});
        if (!transaction.ready()) {
            (void)featherdoc::detail::set_xml_document_clone_allocation_failure(
                this->last_error_info,
                this->extension_state().singleton_part_entries.footnotes);
            return 0U;
        }
        if (!this->ensure_review_notes_part(
                featherdoc::review_note_kind::footnote)) {
            return 0U;
        }
        review_node_rollback_guard paragraph_rollback;

    auto root = this->footnotes.child("w:footnotes");
    auto body = template_part_block_container(this->document);
    if (root == pugi::xml_node{} || body == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "document is not ready for footnote insertion",
                       std::string{document_xml_entry});
        return 0U;
    }

    const auto note_id = next_numeric_id(root, "w:footnote", 1);
    if (!note_id.has_value()) {
        set_last_error(this->last_error_info,
                       document_errc::identifier_space_exhausted,
                       "footnote identifier space is exhausted",
                       this->extension_state().singleton_part_entries.footnotes);
        return 0U;
    }
    const auto note_id_text = std::to_string(*note_id);
    auto note = checked_append_review_note(root, "w:footnote",
                                           "w:footnoteRef", note_id_text,
                                           note_text);
    if (note == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to prepare footnote XML",
                       this->extension_state().singleton_part_entries.footnotes);
        return 0U;
    }

    auto paragraph_node = checked_append_review_body_paragraph(body);
    paragraph_rollback.track(body, paragraph_node);
    if (paragraph_node == pugi::xml_node{} ||
        (!reference_text.empty() &&
         checked_append_review_plain_text_run(paragraph_node,
                                              reference_text) ==
             pugi::xml_node{})) {
        (void)root.remove_child(note);
        if (paragraph_node != pugi::xml_node{}) {
            (void)body.remove_child(paragraph_node);
        }
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to append footnote reference paragraph",
                       std::string{document_xml_entry});
        return 0U;
    }
    auto reference_run = featherdoc::detail::checked_append_xml_element(
        paragraph_node, "w:r");
    auto reference = reference_run == pugi::xml_node{}
                         ? pugi::xml_node{}
                         : featherdoc::detail::checked_append_xml_element(
                               reference_run, "w:footnoteReference");
    if (reference == pugi::xml_node{} ||
        !ensure_attribute_value(reference, "w:id", note_id_text)) {
        (void)root.remove_child(note);
        (void)body.remove_child(paragraph_node);
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to append footnote reference",
                       std::string{document_xml_entry});
        return 0U;
    }
        this->footnotes_dirty = true;
        paragraph_rollback.commit();
        transaction.commit();
        this->last_error_info.clear();
        return 1U;
    } catch (...) {
        (void)featherdoc::detail::set_xml_document_allocation_failure(
            this->last_error_info,
            "footnote append ran out of memory before it could be published",
            this->extension_state().singleton_part_entries.footnotes);
        return 0U;
    }
}

bool Document::replace_footnote(std::size_t note_index,
                                std::string_view note_text) {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before replacing a footnote",
            std::string{document_xml_entry});
        return false;
    }
    if (note_text.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "footnote text must not be empty",
                       this->extension_state().singleton_part_entries.footnotes);
        return false;
    }
    if (!this->ensure_review_notes_part(featherdoc::review_note_kind::footnote,
                                        false)) {
        return false;
    }
    auto root = this->footnotes.child("w:footnotes");
    auto note = note_by_summary_index(
        root, featherdoc::review_note_kind::footnote, note_index);
    if (note == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "footnote index is out of range",
                       this->extension_state().singleton_part_entries.footnotes);
        return false;
    }
    try {
        review_package_transaction transaction(
            {&this->document_relationships, &this->content_types,
             &this->footnotes},
            {&this->has_document_relationships_part,
             &this->document_relationships_dirty,
             &this->content_types_loaded, &this->content_types_dirty,
             &this->has_footnotes_part, &this->footnotes_loaded,
             &this->footnotes_dirty},
            {&this->extension_state().singleton_part_entries.footnotes});
        if (!transaction.ready()) {
            (void)featherdoc::detail::set_xml_document_clone_allocation_failure(
                this->last_error_info,
                this->extension_state().singleton_part_entries.footnotes);
            return false;
        }
        if (!this->ensure_review_notes_part(
                featherdoc::review_note_kind::footnote)) {
            return false;
        }
    auto replacement = checked_insert_review_subtree_before(note);
    if (replacement == pugi::xml_node{} ||
        !checked_set_review_note_text(replacement, "w:footnoteRef",
                                      note_text)) {
        if (replacement != pugi::xml_node{}) {
            (void)root.remove_child(replacement);
        }
        set_last_error(
            this->last_error_info,
            std::make_error_code(std::errc::not_enough_memory),
            "failed to prepare replacement footnote text",
            this->extension_state().singleton_part_entries.footnotes);
        return false;
    }
    if (!root.remove_child(note)) {
        (void)root.remove_child(replacement);
        set_last_error(
            this->last_error_info,
            document_errc::invalid_package_structure,
            "footnote changed before replacement could be published",
            this->extension_state().singleton_part_entries.footnotes);
        return false;
    }
        this->footnotes_dirty = true;
        transaction.commit();
        this->last_error_info.clear();
        return true;
    } catch (...) {
        (void)featherdoc::detail::set_xml_document_allocation_failure(
            this->last_error_info,
            "footnote replacement ran out of memory before it could be published",
            this->extension_state().singleton_part_entries.footnotes);
        return false;
    }
}
bool Document::remove_footnote(std::size_t note_index) {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before removing a footnote",
            std::string{document_xml_entry});
        return false;
    }
    if (!this->ensure_review_notes_part(featherdoc::review_note_kind::footnote,
                                        false)) {
        return false;
    }
    auto root = this->footnotes.child("w:footnotes");
    auto note = note_by_summary_index(
        root, featherdoc::review_note_kind::footnote, note_index);
    if (note == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "footnote index is out of range",
                       this->extension_state().singleton_part_entries.footnotes);
        return false;
    }
    try {
        review_package_transaction transaction(
            {&this->document_relationships, &this->content_types,
             &this->footnotes},
            {&this->has_document_relationships_part,
             &this->document_relationships_dirty,
             &this->content_types_loaded, &this->content_types_dirty,
             &this->has_footnotes_part, &this->footnotes_loaded,
             &this->footnotes_dirty},
            {&this->extension_state().singleton_part_entries.footnotes});
        if (!transaction.ready()) {
            (void)featherdoc::detail::set_xml_document_clone_allocation_failure(
                this->last_error_info,
                this->extension_state().singleton_part_entries.footnotes);
            return false;
        }
        if (!this->ensure_review_notes_part(
                featherdoc::review_note_kind::footnote)) {
            return false;
        }
        const auto note_id = std::string{note.attribute("w:id").value()};
        auto reference_removals = review_node_removal_plan{};
        collect_references_by_id(reference_removals, this->document,
                                 "w:footnoteReference", note_id);
        for (auto &part : this->header_parts) {
            if (part != nullptr) {
                collect_references_by_id(reference_removals, part->xml,
                                         "w:footnoteReference", note_id);
            }
        }
        for (auto &part : this->footer_parts) {
            if (part != nullptr) {
                collect_references_by_id(reference_removals, part->xml,
                                         "w:footnoteReference", note_id);
            }
        }
        if (!finalize_review_node_removal_plan(reference_removals) ||
            note.parent() != root) {
            set_last_error(this->last_error_info,
                           document_errc::invalid_package_structure,
                           "footnote changed while planning removal",
                           this->extension_state()
                               .singleton_part_entries.footnotes);
            return false;
        }

        this->ensure_xml_handle_lifetime();
        auto retirement_roots = reference_removals.removal_runs;
        this->xml_handle_lifetime->retire_subtrees(retirement_roots);
        if (!publish_review_node_removal_plan(reference_removals) ||
            !root.remove_child(note)) {
            set_last_error(this->last_error_info,
                           document_errc::invalid_package_structure,
                           "footnote removal failed during publish",
                           this->extension_state()
                               .singleton_part_entries.footnotes);
            return false;
        }
        this->footnotes_dirty = true;
        transaction.commit();
        this->last_error_info.clear();
        return true;
    } catch (...) {
        (void)featherdoc::detail::set_xml_document_allocation_failure(
            this->last_error_info,
            "footnote removal ran out of memory before it could be published",
            this->extension_state().singleton_part_entries.footnotes);
        return false;
    }
}
std::vector<featherdoc::review_note_summary> Document::list_endnotes() const {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before listing endnotes",
                       std::string{document_xml_entry});
        return {};
    }
    if (this->endnotes_loaded) {
        auto summaries = summarize_notes_from_part(
            const_cast<pugi::xml_document &>(this->endnotes),
            featherdoc::review_note_kind::endnote);
        this->last_error_info.clear();
        return summaries;
    }
    const auto &source_archive_path =
        this->has_source_archive
            ? this->extension_state().source_archive_io_path
            : this->document_path;
    if (source_archive_path.empty() ||
        (!this->has_source_archive &&
         !std::filesystem::exists(source_archive_path))) {
        this->last_error_info.clear();
        return {};
    }

    const auto entry_lookup = related_part_entry_for_type(
        this->document_relationships, endnotes_relationship_type,
        endnotes_xml_entry,
        this->ignored_singleton_relationship_type(endnotes_relationship_type),
        this->last_error_info);
    if (!entry_lookup.valid || !entry_lookup.entry_name.has_value()) {
        return {};
    }
    pugi::xml_document endnotes_document;
    if (!load_optional_docx_xml_part(
            source_archive_path, *entry_lookup.entry_name, endnotes_document,
            this->last_error_info, &this->opened_archive_limits,
            this->extension_state().opened_validation_mode,
            this->has_source_archive
                ? &this->extension_state().source_archive_snapshot
                : nullptr,
            featherdoc::detail::wordprocessingml_part_kind::endnotes)) {
        return {};
    }

    auto summaries = summarize_notes_from_part(
        endnotes_document, featherdoc::review_note_kind::endnote);
    this->last_error_info.clear();
    return summaries;
}

std::size_t Document::append_endnote(std::string_view reference_text,
                                     std::string_view note_text) {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before appending an endnote",
            std::string{document_xml_entry});
        return 0U;
    }
    if (note_text.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "endnote text must not be empty",
                       this->extension_state().singleton_part_entries.endnotes);
        return 0U;
    }
    try {
        review_package_transaction transaction(
            {&this->document_relationships, &this->content_types,
             &this->endnotes},
            {&this->has_document_relationships_part,
             &this->document_relationships_dirty,
             &this->content_types_loaded, &this->content_types_dirty,
             &this->has_endnotes_part, &this->endnotes_loaded,
             &this->endnotes_dirty},
            {&this->extension_state().singleton_part_entries.endnotes});
        if (!transaction.ready()) {
            (void)featherdoc::detail::set_xml_document_clone_allocation_failure(
                this->last_error_info,
                this->extension_state().singleton_part_entries.endnotes);
            return 0U;
        }
        if (!this->ensure_review_notes_part(
                featherdoc::review_note_kind::endnote)) {
            return 0U;
        }
        review_node_rollback_guard paragraph_rollback;

    auto root = this->endnotes.child("w:endnotes");
    auto body = template_part_block_container(this->document);
    if (root == pugi::xml_node{} || body == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "document is not ready for endnote insertion",
                       std::string{document_xml_entry});
        return 0U;
    }

    const auto note_id = next_numeric_id(root, "w:endnote", 1);
    if (!note_id.has_value()) {
        set_last_error(this->last_error_info,
                       document_errc::identifier_space_exhausted,
                       "endnote identifier space is exhausted",
                       this->extension_state().singleton_part_entries.endnotes);
        return 0U;
    }
    const auto note_id_text = std::to_string(*note_id);
    auto note = checked_append_review_note(root, "w:endnote", "w:endnoteRef",
                                           note_id_text, note_text);
    if (note == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to prepare endnote XML",
                       this->extension_state().singleton_part_entries.endnotes);
        return 0U;
    }

    auto paragraph_node = checked_append_review_body_paragraph(body);
    paragraph_rollback.track(body, paragraph_node);
    if (paragraph_node == pugi::xml_node{} ||
        (!reference_text.empty() &&
         checked_append_review_plain_text_run(paragraph_node,
                                              reference_text) ==
             pugi::xml_node{})) {
        (void)root.remove_child(note);
        if (paragraph_node != pugi::xml_node{}) {
            (void)body.remove_child(paragraph_node);
        }
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to append endnote reference paragraph",
                       std::string{document_xml_entry});
        return 0U;
    }
    auto reference_run = featherdoc::detail::checked_append_xml_element(
        paragraph_node, "w:r");
    auto reference = reference_run == pugi::xml_node{}
                         ? pugi::xml_node{}
                         : featherdoc::detail::checked_append_xml_element(
                               reference_run, "w:endnoteReference");
    if (reference == pugi::xml_node{} ||
        !ensure_attribute_value(reference, "w:id", note_id_text)) {
        (void)root.remove_child(note);
        (void)body.remove_child(paragraph_node);
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to append endnote reference",
                       std::string{document_xml_entry});
        return 0U;
    }
        this->endnotes_dirty = true;
        paragraph_rollback.commit();
        transaction.commit();
        this->last_error_info.clear();
        return 1U;
    } catch (...) {
        (void)featherdoc::detail::set_xml_document_allocation_failure(
            this->last_error_info,
            "endnote append ran out of memory before it could be published",
            this->extension_state().singleton_part_entries.endnotes);
        return 0U;
    }
}

bool Document::replace_endnote(std::size_t note_index,
                               std::string_view note_text) {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before replacing an endnote",
            std::string{document_xml_entry});
        return false;
    }
    if (note_text.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "endnote text must not be empty",
                       this->extension_state().singleton_part_entries.endnotes);
        return false;
    }
    if (!this->ensure_review_notes_part(featherdoc::review_note_kind::endnote,
                                        false)) {
        return false;
    }
    auto root = this->endnotes.child("w:endnotes");
    auto note = note_by_summary_index(
        root, featherdoc::review_note_kind::endnote, note_index);
    if (note == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "endnote index is out of range",
                       this->extension_state().singleton_part_entries.endnotes);
        return false;
    }
    try {
        review_package_transaction transaction(
            {&this->document_relationships, &this->content_types,
             &this->endnotes},
            {&this->has_document_relationships_part,
             &this->document_relationships_dirty,
             &this->content_types_loaded, &this->content_types_dirty,
             &this->has_endnotes_part, &this->endnotes_loaded,
             &this->endnotes_dirty},
            {&this->extension_state().singleton_part_entries.endnotes});
        if (!transaction.ready()) {
            (void)featherdoc::detail::set_xml_document_clone_allocation_failure(
                this->last_error_info,
                this->extension_state().singleton_part_entries.endnotes);
            return false;
        }
        if (!this->ensure_review_notes_part(
                featherdoc::review_note_kind::endnote)) {
            return false;
        }
    auto replacement = checked_insert_review_subtree_before(note);
    if (replacement == pugi::xml_node{} ||
        !checked_set_review_note_text(replacement, "w:endnoteRef",
                                      note_text)) {
        if (replacement != pugi::xml_node{}) {
            (void)root.remove_child(replacement);
        }
        set_last_error(
            this->last_error_info,
            std::make_error_code(std::errc::not_enough_memory),
            "failed to prepare replacement endnote text",
            this->extension_state().singleton_part_entries.endnotes);
        return false;
    }
    if (!root.remove_child(note)) {
        (void)root.remove_child(replacement);
        set_last_error(
            this->last_error_info,
            document_errc::invalid_package_structure,
            "endnote changed before replacement could be published",
            this->extension_state().singleton_part_entries.endnotes);
        return false;
    }
        this->endnotes_dirty = true;
        transaction.commit();
        this->last_error_info.clear();
        return true;
    } catch (...) {
        (void)featherdoc::detail::set_xml_document_allocation_failure(
            this->last_error_info,
            "endnote replacement ran out of memory before it could be published",
            this->extension_state().singleton_part_entries.endnotes);
        return false;
    }
}

bool Document::remove_endnote(std::size_t note_index) {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before removing an endnote",
            std::string{document_xml_entry});
        return false;
    }
    if (!this->ensure_review_notes_part(featherdoc::review_note_kind::endnote,
                                        false)) {
        return false;
    }
    auto root = this->endnotes.child("w:endnotes");
    auto note = note_by_summary_index(
        root, featherdoc::review_note_kind::endnote, note_index);
    if (note == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "endnote index is out of range",
                       this->extension_state().singleton_part_entries.endnotes);
        return false;
    }
    try {
        review_package_transaction transaction(
            {&this->document_relationships, &this->content_types,
             &this->endnotes},
            {&this->has_document_relationships_part,
             &this->document_relationships_dirty,
             &this->content_types_loaded, &this->content_types_dirty,
             &this->has_endnotes_part, &this->endnotes_loaded,
             &this->endnotes_dirty},
            {&this->extension_state().singleton_part_entries.endnotes});
        if (!transaction.ready()) {
            (void)featherdoc::detail::set_xml_document_clone_allocation_failure(
                this->last_error_info,
                this->extension_state().singleton_part_entries.endnotes);
            return false;
        }
        if (!this->ensure_review_notes_part(
                featherdoc::review_note_kind::endnote)) {
            return false;
        }
        const auto note_id = std::string{note.attribute("w:id").value()};
        auto reference_removals = review_node_removal_plan{};
        collect_references_by_id(reference_removals, this->document,
                                 "w:endnoteReference", note_id);
        for (auto &part : this->header_parts) {
            if (part != nullptr) {
                collect_references_by_id(reference_removals, part->xml,
                                         "w:endnoteReference", note_id);
            }
        }
        for (auto &part : this->footer_parts) {
            if (part != nullptr) {
                collect_references_by_id(reference_removals, part->xml,
                                         "w:endnoteReference", note_id);
            }
        }
        if (!finalize_review_node_removal_plan(reference_removals) ||
            note.parent() != root) {
            set_last_error(this->last_error_info,
                           document_errc::invalid_package_structure,
                           "endnote changed while planning removal",
                           this->extension_state()
                               .singleton_part_entries.endnotes);
            return false;
        }

        this->ensure_xml_handle_lifetime();
        auto retirement_roots = reference_removals.removal_runs;
        this->xml_handle_lifetime->retire_subtrees(retirement_roots);
        if (!publish_review_node_removal_plan(reference_removals) ||
            !root.remove_child(note)) {
            set_last_error(this->last_error_info,
                           document_errc::invalid_package_structure,
                           "endnote removal failed during publish",
                           this->extension_state()
                               .singleton_part_entries.endnotes);
            return false;
        }
        this->endnotes_dirty = true;
        transaction.commit();
        this->last_error_info.clear();
        return true;
    } catch (...) {
        (void)featherdoc::detail::set_xml_document_allocation_failure(
            this->last_error_info,
            "endnote removal ran out of memory before it could be published",
            this->extension_state().singleton_part_entries.endnotes);
        return false;
    }
}

#include "document_template_review_comment_methods.inc"
