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

auto last_block_paragraph(pugi::xml_node container) -> pugi::xml_node {
    pugi::xml_node paragraph;
    for (auto child = container.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        if (std::string_view{child.name()} == "w:p") {
            paragraph = child;
        }
    }

    return paragraph;
}

auto paragraph_is_effectively_empty(pugi::xml_node paragraph) -> bool {
    if (paragraph == pugi::xml_node{} ||
        std::string_view{paragraph.name()} != "w:p") {
        return false;
    }

    for (auto child = paragraph.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        if (std::string_view{child.name()} != "w:pPr") {
            return false;
        }
    }

    return true;
}

auto select_field_paragraph(pugi::xml_node container) -> pugi::xml_node {
    if (container == pugi::xml_node{}) {
        return {};
    }

    if (auto paragraph = last_block_paragraph(container);
        paragraph_is_effectively_empty(paragraph)) {
        return paragraph;
    }

    return featherdoc::detail::append_paragraph_node(container);
}

auto append_field_result_run(pugi::xml_node field, std::string_view result_text)
    -> bool {
    if (field == pugi::xml_node{}) {
        return false;
    }

    auto run = field.append_child("w:r");
    if (run == pugi::xml_node{}) {
        return false;
    }

    auto run_properties = run.append_child("w:rPr");
    if (run_properties == pugi::xml_node{}) {
        return false;
    }
    if (run_properties.append_child("w:noProof") == pugi::xml_node{}) {
        return false;
    }

    if (result_text.empty()) {
        return true;
    }

    auto text = run.append_child("w:t");
    if (text == pugi::xml_node{}) {
        return false;
    }

    return text.text().set(std::string{result_text}.c_str());
}

bool set_simple_field_on_off_attribute(pugi::xml_node field,
                                       const char *attribute_name,
                                       bool enabled) {
    if (!enabled) {
        return true;
    }

    auto attribute = field.attribute(attribute_name);
    if (attribute == pugi::xml_attribute{}) {
        attribute = field.append_attribute(attribute_name);
    }
    return attribute != pugi::xml_attribute{} && attribute.set_value("true");
}

bool apply_simple_field_state(pugi::xml_node field,
                              featherdoc::field_state_options state) {
    return set_simple_field_on_off_attribute(field, "w:dirty", state.dirty) &&
           set_simple_field_on_off_attribute(field, "w:fldLock", state.locked);
}

auto append_complex_field_char_run(pugi::xml_node paragraph,
                                   const char *field_char_type,
                                   featherdoc::field_state_options state = {},
                                   bool apply_state = false) -> bool {
    if (paragraph == pugi::xml_node{}) {
        return false;
    }

    auto run = paragraph.append_child("w:r");
    if (run == pugi::xml_node{}) {
        return false;
    }

    auto field_char = run.append_child("w:fldChar");
    if (field_char == pugi::xml_node{}) {
        return false;
    }

    auto type_attribute = field_char.append_attribute("w:fldCharType");
    if (type_attribute == pugi::xml_attribute{} ||
        !type_attribute.set_value(field_char_type)) {
        return false;
    }

    return !apply_state || apply_simple_field_state(field_char, state);
}

auto append_complex_field_instruction_run(pugi::xml_node paragraph,
                                          std::string_view instruction) -> bool {
    if (paragraph == pugi::xml_node{} || instruction.empty()) {
        return paragraph != pugi::xml_node{};
    }

    auto run = paragraph.append_child("w:r");
    if (run == pugi::xml_node{}) {
        return false;
    }

    auto instruction_text = run.append_child("w:instrText");
    if (instruction_text == pugi::xml_node{}) {
        return false;
    }

    auto space_attribute = instruction_text.append_attribute("xml:space");
    if (space_attribute == pugi::xml_attribute{} ||
        !space_attribute.set_value("preserve")) {
        return false;
    }

    return instruction_text.text().set(std::string{instruction}.c_str());
}

auto append_complex_field_result_run(pugi::xml_node paragraph,
                                     std::string_view result_text) -> bool {
    if (paragraph == pugi::xml_node{}) {
        return false;
    }
    if (result_text.empty()) {
        return true;
    }

    auto run = paragraph.append_child("w:r");
    if (run == pugi::xml_node{}) {
        return false;
    }

    auto run_properties = run.append_child("w:rPr");
    if (run_properties == pugi::xml_node{} ||
        run_properties.append_child("w:noProof") == pugi::xml_node{}) {
        return false;
    }

    auto text = run.append_child("w:t");
    if (text == pugi::xml_node{}) {
        return false;
    }

    return text.text().set(std::string{result_text}.c_str());
}

auto append_simple_field_run(pugi::xml_node paragraph,
                             std::string_view instruction,
                             std::string_view result_text,
                             featherdoc::field_state_options state) -> bool {
    if (paragraph == pugi::xml_node{}) {
        return false;
    }

    auto field = paragraph.append_child("w:fldSimple");
    if (field == pugi::xml_node{}) {
        return false;
    }

    auto instruction_attribute = field.append_attribute("w:instr");
    if (instruction_attribute == pugi::xml_attribute{}) {
        return false;
    }
    if (!instruction_attribute.set_value(std::string{instruction}.c_str())) {
        return false;
    }
    if (!apply_simple_field_state(field, state)) {
        return false;
    }

    return append_field_result_run(field, result_text);
}

auto trimmed_field_instruction(std::string_view instruction) -> std::string_view {
    auto begin = instruction.begin();
    auto end = instruction.end();
    while (begin != end &&
           std::isspace(static_cast<unsigned char>(*begin)) != 0) {
        ++begin;
    }
    while (begin != end &&
           std::isspace(static_cast<unsigned char>(*(end - 1))) != 0) {
        --end;
    }
    return {begin, end};
}

auto fragment_contains_instruction(
    const featherdoc::complex_field_instruction_fragment &fragment) -> bool {
    if (fragment.kind ==
        featherdoc::complex_field_instruction_fragment_kind::nested_field) {
        return !trimmed_field_instruction(fragment.instruction).empty();
    }
    return !trimmed_field_instruction(fragment.text).empty();
}

auto append_complex_field_sequence(
    pugi::xml_node paragraph,
    std::span<const featherdoc::complex_field_instruction_fragment> fragments,
    std::string_view result_text, featherdoc::field_state_options state) -> bool;

auto append_complex_field_instruction_fragment(
    pugi::xml_node paragraph,
    const featherdoc::complex_field_instruction_fragment &fragment) -> bool {
    if (fragment.kind ==
        featherdoc::complex_field_instruction_fragment_kind::nested_field) {
        featherdoc::complex_field_instruction_fragment nested_text;
        nested_text.kind = featherdoc::complex_field_instruction_fragment_kind::text;
        nested_text.text = fragment.instruction;
        const std::array<featherdoc::complex_field_instruction_fragment, 1U>
            nested_fragments{nested_text};
        return append_complex_field_sequence(
            paragraph, std::span<const featherdoc::complex_field_instruction_fragment>{
                           nested_fragments.data(), nested_fragments.size()},
            fragment.result_text, fragment.state);
    }

    return append_complex_field_instruction_run(paragraph, fragment.text);
}

auto append_complex_field_sequence(
    pugi::xml_node paragraph,
    std::span<const featherdoc::complex_field_instruction_fragment> fragments,
    std::string_view result_text, featherdoc::field_state_options state) -> bool {
    if (!append_complex_field_char_run(paragraph, "begin", state, true)) {
        return false;
    }

    for (const auto &fragment : fragments) {
        if (!append_complex_field_instruction_fragment(paragraph, fragment)) {
            return false;
        }
    }

    return append_complex_field_char_run(paragraph, "separate") &&
           append_complex_field_result_run(paragraph, result_text) &&
           append_complex_field_char_run(paragraph, "end");
}

auto first_field_instruction_token(std::string_view instruction) -> std::string {
    const auto trimmed = trimmed_field_instruction(instruction);
    std::string token;
    for (const auto character : trimmed) {
        if (std::isspace(static_cast<unsigned char>(character)) != 0) {
            break;
        }
        token.push_back(static_cast<char>(
            std::toupper(static_cast<unsigned char>(character))));
    }
    return token;
}

auto field_kind_from_instruction(std::string_view instruction)
    -> featherdoc::field_kind {
    const auto token = first_field_instruction_token(instruction);
    if (token == "PAGE") {
        return featherdoc::field_kind::page;
    }
    if (token == "NUMPAGES") {
        return featherdoc::field_kind::total_pages;
    }
    if (token == "TOC") {
        return featherdoc::field_kind::table_of_contents;
    }
    if (token == "REF") {
        return featherdoc::field_kind::reference;
    }
    if (token == "PAGEREF") {
        return featherdoc::field_kind::page_reference;
    }
    if (token == "STYLEREF") {
        return featherdoc::field_kind::style_reference;
    }
    if (token == "DOCPROPERTY") {
        return featherdoc::field_kind::document_property;
    }
    if (token == "DATE") {
        return featherdoc::field_kind::date;
    }
    if (token == "HYPERLINK") {
        return featherdoc::field_kind::hyperlink;
    }
    if (token == "SEQ") {
        return featherdoc::field_kind::sequence;
    }
    if (token == "INDEX") {
        return featherdoc::field_kind::index;
    }
    if (token == "XE") {
        return featherdoc::field_kind::index_entry;
    }
    return featherdoc::field_kind::custom;
}

auto field_attribute_is_on(pugi::xml_node field, const char *attribute_name)
    -> bool {
    const auto value = std::string_view{field.attribute(attribute_name).value()};
    return value == "1" || value == "true" || value == "on";
}

void collect_replaceable_field_nodes_in_order(pugi::xml_node node,
                                              std::vector<pugi::xml_node> &fields) {
    for (auto child = node.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        const auto name = std::string_view{child.name()};
        if (name == "w:fldSimple") {
            fields.push_back(child);
            continue;
        }
        if (name == "w:r") {
            const auto field_char = child.child("w:fldChar");
            if (field_char != pugi::xml_node{} &&
                std::string_view{field_char.attribute("w:fldCharType").value()} ==
                    "begin") {
                fields.push_back({});
            }
            continue;
        }
        collect_replaceable_field_nodes_in_order(child, fields);
    }
}

auto summarize_field(pugi::xml_node field, std::size_t index,
                     std::size_t depth = 0U) -> featherdoc::field_summary {
    featherdoc::field_summary summary;
    summary.index = index;
    summary.depth = depth;
    summary.instruction = field.attribute("w:instr").value();
    summary.kind = field_kind_from_instruction(summary.instruction);
    summary.result_text = featherdoc::detail::collect_plain_text_from_xml(field);
    summary.dirty = field_attribute_is_on(field, "w:dirty");
    summary.locked = field_attribute_is_on(field, "w:fldLock");
    return summary;
}

struct active_complex_field_summary {
    std::size_t summary_index{};
    bool in_result{false};
};

void append_complex_field_run_text(
    pugi::xml_node run, std::vector<featherdoc::field_summary> &summaries,
    std::vector<active_complex_field_summary> &stack) {
    if (stack.empty()) {
        return;
    }

    auto &active = stack.back();
    auto &summary = summaries[active.summary_index];
    for (auto child = run.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        const auto child_name = std::string_view{child.name()};
        if (child_name == "w:instrText" && !active.in_result) {
            summary.instruction += child.text().get();
        } else if (child_name == "w:t" && active.in_result) {
            summary.result_text += child.text().get();
        }
    }
}

void collect_field_summaries_in_order(
    pugi::xml_node node, std::vector<featherdoc::field_summary> &summaries,
    std::vector<active_complex_field_summary> &stack) {
    for (auto child = node.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        const auto name = std::string_view{child.name()};
        if (name == "w:fldSimple") {
            summaries.push_back(summarize_field(child, summaries.size(), stack.size()));
            continue;
        }

        if (name == "w:r") {
            const auto field_char = child.child("w:fldChar");
            if (field_char != pugi::xml_node{}) {
                const auto field_char_type =
                    std::string_view{field_char.attribute("w:fldCharType").value()};
                if (field_char_type == "begin") {
                    featherdoc::field_summary summary;
                    summary.index = summaries.size();
                    summary.complex = true;
                    summary.depth = stack.size();
                    summary.dirty = field_attribute_is_on(field_char, "w:dirty");
                    summary.locked = field_attribute_is_on(field_char, "w:fldLock");
                    summaries.push_back(std::move(summary));
                    stack.push_back({summaries.size() - 1U, false});
                } else if (field_char_type == "separate") {
                    if (!stack.empty()) {
                        stack.back().in_result = true;
                    }
                } else if (field_char_type == "end") {
                    if (!stack.empty()) {
                        auto &summary = summaries[stack.back().summary_index];
                        summary.kind = field_kind_from_instruction(summary.instruction);
                        stack.pop_back();
                    }
                }
                continue;
            }

            append_complex_field_run_text(child, summaries, stack);
            continue;
        }

        collect_field_summaries_in_order(child, summaries, stack);
    }
}

auto summarize_fields_in_part(featherdoc::document_error_info &last_error_info,
                              pugi::xml_document &document)
    -> std::vector<featherdoc::field_summary> {
    std::vector<featherdoc::field_summary> summaries;
    std::vector<active_complex_field_summary> stack;
    collect_field_summaries_in_order(document, summaries, stack);
    for (std::size_t index = 0U; index < summaries.size(); ++index) {
        summaries[index].index = index;
        summaries[index].kind = field_kind_from_instruction(summaries[index].instruction);
    }

    last_error_info.clear();
    return summaries;
}

auto omml_node_xml(pugi::xml_node node) -> std::string {
    string_xml_writer writer;
    node.print(writer, "", pugi::format_raw);
    return writer.text;
}

auto omml_root_is_supported(std::string_view name) -> bool {
    return name == "m:oMath" || name == "m:oMathPara";
}

void collect_omml_text_from_node(pugi::xml_node node, std::string &text) {
    for (auto child = node.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        const auto name = std::string_view{child.name()};
        if (name == "m:t" || name == "w:t") {
            text += child.text().get();
            continue;
        }
        collect_omml_text_from_node(child, text);
    }
}

void collect_omml_nodes_in_order(pugi::xml_node node,
                                 std::vector<pugi::xml_node> &nodes) {
    for (auto child = node.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        const auto name = std::string_view{child.name()};
        if (name == "m:oMathPara" || name == "m:oMath") {
            nodes.push_back(child);
            continue;
        }
        collect_omml_nodes_in_order(child, nodes);
    }
}

auto summarize_omml_node(pugi::xml_node node, std::size_t index)
    -> featherdoc::omml_summary {
    featherdoc::omml_summary summary;
    summary.index = index;
    summary.display = std::string_view{node.name()} == "m:oMathPara";
    collect_omml_text_from_node(node, summary.text);
    summary.xml = omml_node_xml(node);
    return summary;
}

auto summarize_omml_in_part(featherdoc::document_error_info &last_error_info,
                            pugi::xml_document &document)
    -> std::vector<featherdoc::omml_summary> {
    std::vector<pugi::xml_node> omml_nodes;
    collect_omml_nodes_in_order(document, omml_nodes);

    std::vector<featherdoc::omml_summary> summaries;
    summaries.reserve(omml_nodes.size());
    for (std::size_t index = 0; index < omml_nodes.size(); ++index) {
        summaries.push_back(summarize_omml_node(omml_nodes[index], index));
    }

    last_error_info.clear();
    return summaries;
}

auto parse_omml_fragment(featherdoc::document_error_info &last_error_info,
                         std::string_view entry_name,
                         std::string_view omml_xml,
                         pugi::xml_document &fragment) -> pugi::xml_node {
    if (omml_xml.empty()) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "OMML XML must not be empty", std::string{entry_name});
        return {};
    }

    const auto parse_result = fragment.load_buffer(omml_xml.data(), omml_xml.size());
    if (!parse_result) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       parse_result.description(), std::string{entry_name},
                       parse_result.offset >= 0
                           ? std::optional<std::ptrdiff_t>{parse_result.offset}
                           : std::nullopt);
        return {};
    }

    const auto root = fragment.document_element();
    if (root == pugi::xml_node{} || !omml_root_is_supported(root.name())) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "OMML XML root must be m:oMath or m:oMathPara",
                       std::string{entry_name});
        return {};
    }

    return root;
}

void ensure_omml_namespace(pugi::xml_document &document) {
    auto root = document.document_element();
    if (root == pugi::xml_node{}) {
        return;
    }

    auto math_namespace = root.attribute("xmlns:m");
    if (math_namespace == pugi::xml_attribute{}) {
        math_namespace = root.append_attribute("xmlns:m");
    }
    if (math_namespace != pugi::xml_attribute{}) {
        math_namespace.set_value(std::string{math_namespace_uri}.c_str());
    }
}

auto append_omml_to_part(featherdoc::document_error_info &last_error_info,
                         pugi::xml_document &document,
                         std::string_view entry_name,
                         std::string_view omml_xml) -> bool {
    pugi::xml_document fragment;
    const auto omml_root = parse_omml_fragment(last_error_info, entry_name,
                                               omml_xml, fragment);
    if (omml_root == pugi::xml_node{}) {
        return false;
    }

    const auto container = template_part_block_container(document);
    if (container == pugi::xml_node{}) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "template part does not contain a supported block container",
                       std::string{entry_name});
        return false;
    }

    ensure_omml_namespace(document);
    auto paragraph = featherdoc::detail::append_paragraph_node(container);
    if (paragraph == pugi::xml_node{} ||
        paragraph.append_copy(omml_root) == pugi::xml_node{}) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       std::string{std::string_view{omml_root.name()} == "m:oMathPara"
                                       ? "failed to append display OMML to template part"
                                       : "failed to append inline OMML to template part"},
                       std::string{entry_name});
        return false;
    }

    last_error_info.clear();
    return true;
}

auto replacement_omml_node(featherdoc::document_error_info &last_error_info,
                           std::string_view entry_name,
                           pugi::xml_node target,
                           pugi::xml_node replacement_root,
                           pugi::xml_document &wrapped_document)
    -> pugi::xml_node {
    const auto target_name = std::string_view{target.name()};
    const auto replacement_name = std::string_view{replacement_root.name()};
    if (target_name == replacement_name) {
        return replacement_root;
    }

    if (target_name == "m:oMathPara" && replacement_name == "m:oMath") {
        auto wrapper = wrapped_document.append_child("m:oMathPara");
        if (wrapper == pugi::xml_node{} ||
            wrapper.append_copy(replacement_root) == pugi::xml_node{}) {
            set_last_error(last_error_info,
                           std::make_error_code(std::errc::not_enough_memory),
                           "failed to wrap inline OMML for display replacement",
                           std::string{entry_name});
            return {};
        }
        return wrapper;
    }

    set_last_error(last_error_info,
                   std::make_error_code(std::errc::invalid_argument),
                   "display OMML cannot replace an inline OMML target",
                   std::string{entry_name});
    return {};
}

auto replace_omml_in_part(featherdoc::document_error_info &last_error_info,
                          pugi::xml_document &document,
                          std::string_view entry_name,
                          std::size_t omml_index,
                          std::string_view omml_xml) -> bool {
    std::vector<pugi::xml_node> omml_nodes;
    collect_omml_nodes_in_order(document, omml_nodes);
    if (omml_index >= omml_nodes.size()) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "OMML index is out of range", std::string{entry_name});
        return false;
    }

    pugi::xml_document fragment;
    const auto omml_root = parse_omml_fragment(last_error_info, entry_name,
                                               omml_xml, fragment);
    if (omml_root == pugi::xml_node{}) {
        return false;
    }

    pugi::xml_document wrapped_document;
    auto target = omml_nodes[omml_index];
    const auto replacement = replacement_omml_node(
        last_error_info, entry_name, target, omml_root, wrapped_document);
    if (replacement == pugi::xml_node{}) {
        return false;
    }

    auto parent = target.parent();
    if (parent == pugi::xml_node{}) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to replace OMML node", std::string{entry_name});
        return false;
    }

    if (std::string_view{target.name()} == "m:oMathPara" &&
        std::string_view{parent.name()} != "w:p") {
        auto paragraph = featherdoc::detail::insert_paragraph_node(parent, target);
        if (paragraph == pugi::xml_node{} ||
            paragraph.append_copy(replacement) == pugi::xml_node{} ||
            !parent.remove_child(target)) {
            set_last_error(last_error_info,
                           std::make_error_code(std::errc::not_enough_memory),
                           "failed to replace OMML node", std::string{entry_name});
            return false;
        }
    } else if (parent.insert_copy_before(replacement, target) == pugi::xml_node{} ||
               !parent.remove_child(target)) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to replace OMML node", std::string{entry_name});
        return false;
    }

    ensure_omml_namespace(document);
    last_error_info.clear();
    return true;
}

auto remove_omml_in_part(featherdoc::document_error_info &last_error_info,
                         pugi::xml_document &document,
                         std::string_view entry_name,
                         std::size_t omml_index) -> bool {
    std::vector<pugi::xml_node> omml_nodes;
    collect_omml_nodes_in_order(document, omml_nodes);
    if (omml_index >= omml_nodes.size()) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "OMML index is out of range", std::string{entry_name});
        return false;
    }

    auto target = omml_nodes[omml_index];
    auto parent = target.parent();
    if (parent == pugi::xml_node{} || !parent.remove_child(target)) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to remove OMML node", std::string{entry_name});
        return false;
    }

    last_error_info.clear();
    return true;
}

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

    bool first_row = true;
    for (auto row = table_handle.rows(); row.has_next(); row.next()) {
        ++summary.row_count;

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
    summary.width_twips = cell_handle.width_twips();
    summary.vertical_alignment = cell_handle.vertical_alignment();
    summary.text_direction = cell_handle.text_direction();
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

struct block_bookmark_placeholder final {
    pugi::xml_node paragraph;
    pugi::xml_node bookmark_start;
    pugi::xml_node bookmark_end;
};

struct table_row_bookmark_placeholder final {
    pugi::xml_node table;
    pugi::xml_node row;
    std::size_t cell_count;
};

struct bookmark_block_placeholder final {
    pugi::xml_node container;
    pugi::xml_node start_paragraph;
    pugi::xml_node end_paragraph;
};

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

auto field_identifier_is_valid(std::string_view value) -> bool {
    if (value.empty()) {
        return false;
    }

    for (const auto character : value) {
        const auto byte = static_cast<unsigned char>(character);
        if (std::isspace(byte) != 0 || character == '"' || character == '\\') {
            return false;
        }
    }
    return true;
}

auto field_argument_is_valid(std::string_view value) -> bool {
    if (value.empty()) {
        return false;
    }

    for (const auto character : value) {
        const auto byte = static_cast<unsigned char>(character);
        if (std::iscntrl(byte) != 0 || character == '"' || character == '\\') {
            return false;
        }
    }
    return true;
}

void append_field_argument(std::ostringstream &instruction,
                           std::string_view value) {
    const auto requires_quotes = std::any_of(value.begin(), value.end(), [](char character) {
        return std::isspace(static_cast<unsigned char>(character)) != 0;
    });
    if (requires_quotes) {
        instruction << '"' << value << '"';
        return;
    }
    instruction << value;
}

void append_quoted_field_argument(std::ostringstream &instruction,
                                  std::string_view value) {
    instruction << '"' << value << '"';
}

auto append_field_to_part(featherdoc::document_error_info &last_error_info,
                          pugi::xml_document &document,
                          std::string_view entry_name,
                          std::string_view instruction,
                          std::string_view result_text,
                          featherdoc::field_state_options state) -> bool {
    if (trimmed_field_instruction(instruction).empty()) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "field instruction must not be empty",
                       std::string{entry_name});
        return false;
    }

    const auto container = template_part_block_container(document);
    if (container == pugi::xml_node{}) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "template part does not contain a supported block container",
                       std::string{entry_name});
        return false;
    }

    const auto paragraph = select_field_paragraph(container);
    if (paragraph == pugi::xml_node{} ||
        !append_simple_field_run(paragraph, instruction, result_text, state)) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::io_error),
                       "failed to append field to template part",
                       std::string{entry_name});
        return false;
    }

    last_error_info.clear();
    return true;
}

auto append_complex_field_to_part(
    featherdoc::document_error_info &last_error_info, pugi::xml_document &document,
    std::string_view entry_name,
    std::span<const featherdoc::complex_field_instruction_fragment> fragments,
    std::string_view result_text, featherdoc::complex_field_options options) -> bool {
    if (fragments.empty()) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "complex field instruction must not be empty",
                       std::string{entry_name});
        return false;
    }

    const auto has_instruction = std::any_of(
        fragments.begin(), fragments.end(), fragment_contains_instruction);
    if (!has_instruction) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "complex field instruction must not be empty",
                       std::string{entry_name});
        return false;
    }
    for (const auto &fragment : fragments) {
        if (fragment.kind ==
                featherdoc::complex_field_instruction_fragment_kind::nested_field &&
            trimmed_field_instruction(fragment.instruction).empty()) {
            set_last_error(last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           "nested field instruction must not be empty",
                           std::string{entry_name});
            return false;
        }
    }

    const auto container = template_part_block_container(document);
    if (container == pugi::xml_node{}) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "template part does not contain a supported block container",
                       std::string{entry_name});
        return false;
    }

    const auto paragraph = select_field_paragraph(container);
    if (paragraph == pugi::xml_node{} ||
        !append_complex_field_sequence(paragraph, fragments, result_text,
                                       options.state)) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::io_error),
                       "failed to append complex field to template part",
                       std::string{entry_name});
        return false;
    }

    last_error_info.clear();
    return true;
}

auto replace_field_in_part(featherdoc::document_error_info &last_error_info,
                           pugi::xml_document &document,
                           std::string_view entry_name,
                           std::size_t field_index,
                           std::string_view instruction,
                           std::string_view result_text) -> bool {
    if (trimmed_field_instruction(instruction).empty()) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "field instruction must not be empty",
                       std::string{entry_name});
        return false;
    }

    std::vector<pugi::xml_node> field_nodes;
    collect_replaceable_field_nodes_in_order(document, field_nodes);
    if (field_index >= field_nodes.size()) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "field index is out of range",
                       std::string{entry_name});
        return false;
    }

    auto field = field_nodes[field_index];
    if (field == pugi::xml_node{}) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "field index refers to a complex field",
                       std::string{entry_name});
        return false;
    }
    auto instruction_attribute = field.attribute("w:instr");
    if (instruction_attribute == pugi::xml_attribute{}) {
        instruction_attribute = field.append_attribute("w:instr");
    }
    if (instruction_attribute == pugi::xml_attribute{} ||
        !instruction_attribute.set_value(std::string{instruction}.c_str())) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::io_error),
                       "failed to rewrite field instruction",
                       std::string{entry_name});
        return false;
    }

    for (auto child = field.first_child(); child != pugi::xml_node{};) {
        const auto next = child.next_sibling();
        field.remove_child(child);
        child = next;
    }
    if (!append_field_result_run(field, result_text)) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::io_error),
                       "failed to rewrite field result text",
                       std::string{entry_name});
        return false;
    }

    last_error_info.clear();
    return true;
}

auto build_table_of_contents_instruction(
    featherdoc::table_of_contents_field_options options,
    featherdoc::document_error_info &last_error_info,
    std::string_view entry_name) -> std::optional<std::string> {
    if (options.min_outline_level == 0U || options.max_outline_level == 0U ||
        options.min_outline_level > options.max_outline_level ||
        options.max_outline_level > 9U) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "TOC outline levels must be between 1 and 9 and ordered",
                       std::string{entry_name});
        return std::nullopt;
    }

    std::ostringstream instruction;
    instruction << " TOC \\o \"" << options.min_outline_level << '-'
                << options.max_outline_level << "\"";
    if (options.hyperlinks) {
        instruction << " \\h";
    }
    if (options.hide_page_numbers_in_web_layout) {
        instruction << " \\z";
    }
    if (options.use_outline_levels) {
        instruction << " \\u";
    }
    instruction << ' ';
    return instruction.str();
}

auto build_reference_instruction(
    std::string_view bookmark_name,
    featherdoc::reference_field_options options,
    featherdoc::document_error_info &last_error_info,
    std::string_view entry_name) -> std::optional<std::string> {
    if (!field_identifier_is_valid(bookmark_name)) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "REF bookmark name must not be empty or contain whitespace, quotes, or backslashes",
                       std::string{entry_name});
        return std::nullopt;
    }

    std::ostringstream instruction;
    instruction << " REF " << bookmark_name;
    if (options.hyperlink) {
        instruction << " \\h";
    }
    if (options.preserve_formatting) {
        instruction << " \\* MERGEFORMAT";
    }
    instruction << ' ';
    return instruction.str();
}

