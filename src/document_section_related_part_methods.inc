bool Document::remove_section_related_part_reference(
    std::size_t section_index,
    featherdoc::section_reference_kind reference_kind,
    const char *reference_name) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before removing "
                       "header/footer references");
        return false;
    }

    if (section_index >= this->section_count()) {
        set_last_error(
            this->last_error_info,
            std::make_error_code(std::errc::invalid_argument),
            "section index is out of range for header/footer reference removal",
            std::string{document_xml_entry});
        return false;
    }

    const auto section_properties = this->section_properties(section_index);
    if (section_properties == pugi::xml_node{}) {
        this->last_error_info.clear();
        return false;
    }

    const auto xml_reference_type =
        featherdoc::to_xml_reference_type(reference_kind);
    const auto reference = find_section_reference(
        section_properties, reference_name, xml_reference_type);
    if (reference == pugi::xml_node{}) {
        this->last_error_info.clear();
        return false;
    }

    try {
        this->ensure_xml_handle_lifetime();
    } catch (const std::bad_alloc &) {
        (void)featherdoc::detail::set_xml_document_mutation_allocation_failure(
            this->last_error_info, document_xml_entry);
        return false;
    }

    auto document_work = pugi::xml_document{};
    if (featherdoc::detail::checked_clone_xml_document(this->document,
                                                       document_work) !=
        featherdoc::detail::xml_document_clone_status::success) {
        (void)featherdoc::detail::set_xml_document_clone_allocation_failure(
            this->last_error_info, document_xml_entry);
        return false;
    }

    const auto work_body =
        document_work.child("w:document").child("w:body");
    auto work_section_properties =
        featherdoc::detail::section_properties_at(work_body, section_index);
    const auto work_reference = find_section_reference(
        work_section_properties, reference_name, xml_reference_type);
    if (work_section_properties == pugi::xml_node{} ||
        work_reference == pugi::xml_node{} ||
        !work_section_properties.remove_child(work_reference)) {
        set_last_error(
            this->last_error_info,
            document_errc::invalid_package_structure,
            "failed to resolve the prepared section header/footer reference",
            std::string{document_xml_entry});
        return false;
    }

    const auto publish_result = publish_checked_section_document_update(
        this->document, document_work, section_index, true);
    if (publish_result == section_document_publish_result::failure) {
        (void)featherdoc::detail::set_xml_document_mutation_allocation_failure(
            this->last_error_info, document_xml_entry);
        return false;
    }
    if (publish_result ==
        section_document_publish_result::requires_full_document_publish) {
        set_last_error(
            this->last_error_info,
            document_errc::invalid_package_structure,
            "prepared header/footer reference removal unexpectedly changes "
            "the document relationship namespace",
            std::string{document_xml_entry});
        return false;
    }
    this->last_error_info.clear();
    return true;
}

std::error_code Document::ensure_removal_prerequisites_loaded() {
    if (!relationships_document_allows_mutation(
            this->document_relationships, this->has_document_relationships_part,
            document_relationships_xml_entry, this->last_error_info)) {
        return this->last_error_info.code;
    }

    if (const auto error = this->ensure_content_types_loaded()) {
        return error;
    }

    if (featherdoc::detail::package_relationships_root(
            this->document_relationships) == pugi::xml_node{}) {
        const auto relationships_initialization =
            featherdoc::detail::initialize_empty_relationships_document(
                this->document_relationships);
        if (!relationships_initialization) {
            return featherdoc::detail::set_fixed_xml_initialization_failure(
                this->last_error_info, relationships_initialization,
                document_errc::relationships_xml_parse_failed,
                document_relationships_xml_entry);
        }
    }

    return {};
}

