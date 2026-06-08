#include "featherdoc.hpp"
#include "image_helpers.hpp"
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

struct string_xml_writer final : pugi::xml_writer {
    std::string text;

    void write(const void *data, size_t size) override {
        this->text.append(static_cast<const char *>(data), size);
    }
};

auto set_last_error(featherdoc::document_error_info &error_info,
                    std::error_code code, std::string detail = {},
                    std::string entry_name = {},
                    std::optional<std::ptrdiff_t> xml_offset = std::nullopt)
    -> std::error_code;
auto set_last_error(featherdoc::document_error_info &error_info,
                    featherdoc::document_errc code, std::string detail = {},
                    std::string entry_name = {},
                    std::optional<std::ptrdiff_t> xml_offset = std::nullopt)
    -> std::error_code;

struct hyperlink_relationship_resolution {
    std::optional<std::string> target;
    bool external{false};
};

auto initialize_xml_document(pugi::xml_document &xml_document, std::string_view xml_text)
    -> bool {
    xml_document.reset();
    return static_cast<bool>(
        xml_document.load_buffer(xml_text.data(), xml_text.size()));
}

auto initialize_empty_relationships_document(pugi::xml_document &xml_document)
    -> bool {
    return initialize_xml_document(xml_document, empty_relationships_xml);
}

void ensure_attribute_value(pugi::xml_node node, const char *name, std::string_view value) {
    if (node == pugi::xml_node{}) {
        return;
    }

    auto attribute = node.attribute(name);
    if (attribute == pugi::xml_attribute{}) {
        attribute = node.append_attribute(name);
    }
    attribute.set_value(std::string{value}.c_str());
}

auto next_relationship_id(pugi::xml_node relationships) -> std::string {
    std::unordered_set<std::string> used_relationship_ids;
    for (auto relationship = relationships.child("Relationship");
         relationship != pugi::xml_node{};
         relationship = relationship.next_sibling("Relationship")) {
        const auto id = relationship.attribute("Id").value();
        if (*id != '\0') {
            used_relationship_ids.emplace(id);
        }
    }

    for (std::size_t next_index = 1U;; ++next_index) {
        auto candidate = "rId" + std::to_string(next_index);
        if (!used_relationship_ids.contains(candidate)) {
            return candidate;
        }
    }
}

auto resolve_hyperlink_relationship(
    const pugi::xml_document *relationships_document, std::string_view relationship_id)
    -> hyperlink_relationship_resolution {
    auto resolution = hyperlink_relationship_resolution{};
    if (relationships_document == nullptr || relationship_id.empty()) {
        return resolution;
    }

    const auto relationships = relationships_document->child("Relationships");
    for (auto relationship = relationships.child("Relationship");
         relationship != pugi::xml_node{};
         relationship = relationship.next_sibling("Relationship")) {
        if (std::string_view{relationship.attribute("Id").value()} !=
            relationship_id) {
            continue;
        }
        if (std::string_view{relationship.attribute("Type").value()} !=
            hyperlink_relationship_type) {
            return resolution;
        }

        const auto target = std::string_view{relationship.attribute("Target").value()};
        if (!target.empty()) {
            resolution.target = std::string{target};
        }
        resolution.external =
            std::string_view{relationship.attribute("TargetMode").value()} ==
            "External";
        return resolution;
    }

    return resolution;
}

void collect_hyperlinks_in_order(pugi::xml_node node,
                                 std::vector<pugi::xml_node> &hyperlinks) {
    for (auto child = node.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        if (std::string_view{child.name()} == "w:hyperlink") {
            hyperlinks.push_back(child);
        }
        collect_hyperlinks_in_order(child, hyperlinks);
    }
}

auto summarize_hyperlinks_in_part(
    featherdoc::document_error_info &last_error_info, pugi::xml_document &document,
    const pugi::xml_document *relationships_document)
    -> std::vector<featherdoc::hyperlink_summary> {
    std::vector<pugi::xml_node> hyperlink_nodes;
    collect_hyperlinks_in_order(document, hyperlink_nodes);

    auto summaries = std::vector<featherdoc::hyperlink_summary>{};
    summaries.reserve(hyperlink_nodes.size());
    for (std::size_t index = 0; index < hyperlink_nodes.size(); ++index) {
        auto summary = featherdoc::hyperlink_summary{};
        summary.index = index;
        const auto hyperlink = hyperlink_nodes[index];
        summary.text = featherdoc::detail::collect_plain_text_from_xml(hyperlink);

        const auto relationship_id = std::string_view{hyperlink.attribute("r:id").value()};
        if (!relationship_id.empty()) {
            summary.relationship_id = std::string{relationship_id};
            const auto resolution = resolve_hyperlink_relationship(
                relationships_document, relationship_id);
            summary.target = resolution.target;
            summary.external = resolution.external;
        }

        const auto anchor = std::string_view{hyperlink.attribute("w:anchor").value()};
        if (!anchor.empty()) {
            summary.anchor = std::string{anchor};
        }

        summaries.push_back(std::move(summary));
    }

    last_error_info.clear();
    return summaries;
}

auto template_part_block_container(pugi::xml_document &xml_document) -> pugi::xml_node {
    if (const auto body = xml_document.child("w:document").child("w:body");
        body != pugi::xml_node{}) {
        return body;
    }

    if (const auto header = xml_document.child("w:hdr"); header != pugi::xml_node{}) {
        return header;
    }

    if (const auto footer = xml_document.child("w:ftr"); footer != pugi::xml_node{}) {
        return footer;
    }

    return {};
}

#include "document_template_field_helpers.inc"
#include "document_template_omml_helpers.inc"

auto read_optional_string_attribute(pugi::xml_node node, const char *attribute_name)
    -> std::optional<std::string> {
    if (node == pugi::xml_node{}) {
        return std::nullopt;
    }

    const auto attribute = node.attribute(attribute_name);
    if (attribute == pugi::xml_attribute{} || attribute.value()[0] == '\0') {
        return std::nullopt;
    }

    return std::string{attribute.value()};
}

auto read_on_off_value(pugi::xml_node node) -> std::optional<bool> {
    if (node == pugi::xml_node{}) {
        return std::nullopt;
    }

    const auto attribute = node.attribute("w:val");
    if (attribute == pugi::xml_attribute{} || attribute.value()[0] == '\0') {
        return true;
    }

    const auto value = std::string_view{attribute.value()};
    return value != "0" && value != "false" && value != "off";
}

auto read_part_paragraph_style_id(pugi::xml_node paragraph_properties)
    -> std::optional<std::string> {
    return read_optional_string_attribute(paragraph_properties.child("w:pStyle"), "w:val");
}

auto read_part_paragraph_bidi(pugi::xml_node paragraph_properties) -> std::optional<bool> {
    return read_on_off_value(paragraph_properties.child("w:bidi"));
}

auto parse_u32_attribute_value(const char *text) -> std::optional<std::uint32_t>;

auto read_part_paragraph_alignment(pugi::xml_node paragraph_properties)
    -> std::optional<featherdoc::paragraph_alignment> {
    const auto value = std::string_view{
        paragraph_properties.child("w:jc").attribute("w:val").value()};
    if (value == "left" || value == "start") {
        return featherdoc::paragraph_alignment::left;
    }
    if (value == "center") {
        return featherdoc::paragraph_alignment::center;
    }
    if (value == "right" || value == "end") {
        return featherdoc::paragraph_alignment::right;
    }
    if (value == "both" || value == "mediumKashida" ||
        value == "highKashida" || value == "lowKashida") {
        return featherdoc::paragraph_alignment::justified;
    }
    if (value == "distribute") {
        return featherdoc::paragraph_alignment::distribute;
    }

    return std::nullopt;
}

auto read_part_paragraph_indent_twips(pugi::xml_node paragraph_properties,
                                      const char *attribute_name)
    -> std::optional<std::uint32_t> {
    return parse_u32_attribute_value(
        paragraph_properties.child("w:ind").attribute(attribute_name).value());
}

