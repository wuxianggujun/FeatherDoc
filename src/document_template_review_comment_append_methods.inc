std::size_t Document::append_comment(std::string_view selected_text,
                                     std::string_view comment_text,
                                     std::string_view author,
                                     std::string_view initials,
                                     std::string_view date) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before appending a comment",
                       std::string{document_xml_entry});
        return 0U;
    }
    if (selected_text.empty() || comment_text.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "comment selected text and body text must not be empty",
                       std::string{comments_xml_entry});
        return 0U;
    }
    if (!this->ensure_comments_part()) {
        return 0U;
    }

    auto root = this->comments.child("w:comments");
    auto body = template_part_block_container(this->document);
    if (root == pugi::xml_node{} || body == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "document is not ready for comment insertion",
                       std::string{document_xml_entry});
        return 0U;
    }

    const auto comment_id = next_numeric_id(root, "w:comment", 0L);
    const auto comment_id_text = std::to_string(comment_id);
    auto comment = root.append_child("w:comment");
    if (comment == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to append comment XML",
                       std::string{comments_xml_entry});
        return 0U;
    }
    ensure_attribute_value(comment, "w:id", comment_id_text);
    if (!author.empty()) {
        ensure_attribute_value(comment, "w:author", author);
    }
    if (!initials.empty()) {
        ensure_attribute_value(comment, "w:initials", initials);
    }
    if (!date.empty()) {
        ensure_attribute_value(comment, "w:date", date);
    }
    if (!set_comment_text(comment, comment_text)) {
        root.remove_child(comment);
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to append comment text",
                       std::string{comments_xml_entry});
        return 0U;
    }

    auto paragraph_node = featherdoc::detail::append_paragraph_node(body);
    auto range_start = paragraph_node.append_child("w:commentRangeStart");
    if (paragraph_node == pugi::xml_node{} || range_start == pugi::xml_node{} ||
        !featherdoc::detail::append_plain_text_run(paragraph_node, selected_text)) {
        root.remove_child(comment);
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to append comment range",
                       std::string{document_xml_entry});
        return 0U;
    }
    auto range_end = paragraph_node.append_child("w:commentRangeEnd");
    auto reference_run = paragraph_node.append_child("w:r");
    auto reference = reference_run.append_child("w:commentReference");
    if (range_end == pugi::xml_node{} || reference_run == pugi::xml_node{} ||
        reference == pugi::xml_node{}) {
        root.remove_child(comment);
        body.remove_child(paragraph_node);
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to append comment reference",
                       std::string{document_xml_entry});
        return 0U;
    }
    ensure_attribute_value(range_start, "w:id", comment_id_text);
    ensure_attribute_value(range_end, "w:id", comment_id_text);
    ensure_attribute_value(reference, "w:id", comment_id_text);

    this->comments_dirty = true;
    this->last_error_info.clear();
    return 1U;
}

std::size_t Document::append_paragraph_text_comment(
    std::size_t paragraph_index, std::size_t text_offset,
    std::size_t text_length, std::string_view comment_text,
    std::string_view author, std::string_view initials,
    std::string_view date) {
    if (text_length > std::numeric_limits<std::size_t>::max() - text_offset) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "text range is out of range",
                       std::string{document_xml_entry});
        return 0U;
    }
    return this->append_text_range_comment(paragraph_index, text_offset,
                                           paragraph_index,
                                           text_offset + text_length,
                                           comment_text, author, initials, date);
}