bool Document::remove_related_part(
    std::size_t part_index, std::vector<std::unique_ptr<xml_part_state>> &parts,
    const char *reference_name) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before removing "
                       "header/footer parts");
        return false;
    }

    if (const auto error = this->ensure_removal_prerequisites_loaded()) {
        return false;
    }

    if (part_index >= parts.size()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "header/footer part index is out of range for removal",
                       std::string{document_xml_entry});
        return false;
    }

    const auto &removed_part = *parts[part_index];
    auto entry_name = std::string{};
    auto relationships_entry_name = std::string{};
    auto removed_entries_work = std::unordered_set<std::string>{};
    auto *extension = static_cast<featherdoc::detail::document_extension_state *>(
        nullptr);
    try {
        extension = &this->extension_state();
        entry_name = removed_part.entry_name;
        relationships_entry_name = removed_part.relationships_entry_name;
        removed_entries_work = this->removed_related_part_entries;
        removed_entries_work.insert(entry_name);
        if (!relationships_entry_name.empty()) {
            removed_entries_work.insert(relationships_entry_name);
        }
    } catch (const std::bad_alloc &) {
        (void)featherdoc::detail::set_xml_document_mutation_allocation_failure(
            this->last_error_info, document_xml_entry);
        return false;
    }

    auto document_work = pugi::xml_document{};
    if (featherdoc::detail::checked_clone_xml_document(this->document,
                                                       document_work) !=
        featherdoc::detail::xml_document_clone_status::success) {
        (void)featherdoc::detail::set_xml_document_clone_allocation_failure(
            this->last_error_info, document_xml_entry);
        return false;
    }

    const auto remove_from_section = [&](pugi::xml_node section_properties) {
        for (auto reference = section_properties.child(reference_name);
             reference != pugi::xml_node{};) {
            const auto next = reference.next_sibling(reference_name);
            if (related_part_has_relationship_id(
                    removed_part, reference.attribute("r:id").value())) {
                section_properties.remove_child(reference);
            }

            reference = next;
        }
    };

    const auto body = document_work.child("w:document").child("w:body");
    if (body == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "document.xml does not contain the expected "
                       "w:document/w:body structure",
                       std::string{document_xml_entry});
        return false;
    }

    for (auto child = body.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        if (std::string_view{child.name()} != "w:p") {
            continue;
        }

        if (auto section_properties = child.child("w:pPr").child("w:sectPr");
            section_properties != pugi::xml_node{}) {
            remove_from_section(section_properties);
        }
    }

    if (auto section_properties = body.child("w:sectPr");
        section_properties != pugi::xml_node{}) {
        remove_from_section(section_properties);
    }

    static_assert(std::is_nothrow_move_assignable_v<pugi::xml_document>);
    static_assert(std::is_nothrow_move_assignable_v<
                  decltype(this->removed_related_part_entries)>);

    // TemplatePart retains a pointer to the part's xml_document rather than a
    // node epoch. Retire the complete package generation before destroying the
    // part, then rebind Document-owned cursors to the published DOMs.
    this->invalidate_xml_handles();
    this->document = std::move(document_work);
    this->removed_related_part_entries = std::move(removed_entries_work);
    extension->related_part_relationship_alias_ids.erase(entry_name);
    parts.erase(parts.begin() + static_cast<std::ptrdiff_t>(part_index));
    this->rebind_internal_xml_handles();

    this->last_error_info.clear();
    return true;
}

