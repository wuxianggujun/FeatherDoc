#include "document_section_xml_helpers.hpp"
#include "featherdoc.hpp"

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {
constexpr auto document_xml_entry = std::string_view{"word/document.xml"};

auto set_last_error(featherdoc::document_error_info &error_info,
                    std::error_code code, std::string detail = {},
                    std::string entry_name = {},
                    std::optional<std::ptrdiff_t> xml_offset = std::nullopt)
    -> std::error_code {
    error_info.code = code;
    error_info.detail = std::move(detail);
    error_info.entry_name = std::move(entry_name);
    error_info.xml_offset = xml_offset;
    return code;
}

auto set_last_error(featherdoc::document_error_info &error_info,
                    featherdoc::document_errc code, std::string detail = {},
                    std::string entry_name = {},
                    std::optional<std::ptrdiff_t> xml_offset = std::nullopt)
    -> std::error_code {
    return set_last_error(error_info, featherdoc::make_error_code(code),
                          std::move(detail), std::move(entry_name), xml_offset);
}

auto parse_section_reference_kind(std::string_view value)
    -> std::optional<featherdoc::section_reference_kind> {
    if (value.empty() || value == "default") {
        return featherdoc::section_reference_kind::default_reference;
    }
    if (value == "first") {
        return featherdoc::section_reference_kind::first_page;
    }
    if (value == "even") {
        return featherdoc::section_reference_kind::even_page;
    }
    return std::nullopt;
}

} // namespace

namespace featherdoc {

using detail::read_on_off_value;

std::size_t Document::section_count() const noexcept {
    const auto body = this->document.child("w:document").child("w:body");
    if (body == pugi::xml_node{}) {
        return 0U;
    }

    std::size_t count = 1U;
    for (auto child = body.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        if (std::string_view{child.name()} != "w:p") {
            continue;
        }

        if (child.child("w:pPr").child("w:sectPr") != pugi::xml_node{}) {
            ++count;
        }
    }

    return count;
}

std::size_t Document::header_count() const noexcept {
    return this->header_parts.size();
}

std::size_t Document::footer_count() const noexcept {
    return this->footer_parts.size();
}

std::vector<featherdoc::related_part_inspection_summary>
Document::inspect_related_parts(
    const std::vector<std::unique_ptr<xml_part_state>> &parts,
    const char *reference_name) {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before inspecting related parts",
            std::string{document_xml_entry});
        return {};
    }

    auto summaries = std::vector<featherdoc::related_part_inspection_summary>{};
    summaries.reserve(parts.size());
    auto summary_index_by_relationship_id =
        std::unordered_map<std::string, std::size_t>{};
    summary_index_by_relationship_id.reserve(parts.size());
    for (std::size_t index = 0U; index < parts.size(); ++index) {
        auto summary = featherdoc::related_part_inspection_summary{};
        summary.index = index;
        if (parts[index] != nullptr) {
            summary.relationship_id = parts[index]->relationship_id;
            summary.entry_name = parts[index]->entry_name;
            if (!summary.relationship_id.empty()) {
                summary_index_by_relationship_id.try_emplace(
                    summary.relationship_id, index);
            }
            for (const auto &alias_id :
                 this->related_part_relationship_alias_ids(*parts[index])) {
                if (!alias_id.empty()) {
                    summary_index_by_relationship_id.try_emplace(alias_id,
                                                                 index);
                }
            }
        }
        summaries.push_back(std::move(summary));
    }

    const auto body = this->document.child("w:document").child("w:body");
    if (body == pugi::xml_node{}) {
        set_last_error(
            this->last_error_info, document_errc::invalid_package_structure,
            "word/document.xml does not contain a w:document/w:body root",
            std::string{document_xml_entry});
        return {};
    }

    const auto collect_references = [&](pugi::xml_node section_properties,
                                        std::size_t section_index) {
        for (auto reference = section_properties.child(reference_name);
             reference != pugi::xml_node{};
             reference = reference.next_sibling(reference_name)) {
            const auto relationship_id =
                std::string_view{reference.attribute("r:id").value()};
            if (relationship_id.empty()) {
                continue;
            }

            const auto summary_it = summary_index_by_relationship_id.find(
                std::string{relationship_id});
            if (summary_it == summary_index_by_relationship_id.end()) {
                continue;
            }

            const auto reference_kind = parse_section_reference_kind(
                reference.attribute("w:type").value());
            if (!reference_kind.has_value()) {
                continue;
            }

            summaries[summary_it->second].references.push_back(
                featherdoc::related_part_reference_inspection_summary{
                    section_index, *reference_kind});
        }
    };

    auto section_index = std::size_t{0U};
    for (auto child = body.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        if (std::string_view{child.name()} != "w:p") {
            continue;
        }

        const auto section_properties = child.child("w:pPr").child("w:sectPr");
        if (section_properties == pugi::xml_node{}) {
            continue;
        }

        collect_references(section_properties, section_index);
        ++section_index;
    }
    if (const auto final_section_properties = body.child("w:sectPr");
        final_section_properties != pugi::xml_node{}) {
        collect_references(final_section_properties, section_index);
    }

    this->last_error_info.clear();
    return summaries;
}

