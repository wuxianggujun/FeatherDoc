struct template_schema_group final {
    featherdoc::template_schema_part_selector target{};
    std::vector<featherdoc::template_slot_requirement> requirements;
};

auto template_onboarding_target_label(
    const featherdoc::template_schema_part_selector &target) -> std::string {
    std::ostringstream stream;
    switch (target.part) {
    case featherdoc::template_schema_part_kind::body:
        stream << "body";
        break;
    case featherdoc::template_schema_part_kind::header:
        stream << "header";
        if (target.part_index.has_value()) {
            stream << '[' << *target.part_index << ']';
        }
        break;
    case featherdoc::template_schema_part_kind::footer:
        stream << "footer";
        if (target.part_index.has_value()) {
            stream << '[' << *target.part_index << ']';
        }
        break;
    case featherdoc::template_schema_part_kind::section_header:
        stream << "section-header";
        if (target.section_index.has_value()) {
            stream << '[' << *target.section_index << ']';
        }
        stream << ':' << featherdoc::to_xml_reference_type(target.reference_kind);
        break;
    case featherdoc::template_schema_part_kind::section_footer:
        stream << "section-footer";
        if (target.section_index.has_value()) {
            stream << '[' << *target.section_index << ']';
        }
        stream << ':' << featherdoc::to_xml_reference_type(target.reference_kind);
        break;
    }

    return stream.str();
}

void append_template_onboarding_issue(
    featherdoc::template_onboarding_result &result,
    featherdoc::template_onboarding_issue_severity severity, std::string code,
    std::string message,
    std::optional<featherdoc::template_schema_part_selector> target = std::nullopt,
    std::optional<std::string> slot_name = std::nullopt) {
    featherdoc::template_onboarding_issue issue{};
    issue.severity = severity;
    issue.code = std::move(code);
    issue.message = std::move(message);
    issue.target = std::move(target);
    issue.slot_name = std::move(slot_name);
    result.issues.push_back(std::move(issue));
}

void append_template_onboarding_action(
    featherdoc::template_onboarding_result &result,
    featherdoc::template_onboarding_next_action_kind kind, std::string code,
    std::string message, bool blocking) {
    featherdoc::template_onboarding_next_action action{};
    action.kind = kind;
    action.code = std::move(code);
    action.message = std::move(message);
    action.blocking = blocking;
    result.next_actions.push_back(std::move(action));
}

void append_template_onboarding_validation_issues(
    featherdoc::template_onboarding_result &onboarding,
    const featherdoc::template_schema_validation_result &validation,
    bool baseline_validation) {
    const auto drift_severity = baseline_validation
                                    ? featherdoc::template_onboarding_issue_severity::warning
                                    : featherdoc::template_onboarding_issue_severity::error;

    for (const auto &part_result : validation.part_results) {
        const auto target_label = template_onboarding_target_label(part_result.target);
        if (!part_result.available && !part_result.validation.missing_required.empty()) {
            append_template_onboarding_issue(
                onboarding, drift_severity,
                baseline_validation ? "baseline_part_unavailable"
                                    : "template_part_unavailable",
                "Template target '" + target_label + "' is not available.",
                part_result.target);
        }

        const auto &part_validation = part_result.validation;
        for (const auto &slot_name : part_validation.missing_required) {
            append_template_onboarding_issue(
                onboarding, drift_severity,
                baseline_validation ? "baseline_slot_missing" : "template_slot_missing",
                "Template target '" + target_label + "' is missing required slot '" +
                    slot_name + "'.",
                part_result.target, slot_name);
        }

        for (const auto &slot_name : part_validation.duplicate_bookmarks) {
            append_template_onboarding_issue(
                onboarding, drift_severity,
                baseline_validation ? "baseline_slot_duplicate"
                                    : "template_slot_duplicate",
                "Template target '" + target_label + "' has duplicate slot '" +
                    slot_name + "'.",
                part_result.target, slot_name);
        }

        for (const auto &slot_name : part_validation.malformed_placeholders) {
            append_template_onboarding_issue(
                onboarding, featherdoc::template_onboarding_issue_severity::error,
                "template_slot_malformed",
                "Template target '" + target_label + "' has malformed slot '" +
                    slot_name + "'.",
                part_result.target, slot_name);
        }

        for (const auto &mismatch : part_validation.kind_mismatches) {
            append_template_onboarding_issue(
                onboarding, drift_severity,
                baseline_validation ? "baseline_slot_kind_mismatch"
                                    : "template_slot_kind_mismatch",
                "Template target '" + target_label + "' has incompatible slot kind for '" +
                    mismatch.bookmark_name + "'.",
                part_result.target, mismatch.bookmark_name);
        }

        for (const auto &mismatch : part_validation.occurrence_mismatches) {
            append_template_onboarding_issue(
                onboarding, drift_severity,
                baseline_validation ? "baseline_slot_occurrence_mismatch"
                                    : "template_slot_occurrence_mismatch",
                "Template target '" + target_label + "' has an unexpected occurrence "
                "count for slot '" +
                    mismatch.bookmark_name + "'.",
                part_result.target, mismatch.bookmark_name);
        }

        for (const auto &bookmark : part_validation.unexpected_bookmarks) {
            append_template_onboarding_issue(
                onboarding, featherdoc::template_onboarding_issue_severity::warning,
                baseline_validation ? "baseline_slot_unexpected"
                                    : "template_slot_unexpected",
                "Template target '" + target_label + "' contains undeclared slot '" +
                    bookmark.bookmark_name + "'.",
                part_result.target, bookmark.bookmark_name);
        }
    }
}