bool Document::move_related_part(
    std::size_t source_index, std::size_t target_index,
    std::vector<std::unique_ptr<xml_part_state>> &parts,
    const char *relationship_type) try {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before reordering "
                       "header/footer parts");
        return false;
    }

    if (source_index >= parts.size() || target_index >= parts.size()) {
        set_last_error(
            this->last_error_info,
            std::make_error_code(std::errc::invalid_argument),
            "header/footer part index is out of range for reordering",
            std::string{document_xml_entry});
        return false;
    }

    if (source_index == target_index) {
        this->last_error_info.clear();
        return true;
    }

    if (!relationships_document_allows_mutation(
            this->document_relationships, this->has_document_relationships_part,
            document_relationships_xml_entry, this->last_error_info)) {
        return false;
    }

    auto relationships_work = pugi::xml_document{};
    if (featherdoc::detail::checked_clone_xml_document(
            this->document_relationships, relationships_work) !=
        featherdoc::detail::xml_document_clone_status::success) {
        (void)featherdoc::detail::set_xml_document_clone_allocation_failure(
            this->last_error_info, document_relationships_xml_entry);
        return false;
    }

    auto relationships =
        featherdoc::detail::package_relationships_root(relationships_work);
    if (relationships == pugi::xml_node{}) {
        const auto relationships_initialization =
            featherdoc::detail::initialize_empty_relationships_document(
                relationships_work);
        if (!relationships_initialization) {
            (void)featherdoc::detail::set_fixed_xml_initialization_failure(
                this->last_error_info, relationships_initialization,
                document_errc::relationships_xml_parse_failed,
                document_relationships_xml_entry);
            return false;
        }
    }
    relationships =
        featherdoc::detail::package_relationships_root(relationships_work);

    const auto &source_part = *parts[source_index];
    const auto &target_part = *parts[target_index];
    std::vector<pugi::xml_node> source_relationships;
    std::vector<pugi::xml_node> target_relationships;
    for (auto relationship =
             featherdoc::detail::first_package_relationship(relationships);
         relationship != pugi::xml_node{};
         relationship =
             featherdoc::detail::next_package_relationship(relationship)) {
        if (std::string_view{relationship.attribute("Type").value()} !=
            relationship_type) {
            continue;
        }

        const auto relationship_id =
            std::string_view{relationship.attribute("Id").value()};
        if (related_part_has_relationship_id(source_part, relationship_id)) {
            source_relationships.push_back(relationship);
        } else if (related_part_has_relationship_id(target_part,
                                                    relationship_id)) {
            target_relationships.push_back(relationship);
        }
    }

    if (source_relationships.empty() || target_relationships.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "failed to resolve the requested header/footer "
                       "relationship in document.xml.rels",
                       std::string{document_relationships_xml_entry});
        return false;
    }

    if (source_index < target_index) {
        auto anchor = target_relationships.back();
        for (const auto relationship : source_relationships) {
            const auto moved =
                relationships.insert_move_after(relationship, anchor);
            if (moved == pugi::xml_node{}) {
                set_last_error(
                    this->last_error_info,
                    std::make_error_code(std::errc::not_enough_memory),
                    "failed to reorder the requested header/footer "
                    "relationship aliases",
                    std::string{document_relationships_xml_entry});
                return false;
            }
            anchor = moved;
        }
    } else {
        const auto anchor = target_relationships.front();
        for (const auto relationship : source_relationships) {
            if (relationships.insert_move_before(relationship, anchor) ==
                pugi::xml_node{}) {
                set_last_error(
                    this->last_error_info,
                    std::make_error_code(std::errc::not_enough_memory),
                    "failed to reorder the requested header/footer "
                    "relationship aliases",
                    std::string{document_relationships_xml_entry});
                return false;
            }
        }
    }

    static_assert(std::is_nothrow_move_assignable_v<pugi::xml_document>);
    static_assert(std::is_nothrow_swappable_v<
                  std::unique_ptr<xml_part_state>>);
    if (source_index < target_index) {
        std::rotate(parts.begin() + static_cast<std::ptrdiff_t>(source_index),
                    parts.begin() +
                        static_cast<std::ptrdiff_t>(source_index + 1U),
                    parts.begin() +
                        static_cast<std::ptrdiff_t>(target_index + 1U));
    } else {
        std::rotate(parts.begin() + static_cast<std::ptrdiff_t>(target_index),
                    parts.begin() + static_cast<std::ptrdiff_t>(source_index),
                    parts.begin() +
                        static_cast<std::ptrdiff_t>(source_index + 1U));
    }
    this->document_relationships = std::move(relationships_work);

    this->has_document_relationships_part = true;
    this->document_relationships_dirty = true;
    this->last_error_info.clear();
    return true;
} catch (const std::bad_alloc &) {
    (void)featherdoc::detail::set_xml_document_mutation_allocation_failure(
        this->last_error_info, document_relationships_xml_entry);
    return false;
}

