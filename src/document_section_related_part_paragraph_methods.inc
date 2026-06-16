Paragraph &Document::ensure_related_part_paragraphs(
    std::vector<std::unique_ptr<xml_part_state>> &parts, const char *part_root_name,
    const char *reference_name, const char *relationship_type,
    const char *content_type) {
    this->detached_paragraph.set_parent({});

    const auto count = this->section_count();
    const auto last_section_index = count == 0U ? 0U : count - 1U;
    return this->ensure_section_related_part_paragraphs(
        last_section_index, featherdoc::section_reference_kind::default_reference, parts,
        part_root_name, reference_name, relationship_type, content_type);
}

Paragraph &Document::ensure_section_related_part_paragraphs(
    std::size_t section_index, featherdoc::section_reference_kind reference_kind,
    std::vector<std::unique_ptr<xml_part_state>> &parts, const char *part_root_name,
    const char *reference_name, const char *relationship_type,
    const char *content_type) {
    this->detached_paragraph.set_parent({});

    const auto section_properties = this->ensure_section_properties(section_index);
    if (section_properties == pugi::xml_node{}) {
        return this->detached_paragraph;
    }

    if (reference_kind == featherdoc::section_reference_kind::first_page &&
        !this->ensure_title_page_enabled(section_properties)) {
        return this->detached_paragraph;
    }

    if (reference_kind == featherdoc::section_reference_kind::even_page) {
        if (const auto error = this->ensure_even_and_odd_headers_enabled()) {
            return this->detached_paragraph;
        }
    }

    const auto xml_reference_type = featherdoc::to_xml_reference_type(reference_kind);
    auto reference =
        find_section_reference(section_properties, reference_name, xml_reference_type);
    if (reference != pugi::xml_node{}) {
        const auto relationship_id = std::string_view{reference.attribute("r:id").value()};
        if (!relationship_id.empty()) {
            for (const auto &part : parts) {
                if (part->relationship_id == relationship_id) {
                    part->paragraph.set_parent(part->xml.child(part_root_name));
                    this->last_error_info.clear();
                    return part->paragraph;
                }
            }
        }
    }

    if (const auto error = this->ensure_content_types_loaded()) {
        return this->detached_paragraph;
    }

    auto document_root = this->document.child("w:document");
    if (document_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info, std::make_error_code(std::errc::invalid_argument),
                       "document.xml does not contain the expected w:document root",
                       std::string{document_xml_entry});
        return this->detached_paragraph;
    }

    if (this->document_relationships.child("Relationships") == pugi::xml_node{} &&
        !initialize_empty_relationships_document(this->document_relationships)) {
        set_last_error(this->last_error_info,
                       document_errc::relationships_xml_parse_failed,
                       "failed to initialize default relationships XML",
                       std::string{document_relationships_xml_entry});
        return this->detached_paragraph;
    }

    auto relationships = this->document_relationships.child("Relationships");
    if (relationships == pugi::xml_node{}) {
        set_last_error(this->last_error_info, document_errc::relationships_xml_parse_failed,
                       "word/_rels/document.xml.rels does not contain a Relationships root",
                       std::string{document_relationships_xml_entry});
        return this->detached_paragraph;
    }

    auto types = this->content_types.child("Types");
    if (types == pugi::xml_node{}) {
        set_last_error(this->last_error_info, document_errc::content_types_xml_parse_failed,
                       "[Content_Types].xml does not contain a Types root",
                       std::string{content_types_xml_entry});
        return this->detached_paragraph;
    }

    auto relationships_namespace = document_root.attribute("xmlns:r");
    if (relationships_namespace == pugi::xml_attribute{}) {
        relationships_namespace = document_root.append_attribute("xmlns:r");
    }
    relationships_namespace.set_value(
        office_document_relationships_namespace_uri.data());

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

    const auto part_prefix =
        std::string_view{part_root_name} == "w:hdr" ? "header" : "footer";
    std::unordered_set<std::string> used_part_entries;
    for (const auto &part : parts) {
        used_part_entries.insert(part->entry_name);
    }

    std::string entry_name;
    for (std::size_t next_index = 1U; entry_name.empty(); ++next_index) {
        auto candidate = std::string{"word/"} + part_prefix +
                         std::to_string(next_index) + ".xml";
        if (!used_part_entries.contains(candidate)) {
            entry_name = std::move(candidate);
        }
    }

    auto part = std::make_unique<xml_part_state>();
    part->relationship_id = relationship_id;
    part->entry_name = entry_name;
    part->relationships_entry_name = make_part_relationships_entry(entry_name);
    const auto empty_part_xml =
        std::string_view{part_root_name} == "w:hdr" ? empty_header_xml : empty_footer_xml;
    const auto parse_result =
        part->xml.load_buffer(empty_part_xml.data(), empty_part_xml.size());
    if (!parse_result) {
        set_last_error(this->last_error_info, document_errc::related_part_parse_failed,
                       parse_result.description(), entry_name,
                       parse_result.offset >= 0
                           ? std::optional<std::ptrdiff_t>{parse_result.offset}
                           : std::nullopt);
        return this->detached_paragraph;
    }

    auto relationship = relationships.append_child("Relationship");
    if (relationship == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to create a new document relationship for header/footer",
                       std::string{document_relationships_xml_entry});
        return this->detached_paragraph;
    }

    relationship.append_attribute("Id").set_value(relationship_id.c_str());
    relationship.append_attribute("Type").set_value(relationship_type);
    const auto relationship_target = make_document_relationship_target(entry_name);
    relationship.append_attribute("Target").set_value(relationship_target.c_str());

    const auto override_part_name = make_override_part_name(entry_name);
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
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to append a content type override for header/footer",
                       std::string{content_types_xml_entry});
        return this->detached_paragraph;
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
    content_type_attribute.set_value(content_type);

    if (reference == pugi::xml_node{}) {
        reference = append_section_reference(section_properties, reference_name);
    }

    if (reference == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to attach a section header/footer reference to w:sectPr",
                       std::string{document_xml_entry});
        return this->detached_paragraph;
    }

    auto type_attribute = reference.attribute("w:type");
    if (type_attribute == pugi::xml_attribute{}) {
        type_attribute = reference.append_attribute("w:type");
    }
    type_attribute.set_value(xml_reference_type.data());

    auto relationship_attribute = reference.attribute("r:id");
    if (relationship_attribute == pugi::xml_attribute{}) {
        relationship_attribute = reference.append_attribute("r:id");
    }
    relationship_attribute.set_value(relationship_id.c_str());

    parts.push_back(std::move(part));
    auto &created_part = *parts.back();
    created_part.paragraph.set_parent(created_part.xml.child(part_root_name));

    this->has_document_relationships_part = true;
    this->document_relationships_dirty = true;
    this->content_types_dirty = true;
    this->last_error_info.clear();
    return created_part.paragraph;
}

