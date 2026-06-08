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

std::optional<featherdoc::text_range_preview> Document::preview_text_range(
    std::size_t start_paragraph_index, std::size_t start_text_offset,
    std::size_t end_paragraph_index, std::size_t end_text_offset) const {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before previewing a text range",
                       std::string{document_xml_entry});
        return std::nullopt;
    }

    std::vector<pugi::xml_node> paragraphs;
    auto body =
        const_cast<pugi::xml_document &>(this->document)
            .child("w:document")
            .child("w:body");
    for (auto child = body.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        if (std::string_view{child.name()} == "w:p") {
            paragraphs.push_back(child);
        }
    }

    paragraph_revision_range_plan plan;
    std::string range_error;
    if (!build_paragraph_revision_range_plan(
            paragraphs, start_paragraph_index, start_text_offset,
            end_paragraph_index, end_text_offset, plan, range_error)) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       range_error, std::string{document_xml_entry});
        return std::nullopt;
    }

    auto preview = preview_paragraph_revision_range_segments(
        start_paragraph_index, start_text_offset, end_paragraph_index,
        end_text_offset, plan);
    this->last_error_info.clear();
    return preview;
}

std::vector<featherdoc::text_range_preview> Document::find_text_ranges(
    std::string_view text) const {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before finding text ranges",
                       std::string{document_xml_entry});
        return {};
    }
    if (text.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "search text must not be empty",
                       std::string{document_xml_entry});
        return {};
    }

    struct paragraph_text_range {
        std::size_t paragraph_index{};
        std::size_t global_start{};
        std::size_t text_length{};
    };

    std::vector<pugi::xml_node> paragraphs;
    std::vector<paragraph_text_range> paragraph_ranges;
    std::string document_text;
    auto body =
        const_cast<pugi::xml_document &>(this->document)
            .child("w:document")
            .child("w:body");
    std::size_t paragraph_index = 0U;
    for (auto child = body.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        if (std::string_view{child.name()} != "w:p") {
            continue;
        }

        paragraphs.push_back(child);
        paragraph_text_range range;
        range.paragraph_index = paragraph_index;
        range.global_start = document_text.size();
        for (const auto &span : collect_paragraph_revision_run_spans(child)) {
            document_text += span.text;
        }
        range.text_length = document_text.size() - range.global_start;
        paragraph_ranges.push_back(range);
        ++paragraph_index;
    }

    const auto map_match_start =
        [&paragraph_ranges](std::size_t global_offset, std::size_t &mapped_paragraph,
                            std::size_t &mapped_offset) -> bool {
        if (paragraph_ranges.empty()) {
            return false;
        }
        for (const auto &range : paragraph_ranges) {
            if (range.text_length == 0U) {
                continue;
            }
            const auto range_end = range.global_start + range.text_length;
            if (global_offset < range_end) {
                mapped_paragraph = range.paragraph_index;
                mapped_offset = global_offset - range.global_start;
                return true;
            }
        }

        const auto &last = paragraph_ranges.back();
        mapped_paragraph = last.paragraph_index;
        mapped_offset = last.text_length;
        return true;
    };

    const auto map_match_end =
        [&paragraph_ranges](std::size_t global_offset, std::size_t &mapped_paragraph,
                            std::size_t &mapped_offset) -> bool {
        if (paragraph_ranges.empty()) {
            return false;
        }

        std::optional<paragraph_text_range> previous_text_range;
        for (const auto &range : paragraph_ranges) {
            if (range.text_length == 0U) {
                continue;
            }
            const auto range_end = range.global_start + range.text_length;
            if (global_offset > range.global_start && global_offset <= range_end) {
                mapped_paragraph = range.paragraph_index;
                mapped_offset = global_offset - range.global_start;
                return true;
            }
            if (global_offset == range.global_start &&
                previous_text_range.has_value()) {
                mapped_paragraph = previous_text_range->paragraph_index;
                mapped_offset = previous_text_range->text_length;
                return true;
            }
            previous_text_range = range;
        }

        if (previous_text_range.has_value()) {
            mapped_paragraph = previous_text_range->paragraph_index;
            mapped_offset = previous_text_range->text_length;
            return true;
        }

        const auto &last = paragraph_ranges.back();
        mapped_paragraph = last.paragraph_index;
        mapped_offset = last.text_length;
        return true;
    };

    std::vector<featherdoc::text_range_preview> matches;
    std::size_t search_offset = 0U;
    while (search_offset <= document_text.size()) {
        const auto match_offset = document_text.find(text, search_offset);
        if (match_offset == std::string::npos) {
            break;
        }

        std::size_t start_paragraph_index = 0U;
        std::size_t start_text_offset = 0U;
        std::size_t end_paragraph_index = 0U;
        std::size_t end_text_offset = 0U;
        if (!map_match_start(match_offset, start_paragraph_index,
                             start_text_offset) ||
            !map_match_end(match_offset + text.size(), end_paragraph_index,
                           end_text_offset)) {
            break;
        }

        paragraph_revision_range_plan plan;
        std::string range_error;
        if (!build_paragraph_revision_range_plan(
                paragraphs, start_paragraph_index, start_text_offset,
                end_paragraph_index, end_text_offset, plan, range_error)) {
            set_last_error(this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           range_error, std::string{document_xml_entry});
            return {};
        }

        matches.push_back(preview_paragraph_revision_range_segments(
            start_paragraph_index, start_text_offset, end_paragraph_index,
            end_text_offset, plan));
        search_offset = match_offset + 1U;
    }

    this->last_error_info.clear();
    return matches;
}

