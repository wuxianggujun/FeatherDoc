template_validation_result Document::validate_template(
    std::span<const template_slot_requirement> requirements) const {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before validating a template");
        return {};
    }

    return validate_template_in_part(this->last_error_info,
                                     const_cast<pugi::xml_document &>(this->document),
                                     document_xml_entry, requirements);
}

template_validation_result Document::validate_template(
    std::initializer_list<template_slot_requirement> requirements) const {
    return this->validate_template(
        std::span<const template_slot_requirement>{requirements.begin(),
                                                   requirements.size()});
}

featherdoc::template_schema_validation_result Document::validate_template_schema(
    std::span<const featherdoc::template_schema_entry> entries) const {
    featherdoc::template_schema_validation_result result;
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before validating a template schema");
        return result;
    }

    std::vector<template_schema_group> groups;
    groups.reserve(entries.size());
    for (const auto &entry : entries) {
        if (entry.requirement.bookmark_name.empty()) {
            set_last_error(this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           "template schema bookmark name must not be empty",
                           std::string{document_xml_entry});
            return result;
        }

        featherdoc::template_schema_part_selector normalized_target{};
        std::string error_detail;
        if (!normalize_template_schema_selector(entry.target, normalized_target,
                                                error_detail)) {
            set_last_error(this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::move(error_detail), std::string{document_xml_entry});
            return result;
        }

        auto group_it =
            std::find_if(groups.begin(), groups.end(),
                         [&](const template_schema_group &group) {
                             return template_schema_selector_equals(group.target,
                                                                    normalized_target);
                         });
        if (group_it == groups.end()) {
            groups.push_back({normalized_target, {}});
            group_it = groups.end() - 1;
        }

        group_it->requirements.push_back(entry.requirement);
    }

    auto *mutable_document = const_cast<Document *>(this);
    const auto resolve_section_part_state =
        [mutable_document](std::size_t section_index,
                           featherdoc::section_reference_kind reference_kind,
                           auto &parts, const char *reference_name) {
            for (std::size_t source_section_index = section_index + 1U;
                 source_section_index > 0U; --source_section_index) {
                const auto resolved_section_index = source_section_index - 1U;
                if (auto *part = mutable_document->section_related_part_state(
                        resolved_section_index, reference_kind, parts, reference_name);
                    part != nullptr) {
                    return part;
                }
            }

            return static_cast<Document::xml_part_state *>(nullptr);
        };

    for (const auto &group : groups) {
        featherdoc::template_schema_part_validation_result part_result{};
        part_result.target = group.target;

        pugi::xml_document *xml_document = nullptr;
        switch (group.target.part) {
        case featherdoc::template_schema_part_kind::body:
            xml_document = &mutable_document->document;
            part_result.entry_name = document_xml_entry;
            break;
        case featherdoc::template_schema_part_kind::header:
            if (*group.target.part_index < mutable_document->header_parts.size() &&
                mutable_document->header_parts[*group.target.part_index] != nullptr) {
                auto &part = *mutable_document->header_parts[*group.target.part_index];
                xml_document = &part.xml;
                part_result.entry_name = part.entry_name;
            }
            break;
        case featherdoc::template_schema_part_kind::footer:
            if (*group.target.part_index < mutable_document->footer_parts.size() &&
                mutable_document->footer_parts[*group.target.part_index] != nullptr) {
                auto &part = *mutable_document->footer_parts[*group.target.part_index];
                xml_document = &part.xml;
                part_result.entry_name = part.entry_name;
            }
            break;
        case featherdoc::template_schema_part_kind::section_header:
            if (*group.target.section_index < mutable_document->section_count()) {
                if (auto *part = resolve_section_part_state(
                        *group.target.section_index, group.target.reference_kind,
                        mutable_document->header_parts,
                        "w:headerReference");
                    part != nullptr) {
                    xml_document = &part->xml;
                    part_result.entry_name = part->entry_name;
                }
            }
            break;
        case featherdoc::template_schema_part_kind::section_footer:
            if (*group.target.section_index < mutable_document->section_count()) {
                if (auto *part = resolve_section_part_state(
                        *group.target.section_index, group.target.reference_kind,
                        mutable_document->footer_parts,
                        "w:footerReference");
                    part != nullptr) {
                    xml_document = &part->xml;
                    part_result.entry_name = part->entry_name;
                }
            }
            break;
        }

        if (xml_document != nullptr) {
            part_result.available = true;
            part_result.validation = validate_template_in_part(
                this->last_error_info, *xml_document, part_result.entry_name,
                std::span<const featherdoc::template_slot_requirement>{
                    group.requirements.data(), group.requirements.size()});
        } else {
            part_result.validation = make_unavailable_part_validation(
                std::span<const featherdoc::template_slot_requirement>{
                    group.requirements.data(), group.requirements.size()});
        }

        result.part_results.push_back(std::move(part_result));
    }

    this->last_error_info.clear();
    return result;
}

