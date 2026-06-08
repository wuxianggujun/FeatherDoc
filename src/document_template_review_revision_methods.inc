std::size_t Document::append_insertion_revision(
    std::string_view text, std::string_view author, std::string_view date) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before appending an insertion revision",
                       std::string{document_xml_entry});
        return 0U;
    }
    if (text.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "revision text must not be empty",
                       std::string{document_xml_entry});
        return 0U;
    }

    auto body = template_part_block_container(this->document);
    if (body == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "document is not ready for revision insertion",
                       std::string{document_xml_entry});
        return 0U;
    }

    long next_id = 1L;
    collect_revision_ids(this->document, next_id);
    for (const auto &part : this->header_parts) {
        if (part != nullptr) {
            collect_revision_ids(part->xml, next_id);
        }
    }
    for (const auto &part : this->footer_parts) {
        if (part != nullptr) {
            collect_revision_ids(part->xml, next_id);
        }
    }

    auto paragraph_node = featherdoc::detail::append_paragraph_node(body);
    auto revision = paragraph_node.append_child("w:ins");
    auto run = revision.append_child("w:r");
    if (paragraph_node == pugi::xml_node{} || revision == pugi::xml_node{} ||
        run == pugi::xml_node{} ||
        !set_revision_run_text_content(run, "w:t", text)) {
        if (paragraph_node != pugi::xml_node{}) {
            body.remove_child(paragraph_node);
        }
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to append insertion revision",
                       std::string{document_xml_entry});
        return 0U;
    }
    ensure_attribute_value(revision, "w:id", std::to_string(next_id));
    if (!author.empty()) {
        ensure_attribute_value(revision, "w:author", author);
    }
    if (!date.empty()) {
        ensure_attribute_value(revision, "w:date", date);
    }

    this->last_error_info.clear();
    return 1U;
}

std::size_t Document::append_deletion_revision(
    std::string_view text, std::string_view author, std::string_view date) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before appending a deletion revision",
                       std::string{document_xml_entry});
        return 0U;
    }
    if (text.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "revision text must not be empty",
                       std::string{document_xml_entry});
        return 0U;
    }

    auto body = template_part_block_container(this->document);
    if (body == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "document is not ready for revision insertion",
                       std::string{document_xml_entry});
        return 0U;
    }

    long next_id = 1L;
    collect_revision_ids(this->document, next_id);
    for (const auto &part : this->header_parts) {
        if (part != nullptr) {
            collect_revision_ids(part->xml, next_id);
        }
    }
    for (const auto &part : this->footer_parts) {
        if (part != nullptr) {
            collect_revision_ids(part->xml, next_id);
        }
    }

    auto paragraph_node = featherdoc::detail::append_paragraph_node(body);
    auto revision = paragraph_node.append_child("w:del");
    auto run = revision.append_child("w:r");
    if (paragraph_node == pugi::xml_node{} || revision == pugi::xml_node{} ||
        run == pugi::xml_node{} ||
        !set_revision_run_text_content(run, "w:delText", text)) {
        if (paragraph_node != pugi::xml_node{}) {
            body.remove_child(paragraph_node);
        }
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to append deletion revision",
                       std::string{document_xml_entry});
        return 0U;
    }
    ensure_attribute_value(revision, "w:id", std::to_string(next_id));
    if (!author.empty()) {
        ensure_attribute_value(revision, "w:author", author);
    }
    if (!date.empty()) {
        ensure_attribute_value(revision, "w:date", date);
    }

    this->last_error_info.clear();
    return 1U;
}

