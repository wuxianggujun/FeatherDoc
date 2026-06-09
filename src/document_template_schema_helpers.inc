auto effective_min_occurrences(
    const featherdoc::template_slot_requirement &requirement) -> std::size_t {
    return requirement.min_occurrences.value_or(requirement.required ? 1U : 0U);
}

auto effective_max_occurrences(
    const featherdoc::template_slot_requirement &requirement)
    -> std::optional<std::size_t> {
    return requirement.max_occurrences;
}

auto template_slot_source_prefix(featherdoc::template_slot_source_kind source)
    -> std::string_view {
    switch (source) {
    case featherdoc::template_slot_source_kind::bookmark:
        return "bookmark";
    case featherdoc::template_slot_source_kind::content_control_tag:
        return "content_control_tag";
    case featherdoc::template_slot_source_kind::content_control_alias:
        return "content_control_alias";
    }

    return "bookmark";
}

auto template_slot_key(const featherdoc::template_slot_requirement &requirement)
    -> std::string {
    if (requirement.source == featherdoc::template_slot_source_kind::bookmark) {
        return requirement.bookmark_name;
    }

    auto key = std::string{template_slot_source_prefix(requirement.source)};
    key.push_back(':');
    key += requirement.bookmark_name;
    return key;
}

auto template_slot_key(featherdoc::template_slot_source_kind source,
                       std::string_view name) -> std::string {
    if (source == featherdoc::template_slot_source_kind::bookmark) {
        return std::string{name};
    }

    auto key = std::string{template_slot_source_prefix(source)};
    key.push_back(':');
    key += name;
    return key;
}

auto content_control_kind_to_template_slot_kind(
    featherdoc::content_control_kind kind)
    -> featherdoc::template_slot_kind {
    switch (kind) {
    case featherdoc::content_control_kind::run:
        return featherdoc::template_slot_kind::text;
    case featherdoc::content_control_kind::table_row:
        return featherdoc::template_slot_kind::table_rows;
    case featherdoc::content_control_kind::block:
    case featherdoc::content_control_kind::table_cell:
    case featherdoc::content_control_kind::unknown:
        return featherdoc::template_slot_kind::block;
    }

    return featherdoc::template_slot_kind::block;
}

auto content_control_kind_to_bookmark_kind(
    featherdoc::content_control_kind kind) -> featherdoc::bookmark_kind {
    switch (kind) {
    case featherdoc::content_control_kind::run:
        return featherdoc::bookmark_kind::text;
    case featherdoc::content_control_kind::table_row:
        return featherdoc::bookmark_kind::table_rows;
    case featherdoc::content_control_kind::block:
    case featherdoc::content_control_kind::table_cell:
        return featherdoc::bookmark_kind::block;
    case featherdoc::content_control_kind::unknown:
        return featherdoc::bookmark_kind::malformed;
    }

    return featherdoc::bookmark_kind::malformed;
}

auto requirement_matches_content_control_kind(
    const featherdoc::template_slot_requirement &requirement,
    featherdoc::content_control_kind actual_kind) -> bool {
    return requirement.kind == content_control_kind_to_template_slot_kind(actual_kind);
}

auto template_schema_selector_equals(
    const featherdoc::template_schema_part_selector &left,
    const featherdoc::template_schema_part_selector &right) -> bool {
    return left.part == right.part && left.part_index == right.part_index &&
           left.section_index == right.section_index &&
           left.reference_kind == right.reference_kind;
}

auto normalize_template_schema_selector(
    const featherdoc::template_schema_part_selector &selector,
    featherdoc::template_schema_part_selector &normalized, std::string &error_detail) -> bool;

auto compare_optional_index(const std::optional<std::size_t> &left,
                            const std::optional<std::size_t> &right) -> int {
    if (left.has_value() != right.has_value()) {
        return left.has_value() ? 1 : -1;
    }
    if (!left.has_value()) {
        return 0;
    }
    if (*left < *right) {
        return -1;
    }
    if (*right < *left) {
        return 1;
    }
    return 0;
}

