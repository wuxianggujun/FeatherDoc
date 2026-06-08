std::error_code Document::ensure_numbering_loaded() {
    if (this->numbering_loaded) {
        return {};
    }

    this->numbering.reset();
    this->has_numbering_part = false;

    if (!this->has_source_archive) {
        const auto parse_result =
            this->numbering.load_buffer(empty_numbering_xml.data(), empty_numbering_xml.size());
        if (!parse_result) {
            return set_last_error(
                this->last_error_info, document_errc::numbering_xml_parse_failed,
                parse_result.description(), std::string{numbering_xml_entry},
                parse_result.offset >= 0
                    ? std::optional<std::ptrdiff_t>{parse_result.offset}
                    : std::nullopt);
        }

        this->numbering_loaded = true;
        return {};
    }

    const auto relationships = this->document_relationships.child("Relationships");
    if (relationships == pugi::xml_node{}) {
        return set_last_error(this->last_error_info,
                              document_errc::relationships_xml_parse_failed,
                              "word/_rels/document.xml.rels does not contain a Relationships root",
                              std::string{document_relationships_xml_entry});
    }

    const auto numbering_relationship =
        find_document_relationship_by_type(relationships, numbering_relationship_type);
    if (numbering_relationship == pugi::xml_node{}) {
        const auto parse_result =
            this->numbering.load_buffer(empty_numbering_xml.data(), empty_numbering_xml.size());
        if (!parse_result) {
            return set_last_error(
                this->last_error_info, document_errc::numbering_xml_parse_failed,
                parse_result.description(), std::string{numbering_xml_entry},
                parse_result.offset >= 0
                    ? std::optional<std::ptrdiff_t>{parse_result.offset}
                    : std::nullopt);
        }

        this->numbering_loaded = true;
        return {};
    }

    const auto target = std::string_view{numbering_relationship.attribute("Target").value()};
    if (target.empty()) {
        return set_last_error(this->last_error_info, document_errc::numbering_xml_read_failed,
                              "numbering relationship does not contain a Target attribute",
                              std::string{document_relationships_xml_entry});
    }

    int source_zip_error = 0;
    zip_t *source_zip = zip_openwitherror(this->document_path.string().c_str(),
                                          ZIP_DEFAULT_COMPRESSION_LEVEL, 'r',
                                          &source_zip_error);
    if (!source_zip) {
        return set_last_error(this->last_error_info,
                              document_errc::source_archive_open_failed,
                              "failed to reopen source archive '" +
                                  this->document_path.string() +
                                  "' while loading word/numbering.xml: " +
                                  zip_error_text(source_zip_error));
    }

    const auto numbering_entry_name = normalize_word_part_entry(target);
    std::string numbering_text;
    const auto read_status =
        read_zip_entry_text(source_zip, numbering_entry_name, numbering_text);
    zip_close(source_zip);

    if (read_status != zip_entry_read_status::ok) {
        return set_last_error(this->last_error_info,
                              document_errc::numbering_xml_read_failed,
                              "failed to read required numbering part '" +
                                  numbering_entry_name + "'",
                              numbering_entry_name);
    }

    const auto parse_result =
        this->numbering.load_buffer(numbering_text.data(), numbering_text.size());
    if (!parse_result) {
        return set_last_error(
            this->last_error_info, document_errc::numbering_xml_parse_failed,
            parse_result.description(), numbering_entry_name,
            parse_result.offset >= 0 ? std::optional<std::ptrdiff_t>{parse_result.offset}
                                     : std::nullopt);
    }

    this->numbering_loaded = true;
    this->has_numbering_part = true;
    return {};
}

