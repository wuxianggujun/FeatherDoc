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

namespace {
constexpr auto document_xml_entry = std::string_view{"word/document.xml"};
constexpr auto document_relationships_xml_entry =
    std::string_view{"word/_rels/document.xml.rels"};
constexpr auto content_types_xml_entry = std::string_view{"[Content_Types].xml"};
constexpr auto numbering_xml_entry = std::string_view{"word/numbering.xml"};
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

auto find_managed_abstract_numbering(pugi::xml_node numbering_root, std::string_view name)
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

auto append_level_definition(pugi::xml_node abstract_num, featherdoc::list_kind kind,
                             std::uint32_t level) -> bool {
    auto level_node = abstract_num.append_child("w:lvl");
    if (level_node == pugi::xml_node{}) {
        return false;
    }
    ensure_attribute_value(level_node, "w:ilvl", std::to_string(level));

    if (kind == featherdoc::list_kind::decimal) {
        auto start = level_node.append_child("w:start");
        if (start == pugi::xml_node{}) {
            return false;
        }
        ensure_attribute_value(start, "w:val", "1");
    }

    auto num_format = level_node.append_child("w:numFmt");
    auto level_text = level_node.append_child("w:lvlText");
    auto level_justification = level_node.append_child("w:lvlJc");
    auto paragraph_properties = level_node.append_child("w:pPr");
    if (num_format == pugi::xml_node{} || level_text == pugi::xml_node{} ||
        level_justification == pugi::xml_node{} || paragraph_properties == pugi::xml_node{}) {
        return false;
    }

    ensure_attribute_value(num_format, "w:val",
                           kind == featherdoc::list_kind::bullet ? "bullet" : "decimal");
    if (kind == featherdoc::list_kind::bullet) {
        ensure_attribute_value(level_text, "w:val", bullet_symbol_for_level(level));
    } else {
        ensure_attribute_value(level_text, "w:val", decimal_level_text(level));
    }
    ensure_attribute_value(level_justification, "w:val", "left");

    auto indentation = paragraph_properties.append_child("w:ind");
    if (indentation == pugi::xml_node{}) {
        return false;
    }
    const auto left_indent = std::to_string((level + 1U) * 720U);
    ensure_attribute_value(indentation, "w:left", left_indent);
    ensure_attribute_value(indentation, "w:hanging", "360");
    return true;
}

auto ensure_managed_numbering_instance(pugi::xml_node numbering_root, featherdoc::list_kind kind)
    -> std::optional<std::uint32_t> {
    const auto managed_name = list_name_for(kind);

    auto abstract_num = find_managed_abstract_numbering(numbering_root, managed_name);
    if (abstract_num == pugi::xml_node{}) {
        const auto abstract_num_id =
            max_numbering_id(numbering_root, "w:abstractNum", "w:abstractNumId") + 1U;
        abstract_num = numbering_root.append_child("w:abstractNum");
        if (abstract_num == pugi::xml_node{}) {
            return std::nullopt;
        }

        ensure_attribute_value(abstract_num, "w:abstractNumId",
                               std::to_string(abstract_num_id));

        auto multi_level_type = abstract_num.append_child("w:multiLevelType");
        auto name_node = abstract_num.append_child("w:name");
        if (multi_level_type == pugi::xml_node{} || name_node == pugi::xml_node{}) {
            return std::nullopt;
        }

        ensure_attribute_value(multi_level_type, "w:val", "multilevel");
        ensure_attribute_value(name_node, "w:val", managed_name);

        for (std::uint32_t level = 0U; level <= max_list_level; ++level) {
            if (!append_level_definition(abstract_num, kind, level)) {
                return std::nullopt;
            }
        }
    }

    const auto abstract_num_id =
        parse_u32_attribute_value(abstract_num.attribute("w:abstractNumId").value());
    if (!abstract_num_id.has_value()) {
        return std::nullopt;
    }

    auto numbering_instance =
        find_numbering_instance_for_abstract(numbering_root, *abstract_num_id);
    if (numbering_instance == pugi::xml_node{}) {
        const auto num_id = max_numbering_id(numbering_root, "w:num", "w:numId") + 1U;
        numbering_instance = numbering_root.append_child("w:num");
        if (numbering_instance == pugi::xml_node{}) {
            return std::nullopt;
        }

        ensure_attribute_value(numbering_instance, "w:numId", std::to_string(num_id));

        auto abstract_num_id_node = numbering_instance.append_child("w:abstractNumId");
        if (abstract_num_id_node == pugi::xml_node{}) {
            return std::nullopt;
        }
        ensure_attribute_value(abstract_num_id_node, "w:val", std::to_string(*abstract_num_id));
        return num_id;
    }

    return parse_u32_attribute_value(numbering_instance.attribute("w:numId").value());
}
} // namespace

