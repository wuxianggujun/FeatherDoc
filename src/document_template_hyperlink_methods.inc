std::vector<featherdoc::hyperlink_summary> Document::list_hyperlinks() const {
    return this->list_hyperlinks_in_part(
        const_cast<pugi::xml_document &>(this->document), document_xml_entry);
}

std::size_t Document::append_hyperlink(std::string_view text,
                                       std::string_view target) {
    return this->append_hyperlink_in_part(this->document, document_xml_entry, text,
                                          target);
}

bool Document::replace_hyperlink(std::size_t hyperlink_index,
                                 std::string_view text,
                                 std::string_view target) {
    return this->replace_hyperlink_in_part(this->document, document_xml_entry,
                                           hyperlink_index, text, target);
}

bool Document::remove_hyperlink(std::size_t hyperlink_index) {
    return this->remove_hyperlink_in_part(this->document, document_xml_entry,
                                          hyperlink_index);
}

std::vector<featherdoc::hyperlink_summary> Document::list_hyperlinks_in_part(
    pugi::xml_document &xml_document, std::string_view entry_name) const {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before listing hyperlinks",
                       std::string{entry_name});
        return {};
    }

    const pugi::xml_document *relationships_document = nullptr;
    if (entry_name == document_xml_entry) {
        relationships_document = &this->document_relationships;
    } else if (const auto *part = this->find_related_part_state(entry_name);
               part != nullptr) {
        relationships_document = &part->relationships;
    } else {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "template part is not attached to this document",
                       std::string{entry_name});
        return {};
    }

    return summarize_hyperlinks_in_part(this->last_error_info, xml_document,
                                        relationships_document);
}

std::size_t Document::append_hyperlink_in_part(
    pugi::xml_document &xml_document, std::string_view entry_name,
    std::string_view text, std::string_view target) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before appending a hyperlink",
                       std::string{entry_name});
        return 0U;
    }
    if (text.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "hyperlink text must not be empty",
                       std::string{entry_name});
        return 0U;
    }
    if (target.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "hyperlink target must not be empty",
                       std::string{entry_name});
        return 0U;
    }

    pugi::xml_document *relationships_document = nullptr;
    std::string_view relationships_entry_name;
    bool *has_relationships_part = nullptr;
    bool *relationships_dirty = nullptr;

    if (entry_name == document_xml_entry) {
        relationships_document = &this->document_relationships;
        relationships_entry_name = document_relationships_xml_entry;
        has_relationships_part = &this->has_document_relationships_part;
        relationships_dirty = &this->document_relationships_dirty;
    } else if (auto *part = this->find_related_part_state(entry_name); part != nullptr) {
        relationships_document = &part->relationships;
        relationships_entry_name = part->relationships_entry_name;
        has_relationships_part = &part->has_relationships_part;
        relationships_dirty = &part->relationships_dirty;
    } else {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "template part is not attached to this document",
                       std::string{entry_name});
        return 0U;
    }

    auto part_root = xml_document.document_element();
    auto container = template_part_block_container(xml_document);
    if (part_root == pugi::xml_node{} || container == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       std::string{entry_name} +
                           " does not contain a valid hyperlink insertion parent",
                       std::string{entry_name});
        return 0U;
    }

    if (relationships_document->child("Relationships") == pugi::xml_node{} &&
        !initialize_empty_relationships_document(*relationships_document)) {
        set_last_error(this->last_error_info,
                       document_errc::relationships_xml_parse_failed,
                       "failed to initialize default relationships XML",
                       std::string{relationships_entry_name});
        return 0U;
    }

    auto relationships = relationships_document->child("Relationships");
    if (relationships == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       document_errc::relationships_xml_parse_failed,
                       std::string{relationships_entry_name} +
                           " does not contain a Relationships root",
                       std::string{relationships_entry_name});
        return 0U;
    }

    auto relationships_namespace = part_root.attribute("xmlns:r");
    if (relationships_namespace == pugi::xml_attribute{}) {
        relationships_namespace = part_root.append_attribute("xmlns:r");
    }
    relationships_namespace.set_value(
        office_document_relationships_namespace_uri.data());

    const auto relationship_id = next_relationship_id(relationships);
    auto relationship = relationships.append_child("Relationship");
    if (relationship == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to append a hyperlink relationship",
                       std::string{relationships_entry_name});
        return 0U;
    }
    ensure_attribute_value(relationship, "Id", relationship_id);
    ensure_attribute_value(relationship, "Type", hyperlink_relationship_type);
    ensure_attribute_value(relationship, "Target", target);
    ensure_attribute_value(relationship, "TargetMode", "External");

    auto paragraph_node = featherdoc::detail::append_paragraph_node(container);
    auto hyperlink = paragraph_node.append_child("w:hyperlink");
    auto run = hyperlink.append_child("w:r");
    auto run_properties = run.append_child("w:rPr");
    auto run_style = run_properties.append_child("w:rStyle");
    auto color = run_properties.append_child("w:color");
    auto underline = run_properties.append_child("w:u");
    if (paragraph_node == pugi::xml_node{} || hyperlink == pugi::xml_node{} ||
        run == pugi::xml_node{} || run_properties == pugi::xml_node{} ||
        run_style == pugi::xml_node{} || color == pugi::xml_node{} ||
        underline == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to append hyperlink XML",
                       std::string{entry_name});
        return 0U;
    }

    ensure_attribute_value(hyperlink, "r:id", relationship_id);
    ensure_attribute_value(hyperlink, "w:history", "1");
    ensure_attribute_value(run_style, "w:val", "Hyperlink");
    ensure_attribute_value(color, "w:val", "0563C1");
    ensure_attribute_value(underline, "w:val", "single");
    if (!featherdoc::detail::set_plain_text_run_content(run, text)) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to append hyperlink text",
                       std::string{entry_name});
        return 0U;
    }

    *has_relationships_part = true;
    *relationships_dirty = true;
    this->last_error_info.clear();
    return 1U;
}

