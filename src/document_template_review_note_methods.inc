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
    if (this->document_path.empty() ||
        !std::filesystem::exists(this->document_path)) {
        this->last_error_info.clear();
        return {};
    }

    const auto entry_name = related_part_entry_for_type(
        this->document_relationships, footnotes_relationship_type, "word/footnotes.xml");
    pugi::xml_document footnotes_document;
    if (!load_optional_docx_xml_part(this->document_path, entry_name,
                                     footnotes_document, this->last_error_info)) {
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
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before appending a footnote",
                       std::string{document_xml_entry});
        return 0U;
    }
    if (note_text.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "footnote text must not be empty",
                       std::string{footnotes_xml_entry});
        return 0U;
    }
    if (!this->ensure_review_notes_part(featherdoc::review_note_kind::footnote)) {
        return 0U;
    }

    auto root = this->footnotes.child("w:footnotes");
    auto body = template_part_block_container(this->document);
    if (root == pugi::xml_node{} || body == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "document is not ready for footnote insertion",
                       std::string{document_xml_entry});
        return 0U;
    }

    const auto note_id = next_numeric_id(root, "w:footnote", 1L);
    const auto note_id_text = std::to_string(note_id);
    auto note = root.append_child("w:footnote");
    if (note == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to append footnote XML",
                       std::string{footnotes_xml_entry});
        return 0U;
    }
    ensure_attribute_value(note, "w:id", note_id_text);
    if (!set_review_note_text(note, "w:footnoteRef", note_text)) {
        root.remove_child(note);
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to append footnote text",
                       std::string{footnotes_xml_entry});
        return 0U;
    }

    auto paragraph_node = featherdoc::detail::append_paragraph_node(body);
    if (paragraph_node == pugi::xml_node{} ||
        (!reference_text.empty() &&
         !featherdoc::detail::append_plain_text_run(paragraph_node, reference_text))) {
        root.remove_child(note);
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to append footnote reference paragraph",
                       std::string{document_xml_entry});
        return 0U;
    }
    auto reference_run = paragraph_node.append_child("w:r");
    auto reference = reference_run.append_child("w:footnoteReference");
    if (reference_run == pugi::xml_node{} || reference == pugi::xml_node{}) {
        root.remove_child(note);
        body.remove_child(paragraph_node);
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to append footnote reference",
                       std::string{document_xml_entry});
        return 0U;
    }
    ensure_attribute_value(reference, "w:id", note_id_text);

    this->footnotes_dirty = true;
    this->last_error_info.clear();
    return 1U;
}

bool Document::replace_footnote(std::size_t note_index,
                                std::string_view note_text) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before replacing a footnote",
                       std::string{document_xml_entry});
        return false;
    }
    if (note_text.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "footnote text must not be empty",
                       std::string{footnotes_xml_entry});
        return false;
    }
    if (!this->ensure_review_notes_part(featherdoc::review_note_kind::footnote)) {
        return false;
    }
    auto root = this->footnotes.child("w:footnotes");
    auto note = note_by_summary_index(root, featherdoc::review_note_kind::footnote,
                                      note_index);
    if (note == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "footnote index is out of range",
                       std::string{footnotes_xml_entry});
        return false;
    }
    if (!set_review_note_text(note, "w:footnoteRef", note_text)) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to replace footnote text",
                       std::string{footnotes_xml_entry});
        return false;
    }
    this->footnotes_dirty = true;
    this->last_error_info.clear();
    return true;
}
bool Document::remove_footnote(std::size_t note_index) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before removing a footnote",
                       std::string{document_xml_entry});
        return false;
    }
    if (!this->ensure_review_notes_part(featherdoc::review_note_kind::footnote)) {
        return false;
    }
    auto root = this->footnotes.child("w:footnotes");
    auto note = note_by_summary_index(root, featherdoc::review_note_kind::footnote,
                                      note_index);
    if (note == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "footnote index is out of range",
                       std::string{footnotes_xml_entry});
        return false;
    }
    const auto note_id = std::string{note.attribute("w:id").value()};
    root.remove_child(note);
    remove_references_by_id(this->document, "w:footnoteReference", note_id);
    for (auto &part : this->header_parts) {
        if (part != nullptr) {
            remove_references_by_id(part->xml, "w:footnoteReference", note_id);
        }
    }
    for (auto &part : this->footer_parts) {
        if (part != nullptr) {
            remove_references_by_id(part->xml, "w:footnoteReference", note_id);
        }
    }
    this->footnotes_dirty = true;
    this->last_error_info.clear();
    return true;
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
    if (this->document_path.empty() ||
        !std::filesystem::exists(this->document_path)) {
        this->last_error_info.clear();
        return {};
    }

    const auto entry_name = related_part_entry_for_type(
        this->document_relationships, endnotes_relationship_type, "word/endnotes.xml");
    pugi::xml_document endnotes_document;
    if (!load_optional_docx_xml_part(this->document_path, entry_name,
                                     endnotes_document, this->last_error_info)) {
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
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before appending an endnote",
                       std::string{document_xml_entry});
        return 0U;
    }
    if (note_text.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "endnote text must not be empty",
                       std::string{endnotes_xml_entry});
        return 0U;
    }
    if (!this->ensure_review_notes_part(featherdoc::review_note_kind::endnote)) {
        return 0U;
    }

    auto root = this->endnotes.child("w:endnotes");
    auto body = template_part_block_container(this->document);
    if (root == pugi::xml_node{} || body == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "document is not ready for endnote insertion",
                       std::string{document_xml_entry});
        return 0U;
    }

    const auto note_id = next_numeric_id(root, "w:endnote", 1L);
    const auto note_id_text = std::to_string(note_id);
    auto note = root.append_child("w:endnote");
    if (note == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to append endnote XML",
                       std::string{endnotes_xml_entry});
        return 0U;
    }
    ensure_attribute_value(note, "w:id", note_id_text);
    if (!set_review_note_text(note, "w:endnoteRef", note_text)) {
        root.remove_child(note);
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to append endnote text",
                       std::string{endnotes_xml_entry});
        return 0U;
    }

    auto paragraph_node = featherdoc::detail::append_paragraph_node(body);
    if (paragraph_node == pugi::xml_node{} ||
        (!reference_text.empty() &&
         !featherdoc::detail::append_plain_text_run(paragraph_node, reference_text))) {
        root.remove_child(note);
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to append endnote reference paragraph",
                       std::string{document_xml_entry});
        return 0U;
    }
    auto reference_run = paragraph_node.append_child("w:r");
    auto reference = reference_run.append_child("w:endnoteReference");
    if (reference_run == pugi::xml_node{} || reference == pugi::xml_node{}) {
        root.remove_child(note);
        body.remove_child(paragraph_node);
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to append endnote reference",
                       std::string{document_xml_entry});
        return 0U;
    }
    ensure_attribute_value(reference, "w:id", note_id_text);

    this->endnotes_dirty = true;
    this->last_error_info.clear();
    return 1U;
}