auto build_page_reference_instruction(
    std::string_view bookmark_name,
    featherdoc::page_reference_field_options options,
    featherdoc::document_error_info &last_error_info,
    std::string_view entry_name) -> std::optional<std::string> {
    if (!field_identifier_is_valid(bookmark_name)) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "PAGEREF bookmark name must not be empty or contain whitespace, quotes, or backslashes",
                       std::string{entry_name});
        return std::nullopt;
    }

    std::ostringstream instruction;
    instruction << " PAGEREF " << bookmark_name;
    if (options.hyperlink) {
        instruction << " \\h";
    }
    if (options.relative_position) {
        instruction << " \\p";
    }
    if (options.preserve_formatting) {
        instruction << " \\* MERGEFORMAT";
    }
    instruction << ' ';
    return instruction.str();
}

auto build_style_reference_instruction(
    std::string_view style_name,
    featherdoc::style_reference_field_options options,
    featherdoc::document_error_info &last_error_info,
    std::string_view entry_name) -> std::optional<std::string> {
    if (!field_argument_is_valid(style_name)) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "STYLEREF style name must not be empty or contain quotes, backslashes, or control characters",
                       std::string{entry_name});
        return std::nullopt;
    }

    std::ostringstream instruction;
    instruction << " STYLEREF ";
    append_field_argument(instruction, style_name);
    if (options.paragraph_number) {
        instruction << " \\n";
    }
    if (options.relative_position) {
        instruction << " \\p";
    }
    if (options.preserve_formatting) {
        instruction << " \\* MERGEFORMAT";
    }
    instruction << ' ';
    return instruction.str();
}

auto build_document_property_instruction(
    std::string_view property_name,
    featherdoc::document_property_field_options options,
    featherdoc::document_error_info &last_error_info,
    std::string_view entry_name) -> std::optional<std::string> {
    if (!field_argument_is_valid(property_name)) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "DOCPROPERTY property name must not be empty or contain quotes, backslashes, or control characters",
                       std::string{entry_name});
        return std::nullopt;
    }

    std::ostringstream instruction;
    instruction << " DOCPROPERTY ";
    append_field_argument(instruction, property_name);
    if (options.preserve_formatting) {
        instruction << " \\* MERGEFORMAT";
    }
    instruction << ' ';
    return instruction.str();
}

auto build_date_instruction(featherdoc::date_field_options options,
                            featherdoc::document_error_info &last_error_info,
                            std::string_view entry_name)
    -> std::optional<std::string> {
    if (!options.format.empty() && !field_argument_is_valid(options.format)) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "DATE format must not contain quotes, backslashes, or control characters",
                       std::string{entry_name});
        return std::nullopt;
    }

    std::ostringstream instruction;
    instruction << " DATE";
    if (!options.format.empty()) {
        instruction << " \\@ ";
        append_quoted_field_argument(instruction, options.format);
    }
    if (options.preserve_formatting) {
        instruction << " \\* MERGEFORMAT";
    }
    instruction << ' ';
    return instruction.str();
}

auto build_hyperlink_instruction(
    std::string_view target,
    featherdoc::hyperlink_field_options options,
    featherdoc::document_error_info &last_error_info,
    std::string_view entry_name) -> std::optional<std::string> {
    if (!field_argument_is_valid(target)) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "HYPERLINK target must not be empty or contain quotes, backslashes, or control characters",
                       std::string{entry_name});
        return std::nullopt;
    }
    if (options.anchor.has_value() &&
        !field_argument_is_valid(*options.anchor)) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "HYPERLINK anchor must not be empty or contain quotes, backslashes, or control characters",
                       std::string{entry_name});
        return std::nullopt;
    }
    if (options.tooltip.has_value() &&
        !field_argument_is_valid(*options.tooltip)) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "HYPERLINK tooltip must not be empty or contain quotes, backslashes, or control characters",
                       std::string{entry_name});
        return std::nullopt;
    }

    std::ostringstream instruction;
    instruction << " HYPERLINK ";
    append_quoted_field_argument(instruction, target);
    if (options.anchor.has_value()) {
        instruction << " \\l ";
        append_quoted_field_argument(instruction, *options.anchor);
    }
    if (options.tooltip.has_value()) {
        instruction << " \\o ";
        append_quoted_field_argument(instruction, *options.tooltip);
    }
    if (options.preserve_formatting) {
        instruction << " \\* MERGEFORMAT";
    }
    instruction << ' ';
    return instruction.str();
}

auto build_sequence_instruction(
    std::string_view identifier,
    const featherdoc::sequence_field_options &options,
    featherdoc::document_error_info &last_error_info,
    std::string_view entry_name) -> std::optional<std::string> {
    if (!field_identifier_is_valid(identifier)) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "SEQ identifier must not be empty or contain whitespace, quotes, or backslashes",
                       std::string{entry_name});
        return std::nullopt;
    }
    if (!field_identifier_is_valid(options.number_format)) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "SEQ number format must not be empty or contain whitespace, quotes, or backslashes",
                       std::string{entry_name});
        return std::nullopt;
    }

    std::ostringstream instruction;
    instruction << " SEQ " << identifier << " \\* " << options.number_format;
    if (options.restart_value.has_value()) {
        instruction << " \\r " << *options.restart_value;
    }
    if (options.preserve_formatting) {
        instruction << " \\* MERGEFORMAT";
    }
    instruction << ' ';
    return instruction.str();
}


auto build_caption_sequence_instruction(
    std::string_view label,
    const featherdoc::caption_field_options &options,
    featherdoc::document_error_info &last_error_info,
    std::string_view entry_name) -> std::optional<std::string> {
    auto sequence_options = featherdoc::sequence_field_options{};
    sequence_options.number_format = options.number_format;
    sequence_options.restart_value = options.restart_value;
    sequence_options.preserve_formatting = options.preserve_formatting;
    sequence_options.state = options.state;
    return build_sequence_instruction(label, sequence_options, last_error_info,
                                      entry_name);
}

auto build_index_instruction(featherdoc::index_field_options options,
                             featherdoc::document_error_info &last_error_info,
                             std::string_view entry_name)
    -> std::optional<std::string> {
    if (options.columns.has_value() && *options.columns == 0U) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "INDEX columns must be greater than zero",
                       std::string{entry_name});
        return std::nullopt;
    }

    std::ostringstream instruction;
    instruction << " INDEX";
    if (options.columns.has_value()) {
        instruction << " \\c \"" << *options.columns << "\"";
    }
    if (options.preserve_formatting) {
        instruction << " \\* MERGEFORMAT";
    }
    instruction << ' ';
    return instruction.str();
}

auto build_index_entry_instruction(
    std::string_view entry_text,
    const featherdoc::index_entry_field_options &options,
    featherdoc::document_error_info &last_error_info,
    std::string_view entry_name) -> std::optional<std::string> {
    if (!field_argument_is_valid(entry_text)) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "XE entry text must not be empty or contain quotes, backslashes, or control characters",
                       std::string{entry_name});
        return std::nullopt;
    }
    if (options.subentry.has_value() &&
        !field_argument_is_valid(*options.subentry)) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "XE subentry text must not be empty or contain quotes, backslashes, or control characters",
                       std::string{entry_name});
        return std::nullopt;
    }
    if (options.bookmark_name.has_value() &&
        !field_identifier_is_valid(*options.bookmark_name)) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "XE bookmark name must not be empty or contain whitespace, quotes, or backslashes",
                       std::string{entry_name});
        return std::nullopt;
    }
    if (options.cross_reference.has_value() &&
        !field_argument_is_valid(*options.cross_reference)) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "XE cross reference must not be empty or contain quotes, backslashes, or control characters",
                       std::string{entry_name});
        return std::nullopt;
    }

    std::string full_entry{entry_text};
    if (options.subentry.has_value()) {
        full_entry += ':';
        full_entry += *options.subentry;
    }

    std::ostringstream instruction;
    instruction << " XE ";
    append_quoted_field_argument(instruction, full_entry);
    if (options.bookmark_name.has_value()) {
        instruction << " \\r " << *options.bookmark_name;
    }
    if (options.cross_reference.has_value()) {
        instruction << " \\t ";
        append_quoted_field_argument(instruction, *options.cross_reference);
    }
    if (options.bold_page_number) {
        instruction << " \\b";
    }
    if (options.italic_page_number) {
        instruction << " \\i";
    }
    instruction << ' ';
    return instruction.str();
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

auto summarize_notes_from_part(pugi::xml_document &xml_document,
                               featherdoc::review_note_kind kind)
    -> std::vector<featherdoc::review_note_summary> {
    std::vector<featherdoc::review_note_summary> summaries;
    const char *node_name = kind == featherdoc::review_note_kind::footnote
                                ? "w:footnote"
                                : "w:endnote";
    const auto root = kind == featherdoc::review_note_kind::footnote
                          ? xml_document.child("w:footnotes")
                          : xml_document.child("w:endnotes");
    for (auto note = root.child(node_name); note != pugi::xml_node{};
         note = note.next_sibling(node_name)) {
        const auto type = std::string_view{note.attribute("w:type").value()};
        if (type == "separator" || type == "continuationSeparator") {
            continue;
        }
        featherdoc::review_note_summary summary;
        summary.index = summaries.size();
        summary.kind = kind;
        summary.id = note.attribute("w:id").value();
        summary.text = featherdoc::detail::collect_plain_text_from_xml(note);
        summaries.push_back(std::move(summary));
    }
    return summaries;
}

auto summarize_comments_from_part(pugi::xml_document &xml_document)
    -> std::vector<featherdoc::review_note_summary> {
    std::vector<featherdoc::review_note_summary> summaries;
    const auto root = xml_document.child("w:comments");
    for (auto comment = root.child("w:comment"); comment != pugi::xml_node{};
         comment = comment.next_sibling("w:comment")) {
        featherdoc::review_note_summary summary;
        summary.index = summaries.size();
        summary.kind = featherdoc::review_note_kind::comment;
        summary.id = comment.attribute("w:id").value();
        summary.author = xml_child_attribute_string(comment, "w:author");
        summary.initials = xml_child_attribute_string(comment, "w:initials");
        summary.date = xml_child_attribute_string(comment, "w:date");
        summary.text = featherdoc::detail::collect_plain_text_from_xml(comment);
        summaries.push_back(std::move(summary));
    }
    return summaries;
}

void collect_comment_anchor_text_by_id(
    pugi::xml_node node,
    std::unordered_map<std::string, std::string> &anchor_text_by_id,
    std::optional<std::string> &active_comment_id) {
    for (auto child = node.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        const auto name = std::string_view{child.name()};
        if (name == "w:commentRangeStart") {
            const auto id = std::string{child.attribute("w:id").value()};
            if (!id.empty()) {
                active_comment_id = id;
                anchor_text_by_id.try_emplace(id, std::string{});
            }
            continue;
        }

        if (name == "w:commentRangeEnd") {
            const auto id = std::string_view{child.attribute("w:id").value()};
            if (active_comment_id.has_value() && *active_comment_id == id) {
                active_comment_id.reset();
            }
            continue;
        }

        if (active_comment_id.has_value()) {
            if (name == "w:t" || name == "w:delText") {
                anchor_text_by_id[*active_comment_id] += child.text().get();
            } else if (name == "w:tab") {
                anchor_text_by_id[*active_comment_id].push_back('\t');
            } else if (name == "w:br" || name == "w:cr") {
                anchor_text_by_id[*active_comment_id].push_back('\n');
            }
        }

        collect_comment_anchor_text_by_id(child, anchor_text_by_id,
                                          active_comment_id);
    }
}

auto comment_anchor_text_by_id_from_part(pugi::xml_node root)
    -> std::unordered_map<std::string, std::string> {
    std::unordered_map<std::string, std::string> anchor_text_by_id;
    std::optional<std::string> active_comment_id;
    collect_comment_anchor_text_by_id(root, anchor_text_by_id, active_comment_id);
    return anchor_text_by_id;
}

void merge_comment_anchor_text_by_id(
    std::unordered_map<std::string, std::string> &target, pugi::xml_node root) {
    auto source = comment_anchor_text_by_id_from_part(root);
    for (auto &[id, anchor_text] : source) {
        if (anchor_text.empty()) {
            target.try_emplace(std::move(id), std::string{});
            continue;
        }
        auto [position, inserted] = target.try_emplace(id, anchor_text);
        if (!inserted && position->second.empty()) {
            position->second = anchor_text;
        }
    }
}

void attach_comment_anchor_text(
    std::vector<featherdoc::review_note_summary> &summaries,
    const std::unordered_map<std::string, std::string> &anchor_text_by_id) {
    for (auto &summary : summaries) {
        const auto match = anchor_text_by_id.find(summary.id);
        if (match != anchor_text_by_id.end()) {
            summary.anchor_text = match->second;
        }
    }
}

auto revision_kind_from_node_name(std::string_view node_name)
    -> featherdoc::revision_kind {
    if (node_name == "w:ins") {
        return featherdoc::revision_kind::insertion;
    }
    if (node_name == "w:del") {
        return featherdoc::revision_kind::deletion;
    }
    if (node_name == "w:moveFrom") {
        return featherdoc::revision_kind::move_from;
    }
    if (node_name == "w:moveTo") {
        return featherdoc::revision_kind::move_to;
    }
    if (node_name == "w:pPrChange") {
        return featherdoc::revision_kind::paragraph_property_change;
    }
    if (node_name == "w:rPrChange") {
        return featherdoc::revision_kind::run_property_change;
    }
    return featherdoc::revision_kind::unknown;
}

void collect_revisions_from_node(
    pugi::xml_node node, std::string_view entry_name,
    std::vector<featherdoc::revision_summary> &summaries) {
    for (auto child = node.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        const auto kind = revision_kind_from_node_name(child.name());
        if (kind != featherdoc::revision_kind::unknown) {
            featherdoc::revision_summary summary;
            summary.index = summaries.size();
            summary.kind = kind;
            summary.id = child.attribute("w:id").value();
            summary.author = xml_child_attribute_string(child, "w:author");
            summary.date = xml_child_attribute_string(child, "w:date");
            summary.part_entry_name = std::string{entry_name};
            summary.text = featherdoc::detail::collect_plain_text_from_xml(child);
            summaries.push_back(std::move(summary));
        }
        collect_revisions_from_node(child, entry_name, summaries);
    }
}


void collect_revision_nodes_in_order(pugi::xml_node node,
                                     std::vector<pugi::xml_node> &nodes) {
    for (auto child = node.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        if (revision_kind_from_node_name(child.name()) !=
            featherdoc::revision_kind::unknown) {
            nodes.push_back(child);
        }
        collect_revision_nodes_in_order(child, nodes);
    }
}

void collect_revision_ids(pugi::xml_node node, long &next_id) {
    for (auto child = node.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        if (revision_kind_from_node_name(child.name()) !=
            featherdoc::revision_kind::unknown) {
            const auto id_text = std::string_view{child.attribute("w:id").value()};
            if (!id_text.empty()) {
                char *end = nullptr;
                const auto id_storage = std::string{id_text};
                const auto id = std::strtol(id_storage.c_str(), &end, 10);
                if (end != nullptr && *end == '\0' && id >= next_id) {
                    next_id = id + 1;
                }
            }
        }
        collect_revision_ids(child, next_id);
    }
}

bool revision_text_needs_space_preserve(std::string_view text) {
    return !text.empty() &&
           (std::isspace(static_cast<unsigned char>(text.front())) != 0 ||
            std::isspace(static_cast<unsigned char>(text.back())) != 0);
}

bool append_revision_text_node(pugi::xml_node run, const char *text_node_name,
                               std::string_view text) {
    auto text_node = run.append_child(text_node_name);
    if (text_node == pugi::xml_node{}) {
        return false;
    }
    if (revision_text_needs_space_preserve(text)) {
        auto xml_space = text_node.append_attribute("xml:space");
        if (xml_space == pugi::xml_attribute{}) {
            return false;
        }
        xml_space.set_value("preserve");
    }
    const auto text_storage = std::string{text};
    return text_node.text().set(text_storage.c_str());
}

bool set_revision_run_text_content(pugi::xml_node run, const char *text_node_name,
                                   std::string_view text) {
    if (run == pugi::xml_node{}) {
        return false;
    }

    if (text.empty()) {
        return append_revision_text_node(run, text_node_name, {});
    }

    std::size_t segment_start = 0U;
    std::size_t offset = 0U;
    while (offset < text.size()) {
        const auto current = text[offset];
        if (current != '\n' && current != '\r') {
            ++offset;
            continue;
        }

        if (offset > segment_start &&
            !append_revision_text_node(run, text_node_name,
                                       text.substr(segment_start, offset - segment_start))) {
            return false;
        }
        if (run.append_child("w:br") == pugi::xml_node{}) {
            return false;
        }
        if (current == '\r' && offset + 1U < text.size() && text[offset + 1U] == '\n') {
            ++offset;
        }
        ++offset;
        segment_start = offset;
    }

    if (segment_start < text.size()) {
        return append_revision_text_node(run, text_node_name,
                                         text.substr(segment_start));
    }
    return true;
}

bool set_revision_identity_metadata(pugi::xml_node revision, long revision_id,
                                    std::string_view author,
                                    std::string_view date) {
    if (revision == pugi::xml_node{}) {
        return false;
    }
    ensure_attribute_value(revision, "w:id", std::to_string(revision_id));
    if (!author.empty()) {
        ensure_attribute_value(revision, "w:author", author);
    }
    if (!date.empty()) {
        ensure_attribute_value(revision, "w:date", date);
    }
    return true;
}

pugi::xml_node paragraph_run_by_index(pugi::xml_node paragraph,
                                      std::size_t run_index) {
    if (paragraph == pugi::xml_node{}) {
        return {};
    }
    auto run = paragraph.child("w:r");
    for (std::size_t current_index = 0U;
         current_index < run_index && run != pugi::xml_node{};
         ++current_index) {
        run = featherdoc::detail::next_named_sibling(run, "w:r");
    }
    return run;
}

bool append_copied_run_properties(pugi::xml_node target_run, pugi::xml_node source_run) {
    if (target_run == pugi::xml_node{}) {
        return false;
    }
    const auto source_properties = source_run.child("w:rPr");
    return source_properties == pugi::xml_node{} ||
           target_run.append_copy(source_properties) != pugi::xml_node{};
}

pugi::xml_node append_revision_run(pugi::xml_node revision, pugi::xml_node source_run,
                                   const char *text_node_name, std::string_view text) {
    auto run = revision.append_child("w:r");
    if (run == pugi::xml_node{} || !append_copied_run_properties(run, source_run) ||
        !set_revision_run_text_content(run, text_node_name, text)) {
        return {};
    }
    return run;
}

struct paragraph_revision_run_span {
    pugi::xml_node run;
    std::size_t start_offset = 0U;
    std::size_t length = 0U;
    std::string text;
};

struct paragraph_revision_range_segment {
    pugi::xml_node paragraph;
    std::size_t paragraph_index = 0U;
    std::size_t text_offset = 0U;
    std::size_t text_length = 0U;
};

struct paragraph_revision_range_plan {
    std::vector<paragraph_revision_range_segment> segments;
    std::size_t selected_text_length = 0U;
};

bool revision_range_run_content_supported(pugi::xml_node run) {
    for (auto child = run.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        const auto name = std::string_view{child.name()};
        if (name == "w:rPr" || name == "w:t" || name == "w:tab" ||
            name == "w:br" || name == "w:cr") {
            continue;
        }
        return false;
    }
    return true;
}

auto collect_paragraph_revision_run_spans(pugi::xml_node paragraph)
    -> std::vector<paragraph_revision_run_span> {
    std::vector<paragraph_revision_run_span> spans;
    std::size_t text_offset = 0U;
    for (auto child = paragraph.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        if (std::string_view{child.name()} != "w:r") {
            continue;
        }

        auto text = featherdoc::detail::collect_plain_text_from_xml(child);
        if (text.empty()) {
            continue;
        }

        paragraph_revision_run_span span;
        span.run = child;
        span.start_offset = text_offset;
        span.length = text.size();
        span.text = std::move(text);
        text_offset += span.length;
        spans.push_back(std::move(span));
    }
    return spans;
}

std::size_t paragraph_revision_text_length(
    const std::vector<paragraph_revision_run_span> &spans) {
    if (spans.empty()) {
        return 0U;
    }
    const auto &last = spans.back();
    return last.start_offset + last.length;
}

pugi::xml_node split_revision_text_run(
    pugi::xml_node paragraph, const paragraph_revision_run_span &span,
    std::size_t text_offset) {
    if (text_offset <= span.start_offset ||
        text_offset >= span.start_offset + span.length) {
        return span.run;
    }
    if (!revision_range_run_content_supported(span.run)) {
        return {};
    }

    const auto local_offset = text_offset - span.start_offset;
    auto suffix_run = paragraph.insert_copy_after(span.run, span.run);
    if (suffix_run == pugi::xml_node{}) {
        return {};
    }

    if (!featherdoc::detail::set_plain_text_run_content(
            suffix_run, std::string_view{span.text}.substr(local_offset))) {
        paragraph.remove_child(suffix_run);
        return {};
    }
    if (!featherdoc::detail::set_plain_text_run_content(
            span.run, std::string_view{span.text}.substr(0U, local_offset))) {
        paragraph.remove_child(suffix_run);
        return {};
    }
    return suffix_run;
}

bool split_paragraph_revision_text_at(pugi::xml_node paragraph,
                                      std::size_t text_offset) {
    const auto spans = collect_paragraph_revision_run_spans(paragraph);
    const auto total_length = paragraph_revision_text_length(spans);
    if (text_offset == 0U || text_offset == total_length) {
        return true;
    }
    if (text_offset > total_length) {
        return false;
    }

    for (const auto &span : spans) {
        if (text_offset > span.start_offset &&
            text_offset < span.start_offset + span.length) {
            return split_revision_text_run(paragraph, span, text_offset) !=
                   pugi::xml_node{};
        }
    }
    return true;
}

auto paragraph_revision_spans_in_range(pugi::xml_node paragraph,
                                       std::size_t text_offset,
                                       std::size_t text_length)
    -> std::vector<paragraph_revision_run_span> {
    std::vector<paragraph_revision_run_span> selected;
    const auto end_offset = text_offset + text_length;
    for (auto span : collect_paragraph_revision_run_spans(paragraph)) {
        if (span.start_offset >= text_offset &&
            span.start_offset + span.length <= end_offset) {
            selected.push_back(std::move(span));
        }
    }
    return selected;
}

pugi::xml_node paragraph_revision_insert_before(pugi::xml_node paragraph,
                                                std::size_t text_offset) {
    for (const auto &span : collect_paragraph_revision_run_spans(paragraph)) {
        if (span.start_offset >= text_offset) {
            return span.run;
        }
    }
    return {};
}

pugi::xml_node paragraph_revision_style_run(pugi::xml_node paragraph,
                                            std::size_t text_offset) {
    pugi::xml_node previous_text_run;
    for (const auto &span : collect_paragraph_revision_run_spans(paragraph)) {
        if (span.start_offset >= text_offset) {
            return previous_text_run != pugi::xml_node{} ? previous_text_run
                                                         : span.run;
        }
        previous_text_run = span.run;
    }
    return previous_text_run;
}

pugi::xml_node insert_revision_node(pugi::xml_node paragraph, const char *name,
                                    pugi::xml_node insert_before) {
    return insert_before == pugi::xml_node{}
               ? paragraph.append_child(name)
               : paragraph.insert_child_before(name, insert_before);
}

bool append_revision_runs_from_spans(
    pugi::xml_node revision,
    const std::vector<paragraph_revision_run_span> &spans,
    const char *text_node_name) {
    for (const auto &span : spans) {
        if (!revision_range_run_content_supported(span.run) ||
            append_revision_run(revision, span.run, text_node_name, span.text) ==
                pugi::xml_node{}) {
            return false;
        }
    }
    return true;
}

bool selected_paragraph_revision_spans_supported(
    const std::vector<std::vector<paragraph_revision_run_span>> &selected_segments) {
    for (const auto &selected : selected_segments) {
        for (const auto &span : selected) {
            if (!revision_range_run_content_supported(span.run)) {
                return false;
            }
        }
    }
    return true;
}

bool build_paragraph_revision_range_plan(
    const std::vector<pugi::xml_node> &paragraphs,
    std::size_t start_paragraph_index, std::size_t start_text_offset,
    std::size_t end_paragraph_index, std::size_t end_text_offset,
    paragraph_revision_range_plan &plan, std::string &error_message) {
    plan = {};
    error_message.clear();
    if (start_paragraph_index > end_paragraph_index) {
        error_message = "text range start must not be after end";
        return false;
    }
    if (start_paragraph_index >= paragraphs.size() ||
        end_paragraph_index >= paragraphs.size()) {
        error_message = "paragraph range is out of range";
        return false;
    }

    const auto start_spans =
        collect_paragraph_revision_run_spans(paragraphs[start_paragraph_index]);
    const auto start_text_length = paragraph_revision_text_length(start_spans);
    if (start_text_offset > start_text_length) {
        error_message = "start text offset is out of range";
        return false;
    }

    const auto end_spans =
        collect_paragraph_revision_run_spans(paragraphs[end_paragraph_index]);
    const auto end_text_length = paragraph_revision_text_length(end_spans);
    if (end_text_offset > end_text_length) {
        error_message = "end text offset is out of range";
        return false;
    }
    if (start_paragraph_index == end_paragraph_index &&
        end_text_offset <= start_text_offset) {
        error_message = "text range must not be empty";
        return false;
    }

    for (std::size_t paragraph_index = start_paragraph_index;
         paragraph_index <= end_paragraph_index; ++paragraph_index) {
        const auto spans =
            collect_paragraph_revision_run_spans(paragraphs[paragraph_index]);
        const auto paragraph_text_length = paragraph_revision_text_length(spans);
        const auto segment_start =
            paragraph_index == start_paragraph_index ? start_text_offset : 0U;
        const auto segment_end =
            paragraph_index == end_paragraph_index ? end_text_offset
                                                   : paragraph_text_length;
        if (segment_start > paragraph_text_length ||
            segment_end > paragraph_text_length || segment_end < segment_start) {
            error_message = "text range is out of range";
            return false;
        }
        if (segment_end == segment_start) {
            continue;
        }

        paragraph_revision_range_segment segment;
        segment.paragraph = paragraphs[paragraph_index];
        segment.paragraph_index = paragraph_index;
        segment.text_offset = segment_start;
        segment.text_length = segment_end - segment_start;
        plan.selected_text_length += segment.text_length;
        plan.segments.push_back(segment);
    }

    if (plan.selected_text_length == 0U) {
        error_message = "text range must not be empty";
        return false;
    }
    return true;
}

bool split_paragraph_revision_range_segments(
    const std::vector<paragraph_revision_range_segment> &segments) {
    for (const auto &segment : segments) {
        if (!split_paragraph_revision_text_at(
                segment.paragraph, segment.text_offset + segment.text_length) ||
            !split_paragraph_revision_text_at(segment.paragraph,
                                              segment.text_offset)) {
            return false;
        }
    }
    return true;
}

auto collect_selected_paragraph_revision_spans(
    const std::vector<paragraph_revision_range_segment> &segments)
    -> std::vector<std::vector<paragraph_revision_run_span>> {
    std::vector<std::vector<paragraph_revision_run_span>> selected_segments;
    selected_segments.reserve(segments.size());
    for (const auto &segment : segments) {
        auto selected = paragraph_revision_spans_in_range(
            segment.paragraph, segment.text_offset, segment.text_length);
        if (selected.empty()) {
            return {};
        }
        selected_segments.push_back(std::move(selected));
    }
    return selected_segments;
}

auto preview_paragraph_revision_range_segments(
    std::size_t start_paragraph_index, std::size_t start_text_offset,
    std::size_t end_paragraph_index, std::size_t end_text_offset,
    const paragraph_revision_range_plan &plan) -> featherdoc::text_range_preview {
    featherdoc::text_range_preview preview;
    preview.start_paragraph_index = start_paragraph_index;
    preview.start_text_offset = start_text_offset;
    preview.end_paragraph_index = end_paragraph_index;
    preview.end_text_offset = end_text_offset;
    preview.text_length = plan.selected_text_length;

    for (const auto &segment : plan.segments) {
        featherdoc::text_range_preview_segment preview_segment;
        preview_segment.paragraph_index = segment.paragraph_index;
        preview_segment.text_offset = segment.text_offset;
        preview_segment.text_length = segment.text_length;

        const auto segment_end = segment.text_offset + segment.text_length;
        for (const auto &span :
             collect_paragraph_revision_run_spans(segment.paragraph)) {
            const auto span_start = span.start_offset;
            const auto span_end = span.start_offset + span.length;
            if (span_end <= segment.text_offset || span_start >= segment_end) {
                continue;
            }
            if (!revision_range_run_content_supported(span.run)) {
                preview.plain_text_runs_supported = false;
            }

            const auto selected_start = std::max(span_start, segment.text_offset);
            const auto selected_end = std::min(span_end, segment_end);
            const auto local_start = selected_start - span_start;
            preview_segment.text.append(
                std::string_view{span.text}.substr(local_start,
                                                   selected_end - selected_start));
        }

        preview.text += preview_segment.text;
        preview.segments.push_back(std::move(preview_segment));
    }

    return preview;
}

