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
                       this->extension_state().singleton_part_entries.comments);
        return 0U;
    }
    try {
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
            return 0U;
        }
        if (!this->ensure_comments_part()) {
            return 0U;
        }
        review_node_rollback_guard paragraph_rollback;

    auto root = this->comments.child("w:comments");
    auto body = template_part_block_container(this->document);
    if (root == pugi::xml_node{} || body == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "document is not ready for comment insertion",
                       std::string{document_xml_entry});
        return 0U;
    }

    const auto comment_id = next_numeric_id(root, "w:comment", 0);
    if (!comment_id.has_value()) {
        set_last_error(this->last_error_info,
                       document_errc::identifier_space_exhausted,
                       "comment identifier space is exhausted",
                       this->extension_state().singleton_part_entries.comments);
        return 0U;
    }
    const auto comment_id_text = std::to_string(*comment_id);
    auto comment = checked_append_comment(root, comment_id_text, comment_text,
                                          author, initials, date);
    if (comment == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to prepare comment XML",
                       this->extension_state().singleton_part_entries.comments);
        return 0U;
    }

    auto paragraph_node = checked_append_review_body_paragraph(body);
    paragraph_rollback.track(body, paragraph_node);
    auto range_start = paragraph_node == pugi::xml_node{}
                           ? pugi::xml_node{}
                           : featherdoc::detail::checked_append_xml_element(
                                 paragraph_node, "w:commentRangeStart");
    auto selected_run = range_start == pugi::xml_node{}
                            ? pugi::xml_node{}
                            : checked_append_review_plain_text_run(
                                  paragraph_node, selected_text);
    auto range_end = selected_run == pugi::xml_node{}
                         ? pugi::xml_node{}
                         : featherdoc::detail::checked_append_xml_element(
                               paragraph_node, "w:commentRangeEnd");
    auto reference_run =
        range_end == pugi::xml_node{}
            ? pugi::xml_node{}
            : featherdoc::detail::checked_append_xml_element(paragraph_node,
                                                              "w:r");
    auto reference = reference_run == pugi::xml_node{}
                         ? pugi::xml_node{}
                         : featherdoc::detail::checked_append_xml_element(
                               reference_run, "w:commentReference");
    if (reference == pugi::xml_node{} ||
        !ensure_attribute_value(range_start, "w:id", comment_id_text) ||
        !ensure_attribute_value(range_end, "w:id", comment_id_text) ||
        !ensure_attribute_value(reference, "w:id", comment_id_text)) {
        (void)root.remove_child(comment);
        if (paragraph_node != pugi::xml_node{}) {
            (void)body.remove_child(paragraph_node);
        }
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to prepare comment range",
                       std::string{document_xml_entry});
        return 0U;
    }

        this->comments_dirty = true;
        paragraph_rollback.commit();
        transaction.commit();
        this->last_error_info.clear();
        return 1U;
    } catch (...) {
        (void)featherdoc::detail::set_xml_document_allocation_failure(
            this->last_error_info,
            "comment append ran out of memory before it could be published",
            this->extension_state().singleton_part_entries.comments);
        return 0U;
    }
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
                       this->extension_state().singleton_part_entries.comments);
        return 0U;
    }
    if (end_paragraph_index < start_paragraph_index) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "text range start must not be after end",
                       std::string{document_xml_entry});
        return 0U;
    }

    auto replacements = std::vector<review_paragraph_replacement>{};
    auto comment = pugi::xml_node{};
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
            return 0U;
        }
        if (!paragraph_revision_range_segments_supported(plan.segments)) {
            set_last_error(
                this->last_error_info,
                std::make_error_code(std::errc::operation_not_supported),
                "text range comment requires plain text runs",
                std::string{document_xml_entry});
            return 0U;
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
            return 0U;
        }
        if (!this->ensure_comments_part()) {
            return 0U;
        }

        auto root = this->comments.child("w:comments");
        if (root == pugi::xml_node{}) {
            set_last_error(
                this->last_error_info,
                std::make_error_code(std::errc::invalid_argument),
                "document is not ready for comment insertion",
                this->extension_state().singleton_part_entries.comments);
            return 0U;
        }

        const auto comment_id = next_numeric_id(root, "w:comment", 0);
        if (!comment_id.has_value()) {
            set_last_error(
                this->last_error_info,
                document_errc::identifier_space_exhausted,
                "comment identifier space is exhausted",
                this->extension_state().singleton_part_entries.comments);
            return 0U;
        }
        const auto comment_id_text = std::to_string(*comment_id);

        paragraph_revision_range_plan work_plan;
        if (!prepare_review_paragraph_replacements(plan, work_plan,
                                                   replacements) ||
            !checked_split_review_range_segments(work_plan.segments)) {
            discard_review_paragraph_replacements(replacements);
            replacements.clear();
            (void)featherdoc::detail::set_xml_document_allocation_failure(
                this->last_error_info,
                "text range comment ran out of memory before it could be published",
                document_xml_entry);
            return 0U;
        }

        const auto selected_segments =
            collect_selected_paragraph_revision_spans(work_plan.segments);
        if (selected_segments.size() != work_plan.segments.size() ||
            !selected_paragraph_revision_spans_supported(selected_segments) ||
            !checked_insert_comment_markers_for_selected_spans(
                work_plan.segments, selected_segments, comment_id_text)) {
            discard_review_paragraph_replacements(replacements);
            replacements.clear();
            (void)featherdoc::detail::set_xml_document_allocation_failure(
                this->last_error_info,
                "text range comment ran out of memory before it could be published",
                document_xml_entry);
            return 0U;
        }

        comment = checked_append_comment(root, comment_id_text, comment_text,
                                         author, initials, date);
        if (comment == pugi::xml_node{}) {
            discard_review_paragraph_replacements(replacements);
            replacements.clear();
            (void)featherdoc::detail::set_xml_document_allocation_failure(
                this->last_error_info,
                "comment body ran out of memory before it could be published",
                this->extension_state().singleton_part_entries.comments);
            return 0U;
        }

        this->ensure_xml_handle_lifetime();
        std::vector<pugi::xml_node> retirement_roots;
        retirement_roots.reserve(replacements.size());
        for (const auto &replacement : replacements) {
            retirement_roots.push_back(replacement.original);
        }
        if (!review_paragraph_replacements_are_publishable(replacements)) {
            (void)root.remove_child(comment);
            comment = {};
            discard_review_paragraph_replacements(replacements);
            replacements.clear();
            set_last_error(this->last_error_info,
                           document_errc::invalid_package_structure,
                           "comment paragraph list changed before publish",
                           std::string{document_xml_entry});
            return 0U;
        }

        this->xml_handle_lifetime->retire_subtrees(retirement_roots);
        if (!publish_review_paragraph_replacements(replacements)) {
            set_last_error(this->last_error_info,
                           document_errc::invalid_package_structure,
                           "comment paragraph publish failed",
                           std::string{document_xml_entry});
            return 0U;
        }
        replacements.clear();
        comment = {};
        this->comments_dirty = true;
        this->rebind_internal_xml_handles();
        transaction.commit();
        this->last_error_info.clear();
        return 1U;
    } catch (...) {
        discard_review_paragraph_replacements(replacements);
        (void)featherdoc::detail::set_xml_document_allocation_failure(
            this->last_error_info,
            "text range comment ran out of memory before it could be published",
            document_xml_entry);
        return 0U;
    }
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
                       this->extension_state().singleton_part_entries.comments);
        return 0U;
    }
    if (!this->ensure_comments_part(false)) {
        return 0U;
    }

    auto comments_root = this->comments.child("w:comments");
    auto parent_comment =
        comment_by_summary_index(comments_root, parent_comment_index);
    if (parent_comment == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "parent comment index is out of range",
                       this->extension_state().singleton_part_entries.comments);
        return 0U;
    }
    if (!comment_paragraph_id_can_be_ensured(parent_comment)) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "parent comment body does not contain a paragraph id target",
                       this->extension_state().singleton_part_entries.comments);
        return 0U;
    }
    const auto comment_id = next_numeric_id(comments_root, "w:comment", 0);
    if (!comment_id.has_value()) {
        set_last_error(this->last_error_info,
                       document_errc::identifier_space_exhausted,
                       "comment identifier space is exhausted",
                       this->extension_state().singleton_part_entries.comments);
        return 0U;
    }
    const auto comment_id_text = std::to_string(*comment_id);
    try {
        review_package_transaction transaction(
            {&this->document_relationships, &this->content_types,
             &this->comments, &this->comments_extended},
            {&this->has_document_relationships_part,
             &this->document_relationships_dirty,
             &this->content_types_loaded, &this->content_types_dirty,
             &this->has_comments_part, &this->comments_loaded,
             &this->comments_dirty, &this->has_comments_extended_part,
             &this->comments_extended_loaded,
             &this->comments_extended_dirty},
            {&this->extension_state().singleton_part_entries.comments,
             &this->extension_state().singleton_part_entries.comments_extended});
        if (!transaction.ready()) {
            (void)featherdoc::detail::set_xml_document_clone_allocation_failure(
                this->last_error_info,
                this->extension_state().singleton_part_entries.comments);
            return 0U;
        }
        if (!this->ensure_comments_part()) {
            return 0U;
        }

        const auto parent_para_id = ensure_comment_paragraph_id(
            comments_root, parent_comment, this->comments_dirty);
        if (!parent_para_id.has_value()) {
            set_last_error(
                this->last_error_info,
                std::make_error_code(std::errc::not_enough_memory),
                "failed to prepare parent comment paragraph metadata",
                this->extension_state().singleton_part_entries.comments);
            return 0U;
        }
        if (!this->ensure_comments_extended_part()) {
            return 0U;
        }

        auto extended_root = this->comments_extended.child("w15:commentsEx");
        auto parent_comment_ex = ensure_comment_extended_by_paragraph_id(
            extended_root, *parent_para_id);
        if (parent_comment_ex == pugi::xml_node{}) {
            set_last_error(
                this->last_error_info,
                std::make_error_code(std::errc::not_enough_memory),
                "failed to append parent comment metadata",
                this->extension_state().singleton_part_entries.comments_extended);
            return 0U;
        }

        auto reply = checked_append_comment(comments_root, comment_id_text,
                                            comment_text, author, initials,
                                            date);
        if (reply == pugi::xml_node{}) {
            set_last_error(this->last_error_info,
                           std::make_error_code(std::errc::not_enough_memory),
                           "failed to prepare comment reply XML",
                           this->extension_state().singleton_part_entries.comments);
            return 0U;
        }

        const auto reply_para_id = ensure_comment_paragraph_id(
            comments_root, reply, this->comments_dirty);
        if (!reply_para_id.has_value()) {
            set_last_error(this->last_error_info,
                           std::make_error_code(std::errc::not_enough_memory),
                           "failed to prepare comment reply paragraph metadata",
                           this->extension_state().singleton_part_entries.comments);
            return 0U;
        }

        auto reply_comment_ex = ensure_comment_extended_by_paragraph_id(
            extended_root, *reply_para_id);
        if (reply_comment_ex == pugi::xml_node{} ||
            !ensure_attribute_value(reply_comment_ex, "w15:paraIdParent",
                                    *parent_para_id)) {
            set_last_error(
                this->last_error_info,
                std::make_error_code(std::errc::not_enough_memory),
                "failed to append comment reply metadata",
                this->extension_state().singleton_part_entries.comments_extended);
            return 0U;
        }

        this->comments_dirty = true;
        this->comments_extended_dirty = true;
        transaction.commit();
        this->last_error_info.clear();
        return 1U;
    } catch (...) {
        (void)featherdoc::detail::set_xml_document_allocation_failure(
            this->last_error_info,
            "comment reply ran out of memory before it could be published",
            this->extension_state().singleton_part_entries.comments);
        return 0U;
    }
}
