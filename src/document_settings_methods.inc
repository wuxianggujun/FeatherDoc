std::optional<bool> Document::inspect_update_fields_on_open_enabled() {
    const auto previous_error = this->last_error_info;
    if (this->ensure_settings_loaded()) {
        this->last_error_info = previous_error;
        return std::nullopt;
    }

    const auto settings_root = this->settings.child("w:settings");
    if (settings_root == pugi::xml_node{}) {
        this->last_error_info = previous_error;
        return std::nullopt;
    }

    const auto enabled =
        read_on_off_value(settings_root.child("w:updateFields"))
            .value_or(false);
    this->last_error_info = previous_error;
    return enabled;
}

std::optional<bool> Document::update_fields_on_open_enabled() {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before reading updateFields",
            this->extension_state().singleton_part_entries.settings);
        return std::nullopt;
    }

    if (this->ensure_settings_loaded()) {
        return std::nullopt;
    }

    const auto settings_root = this->settings.child("w:settings");
    if (settings_root == pugi::xml_node{}) {
        set_last_error(
            this->last_error_info, document_errc::settings_xml_parse_failed,
            "'" + this->extension_state().singleton_part_entries.settings +
                "' does not contain the expected w:settings root",
            this->extension_state().singleton_part_entries.settings);
        return std::nullopt;
    }

    this->last_error_info.clear();
    return read_on_off_value(settings_root.child("w:updateFields"))
        .value_or(false);
}

std::error_code Document::ensure_content_types_loaded() {
    const auto ensure_mutation_safe = [&]() -> std::error_code {
        const auto inspection = featherdoc::detail::
            inspect_package_content_types_document_structure(
                this->content_types);
        if (inspection.state ==
            featherdoc::detail::package_content_types_document_state::valid) {
            return {};
        }
        return set_last_error(
            this->last_error_info, document_errc::invalid_package_structure,
            inspection.detail.empty()
                ? "cannot mutate an invalid [Content_Types].xml part"
                : std::string{inspection.detail},
            std::string{content_types_xml_entry});
    };

    if (this->content_types_loaded) {
        return ensure_mutation_safe();
    }

    this->content_types.reset();
    if (!this->has_source_archive) {
        const auto parse_result = this->content_types.load_buffer(
            content_types_xml.data(), content_types_xml.size(),
            pugi::parse_default | pugi::parse_ws_pcdata);
        if (!parse_result) {
            return featherdoc::detail::set_xml_parse_failure(
                this->last_error_info, parse_result,
                document_errc::content_types_xml_parse_failed,
                content_types_xml_entry);
        }

        this->content_types_loaded = true;
        return ensure_mutation_safe();
    }

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
            "loading content type metadata",
            std::string{content_types_xml_entry});
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

    std::string content_types_text;
    const auto read_status = read_zip_entry_text(
        source_zip, content_types_xml_entry, content_types_text,
        &this->opened_archive_limits, &this->last_error_info, entry_catalog);

    if (read_status == zip_entry_read_status::limit_exceeded) {
        return this->last_error_info.code;
    }

    if (read_status != zip_entry_read_status::ok) {
        return set_last_error(
            this->last_error_info, document_errc::content_types_xml_read_failed,
            "failed to read required entry '[Content_Types].xml'",
            std::string{content_types_xml_entry});
    }

    if (const auto close_error = featherdoc::detail::close_read_archive(
            source_zip_guard, this->last_error_info,
            "failed to close source archive after loading [Content_Types].xml",
            std::string{content_types_xml_entry})) {
        return close_error;
    }

    const auto parse_result = this->content_types.load_buffer(
        content_types_text.data(), content_types_text.size(),
        pugi::parse_default | pugi::parse_ws_pcdata);
    if (!parse_result) {
        return featherdoc::detail::set_xml_parse_failure(
            this->last_error_info, parse_result,
            document_errc::content_types_xml_parse_failed,
            content_types_xml_entry);
    }

    this->content_types_loaded = true;
    return ensure_mutation_safe();
}