featherdoc::template_schema_validation_result Document::validate_template_schema(
    std::initializer_list<featherdoc::template_schema_entry> entries) const {
    return this->validate_template_schema(
        std::span<const featherdoc::template_schema_entry>{entries.begin(), entries.size()});
}

featherdoc::template_schema_validation_result Document::validate_template_schema(
    const featherdoc::template_schema &schema) const {
    return this->validate_template_schema(
        std::span<const featherdoc::template_schema_entry>{schema.entries.data(),
                                                           schema.entries.size()});
}

std::optional<featherdoc::template_schema_scan_result>
Document::scan_template_schema(featherdoc::template_schema_scan_options options) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before scanning template schema",
                       std::string{document_xml_entry});
        return std::nullopt;
    }

    if (!options.include_bookmark_slots && !options.include_content_control_slots) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "template schema scan must include at least one slot source",
                       std::string{document_xml_entry});
        return std::nullopt;
    }

    auto result = featherdoc::template_schema_scan_result{};
    if (!append_scanned_template_schema_target(
            this->last_error_info, featherdoc::template_schema_part_selector{},
            this->body_template(), options, result)) {
        return std::nullopt;
    }

    const auto append_direct_related_parts = [&]() -> bool {
        for (std::size_t index = 0U; index < this->header_count(); ++index) {
            featherdoc::template_schema_part_selector target{};
            target.part = featherdoc::template_schema_part_kind::header;
            target.part_index = index;
            if (!append_scanned_template_schema_target(
                    this->last_error_info, target, this->header_template(index),
                    options, result)) {
                return false;
            }
        }

        for (std::size_t index = 0U; index < this->footer_count(); ++index) {
            featherdoc::template_schema_part_selector target{};
            target.part = featherdoc::template_schema_part_kind::footer;
            target.part_index = index;
            if (!append_scanned_template_schema_target(
                    this->last_error_info, target, this->footer_template(index),
                    options, result)) {
                return false;
            }
        }
        return true;
    };

    const auto append_section_target = [&](const featherdoc::section_inspection_summary &section,
                                           bool header_part,
                                           featherdoc::section_reference_kind reference_kind,
                                           bool present) -> bool {
        if (!present) {
            return true;
        }

        featherdoc::template_schema_part_selector target{};
        target.part = header_part ? featherdoc::template_schema_part_kind::section_header
                                  : featherdoc::template_schema_part_kind::section_footer;
        target.section_index = section.index;
        target.reference_kind = reference_kind;
        auto part = header_part ? this->section_header_template(section.index, reference_kind)
                                : this->section_footer_template(section.index, reference_kind);
        return append_scanned_template_schema_target(this->last_error_info, target, part,
                                                   options, result);
    };

    const auto append_section_targets = [&]() -> bool {
        const auto sections = this->inspect_sections();
        if (this->last_error_info.code) {
            return false;
        }

        for (const auto &section : sections.sections) {
            if (!append_section_target(section, true,
                                       featherdoc::section_reference_kind::default_reference,
                                       section.header.has_default) ||
                !append_section_target(section, true,
                                       featherdoc::section_reference_kind::first_page,
                                       section.header.has_first) ||
                !append_section_target(section, true,
                                       featherdoc::section_reference_kind::even_page,
                                       section.header.has_even) ||
                !append_section_target(section, false,
                                       featherdoc::section_reference_kind::default_reference,
                                       section.footer.has_default) ||
                !append_section_target(section, false,
                                       featherdoc::section_reference_kind::first_page,
                                       section.footer.has_first) ||
                !append_section_target(section, false,
                                       featherdoc::section_reference_kind::even_page,
                                       section.footer.has_even)) {
                return false;
            }
        }
        return true;
    };

    const auto append_resolved_section_target = [
        &](std::size_t section_index, bool header_part,
           featherdoc::section_reference_kind reference_kind,
           const std::optional<std::string> &resolved_entry_name,
           const std::optional<std::size_t> &resolved_section_index) -> bool {
        if (!resolved_entry_name.has_value() || !resolved_section_index.has_value()) {
            return true;
        }

        auto part = find_template_part_by_entry_name(*this, header_part, *resolved_entry_name);
        if (!part) {
            set_last_error(this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           "failed to resolve related part by entry name during template schema scan",
                           *resolved_entry_name);
            return false;
        }

        featherdoc::template_schema_part_selector target{};
        target.part = header_part ? featherdoc::template_schema_part_kind::section_header
                                  : featherdoc::template_schema_part_kind::section_footer;
        target.section_index = section_index;
        target.reference_kind = reference_kind;
        return append_scanned_template_schema_target(this->last_error_info, target, part,
                                                   options, result);
    };

    const auto append_resolved_section_targets = [&]() -> bool {
        const auto sections = this->inspect_sections();
        if (this->last_error_info.code) {
            return false;
        }

        for (const auto &section : sections.sections) {
            if (!append_resolved_section_target(
                    section.index, true,
                    featherdoc::section_reference_kind::default_reference,
                    section.header.resolved_default_entry_name,
                    section.header.resolved_default_section_index) ||
                !append_resolved_section_target(
                    section.index, true, featherdoc::section_reference_kind::first_page,
                    section.header.resolved_first_entry_name,
                    section.header.resolved_first_section_index) ||
                !append_resolved_section_target(
                    section.index, true, featherdoc::section_reference_kind::even_page,
                    section.header.resolved_even_entry_name,
                    section.header.resolved_even_section_index) ||
                !append_resolved_section_target(
                    section.index, false,
                    featherdoc::section_reference_kind::default_reference,
                    section.footer.resolved_default_entry_name,
                    section.footer.resolved_default_section_index) ||
                !append_resolved_section_target(
                    section.index, false, featherdoc::section_reference_kind::first_page,
                    section.footer.resolved_first_entry_name,
                    section.footer.resolved_first_section_index) ||
                !append_resolved_section_target(
                    section.index, false, featherdoc::section_reference_kind::even_page,
                    section.footer.resolved_even_entry_name,
                    section.footer.resolved_even_section_index)) {
                return false;
            }
        }
        return true;
    };

    switch (options.target_mode) {
    case featherdoc::template_schema_scan_target_mode::related_parts:
        if (!append_direct_related_parts()) {
            return std::nullopt;
        }
        break;
    case featherdoc::template_schema_scan_target_mode::section_targets:
        if (!append_section_targets()) {
            return std::nullopt;
        }
        break;
    case featherdoc::template_schema_scan_target_mode::resolved_section_targets:
        if (!append_resolved_section_targets()) {
            return std::nullopt;
        }
        break;
    }

    (void)featherdoc::normalize_template_schema(result.schema);
    if (result.schema.entries.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "document does not contain any exportable template slots",
                       std::string{document_xml_entry});
        return std::nullopt;
    }

    this->last_error_info.clear();
    return result;
}

