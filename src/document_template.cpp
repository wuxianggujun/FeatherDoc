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

std::vector<featherdoc::content_control_summary>
Document::list_content_controls() const {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before listing content controls",
                       std::string{document_xml_entry});
        return {};
    }

    return summarize_content_controls_in_part(
        this->last_error_info, const_cast<pugi::xml_document &>(this->document));
}


std::optional<featherdoc::custom_xml_data_binding_sync_result>
Document::sync_content_controls_from_custom_xml() {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before synchronizing content controls from Custom XML",
                       std::string{document_xml_entry});
        return std::nullopt;
    }

    auto custom_xml_parts = load_custom_xml_package_parts(
        this->document_path, this->last_error_info);
    if (!custom_xml_parts.has_value()) {
        return std::nullopt;
    }

    featherdoc::custom_xml_data_binding_sync_result result;
    if (!sync_content_controls_from_custom_xml_in_part(
            this->last_error_info, this->document, document_xml_entry,
            *custom_xml_parts, result)) {
        return std::nullopt;
    }

    for (const auto &part : this->header_parts) {
        if (!sync_content_controls_from_custom_xml_in_part(
                this->last_error_info, part->xml, part->entry_name,
                *custom_xml_parts, result)) {
            return std::nullopt;
        }
    }

    for (const auto &part : this->footer_parts) {
        if (!sync_content_controls_from_custom_xml_in_part(
                this->last_error_info, part->xml, part->entry_name,
                *custom_xml_parts, result)) {
            return std::nullopt;
        }
    }

    this->last_error_info.clear();
    return result;
}

std::vector<featherdoc::hyperlink_summary> Document::list_hyperlinks() const {
    return this->list_hyperlinks_in_part(
        const_cast<pugi::xml_document &>(this->document), document_xml_entry);
}

std::size_t Document::append_hyperlink(std::string_view text,
                                       std::string_view target) {
    return this->append_hyperlink_in_part(this->document, document_xml_entry, text,
                                          target);
}

bool Document::replace_hyperlink(std::size_t hyperlink_index,
                                 std::string_view text,
                                 std::string_view target) {
    return this->replace_hyperlink_in_part(this->document, document_xml_entry,
                                           hyperlink_index, text, target);
}

bool Document::remove_hyperlink(std::size_t hyperlink_index) {
    return this->remove_hyperlink_in_part(this->document, document_xml_entry,
                                          hyperlink_index);
}

std::vector<featherdoc::content_control_summary>
Document::find_content_controls_by_tag(std::string_view tag) const {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before reading content controls",
                       std::string{document_xml_entry});
        return {};
    }

    return filter_content_controls_by_tag_or_alias(
        this->last_error_info, const_cast<pugi::xml_document &>(this->document),
        document_xml_entry, tag, true);
}

std::vector<featherdoc::content_control_summary>
Document::find_content_controls_by_alias(std::string_view alias) const {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before reading content controls",
                       std::string{document_xml_entry});
        return {};
    }

    return filter_content_controls_by_tag_or_alias(
        this->last_error_info, const_cast<pugi::xml_document &>(this->document),
        document_xml_entry, alias, false);
}

std::vector<featherdoc::review_note_summary> Document::list_footnotes() const {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before listing footnotes",
                       std::string{document_xml_entry});
        return {};
    }
    if (this->footnotes_loaded) {
        auto summaries = summarize_notes_from_part(
            const_cast<pugi::xml_document &>(this->footnotes),
            featherdoc::review_note_kind::footnote);
        this->last_error_info.clear();
        return summaries;
    }
    if (this->document_path.empty() ||
        !std::filesystem::exists(this->document_path)) {
        this->last_error_info.clear();
        return {};
    }

    const auto entry_name = related_part_entry_for_type(
        this->document_relationships, footnotes_relationship_type, "word/footnotes.xml");
    pugi::xml_document footnotes_document;
    if (!load_optional_docx_xml_part(this->document_path, entry_name,
                                     footnotes_document, this->last_error_info)) {
        return {};
    }

    auto summaries = summarize_notes_from_part(
        footnotes_document, featherdoc::review_note_kind::footnote);
    this->last_error_info.clear();
    return summaries;
}

std::size_t Document::append_footnote(std::string_view reference_text,
                                      std::string_view note_text) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before appending a footnote",
                       std::string{document_xml_entry});
        return 0U;
    }
    if (note_text.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "footnote text must not be empty",
                       std::string{footnotes_xml_entry});
        return 0U;
    }
    if (!this->ensure_review_notes_part(featherdoc::review_note_kind::footnote)) {
        return 0U;
    }

    auto root = this->footnotes.child("w:footnotes");
    auto body = template_part_block_container(this->document);
    if (root == pugi::xml_node{} || body == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "document is not ready for footnote insertion",
                       std::string{document_xml_entry});
        return 0U;
    }

    const auto note_id = next_numeric_id(root, "w:footnote", 1L);
    const auto note_id_text = std::to_string(note_id);
    auto note = root.append_child("w:footnote");
    if (note == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to append footnote XML",
                       std::string{footnotes_xml_entry});
        return 0U;
    }
    ensure_attribute_value(note, "w:id", note_id_text);
    if (!set_review_note_text(note, "w:footnoteRef", note_text)) {
        root.remove_child(note);
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to append footnote text",
                       std::string{footnotes_xml_entry});
        return 0U;
    }

    auto paragraph = featherdoc::detail::append_paragraph_node(body);
    if (paragraph == pugi::xml_node{} ||
        (!reference_text.empty() &&
         !featherdoc::detail::append_plain_text_run(paragraph, reference_text))) {
        root.remove_child(note);
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to append footnote reference paragraph",
                       std::string{document_xml_entry});
        return 0U;
    }
    auto reference_run = paragraph.append_child("w:r");
    auto reference = reference_run.append_child("w:footnoteReference");
    if (reference_run == pugi::xml_node{} || reference == pugi::xml_node{}) {
        root.remove_child(note);
        body.remove_child(paragraph);
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to append footnote reference",
                       std::string{document_xml_entry});
        return 0U;
    }
    ensure_attribute_value(reference, "w:id", note_id_text);

    this->footnotes_dirty = true;
    this->last_error_info.clear();
    return 1U;
}

bool Document::replace_footnote(std::size_t note_index,
                                std::string_view note_text) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before replacing a footnote",
                       std::string{document_xml_entry});
        return false;
    }
    if (note_text.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "footnote text must not be empty",
                       std::string{footnotes_xml_entry});
        return false;
    }
    if (!this->ensure_review_notes_part(featherdoc::review_note_kind::footnote)) {
        return false;
    }
    auto root = this->footnotes.child("w:footnotes");
    auto note = note_by_summary_index(root, featherdoc::review_note_kind::footnote,
                                      note_index);
    if (note == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "footnote index is out of range",
                       std::string{footnotes_xml_entry});
        return false;
    }
    if (!set_review_note_text(note, "w:footnoteRef", note_text)) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to replace footnote text",
                       std::string{footnotes_xml_entry});
        return false;
    }
    this->footnotes_dirty = true;
    this->last_error_info.clear();
    return true;
}

bool Document::remove_footnote(std::size_t note_index) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before removing a footnote",
                       std::string{document_xml_entry});
        return false;
    }
    if (!this->ensure_review_notes_part(featherdoc::review_note_kind::footnote)) {
        return false;
    }
    auto root = this->footnotes.child("w:footnotes");
    auto note = note_by_summary_index(root, featherdoc::review_note_kind::footnote,
                                      note_index);
    if (note == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "footnote index is out of range",
                       std::string{footnotes_xml_entry});
        return false;
    }
    const auto note_id = std::string{note.attribute("w:id").value()};
    root.remove_child(note);
    remove_references_by_id(this->document, "w:footnoteReference", note_id);
    for (auto &part : this->header_parts) {
        if (part != nullptr) {
            remove_references_by_id(part->xml, "w:footnoteReference", note_id);
        }
    }
    for (auto &part : this->footer_parts) {
        if (part != nullptr) {
            remove_references_by_id(part->xml, "w:footnoteReference", note_id);
        }
    }
    this->footnotes_dirty = true;
    this->last_error_info.clear();
    return true;
}

std::vector<featherdoc::review_note_summary> Document::list_endnotes() const {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before listing endnotes",
                       std::string{document_xml_entry});
        return {};
    }
    if (this->endnotes_loaded) {
        auto summaries = summarize_notes_from_part(
            const_cast<pugi::xml_document &>(this->endnotes),
            featherdoc::review_note_kind::endnote);
        this->last_error_info.clear();
        return summaries;
    }
    if (this->document_path.empty() ||
        !std::filesystem::exists(this->document_path)) {
        this->last_error_info.clear();
        return {};
    }

    const auto entry_name = related_part_entry_for_type(
        this->document_relationships, endnotes_relationship_type, "word/endnotes.xml");
    pugi::xml_document endnotes_document;
    if (!load_optional_docx_xml_part(this->document_path, entry_name,
                                     endnotes_document, this->last_error_info)) {
        return {};
    }

    auto summaries = summarize_notes_from_part(
        endnotes_document, featherdoc::review_note_kind::endnote);
    this->last_error_info.clear();
    return summaries;
}

std::size_t Document::append_endnote(std::string_view reference_text,
                                     std::string_view note_text) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before appending an endnote",
                       std::string{document_xml_entry});
        return 0U;
    }
    if (note_text.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "endnote text must not be empty",
                       std::string{endnotes_xml_entry});
        return 0U;
    }
    if (!this->ensure_review_notes_part(featherdoc::review_note_kind::endnote)) {
        return 0U;
    }

    auto root = this->endnotes.child("w:endnotes");
    auto body = template_part_block_container(this->document);
    if (root == pugi::xml_node{} || body == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "document is not ready for endnote insertion",
                       std::string{document_xml_entry});
        return 0U;
    }

    const auto note_id = next_numeric_id(root, "w:endnote", 1L);
    const auto note_id_text = std::to_string(note_id);
    auto note = root.append_child("w:endnote");
    if (note == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to append endnote XML",
                       std::string{endnotes_xml_entry});
        return 0U;
    }
    ensure_attribute_value(note, "w:id", note_id_text);
    if (!set_review_note_text(note, "w:endnoteRef", note_text)) {
        root.remove_child(note);
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to append endnote text",
                       std::string{endnotes_xml_entry});
        return 0U;
    }

    auto paragraph = featherdoc::detail::append_paragraph_node(body);
    if (paragraph == pugi::xml_node{} ||
        (!reference_text.empty() &&
         !featherdoc::detail::append_plain_text_run(paragraph, reference_text))) {
        root.remove_child(note);
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to append endnote reference paragraph",
                       std::string{document_xml_entry});
        return 0U;
    }
    auto reference_run = paragraph.append_child("w:r");
    auto reference = reference_run.append_child("w:endnoteReference");
    if (reference_run == pugi::xml_node{} || reference == pugi::xml_node{}) {
        root.remove_child(note);
        body.remove_child(paragraph);
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to append endnote reference",
                       std::string{document_xml_entry});
        return 0U;
    }
    ensure_attribute_value(reference, "w:id", note_id_text);

    this->endnotes_dirty = true;
    this->last_error_info.clear();
    return 1U;
}

bool Document::replace_endnote(std::size_t note_index,
                               std::string_view note_text) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before replacing an endnote",
                       std::string{document_xml_entry});
        return false;
    }
    if (note_text.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "endnote text must not be empty",
                       std::string{endnotes_xml_entry});
        return false;
    }
    if (!this->ensure_review_notes_part(featherdoc::review_note_kind::endnote)) {
        return false;
    }
    auto root = this->endnotes.child("w:endnotes");
    auto note = note_by_summary_index(root, featherdoc::review_note_kind::endnote,
                                      note_index);
    if (note == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "endnote index is out of range",
                       std::string{endnotes_xml_entry});
        return false;
    }
    if (!set_review_note_text(note, "w:endnoteRef", note_text)) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to replace endnote text",
                       std::string{endnotes_xml_entry});
        return false;
    }
    this->endnotes_dirty = true;
    this->last_error_info.clear();
    return true;
}

bool Document::remove_endnote(std::size_t note_index) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before removing an endnote",
                       std::string{document_xml_entry});
        return false;
    }
    if (!this->ensure_review_notes_part(featherdoc::review_note_kind::endnote)) {
        return false;
    }
    auto root = this->endnotes.child("w:endnotes");
    auto note = note_by_summary_index(root, featherdoc::review_note_kind::endnote,
                                      note_index);
    if (note == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "endnote index is out of range",
                       std::string{endnotes_xml_entry});
        return false;
    }
    const auto note_id = std::string{note.attribute("w:id").value()};
    root.remove_child(note);
    remove_references_by_id(this->document, "w:endnoteReference", note_id);
    for (auto &part : this->header_parts) {
        if (part != nullptr) {
            remove_references_by_id(part->xml, "w:endnoteReference", note_id);
        }
    }
    for (auto &part : this->footer_parts) {
        if (part != nullptr) {
            remove_references_by_id(part->xml, "w:endnoteReference", note_id);
        }
    }
    this->endnotes_dirty = true;
    this->last_error_info.clear();
    return true;
}

