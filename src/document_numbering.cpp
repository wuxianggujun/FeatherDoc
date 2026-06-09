#include "featherdoc.hpp"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <filesystem>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>

#include <zip.h>

namespace {
constexpr auto document_xml_entry = std::string_view{"word/document.xml"};
constexpr auto document_relationships_xml_entry =
    std::string_view{"word/_rels/document.xml.rels"};
constexpr auto content_types_xml_entry = std::string_view{"[Content_Types].xml"};
constexpr auto numbering_xml_entry = std::string_view{"word/numbering.xml"};
constexpr auto styles_xml_entry = std::string_view{"word/styles.xml"};
constexpr auto numbering_relationship_type = std::string_view{
    "http://schemas.openxmlformats.org/officeDocument/2006/relationships/numbering"};
constexpr auto numbering_content_type = std::string_view{
    "application/vnd.openxmlformats-officedocument.wordprocessingml.numbering+xml"};
constexpr auto empty_relationships_xml = std::string_view{
    R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
</Relationships>
)"}; 
constexpr auto empty_numbering_xml = std::string_view{
    R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:numbering xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
</w:numbering>
)"}; 
constexpr auto managed_bullet_list_name = std::string_view{"FeatherDocBulletList"};
constexpr auto managed_decimal_list_name = std::string_view{"FeatherDocDecimalList"};
constexpr auto max_list_level = 8U;

auto initialize_xml_document(pugi::xml_document &xml_document, std::string_view xml_text)
    -> bool {
    xml_document.reset();
    return static_cast<bool>(
        xml_document.load_buffer(xml_text.data(), xml_text.size()));
}

auto initialize_empty_relationships_document(pugi::xml_document &xml_document) -> bool {
    return initialize_xml_document(xml_document, empty_relationships_xml);
}

auto set_last_error(featherdoc::document_error_info &error_info,
                    std::error_code code, std::string detail = {},
                    std::string entry_name = {},
                    std::optional<std::ptrdiff_t> xml_offset = std::nullopt)
    -> std::error_code {
    error_info.code = code;
    error_info.detail = std::move(detail);
    error_info.entry_name = std::move(entry_name);
    error_info.xml_offset = std::move(xml_offset);
    return code;
}

auto set_last_error(featherdoc::document_error_info &error_info,
                    featherdoc::document_errc code, std::string detail = {},
                    std::string entry_name = {},
                    std::optional<std::ptrdiff_t> xml_offset = std::nullopt)
    -> std::error_code {
    return set_last_error(error_info, featherdoc::make_error_code(code),
                          std::move(detail), std::move(entry_name), std::move(xml_offset));
}

enum class zip_entry_read_status {
    ok,
    missing,
    read_failed,
};

auto zip_error_text(int error_number) -> std::string {
    if (const char *message = zip_strerror(error_number); message != nullptr) {
        return message;
    }

    return "unknown zip error";
}

auto read_zip_entry_text(zip_t *archive, std::string_view entry_name, std::string &content)
    -> zip_entry_read_status {
    if (zip_entry_open(archive, entry_name.data()) != 0) {
        return zip_entry_read_status::missing;
    }

    void *buffer = nullptr;
    size_t buffer_size = 0;
    const auto read_result = zip_entry_read(archive, &buffer, &buffer_size);
    const auto close_result = zip_entry_close(archive);

    if (read_result < 0 || close_result != 0) {
        if (buffer != nullptr) {
            std::free(buffer);
        }
        return zip_entry_read_status::read_failed;
    }

    content.assign(static_cast<const char *>(buffer), buffer_size);
    std::free(buffer);
    return zip_entry_read_status::ok;
}

auto normalize_word_part_entry(std::string_view target) -> std::string {
    std::string normalized_target{target};
    std::replace(normalized_target.begin(), normalized_target.end(), '\\', '/');

    if (!normalized_target.empty() && normalized_target.front() == '/') {
        return std::filesystem::path{normalized_target.substr(1)}
            .lexically_normal()
            .generic_string();
    }

    return (std::filesystem::path{"word"} / std::filesystem::path{normalized_target})
        .lexically_normal()
        .generic_string();
}

auto make_override_part_name(std::string_view entry_name) -> std::string {
    if (entry_name.empty()) {
        return {};
    }

    if (entry_name.front() == '/') {
        return std::string{entry_name};
    }

    return "/" + std::string{entry_name};
}

auto make_document_relationship_target(std::string_view entry_name) -> std::string {
    const auto normalized_entry = std::filesystem::path{std::string{entry_name}}
                                      .lexically_normal();
    const auto relative_target =
        normalized_entry.lexically_relative(std::filesystem::path{"word"});
    if (!relative_target.empty()) {
        return relative_target.generic_string();
    }

    return normalized_entry.filename().generic_string();
}

auto find_document_relationship_by_type(pugi::xml_node relationships,
                                        std::string_view relationship_type)
    -> pugi::xml_node {
    for (auto relationship = relationships.child("Relationship");
         relationship != pugi::xml_node{};
         relationship = relationship.next_sibling("Relationship")) {
        if (std::string_view{relationship.attribute("Type").value()} == relationship_type) {
            return relationship;
        }
    }

    return {};
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

auto ensure_paragraph_properties_node(pugi::xml_node paragraph) -> pugi::xml_node {
    if (paragraph == pugi::xml_node{}) {
        return {};
    }

    auto paragraph_properties = paragraph.child("w:pPr");
    if (paragraph_properties != pugi::xml_node{}) {
        return paragraph_properties;
    }

    if (const auto first_child = paragraph.first_child(); first_child != pugi::xml_node{}) {
        return paragraph.insert_child_before("w:pPr", first_child);
    }

    return paragraph.append_child("w:pPr");
}

auto ensure_style_paragraph_properties_node(pugi::xml_node style) -> pugi::xml_node {
    if (style == pugi::xml_node{}) {
        return {};
    }

    auto paragraph_properties = style.child("w:pPr");
    if (paragraph_properties != pugi::xml_node{}) {
        return paragraph_properties;
    }

    if (const auto run_properties = style.child("w:rPr"); run_properties != pugi::xml_node{}) {
        return style.insert_child_before("w:pPr", run_properties);
    }

    return style.append_child("w:pPr");
}

auto node_has_attributes(pugi::xml_node node) -> bool {
    return node.first_attribute() != pugi::xml_attribute{};
}

void remove_empty_paragraph_properties(pugi::xml_node paragraph) {
    if (paragraph == pugi::xml_node{}) {
        return;
    }

    auto paragraph_properties = paragraph.child("w:pPr");
    if (paragraph_properties == pugi::xml_node{}) {
        return;
    }

    if (paragraph_properties.first_child() == pugi::xml_node{} &&
        !node_has_attributes(paragraph_properties)) {
        paragraph.remove_child(paragraph_properties);
    }
}

auto max_numbering_id(pugi::xml_node numbering_root, const char *child_name,
                      const char *attribute_name) -> std::uint32_t {
    std::uint32_t max_id = 0U;
    for (auto child = numbering_root.child(child_name); child != pugi::xml_node{};
         child = child.next_sibling(child_name)) {
        if (const auto parsed_id =
                parse_u32_attribute_value(child.attribute(attribute_name).value())) {
            max_id = std::max(max_id, *parsed_id);
        }
    }
    return max_id;
}

auto list_name_for(featherdoc::list_kind kind) -> std::string_view {
    switch (kind) {
    case featherdoc::list_kind::bullet:
        return managed_bullet_list_name;
    case featherdoc::list_kind::decimal:
        return managed_decimal_list_name;
    }

    return managed_bullet_list_name;
}

auto numbering_format_name(featherdoc::list_kind kind) -> std::string_view {
    switch (kind) {
    case featherdoc::list_kind::bullet:
        return "bullet";
    case featherdoc::list_kind::decimal:
        return "decimal";
    }

    return "bullet";
}

auto bullet_symbol_for_level(std::uint32_t level) -> std::string_view {
    constexpr auto bullet = std::string_view{"\xE2\x80\xA2"};
    constexpr auto circle = std::string_view{"o"};
    constexpr auto square = std::string_view{"\xE2\x96\xAA"};
    constexpr std::array<std::string_view, 3U> symbols{bullet, circle, square};
    return symbols[level % symbols.size()];
}

auto decimal_level_text(std::uint32_t level) -> std::string {
    std::string text;
    for (std::uint32_t index = 0U; index <= level; ++index) {
        text += "%" + std::to_string(index + 1U);
        text.push_back('.');
    }
    return text;
}

auto find_abstract_numbering_by_name(pugi::xml_node numbering_root, std::string_view name)
    -> pugi::xml_node {
    for (auto abstract_num = numbering_root.child("w:abstractNum");
         abstract_num != pugi::xml_node{};
         abstract_num = abstract_num.next_sibling("w:abstractNum")) {
        if (std::string_view{abstract_num.child("w:name").attribute("w:val").value()} == name) {
            return abstract_num;
        }
    }

    return {};
}

auto find_abstract_numbering_by_id(pugi::xml_node numbering_root,
                                   std::uint32_t abstract_num_id) -> pugi::xml_node {
    const auto abstract_num_id_text = std::to_string(abstract_num_id);
    for (auto abstract_num = numbering_root.child("w:abstractNum");
         abstract_num != pugi::xml_node{};
         abstract_num = abstract_num.next_sibling("w:abstractNum")) {
        if (std::string_view{abstract_num.attribute("w:abstractNumId").value()} ==
            abstract_num_id_text) {
            return abstract_num;
        }
    }

    return {};
}

auto find_level_definition(pugi::xml_node abstract_num, std::uint32_t level)
    -> pugi::xml_node {
    const auto level_text = std::to_string(level);
    for (auto level_node = abstract_num.child("w:lvl"); level_node != pugi::xml_node{};
         level_node = level_node.next_sibling("w:lvl")) {
        if (std::string_view{level_node.attribute("w:ilvl").value()} == level_text) {
            return level_node;
        }
    }

    return {};
}

void remove_named_children(pugi::xml_node parent, const char *child_name) {
    if (parent == pugi::xml_node{}) {
        return;
    }

    for (auto child = parent.child(child_name); child != pugi::xml_node{};
         child = parent.child(child_name)) {
        parent.remove_child(child);
    }
}

void remove_empty_style_paragraph_properties(pugi::xml_node style) {
    if (style == pugi::xml_node{}) {
        return;
    }

    auto paragraph_properties = style.child("w:pPr");
    if (paragraph_properties == pugi::xml_node{}) {
        return;
    }

    if (paragraph_properties.first_child() == pugi::xml_node{} &&
        !node_has_attributes(paragraph_properties)) {
        style.remove_child(paragraph_properties);
    }
}

auto find_style_node(pugi::xml_node styles_root, std::string_view style_id) -> pugi::xml_node {
    for (auto style = styles_root.child("w:style"); style != pugi::xml_node{};
         style = style.next_sibling("w:style")) {
        if (std::string_view{style.attribute("w:styleId").value()} == style_id) {
            return style;
        }
    }

    return {};
}

auto find_numbering_instance_for_abstract(pugi::xml_node numbering_root,
                                          std::uint32_t abstract_num_id)
    -> pugi::xml_node {
    const auto abstract_num_id_text = std::to_string(abstract_num_id);
    for (auto num = numbering_root.child("w:num"); num != pugi::xml_node{};
         num = num.next_sibling("w:num")) {
        if (std::string_view{num.child("w:abstractNumId").attribute("w:val").value()} ==
            abstract_num_id_text) {
            return num;
        }
    }

    return {};
}

auto find_numbering_instance_by_id(pugi::xml_node numbering_root, std::uint32_t num_id)
    -> pugi::xml_node {
    const auto num_id_text = std::to_string(num_id);
    for (auto num = numbering_root.child("w:num"); num != pugi::xml_node{};
         num = num.next_sibling("w:num")) {
        if (std::string_view{num.attribute("w:numId").value()} == num_id_text) {
            return num;
        }
    }

    return {};
}

auto append_custom_level_definition(
    pugi::xml_node abstract_num,
    const featherdoc::numbering_level_definition &definition) -> bool {
    auto level_node = abstract_num.append_child("w:lvl");
    if (level_node == pugi::xml_node{}) {
        return false;
    }
    ensure_attribute_value(level_node, "w:ilvl", std::to_string(definition.level));

    auto start = level_node.append_child("w:start");
    if (start == pugi::xml_node{}) {
        return false;
    }
    ensure_attribute_value(start, "w:val", std::to_string(definition.start));

    auto num_format = level_node.append_child("w:numFmt");
    auto level_text = level_node.append_child("w:lvlText");
    auto level_justification = level_node.append_child("w:lvlJc");
    auto paragraph_properties = level_node.append_child("w:pPr");
    if (num_format == pugi::xml_node{} || level_text == pugi::xml_node{} ||
        level_justification == pugi::xml_node{} || paragraph_properties == pugi::xml_node{}) {
        return false;
    }

    ensure_attribute_value(num_format, "w:val", numbering_format_name(definition.kind));
    ensure_attribute_value(level_text, "w:val", definition.text_pattern);
    ensure_attribute_value(level_justification, "w:val", "left");

    auto indentation = paragraph_properties.append_child("w:ind");
    if (indentation == pugi::xml_node{}) {
        return false;
    }
    const auto left_indent = std::to_string((definition.level + 1U) * 720U);
    ensure_attribute_value(indentation, "w:left", left_indent);
    ensure_attribute_value(indentation, "w:hanging", "360");
    return true;
}

auto append_level_definition(pugi::xml_node abstract_num, featherdoc::list_kind kind,
                             std::uint32_t level) -> bool {
    auto definition = featherdoc::numbering_level_definition{};
    definition.kind = kind;
    definition.start = 1U;
    definition.level = level;
    definition.text_pattern =
        kind == featherdoc::list_kind::bullet ? std::string{bullet_symbol_for_level(level)}
                                              : decimal_level_text(level);
    return append_custom_level_definition(abstract_num, definition);
}

auto sort_numbering_levels(
    const std::vector<featherdoc::numbering_level_definition> &levels)
    -> std::vector<featherdoc::numbering_level_definition> {
    auto sorted_levels = levels;
    std::sort(sorted_levels.begin(), sorted_levels.end(),
              [](const auto &lhs, const auto &rhs) { return lhs.level < rhs.level; });
    return sorted_levels;
}

#include "document_numbering_summary_helpers.inc"

#include "document_numbering_instance_helpers.inc"

} // namespace

namespace featherdoc {

#include "document_numbering_catalog_methods.inc"

std::optional<std::uint32_t> Document::ensure_style_linked_numbering(
    const featherdoc::numbering_definition &definition,
    const std::vector<featherdoc::paragraph_style_numbering_link> &style_links) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before editing style-linked numbering");
        return std::nullopt;
    }