std::vector<featherdoc::related_part_inspection_summary>
Document::inspect_header_parts() {
    return this->inspect_related_parts(this->header_parts, "w:headerReference");
}

std::vector<featherdoc::related_part_inspection_summary>
Document::inspect_footer_parts() {
    return this->inspect_related_parts(this->footer_parts, "w:footerReference");
}

std::optional<bool> Document::inspect_even_and_odd_headers_enabled() {
    const auto previous_error = this->last_error_info;
    if (this->ensure_settings_loaded()) {
        this->last_error_info = previous_error;
        return std::nullopt;
    }

    const auto settings_root = this->settings.child("w:settings");
    if (settings_root == pugi::xml_node{}) {
        this->last_error_info = previous_error;
        return std::nullopt;
    }

    const auto enabled =
        read_on_off_value(settings_root.child("w:evenAndOddHeaders"))
            .value_or(false);
    this->last_error_info = previous_error;
    return enabled;
}

featherdoc::sections_inspection_summary Document::inspect_sections() {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before inspecting sections",
            std::string{document_xml_entry});
        return {};
    }

    auto summary = featherdoc::sections_inspection_summary{};
    summary.section_count = this->section_count();
    summary.header_count = this->header_count();
    summary.footer_count = this->footer_count();
    summary.even_and_odd_headers_enabled =
        this->inspect_even_and_odd_headers_enabled();
    summary.sections.reserve(summary.section_count);
    for (std::size_t section_index = 0U; section_index < summary.section_count;
         ++section_index) {
        if (auto section = this->inspect_section(section_index);
            section.has_value()) {
            section->even_and_odd_headers_enabled =
                summary.even_and_odd_headers_enabled;
            summary.sections.push_back(std::move(*section));
        }
    }

    this->last_error_info.clear();
    return summary;
}

