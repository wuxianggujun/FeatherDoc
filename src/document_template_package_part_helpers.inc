struct related_part_entry_lookup_result {
    bool valid{true};
    std::optional<std::string> entry_name;
};

auto invalid_related_part_target(
    featherdoc::document_error_info &last_error_info, std::string detail)
    -> related_part_entry_lookup_result {
    set_last_error(
        last_error_info, featherdoc::document_errc::invalid_package_structure,
        std::move(detail), std::string{document_relationships_xml_entry});
    return {false, std::nullopt};
}

auto related_part_entry_for_type(
    const pugi::xml_document &relationships_document,
    std::string_view relationship_type, std::string_view fallback_entry,
    bool ignore_relationship, featherdoc::document_error_info &last_error_info)
    -> related_part_entry_lookup_result {
    if (ignore_relationship) {
        return {true, std::nullopt};
    }
    const auto relationships =
        featherdoc::detail::package_relationships_root(relationships_document);
    for (auto relationship =
             featherdoc::detail::first_package_relationship(relationships);
         relationship != pugi::xml_node{};
         relationship =
             featherdoc::detail::next_package_relationship(relationship)) {
        if (std::string_view{relationship.attribute("Type").value()} !=
            relationship_type) {
            continue;
        }
        if (std::string_view{relationship.attribute("TargetMode").value()} ==
            "External") {
            return invalid_related_part_target(
                last_error_info,
                "internal Word relationship must not use TargetMode=External");
        }
        const auto target =
            std::string_view{relationship.attribute("Target").value()};
        const auto resolved =
            featherdoc::detail::resolve_internal_package_relationship_target(
                document_xml_entry, target);
        if (!resolved) {
            return invalid_related_part_target(
                last_error_info,
                std::string{featherdoc::detail::
                                package_relationship_target_error_message(
                                    resolved.error)});
        }
        return {true, resolved.entry_name};
    }
    return {true, std::string{fallback_entry}};
}

auto optional_related_part_entry_for_type(
    const pugi::xml_document &relationships_document,
    std::string_view relationship_type, bool ignore_relationship,
    featherdoc::document_error_info &last_error_info)
    -> related_part_entry_lookup_result {
    if (ignore_relationship) {
        return {true, std::nullopt};
    }
    const auto relationships =
        featherdoc::detail::package_relationships_root(relationships_document);
    for (auto relationship =
             featherdoc::detail::first_package_relationship(relationships);
         relationship != pugi::xml_node{};
         relationship =
             featherdoc::detail::next_package_relationship(relationship)) {
        if (std::string_view{relationship.attribute("Type").value()} !=
            relationship_type) {
            continue;
        }
        if (std::string_view{relationship.attribute("TargetMode").value()} ==
            "External") {
            return invalid_related_part_target(
                last_error_info,
                "internal Word relationship must not use TargetMode=External");
        }
        const auto target =
            std::string_view{relationship.attribute("Target").value()};
        const auto resolved =
            featherdoc::detail::resolve_internal_package_relationship_target(
                document_xml_entry, target);
        if (!resolved) {
            return invalid_related_part_target(
                last_error_info,
                std::string{featherdoc::detail::
                                package_relationship_target_error_message(
                                    resolved.error)});
        }
        return {true, resolved.entry_name};
    }
    return {true, std::nullopt};
}

