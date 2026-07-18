bool Document::insert_run_revision_after(std::size_t paragraph_index,
                                         std::size_t run_index,
                                         std::string_view text,
                                         std::string_view author,
                                         std::string_view date) {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
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

    try {
        auto paragraph_handle = this->paragraphs();
        for (std::size_t current_index = 0U;
             current_index < paragraph_index && paragraph_handle.has_next();
             ++current_index) {
            paragraph_handle.next();
        }
        auto paragraph_node = paragraph_handle.has_next()
                                  ? paragraph_handle.current
                                  : featherdoc::detail::tracked_xml_node{};
        auto paragraph_xml = paragraph_node.node();
        if (paragraph_xml == pugi::xml_node{}) {
            set_last_error(this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           "paragraph index is out of range",
                           std::string{document_xml_entry});
            return false;
        }
        if (paragraph_run_by_index(paragraph_xml, run_index) ==
            pugi::xml_node{}) {
            set_last_error(this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           "run index is out of range",
                           std::string{document_xml_entry});
            return false;
        }

        const auto next_id = this->reserve_review_revision_identifiers(1U);
        if (!next_id.has_value()) {
            return false;
        }
        const auto next_id_text = std::to_string(*next_id);

        if (!mutate_revision_body_document_transaction(
                this->document, *this->xml_handle_lifetime,
                this->last_error_info,
                "run insertion revision ran out of memory before it could be "
                "published",
                [&](pugi::xml_document &work_document) {
                    auto work_paragraph = revision_document_paragraph_by_index(
                        work_document, paragraph_index);
                    const auto work_anchor_run =
                        paragraph_run_by_index(work_paragraph, run_index);
                    if (work_paragraph == pugi::xml_node{} ||
                        work_anchor_run == pugi::xml_node{}) {
                        return false;
                    }
                    const auto next_sibling = work_anchor_run.next_sibling();
                    auto revision =
                        next_sibling == pugi::xml_node{}
                            ? featherdoc::detail::checked_append_xml_element(
                                  work_paragraph, "w:ins")
                            : featherdoc::detail::
                                  checked_insert_xml_element_before(
                                      work_paragraph, "w:ins", next_sibling);
                    return revision != pugi::xml_node{} &&
                           append_revision_run(revision, work_anchor_run, "w:t",
                                               text) != pugi::xml_node{} &&
                           set_revision_identity_metadata_text(
                               revision, next_id_text, author, date);
                })) {
            return false;
        }
        this->rebind_internal_xml_handles();
        this->last_error_info.clear();
        return true;
    } catch (...) {
        (void)featherdoc::detail::set_xml_document_allocation_failure(
            this->last_error_info,
            "run insertion revision ran out of memory before it could be "
            "published",
            document_xml_entry);
        return false;
    }
}

bool Document::delete_run_revision(std::size_t paragraph_index,
                                   std::size_t run_index,
                                   std::string_view author,
                                   std::string_view date) {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before deleting a run revision",
            std::string{document_xml_entry});
        return false;
    }

    try {
        auto paragraph_handle = this->paragraphs();
        for (std::size_t current_index = 0U;
             current_index < paragraph_index && paragraph_handle.has_next();
             ++current_index) {
            paragraph_handle.next();
        }
        auto paragraph_node = paragraph_handle.has_next()
                                  ? paragraph_handle.current
                                  : featherdoc::detail::tracked_xml_node{};
        auto paragraph_xml = paragraph_node.node();
        if (paragraph_xml == pugi::xml_node{}) {
            set_last_error(this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           "paragraph index is out of range",
                           std::string{document_xml_entry});
            return false;
        }
        auto target_run = paragraph_run_by_index(paragraph_xml, run_index);
        if (target_run == pugi::xml_node{}) {
            set_last_error(this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           "run index is out of range",
                           std::string{document_xml_entry});
            return false;
        }

        const auto original_text =
            featherdoc::detail::collect_plain_text_from_xml(target_run);
        if (original_text.empty()) {
            set_last_error(this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           "run revision deletion requires non-empty run text",
                           std::string{document_xml_entry});
            return false;
        }

        const auto next_id = this->reserve_review_revision_identifiers(1U);
        if (!next_id.has_value()) {
            return false;
        }
        const auto next_id_text = std::to_string(*next_id);

        if (!mutate_revision_body_document_transaction(
                this->document, *this->xml_handle_lifetime,
                this->last_error_info,
                "run deletion revision ran out of memory before it could be "
                "published",
                [&](pugi::xml_document &work_document) {
                    auto work_paragraph = revision_document_paragraph_by_index(
                        work_document, paragraph_index);
                    const auto work_target_run =
                        paragraph_run_by_index(work_paragraph, run_index);
                    if (work_paragraph == pugi::xml_node{} ||
                        work_target_run == pugi::xml_node{}) {
                        return false;
                    }
                    auto revision =
                        featherdoc::detail::checked_insert_xml_element_before(
                            work_paragraph, "w:del", work_target_run);
                    return revision != pugi::xml_node{} &&
                           append_revision_run(revision, work_target_run,
                                               "w:delText", original_text) !=
                               pugi::xml_node{} &&
                           set_revision_identity_metadata_text(
                               revision, next_id_text, author, date) &&
                           work_paragraph.remove_child(work_target_run);
                })) {
            return false;
        }
        this->rebind_internal_xml_handles();
        this->last_error_info.clear();
        return true;
    } catch (...) {
        (void)featherdoc::detail::set_xml_document_allocation_failure(
            this->last_error_info,
            "run deletion revision ran out of memory before it could be "
            "published",
            document_xml_entry);
        return false;
    }
}