auto compare_template_schema_selector(
    const featherdoc::template_schema_part_selector &left,
    const featherdoc::template_schema_part_selector &right) -> int {
    if (static_cast<int>(left.part) < static_cast<int>(right.part)) {
        return -1;
    }
    if (static_cast<int>(right.part) < static_cast<int>(left.part)) {
        return 1;
    }
    if (const auto part_index_compare =
            compare_optional_index(left.part_index, right.part_index);
        part_index_compare != 0) {
        return part_index_compare;
    }
    if (const auto section_index_compare =
            compare_optional_index(left.section_index, right.section_index);
        section_index_compare != 0) {
        return section_index_compare;
    }
    if (static_cast<int>(left.reference_kind) <
        static_cast<int>(right.reference_kind)) {
        return -1;
    }
    if (static_cast<int>(right.reference_kind) <
        static_cast<int>(left.reference_kind)) {
        return 1;
    }
    return 0;
}

auto compare_template_slot_requirement(
    const featherdoc::template_slot_requirement &left,
    const featherdoc::template_slot_requirement &right) -> int {
    if (static_cast<int>(left.source) < static_cast<int>(right.source)) {
        return -1;
    }
    if (static_cast<int>(right.source) < static_cast<int>(left.source)) {
        return 1;
    }
    if (left.bookmark_name < right.bookmark_name) {
        return -1;
    }
    if (right.bookmark_name < left.bookmark_name) {
        return 1;
    }
    if (static_cast<int>(left.kind) < static_cast<int>(right.kind)) {
        return -1;
    }
    if (static_cast<int>(right.kind) < static_cast<int>(left.kind)) {
        return 1;
    }
    if (left.required != right.required) {
        return left.required ? 1 : -1;
    }
    if (const auto min_compare =
            compare_optional_index(left.min_occurrences, right.min_occurrences);
        min_compare != 0) {
        return min_compare;
    }
    return compare_optional_index(left.max_occurrences, right.max_occurrences);
}

auto compare_template_slot_requirement_shape(
    const featherdoc::template_slot_requirement &left,
    const featherdoc::template_slot_requirement &right) -> int {
    if (static_cast<int>(left.source) < static_cast<int>(right.source)) {
        return -1;
    }
    if (static_cast<int>(right.source) < static_cast<int>(left.source)) {
        return 1;
    }
    if (static_cast<int>(left.kind) < static_cast<int>(right.kind)) {
        return -1;
    }
    if (static_cast<int>(right.kind) < static_cast<int>(left.kind)) {
        return 1;
    }
    if (left.required != right.required) {
        return left.required ? 1 : -1;
    }
    if (const auto min_compare =
            compare_optional_index(left.min_occurrences, right.min_occurrences);
        min_compare != 0) {
        return min_compare;
    }
    return compare_optional_index(left.max_occurrences, right.max_occurrences);
}

auto template_slot_requirement_equals(
    const featherdoc::template_slot_requirement &left,
    const featherdoc::template_slot_requirement &right) -> bool {
    return compare_template_slot_requirement(left, right) == 0;
}

auto template_schema_entry_equals(const featherdoc::template_schema_entry &left,
                                  const featherdoc::template_schema_entry &right)
    -> bool {
    return template_schema_selector_equals(left.target, right.target) &&
           template_slot_requirement_equals(left.requirement, right.requirement);
}

auto make_template_schema_slot_update(
    const featherdoc::template_slot_requirement &left,
    const featherdoc::template_slot_requirement &right)
    -> featherdoc::template_schema_slot_update {
    featherdoc::template_schema_slot_update update{};
    if (left.kind != right.kind) {
        update.kind = right.kind;
    }
    if (left.required != right.required) {
        update.required = right.required;
    }
    if (left.min_occurrences != right.min_occurrences) {
        if (right.min_occurrences.has_value()) {
            update.min_occurrences = *right.min_occurrences;
        } else {
            update.clear_min_occurrences = true;
        }
    }
    if (left.max_occurrences != right.max_occurrences) {
        if (right.max_occurrences.has_value()) {
            update.max_occurrences = *right.max_occurrences;
        } else {
            update.clear_max_occurrences = true;
        }
    }
    return update;
}