auto read_docx_entry_text(const std::filesystem::path &document_path,
                          std::string_view entry_name, std::string &content,
                          bool &entry_exists,
                          featherdoc::document_error_info &last_error_info,
                          const featherdoc::archive_limits *xml_limits,
                          featherdoc::package_validation_mode validation_mode,
                          const featherdoc::detail::source_archive_fingerprint
                              *expected_source_fingerprint) -> bool {
    entry_exists = false;
    const featherdoc::archive_limits effective_limits =
        xml_limits != nullptr ? *xml_limits : featherdoc::archive_limits{};
    const auto archive_path = featherdoc::detail::path_to_utf8(document_path);
    const auto native_archive_path =
        featherdoc::detail::path_to_native_utf8(document_path);
    const auto opened_archive =
        featherdoc::detail::open_read_zip_archive_with_limits(
            native_archive_path, effective_limits, 0);
    zip_t *zip = opened_archive.archive;
    if (zip == nullptr) {
        if (expected_source_fingerprint != nullptr) {
            (void)featherdoc::detail::set_reopened_source_archive_open_error(
                last_error_info, archive_path, opened_archive,
                "loading an optional review XML part", std::string{entry_name});
            return false;
        }
        if (opened_archive.zip_error == ZIP_EARCHLIMIT) {
            (void)featherdoc::detail::set_archive_reader_open_limit_error(
                last_error_info, archive_path, opened_archive.limit_violation);
            return false;
        }
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::io_error),
                       "failed to open source DOCX archive", archive_path);
        return false;
    }
    featherdoc::detail::read_zip_archive_guard zip_guard{zip};

    featherdoc::detail::archive_entry_catalog catalog;
    const auto validation_error =
        expected_source_fingerprint == nullptr
            ? featherdoc::detail::validate_open_zip_archive_metadata(
                  zip, effective_limits, last_error_info, validation_mode,
                  &catalog)
            : featherdoc::detail::validate_reopened_source_archive_metadata(
                  zip, effective_limits, last_error_info, validation_mode,
                  *expected_source_fingerprint, archive_path, &catalog);
    if (validation_error) {
        content.clear();
        return false;
    }

    const auto *entry_record =
        featherdoc::detail::find_archive_entry_record(catalog, entry_name);
    if (entry_record == nullptr) {
        if (const auto close_error = featherdoc::detail::close_read_archive(
                zip_guard, last_error_info,
                "failed to close source DOCX archive after checking an "
                "optional XML part",
                std::string{entry_name})) {
            content.clear();
            return false;
        }
        content.clear();
        return true;
    }
    entry_exists = true;
    if (featherdoc::detail::open_archive_entry_by_package_name(
            zip, catalog, entry_name) != 0) {
        if (const auto close_error = featherdoc::detail::close_read_archive(
                zip_guard, last_error_info,
                "failed to close source DOCX archive after an XML part could "
                "not be opened",
                std::string{entry_name})) {
            content.clear();
            return false;
        }
        set_last_error(
            last_error_info, std::make_error_code(std::errc::io_error),
            "failed to open source DOCX XML part", std::string{entry_name});
        content.clear();
        return false;
    }

    if (const auto limit_error =
            featherdoc::detail::enforce_open_zip_entry_xml_size_limit(
                zip, effective_limits, entry_name, last_error_info)) {
        zip_entry_close(zip);
        content.clear();
        return false;
    }

    void *buffer = nullptr;
    size_t buffer_size = 0U;
    const auto read_result = zip_entry_read(zip, &buffer, &buffer_size);
    const auto close_result = zip_entry_close(zip);
    std::unique_ptr<void, decltype(&std::free)> buffer_guard{buffer, &std::free};
    if (read_result < 0 || close_result != 0 ||
        (buffer_size > 0U && buffer == nullptr)) {
        set_last_error(
            last_error_info, std::make_error_code(std::errc::io_error),
            "failed to read source DOCX XML part", std::string{entry_name});
        return false;
    }

    if (const auto archive_close_error = featherdoc::detail::close_read_archive(
            zip_guard, last_error_info,
            "failed to close source DOCX archive after reading an XML part",
            std::string{entry_name})) {
        content.clear();
        return false;
    }

    if (buffer_size == 0U) {
        content.clear();
    } else {
        content.assign(static_cast<const char *>(buffer), buffer_size);
    }
    return true;
}

auto load_optional_docx_xml_part(
    const std::filesystem::path &document_path, std::string_view entry_name,
    pugi::xml_document &xml_document,
    featherdoc::document_error_info &last_error_info,
    const featherdoc::archive_limits *xml_limits,
    featherdoc::package_validation_mode validation_mode,
    const featherdoc::detail::source_archive_fingerprint
        *expected_source_fingerprint,
    // Only transitional WordprocessingML parts opt in. Callers for w15,
    // customXml, and OMML pass std::nullopt so their vocabularies and
    // prefix-sensitive markup remain untouched.
    std::optional<featherdoc::detail::wordprocessingml_part_kind>
        wordprocessingml_part_kind) -> bool {
    std::string xml_text;
    bool entry_exists = false;
    if (!read_docx_entry_text(document_path, entry_name, xml_text, entry_exists,
                              last_error_info, xml_limits, validation_mode,
                              expected_source_fingerprint)) {
        return false;
    }
    if (xml_text.empty()) {
        xml_document.reset();
        if (entry_exists && wordprocessingml_part_kind.has_value()) {
            set_last_error(last_error_info,
                           featherdoc::document_errc::invalid_package_structure,
                           "WordprocessingML part '" + std::string{entry_name} +
                               "' is empty",
                           std::string{entry_name});
            return false;
        }
        last_error_info.clear();
        return true;
    }

    const auto parse_result =
        xml_document.load_buffer(xml_text.data(), xml_text.size());
    if (!parse_result) {
        (void)featherdoc::detail::set_xml_parse_failure(
            last_error_info, parse_result,
            std::make_error_code(std::errc::invalid_argument), entry_name);
        return false;
    }

    if (wordprocessingml_part_kind.has_value()) {
        const auto namespace_result =
            featherdoc::detail::canonicalize_wordprocessingml_part(
                xml_document, *wordprocessingml_part_kind);
        if (!namespace_result) {
            (void)featherdoc::detail::set_wordprocessingml_namespace_last_error(
                last_error_info, namespace_result, entry_name);
            xml_document.reset();
            return false;
        }
        if (!namespace_result.root_matches) {
            (void)featherdoc::detail::
                set_wordprocessingml_root_mismatch_last_error(
                    last_error_info, namespace_result,
                    *wordprocessingml_part_kind, entry_name);
            xml_document.reset();
            return false;
        }
    }

    last_error_info.clear();
    return true;
}

auto xml_child_attribute_string(pugi::xml_node node, const char *attribute_name)
    -> std::optional<std::string> {
    const auto value = std::string_view{node.attribute(attribute_name).value()};
    if (value.empty()) {
        return std::nullopt;
    }
    return std::string{value};
}