auto revision_expected_text_mismatch_detail(std::string_view expected_text,
                                            std::string_view actual_text)
    -> std::string {
    std::string detail{"expected text did not match selected text"};
    detail += " (expected: ";
    detail += expected_text;
    detail += ", actual: ";
    detail += actual_text;
    detail += ')';
    return detail;
}

auto validate_revision_expected_text(
    featherdoc::document_error_info &last_error_info,
    const featherdoc::text_range_preview &preview,
    const featherdoc::revision_text_range_options &options) -> bool {
    if (!options.expected_text.has_value()) {
        return true;
    }

    if (preview.text == *options.expected_text) {
        return true;
    }

    set_last_error(last_error_info, std::make_error_code(std::errc::invalid_argument),
                   revision_expected_text_mismatch_detail(*options.expected_text,
                                                         preview.text),
                   std::string{document_xml_entry});
    return false;
}

bool insert_comment_markers_for_selected_spans(
    const std::vector<paragraph_revision_range_segment> &segments,
    const std::vector<std::vector<paragraph_revision_run_span>> &selected_segments,
    std::string_view comment_id) {
    if (segments.empty() || selected_segments.empty() ||
        selected_segments.front().empty() || selected_segments.back().empty()) {
        return false;
    }

    auto first_paragraph = segments.front().paragraph;
    auto last_paragraph = segments.back().paragraph;
    auto first_run = selected_segments.front().front().run;
    auto last_run = selected_segments.back().back().run;
    auto range_start =
        first_paragraph.insert_child_before("w:commentRangeStart", first_run);
    auto range_end =
        last_paragraph.insert_child_after("w:commentRangeEnd", last_run);
    auto reference_run = range_end == pugi::xml_node{}
                             ? pugi::xml_node{}
                             : last_paragraph.insert_child_after("w:r", range_end);
    auto reference = reference_run == pugi::xml_node{}
                         ? pugi::xml_node{}
                         : reference_run.append_child("w:commentReference");
    if (range_start == pugi::xml_node{} || range_end == pugi::xml_node{} ||
        reference_run == pugi::xml_node{} || reference == pugi::xml_node{}) {
        return false;
    }
    ensure_attribute_value(range_start, "w:id", comment_id);
    ensure_attribute_value(range_end, "w:id", comment_id);
    ensure_attribute_value(reference, "w:id", comment_id);
    return true;
}

bool xml_text_looks_like_omml(std::string_view text) {
    const auto first = text.find_first_not_of(" \t\r\n");
    return first != std::string_view::npos && text.substr(first).starts_with("<m:");
}

std::string escape_xml_text(std::string_view text) {
    std::string escaped;
    escaped.reserve(text.size());
    for (const char ch : text) {
        switch (ch) {
        case '&': escaped += "&amp;"; break;
        case '<': escaped += "&lt;"; break;
        case '>': escaped += "&gt;"; break;
        case '"': escaped += "&quot;"; break;
        case '\'': escaped += "&apos;"; break;
        default: escaped.push_back(ch); break;
        }
    }
    return escaped;
}

std::string omml_run_xml(std::string_view text) {
    return "<m:r><m:t>" + escape_xml_text(text) + "</m:t></m:r>";
}

std::string omml_expression_xml(std::string_view text) {
    if (xml_text_looks_like_omml(text)) {
        return std::string{text};
    }
    return omml_run_xml(text);
}

std::string wrap_omml_root(std::string inner_xml, bool display) {
    constexpr auto xmlns =
        " xmlns:m=\"http://schemas.openxmlformats.org/officeDocument/2006/math\"";
    if (display) {
        return std::string{"<m:oMathPara"} + xmlns + "><m:oMath>" +
               std::move(inner_xml) + "</m:oMath></m:oMathPara>";
    }
    return std::string{"<m:oMath"} + xmlns + ">" + std::move(inner_xml) +
           "</m:oMath>";
}

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

void ensure_wordprocessingml_2010_namespace(pugi::xml_node root);
void ensure_wordprocessingml_2012_namespace(pugi::xml_node root);
void ensure_markup_compatibility_ignorable(pugi::xml_node root,
                                           std::string_view prefix);

pugi::xml_node comment_by_summary_index(pugi::xml_node root, std::size_t comment_index) {
    std::size_t current_index = 0;
    for (auto comment = root.child("w:comment"); comment != pugi::xml_node{};
         comment = comment.next_sibling("w:comment")) {
        if (current_index == comment_index) {
            return comment;
        }
        ++current_index;
    }
    return {};
}

bool on_off_attribute_is_true(std::string_view value) {
    return value == "1" || value == "true" || value == "on";
}

std::string uppercase_hex_string(std::string_view value) {
    std::string result;
    result.reserve(value.size());
    for (const char ch : value) {
        result.push_back(static_cast<char>(
            std::toupper(static_cast<unsigned char>(ch))));
    }
    return result;
}

std::string format_hex8(std::uint32_t value) {
    constexpr auto digits = std::string_view{"0123456789ABCDEF"};
    std::string result(8U, '0');
    for (std::size_t index = 0U; index < result.size(); ++index) {
        const auto shift = static_cast<unsigned>((result.size() - index - 1U) * 4U);
        result[index] = digits[(value >> shift) & 0x0FU];
    }
    return result;
}

auto comment_paragraph_id(pugi::xml_node comment) -> std::optional<std::string> {
    const auto paragraph = comment.child("w:p");
    if (paragraph == pugi::xml_node{}) {
        return std::nullopt;
    }
    auto value = std::string_view{paragraph.attribute("w14:paraId").value()};
    if (value.empty()) {
        value = std::string_view{paragraph.attribute("w15:paraId").value()};
    }
    if (value.empty()) {
        return std::nullopt;
    }
    return uppercase_hex_string(value);
}

auto next_comment_paragraph_id(pugi::xml_node comments_root) -> std::string {
    std::unordered_set<std::string> used_ids;
    for (auto comment = comments_root.child("w:comment");
         comment != pugi::xml_node{};
         comment = comment.next_sibling("w:comment")) {
        if (const auto id = comment_paragraph_id(comment); id.has_value()) {
            used_ids.insert(*id);
        }
    }

    for (std::uint32_t value = 1U;; ++value) {
        auto candidate = format_hex8(value);
        if (!used_ids.contains(candidate)) {
            return candidate;
        }
    }
}

auto ensure_comment_paragraph_id(pugi::xml_node comments_root,
                                 pugi::xml_node comment, bool &comments_dirty)
    -> std::optional<std::string> {
    if (const auto existing = comment_paragraph_id(comment); existing.has_value()) {
        return existing;
    }

    auto paragraph = comment.child("w:p");
    if (paragraph == pugi::xml_node{}) {
        return std::nullopt;
    }

    const auto para_id = next_comment_paragraph_id(comments_root);
    ensure_wordprocessingml_2010_namespace(comments_root);
    ensure_markup_compatibility_ignorable(comments_root, "w14");
    ensure_attribute_value(paragraph, "w14:paraId", para_id);
    comments_dirty = true;
    return para_id;
}

struct comment_extended_metadata {
    bool has_done{false};
    bool done{false};
    std::optional<std::string> parent_para_id;
};

auto comment_metadata_by_paragraph_id(const pugi::xml_document &comments_extended)
    -> std::unordered_map<std::string, comment_extended_metadata> {
    std::unordered_map<std::string, comment_extended_metadata> metadata_by_para_id;
    const auto root = comments_extended.child("w15:commentsEx");
    for (auto comment_ex = root.child("w15:commentEx");
         comment_ex != pugi::xml_node{};
         comment_ex = comment_ex.next_sibling("w15:commentEx")) {
        const auto para_id =
            std::string_view{comment_ex.attribute("w15:paraId").value()};
        if (para_id.empty()) {
            continue;
        }
        comment_extended_metadata metadata;
        const auto done =
            std::string_view{comment_ex.attribute("w15:done").value()};
        if (!done.empty()) {
            metadata.has_done = true;
            metadata.done = on_off_attribute_is_true(done);
        }
        const auto parent_para_id =
            std::string_view{comment_ex.attribute("w15:paraIdParent").value()};
        if (!parent_para_id.empty()) {
            metadata.parent_para_id = uppercase_hex_string(parent_para_id);
        }
        metadata_by_para_id[uppercase_hex_string(para_id)] = std::move(metadata);
    }
    return metadata_by_para_id;
}

void attach_comment_extended_metadata(
    std::vector<featherdoc::review_note_summary> &summaries,
    const pugi::xml_document &comments_document,
    const pugi::xml_document &comments_extended) {
    if (summaries.empty()) {
        return;
    }
    const auto metadata_by_para_id =
        comment_metadata_by_paragraph_id(comments_extended);
    if (metadata_by_para_id.empty()) {
        return;
    }

    const auto root = comments_document.child("w:comments");
    std::vector<std::optional<std::string>> para_ids_by_index(summaries.size());
    std::unordered_map<std::string, std::size_t> index_by_para_id;
    std::unordered_map<std::string, std::string> id_by_para_id;
    std::size_t index = 0U;
    for (auto comment = root.child("w:comment");
         comment != pugi::xml_node{} && index < summaries.size();
         comment = comment.next_sibling("w:comment"), ++index) {
        const auto para_id = comment_paragraph_id(comment);
        if (!para_id.has_value()) {
            continue;
        }
        para_ids_by_index[index] = *para_id;
        index_by_para_id[*para_id] = index;
        id_by_para_id[*para_id] = summaries[index].id;
    }

    for (std::size_t summary_index = 0U; summary_index < summaries.size();
         ++summary_index) {
        const auto &para_id = para_ids_by_index[summary_index];
        if (!para_id.has_value()) {
            continue;
        }
        const auto match = metadata_by_para_id.find(*para_id);
        if (match == metadata_by_para_id.end()) {
            continue;
        }
        const auto &metadata = match->second;
        if (metadata.has_done) {
            summaries[summary_index].resolved = metadata.done;
        }
        if (!metadata.parent_para_id.has_value()) {
            continue;
        }
        if (const auto parent_index =
                index_by_para_id.find(*metadata.parent_para_id);
            parent_index != index_by_para_id.end()) {
            summaries[summary_index].parent_index = parent_index->second;
        }
        if (const auto parent_id = id_by_para_id.find(*metadata.parent_para_id);
            parent_id != id_by_para_id.end()) {
            summaries[summary_index].parent_id = parent_id->second;
        }
    }
}

auto comment_extended_by_paragraph_id(pugi::xml_node root, std::string_view para_id)
    -> pugi::xml_node {
    const auto normalized_para_id = uppercase_hex_string(para_id);
    for (auto comment_ex = root.child("w15:commentEx");
         comment_ex != pugi::xml_node{};
         comment_ex = comment_ex.next_sibling("w15:commentEx")) {
        const auto candidate =
            std::string_view{comment_ex.attribute("w15:paraId").value()};
        if (uppercase_hex_string(candidate) == normalized_para_id) {
            return comment_ex;
        }
    }
    return {};
}

auto ensure_comment_extended_by_paragraph_id(pugi::xml_node root,
                                             std::string_view para_id)
    -> pugi::xml_node {
    auto comment_ex = comment_extended_by_paragraph_id(root, para_id);
    if (comment_ex == pugi::xml_node{}) {
        comment_ex = root.append_child("w15:commentEx");
    }
    ensure_attribute_value(comment_ex, "w15:paraId", uppercase_hex_string(para_id));
    return comment_ex;
}

bool remove_comment_extended_by_paragraph_id(pugi::xml_document &comments_extended,
                                             std::string_view para_id) {
    auto root = comments_extended.child("w15:commentsEx");
    auto comment_ex = comment_extended_by_paragraph_id(root, para_id);
    if (root == pugi::xml_node{} || comment_ex == pugi::xml_node{}) {
        return false;
    }
    return root.remove_child(comment_ex);
}

auto child_comment_para_ids_by_parent_para_id(
    const pugi::xml_document &comments_extended)
    -> std::unordered_map<std::string, std::vector<std::string>> {
    std::unordered_map<std::string, std::vector<std::string>> children_by_parent;
    const auto root = comments_extended.child("w15:commentsEx");
    for (auto comment_ex = root.child("w15:commentEx");
         comment_ex != pugi::xml_node{};
         comment_ex = comment_ex.next_sibling("w15:commentEx")) {
        const auto para_id =
            std::string_view{comment_ex.attribute("w15:paraId").value()};
        const auto parent_para_id =
            std::string_view{comment_ex.attribute("w15:paraIdParent").value()};
        if (para_id.empty() || parent_para_id.empty()) {
            continue;
        }
        children_by_parent[uppercase_hex_string(parent_para_id)].push_back(
            uppercase_hex_string(para_id));
    }
    return children_by_parent;
}

auto comment_thread_para_ids(const pugi::xml_document &comments_extended,
                             std::string_view root_para_id)
    -> std::unordered_set<std::string> {
    const auto children_by_parent =
        child_comment_para_ids_by_parent_para_id(comments_extended);
    std::unordered_set<std::string> para_ids;
    std::vector<std::string> pending{uppercase_hex_string(root_para_id)};
    while (!pending.empty()) {
        auto current = std::move(pending.back());
        pending.pop_back();
        if (!para_ids.insert(current).second) {
            continue;
        }
        const auto children = children_by_parent.find(current);
        if (children == children_by_parent.end()) {
            continue;
        }
        pending.insert(pending.end(), children->second.begin(),
                       children->second.end());
    }
    return para_ids;
}

void remove_comment_markers_by_id(pugi::xml_node node, std::string_view id) {
    for (auto child = node.first_child(); child != pugi::xml_node{};) {
        const auto next = child.next_sibling();
        const auto name = std::string_view{child.name()};
        const bool is_marker =
            (name == "w:commentRangeStart" || name == "w:commentRangeEnd" ||
             name == "w:commentReference") &&
            std::string_view{child.attribute("w:id").value()} == id;
        if (is_marker) {
            auto parent = child.parent();
            if (name == "w:commentReference" && std::string_view{parent.name()} == "w:r") {
                parent.parent().remove_child(parent);
            } else {
                parent.remove_child(child);
            }
        } else {
            remove_comment_markers_by_id(child, id);
        }
        child = next;
    }
}

void convert_deleted_text_to_text(pugi::xml_node node) {
    for (auto child = node.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        if (std::string_view{child.name()} == "w:delText") {
            child.set_name("w:t");
        }
        convert_deleted_text_to_text(child);
    }
}

bool unwrap_revision_node(pugi::xml_node revision, bool convert_deleted_text) {
    auto parent = revision.parent();
    if (parent == pugi::xml_node{}) {
        return false;
    }
    for (auto child = revision.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        auto copy = parent.insert_copy_before(child, revision);
        if (copy == pugi::xml_node{}) {
            return false;
        }
        if (convert_deleted_text) {
            convert_deleted_text_to_text(copy);
        }
    }
    return parent.remove_child(revision);
}

bool apply_revision_decision(pugi::xml_node revision, bool accept) {
    const auto kind = revision_kind_from_node_name(revision.name());
    switch (kind) {
    case featherdoc::revision_kind::insertion:
    case featherdoc::revision_kind::move_to:
        return accept ? unwrap_revision_node(revision, false)
                      : revision.parent().remove_child(revision);
    case featherdoc::revision_kind::deletion:
    case featherdoc::revision_kind::move_from:
        return accept ? revision.parent().remove_child(revision)
                      : unwrap_revision_node(revision, true);
    case featherdoc::revision_kind::paragraph_property_change:
    case featherdoc::revision_kind::run_property_change:
        return revision.parent().remove_child(revision);
    case featherdoc::revision_kind::unknown:
        return false;
    }
    return false;
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

bool append_review_separator(pugi::xml_node root, const char *note_node_name,
                             const char *separator_child_name, std::string_view type,
                             std::string_view id) {
    auto note = root.append_child(note_node_name);
    auto paragraph = note.append_child("w:p");
    auto run = paragraph.append_child("w:r");
    if (note == pugi::xml_node{} || paragraph == pugi::xml_node{} ||
        run == pugi::xml_node{} || run.append_child(separator_child_name) == pugi::xml_node{}) {
        return false;
    }
    ensure_attribute_value(note, "w:type", type);
    ensure_attribute_value(note, "w:id", id);
    return true;
}

bool initialize_review_notes_document(pugi::xml_document &xml_document,
                                      featherdoc::review_note_kind kind) {
    xml_document.reset();
    const bool footnote = kind == featherdoc::review_note_kind::footnote;
    auto root = xml_document.append_child(footnote ? "w:footnotes" : "w:endnotes");
    if (root == pugi::xml_node{}) {
        return false;
    }
    ensure_wordprocessingml_namespace(root);
    const char *note_node_name = footnote ? "w:footnote" : "w:endnote";
    return append_review_separator(root, note_node_name, "w:separator",
                                   "separator", "-1") &&
           append_review_separator(root, note_node_name,
                                   "w:continuationSeparator",
                                   "continuationSeparator", "0");
}

bool initialize_comments_document(pugi::xml_document &xml_document) {
    xml_document.reset();
    auto root = xml_document.append_child("w:comments");
    if (root == pugi::xml_node{}) {
        return false;
    }
    ensure_wordprocessingml_namespace(root);
    return true;
}

bool initialize_comments_extended_document(pugi::xml_document &xml_document) {
    xml_document.reset();
    auto root = xml_document.append_child("w15:commentsEx");
    if (root == pugi::xml_node{}) {
        return false;
    }
    ensure_wordprocessingml_2012_namespace(root);
    ensure_markup_compatibility_ignorable(root, "w15");
    return true;
}

long next_numeric_id(pugi::xml_node root, const char *node_name, long minimum_id) {
    long next_id = minimum_id;
    for (auto node = root.child(node_name); node != pugi::xml_node{};
         node = node.next_sibling(node_name)) {
        const auto value_text = std::string_view{node.attribute("w:id").value()};
        if (value_text.empty()) {
            continue;
        }
        char *end = nullptr;
        const auto value_storage = std::string{value_text};
        const auto value = std::strtol(value_storage.c_str(), &end, 10);
        if (end != nullptr && *end == '\0' && value >= next_id) {
            next_id = value + 1;
        }
    }
    return next_id;
}

pugi::xml_node note_by_summary_index(pugi::xml_node root,
                                     featherdoc::review_note_kind kind,
                                     std::size_t note_index) {
    const char *node_name = kind == featherdoc::review_note_kind::footnote
                                ? "w:footnote"
                                : "w:endnote";
    std::size_t current_index = 0U;
    for (auto note = root.child(node_name); note != pugi::xml_node{};
         note = note.next_sibling(node_name)) {
        const auto type = std::string_view{note.attribute("w:type").value()};
        if (type == "separator" || type == "continuationSeparator") {
            continue;
        }
        if (current_index == note_index) {
            return note;
        }
        ++current_index;
    }
    return {};
}

bool set_review_note_text(pugi::xml_node note, const char *reference_child_name,
                          std::string_view note_text) {
    clear_children(note);
    auto paragraph = note.append_child("w:p");
    auto reference_run = paragraph.append_child("w:r");
    auto text_run = paragraph.append_child("w:r");
    if (paragraph == pugi::xml_node{} || reference_run == pugi::xml_node{} ||
        text_run == pugi::xml_node{} ||
        reference_run.append_child(reference_child_name) == pugi::xml_node{}) {
        return false;
    }
    return featherdoc::detail::set_plain_text_run_content(text_run, note_text);
}

bool set_comment_text(pugi::xml_node comment, std::string_view comment_text) {
    clear_children(comment);
    auto paragraph = comment.append_child("w:p");
    auto run = paragraph.append_child("w:r");
    if (paragraph == pugi::xml_node{} || run == pugi::xml_node{}) {
        return false;
    }
    return featherdoc::detail::set_plain_text_run_content(run, comment_text);
}

void remove_references_by_id(pugi::xml_node node, const char *reference_node_name,
                             std::string_view id) {
    for (auto child = node.first_child(); child != pugi::xml_node{};) {
        const auto next = child.next_sibling();
        const bool is_reference = std::string_view{child.name()} == reference_node_name &&
                                  std::string_view{child.attribute("w:id").value()} == id;
        if (is_reference) {
            auto parent = child.parent();
            if (std::string_view{parent.name()} == "w:r") {
                parent.parent().remove_child(parent);
            } else {
                parent.remove_child(child);
            }
        } else {
            remove_references_by_id(child, reference_node_name, id);
        }
        child = next;
    }
}

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

void collect_named_bookmark_starts(pugi::xml_node node, std::string_view bookmark_name,
                                   std::vector<pugi::xml_node> &bookmark_starts) {
    for (auto child = node.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        if (std::string_view{child.name()} == "w:bookmarkStart" &&
            std::string_view{child.attribute("w:name").value()} == bookmark_name) {
            bookmark_starts.push_back(child);
        }

        collect_named_bookmark_starts(child, bookmark_name, bookmark_starts);
    }
}

void collect_bookmark_starts_in_order(pugi::xml_node node,
                                      std::vector<pugi::xml_node> &bookmark_starts) {
    for (auto child = node.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        if (std::string_view{child.name()} == "w:bookmarkStart" &&
            child.attribute("w:name").value()[0] != '\0') {
            bookmark_starts.push_back(child);
        }

        collect_bookmark_starts_in_order(child, bookmark_starts);
    }
}

auto find_matching_bookmark_end(pugi::xml_node bookmark_start) -> pugi::xml_node {
    const auto bookmark_id = std::string_view{bookmark_start.attribute("w:id").value()};
    if (bookmark_id.empty()) {
        return {};
    }

    for (auto node = bookmark_start.next_sibling(); node != pugi::xml_node{};
         node = node.next_sibling()) {
        if (std::string_view{node.name()} == "w:bookmarkEnd" &&
            std::string_view{node.attribute("w:id").value()} == bookmark_id) {
            return node;
        }
    }

    return {};
}

auto next_node_in_document_order(pugi::xml_node node) -> pugi::xml_node {
    if (node == pugi::xml_node{}) {
        return {};
    }

    if (auto child = node.first_child(); child != pugi::xml_node{}) {
        return child;
    }

    for (auto current = node; current != pugi::xml_node{}; current = current.parent()) {
        if (auto sibling = current.next_sibling(); sibling != pugi::xml_node{}) {
            return sibling;
        }
    }

    return {};
}

auto find_top_level_table_ancestor(pugi::xml_node node, pugi::xml_node container)
    -> pugi::xml_node {
    for (auto current = node; current != pugi::xml_node{} && current != container;
         current = current.parent()) {
        if (std::string_view{current.name()} == "w:tbl" && current.parent() == container) {
            return current;
        }
    }

    return {};
}

auto find_top_level_table_index(pugi::xml_node container, pugi::xml_node table_node)
    -> std::optional<std::size_t> {
    if (container == pugi::xml_node{} || table_node == pugi::xml_node{} ||
        std::string_view{table_node.name()} != "w:tbl" || table_node.parent() != container) {
        return std::nullopt;
    }

    std::size_t table_index = 0U;
    for (auto child = container.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        if (child == table_node) {
            return table_index;
        }
        if (std::string_view{child.name()} == "w:tbl") {
            ++table_index;
        }
    }

    return std::nullopt;
}

auto find_first_top_level_table_after_node(pugi::xml_node container, pugi::xml_node node,
                                           pugi::xml_node stop_before = {})
    -> pugi::xml_node {
    for (auto current = next_node_in_document_order(node); current != pugi::xml_node{};
         current = next_node_in_document_order(current)) {
        if (current == stop_before) {
            break;
        }
        if (std::string_view{current.name()} == "w:tbl" && current.parent() == container) {
            return current;
        }
    }

    return {};
}

auto selector_uses_text_matching(
    const featherdoc::template_table_selector &selector) -> bool {
    return selector.after_paragraph_text.has_value() ||
           !selector.header_cell_texts.empty();
}

auto validate_template_table_selector(
    featherdoc::document_error_info &last_error_info, std::string_view entry_name,
    const featherdoc::template_table_selector &selector) -> bool {
    const auto uses_direct_target =
        selector.table_index.has_value() || selector.bookmark_name.has_value();
    const auto uses_text_target = selector_uses_text_matching(selector);

    if (selector.table_index.has_value() && selector.bookmark_name.has_value()) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "template table selector cannot combine table_index and "
                       "bookmark_name",
                       std::string{entry_name});
        return false;
    }

    if (selector.bookmark_name.has_value() && selector.bookmark_name->empty()) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "template table selector bookmark_name must not be empty",
                       std::string{entry_name});
        return false;
    }

    if (selector.after_paragraph_text.has_value() &&
        selector.after_paragraph_text->empty()) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "template table selector after_paragraph_text must not be "
                       "empty",
                       std::string{entry_name});
        return false;
    }

    if (uses_direct_target && uses_text_target) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "template table selector cannot combine table_index or "
                       "bookmark_name with after_paragraph_text or "
                       "header_cell_texts",
                       std::string{entry_name});
        return false;
    }

    if (!uses_direct_target && !uses_text_target) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "template table selector must specify table_index, "
                       "bookmark_name, after_paragraph_text, or "
                       "header_cell_texts",
                       std::string{entry_name});
        return false;
    }

    if (selector.occurrence != 0U && !uses_text_target) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "template table selector occurrence requires "
                       "after_paragraph_text or header_cell_texts",
                       std::string{entry_name});
        return false;
    }

    if (selector.header_row_index != 0U && selector.header_cell_texts.empty()) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "template table selector header_row_index requires "
                       "header_cell_texts",
                       std::string{entry_name});
        return false;
    }

    return true;
}

auto find_top_level_table_by_index(pugi::xml_node container, std::size_t table_index)
    -> pugi::xml_node {
    std::size_t current_table_index = 0U;
    for (auto child = container.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        if (std::string_view{child.name()} != "w:tbl") {
            continue;
        }
        if (current_table_index == table_index) {
            return child;
        }
        ++current_table_index;
    }

    return {};
}

auto push_unique_table_node(std::vector<pugi::xml_node> &matches,
                            pugi::xml_node table_node) -> void {
    if (table_node == pugi::xml_node{}) {
        return;
    }
    if (std::find(matches.begin(), matches.end(), table_node) == matches.end()) {
        matches.push_back(table_node);
    }
}

auto top_level_paragraph_contains_text(pugi::xml_node paragraph_node,
                                       std::string_view text) -> bool {
    if (paragraph_node == pugi::xml_node{} ||
        std::string_view{paragraph_node.name()} != "w:p" || text.empty()) {
        return false;
    }

    return summarize_part_paragraph_node(paragraph_node, 0U).text.find(text) !=
           std::string::npos;
}

auto table_row_cell_texts(pugi::xml_node container, pugi::xml_node table_node,
                          std::size_t row_index)
    -> std::optional<std::vector<std::string>> {
    if (container == pugi::xml_node{} || table_node == pugi::xml_node{} ||
        std::string_view{table_node.name()} != "w:tbl" ||
        table_node.parent() != container) {
        return std::nullopt;
    }

    auto table = featherdoc::Table{container, table_node};
    auto row = table.rows();
    for (std::size_t current_row_index = 0U; row.has_next();
         ++current_row_index, row.next()) {
        if (current_row_index != row_index) {
            continue;
        }

        auto cell_texts = std::vector<std::string>{};
        auto cell = row.cells();
        while (cell.has_next()) {
            cell_texts.push_back(cell.get_text());
            cell.next();
        }
        return cell_texts;
    }

    return std::nullopt;
}

auto top_level_table_matches_selector_header(
    pugi::xml_node container, pugi::xml_node table_node,
    const featherdoc::template_table_selector &selector) -> bool {
    if (selector.header_cell_texts.empty()) {
        return true;
    }

    const auto header_cells =
        table_row_cell_texts(container, table_node, selector.header_row_index);
    return header_cells.has_value() &&
           *header_cells == selector.header_cell_texts;
}

auto find_first_matching_top_level_table_after_node(
    pugi::xml_node container, pugi::xml_node node,
    const featherdoc::template_table_selector &selector) -> pugi::xml_node {
    for (auto current = next_node_in_document_order(node); current != pugi::xml_node{};
         current = next_node_in_document_order(current)) {
        if (std::string_view{current.name()} == "w:tbl" &&
            current.parent() == container &&
            top_level_table_matches_selector_header(container, current, selector)) {
            return current;
        }
    }

    return {};
}

