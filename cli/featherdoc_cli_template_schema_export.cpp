#include "featherdoc_cli_template_schema_export.hpp"

#include "featherdoc_cli_domain_names.hpp"
#include "featherdoc_cli_errors.hpp"

#include <cstddef>
#include <optional>
#include <string>
#include <system_error>
#include <unordered_map>
#include <utility>
#include <vector>

namespace featherdoc_cli {
namespace {

enum class section_part_family {
    header,
    footer,
};

auto bookmark_kind_to_template_slot_kind(featherdoc::bookmark_kind kind)
    -> std::optional<featherdoc::template_slot_kind> {
    switch (kind) {
    case featherdoc::bookmark_kind::text:
        return featherdoc::template_slot_kind::text;
    case featherdoc::bookmark_kind::block:
        return featherdoc::template_slot_kind::block;
    case featherdoc::bookmark_kind::table_rows:
        return featherdoc::template_slot_kind::table_rows;
    case featherdoc::bookmark_kind::block_range:
    case featherdoc::bookmark_kind::malformed:
    case featherdoc::bookmark_kind::mixed:
        return std::nullopt;
    }

    return std::nullopt;
}

auto content_control_kind_to_template_slot_kind(featherdoc::content_control_kind kind)
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

auto append_exported_template_schema_target(
    featherdoc::Document &doc, validation_part_family part_family,
    std::optional<std::size_t> part_index,
    std::optional<std::size_t> section_index,
    std::optional<featherdoc::section_reference_kind> reference_kind,
    std::optional<std::size_t> resolved_from_section_index,
    bool linked_to_previous, featherdoc::TemplatePart part,
    exported_template_schema_result &result, std::string_view command,
    bool json_output) -> bool {
    if (!part) {
        featherdoc::document_error_info error_info{};
        error_info.code = std::make_error_code(std::errc::invalid_argument);
        error_info.detail = "target template part is not available";
        return report_operation_failure(command, "inspect",
                                        "failed to inspect template part",
                                        error_info, json_output);
    }

    const auto entry_name = std::string(part.entry_name());
    const auto bookmarks = part.list_bookmarks();
    if (const auto &error_info = doc.last_error(); error_info.code) {
        return report_document_error(command, "inspect", error_info, json_output);
    }

    exported_template_schema_target target{};
    target.part = part_family;
    target.part_index = part_index;
    target.section_index = section_index;
    target.reference_kind = reference_kind;
    target.resolved_from_section_index = resolved_from_section_index;
    target.linked_to_previous = linked_to_previous;
    target.entry_name = entry_name;

    for (const auto &bookmark : bookmarks) {
        const auto slot_kind = bookmark_kind_to_template_slot_kind(bookmark.kind);
        if (!slot_kind.has_value()) {
            result.skipped_bookmarks.push_back(
                {part_family,
                 part_index,
                 section_index,
                 reference_kind,
                 resolved_from_section_index,
                 linked_to_previous,
                 entry_name,
                 bookmark,
                 "bookmark kind cannot be represented as a template slot"});
            continue;
        }

        featherdoc::template_slot_requirement requirement{};
        requirement.bookmark_name = bookmark.bookmark_name;
        requirement.kind = *slot_kind;
        if (bookmark.occurrence_count > 1U) {
            requirement.min_occurrences = bookmark.occurrence_count;
            requirement.max_occurrences = bookmark.occurrence_count;
        }
        target.requirements.push_back(std::move(requirement));
    }

    const auto content_controls = part.list_content_controls();
    if (const auto &error_info = doc.last_error(); error_info.code) {
        return report_document_error(command, "inspect", error_info, json_output);
    }

    std::vector<featherdoc::template_slot_requirement> content_control_requirements;
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

        const auto key = std::string{template_slot_source_json_name(source)} + ":" +
                         *selector_value;
        auto [count_it, inserted] = content_control_counts.emplace(key, 1U);
        if (!inserted) {
            ++count_it->second;
            continue;
        }

        featherdoc::template_slot_requirement requirement{};
        requirement.bookmark_name = *selector_value;
        requirement.kind =
            content_control_kind_to_template_slot_kind(content_control.kind);
        requirement.source = source;
        content_control_requirements.push_back(std::move(requirement));
    }
    for (auto &requirement : content_control_requirements) {
        const auto key = std::string{template_slot_source_json_name(requirement.source)} +
                         ":" + requirement.bookmark_name;
        if (const auto count_it = content_control_counts.find(key);
            count_it != content_control_counts.end() && count_it->second > 1U) {
            requirement.min_occurrences = count_it->second;
            requirement.max_occurrences = count_it->second;
        }
        target.requirements.push_back(std::move(requirement));
    }