std::vector<featherdoc::review_note_summary> Document::list_comments() const {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before listing comments",
                       std::string{document_xml_entry});
        return {};
    }
    std::unordered_map<std::string, std::string> anchor_text_by_id;
    merge_comment_anchor_text_by_id(
        anchor_text_by_id, const_cast<pugi::xml_document &>(this->document));
    for (const auto &part : this->header_parts) {
        if (part != nullptr) {
            merge_comment_anchor_text_by_id(anchor_text_by_id, part->xml);
        }
    }
    for (const auto &part : this->footer_parts) {
        if (part != nullptr) {
            merge_comment_anchor_text_by_id(anchor_text_by_id, part->xml);
        }
    }

    if (this->comments_loaded) {
        auto &comments_document = const_cast<pugi::xml_document &>(this->comments);
        auto summaries = summarize_comments_from_part(comments_document);
        attach_comment_anchor_text(summaries, anchor_text_by_id);
        if (this->comments_extended_loaded) {
            attach_comment_extended_metadata(
                summaries, comments_document,
                const_cast<pugi::xml_document &>(this->comments_extended));
        } else if (!this->document_path.empty() &&
                   std::filesystem::exists(this->document_path)) {
            const auto entry_name = related_part_entry_for_type(
                this->document_relationships, comments_extended_relationship_type,
                comments_extended_xml_entry);
            pugi::xml_document comments_extended_document;
            if (!load_optional_docx_xml_part(
                    this->document_path, entry_name, comments_extended_document,
                    this->last_error_info)) {
                return {};
            }
            attach_comment_extended_metadata(
                summaries, comments_document, comments_extended_document);
        }
        this->last_error_info.clear();
        return summaries;
    }
    if (this->document_path.empty() ||
        !std::filesystem::exists(this->document_path)) {
        this->last_error_info.clear();
        return {};
    }

    const auto entry_name = related_part_entry_for_type(
        this->document_relationships, comments_relationship_type, "word/comments.xml");
    pugi::xml_document comments_document;
    if (!load_optional_docx_xml_part(this->document_path, entry_name,
                                     comments_document, this->last_error_info)) {
        return {};
    }

    auto summaries = summarize_comments_from_part(comments_document);
    attach_comment_anchor_text(summaries, anchor_text_by_id);
    const auto comments_extended_entry_name = related_part_entry_for_type(
        this->document_relationships, comments_extended_relationship_type,
        comments_extended_xml_entry);
    pugi::xml_document comments_extended_document;
    if (!load_optional_docx_xml_part(this->document_path,
                                     comments_extended_entry_name,
                                     comments_extended_document,
                                     this->last_error_info)) {
        return {};
    }
    attach_comment_extended_metadata(summaries, comments_document,
                                     comments_extended_document);
    this->last_error_info.clear();
    return summaries;
}

std::size_t Document::append_comment(std::string_view selected_text,
                                     std::string_view comment_text,
                                     std::string_view author,
                                     std::string_view initials,
                                     std::string_view date) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before appending a comment",
                       std::string{document_xml_entry});
        return 0U;
    }
    if (selected_text.empty() || comment_text.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "comment selected text and body text must not be empty",
                       std::string{comments_xml_entry});
        return 0U;
    }
    if (!this->ensure_comments_part()) {
        return 0U;
    }

    auto root = this->comments.child("w:comments");
    auto body = template_part_block_container(this->document);
    if (root == pugi::xml_node{} || body == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "document is not ready for comment insertion",
                       std::string{document_xml_entry});
        return 0U;
    }

    const auto comment_id = next_numeric_id(root, "w:comment", 0L);
    const auto comment_id_text = std::to_string(comment_id);
    auto comment = root.append_child("w:comment");
    if (comment == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to append comment XML",
                       std::string{comments_xml_entry});
        return 0U;
    }
    ensure_attribute_value(comment, "w:id", comment_id_text);
    if (!author.empty()) {
        ensure_attribute_value(comment, "w:author", author);
    }
    if (!initials.empty()) {
        ensure_attribute_value(comment, "w:initials", initials);
    }
    if (!date.empty()) {
        ensure_attribute_value(comment, "w:date", date);
    }
    if (!set_comment_text(comment, comment_text)) {
        root.remove_child(comment);
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to append comment text",
                       std::string{comments_xml_entry});
        return 0U;
    }

    auto paragraph = featherdoc::detail::append_paragraph_node(body);
    auto range_start = paragraph.append_child("w:commentRangeStart");
    if (paragraph == pugi::xml_node{} || range_start == pugi::xml_node{} ||
        !featherdoc::detail::append_plain_text_run(paragraph, selected_text)) {
        root.remove_child(comment);
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to append comment range",
                       std::string{document_xml_entry});
        return 0U;
    }
    auto range_end = paragraph.append_child("w:commentRangeEnd");
    auto reference_run = paragraph.append_child("w:r");
    auto reference = reference_run.append_child("w:commentReference");
    if (range_end == pugi::xml_node{} || reference_run == pugi::xml_node{} ||
        reference == pugi::xml_node{}) {
        root.remove_child(comment);
        body.remove_child(paragraph);
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to append comment reference",
                       std::string{document_xml_entry});
        return 0U;
    }
    ensure_attribute_value(range_start, "w:id", comment_id_text);
    ensure_attribute_value(range_end, "w:id", comment_id_text);
    ensure_attribute_value(reference, "w:id", comment_id_text);

    this->comments_dirty = true;
    this->last_error_info.clear();
    return 1U;
}

std::size_t Document::append_paragraph_text_comment(
    std::size_t paragraph_index, std::size_t text_offset,
    std::size_t text_length, std::string_view comment_text,
    std::string_view author, std::string_view initials,
    std::string_view date) {
    if (text_length > std::numeric_limits<std::size_t>::max() - text_offset) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "text range is out of range",
                       std::string{document_xml_entry});
        return 0U;
    }
    return this->append_text_range_comment(paragraph_index, text_offset,
                                           paragraph_index,
                                           text_offset + text_length,
                                           comment_text, author, initials, date);
}

std::size_t Document::append_text_range_comment(
    std::size_t start_paragraph_index, std::size_t start_text_offset,
    std::size_t end_paragraph_index, std::size_t end_text_offset,
    std::string_view comment_text, std::string_view author,
    std::string_view initials, std::string_view date) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before appending a text range comment",
                       std::string{document_xml_entry});
        return 0U;
    }
    if (comment_text.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "comment text must not be empty",
                       std::string{comments_xml_entry});
        return 0U;
    }
    if (end_paragraph_index < start_paragraph_index) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "text range start must not be after end",
                       std::string{document_xml_entry});
        return 0U;
    }

    std::vector<pugi::xml_node> paragraphs;
    auto paragraph_handle = this->paragraphs();
    while (paragraph_handle.has_next()) {
        paragraphs.push_back(paragraph_handle.current);
        paragraph_handle.next();
    }

    paragraph_revision_range_plan plan;
    std::string range_error;
    if (!build_paragraph_revision_range_plan(
            paragraphs, start_paragraph_index, start_text_offset,
            end_paragraph_index, end_text_offset, plan, range_error)) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       range_error, std::string{document_xml_entry});
        return 0U;
    }
    if (!split_paragraph_revision_range_segments(plan.segments)) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::operation_not_supported),
                       "text range comment requires plain text runs",
                       std::string{document_xml_entry});
        return 0U;
    }

    const auto selected_segments =
        collect_selected_paragraph_revision_spans(plan.segments);
    if (selected_segments.size() != plan.segments.size()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "text range is out of range",
                       std::string{document_xml_entry});
        return 0U;
    }
    if (!selected_paragraph_revision_spans_supported(selected_segments)) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::operation_not_supported),
                       "text range comment requires plain text runs",
                       std::string{document_xml_entry});
        return 0U;
    }
    if (!this->ensure_comments_part()) {
        return 0U;
    }

    auto root = this->comments.child("w:comments");
    if (root == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "document is not ready for comment insertion",
                       std::string{comments_xml_entry});
        return 0U;
    }

    const auto comment_id = next_numeric_id(root, "w:comment", 0L);
    const auto comment_id_text = std::to_string(comment_id);
    if (!insert_comment_markers_for_selected_spans(
            plan.segments, selected_segments, comment_id_text)) {
        remove_comment_markers_by_id(this->document, comment_id_text);
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to append text range comment markers",
                       std::string{document_xml_entry});
        return 0U;
    }

    auto comment = root.append_child("w:comment");
    if (comment == pugi::xml_node{}) {
        remove_comment_markers_by_id(this->document, comment_id_text);
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to append comment XML",
                       std::string{comments_xml_entry});
        return 0U;
    }
    ensure_attribute_value(comment, "w:id", comment_id_text);
    if (!author.empty()) {
        ensure_attribute_value(comment, "w:author", author);
    }
    if (!initials.empty()) {
        ensure_attribute_value(comment, "w:initials", initials);
    }
    if (!date.empty()) {
        ensure_attribute_value(comment, "w:date", date);
    }
    if (!date.empty()) {
        ensure_attribute_value(comment, "w:date", date);
    }
    if (!set_comment_text(comment, comment_text)) {
        root.remove_child(comment);
        remove_comment_markers_by_id(this->document, comment_id_text);
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to append comment text",
                       std::string{comments_xml_entry});
        return 0U;
    }

    this->comments_dirty = true;
    this->last_error_info.clear();
    return 1U;
}

std::size_t Document::append_comment_reply(
    std::size_t parent_comment_index, std::string_view comment_text,
    std::string_view author, std::string_view initials,
    std::string_view date) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before appending a comment reply",
                       std::string{document_xml_entry});
        return 0U;
    }
    if (comment_text.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "comment reply text must not be empty",
                       std::string{comments_xml_entry});
        return 0U;
    }
    if (!this->ensure_comments_part()) {
        return 0U;
    }

    auto comments_root = this->comments.child("w:comments");
    auto parent_comment =
        comment_by_summary_index(comments_root, parent_comment_index);
    if (parent_comment == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "parent comment index is out of range",
                       std::string{comments_xml_entry});
        return 0U;
    }
    const auto parent_para_id = ensure_comment_paragraph_id(
        comments_root, parent_comment, this->comments_dirty);
    if (!parent_para_id.has_value()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "parent comment body does not contain a paragraph id target",
                       std::string{comments_xml_entry});
        return 0U;
    }
    if (!this->ensure_comments_extended_part()) {
        return 0U;
    }

    auto extended_root = this->comments_extended.child("w15:commentsEx");
    auto parent_comment_ex =
        ensure_comment_extended_by_paragraph_id(extended_root, *parent_para_id);
    if (parent_comment_ex == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to append parent comment metadata",
                       std::string{comments_extended_xml_entry});
        return 0U;
    }

    const auto comment_id = next_numeric_id(comments_root, "w:comment", 0L);
    const auto comment_id_text = std::to_string(comment_id);
    auto reply = comments_root.append_child("w:comment");
    if (reply == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to append comment reply XML",
                       std::string{comments_xml_entry});
        return 0U;
    }
    ensure_attribute_value(reply, "w:id", comment_id_text);
    if (!author.empty()) {
        ensure_attribute_value(reply, "w:author", author);
    }
    if (!initials.empty()) {
        ensure_attribute_value(reply, "w:initials", initials);
    }
    if (!date.empty()) {
        ensure_attribute_value(reply, "w:date", date);
    }
    if (!set_comment_text(reply, comment_text)) {
        comments_root.remove_child(reply);
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to append comment reply text",
                       std::string{comments_xml_entry});
        return 0U;
    }

    const auto reply_para_id =
        ensure_comment_paragraph_id(comments_root, reply, this->comments_dirty);
    if (!reply_para_id.has_value()) {
        comments_root.remove_child(reply);
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "comment reply body does not contain a paragraph id target",
                       std::string{comments_xml_entry});
        return 0U;
    }

    auto reply_comment_ex =
        ensure_comment_extended_by_paragraph_id(extended_root, *reply_para_id);
    if (reply_comment_ex == pugi::xml_node{}) {
        comments_root.remove_child(reply);
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to append comment reply metadata",
                       std::string{comments_extended_xml_entry});
        return 0U;
    }
    ensure_attribute_value(reply_comment_ex, "w15:paraIdParent",
                           *parent_para_id);

    this->comments_dirty = true;
    this->comments_extended_dirty = true;
    this->last_error_info.clear();
    return 1U;
}

bool Document::set_comment_metadata(
    std::size_t comment_index,
    const featherdoc::comment_metadata_update &metadata) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before setting comment metadata",
                       std::string{document_xml_entry});
        return false;
    }
    const auto has_change = metadata.author.has_value() ||
                            metadata.initials.has_value() ||
                            metadata.date.has_value() ||
                            metadata.clear_author ||
                            metadata.clear_initials ||
                            metadata.clear_date;
    if (!has_change) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "comment metadata update must contain at least one change",
                       std::string{comments_xml_entry});
        return false;
    }
    if ((metadata.author.has_value() && metadata.clear_author) ||
        (metadata.initials.has_value() && metadata.clear_initials) ||
        (metadata.date.has_value() && metadata.clear_date)) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "comment metadata cannot be set and cleared in the same update",
                       std::string{comments_xml_entry});
        return false;
    }
    if (!this->ensure_comments_part()) {
        return false;
    }

    auto root = this->comments.child("w:comments");
    auto comment = comment_by_summary_index(root, comment_index);
    if (comment == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "comment index is out of range",
                       std::string{comments_xml_entry});
        return false;
    }

    const auto apply_string_metadata =
        [](pugi::xml_node target, const char *name,
           const std::optional<std::string> &value, bool clear) {
            if (value.has_value()) {
                ensure_attribute_value(target, name, *value);
            } else if (clear) {
                target.remove_attribute(name);
            }
        };
    apply_string_metadata(comment, "w:author", metadata.author,
                          metadata.clear_author);
    apply_string_metadata(comment, "w:initials", metadata.initials,
                          metadata.clear_initials);
    apply_string_metadata(comment, "w:date", metadata.date,
                          metadata.clear_date);

    this->comments_dirty = true;
    this->last_error_info.clear();
    return true;
}

bool Document::set_paragraph_text_comment_range(
    std::size_t comment_index, std::size_t paragraph_index,
    std::size_t text_offset, std::size_t text_length) {
    if (text_length > std::numeric_limits<std::size_t>::max() - text_offset) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "text range is out of range",
                       std::string{document_xml_entry});
        return false;
    }
    return this->set_text_range_comment_range(comment_index, paragraph_index,
                                              text_offset, paragraph_index,
                                              text_offset + text_length);
}

