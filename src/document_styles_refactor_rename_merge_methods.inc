bool Document::mutate_style_identities_transaction(
    const std::vector<featherdoc::style_refactor_request> &operations) try {
    if (operations.empty()) {
        this->last_error_info.clear();
        return true;
    }
    if (this->ignored_singleton_relationship_type(styles_relationship_type)) {
        set_last_error(
            this->last_error_info, document_errc::invalid_package_structure,
            "cannot attach styles while its source relationship is invalid",
            std::string{document_relationships_xml_entry});
        return false;
    }
    if (!featherdoc::detail::package_relationships_document_allows_mutation(
            this->document_relationships,
            this->has_document_relationships_part)) {
        set_last_error(
            this->last_error_info, document_errc::invalid_package_structure,
            "cannot attach styles while document relationships are invalid",
            std::string{document_relationships_xml_entry});
        return false;
    }
    if (const auto error = this->ensure_content_types_loaded()) {
        return false;
    }

    auto working_styles = pugi::xml_document{};
    auto working_document = pugi::xml_document{};
    auto working_header_parts =
        std::vector<std::unique_ptr<pugi::xml_document>>{};
    auto working_footer_parts =
        std::vector<std::unique_ptr<pugi::xml_document>>{};

    const auto clone_document = [&](const pugi::xml_document &source,
                                    pugi::xml_document &destination,
                                    std::string_view entry_name) -> bool {
        if (detail::checked_clone_xml_document(source, destination) ==
            detail::xml_document_clone_status::success) {
            return true;
        }
        (void)detail::set_xml_document_clone_allocation_failure(
            this->last_error_info, entry_name);
        return false;
    };
    if (!clone_document(
            this->styles, working_styles,
            this->extension_state().singleton_part_entries.styles) ||
        !clone_document(this->document, working_document, document_xml_entry)) {
        return false;
    }

    const auto clone_related_parts = [&](const auto &source_parts,
                                         auto &working_parts) -> bool {
        working_parts.reserve(source_parts.size());
        for (const auto &source_part : source_parts) {
            if (!source_part) {
                working_parts.push_back(nullptr);
                continue;
            }
            auto working_part = std::make_unique<pugi::xml_document>();
            if (!clone_document(source_part->xml, *working_part,
                                source_part->entry_name)) {
                return false;
            }
            working_parts.push_back(std::move(working_part));
        }
        return true;
    };
    if (!clone_related_parts(this->header_parts, working_header_parts) ||
        !clone_related_parts(this->footer_parts, working_footer_parts)) {
        return false;
    }

    auto relationship_target = std::string{};
    try {
        relationship_target = make_document_relationship_target(
            this->extension_state().singleton_part_entries.styles);
    } catch (const std::bad_alloc &) {
        (void)detail::set_xml_document_mutation_allocation_failure(
            this->last_error_info, document_relationships_xml_entry);
        return false;
    }
    auto override_part_name = std::string{};
    try {
        override_part_name = make_override_part_name(
            this->extension_state().singleton_part_entries.styles);
    } catch (const std::bad_alloc &) {
        (void)detail::set_xml_document_mutation_allocation_failure(
            this->last_error_info, content_types_xml_entry);
        return false;
    }
    auto attachment_options = detail::singleton_part_attachment_options{};
    attachment_options.relationship_type = styles_relationship_type;
    attachment_options.relationship_target = relationship_target;
    attachment_options.override_part_name = override_part_name;
    attachment_options.content_type = styles_content_type;
    attachment_options.relationships_entry_name =
        document_relationships_xml_entry;
    attachment_options.content_types_entry_name = content_types_xml_entry;
    auto attachment_work = detail::singleton_part_attachment_work{};
    if (detail::prepare_checked_singleton_part_attachment(
            this->document_relationships, this->content_types,
            attachment_options, this->last_error_info, attachment_work)) {
        return false;
    }

    auto working_styles_root = working_styles.child("w:styles");
    auto working_document_root = working_document.child("w:document");
    if (working_styles_root == pugi::xml_node{} ||
        working_document_root == pugi::xml_node{}) {
        set_last_error(
            this->last_error_info, document_errc::invalid_package_structure,
            "style mutation work documents do not match the validated source",
            this->extension_state().singleton_part_entries.styles);
        return false;
    }

    const auto fail_rewrite_allocation =
        [&](std::string_view entry_name) -> bool {
        (void)detail::set_xml_document_mutation_allocation_failure(
            this->last_error_info, entry_name);
        return false;
    };

    for (const auto &operation : operations) {
        const auto old_style_id = std::string_view{operation.source_style_id};
        const auto new_style_id = std::string_view{operation.target_style_id};
        auto working_old_style =
            find_style_node(working_styles_root, old_style_id);
        if (working_old_style == pugi::xml_node{}) {
            set_last_error(
                this->last_error_info, document_errc::invalid_package_structure,
                "style mutation source id '" + operation.source_style_id +
                    "' disappeared from the sequential work copy",
                this->extension_state().singleton_part_entries.styles);
            return false;
        }

        const auto working_target_style =
            find_style_node(working_styles_root, new_style_id);
        const auto remove_old_style =
            operation.action == featherdoc::style_refactor_action::merge;
        if ((!remove_old_style && working_target_style != pugi::xml_node{}) ||
            (remove_old_style && working_target_style == pugi::xml_node{})) {
            set_last_error(
                this->last_error_info, document_errc::invalid_package_structure,
                "style mutation target id '" + operation.target_style_id +
                    "' no longer matches the validated sequential operation",
                this->extension_state().singleton_part_entries.styles);
            return false;
        }

        if (!remove_old_style) {
            const auto style_id_status = rewrite_style_reference_value_checked(
                working_old_style.attribute("w:styleId"), old_style_id,
                new_style_id);
            if (style_id_status ==
                style_reference_rewrite_status::allocation_failure) {
                return fail_rewrite_allocation(
                    this->extension_state().singleton_part_entries.styles);
            }
            if (style_id_status != style_reference_rewrite_status::rewritten) {
                set_last_error(
                    this->last_error_info,
                    document_errc::invalid_package_structure,
                    "validated style id disappeared from the mutation work "
                    "copy",
                    this->extension_state().singleton_part_entries.styles);
                return false;
            }
        }

        if (rewrite_style_metadata_references_in_styles_checked(
                working_styles_root, old_style_id, new_style_id) ==
            style_reference_rewrite_status::allocation_failure) {
            return fail_rewrite_allocation(
                this->extension_state().singleton_part_entries.styles);
        }
        if (rewrite_style_references_in_tree_checked(
                working_document_root, old_style_id, new_style_id) ==
            style_reference_rewrite_status::allocation_failure) {
            return fail_rewrite_allocation(document_xml_entry);
        }
        for (std::size_t index = 0U; index < this->header_parts.size();
             ++index) {
            if (this->header_parts[index] &&
                rewrite_style_references_in_tree_checked(
                    working_header_parts[index]->child("w:hdr"), old_style_id,
                    new_style_id) ==
                    style_reference_rewrite_status::allocation_failure) {
                return fail_rewrite_allocation(
                    this->header_parts[index]->entry_name);
            }
        }
        for (std::size_t index = 0U; index < this->footer_parts.size();
             ++index) {
            if (this->footer_parts[index] &&
                rewrite_style_references_in_tree_checked(
                    working_footer_parts[index]->child("w:ftr"), old_style_id,
                    new_style_id) ==
                    style_reference_rewrite_status::allocation_failure) {
                return fail_rewrite_allocation(
                    this->footer_parts[index]->entry_name);
            }
        }

        if (remove_old_style &&
            !working_styles_root.remove_child(working_old_style)) {
            set_last_error(
                this->last_error_info,
                std::make_error_code(std::errc::invalid_argument),
                "failed to remove source style id '" +
                    operation.source_style_id + "' from '" +
                    this->extension_state().singleton_part_entries.styles + "'",
                this->extension_state().singleton_part_entries.styles);
            return false;
        }
    }

    this->invalidate_xml_handles();
    this->styles = std::move(working_styles);
    this->document = std::move(working_document);
    for (std::size_t index = 0U; index < this->header_parts.size(); ++index) {
        if (this->header_parts[index] && working_header_parts[index]) {
            this->header_parts[index]->xml =
                std::move(*working_header_parts[index]);
        }
    }
    for (std::size_t index = 0U; index < this->footer_parts.size(); ++index) {
        if (this->footer_parts[index] && working_footer_parts[index]) {
            this->footer_parts[index]->xml =
                std::move(*working_footer_parts[index]);
        }
    }
    detail::publish_checked_singleton_part_attachment(
        std::move(attachment_work), this->document_relationships,
        this->content_types, this->has_document_relationships_part,
        this->document_relationships_dirty, this->content_types_dirty);
    this->has_styles_part = true;
    this->styles_dirty = true;
    this->rebind_internal_xml_handles();
    this->last_error_info.clear();
    return true;
} catch (const std::bad_alloc &) {
    (void)detail::set_xml_document_mutation_allocation_failure(
        this->last_error_info, std::string_view{"word/styles.xml"});
    return false;
}