bool Document::insert_run_revision_after(
    std::size_t paragraph_index, std::size_t run_index, std::string_view text,
    std::string_view author, std::string_view date) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before inserting a run revision",
                       std::string{document_xml_entry});
        return false;
    }
    if (text.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "revision text must not be empty",
                       std::string{document_xml_entry});
        return false;
    }

    auto paragraph_handle = this->paragraphs();
    for (std::size_t current_index = 0U;
         current_index < paragraph_index && paragraph_handle.has_next();
         ++current_index) {
        paragraph_handle.next();
    }
    auto paragraph_node = paragraph_handle.has_next() ? paragraph_handle.current : pugi::xml_node{};
    if (paragraph_node == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "paragraph index is out of range",
                       std::string{document_xml_entry});
        return false;
    }
    auto anchor_run = paragraph_run_by_index(paragraph_node, run_index);
    if (anchor_run == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "run index is out of range",
                       std::string{document_xml_entry});
        return false;
    }

    const auto next_run = featherdoc::detail::next_named_sibling(anchor_run, "w:r");
    auto revision = next_run == pugi::xml_node{}
                        ? paragraph_node.append_child("w:ins")
                        : paragraph_node.insert_child_before("w:ins", next_run);
    if (revision == pugi::xml_node{} ||
        append_revision_run(revision, anchor_run, "w:t", text) == pugi::xml_node{}) {
        if (revision != pugi::xml_node{}) {
            paragraph_node.remove_child(revision);
        }
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to insert run revision",
                       std::string{document_xml_entry});
        return false;
    }
    long next_id = 1L;
    collect_revision_ids(this->document, next_id);
    for (const auto &part : this->header_parts) {
        if (part != nullptr) {
            collect_revision_ids(part->xml, next_id);
        }
    }
    for (const auto &part : this->footer_parts) {
        if (part != nullptr) {
            collect_revision_ids(part->xml, next_id);
        }
    }
    set_revision_identity_metadata(revision, next_id, author, date);
    this->last_error_info.clear();
    return true;
}

bool Document::delete_run_revision(
    std::size_t paragraph_index, std::size_t run_index, std::string_view author,
    std::string_view date) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before deleting a run revision",
                       std::string{document_xml_entry});
        return false;
    }

    auto paragraph_handle = this->paragraphs();
    for (std::size_t current_index = 0U;
         current_index < paragraph_index && paragraph_handle.has_next();
         ++current_index) {
        paragraph_handle.next();
    }
    auto paragraph_node = paragraph_handle.has_next() ? paragraph_handle.current : pugi::xml_node{};
    if (paragraph_node == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "paragraph index is out of range",
                       std::string{document_xml_entry});
        return false;
    }
    auto target_run = paragraph_run_by_index(paragraph_node, run_index);
    if (target_run == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "run index is out of range",
                       std::string{document_xml_entry});
        return false;
    }

    const auto original_text = featherdoc::detail::collect_plain_text_from_xml(target_run);
    if (original_text.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "run revision deletion requires non-empty run text",
                       std::string{document_xml_entry});
        return false;
    }

    auto revision = paragraph_node.insert_child_before("w:del", target_run);
    if (revision == pugi::xml_node{} ||
        append_revision_run(revision, target_run, "w:delText", original_text) ==
            pugi::xml_node{} ||
        !paragraph_node.remove_child(target_run)) {
        if (revision != pugi::xml_node{}) {
            paragraph_node.remove_child(revision);
        }
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to delete run revision",
                       std::string{document_xml_entry});
        return false;
    }
    long next_id = 1L;
    collect_revision_ids(this->document, next_id);
    for (const auto &part : this->header_parts) {
        if (part != nullptr) {
            collect_revision_ids(part->xml, next_id);
        }
    }
    for (const auto &part : this->footer_parts) {
        if (part != nullptr) {
            collect_revision_ids(part->xml, next_id);
        }
    }
    set_revision_identity_metadata(revision, next_id, author, date);
    this->last_error_info.clear();
    return true;
}

