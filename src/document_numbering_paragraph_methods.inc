bool Document::set_paragraph_numbering(Paragraph target_paragraph,
                                       std::uint32_t numbering_definition_id,
                                       std::uint32_t level) try {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before editing paragraph numbering");
        return false;
    }

    auto target_entry_name = std::string_view{};
    if (!this->owns_paragraph_handle(target_paragraph, target_entry_name)) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "target paragraph handle is not valid or does not belong to this document",
                       std::string{document_xml_entry});
        return false;
    }

    if (level > max_list_level) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "numbering level must be in the range [0, 8]",
                       std::string{target_entry_name});
        return false;
    }

    if (const auto error = this->ensure_numbering_loaded()) {
        return false;
    }

    auto numbering_root = this->numbering.child("w:numbering");
    if (numbering_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       document_errc::numbering_xml_parse_failed,
                       "'" + this->extension_state().singleton_part_entries.numbering + "' does not contain the expected w:numbering root",
                       this->extension_state().singleton_part_entries.numbering);
        return false;
    }

    const auto abstract_num =
        find_abstract_numbering_by_id(numbering_root, numbering_definition_id);
    if (abstract_num == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "numbering definition id does not exist",
                       this->extension_state().singleton_part_entries.numbering);
        return false;
    }

    if (find_level_definition(abstract_num, level) == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "requested numbering level is not defined by the target definition",
                       this->extension_state().singleton_part_entries.numbering);
        return false;
    }

    const auto instance_plan = plan_numbering_instance_for_abstract(
        numbering_root, numbering_definition_id);
    if (!instance_plan.succeeded()) {
        set_numbering_mutation_error(
            this->last_error_info, this->extension_state().singleton_part_entries.numbering, instance_plan.status,
            "numbering instance id has reached UINT32_MAX",
            "failed to reserve a numbering instance for the target definition");
        return false;
    }

    if (this->ignored_singleton_relationship_type(
            numbering_relationship_type)) {
        set_last_error(
            this->last_error_info, document_errc::invalid_package_structure,
            "cannot attach numbering while its source relationship is invalid",
            std::string{document_relationships_xml_entry});
        return false;
    }
    if (!featherdoc::detail::package_relationships_document_allows_mutation(
            this->document_relationships,
            this->has_document_relationships_part)) {
        set_last_error(
            this->last_error_info, document_errc::invalid_package_structure,
            "cannot attach numbering while document relationships are invalid",
            std::string{document_relationships_xml_entry});
        return false;
    }
    if (const auto error = this->ensure_content_types_loaded()) {
        return false;
    }

    auto working_numbering = pugi::xml_document{};
    if (featherdoc::detail::checked_clone_xml_document(
            this->numbering, working_numbering) !=
        featherdoc::detail::xml_document_clone_status::success) {
        (void)featherdoc::detail::set_xml_document_clone_allocation_failure(
            this->last_error_info,
            this->extension_state().singleton_part_entries.numbering);
        return false;
    }
    numbering_root = working_numbering.child("w:numbering");

    const auto num_id = ensure_numbering_instance_for_abstract(
        numbering_root, numbering_definition_id, std::nullopt, false,
        instance_plan.plan->reserved_num_id);
    if (!num_id.succeeded()) {
        set_numbering_mutation_error(
            this->last_error_info, this->extension_state().singleton_part_entries.numbering, num_id.status,
            "numbering instance id has reached UINT32_MAX",
            "failed to create or reuse a numbering instance for the target definition");
        return false;
    }

    auto numbering_attachment = singleton_part_attachment_option_storage{};
    numbering_attachment.initialize(
        numbering_relationship_type,
        this->extension_state().singleton_part_entries.numbering,
        numbering_content_type);
    const auto attachment_options =
        std::array{numbering_attachment.options};
    auto attachment_work =
        featherdoc::detail::singleton_part_attachment_work{};
    if (const auto error =
            featherdoc::detail::prepare_checked_singleton_part_attachments(
                this->document_relationships, this->content_types,
                attachment_options, this->last_error_info, attachment_work)) {
        return false;
    }

    auto paragraph_transaction = paragraph_properties_transaction{};
    if (!paragraph_transaction.prepare(target_paragraph.current) ||
        !apply_numbering_to_paragraph_properties(
            paragraph_transaction.working_properties(), level,
            *num_id.identifier) ||
        !paragraph_transaction.commit()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to populate numbering metadata for the target paragraph",
                       std::string{target_entry_name});
        return false;
    }

    static_assert(std::is_nothrow_move_assignable_v<pugi::xml_document>);
    this->numbering = std::move(working_numbering);
    featherdoc::detail::publish_checked_singleton_part_attachment(
        std::move(attachment_work), this->document_relationships,
        this->content_types, this->has_document_relationships_part,
        this->document_relationships_dirty, this->content_types_dirty);
    this->has_numbering_part = true;
    this->numbering_dirty = true;
    this->last_error_info.clear();
    return true;
} catch (const std::bad_alloc &) {
    (void)featherdoc::detail::set_xml_document_mutation_allocation_failure(
        this->last_error_info,
        this->extension_state().singleton_part_entries.numbering);
    return false;
}

