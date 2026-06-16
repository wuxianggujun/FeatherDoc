#include "featherdoc.hpp"
#include "image_helpers.hpp"
#include "document_template_content_control_replacement.hpp"
#include "xml_helpers.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <limits>
#include <optional>
#include <ostream>
#include <sstream>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
#include <unordered_set>
#include <utility>

#include <zip.h>

namespace featherdoc {
auto find_table_index_by_bookmark_in_part(
    featherdoc::document_error_info &last_error_info,
    pugi::xml_document &document, std::string_view entry_name,
    std::string_view bookmark_name) -> std::optional<std::size_t>;
}

namespace {
constexpr auto document_xml_entry = std::string_view{"word/document.xml"};
constexpr auto document_relationships_xml_entry =
    std::string_view{"word/_rels/document.xml.rels"};
constexpr auto footnotes_xml_entry = std::string_view{"word/footnotes.xml"};
constexpr auto endnotes_xml_entry = std::string_view{"word/endnotes.xml"};
constexpr auto comments_xml_entry = std::string_view{"word/comments.xml"};
constexpr auto comments_extended_xml_entry =
    std::string_view{"word/commentsExtended.xml"};
constexpr auto unavailable_template_part_detail =
    std::string_view{"template part is not available"};
constexpr auto page_number_field_instruction = std::string_view{" PAGE "};
constexpr auto total_pages_field_instruction = std::string_view{" NUMPAGES "};
constexpr auto footnotes_relationship_type = std::string_view{
    "http://schemas.openxmlformats.org/officeDocument/2006/relationships/footnotes"};
constexpr auto endnotes_relationship_type = std::string_view{
    "http://schemas.openxmlformats.org/officeDocument/2006/relationships/endnotes"};
constexpr auto comments_relationship_type = std::string_view{
    "http://schemas.openxmlformats.org/officeDocument/2006/relationships/comments"};
constexpr auto comments_extended_relationship_type = std::string_view{
    "http://schemas.microsoft.com/office/2011/relationships/commentsExtended"};
constexpr auto footnotes_content_type = std::string_view{
    "application/vnd.openxmlformats-officedocument.wordprocessingml.footnotes+xml"};
constexpr auto endnotes_content_type = std::string_view{
    "application/vnd.openxmlformats-officedocument.wordprocessingml.endnotes+xml"};
constexpr auto comments_content_type = std::string_view{
    "application/vnd.openxmlformats-officedocument.wordprocessingml.comments+xml"};
constexpr auto comments_extended_content_type = std::string_view{
    "application/vnd.openxmlformats-officedocument.wordprocessingml.commentsExtended+xml"};
constexpr auto footnotes_relationship_target = std::string_view{"footnotes.xml"};
constexpr auto endnotes_relationship_target = std::string_view{"endnotes.xml"};
constexpr auto comments_relationship_target = std::string_view{"comments.xml"};
constexpr auto comments_extended_relationship_target =
    std::string_view{"commentsExtended.xml"};
constexpr auto wordprocessingml_namespace_uri = std::string_view{
    "http://schemas.openxmlformats.org/wordprocessingml/2006/main"};
constexpr auto wordprocessingml_2010_namespace_uri =
    std::string_view{"http://schemas.microsoft.com/office/word/2010/wordml"};
constexpr auto wordprocessingml_2012_namespace_uri =
    std::string_view{"http://schemas.microsoft.com/office/word/2012/wordml"};
constexpr auto markup_compatibility_namespace_uri =
    std::string_view{"http://schemas.openxmlformats.org/markup-compatibility/2006"};
constexpr auto hyperlink_relationship_type = std::string_view{
    "http://schemas.openxmlformats.org/officeDocument/2006/relationships/hyperlink"};
constexpr auto office_document_relationships_namespace_uri = std::string_view{
    "http://schemas.openxmlformats.org/officeDocument/2006/relationships"};
constexpr auto empty_relationships_xml = std::string_view{
    R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
</Relationships>
)"};
constexpr auto math_namespace_uri = std::string_view{
    "http://schemas.openxmlformats.org/officeDocument/2006/math"};

#include "document_template_core_support_helpers.inc"

#include "document_template_hyperlink_helpers.inc"

#include "document_template_part_container_helpers.inc"

#include "document_template_field_helpers.inc"
#include "document_template_omml_helpers.inc"

#include "document_template_part_inspection_helpers.inc"


#include "document_template_error_helpers.inc"

#include "document_template_package_part_helpers.inc"

#include "document_template_revision_helpers.inc"

#include "document_template_relationship_namespace_helpers.inc"

#include "document_template_review_note_helpers.inc"

bool hyperlink_relationship_id_is_used(pugi::xml_node node,
                                       std::string_view relationship_id) {
    for (auto child = node.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        if (std::string_view{child.name()} == "w:hyperlink" &&
            std::string_view{child.attribute("r:id").value()} == relationship_id) {
            return true;
        }
        if (hyperlink_relationship_id_is_used(child, relationship_id)) {
            return true;
        }
    }
    return false;
}

pugi::xml_node find_relationship_by_id(pugi::xml_node relationships,
                                       std::string_view relationship_id) {
    for (auto relationship = relationships.child("Relationship");
         relationship != pugi::xml_node{};
         relationship = relationship.next_sibling("Relationship")) {
        if (std::string_view{relationship.attribute("Id").value()} == relationship_id) {
            return relationship;
        }
    }
    return {};
}

bool rewrite_hyperlink_plain_text(pugi::xml_node hyperlink, std::string_view text) {
    clear_children(hyperlink);
    auto run = hyperlink.append_child("w:r");
    auto run_properties = run.append_child("w:rPr");
    auto run_style = run_properties.append_child("w:rStyle");
    auto color = run_properties.append_child("w:color");
    auto underline = run_properties.append_child("w:u");
    if (run == pugi::xml_node{} || run_properties == pugi::xml_node{} ||
        run_style == pugi::xml_node{} || color == pugi::xml_node{} ||
        underline == pugi::xml_node{}) {
        return false;
    }
    ensure_attribute_value(run_style, "w:val", "Hyperlink");
    ensure_attribute_value(color, "w:val", "0563C1");
    ensure_attribute_value(underline, "w:val", "single");
    return featherdoc::detail::set_plain_text_run_content(run, text);
}

#include "document_template_bookmark_helpers.inc"

#include "document_template_content_control_helpers.inc"

auto collect_block_bookmark_placeholders(featherdoc::document_error_info &last_error_info,
                                         pugi::xml_document &document,
                                         std::string_view entry_name,
                                         std::string_view bookmark_name,
                                         std::vector<block_bookmark_placeholder> &placeholders)
    -> bool {
    std::vector<pugi::xml_node> bookmark_starts;
    collect_named_bookmark_starts(document, bookmark_name, bookmark_starts);

    for (const auto bookmark_start : bookmark_starts) {
        const auto paragraph = bookmark_start.parent();
        if (paragraph == pugi::xml_node{} || std::string_view{paragraph.name()} != "w:p") {
            set_last_error(last_error_info, std::make_error_code(std::errc::invalid_argument),
                           "block bookmark replacement requires the bookmark to live directly "
                           "inside a paragraph",
                           std::string{entry_name});
            return false;
        }

        const auto bookmark_end = find_matching_bookmark_end(bookmark_start);
        if (bookmark_end == pugi::xml_node{} || bookmark_end.parent() != paragraph) {
            set_last_error(last_error_info, std::make_error_code(std::errc::invalid_argument),
                           "block bookmark replacement requires matching bookmark markers "
                           "inside the same paragraph",
                           std::string{entry_name});
            return false;
        }

        if (!paragraph_is_block_placeholder(paragraph, bookmark_start, bookmark_end)) {
            set_last_error(last_error_info, std::make_error_code(std::errc::invalid_argument),
                           "block bookmark replacement requires the target bookmark to occupy "
                           "its own paragraph",
                           std::string{entry_name});
            return false;
        }

        placeholders.push_back({paragraph, bookmark_start, bookmark_end});
    }

    return true;
}

auto collect_bookmark_block_placeholders(
    featherdoc::document_error_info &last_error_info, pugi::xml_document &document,
    std::string_view entry_name, std::string_view bookmark_name,
    std::vector<bookmark_block_placeholder> &placeholders) -> bool {
    std::vector<pugi::xml_node> bookmark_starts;
    collect_named_bookmark_starts(document, bookmark_name, bookmark_starts);

    for (const auto bookmark_start : bookmark_starts) {
        const auto start_paragraph = bookmark_start.parent();
        if (start_paragraph == pugi::xml_node{} ||
            std::string_view{start_paragraph.name()} != "w:p") {
            set_last_error(last_error_info, std::make_error_code(std::errc::invalid_argument),
                           "bookmark block visibility requires the start marker to live "
                           "directly inside a paragraph",
                           std::string{entry_name});
            return false;
        }

        const auto bookmark_end = find_matching_bookmark_end_in_document_order(bookmark_start);
        if (bookmark_end == pugi::xml_node{}) {
            set_last_error(last_error_info, std::make_error_code(std::errc::invalid_argument),
                           "bookmark block visibility requires a matching bookmark end marker",
                           std::string{entry_name});
            return false;
        }

        const auto end_paragraph = bookmark_end.parent();
        if (end_paragraph == pugi::xml_node{} ||
            std::string_view{end_paragraph.name()} != "w:p") {
            set_last_error(last_error_info, std::make_error_code(std::errc::invalid_argument),
                           "bookmark block visibility requires the end marker to live "
                           "directly inside a paragraph",
                           std::string{entry_name});
            return false;
        }

        if (start_paragraph == end_paragraph) {
            set_last_error(last_error_info, std::make_error_code(std::errc::invalid_argument),
                           "bookmark block visibility requires start and end markers to live "
                           "in separate paragraphs",
                           std::string{entry_name});
            return false;
        }

        const auto container = start_paragraph.parent();
        if (container == pugi::xml_node{} || end_paragraph.parent() != container) {
            set_last_error(last_error_info, std::make_error_code(std::errc::invalid_argument),
                           "bookmark block visibility requires both marker paragraphs to "
                           "share the same parent container",
                           std::string{entry_name});
            return false;
        }

        if (!paragraph_is_bookmark_marker(start_paragraph, bookmark_start) ||
            !paragraph_is_bookmark_marker(end_paragraph, bookmark_end)) {
            set_last_error(last_error_info, std::make_error_code(std::errc::invalid_argument),
                           "bookmark block visibility requires both markers to occupy "
                           "their own paragraphs",
                           std::string{entry_name});
            return false;
        }

        placeholders.push_back({container, start_paragraph, end_paragraph});
    }

    return true;
}

auto count_named_children(pugi::xml_node parent, std::string_view child_name)
    -> std::size_t {
    std::size_t count = 0U;
    for (auto child = parent.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        if (std::string_view{child.name()} == child_name) {
            ++count;
        }
    }
    return count;
}

auto rewrite_paragraph_plain_text(pugi::xml_node paragraph, std::string_view text)
    -> bool {
    if (paragraph == pugi::xml_node{}) {
        return false;
    }

    pugi::xml_document run_properties_storage;
    pugi::xml_node run_properties;
    for (auto child = paragraph.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        if (std::string_view{child.name()} == "w:r") {
            const auto source_run_properties = child.child("w:rPr");
            if (source_run_properties != pugi::xml_node{}) {
                run_properties =
                    run_properties_storage.append_copy(source_run_properties);
                break;
            }
        }
    }

    for (auto child = paragraph.first_child(); child != pugi::xml_node{};) {
        const auto next = child.next_sibling();
        if (std::string_view{child.name()} != "w:pPr") {
            paragraph.remove_child(child);
        }
        child = next;
    }

    if (text.empty()) {
        return true;
    }

    auto run = paragraph.append_child("w:r");
    if (run == pugi::xml_node{}) {
        return false;
    }

    if (run_properties != pugi::xml_node{} &&
        run.append_copy(run_properties) == pugi::xml_node{}) {
        return false;
    }

    return featherdoc::detail::set_plain_text_run_content(run, text);
}

auto rewrite_table_cell_plain_text(pugi::xml_node cell, std::string_view text)
    -> bool {
    if (cell == pugi::xml_node{}) {
        return false;
    }

    pugi::xml_node paragraph;
    for (auto child = cell.first_child(); child != pugi::xml_node{};) {
        const auto next = child.next_sibling();
        const auto child_name = std::string_view{child.name()};
        if (child_name == "w:tcPr") {
            child = next;
            continue;
        }

        if (child_name == "w:p" && paragraph == pugi::xml_node{}) {
            paragraph = child;
            child = next;
            continue;
        }

        cell.remove_child(child);
        child = next;
    }

    if (paragraph == pugi::xml_node{}) {
        paragraph = featherdoc::detail::append_paragraph_node(cell);
        if (paragraph == pugi::xml_node{}) {
            return false;
        }
    }

    return rewrite_paragraph_plain_text(paragraph, text);
}

void set_content_control_replacement_error(
    featherdoc::document_error_info &last_error_info,
    std::string_view entry_name,
    std::string_view detail) {
    set_last_error(last_error_info,
                   std::make_error_code(std::errc::invalid_argument),
                   std::string{detail}, std::string{entry_name});
}

template <typename RewriteContentControl>
bool replace_content_control_by_tag_or_alias_in_node(
    featherdoc::document_error_info &last_error_info, pugi::xml_node node,
    std::string_view entry_name, std::string_view value, bool match_tag,
    std::size_t &replaced, RewriteContentControl rewrite_content_control) {
    for (auto child = node.first_child(); child != pugi::xml_node{};) {
        const auto next = child.next_sibling();
        if (std::string_view{child.name()} == "w:sdt" &&
            content_control_matches_tag_or_alias(child, value, match_tag)) {
            if (!rewrite_content_control(last_error_info, entry_name, child)) {
                return false;
            }
            ++replaced;
            child = next;
            continue;
        }

        if (!replace_content_control_by_tag_or_alias_in_node(
                last_error_info, child, entry_name, value, match_tag, replaced,
                rewrite_content_control)) {
            return false;
        }
        child = next;
    }

    return true;
}

#include "document_template_bookmark_replacement_helpers.inc"

#include "document_template_schema_helpers.inc"

} // namespace