bool Document::replace_run_revision(
    std::size_t paragraph_index, std::size_t run_index, std::string_view text,
    std::string_view author, std::string_view date) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before replacing a run revision",
                       std::string{document_xml_entry});
        return false;
    }
    if (text.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "revision text must not be empty",
                       std::string{document_xml_entry});
        return false;
    }

    auto paragraph_handle = this->paragraphs();
    for (std::size_t current_index = 0U;
         current_index < paragraph_index && paragraph_handle.has_next();
         ++current_index) {
        paragraph_handle.next();
    }
    auto paragraph_node = paragraph_handle.has_next() ? paragraph_handle.current : pugi::xml_node{};
    if (paragraph_node == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "paragraph index is out of range",
                       std::string{document_xml_entry});
        return false;
    }
    auto target_run = paragraph_run_by_index(paragraph_node, run_index);
    if (target_run == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "run index is out of range",
                       std::string{document_xml_entry});
        return false;
    }

    const auto original_text = featherdoc::detail::collect_plain_text_from_xml(target_run);
    if (original_text.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "run revision replacement requires non-empty run text",
                       std::string{document_xml_entry});
        return false;
    }

    long revision_id = 1L;
    collect_revision_ids(this->document, revision_id);
    for (const auto &part : this->header_parts) {
        if (part != nullptr) {
            collect_revision_ids(part->xml, revision_id);
        }
    }
    for (const auto &part : this->footer_parts) {
        if (part != nullptr) {
            collect_revision_ids(part->xml, revision_id);
        }
    }
    auto deletion = paragraph_node.insert_child_before("w:del", target_run);
    auto insertion = deletion == pugi::xml_node{}
                         ? pugi::xml_node{}
                         : paragraph_node.insert_child_before("w:ins", target_run);
    if (deletion == pugi::xml_node{} || insertion == pugi::xml_node{} ||
        append_revision_run(deletion, target_run, "w:delText", original_text) ==
            pugi::xml_node{} ||
        append_revision_run(insertion, target_run, "w:t", text) == pugi::xml_node{} ||
        !paragraph_node.remove_child(target_run)) {
        if (deletion != pugi::xml_node{}) {
            paragraph_node.remove_child(deletion);
        }
        if (insertion != pugi::xml_node{}) {
            paragraph_node.remove_child(insertion);
        }
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to replace run revision",
                       std::string{document_xml_entry});
        return false;
    }
    set_revision_identity_metadata(deletion, revision_id, author, date);
    set_revision_identity_metadata(insertion, revision_id + 1L, author, date);
    this->last_error_info.clear();
    return true;
}

bool Document::insert_paragraph_text_revision(
    std::size_t paragraph_index, std::size_t text_offset, std::string_view text,
    std::string_view author, std::string_view date) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before inserting a paragraph text revision",
                       std::string{document_xml_entry});
        return false;
    }
    if (text.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "revision text must not be empty",
                       std::string{document_xml_entry});
        return false;
    }

    auto paragraph_handle = this->paragraphs();
    for (std::size_t current_index = 0U;
         current_index < paragraph_index && paragraph_handle.has_next();
         ++current_index) {
        paragraph_handle.next();
    }
    auto paragraph_node = paragraph_handle.has_next() ? paragraph_handle.current : pugi::xml_node{};
    if (paragraph_node == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "paragraph index is out of range",
                       std::string{document_xml_entry});
        return false;
    }

    const auto spans = collect_paragraph_revision_run_spans(paragraph_node);
    const auto paragraph_text_length = paragraph_revision_text_length(spans);
    if (text_offset > paragraph_text_length) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "text offset is out of range",
                       std::string{document_xml_entry});
        return false;
    }
    if (!split_paragraph_revision_text_at(paragraph_node, text_offset)) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::operation_not_supported),
                       "paragraph text revision range requires plain text runs",
                       std::string{document_xml_entry});
        return false;
    }

    const auto insert_before =
        paragraph_revision_insert_before(paragraph_node, text_offset);
    const auto style_run = paragraph_revision_style_run(paragraph_node, text_offset);
    auto revision = insert_revision_node(paragraph_node, "w:ins", insert_before);
    if (revision == pugi::xml_node{} ||
        append_revision_run(revision, style_run, "w:t", text) == pugi::xml_node{}) {
        if (revision != pugi::xml_node{}) {
            paragraph_node.remove_child(revision);
        }
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to insert paragraph text revision",
                       std::string{document_xml_entry});
        return false;
    }

    long revision_id = 1L;
    collect_revision_ids(this->document, revision_id);
    for (const auto &part : this->header_parts) {
        if (part != nullptr) {
            collect_revision_ids(part->xml, revision_id);
        }
    }
    for (const auto &part : this->footer_parts) {
        if (part != nullptr) {
            collect_revision_ids(part->xml, revision_id);
        }
    }
    set_revision_identity_metadata(revision, revision_id, author, date);
    this->last_error_info.clear();
    return true;
}