auto parse_u32_attribute_value(const char *text) -> std::optional<std::uint32_t> {
    if (text == nullptr || *text == '\0') {
        return std::nullopt;
    }

    char *end = nullptr;
    const auto value = std::strtoul(text, &end, 10);
    if (end == text || *end != '\0' ||
        value > static_cast<unsigned long>(std::numeric_limits<std::uint32_t>::max())) {
        return std::nullopt;
    }

    return static_cast<std::uint32_t>(value);
}

auto summarize_part_paragraph_numbering(pugi::xml_node paragraph_properties)
    -> std::optional<featherdoc::paragraph_inspection_summary::numbering_summary> {
    const auto numbering_properties = paragraph_properties.child("w:numPr");
    if (numbering_properties == pugi::xml_node{}) {
        return std::nullopt;
    }

    auto summary = featherdoc::paragraph_inspection_summary::numbering_summary{};
    summary.level = parse_u32_attribute_value(
        numbering_properties.child("w:ilvl").attribute("w:val").value());
    summary.num_id = parse_u32_attribute_value(
        numbering_properties.child("w:numId").attribute("w:val").value());
    return summary;
}

auto enrich_part_paragraph_numbering(featherdoc::Document *owner,
                                     featherdoc::paragraph_inspection_summary &summary) -> void {
    if (owner == nullptr || !summary.numbering.has_value() ||
        !summary.numbering->num_id.has_value()) {
        return;
    }

    const auto lookup = owner->find_numbering_instance(*summary.numbering->num_id);
    if (!lookup.has_value()) {
        return;
    }

    summary.numbering->definition_id = lookup->definition_id;
    if (!lookup->definition_name.empty()) {
        summary.numbering->definition_name = lookup->definition_name;
    }
}

auto summarize_part_paragraph_node(pugi::xml_node paragraph_node, std::size_t paragraph_index)
    -> featherdoc::paragraph_inspection_summary {
    auto summary = featherdoc::paragraph_inspection_summary{};
    summary.index = paragraph_index;

    const auto paragraph_properties = paragraph_node.child("w:pPr");
    summary.style_id = read_part_paragraph_style_id(paragraph_properties);
    summary.bidi = read_part_paragraph_bidi(paragraph_properties);
    summary.alignment = read_part_paragraph_alignment(paragraph_properties);
    summary.indent_left_twips =
        read_part_paragraph_indent_twips(paragraph_properties, "w:left");
    summary.indent_right_twips =
        read_part_paragraph_indent_twips(paragraph_properties, "w:right");
    summary.first_line_indent_twips =
        read_part_paragraph_indent_twips(paragraph_properties, "w:firstLine");
    summary.hanging_indent_twips =
        read_part_paragraph_indent_twips(paragraph_properties, "w:hanging");
    summary.numbering = summarize_part_paragraph_numbering(paragraph_properties);

    auto paragraph_handle = featherdoc::Paragraph{paragraph_node.parent(), paragraph_node};
    for (auto run = paragraph_handle.runs(); run.has_next(); run.next()) {
        ++summary.run_count;
        summary.text += run.get_text();
    }

    return summary;
}

auto summarize_part_run_handle(const featherdoc::Run &run_handle, std::size_t run_index)
    -> featherdoc::run_inspection_summary {
    auto summary = featherdoc::run_inspection_summary{};
    summary.index = run_index;
    summary.text = run_handle.get_text();
    summary.style_id = run_handle.style_id();
    summary.font_family = run_handle.font_family();
    summary.east_asia_font_family = run_handle.east_asia_font_family();
    summary.text_color = run_handle.text_color();
    summary.bold = run_handle.bold();
    summary.italic = run_handle.italic();
    summary.strikethrough = run_handle.strikethrough();
    summary.underline = run_handle.underline();
    summary.superscript = run_handle.superscript();
    summary.subscript = run_handle.subscript();
    summary.font_size_points = run_handle.font_size_points();
    summary.language = run_handle.language();
    summary.east_asia_language = run_handle.east_asia_language();
    summary.bidi_language = run_handle.bidi_language();
    summary.rtl = run_handle.rtl();
    return summary;
}

auto summarize_part_table_handle(featherdoc::Table table_handle, std::size_t table_index)
    -> featherdoc::table_inspection_summary {
    auto summary = featherdoc::table_inspection_summary{};
    summary.index = table_index;
    summary.style_id = table_handle.style_id();
    summary.width_twips = table_handle.width_twips();
    summary.alignment = table_handle.alignment();
    summary.indent_twips = table_handle.indent_twips();
    summary.cell_spacing_twips = table_handle.cell_spacing_twips();
    summary.cell_margin_top_twips =
        table_handle.cell_margin_twips(featherdoc::cell_margin_edge::top);
    summary.cell_margin_left_twips =
        table_handle.cell_margin_twips(featherdoc::cell_margin_edge::left);
    summary.cell_margin_bottom_twips =
        table_handle.cell_margin_twips(featherdoc::cell_margin_edge::bottom);
    summary.cell_margin_right_twips =
        table_handle.cell_margin_twips(featherdoc::cell_margin_edge::right);
    auto borders = featherdoc::table_borders_inspection_summary{};
    borders.top = table_handle.border(featherdoc::table_border_edge::top);
    borders.left = table_handle.border(featherdoc::table_border_edge::left);
    borders.bottom = table_handle.border(featherdoc::table_border_edge::bottom);
    borders.right = table_handle.border(featherdoc::table_border_edge::right);
    borders.inside_horizontal =
        table_handle.border(featherdoc::table_border_edge::inside_horizontal);
    borders.inside_vertical =
        table_handle.border(featherdoc::table_border_edge::inside_vertical);
    if (borders.top || borders.left || borders.bottom || borders.right ||
        borders.inside_horizontal || borders.inside_vertical) {
        summary.borders = std::move(borders);
    }

    bool first_row = true;
    for (auto row = table_handle.rows(); row.has_next(); row.next()) {
        ++summary.row_count;
        summary.row_height_twips.push_back(row.height_twips());
        summary.row_height_rules.push_back(row.height_rule());
        summary.row_cant_split.push_back(row.cant_split());
        summary.row_repeats_header.push_back(row.repeats_header());

        std::size_t row_column_count = 0U;
        bool first_cell = true;
        for (auto cell = row.cells(); cell.has_next(); cell.next()) {
            row_column_count += cell.column_span();

            if (!first_row) {
                if (first_cell) {
                    summary.text.push_back('\n');
                } else {
                    summary.text.push_back('\t');
                }
            } else if (!first_cell) {
                summary.text.push_back('\t');
            }

            summary.text += cell.get_text();
            first_cell = false;
        }

        summary.column_count = std::max(summary.column_count, row_column_count);
        first_row = false;
    }

    summary.column_widths.reserve(summary.column_count);
    for (std::size_t column_index = 0U; column_index < summary.column_count; ++column_index) {
        summary.column_widths.push_back(table_handle.column_width_twips(column_index));
    }

    return summary;
}

auto summarize_part_table_cell_handle(featherdoc::TableCell cell_handle, std::size_t row_index,
                                      std::size_t cell_index, std::size_t column_index)
    -> featherdoc::table_cell_inspection_summary {
    auto summary = featherdoc::table_cell_inspection_summary{};
    summary.row_index = row_index;
    summary.cell_index = cell_index;
    summary.column_index = column_index;
    summary.column_span = cell_handle.column_span();
    summary.row_span = cell_handle.row_span();
    summary.vertical_merge = cell_handle.vertical_merge();
    summary.width_twips = cell_handle.width_twips();
    summary.vertical_alignment = cell_handle.vertical_alignment();
    summary.text_direction = cell_handle.text_direction();
    summary.fill_color = cell_handle.fill_color();
    summary.margin_top_twips =
        cell_handle.margin_twips(featherdoc::cell_margin_edge::top);
    summary.margin_left_twips =
        cell_handle.margin_twips(featherdoc::cell_margin_edge::left);
    summary.margin_bottom_twips =
        cell_handle.margin_twips(featherdoc::cell_margin_edge::bottom);
    summary.margin_right_twips =
        cell_handle.margin_twips(featherdoc::cell_margin_edge::right);
    summary.border_top = cell_handle.border(featherdoc::cell_border_edge::top);
    summary.border_left = cell_handle.border(featherdoc::cell_border_edge::left);
    summary.border_bottom =
        cell_handle.border(featherdoc::cell_border_edge::bottom);
    summary.border_right =
        cell_handle.border(featherdoc::cell_border_edge::right);
    summary.text = cell_handle.get_text();

    for (auto paragraph = cell_handle.paragraphs(); paragraph.has_next(); paragraph.next()) {
        ++summary.paragraph_count;
    }

    return summary;
}

