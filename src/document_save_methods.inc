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

    std::filesystem::path temp_file;
    std::FILE *temp_stream = nullptr;
    std::uint32_t new_file_permissions = 0U;
    if (const auto reserve_error =
            reserve_unique_temp_file(output_file, temp_file, temp_stream,
                                     new_file_permissions)) {
        return set_last_error(
            this->last_error_info, document_errc::output_archive_open_failed,
            "failed to reserve a temporary output beside '" +
                featherdoc::detail::path_to_utf8(output_file) + "': " +
                reserve_error.message());
    }

    const auto temp_file_utf8 = featherdoc::detail::path_to_utf8(temp_file);
    const auto output_file_utf8 = featherdoc::detail::path_to_utf8(output_file);
    const auto source_file_utf8 = featherdoc::detail::path_to_utf8(source_file);

    int output_zip_error = 0;
    zip_t *new_zip = zip_cstream_openwitherror(
        temp_stream, docx_output_compression_level, 'w', &output_zip_error);
    if (!new_zip) {
        const auto stream_close_result = std::fclose(temp_stream);
        temp_stream = nullptr;
        std::error_code cleanup_error;
        std::filesystem::remove(temp_file, cleanup_error);
        return set_last_error(this->last_error_info,
                              document_errc::output_archive_open_failed,
                              "failed to create output archive '" +
                                  temp_file_utf8 + "': " +
                                  zip_error_text(output_zip_error) +
                                  (stream_close_result == 0
                                       ? std::string{}
                                       : std::string{
                                             "; failed to close reserved temporary file"}));
    }

    std::error_code result;
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
    const bool write_package_relationships =
        this->package_relationships_loaded &&
        (this->package_relationships_dirty || !this->has_source_archive);
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
    if (write_package_relationships) {
        rewritten_entries.insert(std::string{relationships_xml_entry});
    }
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

        const auto write_result =
            zip_entry_write(new_zip, entry_content.data(), entry_content.size());
        const auto close_result = zip_entry_close(new_zip);
        if (write_result < 0 || close_result != 0) {
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
        const auto close_result = zip_entry_close(new_zip);
        if (writer.failed || close_result != 0) {
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
                temp_file_utf8 + "'",
            std::string{document_xml_entry});
    } else {
        xml_zip_writer writer{new_zip};
        this->document.print(writer);

        const auto close_result = zip_entry_close(new_zip);
        if (writer.failed || close_result != 0) {
            record_first_document_error(
                document_errc::output_document_xml_write_failed,
                "failed while streaming 'word/document.xml' into output archive '" +
                    temp_file_utf8 + "'",
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

    if (!result && write_package_relationships) {
        write_xml_entry(relationships_xml_entry, this->package_relationships);
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
        zip_t *orig_zip = zip_openwitherror(source_file_utf8.c_str(),
                                            ZIP_DEFAULT_COMPRESSION_LEVEL, 'r',
                                            &source_zip_error);
        if (!orig_zip) {
            record_first_document_error(
                document_errc::source_archive_open_failed,
                "failed to reopen source archive '" + source_file_utf8 +
                    "': " + zip_error_text(source_zip_error));
        }

        if (!result && orig_zip != nullptr) {
            const auto orig_zip_entry_count = zip_entries_total(orig_zip);
            if (orig_zip_entry_count < 0) {
                record_first_document_error(
                    document_errc::source_archive_entries_failed,
                    "failed to enumerate entries in source archive '" +
                        source_file_utf8 + "'");
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
    const auto close_result = zip_close_ex(new_zip);
    std::error_code sync_error;
    if (!result && close_result == 0) {
        sync_error = sync_temporary_output(temp_stream, output_file,
                                           new_file_permissions);
    }
    const auto stream_close_result = std::fclose(temp_stream);
    temp_stream = nullptr;
    if (!result && sync_error) {
        record_first_document_error(
            document_errc::output_file_sync_failed,
            "failed to synchronize temporary output archive '" +
                temp_file_utf8 + "' before replacement: " +
                sync_error.message());
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
            const auto &verification_detail = verification_document.last_error();
            record_first_document_error(
                document_errc::package_repair_validation_failed,
                "strict validation of repaired output failed: " +
                    (verification_detail.detail.empty()
                         ? verification_error.message()
                         : verification_detail.detail),
                verification_detail.entry_name,
                verification_detail.xml_offset);
        }
    }

    if (result) {
        std::error_code cleanup_error;
        std::filesystem::remove(temp_file, cleanup_error);
        return result;
    }

    if (const auto replace_error =
            replace_file_atomically(temp_file, output_file)) {
        std::error_code cleanup_error;
        std::filesystem::remove(temp_file, cleanup_error);
        return set_last_error(
            this->last_error_info, document_errc::output_replace_failed,
            "failed to atomically replace output document '" + output_file_utf8 +
                "' with temporary archive '" + temp_file_utf8 + "': " +
                replace_error.message());
    }

    if (const auto directory_sync_error =
            sync_output_parent_directory(output_file)) {
        return set_last_error(
            this->last_error_info,
            document_errc::output_directory_sync_failed_after_replace,
            "atomically replaced output document '" + output_file_utf8 +
                "', but failed to synchronize its parent directory; the new "
                "document is visible but crash durability is not confirmed: " +
                directory_sync_error.message());
    }

    this->last_error_info.clear();
    return {};
}