bool Document::delete_paragraph_text_revision(
    std::size_t paragraph_index, std::size_t text_offset, std::size_t text_length,
    std::string_view author, std::string_view date) {
    featherdoc::revision_text_range_options options;
    options.author = std::string{author};
    options.date = std::string{date};
    return this->delete_paragraph_text_revision(paragraph_index, text_offset,
                                                text_length, options);
}

bool Document::delete_paragraph_text_revision(
    std::size_t paragraph_index, std::size_t text_offset, std::size_t text_length,
    const featherdoc::revision_text_range_options &options) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before deleting a paragraph text revision",
                       std::string{document_xml_entry});
        return false;
    }
    if (text_length == 0U) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "text range must not be empty",
                       std::string{document_xml_entry});
        return false;
    }
    if (text_length > std::numeric_limits<std::size_t>::max() - text_offset) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "text range is out of range",
                       std::string{document_xml_entry});
        return false;
    }
    if (options.expected_text.has_value()) {
        const auto preview = this->preview_text_range(
            paragraph_index, text_offset, paragraph_index,
            text_offset + text_length);
        if (!preview.has_value() ||
            !validate_revision_expected_text(this->last_error_info, *preview,
                                             options)) {
            return false;
        }
    }

    auto paragraph_handle = this->paragraphs();
    for (std::size_t current_index = 0U;
         current_index < paragraph_index && paragraph_handle.has_next();
         ++current_index) {
        paragraph_handle.next();
    }
    auto paragraph_node = paragraph_handle.has_next() ? paragraph_handle.current : pugi::xml_node{};
    if (paragraph_node == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "paragraph index is out of range",
                       std::string{document_xml_entry});
        return false;
    }

    const auto spans = collect_paragraph_revision_run_spans(paragraph_node);
    const auto paragraph_text_length = paragraph_revision_text_length(spans);
    if (text_offset > paragraph_text_length ||
        text_length > paragraph_text_length - text_offset) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "text range is out of range",
                       std::string{document_xml_entry});
        return false;
    }
    if (!split_paragraph_revision_text_at(paragraph_node, text_offset + text_length) ||
        !split_paragraph_revision_text_at(paragraph_node, text_offset)) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::operation_not_supported),
                       "paragraph text revision range requires plain text runs",
                       std::string{document_xml_entry});
        return false;
    }

    const auto selected =
        paragraph_revision_spans_in_range(paragraph_node, text_offset, text_length);
    if (selected.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "text range is out of range",
                       std::string{document_xml_entry});
        return false;
    }

    auto revision = paragraph_node.insert_child_before("w:del", selected.front().run);
    if (revision == pugi::xml_node{} ||
        !append_revision_runs_from_spans(revision, selected, "w:delText")) {
        if (revision != pugi::xml_node{}) {
            paragraph_node.remove_child(revision);
        }
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to delete paragraph text revision",
                       std::string{document_xml_entry});
        return false;
    }

    for (const auto &span : selected) {
        if (!paragraph_node.remove_child(span.run)) {
            paragraph_node.remove_child(revision);
            set_last_error(this->last_error_info,
                           std::make_error_code(std::errc::not_enough_memory),
                           "failed to delete paragraph text revision",
                           std::string{document_xml_entry});
            return false;
        }
    }

    long revision_id = 1L;
    collect_revision_ids(this->document, revision_id);
    for (const auto &part : this->header_parts) {
        if (part != nullptr) {
            collect_revision_ids(part->xml, revision_id);
        }
    }
    for (const auto &part : this->footer_parts) {
        if (part != nullptr) {
            collect_revision_ids(part->xml, revision_id);
        }
    }
    set_revision_identity_metadata(revision, revision_id, options.author,
                                   options.date);
    this->last_error_info.clear();
    return true;
}