    if (!target.requirements.empty()) {
        result.targets.push_back(std::move(target));
    }

    return true;
}

auto append_exported_section_targets(featherdoc::Document &doc,
                                     exported_template_schema_result &result,
                                     std::string_view command, bool json_output)
    -> bool {
    const auto sections = doc.inspect_sections();
    if (const auto &error_info = doc.last_error(); error_info.code) {
        return report_document_error(command, "inspect", error_info, json_output);
    }

    for (const auto &section : sections.sections) {
        const auto append_if_present =
            [&](validation_part_family part_family,
                featherdoc::section_reference_kind reference_kind,
                bool present) -> bool {
            if (!present) {
                return true;
            }

            const auto template_part =
                part_family == validation_part_family::section_header
                    ? doc.section_header_template(section.index, reference_kind)
                    : doc.section_footer_template(section.index, reference_kind);
            return append_exported_template_schema_target(
                doc, part_family, std::nullopt, section.index, reference_kind,
                std::nullopt, false, template_part, result, command, json_output);
        };

        if (!append_if_present(validation_part_family::section_header,
                               featherdoc::section_reference_kind::default_reference,
                               section.header.has_default) ||
            !append_if_present(validation_part_family::section_header,
                               featherdoc::section_reference_kind::first_page,
                               section.header.has_first) ||
            !append_if_present(validation_part_family::section_header,
                               featherdoc::section_reference_kind::even_page,
                               section.header.has_even) ||
            !append_if_present(validation_part_family::section_footer,
                               featherdoc::section_reference_kind::default_reference,
                               section.footer.has_default) ||
            !append_if_present(validation_part_family::section_footer,
                               featherdoc::section_reference_kind::first_page,
                               section.footer.has_first) ||
            !append_if_present(validation_part_family::section_footer,
                               featherdoc::section_reference_kind::even_page,
                               section.footer.has_even)) {
            return false;
        }
    }

    return true;
}

auto find_related_template_part_by_entry_name(
    featherdoc::Document &doc, section_part_family family, std::string_view entry_name,
    std::optional<std::size_t> &part_index, featherdoc::TemplatePart &part) -> bool {
    const auto total =
        family == section_part_family::header ? doc.header_count() : doc.footer_count();
    for (std::size_t index = 0U; index < total; ++index) {
        auto current =
            family == section_part_family::header ? doc.header_template(index)
                                                  : doc.footer_template(index);
        if (current && current.entry_name() == entry_name) {
            part_index = index;
            part = current;
            return true;
        }
    }

    return false;
}

auto append_exported_resolved_section_targets(featherdoc::Document &doc,
                                              exported_template_schema_result &result,
                                              std::string_view command,
                                              bool json_output) -> bool {
    const auto sections = doc.inspect_sections();
    if (const auto &error_info = doc.last_error(); error_info.code) {
        return report_document_error(command, "inspect", error_info, json_output);
    }

    for (const auto &section : sections.sections) {
        const auto append_if_resolved =
            [&](validation_part_family part_family, section_part_family part_group,
                featherdoc::section_reference_kind reference_kind,
                const std::optional<std::string> &resolved_entry_name,
                const std::optional<std::size_t> &resolved_section_index) -> bool {
            if (!resolved_entry_name.has_value() || !resolved_section_index.has_value()) {
                return true;
            }

            std::optional<std::size_t> part_index;
            featherdoc::TemplatePart part;
            if (!find_related_template_part_by_entry_name(doc, part_group,
                                                          *resolved_entry_name,
                                                          part_index, part)) {
                featherdoc::document_error_info error_info{};
                error_info.code = std::make_error_code(std::errc::invalid_argument);
                error_info.detail =
                    "failed to resolve related part by entry name during schema export";
                error_info.entry_name = *resolved_entry_name;
                return report_operation_failure(command, "inspect",
                                                "failed to resolve related part",
                                                error_info, json_output);
            }

            return append_exported_template_schema_target(
                doc, part_family, std::nullopt, section.index, reference_kind,
                *resolved_section_index, *resolved_section_index != section.index, part,
                result, command, json_output);
        };

        if (!append_if_resolved(validation_part_family::section_header,
                                section_part_family::header,
                                featherdoc::section_reference_kind::default_reference,
                                section.header.resolved_default_entry_name,
                                section.header.resolved_default_section_index) ||
            !append_if_resolved(validation_part_family::section_header,
                                section_part_family::header,
                                featherdoc::section_reference_kind::first_page,
                                section.header.resolved_first_entry_name,
                                section.header.resolved_first_section_index) ||
            !append_if_resolved(validation_part_family::section_header,
                                section_part_family::header,
                                featherdoc::section_reference_kind::even_page,
                                section.header.resolved_even_entry_name,
                                section.header.resolved_even_section_index) ||
            !append_if_resolved(validation_part_family::section_footer,
                                section_part_family::footer,
                                featherdoc::section_reference_kind::default_reference,
                                section.footer.resolved_default_entry_name,
                                section.footer.resolved_default_section_index) ||
            !append_if_resolved(validation_part_family::section_footer,
                                section_part_family::footer,
                                featherdoc::section_reference_kind::first_page,
                                section.footer.resolved_first_entry_name,
                                section.footer.resolved_first_section_index) ||
            !append_if_resolved(validation_part_family::section_footer,
                                section_part_family::footer,
                                featherdoc::section_reference_kind::even_page,
                                section.footer.resolved_even_entry_name,
                                section.footer.resolved_even_section_index)) {
            return false;
        }
    }

    return true;
}

} // namespace