std::optional<featherdoc::template_schema_patch>
Document::build_template_schema_patch_from_scan(
    const featherdoc::template_schema &baseline,
    featherdoc::template_schema_scan_options options) {
    auto scan = this->scan_template_schema(options);
    if (!scan.has_value()) {
        return std::nullopt;
    }

    auto patch = featherdoc::build_template_schema_patch(baseline, scan->schema);
    this->last_error_info.clear();
    return patch;
}

std::optional<featherdoc::template_onboarding_result>
Document::onboard_template(featherdoc::template_onboarding_options options) {
    auto scan = this->scan_template_schema(options.scan_options);
    if (!scan.has_value()) {
        return std::nullopt;
    }

    featherdoc::template_onboarding_result result{};
    result.scan = *scan;
    result.baseline_schema_available = options.baseline_schema.has_value();

    for (const auto &skipped : result.scan.skipped_bookmarks) {
        const auto severity =
            skipped.bookmark.kind == featherdoc::bookmark_kind::malformed ||
                    skipped.bookmark.kind == featherdoc::bookmark_kind::mixed
                ? featherdoc::template_onboarding_issue_severity::error
                : featherdoc::template_onboarding_issue_severity::warning;
        append_template_onboarding_issue(
            result, severity, "template_slot_skipped",
            "Template target '" + template_onboarding_target_label(skipped.target) +
                "' skipped bookmark '" + skipped.bookmark.bookmark_name + "': " +
                skipped.reason + ".",
            skipped.target, skipped.bookmark.bookmark_name);
    }

    if (options.validate_scanned_schema) {
        result.scanned_schema_validation =
            this->validate_template_schema(result.scan.schema);
        if (this->last_error_info.code) {
            return std::nullopt;
        }
        if (!static_cast<bool>(*result.scanned_schema_validation)) {
            append_template_onboarding_validation_issues(
                result, *result.scanned_schema_validation, false);
        }
    }

    if (options.baseline_schema.has_value()) {
        if (options.validate_baseline_schema) {
            result.baseline_validation =
                this->validate_template_schema(*options.baseline_schema);
            if (this->last_error_info.code) {
                return std::nullopt;
            }
            if (!static_cast<bool>(*result.baseline_validation)) {
                append_template_onboarding_validation_issues(
                    result, *result.baseline_validation, true);
            }
        }

        result.schema_patch = featherdoc::build_template_schema_patch(
            *options.baseline_schema, result.scan.schema);
        result.patch_review = featherdoc::make_template_schema_patch_review_summary(
            *options.baseline_schema, *result.schema_patch);
        result.patch_review->generated_slot_count = result.scan.slot_count();
    }

    if (result.has_errors()) {
        append_template_onboarding_action(
            result,
            featherdoc::template_onboarding_next_action_kind::fix_template_slots,
            "fix_template_slots",
            "Fix malformed, missing, or incompatible template slots before creating "
            "render data.",
            true);
    } else if (!result.baseline_schema_available) {
        append_template_onboarding_action(
            result,
            featherdoc::template_onboarding_next_action_kind::create_schema_baseline,
            "create_schema_baseline",
            "Review the scanned template schema and save it as the initial schema "
            "baseline.",
            false);
        append_template_onboarding_action(
            result,
            featherdoc::template_onboarding_next_action_kind::prepare_render_data,
            "prepare_render_data",
            "Create a render-data skeleton from the scanned template slots.", false);
    } else if (result.requires_schema_review()) {
        append_template_onboarding_action(
            result,
            featherdoc::template_onboarding_next_action_kind::review_schema_patch,
            "review_schema_patch",
            "Review and approve the schema patch before updating the baseline or "
            "running project template smoke.",
            true);
    } else {
        append_template_onboarding_action(
            result,
            featherdoc::template_onboarding_next_action_kind::prepare_render_data,
            "prepare_render_data",
            "Fill render data for all scanned template slots and validate the "
            "mapping.",
            false);
        append_template_onboarding_action(
            result,
            featherdoc::template_onboarding_next_action_kind::run_project_template_smoke,
            "run_project_template_smoke",
            "Run project template smoke after render data validation passes.", false);
    }

    this->last_error_info.clear();
    return result;
}