std::size_t Document::append_text_range_comment(
    std::size_t start_paragraph_index, std::size_t start_text_offset,
    std::size_t end_paragraph_index, std::size_t end_text_offset,
    std::string_view comment_text, std::string_view author,
    std::string_view initials, std::string_view date) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before appending a text range comment",
                       std::string{document_xml_entry});
        return 0U;
    }
    if (comment_text.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "comment text must not be empty",
                       std::string{comments_xml_entry});
        return 0U;
    }
    if (end_paragraph_index < start_paragraph_index) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "text range start must not be after end",
                       std::string{document_xml_entry});
        return 0U;
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
        return 0U;
    }
    if (!split_paragraph_revision_range_segments(plan.segments)) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::operation_not_supported),
                       "text range comment requires plain text runs",
                       std::string{document_xml_entry});
        return 0U;
    }

    const auto selected_segments =
        collect_selected_paragraph_revision_spans(plan.segments);
    if (selected_segments.size() != plan.segments.size()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "text range is out of range",
                       std::string{document_xml_entry});
        return 0U;
    }
    if (!selected_paragraph_revision_spans_supported(selected_segments)) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::operation_not_supported),
                       "text range comment requires plain text runs",
                       std::string{document_xml_entry});
        return 0U;
    }
    if (!this->ensure_comments_part()) {
        return 0U;
    }

    auto root = this->comments.child("w:comments");
    if (root == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "document is not ready for comment insertion",
                       std::string{comments_xml_entry});
        return 0U;
    }

    const auto comment_id = next_numeric_id(root, "w:comment", 0L);
    const auto comment_id_text = std::to_string(comment_id);
    if (!insert_comment_markers_for_selected_spans(
            plan.segments, selected_segments, comment_id_text)) {
        remove_comment_markers_by_id(this->document, comment_id_text);
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to append text range comment markers",
                       std::string{document_xml_entry});
        return 0U;
    }

    auto comment = root.append_child("w:comment");
    if (comment == pugi::xml_node{}) {
        remove_comment_markers_by_id(this->document, comment_id_text);
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to append comment XML",
                       std::string{comments_xml_entry});
        return 0U;
    }
    ensure_attribute_value(comment, "w:id", comment_id_text);
    if (!author.empty()) {
        ensure_attribute_value(comment, "w:author", author);
    }
    if (!initials.empty()) {
        ensure_attribute_value(comment, "w:initials", initials);
    }
    if (!date.empty()) {
        ensure_attribute_value(comment, "w:date", date);
    }
    if (!date.empty()) {
        ensure_attribute_value(comment, "w:date", date);
    }
    if (!set_comment_text(comment, comment_text)) {
        root.remove_child(comment);
        remove_comment_markers_by_id(this->document, comment_id_text);
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to append comment text",
                       std::string{comments_xml_entry});
        return 0U;
    }

    this->comments_dirty = true;
    this->last_error_info.clear();
    return 1U;
}

std::size_t Document::append_comment_reply(
    std::size_t parent_comment_index, std::string_view comment_text,
    std::string_view author, std::string_view initials,
    std::string_view date) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before appending a comment reply",
                       std::string{document_xml_entry});
        return 0U;
    }
    if (comment_text.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "comment reply text must not be empty",
                       std::string{comments_xml_entry});
        return 0U;
    }
    if (!this->ensure_comments_part()) {
        return 0U;
    }

    auto comments_root = this->comments.child("w:comments");
    auto parent_comment =
        comment_by_summary_index(comments_root, parent_comment_index);
    if (parent_comment == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "parent comment index is out of range",
                       std::string{comments_xml_entry});
        return 0U;
    }
    const auto parent_para_id = ensure_comment_paragraph_id(
        comments_root, parent_comment, this->comments_dirty);
    if (!parent_para_id.has_value()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "parent comment body does not contain a paragraph id target",
                       std::string{comments_xml_entry});
        return 0U;
    }
    if (!this->ensure_comments_extended_part()) {
        return 0U;
    }

    auto extended_root = this->comments_extended.child("w15:commentsEx");
    auto parent_comment_ex =
        ensure_comment_extended_by_paragraph_id(extended_root, *parent_para_id);
    if (parent_comment_ex == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to append parent comment metadata",
                       std::string{comments_extended_xml_entry});
        return 0U;
    }

    const auto comment_id = next_numeric_id(comments_root, "w:comment", 0L);
    const auto comment_id_text = std::to_string(comment_id);
    auto reply = comments_root.append_child("w:comment");
    if (reply == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to append comment reply XML",
                       std::string{comments_xml_entry});
        return 0U;
    }
    ensure_attribute_value(reply, "w:id", comment_id_text);
    if (!author.empty()) {
        ensure_attribute_value(reply, "w:author", author);
    }
    if (!initials.empty()) {
        ensure_attribute_value(reply, "w:initials", initials);
    }
    if (!date.empty()) {
        ensure_attribute_value(reply, "w:date", date);
    }
    if (!set_comment_text(reply, comment_text)) {
        comments_root.remove_child(reply);
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to append comment reply text",
                       std::string{comments_xml_entry});
        return 0U;
    }

    const auto reply_para_id =
        ensure_comment_paragraph_id(comments_root, reply, this->comments_dirty);
    if (!reply_para_id.has_value()) {
        comments_root.remove_child(reply);
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "comment reply body does not contain a paragraph id target",
                       std::string{comments_xml_entry});
        return 0U;
    }

    auto reply_comment_ex =
        ensure_comment_extended_by_paragraph_id(extended_root, *reply_para_id);
    if (reply_comment_ex == pugi::xml_node{}) {
        comments_root.remove_child(reply);
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to append comment reply metadata",
                       std::string{comments_extended_xml_entry});
        return 0U;
    }
    ensure_attribute_value(reply_comment_ex, "w15:paraIdParent",
                           *parent_para_id);

    this->comments_dirty = true;
    this->comments_extended_dirty = true;
    this->last_error_info.clear();
    return 1U;
}
