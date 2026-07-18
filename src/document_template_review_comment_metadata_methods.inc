bool Document::set_comment_metadata(
    std::size_t comment_index,
    const featherdoc::comment_metadata_update &metadata) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before setting comment metadata",
                       std::string{document_xml_entry});
        return false;
    }
    const auto has_change = metadata.author.has_value() ||
                            metadata.initials.has_value() ||
                            metadata.date.has_value() ||
                            metadata.clear_author ||
                            metadata.clear_initials ||
                            metadata.clear_date;
    if (!has_change) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "comment metadata update must contain at least one change",
                       this->extension_state().singleton_part_entries.comments);
        return false;
    }
    if ((metadata.author.has_value() && metadata.clear_author) ||
        (metadata.initials.has_value() && metadata.clear_initials) ||
        (metadata.date.has_value() && metadata.clear_date)) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "comment metadata cannot be set and cleared in the same update",
                       this->extension_state().singleton_part_entries.comments);
        return false;
    }
    if (!this->ensure_comments_part(false)) {
        return false;
    }

    auto root = this->comments.child("w:comments");
    auto comment = comment_by_summary_index(root, comment_index);
    if (comment == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "comment index is out of range",
                       this->extension_state().singleton_part_entries.comments);
        return false;
    }
    try {
        review_package_transaction transaction(
            {&this->document_relationships, &this->content_types,
             &this->comments},
            {&this->has_document_relationships_part,
             &this->document_relationships_dirty,
             &this->content_types_loaded, &this->content_types_dirty,
             &this->has_comments_part, &this->comments_loaded,
             &this->comments_dirty},
            {&this->extension_state().singleton_part_entries.comments});
        if (!transaction.ready()) {
            (void)featherdoc::detail::set_xml_document_clone_allocation_failure(
                this->last_error_info,
                this->extension_state().singleton_part_entries.comments);
            return false;
        }
        if (!this->ensure_comments_part()) {
            return false;
        }

        auto replacement = checked_insert_review_subtree_before(comment);
        const auto apply_string_metadata =
            [](pugi::xml_node target, const char *name,
               const std::optional<std::string> &value, bool clear) {
                if (value.has_value()) {
                    return ensure_attribute_value(target, name, *value);
                }
                if (clear) {
                    (void)target.remove_attribute(name);
                }
                return true;
            };
        if (replacement == pugi::xml_node{} ||
            !apply_string_metadata(replacement, "w:author", metadata.author,
                                   metadata.clear_author) ||
            !apply_string_metadata(replacement, "w:initials",
                                   metadata.initials,
                                   metadata.clear_initials) ||
            !apply_string_metadata(replacement, "w:date", metadata.date,
                                   metadata.clear_date)) {
            if (replacement != pugi::xml_node{}) {
                (void)root.remove_child(replacement);
            }
            set_last_error(
                this->last_error_info,
                std::make_error_code(std::errc::not_enough_memory),
                "failed to prepare comment metadata update",
                this->extension_state().singleton_part_entries.comments);
            return false;
        }
        if (!root.remove_child(comment)) {
            (void)root.remove_child(replacement);
            set_last_error(
                this->last_error_info,
                document_errc::invalid_package_structure,
                "comment changed before metadata update could be published",
                this->extension_state().singleton_part_entries.comments);
            return false;
        }

        this->comments_dirty = true;
        transaction.commit();
        this->last_error_info.clear();
        return true;
    } catch (...) {
        (void)featherdoc::detail::set_xml_document_allocation_failure(
            this->last_error_info,
            "comment metadata update ran out of memory before it could be published",
            this->extension_state().singleton_part_entries.comments);
        return false;
    }
}
