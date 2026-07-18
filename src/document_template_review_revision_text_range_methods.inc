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
    if (!paragraph_revision_range_segments_supported(plan.segments)) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::operation_not_supported),
                       "text range revision requires plain text runs",
                       std::string{document_xml_entry});
        return false;
    }

    const auto first_revision_id =
        this->reserve_review_revision_identifiers(plan.segments.size());
    if (!first_revision_id.has_value()) {
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
        paragraph_revision_range_plan work_plan;
        std::string work_range_error;
        if (!build_paragraph_revision_range_plan(
                work_paragraphs, start_paragraph_index, start_text_offset,
                end_paragraph_index, end_text_offset, work_plan,
                work_range_error) ||
            work_plan.segments.size() != plan.segments.size()) {
            set_last_error(this->last_error_info,
                           document_errc::invalid_package_structure,
                           "cloned document.xml text range changed unexpectedly",
                           std::string{document_xml_entry});
            return false;
        }

        if (!checked_split_paragraph_revision_range_segments(
                work_plan.segments)) {
            (void)featherdoc::detail::
                set_xml_document_mutation_allocation_failure(
                    this->last_error_info, document_xml_entry);
            return false;
        }

        const auto selected_segments =
            collect_selected_paragraph_revision_spans(work_plan.segments);
        if (selected_segments.size() != work_plan.segments.size()) {
            set_last_error(this->last_error_info,
                           document_errc::invalid_package_structure,
                           "cloned document.xml text range changed unexpectedly",
                           std::string{document_xml_entry});
            return false;
        }

        for (std::size_t segment_index = 0U;
             segment_index < work_plan.segments.size(); ++segment_index) {
            auto work_paragraph =
                work_plan.segments[segment_index].paragraph;
            const auto &selected = selected_segments[segment_index];
            auto revision =
                featherdoc::detail::checked_insert_xml_element_before(
                    work_paragraph, "w:del", selected.front().run);
            const auto revision_id =
                *first_revision_id +
                static_cast<review_identifier>(segment_index);
            if (revision == pugi::xml_node{} ||
                !append_revision_runs_from_spans(revision, selected,
                                                 "w:delText") ||
                !set_revision_identity_metadata(revision, revision_id,
                                                options.author,
                                                options.date)) {
                (void)featherdoc::detail::
                    set_xml_document_mutation_allocation_failure(
                        this->last_error_info, document_xml_entry);
                return false;
            }

            for (const auto &span : selected) {
                if (!work_paragraph.remove_child(span.run)) {
                    set_last_error(
                        this->last_error_info,
                        document_errc::invalid_package_structure,
                        "cloned document.xml run list changed unexpectedly",
                        std::string{document_xml_entry});
                    return false;
                }
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
            "text range deletion revision ran out of memory before it could be published",
            document_xml_entry);
        return false;
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
    if (!paragraph_revision_range_segments_supported(plan.segments)) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::operation_not_supported),
                       "text range revision requires plain text runs",
                       std::string{document_xml_entry});
        return false;
    }

    const auto first_revision_id =
        this->reserve_review_revision_identifiers(plan.segments.size() + 1U);
    if (!first_revision_id.has_value()) {
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
        paragraph_revision_range_plan work_plan;
        std::string work_range_error;
        if (!build_paragraph_revision_range_plan(
                work_paragraphs, start_paragraph_index, start_text_offset,
                end_paragraph_index, end_text_offset, work_plan,
                work_range_error) ||
            work_plan.segments.size() != plan.segments.size() ||
            start_paragraph_index >= work_paragraphs.size()) {
            set_last_error(this->last_error_info,
                           document_errc::invalid_package_structure,
                           "cloned document.xml text range changed unexpectedly",
                           std::string{document_xml_entry});
            return false;
        }

        if (!checked_split_paragraph_revision_range_segments(
                work_plan.segments)) {
            (void)featherdoc::detail::
                set_xml_document_mutation_allocation_failure(
                    this->last_error_info, document_xml_entry);
            return false;
        }

        const auto selected_segments =
            collect_selected_paragraph_revision_spans(work_plan.segments);
        if (selected_segments.size() != work_plan.segments.size()) {
            set_last_error(this->last_error_info,
                           document_errc::invalid_package_structure,
                           "cloned document.xml text range changed unexpectedly",
                           std::string{document_xml_entry});
            return false;
        }

        auto work_start_paragraph = work_paragraphs[start_paragraph_index];
        const auto insertion_before = paragraph_revision_insert_before(
            work_start_paragraph, start_text_offset);
        const auto style_run = paragraph_revision_style_run(
            work_start_paragraph, start_text_offset);
        bool inserted_replacement = false;
        std::size_t identifier_offset = 0U;

        for (std::size_t segment_index = 0U;
             segment_index < work_plan.segments.size(); ++segment_index) {
            auto work_paragraph =
                work_plan.segments[segment_index].paragraph;
            const auto &selected = selected_segments[segment_index];
            auto deletion =
                featherdoc::detail::checked_insert_xml_element_before(
                    work_paragraph, "w:del", selected.front().run);
            const auto deletion_id =
                *first_revision_id +
                static_cast<review_identifier>(identifier_offset++);
            if (deletion == pugi::xml_node{} ||
                !append_revision_runs_from_spans(deletion, selected,
                                                 "w:delText") ||
                !set_revision_identity_metadata(deletion, deletion_id,
                                                options.author,
                                                options.date)) {
                (void)featherdoc::detail::
                    set_xml_document_mutation_allocation_failure(
                        this->last_error_info, document_xml_entry);
                return false;
            }

            if (!inserted_replacement &&
                work_plan.segments[segment_index].paragraph ==
                    work_start_paragraph) {
                auto insertion =
                    featherdoc::detail::checked_insert_xml_element_before(
                        work_paragraph, "w:ins", selected.front().run);
                const auto insertion_id =
                    *first_revision_id +
                    static_cast<review_identifier>(identifier_offset++);
                if (insertion == pugi::xml_node{} ||
                    append_revision_run(insertion, selected.front().run,
                                        "w:t", text) == pugi::xml_node{} ||
                    !set_revision_identity_metadata(
                        insertion, insertion_id, options.author,
                        options.date)) {
                    (void)featherdoc::detail::
                        set_xml_document_mutation_allocation_failure(
                            this->last_error_info, document_xml_entry);
                    return false;
                }
                inserted_replacement = true;
            }

            for (const auto &span : selected) {
                if (!work_paragraph.remove_child(span.run)) {
                    set_last_error(
                        this->last_error_info,
                        document_errc::invalid_package_structure,
                        "cloned document.xml run list changed unexpectedly",
                        std::string{document_xml_entry});
                    return false;
                }
            }
        }

        if (!inserted_replacement) {
            auto insertion = checked_insert_revision_element(
                work_start_paragraph, "w:ins", insertion_before);
            const auto insertion_id =
                *first_revision_id +
                static_cast<review_identifier>(identifier_offset);
            if (insertion == pugi::xml_node{} ||
                append_revision_run(insertion, style_run, "w:t", text) ==
                    pugi::xml_node{} ||
                !set_revision_identity_metadata(insertion, insertion_id,
                                                options.author,
                                                options.date)) {
                (void)featherdoc::detail::
                    set_xml_document_mutation_allocation_failure(
                        this->last_error_info, document_xml_entry);
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
            "text range replacement revision ran out of memory before it could be published",
            document_xml_entry);
        return false;
    }

    this->last_error_info.clear();
    return true;
}