std::vector<featherdoc::revision_summary> Document::list_revisions() const {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before listing revisions",
                       std::string{document_xml_entry});
        return {};
    }

    std::vector<featherdoc::revision_summary> summaries;
    collect_revisions_from_node(const_cast<pugi::xml_document &>(this->document),
                                document_xml_entry, summaries);
    for (const auto &part : this->header_parts) {
        if (part != nullptr) {
            collect_revisions_from_node(part->xml, part->entry_name, summaries);
        }
    }
    for (const auto &part : this->footer_parts) {
        if (part != nullptr) {
            collect_revisions_from_node(part->xml, part->entry_name, summaries);
        }
    }

    this->last_error_info.clear();
    return summaries;
}

bool Document::set_revision_metadata(
    std::size_t revision_index,
    const featherdoc::revision_metadata_update &metadata) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before setting revision metadata",
                       std::string{document_xml_entry});
        return false;
    }
    const auto has_change = metadata.author.has_value() ||
                            metadata.date.has_value() ||
                            metadata.clear_author || metadata.clear_date;
    if (!has_change) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "revision metadata update must contain at least one change",
                       std::string{document_xml_entry});
        return false;
    }
    if ((metadata.author.has_value() && metadata.clear_author) ||
        (metadata.date.has_value() && metadata.clear_date)) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "revision metadata cannot be set and cleared in the same update",
                       std::string{document_xml_entry});
        return false;
    }

    std::vector<pugi::xml_node> revision_nodes;
    collect_revision_nodes_in_order(this->document, revision_nodes);
    for (auto &part : this->header_parts) {
        if (part != nullptr) {
            collect_revision_nodes_in_order(part->xml, revision_nodes);
        }
    }
    for (auto &part : this->footer_parts) {
        if (part != nullptr) {
            collect_revision_nodes_in_order(part->xml, revision_nodes);
        }
    }
    if (revision_index >= revision_nodes.size()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "revision index is out of range",
                       std::string{document_xml_entry});
        return false;
    }

    auto revision = revision_nodes[revision_index];
    if (metadata.author.has_value()) {
        ensure_attribute_value(revision, "w:author", *metadata.author);
    } else if (metadata.clear_author) {
        revision.remove_attribute("w:author");
    }
    if (metadata.date.has_value()) {
        ensure_attribute_value(revision, "w:date", *metadata.date);
    } else if (metadata.clear_date) {
        revision.remove_attribute("w:date");
    }

    this->last_error_info.clear();
    return true;
}

bool Document::accept_revision(std::size_t revision_index) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before accepting a revision",
                       std::string{document_xml_entry});
        return false;
    }

    std::vector<pugi::xml_node> revision_nodes;
    collect_revision_nodes_in_order(this->document, revision_nodes);
    for (auto &part : this->header_parts) {
        if (part != nullptr) {
            collect_revision_nodes_in_order(part->xml, revision_nodes);
        }
    }
    for (auto &part : this->footer_parts) {
        if (part != nullptr) {
            collect_revision_nodes_in_order(part->xml, revision_nodes);
        }
    }
    if (revision_index >= revision_nodes.size()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "revision index is out of range",
                       std::string{document_xml_entry});
        return false;
    }
    if (!apply_revision_decision(revision_nodes[revision_index], true)) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::io_error),
                       "failed to accept revision",
                       std::string{document_xml_entry});
        return false;
    }
    this->last_error_info.clear();
    return true;
}

bool Document::reject_revision(std::size_t revision_index) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before rejecting a revision",
                       std::string{document_xml_entry});
        return false;
    }

    std::vector<pugi::xml_node> revision_nodes;
    collect_revision_nodes_in_order(this->document, revision_nodes);
    for (auto &part : this->header_parts) {
        if (part != nullptr) {
            collect_revision_nodes_in_order(part->xml, revision_nodes);
        }
    }
    for (auto &part : this->footer_parts) {
        if (part != nullptr) {
            collect_revision_nodes_in_order(part->xml, revision_nodes);
        }
    }
    if (revision_index >= revision_nodes.size()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "revision index is out of range",
                       std::string{document_xml_entry});
        return false;
    }
    if (!apply_revision_decision(revision_nodes[revision_index], false)) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::io_error),
                       "failed to reject revision",
                       std::string{document_xml_entry});
        return false;
    }
    this->last_error_info.clear();
    return true;
}

