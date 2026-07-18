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
    const std::vector<paragraph_revision_range_segment> range_segments{
        {paragraph_node, paragraph_index, text_offset, text_length}};
    if (!paragraph_revision_range_segments_supported(range_segments)) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::operation_not_supported),
                       "paragraph text revision range requires plain text runs",
                       std::string{document_xml_entry});
        return false;
    }

    const auto revision_id = this->reserve_review_revision_identifiers(1U);
    if (!revision_id.has_value()) {
        return false;
    }

    pugi::xml_document revision_document;
    if (featherdoc::detail::checked_clone_xml_document(
            this->document, revision_document) !=
        featherdoc::detail::xml_document_clone_status::success) {
        (void)featherdoc::detail::set_xml_document_clone_allocation_failure(
            this->last_error_info, document_xml_entry);
        return false;
    }

    try {
        auto work_paragraphs =
            collect_revision_document_paragraphs(revision_document);
        if (paragraph_index >= work_paragraphs.size()) {
            set_last_error(this->last_error_info,
                           document_errc::invalid_package_structure,
                           "cloned document.xml paragraph list changed unexpectedly",
                           std::string{document_xml_entry});
            return false;
        }
        auto work_paragraph = work_paragraphs[paragraph_index];

        if (!checked_split_paragraph_revision_text_at(
                work_paragraph, text_offset + text_length) ||
            !checked_split_paragraph_revision_text_at(work_paragraph,
                                                      text_offset)) {
            (void)featherdoc::detail::
                set_xml_document_mutation_allocation_failure(
                    this->last_error_info, document_xml_entry);
            return false;
        }

        const auto selected = paragraph_revision_spans_in_range(
            work_paragraph, text_offset, text_length);
        if (selected.empty()) {
            set_last_error(this->last_error_info,
                           document_errc::invalid_package_structure,
                           "cloned document.xml text range changed unexpectedly",
                           std::string{document_xml_entry});
            return false;
        }

        auto revision =
            featherdoc::detail::checked_insert_xml_element_before(
                work_paragraph, "w:del", selected.front().run);
        if (revision == pugi::xml_node{} ||
            !append_revision_runs_from_spans(revision, selected,
                                             "w:delText") ||
            !set_revision_identity_metadata(revision, *revision_id,
                                            options.author, options.date)) {
            (void)featherdoc::detail::
                set_xml_document_mutation_allocation_failure(
                    this->last_error_info, document_xml_entry);
            return false;
        }

        for (const auto &span : selected) {
            if (!work_paragraph.remove_child(span.run)) {
                set_last_error(this->last_error_info,
                               document_errc::invalid_package_structure,
                               "cloned document.xml run list changed unexpectedly",
                               std::string{document_xml_entry});
                return false;
            }
        }

        const auto published_body =
            this->document.child("w:document").child("w:body");
        static_assert(noexcept(std::declval<pugi::xml_document &>() =
                               std::declval<pugi::xml_document &&>()));
        this->xml_handle_lifetime->retire_subtree(published_body);
        this->document = std::move(revision_document);
        this->rebind_internal_xml_handles();
    } catch (...) {
        (void)featherdoc::detail::set_xml_document_allocation_failure(
            this->last_error_info,
            "paragraph text revision ran out of memory before it could be published",
            document_xml_entry);
        return false;
    }

    this->last_error_info.clear();
    return true;
}