std::error_code Document::ensure_settings_loaded() {
    if (this->settings_loaded) {
        return {};
    }

    this->settings.reset();
    this->has_settings_part = false;

    const auto load_default_settings = [&]() -> std::error_code {
        const auto parse_result = this->settings.load_buffer(
            empty_settings_xml.data(), empty_settings_xml.size());
        if (!parse_result) {
            return featherdoc::detail::set_xml_parse_failure(
                this->last_error_info, parse_result,
                document_errc::settings_xml_parse_failed,
                this->extension_state().singleton_part_entries.settings);
        }

        this->settings_loaded = true;
        return {};
    };

    if (!this->has_source_archive) {
        return load_default_settings();
    }

    const auto relationships = featherdoc::detail::package_relationships_root(
        this->document_relationships);
    if (relationships == pugi::xml_node{} &&
        !this->ignored_singleton_relationship_type(
            settings_relationship_type)) {
        return set_last_error(this->last_error_info,
                              document_errc::relationships_xml_parse_failed,
                              "word/_rels/document.xml.rels does not contain a "
                              "Relationships root",
                              std::string{document_relationships_xml_entry});
    }

    const auto settings_relationship =
        this->ignored_singleton_relationship_type(settings_relationship_type)
            ? pugi::xml_node{}
            : find_document_relationship_by_type(relationships,
                                                 settings_relationship_type);
    if (settings_relationship == pugi::xml_node{} &&
        this->ignored_singleton_relationship_type(settings_relationship_type)) {
        return load_default_settings();
    }

    if (settings_relationship != pugi::xml_node{}) {
        const auto target =
            std::string_view{settings_relationship.attribute("Target").value()};
        if (target.empty()) {
            return set_last_error(
                this->last_error_info, document_errc::settings_xml_read_failed,
                "settings relationship does not contain a Target attribute",
                std::string{document_relationships_xml_entry});
        }
        if (std::string_view{
                settings_relationship.attribute("TargetMode").value()} ==
            "External") {
            return set_last_error(
                this->last_error_info, document_errc::invalid_package_structure,
                "settings relationship must not use TargetMode=External",
                std::string{document_relationships_xml_entry});
        }
        const auto resolved_settings_target =
            featherdoc::detail::resolve_internal_package_relationship_target(
                document_xml_entry, target);
        if (!resolved_settings_target) {
            return set_last_error(
                this->last_error_info, document_errc::invalid_package_structure,
                std::string{featherdoc::detail::
                                package_relationship_target_error_message(
                                    resolved_settings_target.error)},
                std::string{document_relationships_xml_entry});
        }
        this->extension_state().singleton_part_entries.settings =
            resolved_settings_target.entry_name;
    }
    const auto &settings_entry_name =
        this->extension_state().singleton_part_entries.settings;

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
            "loading settings metadata", settings_entry_name);
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

    if (settings_relationship == pugi::xml_node{} &&
        featherdoc::detail::find_archive_entry_record(
            entry_catalog, settings_entry_name) == nullptr) {
        if (const auto close_error = featherdoc::detail::close_read_archive(
                source_zip_guard, this->last_error_info,
                "failed to close source archive after checking for an "
                "orphaned settings part",
                settings_entry_name)) {
            return close_error;
        }
        return load_default_settings();
    }

    std::string settings_text;
    const auto read_status = read_zip_entry_text(
        source_zip, settings_entry_name, settings_text,
        &this->opened_archive_limits, &this->last_error_info, entry_catalog);

    if (read_status == zip_entry_read_status::limit_exceeded) {
        return this->last_error_info.code;
    }

    if (read_status != zip_entry_read_status::ok) {
        return set_last_error(this->last_error_info,
                              document_errc::settings_xml_read_failed,
                              "failed to read required settings part '" +
                                  settings_entry_name + "'",
                              settings_entry_name);
    }

    if (const auto close_error = featherdoc::detail::close_read_archive(
            source_zip_guard, this->last_error_info,
            "failed to close source archive after loading settings",
            settings_entry_name)) {
        return close_error;
    }

    const auto parse_result =
        this->settings.load_buffer(settings_text.data(), settings_text.size());
    if (!parse_result) {
        return featherdoc::detail::set_xml_parse_failure(
            this->last_error_info, parse_result,
            document_errc::settings_xml_parse_failed, settings_entry_name);
    }

    const auto namespace_result =
        featherdoc::detail::canonicalize_wordprocessingml_part(
            this->settings,
            featherdoc::detail::wordprocessingml_part_kind::settings);
    if (!namespace_result) {
        this->settings.reset();
        return featherdoc::detail::set_wordprocessingml_namespace_last_error(
            this->last_error_info, namespace_result, settings_entry_name);
    }
    if (!namespace_result.root_matches) {
        this->settings.reset();
        return featherdoc::detail::
            set_wordprocessingml_root_mismatch_last_error(
                this->last_error_info, namespace_result,
                featherdoc::detail::wordprocessingml_part_kind::settings,
                settings_entry_name);
    }

    this->settings_loaded = true;
    this->has_settings_part = true;
    return {};
}

