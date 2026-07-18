std::error_code Document::ensure_numbering_loaded() {
    if (this->numbering_loaded) {
        return {};
    }

    this->numbering.reset();
    this->has_numbering_part = false;

    const auto load_default_numbering = [&]() -> std::error_code {
        const auto parse_result = this->numbering.load_buffer(
            empty_numbering_xml.data(), empty_numbering_xml.size());
        if (!parse_result) {
            return featherdoc::detail::set_xml_parse_failure(
                this->last_error_info, parse_result,
                document_errc::numbering_xml_parse_failed,
                this->extension_state().singleton_part_entries.numbering);
        }

        this->numbering_loaded = true;
        return {};
    };

    if (!this->has_source_archive) {
        return load_default_numbering();
    }

    const auto relationships = featherdoc::detail::package_relationships_root(
        this->document_relationships);
    if (relationships == pugi::xml_node{} &&
        !this->ignored_singleton_relationship_type(
            numbering_relationship_type)) {
        return set_last_error(this->last_error_info,
                              document_errc::relationships_xml_parse_failed,
                              "word/_rels/document.xml.rels does not contain a "
                              "Relationships root",
                              std::string{document_relationships_xml_entry});
    }

    const auto numbering_relationship =
        this->ignored_singleton_relationship_type(numbering_relationship_type)
            ? pugi::xml_node{}
            : find_document_relationship_by_type(relationships,
                                                 numbering_relationship_type);
    if (numbering_relationship == pugi::xml_node{} &&
        this->ignored_singleton_relationship_type(
            numbering_relationship_type)) {
        return load_default_numbering();
    }

    if (numbering_relationship != pugi::xml_node{}) {
        const auto target = std::string_view{
            numbering_relationship.attribute("Target").value()};
        if (target.empty()) {
            return set_last_error(
                this->last_error_info, document_errc::numbering_xml_read_failed,
                "numbering relationship does not contain a Target attribute",
                std::string{document_relationships_xml_entry});
        }
        if (std::string_view{
                numbering_relationship.attribute("TargetMode").value()} ==
            "External") {
            return set_last_error(
                this->last_error_info, document_errc::invalid_package_structure,
                "numbering relationship must not use TargetMode=External",
                std::string{document_relationships_xml_entry});
        }
        const auto resolved_numbering_target =
            featherdoc::detail::resolve_internal_package_relationship_target(
                document_xml_entry, target);
        if (!resolved_numbering_target) {
            return set_last_error(
                this->last_error_info, document_errc::invalid_package_structure,
                std::string{featherdoc::detail::
                                package_relationship_target_error_message(
                                    resolved_numbering_target.error)},
                std::string{document_relationships_xml_entry});
        }
        this->extension_state().singleton_part_entries.numbering =
            resolved_numbering_target.entry_name;
    }
    const auto &numbering_entry_name =
        this->extension_state().singleton_part_entries.numbering;

    const auto &source_archive_io_path =
        this->extension_state().source_archive_io_path;
    const auto source_archive_path =
        featherdoc::detail::path_to_utf8(source_archive_io_path);
    const auto native_source_archive_path =
        featherdoc::detail::path_to_native_utf8(source_archive_io_path);
    const auto source_archive =
        featherdoc::detail::open_read_zip_archive_with_limits(
            native_source_archive_path, this->opened_archive_limits);
    zip_t *source_zip = source_archive.archive;
    if (!source_zip) {
        return featherdoc::detail::set_reopened_source_archive_open_error(
            this->last_error_info, source_archive_path, source_archive,
            "loading numbering metadata", numbering_entry_name);
    }
    featherdoc::detail::read_zip_archive_guard source_zip_guard{source_zip};

    auto entry_catalog = featherdoc::detail::archive_entry_catalog{};
    if (const auto validation_error =
            featherdoc::detail::validate_reopened_source_archive_metadata(
                source_zip, this->opened_archive_limits, this->last_error_info,
                this->extension_state().opened_validation_mode,
                this->extension_state().source_archive_snapshot,
                source_archive_path, &entry_catalog)) {
        return validation_error;
    }

    if (numbering_relationship == pugi::xml_node{} &&
        featherdoc::detail::find_archive_entry_record(
            entry_catalog, numbering_entry_name) == nullptr) {
        if (const auto close_error = featherdoc::detail::close_read_archive(
                source_zip_guard, this->last_error_info,
                "failed to close source archive after checking for an "
                "orphaned numbering part",
                numbering_entry_name)) {
            return close_error;
        }
        return load_default_numbering();
    }

    std::string numbering_text;
    const auto read_status = read_zip_entry_text(
        source_zip, numbering_entry_name, numbering_text,
        &this->opened_archive_limits, &this->last_error_info, entry_catalog);

    if (read_status == zip_entry_read_status::limit_exceeded) {
        return this->last_error_info.code;
    }

    if (read_status != zip_entry_read_status::ok) {
        return set_last_error(this->last_error_info,
                              document_errc::numbering_xml_read_failed,
                              "failed to read required numbering part '" +
                                  numbering_entry_name + "'",
                              numbering_entry_name);
    }

    if (const auto close_error = featherdoc::detail::close_read_archive(
            source_zip_guard, this->last_error_info,
            "failed to close source archive after loading numbering",
            numbering_entry_name)) {
        return close_error;
    }

    const auto parse_result = this->numbering.load_buffer(
        numbering_text.data(), numbering_text.size());
    if (!parse_result) {
        return featherdoc::detail::set_xml_parse_failure(
            this->last_error_info, parse_result,
            document_errc::numbering_xml_parse_failed, numbering_entry_name);
    }

    const auto namespace_result =
        featherdoc::detail::canonicalize_wordprocessingml_part(
            this->numbering,
            featherdoc::detail::wordprocessingml_part_kind::numbering);
    if (!namespace_result) {
        this->numbering.reset();
        return featherdoc::detail::set_wordprocessingml_namespace_last_error(
            this->last_error_info, namespace_result, numbering_entry_name);
    }
    if (!namespace_result.root_matches) {
        this->numbering.reset();
        return featherdoc::detail::
            set_wordprocessingml_root_mismatch_last_error(
                this->last_error_info, namespace_result,
                featherdoc::detail::wordprocessingml_part_kind::numbering,
                numbering_entry_name);
    }

    this->numbering_loaded = true;
    this->has_numbering_part = true;
    return {};
}