bool Document::set_text_range_comment_range(
    std::size_t comment_index, std::size_t start_paragraph_index,
    std::size_t start_text_offset, std::size_t end_paragraph_index,
    std::size_t end_text_offset) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before moving a comment range",
                       std::string{document_xml_entry});
        return false;
    }
    if (end_paragraph_index < start_paragraph_index) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "text range start must not be after end",
                       std::string{document_xml_entry});
        return false;
    }
    if (!this->ensure_comments_part()) {
        return false;
    }

    auto root = this->comments.child("w:comments");
    auto comment = comment_by_summary_index(root, comment_index);
    if (comment == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "comment index is out of range",
                       std::string{comments_xml_entry});
        return false;
    }
    const auto comment_id = std::string{comment.attribute("w:id").value()};
    if (comment_id.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "comment id is missing",
                       std::string{comments_xml_entry});
        return false;
    }

    std::vector<pugi::xml_node> paragraphs;
    auto paragraph_handle = this->paragraphs();
    while (paragraph_handle.has_next()) {
        paragraphs.push_back(paragraph_handle.current);
        paragraph_handle.next();
    }

    paragraph_revision_range_plan plan;
    std::string range_error;
    if (!build_paragraph_revision_range_plan(
            paragraphs, start_paragraph_index, start_text_offset,
            end_paragraph_index, end_text_offset, plan, range_error)) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       range_error, std::string{document_xml_entry});
        return false;
    }
    if (!split_paragraph_revision_range_segments(plan.segments)) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::operation_not_supported),
                       "comment range requires plain text runs",
                       std::string{document_xml_entry});
        return false;
    }

    const auto selected_segments =
        collect_selected_paragraph_revision_spans(plan.segments);
    if (selected_segments.size() != plan.segments.size()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "text range is out of range",
                       std::string{document_xml_entry});
        return false;
    }
    if (!selected_paragraph_revision_spans_supported(selected_segments)) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::operation_not_supported),
                       "comment range requires plain text runs",
                       std::string{document_xml_entry});
        return false;
    }

    remove_comment_markers_by_id(this->document, comment_id);
    for (auto &part : this->header_parts) {
        if (part != nullptr) {
            remove_comment_markers_by_id(part->xml, comment_id);
        }
    }
    for (auto &part : this->footer_parts) {
        if (part != nullptr) {
            remove_comment_markers_by_id(part->xml, comment_id);
        }
    }

    if (!insert_comment_markers_for_selected_spans(plan.segments,
                                                   selected_segments,
                                                   comment_id)) {
        remove_comment_markers_by_id(this->document, comment_id);
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to move comment range",
                       std::string{document_xml_entry});
        return false;
    }

    this->last_error_info.clear();
    return true;
}

bool Document::set_comment_resolved(std::size_t comment_index, bool resolved) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before setting comment resolved state",
                       std::string{document_xml_entry});
        return false;
    }
    if (!this->ensure_comments_part()) {
        return false;
    }

    auto comments_root = this->comments.child("w:comments");
    auto comment = comment_by_summary_index(comments_root, comment_index);
    if (comment == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "comment index is out of range",
                       std::string{comments_xml_entry});
        return false;
    }
    const auto para_id =
        ensure_comment_paragraph_id(comments_root, comment, this->comments_dirty);
    if (!para_id.has_value()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "comment body does not contain a paragraph id target",
                       std::string{comments_xml_entry});
        return false;
    }

    if (!this->ensure_comments_extended_part()) {
        return false;
    }
    auto extended_root = this->comments_extended.child("w15:commentsEx");
    auto comment_ex = comment_extended_by_paragraph_id(extended_root, *para_id);
    if (comment_ex == pugi::xml_node{}) {
        comment_ex = extended_root.append_child("w15:commentEx");
    }
    if (comment_ex == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to append extended comment metadata",
                       std::string{comments_extended_xml_entry});
        return false;
    }
    ensure_attribute_value(comment_ex, "w15:paraId", *para_id);
    ensure_attribute_value(comment_ex, "w15:done", resolved ? "1" : "0");

    this->comments_extended_dirty = true;
    this->last_error_info.clear();
    return true;
}

bool Document::replace_comment(std::size_t comment_index,
                               std::string_view comment_text) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before replacing a comment",
                       std::string{document_xml_entry});
        return false;
    }
    if (comment_text.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "comment text must not be empty",
                       std::string{comments_xml_entry});
        return false;
    }
    if (!this->ensure_comments_part()) {
        return false;
    }
    auto root = this->comments.child("w:comments");
    auto comment = comment_by_summary_index(root, comment_index);
    if (comment == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "comment index is out of range",
                       std::string{comments_xml_entry});
        return false;
    }
    if (!set_comment_text(comment, comment_text)) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to replace comment text",
                       std::string{comments_xml_entry});
        return false;
    }
    this->comments_dirty = true;
    this->last_error_info.clear();
    return true;
}

bool Document::remove_comment(std::size_t comment_index) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before removing a comment",
                       std::string{document_xml_entry});
        return false;
    }
    if (!this->ensure_comments_part()) {
        return false;
    }
    auto root = this->comments.child("w:comments");
    auto comment = comment_by_summary_index(root, comment_index);
    if (comment == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "comment index is out of range",
                       std::string{comments_xml_entry});
        return false;
    }
    const auto comment_id = std::string{comment.attribute("w:id").value()};
    const auto para_id = comment_paragraph_id(comment);
    std::unordered_set<std::string> comment_ids_to_remove;
    if (!comment_id.empty()) {
        comment_ids_to_remove.insert(comment_id);
    }
    std::unordered_set<std::string> para_ids_to_remove;
    if (para_id.has_value() && !this->ensure_comments_extended_loaded()) {
        return false;
    }

    if (para_id.has_value()) {
        para_ids_to_remove =
            comment_thread_para_ids(this->comments_extended, *para_id);
        for (auto current = root.child("w:comment");
             current != pugi::xml_node{};
             current = current.next_sibling("w:comment")) {
            const auto current_para_id = comment_paragraph_id(current);
            if (!current_para_id.has_value() ||
                !para_ids_to_remove.contains(*current_para_id)) {
                continue;
            }
            const auto current_id =
                std::string{current.attribute("w:id").value()};
            if (!current_id.empty()) {
                comment_ids_to_remove.insert(current_id);
            }
        }
    }

    if (comment_id.empty()) {
        root.remove_child(comment);
    }
    for (auto current = root.child("w:comment"); current != pugi::xml_node{};) {
        const auto next = current.next_sibling("w:comment");
        const auto current_id = std::string{current.attribute("w:id").value()};
        if (!current_id.empty() && comment_ids_to_remove.contains(current_id)) {
            root.remove_child(current);
        }
        current = next;
    }

    for (const auto &removed_comment_id : comment_ids_to_remove) {
        remove_comment_markers_by_id(this->document, removed_comment_id);
        for (auto &part : this->header_parts) {
            if (part != nullptr) {
                remove_comment_markers_by_id(part->xml, removed_comment_id);
            }
        }
        for (auto &part : this->footer_parts) {
            if (part != nullptr) {
                remove_comment_markers_by_id(part->xml, removed_comment_id);
            }
        }
    }

    bool removed_extended_metadata = false;
    for (const auto &removed_para_id : para_ids_to_remove) {
        removed_extended_metadata =
            remove_comment_extended_by_paragraph_id(this->comments_extended,
                                                    removed_para_id) ||
            removed_extended_metadata;
    }
    if (removed_extended_metadata) {
        this->comments_extended_dirty = true;
    }
    this->comments_dirty = true;
    this->last_error_info.clear();
    return true;
}

std::size_t Document::append_insertion_revision(
    std::string_view text, std::string_view author, std::string_view date) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before appending an insertion revision",
                       std::string{document_xml_entry});
        return 0U;
    }
    if (text.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "revision text must not be empty",
                       std::string{document_xml_entry});
        return 0U;
    }

    auto body = template_part_block_container(this->document);
    if (body == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "document is not ready for revision insertion",
                       std::string{document_xml_entry});
        return 0U;
    }

    long next_id = 1L;
    collect_revision_ids(this->document, next_id);
    for (const auto &part : this->header_parts) {
        if (part != nullptr) {
            collect_revision_ids(part->xml, next_id);
        }
    }
    for (const auto &part : this->footer_parts) {
        if (part != nullptr) {
            collect_revision_ids(part->xml, next_id);
        }
    }

    auto paragraph = featherdoc::detail::append_paragraph_node(body);
    auto revision = paragraph.append_child("w:ins");
    auto run = revision.append_child("w:r");
    if (paragraph == pugi::xml_node{} || revision == pugi::xml_node{} ||
        run == pugi::xml_node{} ||
        !set_revision_run_text_content(run, "w:t", text)) {
        if (paragraph != pugi::xml_node{}) {
            body.remove_child(paragraph);
        }
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to append insertion revision",
                       std::string{document_xml_entry});
        return 0U;
    }
    ensure_attribute_value(revision, "w:id", std::to_string(next_id));
    if (!author.empty()) {
        ensure_attribute_value(revision, "w:author", author);
    }
    if (!date.empty()) {
        ensure_attribute_value(revision, "w:date", date);
    }

    this->last_error_info.clear();
    return 1U;
}

std::size_t Document::append_deletion_revision(
    std::string_view text, std::string_view author, std::string_view date) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before appending a deletion revision",
                       std::string{document_xml_entry});
        return 0U;
    }
    if (text.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "revision text must not be empty",
                       std::string{document_xml_entry});
        return 0U;
    }

    auto body = template_part_block_container(this->document);
    if (body == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "document is not ready for revision insertion",
                       std::string{document_xml_entry});
        return 0U;
    }

    long next_id = 1L;
    collect_revision_ids(this->document, next_id);
    for (const auto &part : this->header_parts) {
        if (part != nullptr) {
            collect_revision_ids(part->xml, next_id);
        }
    }
    for (const auto &part : this->footer_parts) {
        if (part != nullptr) {
            collect_revision_ids(part->xml, next_id);
        }
    }

    auto paragraph = featherdoc::detail::append_paragraph_node(body);
    auto revision = paragraph.append_child("w:del");
    auto run = revision.append_child("w:r");
    if (paragraph == pugi::xml_node{} || revision == pugi::xml_node{} ||
        run == pugi::xml_node{} ||
        !set_revision_run_text_content(run, "w:delText", text)) {
        if (paragraph != pugi::xml_node{}) {
            body.remove_child(paragraph);
        }
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to append deletion revision",
                       std::string{document_xml_entry});
        return 0U;
    }
    ensure_attribute_value(revision, "w:id", std::to_string(next_id));
    if (!author.empty()) {
        ensure_attribute_value(revision, "w:author", author);
    }
    if (!date.empty()) {
        ensure_attribute_value(revision, "w:date", date);
    }

    this->last_error_info.clear();
    return 1U;
}

bool Document::insert_run_revision_after(
    std::size_t paragraph_index, std::size_t run_index, std::string_view text,
    std::string_view author, std::string_view date) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before inserting a run revision",
                       std::string{document_xml_entry});
        return false;
    }
    if (text.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "revision text must not be empty",
                       std::string{document_xml_entry});
        return false;
    }

    auto paragraph_handle = this->paragraphs();
    for (std::size_t current_index = 0U;
         current_index < paragraph_index && paragraph_handle.has_next();
         ++current_index) {
        paragraph_handle.next();
    }
    auto paragraph = paragraph_handle.has_next() ? paragraph_handle.current : pugi::xml_node{};
    if (paragraph == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "paragraph index is out of range",
                       std::string{document_xml_entry});
        return false;
    }
    auto anchor_run = paragraph_run_by_index(paragraph, run_index);
    if (anchor_run == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "run index is out of range",
                       std::string{document_xml_entry});
        return false;
    }

    const auto next_run = featherdoc::detail::next_named_sibling(anchor_run, "w:r");
    auto revision = next_run == pugi::xml_node{}
                        ? paragraph.append_child("w:ins")
                        : paragraph.insert_child_before("w:ins", next_run);
    if (revision == pugi::xml_node{} ||
        append_revision_run(revision, anchor_run, "w:t", text) == pugi::xml_node{}) {
        if (revision != pugi::xml_node{}) {
            paragraph.remove_child(revision);
        }
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to insert run revision",
                       std::string{document_xml_entry});
        return false;
    }
    long next_id = 1L;
    collect_revision_ids(this->document, next_id);
    for (const auto &part : this->header_parts) {
        if (part != nullptr) {
            collect_revision_ids(part->xml, next_id);
        }
    }
    for (const auto &part : this->footer_parts) {
        if (part != nullptr) {
            collect_revision_ids(part->xml, next_id);
        }
    }
    set_revision_identity_metadata(revision, next_id, author, date);
    this->last_error_info.clear();
    return true;
}

bool Document::delete_run_revision(
    std::size_t paragraph_index, std::size_t run_index, std::string_view author,
    std::string_view date) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before deleting a run revision",
                       std::string{document_xml_entry});
        return false;
    }

    auto paragraph_handle = this->paragraphs();
    for (std::size_t current_index = 0U;
         current_index < paragraph_index && paragraph_handle.has_next();
         ++current_index) {
        paragraph_handle.next();
    }
    auto paragraph = paragraph_handle.has_next() ? paragraph_handle.current : pugi::xml_node{};
    if (paragraph == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "paragraph index is out of range",
                       std::string{document_xml_entry});
        return false;
    }
    auto target_run = paragraph_run_by_index(paragraph, run_index);
    if (target_run == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "run index is out of range",
                       std::string{document_xml_entry});
        return false;
    }

    const auto original_text = featherdoc::detail::collect_plain_text_from_xml(target_run);
    if (original_text.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "run revision deletion requires non-empty run text",
                       std::string{document_xml_entry});
        return false;
    }

    auto revision = paragraph.insert_child_before("w:del", target_run);
    if (revision == pugi::xml_node{} ||
        append_revision_run(revision, target_run, "w:delText", original_text) ==
            pugi::xml_node{} ||
        !paragraph.remove_child(target_run)) {
        if (revision != pugi::xml_node{}) {
            paragraph.remove_child(revision);
        }
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to delete run revision",
                       std::string{document_xml_entry});
        return false;
    }
    long next_id = 1L;
    collect_revision_ids(this->document, next_id);
    for (const auto &part : this->header_parts) {
        if (part != nullptr) {
            collect_revision_ids(part->xml, next_id);
        }
    }
    for (const auto &part : this->footer_parts) {
        if (part != nullptr) {
            collect_revision_ids(part->xml, next_id);
        }
    }
    set_revision_identity_metadata(revision, next_id, author, date);
    this->last_error_info.clear();
    return true;
}

