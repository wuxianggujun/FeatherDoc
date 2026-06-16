bool Document::remove_section_related_part_reference(
    std::size_t section_index, featherdoc::section_reference_kind reference_kind,
    const char *reference_name) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before removing header/footer references");
        return false;
    }

    if (const auto error = this->ensure_removal_prerequisites_loaded()) {
        return false;
    }

    if (section_index >= this->section_count()) {
        set_last_error(this->last_error_info, std::make_error_code(std::errc::invalid_argument),
                       "section index is out of range for header/footer reference removal",
                       std::string{document_xml_entry});
        return false;
    }

    auto section_properties = this->section_properties(section_index);
    if (section_properties == pugi::xml_node{}) {
        this->last_error_info.clear();
        return false;
    }

    const auto xml_reference_type = featherdoc::to_xml_reference_type(reference_kind);
    const auto reference =
        find_section_reference(section_properties, reference_name, xml_reference_type);
    if (reference == pugi::xml_node{}) {
        this->last_error_info.clear();
        return false;
    }

    const auto removed = section_properties.remove_child(reference);
    if (!removed) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to remove a section header/footer reference from w:sectPr",
                       std::string{document_xml_entry});
        return false;
    }

    if (reference_kind == featherdoc::section_reference_kind::first_page) {
        this->cleanup_first_page_section_markers();
    }

    if (reference_kind == featherdoc::section_reference_kind::even_page) {
        if (const auto error = this->cleanup_even_and_odd_headers_setting()) {
            return false;
        }
    }

    this->last_error_info.clear();
    return true;
}

std::error_code Document::ensure_removal_prerequisites_loaded() {
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

    return {};
}

void Document::cleanup_first_page_section_markers() {
    const auto first_page_reference_type =
        featherdoc::to_xml_reference_type(featherdoc::section_reference_kind::first_page);
    const auto body = this->document.child("w:document").child("w:body");
    if (body == pugi::xml_node{}) {
        return;
    }

    const auto cleanup_section = [&](pugi::xml_node section_properties) {
        if (section_properties == pugi::xml_node{}) {
            return;
        }

        const bool has_first_page_header = section_has_reference_type(
            section_properties, "w:headerReference", first_page_reference_type);
        const bool has_first_page_footer = section_has_reference_type(
            section_properties, "w:footerReference", first_page_reference_type);
        if (!has_first_page_header && !has_first_page_footer) {
            section_properties.remove_child("w:titlePg");
        }
    };

    for (auto child = body.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        if (std::string_view{child.name()} != "w:p") {
            continue;
        }

        cleanup_section(child.child("w:pPr").child("w:sectPr"));
    }

    cleanup_section(body.child("w:sectPr"));
}

std::error_code Document::cleanup_even_and_odd_headers_setting() {
    const auto even_page_reference_type =
        featherdoc::to_xml_reference_type(featherdoc::section_reference_kind::even_page);
    const bool has_any_even_header = document_has_reference_type(
        this->document, "w:headerReference", even_page_reference_type);
    const bool has_any_even_footer = document_has_reference_type(
        this->document, "w:footerReference", even_page_reference_type);
    if (has_any_even_header || has_any_even_footer) {
        return {};
    }

    if (!this->settings_loaded && !this->has_settings_part && !this->has_source_archive) {
        return {};
    }

    if (const auto error = this->ensure_settings_loaded()) {
        return error;
    }

    auto settings_root = this->settings.child("w:settings");
    if (settings_root == pugi::xml_node{}) {
        return {};
    }

    if (settings_root.child("w:evenAndOddHeaders") != pugi::xml_node{}) {
        settings_root.remove_child("w:evenAndOddHeaders");
        this->settings_dirty = true;
    }

    return {};
}