bool Document::mutate_style_identity_transaction(std::string_view old_style_id,
                                                 std::string_view new_style_id,
                                                 bool remove_old_style) try {
    auto operation = featherdoc::style_refactor_request{};
    operation.action = remove_old_style
                           ? featherdoc::style_refactor_action::merge
                           : featherdoc::style_refactor_action::rename;
    operation.source_style_id.assign(old_style_id);
    operation.target_style_id.assign(new_style_id);
    auto operations = std::vector<featherdoc::style_refactor_request>{};
    operations.push_back(std::move(operation));
    return this->mutate_style_identities_transaction(operations);
} catch (const std::bad_alloc &) {
    (void)detail::set_xml_document_mutation_allocation_failure(
        this->last_error_info, std::string_view{"word/styles.xml"});
    return false;
}

bool Document::rename_style(std::string_view old_style_id,
                            std::string_view new_style_id) try {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before renaming styles",
                       this->extension_state().singleton_part_entries.styles);
        return false;
    }

    if (old_style_id.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "old style id must not be empty when renaming styles",
                       this->extension_state().singleton_part_entries.styles);
        return false;
    }

    if (new_style_id.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "new style id must not be empty when renaming styles",
                       this->extension_state().singleton_part_entries.styles);
        return false;
    }

    if (const auto error = this->ensure_styles_loaded()) {
        return false;
    }

    const auto styles_root = this->styles.child("w:styles");
    if (styles_root == pugi::xml_node{}) {
        set_last_error(
            this->last_error_info, document_errc::styles_xml_parse_failed,
            "'" + this->extension_state().singleton_part_entries.styles +
                "' does not contain a w:styles root",
            this->extension_state().singleton_part_entries.styles);
        return false;
    }

    const auto style = find_style_node(styles_root, old_style_id);
    if (style == pugi::xml_node{}) {
        set_last_error(
            this->last_error_info,
            std::make_error_code(std::errc::invalid_argument),
            "style id '" + std::string{old_style_id} + "' was not found in '" +
                this->extension_state().singleton_part_entries.styles + "'",
            this->extension_state().singleton_part_entries.styles);
        return false;
    }

    if (find_style_node(styles_root, new_style_id) != pugi::xml_node{}) {
        set_last_error(
            this->last_error_info,
            std::make_error_code(std::errc::invalid_argument),
            "target style id '" + std::string{new_style_id} +
                "' already exists in '" +
                this->extension_state().singleton_part_entries.styles + "'",
            this->extension_state().singleton_part_entries.styles);
        return false;
    }

    if (read_on_off_attribute(style, "w:default")) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "default style id '" + std::string{old_style_id} +
                           "' cannot be renamed",
                       this->extension_state().singleton_part_entries.styles);
        return false;
    }

    const auto document_root = this->document.child("w:document");
    if (document_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       document_errc::document_xml_parse_failed,
                       "word/document.xml does not contain a w:document root",
                       std::string{document_xml_entry});
        return false;
    }

    return this->mutate_style_identity_transaction(old_style_id, new_style_id,
                                                   false);
} catch (const std::bad_alloc &) {
    (void)detail::set_xml_document_mutation_allocation_failure(
        this->last_error_info, std::string_view{"word/styles.xml"});
    return false;
}

