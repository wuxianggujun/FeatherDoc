std::vector<featherdoc::review_note_summary> Document::list_comments() const {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before listing comments",
                       std::string{document_xml_entry});
        return {};
    }
    std::unordered_map<std::string, std::string> anchor_text_by_id;
    merge_comment_anchor_text_by_id(
        anchor_text_by_id, const_cast<pugi::xml_document &>(this->document));
    for (const auto &part : this->header_parts) {
        if (part != nullptr) {
            merge_comment_anchor_text_by_id(anchor_text_by_id, part->xml);
        }
    }
    for (const auto &part : this->footer_parts) {
        if (part != nullptr) {
            merge_comment_anchor_text_by_id(anchor_text_by_id, part->xml);
        }
    }

    if (this->comments_loaded) {
        auto &comments_document = const_cast<pugi::xml_document &>(this->comments);
        auto summaries = summarize_comments_from_part(comments_document);
        attach_comment_anchor_text(summaries, anchor_text_by_id);
        if (this->comments_extended_loaded) {
            attach_comment_extended_metadata(
                summaries, comments_document,
                const_cast<pugi::xml_document &>(this->comments_extended));
        } else if (!this->document_path.empty() &&
                   std::filesystem::exists(this->document_path)) {
            const auto entry_name = related_part_entry_for_type(
                this->document_relationships, comments_extended_relationship_type,
                comments_extended_xml_entry);
            pugi::xml_document comments_extended_document;
            if (!load_optional_docx_xml_part(
                    this->document_path, entry_name, comments_extended_document,
                    this->last_error_info)) {
                return {};
            }
            attach_comment_extended_metadata(
                summaries, comments_document, comments_extended_document);
        }
        this->last_error_info.clear();
        return summaries;
    }
    if (this->document_path.empty() ||
        !std::filesystem::exists(this->document_path)) {
        this->last_error_info.clear();
        return {};
    }

    const auto entry_name = related_part_entry_for_type(
        this->document_relationships, comments_relationship_type, "word/comments.xml");
    pugi::xml_document comments_document;
    if (!load_optional_docx_xml_part(this->document_path, entry_name,
                                     comments_document, this->last_error_info)) {
        return {};
    }

    auto summaries = summarize_comments_from_part(comments_document);
    attach_comment_anchor_text(summaries, anchor_text_by_id);
    const auto comments_extended_entry_name = related_part_entry_for_type(
        this->document_relationships, comments_extended_relationship_type,
        comments_extended_xml_entry);
    pugi::xml_document comments_extended_document;
    if (!load_optional_docx_xml_part(this->document_path,
                                     comments_extended_entry_name,
                                     comments_extended_document,
                                     this->last_error_info)) {
        return {};
    }
    attach_comment_extended_metadata(summaries, comments_document,
                                     comments_extended_document);
    this->last_error_info.clear();
    return summaries;
}