bool Document::remove_related_part(
    std::size_t part_index, std::vector<std::unique_ptr<xml_part_state>> &parts,
    const char *reference_name) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before removing header/footer parts");
        return false;
    }

    if (const auto error = this->ensure_removal_prerequisites_loaded()) {
        return false;
    }

    if (part_index >= parts.size()) {
        set_last_error(this->last_error_info, std::make_error_code(std::errc::invalid_argument),
                       "header/footer part index is out of range for removal",
                       std::string{document_xml_entry});
        return false;
    }

    const auto relationship_id = parts[part_index]->relationship_id;
    const auto entry_name = parts[part_index]->entry_name;
    const auto relationships_entry_name = parts[part_index]->relationships_entry_name;
    bool removed_first_reference = false;
    bool removed_even_reference = false;

    const auto remove_from_section = [&](pugi::xml_node section_properties) {
        for (auto reference = section_properties.child(reference_name);
             reference != pugi::xml_node{};) {
            const auto next = reference.next_sibling(reference_name);
            if (std::string_view{reference.attribute("r:id").value()} == relationship_id) {
                const auto reference_type =
                    std::string_view{reference.attribute("w:type").value()};
                if (reference_type == featherdoc::to_xml_reference_type(
                                          featherdoc::section_reference_kind::first_page)) {
                    removed_first_reference = true;
                } else if (reference_type ==
                           featherdoc::to_xml_reference_type(
                               featherdoc::section_reference_kind::even_page)) {
                    removed_even_reference = true;
                }
                section_properties.remove_child(reference);
            }

            reference = next;
        }
    };

    const auto body = this->document.child("w:document").child("w:body");
    if (body == pugi::xml_node{}) {
        set_last_error(this->last_error_info, std::make_error_code(std::errc::invalid_argument),
                       "document.xml does not contain the expected w:document/w:body structure",
                       std::string{document_xml_entry});
        return false;
    }

    for (auto child = body.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        if (std::string_view{child.name()} != "w:p") {
            continue;
        }

        if (auto section_properties = child.child("w:pPr").child("w:sectPr");
            section_properties != pugi::xml_node{}) {
            remove_from_section(section_properties);
        }
    }

    if (auto section_properties = body.child("w:sectPr"); section_properties != pugi::xml_node{}) {
        remove_from_section(section_properties);
    }

    if (removed_first_reference) {
        this->cleanup_first_page_section_markers();
    }
    if (removed_even_reference) {
        if (const auto error = this->cleanup_even_and_odd_headers_setting()) {
            return false;
        }
    }

    this->removed_related_part_entries.insert(entry_name);
    if (!relationships_entry_name.empty()) {
        this->removed_related_part_entries.insert(relationships_entry_name);
    }
    parts.erase(parts.begin() + static_cast<std::ptrdiff_t>(part_index));

    this->last_error_info.clear();
    return true;
}

bool Document::move_related_part(
    std::size_t source_index, std::size_t target_index,
    std::vector<std::unique_ptr<xml_part_state>> &parts, const char *relationship_type) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before reordering header/footer parts");
        return false;
    }

    if (source_index >= parts.size() || target_index >= parts.size()) {
        set_last_error(this->last_error_info, std::make_error_code(std::errc::invalid_argument),
                       "header/footer part index is out of range for reordering",
                       std::string{document_xml_entry});
        return false;
    }

    if (source_index == target_index) {
        this->last_error_info.clear();
        return true;
    }

    auto relationships = this->document_relationships.child("Relationships");
    if (relationships == pugi::xml_node{} &&
        !initialize_empty_relationships_document(this->document_relationships)) {
        set_last_error(this->last_error_info,
                       document_errc::relationships_xml_parse_failed,
                       "failed to initialize relationships XML for header/footer reordering",
                       std::string{document_relationships_xml_entry});
        return false;
    }
    relationships = this->document_relationships.child("Relationships");

    const auto source_relationship_id = parts[source_index]->relationship_id;
    const auto target_relationship_id = parts[target_index]->relationship_id;

    auto source_relationship = pugi::xml_node{};
    auto target_relationship = pugi::xml_node{};
    for (auto relationship = relationships.child("Relationship");
         relationship != pugi::xml_node{};
         relationship = relationship.next_sibling("Relationship")) {
        if (std::string_view{relationship.attribute("Type").value()} != relationship_type) {
            continue;
        }

        const auto relationship_id =
            std::string_view{relationship.attribute("Id").value()};
        if (relationship_id == source_relationship_id) {
            source_relationship = relationship;
        } else if (relationship_id == target_relationship_id) {
            target_relationship = relationship;
        }
    }

    if (source_relationship == pugi::xml_node{} ||
        target_relationship == pugi::xml_node{}) {
        set_last_error(
            this->last_error_info,
            std::make_error_code(std::errc::invalid_argument),
            "failed to resolve the requested header/footer relationship in document.xml.rels",
            std::string{document_relationships_xml_entry});
        return false;
    }

    const auto moved_relationship =
        source_index < target_index
            ? relationships.insert_move_after(source_relationship, target_relationship)
            : relationships.insert_move_before(source_relationship, target_relationship);
    if (moved_relationship == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to reorder the requested header/footer relationship",
                       std::string{document_relationships_xml_entry});
        return false;
    }

    auto moved_part = std::move(parts[source_index]);
    parts.erase(parts.begin() + static_cast<std::ptrdiff_t>(source_index));
    parts.insert(parts.begin() + static_cast<std::ptrdiff_t>(target_index),
                 std::move(moved_part));

    this->has_document_relationships_part = true;
    this->document_relationships_dirty = true;
    this->last_error_info.clear();
    return true;
}