std::error_code Document::ensure_numbering_part_attached() {
    if (this->ignored_singleton_relationship_type(
            numbering_relationship_type)) {
        return set_last_error(
            this->last_error_info, document_errc::invalid_package_structure,
            "cannot attach numbering while its source relationship is invalid",
            std::string{document_relationships_xml_entry});
    }
    if (!featherdoc::detail::package_relationships_document_allows_mutation(
            this->document_relationships,
            this->has_document_relationships_part)) {
        return set_last_error(
            this->last_error_info, document_errc::invalid_package_structure,
            "cannot attach numbering while document relationships are invalid",
            std::string{document_relationships_xml_entry});
    }
    if (const auto error = this->ensure_numbering_loaded()) {
        return error;
    }
    const auto had_numbering_part = this->has_numbering_part;

    if (const auto error = this->ensure_content_types_loaded()) {
        return error;
    }

    auto relationship_target = std::string{};
    try {
        relationship_target = make_document_relationship_target(
            this->extension_state().singleton_part_entries.numbering);
    } catch (...) {
        return featherdoc::detail::set_xml_document_mutation_allocation_failure(
            this->last_error_info, document_relationships_xml_entry);
    }
    auto override_part_name = std::string{};
    try {
        override_part_name = make_override_part_name(
            this->extension_state().singleton_part_entries.numbering);
    } catch (...) {
        return featherdoc::detail::set_xml_document_mutation_allocation_failure(
            this->last_error_info, content_types_xml_entry);
    }
    const auto options = featherdoc::detail::singleton_part_attachment_options{
        numbering_relationship_type,
        relationship_target,
        override_part_name,
        numbering_content_type,
        document_relationships_xml_entry,
        content_types_xml_entry,
        document_errc::relationships_xml_parse_failed,
        document_errc::content_types_xml_parse_failed};
    auto work = featherdoc::detail::singleton_part_attachment_work{};
    if (const auto error =
            featherdoc::detail::prepare_checked_singleton_part_attachment(
                this->document_relationships, this->content_types, options,
                this->last_error_info, work)) {
        return error;
    }
    featherdoc::detail::publish_checked_singleton_part_attachment(
        std::move(work), this->document_relationships, this->content_types,
        this->has_document_relationships_part,
        this->document_relationships_dirty, this->content_types_dirty);
    this->has_numbering_part = true;
    if (!had_numbering_part) {
        this->numbering_dirty = true;
    }
    return {};
}

