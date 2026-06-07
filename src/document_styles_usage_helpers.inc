void append_style_usage_hit(
    std::vector<featherdoc::style_usage_hit> &hits,
    featherdoc::style_usage_part_kind part_kind,
    featherdoc::style_usage_hit_kind hit_kind, std::string_view entry_name,
    std::size_t ordinal, std::size_t node_ordinal,
    std::optional<std::size_t> section_index = std::nullopt) {
    auto hit = featherdoc::style_usage_hit{};
    hit.part = part_kind;
    hit.kind = hit_kind;
    hit.entry_name = std::string{entry_name};
    hit.ordinal = ordinal;
    hit.node_ordinal = node_ordinal;
    hit.section_index = section_index;
    hits.push_back(std::move(hit));
}

void append_style_usage_hit_reference(
    std::vector<featherdoc::style_usage_hit_reference> &references,
    std::size_t section_index,
    featherdoc::section_reference_kind reference_kind) {
    for (const auto &reference : references) {
        if (reference.section_index == section_index &&
            reference.reference_kind == reference_kind) {
            return;
        }
    }

    auto reference = featherdoc::style_usage_hit_reference{};
    reference.section_index = section_index;
    reference.reference_kind = reference_kind;
    references.push_back(reference);
}

auto style_usage_reference_kind(std::string_view xml_reference_type)
    -> featherdoc::section_reference_kind {
    if (xml_reference_type ==
        featherdoc::to_xml_reference_type(
            featherdoc::section_reference_kind::first_page)) {
        return featherdoc::section_reference_kind::first_page;
    }

    if (xml_reference_type ==
        featherdoc::to_xml_reference_type(
            featherdoc::section_reference_kind::even_page)) {
        return featherdoc::section_reference_kind::even_page;
    }

    return featherdoc::section_reference_kind::default_reference;
}

struct style_usage_node_ordinals {
    std::size_t paragraph{0};
    std::size_t run{0};
    std::size_t table{0};
};

auto increment_style_usage_node_ordinal(
    style_usage_node_ordinals &ordinals,
    featherdoc::style_usage_hit_kind hit_kind) -> std::size_t {
    switch (hit_kind) {
    case featherdoc::style_usage_hit_kind::paragraph:
        return ++ordinals.paragraph;
    case featherdoc::style_usage_hit_kind::run:
        return ++ordinals.run;
    case featherdoc::style_usage_hit_kind::table:
        return ++ordinals.table;
    }

    return 0U;
}

void apply_style_usage_hit_references(
    std::vector<featherdoc::style_usage_hit> &hits,
    const std::unordered_map<std::string,
                             std::vector<featherdoc::style_usage_hit_reference>>
        &header_references,
    const std::unordered_map<std::string,
                             std::vector<featherdoc::style_usage_hit_reference>>
        &footer_references) {
    for (auto &hit : hits) {
        if (hit.part == featherdoc::style_usage_part_kind::body) {
            continue;
        }

        const auto &references =
            hit.part == featherdoc::style_usage_part_kind::header
                ? header_references
                : footer_references;
        const auto iterator = references.find(hit.entry_name);
        if (iterator != references.end()) {
            hit.references = iterator->second;
        }
    }
}

void collect_style_usage(
    pugi::xml_node node, std::string_view style_id,
    featherdoc::style_usage_breakdown &usage,
    featherdoc::style_usage_part_kind part_kind, std::string_view entry_name,
    std::vector<featherdoc::style_usage_hit> *hits, std::size_t &part_ordinal,
    style_usage_node_ordinals &node_ordinals,
    std::optional<std::size_t> section_index = std::nullopt) {
    if (node == pugi::xml_node{}) {
        return;
    }

    const auto node_name = std::string_view{node.name()};
    if (node_name == "w:p") {
        const auto node_ordinal = increment_style_usage_node_ordinal(
            node_ordinals, featherdoc::style_usage_hit_kind::paragraph);
        if (node_style_reference_matches(node, "w:pPr", "w:pStyle", style_id)) {
            ++usage.paragraph_count;
            ++part_ordinal;
            if (hits != nullptr) {
                append_style_usage_hit(
                    *hits, part_kind,
                    featherdoc::style_usage_hit_kind::paragraph, entry_name,
                    part_ordinal, node_ordinal, section_index);
            }
        }
    } else if (node_name == "w:r") {
        const auto node_ordinal = increment_style_usage_node_ordinal(
            node_ordinals, featherdoc::style_usage_hit_kind::run);
        if (node_style_reference_matches(node, "w:rPr", "w:rStyle", style_id)) {
            ++usage.run_count;
            ++part_ordinal;
            if (hits != nullptr) {
                append_style_usage_hit(
                    *hits, part_kind, featherdoc::style_usage_hit_kind::run,
                    entry_name, part_ordinal, node_ordinal, section_index);
            }
        }
    } else if (node_name == "w:tbl") {
        const auto node_ordinal = increment_style_usage_node_ordinal(
            node_ordinals, featherdoc::style_usage_hit_kind::table);
        if (node_style_reference_matches(node, "w:tblPr", "w:tblStyle",
                                         style_id)) {
            ++usage.table_count;
            ++part_ordinal;
            if (hits != nullptr) {
                append_style_usage_hit(
                    *hits, part_kind, featherdoc::style_usage_hit_kind::table,
                    entry_name, part_ordinal, node_ordinal, section_index);
            }
        }
    }

    for (auto child = node.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        collect_style_usage(child, style_id, usage, part_kind, entry_name, hits,
                            part_ordinal, node_ordinals, section_index);
    }
}

void collect_body_style_usage(pugi::xml_node body, std::string_view style_id,
                              featherdoc::style_usage_breakdown &usage,
                              std::vector<featherdoc::style_usage_hit> *hits) {
    if (body == pugi::xml_node{}) {
        return;
    }

    auto part_ordinal = std::size_t{0};
    auto node_ordinals = style_usage_node_ordinals{};
    auto section_index = std::size_t{0};
    for (auto child = body.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        const auto child_name = std::string_view{child.name()};
        if (child_name == "w:sectPr") {
            continue;
        }

        collect_style_usage(child, style_id, usage,
                            featherdoc::style_usage_part_kind::body,
                            document_xml_entry, hits, part_ordinal,
                            node_ordinals, section_index);
        if (child_name == "w:p" &&
            child.child("w:pPr").child("w:sectPr") != pugi::xml_node{}) {
            ++section_index;
        }
    }
}

void finalize_style_usage_totals(featherdoc::style_usage_summary &usage) {
    usage.paragraph_count = usage.body.paragraph_count +
                            usage.header.paragraph_count +
                            usage.footer.paragraph_count;
    usage.run_count =
        usage.body.run_count + usage.header.run_count + usage.footer.run_count;
    usage.table_count = usage.body.table_count + usage.header.table_count +
                        usage.footer.table_count;
}