bool Document::copy_section_related_part_references(
    std::size_t source_section_index, std::size_t target_section_index,
    const char *reference_name) try {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before copying "
                       "header/footer references");
        return false;
    }

    const auto count = this->section_count();
    if (source_section_index >= count || target_section_index >= count) {
        set_last_error(
            this->last_error_info,
            std::make_error_code(std::errc::invalid_argument),
            "section index is out of range for header/footer reference copy",
            std::string{document_xml_entry});
        return false;
    }

    if (source_section_index == target_section_index) {
        this->last_error_info.clear();
        return true;
    }

    if (!relationships_document_allows_mutation(
            this->document_relationships, this->has_document_relationships_part,
            document_relationships_xml_entry, this->last_error_info)) {
        return false;
    }
    this->ensure_xml_handle_lifetime();

    auto work = section_related_part_mutation_work{};
    if (featherdoc::detail::checked_clone_xml_document(this->document,
                                                       work.document) !=
        featherdoc::detail::xml_document_clone_status::success) {
        (void)featherdoc::detail::set_xml_document_clone_allocation_failure(
            this->last_error_info, document_xml_entry);
        return false;
    }

    auto document_root = work.document.child("w:document");
    auto body = document_root.child("w:body");
    if (document_root == pugi::xml_node{} || body == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       document_errc::invalid_package_structure,
                       "document.xml does not contain the expected "
                       "w:document/w:body structure",
                       std::string{document_xml_entry});
        return false;
    }

    const auto source_section_properties =
        featherdoc::detail::section_properties_at(body,
                                                  source_section_index);
    struct copied_reference_state {
        std::string type;
        std::string relationship_id;
    };
    std::vector<copied_reference_state> copied_references;
    bool copied_first_page_reference = false;
    bool copied_even_page_reference = false;
    bool copied_default_reference = false;
    const auto reference_name_view = std::string_view{reference_name};
    if (reference_name_view != "w:headerReference" &&
        reference_name_view != "w:footerReference") {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "unsupported section related-part reference name",
                       std::string{document_xml_entry});
        return false;
    }
    const auto &related_parts =
        reference_name_view == "w:headerReference" ? this->header_parts
                                                    : this->footer_parts;
    if (source_section_properties != pugi::xml_node{}) {
        for (auto reference = source_section_properties.child(reference_name);
             reference != pugi::xml_node{};
             reference = reference.next_sibling(reference_name)) {
            const auto reference_type =
                std::string_view{reference.attribute("w:type").value()};
            auto *already_copied = &copied_default_reference;
            if (reference_type == "first") {
                already_copied = &copied_first_page_reference;
            } else if (reference_type == "even") {
                already_copied = &copied_even_page_reference;
            } else if (reference_type != "default") {
                set_last_error(
                    this->last_error_info,
                    document_errc::invalid_package_structure,
                    "source section contains an unsupported header/footer "
                    "reference type",
                    std::string{document_xml_entry});
                return false;
            }
            if (*already_copied) {
                set_last_error(
                    this->last_error_info,
                    document_errc::invalid_package_structure,
                    "source section contains duplicate header/footer "
                    "reference types",
                    std::string{document_xml_entry});
                return false;
            }

            const auto relationship_id =
                std::string_view{reference.attribute("r:id").value()};
            const bool resolves_to_related_part =
                !relationship_id.empty() &&
                std::any_of(
                    related_parts.begin(), related_parts.end(),
                    [this, relationship_id](const auto &part) {
                        return related_part_has_relationship_id(
                            *part, relationship_id);
                    });
            if (!resolves_to_related_part) {
                set_last_error(
                    this->last_error_info,
                    document_errc::invalid_package_structure,
                    "source section contains an unresolved header/footer "
                    "relationship",
                    std::string{document_xml_entry});
                return false;
            }

            *already_copied = true;
            copied_reference_state copied_reference;
            copied_reference.type = reference_type;
            copied_reference.relationship_id = relationship_id;
            copied_references.push_back(std::move(copied_reference));
        }
    }

    auto target_section_properties =
        featherdoc::detail::section_properties_at(body, target_section_index);
    if (copied_references.empty() &&
        (target_section_properties == pugi::xml_node{} ||
         target_section_properties.child(reference_name) ==
             pugi::xml_node{})) {
        this->last_error_info.clear();
        return true;
    }
    if (target_section_properties == pugi::xml_node{}) {
        target_section_properties = checked_ensure_section_properties(
            body, target_section_index, count, work.document_changed);
    }
    if (target_section_properties == pugi::xml_node{}) {
        (void)featherdoc::detail::set_xml_document_mutation_allocation_failure(
            this->last_error_info, document_xml_entry);
        return false;
    }

    if (!copied_references.empty() &&
        !checked_set_section_attribute(
            document_root, "xmlns:r",
            office_document_relationships_namespace_uri,
            work.document_changed)) {
        (void)featherdoc::detail::set_xml_document_mutation_allocation_failure(
            this->last_error_info, document_xml_entry);
        return false;
    }

    for (auto reference = target_section_properties.child(reference_name);
         reference != pugi::xml_node{};) {
        const auto next = reference.next_sibling(reference_name);
        target_section_properties.remove_child(reference);
        work.document_changed = true;
        reference = next;
    }

    for (const auto &copied_reference : copied_references) {
        auto reference = checked_append_section_reference(
            target_section_properties, reference_name);
        if (reference == pugi::xml_node{}) {
            set_last_error(this->last_error_info,
                           std::make_error_code(std::errc::not_enough_memory),
                           "failed to attach a copied section header/footer "
                           "reference to w:sectPr",
                           std::string{document_xml_entry});
            return false;
        }

        work.document_changed = true;
        if (!checked_set_section_attribute(reference, "w:type",
                                           copied_reference.type,
                                           work.document_changed) ||
            !checked_set_section_attribute(reference, "r:id",
                                           copied_reference.relationship_id,
                                           work.document_changed)) {
            (void)featherdoc::detail::
                set_xml_document_mutation_allocation_failure(
                    this->last_error_info, document_xml_entry);
            return false;
        }
    }

    if (copied_first_page_reference &&
        !checked_enable_section_title_page(target_section_properties,
                                           work.document_changed)) {
        (void)featherdoc::detail::set_xml_document_mutation_allocation_failure(
            this->last_error_info, document_xml_entry);
        return false;
    }

    const bool needs_even_and_odd_setting = copied_even_page_reference;

    auto *settings_entry =
        needs_even_and_odd_setting
            ? &this->extension_state().singleton_part_entries.settings
            : nullptr;
    auto content_types_load = lazy_xml_document_load_rollback{
        needs_even_and_odd_setting, this->content_types,
        this->content_types_loaded};
    auto settings_load = lazy_xml_document_load_rollback{
        needs_even_and_odd_setting, this->settings, this->settings_loaded,
        &this->has_settings_part, settings_entry};

    std::string settings_entry_name;
    if (needs_even_and_odd_setting) {
        if (this->ignored_singleton_relationship_type(
                settings_relationship_type)) {
            set_last_error(
                this->last_error_info,
                document_errc::invalid_package_structure,
                "cannot attach settings while its source relationship is "
                "invalid",
                std::string{document_relationships_xml_entry});
            return false;
        }
        if (this->ensure_content_types_loaded() ||
            this->ensure_settings_loaded()) {
            return false;
        }
        settings_entry_name = *settings_entry;

        const auto relationship_target =
            make_document_relationship_target(settings_entry_name);
        const auto override_part_name =
            make_override_part_name(settings_entry_name);
        const auto options =
            featherdoc::detail::singleton_part_attachment_options{
                settings_relationship_type,
                relationship_target,
                override_part_name,
                settings_content_type,
                document_relationships_xml_entry,
                content_types_xml_entry,
                document_errc::relationships_xml_parse_failed,
                document_errc::content_types_xml_parse_failed};
        auto attachment =
            featherdoc::detail::singleton_part_attachment_work{};
        if (featherdoc::detail::prepare_checked_singleton_part_attachment(
                this->document_relationships, this->content_types, options,
                this->last_error_info, attachment)) {
            return false;
        }
        work.relationships = std::move(attachment.relationships);
        work.content_types = std::move(attachment.content_types);
        work.relationships_changed = attachment.relationships_changed;
        work.content_types_changed = attachment.content_types_changed;

        if (featherdoc::detail::checked_clone_xml_document(
                this->settings, work.settings) !=
            featherdoc::detail::xml_document_clone_status::success) {
            (void)featherdoc::detail::set_xml_document_clone_allocation_failure(
                this->last_error_info, settings_entry_name);
            return false;
        }
        if (!checked_enable_even_and_odd_headers(work.settings,
                                                 work.settings_changed)) {
            (void)featherdoc::detail::
                set_xml_document_mutation_allocation_failure(
                    this->last_error_info, settings_entry_name);
            return false;
        }
    }

    const auto document_publish_result =
        publish_checked_section_document_update(
            this->document, work.document, target_section_index,
            work.document_changed);
    if (document_publish_result == section_document_publish_result::failure) {
        (void)featherdoc::detail::set_xml_document_mutation_allocation_failure(
            this->last_error_info, document_xml_entry);
        return false;
    }
    if (document_publish_result ==
        section_document_publish_result::requires_full_document_publish) {
        const auto published_body =
            this->document.child("w:document").child("w:body");
        this->xml_handle_lifetime->retire_subtree(published_body);
        static_assert(std::is_nothrow_move_assignable_v<pugi::xml_document>);
        this->document = std::move(work.document);
        this->rebind_internal_xml_handles();
    }
    if (work.relationships_changed) {
        this->document_relationships = std::move(work.relationships);
        this->document_relationships_dirty = true;
    }
    if (work.content_types_changed) {
        this->content_types = std::move(work.content_types);
        this->content_types_dirty = true;
    }
    if (work.settings_changed) {
        this->settings = std::move(work.settings);
        this->settings_dirty = true;
    }
    if (needs_even_and_odd_setting) {
        if (!this->has_settings_part) {
            this->settings_dirty = true;
        }
        this->has_settings_part = true;
        this->has_document_relationships_part = true;
    }

    content_types_load.commit();
    settings_load.commit();

    this->last_error_info.clear();
    return true;
} catch (const std::bad_alloc &) {
    (void)featherdoc::detail::set_xml_document_mutation_allocation_failure(
        this->last_error_info, document_xml_entry);
    return false;
}