bool Document::copy_section_related_part_references(
    std::size_t source_section_index, std::size_t target_section_index,
    const char *reference_name) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before copying header/footer references");
        return false;
    }

    if (source_section_index >= this->section_count() ||
        target_section_index >= this->section_count()) {
        set_last_error(this->last_error_info, std::make_error_code(std::errc::invalid_argument),
                       "section index is out of range for header/footer reference copy",
                       std::string{document_xml_entry});
        return false;
    }

    if (source_section_index == target_section_index) {
        this->last_error_info.clear();
        return true;
    }

    auto target_section_properties = this->ensure_section_properties(target_section_index);
    if (target_section_properties == pugi::xml_node{}) {
        return false;
    }

    const auto source_section_properties = this->section_properties(source_section_index);
    struct copied_reference_state {
        std::string type;
        std::string relationship_id;
    };
    std::vector<copied_reference_state> copied_references;
    if (source_section_properties != pugi::xml_node{}) {
        for (auto reference = source_section_properties.child(reference_name);
             reference != pugi::xml_node{}; reference = reference.next_sibling(reference_name)) {
            copied_references.push_back(copied_reference_state{
                reference.attribute("w:type").value(), reference.attribute("r:id").value()});
        }
    }

    auto document_root = this->document.child("w:document");
    if (document_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info, std::make_error_code(std::errc::invalid_argument),
                       "document.xml does not contain the expected w:document root",
                       std::string{document_xml_entry});
        return false;
    }

    auto relationships_namespace = document_root.attribute("xmlns:r");
    if (relationships_namespace == pugi::xml_attribute{}) {
        relationships_namespace = document_root.append_attribute("xmlns:r");
    }
    relationships_namespace.set_value(
        office_document_relationships_namespace_uri.data());

    for (auto reference = target_section_properties.child(reference_name);
         reference != pugi::xml_node{};) {
        const auto next = reference.next_sibling(reference_name);
        target_section_properties.remove_child(reference);
        reference = next;
    }

    for (const auto &copied_reference : copied_references) {
        auto reference = append_section_reference(target_section_properties, reference_name);
        if (reference == pugi::xml_node{}) {
            set_last_error(this->last_error_info,
                           std::make_error_code(std::errc::not_enough_memory),
                           "failed to attach a copied section header/footer reference to w:sectPr",
                           std::string{document_xml_entry});
            return false;
        }

        auto type_attribute = reference.attribute("w:type");
        if (type_attribute == pugi::xml_attribute{}) {
            type_attribute = reference.append_attribute("w:type");
        }
        type_attribute.set_value(copied_reference.type.c_str());

        auto relationship_attribute = reference.attribute("r:id");
        if (relationship_attribute == pugi::xml_attribute{}) {
            relationship_attribute = reference.append_attribute("r:id");
        }
        relationship_attribute.set_value(copied_reference.relationship_id.c_str());
    }

    this->cleanup_first_page_section_markers();
    const auto first_page_reference_type =
        featherdoc::to_xml_reference_type(featherdoc::section_reference_kind::first_page);
    const bool target_has_first_page_header = section_has_reference_type(
        target_section_properties, "w:headerReference", first_page_reference_type);
    const bool target_has_first_page_footer = section_has_reference_type(
        target_section_properties, "w:footerReference", first_page_reference_type);
    if ((target_has_first_page_header || target_has_first_page_footer) &&
        !this->ensure_title_page_enabled(target_section_properties)) {
        return false;
    }

    const auto even_page_reference_type =
        featherdoc::to_xml_reference_type(featherdoc::section_reference_kind::even_page);
    const bool has_any_even_header = document_has_reference_type(
        this->document, "w:headerReference", even_page_reference_type);
    const bool has_any_even_footer = document_has_reference_type(
        this->document, "w:footerReference", even_page_reference_type);
    if (has_any_even_header || has_any_even_footer) {
        if (const auto error = this->ensure_even_and_odd_headers_enabled()) {
            return false;
        }
    } else if (const auto error = this->cleanup_even_and_odd_headers_setting()) {
        return false;
    }

    this->last_error_info.clear();
    return true;
}