auto find_table_index_by_selector_in_part(
    featherdoc::document_error_info &last_error_info, pugi::xml_document &document,
    std::string_view entry_name,
    const featherdoc::template_table_selector &selector)
    -> std::optional<std::size_t> {
    if (!validate_template_table_selector(last_error_info, entry_name, selector)) {
        return std::nullopt;
    }

    const auto container = template_part_block_container(document);
    if (container == pugi::xml_node{}) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "template part does not contain a supported block container",
                       std::string{entry_name});
        return std::nullopt;
    }

    if (selector.table_index.has_value()) {
        if (find_top_level_table_by_index(container, *selector.table_index) ==
            pugi::xml_node{}) {
            set_last_error(last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           "table index '" +
                               std::to_string(*selector.table_index) +
                               "' is out of range in " + std::string{entry_name},
                           std::string{entry_name});
            return std::nullopt;
        }

        last_error_info.clear();
        return selector.table_index;
    }

    if (selector.bookmark_name.has_value()) {
        return featherdoc::find_table_index_by_bookmark_in_part(
            last_error_info, document, entry_name, *selector.bookmark_name);
    }

    auto matches = std::vector<pugi::xml_node>{};
    if (selector.after_paragraph_text.has_value()) {
        for (auto child = container.first_child(); child != pugi::xml_node{};
             child = child.next_sibling()) {
            if (!top_level_paragraph_contains_text(child,
                                                   *selector.after_paragraph_text)) {
                continue;
            }

            const auto table =
                selector.header_cell_texts.empty()
                    ? find_first_top_level_table_after_node(container, child)
                    : find_first_matching_top_level_table_after_node(container,
                                                                     child,
                                                                     selector);
            push_unique_table_node(matches, table);
        }
    } else {
        for (auto child = container.first_child(); child != pugi::xml_node{};
             child = child.next_sibling()) {
            if (std::string_view{child.name()} == "w:tbl" &&
                top_level_table_matches_selector_header(container, child, selector)) {
                matches.push_back(child);
            }
        }
    }

    if (matches.empty()) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "template table selector did not resolve to a table in " +
                           std::string{entry_name},
                       std::string{entry_name});
        return std::nullopt;
    }

    if (selector.occurrence >= matches.size()) {
        set_last_error(
            last_error_info, std::make_error_code(std::errc::invalid_argument),
            "template table selector occurrence '" +
                std::to_string(selector.occurrence) + "' is out of range for " +
                std::to_string(matches.size()) + " match(es) in " +
                std::string{entry_name},
            std::string{entry_name});
        return std::nullopt;
    }

    const auto resolved_index =
        find_top_level_table_index(container, matches[selector.occurrence]);
    if (!resolved_index.has_value()) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "template table selector resolved an invalid table node in " +
                           std::string{entry_name},
                       std::string{entry_name});
        return std::nullopt;
    }

    last_error_info.clear();
    return resolved_index;
}

auto find_matching_bookmark_end_in_document_order(pugi::xml_node bookmark_start)
    -> pugi::xml_node {
    const auto bookmark_id = std::string_view{bookmark_start.attribute("w:id").value()};
    if (bookmark_id.empty()) {
        return {};
    }

    for (auto node = next_node_in_document_order(bookmark_start); node != pugi::xml_node{};
         node = next_node_in_document_order(node)) {
        if (std::string_view{node.name()} == "w:bookmarkEnd" &&
            std::string_view{node.attribute("w:id").value()} == bookmark_id) {
            return node;
        }
    }

    return {};
}

auto replace_bookmark_range(pugi::xml_node bookmark_start, std::string_view replacement)
    -> bool {
    auto parent = bookmark_start.parent();
    if (bookmark_start == pugi::xml_node{} || parent == pugi::xml_node{}) {
        return false;
    }

    const auto bookmark_end = find_matching_bookmark_end(bookmark_start);
    if (bookmark_end == pugi::xml_node{}) {
        return false;
    }

    for (auto node = bookmark_start.next_sibling();
         node != pugi::xml_node{} && node != bookmark_end;) {
        const auto next = node.next_sibling();
        parent.remove_child(node);
        node = next;
    }

    if (!replacement.empty()) {
        auto replacement_run = parent.insert_child_before("w:r", bookmark_end);
        if (replacement_run == pugi::xml_node{}) {
            return false;
        }
        return featherdoc::detail::set_plain_text_run_content(replacement_run, replacement);
    }

    return true;
}

auto is_ignorable_block_placeholder_node(pugi::xml_node node) -> bool {
    const auto name = std::string_view{node.name()};
    return name == "w:pPr" || name == "w:proofErr" || name == "w:bookmarkStart" ||
           name == "w:bookmarkEnd" || name == "w:permStart" || name == "w:permEnd" ||
           name == "w:commentRangeStart" || name == "w:commentRangeEnd" ||
           name == "w:moveFromRangeStart" || name == "w:moveFromRangeEnd" ||
           name == "w:moveToRangeStart" || name == "w:moveToRangeEnd";
}

auto paragraph_is_block_placeholder(pugi::xml_node paragraph, pugi::xml_node bookmark_start,
                                    pugi::xml_node bookmark_end) -> bool {
    if (paragraph == pugi::xml_node{} || bookmark_start == pugi::xml_node{} ||
        bookmark_end == pugi::xml_node{}) {
        return false;
    }

    bool inside_placeholder = false;
    for (auto child = paragraph.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        if (child == bookmark_start) {
            inside_placeholder = true;
            continue;
        }

        if (child == bookmark_end) {
            inside_placeholder = false;
            continue;
        }

        if (inside_placeholder) {
            continue;
        }

        if (!is_ignorable_block_placeholder_node(child)) {
            return false;
        }
    }

    return !inside_placeholder;
}

auto paragraph_is_bookmark_marker(pugi::xml_node paragraph, pugi::xml_node marker) -> bool {
    if (paragraph == pugi::xml_node{} || marker == pugi::xml_node{} || marker.parent() != paragraph) {
        return false;
    }

    for (auto child = paragraph.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        if (child == marker) {
            continue;
        }

        if (!is_ignorable_block_placeholder_node(child)) {
            return false;
        }
    }

    return true;
}

auto classify_bookmark_start_kind(pugi::xml_node bookmark_start)
    -> featherdoc::bookmark_kind {
    if (bookmark_start == pugi::xml_node{} ||
        std::string_view{bookmark_start.name()} != "w:bookmarkStart") {
        return featherdoc::bookmark_kind::malformed;
    }

    const auto start_parent = bookmark_start.parent();
    if (start_parent == pugi::xml_node{} || std::string_view{start_parent.name()} != "w:p") {
        return find_matching_bookmark_end_in_document_order(bookmark_start) != pugi::xml_node{}
                   ? featherdoc::bookmark_kind::text
                   : featherdoc::bookmark_kind::malformed;
    }

    if (const auto bookmark_end = find_matching_bookmark_end(bookmark_start);
        bookmark_end != pugi::xml_node{} && bookmark_end.parent() == start_parent) {
        if (!paragraph_is_block_placeholder(start_parent, bookmark_start, bookmark_end)) {
            return featherdoc::bookmark_kind::text;
        }

        const auto cell = start_parent.parent();
        const auto row = cell.parent();
        const auto table = row.parent();
        if (cell != pugi::xml_node{} && std::string_view{cell.name()} == "w:tc" &&
            row != pugi::xml_node{} && std::string_view{row.name()} == "w:tr" &&
            table != pugi::xml_node{} && std::string_view{table.name()} == "w:tbl") {
            return featherdoc::bookmark_kind::table_rows;
        }

        return featherdoc::bookmark_kind::block;
    }

    if (const auto bookmark_end = find_matching_bookmark_end_in_document_order(bookmark_start);
        bookmark_end != pugi::xml_node{}) {
        const auto end_parent = bookmark_end.parent();
        if (end_parent != pugi::xml_node{} && std::string_view{end_parent.name()} == "w:p" &&
            end_parent != start_parent && end_parent.parent() == start_parent.parent() &&
            paragraph_is_bookmark_marker(start_parent, bookmark_start) &&
            paragraph_is_bookmark_marker(end_parent, bookmark_end)) {
            return featherdoc::bookmark_kind::block_range;
        }

        return featherdoc::bookmark_kind::text;
    }

    return featherdoc::bookmark_kind::malformed;
}

auto summarize_bookmarks_in_part(featherdoc::document_error_info &last_error_info,
                                 pugi::xml_document &document)
    -> std::vector<featherdoc::bookmark_summary> {
    std::vector<pugi::xml_node> bookmark_starts;
    collect_bookmark_starts_in_order(document, bookmark_starts);

    std::vector<featherdoc::bookmark_summary> summaries;
    std::unordered_map<std::string, std::size_t> summary_indexes;
    for (const auto bookmark_start : bookmark_starts) {
        const auto bookmark_name =
            std::string_view{bookmark_start.attribute("w:name").value()};
        if (bookmark_name.empty()) {
            continue;
        }

        const auto kind = classify_bookmark_start_kind(bookmark_start);
        const auto [index_it, inserted] =
            summary_indexes.emplace(std::string{bookmark_name}, summaries.size());
        if (inserted) {
            summaries.push_back(
                {std::string{bookmark_name}, 0U, featherdoc::bookmark_kind::text});
        }

        auto &summary = summaries[index_it->second];
        ++summary.occurrence_count;
        if (summary.occurrence_count == 1U) {
            summary.kind = kind;
            continue;
        }

        if (summary.kind != kind) {
            summary.kind = featherdoc::bookmark_kind::mixed;
        }
    }

    last_error_info.clear();
    return summaries;
}

auto find_bookmark_in_part(featherdoc::document_error_info &last_error_info,
                           pugi::xml_document &document, std::string_view entry_name,
                           std::string_view bookmark_name)
    -> std::optional<featherdoc::bookmark_summary> {
    if (bookmark_name.empty()) {
        set_last_error(last_error_info, std::make_error_code(std::errc::invalid_argument),
                       "bookmark name must not be empty when reading bookmark metadata",
                       std::string{entry_name});
        return std::nullopt;
    }

    const auto summaries = summarize_bookmarks_in_part(last_error_info, document);
    for (const auto &summary : summaries) {
        if (summary.bookmark_name == bookmark_name) {
            last_error_info.clear();
            return summary;
        }
    }

    set_last_error(last_error_info, std::make_error_code(std::errc::invalid_argument),
                   "bookmark name '" + std::string{bookmark_name} +
                       "' was not found in " + std::string{entry_name},
                   std::string{entry_name});
    return std::nullopt;
}

auto content_control_kind_from_node(pugi::xml_node content_control)
    -> featherdoc::content_control_kind {
    const auto parent = content_control.parent();
    const auto parent_name = std::string_view{parent.name()};
    if (parent_name == "w:p") {
        return featherdoc::content_control_kind::run;
    }
    if (parent_name == "w:tbl") {
        return featherdoc::content_control_kind::table_row;
    }
    if (parent_name == "w:tr") {
        return featherdoc::content_control_kind::table_cell;
    }
    if (parent_name == "w:body" || parent_name == "w:hdr" ||
        parent_name == "w:ftr" || parent_name == "w:tc" ||
        parent_name == "w:sdtContent") {
        return featherdoc::content_control_kind::block;
    }

    return featherdoc::content_control_kind::unknown;
}

auto optional_child_attribute(pugi::xml_node parent, const char *child_name,
                              const char *attribute_name)
    -> std::optional<std::string> {
    const auto child = parent.child(child_name);
    if (child == pugi::xml_node{}) {
        return std::nullopt;
    }

    const auto value = std::string_view{child.attribute(attribute_name).value()};
    if (value.empty()) {
        return std::nullopt;
    }

    return std::string{value};
}

auto optional_attribute(pugi::xml_node node, const char *attribute_name)
    -> std::optional<std::string> {
    if (node == pugi::xml_node{}) {
        return std::nullopt;
    }

    const auto value = std::string_view{node.attribute(attribute_name).value()};
    if (value.empty()) {
        return std::nullopt;
    }

    return std::string{value};
}

auto optional_attribute_any(pugi::xml_node node,
                            std::initializer_list<const char *> attribute_names)
    -> std::optional<std::string> {
    for (const auto *attribute_name : attribute_names) {
        if (auto value = optional_attribute(node, attribute_name)) {
            return value;
        }
    }

    return std::nullopt;
}

auto optional_child_attribute_any(pugi::xml_node parent,
                                  std::initializer_list<const char *> child_names,
                                  std::initializer_list<const char *> attribute_names)
    -> std::optional<std::string> {
    for (const auto *child_name : child_names) {
        const auto child = parent.child(child_name);
        if (child == pugi::xml_node{}) {
            continue;
        }
        for (const auto *attribute_name : attribute_names) {
            if (auto value = optional_attribute(child, attribute_name)) {
                return value;
            }
        }
    }

    return std::nullopt;
}

auto parse_word_bool(std::string_view value) -> std::optional<bool> {
    if (value == "1" || value == "true" || value == "on") {
        return true;
    }
    if (value == "0" || value == "false" || value == "off") {
        return false;
    }
    return std::nullopt;
}

auto child_any(pugi::xml_node parent,
               std::initializer_list<const char *> child_names) -> pugi::xml_node {
    for (const auto *child_name : child_names) {
        const auto child = parent.child(child_name);
        if (child != pugi::xml_node{}) {
            return child;
        }
    }

    return {};
}

auto content_control_form_kind_from_properties(pugi::xml_node properties)
    -> featherdoc::content_control_form_kind {
    if (child_any(properties, {"w:checkBox", "w14:checkbox"}) !=
        pugi::xml_node{}) {
        return featherdoc::content_control_form_kind::checkbox;
    }
    if (properties.child("w:dropDownList") != pugi::xml_node{}) {
        return featherdoc::content_control_form_kind::drop_down_list;
    }
    if (properties.child("w:comboBox") != pugi::xml_node{}) {
        return featherdoc::content_control_form_kind::combo_box;
    }
    if (properties.child("w:date") != pugi::xml_node{}) {
        return featherdoc::content_control_form_kind::date;
    }
    if (properties.child("w:picture") != pugi::xml_node{}) {
        return featherdoc::content_control_form_kind::picture;
    }
    if (properties.child("w:text") != pugi::xml_node{}) {
        return featherdoc::content_control_form_kind::plain_text;
    }
    if (properties.child("w:repeatingSection") != pugi::xml_node{}) {
        return featherdoc::content_control_form_kind::repeating_section;
    }
    if (properties.child("w:group") != pugi::xml_node{}) {
        return featherdoc::content_control_form_kind::group;
    }
    if (properties.child("w:richText") != pugi::xml_node{}) {
        return featherdoc::content_control_form_kind::rich_text;
    }

    return featherdoc::content_control_form_kind::rich_text;
}

auto list_item_attribute(pugi::xml_node item,
                         std::initializer_list<const char *> attribute_names)
    -> std::string {
    for (const auto *attribute_name : attribute_names) {
        const auto value = std::string_view{item.attribute(attribute_name).value()};
        if (!value.empty()) {
            return std::string{value};
        }
    }

    return {};
}

void summarize_content_control_list_items(
    pugi::xml_node properties, std::string_view selected_text,
    featherdoc::content_control_summary &summary) {
    const auto list_parent =
        summary.form_kind == featherdoc::content_control_form_kind::drop_down_list
            ? properties.child("w:dropDownList")
            : properties.child("w:comboBox");
    if (list_parent == pugi::xml_node{}) {
        return;
    }

    for (auto item = list_parent.child("w:listItem"); item != pugi::xml_node{};
         item = item.next_sibling("w:listItem")) {
        auto list_item = featherdoc::content_control_list_item{};
        list_item.display_text = list_item_attribute(
            item, {"w:displayText", "displayText", "w14:displayText"});
        list_item.value =
            list_item_attribute(item, {"w:value", "value", "w14:value"});
        if (list_item.display_text.empty()) {
            list_item.display_text = list_item.value;
        }
        if (list_item.value.empty()) {
            list_item.value = list_item.display_text;
        }

        const auto item_index = summary.list_items.size();
        if (!summary.selected_list_item.has_value() &&
            (selected_text == list_item.display_text || selected_text == list_item.value)) {
            summary.selected_list_item = item_index;
        }
        summary.list_items.push_back(std::move(list_item));
    }
}

void collect_content_controls_in_order(
    pugi::xml_node node, std::vector<pugi::xml_node> &content_controls) {
    for (auto child = node.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        if (std::string_view{child.name()} == "w:sdt") {
            content_controls.push_back(child);
        }
        collect_content_controls_in_order(child, content_controls);
    }
}

auto summarize_content_control(pugi::xml_node content_control,
                               std::size_t index)
    -> featherdoc::content_control_summary {
    auto summary = featherdoc::content_control_summary{};
    summary.index = index;
    summary.kind = content_control_kind_from_node(content_control);

    const auto properties = content_control.child("w:sdtPr");
    summary.form_kind = content_control_form_kind_from_properties(properties);
    summary.tag = optional_child_attribute(properties, "w:tag", "w:val");
    summary.alias = optional_child_attribute(properties, "w:alias", "w:val");
    summary.id = optional_child_attribute(properties, "w:id", "w:val");
    summary.lock = optional_child_attribute(properties, "w:lock", "w:val");
    if (const auto data_binding = properties.child("w:dataBinding");
        data_binding != pugi::xml_node{}) {
        summary.data_binding_store_item_id = optional_attribute_any(
            data_binding, {"w:storeItemID", "storeItemID"});
        summary.data_binding_xpath =
            optional_attribute_any(data_binding, {"w:xpath", "xpath"});
        summary.data_binding_prefix_mappings = optional_attribute_any(
            data_binding, {"w:prefixMappings", "prefixMappings"});
    }
    summary.showing_placeholder =
        properties.child("w:showingPlcHdr") != pugi::xml_node{};

    const auto content = content_control.child("w:sdtContent");
    summary.text = featherdoc::detail::collect_plain_text_from_xml(
        content != pugi::xml_node{} ? content : content_control);

    if (summary.form_kind == featherdoc::content_control_form_kind::checkbox) {
        const auto checkbox = child_any(properties, {"w:checkBox", "w14:checkbox"});
        if (auto checked = optional_child_attribute_any(
                checkbox, {"w:checked", "w14:checked"}, {"w:val", "w14:val"})) {
            summary.checked = parse_word_bool(*checked);
        }
    }

    if (summary.form_kind == featherdoc::content_control_form_kind::date) {
        const auto date = properties.child("w:date");
        summary.date_format =
            optional_child_attribute(date, "w:dateFormat", "w:val");
        summary.date_locale = optional_child_attribute(date, "w:lid", "w:val");
    }

    if (summary.form_kind == featherdoc::content_control_form_kind::drop_down_list ||
        summary.form_kind == featherdoc::content_control_form_kind::combo_box) {
        summarize_content_control_list_items(properties, summary.text, summary);
    }

    return summary;
}

auto summarize_content_controls_in_part(
    featherdoc::document_error_info &last_error_info,
    pugi::xml_document &document)
    -> std::vector<featherdoc::content_control_summary> {
    std::vector<pugi::xml_node> content_control_nodes;
    collect_content_controls_in_order(document, content_control_nodes);

    auto summaries = std::vector<featherdoc::content_control_summary>{};
    summaries.reserve(content_control_nodes.size());
    for (std::size_t index = 0; index < content_control_nodes.size();
         ++index) {
        summaries.push_back(
            summarize_content_control(content_control_nodes[index], index));
    }

    last_error_info.clear();
    return summaries;
}

auto filter_content_controls_by_tag_or_alias(
    featherdoc::document_error_info &last_error_info,
    pugi::xml_document &document, std::string_view entry_name,
    std::string_view value, bool match_tag)
    -> std::vector<featherdoc::content_control_summary> {
    if (value.empty()) {
        set_last_error(
            last_error_info, std::make_error_code(std::errc::invalid_argument),
            match_tag ? "content control tag must not be empty"
                      : "content control alias must not be empty",
            std::string{entry_name});
        return {};
    }

    const auto summaries = summarize_content_controls_in_part(last_error_info,
                                                             document);
    auto matches = std::vector<featherdoc::content_control_summary>{};
    for (const auto &summary : summaries) {
        const auto &candidate = match_tag ? summary.tag : summary.alias;
        if (candidate.has_value() && *candidate == value) {
            matches.push_back(summary);
        }
    }

    last_error_info.clear();
    return matches;
}

void clear_node_children(pugi::xml_node node) {
    for (auto child = node.first_child(); child != pugi::xml_node{};) {
        const auto next = child.next_sibling();
        node.remove_child(child);
        child = next;
    }
}

auto ensure_content_control_content_node(pugi::xml_node content_control)
    -> pugi::xml_node {
    auto content = content_control.child("w:sdtContent");
    if (content != pugi::xml_node{}) {
        return content;
    }

    return content_control.append_child("w:sdtContent");
}

auto append_text_table_cell(pugi::xml_node parent, std::string_view text)
    -> bool {
    auto cell = parent.append_child("w:tc");
    if (cell == pugi::xml_node{}) {
        return false;
    }

    return featherdoc::detail::append_plain_text_paragraph(cell, text);
}

void clear_content_control_placeholder_flag(pugi::xml_node content_control) {
    if (auto properties = content_control.child("w:sdtPr");
        properties != pugi::xml_node{}) {
        for (auto placeholder = properties.child("w:showingPlcHdr");
             placeholder != pugi::xml_node{};) {
            const auto next = placeholder.next_sibling("w:showingPlcHdr");
            properties.remove_child(placeholder);
            placeholder = next;
        }
    }
}

auto rewrite_content_control_plain_text(pugi::xml_node content_control,
                                        std::string_view replacement)
    -> bool {
    if (content_control == pugi::xml_node{}) {
        return false;
    }

    auto content = ensure_content_control_content_node(content_control);
    if (content == pugi::xml_node{}) {
        return false;
    }

    const auto kind = content_control_kind_from_node(content_control);
    clear_node_children(content);

    switch (kind) {
    case featherdoc::content_control_kind::run: {
        auto run = content.append_child("w:r");
        if (run == pugi::xml_node{}) {
            return false;
        }
        if (!featherdoc::detail::set_plain_text_run_content(run, replacement)) {
            return false;
        }
        break;
    }
    case featherdoc::content_control_kind::table_row: {
        auto row = content.append_child("w:tr");
        if (row == pugi::xml_node{} || !append_text_table_cell(row, replacement)) {
            return false;
        }
        break;
    }
    case featherdoc::content_control_kind::table_cell:
        if (!append_text_table_cell(content, replacement)) {
            return false;
        }
        break;
    case featherdoc::content_control_kind::block:
    case featherdoc::content_control_kind::unknown:
        if (!featherdoc::detail::append_plain_text_paragraph(content,
                                                            replacement)) {
            return false;
        }
        break;
    }

    if (auto properties = content_control.child("w:sdtPr");
        properties != pugi::xml_node{}) {
        for (auto placeholder = properties.child("w:showingPlcHdr");
             placeholder != pugi::xml_node{};) {
            const auto next = placeholder.next_sibling("w:showingPlcHdr");
            properties.remove_child(placeholder);
            placeholder = next;
        }
    }

    return true;
}


struct custom_xml_package_part {
    std::string entry_name;
    std::string store_item_id;
    pugi::xml_document xml;
};

auto ascii_lower_copy(std::string_view value) -> std::string {
    std::string lowered;
    lowered.reserve(value.size());
    for (const char ch : value) {
        lowered.push_back(static_cast<char>(
            std::tolower(static_cast<unsigned char>(ch))));
    }
    return lowered;
}

auto starts_with_view(std::string_view value, std::string_view prefix) -> bool {
    return value.size() >= prefix.size() && value.substr(0U, prefix.size()) == prefix;
}

auto ends_with_view(std::string_view value, std::string_view suffix) -> bool {
    return value.size() >= suffix.size() &&
           value.substr(value.size() - suffix.size()) == suffix;
}

auto xml_local_name(std::string_view qualified_name) -> std::string_view {
    const auto colon = qualified_name.find(':');
    if (colon == std::string_view::npos) {
        return qualified_name;
    }
    return qualified_name.substr(colon + 1U);
}

auto clean_xpath_segment(std::string_view segment) -> std::string_view {
    const auto bracket = segment.find('[');
    if (bracket != std::string_view::npos) {
        segment = segment.substr(0U, bracket);
    }
    return segment;
}

auto normalize_store_item_id(std::string_view store_item_id) -> std::string {
    return ascii_lower_copy(store_item_id);
}

auto read_zip_text_entry(zip_t *archive, const std::string &entry_name,
                         std::string &content) -> bool {
    content.clear();
    if (archive == nullptr || zip_entry_open(archive, entry_name.c_str()) != 0) {
        return false;
    }

    void *buffer = nullptr;
    size_t buffer_size = 0U;
    const auto read_result = zip_entry_read(archive, &buffer, &buffer_size);
    if (read_result < 0) {
        zip_entry_close(archive);
        return false;
    }

    if (buffer != nullptr && buffer_size > 0U) {
        content.assign(static_cast<const char *>(buffer), buffer_size);
    }
    std::free(buffer);
    return zip_entry_close(archive) == 0;
}

auto collect_custom_xml_item_entries(zip_t *archive) -> std::vector<std::string> {
    std::vector<std::string> entries;
    if (archive == nullptr) {
        return entries;
    }

    const auto total_entries = zip_entries_total(archive);
    if (total_entries < 0) {
        return entries;
    }

    for (ssize_t index = 0; index < total_entries; ++index) {
        if (zip_entry_openbyindex(archive, static_cast<std::size_t>(index)) != 0) {
            continue;
        }
        const char *entry_name = zip_entry_name(archive);
        if (entry_name != nullptr) {
            const std::string_view name{entry_name};
            const auto slash = name.find_last_of('/');
            const auto file_name = slash == std::string_view::npos
                                       ? name
                                       : name.substr(slash + 1U);
            if (starts_with_view(name, "customXml/") &&
                name.find("/_rels/") == std::string_view::npos &&
                starts_with_view(file_name, "item") &&
                !starts_with_view(file_name, "itemProps") &&
                ends_with_view(file_name, ".xml")) {
                entries.emplace_back(name);
            }
        }
        zip_entry_close(archive);
    }

    return entries;
}

auto custom_xml_relationships_entry(std::string_view item_entry_name) -> std::string {
    const auto slash = item_entry_name.find_last_of('/');
    if (slash == std::string_view::npos) {
        return "_rels/" + std::string{item_entry_name} + ".rels";
    }
    return std::string{item_entry_name.substr(0U, slash + 1U)} + "_rels/" +
           std::string{item_entry_name.substr(slash + 1U)} + ".rels";
}

auto resolve_custom_xml_relationship_target(std::string_view item_entry_name,
                                            std::string_view target) -> std::string {
    if (target.empty()) {
        return {};
    }
    if (target.front() == '/') {
        return std::string{target.substr(1U)};
    }

    std::string base;
    const auto slash = item_entry_name.find_last_of('/');
    if (slash != std::string_view::npos) {
        base = std::string{item_entry_name.substr(0U, slash + 1U)};
    }

    std::string remaining{target};
    while (starts_with_view(remaining, "../")) {
        remaining.erase(0U, 3U);
        if (!base.empty() && base.back() == '/') {
            base.pop_back();
        }
        const auto parent_slash = base.find_last_of('/');
        base = parent_slash == std::string::npos ? std::string{} : base.substr(0U, parent_slash + 1U);
    }

    return base + remaining;
}

auto custom_xml_props_entry_from_relationships(zip_t *archive,
                                               const std::string &item_entry_name)
    -> std::optional<std::string> {
    const auto relationships_entry = custom_xml_relationships_entry(item_entry_name);
    std::string relationships_text;
    if (!read_zip_text_entry(archive, relationships_entry, relationships_text)) {
        return std::nullopt;
    }

    pugi::xml_document relationships;
    if (!relationships.load_buffer(relationships_text.data(), relationships_text.size())) {
        return std::nullopt;
    }

    const auto root = relationships.child("Relationships");
    for (auto relationship = root.child("Relationship");
         relationship != pugi::xml_node{};
         relationship = relationship.next_sibling("Relationship")) {
        const auto type = std::string_view{relationship.attribute("Type").value()};
        if (!ends_with_view(type, "/customXmlProps")) {
            continue;
        }
        const auto target = std::string_view{relationship.attribute("Target").value()};
        const auto resolved = resolve_custom_xml_relationship_target(item_entry_name, target);
        if (!resolved.empty()) {
            return resolved;
        }
    }

    return std::nullopt;
}

auto infer_custom_xml_props_entry(std::string_view item_entry_name)
    -> std::optional<std::string> {
    const auto slash = item_entry_name.find_last_of('/');
    const auto directory = slash == std::string_view::npos
                               ? std::string_view{}
                               : item_entry_name.substr(0U, slash + 1U);
    const auto file_name = slash == std::string_view::npos
                               ? item_entry_name
                               : item_entry_name.substr(slash + 1U);
    if (!starts_with_view(file_name, "item") || !ends_with_view(file_name, ".xml")) {
        return std::nullopt;
    }
    const auto number = file_name.substr(4U, file_name.size() - 4U - 4U);
    if (number.empty()) {
        return std::nullopt;
    }
    return std::string{directory} + "itemProps" + std::string{number} + ".xml";
}

