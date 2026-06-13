Document::Document() = default;

Document::Document(std::filesystem::path file_path)
    : document_path(std::move(file_path)) {}

std::error_code Document::create_empty() {
    this->flag_is_open = false;
    this->has_source_archive = false;
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
    this->document.reset();
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
    this->detached_paragraph.set_parent({});
    this->last_error_info.clear();

    const auto parse_result =
        this->document.load_buffer(empty_document_xml.data(), empty_document_xml.size());
    if (!parse_result) {
        this->document.reset();
        return set_last_error(
            this->last_error_info, document_errc::document_xml_parse_failed,
            parse_result.description(), std::string{document_xml_entry},
            parse_result.offset >= 0 ? std::optional<std::ptrdiff_t>{parse_result.offset}
                                     : std::nullopt);
    }

    if (!initialize_empty_relationships_document(this->document_relationships)) {
        this->document.reset();
        return set_last_error(
            this->last_error_info, document_errc::relationships_xml_parse_failed,
            "failed to initialize default relationships XML",
            std::string{document_relationships_xml_entry});
    }

    const auto content_types_parse_result =
        this->content_types.load_buffer(content_types_xml.data(), content_types_xml.size());
    if (!content_types_parse_result) {
        this->document.reset();
        this->document_relationships.reset();
        return set_last_error(
            this->last_error_info, document_errc::content_types_xml_parse_failed,
            content_types_parse_result.description(),
            std::string{content_types_xml_entry},
            content_types_parse_result.offset >= 0
                ? std::optional<std::ptrdiff_t>{content_types_parse_result.offset}
                : std::nullopt);
    }

    auto body = this->document.child("w:document").child("w:body");
    this->paragraph.set_parent(body);
    this->table.set_owner(this);
    this->table.set_parent(body);
    this->content_types_loaded = true;
    this->flag_is_open = true;
    return {};
}

void Document::set_path(std::filesystem::path file_path) {
    this->document_path = std::move(file_path);
    this->flag_is_open = false;
    this->has_source_archive = false;
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
    this->document.reset();
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
    this->detached_paragraph.set_parent({});
    this->last_error_info.clear();
}

const std::filesystem::path &Document::path() const { return this->document_path; }