bool Document::replace_run_revision(std::size_t paragraph_index,
                                    std::size_t run_index,
                                    std::string_view text,
                                    std::string_view author,
                                    std::string_view date) {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
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

    try {
        auto paragraph_handle = this->paragraphs();
        for (std::size_t current_index = 0U;
             current_index < paragraph_index && paragraph_handle.has_next();
             ++current_index) {
            paragraph_handle.next();
        }
        auto paragraph_node = paragraph_handle.has_next()
                                  ? paragraph_handle.current
                                  : featherdoc::detail::tracked_xml_node{};
        auto paragraph_xml = paragraph_node.node();
        if (paragraph_xml == pugi::xml_node{}) {
            set_last_error(this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           "paragraph index is out of range",
                           std::string{document_xml_entry});
            return false;
        }
        auto target_run = paragraph_run_by_index(paragraph_xml, run_index);
        if (target_run == pugi::xml_node{}) {
            set_last_error(this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           "run index is out of range",
                           std::string{document_xml_entry});
            return false;
        }

        const auto original_text =
            featherdoc::detail::collect_plain_text_from_xml(target_run);
        if (original_text.empty()) {
            set_last_error(
                this->last_error_info,
                std::make_error_code(std::errc::invalid_argument),
                "run revision replacement requires non-empty run text",
                std::string{document_xml_entry});
            return false;
        }

        const auto revision_id = this->reserve_review_revision_identifiers(2U);
        if (!revision_id.has_value()) {
            return false;
        }
        const auto deletion_id_text = std::to_string(*revision_id);
        const auto insertion_id_text = std::to_string(*revision_id + 1);

        if (!mutate_revision_body_document_transaction(
                this->document, *this->xml_handle_lifetime,
                this->last_error_info,
                "run replacement revision ran out of memory before it could be "
                "published",
                [&](pugi::xml_document &work_document) {
                    auto work_paragraph = revision_document_paragraph_by_index(
                        work_document, paragraph_index);
                    const auto work_target_run =
                        paragraph_run_by_index(work_paragraph, run_index);
                    if (work_paragraph == pugi::xml_node{} ||
                        work_target_run == pugi::xml_node{}) {
                        return false;
                    }
                    auto deletion =
                        featherdoc::detail::checked_insert_xml_element_before(
                            work_paragraph, "w:del", work_target_run);
                    auto insertion =
                        deletion == pugi::xml_node{}
                            ? pugi::xml_node{}
                            : featherdoc::detail::
                                  checked_insert_xml_element_before(
                                      work_paragraph, "w:ins", work_target_run);
                    return deletion != pugi::xml_node{} &&
                           insertion != pugi::xml_node{} &&
                           append_revision_run(deletion, work_target_run,
                                               "w:delText", original_text) !=
                               pugi::xml_node{} &&
                           append_revision_run(insertion, work_target_run,
                                               "w:t",
                                               text) != pugi::xml_node{} &&
                           set_revision_identity_metadata_text(
                               deletion, deletion_id_text, author, date) &&
                           set_revision_identity_metadata_text(
                               insertion, insertion_id_text, author, date) &&
                           work_paragraph.remove_child(work_target_run);
                })) {
            return false;
        }
        this->rebind_internal_xml_handles();
        this->last_error_info.clear();
        return true;
    } catch (...) {
        (void)featherdoc::detail::set_xml_document_allocation_failure(
            this->last_error_info,
            "run replacement revision ran out of memory before it could be "
            "published",
            document_xml_entry);
        return false;
    }
}