Paragraph &Document::ensure_section_header_paragraphs(
    std::size_t section_index,
    featherdoc::section_reference_kind reference_kind) {
    return this->ensure_section_related_part_paragraphs(
        section_index, reference_kind, this->header_parts, "w:hdr",
        "w:headerReference", header_relationship_type.data(),
        header_content_type.data());
}

Paragraph &Document::ensure_section_footer_paragraphs(
    std::size_t section_index,
    featherdoc::section_reference_kind reference_kind) {
    return this->ensure_section_related_part_paragraphs(
        section_index, reference_kind, this->footer_parts, "w:ftr",
        "w:footerReference", footer_relationship_type.data(),
        footer_content_type.data());
}

Paragraph &Document::assign_section_header_paragraphs(
    std::size_t section_index, std::size_t header_index,
    featherdoc::section_reference_kind reference_kind) {
    return this->assign_section_related_part_paragraphs(
        section_index, header_index, reference_kind, this->header_parts,
        "w:hdr", "w:headerReference");
}

Paragraph &Document::assign_section_footer_paragraphs(
    std::size_t section_index, std::size_t footer_index,
    featherdoc::section_reference_kind reference_kind) {
    return this->assign_section_related_part_paragraphs(
        section_index, footer_index, reference_kind, this->footer_parts,
        "w:ftr", "w:footerReference");
}

