Document::Document() = default;

Document::Document(std::filesystem::path file_path)
    : document_path(std::move(file_path)) {}

void Document::finish_move_construction_from(Document &other) noexcept {
    if (this->xml_handle_lifetime != nullptr) {
        this->xml_handle_lifetime->invalidate();
    }
    this->rebind_internal_xml_handles();
    other.reset_moved_from_state();
}

void Document::move_assign_from(Document &other) noexcept {
    // A handle belongs to the Document object that created it, not to a DOM
    // allocation that can be transferred to another object. Invalidate both
    // generations before moving any package state so neither object's old
    // facades can retain pointers into the transferred DOM.
    if (this->xml_handle_lifetime != nullptr) {
        this->xml_handle_lifetime->invalidate();
    }
    if (other.xml_handle_lifetime != nullptr) {
        other.xml_handle_lifetime->invalidate();
        this->xml_handle_lifetime.swap(other.xml_handle_lifetime);
    } else if (this->xml_handle_lifetime != nullptr) {
        this->xml_handle_lifetime->set_opaque_owner_state({});
    }

    this->document_path = std::move(other.document_path);
    this->document = std::move(other.document);
    this->package_relationships = std::move(other.package_relationships);
    this->document_relationships = std::move(other.document_relationships);
    this->content_types = std::move(other.content_types);
    this->settings = std::move(other.settings);
    this->numbering = std::move(other.numbering);
    this->styles = std::move(other.styles);
    this->footnotes = std::move(other.footnotes);
    this->endnotes = std::move(other.endnotes);
    this->comments = std::move(other.comments);
    this->comments_extended = std::move(other.comments_extended);
    this->header_parts = std::move(other.header_parts);
    this->footer_parts = std::move(other.footer_parts);
    this->image_parts = std::move(other.image_parts);

    this->flag_is_open = other.flag_is_open;
    this->has_source_archive = other.has_source_archive;
    this->package_relationships_loaded = other.package_relationships_loaded;
    this->package_relationships_dirty = other.package_relationships_dirty;
    this->package_repair_pending = other.package_repair_pending;
    this->has_document_relationships_part =
        other.has_document_relationships_part;
    this->has_settings_part = other.has_settings_part;
    this->has_numbering_part = other.has_numbering_part;
    this->has_styles_part = other.has_styles_part;
    this->has_footnotes_part = other.has_footnotes_part;
    this->has_endnotes_part = other.has_endnotes_part;
    this->has_comments_part = other.has_comments_part;
    this->has_comments_extended_part = other.has_comments_extended_part;
    this->document_relationships_dirty = other.document_relationships_dirty;
    this->content_types_loaded = other.content_types_loaded;
    this->content_types_dirty = other.content_types_dirty;
    this->settings_loaded = other.settings_loaded;
    this->settings_dirty = other.settings_dirty;
    this->numbering_loaded = other.numbering_loaded;
    this->numbering_dirty = other.numbering_dirty;
    this->styles_loaded = other.styles_loaded;
    this->styles_dirty = other.styles_dirty;
    this->footnotes_loaded = other.footnotes_loaded;
    this->footnotes_dirty = other.footnotes_dirty;
    this->endnotes_loaded = other.endnotes_loaded;
    this->endnotes_dirty = other.endnotes_dirty;
    this->comments_loaded = other.comments_loaded;
    this->comments_dirty = other.comments_dirty;
    this->comments_extended_loaded = other.comments_extended_loaded;
    this->comments_extended_dirty = other.comments_extended_dirty;
    this->removed_related_part_entries =
        std::move(other.removed_related_part_entries);
    this->removed_archive_entries = std::move(other.removed_archive_entries);
    this->package_diagnostic_list = std::move(other.package_diagnostic_list);
    this->opened_archive_limits = other.opened_archive_limits;
    this->last_error_info = std::move(other.last_error_info);

    this->rebind_internal_xml_handles();
    other.reset_moved_from_state();
}