std::optional<featherdoc::section_inspection_summary>
Document::inspect_section(std::size_t section_index) {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before inspecting sections",
            std::string{document_xml_entry});
        return std::nullopt;
    }

    if (section_index >= this->section_count()) {
        this->last_error_info.clear();
        return std::nullopt;
    }

    auto summary = featherdoc::section_inspection_summary{};
    summary.index = section_index;
    summary.even_and_odd_headers_enabled =
        this->inspect_even_and_odd_headers_enabled();
    const auto section_properties = this->section_properties(section_index);
    summary.different_first_page_enabled =
        read_on_off_value(section_properties.child("w:titlePg"))
            .value_or(false);

    const auto inspect_part =
        [this, section_index](
            featherdoc::section_part_inspection_summary &part_summary,
            featherdoc::section_reference_kind reference_kind,
            std::vector<std::unique_ptr<xml_part_state>> &parts,
            const char *reference_name) {
            const auto *part = this->section_related_part_state(
                section_index, reference_kind, parts, reference_name);
            if (reference_kind ==
                featherdoc::section_reference_kind::default_reference) {
                part_summary.has_default = part != nullptr;
                if (part != nullptr) {
                    part_summary.default_entry_name = part->entry_name;
                }
                return;
            }

            if (reference_kind ==
                featherdoc::section_reference_kind::first_page) {
                part_summary.has_first = part != nullptr;
                if (part != nullptr) {
                    part_summary.first_entry_name = part->entry_name;
                }
                return;
            }

            part_summary.has_even = part != nullptr;
            if (part != nullptr) {
                part_summary.even_entry_name = part->entry_name;
            }
        };
    const auto inspect_resolved_part =
        [this, section_index](
            featherdoc::section_part_inspection_summary &part_summary,
            featherdoc::section_reference_kind reference_kind,
            std::vector<std::unique_ptr<xml_part_state>> &parts,
            const char *reference_name) {
            for (std::size_t source_section_index = section_index + 1U;
                 source_section_index > 0U; --source_section_index) {
                const auto resolved_section_index = source_section_index - 1U;
                const auto *part = this->section_related_part_state(
                    resolved_section_index, reference_kind, parts,
                    reference_name);
                if (part == nullptr) {
                    continue;
                }

                const auto linked_to_previous =
                    resolved_section_index != section_index;
                if (reference_kind ==
                    featherdoc::section_reference_kind::default_reference) {
                    part_summary.default_linked_to_previous =
                        linked_to_previous;
                    part_summary.resolved_default_entry_name = part->entry_name;
                    part_summary.resolved_default_section_index =
                        resolved_section_index;
                    return;
                }

                if (reference_kind ==
                    featherdoc::section_reference_kind::first_page) {
                    part_summary.first_linked_to_previous = linked_to_previous;
                    part_summary.resolved_first_entry_name = part->entry_name;
                    part_summary.resolved_first_section_index =
                        resolved_section_index;
                    return;
                }

                part_summary.even_linked_to_previous = linked_to_previous;
                part_summary.resolved_even_entry_name = part->entry_name;
                part_summary.resolved_even_section_index =
                    resolved_section_index;
                return;
            }
        };

    inspect_part(summary.header,
                 featherdoc::section_reference_kind::default_reference,
                 this->header_parts, "w:headerReference");
    inspect_part(summary.header, featherdoc::section_reference_kind::first_page,
                 this->header_parts, "w:headerReference");
    inspect_part(summary.header, featherdoc::section_reference_kind::even_page,
                 this->header_parts, "w:headerReference");
    inspect_part(summary.footer,
                 featherdoc::section_reference_kind::default_reference,
                 this->footer_parts, "w:footerReference");
    inspect_part(summary.footer, featherdoc::section_reference_kind::first_page,
                 this->footer_parts, "w:footerReference");
    inspect_part(summary.footer, featherdoc::section_reference_kind::even_page,
                 this->footer_parts, "w:footerReference");
    inspect_resolved_part(summary.header,
                          featherdoc::section_reference_kind::default_reference,
                          this->header_parts, "w:headerReference");
    inspect_resolved_part(summary.header,
                          featherdoc::section_reference_kind::first_page,
                          this->header_parts, "w:headerReference");
    inspect_resolved_part(summary.header,
                          featherdoc::section_reference_kind::even_page,
                          this->header_parts, "w:headerReference");
    inspect_resolved_part(summary.footer,
                          featherdoc::section_reference_kind::default_reference,
                          this->footer_parts, "w:footerReference");
    inspect_resolved_part(summary.footer,
                          featherdoc::section_reference_kind::first_page,
                          this->footer_parts, "w:footerReference");
    inspect_resolved_part(summary.footer,
                          featherdoc::section_reference_kind::even_page,
                          this->footer_parts, "w:footerReference");

    this->last_error_info.clear();
    return summary;
}

} // namespace featherdoc