namespace featherdoc {

std::error_code Document::ensure_numbering_loaded() {
    if (this->numbering_loaded) {
        return {};
    }

    this->numbering.reset();
    this->has_numbering_part = false;

    if (!this->has_source_archive) {
        const auto parse_result =
            this->numbering.load_buffer(empty_numbering_xml.data(), empty_numbering_xml.size());
        if (!parse_result) {
            return set_last_error(
                this->last_error_info, document_errc::numbering_xml_parse_failed,
                parse_result.description(), std::string{numbering_xml_entry},
                parse_result.offset >= 0
                    ? std::optional<std::ptrdiff_t>{parse_result.offset}
                    : std::nullopt);
        }

        this->numbering_loaded = true;
        return {};
    }

    const auto relationships = this->document_relationships.child("Relationships");
    if (relationships == pugi::xml_node{}) {
        return set_last_error(this->last_error_info,
                              document_errc::relationships_xml_parse_failed,
                              "word/_rels/document.xml.rels does not contain a Relationships root",
                              std::string{document_relationships_xml_entry});
    }

    const auto numbering_relationship =
        find_document_relationship_by_type(relationships, numbering_relationship_type);
    if (numbering_relationship == pugi::xml_node{}) {
        const auto parse_result =
            this->numbering.load_buffer(empty_numbering_xml.data(), empty_numbering_xml.size());
        if (!parse_result) {
            return set_last_error(
                this->last_error_info, document_errc::numbering_xml_parse_failed,
                parse_result.description(), std::string{numbering_xml_entry},
                parse_result.offset >= 0
                    ? std::optional<std::ptrdiff_t>{parse_result.offset}
                    : std::nullopt);
        }

        this->numbering_loaded = true;
        return {};
    }

    const auto target = std::string_view{numbering_relationship.attribute("Target").value()};
    if (target.empty()) {
        return set_last_error(this->last_error_info, document_errc::numbering_xml_read_failed,
                              "numbering relationship does not contain a Target attribute",
                              std::string{document_relationships_xml_entry});
    }

    int source_zip_error = 0;
    zip_t *source_zip = zip_openwitherror(this->document_path.string().c_str(),
                                          ZIP_DEFAULT_COMPRESSION_LEVEL, 'r',
                                          &source_zip_error);
    if (!source_zip) {
        return set_last_error(this->last_error_info,
                              document_errc::source_archive_open_failed,
                              "failed to reopen source archive '" +
                                  this->document_path.string() +
                                  "' while loading word/numbering.xml: " +
                                  zip_error_text(source_zip_error));
    }

    const auto numbering_entry_name = normalize_word_part_entry(target);
    std::string numbering_text;
    const auto read_status =
        read_zip_entry_text(source_zip, numbering_entry_name, numbering_text);
    zip_close(source_zip);

    if (read_status != zip_entry_read_status::ok) {
        return set_last_error(this->last_error_info,
                              document_errc::numbering_xml_read_failed,
                              "failed to read required numbering part '" +
                                  numbering_entry_name + "'",
                              numbering_entry_name);
    }

    const auto parse_result =
        this->numbering.load_buffer(numbering_text.data(), numbering_text.size());
    if (!parse_result) {
        return set_last_error(
            this->last_error_info, document_errc::numbering_xml_parse_failed,
            parse_result.description(), numbering_entry_name,
            parse_result.offset >= 0 ? std::optional<std::ptrdiff_t>{parse_result.offset}
                                     : std::nullopt);
    }

    this->numbering_loaded = true;
    this->has_numbering_part = true;
    return {};
}

std::error_code Document::ensure_numbering_part_attached() {
    if (const auto error = this->ensure_numbering_loaded()) {
        return error;
    }

    if (const auto error = this->ensure_content_types_loaded()) {
        return error;
    }

    if (this->document_relationships.child("Relationships") == pugi::xml_node{} &&
        !initialize_empty_relationships_document(this->document_relationships)) {
        return set_last_error(this->last_error_info,
                              document_errc::relationships_xml_parse_failed,
                              "failed to initialize default relationships XML",
                              std::string{document_relationships_xml_entry});
    }

    auto relationships = this->document_relationships.child("Relationships");
    if (relationships == pugi::xml_node{}) {
        return set_last_error(this->last_error_info,
                              document_errc::relationships_xml_parse_failed,
                              "word/_rels/document.xml.rels does not contain a Relationships root",
                              std::string{document_relationships_xml_entry});
    }

    auto types = this->content_types.child("Types");
    if (types == pugi::xml_node{}) {
        return set_last_error(this->last_error_info,
                              document_errc::content_types_xml_parse_failed,
                              "[Content_Types].xml does not contain a Types root",
                              std::string{content_types_xml_entry});
    }

    auto numbering_relationship =
        find_document_relationship_by_type(relationships, numbering_relationship_type);
    if (numbering_relationship == pugi::xml_node{}) {
        std::unordered_set<std::string> used_relationship_ids;
        for (auto relationship = relationships.child("Relationship");
             relationship != pugi::xml_node{};
             relationship = relationship.next_sibling("Relationship")) {
            const auto id = relationship.attribute("Id").value();
            if (*id != '\0') {
                used_relationship_ids.emplace(id);
            }
        }

        std::string relationship_id;
        for (std::size_t next_index = 1U; relationship_id.empty(); ++next_index) {
            auto candidate = "rId" + std::to_string(next_index);
            if (!used_relationship_ids.contains(candidate)) {
                relationship_id = std::move(candidate);
            }
        }

        numbering_relationship = relationships.append_child("Relationship");
        if (numbering_relationship == pugi::xml_node{}) {
            return set_last_error(this->last_error_info,
                                  std::make_error_code(std::errc::not_enough_memory),
                                  "failed to create a document relationship for numbering.xml",
                                  std::string{document_relationships_xml_entry});
        }

        ensure_attribute_value(numbering_relationship, "Id", relationship_id);
        ensure_attribute_value(numbering_relationship, "Type", numbering_relationship_type);
        ensure_attribute_value(numbering_relationship, "Target",
                               make_document_relationship_target(numbering_xml_entry));

        this->has_document_relationships_part = true;
        this->document_relationships_dirty = true;
    }

    const auto override_part_name = make_override_part_name(numbering_xml_entry);
    auto override_node = pugi::xml_node{};
    for (auto existing_override = types.child("Override");
         existing_override != pugi::xml_node{};
         existing_override = existing_override.next_sibling("Override")) {
        if (std::string_view{existing_override.attribute("PartName").value()} ==
            override_part_name) {
            override_node = existing_override;
            break;
        }
    }

    if (override_node == pugi::xml_node{}) {
        override_node = types.append_child("Override");
    }
    if (override_node == pugi::xml_node{}) {
        return set_last_error(this->last_error_info,
                              std::make_error_code(std::errc::not_enough_memory),
                              "failed to append a content type override for numbering.xml",
                              std::string{content_types_xml_entry});
    }

    ensure_attribute_value(override_node, "PartName", override_part_name);
    ensure_attribute_value(override_node, "ContentType", numbering_content_type);

    this->has_numbering_part = true;
    this->content_types_dirty = true;
    return {};
}

bool Document::set_paragraph_list(Paragraph target_paragraph, featherdoc::list_kind kind,
                                  std::uint32_t level) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before editing paragraph lists");
        return false;
    }

    if (!target_paragraph.has_next() || target_paragraph.current == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "target paragraph handle is not valid",
                       std::string{document_xml_entry});
        return false;
    }

    if (level > max_list_level) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "list level must be in the range [0, 8]",
                       std::string{document_xml_entry});
        return false;
    }

    if (const auto error = this->ensure_numbering_part_attached()) {
        return false;
    }

    auto numbering_root = this->numbering.child("w:numbering");
    if (numbering_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       document_errc::numbering_xml_parse_failed,
                       "word/numbering.xml does not contain the expected w:numbering root",
                       std::string{numbering_xml_entry});
        return false;
    }

    const auto num_id = ensure_managed_numbering_instance(numbering_root, kind);
    if (!num_id.has_value()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to create or reuse a managed numbering definition",
                       std::string{numbering_xml_entry});
        return false;
    }

    auto paragraph_properties = ensure_paragraph_properties_node(target_paragraph.current);
    if (paragraph_properties == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to create w:pPr for the target paragraph",
                       std::string{document_xml_entry});
        return false;
    }

    auto numbering_properties = paragraph_properties.child("w:numPr");
    if (numbering_properties == pugi::xml_node{}) {
        numbering_properties = paragraph_properties.append_child("w:numPr");
    }
    if (numbering_properties == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to create w:numPr for the target paragraph",
                       std::string{document_xml_entry});
        return false;
    }

    auto level_node = numbering_properties.child("w:ilvl");
    if (level_node == pugi::xml_node{}) {
        level_node = numbering_properties.append_child("w:ilvl");
    }
    auto num_id_node = numbering_properties.child("w:numId");
    if (num_id_node == pugi::xml_node{}) {
        num_id_node = numbering_properties.append_child("w:numId");
    }
    if (level_node == pugi::xml_node{} || num_id_node == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to populate numbering metadata for the target paragraph",
                       std::string{document_xml_entry});
        return false;
    }

    ensure_attribute_value(level_node, "w:val", std::to_string(level));
    ensure_attribute_value(num_id_node, "w:val", std::to_string(*num_id));

    this->numbering_dirty = true;
    this->last_error_info.clear();
    return true;
}

bool Document::clear_paragraph_list(Paragraph target_paragraph) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before editing paragraph lists");
        return false;
    }

    if (!target_paragraph.has_next() || target_paragraph.current == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "target paragraph handle is not valid",
                       std::string{document_xml_entry});
        return false;
    }

    if (auto paragraph_properties = target_paragraph.current.child("w:pPr");
        paragraph_properties != pugi::xml_node{}) {
        paragraph_properties.remove_child("w:numPr");
        remove_empty_paragraph_properties(target_paragraph.current);
    }

    this->last_error_info.clear();
    return true;
}

} // namespace featherdoc