void write_schema_review_json_string(std::ostream &stream,
                                     std::string_view text) {
    constexpr auto hex_digits = std::string_view{"0123456789abcdef"};
    stream << '"';
    for (const unsigned char value : text) {
        switch (value) {
        case '\\':
            stream << "\\\\";
            break;
        case '"':
            stream << "\\\"";
            break;
        case '\b':
            stream << "\\b";
            break;
        case '\f':
            stream << "\\f";
            break;
        case '\n':
            stream << "\\n";
            break;
        case '\r':
            stream << "\\r";
            break;
        case '\t':
            stream << "\\t";
            break;
        default:
            if (value < 0x20U) {
                stream << "\\u00" << hex_digits[value >> 4U]
                       << hex_digits[value & 0x0FU];
            } else {
                stream << static_cast<char>(value);
            }
            break;
        }
    }
    stream << '"';
}

auto template_schema_slot_count(featherdoc::template_schema schema) -> std::size_t {
    (void)featherdoc::normalize_template_schema(schema);
    return schema.entries.size();
}

auto template_schema_entries_equal(
    const std::vector<featherdoc::template_schema_entry> &left,
    const std::vector<featherdoc::template_schema_entry> &right) -> bool {
    if (left.size() != right.size()) {
        return false;
    }
    for (std::size_t index = 0U; index < left.size(); ++index) {
        if (!template_schema_entry_equals(left[index], right[index])) {
            return false;
        }
    }
    return true;
}

auto canonical_template_schema_selector(
    const featherdoc::template_schema_part_selector &selector)
    -> featherdoc::template_schema_part_selector {
    featherdoc::template_schema_part_selector normalized{};
    std::string error_detail;
    if (normalize_template_schema_selector(selector, normalized, error_detail)) {
        return normalized;
    }
    return selector;
}

auto template_schema_entry_less(const featherdoc::template_schema_entry &left,
                                const featherdoc::template_schema_entry &right)
    -> bool {
    if (const auto selector_compare =
            compare_template_schema_selector(left.target, right.target);
        selector_compare != 0) {
        return selector_compare < 0;
    }
    return compare_template_slot_requirement(left.requirement, right.requirement) < 0;
}

auto find_template_schema_slot(
    std::vector<featherdoc::template_schema_entry> &entries,
    const featherdoc::template_schema_part_selector &target,
    featherdoc::template_slot_source_kind source, std::string_view bookmark_name) {
    return std::find_if(entries.begin(), entries.end(),
                        [&](const featherdoc::template_schema_entry &entry) {
                            return template_schema_selector_equals(entry.target, target) &&
                                   entry.requirement.source == source &&
                                   entry.requirement.bookmark_name == bookmark_name;
                        });
}

auto template_schema_has_target(const featherdoc::template_schema &schema,
                                const featherdoc::template_schema_part_selector &target)
    -> bool {
    return std::any_of(schema.entries.begin(), schema.entries.end(),
                       [&](const featherdoc::template_schema_entry &entry) {
                           return template_schema_selector_equals(entry.target, target);
                       });
}

auto normalize_template_schema_selector(
    const featherdoc::template_schema_part_selector &selector,
    featherdoc::template_schema_part_selector &normalized, std::string &error_detail) -> bool {
    normalized = selector;

    switch (selector.part) {
    case featherdoc::template_schema_part_kind::body:
        if (selector.part_index.has_value() || selector.section_index.has_value()) {
            error_detail =
                "body schema target does not accept part_index or section_index";
            return false;
        }
        if (selector.reference_kind != featherdoc::section_reference_kind::default_reference) {
            error_detail =
                "body schema target does not accept a non-default reference kind";
            return false;
        }
        break;
    case featherdoc::template_schema_part_kind::header:
    case featherdoc::template_schema_part_kind::footer:
        if (selector.section_index.has_value()) {
            error_detail = "header/footer schema target does not accept section_index";
            return false;
        }
        if (selector.reference_kind != featherdoc::section_reference_kind::default_reference) {
            error_detail =
                "header/footer schema target does not accept a non-default reference kind";
            return false;
        }
        normalized.part_index = selector.part_index.value_or(0U);
        break;
    case featherdoc::template_schema_part_kind::section_header:
    case featherdoc::template_schema_part_kind::section_footer:
        if (!selector.section_index.has_value()) {
            error_detail =
                "section-header/section-footer schema target requires section_index";
            return false;
        }
        if (selector.part_index.has_value()) {
            error_detail =
                "section-header/section-footer schema target does not accept part_index";
            return false;
        }
        break;
    }

    return true;
}