auto collect_part_table_cell_summaries(featherdoc::Table table_handle)
    -> std::vector<featherdoc::table_cell_inspection_summary> {
    auto summaries = std::vector<featherdoc::table_cell_inspection_summary>{};
    auto row = table_handle.rows();
    for (std::size_t row_index = 0U; row.has_next(); ++row_index, row.next()) {
        std::size_t column_index = 0U;
        auto cell = row.cells();
        for (std::size_t cell_index = 0U; cell.has_next(); ++cell_index, cell.next()) {
            auto summary =
                summarize_part_table_cell_handle(cell, row_index, cell_index, column_index);
            column_index += summary.column_span;
            summaries.push_back(std::move(summary));
        }
    }

    return summaries;
}


auto set_last_error(featherdoc::document_error_info &error_info,
                    std::error_code code, std::string detail,
                    std::string entry_name,
                    std::optional<std::ptrdiff_t> xml_offset)
    -> std::error_code {
    error_info.code = code;
    error_info.detail = std::move(detail);
    error_info.entry_name = std::move(entry_name);
    error_info.xml_offset = std::move(xml_offset);
    return code;
}

auto set_last_error(featherdoc::document_error_info &error_info,
                    featherdoc::document_errc code, std::string detail,
                    std::string entry_name,
                    std::optional<std::ptrdiff_t> xml_offset)
    -> std::error_code {
    return set_last_error(error_info, featherdoc::make_error_code(code),
                          std::move(detail), std::move(entry_name), std::move(xml_offset));
}

auto normalize_related_word_entry_name(std::string_view target) -> std::string {
    std::string normalized{target};
    std::replace(normalized.begin(), normalized.end(), '\\', '/');
    if (!normalized.empty() && normalized.front() == '/') {
        normalized.erase(normalized.begin());
    } else if (normalized.rfind("word/", 0U) != 0U) {
        normalized.insert(0U, "word/");
    }
    return std::filesystem::path{normalized}.lexically_normal().generic_string();
}

auto related_part_entry_for_type(const pugi::xml_document &relationships_document,
                                 std::string_view relationship_type,
                                 std::string_view fallback_entry) -> std::string {
    const auto relationships = relationships_document.child("Relationships");
    for (auto relationship = relationships.child("Relationship");
         relationship != pugi::xml_node{};
         relationship = relationship.next_sibling("Relationship")) {
        if (std::string_view{relationship.attribute("Type").value()} !=
            relationship_type) {
            continue;
        }
        const auto target = std::string_view{relationship.attribute("Target").value()};
        if (!target.empty()) {
            return normalize_related_word_entry_name(target);
        }
    }
    return std::string{fallback_entry};
}

auto optional_related_part_entry_for_type(
    const pugi::xml_document &relationships_document,
    std::string_view relationship_type) -> std::optional<std::string> {
    const auto relationships = relationships_document.child("Relationships");
    for (auto relationship = relationships.child("Relationship");
         relationship != pugi::xml_node{};
         relationship = relationship.next_sibling("Relationship")) {
        if (std::string_view{relationship.attribute("Type").value()} !=
            relationship_type) {
            continue;
        }
        const auto target =
            std::string_view{relationship.attribute("Target").value()};
        if (!target.empty()) {
            return normalize_related_word_entry_name(target);
        }
    }
    return std::nullopt;
}

auto read_docx_entry_text(const std::filesystem::path &document_path,
                          std::string_view entry_name,
                          std::string &content,
                          featherdoc::document_error_info &last_error_info)
    -> bool {
    int zip_error = 0;
    zip_t *zip = zip_openwitherror(document_path.string().c_str(), 0, 'r', &zip_error);
    if (zip == nullptr) {
        set_last_error(last_error_info, std::make_error_code(std::errc::io_error),
                       "failed to open source DOCX archive",
                       document_path.string());
        return false;
    }

    if (zip_entry_open(zip, entry_name.data()) != 0) {
        zip_close(zip);
        content.clear();
        return true;
    }

    void *buffer = nullptr;
    size_t buffer_size = 0U;
    const auto read_result = zip_entry_read(zip, &buffer, &buffer_size);
    const auto close_result = zip_entry_close(zip);
    zip_close(zip);
    if (read_result < 0 || close_result != 0) {
        if (buffer != nullptr) {
            std::free(buffer);
        }
        set_last_error(last_error_info, std::make_error_code(std::errc::io_error),
                       "failed to read source DOCX XML part",
                       std::string{entry_name});
        return false;
    }

    content.assign(static_cast<const char *>(buffer), buffer_size);
    std::free(buffer);
    return true;
}

auto load_optional_docx_xml_part(
    const std::filesystem::path &document_path, std::string_view entry_name,
    pugi::xml_document &xml_document,
    featherdoc::document_error_info &last_error_info) -> bool {
    std::string xml_text;
    if (!read_docx_entry_text(document_path, entry_name, xml_text, last_error_info)) {
        return false;
    }
    if (xml_text.empty()) {
        xml_document.reset();
        last_error_info.clear();
        return true;
    }

    const auto parse_result = xml_document.load_buffer(xml_text.data(), xml_text.size());
    if (!parse_result) {
        set_last_error(last_error_info, std::make_error_code(std::errc::invalid_argument),
                       parse_result.description(), std::string{entry_name},
                       parse_result.offset >= 0
                           ? std::optional<std::ptrdiff_t>{parse_result.offset}
                           : std::nullopt);
        return false;
    }

    last_error_info.clear();
    return true;
}

auto xml_child_attribute_string(pugi::xml_node node, const char *attribute_name)
    -> std::optional<std::string> {
    const auto value = std::string_view{node.attribute(attribute_name).value()};
    if (value.empty()) {
        return std::nullopt;
    }
    return std::string{value};
}

#include "document_template_revision_helpers.inc"

bool add_content_type_override(pugi::xml_document &content_types,
                               bool &content_types_dirty,
                               featherdoc::document_error_info &last_error_info,
                               std::string_view part_name,
                               std::string_view content_type) {
    auto types = content_types.child("Types");
    if (types == pugi::xml_node{}) {
        set_last_error(last_error_info,
                       featherdoc::document_errc::content_types_xml_parse_failed,
                       "[Content_Types].xml does not contain a Types root",
                       "[Content_Types].xml");
        return false;
    }
    const auto normalized_part_name = std::string{"/"} + std::string{part_name};
    for (auto override_node = types.child("Override");
         override_node != pugi::xml_node{};
         override_node = override_node.next_sibling("Override")) {
        if (std::string_view{override_node.attribute("PartName").value()} ==
            normalized_part_name) {
            ensure_attribute_value(override_node, "ContentType", content_type);
            content_types_dirty = true;
            return true;
        }
    }
    auto override_node = types.append_child("Override");
    if (override_node == pugi::xml_node{}) {
        set_last_error(last_error_info, std::make_error_code(std::errc::not_enough_memory),
                       "failed to append content type override",
                       "[Content_Types].xml");
        return false;
    }
    ensure_attribute_value(override_node, "PartName", normalized_part_name);
    ensure_attribute_value(override_node, "ContentType", content_type);
    content_types_dirty = true;
    return true;
}