bool Document::replace_run_revision(
    std::size_t paragraph_index, std::size_t run_index, std::string_view text,
    std::string_view author, std::string_view date) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before replacing a run revision",
                       std::string{document_xml_entry});
        return false;
    }
    if (text.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "revision text must not be empty",
                       std::string{document_xml_entry});
        return false;
    }

    auto paragraph_handle = this->paragraphs();
    for (std::size_t current_index = 0U;
         current_index < paragraph_index && paragraph_handle.has_next();
         ++current_index) {
        paragraph_handle.next();
    }
    auto paragraph = paragraph_handle.has_next() ? paragraph_handle.current : pugi::xml_node{};
    if (paragraph == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "paragraph index is out of range",
                       std::string{document_xml_entry});
        return false;
    }
    auto target_run = paragraph_run_by_index(paragraph, run_index);
    if (target_run == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "run index is out of range",
                       std::string{document_xml_entry});
        return false;
    }

    const auto original_text = featherdoc::detail::collect_plain_text_from_xml(target_run);
    if (original_text.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "run revision replacement requires non-empty run text",
                       std::string{document_xml_entry});
        return false;
    }

    long revision_id = 1L;
    collect_revision_ids(this->document, revision_id);
    for (const auto &part : this->header_parts) {
        if (part != nullptr) {
            collect_revision_ids(part->xml, revision_id);
        }
    }
    for (const auto &part : this->footer_parts) {
        if (part != nullptr) {
            collect_revision_ids(part->xml, revision_id);
        }
    }
    auto deletion = paragraph.insert_child_before("w:del", target_run);
    auto insertion = deletion == pugi::xml_node{}
                         ? pugi::xml_node{}
                         : paragraph.insert_child_before("w:ins", target_run);
    if (deletion == pugi::xml_node{} || insertion == pugi::xml_node{} ||
        append_revision_run(deletion, target_run, "w:delText", original_text) ==
            pugi::xml_node{} ||
        append_revision_run(insertion, target_run, "w:t", text) == pugi::xml_node{} ||
        !paragraph.remove_child(target_run)) {
        if (deletion != pugi::xml_node{}) {
            paragraph.remove_child(deletion);
        }
        if (insertion != pugi::xml_node{}) {
            paragraph.remove_child(insertion);
        }
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to replace run revision",
                       std::string{document_xml_entry});
        return false;
    }
    set_revision_identity_metadata(deletion, revision_id, author, date);
    set_revision_identity_metadata(insertion, revision_id + 1L, author, date);
    this->last_error_info.clear();
    return true;
}

bool Document::insert_paragraph_text_revision(
    std::size_t paragraph_index, std::size_t text_offset, std::string_view text,
    std::string_view author, std::string_view date) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before inserting a paragraph text revision",
                       std::string{document_xml_entry});
        return false;
    }
    if (text.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "revision text must not be empty",
                       std::string{document_xml_entry});
        return false;
    }

    auto paragraph_handle = this->paragraphs();
    for (std::size_t current_index = 0U;
         current_index < paragraph_index && paragraph_handle.has_next();
         ++current_index) {
        paragraph_handle.next();
    }
    auto paragraph = paragraph_handle.has_next() ? paragraph_handle.current : pugi::xml_node{};
    if (paragraph == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "paragraph index is out of range",
                       std::string{document_xml_entry});
        return false;
    }

    const auto spans = collect_paragraph_revision_run_spans(paragraph);
    const auto paragraph_text_length = paragraph_revision_text_length(spans);
    if (text_offset > paragraph_text_length) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "text offset is out of range",
                       std::string{document_xml_entry});
        return false;
    }
    if (!split_paragraph_revision_text_at(paragraph, text_offset)) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::operation_not_supported),
                       "paragraph text revision range requires plain text runs",
                       std::string{document_xml_entry});
        return false;
    }

    const auto insert_before =
        paragraph_revision_insert_before(paragraph, text_offset);
    const auto style_run = paragraph_revision_style_run(paragraph, text_offset);
    auto revision = insert_revision_node(paragraph, "w:ins", insert_before);
    if (revision == pugi::xml_node{} ||
        append_revision_run(revision, style_run, "w:t", text) == pugi::xml_node{}) {
        if (revision != pugi::xml_node{}) {
            paragraph.remove_child(revision);
        }
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to insert paragraph text revision",
                       std::string{document_xml_entry});
        return false;
    }

    long revision_id = 1L;
    collect_revision_ids(this->document, revision_id);
    for (const auto &part : this->header_parts) {
        if (part != nullptr) {
            collect_revision_ids(part->xml, revision_id);
        }
    }
    for (const auto &part : this->footer_parts) {
        if (part != nullptr) {
            collect_revision_ids(part->xml, revision_id);
        }
    }
    set_revision_identity_metadata(revision, revision_id, author, date);
    this->last_error_info.clear();
    return true;
}

bool Document::delete_paragraph_text_revision(
    std::size_t paragraph_index, std::size_t text_offset, std::size_t text_length,
    std::string_view author, std::string_view date) {
    featherdoc::revision_text_range_options options;
    options.author = std::string{author};
    options.date = std::string{date};
    return this->delete_paragraph_text_revision(paragraph_index, text_offset,
                                                text_length, options);
}

bool Document::delete_paragraph_text_revision(
    std::size_t paragraph_index, std::size_t text_offset, std::size_t text_length,
    const featherdoc::revision_text_range_options &options) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before deleting a paragraph text revision",
                       std::string{document_xml_entry});
        return false;
    }
    if (text_length == 0U) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "text range must not be empty",
                       std::string{document_xml_entry});
        return false;
    }
    if (text_length > std::numeric_limits<std::size_t>::max() - text_offset) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "text range is out of range",
                       std::string{document_xml_entry});
        return false;
    }
    if (options.expected_text.has_value()) {
        const auto preview = this->preview_text_range(
            paragraph_index, text_offset, paragraph_index,
            text_offset + text_length);
        if (!preview.has_value() ||
            !validate_revision_expected_text(this->last_error_info, *preview,
                                             options)) {
            return false;
        }
    }

    auto paragraph_handle = this->paragraphs();
    for (std::size_t current_index = 0U;
         current_index < paragraph_index && paragraph_handle.has_next();
         ++current_index) {
        paragraph_handle.next();
    }
    auto paragraph = paragraph_handle.has_next() ? paragraph_handle.current : pugi::xml_node{};
    if (paragraph == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "paragraph index is out of range",
                       std::string{document_xml_entry});
        return false;
    }

    const auto spans = collect_paragraph_revision_run_spans(paragraph);
    const auto paragraph_text_length = paragraph_revision_text_length(spans);
    if (text_offset > paragraph_text_length ||
        text_length > paragraph_text_length - text_offset) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "text range is out of range",
                       std::string{document_xml_entry});
        return false;
    }
    if (!split_paragraph_revision_text_at(paragraph, text_offset + text_length) ||
        !split_paragraph_revision_text_at(paragraph, text_offset)) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::operation_not_supported),
                       "paragraph text revision range requires plain text runs",
                       std::string{document_xml_entry});
        return false;
    }

    const auto selected =
        paragraph_revision_spans_in_range(paragraph, text_offset, text_length);
    if (selected.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "text range is out of range",
                       std::string{document_xml_entry});
        return false;
    }

    auto revision = paragraph.insert_child_before("w:del", selected.front().run);
    if (revision == pugi::xml_node{} ||
        !append_revision_runs_from_spans(revision, selected, "w:delText")) {
        if (revision != pugi::xml_node{}) {
            paragraph.remove_child(revision);
        }
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to delete paragraph text revision",
                       std::string{document_xml_entry});
        return false;
    }

    for (const auto &span : selected) {
        if (!paragraph.remove_child(span.run)) {
            paragraph.remove_child(revision);
            set_last_error(this->last_error_info,
                           std::make_error_code(std::errc::not_enough_memory),
                           "failed to delete paragraph text revision",
                           std::string{document_xml_entry});
            return false;
        }
    }

    long revision_id = 1L;
    collect_revision_ids(this->document, revision_id);
    for (const auto &part : this->header_parts) {
        if (part != nullptr) {
            collect_revision_ids(part->xml, revision_id);
        }
    }
    for (const auto &part : this->footer_parts) {
        if (part != nullptr) {
            collect_revision_ids(part->xml, revision_id);
        }
    }
    set_revision_identity_metadata(revision, revision_id, options.author,
                                   options.date);
    this->last_error_info.clear();
    return true;
}

bool Document::replace_paragraph_text_revision(
    std::size_t paragraph_index, std::size_t text_offset, std::size_t text_length,
    std::string_view text, std::string_view author, std::string_view date) {
    featherdoc::revision_text_range_options options;
    options.author = std::string{author};
    options.date = std::string{date};
    return this->replace_paragraph_text_revision(paragraph_index, text_offset,
                                                 text_length, text, options);
}

bool Document::replace_paragraph_text_revision(
    std::size_t paragraph_index, std::size_t text_offset, std::size_t text_length,
    std::string_view text,
    const featherdoc::revision_text_range_options &options) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before replacing a paragraph text revision",
                       std::string{document_xml_entry});
        return false;
    }
    if (text.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "revision text must not be empty",
                       std::string{document_xml_entry});
        return false;
    }
    if (text_length == 0U) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "text range must not be empty",
                       std::string{document_xml_entry});
        return false;
    }
    if (text_length > std::numeric_limits<std::size_t>::max() - text_offset) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "text range is out of range",
                       std::string{document_xml_entry});
        return false;
    }
    if (options.expected_text.has_value()) {
        const auto preview = this->preview_text_range(
            paragraph_index, text_offset, paragraph_index,
            text_offset + text_length);
        if (!preview.has_value() ||
            !validate_revision_expected_text(this->last_error_info, *preview,
                                             options)) {
            return false;
        }
    }

    auto paragraph_handle = this->paragraphs();
    for (std::size_t current_index = 0U;
         current_index < paragraph_index && paragraph_handle.has_next();
         ++current_index) {
        paragraph_handle.next();
    }
    auto paragraph = paragraph_handle.has_next() ? paragraph_handle.current : pugi::xml_node{};
    if (paragraph == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "paragraph index is out of range",
                       std::string{document_xml_entry});
        return false;
    }

    const auto spans = collect_paragraph_revision_run_spans(paragraph);
    const auto paragraph_text_length = paragraph_revision_text_length(spans);
    if (text_offset > paragraph_text_length ||
        text_length > paragraph_text_length - text_offset) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "text range is out of range",
                       std::string{document_xml_entry});
        return false;
    }
    if (!split_paragraph_revision_text_at(paragraph, text_offset + text_length) ||
        !split_paragraph_revision_text_at(paragraph, text_offset)) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::operation_not_supported),
                       "paragraph text revision range requires plain text runs",
                       std::string{document_xml_entry});
        return false;
    }

    const auto selected =
        paragraph_revision_spans_in_range(paragraph, text_offset, text_length);
    if (selected.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "text range is out of range",
                       std::string{document_xml_entry});
        return false;
    }

    long revision_id = 1L;
    collect_revision_ids(this->document, revision_id);
    for (const auto &part : this->header_parts) {
        if (part != nullptr) {
            collect_revision_ids(part->xml, revision_id);
        }
    }
    for (const auto &part : this->footer_parts) {
        if (part != nullptr) {
            collect_revision_ids(part->xml, revision_id);
        }
    }

    auto deletion = paragraph.insert_child_before("w:del", selected.front().run);
    auto insertion = deletion == pugi::xml_node{}
                         ? pugi::xml_node{}
                         : paragraph.insert_child_before("w:ins", selected.front().run);
    if (deletion == pugi::xml_node{} || insertion == pugi::xml_node{} ||
        !append_revision_runs_from_spans(deletion, selected, "w:delText") ||
        append_revision_run(insertion, selected.front().run, "w:t", text) ==
            pugi::xml_node{}) {
        if (deletion != pugi::xml_node{}) {
            paragraph.remove_child(deletion);
        }
        if (insertion != pugi::xml_node{}) {
            paragraph.remove_child(insertion);
        }
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to replace paragraph text revision",
                       std::string{document_xml_entry});
        return false;
    }

    for (const auto &span : selected) {
        if (!paragraph.remove_child(span.run)) {
            paragraph.remove_child(deletion);
            paragraph.remove_child(insertion);
            set_last_error(this->last_error_info,
                           std::make_error_code(std::errc::not_enough_memory),
                           "failed to replace paragraph text revision",
                           std::string{document_xml_entry});
            return false;
        }
    }

    set_revision_identity_metadata(deletion, revision_id, options.author,
                                   options.date);
    set_revision_identity_metadata(insertion, revision_id + 1L, options.author,
                                   options.date);
    this->last_error_info.clear();
    return true;
}

bool Document::insert_text_range_revision(
    std::size_t paragraph_index, std::size_t text_offset, std::string_view text,
    std::string_view author, std::string_view date) {
    return this->insert_paragraph_text_revision(paragraph_index, text_offset, text,
                                                author, date);
}

bool Document::delete_text_range_revision(
    std::size_t start_paragraph_index, std::size_t start_text_offset,
    std::size_t end_paragraph_index, std::size_t end_text_offset,
    std::string_view author, std::string_view date) {
    featherdoc::revision_text_range_options options;
    options.author = std::string{author};
    options.date = std::string{date};
    return this->delete_text_range_revision(
        start_paragraph_index, start_text_offset, end_paragraph_index,
        end_text_offset, options);
}

