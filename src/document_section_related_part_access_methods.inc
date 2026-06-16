Paragraph &Document::header_paragraphs(std::size_t index) {
    if (index >= this->header_parts.size()) {
        this->detached_paragraph.set_parent({});
        return this->detached_paragraph;
    }

    auto &part = *this->header_parts[index];
    part.paragraph.set_parent(part.xml.child("w:hdr"));
    return part.paragraph;
}

Paragraph &Document::footer_paragraphs(std::size_t index) {
    if (index >= this->footer_parts.size()) {
        this->detached_paragraph.set_parent({});
        return this->detached_paragraph;
    }

    auto &part = *this->footer_parts[index];
    part.paragraph.set_parent(part.xml.child("w:ftr"));
    return part.paragraph;
}

Paragraph &Document::related_part_paragraphs_by_relationship_id(
    std::string_view relationship_id, std::vector<std::unique_ptr<xml_part_state>> &parts,
    const char *part_root_name) {
    if (relationship_id.empty()) {
        this->detached_paragraph.set_parent({});
        return this->detached_paragraph;
    }

    for (auto &part : parts) {
        if (part->relationship_id == relationship_id) {
            part->paragraph.set_parent(part->xml.child(part_root_name));
            return part->paragraph;
        }
    }

    this->detached_paragraph.set_parent({});
    return this->detached_paragraph;
}

Paragraph &Document::section_related_part_paragraphs(
    std::size_t section_index, featherdoc::section_reference_kind reference_kind,
    std::vector<std::unique_ptr<xml_part_state>> &parts, const char *reference_name,
    const char *part_root_name) {
    auto section_properties = this->section_properties(section_index);
    if (section_properties == pugi::xml_node{}) {
        this->detached_paragraph.set_parent({});
        return this->detached_paragraph;
    }

    const auto xml_reference_type = featherdoc::to_xml_reference_type(reference_kind);
    for (auto reference = section_properties.child(reference_name);
         reference != pugi::xml_node{};
         reference = reference.next_sibling(reference_name)) {
        if (std::string_view{reference.attribute("w:type").value()} != xml_reference_type) {
            continue;
        }

        return this->related_part_paragraphs_by_relationship_id(
            reference.attribute("r:id").value(), parts, part_root_name);
    }

    this->detached_paragraph.set_parent({});
    return this->detached_paragraph;
}

Document::xml_part_state *Document::section_related_part_state(
    std::size_t section_index, featherdoc::section_reference_kind reference_kind,
    std::vector<std::unique_ptr<xml_part_state>> &parts, const char *reference_name) {
    const auto section_properties = this->section_properties(section_index);
    if (section_properties == pugi::xml_node{}) {
        return nullptr;
    }

    const auto xml_reference_type = featherdoc::to_xml_reference_type(reference_kind);
    for (auto reference = section_properties.child(reference_name);
         reference != pugi::xml_node{};
         reference = reference.next_sibling(reference_name)) {
        if (std::string_view{reference.attribute("w:type").value()} != xml_reference_type) {
            continue;
        }

        const auto relationship_id = std::string_view{reference.attribute("r:id").value()};
        if (relationship_id.empty()) {
            return nullptr;
        }

        for (auto &part : parts) {
            if (part->relationship_id == relationship_id) {
                return part.get();
            }
        }

        return nullptr;
    }

    return nullptr;
}

Document::xml_part_state *Document::find_related_part_state(std::string_view entry_name) {
    for (auto &part : this->header_parts) {
        if (part->entry_name == entry_name) {
            return part.get();
        }
    }

    for (auto &part : this->footer_parts) {
        if (part->entry_name == entry_name) {
            return part.get();
        }
    }

    return nullptr;
}

const Document::xml_part_state *Document::find_related_part_state(
    std::string_view entry_name) const {
    for (const auto &part : this->header_parts) {
        if (part->entry_name == entry_name) {
            return part.get();
        }
    }

    for (const auto &part : this->footer_parts) {
        if (part->entry_name == entry_name) {
            return part.get();
        }
    }

    return nullptr;
}

Paragraph &Document::section_header_paragraphs(
    std::size_t section_index, featherdoc::section_reference_kind reference_kind) {
    return this->section_related_part_paragraphs(
        section_index, reference_kind, this->header_parts, "w:headerReference", "w:hdr");
}

Paragraph &Document::section_footer_paragraphs(
    std::size_t section_index, featherdoc::section_reference_kind reference_kind) {
    return this->section_related_part_paragraphs(
        section_index, reference_kind, this->footer_parts, "w:footerReference", "w:ftr");
}