std::error_code Document::ensure_settings_part_attached() {
    if (this->ignored_singleton_relationship_type(settings_relationship_type)) {
        return set_last_error(
            this->last_error_info, document_errc::invalid_package_structure,
            "cannot attach settings while its source relationship is invalid",
            std::string{document_relationships_xml_entry});
    }
    if (!featherdoc::detail::package_relationships_document_allows_mutation(
            this->document_relationships,
            this->has_document_relationships_part)) {
        return set_last_error(
            this->last_error_info, document_errc::invalid_package_structure,
            "cannot attach settings while document relationships are invalid",
            std::string{document_relationships_xml_entry});
    }
    if (const auto error = this->ensure_settings_loaded()) {
        return error;
    }
    const auto had_settings_part = this->has_settings_part;

    if (const auto error = this->ensure_content_types_loaded()) {
        return error;
    }

    auto relationship_target = std::string{};
    try {
        relationship_target = make_document_relationship_target(
            this->extension_state().singleton_part_entries.settings);
    } catch (...) {
        return featherdoc::detail::set_xml_document_mutation_allocation_failure(
            this->last_error_info, document_relationships_xml_entry);
    }
    auto override_part_name = std::string{};
    try {
        override_part_name = make_override_part_name(
            this->extension_state().singleton_part_entries.settings);
    } catch (...) {
        return featherdoc::detail::set_xml_document_mutation_allocation_failure(
            this->last_error_info, content_types_xml_entry);
    }
    const auto options = featherdoc::detail::singleton_part_attachment_options{
        settings_relationship_type,
        relationship_target,
        override_part_name,
        settings_content_type,
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
    this->has_settings_part = true;
    if (!had_settings_part) {
        this->settings_dirty = true;
    }
    return {};
}

bool Document::enable_update_fields_on_open() {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before enabling updateFields",
            this->extension_state().singleton_part_entries.settings);
        return false;
    }
    if (const auto error = this->ensure_settings_loaded()) {
        return false;
    }

    auto settings_work = pugi::xml_document{};
    if (featherdoc::detail::checked_clone_xml_document(this->settings,
                                                       settings_work) !=
        featherdoc::detail::xml_document_clone_status::success) {
        (void)featherdoc::detail::set_xml_document_clone_allocation_failure(
            this->last_error_info,
            this->extension_state().singleton_part_entries.settings);
        return false;
    }

    auto settings_root = settings_work.child("w:settings");
    if (settings_root == pugi::xml_node{}) {
        set_last_error(
            this->last_error_info, document_errc::settings_xml_parse_failed,
            "'" + this->extension_state().singleton_part_entries.settings +
                "' does not contain the expected w:settings root",
            this->extension_state().singleton_part_entries.settings);
        return false;
    }

    auto update_fields = settings_root.child("w:updateFields");
    if (update_fields == pugi::xml_node{}) {
        update_fields = featherdoc::detail::checked_append_xml_element(
            settings_root, "w:updateFields");
    }
    if (update_fields == pugi::xml_node{} ||
        !featherdoc::detail::checked_set_xml_attribute_value(update_fields,
                                                             "w:val", "1")) {
        (void)featherdoc::detail::set_xml_document_mutation_allocation_failure(
            this->last_error_info,
            this->extension_state().singleton_part_entries.settings);
        return false;
    }

    if (const auto error = this->ensure_settings_part_attached()) {
        return false;
    }

    static_assert(std::is_nothrow_move_assignable_v<pugi::xml_document>);
    this->settings = std::move(settings_work);
    this->settings_dirty = true;
    this->last_error_info.clear();
    return true;
}

bool Document::clear_update_fields_on_open() {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before clearing updateFields",
            this->extension_state().singleton_part_entries.settings);
        return false;
    }
    if (const auto error = this->ensure_settings_loaded()) {
        return false;
    }

    auto settings_root = this->settings.child("w:settings");
    if (settings_root == pugi::xml_node{}) {
        set_last_error(
            this->last_error_info, document_errc::settings_xml_parse_failed,
            "'" + this->extension_state().singleton_part_entries.settings +
                "' does not contain the expected w:settings root",
            this->extension_state().singleton_part_entries.settings);
        return false;
    }

    if (settings_root.child("w:updateFields") != pugi::xml_node{}) {
        settings_root.remove_child("w:updateFields");
        this->settings_dirty = true;
    }

    this->last_error_info.clear();
    return true;
}