    auto validation_detail = std::string{};
    if (!validate_numbering_definition(definition, validation_detail)) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       std::move(validation_detail), std::string{numbering_xml_entry});
        return std::nullopt;
    }

    if (style_links.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "expected at least one paragraph style link",
                       std::string{styles_xml_entry});
        return std::nullopt;
    }

    for (std::size_t index = 0; index < style_links.size(); ++index) {
        const auto &style_link = style_links[index];
        if (style_link.style_id.empty()) {
            set_last_error(this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           "style link style id must not be empty",
                           std::string{styles_xml_entry});
            return std::nullopt;
        }

        if (style_link.level > max_list_level) {
            set_last_error(this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           "style link numbering level must be in the range [0, 8]",
                           std::string{styles_xml_entry});
            return std::nullopt;
        }

        for (std::size_t duplicate_index = index + 1U; duplicate_index < style_links.size();
             ++duplicate_index) {
            if (style_links[duplicate_index].style_id == style_link.style_id) {
                set_last_error(this->last_error_info,
                               std::make_error_code(std::errc::invalid_argument),
                               "style id '" + style_link.style_id +
                                   "' appears more than once in the style link list",
                               std::string{styles_xml_entry});
                return std::nullopt;
            }
        }
    }

    if (const auto error = this->ensure_styles_part_attached()) {
        return std::nullopt;
    }
    if (const auto error = this->ensure_numbering_part_attached()) {
        return std::nullopt;
    }

    auto styles_root = this->styles.child("w:styles");
    if (styles_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info, document_errc::styles_xml_parse_failed,
                       "word/styles.xml does not contain a w:styles root",
                       std::string{styles_xml_entry});
        return std::nullopt;
    }

    for (const auto &style_link : style_links) {
        const auto style = find_style_node(styles_root, style_link.style_id);
        if (style == pugi::xml_node{}) {
            set_last_error(this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           "style id '" + style_link.style_id +
                               "' was not found in word/styles.xml",
                           std::string{styles_xml_entry});
            return std::nullopt;
        }

        if (std::string_view{style.attribute("w:type").value()} != "paragraph") {
            set_last_error(this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           "style id '" + style_link.style_id +
                               "' is not a paragraph style and cannot carry paragraph numbering",
                           std::string{styles_xml_entry});
            return std::nullopt;
        }
    }

    const auto numbering_definition_id = this->ensure_numbering_definition(definition);
    if (!numbering_definition_id.has_value()) {
        return std::nullopt;
    }

    auto numbering_root = this->numbering.child("w:numbering");
    if (numbering_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       document_errc::numbering_xml_parse_failed,
                       "word/numbering.xml does not contain the expected w:numbering root",
                       std::string{numbering_xml_entry});
        return std::nullopt;
    }

    const auto abstract_num =
        find_abstract_numbering_by_id(numbering_root, *numbering_definition_id);
    if (abstract_num == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "numbering definition id does not exist",
                       std::string{numbering_xml_entry});
        return std::nullopt;
    }

    for (const auto &style_link : style_links) {
        if (find_level_definition(abstract_num, style_link.level) == pugi::xml_node{}) {
            set_last_error(this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           "requested numbering level is not defined by the target definition",
                           std::string{numbering_xml_entry});
            return std::nullopt;
        }
    }

    const auto num_id =
        ensure_numbering_instance_for_abstract(numbering_root, *numbering_definition_id);
    if (!num_id.has_value()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to create or reuse a numbering instance for the target definition",
                       std::string{numbering_xml_entry});
        return std::nullopt;
    }

    styles_root = this->styles.child("w:styles");
    for (const auto &style_link : style_links) {
        const auto style = find_style_node(styles_root, style_link.style_id);
        if (!apply_numbering_to_style(style, style_link.level, *num_id)) {
            set_last_error(this->last_error_info,
                           std::make_error_code(std::errc::not_enough_memory),
                           "failed to populate numbering metadata for the target style",
                           std::string{styles_xml_entry});
            return std::nullopt;
        }
    }

    this->numbering_dirty = true;
    this->styles_dirty = true;
    this->last_error_info.clear();
    return numbering_definition_id;
}

#include "document_numbering_paragraph_methods.inc"

} // namespace featherdoc
