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

std::vector<featherdoc::paragraph_inspection_summary> TemplatePart::inspect_paragraphs() {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return {};
    }

    auto summaries = std::vector<featherdoc::paragraph_inspection_summary>{};
    auto paragraph_handle = this->paragraphs();
    for (std::size_t paragraph_index = 0U; paragraph_handle.has_next();
         ++paragraph_index, paragraph_handle.next()) {
        auto summary = summarize_part_paragraph_node(paragraph_handle.current, paragraph_index);
        enrich_part_paragraph_numbering(this->owner, summary);
        summaries.push_back(std::move(summary));
    }

    this->last_error_info->clear();
    return summaries;
}

std::optional<featherdoc::paragraph_inspection_summary>
TemplatePart::inspect_paragraph(std::size_t paragraph_index) {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return std::nullopt;
    }

    auto paragraph_handle = this->paragraphs();
    for (std::size_t current_index = 0U;
         current_index < paragraph_index && paragraph_handle.has_next(); ++current_index) {
        paragraph_handle.next();
    }

    if (!paragraph_handle.has_next()) {
        this->last_error_info->clear();
        return std::nullopt;
    }

    auto summary = summarize_part_paragraph_node(paragraph_handle.current, paragraph_index);
    enrich_part_paragraph_numbering(this->owner, summary);
    this->last_error_info->clear();
    return summary;
}

std::vector<featherdoc::run_inspection_summary>
TemplatePart::inspect_paragraph_runs(std::size_t paragraph_index) {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return {};
    }

    auto paragraph_handle = this->paragraphs();
    for (std::size_t current_index = 0U;
         current_index < paragraph_index && paragraph_handle.has_next(); ++current_index) {
        paragraph_handle.next();
    }

    if (!paragraph_handle.has_next()) {
        this->last_error_info->clear();
        return {};
    }

    auto summaries = std::vector<featherdoc::run_inspection_summary>{};
    auto run = paragraph_handle.runs();
    for (std::size_t run_index = 0U; run.has_next(); ++run_index, run.next()) {
        summaries.push_back(summarize_part_run_handle(run, run_index));
    }

    this->last_error_info->clear();
    return summaries;
}

std::optional<featherdoc::run_inspection_summary>
TemplatePart::inspect_paragraph_run(std::size_t paragraph_index, std::size_t run_index) {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return std::nullopt;
    }

    const auto runs = this->inspect_paragraph_runs(paragraph_index);
    if (run_index >= runs.size()) {
        this->last_error_info->clear();
        return std::nullopt;
    }

    return runs[run_index];
}

std::size_t TemplatePart::replace_bookmark_text(const std::string &bookmark_name,
                                                const std::string &replacement) {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return 0U;
    }

    return replace_bookmark_text_in_part(*this->last_error_info, *this->xml_document,
                                         this->entry_name_storage, bookmark_name,
                                         replacement);
}

std::size_t TemplatePart::replace_bookmark_text(const char *bookmark_name,
                                                const char *replacement) {
    if (bookmark_name == nullptr || replacement == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           "bookmark name and replacement text must not be null",
                           this->entry_name_storage);
        }
        return 0U;
    }

    return this->replace_bookmark_text(std::string{bookmark_name},
                                       std::string{replacement});
}

bookmark_fill_result TemplatePart::fill_bookmarks(
    std::span<const bookmark_text_binding> bindings) {
    bookmark_fill_result result;
    result.requested = bindings.size();

    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return result;
    }

    return fill_bookmarks_in_part(*this->last_error_info, *this->xml_document,
                                  this->entry_name_storage, bindings);
}

bookmark_fill_result TemplatePart::fill_bookmarks(
    std::initializer_list<bookmark_text_binding> bindings) {
    return this->fill_bookmarks(
        std::span<const bookmark_text_binding>{bindings.begin(), bindings.size()});
}

std::vector<featherdoc::bookmark_summary> TemplatePart::list_bookmarks() const {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return {};
    }

    return summarize_bookmarks_in_part(*this->last_error_info, *this->xml_document);
}

std::optional<featherdoc::bookmark_summary> TemplatePart::find_bookmark(
    std::string_view bookmark_name) const {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return std::nullopt;
    }

    return find_bookmark_in_part(*this->last_error_info, *this->xml_document,
                                 this->entry_name_storage, bookmark_name);
}

