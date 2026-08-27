bool Document::set_comment_resolved(std::size_t comment_index, bool resolved) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before setting comment resolved state",
                       std::string{document_xml_entry});
        return false;
    }
    if (!this->ensure_comments_part(false)) {
        return false;
    }

    auto comments_root = this->comments.child("w:comments");
    auto comment = comment_by_summary_index(comments_root, comment_index);
    if (comment == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "comment index is out of range",
                       this->extension_state().singleton_part_entries.comments);
        return false;
    }
    if (!comment_paragraph_id_can_be_ensured(comment)) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "comment body does not contain a paragraph id target",
                       this->extension_state().singleton_part_entries.comments);
        return false;
    }
    try {
        review_package_transaction transaction(
            {&this->document_relationships, &this->content_types,
             &this->comments, &this->comments_extended},
            {&this->has_document_relationships_part,
             &this->document_relationships_dirty,
             &this->content_types_loaded, &this->content_types_dirty,
             &this->has_comments_part, &this->comments_loaded,
             &this->comments_dirty, &this->has_comments_extended_part,
             &this->comments_extended_loaded,
             &this->comments_extended_dirty},
            {&this->extension_state().singleton_part_entries.comments,
             &this->extension_state().singleton_part_entries.comments_extended});
        if (!transaction.ready()) {
            (void)featherdoc::detail::set_xml_document_clone_allocation_failure(
                this->last_error_info,
                this->extension_state().singleton_part_entries.comments);
            return false;
        }
        if (!this->ensure_comments_part()) {
            return false;
        }

        const auto para_id = ensure_comment_paragraph_id(
            comments_root, comment, this->comments_dirty);
        if (!para_id.has_value()) {
            set_last_error(
                this->last_error_info,
                std::make_error_code(std::errc::not_enough_memory),
                "failed to prepare comment paragraph metadata",
                this->extension_state().singleton_part_entries.comments);
            return false;
        }

        if (!this->ensure_comments_extended_part()) {
            return false;
        }
        auto extended_root = this->comments_extended.child("w15:commentsEx");
        auto comment_ex = ensure_comment_extended_by_paragraph_id(
            extended_root, *para_id);
        if (comment_ex == pugi::xml_node{} ||
            !ensure_attribute_value(comment_ex, "w15:done",
                                    resolved ? "1" : "0")) {
            set_last_error(
                this->last_error_info,
                std::make_error_code(std::errc::not_enough_memory),
                "failed to prepare extended comment metadata",
                this->extension_state().singleton_part_entries.comments_extended);
            return false;
        }

        this->comments_extended_dirty = true;
        transaction.commit();
        this->last_error_info.clear();
        return true;
    } catch (...) {
        (void)featherdoc::detail::set_xml_document_allocation_failure(
            this->last_error_info,
            "comment resolved state ran out of memory before it could be published",
            this->extension_state().singleton_part_entries.comments);
        return false;
    }
}

bool Document::replace_comment(std::size_t comment_index,
                               std::string_view comment_text) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before replacing a comment",
                       std::string{document_xml_entry});
        return false;
    }
    if (comment_text.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "comment text must not be empty",
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
        if (replacement == pugi::xml_node{} ||
            !checked_set_comment_text(replacement, comment_text)) {
            if (replacement != pugi::xml_node{}) {
                (void)root.remove_child(replacement);
            }
            set_last_error(
                this->last_error_info,
                std::make_error_code(std::errc::not_enough_memory),
                "failed to prepare replacement comment text",
                this->extension_state().singleton_part_entries.comments);
            return false;
        }
        if (!root.remove_child(comment)) {
            (void)root.remove_child(replacement);
            set_last_error(
                this->last_error_info,
                document_errc::invalid_package_structure,
                "comment changed before replacement could be published",
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
            "comment replacement ran out of memory before it could be published",
            this->extension_state().singleton_part_entries.comments);
        return false;
    }
}