std::optional<std::uint32_t> Document::ensure_numbering_definition(
    const featherdoc::numbering_definition &definition) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before editing numbering "
                       "definitions");
        return std::nullopt;
    }

    auto validation_detail = std::string{};
    if (!validate_numbering_definition(definition, validation_detail)) {
        set_last_error(
            this->last_error_info,
            std::make_error_code(std::errc::invalid_argument),
            std::move(validation_detail),
            this->extension_state().singleton_part_entries.numbering);
        return std::nullopt;
    }

    if (const auto error = this->ensure_numbering_loaded()) {
        return std::nullopt;
    }

    auto numbering_root = this->numbering.child("w:numbering");
    if (numbering_root == pugi::xml_node{}) {
        set_last_error(
            this->last_error_info, document_errc::numbering_xml_parse_failed,
            "'" + this->extension_state().singleton_part_entries.numbering +
                "' does not contain the expected w:numbering root",
            this->extension_state().singleton_part_entries.numbering);
        return std::nullopt;
    }

    const auto definition_plan =
        plan_numbering_abstract_definition(numbering_root, definition.name);
    if (!definition_plan.succeeded()) {
        set_numbering_mutation_error(
            this->last_error_info,
            this->extension_state().singleton_part_entries.numbering,
            definition_plan.status,
            "abstract numbering id has reached UINT32_MAX",
            "failed to reserve an id for the requested numbering definition");
        return std::nullopt;
    }

    auto numbering_work = pugi::xml_document{};
    if (featherdoc::detail::checked_clone_xml_document(this->numbering,
                                                       numbering_work) !=
        featherdoc::detail::xml_document_clone_status::success) {
        (void)featherdoc::detail::set_xml_document_clone_allocation_failure(
            this->last_error_info,
            this->extension_state().singleton_part_entries.numbering);
        return std::nullopt;
    }
    numbering_root = numbering_work.child("w:numbering");

    auto abstract_num_id = numbering_identifier_result{};
    try {
        abstract_num_id = ensure_custom_abstract_numbering(
            numbering_root, definition,
            definition_plan.plan->reserved_abstract_num_id);
    } catch (...) {
        (void)featherdoc::detail::set_xml_document_mutation_allocation_failure(
            this->last_error_info,
            this->extension_state().singleton_part_entries.numbering);
        return std::nullopt;
    }
    if (!abstract_num_id.succeeded()) {
        if (abstract_num_id.status ==
            numbering_mutation_status::identifier_space_exhausted) {
            set_last_error(
                this->last_error_info,
                document_errc::identifier_space_exhausted,
                "abstract numbering id has reached UINT32_MAX",
                this->extension_state().singleton_part_entries.numbering);
        } else {
            set_last_error(
                this->last_error_info,
                std::make_error_code(std::errc::not_enough_memory),
                "failed to create or update the requested numbering definition",
                this->extension_state().singleton_part_entries.numbering);
        }
        return std::nullopt;
    }

    if (const auto error = this->ensure_numbering_part_attached()) {
        return std::nullopt;
    }

    static_assert(std::is_nothrow_move_assignable_v<pugi::xml_document>);
    this->numbering = std::move(numbering_work);
    this->numbering_dirty = true;
    this->last_error_info.clear();
    return abstract_num_id.identifier;
}