bool ensure_document_relationship(pugi::xml_document &relationships_document,
                                  bool &has_relationships_part,
                                  bool &relationships_dirty,
                                  featherdoc::document_error_info &last_error_info,
                                  std::string_view relationship_type,
                                  std::string_view target) {
    if (relationships_document.child("Relationships") == pugi::xml_node{} &&
        !initialize_empty_relationships_document(relationships_document)) {
        set_last_error(last_error_info,
                       featherdoc::document_errc::relationships_xml_parse_failed,
                       "failed to initialize default relationships XML",
                       std::string{document_relationships_xml_entry});
        return false;
    }
    auto relationships = relationships_document.child("Relationships");
    for (auto relationship = relationships.child("Relationship");
         relationship != pugi::xml_node{};
         relationship = relationship.next_sibling("Relationship")) {
        if (std::string_view{relationship.attribute("Type").value()} == relationship_type) {
            ensure_attribute_value(relationship, "Target", target);
            has_relationships_part = true;
            relationships_dirty = true;
            return true;
        }
    }
    const auto relationship_id = next_relationship_id(relationships);
    auto relationship = relationships.append_child("Relationship");
    if (relationship == pugi::xml_node{}) {
        set_last_error(last_error_info, std::make_error_code(std::errc::not_enough_memory),
                       "failed to append document relationship",
                       std::string{document_relationships_xml_entry});
        return false;
    }
    ensure_attribute_value(relationship, "Id", relationship_id);
    ensure_attribute_value(relationship, "Type", relationship_type);
    ensure_attribute_value(relationship, "Target", target);
    has_relationships_part = true;
    relationships_dirty = true;
    return true;
}

void clear_children(pugi::xml_node node) {
    for (auto child = node.first_child(); child != pugi::xml_node{};) {
        const auto next = child.next_sibling();
        node.remove_child(child);
        child = next;
    }
}

void ensure_wordprocessingml_namespace(pugi::xml_node root) {
    if (root == pugi::xml_node{}) {
        return;
    }

    auto word_namespace = root.attribute("xmlns:w");
    if (word_namespace == pugi::xml_attribute{}) {
        word_namespace = root.append_attribute("xmlns:w");
    }
    if (word_namespace != pugi::xml_attribute{}) {
        word_namespace.set_value(std::string{wordprocessingml_namespace_uri}.c_str());
    }
}

void ensure_wordprocessingml_2010_namespace(pugi::xml_node root) {
    if (root == pugi::xml_node{}) {
        return;
    }

    auto word_namespace = root.attribute("xmlns:w14");
    if (word_namespace == pugi::xml_attribute{}) {
        word_namespace = root.append_attribute("xmlns:w14");
    }
    if (word_namespace != pugi::xml_attribute{}) {
        word_namespace.set_value(
            std::string{wordprocessingml_2010_namespace_uri}.c_str());
    }
}

void ensure_markup_compatibility_ignorable(pugi::xml_node root,
                                           std::string_view prefix) {
    if (root == pugi::xml_node{} || prefix.empty()) {
        return;
    }

    auto markup_namespace = root.attribute("xmlns:mc");
    if (markup_namespace == pugi::xml_attribute{}) {
        markup_namespace = root.append_attribute("xmlns:mc");
    }
    if (markup_namespace != pugi::xml_attribute{}) {
        markup_namespace.set_value(
            std::string{markup_compatibility_namespace_uri}.c_str());
    }

    auto ignorable = root.attribute("mc:Ignorable");
    if (ignorable == pugi::xml_attribute{}) {
        ignorable = root.append_attribute("mc:Ignorable");
    }
    if (ignorable == pugi::xml_attribute{}) {
        return;
    }
    const auto value = std::string_view{ignorable.value()};
    if (value.empty()) {
        ignorable.set_value(std::string{prefix}.c_str());
        return;
    }

    std::istringstream tokens{std::string{value}};
    for (std::string token; tokens >> token;) {
        if (token == prefix) {
            return;
        }
    }

    auto updated = std::string{value};
    updated.push_back(' ');
    updated += prefix;
    ignorable.set_value(updated.c_str());
}

void ensure_wordprocessingml_2012_namespace(pugi::xml_node root) {
    if (root == pugi::xml_node{}) {
        return;
    }

    auto word_namespace = root.attribute("xmlns:w15");
    if (word_namespace == pugi::xml_attribute{}) {
        word_namespace = root.append_attribute("xmlns:w15");
    }
    if (word_namespace != pugi::xml_attribute{}) {
        word_namespace.set_value(
            std::string{wordprocessingml_2012_namespace_uri}.c_str());
    }
}

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

auto rewrite_paragraph_plain_text(pugi::xml_node paragraph, std::string_view text) -> bool {
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
                run_properties = run_properties_storage.append_copy(source_run_properties);
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

    if (run_properties != pugi::xml_node{} && run.append_copy(run_properties) == pugi::xml_node{}) {
        return false;
    }

    return featherdoc::detail::set_plain_text_run_content(run, text);
}

auto rewrite_table_cell_plain_text(pugi::xml_node cell, std::string_view text) -> bool {
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

auto append_plain_text_table(pugi::xml_node parent,
                             const std::vector<std::vector<std::string>> &rows)
    -> bool {
    auto table = parent.append_child("w:tbl");
    if (table == pugi::xml_node{}) {
        return false;
    }

    for (const auto &row_values : rows) {
        auto row = table.append_child("w:tr");
        if (row == pugi::xml_node{}) {
            return false;
        }

        for (const auto &cell_text : row_values) {
            if (!append_text_table_cell(row, cell_text)) {
                return false;
            }
        }
    }

    return true;
}

auto validate_plain_text_table_rows(
    featherdoc::document_error_info &last_error_info,
    std::string_view entry_name,
    const std::vector<std::vector<std::string>> &rows) -> bool {
    if (rows.empty()) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "replacement table must contain at least one row",
                       std::string{entry_name});
        return false;
    }

    for (const auto &row : rows) {
        if (row.empty()) {
            set_last_error(last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           "replacement table rows must contain at least one cell",
                           std::string{entry_name});
            return false;
        }
    }

    return true;
}

void set_content_control_replacement_error(
    featherdoc::document_error_info &last_error_info,
    std::string_view entry_name,
    std::string_view detail) {
    set_last_error(last_error_info,
                   std::make_error_code(std::errc::invalid_argument),
                   std::string{detail}, std::string{entry_name});
}

auto rewrite_content_control_with_paragraphs(
    featherdoc::document_error_info &last_error_info,
    std::string_view entry_name, pugi::xml_node content_control,
    const std::vector<std::string> &paragraphs) -> bool {
    auto content = ensure_content_control_content_node(content_control);
    if (content == pugi::xml_node{}) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to resolve content control content for paragraph replacement",
                       std::string{entry_name});
        return false;
    }

    const auto kind = content_control_kind_from_node(content_control);
    if (kind == featherdoc::content_control_kind::run) {
        set_content_control_replacement_error(
            last_error_info, entry_name,
            "paragraph replacement requires a block or table-cell content control");
        return false;
    }
    if (kind == featherdoc::content_control_kind::table_row) {
        set_content_control_replacement_error(
            last_error_info, entry_name,
            "paragraph replacement cannot target a table-row content control");
        return false;
    }

    clear_node_children(content);

    if (kind == featherdoc::content_control_kind::table_cell) {
        auto cell = content.append_child("w:tc");
        if (cell == pugi::xml_node{}) {
            set_last_error(last_error_info,
                           std::make_error_code(std::errc::not_enough_memory),
                           "failed to append a replacement content control cell",
                           std::string{entry_name});
            return false;
        }

        if (paragraphs.empty()) {
            if (!featherdoc::detail::append_plain_text_paragraph(cell, {})) {
                set_last_error(last_error_info,
                               std::make_error_code(std::errc::not_enough_memory),
                               "failed to append an empty replacement paragraph",
                               std::string{entry_name});
                return false;
            }
        }

        for (const auto &paragraph_text : paragraphs) {
            if (!featherdoc::detail::append_plain_text_paragraph(cell,
                                                                 paragraph_text)) {
                set_last_error(last_error_info,
                               std::make_error_code(std::errc::not_enough_memory),
                               "failed to append a replacement paragraph",
                               std::string{entry_name});
                return false;
            }
        }

        clear_content_control_placeholder_flag(content_control);
        return true;
    }

    for (const auto &paragraph_text : paragraphs) {
        if (!featherdoc::detail::append_plain_text_paragraph(content,
                                                             paragraph_text)) {
            set_last_error(last_error_info,
                           std::make_error_code(std::errc::not_enough_memory),
                           "failed to append a replacement paragraph",
                           std::string{entry_name});
            return false;
        }
    }

    clear_content_control_placeholder_flag(content_control);
    return true;
}

