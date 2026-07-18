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

    if (template_part_block_container(this->document) == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "document is not ready for revision insertion",
                       std::string{document_xml_entry});
        return 0U;
    }

    try {
        const auto next_id = this->reserve_review_revision_identifiers(1U);
        if (!next_id.has_value()) {
            return 0U;
        }
        const auto next_id_text = std::to_string(*next_id);

        if (!mutate_revision_body_document_transaction(
                this->document, *this->xml_handle_lifetime,
                this->last_error_info,
                "insertion revision ran out of memory before it could be "
                "published",
                [&](pugi::xml_document &work_document) {
                    const auto work_body =
                        template_part_block_container(work_document);
                    auto paragraph_node =
                        featherdoc::detail::append_paragraph_node(work_body);
                    auto revision =
                        featherdoc::detail::checked_append_xml_element(
                            paragraph_node, "w:ins");
                    auto run = featherdoc::detail::checked_append_xml_element(
                        revision, "w:r");
                    return paragraph_node != pugi::xml_node{} &&
                           revision != pugi::xml_node{} &&
                           run != pugi::xml_node{} &&
                           set_revision_run_text_content(run, "w:t", text) &&
                           set_revision_identity_metadata_text(
                               revision, next_id_text, author, date);
                })) {
            return 0U;
        }

        this->rebind_internal_xml_handles();
        this->last_error_info.clear();
        return 1U;
    } catch (...) {
        (void)featherdoc::detail::set_xml_document_allocation_failure(
            this->last_error_info,
            "insertion revision ran out of memory before it could be published",
            document_xml_entry);
        return 0U;
    }
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

    if (template_part_block_container(this->document) == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "document is not ready for revision insertion",
                       std::string{document_xml_entry});
        return 0U;
    }

    try {
        const auto next_id = this->reserve_review_revision_identifiers(1U);
        if (!next_id.has_value()) {
            return 0U;
        }
        const auto next_id_text = std::to_string(*next_id);

        if (!mutate_revision_body_document_transaction(
                this->document, *this->xml_handle_lifetime,
                this->last_error_info,
                "deletion revision ran out of memory before it could be "
                "published",
                [&](pugi::xml_document &work_document) {
                    const auto work_body =
                        template_part_block_container(work_document);
                    auto paragraph_node =
                        featherdoc::detail::append_paragraph_node(work_body);
                    auto revision =
                        featherdoc::detail::checked_append_xml_element(
                            paragraph_node, "w:del");
                    auto run = featherdoc::detail::checked_append_xml_element(
                        revision, "w:r");
                    return paragraph_node != pugi::xml_node{} &&
                           revision != pugi::xml_node{} &&
                           run != pugi::xml_node{} &&
                           set_revision_run_text_content(run, "w:delText",
                                                         text) &&
                           set_revision_identity_metadata_text(
                               revision, next_id_text, author, date);
                })) {
            return 0U;
        }

        this->rebind_internal_xml_handles();
        this->last_error_info.clear();
        return 1U;
    } catch (...) {
        (void)featherdoc::detail::set_xml_document_allocation_failure(
            this->last_error_info,
            "deletion revision ran out of memory before it could be published",
            document_xml_entry);
        return 0U;
    }
}
