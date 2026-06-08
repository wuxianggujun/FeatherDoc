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

std::error_code Document::save() const { return this->save_as(this->document_path); }

std::error_code Document::save_as(std::filesystem::path target_path) const {
    this->last_error_info.clear();

    if (target_path.empty()) {
        return set_last_error(this->last_error_info, document_errc::empty_path,
                              "save_as() target path must not be empty");
    }

    if (!this->is_open()) {
        return set_last_error(this->last_error_info,
                              document_errc::document_not_open,
                              "call open() successfully before save() or save_as()");
    }

    const std::filesystem::path output_file{std::move(target_path)};
    const std::filesystem::path source_file{this->document_path};

    auto temp_file = output_file;
    temp_file += ".tmp";
    auto backup_file = output_file;
    backup_file += ".bak";

    std::error_code filesystem_error;
    std::filesystem::remove(temp_file, filesystem_error);
    filesystem_error.clear();
    std::filesystem::remove(backup_file, filesystem_error);
    filesystem_error.clear();

    int output_zip_error = 0;
    zip_t *new_zip = zip_openwitherror(temp_file.string().c_str(),
                                       docx_output_compression_level, 'w',
                                       &output_zip_error);
    if (!new_zip) {
        return set_last_error(this->last_error_info,
                              document_errc::output_archive_open_failed,
                              "failed to create output archive '" +
                                  temp_file.string() + "': " +
                                  zip_error_text(output_zip_error));
    }

    std::error_code result;
    auto record_first_error =
        [&](std::error_code code, std::string detail = {},
            std::string entry_name = {},
            std::optional<std::ptrdiff_t> xml_offset = std::nullopt) {
            if (!result) {
                result = set_last_error(this->last_error_info, code,
                                        std::move(detail), std::move(entry_name),
                                        xml_offset);
            }
        };

    auto record_first_document_error =
        [&](document_errc code, std::string detail = {},
            std::string entry_name = {},
            std::optional<std::ptrdiff_t> xml_offset = std::nullopt) {
            if (!result) {
                result = set_last_error(this->last_error_info, code,
                                        std::move(detail), std::move(entry_name),
                                        xml_offset);
            }
        };

    const auto collect_referenced_relationship_ids = [&](const char *reference_name) {
        std::unordered_set<std::string> relationship_ids;
        const auto body = this->document.child("w:document").child("w:body");
        if (body == pugi::xml_node{}) {
            return relationship_ids;
        }

        const auto collect_from_section_properties = [&](pugi::xml_node section_properties) {
            for (auto reference = section_properties.child(reference_name);
                 reference != pugi::xml_node{};
                 reference = reference.next_sibling(reference_name)) {
                const auto relationship_id = reference.attribute("r:id").value();
                if (*relationship_id != '\0') {
                    relationship_ids.emplace(relationship_id);
                }
            }
        };

        for (auto child = body.first_child(); child != pugi::xml_node{};
             child = child.next_sibling()) {
            if (std::string_view{child.name()} != "w:p") {
                continue;
            }

            const auto section_properties = child.child("w:pPr").child("w:sectPr");
            if (section_properties != pugi::xml_node{}) {
                collect_from_section_properties(section_properties);
            }
        }

        if (const auto section_properties = body.child("w:sectPr");
            section_properties != pugi::xml_node{}) {
            collect_from_section_properties(section_properties);
        }

        return relationship_ids;
    };

    const auto referenced_header_relationship_ids =
        collect_referenced_relationship_ids("w:headerReference");
    const auto referenced_footer_relationship_ids =
        collect_referenced_relationship_ids("w:footerReference");

    std::vector<const xml_part_state *> active_header_parts;
    active_header_parts.reserve(this->header_parts.size());
    for (const auto &part : this->header_parts) {
        if (referenced_header_relationship_ids.contains(part->relationship_id)) {
            active_header_parts.push_back(part.get());
        }
    }

    std::vector<const xml_part_state *> active_footer_parts;
    active_footer_parts.reserve(this->footer_parts.size());
    for (const auto &part : this->footer_parts) {
        if (referenced_footer_relationship_ids.contains(part->relationship_id)) {
            active_footer_parts.push_back(part.get());
        }
    }

    const bool prune_unused_related_parts =
        active_header_parts.size() != this->header_parts.size() ||
        active_footer_parts.size() != this->footer_parts.size() ||
        !this->removed_related_part_entries.empty();

    std::unordered_set<std::string> active_override_part_names;
    for (const auto *part : active_header_parts) {
        active_override_part_names.insert(make_override_part_name(part->entry_name));
    }
    for (const auto *part : active_footer_parts) {
        active_override_part_names.insert(make_override_part_name(part->entry_name));
    }

    pugi::xml_document document_relationships_to_write;
    const pugi::xml_document *document_relationships_source = &this->document_relationships;
    const bool write_document_relationships =
        this->has_document_relationships_part &&
        (this->document_relationships_dirty || !this->has_source_archive ||
         prune_unused_related_parts);
    if (write_document_relationships && prune_unused_related_parts) {
        document_relationships_to_write.reset(this->document_relationships);
        auto relationships = document_relationships_to_write.child("Relationships");
        for (auto relationship = relationships.child("Relationship");
             relationship != pugi::xml_node{};) {
            const auto next = relationship.next_sibling("Relationship");
            const auto relationship_type =
                std::string_view{relationship.attribute("Type").value()};
            const auto relationship_id = std::string{relationship.attribute("Id").value()};

            const bool remove_header_relationship =
                relationship_type == header_relationship_type &&
                !referenced_header_relationship_ids.contains(relationship_id);
            const bool remove_footer_relationship =
                relationship_type == footer_relationship_type &&
                !referenced_footer_relationship_ids.contains(relationship_id);
            if (remove_header_relationship || remove_footer_relationship) {
                relationships.remove_child(relationship);
            }

            relationship = next;
        }

        document_relationships_source = &document_relationships_to_write;
    }

    pugi::xml_document content_types_to_write;
    const pugi::xml_document *content_types_source = &this->content_types;
    const bool write_content_types =
        this->content_types_dirty || !this->has_source_archive ||
        (prune_unused_related_parts && this->content_types_loaded);
    if (write_content_types && this->content_types_loaded && prune_unused_related_parts) {
        content_types_to_write.reset(this->content_types);
        auto types = content_types_to_write.child("Types");
        for (auto override_node = types.child("Override"); override_node != pugi::xml_node{};) {
            const auto next = override_node.next_sibling("Override");
            const auto content_type =
                std::string_view{override_node.attribute("ContentType").value()};
            const auto part_name =
                std::string{override_node.attribute("PartName").value()};

            const bool is_related_part_override =
                content_type == header_content_type || content_type == footer_content_type;
            if (is_related_part_override &&
                !active_override_part_names.contains(part_name)) {
                types.remove_child(override_node);
            }

            override_node = next;
        }

        content_types_source = &content_types_to_write;
    }

    std::unordered_set<std::string> rewritten_entries;
    rewritten_entries.insert(std::string{document_xml_entry});
    if (write_content_types) {
        rewritten_entries.insert(std::string{content_types_xml_entry});
    }
    if (write_document_relationships) {
        rewritten_entries.insert(std::string{document_relationships_xml_entry});
    }
    if (this->has_settings_part && (this->settings_dirty || !this->has_source_archive)) {
        rewritten_entries.insert(std::string{settings_xml_entry});
    }
    if (this->has_numbering_part && (this->numbering_dirty || !this->has_source_archive)) {
        rewritten_entries.insert(std::string{numbering_xml_entry});
    }
    if (this->has_styles_part && (this->styles_dirty || !this->has_source_archive)) {
        rewritten_entries.insert(std::string{styles_xml_entry});
    }
    if (this->has_footnotes_part && (this->footnotes_dirty || !this->has_source_archive)) {
        rewritten_entries.insert(std::string{footnotes_xml_entry});
    }
    if (this->has_endnotes_part && (this->endnotes_dirty || !this->has_source_archive)) {
        rewritten_entries.insert(std::string{endnotes_xml_entry});
    }
    if (this->has_comments_part && (this->comments_dirty || !this->has_source_archive)) {
        rewritten_entries.insert(std::string{comments_xml_entry});
    }
    if (this->has_comments_extended_part &&
        (this->comments_extended_dirty || !this->has_source_archive)) {
        rewritten_entries.insert(std::string{comments_extended_xml_entry});
    }
    for (const auto &part : this->header_parts) {
        rewritten_entries.insert(part->entry_name);
        if (!part->relationships_entry_name.empty()) {
            rewritten_entries.insert(part->relationships_entry_name);
        }
    }
    for (const auto &part : this->footer_parts) {
        rewritten_entries.insert(part->entry_name);
        if (!part->relationships_entry_name.empty()) {
            rewritten_entries.insert(part->relationships_entry_name);
        }
    }
    for (const auto &part : this->image_parts) {
        rewritten_entries.insert(part.entry_name);
    }
    for (const auto &entry_name : this->removed_related_part_entries) {
        rewritten_entries.insert(entry_name);
    }
    for (const auto &entry_name : this->removed_archive_entries) {
        rewritten_entries.insert(entry_name);
    }

    auto write_buffer_entry = [&](std::string_view entry_name, std::string_view entry_content) {
        if (zip_entry_open(new_zip, entry_name.data()) != 0) {
            record_first_document_error(document_errc::output_entry_open_failed,
                                        "failed to create output entry '" +
                                            std::string{entry_name} + "'",
                                        std::string{entry_name});
            return;
        }

        if (zip_entry_write(new_zip, entry_content.data(), entry_content.size()) < 0 ||
            zip_entry_close(new_zip) != 0) {
            record_first_document_error(document_errc::output_entry_write_failed,
                                        "failed to write output entry '" +
                                            std::string{entry_name} + "'",
                                            std::string{entry_name});
        }
    };

    auto write_text_entry = [&](std::string_view entry_name, std::string_view entry_content) {
        write_buffer_entry(entry_name, entry_content);
    };

    auto write_xml_entry = [&](std::string_view entry_name,
                               const pugi::xml_document &xml_document) {
        if (zip_entry_open(new_zip, entry_name.data()) != 0) {
            record_first_document_error(document_errc::output_entry_open_failed,
                                        "failed to create output entry '" +
                                            std::string{entry_name} + "'",
                                        std::string{entry_name});
            return;
        }

        xml_zip_writer writer{new_zip};
        xml_document.print(writer);
        if (writer.failed || zip_entry_close(new_zip) != 0) {
            record_first_document_error(document_errc::output_entry_write_failed,
                                        "failed to write output entry '" +
                                            std::string{entry_name} + "'",
                                        std::string{entry_name});
        }
    };

    auto write_related_xml_part = [&](const xml_part_state &part) {
        write_xml_entry(part.entry_name, part.xml);
    };

    auto write_related_part_relationships = [&](const xml_part_state &part) {
        if (!part.has_relationships_part || part.relationships_entry_name.empty()) {
            return;
        }

        write_xml_entry(part.relationships_entry_name, part.relationships);
    };

    if (zip_entry_open(new_zip, document_xml_entry.data()) != 0) {
        record_first_document_error(
            document_errc::output_document_xml_open_failed,
            "failed to create 'word/document.xml' in output archive '" +
                temp_file.string() + "'",
            std::string{document_xml_entry});
    } else {
        xml_zip_writer writer{new_zip};
        this->document.print(writer);

        if (writer.failed || zip_entry_close(new_zip) != 0) {
            record_first_document_error(
                document_errc::output_document_xml_write_failed,
                "failed while streaming 'word/document.xml' into output archive '" +
                    temp_file.string() + "'",
                std::string{document_xml_entry});
        }
    }

    if (!result && write_content_types) {
        if (this->content_types_loaded) {
            write_xml_entry(content_types_xml_entry, *content_types_source);
        } else {
            write_text_entry(content_types_xml_entry, content_types_xml);
        }
    }

    if (!result && write_document_relationships) {
        write_xml_entry(document_relationships_xml_entry, *document_relationships_source);
    }

    if (!result && this->has_settings_part &&
        (this->settings_dirty || !this->has_source_archive)) {
        write_xml_entry(settings_xml_entry, this->settings);
    }

    if (!result && this->has_numbering_part &&
        (this->numbering_dirty || !this->has_source_archive)) {
        write_xml_entry(numbering_xml_entry, this->numbering);
    }

    if (!result && this->has_styles_part &&
        (this->styles_dirty || !this->has_source_archive)) {
        write_xml_entry(styles_xml_entry, this->styles);
    }

    if (!result && this->has_footnotes_part &&
        (this->footnotes_dirty || !this->has_source_archive)) {
        write_xml_entry(footnotes_xml_entry, this->footnotes);
    }

    if (!result && this->has_endnotes_part &&
        (this->endnotes_dirty || !this->has_source_archive)) {
        write_xml_entry(endnotes_xml_entry, this->endnotes);
    }

    if (!result && this->has_comments_part &&
        (this->comments_dirty || !this->has_source_archive)) {
        write_xml_entry(comments_xml_entry, this->comments);
    }

    if (!result && this->has_comments_extended_part &&
        (this->comments_extended_dirty || !this->has_source_archive)) {
        write_xml_entry(comments_extended_xml_entry, this->comments_extended);
    }

    if (!result) {
        for (const auto *part : active_header_parts) {
            write_related_xml_part(*part);
            if (result) {
                break;
            }
            write_related_part_relationships(*part);
            if (result) {
                break;
            }
        }
    }

    if (!result) {
        for (const auto *part : active_footer_parts) {
            write_related_xml_part(*part);
            if (result) {
                break;
            }
            write_related_part_relationships(*part);
            if (result) {
                break;
            }
        }
    }

    if (!result) {
        for (const auto &part : this->image_parts) {
            write_buffer_entry(part.entry_name, part.data);
            if (result) {
                break;
            }
        }
    }

    if (!result && !this->has_source_archive) {
        for (const auto &[entry_name, entry_content] : minimal_docx_entries) {
            write_text_entry(entry_name, entry_content);
            if (result) {
                break;
            }
        }
    }

    if (!result && this->has_source_archive) {
        int source_zip_error = 0;
        zip_t *orig_zip = zip_openwitherror(source_file.string().c_str(),
                                            ZIP_DEFAULT_COMPRESSION_LEVEL, 'r',
                                            &source_zip_error);
        if (!orig_zip) {
            record_first_document_error(
                document_errc::source_archive_open_failed,
                "failed to reopen source archive '" + source_file.string() +
                    "': " + zip_error_text(source_zip_error));
        }

        if (!result && orig_zip != nullptr) {
            const auto orig_zip_entry_count = zip_entries_total(orig_zip);
            if (orig_zip_entry_count < 0) {
                record_first_document_error(
                    document_errc::source_archive_entries_failed,
                    "failed to enumerate entries in source archive '" +
                        source_file.string() + "'");
            }
            for (ssize_t i = 0; !result && i < orig_zip_entry_count; ++i) {
                if (zip_entry_openbyindex(orig_zip, static_cast<size_t>(i)) != 0) {
                    record_first_document_error(
                        document_errc::source_entry_open_failed,
                        "failed to open source archive entry at index " +
                            std::to_string(static_cast<long long>(i)));
                    break;
                }

                const char *name = zip_entry_name(orig_zip);
                if (name == nullptr) {
                    zip_entry_close(orig_zip);
                    record_first_document_error(
                        document_errc::source_entry_name_failed,
                        "failed to read source archive entry name at index " +
                            std::to_string(static_cast<long long>(i)));
                    break;
                }

                const std::string entry_name{name};
                if (rewritten_entries.find(entry_name) == rewritten_entries.end()) {
                    if (zip_entry_open(new_zip, name) != 0) {
                        zip_entry_close(orig_zip);
                        record_first_document_error(
                            document_errc::output_entry_open_failed,
                            "failed to create output entry '" + entry_name + "'",
                            entry_name);
                        break;
                    }

                    if (zip_entry_isdir(orig_zip)) {
                        if (zip_entry_close(new_zip) != 0) {
                            zip_entry_close(orig_zip);
                            record_first_document_error(
                                document_errc::output_entry_write_failed,
                                "failed to finalize output directory entry '" +
                                    entry_name + "'",
                                entry_name);
                            break;
                        }
                    } else {
                        zip_entry_copy_context context{new_zip};
                        if (zip_entry_extract(orig_zip, copy_zip_entry_chunk,
                                              &context) < 0 ||
                            context.failed || zip_entry_close(new_zip) != 0) {
                            zip_entry_close(orig_zip);
                            record_first_document_error(
                                document_errc::output_entry_write_failed,
                                "failed to copy archive entry '" + entry_name +
                                    "' into output archive",
                                entry_name);
                            break;
                        }
                    }
                }

                if (zip_entry_close(orig_zip) != 0) {
                    record_first_document_error(
                        document_errc::source_entry_close_failed,
                        "failed to close source archive entry '" + entry_name + "'",
                        entry_name);
                    break;
                }
            }
        }

        if (orig_zip != nullptr) {
            zip_close(orig_zip);
        }
    }
    zip_close(new_zip);

    if (result) {
        std::filesystem::remove(temp_file, filesystem_error);
        return result;
    }

    if (std::filesystem::exists(output_file, filesystem_error)) {
        if (filesystem_error) {
            std::filesystem::remove(temp_file, filesystem_error);
            return set_last_error(this->last_error_info, filesystem_error,
                                  "failed to inspect output target '" +
                                      output_file.string() + "'");
        }

        std::filesystem::rename(output_file, backup_file, filesystem_error);
        if (filesystem_error) {
            std::filesystem::remove(temp_file, filesystem_error);
            return set_last_error(this->last_error_info, filesystem_error,
                                  "failed to move output file '" +
                                      output_file.string() + "' to backup '" +
                                      backup_file.string() + "'");
        }
    }

    std::filesystem::rename(temp_file, output_file, filesystem_error);
    if (filesystem_error) {
        std::error_code restore_error;
        if (std::filesystem::exists(backup_file, restore_error)) {
            std::filesystem::rename(backup_file, output_file, restore_error);
        }
        std::filesystem::remove(temp_file, restore_error);
        if (restore_error) {
            return set_last_error(
                this->last_error_info, restore_error,
                "failed to restore original file after rename failure; backup was '" +
                    backup_file.string() + "'");
        }
        return set_last_error(this->last_error_info, filesystem_error,
                              "failed to replace output file '" +
                                  output_file.string() + "' with temporary file '" +
                                  temp_file.string() + "'");
    }

    std::filesystem::remove(backup_file, filesystem_error);
    if (filesystem_error) {
        return set_last_error(this->last_error_info, filesystem_error,
                              "saved document but failed to remove backup file '" +
                                  backup_file.string() + "'");
    }

    this->last_error_info.clear();
    return {};
}

bool Document::is_open() const { return this->flag_is_open; }

const document_error_info &Document::last_error() const noexcept {
    return this->last_error_info;
}