std::vector<featherdoc::numbering_definition_summary>
Document::list_numbering_definitions() {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before listing numbering "
            "definitions",
            this->extension_state().singleton_part_entries.numbering);
        return {};
    }

    if (const auto error = this->ensure_numbering_loaded()) {
        return {};
    }

    const auto numbering_root = this->numbering.child("w:numbering");
    if (numbering_root == pugi::xml_node{}) {
        set_last_error(
            this->last_error_info, document_errc::numbering_xml_parse_failed,
            "'" + this->extension_state().singleton_part_entries.numbering +
                "' does not contain the expected w:numbering root",
            this->extension_state().singleton_part_entries.numbering);
        return {};
    }

    auto summaries = std::vector<featherdoc::numbering_definition_summary>{};
    for (auto abstract_num = numbering_root.child("w:abstractNum");
         abstract_num != pugi::xml_node{};
         abstract_num = abstract_num.next_sibling("w:abstractNum")) {
        if (const auto summary = summarize_numbering_definition_node(
                numbering_root, abstract_num);
            summary.has_value()) {
            summaries.push_back(*summary);
        }
    }

    this->last_error_info.clear();
    return summaries;
}

std::optional<featherdoc::numbering_definition_summary>
Document::find_numbering_definition(std::uint32_t definition_id) {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before reading numbering metadata",
            this->extension_state().singleton_part_entries.numbering);
        return std::nullopt;
    }

    if (const auto error = this->ensure_numbering_loaded()) {
        return std::nullopt;
    }

    const auto numbering_root = this->numbering.child("w:numbering");
    if (numbering_root == pugi::xml_node{}) {
        set_last_error(
            this->last_error_info, document_errc::numbering_xml_parse_failed,
            "'" + this->extension_state().singleton_part_entries.numbering +
                "' does not contain the expected w:numbering root",
            this->extension_state().singleton_part_entries.numbering);
        return std::nullopt;
    }

    const auto abstract_num =
        find_abstract_numbering_by_id(numbering_root, definition_id);
    if (abstract_num == pugi::xml_node{}) {
        set_last_error(
            this->last_error_info,
            std::make_error_code(std::errc::invalid_argument),
            "numbering definition id '" + std::to_string(definition_id) +
                "' was not found in '" +
                this->extension_state().singleton_part_entries.numbering + "'",
            this->extension_state().singleton_part_entries.numbering);
        return std::nullopt;
    }

    const auto summary =
        summarize_numbering_definition_node(numbering_root, abstract_num);
    if (!summary.has_value()) {
        set_last_error(
            this->last_error_info, document_errc::numbering_xml_parse_failed,
            "failed to summarize numbering definition metadata",
            this->extension_state().singleton_part_entries.numbering);
        return std::nullopt;
    }

    this->last_error_info.clear();
    return summary;
}

std::optional<featherdoc::numbering_instance_lookup_summary>
Document::find_numbering_instance(std::uint32_t instance_id) {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before reading numbering "
            "instance metadata",
            this->extension_state().singleton_part_entries.numbering);
        return std::nullopt;
    }

    if (const auto error = this->ensure_numbering_loaded()) {
        return std::nullopt;
    }

    const auto numbering_root = this->numbering.child("w:numbering");
    if (numbering_root == pugi::xml_node{}) {
        set_last_error(
            this->last_error_info, document_errc::numbering_xml_parse_failed,
            "'" + this->extension_state().singleton_part_entries.numbering +
                "' does not contain the expected w:numbering root",
            this->extension_state().singleton_part_entries.numbering);
        return std::nullopt;
    }

    const auto numbering_instance =
        find_numbering_instance_by_id(numbering_root, instance_id);
    if (numbering_instance == pugi::xml_node{}) {
        set_last_error(
            this->last_error_info,
            std::make_error_code(std::errc::invalid_argument),
            "numbering instance id '" + std::to_string(instance_id) +
                "' was not found in '" +
                this->extension_state().singleton_part_entries.numbering + "'",
            this->extension_state().singleton_part_entries.numbering);
        return std::nullopt;
    }

    const auto summary = summarize_numbering_instance_lookup_node(
        numbering_root, numbering_instance);
    if (!summary.has_value()) {
        set_last_error(
            this->last_error_info, document_errc::numbering_xml_parse_failed,
            "failed to summarize numbering instance metadata",
            this->extension_state().singleton_part_entries.numbering);
        return std::nullopt;
    }

    this->last_error_info.clear();
    return summary;
}