auto custom_xml_store_item_id_from_props(std::string_view props_xml)
    -> std::optional<std::string> {
    pugi::xml_document props;
    if (!props.load_buffer(props_xml.data(), props_xml.size())) {
        return std::nullopt;
    }
    const auto root = props.document_element();
    if (root == pugi::xml_node{}) {
        return std::nullopt;
    }
    return optional_attribute_any(root, {"ds:itemID", "itemID"});
}

auto load_custom_xml_package_parts(
    const std::filesystem::path &document_path,
    featherdoc::document_error_info &last_error_info)
    -> std::optional<std::unordered_map<std::string, custom_xml_package_part>> {
    std::unordered_map<std::string, custom_xml_package_part> parts;
    if (document_path.empty()) {
        return parts;
    }

    int zip_error = 0;
    zip_t *archive = zip_openwitherror(document_path.string().c_str(),
                                       ZIP_DEFAULT_COMPRESSION_LEVEL, 'r',
                                       &zip_error);
    if (archive == nullptr) {
        set_last_error(last_error_info, featherdoc::document_errc::archive_open_failed,
                       "failed to open archive for Custom XML sync: " +
                           std::to_string(zip_error));
        return std::nullopt;
    }

    const auto item_entries = collect_custom_xml_item_entries(archive);
    for (const auto &item_entry_name : item_entries) {
        auto props_entry_name =
            custom_xml_props_entry_from_relationships(archive, item_entry_name);
        if (!props_entry_name.has_value()) {
            props_entry_name = infer_custom_xml_props_entry(item_entry_name);
        }
        if (!props_entry_name.has_value()) {
            continue;
        }

        std::string props_xml;
        if (!read_zip_text_entry(archive, *props_entry_name, props_xml)) {
            continue;
        }
        auto store_item_id = custom_xml_store_item_id_from_props(props_xml);
        if (!store_item_id.has_value() || store_item_id->empty()) {
            continue;
        }

        std::string item_xml;
        if (!read_zip_text_entry(archive, item_entry_name, item_xml)) {
            continue;
        }
        pugi::xml_document item_document;
        if (!item_document.load_buffer(item_xml.data(), item_xml.size())) {
            continue;
        }

        custom_xml_package_part part;
        part.entry_name = item_entry_name;
        part.store_item_id = *store_item_id;
        part.xml.reset(item_document);
        parts[normalize_store_item_id(part.store_item_id)] = std::move(part);
    }

    zip_close(archive);
    return parts;
}

void collect_custom_xml_plain_text(std::string &text, pugi::xml_node node) {
    for (auto child = node.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        const auto type = child.type();
        if (type == pugi::node_pcdata || type == pugi::node_cdata) {
            text += child.value();
            continue;
        }
        collect_custom_xml_plain_text(text, child);
    }
}

auto custom_xml_node_text(pugi::xml_node node) -> std::string {
    std::string text;
    collect_custom_xml_plain_text(text, node);
    return text;
}

auto find_child_by_local_name(pugi::xml_node parent, std::string_view local_name)
    -> pugi::xml_node {
    for (auto child = parent.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        if (xml_local_name(child.name()) == local_name) {
            return child;
        }
    }
    return {};
}

auto find_attribute_by_local_name(pugi::xml_node node, std::string_view local_name)
    -> std::optional<std::string> {
    for (auto attribute = node.first_attribute(); attribute != pugi::xml_attribute{};
         attribute = attribute.next_attribute()) {
        if (xml_local_name(attribute.name()) == local_name) {
            return std::string{attribute.value()};
        }
    }
    return std::nullopt;
}

auto custom_xml_value_for_xpath(const custom_xml_package_part &part,
                                std::string_view xpath) -> std::optional<std::string> {
    if (xpath.empty() || xpath.front() != '/') {
        return std::nullopt;
    }

    std::vector<std::string_view> segments;
    std::size_t offset = 1U;
    while (offset <= xpath.size()) {
        const auto slash = xpath.find('/', offset);
        const auto end = slash == std::string_view::npos ? xpath.size() : slash;
        auto segment = clean_xpath_segment(xpath.substr(offset, end - offset));
        if (!segment.empty()) {
            segments.push_back(segment);
        }
        if (slash == std::string_view::npos) {
            break;
        }
        offset = slash + 1U;
    }
    if (segments.empty()) {
        return std::nullopt;
    }

    auto current = part.xml.document_element();
    if (current == pugi::xml_node{}) {
        return std::nullopt;
    }

    std::size_t segment_index = 0U;
    if (xml_local_name(current.name()) == xml_local_name(segments.front())) {
        segment_index = 1U;
    }

    for (; segment_index < segments.size(); ++segment_index) {
        const auto segment = segments[segment_index];
        if (segment == "text()") {
            return custom_xml_node_text(current);
        }
        if (!segment.empty() && segment.front() == '@') {
            return find_attribute_by_local_name(
                current, xml_local_name(segment.substr(1U)));
        }
        current = find_child_by_local_name(current, xml_local_name(segment));
        if (current == pugi::xml_node{}) {
            return std::nullopt;
        }
    }

    return custom_xml_node_text(current);
}

void append_custom_xml_sync_issue(
    featherdoc::custom_xml_data_binding_sync_result &result,
    const std::string &part_entry_name,
    const featherdoc::content_control_summary &summary,
    std::string_view reason) {
    featherdoc::custom_xml_data_binding_sync_issue issue;
    issue.part_entry_name = part_entry_name;
    issue.content_control_index = summary.index;
    issue.tag = summary.tag;
    issue.alias = summary.alias;
    issue.store_item_id = summary.data_binding_store_item_id.value_or(std::string{});
    issue.xpath = summary.data_binding_xpath.value_or(std::string{});
    issue.reason = std::string{reason};
    result.issues.push_back(std::move(issue));
}

bool sync_content_controls_from_custom_xml_in_part(
    featherdoc::document_error_info &last_error_info, pugi::xml_document &document,
    std::string_view entry_name,
    const std::unordered_map<std::string, custom_xml_package_part> &custom_xml_parts,
    featherdoc::custom_xml_data_binding_sync_result &result) {
    std::vector<pugi::xml_node> content_controls;
    collect_content_controls_in_order(document, content_controls);

    for (std::size_t index = 0U; index < content_controls.size(); ++index) {
        const auto content_control = content_controls[index];
        auto summary = summarize_content_control(content_control, index);
        ++result.scanned_content_controls;

        if (!summary.data_binding_store_item_id.has_value() ||
            summary.data_binding_store_item_id->empty() ||
            !summary.data_binding_xpath.has_value() ||
            summary.data_binding_xpath->empty()) {
            continue;
        }

        ++result.bound_content_controls;
        const auto part_it = custom_xml_parts.find(
            normalize_store_item_id(*summary.data_binding_store_item_id));
        if (part_it == custom_xml_parts.end()) {
            append_custom_xml_sync_issue(result, std::string{entry_name}, summary,
                                         "custom_xml_part_not_found");
            continue;
        }

        const auto value = custom_xml_value_for_xpath(part_it->second,
                                                      *summary.data_binding_xpath);
        if (!value.has_value()) {
            append_custom_xml_sync_issue(result, std::string{entry_name}, summary,
                                         "custom_xml_value_not_found");
            continue;
        }

        if (!rewrite_content_control_plain_text(content_control, *value)) {
            set_last_error(last_error_info,
                           std::make_error_code(std::errc::not_enough_memory),
                           "failed to synchronize content control text from Custom XML",
                           std::string{entry_name});
            return false;
        }

        featherdoc::custom_xml_data_binding_sync_item item;
        item.part_entry_name = std::string{entry_name};
        item.content_control_index = summary.index;
        item.tag = summary.tag;
        item.alias = summary.alias;
        item.store_item_id = *summary.data_binding_store_item_id;
        item.xpath = *summary.data_binding_xpath;
        item.previous_text = summary.text;
        item.value = *value;
        result.synced_items.push_back(std::move(item));
        ++result.synced_content_controls;
    }

    return true;
}

auto content_control_matches_tag_or_alias(pugi::xml_node content_control,
                                             std::string_view value,
                                             bool match_tag) -> bool {
    const auto properties = content_control.child("w:sdtPr");
    const auto candidate = optional_child_attribute(
        properties, match_tag ? "w:tag" : "w:alias", "w:val");
    return candidate.has_value() && *candidate == value;
}

auto ensure_content_control_properties_node(pugi::xml_node content_control)
    -> pugi::xml_node {
    auto properties = content_control.child("w:sdtPr");
    if (properties != pugi::xml_node{}) {
        return properties;
    }

    return content_control.prepend_child("w:sdtPr");
}

void clear_content_control_lock(pugi::xml_node properties) {
    for (auto lock = properties.child("w:lock"); lock != pugi::xml_node{};) {
        const auto next = lock.next_sibling("w:lock");
        properties.remove_child(lock);
        lock = next;
    }
}

bool set_content_control_lock(pugi::xml_node properties,
                              std::string_view lock_value) {
    clear_content_control_lock(properties);
    auto lock = properties.append_child("w:lock");
    if (lock == pugi::xml_node{}) {
        return false;
    }
    ensure_attribute_value(lock, "w:val", lock_value);
    return true;
}

void clear_content_control_data_binding(pugi::xml_node properties) {
    for (auto data_binding = properties.child("w:dataBinding");
         data_binding != pugi::xml_node{};) {
        const auto next = data_binding.next_sibling("w:dataBinding");
        properties.remove_child(data_binding);
        data_binding = next;
    }
}

auto content_control_data_binding_options_have_value(
    const featherdoc::content_control_form_state_options &options) -> bool {
    return options.data_binding_store_item_id.has_value() ||
           options.data_binding_xpath.has_value() ||
           options.data_binding_prefix_mappings.has_value();
}

auto validate_content_control_data_binding_options(
    const featherdoc::content_control_form_state_options &options,
    std::string &detail) -> bool {
    if (!content_control_data_binding_options_have_value(options)) {
        return true;
    }

    if (!options.data_binding_store_item_id.has_value() ||
        options.data_binding_store_item_id->empty() ||
        !options.data_binding_xpath.has_value() ||
        options.data_binding_xpath->empty()) {
        detail =
            "content control data binding requires non-empty store item id and xpath";
        return false;
    }

    if (options.data_binding_prefix_mappings.has_value() &&
        options.data_binding_prefix_mappings->empty()) {
        detail =
            "content control data binding prefix mappings must not be empty";
        return false;
    }

    return true;
}

bool set_content_control_data_binding(
    pugi::xml_node properties,
    const featherdoc::content_control_form_state_options &options) {
    clear_content_control_data_binding(properties);
    auto data_binding = properties.append_child("w:dataBinding");
    if (data_binding == pugi::xml_node{}) {
        return false;
    }

    ensure_attribute_value(data_binding, "w:storeItemID",
                           *options.data_binding_store_item_id);
    ensure_attribute_value(data_binding, "w:xpath", *options.data_binding_xpath);
    if (options.data_binding_prefix_mappings.has_value()) {
        ensure_attribute_value(data_binding, "w:prefixMappings",
                               *options.data_binding_prefix_mappings);
    }
    return true;
}

auto find_checkbox_node(pugi::xml_node properties) -> pugi::xml_node {
    return child_any(properties, {"w14:checkbox", "w:checkBox", "w:checkBox"});
}

bool set_content_control_checkbox_state(pugi::xml_node content_control,
                                        pugi::xml_node properties,
                                        bool checked) {
    auto checkbox = find_checkbox_node(properties);
    if (checkbox == pugi::xml_node{}) {
        return false;
    }

    const auto checkbox_name = std::string_view{checkbox.name()};
    const auto checked_node_name = checkbox_name.rfind("w14:", 0) == 0
                                       ? "w14:checked"
                                       : "w:checked";
    const auto checked_attribute_name = checkbox_name.rfind("w14:", 0) == 0
                                            ? "w14:val"
                                            : "w:val";
    auto checked_node = checkbox.child(checked_node_name);
    if (checked_node == pugi::xml_node{}) {
        checked_node = checkbox.append_child(checked_node_name);
    }
    if (checked_node == pugi::xml_node{}) {
        return false;
    }

    ensure_attribute_value(checked_node, checked_attribute_name,
                           checked ? "1" : "0");
    return rewrite_content_control_plain_text(content_control,
                                              checked ? "\xE2\x98\x92"
                                                      : "\xE2\x98\x90");
}

auto find_content_control_list_parent(pugi::xml_node properties)
    -> pugi::xml_node {
    const auto drop_down = properties.child("w:dropDownList");
    if (drop_down != pugi::xml_node{}) {
        return drop_down;
    }

    return properties.child("w:comboBox");
}

bool set_content_control_selected_item(
    featherdoc::document_error_info &last_error_info,
    pugi::xml_node content_control, pugi::xml_node properties,
    std::string_view entry_name, std::string_view selected_item) {
    const auto list_parent = find_content_control_list_parent(properties);
    if (list_parent == pugi::xml_node{}) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "content control is not a drop-down or combo box",
                       std::string{entry_name});
        return false;
    }

    auto selected_display_text = std::string{};
    for (auto item = list_parent.child("w:listItem"); item != pugi::xml_node{};
         item = item.next_sibling("w:listItem")) {
        const auto display_text = list_item_attribute(
            item, {"w:displayText", "displayText", "w14:displayText"});
        const auto value =
            list_item_attribute(item, {"w:value", "value", "w14:value"});
        if (selected_item == display_text || selected_item == value) {
            selected_display_text = display_text.empty() ? value : display_text;
            break;
        }
    }

    if (selected_display_text.empty() &&
        std::string_view{list_parent.name()} == "w:comboBox") {
        selected_display_text = std::string{selected_item};
    }

    if (selected_display_text.empty()) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "selected content-control list item was not found",
                       std::string{entry_name});
        return false;
    }

    if (!rewrite_content_control_plain_text(content_control, selected_display_text)) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to rewrite content-control list value",
                       std::string{entry_name});
        return false;
    }

    return true;
}

bool set_content_control_date_state(
    featherdoc::document_error_info &last_error_info,
    pugi::xml_node content_control, pugi::xml_node properties,
    std::string_view entry_name,
    const featherdoc::content_control_form_state_options &options) {
    auto date = properties.child("w:date");
    if (date == pugi::xml_node{}) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "content control is not a date picker",
                       std::string{entry_name});
        return false;
    }

    if (options.date_format.has_value()) {
        auto date_format = date.child("w:dateFormat");
        if (date_format == pugi::xml_node{}) {
            date_format = date.append_child("w:dateFormat");
        }
        if (date_format == pugi::xml_node{}) {
            return false;
        }
        ensure_attribute_value(date_format, "w:val", *options.date_format);
    }

    if (options.date_locale.has_value()) {
        auto locale = date.child("w:lid");
        if (locale == pugi::xml_node{}) {
            locale = date.append_child("w:lid");
        }
        if (locale == pugi::xml_node{}) {
            return false;
        }
        ensure_attribute_value(locale, "w:val", *options.date_locale);
    }

    if (options.date_text.has_value() &&
        !rewrite_content_control_plain_text(content_control, *options.date_text)) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to rewrite content-control date text",
                       std::string{entry_name});
        return false;
    }

    return true;
}

auto content_control_form_state_has_changes(
    const featherdoc::content_control_form_state_options &options) -> bool {
    return options.lock.has_value() || options.clear_lock ||
           options.clear_data_binding ||
           content_control_data_binding_options_have_value(options) ||
           options.checked.has_value() || options.selected_list_item.has_value() ||
           options.date_text.has_value() || options.date_format.has_value() ||
           options.date_locale.has_value();
}

bool apply_content_control_form_state(
    featherdoc::document_error_info &last_error_info,
    pugi::xml_node content_control, std::string_view entry_name,
    const featherdoc::content_control_form_state_options &options) {
    auto properties = ensure_content_control_properties_node(content_control);
    if (properties == pugi::xml_node{}) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to create content-control properties",
                       std::string{entry_name});
        return false;
    }

    if (options.clear_lock) {
        clear_content_control_lock(properties);
    }
    if (options.lock.has_value() &&
        !set_content_control_lock(properties, *options.lock)) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to set content-control lock",
                       std::string{entry_name});
        return false;
    }

    if (options.clear_data_binding) {
        clear_content_control_data_binding(properties);
    }
    if (content_control_data_binding_options_have_value(options) &&
        !set_content_control_data_binding(properties, options)) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to set content-control data binding",
                       std::string{entry_name});
        return false;
    }

    if (options.checked.has_value() &&
        !set_content_control_checkbox_state(content_control, properties,
                                            *options.checked)) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "content control is not a checkbox",
                       std::string{entry_name});
        return false;
    }

    if (options.selected_list_item.has_value() &&
        !set_content_control_selected_item(last_error_info, content_control,
                                           properties, entry_name,
                                           *options.selected_list_item)) {
        return false;
    }

    if ((options.date_text.has_value() || options.date_format.has_value() ||
         options.date_locale.has_value()) &&
        !set_content_control_date_state(last_error_info, content_control,
                                        properties, entry_name, options)) {
        return false;
    }

    clear_content_control_placeholder_flag(content_control);
    return true;
}

bool set_content_control_form_state_by_tag_or_alias_in_node(
    featherdoc::document_error_info &last_error_info, pugi::xml_node node,
    std::string_view entry_name, std::string_view value,
    const featherdoc::content_control_form_state_options &options,
    bool match_tag, std::size_t &updated) {
    for (auto child = node.first_child(); child != pugi::xml_node{};) {
        const auto next = child.next_sibling();
        if (std::string_view{child.name()} == "w:sdt" &&
            content_control_matches_tag_or_alias(child, value, match_tag)) {
            if (!apply_content_control_form_state(last_error_info, child,
                                                  entry_name, options)) {
                return false;
            }
            ++updated;
            child = next;
            continue;
        }

        if (!set_content_control_form_state_by_tag_or_alias_in_node(
                last_error_info, child, entry_name, value, options, match_tag,
                updated)) {
            return false;
        }
        child = next;
    }

    return true;
}

auto set_content_control_form_state_by_tag_or_alias_in_part(
    featherdoc::document_error_info &last_error_info,
    pugi::xml_document &document, std::string_view entry_name,
    std::string_view value,
    const featherdoc::content_control_form_state_options &options,
    bool match_tag) -> std::size_t {
    if (value.empty()) {
        set_last_error(
            last_error_info, std::make_error_code(std::errc::invalid_argument),
            match_tag ? "content control tag must not be empty"
                      : "content control alias must not be empty",
            std::string{entry_name});
        return 0U;
    }
    if (!content_control_form_state_has_changes(options)) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "content control form-state options must include at least one change",
                       std::string{entry_name});
        return 0U;
    }
    if (options.clear_lock && options.lock.has_value()) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "content control lock cannot be set and cleared together",
                       std::string{entry_name});
        return 0U;
    }
    if (options.clear_data_binding &&
        content_control_data_binding_options_have_value(options)) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "content control data binding cannot be set and cleared together",
                       std::string{entry_name});
        return 0U;
    }
    std::string data_binding_detail;
    if (!validate_content_control_data_binding_options(options,
                                                       data_binding_detail)) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       std::move(data_binding_detail), std::string{entry_name});
        return 0U;
    }

    std::size_t updated = 0U;
    if (!set_content_control_form_state_by_tag_or_alias_in_node(
            last_error_info, document, entry_name, value, options, match_tag,
            updated)) {
        return updated;
    }

    last_error_info.clear();
    return updated;
}

bool replace_content_control_text_by_tag_or_alias_in_node(
    featherdoc::document_error_info &last_error_info, pugi::xml_node node,
    std::string_view entry_name, std::string_view value,
    std::string_view replacement, bool match_tag, std::size_t &replaced) {
    for (auto child = node.first_child(); child != pugi::xml_node{};) {
        const auto next = child.next_sibling();
        if (std::string_view{child.name()} == "w:sdt" &&
            content_control_matches_tag_or_alias(child, value, match_tag)) {
            if (!rewrite_content_control_plain_text(child, replacement)) {
                set_last_error(
                    last_error_info,
                    std::make_error_code(std::errc::not_enough_memory),
                    "failed to rewrite content control text",
                    std::string{entry_name});
                return false;
            }
            ++replaced;
            child = next;
            continue;
        }

        if (!replace_content_control_text_by_tag_or_alias_in_node(
                last_error_info, child, entry_name, value, replacement,
                match_tag, replaced)) {
            return false;
        }
        child = next;
    }

    return true;
}

auto replace_content_control_text_by_tag_or_alias_in_part(
    featherdoc::document_error_info &last_error_info,
    pugi::xml_document &document, std::string_view entry_name,
    std::string_view value, std::string_view replacement, bool match_tag)
    -> std::size_t {
    if (value.empty()) {
        set_last_error(
            last_error_info, std::make_error_code(std::errc::invalid_argument),
            match_tag ? "content control tag must not be empty"
                      : "content control alias must not be empty",
            std::string{entry_name});
        return 0U;
    }

    std::size_t replaced = 0U;
    if (!replace_content_control_text_by_tag_or_alias_in_node(
            last_error_info, document, entry_name, value, replacement,
            match_tag, replaced)) {
        return replaced;
    }

    last_error_info.clear();
    return replaced;
}

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

auto collect_table_row_bookmark_placeholders(
    featherdoc::document_error_info &last_error_info, pugi::xml_document &document,
    std::string_view entry_name,
    std::string_view bookmark_name, std::vector<table_row_bookmark_placeholder> &placeholders)
    -> bool {
    std::vector<block_bookmark_placeholder> block_placeholders;
    if (!collect_block_bookmark_placeholders(last_error_info, document, entry_name,
                                             bookmark_name,
                                             block_placeholders)) {
        return false;
    }

    for (const auto &placeholder : block_placeholders) {
        const auto cell = placeholder.paragraph.parent();
        if (cell == pugi::xml_node{} || std::string_view{cell.name()} != "w:tc") {
            set_last_error(last_error_info, std::make_error_code(std::errc::invalid_argument),
                           "table row replacement requires the bookmark paragraph to live "
                           "inside a table cell",
                           std::string{entry_name});
            return false;
        }

        const auto row = cell.parent();
        if (row == pugi::xml_node{} || std::string_view{row.name()} != "w:tr") {
            set_last_error(last_error_info, std::make_error_code(std::errc::invalid_argument),
                           "table row replacement requires the bookmark paragraph to live "
                           "inside a table row",
                           std::string{entry_name});
            return false;
        }

        const auto table = row.parent();
        if (table == pugi::xml_node{} || std::string_view{table.name()} != "w:tbl") {
            set_last_error(last_error_info, std::make_error_code(std::errc::invalid_argument),
                           "table row replacement requires the bookmark row to belong to a "
                           "table",
                           std::string{entry_name});
            return false;
        }

        for (const auto &existing : placeholders) {
            if (existing.row == row) {
                set_last_error(last_error_info,
                               std::make_error_code(std::errc::invalid_argument),
                               "table row replacement requires each template row to contain "
                               "the target bookmark at most once",
                               std::string{entry_name});
                return false;
            }
        }

        const auto cell_count = count_named_children(row, "w:tc");
        if (cell_count == 0U) {
            set_last_error(last_error_info, std::make_error_code(std::errc::invalid_argument),
                           "table row replacement requires the template row to contain at "
                           "least one cell",
                           std::string{entry_name});
            return false;
        }

        placeholders.push_back({table, row, cell_count});
    }

    return true;
}

auto replace_placeholder_with_table_rows(
    featherdoc::document_error_info &last_error_info,
    std::string_view entry_name, table_row_bookmark_placeholder placeholder,
    const std::vector<std::vector<std::string>> &rows) -> bool {
    if (placeholder.table == pugi::xml_node{} || placeholder.row == pugi::xml_node{}) {
        set_last_error(last_error_info, std::make_error_code(std::errc::invalid_argument),
                       "failed to resolve the placeholder row for table row replacement",
                       std::string{entry_name});
        return false;
    }

    if (placeholder.row.parent() != placeholder.table) {
        set_last_error(last_error_info, std::make_error_code(std::errc::invalid_argument),
                       "failed to resolve the placeholder row parent table",
                       std::string{entry_name});
        return false;
    }

    for (const auto &row_values : rows) {
        if (row_values.size() != placeholder.cell_count) {
            set_last_error(
                last_error_info, std::make_error_code(std::errc::invalid_argument),
                "replacement table row cell count must exactly match the template row cell "
                "count",
                std::string{entry_name});
            return false;
        }
    }

    for (const auto &row_values : rows) {
        auto row_copy = placeholder.table.insert_copy_before(placeholder.row, placeholder.row);
        if (row_copy == pugi::xml_node{}) {
            set_last_error(last_error_info,
                           std::make_error_code(std::errc::not_enough_memory),
                           "failed to clone the template row before the bookmark row",
                           std::string{entry_name});
            return false;
        }

        std::size_t cell_index = 0U;
        for (auto cell = row_copy.child("w:tc"); cell != pugi::xml_node{};
             cell = featherdoc::detail::next_named_sibling(cell, "w:tc"), ++cell_index) {
            if (!rewrite_table_cell_plain_text(cell, row_values[cell_index])) {
                set_last_error(last_error_info,
                               std::make_error_code(std::errc::not_enough_memory),
                               "failed to rewrite a cloned template row cell",
                               std::string{entry_name});
                return false;
            }
        }
    }

    if (!placeholder.table.remove_child(placeholder.row)) {
        set_last_error(last_error_info, std::make_error_code(std::errc::not_enough_memory),
                       "failed to remove the bookmark placeholder row after table row "
                       "replacement",
                       std::string{entry_name});
        return false;
    }

    return true;
}

auto insert_table_before_placeholder(
    featherdoc::document_error_info &last_error_info, std::string_view entry_name,
    pugi::xml_node placeholder_paragraph,
    const std::vector<std::vector<std::string>> &rows) -> bool {
    auto parent = placeholder_paragraph.parent();
    if (placeholder_paragraph == pugi::xml_node{} || parent == pugi::xml_node{}) {
        set_last_error(last_error_info, std::make_error_code(std::errc::invalid_argument),
                       "failed to resolve the placeholder paragraph for table replacement",
                       std::string{entry_name});
        return false;
    }

    auto table_node = parent.insert_child_before("w:tbl", placeholder_paragraph);
    if (table_node == pugi::xml_node{}) {
        set_last_error(last_error_info, std::make_error_code(std::errc::not_enough_memory),
                       "failed to insert the replacement table before the bookmark paragraph",
                       std::string{entry_name});
        return false;
    }

    featherdoc::Table table(parent, table_node);
    for (const auto &row_values : rows) {
        auto row = table.append_row(row_values.size());
        if (!row.has_next()) {
            set_last_error(last_error_info, std::make_error_code(std::errc::not_enough_memory),
                           "failed to append a replacement table row",
                           std::string{entry_name});
            return false;
        }

        auto cell = row.cells();
        for (const auto &cell_text : row_values) {
            if (!cell.has_next()) {
                set_last_error(last_error_info,
                               std::make_error_code(std::errc::not_enough_memory),
                               "failed to resolve a replacement table cell",
                               std::string{entry_name});
                return false;
            }

            if (!cell_text.empty() && !cell.paragraphs().add_run(cell_text).has_next()) {
                set_last_error(last_error_info,
                               std::make_error_code(std::errc::not_enough_memory),
                               "failed to append replacement table text",
                               std::string{entry_name});
                return false;
            }

            cell.next();
        }
    }

    if (!parent.remove_child(placeholder_paragraph)) {
        set_last_error(last_error_info, std::make_error_code(std::errc::not_enough_memory),
                       "failed to remove the bookmark placeholder paragraph after table "
                       "replacement",
                       std::string{entry_name});
        return false;
    }

    return true;
}

auto replace_placeholder_with_plain_text_paragraphs(
    featherdoc::document_error_info &last_error_info, std::string_view entry_name,
    pugi::xml_node placeholder_paragraph,
    const std::vector<std::string> &paragraphs) -> bool {
    auto parent = placeholder_paragraph.parent();
    if (placeholder_paragraph == pugi::xml_node{} || parent == pugi::xml_node{}) {
        set_last_error(last_error_info, std::make_error_code(std::errc::invalid_argument),
                       "failed to resolve the placeholder paragraph for paragraph "
                       "replacement",
                       std::string{entry_name});
        return false;
    }

    for (const auto &paragraph_text : paragraphs) {
        if (!featherdoc::detail::insert_plain_text_paragraph(
                parent, placeholder_paragraph, paragraph_text)) {
            set_last_error(last_error_info,
                           std::make_error_code(std::errc::not_enough_memory),
                           "failed to insert a replacement paragraph before the bookmark "
                           "placeholder",
                           std::string{entry_name});
            return false;
        }
    }

    if (!parent.remove_child(placeholder_paragraph)) {
        set_last_error(last_error_info, std::make_error_code(std::errc::not_enough_memory),
                       "failed to remove the bookmark placeholder paragraph after paragraph "
                       "replacement",
                       std::string{entry_name});
        return false;
    }

    return true;
}