bool Document::remove_comment(std::size_t comment_index) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before removing a comment",
                       std::string{document_xml_entry});
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
             &this->comments, &this->comments_extended},
            {&this->has_document_relationships_part,
             &this->document_relationships_dirty,
             &this->content_types_loaded, &this->content_types_dirty,
             &this->has_comments_part, &this->comments_loaded,
             &this->comments_dirty, &this->has_comments_extended_part,
             &this->comments_extended_loaded,
             &this->comments_extended_dirty},
            {&this->extension_state().singleton_part_entries.comments,
             &this->extension_state().singleton_part_entries.comments_extended});
        if (!transaction.ready()) {
            (void)featherdoc::detail::set_xml_document_clone_allocation_failure(
                this->last_error_info,
                this->extension_state().singleton_part_entries.comments);
            return false;
        }
        if (!this->ensure_comments_part()) {
            return false;
        }
        const auto comment_id =
            std::string{comment.attribute("w:id").value()};
        const auto para_id = comment_paragraph_id(comment);
        std::unordered_set<std::string> comment_ids_to_remove;
        if (!comment_id.empty()) {
            comment_ids_to_remove.insert(comment_id);
        }
        std::unordered_set<std::string> para_ids_to_remove;
        if (para_id.has_value() &&
            !this->ensure_comments_extended_loaded()) {
            return false;
        }

        if (para_id.has_value()) {
            para_ids_to_remove =
                comment_thread_para_ids(this->comments_extended, *para_id);
            for (auto current = root.child("w:comment");
                 current != pugi::xml_node{};
                 current = current.next_sibling("w:comment")) {
                const auto current_para_id = comment_paragraph_id(current);
                if (!current_para_id.has_value() ||
                    !para_ids_to_remove.contains(*current_para_id)) {
                    continue;
                }
                const auto current_id =
                    std::string{current.attribute("w:id").value()};
                if (!current_id.empty()) {
                    comment_ids_to_remove.insert(current_id);
                }
            }
        }

        std::vector<pugi::xml_node> comments_to_remove;
        if (comment_id.empty()) {
            comments_to_remove.push_back(comment);
        } else {
            for (auto current = root.child("w:comment");
                 current != pugi::xml_node{};
                 current = current.next_sibling("w:comment")) {
                const auto current_id =
                    std::string{current.attribute("w:id").value()};
                if (!current_id.empty() &&
                    comment_ids_to_remove.contains(current_id)) {
                    comments_to_remove.push_back(current);
                }
            }
        }

        std::vector<pugi::xml_node> extended_to_remove;
        auto extended_root =
            this->comments_extended.child("w15:commentsEx");
        for (const auto &removed_para_id : para_ids_to_remove) {
            const auto comment_ex = comment_extended_by_paragraph_id(
                extended_root, removed_para_id);
            if (comment_ex != pugi::xml_node{}) {
                extended_to_remove.push_back(comment_ex);
            }
        }

        auto marker_removals = review_node_removal_plan{};
        collect_comment_markers_by_ids(marker_removals, this->document,
                                       comment_ids_to_remove);
        for (auto &part : this->header_parts) {
            if (part != nullptr) {
                collect_comment_markers_by_ids(
                    marker_removals, part->xml, comment_ids_to_remove);
            }
        }
        for (auto &part : this->footer_parts) {
            if (part != nullptr) {
                collect_comment_markers_by_ids(
                    marker_removals, part->xml, comment_ids_to_remove);
            }
        }
        if (!finalize_review_node_removal_plan(marker_removals) ||
            !std::ranges::all_of(
                comments_to_remove,
                [root](pugi::xml_node node) { return node.parent() == root; }) ||
            !std::ranges::all_of(
                extended_to_remove, [extended_root](pugi::xml_node node) {
                    return node.parent() == extended_root;
                })) {
            set_last_error(this->last_error_info,
                           document_errc::invalid_package_structure,
                           "comment thread changed while planning removal",
                           this->extension_state()
                               .singleton_part_entries.comments);
            return false;
        }

        this->ensure_xml_handle_lifetime();
        auto retirement_roots = marker_removals.removal_runs;
        this->xml_handle_lifetime->retire_subtrees(retirement_roots);

        if (!publish_review_node_removal_plan(marker_removals)) {
            set_last_error(this->last_error_info,
                           document_errc::invalid_package_structure,
                           "comment marker removal failed during publish",
                           std::string{document_xml_entry});
            return false;
        }
        for (const auto node : comments_to_remove) {
            if (!root.remove_child(node)) {
                set_last_error(this->last_error_info,
                               document_errc::invalid_package_structure,
                               "comment removal failed during publish",
                               this->extension_state()
                                   .singleton_part_entries.comments);
                return false;
            }
        }
        for (const auto node : extended_to_remove) {
            if (!extended_root.remove_child(node)) {
                set_last_error(
                    this->last_error_info,
                    document_errc::invalid_package_structure,
                    "extended comment removal failed during publish",
                    this->extension_state()
                        .singleton_part_entries.comments_extended);
                return false;
            }
        }

        if (!extended_to_remove.empty()) {
            this->comments_extended_dirty = true;
        }
        this->comments_dirty = true;
        transaction.commit();
        this->last_error_info.clear();
        return true;
    } catch (...) {
        (void)featherdoc::detail::set_xml_document_allocation_failure(
            this->last_error_info,
            "comment removal ran out of memory before it could be published",
            this->extension_state().singleton_part_entries.comments);
        return false;
    }
}
