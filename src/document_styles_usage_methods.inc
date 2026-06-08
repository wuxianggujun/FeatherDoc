std::optional<featherdoc::style_usage_summary>
Document::find_style_usage(std::string_view style_id) {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before reading style usage",
            std::string{document_xml_entry});
        return std::nullopt;
    }

    if (style_id.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id must not be empty when reading style usage",
                       std::string{document_xml_entry});
        return std::nullopt;
    }

    if (const auto error = this->ensure_styles_loaded()) {
        return std::nullopt;
    }

    const auto styles_root = this->styles.child("w:styles");
    if (styles_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       document_errc::styles_xml_parse_failed,
                       "word/styles.xml does not contain a w:styles root",
                       std::string{styles_xml_entry});
        return std::nullopt;
    }

    if (find_style_node(styles_root, style_id) == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id '" + std::string{style_id} +
                           "' was not found in word/styles.xml",
                       std::string{styles_xml_entry});
        return std::nullopt;
    }

    const auto document_root = this->document.child("w:document");
    if (document_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       document_errc::document_xml_parse_failed,
                       "word/document.xml does not contain a w:document root",
                       std::string{document_xml_entry});
        return std::nullopt;
    }

    auto usage = featherdoc::style_usage_summary{};
    usage.style_id = std::string{style_id};
    collect_body_style_usage(document_root.child("w:body"), style_id,
                             usage.body, &usage.hits);

    for (const auto &part : this->header_parts) {
        if (!part) {
            continue;
        }

        auto part_ordinal = std::size_t{0};
        auto node_ordinals = style_usage_node_ordinals{};
        collect_style_usage(part->xml.child("w:hdr"), style_id, usage.header,
                            featherdoc::style_usage_part_kind::header,
                            part->entry_name, &usage.hits, part_ordinal,
                            node_ordinals);
    }

    for (const auto &part : this->footer_parts) {
        if (!part) {
            continue;
        }

        auto part_ordinal = std::size_t{0};
        auto node_ordinals = style_usage_node_ordinals{};
        collect_style_usage(part->xml.child("w:ftr"), style_id, usage.footer,
                            featherdoc::style_usage_part_kind::footer,
                            part->entry_name, &usage.hits, part_ordinal,
                            node_ordinals);
    }

    const auto build_part_references = [&](const char *reference_name,
                                           const auto &parts) {
        auto references_by_relationship_id = std::unordered_map<
            std::string, std::vector<featherdoc::style_usage_hit_reference>>{};
        const auto section_total = this->section_count();
        for (std::size_t section_index = 0; section_index < section_total;
             ++section_index) {
            const auto section_properties =
                this->section_properties(section_index);
            if (section_properties == pugi::xml_node{}) {
                continue;
            }

            for (auto reference = section_properties.child(reference_name);
                 reference != pugi::xml_node{};
                 reference = reference.next_sibling(reference_name)) {
                const auto relationship_id =
                    std::string_view{reference.attribute("r:id").value()};
                if (relationship_id.empty()) {
                    continue;
                }

                auto &references =
                    references_by_relationship_id[std::string{relationship_id}];
                append_style_usage_hit_reference(
                    references, section_index,
                    style_usage_reference_kind(
                        reference.attribute("w:type").value()));
            }
        }

        auto references_by_entry_name = std::unordered_map<
            std::string, std::vector<featherdoc::style_usage_hit_reference>>{};
        for (const auto &part : parts) {
            if (!part || part->entry_name.empty() ||
                part->relationship_id.empty()) {
                continue;
            }

            const auto iterator =
                references_by_relationship_id.find(part->relationship_id);
            if (iterator == references_by_relationship_id.end()) {
                continue;
            }

            references_by_entry_name.emplace(part->entry_name,
                                             iterator->second);
        }

        return references_by_entry_name;
    };

    const auto header_references =
        build_part_references("w:headerReference", this->header_parts);
    const auto footer_references =
        build_part_references("w:footerReference", this->footer_parts);
    apply_style_usage_hit_references(usage.hits, header_references,
                                     footer_references);

    finalize_style_usage_totals(usage);

    this->last_error_info.clear();
    return usage;
}

std::optional<featherdoc::style_usage_report> Document::list_style_usage() {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before listing style usage",
            std::string{styles_xml_entry});
        return std::nullopt;
    }

    auto styles = this->list_styles();
    if (const auto &error_info = this->last_error(); error_info.code) {
        return std::nullopt;
    }

    auto report = featherdoc::style_usage_report{};
    report.style_count = styles.size();
    report.entries.reserve(styles.size());

    for (auto &style : styles) {
        auto usage = featherdoc::style_usage_summary{};
        usage.style_id = style.style_id;

        if (!style.style_id.empty()) {
            auto style_usage = this->find_style_usage(style.style_id);
            if (!style_usage.has_value()) {
                return std::nullopt;
            }
            usage = std::move(*style_usage);
        }

        const auto reference_count = usage.total_count();
        report.total_reference_count += reference_count;
        if (reference_count == 0U) {
            ++report.unused_style_count;
        } else {
            ++report.used_style_count;
        }

        report.entries.push_back(featherdoc::style_usage_report_entry{
            std::move(style), std::move(usage)});
    }

    this->last_error_info.clear();
    return report;
}