auto replace_bookmark_text_in_part(featherdoc::document_error_info &last_error_info,
                                   pugi::xml_document &document, std::string_view entry_name,
                                   std::string_view bookmark_name,
                                   std::string_view replacement) -> std::size_t {
    if (bookmark_name.empty()) {
        set_last_error(last_error_info, std::make_error_code(std::errc::invalid_argument),
                       "bookmark name must not be empty", std::string{entry_name});
        return 0U;
    }

    std::vector<pugi::xml_node> bookmark_starts;
    collect_named_bookmark_starts(document, bookmark_name, bookmark_starts);

    std::size_t replaced = 0U;
    for (const auto bookmark_start : bookmark_starts) {
        if (replace_bookmark_range(bookmark_start, replacement)) {
            ++replaced;
        }
    }

    last_error_info.clear();
    return replaced;
}

auto fill_bookmarks_in_part(featherdoc::document_error_info &last_error_info,
                            pugi::xml_document &document, std::string_view entry_name,
                            std::span<const featherdoc::bookmark_text_binding> bindings)
    -> featherdoc::bookmark_fill_result {
    featherdoc::bookmark_fill_result result;
    result.requested = bindings.size();

    std::unordered_set<std::string> seen_bookmarks;
    for (const auto &binding : bindings) {
        if (binding.bookmark_name.empty()) {
            set_last_error(last_error_info, std::make_error_code(std::errc::invalid_argument),
                           "bookmark binding name must not be empty",
                           std::string{entry_name});
            return result;
        }

        const auto [_, inserted] = seen_bookmarks.emplace(binding.bookmark_name);
        if (!inserted) {
            set_last_error(last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           "duplicate bookmark binding name: " + binding.bookmark_name,
                           std::string{entry_name});
            return result;
        }
    }

    for (const auto &binding : bindings) {
        const auto replaced = replace_bookmark_text_in_part(
            last_error_info, document, entry_name, binding.bookmark_name, binding.text);
        if (replaced == 0U) {
            result.missing_bookmarks.push_back(binding.bookmark_name);
            continue;
        }

        ++result.matched;
        result.replaced += replaced;
    }

    last_error_info.clear();
    return result;
}

auto validate_placeholder_requirement(
    pugi::xml_document &document, std::string_view entry_name,
    const featherdoc::template_slot_requirement &requirement) -> bool {
    if (requirement.source != featherdoc::template_slot_source_kind::bookmark) {
        return true;
    }

    featherdoc::document_error_info validation_error_info;

    switch (requirement.kind) {
    case featherdoc::template_slot_kind::text:
        return true;
    case featherdoc::template_slot_kind::table_rows: {
        std::vector<table_row_bookmark_placeholder> placeholders;
        return collect_table_row_bookmark_placeholders(
            validation_error_info, document, entry_name, requirement.bookmark_name,
            placeholders);
    }
    case featherdoc::template_slot_kind::table:
    case featherdoc::template_slot_kind::image:
    case featherdoc::template_slot_kind::floating_image:
    case featherdoc::template_slot_kind::block: {
        std::vector<block_bookmark_placeholder> placeholders;
        return collect_block_bookmark_placeholders(
            validation_error_info, document, entry_name, requirement.bookmark_name,
            placeholders);
    }
    }

    return false;
}

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

auto set_bookmark_block_visibility_in_part(
    featherdoc::document_error_info &last_error_info, pugi::xml_document &document,
    std::string_view entry_name, std::string_view bookmark_name, bool visible)
    -> std::size_t {
    if (bookmark_name.empty()) {
        set_last_error(last_error_info, std::make_error_code(std::errc::invalid_argument),
                       "bookmark name must not be empty", std::string{entry_name});
        return 0U;
    }

    std::vector<bookmark_block_placeholder> placeholders;
    if (!collect_bookmark_block_placeholders(last_error_info, document, entry_name,
                                             bookmark_name, placeholders)) {
        return 0U;
    }

    if (placeholders.empty()) {
        last_error_info.clear();
        return 0U;
    }

    std::size_t replaced = 0U;
    for (auto it = placeholders.rbegin(); it != placeholders.rend(); ++it) {
        auto placeholder = *it;
        if (placeholder.container == pugi::xml_node{} ||
            placeholder.start_paragraph.parent() != placeholder.container ||
            placeholder.end_paragraph.parent() != placeholder.container) {
            set_last_error(last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           "failed to resolve the bookmark block marker paragraphs",
                           std::string{entry_name});
            return 0U;
        }

        if (visible) {
            if (!placeholder.container.remove_child(placeholder.end_paragraph) ||
                !placeholder.container.remove_child(placeholder.start_paragraph)) {
                set_last_error(last_error_info,
                               std::make_error_code(std::errc::not_enough_memory),
                               "failed to remove bookmark block marker paragraphs",
                               std::string{entry_name});
                return 0U;
            }
        } else {
            auto node = placeholder.start_paragraph;
            bool removed_end = false;
            while (node != pugi::xml_node{}) {
                const auto next = node.next_sibling();
                const bool is_end = node == placeholder.end_paragraph;
                if (!placeholder.container.remove_child(node)) {
                    set_last_error(last_error_info,
                                   std::make_error_code(std::errc::not_enough_memory),
                                   "failed to remove nodes inside the bookmark block range",
                                   std::string{entry_name});
                    return 0U;
                }
                if (is_end) {
                    removed_end = true;
                    break;
                }
                node = next;
            }

            if (!removed_end) {
                set_last_error(last_error_info,
                               std::make_error_code(std::errc::invalid_argument),
                               "failed to locate the bookmark block end marker while "
                               "removing the block",
                               std::string{entry_name});
                return 0U;
            }
        }

        ++replaced;
    }

    last_error_info.clear();
    return replaced;
}

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

auto apply_bookmark_block_visibility_in_part(
    featherdoc::document_error_info &last_error_info, pugi::xml_document &document,
    std::string_view entry_name,
    std::span<const featherdoc::bookmark_block_visibility_binding> bindings)
    -> featherdoc::bookmark_block_visibility_result {
    featherdoc::bookmark_block_visibility_result result;
    result.requested = bindings.size();

    std::unordered_set<std::string> seen_bookmarks;
    for (const auto &binding : bindings) {
        if (binding.bookmark_name.empty()) {
            set_last_error(last_error_info, std::make_error_code(std::errc::invalid_argument),
                           "bookmark block visibility binding name must not be empty",
                           std::string{entry_name});
            return result;
        }

        const auto [_, inserted] = seen_bookmarks.emplace(binding.bookmark_name);
        if (!inserted) {
            set_last_error(last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           "duplicate bookmark block visibility binding name: " +
                               binding.bookmark_name,
                           std::string{entry_name});
            return result;
        }
    }

    for (const auto &binding : bindings) {
        const auto replaced = set_bookmark_block_visibility_in_part(
            last_error_info, document, entry_name, binding.bookmark_name, binding.visible);
        if (replaced == 0U) {
            result.missing_bookmarks.push_back(binding.bookmark_name);
            continue;
        }

        ++result.matched;
        if (binding.visible) {
            result.kept += replaced;
        } else {
            result.removed += replaced;
        }
    }

    last_error_info.clear();
    return result;
}

auto replace_bookmark_with_paragraphs_in_part(
    featherdoc::document_error_info &last_error_info, pugi::xml_document &document,
    std::string_view entry_name, std::string_view bookmark_name,
    const std::vector<std::string> &paragraphs) -> std::size_t {
    if (bookmark_name.empty()) {
        set_last_error(last_error_info, std::make_error_code(std::errc::invalid_argument),
                       "bookmark name must not be empty", std::string{entry_name});
        return 0U;
    }

    std::vector<block_bookmark_placeholder> placeholders;
    if (!collect_block_bookmark_placeholders(last_error_info, document, entry_name,
                                             bookmark_name, placeholders)) {
        return 0U;
    }

    if (placeholders.empty()) {
        last_error_info.clear();
        return 0U;
    }

    std::size_t replaced = 0U;
    for (const auto &placeholder : placeholders) {
        if (!replace_placeholder_with_plain_text_paragraphs(
                last_error_info, entry_name, placeholder.paragraph, paragraphs)) {
            return 0U;
        }
        ++replaced;
    }

    last_error_info.clear();
    return replaced;
}

auto remove_bookmark_block_in_part(featherdoc::document_error_info &last_error_info,
                                   pugi::xml_document &document,
                                   std::string_view entry_name,
                                   std::string_view bookmark_name)
    -> std::size_t {
    return replace_bookmark_with_paragraphs_in_part(last_error_info, document, entry_name,
                                                    bookmark_name, {});
}

auto replace_bookmark_with_table_rows_in_part(
    featherdoc::document_error_info &last_error_info, pugi::xml_document &document,
    std::string_view entry_name, std::string_view bookmark_name,
    const std::vector<std::vector<std::string>> &rows) -> std::size_t {
    if (bookmark_name.empty()) {
        set_last_error(last_error_info, std::make_error_code(std::errc::invalid_argument),
                       "bookmark name must not be empty", std::string{entry_name});
        return 0U;
    }

    std::vector<table_row_bookmark_placeholder> placeholders;
    if (!collect_table_row_bookmark_placeholders(last_error_info, document, entry_name,
                                                 bookmark_name, placeholders)) {
        return 0U;
    }

    if (placeholders.empty()) {
        last_error_info.clear();
        return 0U;
    }

    std::size_t replaced = 0U;
    for (const auto &placeholder : placeholders) {
        if (!replace_placeholder_with_table_rows(last_error_info, entry_name, placeholder,
                                                 rows)) {
            return 0U;
        }
        ++replaced;
    }

    last_error_info.clear();
    return replaced;
}

auto replace_bookmark_with_table_in_part(
    featherdoc::document_error_info &last_error_info, pugi::xml_document &document,
    std::string_view entry_name, std::string_view bookmark_name,
    const std::vector<std::vector<std::string>> &rows) -> std::size_t {
    if (bookmark_name.empty()) {
        set_last_error(last_error_info, std::make_error_code(std::errc::invalid_argument),
                       "bookmark name must not be empty", std::string{entry_name});
        return 0U;
    }

    if (rows.empty()) {
        set_last_error(last_error_info, std::make_error_code(std::errc::invalid_argument),
                       "replacement table must contain at least one row",
                       std::string{entry_name});
        return 0U;
    }

    for (const auto &row : rows) {
        if (row.empty()) {
            set_last_error(last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           "replacement table rows must contain at least one cell",
                           std::string{entry_name});
            return 0U;
        }
    }

    std::vector<block_bookmark_placeholder> placeholders;
    if (!collect_block_bookmark_placeholders(last_error_info, document, entry_name,
                                             bookmark_name, placeholders)) {
        return 0U;
    }

    if (placeholders.empty()) {
        last_error_info.clear();
        return 0U;
    }

    std::size_t replaced = 0U;
    for (const auto &placeholder : placeholders) {
        if (!insert_table_before_placeholder(last_error_info, entry_name,
                                             placeholder.paragraph, rows)) {
            return 0U;
        }
        ++replaced;
    }

    last_error_info.clear();
    return replaced;
}
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

std::string make_omml_text(std::string_view text, bool display) {
    return wrap_omml_root(omml_expression_xml(text), display);
}

std::string make_omml_fraction(std::string_view numerator,
                               std::string_view denominator,
                               bool display) {
    return wrap_omml_root("<m:f><m:num>" + omml_expression_xml(numerator) +
                              "</m:num><m:den>" + omml_expression_xml(denominator) +
                              "</m:den></m:f>",
                          display);
}

std::string make_omml_superscript(std::string_view base,
                                  std::string_view superscript,
                                  bool display) {
    return wrap_omml_root("<m:sSup><m:e>" + omml_expression_xml(base) +
                              "</m:e><m:sup>" + omml_expression_xml(superscript) +
                              "</m:sup></m:sSup>",
                          display);
}

std::string make_omml_subscript(std::string_view base,
                                std::string_view subscript,
                                bool display) {
    return wrap_omml_root("<m:sSub><m:e>" + omml_expression_xml(base) +
                              "</m:e><m:sub>" + omml_expression_xml(subscript) +
                              "</m:sub></m:sSub>",
                          display);
}

std::string make_omml_radical(std::string_view radicand,
                              std::string_view degree,
                              bool display) {
    std::string inner = "<m:rad>";
    if (degree.empty()) {
        inner += "<m:radPr><m:degHide m:val=\"1\"/></m:radPr>";
    } else {
        inner += "<m:deg>" + omml_expression_xml(degree) + "</m:deg>";
    }
    inner += "<m:e>" + omml_expression_xml(radicand) + "</m:e></m:rad>";
    return wrap_omml_root(std::move(inner), display);
}

std::string make_omml_delimiter(std::string_view expression,
                                std::string_view begin,
                                std::string_view end,
                                bool display) {
    auto inner = std::string{"<m:d><m:dPr><m:begChr m:val=\""} +
                 escape_xml_text(begin) + "\"/><m:endChr m:val=\"" +
                 escape_xml_text(end) + "\"/></m:dPr><m:e>" +
                 omml_expression_xml(expression) + "</m:e></m:d>";
    return wrap_omml_root(std::move(inner), display);
}

std::string make_omml_nary(std::string_view operator_character,
                           std::string_view expression,
                           std::string_view lower_limit,
                           std::string_view upper_limit,
                           bool display) {
    auto inner = std::string{"<m:nary><m:naryPr><m:chr m:val=\""} +
                 escape_xml_text(operator_character) +
                 "\"/><m:limLoc m:val=\"undOvr\"/>";
    if (lower_limit.empty()) {
        inner += "<m:subHide m:val=\"1\"/>";
    }
    if (upper_limit.empty()) {
        inner += "<m:supHide m:val=\"1\"/>";
    }
    inner += "</m:naryPr>";
    if (!lower_limit.empty()) {
        inner += "<m:sub>" + omml_expression_xml(lower_limit) + "</m:sub>";
    }
    if (!upper_limit.empty()) {
        inner += "<m:sup>" + omml_expression_xml(upper_limit) + "</m:sup>";
    }
    inner += "<m:e>" + omml_expression_xml(expression) + "</m:e></m:nary>";
    return wrap_omml_root(std::move(inner), display);
}

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

bool TemplatePart::append_page_number_field(featherdoc::field_state_options state) {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return false;
    }

    const auto container = template_part_block_container(*this->xml_document);
    if (container == pugi::xml_node{}) {
        set_last_error(*this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "template part does not contain a supported block container",
                       this->entry_name_storage);
        return false;
    }

    const auto paragraph = select_field_paragraph(container);
    if (paragraph == pugi::xml_node{} ||
        !append_simple_field_run(paragraph, page_number_field_instruction, "1", state)) {
        set_last_error(*this->last_error_info,
                       std::make_error_code(std::errc::io_error),
                       "failed to append page number field to template part",
                       this->entry_name_storage);
        return false;
    }

    this->last_error_info->clear();
    return true;
}

bool TemplatePart::append_total_pages_field(featherdoc::field_state_options state) {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return false;
    }

    const auto container = template_part_block_container(*this->xml_document);
    if (container == pugi::xml_node{}) {
        set_last_error(*this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "template part does not contain a supported block container",
                       this->entry_name_storage);
        return false;
    }

    const auto paragraph = select_field_paragraph(container);
    if (paragraph == pugi::xml_node{} ||
        !append_simple_field_run(paragraph, total_pages_field_instruction, "1", state)) {
        set_last_error(*this->last_error_info,
                       std::make_error_code(std::errc::io_error),
                       "failed to append total pages field to template part",
                       this->entry_name_storage);
        return false;
    }

    this->last_error_info->clear();
    return true;
}

std::vector<featherdoc::field_summary> TemplatePart::list_fields() const {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return {};
    }

    return summarize_fields_in_part(*this->last_error_info, *this->xml_document);
}

bool TemplatePart::append_field(std::string_view instruction,
                                std::string_view result_text,
                                featherdoc::field_state_options state) {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return false;
    }

    return append_field_to_part(*this->last_error_info, *this->xml_document,
                                this->entry_name_storage, instruction,
                                result_text, state);
}

bool TemplatePart::append_complex_field(
    std::string_view instruction, std::string_view result_text,
    featherdoc::complex_field_options options) {
    return this->append_complex_field(
        std::initializer_list<featherdoc::complex_field_instruction_fragment>{
            featherdoc::complex_field_text_fragment(instruction)},
        result_text, options);
}

bool TemplatePart::append_complex_field(
    std::span<const featherdoc::complex_field_instruction_fragment> instruction_fragments,
    std::string_view result_text, featherdoc::complex_field_options options) {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return false;
    }

    return append_complex_field_to_part(
        *this->last_error_info, *this->xml_document, this->entry_name_storage,
        instruction_fragments, result_text, options);
}

bool TemplatePart::append_complex_field(
    std::initializer_list<featherdoc::complex_field_instruction_fragment>
        instruction_fragments,
    std::string_view result_text, featherdoc::complex_field_options options) {
    return this->append_complex_field(
        std::span<const featherdoc::complex_field_instruction_fragment>{
            instruction_fragments.begin(), instruction_fragments.size()},
        result_text, options);
}

bool TemplatePart::append_table_of_contents_field(
    featherdoc::table_of_contents_field_options options,
    std::string_view result_text) {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return false;
    }

    const auto instruction = build_table_of_contents_instruction(
        options, *this->last_error_info, this->entry_name_storage);
    if (!instruction.has_value()) {
        return false;
    }

    return append_field_to_part(*this->last_error_info, *this->xml_document,
                                this->entry_name_storage, *instruction,
                                result_text, options.state);
}

bool TemplatePart::append_reference_field(
    std::string_view bookmark_name,
    featherdoc::reference_field_options options,
    std::string_view result_text) {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return false;
    }

    const auto instruction = build_reference_instruction(
        bookmark_name, options, *this->last_error_info, this->entry_name_storage);
    if (!instruction.has_value()) {
        return false;
    }

    return append_field_to_part(*this->last_error_info, *this->xml_document,
                                this->entry_name_storage, *instruction,
                                result_text, options.state);
}

bool TemplatePart::append_page_reference_field(
    std::string_view bookmark_name,
    featherdoc::page_reference_field_options options,
    std::string_view result_text) {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return false;
    }

    const auto instruction = build_page_reference_instruction(
        bookmark_name, options, *this->last_error_info, this->entry_name_storage);
    if (!instruction.has_value()) {
        return false;
    }

    return append_field_to_part(*this->last_error_info, *this->xml_document,
                                this->entry_name_storage, *instruction,
                                result_text, options.state);
}

bool TemplatePart::append_style_reference_field(
    std::string_view style_name,
    featherdoc::style_reference_field_options options,
    std::string_view result_text) {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return false;
    }

    const auto instruction = build_style_reference_instruction(
        style_name, options, *this->last_error_info, this->entry_name_storage);
    if (!instruction.has_value()) {
        return false;
    }

    return append_field_to_part(*this->last_error_info, *this->xml_document,
                                this->entry_name_storage, *instruction,
                                result_text, options.state);
}

bool TemplatePart::append_document_property_field(
    std::string_view property_name,
    featherdoc::document_property_field_options options,
    std::string_view result_text) {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return false;
    }

    const auto instruction = build_document_property_instruction(
        property_name, options, *this->last_error_info, this->entry_name_storage);
    if (!instruction.has_value()) {
        return false;
    }

    return append_field_to_part(*this->last_error_info, *this->xml_document,
                                this->entry_name_storage, *instruction,
                                result_text, options.state);
}

bool TemplatePart::append_date_field(featherdoc::date_field_options options,
                                     std::string_view result_text) {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return false;
    }

    const auto instruction = build_date_instruction(
        options, *this->last_error_info, this->entry_name_storage);
    if (!instruction.has_value()) {
        return false;
    }

    return append_field_to_part(*this->last_error_info, *this->xml_document,
                                this->entry_name_storage, *instruction,
                                result_text, options.state);
}

bool TemplatePart::append_hyperlink_field(
    std::string_view target,
    featherdoc::hyperlink_field_options options,
    std::string_view result_text) {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return false;
    }

    const auto instruction = build_hyperlink_instruction(
        target, options, *this->last_error_info, this->entry_name_storage);
    if (!instruction.has_value()) {
        return false;
    }

    return append_field_to_part(*this->last_error_info, *this->xml_document,
                                this->entry_name_storage, *instruction,
                                result_text, options.state);
}

bool TemplatePart::append_sequence_field(
    std::string_view identifier,
    featherdoc::sequence_field_options options,
    std::string_view result_text) {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return false;
    }

    const auto instruction = build_sequence_instruction(
        identifier, options, *this->last_error_info, this->entry_name_storage);
    if (!instruction.has_value()) {
        return false;
    }

    return append_field_to_part(*this->last_error_info, *this->xml_document,
                                this->entry_name_storage, *instruction,
                                result_text, options.state);
}


bool TemplatePart::append_caption(std::string_view label,
                                  std::string_view caption_text,
                                  featherdoc::caption_field_options options,
                                  std::string_view number_result_text) {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return false;
    }

    const auto instruction = build_caption_sequence_instruction(
        label, options, *this->last_error_info, this->entry_name_storage);
    if (!instruction.has_value()) {
        return false;
    }

    const auto container = template_part_block_container(*this->xml_document);
    if (container == pugi::xml_node{}) {
        set_last_error(*this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "template part does not contain a supported block container",
                       this->entry_name_storage);
        return false;
    }

    const auto paragraph = featherdoc::detail::append_paragraph_node(container);
    if (paragraph == pugi::xml_node{}) {
        set_last_error(*this->last_error_info,
                       std::make_error_code(std::errc::io_error),
                       "failed to append caption paragraph to template part",
                       this->entry_name_storage);
        return false;
    }

    std::string label_text{label};
    label_text.push_back(' ');
    if (!featherdoc::detail::append_plain_text_run(paragraph, label_text) ||
        !append_simple_field_run(paragraph, *instruction, number_result_text,
                                 options.state)) {
        set_last_error(*this->last_error_info,
                       std::make_error_code(std::errc::io_error),
                       "failed to append caption field to template part",
                       this->entry_name_storage);
        return false;
    }

    if (!caption_text.empty()) {
        std::string trailing_text = options.separator;
        trailing_text += caption_text;
        if (!featherdoc::detail::append_plain_text_run(paragraph, trailing_text)) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::io_error),
                           "failed to append caption text to template part",
                           this->entry_name_storage);
            return false;
        }
    }

    this->last_error_info->clear();
    return true;
}

bool TemplatePart::append_index_field(featherdoc::index_field_options options,
                                      std::string_view result_text) {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return false;
    }

    const auto instruction = build_index_instruction(
        options, *this->last_error_info, this->entry_name_storage);
    if (!instruction.has_value()) {
        return false;
    }

    return append_field_to_part(*this->last_error_info, *this->xml_document,
                                this->entry_name_storage, *instruction,
                                result_text, options.state);
}

bool TemplatePart::append_index_entry_field(
    std::string_view entry_text,
    featherdoc::index_entry_field_options options,
    std::string_view result_text) {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return false;
    }

    const auto instruction = build_index_entry_instruction(
        entry_text, options, *this->last_error_info, this->entry_name_storage);
    if (!instruction.has_value()) {
        return false;
    }

    return append_field_to_part(*this->last_error_info, *this->xml_document,
                                this->entry_name_storage, *instruction,
                                result_text, options.state);
}

bool TemplatePart::replace_field(std::size_t field_index,
                                 std::string_view instruction,
                                 std::string_view result_text) {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return false;
    }

    return replace_field_in_part(*this->last_error_info, *this->xml_document,
                                 this->entry_name_storage, field_index,
                                 instruction, result_text);
}

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

Table TemplatePart::tables() {
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
    auto table = Table(container, container.child("w:tbl"));
    table.set_owner(this->owner);
    return table;
}

Table TemplatePart::append_table(std::size_t row_count, std::size_t column_count) {
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

    const auto table_node = detail::append_table_node(container);
    auto created_table = Table(container, table_node);
    created_table.set_owner(this->owner);

    for (std::size_t row_index = 0; row_index < row_count; ++row_index) {
        created_table.append_row(column_count);
    }

    this->last_error_info->clear();
    return created_table;
}

std::vector<featherdoc::table_inspection_summary> TemplatePart::inspect_tables() {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return {};
    }

    auto summaries = std::vector<featherdoc::table_inspection_summary>{};
    auto table_handle = this->tables();
    for (std::size_t table_index = 0U; table_handle.has_next();
         ++table_index, table_handle.next()) {
        summaries.push_back(summarize_part_table_handle(table_handle, table_index));
    }

    this->last_error_info->clear();
    return summaries;
}

std::optional<featherdoc::table_inspection_summary>
TemplatePart::inspect_table(std::size_t table_index) {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return std::nullopt;
    }

    auto table_handle = this->tables();
    for (std::size_t current_index = 0U;
         current_index < table_index && table_handle.has_next(); ++current_index) {
        table_handle.next();
    }

    if (!table_handle.has_next()) {
        this->last_error_info->clear();
        return std::nullopt;
    }

    auto summary = summarize_part_table_handle(table_handle, table_index);
    this->last_error_info->clear();
    return summary;
}

std::vector<featherdoc::table_cell_inspection_summary>
TemplatePart::inspect_table_cells(std::size_t table_index) {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return {};
    }

    auto table_handle = this->tables();
    for (std::size_t current_index = 0U;
         current_index < table_index && table_handle.has_next(); ++current_index) {
        table_handle.next();
    }

    if (!table_handle.has_next()) {
        this->last_error_info->clear();
        return {};
    }

    auto summaries = collect_part_table_cell_summaries(table_handle);
    this->last_error_info->clear();
    return summaries;
}

std::optional<featherdoc::table_cell_inspection_summary>
TemplatePart::inspect_table_cell(std::size_t table_index, std::size_t row_index,
                                 std::size_t cell_index) {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return std::nullopt;
    }

    const auto cells = this->inspect_table_cells(table_index);
    for (const auto &cell : cells) {
        if (cell.row_index == row_index && cell.cell_index == cell_index) {
            this->last_error_info->clear();
            return cell;
        }
    }

    this->last_error_info->clear();
    return std::nullopt;
}

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

std::size_t TemplatePart::replace_content_control_text_by_tag(
    std::string_view tag, std::string_view replacement) {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return 0U;
    }

    return replace_content_control_text_by_tag_or_alias_in_part(
        *this->last_error_info, *this->xml_document, this->entry_name_storage,
        tag, replacement, true);
}

std::size_t TemplatePart::replace_content_control_text_by_alias(
    std::string_view alias, std::string_view replacement) {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return 0U;
    }

    return replace_content_control_text_by_tag_or_alias_in_part(
        *this->last_error_info, *this->xml_document, this->entry_name_storage,
        alias, replacement, false);
}

std::size_t TemplatePart::set_content_control_form_state_by_tag(
    std::string_view tag,
    const featherdoc::content_control_form_state_options &options) {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return 0U;
    }

    return set_content_control_form_state_by_tag_or_alias_in_part(
        *this->last_error_info, *this->xml_document, this->entry_name_storage,
        tag, options, true);
}

std::size_t TemplatePart::set_content_control_form_state_by_alias(
    std::string_view alias,
    const featherdoc::content_control_form_state_options &options) {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return 0U;
    }

    return set_content_control_form_state_by_tag_or_alias_in_part(
        *this->last_error_info, *this->xml_document, this->entry_name_storage,
        alias, options, false);
}

std::size_t TemplatePart::replace_content_control_with_paragraphs_by_tag(
    std::string_view tag, const std::vector<std::string> &paragraphs) {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return 0U;
    }

    return replace_content_control_with_paragraphs_by_tag_or_alias_in_part(
        *this->last_error_info, *this->xml_document, this->entry_name_storage,
        tag, paragraphs, true);
}

std::size_t TemplatePart::replace_content_control_with_paragraphs_by_alias(
    std::string_view alias, const std::vector<std::string> &paragraphs) {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return 0U;
    }

    return replace_content_control_with_paragraphs_by_tag_or_alias_in_part(
        *this->last_error_info, *this->xml_document, this->entry_name_storage,
        alias, paragraphs, false);
}

std::size_t TemplatePart::replace_content_control_with_table_rows_by_tag(
    std::string_view tag, const std::vector<std::vector<std::string>> &rows) {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return 0U;
    }

    return replace_content_control_with_table_rows_by_tag_or_alias_in_part(
        *this->last_error_info, *this->xml_document, this->entry_name_storage,
        tag, rows, true);
}

