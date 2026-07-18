const std::vector<package_diagnostic> &
Document::package_diagnostics() const noexcept {
    return this->package_diagnostic_list;
}

std::optional<package_repair_report>
Document::repair_package(const document_repair_options &options) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() successfully before repair_package()");
        return std::nullopt;
    }

    package_repair_report report;
    report.diagnostics_before = this->package_diagnostic_list;
    if (report.diagnostics_before.empty()) {
        this->last_error_info.clear();
        return report;
    }

    const auto option_allows = [&](package_diagnostic_code code) {
        switch (code) {
        case package_diagnostic_code::missing_document_body:
            return options.repair_document_body;
        case package_diagnostic_code::missing_root_relationships:
        case package_diagnostic_code::missing_main_document_relationship:
        case package_diagnostic_code::invalid_main_document_relationship:
            return options.repair_root_relationships;
        case package_diagnostic_code::missing_content_types:
        case package_diagnostic_code::missing_main_document_content_type:
        case package_diagnostic_code::invalid_main_document_content_type:
            return options.repair_content_types;
        default:
            return false;
        }
    };

    for (const auto &diagnostic : report.diagnostics_before) {
        if (!diagnostic.repairable || !option_allows(diagnostic.code)) {
            set_last_error(
                this->last_error_info, document_errc::package_repair_not_possible,
                !diagnostic.repairable
                    ? "package contains an unsafe repair condition: " +
                          diagnostic.detail
                    : "repair options do not allow repairing: " +
                          diagnostic.detail,
                diagnostic.entry_name);
            return std::nullopt;
        }
    }

    pugi::xml_document repaired_document;
    pugi::xml_document repaired_package_relationships;
    pugi::xml_document repaired_content_types;
    if (featherdoc::detail::checked_clone_xml_document(
            this->document, repaired_document) !=
            featherdoc::detail::xml_document_clone_status::success ||
        featherdoc::detail::checked_clone_xml_document(
            this->package_relationships, repaired_package_relationships) !=
            featherdoc::detail::xml_document_clone_status::success ||
        featherdoc::detail::checked_clone_xml_document(
            this->content_types, repaired_content_types) !=
            featherdoc::detail::xml_document_clone_status::success) {
        (void)featherdoc::detail::set_xml_document_clone_allocation_failure(
            this->last_error_info);
        return std::nullopt;
    }

    bool repaired_document_body = false;
    bool repaired_root_relationships = false;
    bool repaired_content_types_part = false;

    const auto record_action = [&](const package_diagnostic &diagnostic,
                                   std::string detail) {
        report.actions.push_back(package_repair_action{
            diagnostic.code, diagnostic.entry_name, std::move(detail)});
    };

    const auto append_unique_relationship_id = [](pugi::xml_node relationships) {
        std::unordered_set<std::string> used_ids;
        for (auto relationship =
                 featherdoc::detail::first_package_relationship(relationships);
             relationship != pugi::xml_node{};
             relationship =
                 featherdoc::detail::next_package_relationship(relationship)) {
            used_ids.emplace(relationship.attribute("Id").value());
        }
        // Among N existing IDs, at least one value in [1, N + 1] is free.
        // Keep the search bounded by package data instead of scanning the
        // complete uint32_t identifier space for a hostile archive.
        for (std::size_t id = 1U; id <= used_ids.size() + 1U; ++id) {
            auto candidate = std::string{"rId"} + std::to_string(id);
            if (!used_ids.contains(candidate)) {
                return candidate;
            }
        }
        return std::string{};
    };

    const auto append_attribute_value = [](pugi::xml_node node,
                                           std::string_view name,
                                           std::string_view value) {
        auto attribute = node.append_attribute("");
        return attribute != pugi::xml_attribute{} &&
               attribute.set_name(name.data(), name.size()) &&
               attribute.set_value(value.data(), value.size());
    };

    const auto fail_mutation_allocation = [&](std::string_view entry_name) {
        (void)featherdoc::detail::
            set_xml_document_mutation_allocation_failure(
                this->last_error_info, entry_name);
    };

    for (const auto &diagnostic : report.diagnostics_before) {
        switch (diagnostic.code) {
        case package_diagnostic_code::missing_document_body: {
            auto root = repaired_document.child("w:document");
            if (root == pugi::xml_node{}) {
                set_last_error(this->last_error_info,
                               document_errc::package_repair_not_possible,
                               "w:document disappeared during repair",
                               diagnostic.entry_name);
                return std::nullopt;
            }
            auto body = root.append_child(pugi::node_element);
            if (body == pugi::xml_node{} || !body.set_name("w:body")) {
                fail_mutation_allocation(diagnostic.entry_name);
                return std::nullopt;
            }
            repaired_document_body = true;
            record_action(diagnostic, "created the missing w:body element");
            break;
        }
        case package_diagnostic_code::missing_root_relationships: {
            repaired_package_relationships.reset();
            const auto parse_result = repaired_package_relationships.load_buffer(
                relationships_xml.data(), relationships_xml.size());
            if (!parse_result) {
                if (parse_result.status == pugi::status_out_of_memory) {
                    (void)featherdoc::detail::set_xml_parse_failure(
                        this->last_error_info, parse_result,
                        document_errc::package_repair_not_possible,
                        diagnostic.entry_name);
                } else {
                    set_last_error(
                        this->last_error_info,
                        document_errc::package_repair_not_possible,
                        "failed to initialize root relationships",
                        diagnostic.entry_name);
                }
                return std::nullopt;
            }
            repaired_root_relationships = true;
            record_action(diagnostic,
                          "created the missing OPC root relationships part");
            break;
        }
        case package_diagnostic_code::missing_main_document_relationship: {
            auto relationships = featherdoc::detail::package_relationships_root(
                repaired_package_relationships);
            const auto relationship_id = append_unique_relationship_id(relationships);
            auto relationship =
                featherdoc::detail::append_package_relationship(relationships);
            if (relationship_id.empty()) {
                set_last_error(this->last_error_info,
                               document_errc::package_repair_not_possible,
                               "failed to reserve a main document relationship ID",
                               diagnostic.entry_name);
                return std::nullopt;
            }
            if (relationship == pugi::xml_node{} ||
                *relationship.name() == '\0' ||
                !append_attribute_value(relationship, "Id", relationship_id) ||
                !append_attribute_value(relationship, "Type",
                                        office_document_relationship_type) ||
                !append_attribute_value(relationship, "Target",
                                        document_xml_entry)) {
                fail_mutation_allocation(diagnostic.entry_name);
                return std::nullopt;
            }
            repaired_root_relationships = true;
            record_action(diagnostic,
                          "added the missing main document relationship");
            break;
        }
        case package_diagnostic_code::invalid_main_document_relationship: {
            auto relationships = featherdoc::detail::package_relationships_root(
                repaired_package_relationships);
            pugi::xml_node main_relationship;
            for (auto relationship =
                     featherdoc::detail::first_package_relationship(
                         relationships);
                 relationship != pugi::xml_node{};
                 relationship = featherdoc::detail::next_package_relationship(
                     relationship)) {
                if (std::string_view{relationship.attribute("Type").value()} ==
                    office_document_relationship_type) {
                    main_relationship = relationship;
                    break;
                }
            }
            if (main_relationship == pugi::xml_node{}) {
                set_last_error(this->last_error_info,
                               document_errc::package_repair_not_possible,
                               "main document relationship disappeared during repair",
                               diagnostic.entry_name);
                return std::nullopt;
            }
            auto target = main_relationship.attribute("Target");
            if (target == pugi::xml_attribute{}) {
                target = main_relationship.append_attribute("");
                if (target == pugi::xml_attribute{} ||
                    !target.set_name("Target")) {
                    fail_mutation_allocation(diagnostic.entry_name);
                    return std::nullopt;
                }
            }
            if (!target.set_value(document_xml_entry.data(),
                                  document_xml_entry.size())) {
                fail_mutation_allocation(diagnostic.entry_name);
                return std::nullopt;
            }
            repaired_root_relationships = true;
            record_action(diagnostic,
                          "corrected the main document relationship target");
            break;
        }
        case package_diagnostic_code::missing_content_types: {
            repaired_content_types.reset();
            const auto parse_result = repaired_content_types.load_buffer(
                content_types_xml.data(), content_types_xml.size(),
                pugi::parse_default | pugi::parse_ws_pcdata);
            if (!parse_result) {
                if (parse_result.status == pugi::status_out_of_memory) {
                    (void)featherdoc::detail::set_xml_parse_failure(
                        this->last_error_info, parse_result,
                        document_errc::package_repair_not_possible,
                        diagnostic.entry_name);
                } else {
                    set_last_error(
                        this->last_error_info,
                        document_errc::package_repair_not_possible,
                        "failed to initialize [Content_Types].xml",
                        diagnostic.entry_name);
                }
                return std::nullopt;
            }
            repaired_content_types_part = true;
            record_action(diagnostic,
                          "created the missing [Content_Types].xml part");
            break;
        }
        case package_diagnostic_code::missing_main_document_content_type: {
            auto types = featherdoc::detail::package_content_types_root(
                repaired_content_types);
            auto override_node =
                featherdoc::detail::append_package_content_type_override(
                    types);
            if (override_node == pugi::xml_node{} ||
                *override_node.name() == '\0' ||
                !append_attribute_value(override_node, "PartName",
                                        "/word/document.xml") ||
                !append_attribute_value(override_node, "ContentType",
                                        main_document_content_type)) {
                fail_mutation_allocation(diagnostic.entry_name);
                return std::nullopt;
            }
            repaired_content_types_part = true;
            record_action(diagnostic,
                          "added the missing main document content type");
            break;
        }
        case package_diagnostic_code::invalid_main_document_content_type: {
            auto types = featherdoc::detail::package_content_types_root(
                repaired_content_types);
            pugi::xml_node main_override;
            for (auto override_node =
                     featherdoc::detail::first_package_content_type_override(
                         types);
                 override_node != pugi::xml_node{};
                 override_node = featherdoc::detail::
                     next_package_content_type_override(override_node)) {
                if (featherdoc::detail::content_type_part_names_equivalent(
                        override_node.attribute("PartName").value(),
                        "/word/document.xml")) {
                    main_override = override_node;
                    break;
                }
            }
            if (main_override == pugi::xml_node{}) {
                set_last_error(this->last_error_info,
                               document_errc::package_repair_not_possible,
                               "main document content type disappeared during repair",
                               diagnostic.entry_name);
                return std::nullopt;
            }
            auto content_type = main_override.attribute("ContentType");
            if (content_type == pugi::xml_attribute{}) {
                content_type = main_override.append_attribute("");
                if (content_type == pugi::xml_attribute{} ||
                    !content_type.set_name("ContentType")) {
                    fail_mutation_allocation(diagnostic.entry_name);
                    return std::nullopt;
                }
            }
            if (!content_type.set_value(main_document_content_type.data(),
                                        main_document_content_type.size())) {
                fail_mutation_allocation(diagnostic.entry_name);
                return std::nullopt;
            }
            repaired_content_types_part = true;
            record_action(diagnostic,
                          "corrected the main document content type");
            break;
        }
        default:
            break;
        }
    }

    if (report.changed()) {
        this->invalidate_xml_handles();
    }
    if (repaired_document_body) {
        this->document = std::move(repaired_document);
    }
    if (repaired_root_relationships) {
        this->package_relationships =
            std::move(repaired_package_relationships);
        this->package_relationships_loaded = true;
        this->package_relationships_dirty = true;
    }
    if (repaired_content_types_part) {
        this->content_types = std::move(repaired_content_types);
        this->content_types_loaded = true;
        this->content_types_dirty = true;
    }

    auto body = this->document.child("w:document").child("w:body");
    this->paragraph.set_parent(this->tracked_node(body));
    this->table.set_owner(this);
    this->table.set_parent(this->tracked_node(body));
    this->package_repair_pending = report.changed();
    this->package_diagnostic_list.clear();
    this->last_error_info.clear();
    return report;
}