namespace featherdoc {

featherdoc::complex_field_instruction_fragment
complex_field_text_fragment(std::string_view text) {
    featherdoc::complex_field_instruction_fragment fragment;
    fragment.kind = featherdoc::complex_field_instruction_fragment_kind::text;
    fragment.text = std::string{text};
    return fragment;
}

featherdoc::complex_field_instruction_fragment
complex_field_nested_fragment(std::string_view instruction,
                              std::string_view result_text,
                              featherdoc::field_state_options state) {
    featherdoc::complex_field_instruction_fragment fragment;
    fragment.kind = featherdoc::complex_field_instruction_fragment_kind::nested_field;
    fragment.instruction = std::string{instruction};
    fragment.result_text = std::string{result_text};
    fragment.state = state;
    return fragment;
}

#include "document_template_omml_factories.inc"

#include "document_template_schema_patch_methods.inc"

#include "document_template_part_methods.inc"

#include "document_template_part_access_methods.inc"

#include "document_template_bookmark_methods.inc"

#include "document_template_content_control_methods.inc"

#include "document_template_hyperlink_methods.inc"

#include "document_template_review_note_methods.inc"

#include "document_template_review_revision_methods.inc"

#include "document_template_revision_management_methods.inc"

#include "document_template_review_part_methods.inc"

#include "document_template_omml_methods.inc"

#include "document_template_schema_methods.inc"

#include "document_template_bookmark_replacement_methods.inc"

} // namespace featherdoc