std::error_code Document::open() {
    this->flag_is_open = false;
    this->has_source_archive = false;
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
    this->document.reset();
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
    this->detached_paragraph.set_parent({});
    this->last_error_info.clear();

    if (this->document_path.empty()) {
        return set_last_error(this->last_error_info, document_errc::empty_path,
                              "set_path() or the constructor must provide a .docx path");
    }

    std::error_code filesystem_error;
    const auto file_status =
        std::filesystem::status(this->document_path, filesystem_error);
    if (filesystem_error) {
        if (filesystem_error == std::errc::no_such_file_or_directory) {
            return set_last_error(this->last_error_info,
                                  std::make_error_code(
                                      std::errc::no_such_file_or_directory),
                                  "document file does not exist: " +
                                      this->document_path.string());
        }
        return set_last_error(this->last_error_info, filesystem_error,
                              "failed to inspect document path: " +
                                  this->document_path.string());
    }

    if (!std::filesystem::exists(file_status)) {
        return set_last_error(this->last_error_info,
                              std::make_error_code(
                                  std::errc::no_such_file_or_directory),
                              "document file does not exist: " +
                                  this->document_path.string());
    }

    if (!std::filesystem::is_regular_file(file_status)) {
        if (std::filesystem::is_directory(file_status)) {
            return set_last_error(this->last_error_info,
                                  std::make_error_code(std::errc::is_a_directory),
                                  "document path points to a directory: " +
                                      this->document_path.string());
        }
        return set_last_error(this->last_error_info,
                              std::make_error_code(std::errc::invalid_argument),
                              "document path is not a regular file: " +
                                  this->document_path.string());
    }

    const auto archive_path = this->document_path.string();
    void *buf = nullptr;
    size_t bufsize = 0;
    int zip_error = 0;

    zip_t *zip = zip_openwitherror(archive_path.c_str(),
                                   ZIP_DEFAULT_COMPRESSION_LEVEL, 'r', &zip_error);

    if (!zip) {
        return set_last_error(this->last_error_info,
                              document_errc::archive_open_failed,
                              "failed to open archive '" + archive_path +
                                  "': " + zip_error_text(zip_error));
    }

    if (zip_entry_open(zip, document_xml_entry.data()) != 0) {
        zip_close(zip);
        return set_last_error(this->last_error_info,
                              document_errc::document_xml_open_failed,
                              "failed to open required entry 'word/document.xml'",
                              std::string{document_xml_entry});
    }

    if (zip_entry_isencrypted(zip) > 0) {
        zip_entry_close(zip);
        zip_close(zip);
        return set_last_error(
            this->last_error_info, document_errc::encrypted_document_unsupported,
            "password-protected or encrypted .docx files are not supported",
            std::string{document_xml_entry});
    }

    const auto read_result = zip_entry_read(zip, &buf, &bufsize);
    if (read_result < 0) {
        zip_entry_close(zip);
        zip_close(zip);
        if (read_result == ZIP_EPASSWD) {
            return set_last_error(
                this->last_error_info, document_errc::encrypted_document_unsupported,
                "password-protected or encrypted .docx files are not supported",
                std::string{document_xml_entry});
        }
        return set_last_error(this->last_error_info,
                              document_errc::document_xml_read_failed,
                              "failed to read XML entry from archive '" +
                                  archive_path + "'",
                              std::string{document_xml_entry});
    }

    zip_entry_close(zip);

    // Keep zip-owned buffers on the zip side to avoid allocator/ABI mismatches
    // when FeatherDoc is consumed through shared-library boundaries.
    const auto parse_result = this->document.load_buffer(buf, bufsize);
    std::free(buf);
    buf = nullptr;

    this->flag_is_open = static_cast<bool>(parse_result);
    if (!this->flag_is_open) {
        zip_close(zip);
        this->document.reset();
        return set_last_error(
            this->last_error_info, document_errc::document_xml_parse_failed,
            parse_result.description(), std::string{document_xml_entry},
            parse_result.offset >= 0 ? std::optional<std::ptrdiff_t>{parse_result.offset}
                                     : std::nullopt);
    }

    const auto related_parts = load_document_relationships_part(
        zip, this->document_relationships, this->has_document_relationships_part,
        this->last_error_info);
    if (!related_parts.has_value()) {
        zip_close(zip);
        this->document.reset();
        this->flag_is_open = false;
        return this->last_error_info.code;
    }

    if (!this->has_document_relationships_part) {
        initialize_empty_relationships_document(this->document_relationships);
    }

    std::string content_types_text;
    const auto content_types_status =
        read_zip_entry_text(zip, content_types_xml_entry, content_types_text);
    if (content_types_status == zip_entry_read_status::ok) {
        const auto content_types_parse_result =
            this->content_types.load_buffer(content_types_text.data(), content_types_text.size());
        if (content_types_parse_result) {
            this->content_types_loaded = true;
        } else {
            this->content_types.reset();
        }
    }

    for (const auto &related_part : *related_parts) {
        auto part = std::make_unique<xml_part_state>();
        part->relationship_id = related_part.relationship_id;
        part->entry_name = related_part.entry_name;
        part->relationships_entry_name = make_part_relationships_entry(part->entry_name);

        if (const auto error =
                load_related_xml_part(zip, part->entry_name, part->xml, this->last_error_info)) {
            zip_close(zip);
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
                part->has_relationships_part, this->last_error_info)) {
            zip_close(zip);
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

    zip_close(zip);

    auto body = this->document.child("w:document").child("w:body");
    this->paragraph.set_parent(body);
    this->table.set_owner(this);
    this->table.set_parent(body);
    this->has_source_archive = true;
    this->last_error_info.clear();
    return {};
}

#include "document_save_methods.inc"
bool Document::is_open() const { return this->flag_is_open; }

const document_error_info &Document::last_error() const noexcept {
    return this->last_error_info;
}