bool Document::replace_paragraph_text_revision(
    std::size_t paragraph_index, std::size_t text_offset, std::size_t text_length,
    std::string_view text, std::string_view author, std::string_view date) {
    featherdoc::revision_text_range_options options;
    options.author = std::string{author};
    options.date = std::string{date};
    return this->replace_paragraph_text_revision(paragraph_index, text_offset,
                                                 text_length, text, options);
}

bool Document::replace_paragraph_text_revision(
    std::size_t paragraph_index, std::size_t text_offset, std::size_t text_length,
    std::string_view text,
    const featherdoc::revision_text_range_options &options) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before replacing a paragraph text revision",
                       std::string{document_xml_entry});
        return false;
    }
    if (text.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "revision text must not be empty",
                       std::string{document_xml_entry});
        return false;
    }
    if (text_length == 0U) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "text range must not be empty",
                       std::string{document_xml_entry});
        return false;
    }
    if (text_length > std::numeric_limits<std::size_t>::max() - text_offset) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "text range is out of range",
                       std::string{document_xml_entry});
        return false;
    }
    if (options.expected_text.has_value()) {
        const auto preview = this->preview_text_range(
            paragraph_index, text_offset, paragraph_index,
            text_offset + text_length);
        if (!preview.has_value() ||
            !validate_revision_expected_text(this->last_error_info, *preview,
                                             options)) {
            return false;
        }
    }

    auto paragraph_handle = this->paragraphs();
    for (std::size_t current_index = 0U;
         current_index < paragraph_index && paragraph_handle.has_next();
         ++current_index) {
        paragraph_handle.next();
    }
    auto paragraph_node = paragraph_handle.has_next() ? paragraph_handle.current : pugi::xml_node{};
    if (paragraph_node == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "paragraph index is out of range",
                       std::string{document_xml_entry});
        return false;
    }

    const auto spans = collect_paragraph_revision_run_spans(paragraph_node);
    const auto paragraph_text_length = paragraph_revision_text_length(spans);
    if (text_offset > paragraph_text_length ||
        text_length > paragraph_text_length - text_offset) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "text range is out of range",
                       std::string{document_xml_entry});
        return false;
    }
    if (!split_paragraph_revision_text_at(paragraph_node, text_offset + text_length) ||
        !split_paragraph_revision_text_at(paragraph_node, text_offset)) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::operation_not_supported),
                       "paragraph text revision range requires plain text runs",
                       std::string{document_xml_entry});
        return false;
    }

    const auto selected =
        paragraph_revision_spans_in_range(paragraph_node, text_offset, text_length);
    if (selected.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "text range is out of range",
                       std::string{document_xml_entry});
        return false;
    }

    long revision_id = 1L;
    collect_revision_ids(this->document, revision_id);
    for (const auto &part : this->header_parts) {
        if (part != nullptr) {
            collect_revision_ids(part->xml, revision_id);
        }
    }
    for (const auto &part : this->footer_parts) {
        if (part != nullptr) {
            collect_revision_ids(part->xml, revision_id);
        }
    }

    auto deletion = paragraph_node.insert_child_before("w:del", selected.front().run);
    auto insertion = deletion == pugi::xml_node{}
                         ? pugi::xml_node{}
                         : paragraph_node.insert_child_before("w:ins", selected.front().run);
    if (deletion == pugi::xml_node{} || insertion == pugi::xml_node{} ||
        !append_revision_runs_from_spans(deletion, selected, "w:delText") ||
        append_revision_run(insertion, selected.front().run, "w:t", text) ==
            pugi::xml_node{}) {
        if (deletion != pugi::xml_node{}) {
            paragraph_node.remove_child(deletion);
        }
        if (insertion != pugi::xml_node{}) {
            paragraph_node.remove_child(insertion);
        }
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to replace paragraph text revision",
                       std::string{document_xml_entry});
        return false;
    }

    for (const auto &span : selected) {
        if (!paragraph_node.remove_child(span.run)) {
            paragraph_node.remove_child(deletion);
            paragraph_node.remove_child(insertion);
            set_last_error(this->last_error_info,
                           std::make_error_code(std::errc::not_enough_memory),
                           "failed to replace paragraph text revision",
                           std::string{document_xml_entry});
            return false;
        }
    }

    set_revision_identity_metadata(deletion, revision_id, options.author,
                                   options.date);
    set_revision_identity_metadata(insertion, revision_id + 1L, options.author,
                                   options.date);
    this->last_error_info.clear();
    return true;
}

