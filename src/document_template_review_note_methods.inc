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

#include "document_template_review_comment_methods.inc"