std::size_t TemplatePart::replace_content_control_with_table_rows_by_alias(
    std::string_view alias, const std::vector<std::vector<std::string>> &rows) {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return 0U;
    }

    return replace_content_control_with_table_rows_by_tag_or_alias_in_part(
        *this->last_error_info, *this->xml_document, this->entry_name_storage,
        alias, rows, false);
}

std::size_t TemplatePart::replace_content_control_with_table_by_tag(
    std::string_view tag, const std::vector<std::vector<std::string>> &rows) {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return 0U;
    }

    return replace_content_control_with_table_by_tag_or_alias_in_part(
        *this->last_error_info, *this->xml_document, this->entry_name_storage,
        tag, rows, true);
}

std::size_t TemplatePart::replace_content_control_with_table_by_alias(
    std::string_view alias, const std::vector<std::vector<std::string>> &rows) {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return 0U;
    }

    return replace_content_control_with_table_by_tag_or_alias_in_part(
        *this->last_error_info, *this->xml_document, this->entry_name_storage,
        alias, rows, false);
}

std::size_t TemplatePart::replace_content_control_with_image_by_tag(
    std::string_view tag, const std::filesystem::path &image_path) {
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

    return this->owner->replace_content_control_with_image_in_part(
        *this->xml_document, this->entry_name_storage, tag, true, image_path,
        std::nullopt);
}

std::size_t TemplatePart::replace_content_control_with_image_by_tag(
    std::string_view tag, const std::filesystem::path &image_path,
    std::uint32_t width_px, std::uint32_t height_px) {
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

    return this->owner->replace_content_control_with_image_in_part(
        *this->xml_document, this->entry_name_storage, tag, true, image_path,
        std::pair<std::uint32_t, std::uint32_t>{width_px, height_px});
}

std::size_t TemplatePart::replace_content_control_with_image_by_alias(
    std::string_view alias, const std::filesystem::path &image_path) {
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

    return this->owner->replace_content_control_with_image_in_part(
        *this->xml_document, this->entry_name_storage, alias, false, image_path,
        std::nullopt);
}

std::size_t TemplatePart::replace_content_control_with_image_by_alias(
    std::string_view alias, const std::filesystem::path &image_path,
    std::uint32_t width_px, std::uint32_t height_px) {
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

    return this->owner->replace_content_control_with_image_in_part(
        *this->xml_document, this->entry_name_storage, alias, false, image_path,
        std::pair<std::uint32_t, std::uint32_t>{width_px, height_px});
}

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

std::size_t TemplatePart::replace_bookmark_with_paragraphs(
    std::string_view bookmark_name, const std::vector<std::string> &paragraphs) {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return 0U;
    }

    return replace_bookmark_with_paragraphs_in_part(
        *this->last_error_info, *this->xml_document, this->entry_name_storage,
        bookmark_name, paragraphs);
}

std::size_t TemplatePart::replace_bookmark_with_table_rows(
    std::string_view bookmark_name, const std::vector<std::vector<std::string>> &rows) {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return 0U;
    }

    return replace_bookmark_with_table_rows_in_part(
        *this->last_error_info, *this->xml_document, this->entry_name_storage,
        bookmark_name, rows);
}

std::size_t TemplatePart::replace_bookmark_with_table(
    std::string_view bookmark_name, const std::vector<std::vector<std::string>> &rows) {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return 0U;
    }

    return replace_bookmark_with_table_in_part(*this->last_error_info, *this->xml_document,
                                               this->entry_name_storage, bookmark_name,
                                               rows);
}

std::size_t TemplatePart::replace_bookmark_with_image(
    std::string_view bookmark_name, const std::filesystem::path &image_path) {
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

    return this->owner->replace_bookmark_with_image_in_part(
        *this->xml_document, this->entry_name_storage, bookmark_name, image_path,
        std::nullopt);
}

std::size_t TemplatePart::replace_bookmark_with_image(
    std::string_view bookmark_name, const std::filesystem::path &image_path,
    std::uint32_t width_px, std::uint32_t height_px) {
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

    return this->owner->replace_bookmark_with_image_in_part(
        *this->xml_document, this->entry_name_storage, bookmark_name, image_path,
        std::pair<std::uint32_t, std::uint32_t>{width_px, height_px});
}

std::size_t TemplatePart::replace_bookmark_with_floating_image(
    std::string_view bookmark_name, const std::filesystem::path &image_path,
    featherdoc::floating_image_options options) {
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

    return this->owner->replace_bookmark_with_floating_image_in_part(
        *this->xml_document, this->entry_name_storage, bookmark_name, image_path,
        std::nullopt, std::move(options));
}

std::size_t TemplatePart::replace_bookmark_with_floating_image(
    std::string_view bookmark_name, const std::filesystem::path &image_path,
    std::uint32_t width_px, std::uint32_t height_px,
    featherdoc::floating_image_options options) {
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

    return this->owner->replace_bookmark_with_floating_image_in_part(
        *this->xml_document, this->entry_name_storage, bookmark_name, image_path,
        std::pair<std::uint32_t, std::uint32_t>{width_px, height_px},
        std::move(options));
}

std::size_t TemplatePart::remove_bookmark_block(std::string_view bookmark_name) {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return 0U;
    }

    return remove_bookmark_block_in_part(*this->last_error_info, *this->xml_document,
                                         this->entry_name_storage, bookmark_name);
}

std::size_t TemplatePart::set_bookmark_block_visibility(
    std::string_view bookmark_name, bool visible) {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return 0U;
    }

    return set_bookmark_block_visibility_in_part(*this->last_error_info, *this->xml_document,
                                                 this->entry_name_storage, bookmark_name,
                                                 visible);
}

bookmark_block_visibility_result TemplatePart::apply_bookmark_block_visibility(
    std::span<const bookmark_block_visibility_binding> bindings) {
    bookmark_block_visibility_result result;
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

    return apply_bookmark_block_visibility_in_part(*this->last_error_info,
                                                   *this->xml_document,
                                                   this->entry_name_storage, bindings);
}

bookmark_block_visibility_result TemplatePart::apply_bookmark_block_visibility(
    std::initializer_list<bookmark_block_visibility_binding> bindings) {
    return this->apply_bookmark_block_visibility(
        std::span<const bookmark_block_visibility_binding>{bindings.begin(),
                                                           bindings.size()});
}

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