void Document::rebind_internal_xml_handles() {
    if (this->xml_handle_lifetime == nullptr) {
        // Moving from an already moved-from Document transfers no lifetime
        // token. Keep the noexcept move path allocation-free; an operation
        // that later exposes an XML handle recreates the token lazily.
        this->paragraph.set_parent(detail::tracked_xml_node{});
        this->detached_paragraph.set_parent(detail::tracked_xml_node{});
        this->table.set_owner(this);
        this->table.set_parent(detail::tracked_xml_node{});
        return;
    }

    const auto body = this->document.child("w:document").child("w:body");
    this->paragraph.set_parent(this->tracked_node(body));
    this->detached_paragraph.set_parent(detail::tracked_xml_node{});
    this->table.set_owner(this);
    this->table.set_parent(this->tracked_node(body));

    for (auto &part : this->header_parts) {
        part->paragraph.set_parent(
            this->tracked_node(part->xml.child("w:hdr")));
    }
    for (auto &part : this->footer_parts) {
        part->paragraph.set_parent(
            this->tracked_node(part->xml.child("w:ftr")));
    }
}

void Document::reset_moved_from_state() {
    this->document_path.clear();
    this->document.reset();
    this->package_relationships.reset();
    this->document_relationships.reset();
    this->content_types.reset();
    this->settings.reset();
    this->numbering.reset();
    this->styles.reset();
    this->footnotes.reset();
    this->endnotes.reset();
    this->comments.reset();
    this->comments_extended.reset();
    this->header_parts.clear();
    this->footer_parts.clear();
    this->image_parts.clear();
    if (this->xml_handle_lifetime != nullptr) {
        this->xml_handle_lifetime->set_opaque_owner_state({});
    }

    this->flag_is_open = false;
    this->has_source_archive = false;
    this->package_relationships_loaded = false;
    this->package_relationships_dirty = false;
    this->package_repair_pending = false;
    this->has_document_relationships_part = false;
    this->has_settings_part = false;
    this->has_numbering_part = false;
    this->has_styles_part = false;
    this->has_footnotes_part = false;
    this->has_endnotes_part = false;
    this->has_comments_part = false;
    this->has_comments_extended_part = false;
    this->document_relationships_dirty = false;
    this->content_types_loaded = false;
    this->content_types_dirty = false;
    this->settings_loaded = false;
    this->settings_dirty = false;
    this->numbering_loaded = false;
    this->numbering_dirty = false;
    this->styles_loaded = false;
    this->styles_dirty = false;
    this->footnotes_loaded = false;
    this->footnotes_dirty = false;
    this->endnotes_loaded = false;
    this->endnotes_dirty = false;
    this->comments_loaded = false;
    this->comments_dirty = false;
    this->comments_extended_loaded = false;
    this->comments_extended_dirty = false;
    this->removed_related_part_entries.clear();
    this->removed_archive_entries.clear();
    this->package_diagnostic_list.clear();
    this->opened_archive_limits = {};
    this->last_error_info.clear();
    this->paragraph.set_parent(detail::tracked_xml_node{});
    this->detached_paragraph.set_parent(detail::tracked_xml_node{});
    this->table.set_owner(this);
    this->table.set_parent(detail::tracked_xml_node{});
}