auto rewrite_content_control_with_table_rows(
    featherdoc::document_error_info &last_error_info,
    std::string_view entry_name, pugi::xml_node content_control,
    const std::vector<std::vector<std::string>> &rows) -> bool {
    if (content_control_kind_from_node(content_control) !=
        featherdoc::content_control_kind::table_row) {
        set_content_control_replacement_error(
            last_error_info, entry_name,
            "table row replacement requires a table-row content control");
        return false;
    }

    auto content = ensure_content_control_content_node(content_control);
    if (content == pugi::xml_node{}) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to resolve content control content for table row replacement",
                       std::string{entry_name});
        return false;
    }

    const auto template_row = content.child("w:tr");
    if (template_row == pugi::xml_node{}) {
        if (rows.empty()) {
            clear_node_children(content);
            clear_content_control_placeholder_flag(content_control);
            return true;
        }

        set_content_control_replacement_error(
            last_error_info, entry_name,
            "table row replacement requires the content control to contain a template row");
        return false;
    }

    const auto cell_count = count_named_children(template_row, "w:tc");
    if (cell_count == 0U && !rows.empty()) {
        set_content_control_replacement_error(
            last_error_info, entry_name,
            "table row replacement requires the template row to contain at least one cell");
        return false;
    }

    for (const auto &row_values : rows) {
        if (row_values.size() != cell_count) {
            set_content_control_replacement_error(
                last_error_info, entry_name,
                "replacement table row cell count must exactly match the template row cell count");
            return false;
        }
    }

    pugi::xml_document template_row_document;
    const auto template_row_copy = template_row_document.append_copy(template_row);
    if (template_row_copy == pugi::xml_node{}) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to clone the content control template row",
                       std::string{entry_name});
        return false;
    }

    clear_node_children(content);
    for (const auto &row_values : rows) {
        auto row_copy = content.append_copy(template_row_copy);
        if (row_copy == pugi::xml_node{}) {
            set_last_error(last_error_info,
                           std::make_error_code(std::errc::not_enough_memory),
                           "failed to append a replacement content control row",
                           std::string{entry_name});
            return false;
        }

        std::size_t cell_index = 0U;
        for (auto cell = row_copy.child("w:tc"); cell != pugi::xml_node{};
             cell = featherdoc::detail::next_named_sibling(cell, "w:tc"),
             ++cell_index) {
            if (!rewrite_table_cell_plain_text(cell, row_values[cell_index])) {
                set_last_error(last_error_info,
                               std::make_error_code(std::errc::not_enough_memory),
                               "failed to rewrite a replacement content control row cell",
                               std::string{entry_name});
                return false;
            }
        }
    }

    clear_content_control_placeholder_flag(content_control);
    return true;
}

auto rewrite_content_control_with_table(
    featherdoc::document_error_info &last_error_info,
    std::string_view entry_name, pugi::xml_node content_control,
    const std::vector<std::vector<std::string>> &rows) -> bool {
    auto content = ensure_content_control_content_node(content_control);
    if (content == pugi::xml_node{}) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to resolve content control content for table replacement",
                       std::string{entry_name});
        return false;
    }

    const auto kind = content_control_kind_from_node(content_control);
    if (kind == featherdoc::content_control_kind::run) {
        set_content_control_replacement_error(
            last_error_info, entry_name,
            "table replacement requires a block or table-cell content control");
        return false;
    }
    if (kind == featherdoc::content_control_kind::table_row) {
        set_content_control_replacement_error(
            last_error_info, entry_name,
            "table replacement cannot target a table-row content control");
        return false;
    }

    clear_node_children(content);

    if (kind == featherdoc::content_control_kind::table_cell) {
        auto cell = content.append_child("w:tc");
        if (cell == pugi::xml_node{} || !append_plain_text_table(cell, rows)) {
            set_last_error(last_error_info,
                           std::make_error_code(std::errc::not_enough_memory),
                           "failed to append a replacement table inside a content control cell",
                           std::string{entry_name});
            return false;
        }

        if (!featherdoc::detail::append_plain_text_paragraph(cell, {})) {
            set_last_error(last_error_info,
                           std::make_error_code(std::errc::not_enough_memory),
                           "failed to append a trailing replacement table paragraph",
                           std::string{entry_name});
            return false;
        }

        clear_content_control_placeholder_flag(content_control);
        return true;
    }

    if (!append_plain_text_table(content, rows)) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to append a replacement content control table",
                       std::string{entry_name});
        return false;
    }

    clear_content_control_placeholder_flag(content_control);
    return true;
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

auto replace_content_control_with_paragraphs_by_tag_or_alias_in_part(
    featherdoc::document_error_info &last_error_info,
    pugi::xml_document &document, std::string_view entry_name,
    std::string_view value, const std::vector<std::string> &paragraphs,
    bool match_tag) -> std::size_t {
    if (value.empty()) {
        set_last_error(
            last_error_info, std::make_error_code(std::errc::invalid_argument),
            match_tag ? "content control tag must not be empty"
                      : "content control alias must not be empty",
            std::string{entry_name});
        return 0U;
    }

    std::size_t replaced = 0U;
    const auto rewrite = [&paragraphs](
                             featherdoc::document_error_info &current_last_error,
                             std::string_view current_entry_name,
                             pugi::xml_node content_control) {
        return rewrite_content_control_with_paragraphs(
            current_last_error, current_entry_name, content_control, paragraphs);
    };
    if (!replace_content_control_by_tag_or_alias_in_node(
            last_error_info, document, entry_name, value, match_tag, replaced,
            rewrite)) {
        return replaced;
    }

    last_error_info.clear();
    return replaced;
}

auto replace_content_control_with_table_rows_by_tag_or_alias_in_part(
    featherdoc::document_error_info &last_error_info,
    pugi::xml_document &document, std::string_view entry_name,
    std::string_view value, const std::vector<std::vector<std::string>> &rows,
    bool match_tag) -> std::size_t {
    if (value.empty()) {
        set_last_error(
            last_error_info, std::make_error_code(std::errc::invalid_argument),
            match_tag ? "content control tag must not be empty"
                      : "content control alias must not be empty",
            std::string{entry_name});
        return 0U;
    }

    std::size_t replaced = 0U;
    const auto rewrite = [&rows](
                             featherdoc::document_error_info &current_last_error,
                             std::string_view current_entry_name,
                             pugi::xml_node content_control) {
        return rewrite_content_control_with_table_rows(
            current_last_error, current_entry_name, content_control, rows);
    };
    if (!replace_content_control_by_tag_or_alias_in_node(
            last_error_info, document, entry_name, value, match_tag, replaced,
            rewrite)) {
        return replaced;
    }

    last_error_info.clear();
    return replaced;
}