auto build_exported_template_schema(featherdoc::Document &doc,
                                    bool section_targets,
                                    bool resolved_section_targets,
                                    exported_template_schema_result &result,
                                    std::string_view command, bool json_output)
    -> bool {
    result = {};
    if (!append_exported_template_schema_target(
            doc, validation_part_family::body, std::nullopt, std::nullopt,
            std::nullopt, std::nullopt, false, doc.body_template(), result, command,
            json_output)) {
        return false;
    }

    if (resolved_section_targets) {
        if (!append_exported_resolved_section_targets(doc, result, command, json_output)) {
            return false;
        }
    } else if (section_targets) {
        if (!append_exported_section_targets(doc, result, command, json_output)) {
            return false;
        }
    } else {
        for (std::size_t index = 0U; index < doc.header_count(); ++index) {
            if (!append_exported_template_schema_target(
                    doc, validation_part_family::header, index, std::nullopt,
                    std::nullopt, std::nullopt, false, doc.header_template(index),
                    result, command, json_output)) {
                return false;
            }
        }

        for (std::size_t index = 0U; index < doc.footer_count(); ++index) {
            if (!append_exported_template_schema_target(
                    doc, validation_part_family::footer, index, std::nullopt,
                    std::nullopt, std::nullopt, false, doc.footer_template(index),
                    result, command, json_output)) {
                return false;
            }
        }
    }

    if (!result.targets.empty()) {
        return true;
    }

    featherdoc::document_error_info error_info{};
    error_info.code = std::make_error_code(std::errc::invalid_argument);
    error_info.detail = "document does not contain any exportable template bookmarks";
    return report_operation_failure(command, "inspect",
                                    "failed to export template schema", error_info,
                                    json_output);
}

} // namespace featherdoc_cli