std::error_code Document::create_empty() {
    this->invalidate_xml_handles();
    this->flag_is_open = false;
    this->has_source_archive = false;
    this->package_relationships_loaded = false;
    this->package_relationships_dirty = false;
    this->package_repair_pending = false;
    this->has_document_relationships_part = false;
    this->has_settings_part = false;
    this->has_numbering_part = false;
    this->has_styles_part = false;
    this->has_footnotes_part = false;
    this->has_endnotes_part = false;
    this->has_comments_part = false;
    this->has_comments_extended_part = false;
    this->document_relationships_dirty = false;
    this->content_types_loaded = false;
    this->content_types_dirty = false;
    this->settings_loaded = false;
    this->settings_dirty = false;
    this->numbering_loaded = false;
    this->numbering_dirty = false;
    this->styles_loaded = false;
    this->styles_dirty = false;
    this->footnotes_loaded = false;
    this->footnotes_dirty = false;
    this->endnotes_loaded = false;
    this->endnotes_dirty = false;
    this->comments_loaded = false;
    this->comments_dirty = false;
    this->comments_extended_loaded = false;
    this->comments_extended_dirty = false;
    this->removed_related_part_entries.clear();
    this->removed_archive_entries.clear();
    this->extension_state() = {};
    this->package_diagnostic_list.clear();
    this->opened_archive_limits = {};
    this->document.reset();
    this->package_relationships.reset();
    this->document_relationships.reset();
    this->content_types.reset();
    this->settings.reset();
    this->numbering.reset();
    this->styles.reset();
    this->footnotes.reset();
    this->endnotes.reset();
    this->comments.reset();
    this->comments_extended.reset();
    this->header_parts.clear();
    this->footer_parts.clear();
    this->image_parts.clear();
    this->detached_paragraph.set_parent(detail::tracked_xml_node{});
    this->last_error_info.clear();

    const auto parse_result = this->document.load_buffer(
        empty_document_xml.data(), empty_document_xml.size());
    if (!parse_result) {
        this->document.reset();
        return featherdoc::detail::set_xml_parse_failure(
            this->last_error_info, parse_result,
            document_errc::document_xml_parse_failed, document_xml_entry);
    }

    const auto relationships_initialization =
        featherdoc::detail::initialize_empty_relationships_document(
            this->document_relationships);
    if (!relationships_initialization) {
        this->document.reset();
        return featherdoc::detail::set_fixed_xml_initialization_failure(
            this->last_error_info, relationships_initialization,
            document_errc::relationships_xml_parse_failed,
            document_relationships_xml_entry);
    }

    const auto content_types_parse_result = this->content_types.load_buffer(
        content_types_xml.data(), content_types_xml.size(),
        pugi::parse_default | pugi::parse_ws_pcdata);
    if (!content_types_parse_result) {
        this->document.reset();
        this->document_relationships.reset();
        return featherdoc::detail::set_xml_parse_failure(
            this->last_error_info, content_types_parse_result,
            document_errc::content_types_xml_parse_failed,
            content_types_xml_entry);
    }

    auto body = this->document.child("w:document").child("w:body");
    this->paragraph.set_parent(this->tracked_node(body));
    this->table.set_owner(this);
    this->table.set_parent(this->tracked_node(body));
    this->content_types_loaded = true;
    this->flag_is_open = true;
    return {};
}

void Document::set_path(std::filesystem::path file_path) {
    this->invalidate_xml_handles();
    this->document_path = std::move(file_path);
    this->flag_is_open = false;
    this->has_source_archive = false;
    this->package_relationships_loaded = false;
    this->package_relationships_dirty = false;
    this->package_repair_pending = false;
    this->has_document_relationships_part = false;
    this->has_settings_part = false;
    this->has_numbering_part = false;
    this->has_styles_part = false;
    this->has_footnotes_part = false;
    this->has_endnotes_part = false;
    this->has_comments_part = false;
    this->has_comments_extended_part = false;
    this->document_relationships_dirty = false;
    this->content_types_loaded = false;
    this->content_types_dirty = false;
    this->settings_loaded = false;
    this->settings_dirty = false;
    this->numbering_loaded = false;
    this->numbering_dirty = false;
    this->styles_loaded = false;
    this->styles_dirty = false;
    this->footnotes_loaded = false;
    this->footnotes_dirty = false;
    this->endnotes_loaded = false;
    this->endnotes_dirty = false;
    this->comments_loaded = false;
    this->comments_dirty = false;
    this->comments_extended_loaded = false;
    this->comments_extended_dirty = false;
    this->removed_related_part_entries.clear();
    this->removed_archive_entries.clear();
    this->extension_state() = {};
    this->package_diagnostic_list.clear();
    this->opened_archive_limits = {};
    this->document.reset();
    this->package_relationships.reset();
    this->document_relationships.reset();
    this->content_types.reset();
    this->settings.reset();
    this->numbering.reset();
    this->styles.reset();
    this->footnotes.reset();
    this->endnotes.reset();
    this->comments.reset();
    this->comments_extended.reset();
    this->header_parts.clear();
    this->footer_parts.clear();
    this->image_parts.clear();
    this->detached_paragraph.set_parent(detail::tracked_xml_node{});
    this->last_error_info.clear();
}