Paragraph &Document::ensure_section_header_paragraphs(
    std::size_t section_index, featherdoc::section_reference_kind reference_kind) {
    return this->ensure_section_related_part_paragraphs(
        section_index, reference_kind, this->header_parts, "w:hdr",
        "w:headerReference", header_relationship_type.data(),
        header_content_type.data());
}

Paragraph &Document::ensure_section_footer_paragraphs(
    std::size_t section_index, featherdoc::section_reference_kind reference_kind) {
    return this->ensure_section_related_part_paragraphs(
        section_index, reference_kind, this->footer_parts, "w:ftr",
        "w:footerReference", footer_relationship_type.data(),
        footer_content_type.data());
}

Paragraph &Document::assign_section_header_paragraphs(
    std::size_t section_index, std::size_t header_index,
    featherdoc::section_reference_kind reference_kind) {
    return this->assign_section_related_part_paragraphs(
        section_index, header_index, reference_kind, this->header_parts, "w:hdr",
        "w:headerReference");
}

Paragraph &Document::assign_section_footer_paragraphs(
    std::size_t section_index, std::size_t footer_index,
    featherdoc::section_reference_kind reference_kind) {
    return this->assign_section_related_part_paragraphs(
        section_index, footer_index, reference_kind, this->footer_parts, "w:ftr",
        "w:footerReference");
}

bool Document::remove_section_header_reference(
    std::size_t section_index, featherdoc::section_reference_kind reference_kind) {
    return this->remove_section_related_part_reference(
        section_index, reference_kind, "w:headerReference");
}

bool Document::remove_section_footer_reference(
    std::size_t section_index, featherdoc::section_reference_kind reference_kind) {
    return this->remove_section_related_part_reference(
        section_index, reference_kind, "w:footerReference");
}

bool Document::remove_header_part(std::size_t index) {
    return this->remove_related_part(index, this->header_parts, "w:headerReference");
}

bool Document::remove_footer_part(std::size_t index) {
    return this->remove_related_part(index, this->footer_parts, "w:footerReference");
}

bool Document::move_header_part(std::size_t source_index, std::size_t target_index) {
    return this->move_related_part(source_index, target_index, this->header_parts,
                                   header_relationship_type.data());
}

bool Document::move_footer_part(std::size_t source_index, std::size_t target_index) {
    return this->move_related_part(source_index, target_index, this->footer_parts,
                                   footer_relationship_type.data());
}

bool Document::copy_section_header_references(std::size_t source_section_index,
                                              std::size_t target_section_index) {
    return this->copy_section_related_part_references(
        source_section_index, target_section_index, "w:headerReference");
}

bool Document::copy_section_footer_references(std::size_t source_section_index,
                                              std::size_t target_section_index) {
    return this->copy_section_related_part_references(
        source_section_index, target_section_index, "w:footerReference");
}