featherdoc::numbering_catalog Document::export_numbering_catalog() {
    auto catalog = featherdoc::numbering_catalog{};
    const auto summaries = this->list_numbering_definitions();
    if (this->last_error()) {
        return catalog;
    }

    catalog.definitions.reserve(summaries.size());
    for (const auto &summary : summaries) {
        auto catalog_definition = featherdoc::numbering_catalog_definition{};
        catalog_definition.definition.name = summary.name;
        catalog_definition.definition.levels = summary.levels;
        catalog_definition.instances = summary.instances;
        catalog.definitions.push_back(std::move(catalog_definition));
    }

    this->last_error_info.clear();
    return catalog;
}

featherdoc::numbering_catalog_import_summary Document::import_numbering_catalog(
    const featherdoc::numbering_catalog &catalog) {
    auto summary = featherdoc::numbering_catalog_import_summary{};
    summary.input_definition_count = catalog.definitions.size();

    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before importing numbering catalogs",
            this->extension_state().singleton_part_entries.numbering);
        return summary;
    }

    try {
        std::unordered_set<std::string> seen_names;
        for (const auto &catalog_definition : catalog.definitions) {
            auto validation_detail = std::string{};
            if (!validate_numbering_catalog_definition(catalog_definition,
                                                       validation_detail)) {
                set_last_error(
                    this->last_error_info,
                    std::make_error_code(std::errc::invalid_argument),
                    std::move(validation_detail),
                    this->extension_state().singleton_part_entries.numbering);
                return summary;
            }
            if (!seen_names.insert(catalog_definition.definition.name).second) {
                set_last_error(
                    this->last_error_info,
                    std::make_error_code(std::errc::invalid_argument),
                    "numbering catalog definition names must be unique",
                    this->extension_state().singleton_part_entries.numbering);
                return summary;
            }
        }
    } catch (const std::bad_alloc &) {
        (void)featherdoc::detail::set_xml_document_mutation_allocation_failure(
            this->last_error_info,
            this->extension_state().singleton_part_entries.numbering);
        return summary;
    }

    if (catalog.definitions.empty()) {
        this->last_error_info.clear();
        return summary;
    }

    if (const auto error = this->ensure_numbering_loaded()) {
        return summary;
    }

    auto numbering_root = this->numbering.child("w:numbering");
    if (numbering_root == pugi::xml_node{}) {
        set_last_error(
            this->last_error_info, document_errc::numbering_xml_parse_failed,
            "'" + this->extension_state().singleton_part_entries.numbering +
                "' does not contain the expected w:numbering root",
            this->extension_state().singleton_part_entries.numbering);
        return summary;
    }

    auto import_plan = numbering_catalog_import_plan_result{};
    try {
        import_plan = plan_numbering_catalog_import(numbering_root, catalog);
    } catch (const std::bad_alloc &) {
        (void)featherdoc::detail::set_xml_document_mutation_allocation_failure(
            this->last_error_info,
            this->extension_state().singleton_part_entries.numbering);
        return summary;
    }
    if (!import_plan.succeeded()) {
        const auto exhausted_detail =
            import_plan.exhausted_identifier_kind ==
                    numbering_catalog_exhausted_identifier_kind::
                        numbering_instance
                ? "numbering instance id has reached UINT32_MAX"
                : "abstract numbering id has reached UINT32_MAX";
        set_numbering_mutation_error(
            this->last_error_info,
            this->extension_state().singleton_part_entries.numbering,
            import_plan.status, exhausted_detail,
            "failed to reserve ids for the numbering catalog import");
        return summary;
    }

    auto work_numbering = pugi::xml_document{};
    if (featherdoc::detail::checked_clone_xml_document(this->numbering,
                                                       work_numbering) !=
        featherdoc::detail::xml_document_clone_status::success) {
        (void)featherdoc::detail::set_xml_document_clone_allocation_failure(
            this->last_error_info,
            this->extension_state().singleton_part_entries.numbering);
        return summary;
    }
    numbering_root = work_numbering.child("w:numbering");

    auto completed_summary = featherdoc::numbering_catalog_import_summary{};
    completed_summary.input_definition_count = catalog.definitions.size();
    try {
        completed_summary.definitions.reserve(catalog.definitions.size());
        for (std::size_t definition_index = 0U;
             definition_index < catalog.definitions.size();
             ++definition_index) {
            const auto &catalog_definition =
                catalog.definitions[definition_index];
            const auto &definition_plan =
                import_plan.plan->definitions[definition_index];
            const auto abstract_num_id = ensure_custom_abstract_numbering(
                numbering_root, catalog_definition.definition,
                definition_plan.reserved_abstract_num_id);
            if (!abstract_num_id.succeeded()) {
                if (abstract_num_id.status ==
                    numbering_mutation_status::identifier_space_exhausted) {
                    set_last_error(
                        this->last_error_info,
                        document_errc::identifier_space_exhausted,
                        "abstract numbering id has reached UINT32_MAX",
                        this->extension_state()
                            .singleton_part_entries.numbering);
                } else {
                    set_last_error(
                        this->last_error_info,
                        std::make_error_code(std::errc::not_enough_memory),
                        "failed to import numbering catalog definition",
                        this->extension_state()
                            .singleton_part_entries.numbering);
                }
                return summary;
            }

            auto imported_definition =
                featherdoc::imported_numbering_definition_summary{};
            imported_definition.name = catalog_definition.definition.name;
            imported_definition.definition_id = *abstract_num_id.identifier;

            for (std::size_t instance_index = 0U;
                 instance_index < catalog_definition.instances.size();
                 ++instance_index) {
                const auto &instance =
                    catalog_definition.instances[instance_index];
                const auto num_id = append_numbering_instance_for_abstract(
                    numbering_root, *abstract_num_id.identifier,
                    instance.level_overrides,
                    definition_plan.reserved_num_ids[instance_index]);
                if (!num_id.succeeded()) {
                    if (num_id.status ==
                        numbering_mutation_status::identifier_space_exhausted) {
                        set_last_error(
                            this->last_error_info,
                            document_errc::identifier_space_exhausted,
                            "numbering instance id has reached UINT32_MAX",
                            this->extension_state()
                                .singleton_part_entries.numbering);
                    } else {
                        set_last_error(
                            this->last_error_info,
                            std::make_error_code(std::errc::not_enough_memory),
                            "failed to import numbering catalog instance",
                            this->extension_state()
                                .singleton_part_entries.numbering);
                    }
                    return summary;
                }
                imported_definition.instance_ids.push_back(*num_id.identifier);
                ++completed_summary.imported_instance_count;
            }

            completed_summary.definitions.push_back(
                std::move(imported_definition));
            ++completed_summary.imported_definition_count;
        }
    } catch (const std::bad_alloc &) {
        (void)featherdoc::detail::set_xml_document_mutation_allocation_failure(
            this->last_error_info,
            this->extension_state().singleton_part_entries.numbering);
        return summary;
    }

    if (const auto error = this->ensure_numbering_part_attached()) {
        return summary;
    }

    static_assert(std::is_nothrow_move_assignable_v<pugi::xml_document>);
    this->numbering = std::move(work_numbering);
    this->numbering_dirty = true;
    this->last_error_info.clear();
    return completed_summary;
}