const std::filesystem::path &Document::path() const {
    return this->document_path;
}

std::error_code Document::open() { return this->open(document_open_options{}); }

std::error_code Document::open(const document_open_options &options) {
    this->invalidate_xml_handles();
    this->flag_is_open = false;
    this->has_source_archive = false;
    this->package_relationships_loaded = false;
    this->package_relationships_dirty = false;
    this->package_repair_pending = false;
    this->has_document_relationships_part = false;
    this->has_settings_part = false;
    this->has_numbering_part = false;
    this->has_styles_part = false;
    this->has_footnotes_part = false;
    this->has_endnotes_part = false;
    this->has_comments_part = false;
    this->has_comments_extended_part = false;
    this->document_relationships_dirty = false;
    this->content_types_loaded = false;
    this->content_types_dirty = false;
    this->settings_loaded = false;
    this->settings_dirty = false;
    this->numbering_loaded = false;
    this->numbering_dirty = false;
    this->styles_loaded = false;
    this->styles_dirty = false;
    this->footnotes_loaded = false;
    this->footnotes_dirty = false;
    this->endnotes_loaded = false;
    this->endnotes_dirty = false;
    this->comments_loaded = false;
    this->comments_dirty = false;
    this->comments_extended_loaded = false;
    this->comments_extended_dirty = false;
    this->removed_related_part_entries.clear();
    this->removed_archive_entries.clear();
    this->extension_state() = {};
    this->package_diagnostic_list.clear();
    this->extension_state().opened_validation_mode = options.validation;
    this->opened_archive_limits = options.limits;
    this->document.reset();
    this->package_relationships.reset();
    this->document_relationships.reset();
    this->content_types.reset();
    this->settings.reset();
    this->numbering.reset();
    this->styles.reset();
    this->footnotes.reset();
    this->endnotes.reset();
    this->comments.reset();
    this->comments_extended.reset();
    this->header_parts.clear();
    this->footer_parts.clear();
    this->image_parts.clear();
    this->detached_paragraph.set_parent(detail::tracked_xml_node{});
    this->last_error_info.clear();

    if (this->document_path.empty()) {
        return set_last_error(
            this->last_error_info, document_errc::empty_path,
            "set_path() or the constructor must provide a .docx path");
    }
    if (featherdoc::detail::path_has_embedded_nul(this->document_path)) {
        return set_last_error(this->last_error_info,
                              std::make_error_code(std::errc::invalid_argument),
                              "document path must not contain an embedded NUL");
    }

    std::error_code filesystem_error;
    auto source_archive_io_path =
        std::filesystem::absolute(this->document_path, filesystem_error);
    if (filesystem_error) {
        return set_last_error(
            this->last_error_info, filesystem_error,
            "failed to resolve document path: " +
                featherdoc::detail::path_to_utf8(this->document_path));
    }
    source_archive_io_path = source_archive_io_path.lexically_normal();

    const auto file_status =
        std::filesystem::status(source_archive_io_path, filesystem_error);
    if (filesystem_error) {
        if (filesystem_error == std::errc::no_such_file_or_directory) {
            return set_last_error(
                this->last_error_info,
                std::make_error_code(std::errc::no_such_file_or_directory),
                "document file does not exist: " +
                    featherdoc::detail::path_to_utf8(source_archive_io_path));
        }
        return set_last_error(
            this->last_error_info, filesystem_error,
            "failed to inspect document path: " +
                featherdoc::detail::path_to_utf8(source_archive_io_path));
    }

    if (!std::filesystem::exists(file_status)) {
        return set_last_error(
            this->last_error_info,
            std::make_error_code(std::errc::no_such_file_or_directory),
            "document file does not exist: " +
                featherdoc::detail::path_to_utf8(source_archive_io_path));
    }

    if (!std::filesystem::is_regular_file(file_status)) {
        if (std::filesystem::is_directory(file_status)) {
            return set_last_error(
                this->last_error_info,
                std::make_error_code(std::errc::is_a_directory),
                "document path points to a directory: " +
                    featherdoc::detail::path_to_utf8(source_archive_io_path));
        }
        return set_last_error(
            this->last_error_info,
            std::make_error_code(std::errc::invalid_argument),
            "document path is not a regular file: " +
                featherdoc::detail::path_to_utf8(source_archive_io_path));
    }

    const auto archive_path =
        featherdoc::detail::path_to_utf8(source_archive_io_path);
    const auto native_archive_path =
        featherdoc::detail::path_to_native_utf8(source_archive_io_path);
    void *buf = nullptr;
    size_t bufsize = 0;
    const auto opened_archive =
        featherdoc::detail::open_read_zip_archive_with_limits(
            native_archive_path, options.limits);
    zip_t *zip = opened_archive.archive;

    if (!zip) {
        if (opened_archive.zip_error == ZIP_EARCHLIMIT) {
            return featherdoc::detail::set_archive_reader_open_limit_error(
                this->last_error_info, archive_path,
                opened_archive.limit_violation);
        }
        return set_last_error(
            this->last_error_info, document_errc::archive_open_failed,
            "failed to open archive '" + archive_path +
                "': " + zip_error_text(opened_archive.zip_error));
    }
    featherdoc::detail::read_zip_archive_guard zip_guard{zip};

    auto entry_catalog = featherdoc::detail::archive_entry_catalog{};
    if (const auto limit_error =
            featherdoc::detail::validate_open_zip_archive_metadata(
                zip, options.limits, this->last_error_info, options.validation,
                &entry_catalog)) {
        return limit_error;
    }

    if (featherdoc::detail::open_archive_entry_by_package_name(
            zip, entry_catalog, document_xml_entry) != 0) {
        return set_last_error(
            this->last_error_info, document_errc::document_xml_open_failed,
            "failed to open required entry 'word/document.xml'",
            std::string{document_xml_entry});
    }

    if (zip_entry_isencrypted(zip) > 0) {
        zip_entry_close(zip);
        return set_last_error(
            this->last_error_info,
            document_errc::encrypted_document_unsupported,
            "password-protected or encrypted .docx files are not supported",
            std::string{document_xml_entry});
    }

    const auto read_result = zip_entry_read(zip, &buf, &bufsize);
    std::unique_ptr<void, decltype(&std::free)> document_buffer{buf,
                                                               &std::free};
    if (read_result < 0) {
        zip_entry_close(zip);
        if (read_result == ZIP_EPASSWD) {
            return set_last_error(
                this->last_error_info,
                document_errc::encrypted_document_unsupported,
                "password-protected or encrypted .docx files are not supported",
                std::string{document_xml_entry});
        }
        return set_last_error(
            this->last_error_info, document_errc::document_xml_read_failed,
            "failed to read XML entry from archive '" + archive_path + "'",
            std::string{document_xml_entry});
    }

    if (zip_entry_close(zip) != 0) {
        return set_last_error(
            this->last_error_info, document_errc::source_entry_close_failed,
            "failed to close required entry 'word/document.xml' after reading",
            std::string{document_xml_entry});
    }

    // Keep zip-owned buffers on the zip side to avoid allocator/ABI mismatches
    // when FeatherDoc is consumed through shared-library boundaries.
    const auto parse_result =
        this->document.load_buffer(document_buffer.get(), bufsize);
    document_buffer.reset();

    if (!parse_result) {
        this->document.reset();
        return featherdoc::detail::set_xml_parse_failure(
            this->last_error_info, parse_result,
            document_errc::document_xml_parse_failed, document_xml_entry);
    }

    // Namespace canonicalization is a read-time compatibility view. It must
    // not mark the source part dirty or alter clean-part raw-copy policy.
    const auto document_namespace_result =
        featherdoc::detail::canonicalize_wordprocessingml_part(
            this->document,
            featherdoc::detail::wordprocessingml_part_kind::main_document);
    if (!document_namespace_result) {
        this->document.reset();
        return featherdoc::detail::set_wordprocessingml_namespace_last_error(
            this->last_error_info, document_namespace_result,
            document_xml_entry);
    }

    const auto document_root = this->document.child("w:document");
    std::optional<package_diagnostic> document_diagnostic;
    if (!document_namespace_result.root_matches) {
        document_diagnostic = package_diagnostic{
            package_diagnostic_code::invalid_document_root,
            package_diagnostic_severity::error, std::string{document_xml_entry},
            "word/document.xml does not contain a document root in the "
            "supported WordprocessingML namespace",
            false};
    } else if (document_root.child("w:body") == pugi::xml_node{}) {
        document_diagnostic = package_diagnostic{
            package_diagnostic_code::missing_document_body,
            package_diagnostic_severity::warning,
            std::string{document_xml_entry},
            "word/document.xml does not contain w:body", true};
    }

    if (document_diagnostic.has_value()) {
        if (options.validation == package_validation_mode::strict) {
            this->document.reset();
            return set_last_error(
                this->last_error_info, document_errc::invalid_package_structure,
                document_diagnostic->detail, std::string{document_xml_entry});
        }
        this->package_diagnostic_list.push_back(*document_diagnostic);
    }

    const auto root_relationships_inspection = inspect_root_relationships(
        zip, entry_catalog, this->package_relationships, this->last_error_info);
    if (root_relationships_inspection.fatal_error) {
        this->document.reset();
        this->package_relationships.reset();
        return root_relationships_inspection.fatal_error;
    }
    const auto &root_relationships_diagnostic =
        root_relationships_inspection.diagnostic;
    this->package_relationships_loaded =
        featherdoc::detail::package_relationships_root(
            this->package_relationships) != pugi::xml_node{};
    if (root_relationships_diagnostic.has_value()) {
        if (options.validation == package_validation_mode::strict) {
            this->document.reset();
            this->package_relationships.reset();
            return set_last_error(this->last_error_info,
                                  document_errc::invalid_package_structure,
                                  root_relationships_diagnostic->detail,
                                  root_relationships_diagnostic->entry_name);
        }
        this->package_diagnostic_list.push_back(*root_relationships_diagnostic);
    }

    const auto related_parts = load_document_relationships_part(
        zip, this->document_relationships,
        this->has_document_relationships_part, this->last_error_info,
        entry_catalog, options.validation, this->package_diagnostic_list,
        this->extension_state().ignored_singleton_relationship_types);
    if (!related_parts.has_value()) {
        this->document.reset();
        this->flag_is_open = false;
        return this->last_error_info.code;
    }

    if (!this->has_document_relationships_part) {
        const auto relationships_initialization =
            featherdoc::detail::initialize_empty_relationships_document(
                this->document_relationships);
        if (!relationships_initialization) {
            this->document.reset();
            this->package_relationships.reset();
            this->document_relationships.reset();
            this->flag_is_open = false;
            return featherdoc::detail::set_fixed_xml_initialization_failure(
                this->last_error_info, relationships_initialization,
                document_errc::relationships_xml_parse_failed,
                document_relationships_xml_entry);
        }
    }

    const auto relationships_root =
        featherdoc::detail::package_relationships_root(
            this->document_relationships);
    const auto discover_singleton_part_entry = [&](std::string_view
                                                       relationship_type,
                                                   std::string &entry_name) {
        if (this->ignored_singleton_relationship_type(relationship_type)) {
            return;
        }
        const auto relationship = find_document_relationship_by_type(
            relationships_root, relationship_type);
        if (relationship == pugi::xml_node{} ||
            std::string_view{relationship.attribute("TargetMode").value()} ==
                "External") {
            return;
        }

        const auto target =
            std::string_view{relationship.attribute("Target").value()};
        const auto resolved_target =
            featherdoc::detail::resolve_internal_package_relationship_target(
                document_xml_entry, target);
        if (resolved_target) {
            entry_name = resolved_target.entry_name;
        }
    };
    discover_singleton_part_entry(
        settings_relationship_type,
        this->extension_state().singleton_part_entries.settings);
    discover_singleton_part_entry(
        numbering_relationship_type,
        this->extension_state().singleton_part_entries.numbering);
    discover_singleton_part_entry(
        styles_relationship_type,
        this->extension_state().singleton_part_entries.styles);
    discover_singleton_part_entry(
        footnotes_relationship_type,
        this->extension_state().singleton_part_entries.footnotes);
    discover_singleton_part_entry(
        endnotes_relationship_type,
        this->extension_state().singleton_part_entries.endnotes);
    discover_singleton_part_entry(
        comments_relationship_type,
        this->extension_state().singleton_part_entries.comments);
    discover_singleton_part_entry(
        comments_extended_relationship_type,
        this->extension_state().singleton_part_entries.comments_extended);

    std::string content_types_text;
    const auto content_types_status =
        read_zip_entry_text(zip, content_types_xml_entry, content_types_text,
                            nullptr, nullptr, entry_catalog);
    if (content_types_status == zip_entry_read_status::ok) {
        const auto content_types_parse_result = this->content_types.load_buffer(
            content_types_text.data(), content_types_text.size(),
            pugi::parse_default | pugi::parse_ws_pcdata);
        if (content_types_parse_result) {
            this->content_types_loaded = true;
            if (const auto content_type_diagnostic =
                    inspect_main_content_type(this->content_types)) {
                const auto diagnostic_code = content_type_diagnostic->code;
                const bool unsafe_content_type_declaration =
                    diagnostic_code == package_diagnostic_code::
                                           duplicate_content_type_override ||
                    diagnostic_code == package_diagnostic_code::
                                           duplicate_content_type_default ||
                    diagnostic_code ==
                        package_diagnostic_code::
                            ambiguous_main_document_content_type ||
                    diagnostic_code == package_diagnostic_code::
                                           invalid_content_type_part_name ||
                    diagnostic_code == package_diagnostic_code::
                                           invalid_content_type_extension ||
                    diagnostic_code == package_diagnostic_code::
                                           invalid_content_type_media_type;
                if (options.validation == package_validation_mode::strict ||
                    unsafe_content_type_declaration) {
                    this->document.reset();
                    this->package_relationships.reset();
                    this->content_types.reset();
                    return set_last_error(
                        this->last_error_info,
                        document_errc::invalid_package_structure,
                        content_type_diagnostic->detail,
                        content_type_diagnostic->entry_name);
                }
                this->package_diagnostic_list.push_back(
                    *content_type_diagnostic);
            }
        } else {
            this->content_types.reset();
            if (content_types_parse_result.status ==
                pugi::status_out_of_memory) {
                this->document.reset();
                this->package_relationships.reset();
                return featherdoc::detail::set_xml_parse_allocation_failure(
                    this->last_error_info, content_types_xml_entry);
            }
            if (options.validation == package_validation_mode::strict) {
                this->document.reset();
                return featherdoc::detail::set_xml_parse_failure(
                    this->last_error_info, content_types_parse_result,
                    document_errc::content_types_xml_parse_failed,
                    content_types_xml_entry);
            }
            this->package_diagnostic_list.push_back(package_diagnostic{
                package_diagnostic_code::malformed_content_types,
                package_diagnostic_severity::error,
                std::string{content_types_xml_entry},
                content_types_parse_result.description(), false});
        }
    } else if (options.validation == package_validation_mode::strict) {
        this->document.reset();
        return set_last_error(
            this->last_error_info, document_errc::content_types_xml_read_failed,
            "required [Content_Types].xml part is missing or unreadable",
            std::string{content_types_xml_entry});
    } else {
        this->package_diagnostic_list.push_back(package_diagnostic{
            package_diagnostic_code::missing_content_types,
            content_types_status == zip_entry_read_status::missing
                ? package_diagnostic_severity::warning
                : package_diagnostic_severity::error,
            std::string{content_types_xml_entry},
            content_types_status == zip_entry_read_status::missing
                ? "required [Content_Types].xml part is missing"
                : "required [Content_Types].xml part is unreadable",
            content_types_status == zip_entry_read_status::missing});
    }

    for (const auto &related_part : *related_parts) {
        auto part = std::make_unique<xml_part_state>();
        if (related_part.relationship_ids.empty()) {
            continue;
        }
        part->relationship_id = related_part.relationship_ids.front();
        part->entry_name = related_part.entry_name;
        auto &relationship_alias_ids =
            this->extension_state()
                .related_part_relationship_alias_ids[part->entry_name];
        relationship_alias_ids.assign(related_part.relationship_ids.begin() + 1,
                                      related_part.relationship_ids.end());
        part->relationships_entry_name =
            make_part_relationships_entry(part->entry_name);

        if (const auto error = load_related_xml_part(
                zip, part->entry_name, part->xml, this->last_error_info,
                options.limits, entry_catalog,
                related_part.relationship_type == header_relationship_type
                    ? featherdoc::detail::wordprocessingml_part_kind::header
                    : featherdoc::detail::wordprocessingml_part_kind::footer)) {
            this->document.reset();
            this->document_relationships.reset();
            this->content_types.reset();
            this->settings.reset();
            this->numbering.reset();
            this->styles.reset();
            this->header_parts.clear();
            this->footer_parts.clear();
            this->image_parts.clear();
            this->flag_is_open = false;
            return error;
        }

        if (const auto error = load_optional_relationships_part(
                zip, part->relationships_entry_name, part->relationships,
                part->has_relationships_part, this->last_error_info,
                options.limits, entry_catalog, options.validation,
                this->package_diagnostic_list)) {
            this->document.reset();
            this->document_relationships.reset();
            this->content_types.reset();
            this->settings.reset();
            this->numbering.reset();
            this->styles.reset();
            this->header_parts.clear();
            this->footer_parts.clear();
            this->image_parts.clear();
            this->flag_is_open = false;
            return error;
        }

        if (related_part.relationship_type == header_relationship_type) {
            this->header_parts.push_back(std::move(part));
        } else if (related_part.relationship_type == footer_relationship_type) {
            this->footer_parts.push_back(std::move(part));
        }
    }

    if (const auto close_error = featherdoc::detail::close_read_archive(
            zip_guard, this->last_error_info,
            "failed to close document archive after loading package parts",
            archive_path)) {
        this->document.reset();
        this->package_relationships.reset();
        this->document_relationships.reset();
        this->content_types.reset();
        this->settings.reset();
        this->numbering.reset();
        this->styles.reset();
        this->header_parts.clear();
        this->footer_parts.clear();
        this->image_parts.clear();
        this->flag_is_open = false;
        return close_error;
    }

    auto body = this->document.child("w:document").child("w:body");
    this->paragraph.set_parent(this->tracked_node(body));
    this->table.set_owner(this);
    this->table.set_parent(this->tracked_node(body));
    for (const auto &entry : entry_catalog.records) {
        if (!entry.is_directory) {
            this->extension_state().source_archive_part_identities.insert(
                entry.identity_key);
        }
    }
    this->extension_state().source_archive_snapshot =
        featherdoc::detail::source_archive_fingerprint_from_catalog(
            entry_catalog);
    this->extension_state().source_archive_io_path =
        std::move(source_archive_io_path);
    this->has_source_archive = true;
    this->flag_is_open = true;
    this->last_error_info.clear();
    return {};
}

#include "document_package_repair_methods.inc"
#include "document_save_methods.inc"
bool Document::is_open() const { return this->flag_is_open; }

const document_error_info &Document::last_error() const noexcept {
    return this->last_error_info;
}