auto make_unavailable_part_validation(
    std::span<const featherdoc::template_slot_requirement> requirements)
    -> featherdoc::template_validation_result {
    featherdoc::template_validation_result result;
    std::unordered_set<std::string> missing_seen;
    for (const auto &requirement : requirements) {
        const auto slot_key = template_slot_key(requirement);
        if (effective_min_occurrences(requirement) > 0U &&
            missing_seen.emplace(slot_key).second) {
            result.missing_required.push_back(slot_key);
        }
    }

    return result;
}

auto requirement_matches_bookmark_kind(
    const featherdoc::template_slot_requirement &requirement,
    featherdoc::bookmark_kind actual_kind) -> bool {
    switch (requirement.kind) {
    case featherdoc::template_slot_kind::text:
        return actual_kind == featherdoc::bookmark_kind::text;
    case featherdoc::template_slot_kind::table_rows:
        return actual_kind == featherdoc::bookmark_kind::table_rows;
    case featherdoc::template_slot_kind::table:
    case featherdoc::template_slot_kind::image:
    case featherdoc::template_slot_kind::floating_image:
    case featherdoc::template_slot_kind::block:
        return actual_kind == featherdoc::bookmark_kind::block;
    }

    return false;
}

auto requirement_matches_named_bookmark_kinds(
    pugi::xml_document &document, std::string_view bookmark_name,
    const featherdoc::template_slot_requirement &requirement) -> bool {
    std::vector<pugi::xml_node> bookmark_starts;
    collect_named_bookmark_starts(document, bookmark_name, bookmark_starts);
    for (const auto bookmark_start : bookmark_starts) {
        if (!requirement_matches_bookmark_kind(requirement,
                                               classify_bookmark_start_kind(bookmark_start))) {
            return false;
        }
    }

    return true;
}