bool Document::set_paragraph_style_numbering(std::string_view style_id,
                                             std::uint32_t numbering_definition_id,
                                             std::uint32_t level) try {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before editing style numbering",
                       this->extension_state().singleton_part_entries.styles);
        return false;
    }

    if (style_id.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id must not be empty when editing style numbering",
                       this->extension_state().singleton_part_entries.styles);
        return false;
    }

    if (level > max_list_level) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "numbering level must be in the range [0, 8]",
                       this->extension_state().singleton_part_entries.styles);
        return false;
    }

    if (const auto error = this->ensure_styles_loaded()) {
        return false;
    }
    if (const auto error = this->ensure_numbering_loaded()) {
        return false;
    }

    auto styles_root = this->styles.child("w:styles");
    if (styles_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info, document_errc::styles_xml_parse_failed,
                       "'" + this->extension_state().singleton_part_entries.styles + "' does not contain a w:styles root",
                       this->extension_state().singleton_part_entries.styles);
        return false;
    }

    auto style = find_style_node(styles_root, style_id);
    if (style == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id '" + std::string{style_id} +
                           "' was not found in '" + this->extension_state().singleton_part_entries.styles + "'",
                       this->extension_state().singleton_part_entries.styles);
        return false;
    }

    if (std::string_view{style.attribute("w:type").value()} != "paragraph") {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id '" + std::string{style_id} +
                           "' is not a paragraph style and cannot carry paragraph numbering",
                       this->extension_state().singleton_part_entries.styles);
        return false;
    }

    auto numbering_root = this->numbering.child("w:numbering");
    if (numbering_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       document_errc::numbering_xml_parse_failed,
                       "'" + this->extension_state().singleton_part_entries.numbering + "' does not contain the expected w:numbering root",
                       this->extension_state().singleton_part_entries.numbering);
        return false;
    }

    const auto abstract_num =
        find_abstract_numbering_by_id(numbering_root, numbering_definition_id);
    if (abstract_num == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "numbering definition id does not exist",
                       this->extension_state().singleton_part_entries.numbering);
        return false;
    }

    if (find_level_definition(abstract_num, level) == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "requested numbering level is not defined by the target definition",
                       this->extension_state().singleton_part_entries.numbering);
        return false;
    }

    const auto instance_plan = plan_numbering_instance_for_abstract(
        numbering_root, numbering_definition_id);
    if (!instance_plan.succeeded()) {
        set_numbering_mutation_error(
            this->last_error_info, this->extension_state().singleton_part_entries.numbering, instance_plan.status,
            "numbering instance id has reached UINT32_MAX",
            "failed to reserve a numbering instance for the target definition");
        return false;
    }

    if (this->ignored_singleton_relationship_type(styles_relationship_type) ||
        this->ignored_singleton_relationship_type(
            numbering_relationship_type)) {
        set_last_error(
            this->last_error_info, document_errc::invalid_package_structure,
            "cannot attach styles or numbering while a source relationship is invalid",
            std::string{document_relationships_xml_entry});
        return false;
    }
    if (!featherdoc::detail::package_relationships_document_allows_mutation(
            this->document_relationships,
            this->has_document_relationships_part)) {
        set_last_error(
            this->last_error_info, document_errc::invalid_package_structure,
            "cannot attach styles or numbering while document relationships are invalid",
            std::string{document_relationships_xml_entry});
        return false;
    }
    if (const auto error = this->ensure_content_types_loaded()) {
        return false;
    }

    auto working_styles = pugi::xml_document{};
    auto working_numbering = pugi::xml_document{};
    if (featherdoc::detail::checked_clone_xml_document(this->styles,
                                                       working_styles) !=
        featherdoc::detail::xml_document_clone_status::success) {
        (void)featherdoc::detail::set_xml_document_clone_allocation_failure(
            this->last_error_info,
            this->extension_state().singleton_part_entries.styles);
        return false;
    }
    if (featherdoc::detail::checked_clone_xml_document(
            this->numbering, working_numbering) !=
        featherdoc::detail::xml_document_clone_status::success) {
        (void)featherdoc::detail::set_xml_document_clone_allocation_failure(
            this->last_error_info,
            this->extension_state().singleton_part_entries.numbering);
        return false;
    }

    styles_root = working_styles.child("w:styles");
    style = find_style_node(styles_root, style_id);
    numbering_root = working_numbering.child("w:numbering");

    const auto num_id = ensure_numbering_instance_for_abstract(
        numbering_root, numbering_definition_id, std::nullopt, false,
        instance_plan.plan->reserved_num_id);
    if (!num_id.succeeded()) {
        set_numbering_mutation_error(
            this->last_error_info, this->extension_state().singleton_part_entries.numbering, num_id.status,
            "numbering instance id has reached UINT32_MAX",
            "failed to create or reuse a numbering instance for the target definition");
        return false;
    }

    if (!apply_numbering_to_style(style, level, *num_id.identifier)) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to populate numbering metadata for the target style",
                       this->extension_state().singleton_part_entries.styles);
        return false;
    }

    auto styles_attachment = singleton_part_attachment_option_storage{};
    styles_attachment.initialize(
        styles_relationship_type,
        this->extension_state().singleton_part_entries.styles,
        styles_content_type);
    auto numbering_attachment = singleton_part_attachment_option_storage{};
    numbering_attachment.initialize(
        numbering_relationship_type,
        this->extension_state().singleton_part_entries.numbering,
        numbering_content_type);
    const auto attachment_options = std::array{
        styles_attachment.options, numbering_attachment.options};
    auto attachment_work =
        featherdoc::detail::singleton_part_attachment_work{};
    if (const auto error =
            featherdoc::detail::prepare_checked_singleton_part_attachments(
                this->document_relationships, this->content_types,
                attachment_options, this->last_error_info, attachment_work)) {
        return false;
    }

    static_assert(std::is_nothrow_move_assignable_v<pugi::xml_document>);
    this->styles = std::move(working_styles);
    this->numbering = std::move(working_numbering);
    featherdoc::detail::publish_checked_singleton_part_attachment(
        std::move(attachment_work), this->document_relationships,
        this->content_types, this->has_document_relationships_part,
        this->document_relationships_dirty, this->content_types_dirty);
    this->has_styles_part = true;
    this->has_numbering_part = true;
    this->numbering_dirty = true;
    this->styles_dirty = true;
    this->last_error_info.clear();
    return true;
} catch (const std::bad_alloc &) {
    (void)featherdoc::detail::set_xml_document_mutation_allocation_failure(
        this->last_error_info,
        this->extension_state().singleton_part_entries.styles);
    return false;
}

