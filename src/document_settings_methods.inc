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
        read_on_off_value(settings_root.child("w:updateFields")).value_or(false);
    this->last_error_info = previous_error;
    return enabled;
}

std::optional<bool> Document::update_fields_on_open_enabled() {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before reading updateFields",
                       std::string{settings_xml_entry});
        return std::nullopt;
    }

    if (this->ensure_settings_loaded()) {
        return std::nullopt;
    }

    const auto settings_root = this->settings.child("w:settings");
    if (settings_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       document_errc::settings_xml_parse_failed,
                       "word/settings.xml does not contain the expected w:settings root",
                       std::string{settings_xml_entry});
        return std::nullopt;
    }

    this->last_error_info.clear();
    return read_on_off_value(settings_root.child("w:updateFields")).value_or(false);
}

std::error_code Document::ensure_content_types_loaded() {
    if (this->content_types_loaded) {
        return {};
    }

    this->content_types.reset();
    if (!this->has_source_archive) {
        const auto parse_result =
            this->content_types.load_buffer(content_types_xml.data(), content_types_xml.size());
        if (!parse_result) {
            return set_last_error(
                this->last_error_info, document_errc::content_types_xml_parse_failed,
                parse_result.description(), std::string{content_types_xml_entry},
                parse_result.offset >= 0
                    ? std::optional<std::ptrdiff_t>{parse_result.offset}
                    : std::nullopt);
        }

        this->content_types_loaded = true;
        return {};
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
                                  "' while loading [Content_Types].xml: " +
                                  zip_error_text(source_zip_error));
    }

    std::string content_types_text;
    const auto read_status =
        read_zip_entry_text(source_zip, content_types_xml_entry, content_types_text);
    zip_close(source_zip);

    if (read_status != zip_entry_read_status::ok) {
        return set_last_error(this->last_error_info,
                              document_errc::content_types_xml_read_failed,
                              "failed to read required entry '[Content_Types].xml'",
                              std::string{content_types_xml_entry});
    }

    const auto parse_result =
        this->content_types.load_buffer(content_types_text.data(), content_types_text.size());
    if (!parse_result) {
        return set_last_error(
            this->last_error_info, document_errc::content_types_xml_parse_failed,
            parse_result.description(), std::string{content_types_xml_entry},
            parse_result.offset >= 0 ? std::optional<std::ptrdiff_t>{parse_result.offset}
                                     : std::nullopt);
    }

    this->content_types_loaded = true;
    return {};
}

std::error_code Document::ensure_settings_loaded() {
    if (this->settings_loaded) {
        return {};
    }

    this->settings.reset();
    this->has_settings_part = false;

    if (!this->has_source_archive) {
        const auto parse_result =
            this->settings.load_buffer(empty_settings_xml.data(), empty_settings_xml.size());
        if (!parse_result) {
            return set_last_error(
                this->last_error_info, document_errc::settings_xml_parse_failed,
                parse_result.description(), std::string{settings_xml_entry},
                parse_result.offset >= 0
                    ? std::optional<std::ptrdiff_t>{parse_result.offset}
                    : std::nullopt);
        }

        this->settings_loaded = true;
        return {};
    }

    const auto relationships = this->document_relationships.child("Relationships");
    if (relationships == pugi::xml_node{}) {
        return set_last_error(this->last_error_info,
                              document_errc::relationships_xml_parse_failed,
                              "word/_rels/document.xml.rels does not contain a Relationships root",
                              std::string{document_relationships_xml_entry});
    }

    const auto settings_relationship =
        find_document_relationship_by_type(relationships, settings_relationship_type);
    if (settings_relationship == pugi::xml_node{}) {
        const auto parse_result =
            this->settings.load_buffer(empty_settings_xml.data(), empty_settings_xml.size());
        if (!parse_result) {
            return set_last_error(
                this->last_error_info, document_errc::settings_xml_parse_failed,
                parse_result.description(), std::string{settings_xml_entry},
                parse_result.offset >= 0
                    ? std::optional<std::ptrdiff_t>{parse_result.offset}
                    : std::nullopt);
        }

        this->settings_loaded = true;
        return {};
    }

    const auto target = std::string_view{settings_relationship.attribute("Target").value()};
    if (target.empty()) {
        return set_last_error(this->last_error_info, document_errc::settings_xml_read_failed,
                              "settings relationship does not contain a Target attribute",
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
                                  "' while loading word/settings.xml: " +
                                  zip_error_text(source_zip_error));
    }

    const auto settings_entry_name = normalize_word_part_entry(target);
    std::string settings_text;
    const auto read_status =
        read_zip_entry_text(source_zip, settings_entry_name, settings_text);
    zip_close(source_zip);

    if (read_status != zip_entry_read_status::ok) {
        return set_last_error(this->last_error_info,
                              document_errc::settings_xml_read_failed,
                              "failed to read required settings part '" +
                                  settings_entry_name + "'",
                              settings_entry_name);
    }

    const auto parse_result =
        this->settings.load_buffer(settings_text.data(), settings_text.size());
    if (!parse_result) {
        return set_last_error(
            this->last_error_info, document_errc::settings_xml_parse_failed,
            parse_result.description(), settings_entry_name,
            parse_result.offset >= 0 ? std::optional<std::ptrdiff_t>{parse_result.offset}
                                     : std::nullopt);
    }

    this->settings_loaded = true;
    this->has_settings_part = true;
    return {};
}