auto append_scanned_template_schema_target(
    featherdoc::document_error_info &last_error_info,
    const featherdoc::template_schema_part_selector &target,
    featherdoc::TemplatePart part,
    const featherdoc::template_schema_scan_options &options,
    featherdoc::template_schema_scan_result &result) -> bool {
    if (!part) {
        set_last_error(last_error_info, std::make_error_code(std::errc::invalid_argument),
                       "target template part is not available");
        return false;
    }

    const auto entry_name = std::string{part.entry_name()};
    if (options.include_bookmark_slots) {
        const auto bookmarks = part.list_bookmarks();
        if (last_error_info.code) {
            return false;
        }

        for (const auto &bookmark : bookmarks) {
            const auto slot_kind = bookmark_kind_to_template_slot_kind(bookmark.kind);
            if (!slot_kind.has_value()) {
                result.skipped_bookmarks.push_back(
                    {target, entry_name, bookmark,
                     "bookmark kind cannot be represented as a template slot"});
                continue;
            }

            featherdoc::template_schema_entry entry{};
            entry.target = target;
            entry.requirement.bookmark_name = bookmark.bookmark_name;
            entry.requirement.kind = *slot_kind;
            if (bookmark.occurrence_count > 1U) {
                entry.requirement.min_occurrences = bookmark.occurrence_count;
                entry.requirement.max_occurrences = bookmark.occurrence_count;
            }
            result.schema.entries.push_back(std::move(entry));
        }
    }

    if (options.include_content_control_slots) {
        const auto content_controls = part.list_content_controls();
        if (last_error_info.code) {
            return false;
        }

        std::vector<featherdoc::template_schema_entry> content_control_entries;
        std::unordered_map<std::string, std::size_t> content_control_counts;
        for (const auto &content_control : content_controls) {
            auto source = featherdoc::template_slot_source_kind::content_control_tag;
            std::optional<std::string> selector_value = content_control.tag;
            if (!selector_value.has_value()) {
                source = featherdoc::template_slot_source_kind::content_control_alias;
                selector_value = content_control.alias;
            }
            if (!selector_value.has_value()) {
                continue;
            }

            const auto key = template_slot_key(source, *selector_value);
            auto [count_it, inserted] = content_control_counts.emplace(key, 1U);
            if (!inserted) {
                ++count_it->second;
                continue;
            }

            featherdoc::template_schema_entry entry{};
            entry.target = target;
            entry.requirement.bookmark_name = *selector_value;
            entry.requirement.kind = content_control_kind_to_template_slot_kind(content_control.kind);
            entry.requirement.source = source;
            content_control_entries.push_back(std::move(entry));
        }

        for (auto &entry : content_control_entries) {
            const auto key = template_slot_key(entry.requirement.source,
                                               entry.requirement.bookmark_name);
            if (const auto count_it = content_control_counts.find(key);
                count_it != content_control_counts.end() && count_it->second > 1U) {
                entry.requirement.min_occurrences = count_it->second;
                entry.requirement.max_occurrences = count_it->second;
            }
            result.schema.entries.push_back(std::move(entry));
        }
    }

    return true;
}

auto find_template_part_by_entry_name(featherdoc::Document &doc,
                                      bool header_part,
                                      std::string_view entry_name)
    -> featherdoc::TemplatePart {
    const auto total = header_part ? doc.header_count() : doc.footer_count();
    for (std::size_t index = 0U; index < total; ++index) {
        auto part = header_part ? doc.header_template(index) : doc.footer_template(index);
        if (part && part.entry_name() == entry_name) {
            return part;
        }
    }

    return {};
}