Paragraph &Document::assign_section_related_part_paragraphs(
    std::size_t section_index, std::size_t part_index,
    featherdoc::section_reference_kind reference_kind,
    std::vector<std::unique_ptr<xml_part_state>> &parts, const char *part_root_name,
    const char *reference_name) {
    this->detached_paragraph.set_parent({});

    if (part_index >= parts.size()) {
        set_last_error(this->last_error_info, std::make_error_code(std::errc::invalid_argument),
                       "header/footer part index is out of range for section assignment",
                       std::string{document_xml_entry});
        return this->detached_paragraph;
    }

    const auto section_properties = this->ensure_section_properties(section_index);
    if (section_properties == pugi::xml_node{}) {
        return this->detached_paragraph;
    }

    if (reference_kind == featherdoc::section_reference_kind::first_page &&
        !this->ensure_title_page_enabled(section_properties)) {
        return this->detached_paragraph;
    }

    if (reference_kind == featherdoc::section_reference_kind::even_page) {
        if (const auto error = this->ensure_even_and_odd_headers_enabled()) {
            return this->detached_paragraph;
        }
    }

    auto document_root = this->document.child("w:document");
    if (document_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info, std::make_error_code(std::errc::invalid_argument),
                       "document.xml does not contain the expected w:document root",
                       std::string{document_xml_entry});
        return this->detached_paragraph;
    }

    auto relationships_namespace = document_root.attribute("xmlns:r");
    if (relationships_namespace == pugi::xml_attribute{}) {
        relationships_namespace = document_root.append_attribute("xmlns:r");
    }
    relationships_namespace.set_value(
        office_document_relationships_namespace_uri.data());

    const auto xml_reference_type = featherdoc::to_xml_reference_type(reference_kind);
    auto reference =
        find_section_reference(section_properties, reference_name, xml_reference_type);
    if (reference == pugi::xml_node{}) {
        reference = append_section_reference(section_properties, reference_name);
    }

    if (reference == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to attach a section header/footer reference to w:sectPr",
                       std::string{document_xml_entry});
        return this->detached_paragraph;
    }

    auto &part = *parts[part_index];
    if (part.relationship_id.empty()) {
        set_last_error(this->last_error_info, std::make_error_code(std::errc::invalid_argument),
                       "target header/footer part does not have a relationship id",
                       part.entry_name);
        return this->detached_paragraph;
    }

    auto type_attribute = reference.attribute("w:type");
    if (type_attribute == pugi::xml_attribute{}) {
        type_attribute = reference.append_attribute("w:type");
    }
    type_attribute.set_value(xml_reference_type.data());

    auto relationship_attribute = reference.attribute("r:id");
    if (relationship_attribute == pugi::xml_attribute{}) {
        relationship_attribute = reference.append_attribute("r:id");
    }
    relationship_attribute.set_value(part.relationship_id.c_str());

    part.paragraph.set_parent(part.xml.child(part_root_name));
    this->last_error_info.clear();
    return part.paragraph;
}