bool Document::remove_section_header_reference(
    std::size_t section_index,
    featherdoc::section_reference_kind reference_kind) try {
    return this->remove_section_related_part_reference(
        section_index, reference_kind, "w:headerReference");
} catch (const std::bad_alloc &) {
    (void)featherdoc::detail::set_xml_document_mutation_allocation_failure(
        this->last_error_info, document_xml_entry);
    return false;
}

bool Document::remove_section_footer_reference(
    std::size_t section_index,
    featherdoc::section_reference_kind reference_kind) try {
    return this->remove_section_related_part_reference(
        section_index, reference_kind, "w:footerReference");
} catch (const std::bad_alloc &) {
    (void)featherdoc::detail::set_xml_document_mutation_allocation_failure(
        this->last_error_info, document_xml_entry);
    return false;
}

bool Document::remove_header_part(std::size_t index) try {
    return this->remove_related_part(index, this->header_parts,
                                     "w:headerReference");
} catch (const std::bad_alloc &) {
    (void)featherdoc::detail::set_xml_document_mutation_allocation_failure(
        this->last_error_info, document_xml_entry);
    return false;
}

bool Document::remove_footer_part(std::size_t index) try {
    return this->remove_related_part(index, this->footer_parts,
                                     "w:footerReference");
} catch (const std::bad_alloc &) {
    (void)featherdoc::detail::set_xml_document_mutation_allocation_failure(
        this->last_error_info, document_xml_entry);
    return false;
}

bool Document::move_header_part(std::size_t source_index,
                                std::size_t target_index) {
    return this->move_related_part(source_index, target_index,
                                   this->header_parts,
                                   header_relationship_type.data());
}

bool Document::move_footer_part(std::size_t source_index,
                                std::size_t target_index) {
    return this->move_related_part(source_index, target_index,
                                   this->footer_parts,
                                   footer_relationship_type.data());
}

bool Document::copy_section_header_references(
    std::size_t source_section_index, std::size_t target_section_index) {
    return this->copy_section_related_part_references(
        source_section_index, target_section_index, "w:headerReference");
}

bool Document::copy_section_footer_references(
    std::size_t source_section_index, std::size_t target_section_index) {
    return this->copy_section_related_part_references(
        source_section_index, target_section_index, "w:footerReference");
}