auto validate_template_in_part(
    featherdoc::document_error_info &last_error_info, pugi::xml_document &document,
    std::string_view entry_name,
    std::span<const featherdoc::template_slot_requirement> requirements)
    -> featherdoc::template_validation_result {
    featherdoc::template_validation_result result;

    const auto bookmark_summaries =
        summarize_bookmarks_in_part(last_error_info, document);
    if (last_error_info.code) {
        return result;
    }
    const auto content_control_summaries =
        summarize_content_controls_in_part(last_error_info, document);
    if (last_error_info.code) {
        return result;
    }

    std::unordered_map<std::string, featherdoc::bookmark_summary> bookmarks_by_name;
    bookmarks_by_name.reserve(bookmark_summaries.size());
    for (const auto &summary : bookmark_summaries) {
        bookmarks_by_name.emplace(summary.bookmark_name, summary);
    }

    std::unordered_map<std::string, std::vector<featherdoc::content_control_summary>>
        content_controls_by_selector;
    for (const auto &summary : content_control_summaries) {
        if (summary.tag.has_value()) {
            content_controls_by_selector[template_slot_key(
                                             featherdoc::template_slot_source_kind::content_control_tag,
                                             *summary.tag)]
                .push_back(summary);
        }
        if (summary.alias.has_value()) {
            content_controls_by_selector[template_slot_key(
                                             featherdoc::template_slot_source_kind::content_control_alias,
                                             *summary.alias)]
                .push_back(summary);
        }
    }

    std::unordered_set<std::string> declared_slots;
    std::unordered_set<std::string> declared_bookmarks;
    std::unordered_set<std::string> missing_seen;
    std::unordered_set<std::string> duplicates_seen;
    std::unordered_set<std::string> malformed_seen;
    std::unordered_set<std::string> kind_mismatch_seen;
    std::unordered_set<std::string> occurrence_mismatch_seen;
    for (const auto &requirement : requirements) {
        const auto slot_key = template_slot_key(requirement);
        if (requirement.bookmark_name.empty()) {
            const auto *message =
                requirement.source == featherdoc::template_slot_source_kind::bookmark
                    ? "template slot bookmark name must not be empty"
                    : "template slot name must not be empty";
            set_last_error(last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           message, std::string{entry_name});
            return result;
        }

        const auto [_, inserted] = declared_slots.emplace(slot_key);
        if (!inserted) {
            set_last_error(last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           "duplicate template slot requirement for slot '" +
                               slot_key + "'",
                           std::string{entry_name});
            return result;
        }

        if (requirement.source == featherdoc::template_slot_source_kind::bookmark) {
            declared_bookmarks.emplace(requirement.bookmark_name);
        }

        const auto min_occurrences = effective_min_occurrences(requirement);
        const auto max_occurrences = effective_max_occurrences(requirement);
        if (max_occurrences.has_value() && *max_occurrences < min_occurrences) {
            set_last_error(last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           "template slot occurrence range is invalid for slot '" +
                               slot_key + "'",
                           std::string{entry_name});
            return result;
        }

        if (requirement.source != featherdoc::template_slot_source_kind::bookmark) {
            const auto control_it = content_controls_by_selector.find(slot_key);
            const auto content_control_count =
                control_it == content_controls_by_selector.end()
                    ? 0U
                    : control_it->second.size();

            if (min_occurrences > 0U && content_control_count == 0U &&
                missing_seen.emplace(slot_key).second) {
                result.missing_required.push_back(slot_key);
            }

            if (content_control_count > 1U &&
                (!max_occurrences.has_value() || *max_occurrences <= 1U) &&
                duplicates_seen.emplace(slot_key).second) {
                result.duplicate_bookmarks.push_back(slot_key);
            }

            if (content_control_count == 0U) {
                continue;
            }

            if ((content_control_count < min_occurrences ||
                 (max_occurrences.has_value() &&
                  content_control_count > *max_occurrences)) &&
                occurrence_mismatch_seen.emplace(slot_key).second) {
                result.occurrence_mismatches.push_back(
                    {slot_key, content_control_count, min_occurrences,
                     max_occurrences});
            }

            const auto mismatch_it = std::find_if(
                control_it->second.begin(), control_it->second.end(),
                [&](const featherdoc::content_control_summary &summary) {
                    return !requirement_matches_content_control_kind(requirement,
                                                                     summary.kind);
                });
            if (mismatch_it != control_it->second.end() &&
                kind_mismatch_seen.emplace(slot_key).second) {
                result.kind_mismatches.push_back(
                    {slot_key, requirement.kind,
                     content_control_kind_to_bookmark_kind(mismatch_it->kind),
                     content_control_count});
            }
            continue;
        }

        const auto summary_it = bookmarks_by_name.find(requirement.bookmark_name);
        const auto bookmark_count =
            summary_it == bookmarks_by_name.end() ? 0U : summary_it->second.occurrence_count;

        if (min_occurrences > 0U && bookmark_count == 0U &&
            missing_seen.emplace(slot_key).second) {
            result.missing_required.push_back(slot_key);
        }

        if (bookmark_count > 1U && (!max_occurrences.has_value() || *max_occurrences <= 1U) &&
            duplicates_seen.emplace(slot_key).second) {
            result.duplicate_bookmarks.push_back(slot_key);
        }

        if (bookmark_count == 0U) {
            continue;
        }

        if ((bookmark_count < min_occurrences ||
             (max_occurrences.has_value() && bookmark_count > *max_occurrences)) &&
            occurrence_mismatch_seen.emplace(slot_key).second) {
            result.occurrence_mismatches.push_back(
                {slot_key, bookmark_count, min_occurrences, max_occurrences});
        }

        if (!validate_placeholder_requirement(document, entry_name, requirement) &&
            malformed_seen.emplace(slot_key).second) {
            result.malformed_placeholders.push_back(slot_key);
            continue;
        }

        if (!requirement_matches_named_bookmark_kinds(document, requirement.bookmark_name,
                                                      requirement) &&
            kind_mismatch_seen.emplace(slot_key).second) {
            result.kind_mismatches.push_back({slot_key, requirement.kind,
                                              summary_it->second.kind,
                                              summary_it->second.occurrence_count});
        }
    }

    for (const auto &summary : bookmark_summaries) {
        if (!declared_bookmarks.contains(summary.bookmark_name)) {
            result.unexpected_bookmarks.push_back(summary);
        }
    }

    last_error_info.clear();
    return result;
}

#include "document_template_schema_onboarding_helpers.inc"