std::error_code Document::ensure_numbering_part_attached() {
    if (const auto error = this->ensure_numbering_loaded()) {
        return error;
    }

    if (const auto error = this->ensure_content_types_loaded()) {
        return error;
    }

    if (this->document_relationships.child("Relationships") == pugi::xml_node{} &&
        !initialize_empty_relationships_document(this->document_relationships)) {
        return set_last_error(this->last_error_info,
                              document_errc::relationships_xml_parse_failed,
                              "failed to initialize default relationships XML",
                              std::string{document_relationships_xml_entry});
    }

    auto relationships = this->document_relationships.child("Relationships");
    if (relationships == pugi::xml_node{}) {
        return set_last_error(this->last_error_info,
                              document_errc::relationships_xml_parse_failed,
                              "word/_rels/document.xml.rels does not contain a Relationships root",
                              std::string{document_relationships_xml_entry});
    }

    auto types = this->content_types.child("Types");
    if (types == pugi::xml_node{}) {
        return set_last_error(this->last_error_info,
                              document_errc::content_types_xml_parse_failed,
                              "[Content_Types].xml does not contain a Types root",
                              std::string{content_types_xml_entry});
    }

    auto numbering_relationship =
        find_document_relationship_by_type(relationships, numbering_relationship_type);
    if (numbering_relationship == pugi::xml_node{}) {
        std::unordered_set<std::string> used_relationship_ids;
        for (auto relationship = relationships.child("Relationship");
             relationship != pugi::xml_node{};
             relationship = relationship.next_sibling("Relationship")) {
            const auto id = relationship.attribute("Id").value();
            if (*id != '\0') {
                used_relationship_ids.emplace(id);
            }
        }

        std::string relationship_id;
        for (std::size_t next_index = 1U; relationship_id.empty(); ++next_index) {
            auto candidate = "rId" + std::to_string(next_index);
            if (!used_relationship_ids.contains(candidate)) {
                relationship_id = std::move(candidate);
            }
        }

        numbering_relationship = relationships.append_child("Relationship");
        if (numbering_relationship == pugi::xml_node{}) {
            return set_last_error(this->last_error_info,
                                  std::make_error_code(std::errc::not_enough_memory),
                                  "failed to create a document relationship for numbering.xml",
                                  std::string{document_relationships_xml_entry});
        }

        ensure_attribute_value(numbering_relationship, "Id", relationship_id);
        ensure_attribute_value(numbering_relationship, "Type", numbering_relationship_type);
        ensure_attribute_value(numbering_relationship, "Target",
                               make_document_relationship_target(numbering_xml_entry));

        this->has_document_relationships_part = true;
        this->document_relationships_dirty = true;
    }

    const auto override_part_name = make_override_part_name(numbering_xml_entry);
    auto override_node = pugi::xml_node{};
    for (auto existing_override = types.child("Override");
         existing_override != pugi::xml_node{};
         existing_override = existing_override.next_sibling("Override")) {
        if (std::string_view{existing_override.attribute("PartName").value()} ==
            override_part_name) {
            override_node = existing_override;
            break;
        }
    }

    if (override_node == pugi::xml_node{}) {
        override_node = types.append_child("Override");
    }
    if (override_node == pugi::xml_node{}) {
        return set_last_error(this->last_error_info,
                              std::make_error_code(std::errc::not_enough_memory),
                              "failed to append a content type override for numbering.xml",
                              std::string{content_types_xml_entry});
    }

    ensure_attribute_value(override_node, "PartName", override_part_name);
    ensure_attribute_value(override_node, "ContentType", numbering_content_type);

    this->has_numbering_part = true;
    this->content_types_dirty = true;
    return {};
}

std::optional<std::uint32_t> Document::ensure_numbering_definition(
    const featherdoc::numbering_definition &definition) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before editing numbering definitions");
        return std::nullopt;
    }

    auto validation_detail = std::string{};
    if (!validate_numbering_definition(definition, validation_detail)) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       std::move(validation_detail), std::string{numbering_xml_entry});
        return std::nullopt;
    }

    if (const auto error = this->ensure_numbering_part_attached()) {
        return std::nullopt;
    }

    auto numbering_root = this->numbering.child("w:numbering");
    if (numbering_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       document_errc::numbering_xml_parse_failed,
                       "word/numbering.xml does not contain the expected w:numbering root",
                       std::string{numbering_xml_entry});
        return std::nullopt;
    }

    const auto abstract_num_id = ensure_custom_abstract_numbering(numbering_root, definition);
    if (!abstract_num_id.has_value()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to create or update the requested numbering definition",
                       std::string{numbering_xml_entry});
        return std::nullopt;
    }

    this->numbering_dirty = true;
    this->last_error_info.clear();
    return abstract_num_id;
}

std::vector<featherdoc::numbering_definition_summary> Document::list_numbering_definitions() {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before listing numbering definitions",
                       std::string{numbering_xml_entry});
        return {};
    }

    if (const auto error = this->ensure_numbering_loaded()) {
        return {};
    }

    const auto numbering_root = this->numbering.child("w:numbering");
    if (numbering_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       document_errc::numbering_xml_parse_failed,
                       "word/numbering.xml does not contain the expected w:numbering root",
                       std::string{numbering_xml_entry});
        return {};
    }

    auto summaries = std::vector<featherdoc::numbering_definition_summary>{};
    for (auto abstract_num = numbering_root.child("w:abstractNum");
         abstract_num != pugi::xml_node{};
         abstract_num = abstract_num.next_sibling("w:abstractNum")) {
        if (const auto summary =
                summarize_numbering_definition_node(numbering_root, abstract_num);
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
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before reading numbering metadata",
                       std::string{numbering_xml_entry});
        return std::nullopt;
    }

    if (const auto error = this->ensure_numbering_loaded()) {
        return std::nullopt;
    }

    const auto numbering_root = this->numbering.child("w:numbering");
    if (numbering_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       document_errc::numbering_xml_parse_failed,
                       "word/numbering.xml does not contain the expected w:numbering root",
                       std::string{numbering_xml_entry});
        return std::nullopt;
    }

    const auto abstract_num = find_abstract_numbering_by_id(numbering_root, definition_id);
    if (abstract_num == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "numbering definition id '" + std::to_string(definition_id) +
                           "' was not found in word/numbering.xml",
                       std::string{numbering_xml_entry});
        return std::nullopt;
    }

    const auto summary = summarize_numbering_definition_node(numbering_root, abstract_num);
    if (!summary.has_value()) {
        set_last_error(this->last_error_info,
                       document_errc::numbering_xml_parse_failed,
                       "failed to summarize numbering definition metadata",
                       std::string{numbering_xml_entry});
        return std::nullopt;
    }

    this->last_error_info.clear();
    return summary;
}