bool Document::clear_paragraph_style_numbering(std::string_view style_id) try {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before editing style numbering",
                       this->extension_state().singleton_part_entries.styles);
        return false;
    }

    if (style_id.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id must not be empty when editing style numbering",
                       this->extension_state().singleton_part_entries.styles);
        return false;
    }

    if (const auto error = this->ensure_styles_loaded()) {
        return false;
    }

    auto styles_root = this->styles.child("w:styles");
    if (styles_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info, document_errc::styles_xml_parse_failed,
                       "'" + this->extension_state().singleton_part_entries.styles + "' does not contain a w:styles root",
                       this->extension_state().singleton_part_entries.styles);
        return false;
    }

    auto style = find_style_node(styles_root, style_id);
    if (style == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id '" + std::string{style_id} +
                           "' was not found in '" + this->extension_state().singleton_part_entries.styles + "'",
                       this->extension_state().singleton_part_entries.styles);
        return false;
    }

    if (std::string_view{style.attribute("w:type").value()} != "paragraph") {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id '" + std::string{style_id} +
                           "' is not a paragraph style and cannot carry paragraph numbering",
                       this->extension_state().singleton_part_entries.styles);
        return false;
    }

    if (!has_direct_numbering_properties(style)) {
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
    if (featherdoc::detail::checked_clone_xml_document(this->styles,
                                                       working_styles) !=
        featherdoc::detail::xml_document_clone_status::success) {
        (void)featherdoc::detail::set_xml_document_clone_allocation_failure(
            this->last_error_info,
            this->extension_state().singleton_part_entries.styles);
        return false;
    }
    const auto working_style = find_style_node(
        working_styles.child("w:styles"), style_id);
    bool changed = false;
    if (!clear_direct_numbering_properties(working_style, changed) ||
        !changed) {
        set_last_error(
            this->last_error_info,
            std::make_error_code(std::errc::not_enough_memory),
            "failed to clear numbering metadata from the target style",
            this->extension_state().singleton_part_entries.styles);
        return false;
    }

    auto styles_attachment = singleton_part_attachment_option_storage{};
    styles_attachment.initialize(
        styles_relationship_type,
        this->extension_state().singleton_part_entries.styles,
        styles_content_type);
    const auto attachment_options = std::array{styles_attachment.options};
    auto attachment_work =
        featherdoc::detail::singleton_part_attachment_work{};
    if (const auto error =
            featherdoc::detail::prepare_checked_singleton_part_attachments(
                this->document_relationships, this->content_types,
                attachment_options, this->last_error_info, attachment_work)) {
        return false;
    }

    static_assert(std::is_nothrow_move_assignable_v<pugi::xml_document>);
    this->styles = std::move(working_styles);
    featherdoc::detail::publish_checked_singleton_part_attachment(
        std::move(attachment_work), this->document_relationships,
        this->content_types, this->has_document_relationships_part,
        this->document_relationships_dirty, this->content_types_dirty);
    this->has_styles_part = true;
    this->styles_dirty = true;
    this->last_error_info.clear();
    return true;
} catch (const std::bad_alloc &) {
    (void)featherdoc::detail::set_xml_document_mutation_allocation_failure(
        this->last_error_info,
        this->extension_state().singleton_part_entries.styles);
    return false;
}

