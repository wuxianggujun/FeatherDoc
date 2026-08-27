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
    if (!this->ensure_comments_part(false)) {
        return false;
    }

    auto root = this->comments.child("w:comments");
    auto comment = comment_by_summary_index(root, comment_index);
    if (comment == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "comment index is out of range",
                       this->extension_state().singleton_part_entries.comments);
        return false;
    }
    const auto comment_id = std::string{comment.attribute("w:id").value()};
    if (comment_id.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "comment id is missing",
                       this->extension_state().singleton_part_entries.comments);
        return false;
    }

    auto replacements = std::vector<review_paragraph_replacement>{};
    try {
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
            set_last_error(
                this->last_error_info,
                std::make_error_code(std::errc::operation_not_supported),
                "comment range requires plain text runs",
                std::string{document_xml_entry});
            return false;
        }
        review_package_transaction transaction(
            {&this->document_relationships, &this->content_types,
             &this->comments},
            {&this->has_document_relationships_part,
             &this->document_relationships_dirty,
             &this->content_types_loaded, &this->content_types_dirty,
             &this->has_comments_part, &this->comments_loaded,
             &this->comments_dirty},
            {&this->extension_state().singleton_part_entries.comments});
        if (!transaction.ready()) {
            (void)featherdoc::detail::set_xml_document_clone_allocation_failure(
                this->last_error_info,
                this->extension_state().singleton_part_entries.comments);
            return false;
        }
        if (!this->ensure_comments_part()) {
            return false;
        }

        std::vector<pugi::xml_node> replaced_paragraphs;
        replaced_paragraphs.reserve(plan.segments.size());
        for (const auto &segment : plan.segments) {
            replaced_paragraphs.push_back(segment.paragraph);
        }

        const auto comment_ids =
            std::unordered_set<std::string>{comment_id};
        auto marker_removals = review_node_removal_plan{};
        collect_comment_markers_by_ids(
            marker_removals, this->document, comment_ids,
            std::span<const pugi::xml_node>{replaced_paragraphs});
        for (auto &part : this->header_parts) {
            if (part != nullptr) {
                collect_comment_markers_by_ids(marker_removals, part->xml,
                                               comment_ids);
            }
        }
        for (auto &part : this->footer_parts) {
            if (part != nullptr) {
                collect_comment_markers_by_ids(marker_removals, part->xml,
                                               comment_ids);
            }
        }
        if (!finalize_review_node_removal_plan(marker_removals)) {
            set_last_error(this->last_error_info,
                           document_errc::invalid_package_structure,
                           "comment markers changed while planning range move",
                           std::string{document_xml_entry});
            return false;
        }

        paragraph_revision_range_plan work_plan;
        if (!prepare_review_paragraph_replacements(plan, work_plan,
                                                   replacements)) {
            (void)featherdoc::detail::set_xml_document_allocation_failure(
                this->last_error_info,
                "comment range ran out of memory before it could be published",
                document_xml_entry);
            return false;
        }
        for (const auto &replacement : replacements) {
            if (!remove_comment_markers_by_id(replacement.replacement,
                                              comment_id)) {
                discard_review_paragraph_replacements(replacements);
                replacements.clear();
                (void)featherdoc::detail::set_xml_document_allocation_failure(
                    this->last_error_info,
                    "comment range ran out of memory before it could be published",
                    document_xml_entry);
                return false;
            }
        }
        if (!checked_split_review_range_segments(work_plan.segments)) {
            discard_review_paragraph_replacements(replacements);
            replacements.clear();
            (void)featherdoc::detail::set_xml_document_allocation_failure(
                this->last_error_info,
                "comment range ran out of memory before it could be published",
                document_xml_entry);
            return false;
        }

        const auto selected_segments =
            collect_selected_paragraph_revision_spans(work_plan.segments);
        if (selected_segments.size() != work_plan.segments.size() ||
            !selected_paragraph_revision_spans_supported(selected_segments) ||
            !checked_insert_comment_markers_for_selected_spans(
                work_plan.segments, selected_segments, comment_id)) {
            discard_review_paragraph_replacements(replacements);
            replacements.clear();
            (void)featherdoc::detail::set_xml_document_allocation_failure(
                this->last_error_info,
                "comment range ran out of memory before it could be published",
                document_xml_entry);
            return false;
        }

        this->ensure_xml_handle_lifetime();
        std::vector<pugi::xml_node> retirement_roots;
        retirement_roots.reserve(replacements.size() +
                                 marker_removals.removal_runs.size());
        for (const auto &replacement : replacements) {
            retirement_roots.push_back(replacement.original);
        }
        retirement_roots.insert(retirement_roots.end(),
                                marker_removals.removal_runs.begin(),
                                marker_removals.removal_runs.end());
        if (!review_paragraph_replacements_are_publishable(replacements)) {
            discard_review_paragraph_replacements(replacements);
            replacements.clear();
            set_last_error(this->last_error_info,
                           document_errc::invalid_package_structure,
                           "comment paragraph list changed before publish",
                           std::string{document_xml_entry});
            return false;
        }

        this->xml_handle_lifetime->retire_subtrees(retirement_roots);
        if (!publish_review_node_removal_plan(marker_removals) ||
            !publish_review_paragraph_replacements(replacements)) {
            set_last_error(this->last_error_info,
                           document_errc::invalid_package_structure,
                           "comment range publish failed",
                           std::string{document_xml_entry});
            return false;
        }
        replacements.clear();
        this->rebind_internal_xml_handles();
        transaction.commit();
        this->last_error_info.clear();
        return true;
    } catch (...) {
        discard_review_paragraph_replacements(replacements);
        (void)featherdoc::detail::set_xml_document_allocation_failure(
            this->last_error_info,
            "comment range ran out of memory before it could be published",
            document_xml_entry);
        return false;
    }
}
