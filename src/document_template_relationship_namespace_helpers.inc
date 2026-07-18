bool add_content_type_override(pugi::xml_document &content_types,
                               bool &content_types_dirty,
                               featherdoc::document_error_info &last_error_info,
                               std::string_view part_name,
                               std::string_view content_type) {
    auto types = featherdoc::detail::package_content_types_root(content_types);
    if (types == pugi::xml_node{}) {
        set_last_error(
            last_error_info,
            featherdoc::document_errc::content_types_xml_parse_failed,
            "[Content_Types].xml does not contain a Types root",
            "[Content_Types].xml");
        return false;
    }
    const auto normalized_part_name =
        featherdoc::detail::make_package_content_type_part_name(part_name);
    if (normalized_part_name.empty()) {
        set_last_error(last_error_info,
                       featherdoc::document_errc::invalid_package_structure,
                       "content type override PartName is invalid",
                       "[Content_Types].xml");
        return false;
    }
    for (auto override_node =
             featherdoc::detail::first_package_content_type_override(types);
         override_node != pugi::xml_node{};
         override_node =
             featherdoc::detail::next_package_content_type_override(
                 override_node)) {
        if (featherdoc::detail::content_type_part_names_equivalent(
                override_node.attribute("PartName").value(),
                normalized_part_name)) {
            ensure_attribute_value(override_node, "ContentType", content_type);
            content_types_dirty = true;
            return true;
        }
    }
    auto override_node =
        featherdoc::detail::append_package_content_type_override(types);
    if (override_node == pugi::xml_node{}) {
        set_last_error(
            last_error_info, std::make_error_code(std::errc::not_enough_memory),
            "failed to append content type override", "[Content_Types].xml");
        return false;
    }
    ensure_attribute_value(override_node, "PartName", normalized_part_name);
    ensure_attribute_value(override_node, "ContentType", content_type);
    content_types_dirty = true;
    return true;
}

bool ensure_document_relationship(
    pugi::xml_document &relationships_document, bool &has_relationships_part,
    bool &relationships_dirty, featherdoc::document_error_info &last_error_info,
    std::string_view relationship_type, std::string_view target) {
    if (!relationships_document_allows_mutation(
            relationships_document, has_relationships_part,
            document_relationships_xml_entry, last_error_info)) {
        return false;
    }
    if (featherdoc::detail::package_relationships_root(
            relationships_document) == pugi::xml_node{}) {
        const auto relationships_initialization =
            featherdoc::detail::initialize_empty_relationships_document(
                relationships_document);
        if (!relationships_initialization) {
            (void)featherdoc::detail::set_fixed_xml_initialization_failure(
                last_error_info, relationships_initialization,
                featherdoc::document_errc::relationships_xml_parse_failed,
                document_relationships_xml_entry);
            return false;
        }
    }
    auto relationships =
        featherdoc::detail::package_relationships_root(relationships_document);
    for (auto relationship =
             featherdoc::detail::first_package_relationship(relationships);
         relationship != pugi::xml_node{};
         relationship =
             featherdoc::detail::next_package_relationship(relationship)) {
        if (std::string_view{relationship.attribute("Type").value()} ==
            relationship_type) {
            ensure_attribute_value(relationship, "Target", target);
            has_relationships_part = true;
            relationships_dirty = true;
            return true;
        }
    }
    const auto relationship_id = next_relationship_id(relationships);
    auto relationship =
        featherdoc::detail::append_package_relationship(relationships);
    if (relationship == pugi::xml_node{}) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to append document relationship",
                       std::string{document_relationships_xml_entry});
        return false;
    }
    ensure_attribute_value(relationship, "Id", relationship_id);
    ensure_attribute_value(relationship, "Type", relationship_type);
    ensure_attribute_value(relationship, "Target", target);
    has_relationships_part = true;
    relationships_dirty = true;
    return true;
}

void clear_children(pugi::xml_node node) {
    for (auto child = node.first_child(); child != pugi::xml_node{};) {
        const auto next = child.next_sibling();
        node.remove_child(child);
        child = next;
    }
}

void ensure_wordprocessingml_namespace(pugi::xml_node root) {
    if (root == pugi::xml_node{}) {
        return;
    }

    auto word_namespace = root.attribute("xmlns:w");
    if (word_namespace == pugi::xml_attribute{}) {
        word_namespace = root.append_attribute("xmlns:w");
    }
    if (word_namespace != pugi::xml_attribute{}) {
        word_namespace.set_value(
            std::string{wordprocessingml_namespace_uri}.c_str());
    }
}

void ensure_wordprocessingml_2010_namespace(pugi::xml_node root) {
    if (root == pugi::xml_node{}) {
        return;
    }

    auto word_namespace = root.attribute("xmlns:w14");
    if (word_namespace == pugi::xml_attribute{}) {
        word_namespace = root.append_attribute("xmlns:w14");
    }
    if (word_namespace != pugi::xml_attribute{}) {
        word_namespace.set_value(
            std::string{wordprocessingml_2010_namespace_uri}.c_str());
    }
}

void ensure_markup_compatibility_ignorable(pugi::xml_node root,
                                           std::string_view prefix) {
    if (root == pugi::xml_node{} || prefix.empty()) {
        return;
    }

    auto markup_namespace = root.attribute("xmlns:mc");
    if (markup_namespace == pugi::xml_attribute{}) {
        markup_namespace = root.append_attribute("xmlns:mc");
    }
    if (markup_namespace != pugi::xml_attribute{}) {
        markup_namespace.set_value(
            std::string{markup_compatibility_namespace_uri}.c_str());
    }

    auto ignorable = root.attribute("mc:Ignorable");
    if (ignorable == pugi::xml_attribute{}) {
        ignorable = root.append_attribute("mc:Ignorable");
    }
    if (ignorable == pugi::xml_attribute{}) {
        return;
    }
    const auto value = std::string_view{ignorable.value()};
    if (value.empty()) {
        ignorable.set_value(std::string{prefix}.c_str());
        return;
    }

    std::istringstream tokens{std::string{value}};
    for (std::string token; tokens >> token;) {
        if (token == prefix) {
            return;
        }
    }

    auto updated = std::string{value};
    updated.push_back(' ');
    updated += prefix;
    ignorable.set_value(updated.c_str());
}

void ensure_wordprocessingml_2012_namespace(pugi::xml_node root) {
    if (root == pugi::xml_node{}) {
        return;
    }

    auto word_namespace = root.attribute("xmlns:w15");
    if (word_namespace == pugi::xml_attribute{}) {
        word_namespace = root.append_attribute("xmlns:w15");
    }
    if (word_namespace != pugi::xml_attribute{}) {
        word_namespace.set_value(
            std::string{wordprocessingml_2012_namespace_uri}.c_str());
    }
}