bool Document::insert_text_range_revision(
    std::size_t paragraph_index, std::size_t text_offset, std::string_view text,
    std::string_view author, std::string_view date) {
    return this->insert_paragraph_text_revision(paragraph_index, text_offset, text,
                                                author, date);
}

bool Document::delete_text_range_revision(
    std::size_t start_paragraph_index, std::size_t start_text_offset,
    std::size_t end_paragraph_index, std::size_t end_text_offset,
    std::string_view author, std::string_view date) {
    featherdoc::revision_text_range_options options;
    options.author = std::string{author};
    options.date = std::string{date};
    return this->delete_text_range_revision(
        start_paragraph_index, start_text_offset, end_paragraph_index,
        end_text_offset, options);
}

bool Document::delete_text_range_revision(
    std::size_t start_paragraph_index, std::size_t start_text_offset,
    std::size_t end_paragraph_index, std::size_t end_text_offset,
    const featherdoc::revision_text_range_options &options) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before deleting a text range revision",
                       std::string{document_xml_entry});
        return false;
    }
    if (options.expected_text.has_value()) {
        const auto preview = this->preview_text_range(
            start_paragraph_index, start_text_offset, end_paragraph_index,
            end_text_offset);
        if (!preview.has_value() ||
            !validate_revision_expected_text(this->last_error_info, *preview,
                                             options)) {
            return false;
        }
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
                       "text range revision requires plain text runs",
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

    long revision_id = 1L;
    collect_revision_ids(this->document, revision_id);
    for (const auto &part : this->header_parts) {
        if (part != nullptr) {
            collect_revision_ids(part->xml, revision_id);
        }
    }
    for (const auto &part : this->footer_parts) {
        if (part != nullptr) {
            collect_revision_ids(part->xml, revision_id);
        }
    }

    for (std::size_t segment_index = 0U; segment_index < plan.segments.size();
         ++segment_index) {
        auto paragraph_node = plan.segments[segment_index].paragraph;
        const auto &selected = selected_segments[segment_index];
        auto revision = paragraph_node.insert_child_before("w:del", selected.front().run);
        if (revision == pugi::xml_node{} ||
            !append_revision_runs_from_spans(revision, selected, "w:delText")) {
            if (revision != pugi::xml_node{}) {
                paragraph_node.remove_child(revision);
            }
            set_last_error(this->last_error_info,
                           std::make_error_code(std::errc::not_enough_memory),
                           "failed to delete text range revision",
                           std::string{document_xml_entry});
            return false;
        }

        for (const auto &span : selected) {
            if (!paragraph_node.remove_child(span.run)) {
                paragraph_node.remove_child(revision);
                set_last_error(this->last_error_info,
                               std::make_error_code(std::errc::not_enough_memory),
                               "failed to delete text range revision",
                               std::string{document_xml_entry});
                return false;
            }
        }

        set_revision_identity_metadata(revision, revision_id, options.author,
                                       options.date);
        ++revision_id;
    }

    this->last_error_info.clear();
    return true;
}

bool Document::replace_text_range_revision(
    std::size_t start_paragraph_index, std::size_t start_text_offset,
    std::size_t end_paragraph_index, std::size_t end_text_offset,
    std::string_view text, std::string_view author, std::string_view date) {
    featherdoc::revision_text_range_options options;
    options.author = std::string{author};
    options.date = std::string{date};
    return this->replace_text_range_revision(
        start_paragraph_index, start_text_offset, end_paragraph_index,
        end_text_offset, text, options);
}

