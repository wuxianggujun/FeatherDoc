std::error_code Document::save() const {
    const auto &source_archive_io_path =
        this->extension_state().source_archive_io_path;
    return this->save_as(this->has_source_archive &&
                                 !source_archive_io_path.empty()
                             ? source_archive_io_path
                             : this->document_path);
}

std::error_code Document::save_as(std::filesystem::path target_path) const try {
    this->last_error_info.clear();

    if (target_path.empty()) {
        return set_last_error(this->last_error_info, document_errc::empty_path,
                              "save_as() target path must not be empty");
    }
    if (featherdoc::detail::path_has_embedded_nul(target_path)) {
        return set_last_error(
            this->last_error_info,
            std::make_error_code(std::errc::invalid_argument),
            "save_as() target path must not contain an embedded NUL");
    }

    if (!this->is_open()) {
        return set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() successfully before save() or save_as()");
    }

    std::error_code output_path_error;
    auto output_file =
        std::filesystem::absolute(target_path, output_path_error);
    if (output_path_error) {
        return set_last_error(
            this->last_error_info, output_path_error,
            "failed to resolve save_as() target path: " +
                featherdoc::detail::path_to_utf8(target_path));
    }
    output_file = output_file.lexically_normal();

    const auto validate_dirty_relationships_part =
        [&](const pugi::xml_document &relationships_document,
            bool has_relationships_part, bool relationships_dirty,
            std::string_view entry_name) -> std::error_code {
        if (!has_relationships_part || !relationships_dirty ||
            featherdoc::detail::inspect_package_relationships_document(
                relationships_document) ==
                featherdoc::detail::package_relationships_document_state::
                    valid) {
            return {};
        }

        return set_last_error(
            this->last_error_info, document_errc::invalid_package_structure,
            "refusing to save a modified relationships part without a valid "
            "Relationships root in the package relationships namespace",
            std::string{entry_name});
    };

    if (const auto validation_error = validate_dirty_relationships_part(
            this->package_relationships, this->package_relationships_loaded,
            this->package_relationships_dirty, relationships_xml_entry)) {
        return validation_error;
    }
    if (const auto validation_error = validate_dirty_relationships_part(
            this->document_relationships, this->has_document_relationships_part,
            this->document_relationships_dirty,
            document_relationships_xml_entry)) {
        return validation_error;
    }
    const auto validate_related_part_relationships =
        [&](const auto &parts) -> std::error_code {
        for (const auto &part : parts) {
            if (const auto validation_error = validate_dirty_relationships_part(
                    part->relationships, part->has_relationships_part,
                    part->relationships_dirty,
                    part->relationships_entry_name)) {
                return validation_error;
            }
        }
        return {};
    };
    if (const auto validation_error =
            validate_related_part_relationships(this->header_parts)) {
        return validation_error;
    }
    if (const auto validation_error =
            validate_related_part_relationships(this->footer_parts)) {
        return validation_error;
    }

    const std::filesystem::path source_file =
        this->has_source_archive
            ? this->extension_state().source_archive_io_path
            : this->document_path;
    if (this->has_source_archive && source_file.empty()) {
        return set_last_error(
            this->last_error_info, document_errc::invalid_package_structure,
            "the opened document no longer has a bound source archive path");
    }
    auto output_parent_directory = output_file.parent_path();
    if (output_parent_directory.empty()) {
        output_parent_directory = ".";
    }
    const bool replaces_source_archive =
        this->has_source_archive &&
        output_path_replaces_source_archive(output_file, source_file);

    temporary_output_transaction temporary_output;
    auto &temp_file = temporary_output.path_slot();
    auto &temp_stream = temporary_output.stream_slot();
    std::uint32_t new_file_permissions = 0U;
    if (const auto reserve_error = reserve_unique_temp_file(
            output_file, temp_file, temp_stream, new_file_permissions)) {
        return set_last_error(
            this->last_error_info, document_errc::output_archive_open_failed,
            "failed to reserve a temporary output beside '" +
                featherdoc::detail::path_to_utf8(output_file) +
                "': " + reserve_error.message());
    }
    temporary_output.arm_reserved_file_cleanup();

    const auto temp_file_utf8 = featherdoc::detail::path_to_utf8(temp_file);
    const auto output_file_utf8 = featherdoc::detail::path_to_utf8(output_file);
    const auto source_file_utf8 = featherdoc::detail::path_to_utf8(source_file);
    const auto source_file_native_utf8 =
        featherdoc::detail::path_to_native_utf8(source_file);

    int output_zip_error = 0;
    zip_t *new_zip = zip_cstream_openwitherror(
        temp_stream, docx_output_compression_level, 'w', &output_zip_error);
    if (!new_zip) {
        const auto stream_close_result = temporary_output.close_stream();
        return set_last_error(
            this->last_error_info, document_errc::output_archive_open_failed,
            "failed to create output archive '" + temp_file_utf8 +
                "': " + zip_error_text(output_zip_error) +
                (stream_close_result == 0
                     ? std::string{}
                     : std::string{
                           "; failed to close reserved temporary file"}));
    }
    temporary_output.attach_archive(new_zip);

    std::error_code result;
    auto record_first_document_error =
        [&](document_errc code, std::string detail = {},
            std::string entry_name = {},
            std::optional<std::ptrdiff_t> xml_offset = std::nullopt) {
            if (!result) {
                result = set_last_error(this->last_error_info, code,
                                        std::move(detail),
                                        std::move(entry_name), xml_offset);
            }
        };

    const auto collect_referenced_relationship_ids =
        [&](const char *reference_name) {
            std::unordered_set<std::string> relationship_ids;
            const auto body =
                this->document.child("w:document").child("w:body");
            if (body == pugi::xml_node{}) {
                return relationship_ids;
            }

            const auto collect_from_section_properties =
                [&](pugi::xml_node section_properties) {
                    for (auto reference =
                             section_properties.child(reference_name);
                         reference != pugi::xml_node{};
                         reference = reference.next_sibling(reference_name)) {
                        const auto relationship_id =
                            reference.attribute("r:id").value();
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

                const auto section_properties =
                    child.child("w:pPr").child("w:sectPr");
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
        const bool referenced = std::ranges::any_of(
            referenced_header_relationship_ids,
            [&](const auto &relationship_id) {
                return related_part_has_relationship_id(*part, relationship_id);
            });
        if (referenced) {
            active_header_parts.push_back(part.get());
        }
    }

    std::vector<const xml_part_state *> active_footer_parts;
    active_footer_parts.reserve(this->footer_parts.size());
    for (const auto &part : this->footer_parts) {
        const bool referenced = std::ranges::any_of(
            referenced_footer_relationship_ids,
            [&](const auto &relationship_id) {
                return related_part_has_relationship_id(*part, relationship_id);
            });
        if (referenced) {
            active_footer_parts.push_back(part.get());
        }
    }

    const auto collect_part_relationship_ids = [this](
                                                   const xml_part_state &part) {
        auto relationship_ids = std::unordered_set<std::string>{};
        if (!part.relationship_id.empty()) {
            relationship_ids.insert(part.relationship_id);
        }
        for (const auto &alias_id :
             this->related_part_relationship_alias_ids(part)) {
            if (!alias_id.empty()) {
                relationship_ids.insert(alias_id);
            }
        }
        return relationship_ids;
    };
    auto active_header_relationship_ids = std::unordered_set<std::string>{};
    for (const auto *part : active_header_parts) {
        const auto part_ids = collect_part_relationship_ids(*part);
        active_header_relationship_ids.insert(part_ids.begin(), part_ids.end());
    }
    auto active_footer_relationship_ids = std::unordered_set<std::string>{};
    for (const auto *part : active_footer_parts) {
        const auto part_ids = collect_part_relationship_ids(*part);
        active_footer_relationship_ids.insert(part_ids.begin(), part_ids.end());
    }

    const bool prune_unused_related_parts =
        active_header_parts.size() != this->header_parts.size() ||
        active_footer_parts.size() != this->footer_parts.size() ||
        !this->removed_related_part_entries.empty();

    std::unordered_set<std::string> active_override_part_identities;
    const auto remember_active_override = [&](std::string_view entry_name) {
        const auto part_identity =
            featherdoc::detail::content_type_part_name_identity(
                make_override_part_name(entry_name));
        if (part_identity.has_value()) {
            active_override_part_identities.insert(*part_identity);
        }
    };
    for (const auto *part : active_header_parts) {
        remember_active_override(part->entry_name);
    }
    for (const auto *part : active_footer_parts) {
        remember_active_override(part->entry_name);
    }

    pugi::xml_document document_relationships_to_write;
    const pugi::xml_document *document_relationships_source =
        &this->document_relationships;
    const bool write_document_relationships =
        this->has_document_relationships_part &&
        (this->document_relationships_dirty || !this->has_source_archive ||
         prune_unused_related_parts);
    if (write_document_relationships && prune_unused_related_parts) {
        if (featherdoc::detail::checked_clone_xml_document(
                this->document_relationships,
                document_relationships_to_write) !=
            featherdoc::detail::xml_document_clone_status::success) {
            result = featherdoc::detail::
                set_xml_document_clone_allocation_failure(
                    this->last_error_info,
                    document_relationships_xml_entry);
        } else {
            auto relationships = featherdoc::detail::package_relationships_root(
                document_relationships_to_write);
            for (auto relationship = featherdoc::detail::
                     first_package_relationship(relationships);
                 relationship != pugi::xml_node{};) {
                const auto next = featherdoc::detail::
                    next_package_relationship(relationship);
                const auto relationship_type =
                    std::string_view{relationship.attribute("Type").value()};
                const auto relationship_id =
                    std::string{relationship.attribute("Id").value()};

                const bool remove_header_relationship =
                    relationship_type == header_relationship_type &&
                    !active_header_relationship_ids.contains(relationship_id);
                const bool remove_footer_relationship =
                    relationship_type == footer_relationship_type &&
                    !active_footer_relationship_ids.contains(relationship_id);
                if (remove_header_relationship ||
                    remove_footer_relationship) {
                    relationships.remove_child(relationship);
                }

                relationship = next;
            }

            document_relationships_source = &document_relationships_to_write;
        }
    }

    pugi::xml_document content_types_to_write;
    const pugi::xml_document *content_types_source = &this->content_types;
    const bool content_types_are_mutation_safe =
        this->content_types_loaded &&
        featherdoc::detail::package_content_types_document_allows_mutation(
            this->content_types);
    const bool write_content_types =
        this->content_types_dirty || !this->has_source_archive ||
        (prune_unused_related_parts && content_types_are_mutation_safe);
    const bool write_package_relationships =
        this->package_relationships_loaded &&
        (this->package_relationships_dirty || !this->has_source_archive);
    if (!result && write_content_types && content_types_are_mutation_safe &&
        prune_unused_related_parts) {
        if (featherdoc::detail::checked_clone_xml_document(
                this->content_types, content_types_to_write) !=
            featherdoc::detail::xml_document_clone_status::success) {
            result = featherdoc::detail::
                set_xml_document_clone_allocation_failure(
                    this->last_error_info, content_types_xml_entry);
        } else {
            auto types = featherdoc::detail::package_content_types_root(
                content_types_to_write);
            for (auto override_node = featherdoc::detail::
                     first_package_content_type_override(types);
                 override_node != pugi::xml_node{};) {
                const auto next = featherdoc::detail::
                    next_package_content_type_override(override_node);
                const auto content_type = std::string_view{
                    override_node.attribute("ContentType").value()};
                const auto part_name =
                    std::string{override_node.attribute("PartName").value()};
                const auto part_identity = featherdoc::detail::
                    content_type_part_name_identity(part_name);

                const bool is_related_part_override =
                    content_type == header_content_type ||
                    content_type == footer_content_type;
                if (is_related_part_override &&
                    (!part_identity.has_value() ||
                     !active_override_part_identities.contains(
                         *part_identity))) {
                    types.remove_child(override_node);
                }

                override_node = next;
            }

            content_types_source = &content_types_to_write;
        }
    }

    std::unordered_set<std::string> rewritten_entries;
    rewritten_entries.insert(std::string{document_xml_entry});
    if (write_package_relationships) {
        rewritten_entries.insert(std::string{relationships_xml_entry});
    }
    if (write_content_types) {
        rewritten_entries.insert(std::string{content_types_xml_entry});
    }
    if (write_document_relationships) {
        rewritten_entries.insert(std::string{document_relationships_xml_entry});
    }
    if (this->has_settings_part &&
        (this->settings_dirty || !this->has_source_archive)) {
        rewritten_entries.insert(this->extension_state().singleton_part_entries.settings);
    }
    if (this->has_numbering_part &&
        (this->numbering_dirty || !this->has_source_archive)) {
        rewritten_entries.insert(this->extension_state().singleton_part_entries.numbering);
    }
    if (this->has_styles_part &&
        (this->styles_dirty || !this->has_source_archive)) {
        rewritten_entries.insert(this->extension_state().singleton_part_entries.styles);
    }
    if (this->has_footnotes_part &&
        (this->footnotes_dirty || !this->has_source_archive)) {
        rewritten_entries.insert(this->extension_state().singleton_part_entries.footnotes);
    }
    if (this->has_endnotes_part &&
        (this->endnotes_dirty || !this->has_source_archive)) {
        rewritten_entries.insert(this->extension_state().singleton_part_entries.endnotes);
    }
    if (this->has_comments_part &&
        (this->comments_dirty || !this->has_source_archive)) {
        rewritten_entries.insert(this->extension_state().singleton_part_entries.comments);
    }
    if (this->has_comments_extended_part &&
        (this->comments_extended_dirty || !this->has_source_archive)) {
        rewritten_entries.insert(
            this->extension_state().singleton_part_entries.comments_extended);
    }
    for (const auto &part : this->header_parts) {
        rewritten_entries.insert(part->entry_name);
        if (part->has_relationships_part &&
            !part->relationships_entry_name.empty()) {
            rewritten_entries.insert(part->relationships_entry_name);
        }
    }
    for (const auto &part : this->footer_parts) {
        rewritten_entries.insert(part->entry_name);
        if (part->has_relationships_part &&
            !part->relationships_entry_name.empty()) {
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

    std::unordered_set<std::string> rewritten_entry_identities;
    rewritten_entry_identities.reserve(rewritten_entries.size());
    for (const auto &entry_name : rewritten_entries) {
        const auto canonical_entry =
            featherdoc::detail::canonical_package_part_key(entry_name);
        if (!canonical_entry) {
            record_first_document_error(
                document_errc::invalid_package_structure,
                "cannot save an internally tracked package part with an "
                "invalid name: " +
                    std::string{featherdoc::detail::
                                    package_relationship_target_error_message(
                                        canonical_entry.error)},
                entry_name);
            break;
        }
        rewritten_entry_identities.insert(
            featherdoc::detail::package_part_identity(
                canonical_entry.entry_name));
    }

    std::unordered_set<std::string> output_entry_identities;
    output_entry_identities.reserve(rewritten_entries.size() + 8U);
    std::set<std::string> output_part_identities;
    const auto reserve_output_entry_name =
        [&](std::string_view entry_name,
            bool directory_entry) -> std::optional<std::string> {
        const auto canonical_entry =
            featherdoc::detail::canonical_package_part_key(entry_name);
        if (!canonical_entry || canonical_entry.entry_name.empty() ||
            canonical_entry.entry_name.ends_with('/') != directory_entry) {
            record_first_document_error(
                document_errc::invalid_package_structure,
                "cannot write an output package entry with an invalid name",
                std::string{entry_name});
            return std::nullopt;
        }

        const auto identity = featherdoc::detail::package_part_identity(
            canonical_entry.entry_name);
        if (!output_entry_identities.insert(identity).second) {
            record_first_document_error(
                document_errc::invalid_package_structure,
                "refusing to write duplicate logical output package entry '" +
                    canonical_entry.entry_name + "'",
                canonical_entry.entry_name);
            return std::nullopt;
        }
        if (!directory_entry && identity != "[content_types].xml") {
            if (featherdoc::detail::
                    package_part_identity_has_derivation_conflict(
                        output_part_identities, identity)) {
                record_first_document_error(
                    document_errc::invalid_package_structure,
                    "refusing to write package part names where one is "
                    "derived from the other by appending path segments",
                    canonical_entry.entry_name);
                return std::nullopt;
            }
            output_part_identities.insert(identity);
        }
        return canonical_entry.entry_name;
    };

    auto write_buffer_entry = [&](std::string_view entry_name,
                                  std::string_view entry_content) {
        const auto reserved_entry_name =
            reserve_output_entry_name(entry_name, false);
        if (!reserved_entry_name.has_value()) {
            return;
        }
        const auto &terminated_entry_name = *reserved_entry_name;
        if (zip_entry_open(new_zip, terminated_entry_name.c_str()) != 0) {
            record_first_document_error(document_errc::output_entry_open_failed,
                                        "failed to create output entry '" +
                                            terminated_entry_name + "'",
                                        terminated_entry_name);
            return;
        }

        const auto write_result = zip_entry_write(new_zip, entry_content.data(),
                                                  entry_content.size());
        const auto close_result = zip_entry_close(new_zip);
        if (write_result < 0 || close_result != 0) {
            record_first_document_error(
                document_errc::output_entry_write_failed,
                "failed to write output entry '" + terminated_entry_name + "'",
                terminated_entry_name);
        }
    };

    auto write_text_entry = [&](std::string_view entry_name,
                                std::string_view entry_content) {
        write_buffer_entry(entry_name, entry_content);
    };

    auto write_xml_entry = [&](std::string_view entry_name,
                               const pugi::xml_document &xml_document) {
        const auto reserved_entry_name =
            reserve_output_entry_name(entry_name, false);
        if (!reserved_entry_name.has_value()) {
            return;
        }
        const auto &terminated_entry_name = *reserved_entry_name;
        if (zip_entry_open(new_zip, terminated_entry_name.c_str()) != 0) {
            record_first_document_error(document_errc::output_entry_open_failed,
                                        "failed to create output entry '" +
                                            terminated_entry_name + "'",
                                        terminated_entry_name);
            return;
        }

        xml_zip_writer writer{new_zip,
                              this->opened_archive_limits.max_xml_part_bytes};
        xml_document.print(writer, "", pugi::format_raw,
                           pugi::encoding_utf8);
        const auto close_result = zip_entry_close(new_zip);
        if (writer.limit_exceeded) {
            record_first_document_error(
                document_errc::archive_limit_exceeded,
                "serialized output XML entry '" + terminated_entry_name +
                    "' is " + std::to_string(writer.attempted_bytes) +
                    " bytes; limit is " +
                    std::to_string(writer.max_bytes),
                terminated_entry_name);
        } else if (writer.failed || close_result != 0) {
            record_first_document_error(
                document_errc::output_entry_write_failed,
                "failed to write output entry '" + terminated_entry_name + "'",
                terminated_entry_name);
        }
    };

    auto write_related_xml_part = [&](const xml_part_state &part) {
        write_xml_entry(part.entry_name, part.xml);
    };

    auto write_related_part_relationships = [&](const xml_part_state &part) {
        if (!part.has_relationships_part ||
            part.relationships_entry_name.empty()) {
            return;
        }

        write_xml_entry(part.relationships_entry_name, part.relationships);
    };

    if (!result) {
        const auto document_output_entry =
            reserve_output_entry_name(document_xml_entry, false);
        if (document_output_entry.has_value()) {
            if (zip_entry_open(new_zip, document_output_entry->c_str()) != 0) {
                record_first_document_error(
                    document_errc::output_document_xml_open_failed,
                    "failed to create 'word/document.xml' in output archive '" +
                        temp_file_utf8 + "'",
                    std::string{document_xml_entry});
            } else {
                xml_zip_writer writer{
                    new_zip, this->opened_archive_limits.max_xml_part_bytes};
                this->document.print(writer, "", pugi::format_raw,
                                     pugi::encoding_utf8);

                const auto close_result = zip_entry_close(new_zip);
                if (writer.limit_exceeded) {
                    record_first_document_error(
                        document_errc::archive_limit_exceeded,
                        "serialized output XML entry '" +
                            std::string{document_xml_entry} + "' is " +
                            std::to_string(writer.attempted_bytes) +
                            " bytes; limit is " +
                            std::to_string(writer.max_bytes),
                        std::string{document_xml_entry});
                } else if (writer.failed || close_result != 0) {
                    record_first_document_error(
                        document_errc::output_document_xml_write_failed,
                        "failed while streaming 'word/document.xml' into "
                        "output archive '" +
                            temp_file_utf8 + "'",
                        std::string{document_xml_entry});
                }
            }
        }
    }

    if (!result && write_content_types) {
        if (this->content_types_loaded) {
            write_xml_entry(content_types_xml_entry, *content_types_source);
        } else {
            write_text_entry(content_types_xml_entry, content_types_xml);
        }
    }

    if (!result && write_package_relationships) {
        write_xml_entry(relationships_xml_entry, this->package_relationships);
    }

    if (!result && write_document_relationships) {
        write_xml_entry(document_relationships_xml_entry,
                        *document_relationships_source);
    }

    if (!result && this->has_settings_part &&
        (this->settings_dirty || !this->has_source_archive)) {
        write_xml_entry(this->extension_state().singleton_part_entries.settings, this->settings);
    }

    if (!result && this->has_numbering_part &&
        (this->numbering_dirty || !this->has_source_archive)) {
        write_xml_entry(this->extension_state().singleton_part_entries.numbering,
                        this->numbering);
    }

    if (!result && this->has_styles_part &&
        (this->styles_dirty || !this->has_source_archive)) {
        write_xml_entry(this->extension_state().singleton_part_entries.styles, this->styles);
    }

    if (!result && this->has_footnotes_part &&
        (this->footnotes_dirty || !this->has_source_archive)) {
        write_xml_entry(this->extension_state().singleton_part_entries.footnotes,
                        this->footnotes);
    }

    if (!result && this->has_endnotes_part &&
        (this->endnotes_dirty || !this->has_source_archive)) {
        write_xml_entry(this->extension_state().singleton_part_entries.endnotes, this->endnotes);
    }

    if (!result && this->has_comments_part &&
        (this->comments_dirty || !this->has_source_archive)) {
        write_xml_entry(this->extension_state().singleton_part_entries.comments, this->comments);
    }

    if (!result && this->has_comments_extended_part &&
        (this->comments_extended_dirty || !this->has_source_archive)) {
        write_xml_entry(this->extension_state().singleton_part_entries.comments_extended,
                        this->comments_extended);
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
        const auto opened_source_archive =
            featherdoc::detail::open_read_zip_archive_with_limits(
                source_file_native_utf8, this->opened_archive_limits);
        zip_t *orig_zip = opened_source_archive.archive;
        featherdoc::detail::read_zip_archive_guard orig_zip_guard{orig_zip};
        if (!orig_zip) {
            result =
                featherdoc::detail::set_reopened_source_archive_open_error(
                    this->last_error_info, source_file_utf8,
                    opened_source_archive, "copying unchanged package parts");
        }

        featherdoc::detail::archive_entry_catalog source_catalog;
        if (!result && orig_zip != nullptr) {
            result =
                featherdoc::detail::validate_reopened_source_archive_metadata(
                    orig_zip, this->opened_archive_limits,
                    this->last_error_info,
                    this->extension_state().opened_validation_mode,
                    this->extension_state().source_archive_snapshot,
                    source_file_utf8, &source_catalog);
        }

        if (!result && orig_zip != nullptr) {
            for (const auto &source_entry : source_catalog.records) {
                if (rewritten_entry_identities.contains(
                        source_entry.identity_key)) {
                    continue;
                }

                if (zip_entry_openbyindex(orig_zip, source_entry.zip_index) !=
                    0) {
                    record_first_document_error(
                        document_errc::source_entry_open_failed,
                        "failed to open source archive entry at index " +
                            std::to_string(source_entry.zip_index),
                        source_entry.physical_name);
                    break;
                }

                const auto reserved_entry_name = reserve_output_entry_name(
                    source_entry.canonical_key, source_entry.is_directory);
                if (!reserved_entry_name.has_value()) {
                    (void)zip_entry_close(orig_zip);
                    break;
                }
                const auto &entry_name = *reserved_entry_name;
                if (zip_entry_open(new_zip, entry_name.c_str()) != 0) {
                    zip_entry_close(orig_zip);
                    record_first_document_error(
                        document_errc::output_entry_open_failed,
                        "failed to create output entry '" + entry_name + "'",
                        entry_name);
                    break;
                }

                if (source_entry.is_directory) {
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
                    const auto extract_result = zip_entry_extract(
                        orig_zip, copy_zip_entry_chunk, &context);
                    const auto close_result = zip_entry_close(new_zip);
                    if (extract_result < 0 || context.failed ||
                        close_result != 0) {
                        zip_entry_close(orig_zip);
                        record_first_document_error(
                            document_errc::output_entry_write_failed,
                            "failed to copy archive entry '" + entry_name +
                                "' into output archive",
                            entry_name);
                        break;
                    }
                }

                if (zip_entry_close(orig_zip) != 0) {
                    record_first_document_error(
                        document_errc::source_entry_close_failed,
                        "failed to close source archive entry '" + entry_name +
                            "'",
                        entry_name);
                    break;
                }
            }
        }

        if (orig_zip != nullptr) {
            if (!result) {
                result = featherdoc::detail::close_read_archive(
                    orig_zip_guard, this->last_error_info,
                    "failed to close source archive after copying package "
                    "entries",
                    source_file_utf8);
            }
        }
    }
    const auto close_result = temporary_output.close_archive();
    std::error_code sync_error;
    if (!result && close_result == 0) {
        sync_error = sync_temporary_output(temp_stream, output_file,
                                           new_file_permissions);
    }
    const auto stream_close_result = temporary_output.close_stream();
    if (!result && sync_error) {
        record_first_document_error(
            document_errc::output_file_sync_failed,
            "failed to synchronize temporary output archive '" +
                temp_file_utf8 +
                "' before replacement: " + sync_error.message());
    } else if (!result && (close_result != 0 || stream_close_result != 0)) {
        record_first_document_error(
            document_errc::output_archive_finalize_failed,
            "failed to finalize temporary output archive '" + temp_file_utf8 +
                "': " +
                (close_result != 0
                     ? zip_error_text(close_result)
                     : std::string{"failed to flush or close temporary file"}));
    }

    if (!result && this->package_repair_pending) {
        document_open_options verification_options;
        verification_options.limits = this->opened_archive_limits;
        Document verification_document(temp_file);
        if (const auto verification_error =
                verification_document.open(verification_options)) {
            const auto &verification_detail =
                verification_document.last_error();
            record_first_document_error(
                document_errc::package_repair_validation_failed,
                "strict validation of repaired output failed: " +
                    (verification_detail.detail.empty()
                         ? verification_error.message()
                         : verification_detail.detail),
                verification_detail.entry_name, verification_detail.xml_offset);
        }
    }

    featherdoc::detail::source_archive_fingerprint replacement_snapshot;
    if (!result && replaces_source_archive) {
        const auto &snapshot_limits = this->opened_archive_limits;
        const auto temp_file_native_utf8 =
            featherdoc::detail::path_to_native_utf8(temp_file);
        const auto reopened_output =
            featherdoc::detail::open_read_zip_archive_with_limits(
                temp_file_native_utf8, snapshot_limits);
        zip_t *output_archive = reopened_output.archive;
        featherdoc::detail::read_zip_archive_guard output_archive_guard{
            output_archive};
        featherdoc::document_error_info output_validation_error;
        featherdoc::detail::archive_entry_catalog output_catalog;
        if (output_archive == nullptr) {
            if (reopened_output.zip_error == ZIP_EARCHLIMIT) {
                (void)featherdoc::detail::set_archive_reader_open_limit_error(
                    output_validation_error, temp_file_utf8,
                    reopened_output.limit_violation);
                record_first_document_error(
                    document_errc::archive_limit_exceeded,
                    "refusing to replace the source archive because the "
                    "finalized output exceeds the limits used to open it: " +
                        output_validation_error.detail);
            } else {
                record_first_document_error(
                    document_errc::output_archive_finalize_failed,
                    "failed to reopen finalized temporary output archive '" +
                        temp_file_utf8 + "' for source snapshot capture: " +
                        zip_error_text(reopened_output.zip_error));
            }
        } else if (const auto validation_error =
                       featherdoc::detail::validate_open_zip_archive_metadata(
                           output_archive, snapshot_limits,
                           output_validation_error,
                           this->extension_state().opened_validation_mode,
                           &output_catalog)) {
            const bool limit_exceeded =
                output_validation_error.code ==
                featherdoc::make_error_code(
                    document_errc::archive_limit_exceeded);
            record_first_document_error(
                limit_exceeded ? document_errc::archive_limit_exceeded
                               : document_errc::output_archive_finalize_failed,
                (limit_exceeded
                     ? std::string{"refusing to replace the source archive "
                                   "because the finalized output exceeds the "
                                   "limits used to open it: "}
                     : std::string{"finalized temporary output archive "
                                   "metadata is invalid: "}) +
                    (output_validation_error.detail.empty()
                         ? validation_error.message()
                         : output_validation_error.detail),
                output_validation_error.entry_name);
        } else {
            replacement_snapshot =
                featherdoc::detail::source_archive_fingerprint_from_catalog(
                    output_catalog);
        }
        if (output_archive != nullptr) {
            if (!result) {
                if (const auto output_close_error =
                        featherdoc::detail::close_read_archive(
                            output_archive_guard,
                            output_validation_error,
                            "failed to close temporary output archive after "
                            "source snapshot capture",
                            temp_file_utf8)) {
                    record_first_document_error(
                        document_errc::output_archive_finalize_failed,
                        output_close_error.message());
                }
            }
        }
    }

    // Recheck the source pathname immediately before replacing it. The first
    // check protects copied data because it is performed on the same open ZIP
    // handle; this second check prevents knowingly overwriting a concurrent
    // replacement that happened while the output was being assembled.
    if (!result && replaces_source_archive) {
        const auto reopened_source =
            featherdoc::detail::open_read_zip_archive_with_limits(
                source_file_native_utf8, this->opened_archive_limits);
        zip_t *source_archive = reopened_source.archive;
        featherdoc::detail::read_zip_archive_guard source_archive_guard{
            source_archive};
        if (source_archive == nullptr) {
            result =
                featherdoc::detail::set_reopened_source_archive_open_error(
                    this->last_error_info, source_file_utf8, reopened_source,
                    "performing the final pre-replacement consistency check");
        } else {
            featherdoc::detail::archive_entry_catalog source_catalog;
            result =
                featherdoc::detail::validate_reopened_source_archive_metadata(
                    source_archive, this->opened_archive_limits,
                    this->last_error_info,
                    this->extension_state().opened_validation_mode,
                    this->extension_state().source_archive_snapshot,
                    source_file_utf8, &source_catalog);
            if (!result) {
                result = featherdoc::detail::close_read_archive(
                    source_archive_guard, this->last_error_info,
                    "failed to close source archive after the pre-replacement "
                    "consistency check",
                    source_file_utf8);
            }
        }
    }

    if (result) {
        return result;
    }

    if (const auto replace_error =
            replace_file_atomically(temp_file, output_file)) {
        return set_last_error(
            this->last_error_info, document_errc::output_replace_failed,
            "failed to atomically replace output document '" +
                output_file_utf8 + "' with temporary archive '" +
                temp_file_utf8 + "': " + replace_error.message());
    }
    temporary_output.commit();

    if (replaces_source_archive) {
        this->extension_state().source_archive_snapshot =
            std::move(replacement_snapshot);
    }

    if (const auto directory_sync_error =
            sync_output_parent_directory(output_parent_directory)) {
        return set_post_replace_directory_sync_error(
            this->last_error_info, output_file_utf8, directory_sync_error);
    }

    this->last_error_info.clear();
    return {};
} catch (const std::bad_alloc &) {
    return featherdoc::detail::set_xml_document_allocation_failure(
        this->last_error_info, "document save ran out of memory");
}