std::vector<featherdoc::content_control_summary>
TemplatePart::list_content_controls() const {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return {};
    }

    return summarize_content_controls_in_part(*this->last_error_info,
                                              *this->xml_document);
}

std::vector<featherdoc::hyperlink_summary> TemplatePart::list_hyperlinks() const {
    if (this->xml_document == nullptr || this->last_error_info == nullptr ||
        this->owner == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return {};
    }

    return this->owner->list_hyperlinks_in_part(*this->xml_document,
                                                this->entry_name_storage);
}

std::size_t TemplatePart::append_hyperlink(std::string_view text,
                                           std::string_view target) {
    if (this->xml_document == nullptr || this->last_error_info == nullptr ||
        this->owner == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return 0U;
    }

    return this->owner->append_hyperlink_in_part(
        *this->xml_document, this->entry_name_storage, text, target);
}

bool TemplatePart::replace_hyperlink(std::size_t hyperlink_index,
                                      std::string_view text,
                                      std::string_view target) {
    if (this->xml_document == nullptr || this->last_error_info == nullptr ||
        this->owner == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return false;
    }

    return this->owner->replace_hyperlink_in_part(
        *this->xml_document, this->entry_name_storage, hyperlink_index, text, target);
}

bool TemplatePart::remove_hyperlink(std::size_t hyperlink_index) {
    if (this->xml_document == nullptr || this->last_error_info == nullptr ||
        this->owner == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return false;
    }

    return this->owner->remove_hyperlink_in_part(
        *this->xml_document, this->entry_name_storage, hyperlink_index);
}

std::vector<featherdoc::content_control_summary>
TemplatePart::find_content_controls_by_tag(std::string_view tag) const {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return {};
    }

    return filter_content_controls_by_tag_or_alias(
        *this->last_error_info, *this->xml_document, this->entry_name_storage,
        tag, true);
}

std::vector<featherdoc::content_control_summary>
TemplatePart::find_content_controls_by_alias(std::string_view alias) const {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return {};
    }

    return filter_content_controls_by_tag_or_alias(
        *this->last_error_info, *this->xml_document, this->entry_name_storage,
        alias, false);
}

#include "document_template_part_content_control_replacement_methods.inc"

std::optional<std::size_t> TemplatePart::find_table_index_by_bookmark(
    std::string_view bookmark_name) const {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return std::nullopt;
    }

    return find_table_index_by_bookmark_in_part(*this->last_error_info,
                                                *this->xml_document,
                                                this->entry_name_storage,
                                                bookmark_name);
}

std::optional<std::size_t> TemplatePart::find_table_index(
    const featherdoc::template_table_selector &selector) const {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return std::nullopt;
    }

    return find_table_index_by_selector_in_part(*this->last_error_info,
                                                *this->xml_document,
                                                this->entry_name_storage,
                                                selector);
}

std::optional<featherdoc::Table> TemplatePart::find_table_by_bookmark(
    std::string_view bookmark_name) {
    const auto resolved_index = this->find_table_index_by_bookmark(bookmark_name);
    if (!resolved_index.has_value()) {
        return std::nullopt;
    }

    auto table_handle = this->tables();
    for (std::size_t current_index = 0U;
         current_index < *resolved_index && table_handle.has_next(); ++current_index) {
        table_handle.next();
    }

    if (!table_handle.has_next()) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::state_not_recoverable),
                           "bookmark-resolved table index '" +
                               std::to_string(*resolved_index) +
                               "' is no longer present in template part",
                           this->entry_name_storage);
        }
        return std::nullopt;
    }

    this->last_error_info->clear();
    return table_handle;
}

std::optional<featherdoc::Table> TemplatePart::find_table(
    const featherdoc::template_table_selector &selector) {
    const auto resolved_index = this->find_table_index(selector);
    if (!resolved_index.has_value()) {
        return std::nullopt;
    }

    auto table_handle = this->tables();
    for (std::size_t current_index = 0U;
         current_index < *resolved_index && table_handle.has_next(); ++current_index) {
        table_handle.next();
    }

    if (!table_handle.has_next()) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::state_not_recoverable),
                           "selector-resolved table index '" +
                               std::to_string(*resolved_index) +
                               "' is no longer present in template part",
                           this->entry_name_storage);
        }
        return std::nullopt;
    }

    this->last_error_info->clear();
    return table_handle;
}

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