std::error_code Document::ensure_settings_part_attached() {
    if (const auto error = this->ensure_settings_loaded()) {
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

    auto settings_relationship =
        find_document_relationship_by_type(relationships, settings_relationship_type);
    if (settings_relationship == pugi::xml_node{}) {
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

        settings_relationship = relationships.append_child("Relationship");
        if (settings_relationship == pugi::xml_node{}) {
            return set_last_error(this->last_error_info,
                                  std::make_error_code(std::errc::not_enough_memory),
                                  "failed to create a document relationship for settings.xml",
                                  std::string{document_relationships_xml_entry});
        }

        settings_relationship.append_attribute("Id").set_value(relationship_id.c_str());
        settings_relationship.append_attribute("Type").set_value(
            settings_relationship_type.data());
        const auto relationship_target =
            make_document_relationship_target(settings_xml_entry);
        settings_relationship.append_attribute("Target").set_value(
            relationship_target.c_str());

        this->has_document_relationships_part = true;
        this->document_relationships_dirty = true;
    }

    const auto override_part_name = make_override_part_name(settings_xml_entry);
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
                              "failed to append a content type override for settings.xml",
                              std::string{content_types_xml_entry});
    }

    auto part_name_attribute = override_node.attribute("PartName");
    if (part_name_attribute == pugi::xml_attribute{}) {
        part_name_attribute = override_node.append_attribute("PartName");
    }
    part_name_attribute.set_value(override_part_name.c_str());

    auto content_type_attribute = override_node.attribute("ContentType");
    if (content_type_attribute == pugi::xml_attribute{}) {
        content_type_attribute = override_node.append_attribute("ContentType");
    }
    content_type_attribute.set_value(settings_content_type.data());

    this->has_settings_part = true;
    this->content_types_dirty = true;
    return {};
}

bool Document::enable_update_fields_on_open() {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before enabling updateFields",
                       std::string{settings_xml_entry});
        return false;
    }
    if (const auto error = this->ensure_settings_part_attached()) {
        return false;
    }

    auto settings_root = this->settings.child("w:settings");
    if (settings_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       document_errc::settings_xml_parse_failed,
                       "word/settings.xml does not contain the expected w:settings root",
                       std::string{settings_xml_entry});
        return false;
    }

    if (ensure_on_off_node_enabled(settings_root, "w:updateFields") ==
        pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to enable w:updateFields in word/settings.xml",
                       std::string{settings_xml_entry});
        return false;
    }

    this->settings_dirty = true;
    this->last_error_info.clear();
    return true;
}

bool Document::clear_update_fields_on_open() {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before clearing updateFields",
                       std::string{settings_xml_entry});
        return false;
    }
    if (const auto error = this->ensure_settings_loaded()) {
        return false;
    }

    auto settings_root = this->settings.child("w:settings");
    if (settings_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       document_errc::settings_xml_parse_failed,
                       "word/settings.xml does not contain the expected w:settings root",
                       std::string{settings_xml_entry});
        return false;
    }

    if (settings_root.child("w:updateFields") != pugi::xml_node{}) {
        settings_root.remove_child("w:updateFields");
        this->settings_dirty = true;
    }

    this->last_error_info.clear();
    return true;
}