bool Document::delete_text_range_revision(
    std::size_t start_paragraph_index, std::size_t start_text_offset,
    std::size_t end_paragraph_index, std::size_t end_text_offset,
    const featherdoc::revision_text_range_options &options) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before deleting a text range revision",
                       std::string{document_xml_entry});
        return false;
    }
    if (options.expected_text.has_value()) {
        const auto preview = this->preview_text_range(
            start_paragraph_index, start_text_offset, end_paragraph_index,
            end_text_offset);
        if (!preview.has_value() ||
            !validate_revision_expected_text(this->last_error_info, *preview,
                                             options)) {
            return false;
        }
    }

    std::vector<pugi::xml_node> paragraphs;
    auto paragraph_handle = this->paragraphs();
    while (paragraph_handle.has_next()) {
        paragraphs.push_back(paragraph_handle.current);
        paragraph_handle.next();
    }

    paragraph_revision_range_plan plan;
    std::string range_error;
    if (!build_paragraph_revision_range_plan(
            paragraphs, start_paragraph_index, start_text_offset,
            end_paragraph_index, end_text_offset, plan, range_error)) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       range_error, std::string{document_xml_entry});
        return false;
    }
    if (!split_paragraph_revision_range_segments(plan.segments)) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::operation_not_supported),
                       "text range revision requires plain text runs",
                       std::string{document_xml_entry});
        return false;
    }

    const auto selected_segments =
        collect_selected_paragraph_revision_spans(plan.segments);
    if (selected_segments.size() != plan.segments.size()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "text range is out of range",
                       std::string{document_xml_entry});
        return false;
    }

    long revision_id = 1L;
    collect_revision_ids(this->document, revision_id);
    for (const auto &part : this->header_parts) {
        if (part != nullptr) {
            collect_revision_ids(part->xml, revision_id);
        }
    }
    for (const auto &part : this->footer_parts) {
        if (part != nullptr) {
            collect_revision_ids(part->xml, revision_id);
        }
    }

    for (std::size_t segment_index = 0U; segment_index < plan.segments.size();
         ++segment_index) {
        auto paragraph = plan.segments[segment_index].paragraph;
        const auto &selected = selected_segments[segment_index];
        auto revision = paragraph.insert_child_before("w:del", selected.front().run);
        if (revision == pugi::xml_node{} ||
            !append_revision_runs_from_spans(revision, selected, "w:delText")) {
            if (revision != pugi::xml_node{}) {
                paragraph.remove_child(revision);
            }
            set_last_error(this->last_error_info,
                           std::make_error_code(std::errc::not_enough_memory),
                           "failed to delete text range revision",
                           std::string{document_xml_entry});
            return false;
        }

        for (const auto &span : selected) {
            if (!paragraph.remove_child(span.run)) {
                paragraph.remove_child(revision);
                set_last_error(this->last_error_info,
                               std::make_error_code(std::errc::not_enough_memory),
                               "failed to delete text range revision",
                               std::string{document_xml_entry});
                return false;
            }
        }

        set_revision_identity_metadata(revision, revision_id, options.author,
                                       options.date);
        ++revision_id;
    }

    this->last_error_info.clear();
    return true;
}

bool Document::replace_text_range_revision(
    std::size_t start_paragraph_index, std::size_t start_text_offset,
    std::size_t end_paragraph_index, std::size_t end_text_offset,
    std::string_view text, std::string_view author, std::string_view date) {
    featherdoc::revision_text_range_options options;
    options.author = std::string{author};
    options.date = std::string{date};
    return this->replace_text_range_revision(
        start_paragraph_index, start_text_offset, end_paragraph_index,
        end_text_offset, text, options);
}

bool Document::replace_text_range_revision(
    std::size_t start_paragraph_index, std::size_t start_text_offset,
    std::size_t end_paragraph_index, std::size_t end_text_offset,
    std::string_view text,
    const featherdoc::revision_text_range_options &options) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before replacing a text range revision",
                       std::string{document_xml_entry});
        return false;
    }
    if (text.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "revision text must not be empty",
                       std::string{document_xml_entry});
        return false;
    }
    if (options.expected_text.has_value()) {
        const auto preview = this->preview_text_range(
            start_paragraph_index, start_text_offset, end_paragraph_index,
            end_text_offset);
        if (!preview.has_value() ||
            !validate_revision_expected_text(this->last_error_info, *preview,
                                             options)) {
            return false;
        }
    }

    std::vector<pugi::xml_node> paragraphs;
    auto paragraph_handle = this->paragraphs();
    while (paragraph_handle.has_next()) {
        paragraphs.push_back(paragraph_handle.current);
        paragraph_handle.next();
    }

    paragraph_revision_range_plan plan;
    std::string range_error;
    if (!build_paragraph_revision_range_plan(
            paragraphs, start_paragraph_index, start_text_offset,
            end_paragraph_index, end_text_offset, plan, range_error)) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       range_error, std::string{document_xml_entry});
        return false;
    }
    if (!split_paragraph_revision_range_segments(plan.segments)) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::operation_not_supported),
                       "text range revision requires plain text runs",
                       std::string{document_xml_entry});
        return false;
    }

    const auto selected_segments =
        collect_selected_paragraph_revision_spans(plan.segments);
    if (selected_segments.size() != plan.segments.size()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "text range is out of range",
                       std::string{document_xml_entry});
        return false;
    }

    long revision_id = 1L;
    collect_revision_ids(this->document, revision_id);
    for (const auto &part : this->header_parts) {
        if (part != nullptr) {
            collect_revision_ids(part->xml, revision_id);
        }
    }
    for (const auto &part : this->footer_parts) {
        if (part != nullptr) {
            collect_revision_ids(part->xml, revision_id);
        }
    }

    auto start_paragraph = paragraphs[start_paragraph_index];
    const auto insertion_before =
        paragraph_revision_insert_before(start_paragraph, start_text_offset);
    const auto style_run =
        paragraph_revision_style_run(start_paragraph, start_text_offset);
    bool inserted_replacement = false;

    for (std::size_t segment_index = 0U; segment_index < plan.segments.size();
         ++segment_index) {
        auto paragraph = plan.segments[segment_index].paragraph;
        const auto &selected = selected_segments[segment_index];
        auto deletion = paragraph.insert_child_before("w:del", selected.front().run);
        if (deletion == pugi::xml_node{} ||
            !append_revision_runs_from_spans(deletion, selected, "w:delText")) {
            if (deletion != pugi::xml_node{}) {
                paragraph.remove_child(deletion);
            }
            set_last_error(this->last_error_info,
                           std::make_error_code(std::errc::not_enough_memory),
                           "failed to replace text range revision",
                           std::string{document_xml_entry});
            return false;
        }
        set_revision_identity_metadata(deletion, revision_id, options.author,
                                       options.date);
        ++revision_id;

        if (!inserted_replacement &&
            plan.segments[segment_index].paragraph == start_paragraph) {
            auto insertion =
                paragraph.insert_child_before("w:ins", selected.front().run);
            if (insertion == pugi::xml_node{} ||
                append_revision_run(insertion, selected.front().run, "w:t", text) ==
                    pugi::xml_node{}) {
                if (insertion != pugi::xml_node{}) {
                    paragraph.remove_child(insertion);
                }
                paragraph.remove_child(deletion);
                set_last_error(this->last_error_info,
                               std::make_error_code(std::errc::not_enough_memory),
                               "failed to replace text range revision",
                               std::string{document_xml_entry});
                return false;
            }
            set_revision_identity_metadata(insertion, revision_id,
                                           options.author, options.date);
            ++revision_id;
            inserted_replacement = true;
        }

        for (const auto &span : selected) {
            if (!paragraph.remove_child(span.run)) {
                paragraph.remove_child(deletion);
                set_last_error(this->last_error_info,
                               std::make_error_code(std::errc::not_enough_memory),
                               "failed to replace text range revision",
                               std::string{document_xml_entry});
                return false;
            }
        }
    }

    if (!inserted_replacement) {
        auto insertion = insert_revision_node(start_paragraph, "w:ins",
                                              insertion_before);
        if (insertion == pugi::xml_node{} ||
            append_revision_run(insertion, style_run, "w:t", text) ==
                pugi::xml_node{}) {
            if (insertion != pugi::xml_node{}) {
                start_paragraph.remove_child(insertion);
            }
            set_last_error(this->last_error_info,
                           std::make_error_code(std::errc::not_enough_memory),
                           "failed to replace text range revision",
                           std::string{document_xml_entry});
            return false;
        }
        set_revision_identity_metadata(insertion, revision_id, options.author,
                                       options.date);
    }

    this->last_error_info.clear();
    return true;
}

std::optional<featherdoc::text_range_preview> Document::preview_text_range(
    std::size_t start_paragraph_index, std::size_t start_text_offset,
    std::size_t end_paragraph_index, std::size_t end_text_offset) const {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before previewing a text range",
                       std::string{document_xml_entry});
        return std::nullopt;
    }

    std::vector<pugi::xml_node> paragraphs;
    auto body =
        const_cast<pugi::xml_document &>(this->document)
            .child("w:document")
            .child("w:body");
    for (auto child = body.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        if (std::string_view{child.name()} == "w:p") {
            paragraphs.push_back(child);
        }
    }

    paragraph_revision_range_plan plan;
    std::string range_error;
    if (!build_paragraph_revision_range_plan(
            paragraphs, start_paragraph_index, start_text_offset,
            end_paragraph_index, end_text_offset, plan, range_error)) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       range_error, std::string{document_xml_entry});
        return std::nullopt;
    }

    auto preview = preview_paragraph_revision_range_segments(
        start_paragraph_index, start_text_offset, end_paragraph_index,
        end_text_offset, plan);
    this->last_error_info.clear();
    return preview;
}

std::vector<featherdoc::text_range_preview> Document::find_text_ranges(
    std::string_view text) const {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before finding text ranges",
                       std::string{document_xml_entry});
        return {};
    }
    if (text.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "search text must not be empty",
                       std::string{document_xml_entry});
        return {};
    }

    struct paragraph_text_range {
        std::size_t paragraph_index{};
        std::size_t global_start{};
        std::size_t text_length{};
    };

    std::vector<pugi::xml_node> paragraphs;
    std::vector<paragraph_text_range> paragraph_ranges;
    std::string document_text;
    auto body =
        const_cast<pugi::xml_document &>(this->document)
            .child("w:document")
            .child("w:body");
    std::size_t paragraph_index = 0U;
    for (auto child = body.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        if (std::string_view{child.name()} != "w:p") {
            continue;
        }

        paragraphs.push_back(child);
        paragraph_text_range range;
        range.paragraph_index = paragraph_index;
        range.global_start = document_text.size();
        for (const auto &span : collect_paragraph_revision_run_spans(child)) {
            document_text += span.text;
        }
        range.text_length = document_text.size() - range.global_start;
        paragraph_ranges.push_back(range);
        ++paragraph_index;
    }

    const auto map_match_start =
        [&paragraph_ranges](std::size_t global_offset, std::size_t &mapped_paragraph,
                            std::size_t &mapped_offset) -> bool {
        if (paragraph_ranges.empty()) {
            return false;
        }
        for (const auto &range : paragraph_ranges) {
            if (range.text_length == 0U) {
                continue;
            }
            const auto range_end = range.global_start + range.text_length;
            if (global_offset < range_end) {
                mapped_paragraph = range.paragraph_index;
                mapped_offset = global_offset - range.global_start;
                return true;
            }
        }

        const auto &last = paragraph_ranges.back();
        mapped_paragraph = last.paragraph_index;
        mapped_offset = last.text_length;
        return true;
    };

    const auto map_match_end =
        [&paragraph_ranges](std::size_t global_offset, std::size_t &mapped_paragraph,
                            std::size_t &mapped_offset) -> bool {
        if (paragraph_ranges.empty()) {
            return false;
        }

        std::optional<paragraph_text_range> previous_text_range;
        for (const auto &range : paragraph_ranges) {
            if (range.text_length == 0U) {
                continue;
            }
            const auto range_end = range.global_start + range.text_length;
            if (global_offset > range.global_start && global_offset <= range_end) {
                mapped_paragraph = range.paragraph_index;
                mapped_offset = global_offset - range.global_start;
                return true;
            }
            if (global_offset == range.global_start &&
                previous_text_range.has_value()) {
                mapped_paragraph = previous_text_range->paragraph_index;
                mapped_offset = previous_text_range->text_length;
                return true;
            }
            previous_text_range = range;
        }

        if (previous_text_range.has_value()) {
            mapped_paragraph = previous_text_range->paragraph_index;
            mapped_offset = previous_text_range->text_length;
            return true;
        }

        const auto &last = paragraph_ranges.back();
        mapped_paragraph = last.paragraph_index;
        mapped_offset = last.text_length;
        return true;
    };

    std::vector<featherdoc::text_range_preview> matches;
    std::size_t search_offset = 0U;
    while (search_offset <= document_text.size()) {
        const auto match_offset = document_text.find(text, search_offset);
        if (match_offset == std::string::npos) {
            break;
        }

        std::size_t start_paragraph_index = 0U;
        std::size_t start_text_offset = 0U;
        std::size_t end_paragraph_index = 0U;
        std::size_t end_text_offset = 0U;
        if (!map_match_start(match_offset, start_paragraph_index,
                             start_text_offset) ||
            !map_match_end(match_offset + text.size(), end_paragraph_index,
                           end_text_offset)) {
            break;
        }

        paragraph_revision_range_plan plan;
        std::string range_error;
        if (!build_paragraph_revision_range_plan(
                paragraphs, start_paragraph_index, start_text_offset,
                end_paragraph_index, end_text_offset, plan, range_error)) {
            set_last_error(this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           range_error, std::string{document_xml_entry});
            return {};
        }

        matches.push_back(preview_paragraph_revision_range_segments(
            start_paragraph_index, start_text_offset, end_paragraph_index,
            end_text_offset, plan));
        search_offset = match_offset + 1U;
    }

    this->last_error_info.clear();
    return matches;
}

std::vector<featherdoc::revision_summary> Document::list_revisions() const {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before listing revisions",
                       std::string{document_xml_entry});
        return {};
    }

    std::vector<featherdoc::revision_summary> summaries;
    collect_revisions_from_node(const_cast<pugi::xml_document &>(this->document),
                                document_xml_entry, summaries);
    for (const auto &part : this->header_parts) {
        if (part != nullptr) {
            collect_revisions_from_node(part->xml, part->entry_name, summaries);
        }
    }
    for (const auto &part : this->footer_parts) {
        if (part != nullptr) {
            collect_revisions_from_node(part->xml, part->entry_name, summaries);
        }
    }

    this->last_error_info.clear();
    return summaries;
}

bool Document::set_revision_metadata(
    std::size_t revision_index,
    const featherdoc::revision_metadata_update &metadata) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before setting revision metadata",
                       std::string{document_xml_entry});
        return false;
    }
    const auto has_change = metadata.author.has_value() ||
                            metadata.date.has_value() ||
                            metadata.clear_author || metadata.clear_date;
    if (!has_change) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "revision metadata update must contain at least one change",
                       std::string{document_xml_entry});
        return false;
    }
    if ((metadata.author.has_value() && metadata.clear_author) ||
        (metadata.date.has_value() && metadata.clear_date)) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "revision metadata cannot be set and cleared in the same update",
                       std::string{document_xml_entry});
        return false;
    }

    std::vector<pugi::xml_node> revision_nodes;
    collect_revision_nodes_in_order(this->document, revision_nodes);
    for (auto &part : this->header_parts) {
        if (part != nullptr) {
            collect_revision_nodes_in_order(part->xml, revision_nodes);
        }
    }
    for (auto &part : this->footer_parts) {
        if (part != nullptr) {
            collect_revision_nodes_in_order(part->xml, revision_nodes);
        }
    }
    if (revision_index >= revision_nodes.size()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "revision index is out of range",
                       std::string{document_xml_entry});
        return false;
    }

    auto revision = revision_nodes[revision_index];
    if (metadata.author.has_value()) {
        ensure_attribute_value(revision, "w:author", *metadata.author);
    } else if (metadata.clear_author) {
        revision.remove_attribute("w:author");
    }
    if (metadata.date.has_value()) {
        ensure_attribute_value(revision, "w:date", *metadata.date);
    } else if (metadata.clear_date) {
        revision.remove_attribute("w:date");
    }

    this->last_error_info.clear();
    return true;
}

