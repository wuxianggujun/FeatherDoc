struct hyperlink_relationship_resolution {
    std::optional<std::string> target;
    bool external{false};
};

auto resolve_hyperlink_relationship(
    const pugi::xml_document *relationships_document, std::string_view relationship_id)
    -> hyperlink_relationship_resolution {
    auto resolution = hyperlink_relationship_resolution{};
    if (relationships_document == nullptr || relationship_id.empty()) {
        return resolution;
    }

    const auto relationships = relationships_document->child("Relationships");
    for (auto relationship = relationships.child("Relationship");
         relationship != pugi::xml_node{};
         relationship = relationship.next_sibling("Relationship")) {
        if (std::string_view{relationship.attribute("Id").value()} !=
            relationship_id) {
            continue;
        }
        if (std::string_view{relationship.attribute("Type").value()} !=
            hyperlink_relationship_type) {
            return resolution;
        }

        const auto target = std::string_view{relationship.attribute("Target").value()};
        if (!target.empty()) {
            resolution.target = std::string{target};
        }
        resolution.external =
            std::string_view{relationship.attribute("TargetMode").value()} ==
            "External";
        return resolution;
    }

    return resolution;
}

void collect_hyperlinks_in_order(pugi::xml_node node,
                                 std::vector<pugi::xml_node> &hyperlinks) {
    for (auto child = node.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        if (std::string_view{child.name()} == "w:hyperlink") {
            hyperlinks.push_back(child);
        }
        collect_hyperlinks_in_order(child, hyperlinks);
    }
}

auto summarize_hyperlinks_in_part(
    featherdoc::document_error_info &last_error_info, pugi::xml_document &document,
    const pugi::xml_document *relationships_document)
    -> std::vector<featherdoc::hyperlink_summary> {
    std::vector<pugi::xml_node> hyperlink_nodes;
    collect_hyperlinks_in_order(document, hyperlink_nodes);

    auto summaries = std::vector<featherdoc::hyperlink_summary>{};
    summaries.reserve(hyperlink_nodes.size());
    for (std::size_t index = 0; index < hyperlink_nodes.size(); ++index) {
        auto summary = featherdoc::hyperlink_summary{};
        summary.index = index;
        const auto hyperlink = hyperlink_nodes[index];
        summary.text = featherdoc::detail::collect_plain_text_from_xml(hyperlink);

        const auto relationship_id = std::string_view{hyperlink.attribute("r:id").value()};
        if (!relationship_id.empty()) {
            summary.relationship_id = std::string{relationship_id};
            const auto resolution = resolve_hyperlink_relationship(
                relationships_document, relationship_id);
            summary.target = resolution.target;
            summary.external = resolution.external;
        }

        const auto anchor = std::string_view{hyperlink.attribute("w:anchor").value()};
        if (!anchor.empty()) {
            summary.anchor = std::string{anchor};
        }

        summaries.push_back(std::move(summary));
    }

    last_error_info.clear();
    return summaries;
}
