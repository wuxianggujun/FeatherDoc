bool Document::set_comment_resolved(std::size_t comment_index, bool resolved) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before setting comment resolved state",
                       std::string{document_xml_entry});
        return false;
    }
    if (!this->ensure_comments_part()) {
        return false;
    }

    auto comments_root = this->comments.child("w:comments");
    auto comment = comment_by_summary_index(comments_root, comment_index);
    if (comment == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "comment index is out of range",
                       std::string{comments_xml_entry});
        return false;
    }
    const auto para_id =
        ensure_comment_paragraph_id(comments_root, comment, this->comments_dirty);
    if (!para_id.has_value()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "comment body does not contain a paragraph id target",
                       std::string{comments_xml_entry});
        return false;
    }

    if (!this->ensure_comments_extended_part()) {
        return false;
    }
    auto extended_root = this->comments_extended.child("w15:commentsEx");
    auto comment_ex = comment_extended_by_paragraph_id(extended_root, *para_id);
    if (comment_ex == pugi::xml_node{}) {
        comment_ex = extended_root.append_child("w15:commentEx");
    }
    if (comment_ex == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to append extended comment metadata",
                       std::string{comments_extended_xml_entry});
        return false;
    }
    ensure_attribute_value(comment_ex, "w15:paraId", *para_id);
    ensure_attribute_value(comment_ex, "w15:done", resolved ? "1" : "0");

    this->comments_extended_dirty = true;
    this->last_error_info.clear();
    return true;
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
                       std::string{comments_xml_entry});
        return false;
    }
    if (!this->ensure_comments_part()) {
        return false;
    }
    auto root = this->comments.child("w:comments");
    auto comment = comment_by_summary_index(root, comment_index);
    if (comment == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "comment index is out of range",
                       std::string{comments_xml_entry});
        return false;
    }
    if (!set_comment_text(comment, comment_text)) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to replace comment text",
                       std::string{comments_xml_entry});
        return false;
    }
    this->comments_dirty = true;
    this->last_error_info.clear();
    return true;
}

bool Document::remove_comment(std::size_t comment_index) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before removing a comment",
                       std::string{document_xml_entry});
        return false;
    }
    if (!this->ensure_comments_part()) {
        return false;
    }
    auto root = this->comments.child("w:comments");
    auto comment = comment_by_summary_index(root, comment_index);
    if (comment == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "comment index is out of range",
                       std::string{comments_xml_entry});
        return false;
    }
    const auto comment_id = std::string{comment.attribute("w:id").value()};
    const auto para_id = comment_paragraph_id(comment);
    std::unordered_set<std::string> comment_ids_to_remove;
    if (!comment_id.empty()) {
        comment_ids_to_remove.insert(comment_id);
    }
    std::unordered_set<std::string> para_ids_to_remove;
    if (para_id.has_value() && !this->ensure_comments_extended_loaded()) {
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

    if (comment_id.empty()) {
        root.remove_child(comment);
    }
    for (auto current = root.child("w:comment"); current != pugi::xml_node{};) {
        const auto next = current.next_sibling("w:comment");
        const auto current_id = std::string{current.attribute("w:id").value()};
        if (!current_id.empty() && comment_ids_to_remove.contains(current_id)) {
            root.remove_child(current);
        }
        current = next;
    }

    for (const auto &removed_comment_id : comment_ids_to_remove) {
        remove_comment_markers_by_id(this->document, removed_comment_id);
        for (auto &part : this->header_parts) {
            if (part != nullptr) {
                remove_comment_markers_by_id(part->xml, removed_comment_id);
            }
        }
        for (auto &part : this->footer_parts) {
            if (part != nullptr) {
                remove_comment_markers_by_id(part->xml, removed_comment_id);
            }
        }
    }

    bool removed_extended_metadata = false;
    for (const auto &removed_para_id : para_ids_to_remove) {
        removed_extended_metadata =
            remove_comment_extended_by_paragraph_id(this->comments_extended,
                                                    removed_para_id) ||
            removed_extended_metadata;
    }
    if (removed_extended_metadata) {
        this->comments_extended_dirty = true;
    }
    this->comments_dirty = true;
    this->last_error_info.clear();
    return true;
}