namespace {

auto semantic_optional_string(const std::optional<std::string> &value)
    -> std::string {
    return value.has_value() ? *value : std::string{};
}

template <typename Value>
auto semantic_optional_numeric(const std::optional<Value> &value) -> std::string {
    return value.has_value() ? std::to_string(*value) : std::string{};
}

auto semantic_bool(const std::optional<bool> &value) -> std::string {
    if (!value.has_value()) {
        return {};
    }
    return *value ? "true" : "false";
}

auto semantic_content_control_kind_name(featherdoc::content_control_kind kind)
    -> std::string_view {
    switch (kind) {
    case featherdoc::content_control_kind::block:
        return "block";
    case featherdoc::content_control_kind::run:
        return "run";
    case featherdoc::content_control_kind::table_row:
        return "table_row";
    case featherdoc::content_control_kind::table_cell:
        return "table_cell";
    case featherdoc::content_control_kind::unknown:
        return "unknown";
    }

    return "unknown";
}

auto semantic_content_control_form_kind_name(
    featherdoc::content_control_form_kind kind) -> std::string_view {
    switch (kind) {
    case featherdoc::content_control_form_kind::rich_text:
        return "rich_text";
    case featherdoc::content_control_form_kind::plain_text:
        return "plain_text";
    case featherdoc::content_control_form_kind::picture:
        return "picture";
    case featherdoc::content_control_form_kind::checkbox:
        return "checkbox";
    case featherdoc::content_control_form_kind::drop_down_list:
        return "drop_down_list";
    case featherdoc::content_control_form_kind::combo_box:
        return "combo_box";
    case featherdoc::content_control_form_kind::date:
        return "date";
    case featherdoc::content_control_form_kind::repeating_section:
        return "repeating_section";
    case featherdoc::content_control_form_kind::group:
        return "group";
    case featherdoc::content_control_form_kind::unknown:
        return "unknown";
    }

    return "unknown";
}

auto semantic_drawing_image_placement_name(
    featherdoc::drawing_image_placement placement) -> std::string_view {
    switch (placement) {
    case featherdoc::drawing_image_placement::inline_object:
        return "inline";
    case featherdoc::drawing_image_placement::anchored_object:
        return "anchored";
    }

    return "inline";
}

auto semantic_table_position_value(
    const std::optional<featherdoc::table_position> &position) -> std::string {
    if (!position.has_value()) {
        return {};
    }

    std::ostringstream stream;
    stream << "horizontal_reference="
           << static_cast<int>(position->horizontal_reference)
           << ";horizontal_offset=" << position->horizontal_offset_twips
           << ";vertical_reference="
           << static_cast<int>(position->vertical_reference)
           << ";vertical_offset=" << position->vertical_offset_twips
           << ";left=" << semantic_optional_numeric(position->left_from_text_twips)
           << ";right=" << semantic_optional_numeric(position->right_from_text_twips)
           << ";top=" << semantic_optional_numeric(position->top_from_text_twips)
           << ";bottom=" << semantic_optional_numeric(position->bottom_from_text_twips)
           << ";overlap=";
    if (position->overlap.has_value()) {
        stream << static_cast<int>(*position->overlap);
    }
    return stream.str();
}

auto semantic_column_widths_value(
    const std::vector<std::optional<std::uint32_t>> &widths) -> std::string {
    std::ostringstream stream;
    for (std::size_t index = 0; index < widths.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        if (widths[index].has_value()) {
            stream << *widths[index];
        } else {
            stream << "null";
        }
    }
    return stream.str();
}

auto semantic_paragraph_value(
    const featherdoc::paragraph_inspection_summary &paragraph) -> std::string {
    std::ostringstream stream;
    stream << "text=" << paragraph.text << "\nstyle="
           << semantic_optional_string(paragraph.style_id) << "\nbidi="
           << semantic_bool(paragraph.bidi) << "\nruns=" << paragraph.run_count;
    if (paragraph.numbering.has_value()) {
        stream << "\nnumbering="
               << semantic_optional_numeric(paragraph.numbering->num_id) << ':'
               << semantic_optional_numeric(paragraph.numbering->level) << ':'
               << semantic_optional_numeric(paragraph.numbering->definition_id) << ':'
               << semantic_optional_string(paragraph.numbering->definition_name);
    }
    return stream.str();
}

auto semantic_table_value(
    const featherdoc::table_inspection_summary &table) -> std::string {
    std::ostringstream stream;
    stream << "text=" << table.text << "\nstyle="
           << semantic_optional_string(table.style_id) << "\nrows="
           << table.row_count << "\ncolumns=" << table.column_count
           << "\nwidth=" << semantic_optional_numeric(table.width_twips)
           << "\ncolumn_widths="
           << semantic_column_widths_value(table.column_widths)
           << "\nposition=" << semantic_table_position_value(table.position);
    return stream.str();
}

auto semantic_floating_options_value(
    const std::optional<featherdoc::floating_image_options> &options)
    -> std::string {
    if (!options.has_value()) {
        return {};
    }

    std::ostringstream stream;
    stream << "offset=" << options->horizontal_offset_px << ','
           << options->vertical_offset_px << ";behind="
           << (options->behind_text ? "true" : "false") << ";overlap="
           << (options->allow_overlap ? "true" : "false") << ";z="
           << options->z_order << ";wrap_left="
           << options->wrap_distance_left_px << ";wrap_right="
           << options->wrap_distance_right_px << ";wrap_top="
           << options->wrap_distance_top_px << ";wrap_bottom="
           << options->wrap_distance_bottom_px;
    if (options->crop.has_value()) {
        stream << ";crop=" << options->crop->left_per_mille << ','
               << options->crop->top_per_mille << ','
               << options->crop->right_per_mille << ','
               << options->crop->bottom_per_mille;
    }
    return stream.str();
}

auto semantic_image_value(const featherdoc::drawing_image_info &image,
                          const featherdoc::document_semantic_diff_options &options)
    -> std::string {
    std::ostringstream stream;
    stream << "placement=" << semantic_drawing_image_placement_name(image.placement)
           << "\nentry=" << image.entry_name << "\ndisplay="
           << image.display_name << "\ncontent_type=" << image.content_type
           << "\nwidth=" << image.width_px << "\nheight=" << image.height_px
           << "\nfloating=" << semantic_floating_options_value(image.floating_options);
    if (options.compare_image_relationship_ids) {
        stream << "\nrelationship_id=" << image.relationship_id;
    }
    return stream.str();
}

auto semantic_list_items_value(
    const std::vector<featherdoc::content_control_list_item> &items)
    -> std::string {
    std::ostringstream stream;
    for (std::size_t index = 0; index < items.size(); ++index) {
        if (index != 0U) {
            stream << '|';
        }
        stream << items[index].display_text << '=' << items[index].value;
    }
    return stream.str();
}

auto semantic_content_control_value(
    const featherdoc::content_control_summary &content_control,
    const featherdoc::document_semantic_diff_options &options) -> std::string {
    std::ostringstream stream;
    stream << "kind=" << semantic_content_control_kind_name(content_control.kind)
           << "\nform="
           << semantic_content_control_form_kind_name(content_control.form_kind)
           << "\ntag=" << semantic_optional_string(content_control.tag)
           << "\nalias=" << semantic_optional_string(content_control.alias)
           << "\nlock=" << semantic_optional_string(content_control.lock)
           << "\nbinding_store="
           << semantic_optional_string(content_control.data_binding_store_item_id)
           << "\nbinding_xpath="
           << semantic_optional_string(content_control.data_binding_xpath)
           << "\nbinding_prefix="
           << semantic_optional_string(content_control.data_binding_prefix_mappings)
           << "\nchecked=" << semantic_bool(content_control.checked)
           << "\ndate_format="
           << semantic_optional_string(content_control.date_format)
           << "\ndate_locale="
           << semantic_optional_string(content_control.date_locale)
           << "\nselected="
           << semantic_optional_numeric(content_control.selected_list_item)
           << "\nitems="
           << semantic_list_items_value(content_control.list_items)
           << "\nplaceholder="
           << (content_control.showing_placeholder ? "true" : "false")
           << "\ntext=" << content_control.text;
    if (options.compare_content_control_ids) {
        stream << "\nid=" << semantic_optional_string(content_control.id);
    }
    return stream.str();
}


auto semantic_field_kind_name(featherdoc::field_kind kind) -> std::string_view {
    switch (kind) {
    case featherdoc::field_kind::page:
        return "page";
    case featherdoc::field_kind::total_pages:
        return "total_pages";
    case featherdoc::field_kind::table_of_contents:
        return "table_of_contents";
    case featherdoc::field_kind::reference:
        return "reference";
    case featherdoc::field_kind::page_reference:
        return "page_reference";
    case featherdoc::field_kind::style_reference:
        return "style_reference";
    case featherdoc::field_kind::document_property:
        return "document_property";
    case featherdoc::field_kind::date:
        return "date";
    case featherdoc::field_kind::hyperlink:
        return "hyperlink";
    case featherdoc::field_kind::sequence:
        return "sequence";
    case featherdoc::field_kind::index:
        return "index";
    case featherdoc::field_kind::index_entry:
        return "index_entry";
    case featherdoc::field_kind::custom:
        return "custom";
    }

    return "custom";
}

auto semantic_field_summary_value(const featherdoc::field_summary &field)
    -> std::string {
    std::ostringstream stream;
    stream << "kind=" << semantic_field_kind_name(field.kind)
           << "\ninstruction=" << field.instruction
           << "\nresult_text=" << field.result_text
           << "\ndirty=" << (field.dirty ? "true" : "false")
           << "\nlocked=" << (field.locked ? "true" : "false")
           << "\ncomplex=" << (field.complex ? "true" : "false")
           << "\ndepth=" << field.depth;
    return stream.str();
}


auto semantic_style_kind_name(featherdoc::style_kind kind) -> std::string_view {
    switch (kind) {
    case featherdoc::style_kind::paragraph:
        return "paragraph";
    case featherdoc::style_kind::character:
        return "character";
    case featherdoc::style_kind::table:
        return "table";
    case featherdoc::style_kind::numbering:
        return "numbering";
    case featherdoc::style_kind::unknown:
        return "unknown";
    }

    return "unknown";
}

auto semantic_list_kind_name(featherdoc::list_kind kind) -> std::string_view {
    switch (kind) {
    case featherdoc::list_kind::bullet:
        return "bullet";
    case featherdoc::list_kind::decimal:
        return "decimal";
    }

    return "decimal";
}

auto semantic_numbering_level_value(
    const featherdoc::numbering_level_definition &level) -> std::string {
    std::ostringstream stream;
    stream << "level=" << level.level << ";kind="
           << semantic_list_kind_name(level.kind) << ";start=" << level.start
           << ";text_pattern=" << level.text_pattern;
    return stream.str();
}

auto semantic_numbering_instance_value(
    const featherdoc::numbering_instance_summary &instance) -> std::string {
    std::ostringstream stream;
    stream << "id=" << instance.instance_id << ";overrides=";
    for (std::size_t index = 0U; index < instance.level_overrides.size(); ++index) {
        const auto &override = instance.level_overrides[index];
        if (index != 0U) {
            stream << '|';
        }
        stream << override.level << ':'
               << semantic_optional_numeric(override.start_override) << ':';
        if (override.level_definition.has_value()) {
            stream << semantic_numbering_level_value(*override.level_definition);
        }
    }
    return stream.str();
}

auto semantic_style_numbering_value(
    const std::optional<featherdoc::style_summary::numbering_summary> &numbering)
    -> std::string {
    if (!numbering.has_value()) {
        return {};
    }

    std::ostringstream stream;
    stream << "num_id=" << semantic_optional_numeric(numbering->num_id)
           << ";level=" << semantic_optional_numeric(numbering->level)
           << ";definition_id="
           << semantic_optional_numeric(numbering->definition_id)
           << ";definition_name="
           << semantic_optional_string(numbering->definition_name);
    if (numbering->instance.has_value()) {
        stream << ";instance="
               << semantic_numbering_instance_value(*numbering->instance);
    }
    return stream.str();
}

auto semantic_style_summary_value(const featherdoc::style_summary &style)
    -> std::string {
    std::ostringstream stream;
    stream << "style_id=" << style.style_id << "\nname=" << style.name
           << "\nbased_on=" << semantic_optional_string(style.based_on)
           << "\nkind=" << semantic_style_kind_name(style.kind)
           << "\ntype=" << style.type_name
           << "\nnumbering=" << semantic_style_numbering_value(style.numbering)
           << "\ndefault=" << (style.is_default ? "true" : "false")
           << "\ncustom=" << (style.is_custom ? "true" : "false")
           << "\nsemi_hidden="
           << (style.is_semi_hidden ? "true" : "false")
           << "\nunhide_when_used="
           << (style.is_unhide_when_used ? "true" : "false")
           << "\nquick_format=" << (style.is_quick_format ? "true" : "false");
    return stream.str();
}

auto semantic_numbering_definition_summary_value(
    const featherdoc::numbering_definition_summary &definition) -> std::string {
    std::ostringstream stream;
    stream << "definition_id=" << definition.definition_id << "\nname="
           << definition.name << "\nlevels=";
    for (std::size_t index = 0U; index < definition.levels.size(); ++index) {
        if (index != 0U) {
            stream << '|';
        }
        stream << semantic_numbering_level_value(definition.levels[index]);
    }
    stream << "\ninstance_ids=";
    for (std::size_t index = 0U; index < definition.instance_ids.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        stream << definition.instance_ids[index];
    }
    stream << "\ninstances=";
    for (std::size_t index = 0U; index < definition.instances.size(); ++index) {
        if (index != 0U) {
            stream << '|';
        }
        stream << semantic_numbering_instance_value(definition.instances[index]);
    }
    return stream.str();
}


auto semantic_review_note_kind_name(featherdoc::review_note_kind kind)
    -> std::string_view {
    switch (kind) {
    case featherdoc::review_note_kind::footnote:
        return "footnote";
    case featherdoc::review_note_kind::endnote:
        return "endnote";
    case featherdoc::review_note_kind::comment:
        return "comment";
    }

    return "footnote";
}

auto semantic_revision_kind_name(featherdoc::revision_kind kind) -> std::string_view {
    switch (kind) {
    case featherdoc::revision_kind::insertion:
        return "insertion";
    case featherdoc::revision_kind::deletion:
        return "deletion";
    case featherdoc::revision_kind::move_from:
        return "move_from";
    case featherdoc::revision_kind::move_to:
        return "move_to";
    case featherdoc::revision_kind::paragraph_property_change:
        return "paragraph_property_change";
    case featherdoc::revision_kind::run_property_change:
        return "run_property_change";
    case featherdoc::revision_kind::unknown:
        return "unknown";
    }

    return "unknown";
}

auto semantic_review_note_summary_value(
    const featherdoc::review_note_summary &note) -> std::string {
    std::ostringstream stream;
    stream << "kind=" << semantic_review_note_kind_name(note.kind)
           << "\nid=" << note.id
           << "\nauthor=" << semantic_optional_string(note.author)
           << "\ninitials=" << semantic_optional_string(note.initials)
           << "\ndate=" << semantic_optional_string(note.date)
           << "\nanchor_text=" << semantic_optional_string(note.anchor_text)
           << "\nresolved=" << (note.resolved ? "true" : "false")
           << "\nparent_index=" << semantic_optional_numeric(note.parent_index)
           << "\nparent_id=" << semantic_optional_string(note.parent_id)
           << "\ntext=" << note.text;
    return stream.str();
}

auto semantic_revision_summary_value(const featherdoc::revision_summary &revision)
    -> std::string {
    std::ostringstream stream;
    stream << "kind=" << semantic_revision_kind_name(revision.kind)
           << "\nid=" << revision.id
           << "\nauthor=" << semantic_optional_string(revision.author)
           << "\ndate=" << semantic_optional_string(revision.date)
           << "\npart=" << revision.part_entry_name
           << "\ntext=" << revision.text;
    return stream.str();
}


auto semantic_page_orientation_value(featherdoc::page_orientation orientation)
    -> std::string_view {
    switch (orientation) {
    case featherdoc::page_orientation::portrait:
        return "portrait";
    case featherdoc::page_orientation::landscape:
        return "landscape";
    }

    return "portrait";
}

auto semantic_section_part_value(
    const featherdoc::section_part_inspection_summary &part) -> std::string {
    std::ostringstream stream;
    stream << "has_default=" << semantic_bool(part.has_default)
           << ";has_first=" << semantic_bool(part.has_first)
           << ";has_even=" << semantic_bool(part.has_even)
           << ";default_linked="
           << semantic_bool(part.default_linked_to_previous)
           << ";first_linked=" << semantic_bool(part.first_linked_to_previous)
           << ";even_linked=" << semantic_bool(part.even_linked_to_previous)
           << ";default_entry=" << semantic_optional_string(part.default_entry_name)
           << ";first_entry=" << semantic_optional_string(part.first_entry_name)
           << ";even_entry=" << semantic_optional_string(part.even_entry_name)
           << ";resolved_default="
           << semantic_optional_string(part.resolved_default_entry_name)
           << ";resolved_first="
           << semantic_optional_string(part.resolved_first_entry_name)
           << ";resolved_even="
           << semantic_optional_string(part.resolved_even_entry_name)
           << ";resolved_default_section="
           << semantic_optional_numeric(part.resolved_default_section_index)
           << ";resolved_first_section="
           << semantic_optional_numeric(part.resolved_first_section_index)
           << ";resolved_even_section="
           << semantic_optional_numeric(part.resolved_even_section_index);
    return stream.str();
}

auto semantic_section_page_setup_value(
    const std::optional<featherdoc::section_page_setup> &page_setup) -> std::string {
    if (!page_setup.has_value()) {
        return "implicit";
    }

    std::ostringstream stream;
    stream << "orientation=" << semantic_page_orientation_value(page_setup->orientation)
           << ";width=" << page_setup->width_twips
           << ";height=" << page_setup->height_twips
           << ";margin_top=" << page_setup->margins.top_twips
           << ";margin_right=" << page_setup->margins.right_twips
           << ";margin_bottom=" << page_setup->margins.bottom_twips
           << ";margin_left=" << page_setup->margins.left_twips
           << ";margin_header=" << page_setup->margins.header_twips
           << ";margin_footer=" << page_setup->margins.footer_twips
           << ";page_number_start="
           << semantic_optional_numeric(page_setup->page_number_start);
    return stream.str();
}

auto semantic_section_value(
    const featherdoc::section_inspection_summary &section,
    const std::optional<featherdoc::section_page_setup> &page_setup)
    -> std::string {
    std::ostringstream stream;
    stream << "even_and_odd="
           << semantic_bool(section.even_and_odd_headers_enabled)
           << "\ndifferent_first_page="
           << semantic_bool(section.different_first_page_enabled)
           << "\npage_setup=" << semantic_section_page_setup_value(page_setup)
           << "\nheader=" << semantic_section_part_value(section.header)
           << "\nfooter=" << semantic_section_part_value(section.footer);
    return stream.str();
}


struct semantic_template_part_view {
    featherdoc::template_schema_part_selector target{};
    std::string entry_name;
    std::optional<std::size_t> resolved_from_section_index;
    TemplatePart part;
};

void append_semantic_template_part(
    std::vector<semantic_template_part_view> &parts,
    featherdoc::template_schema_part_kind part_kind,
    std::optional<std::size_t> part_index,
    std::optional<std::size_t> section_index,
    featherdoc::section_reference_kind reference_kind, TemplatePart part) {
    auto target = featherdoc::template_schema_part_selector{};
    target.part = part_kind;
    target.part_index = part_index;
    target.section_index = section_index;
    target.reference_kind = reference_kind;
    parts.push_back(semantic_template_part_view{target,
                                                std::string{part.entry_name()},
                                                std::nullopt,
                                                std::move(part)});
}

void append_semantic_template_part(
    std::vector<semantic_template_part_view> &parts,
    featherdoc::template_schema_part_kind part_kind,
    std::optional<std::size_t> part_index, TemplatePart part) {
    append_semantic_template_part(
        parts, part_kind, part_index, std::nullopt,
        featherdoc::section_reference_kind::default_reference, std::move(part));
}

void append_resolved_semantic_section_template_part(
    std::vector<semantic_template_part_view> &parts,
    featherdoc::template_schema_part_kind part_kind, std::size_t section_index,
    featherdoc::section_reference_kind reference_kind,
    const std::optional<std::string> &resolved_entry_name,
    std::optional<std::size_t> resolved_from_section_index, TemplatePart part) {
    if (!resolved_entry_name.has_value()) {
        return;
    }

    auto entry_name = std::string{part.entry_name()};
    if (entry_name.empty() || entry_name != *resolved_entry_name) {
        return;
    }

    auto target = featherdoc::template_schema_part_selector{};
    target.part = part_kind;
    target.section_index = section_index;
    target.reference_kind = reference_kind;
    parts.push_back(semantic_template_part_view{target,
                                                std::move(entry_name),
                                                resolved_from_section_index,
                                                std::move(part)});
}

auto collect_semantic_template_parts(
    Document &document,
    const featherdoc::document_semantic_diff_options &options)
    -> std::vector<semantic_template_part_view> {
    auto parts = std::vector<semantic_template_part_view>{};
    append_semantic_template_part(parts,
                                  featherdoc::template_schema_part_kind::body,
                                  std::nullopt, document.body_template());

    const auto header_count = document.header_count();
    for (std::size_t index = 0U; index < header_count; ++index) {
        append_semantic_template_part(
            parts, featherdoc::template_schema_part_kind::header, index,
            document.header_template(index));
    }

    const auto footer_count = document.footer_count();
    for (std::size_t index = 0U; index < footer_count; ++index) {
        append_semantic_template_part(
            parts, featherdoc::template_schema_part_kind::footer, index,
            document.footer_template(index));
    }

    if (!options.compare_resolved_section_template_parts) {
        return parts;
    }

    const auto sections = document.inspect_sections();
    if (document.last_error().code) {
        return parts;
    }

    for (const auto &section : sections.sections) {
        append_resolved_semantic_section_template_part(
            parts, featherdoc::template_schema_part_kind::section_header,
            section.index, featherdoc::section_reference_kind::default_reference,
            section.header.resolved_default_entry_name,
            section.header.resolved_default_section_index,
            document.section_header_template(
                section.header.resolved_default_section_index.value_or(section.index),
                featherdoc::section_reference_kind::default_reference));
        append_resolved_semantic_section_template_part(
            parts, featherdoc::template_schema_part_kind::section_header,
            section.index, featherdoc::section_reference_kind::first_page,
            section.header.resolved_first_entry_name,
            section.header.resolved_first_section_index,
            document.section_header_template(
                section.header.resolved_first_section_index.value_or(section.index),
                featherdoc::section_reference_kind::first_page));
        append_resolved_semantic_section_template_part(
            parts, featherdoc::template_schema_part_kind::section_header,
            section.index, featherdoc::section_reference_kind::even_page,
            section.header.resolved_even_entry_name,
            section.header.resolved_even_section_index,
            document.section_header_template(
                section.header.resolved_even_section_index.value_or(section.index),
                featherdoc::section_reference_kind::even_page));
        append_resolved_semantic_section_template_part(
            parts, featherdoc::template_schema_part_kind::section_footer,
            section.index, featherdoc::section_reference_kind::default_reference,
            section.footer.resolved_default_entry_name,
            section.footer.resolved_default_section_index,
            document.section_footer_template(
                section.footer.resolved_default_section_index.value_or(section.index),
                featherdoc::section_reference_kind::default_reference));
        append_resolved_semantic_section_template_part(
            parts, featherdoc::template_schema_part_kind::section_footer,
            section.index, featherdoc::section_reference_kind::first_page,
            section.footer.resolved_first_entry_name,
            section.footer.resolved_first_section_index,
            document.section_footer_template(
                section.footer.resolved_first_section_index.value_or(section.index),
                featherdoc::section_reference_kind::first_page));
        append_resolved_semantic_section_template_part(
            parts, featherdoc::template_schema_part_kind::section_footer,
            section.index, featherdoc::section_reference_kind::even_page,
            section.footer.resolved_even_entry_name,
            section.footer.resolved_even_section_index,
            document.section_footer_template(
                section.footer.resolved_even_section_index.value_or(section.index),
                featherdoc::section_reference_kind::even_page));
    }

    return parts;
}

auto is_counted_semantic_template_part(
    const featherdoc::template_schema_part_selector &target) -> bool {
    return target.part == featherdoc::template_schema_part_kind::header ||
           target.part == featherdoc::template_schema_part_kind::footer;
}

auto semantic_template_part_targets_equal(
    const featherdoc::template_schema_part_selector &left,
    const featherdoc::template_schema_part_selector &right) -> bool {
    return left.part == right.part && left.part_index == right.part_index &&
           left.section_index == right.section_index &&
           left.reference_kind == right.reference_kind;
}

auto find_matching_semantic_template_part(
    const std::vector<semantic_template_part_view> &parts,
    const std::vector<bool> &matched,
    const featherdoc::template_schema_part_selector &target)
    -> std::optional<std::size_t> {
    for (std::size_t index = 0U; index < parts.size(); ++index) {
        if (!matched[index] &&
            semantic_template_part_targets_equal(parts[index].target, target)) {
            return index;
        }
    }

    return std::nullopt;
}

void accumulate_semantic_template_part_summary(
    featherdoc::document_semantic_diff_category_summary &summary,
    const featherdoc::document_semantic_diff_part_result &part_result) {
    if (part_result.different()) {
        ++summary.changed_count;
    } else {
        ++summary.unchanged_count;
    }
}


struct semantic_field_value {
    std::string path;
    std::string value;
};

auto is_semantic_field_name(std::string_view text) -> bool {
    if (text.empty()) {
        return false;
    }
    for (const auto ch : text) {
        const auto valid = (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') ||
                           (ch >= '0' && ch <= '9') || ch == '_';
        if (!valid) {
            return false;
        }
    }
    return true;
}

auto try_expand_semicolon_fields(std::string_view path, std::string_view value,
                                 std::vector<semantic_field_value> &fields)
    -> bool {
    if (value.find(';') == std::string_view::npos) {
        return false;
    }

    auto expanded = std::vector<semantic_field_value>{};
    std::size_t start = 0U;
    while (start <= value.size()) {
        const auto end = value.find(';', start);
        const auto segment = value.substr(
            start, end == std::string_view::npos ? std::string_view::npos
                                                 : end - start);
        if (segment.empty()) {
            return false;
        }
        const auto equals = segment.find('=');
        if (equals == std::string_view::npos || equals == 0U) {
            return false;
        }
        const auto name = segment.substr(0U, equals);
        if (!is_semantic_field_name(name)) {
            return false;
        }
        auto child_path = std::string{path};
        child_path.push_back('.');
        child_path.append(name.data(), name.size());
        expanded.push_back(semantic_field_value{
            std::move(child_path),
            std::string{segment.substr(equals + 1U)}});
        if (end == std::string_view::npos) {
            break;
        }
        start = end + 1U;
    }

    if (expanded.size() < 2U) {
        return false;
    }
    fields.insert(fields.end(), expanded.begin(), expanded.end());
    return true;
}

auto parse_semantic_field_values(std::string_view value)
    -> std::vector<semantic_field_value> {
    auto top_level = std::vector<semantic_field_value>{};
    std::size_t start = 0U;
    while (start <= value.size()) {
        const auto end = value.find('\n', start);
        const auto line = value.substr(
            start, end == std::string_view::npos ? std::string_view::npos
                                                 : end - start);
        const auto equals = line.find('=');
        if (equals != std::string_view::npos &&
            is_semantic_field_name(line.substr(0U, equals))) {
            top_level.push_back(semantic_field_value{
                std::string{line.substr(0U, equals)},
                std::string{line.substr(equals + 1U)}});
        } else if (!top_level.empty()) {
            top_level.back().value.push_back('\n');
            top_level.back().value.append(line.data(), line.size());
        } else if (!line.empty()) {
            top_level.push_back(semantic_field_value{"value", std::string{line}});
        }
        if (end == std::string_view::npos) {
            break;
        }
        start = end + 1U;
    }

    auto fields = std::vector<semantic_field_value>{};
    for (const auto &field : top_level) {
        if (!try_expand_semicolon_fields(field.path, field.value, fields)) {
            fields.push_back(field);
        }
    }
    return fields;
}

auto find_semantic_field_value(const std::vector<semantic_field_value> &fields,
                               std::string_view path) -> std::optional<std::string> {
    for (const auto &field : fields) {
        if (field.path == path) {
            return field.value;
        }
    }
    return std::nullopt;
}

auto semantic_field_changes(std::string_view left_value, std::string_view right_value)
    -> std::vector<featherdoc::document_semantic_diff_field_change> {
    const auto left_fields = parse_semantic_field_values(left_value);
    const auto right_fields = parse_semantic_field_values(right_value);
    auto changes = std::vector<featherdoc::document_semantic_diff_field_change>{};

    for (const auto &left_field : left_fields) {
        const auto right_field = find_semantic_field_value(right_fields, left_field.path);
        if (!right_field.has_value() || *right_field != left_field.value) {
            changes.push_back(featherdoc::document_semantic_diff_field_change{
                left_field.path, left_field.value,
                right_field.value_or(std::string{})});
        }
    }

    for (const auto &right_field : right_fields) {
        if (!find_semantic_field_value(left_fields, right_field.path).has_value()) {
            changes.push_back(featherdoc::document_semantic_diff_field_change{
                right_field.path, std::string{}, right_field.value});
        }
    }

    return changes;
}

auto make_semantic_sequence_change(std::string_view field)
    -> featherdoc::document_semantic_diff_change {
    auto change = featherdoc::document_semantic_diff_change{};
    change.field = std::string{field};
    return change;
}

void append_semantic_added_change(
    std::string_view field, const std::vector<std::string> &right_values,
    std::size_t right_index,
    featherdoc::document_semantic_diff_category_summary &summary,
    std::vector<featherdoc::document_semantic_diff_change> &changes) {
    ++summary.added_count;
    auto change = make_semantic_sequence_change(field);
    change.kind = featherdoc::document_semantic_diff_change_kind::added;
    change.right_index = right_index;
    change.right_value = right_values[right_index];
    changes.push_back(std::move(change));
}

void append_semantic_removed_change(
    std::string_view field, const std::vector<std::string> &left_values,
    std::size_t left_index,
    featherdoc::document_semantic_diff_category_summary &summary,
    std::vector<featherdoc::document_semantic_diff_change> &changes) {
    ++summary.removed_count;
    auto change = make_semantic_sequence_change(field);
    change.kind = featherdoc::document_semantic_diff_change_kind::removed;
    change.left_index = left_index;
    change.left_value = left_values[left_index];
    changes.push_back(std::move(change));
}

void append_semantic_changed_change(
    std::string_view field, const std::vector<std::string> &left_values,
    const std::vector<std::string> &right_values, std::size_t left_index,
    std::size_t right_index,
    featherdoc::document_semantic_diff_category_summary &summary,
    std::vector<featherdoc::document_semantic_diff_change> &changes) {
    ++summary.changed_count;
    auto change = make_semantic_sequence_change(field);
    change.kind = featherdoc::document_semantic_diff_change_kind::changed;
    change.left_index = left_index;
    change.right_index = right_index;
    change.left_value = left_values[left_index];
    change.right_value = right_values[right_index];
    change.field_changes = semantic_field_changes(change.left_value, change.right_value);
    changes.push_back(std::move(change));
}

void compare_semantic_values_by_index(
    const std::vector<std::string> &left_values,
    const std::vector<std::string> &right_values, std::string_view field,
    featherdoc::document_semantic_diff_category_summary &summary,
    std::vector<featherdoc::document_semantic_diff_change> &changes) {
    const auto item_count = std::max(left_values.size(), right_values.size());
    for (std::size_t item_index = 0; item_index < item_count; ++item_index) {
        if (item_index >= left_values.size()) {
            append_semantic_added_change(field, right_values, item_index, summary,
                                         changes);
            continue;
        }

        if (item_index >= right_values.size()) {
            append_semantic_removed_change(field, left_values, item_index, summary,
                                           changes);
            continue;
        }

        if (left_values[item_index] != right_values[item_index]) {
            append_semantic_changed_change(field, left_values, right_values,
                                           item_index, item_index, summary,
                                           changes);
        } else {
            ++summary.unchanged_count;
        }
    }
}

auto can_align_semantic_values(
    std::size_t left_count, std::size_t right_count,
    const featherdoc::document_semantic_diff_options &options) -> bool {
    if (!options.align_sequences_by_content) {
        return false;
    }

    const auto row_count = left_count + 1U;
    const auto column_count = right_count + 1U;
    if (row_count == 0U || column_count == 0U) {
        return false;
    }
    return row_count <= options.alignment_cell_limit / column_count;
}

void compare_semantic_values_by_content_alignment(
    const std::vector<std::string> &left_values,
    const std::vector<std::string> &right_values, std::string_view field,
    featherdoc::document_semantic_diff_category_summary &summary,
    std::vector<featherdoc::document_semantic_diff_change> &changes) {
    const auto left_count = left_values.size();
    const auto right_count = right_values.size();
    const auto column_count = right_count + 1U;
    auto cost = std::vector<std::size_t>((left_count + 1U) * column_count, 0U);
    const auto cost_index = [column_count](std::size_t left_position,
                                           std::size_t right_position) {
        return left_position * column_count + right_position;
    };

    for (std::size_t left_position = 1U; left_position <= left_count;
         ++left_position) {
        cost[cost_index(left_position, 0U)] = left_position;
    }
    for (std::size_t right_position = 1U; right_position <= right_count;
         ++right_position) {
        cost[cost_index(0U, right_position)] = right_position;
    }

    for (std::size_t left_position = 1U; left_position <= left_count;
         ++left_position) {
        for (std::size_t right_position = 1U; right_position <= right_count;
             ++right_position) {
            const auto substitution_cost =
                left_values[left_position - 1U] == right_values[right_position - 1U]
                    ? 0U
                    : 1U;
            auto best_cost = cost[cost_index(left_position - 1U,
                                             right_position - 1U)] +
                             substitution_cost;
            const auto removal_cost =
                cost[cost_index(left_position - 1U, right_position)] + 1U;
            if (removal_cost < best_cost) {
                best_cost = removal_cost;
            }
            const auto addition_cost =
                cost[cost_index(left_position, right_position - 1U)] + 1U;
            if (addition_cost < best_cost) {
                best_cost = addition_cost;
            }
            cost[cost_index(left_position, right_position)] = best_cost;
        }
    }

    auto aligned_changes = std::vector<featherdoc::document_semantic_diff_change>{};
    std::size_t left_position = left_count;
    std::size_t right_position = right_count;
    while (left_position > 0U || right_position > 0U) {
        const auto current_cost = cost[cost_index(left_position, right_position)];
        if (left_position > 0U && right_position > 0U) {
            const auto left_index = left_position - 1U;
            const auto right_index = right_position - 1U;
            const auto substitution_cost =
                left_values[left_index] == right_values[right_index] ? 0U : 1U;
            const auto diagonal_cost =
                cost[cost_index(left_position - 1U, right_position - 1U)] +
                substitution_cost;
            if (current_cost == diagonal_cost) {
                if (substitution_cost == 0U) {
                    ++summary.unchanged_count;
                } else {
                    append_semantic_changed_change(field, left_values, right_values,
                                                   left_index, right_index, summary,
                                                   aligned_changes);
                }
                --left_position;
                --right_position;
                continue;
            }
        }

        if (left_position > 0U &&
            current_cost == cost[cost_index(left_position - 1U, right_position)] +
                                1U) {
            append_semantic_removed_change(field, left_values, left_position - 1U,
                                           summary, aligned_changes);
            --left_position;
            continue;
        }

        append_semantic_added_change(field, right_values, right_position - 1U,
                                     summary, aligned_changes);
        --right_position;
    }

    std::reverse(aligned_changes.begin(), aligned_changes.end());
    changes.insert(changes.end(), aligned_changes.begin(), aligned_changes.end());
}

template <typename Item, typename Fingerprint>
void compare_semantic_sequence(
    const std::vector<Item> &left_items, const std::vector<Item> &right_items,
    std::string_view field, Fingerprint fingerprint,
    const featherdoc::document_semantic_diff_options &options,
    featherdoc::document_semantic_diff_category_summary &summary,
    std::vector<featherdoc::document_semantic_diff_change> &changes) {
    summary.left_count = left_items.size();
    summary.right_count = right_items.size();

    auto left_values = std::vector<std::string>{};
    auto right_values = std::vector<std::string>{};
    left_values.reserve(left_items.size());
    right_values.reserve(right_items.size());
    for (const auto &left_item : left_items) {
        left_values.push_back(fingerprint(left_item));
    }
    for (const auto &right_item : right_items) {
        right_values.push_back(fingerprint(right_item));
    }

    if (can_align_semantic_values(left_values.size(), right_values.size(), options)) {
        compare_semantic_values_by_content_alignment(left_values, right_values, field,
                                                     summary, changes);
        return;
    }

    compare_semantic_values_by_index(left_values, right_values, field, summary,
                                     changes);
}

template <typename Item, typename Key, typename Fingerprint>
void compare_semantic_sequence_by_key(
    const std::vector<Item> &left_items, const std::vector<Item> &right_items,
    std::string_view field, Key key, Fingerprint fingerprint,
    featherdoc::document_semantic_diff_category_summary &summary,
    std::vector<featherdoc::document_semantic_diff_change> &changes) {
    summary.left_count = left_items.size();
    summary.right_count = right_items.size();

    auto matched_right_items = std::vector<bool>(right_items.size(), false);
    for (std::size_t left_index = 0U; left_index < left_items.size(); ++left_index) {
        const auto left_key = key(left_items[left_index]);
        auto right_index = std::optional<std::size_t>{};
        for (std::size_t candidate_index = 0U; candidate_index < right_items.size();
             ++candidate_index) {
            if (!matched_right_items[candidate_index] &&
                key(right_items[candidate_index]) == left_key) {
                right_index = candidate_index;
                break;
            }
        }

        if (!right_index.has_value()) {
            auto values = std::vector<std::string>{fingerprint(left_items[left_index])};
            append_semantic_removed_change(field, values, 0U, summary, changes);
            changes.back().left_index = left_index;
            continue;
        }

        matched_right_items[*right_index] = true;
        const auto left_value = fingerprint(left_items[left_index]);
        const auto right_value = fingerprint(right_items[*right_index]);
        if (left_value != right_value) {
            auto left_values = std::vector<std::string>{left_value};
            auto right_values = std::vector<std::string>{right_value};
            append_semantic_changed_change(field, left_values, right_values, 0U, 0U,
                                           summary, changes);
            changes.back().left_index = left_index;
            changes.back().right_index = *right_index;
        } else {
            ++summary.unchanged_count;
        }
    }

    for (std::size_t right_index = 0U; right_index < right_items.size(); ++right_index) {
        if (!matched_right_items[right_index]) {
            auto values = std::vector<std::string>{fingerprint(right_items[right_index])};
            append_semantic_added_change(field, values, 0U, summary, changes);
            changes.back().right_index = right_index;
        }
    }
}

} // namespace

std::optional<featherdoc::document_semantic_diff_result>
Document::compare_semantic(
    const Document &other,
    featherdoc::document_semantic_diff_options options) const {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before comparing documents",
                       std::string{document_xml_entry});
        return std::nullopt;
    }

    if (!other.is_open()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "right document must be opened before semantic comparison");
        return std::nullopt;
    }

    auto &left = const_cast<Document &>(*this);
    auto &right = const_cast<Document &>(other);
    auto result = featherdoc::document_semantic_diff_result{};

    if (options.compare_paragraphs) {
        const auto left_paragraphs = left.inspect_paragraphs();
        if (this->last_error_info.code) {
            return std::nullopt;
        }
        const auto right_paragraphs = right.inspect_paragraphs();
        if (other.last_error().code) {
            this->last_error_info = other.last_error();
            return std::nullopt;
        }
        compare_semantic_sequence(
            left_paragraphs, right_paragraphs, "paragraph", semantic_paragraph_value,
            options, result.paragraphs, result.paragraph_changes);
    }

    if (options.compare_tables) {
        const auto left_tables = left.inspect_tables();
        if (this->last_error_info.code) {
            return std::nullopt;
        }
        const auto right_tables = right.inspect_tables();
        if (other.last_error().code) {
            this->last_error_info = other.last_error();
            return std::nullopt;
        }
        compare_semantic_sequence(left_tables, right_tables, "table",
                                  semantic_table_value, options, result.tables,
                                  result.table_changes);
    }

    if (options.compare_images) {
        const auto left_images = this->drawing_images();
        if (this->last_error_info.code) {
            return std::nullopt;
        }
        const auto right_images = other.drawing_images();
        if (other.last_error().code) {
            this->last_error_info = other.last_error();
            return std::nullopt;
        }
        compare_semantic_sequence(
            left_images, right_images, "image",
            [&options](const featherdoc::drawing_image_info &image) {
                return semantic_image_value(image, options);
            },
            options, result.images, result.image_changes);
    }

    if (options.compare_content_controls) {
        const auto left_controls = this->list_content_controls();
        if (this->last_error_info.code) {
            return std::nullopt;
        }
        const auto right_controls = other.list_content_controls();
        if (other.last_error().code) {
            this->last_error_info = other.last_error();
            return std::nullopt;
        }
        compare_semantic_sequence(
            left_controls, right_controls, "content_control",
            [&options](const featherdoc::content_control_summary &content_control) {
                return semantic_content_control_value(content_control, options);
            },
            options, result.content_controls, result.content_control_changes);
    }

    if (options.compare_fields) {
        const auto left_fields = left.body_template().list_fields();
        if (this->last_error_info.code) {
            return std::nullopt;
        }
        const auto right_fields = right.body_template().list_fields();
        if (other.last_error().code) {
            this->last_error_info = other.last_error();
            return std::nullopt;
        }
        compare_semantic_sequence(left_fields, right_fields, "field",
                                  semantic_field_summary_value, options,
                                  result.fields, result.field_changes);
    }

    if (options.compare_styles) {
        const auto left_styles = left.list_styles();
        if (this->last_error_info.code) {
            return std::nullopt;
        }
        const auto right_styles = right.list_styles();
        if (other.last_error().code) {
            this->last_error_info = other.last_error();
            return std::nullopt;
        }
        compare_semantic_sequence_by_key(
            left_styles, right_styles, "style",
            [](const featherdoc::style_summary &style) { return style.style_id; },
            semantic_style_summary_value, result.styles, result.style_changes);
    }

    if (options.compare_numbering) {
        const auto left_numbering = left.list_numbering_definitions();
        if (this->last_error_info.code) {
            return std::nullopt;
        }
        const auto right_numbering = right.list_numbering_definitions();
        if (other.last_error().code) {
            this->last_error_info = other.last_error();
            return std::nullopt;
        }
        compare_semantic_sequence_by_key(
            left_numbering, right_numbering, "numbering",
            [](const featherdoc::numbering_definition_summary &definition) {
                return definition.definition_id;
            },
            semantic_numbering_definition_summary_value, result.numbering,
            result.numbering_changes);
    }

    if (options.compare_footnotes) {
        const auto left_footnotes = left.list_footnotes();
        if (this->last_error_info.code) {
            return std::nullopt;
        }
        const auto right_footnotes = right.list_footnotes();
        if (other.last_error().code) {
            this->last_error_info = other.last_error();
            return std::nullopt;
        }
        compare_semantic_sequence(left_footnotes, right_footnotes, "footnote",
                                  semantic_review_note_summary_value, options,
                                  result.footnotes, result.footnote_changes);
    }

    if (options.compare_endnotes) {
        const auto left_endnotes = left.list_endnotes();
        if (this->last_error_info.code) {
            return std::nullopt;
        }
        const auto right_endnotes = right.list_endnotes();
        if (other.last_error().code) {
            this->last_error_info = other.last_error();
            return std::nullopt;
        }
        compare_semantic_sequence(left_endnotes, right_endnotes, "endnote",
                                  semantic_review_note_summary_value, options,
                                  result.endnotes, result.endnote_changes);
    }

    if (options.compare_comments) {
        const auto left_comments = left.list_comments();
        if (this->last_error_info.code) {
            return std::nullopt;
        }
        const auto right_comments = right.list_comments();
        if (other.last_error().code) {
            this->last_error_info = other.last_error();
            return std::nullopt;
        }
        compare_semantic_sequence(left_comments, right_comments, "comment",
                                  semantic_review_note_summary_value, options,
                                  result.comments, result.comment_changes);
    }

    if (options.compare_revisions) {
        const auto left_revisions = left.list_revisions();
        if (this->last_error_info.code) {
            return std::nullopt;
        }
        const auto right_revisions = right.list_revisions();
        if (other.last_error().code) {
            this->last_error_info = other.last_error();
            return std::nullopt;
        }
        compare_semantic_sequence(left_revisions, right_revisions, "revision",
                                  semantic_revision_summary_value, options,
                                  result.revisions, result.revision_changes);
    }

    if (options.compare_template_parts) {
        auto left_parts = collect_semantic_template_parts(left, options);
        if (this->last_error_info.code) {
            return std::nullopt;
        }
        auto right_parts = collect_semantic_template_parts(right, options);
        if (other.last_error().code) {
            this->last_error_info = other.last_error();
            return std::nullopt;
        }

        for (const auto &part : left_parts) {
            if (is_counted_semantic_template_part(part.target)) {
                ++result.template_parts.left_count;
            }
        }
        for (const auto &part : right_parts) {
            if (is_counted_semantic_template_part(part.target)) {
                ++result.template_parts.right_count;
            }
        }

        auto matched_right_parts = std::vector<bool>(right_parts.size(), false);
        for (auto &left_part : left_parts) {
            const auto right_index = find_matching_semantic_template_part(
                right_parts, matched_right_parts, left_part.target);
            if (!right_index.has_value()) {
                if (is_counted_semantic_template_part(left_part.target)) {
                    ++result.template_parts.removed_count;
                }
                continue;
            }

            auto &right_part = right_parts[*right_index];
            auto part_result = featherdoc::document_semantic_diff_part_result{};
            part_result.target = left_part.target;
            part_result.entry_name = left_part.entry_name;
            if (right_part.entry_name != left_part.entry_name &&
                !right_part.entry_name.empty()) {
                part_result.entry_name += " -> ";
                part_result.entry_name += right_part.entry_name;
            }
            if (left_part.target.part ==
                    featherdoc::template_schema_part_kind::section_header ||
                left_part.target.part ==
                    featherdoc::template_schema_part_kind::section_footer) {
                part_result.left_resolved_from_section_index =
                    left_part.resolved_from_section_index;
                part_result.right_resolved_from_section_index =
                    right_part.resolved_from_section_index;
            }

            if (options.compare_paragraphs) {
                const auto left_paragraphs = left_part.part.inspect_paragraphs();
                if (this->last_error_info.code) {
                    return std::nullopt;
                }
                const auto right_paragraphs = right_part.part.inspect_paragraphs();
                if (other.last_error().code) {
                    this->last_error_info = other.last_error();
                    return std::nullopt;
                }
                compare_semantic_sequence(
                    left_paragraphs, right_paragraphs, "paragraph",
                    semantic_paragraph_value, options, part_result.paragraphs,
                    part_result.paragraph_changes);
            }

            if (options.compare_tables) {
                const auto left_tables = left_part.part.inspect_tables();
                if (this->last_error_info.code) {
                    return std::nullopt;
                }
                const auto right_tables = right_part.part.inspect_tables();
                if (other.last_error().code) {
                    this->last_error_info = other.last_error();
                    return std::nullopt;
                }
                compare_semantic_sequence(left_tables, right_tables, "table",
                                          semantic_table_value, options,
                                          part_result.tables,
                                          part_result.table_changes);
            }

            if (options.compare_images) {
                const auto left_images = left_part.part.drawing_images();
                if (this->last_error_info.code) {
                    return std::nullopt;
                }
                const auto right_images = right_part.part.drawing_images();
                if (other.last_error().code) {
                    this->last_error_info = other.last_error();
                    return std::nullopt;
                }
                compare_semantic_sequence(
                    left_images, right_images, "image",
                    [&options](const featherdoc::drawing_image_info &image) {
                        return semantic_image_value(image, options);
                    },
                    options, part_result.images, part_result.image_changes);
            }

            if (options.compare_content_controls) {
                const auto left_controls = left_part.part.list_content_controls();
                if (this->last_error_info.code) {
                    return std::nullopt;
                }
                const auto right_controls = right_part.part.list_content_controls();
                if (other.last_error().code) {
                    this->last_error_info = other.last_error();
                    return std::nullopt;
                }
                compare_semantic_sequence(
                    left_controls, right_controls, "content_control",
                    [&options](
                        const featherdoc::content_control_summary &content_control) {
                        return semantic_content_control_value(content_control, options);
                    },
                    options, part_result.content_controls,
                    part_result.content_control_changes);
            }

            if (options.compare_fields) {
                const auto left_fields = left_part.part.list_fields();
                if (this->last_error_info.code) {
                    return std::nullopt;
                }
                const auto right_fields = right_part.part.list_fields();
                if (other.last_error().code) {
                    this->last_error_info = other.last_error();
                    return std::nullopt;
                }
                compare_semantic_sequence(left_fields, right_fields, "field",
                                          semantic_field_summary_value, options,
                                          part_result.fields,
                                          part_result.field_changes);
            }

            if (is_counted_semantic_template_part(part_result.target)) {
                accumulate_semantic_template_part_summary(result.template_parts,
                                                          part_result);
            }
            result.template_part_results.push_back(std::move(part_result));
            matched_right_parts[*right_index] = true;
        }

        for (std::size_t index = 0U; index < right_parts.size(); ++index) {
            if (!matched_right_parts[index] &&
                is_counted_semantic_template_part(right_parts[index].target)) {
                ++result.template_parts.added_count;
            }
        }
    }

    if (options.compare_sections) {
        const auto left_sections = left.inspect_sections();
        if (this->last_error_info.code) {
            return std::nullopt;
        }
        const auto right_sections = right.inspect_sections();
        if (other.last_error().code) {
            this->last_error_info = other.last_error();
            return std::nullopt;
        }

        auto left_section_values = std::vector<std::string>{};
        auto right_section_values = std::vector<std::string>{};
        left_section_values.reserve(left_sections.sections.size());
        right_section_values.reserve(right_sections.sections.size());
        for (const auto &section : left_sections.sections) {
            auto page_setup = this->get_section_page_setup(section.index);
            if (this->last_error_info.code) {
                return std::nullopt;
            }
            left_section_values.push_back(semantic_section_value(section, page_setup));
        }
        for (const auto &section : right_sections.sections) {
            auto page_setup = other.get_section_page_setup(section.index);
            if (other.last_error().code) {
                this->last_error_info = other.last_error();
                return std::nullopt;
            }
            right_section_values.push_back(semantic_section_value(section, page_setup));
        }

        result.sections.left_count = left_section_values.size();
        result.sections.right_count = right_section_values.size();
        if (can_align_semantic_values(left_section_values.size(),
                                      right_section_values.size(), options)) {
            compare_semantic_values_by_content_alignment(
                left_section_values, right_section_values, "section", result.sections,
                result.section_changes);
        } else {
            compare_semantic_values_by_index(left_section_values, right_section_values,
                                             "section", result.sections,
                                             result.section_changes);
        }
    }

    this->last_error_info.clear();
    return result;
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