std::optional<featherdoc::numbering_instance_lookup_summary>
Document::find_numbering_instance(std::uint32_t instance_id) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before reading numbering instance metadata",
                       std::string{numbering_xml_entry});
        return std::nullopt;
    }

    if (const auto error = this->ensure_numbering_loaded()) {
        return std::nullopt;
    }

    const auto numbering_root = this->numbering.child("w:numbering");
    if (numbering_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       document_errc::numbering_xml_parse_failed,
                       "word/numbering.xml does not contain the expected w:numbering root",
                       std::string{numbering_xml_entry});
        return std::nullopt;
    }

    const auto numbering_instance = find_numbering_instance_by_id(numbering_root, instance_id);
    if (numbering_instance == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "numbering instance id '" + std::to_string(instance_id) +
                           "' was not found in word/numbering.xml",
                       std::string{numbering_xml_entry});
        return std::nullopt;
    }

    const auto summary =
        summarize_numbering_instance_lookup_node(numbering_root, numbering_instance);
    if (!summary.has_value()) {
        set_last_error(this->last_error_info,
                       document_errc::numbering_xml_parse_failed,
                       "failed to summarize numbering instance metadata",
                       std::string{numbering_xml_entry});
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
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before importing numbering catalogs",
                       std::string{numbering_xml_entry});
        return summary;
    }

    std::unordered_set<std::string> seen_names;
    for (const auto &catalog_definition : catalog.definitions) {
        auto validation_detail = std::string{};
        if (!validate_numbering_catalog_definition(catalog_definition,
                                                   validation_detail)) {
            set_last_error(this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::move(validation_detail),
                           std::string{numbering_xml_entry});
            return summary;
        }
        if (!seen_names.insert(catalog_definition.definition.name).second) {
            set_last_error(this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           "numbering catalog definition names must be unique",
                           std::string{numbering_xml_entry});
            return summary;
        }
    }

    if (catalog.definitions.empty()) {
        this->last_error_info.clear();
        return summary;
    }

    if (const auto error = this->ensure_numbering_part_attached()) {
        return summary;
    }

    auto numbering_root = this->numbering.child("w:numbering");
    if (numbering_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       document_errc::numbering_xml_parse_failed,
                       "word/numbering.xml does not contain the expected w:numbering root",
                       std::string{numbering_xml_entry});
        return summary;
    }

    summary.definitions.reserve(catalog.definitions.size());
    for (const auto &catalog_definition : catalog.definitions) {
        const auto abstract_num_id = ensure_custom_abstract_numbering(
            numbering_root, catalog_definition.definition);
        if (!abstract_num_id.has_value()) {
            set_last_error(this->last_error_info,
                           std::make_error_code(std::errc::not_enough_memory),
                           "failed to import numbering catalog definition",
                           std::string{numbering_xml_entry});
            return summary;
        }

        auto imported_definition = featherdoc::imported_numbering_definition_summary{};
        imported_definition.name = catalog_definition.definition.name;
        imported_definition.definition_id = *abstract_num_id;

        for (const auto &instance : catalog_definition.instances) {
            const auto num_id = append_numbering_instance_for_abstract(
                numbering_root, *abstract_num_id, instance.level_overrides);
            if (!num_id.has_value()) {
                set_last_error(this->last_error_info,
                               std::make_error_code(std::errc::not_enough_memory),
                               "failed to import numbering catalog instance",
                               std::string{numbering_xml_entry});
                return summary;
            }
            imported_definition.instance_ids.push_back(*num_id);
            ++summary.imported_instance_count;
        }

        summary.definitions.push_back(std::move(imported_definition));
        ++summary.imported_definition_count;
    }

    this->numbering_dirty = true;
    this->last_error_info.clear();
    return summary;
}