bool Document::replace_hyperlink_in_part(
    pugi::xml_document &xml_document, std::string_view entry_name,
    std::size_t hyperlink_index, std::string_view text,
    std::string_view target) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before replacing a hyperlink",
                       std::string{entry_name});
        return false;
    }
    if (text.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "hyperlink text must not be empty",
                       std::string{entry_name});
        return false;
    }
    if (target.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "hyperlink target must not be empty",
                       std::string{entry_name});
        return false;
    }

    pugi::xml_document *relationships_document = nullptr;
    std::string_view relationships_entry_name;
    bool *has_relationships_part = nullptr;
    bool *relationships_dirty = nullptr;
    if (entry_name == document_xml_entry) {
        relationships_document = &this->document_relationships;
        relationships_entry_name = document_relationships_xml_entry;
        has_relationships_part = &this->has_document_relationships_part;
        relationships_dirty = &this->document_relationships_dirty;
    } else if (auto *part = this->find_related_part_state(entry_name); part != nullptr) {
        relationships_document = &part->relationships;
        relationships_entry_name = part->relationships_entry_name;
        has_relationships_part = &part->has_relationships_part;
        relationships_dirty = &part->relationships_dirty;
    } else {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "template part is not attached to this document",
                       std::string{entry_name});
        return false;
    }

    std::vector<pugi::xml_node> hyperlink_nodes;
    collect_hyperlinks_in_order(xml_document, hyperlink_nodes);
    if (hyperlink_index >= hyperlink_nodes.size()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "hyperlink index is out of range",
                       std::string{entry_name});
        return false;
    }
    if (relationships_document->child("Relationships") == pugi::xml_node{} &&
        !initialize_empty_relationships_document(*relationships_document)) {
        set_last_error(this->last_error_info,
                       document_errc::relationships_xml_parse_failed,
                       "failed to initialize default relationships XML",
                       std::string{relationships_entry_name});
        return false;
    }

    auto relationships = relationships_document->child("Relationships");
    auto hyperlink = hyperlink_nodes[hyperlink_index];
    auto relationship_id = std::string{hyperlink.attribute("r:id").value()};
    auto relationship = relationship_id.empty()
                            ? pugi::xml_node{}
                            : find_relationship_by_id(relationships, relationship_id);
    if (relationship == pugi::xml_node{} ||
        std::string_view{relationship.attribute("Type").value()} !=
            hyperlink_relationship_type) {
        relationship_id = next_relationship_id(relationships);
        relationship = relationships.append_child("Relationship");
        if (relationship == pugi::xml_node{}) {
            set_last_error(this->last_error_info,
                           std::make_error_code(std::errc::not_enough_memory),
                           "failed to append hyperlink relationship",
                           std::string{relationships_entry_name});
            return false;
        }
        ensure_attribute_value(relationship, "Id", relationship_id);
        ensure_attribute_value(hyperlink, "r:id", relationship_id);
    }
    ensure_attribute_value(relationship, "Type", hyperlink_relationship_type);
    ensure_attribute_value(relationship, "Target", target);
    ensure_attribute_value(relationship, "TargetMode", "External");
    ensure_attribute_value(hyperlink, "w:history", "1");

    auto part_root = xml_document.document_element();
    if (part_root != pugi::xml_node{}) {
        auto relationships_namespace = part_root.attribute("xmlns:r");
        if (relationships_namespace == pugi::xml_attribute{}) {
            relationships_namespace = part_root.append_attribute("xmlns:r");
        }
        if (relationships_namespace != pugi::xml_attribute{}) {
            relationships_namespace.set_value(
                office_document_relationships_namespace_uri.data());
        }
    }

    if (!rewrite_hyperlink_plain_text(hyperlink, text)) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to replace hyperlink text",
                       std::string{entry_name});
        return false;
    }

    *has_relationships_part = true;
    *relationships_dirty = true;
    this->last_error_info.clear();
    return true;
}