bool Document::set_paragraph_list(Paragraph target_paragraph, featherdoc::list_kind kind,
                                  std::uint32_t level) try {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before editing paragraph lists");
        return false;
    }

    auto target_entry_name = std::string_view{};
    if (!this->owns_paragraph_handle(target_paragraph, target_entry_name)) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "target paragraph handle is not valid or does not belong to this document",
                       std::string{document_xml_entry});
        return false;
    }

    if (level > max_list_level) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "list level must be in the range [0, 8]",
                       std::string{target_entry_name});
        return false;
    }

    if (const auto error = this->ensure_numbering_loaded()) {
        return false;
    }

    auto numbering_root = this->numbering.child("w:numbering");
    if (numbering_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       document_errc::numbering_xml_parse_failed,
                       "'" + this->extension_state().singleton_part_entries.numbering + "' does not contain the expected w:numbering root",
                       this->extension_state().singleton_part_entries.numbering);
        return false;
    }

    const auto previous_num_id = previous_managed_num_id_for_kind(
        target_paragraph.current, numbering_root, kind);
    auto instance_plan = numbering_definition_instance_plan_result{};
    if (!previous_num_id.has_value()) {
        instance_plan = plan_numbering_definition_instance(
            numbering_root, list_name_for(kind));
        if (!instance_plan.succeeded()) {
            set_numbering_mutation_error(
                this->last_error_info, this->extension_state().singleton_part_entries.numbering, instance_plan.status,
                "numbering id has reached UINT32_MAX",
                "failed to reserve a managed numbering definition and instance");
            return false;
        }
    }

    if (this->ignored_singleton_relationship_type(
            numbering_relationship_type)) {
        set_last_error(
            this->last_error_info, document_errc::invalid_package_structure,
            "cannot attach numbering while its source relationship is invalid",
            std::string{document_relationships_xml_entry});
        return false;
    }
    if (!featherdoc::detail::package_relationships_document_allows_mutation(
            this->document_relationships,
            this->has_document_relationships_part)) {
        set_last_error(
            this->last_error_info, document_errc::invalid_package_structure,
            "cannot attach numbering while document relationships are invalid",
            std::string{document_relationships_xml_entry});
        return false;
    }
    if (const auto error = this->ensure_content_types_loaded()) {
        return false;
    }

    auto working_numbering = pugi::xml_document{};
    if (featherdoc::detail::checked_clone_xml_document(
            this->numbering, working_numbering) !=
        featherdoc::detail::xml_document_clone_status::success) {
        (void)featherdoc::detail::set_xml_document_clone_allocation_failure(
            this->last_error_info,
            this->extension_state().singleton_part_entries.numbering);
        return false;
    }
    numbering_root = working_numbering.child("w:numbering");

    auto num_id = numbering_identifier_result{};
    if (previous_num_id.has_value()) {
        num_id = {numbering_mutation_status::success, *previous_num_id};
    } else {
        num_id = commit_managed_numbering_instance(
            numbering_root, kind, *instance_plan.plan);
    }
    if (!num_id.succeeded()) {
        set_numbering_mutation_error(
            this->last_error_info, this->extension_state().singleton_part_entries.numbering, num_id.status,
            "numbering id has reached UINT32_MAX",
            "failed to create or reuse a managed numbering definition");
        return false;
    }

    auto numbering_attachment = singleton_part_attachment_option_storage{};
    numbering_attachment.initialize(
        numbering_relationship_type,
        this->extension_state().singleton_part_entries.numbering,
        numbering_content_type);
    const auto attachment_options =
        std::array{numbering_attachment.options};
    auto attachment_work =
        featherdoc::detail::singleton_part_attachment_work{};
    if (const auto error =
            featherdoc::detail::prepare_checked_singleton_part_attachments(
                this->document_relationships, this->content_types,
                attachment_options, this->last_error_info, attachment_work)) {
        return false;
    }

    auto paragraph_transaction = paragraph_properties_transaction{};
    if (!paragraph_transaction.prepare(target_paragraph.current) ||
        !apply_numbering_to_paragraph_properties(
            paragraph_transaction.working_properties(), level,
            *num_id.identifier) ||
        !paragraph_transaction.commit()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to populate numbering metadata for the target paragraph",
                       std::string{target_entry_name});
        return false;
    }

    static_assert(std::is_nothrow_move_assignable_v<pugi::xml_document>);
    this->numbering = std::move(working_numbering);
    featherdoc::detail::publish_checked_singleton_part_attachment(
        std::move(attachment_work), this->document_relationships,
        this->content_types, this->has_document_relationships_part,
        this->document_relationships_dirty, this->content_types_dirty);
    this->has_numbering_part = true;
    this->numbering_dirty = true;
    this->last_error_info.clear();
    return true;
} catch (const std::bad_alloc &) {
    (void)featherdoc::detail::set_xml_document_mutation_allocation_failure(
        this->last_error_info,
        this->extension_state().singleton_part_entries.numbering);
    return false;
}