auto replace_content_control_with_table_by_tag_or_alias_in_part(
    featherdoc::document_error_info &last_error_info,
    pugi::xml_document &document, std::string_view entry_name,
    std::string_view value, const std::vector<std::vector<std::string>> &rows,
    bool match_tag) -> std::size_t {
    if (value.empty()) {
        set_last_error(
            last_error_info, std::make_error_code(std::errc::invalid_argument),
            match_tag ? "content control tag must not be empty"
                      : "content control alias must not be empty",
            std::string{entry_name});
        return 0U;
    }

    if (!validate_plain_text_table_rows(last_error_info, entry_name, rows)) {
        return 0U;
    }

    std::size_t replaced = 0U;
    const auto rewrite = [&rows](
                             featherdoc::document_error_info &current_last_error,
                             std::string_view current_entry_name,
                             pugi::xml_node content_control) {
        return rewrite_content_control_with_table(
            current_last_error, current_entry_name, content_control, rows);
    };
    if (!replace_content_control_by_tag_or_alias_in_node(
            last_error_info, document, entry_name, value, match_tag, replaced,
            rewrite)) {
        return replaced;
    }

    last_error_info.clear();
    return replaced;
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

template_schema_normalization_summary normalize_template_schema(
    featherdoc::template_schema &schema) {
    featherdoc::template_schema_normalization_summary summary{};
    summary.original_slots = schema.entries.size();

    const auto original_entries = schema.entries;
    std::vector<featherdoc::template_schema_entry> normalized_entries;
    normalized_entries.reserve(schema.entries.size());
    for (auto entry : schema.entries) {
        entry.target = canonical_template_schema_selector(entry.target);
        const auto existing_it = find_template_schema_slot(
            normalized_entries, entry.target, entry.requirement.source,
            entry.requirement.bookmark_name);
        if (existing_it == normalized_entries.end()) {
            normalized_entries.push_back(std::move(entry));
            continue;
        }

        *existing_it = std::move(entry);
        ++summary.duplicate_slots_removed;
    }

    std::sort(normalized_entries.begin(), normalized_entries.end(),
              template_schema_entry_less);
    summary.final_slots = normalized_entries.size();
    summary.reordered_or_normalized =
        !template_schema_entries_equal(original_entries, normalized_entries);
    schema.entries = std::move(normalized_entries);
    return summary;
}

template_schema_patch_summary merge_template_schema(
    featherdoc::template_schema &base, const featherdoc::template_schema &overlay) {
    featherdoc::template_schema_patch_summary summary{};

    (void)normalize_template_schema(base);
    auto normalized_overlay = overlay;
    (void)normalize_template_schema(normalized_overlay);

    for (const auto &overlay_entry : normalized_overlay.entries) {
        const auto existing_it = find_template_schema_slot(
            base.entries, overlay_entry.target, overlay_entry.requirement.source,
            overlay_entry.requirement.bookmark_name);
        if (existing_it == base.entries.end()) {
            base.entries.push_back(overlay_entry);
            ++summary.inserted_slots;
            continue;
        }

        if (!template_schema_entry_equals(*existing_it, overlay_entry)) {
            *existing_it = overlay_entry;
            ++summary.replaced_slots;
        }
    }

    (void)normalize_template_schema(base);
    return summary;
}

template_schema_patch_summary apply_template_schema_patch(
    featherdoc::template_schema &schema, const featherdoc::template_schema_patch &patch) {
    featherdoc::template_schema_patch_summary summary{};
    (void)normalize_template_schema(schema);

    for (const auto &raw_target : patch.remove_targets) {
        const auto target = canonical_template_schema_selector(raw_target);
        const auto previous_size = schema.entries.size();
        schema.entries.erase(
            std::remove_if(schema.entries.begin(), schema.entries.end(),
                           [&](const featherdoc::template_schema_entry &entry) {
                               return template_schema_selector_equals(entry.target, target);
                           }),
            schema.entries.end());
        if (schema.entries.size() != previous_size) {
            ++summary.removed_targets;
        }
    }

    for (const auto &remove_slot : patch.remove_slots) {
        const auto target = canonical_template_schema_selector(remove_slot.target);
        const auto previous_size = schema.entries.size();
        schema.entries.erase(
            std::remove_if(schema.entries.begin(), schema.entries.end(),
                           [&](const featherdoc::template_schema_entry &entry) {
                               return template_schema_selector_equals(entry.target, target) &&
                                      entry.requirement.source == remove_slot.source &&
                                       entry.requirement.bookmark_name ==
                                          remove_slot.bookmark_name;
                           }),
            schema.entries.end());
        summary.removed_slots += previous_size - schema.entries.size();
    }

    for (const auto &rename_slot : patch.rename_slots) {
        const auto target = canonical_template_schema_selector(rename_slot.slot.target);
        for (auto &entry : schema.entries) {
            if (!template_schema_selector_equals(entry.target, target) ||
                entry.requirement.source != rename_slot.slot.source ||
                entry.requirement.bookmark_name != rename_slot.slot.bookmark_name ||
                entry.requirement.bookmark_name == rename_slot.new_bookmark_name) {
                continue;
            }

            entry.requirement.bookmark_name = rename_slot.new_bookmark_name;
            ++summary.renamed_slots;
        }
    }

    for (const auto &update_slot : patch.update_slots) {
        const auto update_summary = update_template_schema_slot(
            schema, update_slot.slot, update_slot.update);
        summary.replaced_slots += update_summary.replaced_slots;
    }

    if (!patch.upsert_slots.empty()) {
        featherdoc::template_schema overlay{};
        overlay.entries = patch.upsert_slots;
        const auto merge_summary = merge_template_schema(schema, overlay);
        summary.inserted_slots += merge_summary.inserted_slots;
        summary.replaced_slots += merge_summary.replaced_slots;
    }

    (void)normalize_template_schema(schema);
    return summary;
}

template_schema_patch_summary preview_template_schema_patch(
    const featherdoc::template_schema &schema,
    const featherdoc::template_schema_patch &patch) {
    auto preview_schema = schema;
    return apply_template_schema_patch(preview_schema, patch);
}

template_schema_patch_summary preview_template_schema_patch(
    const featherdoc::template_schema &left,
    const featherdoc::template_schema &right) {
    const auto patch = build_template_schema_patch(left, right);
    return preview_template_schema_patch(left, patch);
}

featherdoc::template_schema_patch_review_summary
make_template_schema_patch_review_summary(
    const featherdoc::template_schema &baseline,
    const featherdoc::template_schema_patch &patch) {
    featherdoc::template_schema_patch_review_summary summary{};
    summary.baseline_slot_count = template_schema_slot_count(baseline);
    summary.patch_upsert_slot_count = patch.upsert_slots.size();
    summary.patch_remove_target_count = patch.remove_targets.size();
    summary.patch_remove_slot_count = patch.remove_slots.size();
    summary.patch_rename_slot_count = patch.rename_slots.size();
    summary.patch_update_slot_count = patch.update_slots.size();
    summary.preview = preview_template_schema_patch(baseline, patch);
    return summary;
}

featherdoc::template_schema_patch_review_summary
make_template_schema_patch_review_summary(
    const featherdoc::template_schema &baseline,
    const featherdoc::template_schema &generated) {
    const auto patch = build_template_schema_patch(baseline, generated);
    auto summary = make_template_schema_patch_review_summary(baseline, patch);
    summary.generated_slot_count = template_schema_slot_count(generated);
    return summary;
}

void write_template_schema_patch_review_json(
    std::ostream &stream,
    const featherdoc::template_schema_patch_review_summary &summary) {
    stream << '{';
    stream << "\"schema\":";
    write_schema_review_json_string(
        stream, "featherdoc.template_schema_patch_review.v1");
    stream << ",\"baseline_slot_count\":" << summary.baseline_slot_count;
    stream << ",\"generated_slot_count\":";
    if (summary.generated_slot_count.has_value()) {
        stream << *summary.generated_slot_count;
    } else {
        stream << "null";
    }
    stream << ",\"patch\":{"
           << "\"upsert_slot_count\":" << summary.patch_upsert_slot_count
           << ",\"remove_target_count\":" << summary.patch_remove_target_count
           << ",\"remove_slot_count\":" << summary.patch_remove_slot_count
           << ",\"rename_slot_count\":" << summary.patch_rename_slot_count
           << ",\"update_slot_count\":" << summary.patch_update_slot_count
           << "}";
    stream << ",\"preview\":{"
           << "\"removed_targets\":" << summary.preview.removed_targets
           << ",\"removed_slots\":" << summary.preview.removed_slots
           << ",\"renamed_slots\":" << summary.preview.renamed_slots
           << ",\"inserted_slots\":" << summary.preview.inserted_slots
           << ",\"replaced_slots\":" << summary.preview.replaced_slots
           << ",\"changed\":" << (summary.preview.changed() ? "true" : "false")
           << "}";
    stream << ",\"changed\":" << (summary.changed() ? "true" : "false");
    stream << '}';
}

std::string template_schema_patch_review_json(
    const featherdoc::template_schema_patch_review_summary &summary) {
    std::ostringstream stream;
    write_template_schema_patch_review_json(stream, summary);
    return stream.str();
}

template_schema_patch_summary replace_template_schema_target(
    featherdoc::template_schema &schema,
    const featherdoc::template_schema_part_selector &target,
    std::span<const featherdoc::template_schema_entry> entries) {
    featherdoc::template_schema_patch patch{};
    patch.remove_targets.push_back(target);
    patch.upsert_slots.insert(patch.upsert_slots.end(), entries.begin(), entries.end());
    return apply_template_schema_patch(schema, patch);
}

template_schema_patch_summary replace_template_schema_target(
    featherdoc::template_schema &schema,
    const featherdoc::template_schema_part_selector &target,
    std::initializer_list<featherdoc::template_schema_entry> entries) {
    return replace_template_schema_target(
        schema, target, std::span<const featherdoc::template_schema_entry>{entries.begin(), entries.size()});
}

template_schema_patch_summary upsert_template_schema_slot(
    featherdoc::template_schema &schema, const featherdoc::template_schema_entry &entry) {
    featherdoc::template_schema_patch patch{};
    patch.upsert_slots.push_back(entry);
    return apply_template_schema_patch(schema, patch);
}

template_schema_patch_summary remove_template_schema_target(
    featherdoc::template_schema &schema,
    const featherdoc::template_schema_part_selector &target) {
    featherdoc::template_schema_patch patch{};
    patch.remove_targets.push_back(target);
    return apply_template_schema_patch(schema, patch);
}

template_schema_patch_summary rename_template_schema_target(
    featherdoc::template_schema &schema,
    const featherdoc::template_schema_part_selector &source_target,
    const featherdoc::template_schema_part_selector &target) {
    (void)normalize_template_schema(schema);

    const auto source = canonical_template_schema_selector(source_target);
    const auto destination = canonical_template_schema_selector(target);
    if (template_schema_selector_equals(source, destination)) {
        return {};
    }

    featherdoc::template_schema_patch patch{};
    for (const auto &entry : schema.entries) {
        if (!template_schema_selector_equals(entry.target, source)) {
            continue;
        }

        auto moved_entry = entry;
        moved_entry.target = destination;
        patch.upsert_slots.push_back(std::move(moved_entry));
    }

    if (patch.upsert_slots.empty()) {
        return {};
    }

    patch.remove_targets.push_back(source);
    return apply_template_schema_patch(schema, patch);
}

template_schema_patch_summary remove_template_schema_slot(
    featherdoc::template_schema &schema,
    const featherdoc::template_schema_slot_selector &slot) {
    featherdoc::template_schema_patch patch{};
    patch.remove_slots.push_back(slot);
    return apply_template_schema_patch(schema, patch);
}

template_schema_patch_summary rename_template_schema_slot(
    featherdoc::template_schema &schema,
    const featherdoc::template_schema_slot_selector &slot,
    std::string_view new_bookmark_name) {
    featherdoc::template_schema_patch patch{};
    featherdoc::template_schema_slot_rename rename_slot{};
    rename_slot.slot = slot;
    rename_slot.new_bookmark_name = std::string{new_bookmark_name};
    patch.rename_slots.push_back(std::move(rename_slot));
    return apply_template_schema_patch(schema, patch);
}

template_schema_patch_summary update_template_schema_slot(
    featherdoc::template_schema &schema,
    const featherdoc::template_schema_slot_selector &slot,
    const featherdoc::template_schema_slot_update &update) {
    featherdoc::template_schema_patch_summary summary{};
    (void)normalize_template_schema(schema);

    const auto target = canonical_template_schema_selector(slot.target);
    const auto existing_it = find_template_schema_slot(
        schema.entries, target, slot.source, slot.bookmark_name);
    if (existing_it == schema.entries.end()) {
        return summary;
    }

    auto updated_entry = *existing_it;
    if (update.kind.has_value()) {
        updated_entry.requirement.kind = *update.kind;
    }
    if (update.required.has_value()) {
        updated_entry.requirement.required = *update.required;
    }
    if (update.clear_min_occurrences) {
        updated_entry.requirement.min_occurrences.reset();
    }
    if (update.min_occurrences.has_value()) {
        updated_entry.requirement.min_occurrences = *update.min_occurrences;
    }
    if (update.clear_max_occurrences) {
        updated_entry.requirement.max_occurrences.reset();
    }
    if (update.max_occurrences.has_value()) {
        updated_entry.requirement.max_occurrences = *update.max_occurrences;
    }

    if (template_schema_entry_equals(*existing_it, updated_entry)) {
        return summary;
    }

    *existing_it = std::move(updated_entry);
    ++summary.replaced_slots;
    (void)normalize_template_schema(schema);
    return summary;
}

template_schema_patch build_template_schema_patch(
    const featherdoc::template_schema &left, const featherdoc::template_schema &right) {
    auto normalized_left = left;
    auto normalized_right = right;
    (void)normalize_template_schema(normalized_left);
    (void)normalize_template_schema(normalized_right);

    featherdoc::template_schema_patch patch{};

    std::vector<featherdoc::template_schema_entry> removed_entries;
    std::vector<featherdoc::template_schema_entry> added_entries;

    for (const auto &left_entry : normalized_left.entries) {
        if (!template_schema_has_target(normalized_right, left_entry.target)) {
            if (std::none_of(patch.remove_targets.begin(), patch.remove_targets.end(),
                             [&](const featherdoc::template_schema_part_selector &target) {
                                 return template_schema_selector_equals(target,
                                                                        left_entry.target);
                             })) {
                patch.remove_targets.push_back(left_entry.target);
            }
            continue;
        }

        const auto right_it = find_template_schema_slot(
            normalized_right.entries, left_entry.target, left_entry.requirement.source,
            left_entry.requirement.bookmark_name);
        if (right_it == normalized_right.entries.end()) {
            removed_entries.push_back(left_entry);
            continue;
        }
        if (!template_schema_entry_equals(left_entry, *right_it)) {
            featherdoc::template_schema_slot_patch_update update_slot{};
            update_slot.slot.target = left_entry.target;
            update_slot.slot.source = left_entry.requirement.source;
            update_slot.slot.bookmark_name = left_entry.requirement.bookmark_name;
            update_slot.update = make_template_schema_slot_update(
                left_entry.requirement, right_it->requirement);
            patch.update_slots.push_back(std::move(update_slot));
        }
    }

    for (const auto &right_entry : normalized_right.entries) {
        if (!template_schema_has_target(normalized_left, right_entry.target)) {
            patch.upsert_slots.push_back(right_entry);
            continue;
        }

        const auto left_it = find_template_schema_slot(
            normalized_left.entries, right_entry.target, right_entry.requirement.source,
            right_entry.requirement.bookmark_name);
        if (left_it == normalized_left.entries.end()) {
            added_entries.push_back(right_entry);
        }
    }

    std::vector<bool> removed_consumed(removed_entries.size(), false);
    std::vector<bool> added_consumed(added_entries.size(), false);
    const auto find_unique_renamed_slot = [&](std::size_t removed_index,
                                              int match_level)
        -> std::optional<std::size_t> {
        const auto matches = [&](const featherdoc::template_schema_entry &removed,
                                 const featherdoc::template_schema_entry &added) {
            if (!template_schema_selector_equals(removed.target, added.target) ||
                removed.requirement.source != added.requirement.source) {
                return false;
            }
            if (match_level == 0) {
                return compare_template_slot_requirement_shape(removed.requirement,
                                                               added.requirement) == 0;
            }
            if (match_level == 1) {
                return removed.requirement.kind == added.requirement.kind;
            }
            return true;
        };

        std::optional<std::size_t> matched_added_index;
        std::size_t matched_added_count = 0U;
        for (std::size_t added_index = 0U; added_index < added_entries.size();
             ++added_index) {
            if (added_consumed[added_index] ||
                !matches(removed_entries[removed_index], added_entries[added_index])) {
                continue;
            }
            matched_added_index = added_index;
            ++matched_added_count;
        }
        if (matched_added_count != 1U || !matched_added_index.has_value()) {
            return std::nullopt;
        }

        std::size_t matched_removed_count = 0U;
        for (std::size_t candidate_removed_index = 0U;
             candidate_removed_index < removed_entries.size();
             ++candidate_removed_index) {
            if (removed_consumed[candidate_removed_index] ||
                !matches(removed_entries[candidate_removed_index],
                         added_entries[*matched_added_index])) {
                continue;
            }
            ++matched_removed_count;
        }
        if (matched_removed_count != 1U) {
            return std::nullopt;
        }
        return matched_added_index;
    };

    for (std::size_t removed_index = 0U; removed_index < removed_entries.size();
         ++removed_index) {
        auto matched_added_index = find_unique_renamed_slot(removed_index, 0);
        if (!matched_added_index.has_value()) {
            matched_added_index = find_unique_renamed_slot(removed_index, 1);
        }
        if (!matched_added_index.has_value()) {
            matched_added_index = find_unique_renamed_slot(removed_index, 2);
        }
        if (!matched_added_index.has_value()) {
            continue;
        }

        featherdoc::template_schema_slot_rename rename_slot{};
        rename_slot.slot.target = removed_entries[removed_index].target;
        rename_slot.slot.source = removed_entries[removed_index].requirement.source;
        rename_slot.slot.bookmark_name =
            removed_entries[removed_index].requirement.bookmark_name;
        rename_slot.new_bookmark_name =
            added_entries[*matched_added_index].requirement.bookmark_name;
        patch.rename_slots.push_back(std::move(rename_slot));

        if (compare_template_slot_requirement_shape(
                removed_entries[removed_index].requirement,
                added_entries[*matched_added_index].requirement) != 0) {
            featherdoc::template_schema_slot_patch_update update_slot{};
            update_slot.slot.target = removed_entries[removed_index].target;
            update_slot.slot.source = removed_entries[removed_index].requirement.source;
            update_slot.slot.bookmark_name =
                added_entries[*matched_added_index].requirement.bookmark_name;
            update_slot.update = make_template_schema_slot_update(
                removed_entries[removed_index].requirement,
                added_entries[*matched_added_index].requirement);
            patch.update_slots.push_back(std::move(update_slot));
        }

        removed_consumed[removed_index] = true;
        added_consumed[*matched_added_index] = true;
    }

    for (std::size_t removed_index = 0U; removed_index < removed_entries.size();
         ++removed_index) {
        if (removed_consumed[removed_index]) {
            continue;
        }

        featherdoc::template_schema_slot_selector remove_slot{};
        remove_slot.target = removed_entries[removed_index].target;
        remove_slot.source = removed_entries[removed_index].requirement.source;
        remove_slot.bookmark_name =
            removed_entries[removed_index].requirement.bookmark_name;
        patch.remove_slots.push_back(std::move(remove_slot));
    }

    for (std::size_t added_index = 0U; added_index < added_entries.size();
         ++added_index) {
        if (!added_consumed[added_index]) {
            patch.upsert_slots.push_back(added_entries[added_index]);
        }
    }

    return patch;
}

#include "document_template_part_methods.inc"

TemplatePart Document::body_template() {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before accessing the body template");
        return {this, nullptr, &this->last_error_info, {}};
    }

    this->last_error_info.clear();
    return {this, &this->document, &this->last_error_info,
            std::string{document_xml_entry}};
}