bool Document::accept_revision(std::size_t revision_index) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before accepting a revision",
                       std::string{document_xml_entry});
        return false;
    }

    std::vector<pugi::xml_node> revision_nodes;
    collect_revision_nodes_in_order(this->document, revision_nodes);
    for (auto &part : this->header_parts) {
        if (part != nullptr) {
            collect_revision_nodes_in_order(part->xml, revision_nodes);
        }
    }
    for (auto &part : this->footer_parts) {
        if (part != nullptr) {
            collect_revision_nodes_in_order(part->xml, revision_nodes);
        }
    }
    if (revision_index >= revision_nodes.size()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "revision index is out of range",
                       std::string{document_xml_entry});
        return false;
    }
    if (!apply_revision_decision(revision_nodes[revision_index], true)) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::io_error),
                       "failed to accept revision",
                       std::string{document_xml_entry});
        return false;
    }
    this->last_error_info.clear();
    return true;
}

bool Document::reject_revision(std::size_t revision_index) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before rejecting a revision",
                       std::string{document_xml_entry});
        return false;
    }

    std::vector<pugi::xml_node> revision_nodes;
    collect_revision_nodes_in_order(this->document, revision_nodes);
    for (auto &part : this->header_parts) {
        if (part != nullptr) {
            collect_revision_nodes_in_order(part->xml, revision_nodes);
        }
    }
    for (auto &part : this->footer_parts) {
        if (part != nullptr) {
            collect_revision_nodes_in_order(part->xml, revision_nodes);
        }
    }
    if (revision_index >= revision_nodes.size()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "revision index is out of range",
                       std::string{document_xml_entry});
        return false;
    }
    if (!apply_revision_decision(revision_nodes[revision_index], false)) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::io_error),
                       "failed to reject revision",
                       std::string{document_xml_entry});
        return false;
    }
    this->last_error_info.clear();
    return true;
}

std::size_t Document::accept_all_revisions() {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before accepting revisions",
                       std::string{document_xml_entry});
        return 0U;
    }

    std::size_t accepted = 0U;
    for (;;) {
        std::vector<pugi::xml_node> revision_nodes;
        collect_revision_nodes_in_order(this->document, revision_nodes);
        for (auto &part : this->header_parts) {
            if (part != nullptr) {
                collect_revision_nodes_in_order(part->xml, revision_nodes);
            }
        }
        for (auto &part : this->footer_parts) {
            if (part != nullptr) {
                collect_revision_nodes_in_order(part->xml, revision_nodes);
            }
        }
        if (revision_nodes.empty()) {
            break;
        }
        if (!apply_revision_decision(revision_nodes.front(), true)) {
            set_last_error(this->last_error_info,
                           std::make_error_code(std::errc::io_error),
                           "failed to accept revision",
                           std::string{document_xml_entry});
            return accepted;
        }
        ++accepted;
    }

    this->last_error_info.clear();
    return accepted;
}

std::size_t Document::reject_all_revisions() {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before rejecting revisions",
                       std::string{document_xml_entry});
        return 0U;
    }

    std::size_t rejected = 0U;
    for (;;) {
        std::vector<pugi::xml_node> revision_nodes;
        collect_revision_nodes_in_order(this->document, revision_nodes);
        for (auto &part : this->header_parts) {
            if (part != nullptr) {
                collect_revision_nodes_in_order(part->xml, revision_nodes);
            }
        }
        for (auto &part : this->footer_parts) {
            if (part != nullptr) {
                collect_revision_nodes_in_order(part->xml, revision_nodes);
            }
        }
        if (revision_nodes.empty()) {
            break;
        }
        if (!apply_revision_decision(revision_nodes.front(), false)) {
            set_last_error(this->last_error_info,
                           std::make_error_code(std::errc::io_error),
                           "failed to reject revision",
                           std::string{document_xml_entry});
            return rejected;
        }
        ++rejected;
    }

    this->last_error_info.clear();
    return rejected;
}

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

std::size_t Document::replace_content_control_text_by_tag(
    std::string_view tag, std::string_view replacement) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before replacing content control text",
                       std::string{document_xml_entry});
        return 0U;
    }

    return replace_content_control_text_by_tag_or_alias_in_part(
        this->last_error_info, this->document, document_xml_entry, tag,
        replacement, true);
}

std::size_t Document::replace_content_control_text_by_alias(
    std::string_view alias, std::string_view replacement) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before replacing content control text",
                       std::string{document_xml_entry});
        return 0U;
    }

    return replace_content_control_text_by_tag_or_alias_in_part(
        this->last_error_info, this->document, document_xml_entry, alias,
        replacement, false);
}

std::size_t Document::set_content_control_form_state_by_tag(
    std::string_view tag,
    const featherdoc::content_control_form_state_options &options) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before setting content control form state",
                       std::string{document_xml_entry});
        return 0U;
    }

    return set_content_control_form_state_by_tag_or_alias_in_part(
        this->last_error_info, this->document, document_xml_entry, tag, options,
        true);
}

std::size_t Document::set_content_control_form_state_by_alias(
    std::string_view alias,
    const featherdoc::content_control_form_state_options &options) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before setting content control form state",
                       std::string{document_xml_entry});
        return 0U;
    }

    return set_content_control_form_state_by_tag_or_alias_in_part(
        this->last_error_info, this->document, document_xml_entry, alias,
        options, false);
}

std::size_t Document::replace_content_control_with_paragraphs_by_tag(
    std::string_view tag, const std::vector<std::string> &paragraphs) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before replacing content control content",
                       std::string{document_xml_entry});
        return 0U;
    }

    return replace_content_control_with_paragraphs_by_tag_or_alias_in_part(
        this->last_error_info, this->document, document_xml_entry, tag,
        paragraphs, true);
}

std::size_t Document::replace_content_control_with_paragraphs_by_alias(
    std::string_view alias, const std::vector<std::string> &paragraphs) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before replacing content control content",
                       std::string{document_xml_entry});
        return 0U;
    }

    return replace_content_control_with_paragraphs_by_tag_or_alias_in_part(
        this->last_error_info, this->document, document_xml_entry, alias,
        paragraphs, false);
}

std::size_t Document::replace_content_control_with_table_rows_by_tag(
    std::string_view tag, const std::vector<std::vector<std::string>> &rows) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before replacing content control content",
                       std::string{document_xml_entry});
        return 0U;
    }

    return replace_content_control_with_table_rows_by_tag_or_alias_in_part(
        this->last_error_info, this->document, document_xml_entry, tag, rows,
        true);
}

std::size_t Document::replace_content_control_with_table_rows_by_alias(
    std::string_view alias, const std::vector<std::vector<std::string>> &rows) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before replacing content control content",
                       std::string{document_xml_entry});
        return 0U;
    }

    return replace_content_control_with_table_rows_by_tag_or_alias_in_part(
        this->last_error_info, this->document, document_xml_entry, alias, rows,
        false);
}

std::size_t Document::replace_content_control_with_table_by_tag(
    std::string_view tag, const std::vector<std::vector<std::string>> &rows) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before replacing content control content",
                       std::string{document_xml_entry});
        return 0U;
    }

    return replace_content_control_with_table_by_tag_or_alias_in_part(
        this->last_error_info, this->document, document_xml_entry, tag, rows,
        true);
}

std::size_t Document::replace_content_control_with_table_by_alias(
    std::string_view alias, const std::vector<std::vector<std::string>> &rows) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before replacing content control content",
                       std::string{document_xml_entry});
        return 0U;
    }

    return replace_content_control_with_table_by_tag_or_alias_in_part(
        this->last_error_info, this->document, document_xml_entry, alias, rows,
        false);
}

std::size_t Document::replace_content_control_with_image_by_tag(
    std::string_view tag, const std::filesystem::path &image_path) {
    return this->replace_content_control_with_image_in_part(
        this->document, document_xml_entry, tag, true, image_path, std::nullopt);
}

std::size_t Document::replace_content_control_with_image_by_tag(
    std::string_view tag, const std::filesystem::path &image_path,
    std::uint32_t width_px, std::uint32_t height_px) {
    return this->replace_content_control_with_image_in_part(
        this->document, document_xml_entry, tag, true, image_path,
        std::pair<std::uint32_t, std::uint32_t>{width_px, height_px});
}

std::size_t Document::replace_content_control_with_image_by_alias(
    std::string_view alias, const std::filesystem::path &image_path) {
    return this->replace_content_control_with_image_in_part(
        this->document, document_xml_entry, alias, false, image_path, std::nullopt);
}

std::size_t Document::replace_content_control_with_image_by_alias(
    std::string_view alias, const std::filesystem::path &image_path,
    std::uint32_t width_px, std::uint32_t height_px) {
    return this->replace_content_control_with_image_in_part(
        this->document, document_xml_entry, alias, false, image_path,
        std::pair<std::uint32_t, std::uint32_t>{width_px, height_px});
}

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

std::size_t Document::replace_bookmark_with_paragraphs(
    std::string_view bookmark_name, const std::vector<std::string> &paragraphs) {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before replacing a bookmark with paragraphs");
        return 0U;
    }

    return replace_bookmark_with_paragraphs_in_part(
        this->last_error_info, this->document, document_xml_entry, bookmark_name,
        paragraphs);
}

std::size_t Document::replace_bookmark_with_table_rows(
    std::string_view bookmark_name, const std::vector<std::vector<std::string>> &rows) {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before replacing a bookmark with table rows");
        return 0U;
    }

    return replace_bookmark_with_table_rows_in_part(
        this->last_error_info, this->document, document_xml_entry, bookmark_name, rows);
}

std::size_t Document::replace_bookmark_with_table(
    std::string_view bookmark_name, const std::vector<std::vector<std::string>> &rows) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before replacing a bookmark with a table");
        return 0U;
    }

    return replace_bookmark_with_table_in_part(this->last_error_info, this->document,
                                               document_xml_entry, bookmark_name, rows);
}

std::size_t Document::replace_bookmark_with_image_in_part(
    pugi::xml_document &xml_document, std::string_view entry_name,
    std::string_view bookmark_name, const std::filesystem::path &image_path,
    std::optional<std::pair<std::uint32_t, std::uint32_t>> dimensions) {
    featherdoc::detail::image_file_info image_info;
    auto error_code = featherdoc::document_errc::success;
    std::string detail;

    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before replacing a bookmark with an image");
        return 0U;
    }

    if (bookmark_name.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "bookmark name must not be empty", std::string{entry_name});
        return 0U;
    }

    if (dimensions.has_value() &&
        (dimensions->first == 0U || dimensions->second == 0U)) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "image width and height must both be greater than zero",
                       image_path.string());
        return 0U;
    }

    std::vector<block_bookmark_placeholder> placeholders;
    if (!collect_block_bookmark_placeholders(this->last_error_info, xml_document, entry_name,
                                             bookmark_name, placeholders)) {
        return 0U;
    }

    if (placeholders.empty()) {
        this->last_error_info.clear();
        return 0U;
    }

    if (!featherdoc::detail::load_image_file(image_path, image_info, error_code, detail)) {
        set_last_error(this->last_error_info, error_code, std::move(detail),
                       image_path.string());
        return 0U;
    }

    pugi::xml_document *relationships_document = nullptr;
    std::string_view relationships_entry_name;
    bool *has_relationships_part = nullptr;
    bool *relationships_dirty = nullptr;

    if (entry_name == document_xml_entry) {
        relationships_document = &this->document_relationships;
        relationships_entry_name = document_relationships_xml_entry;
        has_relationships_part = &this->has_document_relationships_part;
        relationships_dirty = &this->document_relationships_dirty;
    } else if (auto *part = this->find_related_part_state(entry_name); part != nullptr) {
        relationships_document = &part->relationships;
        relationships_entry_name = part->relationships_entry_name;
        has_relationships_part = &part->has_relationships_part;
        relationships_dirty = &part->relationships_dirty;
    } else {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "template part is not attached to this document",
                       std::string{entry_name});
        return 0U;
    }

    const auto replacement_width =
        dimensions.has_value() ? dimensions->first : image_info.width_px;
    const auto replacement_height =
        dimensions.has_value() ? dimensions->second : image_info.height_px;

    std::size_t replaced = 0U;
    for (const auto &placeholder : placeholders) {
        auto parent = placeholder.paragraph.parent();
        if (!this->append_inline_image_part(
                xml_document, entry_name, *relationships_document,
                relationships_entry_name, *has_relationships_part, *relationships_dirty,
                parent, placeholder.paragraph, std::string{image_info.data},
                std::string{image_info.extension}, std::string{image_info.content_type},
                image_path.filename().string(), replacement_width,
                replacement_height)) {
            return 0U;
        }

        if (!parent.remove_child(placeholder.paragraph)) {
            set_last_error(this->last_error_info,
                           std::make_error_code(std::errc::not_enough_memory),
                           "failed to remove the bookmark placeholder paragraph after image "
                           "replacement",
                           std::string{entry_name});
            return 0U;
        }

        ++replaced;
    }

    this->last_error_info.clear();
    return replaced;
}

std::vector<featherdoc::hyperlink_summary> Document::list_hyperlinks_in_part(
    pugi::xml_document &xml_document, std::string_view entry_name) const {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before listing hyperlinks",
                       std::string{entry_name});
        return {};
    }

    const pugi::xml_document *relationships_document = nullptr;
    if (entry_name == document_xml_entry) {
        relationships_document = &this->document_relationships;
    } else if (const auto *part = this->find_related_part_state(entry_name);
               part != nullptr) {
        relationships_document = &part->relationships;
    } else {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "template part is not attached to this document",
                       std::string{entry_name});
        return {};
    }

    return summarize_hyperlinks_in_part(this->last_error_info, xml_document,
                                        relationships_document);
}