bool Document::replace_text_range_revision(
    std::size_t start_paragraph_index, std::size_t start_text_offset,
    std::size_t end_paragraph_index, std::size_t end_text_offset,
    std::string_view text,
    const featherdoc::revision_text_range_options &options) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before replacing a text range revision",
                       std::string{document_xml_entry});
        return false;
    }
    if (text.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "revision text must not be empty",
                       std::string{document_xml_entry});
        return false;
    }
    if (options.expected_text.has_value()) {
        const auto preview = this->preview_text_range(
            start_paragraph_index, start_text_offset, end_paragraph_index,
            end_text_offset);
        if (!preview.has_value() ||
            !validate_revision_expected_text(this->last_error_info, *preview,
                                             options)) {
            return false;
        }
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
                       "text range revision requires plain text runs",
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

    long revision_id = 1L;
    collect_revision_ids(this->document, revision_id);
    for (const auto &part : this->header_parts) {
        if (part != nullptr) {
            collect_revision_ids(part->xml, revision_id);
        }
    }
    for (const auto &part : this->footer_parts) {
        if (part != nullptr) {
            collect_revision_ids(part->xml, revision_id);
        }
    }

    auto start_paragraph = paragraphs[start_paragraph_index];
    const auto insertion_before =
        paragraph_revision_insert_before(start_paragraph, start_text_offset);
    const auto style_run =
        paragraph_revision_style_run(start_paragraph, start_text_offset);
    bool inserted_replacement = false;

    for (std::size_t segment_index = 0U; segment_index < plan.segments.size();
         ++segment_index) {
        auto paragraph_node = plan.segments[segment_index].paragraph;
        const auto &selected = selected_segments[segment_index];
        auto deletion = paragraph_node.insert_child_before("w:del", selected.front().run);
        if (deletion == pugi::xml_node{} ||
            !append_revision_runs_from_spans(deletion, selected, "w:delText")) {
            if (deletion != pugi::xml_node{}) {
                paragraph_node.remove_child(deletion);
            }
            set_last_error(this->last_error_info,
                           std::make_error_code(std::errc::not_enough_memory),
                           "failed to replace text range revision",
                           std::string{document_xml_entry});
            return false;
        }
        set_revision_identity_metadata(deletion, revision_id, options.author,
                                       options.date);
        ++revision_id;

        if (!inserted_replacement &&
            plan.segments[segment_index].paragraph == start_paragraph) {
            auto insertion =
                paragraph_node.insert_child_before("w:ins", selected.front().run);
            if (insertion == pugi::xml_node{} ||
                append_revision_run(insertion, selected.front().run, "w:t", text) ==
                    pugi::xml_node{}) {
                if (insertion != pugi::xml_node{}) {
                    paragraph_node.remove_child(insertion);
                }
                paragraph_node.remove_child(deletion);
                set_last_error(this->last_error_info,
                               std::make_error_code(std::errc::not_enough_memory),
                               "failed to replace text range revision",
                               std::string{document_xml_entry});
                return false;
            }
            set_revision_identity_metadata(insertion, revision_id,
                                           options.author, options.date);
            ++revision_id;
            inserted_replacement = true;
        }

        for (const auto &span : selected) {
            if (!paragraph_node.remove_child(span.run)) {
                paragraph_node.remove_child(deletion);
                set_last_error(this->last_error_info,
                               std::make_error_code(std::errc::not_enough_memory),
                               "failed to replace text range revision",
                               std::string{document_xml_entry});
                return false;
            }
        }
    }

    if (!inserted_replacement) {
        auto insertion = insert_revision_node(start_paragraph, "w:ins",
                                              insertion_before);
        if (insertion == pugi::xml_node{} ||
            append_revision_run(insertion, style_run, "w:t", text) ==
                pugi::xml_node{}) {
            if (insertion != pugi::xml_node{}) {
                start_paragraph.remove_child(insertion);
            }
            set_last_error(this->last_error_info,
                           std::make_error_code(std::errc::not_enough_memory),
                           "failed to replace text range revision",
                           std::string{document_xml_entry});
            return false;
        }
        set_revision_identity_metadata(insertion, revision_id, options.author,
                                       options.date);
    }

    this->last_error_info.clear();
    return true;
}