std::size_t Document::accept_all_revisions() {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before accepting revisions",
                       std::string{document_xml_entry});
        return 0U;
    }

    std::size_t accepted = 0U;
    for (;;) {
        std::vector<pugi::xml_node> revision_nodes;
        collect_revision_nodes_in_order(this->document, revision_nodes);
        for (auto &part : this->header_parts) {
            if (part != nullptr) {
                collect_revision_nodes_in_order(part->xml, revision_nodes);
            }
        }
        for (auto &part : this->footer_parts) {
            if (part != nullptr) {
                collect_revision_nodes_in_order(part->xml, revision_nodes);
            }
        }
        if (revision_nodes.empty()) {
            break;
        }
        if (!apply_revision_decision(revision_nodes.front(), true)) {
            set_last_error(this->last_error_info,
                           std::make_error_code(std::errc::io_error),
                           "failed to accept revision",
                           std::string{document_xml_entry});
            return accepted;
        }
        ++accepted;
    }

    this->last_error_info.clear();
    return accepted;
}

std::size_t Document::reject_all_revisions() {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before rejecting revisions",
                       std::string{document_xml_entry});
        return 0U;
    }

    std::size_t rejected = 0U;
    for (;;) {
        std::vector<pugi::xml_node> revision_nodes;
        collect_revision_nodes_in_order(this->document, revision_nodes);
        for (auto &part : this->header_parts) {
            if (part != nullptr) {
                collect_revision_nodes_in_order(part->xml, revision_nodes);
            }
        }
        for (auto &part : this->footer_parts) {
            if (part != nullptr) {
                collect_revision_nodes_in_order(part->xml, revision_nodes);
            }
        }
        if (revision_nodes.empty()) {
            break;
        }
        if (!apply_revision_decision(revision_nodes.front(), false)) {
            set_last_error(this->last_error_info,
                           std::make_error_code(std::errc::io_error),
                           "failed to reject revision",
                           std::string{document_xml_entry});
            return rejected;
        }
        ++rejected;
    }

    this->last_error_info.clear();
    return rejected;
}

bool Document::ensure_review_notes_part(featherdoc::review_note_kind kind) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before editing review notes",
                       std::string{document_xml_entry});
        return false;
    }
    if (kind != featherdoc::review_note_kind::footnote &&
        kind != featherdoc::review_note_kind::endnote) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "review note kind must be footnote or endnote",
                       std::string{document_xml_entry});
        return false;
    }
    if (const auto error = this->ensure_content_types_loaded()) {
        return false;
    }

    const bool footnote = kind == featherdoc::review_note_kind::footnote;
    auto &notes_document = footnote ? this->footnotes : this->endnotes;
    auto &has_notes_part = footnote ? this->has_footnotes_part : this->has_endnotes_part;
    auto &notes_loaded = footnote ? this->footnotes_loaded : this->endnotes_loaded;
    auto &notes_dirty = footnote ? this->footnotes_dirty : this->endnotes_dirty;
    const auto part_entry = footnote ? footnotes_xml_entry : endnotes_xml_entry;
    const auto relationship_type = footnote ? footnotes_relationship_type
                                            : endnotes_relationship_type;
    const auto relationship_target = footnote ? footnotes_relationship_target
                                              : endnotes_relationship_target;
    const auto content_type = footnote ? footnotes_content_type : endnotes_content_type;
    const char *root_name = footnote ? "w:footnotes" : "w:endnotes";

    if (!notes_loaded) {
        bool loaded_existing_part = false;
        if (this->has_source_archive && !this->document_path.empty() &&
            std::filesystem::exists(this->document_path)) {
            const auto entry_name = related_part_entry_for_type(
                this->document_relationships, relationship_type, part_entry);
            if (!load_optional_docx_xml_part(this->document_path, entry_name,
                                             notes_document, this->last_error_info)) {
                return false;
            }
            loaded_existing_part = notes_document.document_element() != pugi::xml_node{};
        }
        if (!loaded_existing_part && !initialize_review_notes_document(notes_document, kind)) {
            set_last_error(this->last_error_info,
                           std::make_error_code(std::errc::not_enough_memory),
                           "failed to initialize review notes part",
                           std::string{part_entry});
            return false;
        }
        notes_loaded = true;
    }

    auto root = notes_document.child(root_name);
    if (root == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "review notes part has an unsupported root",
                       std::string{part_entry});
        return false;
    }
    ensure_wordprocessingml_namespace(root);

    if (!ensure_document_relationship(this->document_relationships,
                                      this->has_document_relationships_part,
                                      this->document_relationships_dirty,
                                      this->last_error_info, relationship_type,
                                      relationship_target)) {
        return false;
    }
    if (!add_content_type_override(this->content_types, this->content_types_dirty,
                                   this->last_error_info, part_entry, content_type)) {
        return false;
    }

    has_notes_part = true;
    notes_dirty = true;
    this->last_error_info.clear();
    return true;
}