std::size_t Document::append_hyperlink_in_part(
    pugi::xml_document &xml_document, std::string_view entry_name,
    std::string_view text, std::string_view target) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before appending a hyperlink",
                       std::string{entry_name});
        return 0U;
    }
    if (text.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "hyperlink text must not be empty",
                       std::string{entry_name});
        return 0U;
    }
    if (target.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "hyperlink target must not be empty",
                       std::string{entry_name});
        return 0U;
    }

    pugi::xml_document *relationships_document = nullptr;
    std::string_view relationships_entry_name;
    bool *has_relationships_part = nullptr;
    bool *relationships_dirty = nullptr;

    if (entry_name == document_xml_entry) {
        relationships_document = &this->document_relationships;
        relationships_entry_name = document_relationships_xml_entry;
        has_relationships_part = &this->has_document_relationships_part;
        relationships_dirty = &this->document_relationships_dirty;
    } else if (auto *part = this->find_related_part_state(entry_name); part != nullptr) {
        relationships_document = &part->relationships;
        relationships_entry_name = part->relationships_entry_name;
        has_relationships_part = &part->has_relationships_part;
        relationships_dirty = &part->relationships_dirty;
    } else {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "template part is not attached to this document",
                       std::string{entry_name});
        return 0U;
    }

    auto part_root = xml_document.document_element();
    auto container = template_part_block_container(xml_document);
    if (part_root == pugi::xml_node{} || container == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       std::string{entry_name} +
                           " does not contain a valid hyperlink insertion parent",
                       std::string{entry_name});
        return 0U;
    }

    if (relationships_document->child("Relationships") == pugi::xml_node{} &&
        !initialize_empty_relationships_document(*relationships_document)) {
        set_last_error(this->last_error_info,
                       document_errc::relationships_xml_parse_failed,
                       "failed to initialize default relationships XML",
                       std::string{relationships_entry_name});
        return 0U;
    }

    auto relationships = relationships_document->child("Relationships");
    if (relationships == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       document_errc::relationships_xml_parse_failed,
                       std::string{relationships_entry_name} +
                           " does not contain a Relationships root",
                       std::string{relationships_entry_name});
        return 0U;
    }

    auto relationships_namespace = part_root.attribute("xmlns:r");
    if (relationships_namespace == pugi::xml_attribute{}) {
        relationships_namespace = part_root.append_attribute("xmlns:r");
    }
    relationships_namespace.set_value(
        office_document_relationships_namespace_uri.data());

    const auto relationship_id = next_relationship_id(relationships);
    auto relationship = relationships.append_child("Relationship");
    if (relationship == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to append a hyperlink relationship",
                       std::string{relationships_entry_name});
        return 0U;
    }
    ensure_attribute_value(relationship, "Id", relationship_id);
    ensure_attribute_value(relationship, "Type", hyperlink_relationship_type);
    ensure_attribute_value(relationship, "Target", target);
    ensure_attribute_value(relationship, "TargetMode", "External");

    auto paragraph = featherdoc::detail::append_paragraph_node(container);
    auto hyperlink = paragraph.append_child("w:hyperlink");
    auto run = hyperlink.append_child("w:r");
    auto run_properties = run.append_child("w:rPr");
    auto run_style = run_properties.append_child("w:rStyle");
    auto color = run_properties.append_child("w:color");
    auto underline = run_properties.append_child("w:u");
    if (paragraph == pugi::xml_node{} || hyperlink == pugi::xml_node{} ||
        run == pugi::xml_node{} || run_properties == pugi::xml_node{} ||
        run_style == pugi::xml_node{} || color == pugi::xml_node{} ||
        underline == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to append hyperlink XML",
                       std::string{entry_name});
        return 0U;
    }

    ensure_attribute_value(hyperlink, "r:id", relationship_id);
    ensure_attribute_value(hyperlink, "w:history", "1");
    ensure_attribute_value(run_style, "w:val", "Hyperlink");
    ensure_attribute_value(color, "w:val", "0563C1");
    ensure_attribute_value(underline, "w:val", "single");
    if (!featherdoc::detail::set_plain_text_run_content(run, text)) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to append hyperlink text",
                       std::string{entry_name});
        return 0U;
    }

    *has_relationships_part = true;
    *relationships_dirty = true;
    this->last_error_info.clear();
    return 1U;
}

bool Document::ensure_review_notes_part(featherdoc::review_note_kind kind) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before editing review notes",
                       std::string{document_xml_entry});
        return false;
    }
    if (kind != featherdoc::review_note_kind::footnote &&
        kind != featherdoc::review_note_kind::endnote) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "review note kind must be footnote or endnote",
                       std::string{document_xml_entry});
        return false;
    }
    if (const auto error = this->ensure_content_types_loaded()) {
        return false;
    }

    const bool footnote = kind == featherdoc::review_note_kind::footnote;
    auto &notes_document = footnote ? this->footnotes : this->endnotes;
    auto &has_notes_part = footnote ? this->has_footnotes_part : this->has_endnotes_part;
    auto &notes_loaded = footnote ? this->footnotes_loaded : this->endnotes_loaded;
    auto &notes_dirty = footnote ? this->footnotes_dirty : this->endnotes_dirty;
    const auto part_entry = footnote ? footnotes_xml_entry : endnotes_xml_entry;
    const auto relationship_type = footnote ? footnotes_relationship_type
                                            : endnotes_relationship_type;
    const auto relationship_target = footnote ? footnotes_relationship_target
                                              : endnotes_relationship_target;
    const auto content_type = footnote ? footnotes_content_type : endnotes_content_type;
    const char *root_name = footnote ? "w:footnotes" : "w:endnotes";

    if (!notes_loaded) {
        bool loaded_existing_part = false;
        if (this->has_source_archive && !this->document_path.empty() &&
            std::filesystem::exists(this->document_path)) {
            const auto entry_name = related_part_entry_for_type(
                this->document_relationships, relationship_type, part_entry);
            if (!load_optional_docx_xml_part(this->document_path, entry_name,
                                             notes_document, this->last_error_info)) {
                return false;
            }
            loaded_existing_part = notes_document.document_element() != pugi::xml_node{};
        }
        if (!loaded_existing_part && !initialize_review_notes_document(notes_document, kind)) {
            set_last_error(this->last_error_info,
                           std::make_error_code(std::errc::not_enough_memory),
                           "failed to initialize review notes part",
                           std::string{part_entry});
            return false;
        }
        notes_loaded = true;
    }

    auto root = notes_document.child(root_name);
    if (root == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "review notes part has an unsupported root",
                       std::string{part_entry});
        return false;
    }
    ensure_wordprocessingml_namespace(root);

    if (!ensure_document_relationship(this->document_relationships,
                                      this->has_document_relationships_part,
                                      this->document_relationships_dirty,
                                      this->last_error_info, relationship_type,
                                      relationship_target)) {
        return false;
    }
    if (!add_content_type_override(this->content_types, this->content_types_dirty,
                                   this->last_error_info, part_entry, content_type)) {
        return false;
    }

    has_notes_part = true;
    notes_dirty = true;
    this->last_error_info.clear();
    return true;
}

bool Document::ensure_comments_part() {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before editing comments",
                       std::string{document_xml_entry});
        return false;
    }
    if (const auto error = this->ensure_content_types_loaded()) {
        return false;
    }

    if (!this->comments_loaded) {
        bool loaded_existing_part = false;
        if (this->has_source_archive && !this->document_path.empty() &&
            std::filesystem::exists(this->document_path)) {
            const auto entry_name = related_part_entry_for_type(
                this->document_relationships, comments_relationship_type,
                comments_xml_entry);
            if (!load_optional_docx_xml_part(this->document_path, entry_name,
                                             this->comments, this->last_error_info)) {
                return false;
            }
            loaded_existing_part = this->comments.document_element() != pugi::xml_node{};
        }
        if (!loaded_existing_part && !initialize_comments_document(this->comments)) {
            set_last_error(this->last_error_info,
                           std::make_error_code(std::errc::not_enough_memory),
                           "failed to initialize comments part",
                           std::string{comments_xml_entry});
            return false;
        }
        this->comments_loaded = true;
    }

    auto root = this->comments.child("w:comments");
    if (root == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "comments part has an unsupported root",
                       std::string{comments_xml_entry});
        return false;
    }
    ensure_wordprocessingml_namespace(root);

    if (!ensure_document_relationship(this->document_relationships,
                                      this->has_document_relationships_part,
                                      this->document_relationships_dirty,
                                      this->last_error_info, comments_relationship_type,
                                      comments_relationship_target)) {
        return false;
    }
    if (!add_content_type_override(this->content_types, this->content_types_dirty,
                                   this->last_error_info, comments_xml_entry,
                                   comments_content_type)) {
        return false;
    }

    this->has_comments_part = true;
    this->comments_dirty = true;
    this->last_error_info.clear();
    return true;
}

bool Document::ensure_comments_extended_loaded() {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before loading comment metadata",
                       std::string{document_xml_entry});
        return false;
    }
    if (this->comments_extended_loaded) {
        return true;
    }

    this->comments_extended.reset();
    this->has_comments_extended_part = false;
    bool loaded_existing_part = false;
    if (this->has_source_archive && !this->document_path.empty() &&
        std::filesystem::exists(this->document_path)) {
        const auto entry_name = optional_related_part_entry_for_type(
            this->document_relationships, comments_extended_relationship_type);
        if (entry_name.has_value()) {
            if (!load_optional_docx_xml_part(
                    this->document_path, *entry_name, this->comments_extended,
                    this->last_error_info)) {
                return false;
            }
            loaded_existing_part =
                this->comments_extended.document_element() != pugi::xml_node{};
        }
    }

    if (!loaded_existing_part &&
        !initialize_comments_extended_document(this->comments_extended)) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to initialize extended comments part",
                       std::string{comments_extended_xml_entry});
        return false;
    }

    this->comments_extended_loaded = true;
    this->has_comments_extended_part = loaded_existing_part;
    this->last_error_info.clear();
    return true;
}

bool Document::ensure_comments_extended_part() {
    if (const auto error = this->ensure_content_types_loaded()) {
        return false;
    }
    if (!this->ensure_comments_extended_loaded()) {
        return false;
    }

    auto root = this->comments_extended.child("w15:commentsEx");
    if (root == pugi::xml_node{}) {
        if (!initialize_comments_extended_document(this->comments_extended)) {
            set_last_error(this->last_error_info,
                           std::make_error_code(std::errc::not_enough_memory),
                           "failed to initialize extended comments part",
                           std::string{comments_extended_xml_entry});
            return false;
        }
        root = this->comments_extended.child("w15:commentsEx");
    }
    if (root == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "extended comments part has an unsupported root",
                       std::string{comments_extended_xml_entry});
        return false;
    }
    ensure_wordprocessingml_2012_namespace(root);

    if (!ensure_document_relationship(
            this->document_relationships, this->has_document_relationships_part,
            this->document_relationships_dirty, this->last_error_info,
            comments_extended_relationship_type,
            comments_extended_relationship_target)) {
        return false;
    }
    if (!add_content_type_override(this->content_types, this->content_types_dirty,
                                   this->last_error_info,
                                   comments_extended_xml_entry,
                                   comments_extended_content_type)) {
        return false;
    }

    this->has_comments_extended_part = true;
    this->comments_extended_dirty = true;
    this->last_error_info.clear();
    return true;
}

bool Document::replace_hyperlink_in_part(
    pugi::xml_document &xml_document, std::string_view entry_name,
    std::size_t hyperlink_index, std::string_view text,
    std::string_view target) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before replacing a hyperlink",
                       std::string{entry_name});
        return false;
    }
    if (text.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "hyperlink text must not be empty",
                       std::string{entry_name});
        return false;
    }
    if (target.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "hyperlink target must not be empty",
                       std::string{entry_name});
        return false;
    }

    pugi::xml_document *relationships_document = nullptr;
    std::string_view relationships_entry_name;
    bool *has_relationships_part = nullptr;
    bool *relationships_dirty = nullptr;
    if (entry_name == document_xml_entry) {
        relationships_document = &this->document_relationships;
        relationships_entry_name = document_relationships_xml_entry;
        has_relationships_part = &this->has_document_relationships_part;
        relationships_dirty = &this->document_relationships_dirty;
    } else if (auto *part = this->find_related_part_state(entry_name); part != nullptr) {
        relationships_document = &part->relationships;
        relationships_entry_name = part->relationships_entry_name;
        has_relationships_part = &part->has_relationships_part;
        relationships_dirty = &part->relationships_dirty;
    } else {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "template part is not attached to this document",
                       std::string{entry_name});
        return false;
    }

    std::vector<pugi::xml_node> hyperlink_nodes;
    collect_hyperlinks_in_order(xml_document, hyperlink_nodes);
    if (hyperlink_index >= hyperlink_nodes.size()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "hyperlink index is out of range",
                       std::string{entry_name});
        return false;
    }
    if (relationships_document->child("Relationships") == pugi::xml_node{} &&
        !initialize_empty_relationships_document(*relationships_document)) {
        set_last_error(this->last_error_info,
                       document_errc::relationships_xml_parse_failed,
                       "failed to initialize default relationships XML",
                       std::string{relationships_entry_name});
        return false;
    }

    auto relationships = relationships_document->child("Relationships");
    auto hyperlink = hyperlink_nodes[hyperlink_index];
    auto relationship_id = std::string{hyperlink.attribute("r:id").value()};
    auto relationship = relationship_id.empty()
                            ? pugi::xml_node{}
                            : find_relationship_by_id(relationships, relationship_id);
    if (relationship == pugi::xml_node{} ||
        std::string_view{relationship.attribute("Type").value()} !=
            hyperlink_relationship_type) {
        relationship_id = next_relationship_id(relationships);
        relationship = relationships.append_child("Relationship");
        if (relationship == pugi::xml_node{}) {
            set_last_error(this->last_error_info,
                           std::make_error_code(std::errc::not_enough_memory),
                           "failed to append hyperlink relationship",
                           std::string{relationships_entry_name});
            return false;
        }
        ensure_attribute_value(relationship, "Id", relationship_id);
        ensure_attribute_value(hyperlink, "r:id", relationship_id);
    }
    ensure_attribute_value(relationship, "Type", hyperlink_relationship_type);
    ensure_attribute_value(relationship, "Target", target);
    ensure_attribute_value(relationship, "TargetMode", "External");
    ensure_attribute_value(hyperlink, "w:history", "1");

    auto part_root = xml_document.document_element();
    if (part_root != pugi::xml_node{}) {
        auto relationships_namespace = part_root.attribute("xmlns:r");
        if (relationships_namespace == pugi::xml_attribute{}) {
            relationships_namespace = part_root.append_attribute("xmlns:r");
        }
        if (relationships_namespace != pugi::xml_attribute{}) {
            relationships_namespace.set_value(
                office_document_relationships_namespace_uri.data());
        }
    }

    if (!rewrite_hyperlink_plain_text(hyperlink, text)) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to replace hyperlink text",
                       std::string{entry_name});
        return false;
    }

    *has_relationships_part = true;
    *relationships_dirty = true;
    this->last_error_info.clear();
    return true;
}

