bool Document::set_paragraph_text_comment_range(
    std::size_t comment_index, std::size_t paragraph_index,
    std::size_t text_offset, std::size_t text_length) {
    if (text_length > std::numeric_limits<std::size_t>::max() - text_offset) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "text range is out of range",
                       std::string{document_xml_entry});
        return false;
    }
    return this->set_text_range_comment_range(comment_index, paragraph_index,
                                              text_offset, paragraph_index,
                                              text_offset + text_length);
}

bool Document::set_text_range_comment_range(
    std::size_t comment_index, std::size_t start_paragraph_index,
    std::size_t start_text_offset, std::size_t end_paragraph_index,
    std::size_t end_text_offset) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before moving a comment range",
                       std::string{document_xml_entry});
        return false;
    }
    if (end_paragraph_index < start_paragraph_index) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "text range start must not be after end",
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
    if (comment_id.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "comment id is missing",
                       std::string{comments_xml_entry});
        return false;
    }

    std::vector<pugi::xml_node> paragraphs;
    auto paragraph_handle = this->paragraphs();
    while (paragraph_handle.has_next()) {
        paragraphs.push_back(paragraph_handle.current);
        paragraph_handle.next();
    }

    paragraph_revision_range_plan plan;
    std::string range_error;
    if (!build_paragraph_revision_range_plan(
            paragraphs, start_paragraph_index, start_text_offset,
            end_paragraph_index, end_text_offset, plan, range_error)) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       range_error, std::string{document_xml_entry});
        return false;
    }
    if (!split_paragraph_revision_range_segments(plan.segments)) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::operation_not_supported),
                       "comment range requires plain text runs",
                       std::string{document_xml_entry});
        return false;
    }

    const auto selected_segments =
        collect_selected_paragraph_revision_spans(plan.segments);
    if (selected_segments.size() != plan.segments.size()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "text range is out of range",
                       std::string{document_xml_entry});
        return false;
    }
    if (!selected_paragraph_revision_spans_supported(selected_segments)) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::operation_not_supported),
                       "comment range requires plain text runs",
                       std::string{document_xml_entry});
        return false;
    }

    remove_comment_markers_by_id(this->document, comment_id);
    for (auto &part : this->header_parts) {
        if (part != nullptr) {
            remove_comment_markers_by_id(part->xml, comment_id);
        }
    }
    for (auto &part : this->footer_parts) {
        if (part != nullptr) {
            remove_comment_markers_by_id(part->xml, comment_id);
        }
    }

    if (!insert_comment_markers_for_selected_spans(plan.segments,
                                                   selected_segments,
                                                   comment_id)) {
        remove_comment_markers_by_id(this->document, comment_id);
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to move comment range",
                       std::string{document_xml_entry});
        return false;
    }

    this->last_error_info.clear();
    return true;
}