bool Document::ensure_comments_part() {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before editing comments",
                       std::string{document_xml_entry});
        return false;
    }
    if (const auto error = this->ensure_content_types_loaded()) {
        return false;
    }

    if (!this->comments_loaded) {
        bool loaded_existing_part = false;
        if (this->has_source_archive && !this->document_path.empty() &&
            std::filesystem::exists(this->document_path)) {
            const auto entry_name = related_part_entry_for_type(
                this->document_relationships, comments_relationship_type,
                comments_xml_entry);
            if (!load_optional_docx_xml_part(this->document_path, entry_name,
                                             this->comments, this->last_error_info)) {
                return false;
            }
            loaded_existing_part = this->comments.document_element() != pugi::xml_node{};
        }
        if (!loaded_existing_part && !initialize_comments_document(this->comments)) {
            set_last_error(this->last_error_info,
                           std::make_error_code(std::errc::not_enough_memory),
                           "failed to initialize comments part",
                           std::string{comments_xml_entry});
            return false;
        }
        this->comments_loaded = true;
    }

    auto root = this->comments.child("w:comments");
    if (root == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "comments part has an unsupported root",
                       std::string{comments_xml_entry});
        return false;
    }
    ensure_wordprocessingml_namespace(root);

    if (!ensure_document_relationship(this->document_relationships,
                                      this->has_document_relationships_part,
                                      this->document_relationships_dirty,
                                      this->last_error_info, comments_relationship_type,
                                      comments_relationship_target)) {
        return false;
    }
    if (!add_content_type_override(this->content_types, this->content_types_dirty,
                                   this->last_error_info, comments_xml_entry,
                                   comments_content_type)) {
        return false;
    }

    this->has_comments_part = true;
    this->comments_dirty = true;
    this->last_error_info.clear();
    return true;
}

bool Document::ensure_comments_extended_loaded() {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before loading comment metadata",
                       std::string{document_xml_entry});
        return false;
    }
    if (this->comments_extended_loaded) {
        return true;
    }

    this->comments_extended.reset();
    this->has_comments_extended_part = false;
    bool loaded_existing_part = false;
    if (this->has_source_archive && !this->document_path.empty() &&
        std::filesystem::exists(this->document_path)) {
        const auto entry_name = optional_related_part_entry_for_type(
            this->document_relationships, comments_extended_relationship_type);
        if (entry_name.has_value()) {
            if (!load_optional_docx_xml_part(
                    this->document_path, *entry_name, this->comments_extended,
                    this->last_error_info)) {
                return false;
            }
            loaded_existing_part =
                this->comments_extended.document_element() != pugi::xml_node{};
        }
    }

    if (!loaded_existing_part &&
        !initialize_comments_extended_document(this->comments_extended)) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to initialize extended comments part",
                       std::string{comments_extended_xml_entry});
        return false;
    }

    this->comments_extended_loaded = true;
    this->has_comments_extended_part = loaded_existing_part;
    this->last_error_info.clear();
    return true;
}

bool Document::ensure_comments_extended_part() {
    if (const auto error = this->ensure_content_types_loaded()) {
        return false;
    }
    if (!this->ensure_comments_extended_loaded()) {
        return false;
    }

    auto root = this->comments_extended.child("w15:commentsEx");
    if (root == pugi::xml_node{}) {
        if (!initialize_comments_extended_document(this->comments_extended)) {
            set_last_error(this->last_error_info,
                           std::make_error_code(std::errc::not_enough_memory),
                           "failed to initialize extended comments part",
                           std::string{comments_extended_xml_entry});
            return false;
        }
        root = this->comments_extended.child("w15:commentsEx");
    }
    if (root == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "extended comments part has an unsupported root",
                       std::string{comments_extended_xml_entry});
        return false;
    }
    ensure_wordprocessingml_2012_namespace(root);

    if (!ensure_document_relationship(
            this->document_relationships, this->has_document_relationships_part,
            this->document_relationships_dirty, this->last_error_info,
            comments_extended_relationship_type,
            comments_extended_relationship_target)) {
        return false;
    }
    if (!add_content_type_override(this->content_types, this->content_types_dirty,
                                   this->last_error_info,
                                   comments_extended_xml_entry,
                                   comments_extended_content_type)) {
        return false;
    }

    this->has_comments_extended_part = true;
    this->comments_extended_dirty = true;
    this->last_error_info.clear();
    return true;
}