bool Document::restart_paragraph_list(Paragraph target_paragraph, featherdoc::list_kind kind,
                                      std::uint32_t level) try {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before editing paragraph lists");
        return false;
    }

    auto target_entry_name = std::string_view{};
    if (!this->owns_paragraph_handle(target_paragraph, target_entry_name)) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "target paragraph handle is not valid or does not belong to this document",
                       std::string{document_xml_entry});
        return false;
    }

    if (level > max_list_level) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "list level must be in the range [0, 8]",
                       std::string{target_entry_name});
        return false;
    }

    if (const auto error = this->ensure_numbering_loaded()) {
        return false;
    }

    auto numbering_root = this->numbering.child("w:numbering");
    if (numbering_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       document_errc::numbering_xml_parse_failed,
                       "'" + this->extension_state().singleton_part_entries.numbering + "' does not contain the expected w:numbering root",
                       this->extension_state().singleton_part_entries.numbering);
        return false;
    }

    const auto instance_plan = plan_numbering_definition_instance(
        numbering_root, list_name_for(kind), true);
    if (!instance_plan.succeeded()) {
        set_numbering_mutation_error(
            this->last_error_info, this->extension_state().singleton_part_entries.numbering, instance_plan.status,
            "numbering id has reached UINT32_MAX",
            "failed to reserve a restarted managed numbering instance");
        return false;
    }

    if (this->ignored_singleton_relationship_type(
            numbering_relationship_type)) {
        set_last_error(
            this->last_error_info, document_errc::invalid_package_structure,
            "cannot attach numbering while its source relationship is invalid",
            std::string{document_relationships_xml_entry});
        return false;
    }
    if (!featherdoc::detail::package_relationships_document_allows_mutation(
            this->document_relationships,
            this->has_document_relationships_part)) {
        set_last_error(
            this->last_error_info, document_errc::invalid_package_structure,
            "cannot attach numbering while document relationships are invalid",
            std::string{document_relationships_xml_entry});
        return false;
    }
    if (const auto error = this->ensure_content_types_loaded()) {
        return false;
    }

    auto working_numbering = pugi::xml_document{};
    if (featherdoc::detail::checked_clone_xml_document(
            this->numbering, working_numbering) !=
        featherdoc::detail::xml_document_clone_status::success) {
        (void)featherdoc::detail::set_xml_document_clone_allocation_failure(
            this->last_error_info,
            this->extension_state().singleton_part_entries.numbering);
        return false;
    }
    numbering_root = working_numbering.child("w:numbering");

    const auto num_id = commit_managed_numbering_instance(
        numbering_root, kind, *instance_plan.plan,
        std::optional<std::uint32_t>{level}, true);
    if (!num_id.succeeded()) {
        set_numbering_mutation_error(
            this->last_error_info, this->extension_state().singleton_part_entries.numbering, num_id.status,
            "numbering id has reached UINT32_MAX",
            "failed to create a restarted managed numbering definition");
        return false;
    }

    auto numbering_attachment = singleton_part_attachment_option_storage{};
    numbering_attachment.initialize(
        numbering_relationship_type,
        this->extension_state().singleton_part_entries.numbering,
        numbering_content_type);
    const auto attachment_options =
        std::array{numbering_attachment.options};
    auto attachment_work =
        featherdoc::detail::singleton_part_attachment_work{};
    if (const auto error =
            featherdoc::detail::prepare_checked_singleton_part_attachments(
                this->document_relationships, this->content_types,
                attachment_options, this->last_error_info, attachment_work)) {
        return false;
    }

    auto paragraph_transaction = paragraph_properties_transaction{};
    if (!paragraph_transaction.prepare(target_paragraph.current) ||
        !apply_numbering_to_paragraph_properties(
            paragraph_transaction.working_properties(), level,
            *num_id.identifier) ||
        !paragraph_transaction.commit()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to populate numbering metadata for the target paragraph",
                       std::string{target_entry_name});
        return false;
    }

    static_assert(std::is_nothrow_move_assignable_v<pugi::xml_document>);
    this->numbering = std::move(working_numbering);
    featherdoc::detail::publish_checked_singleton_part_attachment(
        std::move(attachment_work), this->document_relationships,
        this->content_types, this->has_document_relationships_part,
        this->document_relationships_dirty, this->content_types_dirty);
    this->has_numbering_part = true;
    this->numbering_dirty = true;
    this->last_error_info.clear();
    return true;
} catch (const std::bad_alloc &) {
    (void)featherdoc::detail::set_xml_document_mutation_allocation_failure(
        this->last_error_info,
        this->extension_state().singleton_part_entries.numbering);
    return false;
}

bool Document::clear_paragraph_list(Paragraph target_paragraph) try {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before editing paragraph lists");
        return false;
    }

    auto target_entry_name = std::string_view{};
    if (!this->owns_paragraph_handle(target_paragraph, target_entry_name)) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "target paragraph handle is not valid or does not belong to this document",
                       std::string{document_xml_entry});
        return false;
    }

    if (!has_direct_numbering_properties(target_paragraph.current)) {
        this->last_error_info.clear();
        return true;
    }

    bool changed = false;
    if (!clear_direct_numbering_properties(target_paragraph.current, changed) ||
        !changed) {
        set_last_error(
            this->last_error_info,
            std::make_error_code(std::errc::not_enough_memory),
            "failed to clear numbering metadata from the target paragraph",
            std::string{target_entry_name});
        return false;
    }

    this->last_error_info.clear();
    return true;
} catch (const std::bad_alloc &) {
    (void)featherdoc::detail::set_xml_document_mutation_allocation_failure(
        this->last_error_info, document_xml_entry);
    return false;
}