bool Document::remove_hyperlink_in_part(
    pugi::xml_document &xml_document, std::string_view entry_name,
    std::size_t hyperlink_index) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before removing a hyperlink",
                       std::string{entry_name});
        return false;
    }

    pugi::xml_document *relationships_document = nullptr;
    std::string_view relationships_entry_name;
    bool *relationships_dirty = nullptr;
    if (entry_name == document_xml_entry) {
        relationships_document = &this->document_relationships;
        relationships_entry_name = document_relationships_xml_entry;
        relationships_dirty = &this->document_relationships_dirty;
    } else if (auto *part = this->find_related_part_state(entry_name); part != nullptr) {
        relationships_document = &part->relationships;
        relationships_entry_name = part->relationships_entry_name;
        relationships_dirty = &part->relationships_dirty;
    } else {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "template part is not attached to this document",
                       std::string{entry_name});
        return false;
    }

    std::vector<pugi::xml_node> hyperlink_nodes;
    collect_hyperlinks_in_order(xml_document, hyperlink_nodes);
    if (hyperlink_index >= hyperlink_nodes.size()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "hyperlink index is out of range",
                       std::string{entry_name});
        return false;
    }

    auto hyperlink = hyperlink_nodes[hyperlink_index];
    const auto relationship_id = std::string{hyperlink.attribute("r:id").value()};
    const auto display_text = featherdoc::detail::collect_plain_text_from_xml(hyperlink);
    auto parent = hyperlink.parent();
    if (parent == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "hyperlink does not have a removable parent",
                       std::string{entry_name});
        return false;
    }
    if (!display_text.empty()) {
        auto replacement_run = parent.insert_child_before("w:r", hyperlink);
        if (replacement_run == pugi::xml_node{} ||
            !featherdoc::detail::set_plain_text_run_content(replacement_run,
                                                            display_text)) {
            set_last_error(this->last_error_info,
                           std::make_error_code(std::errc::not_enough_memory),
                           "failed to preserve hyperlink display text",
                           std::string{entry_name});
            return false;
        }
    }
    parent.remove_child(hyperlink);

    auto relationships = relationships_document->child("Relationships");
    if (!relationship_id.empty() && relationships != pugi::xml_node{} &&
        !hyperlink_relationship_id_is_used(xml_document, relationship_id)) {
        auto relationship = find_relationship_by_id(relationships, relationship_id);
        if (relationship != pugi::xml_node{} &&
            std::string_view{relationship.attribute("Type").value()} ==
                hyperlink_relationship_type) {
            relationships.remove_child(relationship);
            *relationships_dirty = true;
        }
    }

    this->last_error_info.clear();
    return true;
}

std::size_t Document::replace_content_control_with_image_in_part(
    pugi::xml_document &xml_document, std::string_view entry_name,
    std::string_view value, bool match_tag,
    const std::filesystem::path &image_path,
    std::optional<std::pair<std::uint32_t, std::uint32_t>> dimensions) {
    featherdoc::detail::image_file_info image_info;
    auto error_code = featherdoc::document_errc::success;
    std::string detail;

    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before replacing a content control with an image");
        return 0U;
    }

    if (value.empty()) {
        set_last_error(
            this->last_error_info, std::make_error_code(std::errc::invalid_argument),
            match_tag ? "content control tag must not be empty"
                      : "content control alias must not be empty",
            std::string{entry_name});
        return 0U;
    }

    if (dimensions.has_value() &&
        (dimensions->first == 0U || dimensions->second == 0U)) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "image width and height must both be greater than zero",
                       image_path.string());
        return 0U;
    }

    const auto matches = filter_content_controls_by_tag_or_alias(
        this->last_error_info, xml_document, entry_name, value, match_tag);
    if (matches.empty()) {
        this->last_error_info.clear();
        return 0U;
    }

    if (!featherdoc::detail::load_image_file(image_path, image_info, error_code, detail)) {
        set_last_error(this->last_error_info, error_code, std::move(detail),
                       image_path.string());
        return 0U;
    }

    pugi::xml_document *relationships_document = nullptr;
    std::string_view relationships_entry_name;
    bool *has_relationships_part = nullptr;
    bool *relationships_dirty = nullptr;

    if (entry_name == document_xml_entry) {
        relationships_document = &this->document_relationships;
        relationships_entry_name = document_relationships_xml_entry;
        has_relationships_part = &this->has_document_relationships_part;
        relationships_dirty = &this->document_relationships_dirty;
    } else if (auto *part = this->find_related_part_state(entry_name); part != nullptr) {
        relationships_document = &part->relationships;
        relationships_entry_name = part->relationships_entry_name;
        has_relationships_part = &part->has_relationships_part;
        relationships_dirty = &part->relationships_dirty;
    } else {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "template part is not attached to this document",
                       std::string{entry_name});
        return 0U;
    }

    const auto replacement_width =
        dimensions.has_value() ? dimensions->first : image_info.width_px;
    const auto replacement_height =
        dimensions.has_value() ? dimensions->second : image_info.height_px;

    std::size_t replaced = 0U;
    const auto rewrite = [&](featherdoc::document_error_info &current_last_error,
                             std::string_view current_entry_name,
                             pugi::xml_node content_control) {
        auto content = ensure_content_control_content_node(content_control);
        if (content == pugi::xml_node{}) {
            set_last_error(current_last_error,
                           std::make_error_code(std::errc::not_enough_memory),
                           "failed to resolve content control content for image replacement",
                           std::string{current_entry_name});
            return false;
        }

        const auto kind = content_control_kind_from_node(content_control);
        if (kind == featherdoc::content_control_kind::table_row) {
            set_content_control_replacement_error(
                current_last_error, current_entry_name,
                "image replacement requires a block, run, or table-cell content control");
            return false;
        }

        clear_node_children(content);

        auto image_parent = pugi::xml_node{};
        switch (kind) {
        case featherdoc::content_control_kind::run: {
            auto run = content.append_child("w:r");
            if (run == pugi::xml_node{}) {
                set_last_error(current_last_error,
                               std::make_error_code(std::errc::not_enough_memory),
                               "failed to append a replacement image run",
                               std::string{current_entry_name});
                return false;
            }
            image_parent = run;
            break;
        }
        case featherdoc::content_control_kind::table_cell: {
            auto cell = content.append_child("w:tc");
            if (cell == pugi::xml_node{}) {
                set_last_error(current_last_error,
                               std::make_error_code(std::errc::not_enough_memory),
                               "failed to append a replacement image cell",
                               std::string{current_entry_name});
                return false;
            }
            image_parent = cell;
            break;
        }
        case featherdoc::content_control_kind::block:
        case featherdoc::content_control_kind::unknown:
            image_parent = content;
            break;
        case featherdoc::content_control_kind::table_row:
            return false;
        }

        if (!this->append_inline_image_part(
                xml_document, current_entry_name, *relationships_document,
                relationships_entry_name, *has_relationships_part, *relationships_dirty,
                image_parent, {}, std::string{image_info.data},
                std::string{image_info.extension}, std::string{image_info.content_type},
                image_path.filename().string(), replacement_width, replacement_height)) {
            return false;
        }

        clear_content_control_placeholder_flag(content_control);
        return true;
    };

    if (!replace_content_control_by_tag_or_alias_in_node(
            this->last_error_info, xml_document, entry_name, value, match_tag,
            replaced, rewrite)) {
        return replaced;
    }

    this->last_error_info.clear();
    return replaced;
}

std::size_t Document::replace_bookmark_with_floating_image_in_part(
    pugi::xml_document &xml_document, std::string_view entry_name,
    std::string_view bookmark_name, const std::filesystem::path &image_path,
    std::optional<std::pair<std::uint32_t, std::uint32_t>> dimensions,
    featherdoc::floating_image_options options) {
    featherdoc::detail::image_file_info image_info;
    auto error_code = featherdoc::document_errc::success;
    std::string detail;

    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before replacing a bookmark with a floating image");
        return 0U;
    }

    if (bookmark_name.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "bookmark name must not be empty", std::string{entry_name});
        return 0U;
    }

    if (dimensions.has_value() &&
        (dimensions->first == 0U || dimensions->second == 0U)) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "image width and height must both be greater than zero",
                       image_path.string());
        return 0U;
    }

    std::vector<block_bookmark_placeholder> placeholders;
    if (!collect_block_bookmark_placeholders(this->last_error_info, xml_document, entry_name,
                                             bookmark_name, placeholders)) {
        return 0U;
    }

    if (placeholders.empty()) {
        this->last_error_info.clear();
        return 0U;
    }

    if (!featherdoc::detail::load_image_file(image_path, image_info, error_code, detail)) {
        set_last_error(this->last_error_info, error_code, std::move(detail),
                       image_path.string());
        return 0U;
    }

    pugi::xml_document *relationships_document = nullptr;
    std::string_view relationships_entry_name;
    bool *has_relationships_part = nullptr;
    bool *relationships_dirty = nullptr;

    if (entry_name == document_xml_entry) {
        relationships_document = &this->document_relationships;
        relationships_entry_name = document_relationships_xml_entry;
        has_relationships_part = &this->has_document_relationships_part;
        relationships_dirty = &this->document_relationships_dirty;
    } else if (auto *part = this->find_related_part_state(entry_name); part != nullptr) {
        relationships_document = &part->relationships;
        relationships_entry_name = part->relationships_entry_name;
        has_relationships_part = &part->has_relationships_part;
        relationships_dirty = &part->relationships_dirty;
    } else {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "template part is not attached to this document",
                       std::string{entry_name});
        return 0U;
    }

    const auto replacement_width =
        dimensions.has_value() ? dimensions->first : image_info.width_px;
    const auto replacement_height =
        dimensions.has_value() ? dimensions->second : image_info.height_px;

    std::size_t replaced = 0U;
    for (const auto &placeholder : placeholders) {
        auto parent = placeholder.paragraph.parent();
        if (!this->append_floating_image_part(
                xml_document, entry_name, *relationships_document,
                relationships_entry_name, *has_relationships_part, *relationships_dirty,
                parent, placeholder.paragraph, std::string{image_info.data},
                std::string{image_info.extension}, std::string{image_info.content_type},
                image_path.filename().string(), replacement_width,
                replacement_height, options)) {
            return 0U;
        }

        if (!parent.remove_child(placeholder.paragraph)) {
            set_last_error(this->last_error_info,
                           std::make_error_code(std::errc::not_enough_memory),
                           "failed to remove the bookmark placeholder paragraph after floating "
                           "image replacement",
                           std::string{entry_name});
            return 0U;
        }

        ++replaced;
    }

    this->last_error_info.clear();
    return replaced;
}

std::size_t Document::replace_bookmark_with_image(
    std::string_view bookmark_name, const std::filesystem::path &image_path) {
    return this->replace_bookmark_with_image_in_part(
        this->document, document_xml_entry, bookmark_name, image_path, std::nullopt);
}

std::size_t Document::replace_bookmark_with_image(
    std::string_view bookmark_name, const std::filesystem::path &image_path,
    std::uint32_t width_px, std::uint32_t height_px) {
    return this->replace_bookmark_with_image_in_part(
        this->document, document_xml_entry, bookmark_name, image_path,
        std::pair<std::uint32_t, std::uint32_t>{width_px, height_px});
}

std::size_t Document::replace_bookmark_with_floating_image(
    std::string_view bookmark_name, const std::filesystem::path &image_path,
    featherdoc::floating_image_options options) {
    return this->replace_bookmark_with_floating_image_in_part(
        this->document, document_xml_entry, bookmark_name, image_path, std::nullopt,
        std::move(options));
}

std::size_t Document::replace_bookmark_with_floating_image(
    std::string_view bookmark_name, const std::filesystem::path &image_path,
    std::uint32_t width_px, std::uint32_t height_px,
    featherdoc::floating_image_options options) {
    return this->replace_bookmark_with_floating_image_in_part(
        this->document, document_xml_entry, bookmark_name, image_path,
        std::pair<std::uint32_t, std::uint32_t>{width_px, height_px},
        std::move(options));
}

std::size_t Document::remove_bookmark_block(std::string_view bookmark_name) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before removing a bookmark block");
        return 0U;
    }

    return remove_bookmark_block_in_part(this->last_error_info, this->document,
                                         document_xml_entry, bookmark_name);
}

std::size_t Document::set_bookmark_block_visibility(std::string_view bookmark_name,
                                                    bool visible) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before changing bookmark block visibility");
        return 0U;
    }

    return set_bookmark_block_visibility_in_part(this->last_error_info, this->document,
                                                 document_xml_entry, bookmark_name, visible);
}

bookmark_block_visibility_result Document::apply_bookmark_block_visibility(
    std::span<const bookmark_block_visibility_binding> bindings) {
    bookmark_block_visibility_result result;
    result.requested = bindings.size();

    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before applying bookmark block visibility");
        return result;
    }

    return apply_bookmark_block_visibility_in_part(this->last_error_info, this->document,
                                                   document_xml_entry, bindings);
}

bookmark_block_visibility_result Document::apply_bookmark_block_visibility(
    std::initializer_list<bookmark_block_visibility_binding> bindings) {
    return this->apply_bookmark_block_visibility(
        std::span<const bookmark_block_visibility_binding>{bindings.begin(),
                                                           bindings.size()});
}

} // namespace featherdoc