bool Document::replace_endnote(std::size_t note_index,
                               std::string_view note_text) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before replacing an endnote",
                       std::string{document_xml_entry});
        return false;
    }
    if (note_text.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "endnote text must not be empty",
                       std::string{endnotes_xml_entry});
        return false;
    }
    if (!this->ensure_review_notes_part(featherdoc::review_note_kind::endnote)) {
        return false;
    }
    auto root = this->endnotes.child("w:endnotes");
    auto note = note_by_summary_index(root, featherdoc::review_note_kind::endnote,
                                      note_index);
    if (note == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "endnote index is out of range",
                       std::string{endnotes_xml_entry});
        return false;
    }
    if (!set_review_note_text(note, "w:endnoteRef", note_text)) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to replace endnote text",
                       std::string{endnotes_xml_entry});
        return false;
    }
    this->endnotes_dirty = true;
    this->last_error_info.clear();
    return true;
}

bool Document::remove_endnote(std::size_t note_index) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before removing an endnote",
                       std::string{document_xml_entry});
        return false;
    }
    if (!this->ensure_review_notes_part(featherdoc::review_note_kind::endnote)) {
        return false;
    }
    auto root = this->endnotes.child("w:endnotes");
    auto note = note_by_summary_index(root, featherdoc::review_note_kind::endnote,
                                      note_index);
    if (note == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "endnote index is out of range",
                       std::string{endnotes_xml_entry});
        return false;
    }
    const auto note_id = std::string{note.attribute("w:id").value()};
    root.remove_child(note);
    remove_references_by_id(this->document, "w:endnoteReference", note_id);
    for (auto &part : this->header_parts) {
        if (part != nullptr) {
            remove_references_by_id(part->xml, "w:endnoteReference", note_id);
        }
    }
    for (auto &part : this->footer_parts) {
        if (part != nullptr) {
            remove_references_by_id(part->xml, "w:endnoteReference", note_id);
        }
    }
    this->endnotes_dirty = true;
    this->last_error_info.clear();
    return true;
}

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
                       std::string{comments_xml_entry});
        return false;
    }
    if ((metadata.author.has_value() && metadata.clear_author) ||
        (metadata.initials.has_value() && metadata.clear_initials) ||
        (metadata.date.has_value() && metadata.clear_date)) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "comment metadata cannot be set and cleared in the same update",
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

    const auto apply_string_metadata =
        [](pugi::xml_node target, const char *name,
           const std::optional<std::string> &value, bool clear) {
            if (value.has_value()) {
                ensure_attribute_value(target, name, *value);
            } else if (clear) {
                target.remove_attribute(name);
            }
        };
    apply_string_metadata(comment, "w:author", metadata.author,
                          metadata.clear_author);
    apply_string_metadata(comment, "w:initials", metadata.initials,
                          metadata.clear_initials);
    apply_string_metadata(comment, "w:date", metadata.date,
                          metadata.clear_date);

    this->comments_dirty = true;
    this->last_error_info.clear();
    return true;
}

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