bool Document::merge_style(std::string_view source_style_id,
                           std::string_view target_style_id) try {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before merging styles",
                       this->extension_state().singleton_part_entries.styles);
        return false;
    }

    if (source_style_id.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "source style id must not be empty when merging styles",
                       this->extension_state().singleton_part_entries.styles);
        return false;
    }

    if (target_style_id.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "target style id must not be empty when merging styles",
                       this->extension_state().singleton_part_entries.styles);
        return false;
    }

    if (source_style_id == target_style_id) {
        set_last_error(
            this->last_error_info,
            std::make_error_code(std::errc::invalid_argument),
            "source and target style ids must be different when merging styles",
            this->extension_state().singleton_part_entries.styles);
        return false;
    }

    if (const auto error = this->ensure_styles_loaded()) {
        return false;
    }

    auto styles_root = this->styles.child("w:styles");
    if (styles_root == pugi::xml_node{}) {
        set_last_error(
            this->last_error_info, document_errc::styles_xml_parse_failed,
            "'" + this->extension_state().singleton_part_entries.styles +
                "' does not contain a w:styles root",
            this->extension_state().singleton_part_entries.styles);
        return false;
    }

    const auto source_style = find_style_node(styles_root, source_style_id);
    if (source_style == pugi::xml_node{}) {
        set_last_error(
            this->last_error_info,
            std::make_error_code(std::errc::invalid_argument),
            "source style id '" + std::string{source_style_id} +
                "' was not found in '" +
                this->extension_state().singleton_part_entries.styles + "'",
            this->extension_state().singleton_part_entries.styles);
        return false;
    }

    const auto target_style = find_style_node(styles_root, target_style_id);
    if (target_style == pugi::xml_node{}) {
        set_last_error(
            this->last_error_info,
            std::make_error_code(std::errc::invalid_argument),
            "target style id '" + std::string{target_style_id} +
                "' was not found in '" +
                this->extension_state().singleton_part_entries.styles + "'",
            this->extension_state().singleton_part_entries.styles);
        return false;
    }

    if (read_on_off_attribute(source_style, "w:default")) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "default style id '" + std::string{source_style_id} +
                           "' cannot be merged",
                       this->extension_state().singleton_part_entries.styles);
        return false;
    }

    const auto source_summary = summarize_style_node(source_style);
    const auto target_summary = summarize_style_node(target_style);
    if (source_summary.kind != target_summary.kind ||
        source_summary.type_name != target_summary.type_name) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "source style id '" + std::string{source_style_id} +
                           "' has type '" + source_summary.type_name +
                           "' but target style id '" +
                           std::string{target_style_id} + "' has type '" +
                           target_summary.type_name + "'",
                       this->extension_state().singleton_part_entries.styles);
        return false;
    }

    const auto document_root = this->document.child("w:document");
    if (document_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       document_errc::document_xml_parse_failed,
                       "word/document.xml does not contain a w:document root",
                       std::string{document_xml_entry});
        return false;
    }

    return this->mutate_style_identity_transaction(source_style_id,
                                                   target_style_id, true);
} catch (const std::bad_alloc &) {
    (void)detail::set_xml_document_mutation_allocation_failure(
        this->last_error_info, std::string_view{"word/styles.xml"});
    return false;
}