TemplatePart Document::header_template(std::size_t index) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before accessing a header template");
        return {this, nullptr, &this->last_error_info, {}};
    }

    if (index >= this->header_parts.size()) {
        this->last_error_info.clear();
        return {this, nullptr, &this->last_error_info, {}};
    }

    this->last_error_info.clear();
    return {this, &this->header_parts[index]->xml, &this->last_error_info,
            this->header_parts[index]->entry_name};
}

TemplatePart Document::footer_template(std::size_t index) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before accessing a footer template");
        return {this, nullptr, &this->last_error_info, {}};
    }

    if (index >= this->footer_parts.size()) {
        this->last_error_info.clear();
        return {this, nullptr, &this->last_error_info, {}};
    }

    this->last_error_info.clear();
    return {this, &this->footer_parts[index]->xml, &this->last_error_info,
            this->footer_parts[index]->entry_name};
}

TemplatePart Document::section_header_template(
    std::size_t section_index, featherdoc::section_reference_kind reference_kind) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before accessing a section header "
                       "template");
        return {this, nullptr, &this->last_error_info, {}};
    }

    auto *part = this->section_related_part_state(section_index, reference_kind,
                                                  this->header_parts,
                                                  "w:headerReference");
    if (part == nullptr) {
        this->last_error_info.clear();
        return {this, nullptr, &this->last_error_info, {}};
    }

    this->last_error_info.clear();
    return {this, &part->xml, &this->last_error_info, part->entry_name};
}

