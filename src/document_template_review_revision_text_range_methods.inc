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