bool Document::remove_hyperlink_in_part(
    pugi::xml_document &xml_document, std::string_view entry_name,
    std::size_t hyperlink_index) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before removing a hyperlink",
                       std::string{entry_name});
        return false;
    }

    pugi::xml_document *relationships_document = nullptr;
    std::string_view relationships_entry_name;
    bool *relationships_dirty = nullptr;
    if (entry_name == document_xml_entry) {
        relationships_document = &this->document_relationships;
        relationships_entry_name = document_relationships_xml_entry;
        relationships_dirty = &this->document_relationships_dirty;
    } else if (auto *part = this->find_related_part_state(entry_name); part != nullptr) {
        relationships_document = &part->relationships;
        relationships_entry_name = part->relationships_entry_name;
        relationships_dirty = &part->relationships_dirty;
    } else {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "template part is not attached to this document",
                       std::string{entry_name});
        return false;
    }

    std::vector<pugi::xml_node> hyperlink_nodes;
    collect_hyperlinks_in_order(xml_document, hyperlink_nodes);
    if (hyperlink_index >= hyperlink_nodes.size()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "hyperlink index is out of range",
                       std::string{entry_name});
        return false;
    }

    auto hyperlink = hyperlink_nodes[hyperlink_index];
    const auto relationship_id = std::string{hyperlink.attribute("r:id").value()};
    const auto display_text = featherdoc::detail::collect_plain_text_from_xml(hyperlink);
    auto parent = hyperlink.parent();
    if (parent == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "hyperlink does not have a removable parent",
                       std::string{entry_name});
        return false;
    }
    if (!display_text.empty()) {
        auto replacement_run = parent.insert_child_before("w:r", hyperlink);
        if (replacement_run == pugi::xml_node{} ||
            !featherdoc::detail::set_plain_text_run_content(replacement_run,
                                                            display_text)) {
            set_last_error(this->last_error_info,
                           std::make_error_code(std::errc::not_enough_memory),
                           "failed to preserve hyperlink display text",
                           std::string{entry_name});
            return false;
        }
    }
    parent.remove_child(hyperlink);

    auto relationships = relationships_document->child("Relationships");
    if (!relationship_id.empty() && relationships != pugi::xml_node{} &&
        !hyperlink_relationship_id_is_used(xml_document, relationship_id)) {
        auto relationship = find_relationship_by_id(relationships, relationship_id);
        if (relationship != pugi::xml_node{} &&
            std::string_view{relationship.attribute("Type").value()} ==
                hyperlink_relationship_type) {
            relationships.remove_child(relationship);
            *relationships_dirty = true;
        }
    }

    this->last_error_info.clear();
    return true;
}
