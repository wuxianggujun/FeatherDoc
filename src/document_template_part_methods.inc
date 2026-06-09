TemplatePart::TemplatePart() = default;

TemplatePart::TemplatePart(Document *owner, pugi::xml_document *xml_document,
                           document_error_info *last_error_info, std::string entry_name)
    : owner(owner), xml_document(xml_document), last_error_info(last_error_info),
      entry_name_storage(std::move(entry_name)) {}

TemplatePart::operator bool() const noexcept { return this->xml_document != nullptr; }

std::string_view TemplatePart::entry_name() const noexcept {
    return this->entry_name_storage;
}

Paragraph TemplatePart::paragraphs() {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return {};
    }

    const auto container = template_part_block_container(*this->xml_document);
    if (container == pugi::xml_node{}) {
        set_last_error(*this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "template part does not contain a supported block container",
                       this->entry_name_storage);
        return {};
    }

    this->last_error_info->clear();
    return {container, container.child("w:p")};
}

Paragraph TemplatePart::append_paragraph(const std::string &text,
                                         featherdoc::formatting_flag formatting) {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return {};
    }

    const auto container = template_part_block_container(*this->xml_document);
    if (container == pugi::xml_node{}) {
        set_last_error(*this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "template part does not contain a supported block container",
                       this->entry_name_storage);
        return {};
    }

    const auto paragraph_node = detail::append_paragraph_node(container);
    if (paragraph_node == pugi::xml_node{}) {
        set_last_error(*this->last_error_info,
                       std::make_error_code(std::errc::io_error),
                       "failed to append paragraph to template part",
                       this->entry_name_storage);
        return {};
    }

    auto paragraph = Paragraph(container, paragraph_node);
    if (!text.empty() && !paragraph.add_run(text, formatting).has_next()) {
        set_last_error(*this->last_error_info,
                       std::make_error_code(std::errc::io_error),
                       "failed to append paragraph text to template part",
                       this->entry_name_storage);
        return {};
    }

    this->last_error_info->clear();
    return paragraph;
}

#include "document_template_part_field_methods.inc"

std::vector<featherdoc::omml_summary> TemplatePart::list_omml() const {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return {};
    }

    return summarize_omml_in_part(*this->last_error_info, *this->xml_document);
}

bool TemplatePart::append_omml(std::string_view omml_xml) {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return false;
    }

    return append_omml_to_part(*this->last_error_info, *this->xml_document,
                               this->entry_name_storage, omml_xml);
}

bool TemplatePart::replace_omml(std::size_t omml_index,
                                std::string_view omml_xml) {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return false;
    }

    return replace_omml_in_part(*this->last_error_info, *this->xml_document,
                                this->entry_name_storage, omml_index, omml_xml);
}

bool TemplatePart::remove_omml(std::size_t omml_index) {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return false;
    }

    return remove_omml_in_part(*this->last_error_info, *this->xml_document,
                               this->entry_name_storage, omml_index);
}

#include "document_template_part_table_methods.inc"

auto find_table_index_by_bookmark_in_part(featherdoc::document_error_info &last_error_info,
                                          pugi::xml_document &document,
                                          std::string_view entry_name,
                                          std::string_view bookmark_name)
    -> std::optional<std::size_t> {
    if (bookmark_name.empty()) {
        set_last_error(
            last_error_info, std::make_error_code(std::errc::invalid_argument),
            "bookmark name must not be empty when resolving a table by bookmark",
            std::string{entry_name});
        return std::nullopt;
    }

    const auto container = template_part_block_container(document);
    if (container == pugi::xml_node{}) {
        set_last_error(last_error_info, std::make_error_code(std::errc::invalid_argument),
                       "template part does not contain a supported block container",
                       std::string{entry_name});
        return std::nullopt;
    }

    std::vector<pugi::xml_node> bookmark_starts;
    collect_named_bookmark_starts(document, bookmark_name, bookmark_starts);
    if (bookmark_starts.empty()) {
        set_last_error(last_error_info, std::make_error_code(std::errc::invalid_argument),
                       "bookmark name '" + std::string{bookmark_name} +
                           "' was not found in " + std::string{entry_name},
                       std::string{entry_name});
        return std::nullopt;
    }

    for (const auto bookmark_start : bookmark_starts) {
        if (const auto table = find_top_level_table_ancestor(bookmark_start, container);
            table != pugi::xml_node{}) {
            if (const auto table_index = find_top_level_table_index(container, table)) {
                last_error_info.clear();
                return table_index;
            }
        }

        const auto bookmark_end = find_matching_bookmark_end_in_document_order(bookmark_start);
        if (bookmark_end != pugi::xml_node{}) {
            if (const auto table =
                    find_first_top_level_table_after_node(container, bookmark_start, bookmark_end);
                table != pugi::xml_node{}) {
                if (const auto table_index = find_top_level_table_index(container, table)) {
                    last_error_info.clear();
                    return table_index;
                }
            }

            if (const auto table = find_top_level_table_ancestor(bookmark_end, container);
                table != pugi::xml_node{}) {
                if (const auto table_index = find_top_level_table_index(container, table)) {
                    last_error_info.clear();
                    return table_index;
                }
            }
        }

        const auto search_anchor =
            bookmark_end != pugi::xml_node{} ? bookmark_end : bookmark_start;
        if (const auto table =
                find_first_top_level_table_after_node(container, search_anchor);
            table != pugi::xml_node{}) {
            if (const auto table_index = find_top_level_table_index(container, table)) {
                last_error_info.clear();
                return table_index;
            }
        }
    }

    set_last_error(last_error_info, std::make_error_code(std::errc::invalid_argument),
                   "bookmark name '" + std::string{bookmark_name} +
                       "' did not resolve to a table in " + std::string{entry_name},
                   std::string{entry_name});
    return std::nullopt;
}

#include "document_template_part_paragraph_inspection_methods.inc"

#include "document_template_part_bookmark_methods.inc"

#include "document_template_part_content_access_methods.inc"

#include "document_template_part_content_control_replacement_methods.inc"

#include "document_template_part_table_lookup_methods.inc"

template_validation_result TemplatePart::validate_template(
    std::span<const template_slot_requirement> requirements) const {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return {};
    }

    return validate_template_in_part(*this->last_error_info, *this->xml_document,
                                     this->entry_name_storage, requirements);
}

template_validation_result TemplatePart::validate_template(
    std::initializer_list<template_slot_requirement> requirements) const {
    return this->validate_template(
        std::span<const template_slot_requirement>{requirements.begin(),
                                                   requirements.size()});
}

#include "document_template_part_bookmark_replacement_methods.inc"