TemplatePart Document::section_footer_template(
    std::size_t section_index, featherdoc::section_reference_kind reference_kind) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before accessing a section footer "
                       "template");
        return {this, nullptr, &this->last_error_info, {}};
    }

    auto *part = this->section_related_part_state(section_index, reference_kind,
                                                  this->footer_parts,
                                                  "w:footerReference");
    if (part == nullptr) {
        this->last_error_info.clear();
        return {this, nullptr, &this->last_error_info, {}};
    }

    this->last_error_info.clear();
    return {this, &part->xml, &this->last_error_info, part->entry_name};
}

std::size_t Document::replace_bookmark_text(const std::string &bookmark_name,
                                            const std::string &replacement) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before filling bookmark text");
        return 0U;
    }

    return replace_bookmark_text_in_part(this->last_error_info, this->document,
                                         document_xml_entry, bookmark_name,
                                         replacement);
}

std::size_t Document::replace_bookmark_text(const char *bookmark_name,
                                            const char *replacement) {
    if (bookmark_name == nullptr || replacement == nullptr) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "bookmark name and replacement text must not be null",
                       std::string{document_xml_entry});
        return 0U;
    }

    return this->replace_bookmark_text(std::string{bookmark_name},
                                       std::string{replacement});
}

bookmark_fill_result Document::fill_bookmarks(
    std::span<const bookmark_text_binding> bindings) {
    bookmark_fill_result result;
    result.requested = bindings.size();

    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before filling bookmarks");
        return result;
    }

    return fill_bookmarks_in_part(this->last_error_info, this->document,
                                  document_xml_entry, bindings);
}

bookmark_fill_result Document::fill_bookmarks(
    std::initializer_list<bookmark_text_binding> bindings) {
    return this->fill_bookmarks(
        std::span<const bookmark_text_binding>{bindings.begin(), bindings.size()});
}

std::vector<featherdoc::bookmark_summary> Document::list_bookmarks() const {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before listing bookmarks",
                       std::string{document_xml_entry});
        return {};
    }

    return summarize_bookmarks_in_part(this->last_error_info,
                                       const_cast<pugi::xml_document &>(this->document));
}

std::optional<featherdoc::bookmark_summary> Document::find_bookmark(
    std::string_view bookmark_name) const {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before reading bookmark metadata",
                       std::string{document_xml_entry});
        return std::nullopt;
    }

    return find_bookmark_in_part(this->last_error_info,
                                 const_cast<pugi::xml_document &>(this->document),
                                 document_xml_entry, bookmark_name);
}

#include "document_template_content_control_methods.inc"

#include "document_template_hyperlink_methods.inc"

#include "document_template_review_revision_methods.inc"

std::vector<featherdoc::omml_summary> Document::list_omml() const {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before listing OMML equations",
                       std::string{document_xml_entry});
        return {};
    }

    return summarize_omml_in_part(
        this->last_error_info, const_cast<pugi::xml_document &>(this->document));
}

bool Document::append_omml(std::string_view omml_xml) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before appending OMML",
                       std::string{document_xml_entry});
        return false;
    }

    return append_omml_to_part(this->last_error_info, this->document,
                               document_xml_entry, omml_xml);
}

bool Document::replace_omml(std::size_t omml_index, std::string_view omml_xml) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before replacing OMML",
                       std::string{document_xml_entry});
        return false;
    }

    return replace_omml_in_part(this->last_error_info, this->document,
                                document_xml_entry, omml_index, omml_xml);
}

bool Document::remove_omml(std::size_t omml_index) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before removing OMML",
                       std::string{document_xml_entry});
        return false;
    }

    return remove_omml_in_part(this->last_error_info, this->document,
                               document_xml_entry, omml_index);
}

#include "document_template_schema_methods.inc"

#include "document_template_bookmark_replacement_methods.inc"

} // namespace featherdoc
