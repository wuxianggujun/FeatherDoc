struct semantic_template_part_view {
    featherdoc::template_schema_part_selector target{};
    std::string entry_name;
    std::optional<std::size_t> resolved_from_section_index;
    TemplatePart part;
};

void append_semantic_template_part(
    std::vector<semantic_template_part_view> &parts,
    featherdoc::template_schema_part_kind part_kind,
    std::optional<std::size_t> part_index,
    std::optional<std::size_t> section_index,
    featherdoc::section_reference_kind reference_kind, TemplatePart part) {
    auto target = featherdoc::template_schema_part_selector{};
    target.part = part_kind;
    target.part_index = part_index;
    target.section_index = section_index;
    target.reference_kind = reference_kind;
    parts.push_back(semantic_template_part_view{target,
                                                std::string{part.entry_name()},
                                                std::nullopt,
                                                std::move(part)});
}

void append_semantic_template_part(
    std::vector<semantic_template_part_view> &parts,
    featherdoc::template_schema_part_kind part_kind,
    std::optional<std::size_t> part_index, TemplatePart part) {
    append_semantic_template_part(
        parts, part_kind, part_index, std::nullopt,
        featherdoc::section_reference_kind::default_reference, std::move(part));
}

void append_resolved_semantic_section_template_part(
    std::vector<semantic_template_part_view> &parts,
    featherdoc::template_schema_part_kind part_kind, std::size_t section_index,
    featherdoc::section_reference_kind reference_kind,
    const std::optional<std::string> &resolved_entry_name,
    std::optional<std::size_t> resolved_from_section_index, TemplatePart part) {
    if (!resolved_entry_name.has_value()) {
        return;
    }

    auto entry_name = std::string{part.entry_name()};
    if (entry_name.empty() || entry_name != *resolved_entry_name) {
        return;
    }

    auto target = featherdoc::template_schema_part_selector{};
    target.part = part_kind;
    target.section_index = section_index;
    target.reference_kind = reference_kind;
    parts.push_back(semantic_template_part_view{target,
                                                std::move(entry_name),
                                                resolved_from_section_index,
                                                std::move(part)});
}

auto collect_semantic_template_parts(
    Document &document,
    const featherdoc::document_semantic_diff_options &options)
    -> std::vector<semantic_template_part_view> {
    auto parts = std::vector<semantic_template_part_view>{};
    append_semantic_template_part(parts,
                                  featherdoc::template_schema_part_kind::body,
                                  std::nullopt, document.body_template());

    const auto header_count = document.header_count();
    for (std::size_t index = 0U; index < header_count; ++index) {
        append_semantic_template_part(
            parts, featherdoc::template_schema_part_kind::header, index,
            document.header_template(index));
    }

    const auto footer_count = document.footer_count();
    for (std::size_t index = 0U; index < footer_count; ++index) {
        append_semantic_template_part(
            parts, featherdoc::template_schema_part_kind::footer, index,
            document.footer_template(index));
    }

    if (!options.compare_resolved_section_template_parts) {
        return parts;
    }

    const auto sections = document.inspect_sections();
    if (document.last_error().code) {
        return parts;
    }

    for (const auto &section : sections.sections) {
        append_resolved_semantic_section_template_part(
            parts, featherdoc::template_schema_part_kind::section_header,
            section.index, featherdoc::section_reference_kind::default_reference,
            section.header.resolved_default_entry_name,
            section.header.resolved_default_section_index,
            document.section_header_template(
                section.header.resolved_default_section_index.value_or(section.index),
                featherdoc::section_reference_kind::default_reference));
        append_resolved_semantic_section_template_part(
            parts, featherdoc::template_schema_part_kind::section_header,
            section.index, featherdoc::section_reference_kind::first_page,
            section.header.resolved_first_entry_name,
            section.header.resolved_first_section_index,
            document.section_header_template(
                section.header.resolved_first_section_index.value_or(section.index),
                featherdoc::section_reference_kind::first_page));
        append_resolved_semantic_section_template_part(
            parts, featherdoc::template_schema_part_kind::section_header,
            section.index, featherdoc::section_reference_kind::even_page,
            section.header.resolved_even_entry_name,
            section.header.resolved_even_section_index,
            document.section_header_template(
                section.header.resolved_even_section_index.value_or(section.index),
                featherdoc::section_reference_kind::even_page));
        append_resolved_semantic_section_template_part(
            parts, featherdoc::template_schema_part_kind::section_footer,
            section.index, featherdoc::section_reference_kind::default_reference,
            section.footer.resolved_default_entry_name,
            section.footer.resolved_default_section_index,
            document.section_footer_template(
                section.footer.resolved_default_section_index.value_or(section.index),
                featherdoc::section_reference_kind::default_reference));
        append_resolved_semantic_section_template_part(
            parts, featherdoc::template_schema_part_kind::section_footer,
            section.index, featherdoc::section_reference_kind::first_page,
            section.footer.resolved_first_entry_name,
            section.footer.resolved_first_section_index,
            document.section_footer_template(
                section.footer.resolved_first_section_index.value_or(section.index),
                featherdoc::section_reference_kind::first_page));
        append_resolved_semantic_section_template_part(
            parts, featherdoc::template_schema_part_kind::section_footer,
            section.index, featherdoc::section_reference_kind::even_page,
            section.footer.resolved_even_entry_name,
            section.footer.resolved_even_section_index,
            document.section_footer_template(
                section.footer.resolved_even_section_index.value_or(section.index),
                featherdoc::section_reference_kind::even_page));
    }

    return parts;
}

auto is_counted_semantic_template_part(
    const featherdoc::template_schema_part_selector &target) -> bool {
    return target.part == featherdoc::template_schema_part_kind::header ||
           target.part == featherdoc::template_schema_part_kind::footer;
}

auto semantic_template_part_targets_equal(
    const featherdoc::template_schema_part_selector &left,
    const featherdoc::template_schema_part_selector &right) -> bool {
    return left.part == right.part && left.part_index == right.part_index &&
           left.section_index == right.section_index &&
           left.reference_kind == right.reference_kind;
}

auto find_matching_semantic_template_part(
    const std::vector<semantic_template_part_view> &parts,
    const std::vector<bool> &matched,
    const featherdoc::template_schema_part_selector &target)
    -> std::optional<std::size_t> {
    for (std::size_t index = 0U; index < parts.size(); ++index) {
        if (!matched[index] &&
            semantic_template_part_targets_equal(parts[index].target, target)) {
            return index;
        }
    }

    return std::nullopt;
}

void accumulate_semantic_template_part_summary(
    featherdoc::document_semantic_diff_category_summary &summary,
    const featherdoc::document_semantic_diff_part_result &part_result) {
    if (part_result.different()) {
        ++summary.changed_count;
    } else {
        ++summary.unchanged_count;
    }
}
