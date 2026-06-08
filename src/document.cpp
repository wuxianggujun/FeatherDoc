#include "featherdoc.hpp"
#include "xml_helpers.hpp"

#include <algorithm>
#include <array>
#include <charconv>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>

#include <zip.h>

namespace {
constexpr auto document_xml_entry = std::string_view{"word/document.xml"};
constexpr auto document_relationships_xml_entry =
    std::string_view{"word/_rels/document.xml.rels"};
constexpr auto relationships_xml_entry = std::string_view{"_rels/.rels"};
constexpr auto content_types_xml_entry = std::string_view{"[Content_Types].xml"};
constexpr auto settings_xml_entry = std::string_view{"word/settings.xml"};
constexpr auto numbering_xml_entry = std::string_view{"word/numbering.xml"};
constexpr auto styles_xml_entry = std::string_view{"word/styles.xml"};
constexpr auto footnotes_xml_entry = std::string_view{"word/footnotes.xml"};
constexpr auto endnotes_xml_entry = std::string_view{"word/endnotes.xml"};
constexpr auto comments_xml_entry = std::string_view{"word/comments.xml"};
constexpr auto comments_extended_xml_entry =
    std::string_view{"word/commentsExtended.xml"};
constexpr auto office_document_relationships_namespace_uri = std::string_view{
    "http://schemas.openxmlformats.org/officeDocument/2006/relationships"};
constexpr auto header_relationship_type = std::string_view{
    "http://schemas.openxmlformats.org/officeDocument/2006/relationships/header"};
constexpr auto footer_relationship_type = std::string_view{
    "http://schemas.openxmlformats.org/officeDocument/2006/relationships/footer"};
constexpr auto settings_relationship_type = std::string_view{
    "http://schemas.openxmlformats.org/officeDocument/2006/relationships/settings"};
constexpr auto footnotes_relationship_type = std::string_view{
    "http://schemas.openxmlformats.org/officeDocument/2006/relationships/footnotes"};
constexpr auto endnotes_relationship_type = std::string_view{
    "http://schemas.openxmlformats.org/officeDocument/2006/relationships/endnotes"};
constexpr auto comments_relationship_type = std::string_view{
    "http://schemas.openxmlformats.org/officeDocument/2006/relationships/comments"};
constexpr auto comments_extended_relationship_type = std::string_view{
    "http://schemas.microsoft.com/office/2011/relationships/commentsExtended"};
constexpr auto header_content_type = std::string_view{
    "application/vnd.openxmlformats-officedocument.wordprocessingml.header+xml"};
constexpr auto footer_content_type = std::string_view{
    "application/vnd.openxmlformats-officedocument.wordprocessingml.footer+xml"};
constexpr auto settings_content_type = std::string_view{
    "application/vnd.openxmlformats-officedocument.wordprocessingml.settings+xml"};
constexpr auto footnotes_content_type = std::string_view{
    "application/vnd.openxmlformats-officedocument.wordprocessingml.footnotes+xml"};
constexpr auto endnotes_content_type = std::string_view{
    "application/vnd.openxmlformats-officedocument.wordprocessingml.endnotes+xml"};
constexpr auto comments_content_type = std::string_view{
    "application/vnd.openxmlformats-officedocument.wordprocessingml.comments+xml"};
constexpr auto comments_extended_content_type = std::string_view{
    "application/vnd.openxmlformats-officedocument.wordprocessingml.commentsExtended+xml"};
constexpr auto empty_document_xml = std::string_view{
    R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p/>
  </w:body>
</w:document>
)"};
constexpr auto relationships_xml = std::string_view{
    R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rId1"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument"
                Target="word/document.xml"/>
</Relationships>
)"};
constexpr auto content_types_xml = std::string_view{
    R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">
  <Default Extension="rels"
           ContentType="application/vnd.openxmlformats-package.relationships+xml"/>
  <Default Extension="xml" ContentType="application/xml"/>
  <Override PartName="/word/document.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml"/>
</Types>
)"};
constexpr auto empty_relationships_xml = std::string_view{
    R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
</Relationships>
)"}; 
constexpr auto empty_header_xml = std::string_view{
    R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:hdr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p/>
</w:hdr>
)"}; 
constexpr auto empty_footer_xml = std::string_view{
    R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:ftr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p/>
</w:ftr>
)"}; 
constexpr auto empty_settings_xml = std::string_view{
    R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:settings xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
</w:settings>
)"}; 
constexpr int docx_output_compression_level = 0;

struct packaged_entry final {
    std::string_view name;
    std::string_view content;
};

constexpr auto minimal_docx_entries = std::array{
    packaged_entry{relationships_xml_entry, relationships_xml},
};

struct related_part_entry final {
    std::string relationship_id;
    std::string relationship_type;
    std::string entry_name;
};

struct xml_zip_writer final : pugi::xml_writer {
    zip_t *archive{nullptr};
    bool failed{false};

    explicit xml_zip_writer(zip_t *archive_handle) : archive(archive_handle) {}

    void write(const void *data, size_t size) override {
        if (this->failed || size == 0) {
            return;
        }

        if (zip_entry_write(this->archive, data, size) < 0) {
            this->failed = true;
        }
    }
};

struct zip_entry_copy_context {
    zip_t *target_archive{nullptr};
    bool failed{false};
};

auto copy_zip_entry_chunk(void *arg, std::uint64_t /*offset*/, const void *data,
                          size_t size) -> size_t {
    auto *context = static_cast<zip_entry_copy_context *>(arg);
    if (context == nullptr || context->failed || size == 0) {
        return 0;
    }

    if (zip_entry_write(context->target_archive, data, size) < 0) {
        context->failed = true;
        return 0;
    }

    return size;
}

auto zip_error_text(int error_number) -> std::string {
    if (const char *message = zip_strerror(error_number); message != nullptr) {
        return message;
    }

    return "unknown zip error";
}

auto split_plain_text_paragraphs(std::string_view text) -> std::vector<std::string> {
    std::vector<std::string> paragraphs;
    std::size_t begin = 0U;
    while (begin <= text.size()) {
        const auto line_end = text.find('\n', begin);
        const auto end = line_end == std::string_view::npos ? text.size() : line_end;

        std::string paragraph_text(text.substr(begin, end - begin));
        if (!paragraph_text.empty() && paragraph_text.back() == '\r') {
            paragraph_text.pop_back();
        }
        paragraphs.push_back(std::move(paragraph_text));

        if (line_end == std::string_view::npos) {
            break;
        }

        begin = end + 1U;
    }

    if (paragraphs.empty()) {
        paragraphs.emplace_back();
    }

    if (!text.empty() && (text.ends_with('\n') || text.ends_with('\r'))) {
        while (paragraphs.size() > 1U && paragraphs.back().empty()) {
            paragraphs.pop_back();
        }
    }

    return paragraphs;
}

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

enum class xml_uint_attribute_status {
    ok,
    missing,
    invalid,
};

auto parse_xml_uint32_attribute(pugi::xml_node node, const char *attribute_name,
                                std::uint32_t &value) -> xml_uint_attribute_status {
    const auto attribute = node.attribute(attribute_name);
    if (attribute == pugi::xml_attribute{}) {
        return xml_uint_attribute_status::missing;
    }

    const auto text = std::string_view{attribute.value()};
    if (text.empty()) {
        return xml_uint_attribute_status::invalid;
    }

    const auto *begin = text.data();
    const auto *end = begin + text.size();
    const auto result = std::from_chars(begin, end, value);
    if (result.ec != std::errc{} || result.ptr != end) {
        return xml_uint_attribute_status::invalid;
    }

    return xml_uint_attribute_status::ok;
}

enum class zip_entry_read_status {
    ok,
    missing,
    read_failed,
};

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

auto make_part_relationships_entry(std::string_view entry_name) -> std::string {
    if (entry_name.empty()) {
        return {};
    }

    const auto normalized_entry =
        std::filesystem::path{std::string{entry_name}}.lexically_normal();
    const auto filename = normalized_entry.filename().generic_string();
    if (filename.empty()) {
        return {};
    }

    return (normalized_entry.parent_path() / "_rels" /
            std::filesystem::path{filename + ".rels"})
        .lexically_normal()
        .generic_string();
}

auto find_section_reference(pugi::xml_node section_properties, const char *reference_name,
                            std::string_view xml_reference_type) -> pugi::xml_node {
    for (auto reference = section_properties.child(reference_name);
         reference != pugi::xml_node{};
         reference = reference.next_sibling(reference_name)) {
        if (std::string_view{reference.attribute("w:type").value()} == xml_reference_type) {
            return reference;
        }
    }

    return {};
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

auto append_section_reference(pugi::xml_node section_properties, const char *reference_name)
    -> pugi::xml_node {
    const auto reference_name_view = std::string_view{reference_name};
    auto insertion_anchor = pugi::xml_node{};

    for (auto child = section_properties.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        const auto child_name = std::string_view{child.name()};

        if (reference_name_view == "w:headerReference") {
            if (child_name == "w:footerReference") {
                insertion_anchor = child;
                break;
            }

            if (child_name != "w:headerReference") {
                insertion_anchor = child;
                break;
            }

            continue;
        }

        if (child_name != "w:headerReference" && child_name != "w:footerReference") {
            insertion_anchor = child;
            break;
        }
    }

    return insertion_anchor == pugi::xml_node{}
               ? section_properties.append_child(reference_name)
               : section_properties.insert_child_before(reference_name, insertion_anchor);
}

void clear_section_header_footer_references(pugi::xml_node section_properties) {
    if (section_properties == pugi::xml_node{}) {
        return;
    }

    for (auto reference = section_properties.child("w:headerReference");
         reference != pugi::xml_node{};) {
        const auto next = reference.next_sibling("w:headerReference");
        section_properties.remove_child(reference);
        reference = next;
    }

    for (auto reference = section_properties.child("w:footerReference");
         reference != pugi::xml_node{};) {
        const auto next = reference.next_sibling("w:footerReference");
        section_properties.remove_child(reference);
        reference = next;
    }

    section_properties.remove_child("w:titlePg");
}

struct section_body_snapshot final {
    pugi::xml_document xml;
    pugi::xml_node root;
    pugi::xml_node content_root;
    pugi::xml_node properties_root;

    section_body_snapshot() {
        this->root = this->xml.append_child("section");
        this->content_root = this->root.append_child("content");
        this->properties_root = this->root.append_child("properties");
    }

    [[nodiscard]] auto section_properties() const -> pugi::xml_node {
        return this->properties_root.child("w:sectPr");
    }
};

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

void remove_empty_paragraph(pugi::xml_node paragraph) {
    if (paragraph == pugi::xml_node{}) {
        return;
    }

    remove_empty_paragraph_properties(paragraph);
    if (paragraph.first_child() != pugi::xml_node{} || node_has_attributes(paragraph)) {
        return;
    }

    if (auto parent = paragraph.parent(); parent != pugi::xml_node{}) {
        parent.remove_child(paragraph);
    }
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

auto ensure_on_off_node_enabled(pugi::xml_node parent, const char *child_name)
    -> pugi::xml_node {
    auto child = parent.child(child_name);
    if (child == pugi::xml_node{}) {
        child = parent.append_child(child_name);
    }
    if (child == pugi::xml_node{}) {
        return {};
    }

    auto value_attribute = child.attribute("w:val");
    if (value_attribute == pugi::xml_attribute{}) {
        value_attribute = child.append_attribute("w:val");
    }
    value_attribute.set_value("1");
    return child;
}

auto ensure_section_title_page_node(pugi::xml_node section_properties) -> pugi::xml_node {
    auto title_page = section_properties.child("w:titlePg");
    if (title_page == pugi::xml_node{}) {
        auto insertion_anchor = pugi::xml_node{};
        for (auto child = section_properties.first_child(); child != pugi::xml_node{};
             child = child.next_sibling()) {
            const auto child_name = std::string_view{child.name()};
            if (child_name == "w:textDirection" || child_name == "w:bidi" ||
                child_name == "w:rtlGutter" || child_name == "w:docGrid" ||
                child_name == "w:printerSettings" || child_name == "w:sectPrChange") {
                insertion_anchor = child;
                break;
            }
        }

        title_page = insertion_anchor == pugi::xml_node{}
                         ? section_properties.append_child("w:titlePg")
                         : section_properties.insert_child_before("w:titlePg",
                                                                  insertion_anchor);
    }

    if (title_page == pugi::xml_node{}) {
        return {};
    }

    auto value_attribute = title_page.attribute("w:val");
    if (value_attribute == pugi::xml_attribute{}) {
        value_attribute = title_page.append_attribute("w:val");
    }
    value_attribute.set_value("1");
    return title_page;
}

auto append_section_property_node(pugi::xml_node section_properties, const char *child_name)
    -> pugi::xml_node {
    const auto child_name_view = std::string_view{child_name};
    auto insertion_anchor = pugi::xml_node{};

    auto should_insert_before = [child_name_view](std::string_view existing_name) {
        if (child_name_view == "w:pgSz") {
            return existing_name == "w:pgMar" || existing_name == "w:paperSrc" ||
                   existing_name == "w:pgBorders" ||
                   existing_name == "w:lnNumType" ||
                   existing_name == "w:pgNumType" || existing_name == "w:cols" ||
                   existing_name == "w:formProt" || existing_name == "w:vAlign" ||
                   existing_name == "w:noEndnote" || existing_name == "w:titlePg" ||
                   existing_name == "w:textDirection" || existing_name == "w:bidi" ||
                   existing_name == "w:rtlGutter" ||
                   existing_name == "w:docGrid" ||
                   existing_name == "w:printerSettings" ||
                   existing_name == "w:sectPrChange";
        }

        if (child_name_view == "w:pgMar") {
            return existing_name == "w:paperSrc" ||
                   existing_name == "w:pgBorders" ||
                   existing_name == "w:lnNumType" ||
                   existing_name == "w:pgNumType" || existing_name == "w:cols" ||
                   existing_name == "w:formProt" || existing_name == "w:vAlign" ||
                   existing_name == "w:noEndnote" || existing_name == "w:titlePg" ||
                   existing_name == "w:textDirection" || existing_name == "w:bidi" ||
                   existing_name == "w:rtlGutter" ||
                   existing_name == "w:docGrid" ||
                   existing_name == "w:printerSettings" ||
                   existing_name == "w:sectPrChange";
        }

        if (child_name_view == "w:pgNumType") {
            return existing_name == "w:cols" || existing_name == "w:formProt" ||
                   existing_name == "w:vAlign" || existing_name == "w:noEndnote" ||
                   existing_name == "w:titlePg" ||
                   existing_name == "w:textDirection" || existing_name == "w:bidi" ||
                   existing_name == "w:rtlGutter" ||
                   existing_name == "w:docGrid" ||
                   existing_name == "w:printerSettings" ||
                   existing_name == "w:sectPrChange";
        }

        return false;
    };

    for (auto child = section_properties.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        if (should_insert_before(std::string_view{child.name()})) {
            insertion_anchor = child;
            break;
        }
    }

    return insertion_anchor == pugi::xml_node{}
               ? section_properties.append_child(child_name)
               : section_properties.insert_child_before(child_name, insertion_anchor);
}

auto ensure_section_property_node(pugi::xml_node section_properties, const char *child_name)
    -> pugi::xml_node {
    auto child = section_properties.child(child_name);
    if (child != pugi::xml_node{}) {
        return child;
    }

    return append_section_property_node(section_properties, child_name);
}

auto ensure_xml_uint32_attribute(pugi::xml_node node, const char *attribute_name,
                                 std::uint32_t value) -> bool {
    if (node == pugi::xml_node{}) {
        return false;
    }

    auto attribute = node.attribute(attribute_name);
    if (attribute == pugi::xml_attribute{}) {
        attribute = node.append_attribute(attribute_name);
    }
    if (attribute == pugi::xml_attribute{}) {
        return false;
    }

    const auto text = std::to_string(value);
    return attribute.set_value(text.c_str());
}

void remove_empty_node(pugi::xml_node node) {
    if (node == pugi::xml_node{}) {
        return;
    }

    if (node.first_child() != pugi::xml_node{} || node_has_attributes(node)) {
        return;
    }

    if (auto parent = node.parent(); parent != pugi::xml_node{}) {
        parent.remove_child(node);
    }
}

auto section_break_paragraph_for(pugi::xml_node section_properties) -> pugi::xml_node {
    if (section_properties == pugi::xml_node{}) {
        return {};
    }

    const auto paragraph_properties = section_properties.parent();
    if (paragraph_properties == pugi::xml_node{} ||
        std::string_view{paragraph_properties.name()} != "w:pPr") {
        return {};
    }

    const auto paragraph = paragraph_properties.parent();
    if (paragraph == pugi::xml_node{} || std::string_view{paragraph.name()} != "w:p") {
        return {};
    }

    return paragraph;
}

auto replace_section_properties_contents(pugi::xml_node target_section_properties,
                                         pugi::xml_node source_section_properties) -> bool {
    if (target_section_properties == pugi::xml_node{} ||
        source_section_properties == pugi::xml_node{}) {
        return false;
    }

    for (auto attribute = target_section_properties.first_attribute();
         attribute != pugi::xml_attribute{};) {
        const auto next = attribute.next_attribute();
        target_section_properties.remove_attribute(attribute);
        attribute = next;
    }

    for (auto child = target_section_properties.first_child(); child != pugi::xml_node{};) {
        const auto next = child.next_sibling();
        target_section_properties.remove_child(child);
        child = next;
    }

    for (auto attribute = source_section_properties.first_attribute();
         attribute != pugi::xml_attribute{}; attribute = attribute.next_attribute()) {
        auto copied_attribute = target_section_properties.append_attribute(attribute.name());
        if (copied_attribute == pugi::xml_attribute{}) {
            return false;
        }
        copied_attribute.set_value(attribute.value());
    }

    for (auto child = source_section_properties.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        if (target_section_properties.append_copy(child) == pugi::xml_node{}) {
            return false;
        }
    }

    return true;
}

auto ensure_paragraph_properties_node(pugi::xml_node paragraph) -> pugi::xml_node {
    if (paragraph == pugi::xml_node{}) {
        return {};
    }

    auto paragraph_properties = paragraph.child("w:pPr");
    if (paragraph_properties != pugi::xml_node{}) {
        return paragraph_properties;
    }

    const auto first_child = paragraph.first_child();
    return first_child == pugi::xml_node{}
               ? paragraph.append_child("w:pPr")
               : paragraph.insert_child_before("w:pPr", first_child);
}

auto capture_section_snapshot(pugi::xml_node first_child, pugi::xml_node end_exclusive,
                              pugi::xml_node section_properties,
                              bool strip_last_paragraph_section_properties)
    -> std::unique_ptr<section_body_snapshot> {
    auto snapshot = std::make_unique<section_body_snapshot>();
    if (snapshot->root == pugi::xml_node{} || snapshot->content_root == pugi::xml_node{} ||
        snapshot->properties_root == pugi::xml_node{}) {
        return nullptr;
    }

    for (auto child = first_child; child != pugi::xml_node{} && child != end_exclusive;
         child = child.next_sibling()) {
        if (snapshot->content_root.append_copy(child) == pugi::xml_node{}) {
            return nullptr;
        }
    }

    if (section_properties != pugi::xml_node{}) {
        if (snapshot->properties_root.append_copy(section_properties) == pugi::xml_node{}) {
            return nullptr;
        }
    } else if (snapshot->properties_root.append_child("w:sectPr") == pugi::xml_node{}) {
        return nullptr;
    }

    if (strip_last_paragraph_section_properties) {
        auto last_copied_child = snapshot->content_root.last_child();
        if (last_copied_child != pugi::xml_node{} &&
            std::string_view{last_copied_child.name()} == "w:p") {
            if (auto paragraph_properties = last_copied_child.child("w:pPr");
                paragraph_properties != pugi::xml_node{}) {
                paragraph_properties.remove_child("w:sectPr");
                remove_empty_paragraph_properties(last_copied_child);
            }
        }
    }

    return snapshot;
}

auto collect_section_snapshots(pugi::xml_node body)
    -> std::optional<std::vector<std::unique_ptr<section_body_snapshot>>> {
    if (body == pugi::xml_node{}) {
        return std::nullopt;
    }

    std::vector<std::unique_ptr<section_body_snapshot>> snapshots;
    auto body_section_properties = body.child("w:sectPr");
    auto current_section_first_child = body.first_child();

    for (auto child = body.first_child(); child != pugi::xml_node{} && child != body_section_properties;
         child = child.next_sibling()) {
        if (std::string_view{child.name()} != "w:p") {
            continue;
        }

        auto paragraph_section_properties = child.child("w:pPr").child("w:sectPr");
        if (paragraph_section_properties == pugi::xml_node{}) {
            continue;
        }

        auto snapshot = capture_section_snapshot(current_section_first_child,
                                                child.next_sibling(),
                                                paragraph_section_properties, true);
        if (snapshot == nullptr) {
            return std::nullopt;
        }

        snapshots.push_back(std::move(snapshot));
        current_section_first_child = child.next_sibling();
    }

    auto final_snapshot = capture_section_snapshot(current_section_first_child,
                                                   body_section_properties,
                                                   body_section_properties, false);
    if (final_snapshot == nullptr) {
        return std::nullopt;
    }

    snapshots.push_back(std::move(final_snapshot));
    return snapshots;
}

auto rebuild_body_from_section_snapshots(
    pugi::xml_node body, const std::vector<std::unique_ptr<section_body_snapshot>> &snapshots)
    -> bool {
    if (body == pugi::xml_node{}) {
        return false;
    }

    for (auto child = body.first_child(); child != pugi::xml_node{};) {
        const auto next = child.next_sibling();
        body.remove_child(child);
        child = next;
    }

    for (std::size_t index = 0; index < snapshots.size(); ++index) {
        const auto &snapshot = snapshots[index];
        auto last_content_node = pugi::xml_node{};
        for (auto child = snapshot->content_root.first_child(); child != pugi::xml_node{};
             child = child.next_sibling()) {
            last_content_node = body.append_copy(child);
            if (last_content_node == pugi::xml_node{}) {
                return false;
            }
        }

        const auto section_properties = snapshot->section_properties();
        if (index + 1U < snapshots.size()) {
            auto host_paragraph = pugi::xml_node{};
            if (last_content_node != pugi::xml_node{} &&
                std::string_view{last_content_node.name()} == "w:p") {
                host_paragraph = last_content_node;
            } else {
                host_paragraph = featherdoc::detail::append_paragraph_node(body);
            }

            if (host_paragraph == pugi::xml_node{}) {
                return false;
            }

            auto paragraph_properties = ensure_paragraph_properties_node(host_paragraph);
            if (paragraph_properties == pugi::xml_node{}) {
                return false;
            }

            if (paragraph_properties.append_copy(section_properties) == pugi::xml_node{}) {
                return false;
            }
        } else if (body.append_copy(section_properties) == pugi::xml_node{}) {
            return false;
        }
    }

    return true;
}

auto section_has_reference_type(pugi::xml_node section_properties, const char *reference_name,
                                std::string_view xml_reference_type) -> bool {
    return find_section_reference(section_properties, reference_name, xml_reference_type) !=
           pugi::xml_node{};
}

auto document_has_reference_type(const pugi::xml_document &document, const char *reference_name,
                                 std::string_view xml_reference_type) -> bool {
    const auto body = document.child("w:document").child("w:body");
    if (body == pugi::xml_node{}) {
        return false;
    }

    const auto section_has_reference = [&](pugi::xml_node section_properties) {
        return section_has_reference_type(section_properties, reference_name, xml_reference_type);
    };

    for (auto child = body.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        if (std::string_view{child.name()} != "w:p") {
            continue;
        }

        if (const auto section_properties = child.child("w:pPr").child("w:sectPr");
            section_properties != pugi::xml_node{} && section_has_reference(section_properties)) {
            return true;
        }
    }

    if (const auto section_properties = body.child("w:sectPr");
        section_properties != pugi::xml_node{} && section_has_reference(section_properties)) {
        return true;
    }

    return false;
}

auto load_document_relationships_part(
    zip_t *archive, pugi::xml_document &relationships_xml_document,
    bool &has_relationships_part,
    featherdoc::document_error_info &last_error_info)
    -> std::optional<std::vector<related_part_entry>> {
    has_relationships_part = false;
    relationships_xml_document.reset();

    std::string relationships_xml_text;
    const auto relationships_status =
        read_zip_entry_text(archive, document_relationships_xml_entry, relationships_xml_text);
    if (relationships_status == zip_entry_read_status::missing) {
        return std::vector<related_part_entry>{};
    }

    if (relationships_status == zip_entry_read_status::read_failed) {
        set_last_error(last_error_info, featherdoc::document_errc::relationships_xml_read_failed,
                       "failed to read relationships entry 'word/_rels/document.xml.rels'",
                       std::string{document_relationships_xml_entry});
        return std::nullopt;
    }

    const auto parse_result = relationships_xml_document.load_buffer(
        relationships_xml_text.data(), relationships_xml_text.size());
    if (!parse_result) {
        set_last_error(
            last_error_info, featherdoc::document_errc::relationships_xml_parse_failed,
            parse_result.description(), std::string{document_relationships_xml_entry},
            parse_result.offset >= 0 ? std::optional<std::ptrdiff_t>{parse_result.offset}
                                     : std::nullopt);
        return std::nullopt;
    }

    has_relationships_part = true;
    std::unordered_set<std::string> seen_entries;
    std::vector<related_part_entry> related_parts;
    const auto relationships = relationships_xml_document.child("Relationships");
    for (auto relationship = relationships.child("Relationship");
         relationship != pugi::xml_node{}; relationship = relationship.next_sibling("Relationship")) {
        const auto type = std::string_view{relationship.attribute("Type").value()};
        if (type != header_relationship_type && type != footer_relationship_type) {
            continue;
        }

        const auto target = std::string_view{relationship.attribute("Target").value()};
        if (target.empty()) {
            continue;
        }

        std::string entry_name = normalize_word_part_entry(target);
        if (!entry_name.empty() && seen_entries.insert(entry_name).second) {
            related_parts.push_back(related_part_entry{
                relationship.attribute("Id").value(), std::string{type},
                std::move(entry_name)});
        }
    }

    return related_parts;
}

auto load_related_xml_part(zip_t *archive, std::string_view entry_name,
                           pugi::xml_document &xml_document,
                           featherdoc::document_error_info &last_error_info)
    -> std::error_code {
    std::string xml_text;
    const auto read_status = read_zip_entry_text(archive, entry_name, xml_text);
    if (read_status == zip_entry_read_status::missing) {
        return set_last_error(last_error_info, featherdoc::document_errc::related_part_open_failed,
                              "failed to open related document part '" +
                                  std::string{entry_name} + "'",
                              std::string{entry_name});
    }

    if (read_status == zip_entry_read_status::read_failed) {
        return set_last_error(last_error_info, featherdoc::document_errc::related_part_read_failed,
                              "failed to read related document part '" +
                                  std::string{entry_name} + "'",
                              std::string{entry_name});
    }

    const auto parse_result = xml_document.load_buffer(xml_text.data(), xml_text.size());
    if (!parse_result) {
        return set_last_error(
            last_error_info, featherdoc::document_errc::related_part_parse_failed,
            parse_result.description(), std::string{entry_name},
            parse_result.offset >= 0 ? std::optional<std::ptrdiff_t>{parse_result.offset}
                                     : std::nullopt);
    }

    return {};
}

auto load_optional_relationships_part(
    zip_t *archive, std::string_view entry_name, pugi::xml_document &xml_document,
    bool &has_relationships_part,
    featherdoc::document_error_info &last_error_info) -> std::error_code {
    has_relationships_part = false;
    xml_document.reset();

    if (entry_name.empty()) {
        return {};
    }

    std::string xml_text;
    const auto read_status = read_zip_entry_text(archive, entry_name, xml_text);
    if (read_status == zip_entry_read_status::missing) {
        return {};
    }

    if (read_status == zip_entry_read_status::read_failed) {
        return set_last_error(last_error_info,
                              featherdoc::document_errc::relationships_xml_read_failed,
                              "failed to read relationships entry '" +
                                  std::string{entry_name} + "'",
                              std::string{entry_name});
    }

    const auto parse_result = xml_document.load_buffer(xml_text.data(), xml_text.size());
    if (!parse_result) {
        return set_last_error(
            last_error_info, featherdoc::document_errc::relationships_xml_parse_failed,
            parse_result.description(), std::string{entry_name},
            parse_result.offset >= 0 ? std::optional<std::ptrdiff_t>{parse_result.offset}
                                     : std::nullopt);
    }

    has_relationships_part = true;
    return {};
}

} // namespace

namespace featherdoc {

std::optional<bool> Document::inspect_update_fields_on_open_enabled() {
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
        read_on_off_value(settings_root.child("w:updateFields")).value_or(false);
    this->last_error_info = previous_error;
    return enabled;
}

std::optional<bool> Document::update_fields_on_open_enabled() {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before reading updateFields",
                       std::string{settings_xml_entry});
        return std::nullopt;
    }

    if (this->ensure_settings_loaded()) {
        return std::nullopt;
    }

    const auto settings_root = this->settings.child("w:settings");
    if (settings_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       document_errc::settings_xml_parse_failed,
                       "word/settings.xml does not contain the expected w:settings root",
                       std::string{settings_xml_entry});
        return std::nullopt;
    }

    this->last_error_info.clear();
    return read_on_off_value(settings_root.child("w:updateFields")).value_or(false);
}

std::error_code Document::ensure_content_types_loaded() {
    if (this->content_types_loaded) {
        return {};
    }

    this->content_types.reset();
    if (!this->has_source_archive) {
        const auto parse_result =
            this->content_types.load_buffer(content_types_xml.data(), content_types_xml.size());
        if (!parse_result) {
            return set_last_error(
                this->last_error_info, document_errc::content_types_xml_parse_failed,
                parse_result.description(), std::string{content_types_xml_entry},
                parse_result.offset >= 0
                    ? std::optional<std::ptrdiff_t>{parse_result.offset}
                    : std::nullopt);
        }

        this->content_types_loaded = true;
        return {};
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
                                  "' while loading [Content_Types].xml: " +
                                  zip_error_text(source_zip_error));
    }

    std::string content_types_text;
    const auto read_status =
        read_zip_entry_text(source_zip, content_types_xml_entry, content_types_text);
    zip_close(source_zip);

    if (read_status != zip_entry_read_status::ok) {
        return set_last_error(this->last_error_info,
                              document_errc::content_types_xml_read_failed,
                              "failed to read required entry '[Content_Types].xml'",
                              std::string{content_types_xml_entry});
    }

    const auto parse_result =
        this->content_types.load_buffer(content_types_text.data(), content_types_text.size());
    if (!parse_result) {
        return set_last_error(
            this->last_error_info, document_errc::content_types_xml_parse_failed,
            parse_result.description(), std::string{content_types_xml_entry},
            parse_result.offset >= 0 ? std::optional<std::ptrdiff_t>{parse_result.offset}
                                     : std::nullopt);
    }

    this->content_types_loaded = true;
    return {};
}

std::error_code Document::ensure_settings_loaded() {
    if (this->settings_loaded) {
        return {};
    }

    this->settings.reset();
    this->has_settings_part = false;

    if (!this->has_source_archive) {
        const auto parse_result =
            this->settings.load_buffer(empty_settings_xml.data(), empty_settings_xml.size());
        if (!parse_result) {
            return set_last_error(
                this->last_error_info, document_errc::settings_xml_parse_failed,
                parse_result.description(), std::string{settings_xml_entry},
                parse_result.offset >= 0
                    ? std::optional<std::ptrdiff_t>{parse_result.offset}
                    : std::nullopt);
        }

        this->settings_loaded = true;
        return {};
    }

    const auto relationships = this->document_relationships.child("Relationships");
    if (relationships == pugi::xml_node{}) {
        return set_last_error(this->last_error_info,
                              document_errc::relationships_xml_parse_failed,
                              "word/_rels/document.xml.rels does not contain a Relationships root",
                              std::string{document_relationships_xml_entry});
    }

    const auto settings_relationship =
        find_document_relationship_by_type(relationships, settings_relationship_type);
    if (settings_relationship == pugi::xml_node{}) {
        const auto parse_result =
            this->settings.load_buffer(empty_settings_xml.data(), empty_settings_xml.size());
        if (!parse_result) {
            return set_last_error(
                this->last_error_info, document_errc::settings_xml_parse_failed,
                parse_result.description(), std::string{settings_xml_entry},
                parse_result.offset >= 0
                    ? std::optional<std::ptrdiff_t>{parse_result.offset}
                    : std::nullopt);
        }

        this->settings_loaded = true;
        return {};
    }

    const auto target = std::string_view{settings_relationship.attribute("Target").value()};
    if (target.empty()) {
        return set_last_error(this->last_error_info, document_errc::settings_xml_read_failed,
                              "settings relationship does not contain a Target attribute",
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
                                  "' while loading word/settings.xml: " +
                                  zip_error_text(source_zip_error));
    }

    const auto settings_entry_name = normalize_word_part_entry(target);
    std::string settings_text;
    const auto read_status =
        read_zip_entry_text(source_zip, settings_entry_name, settings_text);
    zip_close(source_zip);

    if (read_status != zip_entry_read_status::ok) {
        return set_last_error(this->last_error_info,
                              document_errc::settings_xml_read_failed,
                              "failed to read required settings part '" +
                                  settings_entry_name + "'",
                              settings_entry_name);
    }

    const auto parse_result =
        this->settings.load_buffer(settings_text.data(), settings_text.size());
    if (!parse_result) {
        return set_last_error(
            this->last_error_info, document_errc::settings_xml_parse_failed,
            parse_result.description(), settings_entry_name,
            parse_result.offset >= 0 ? std::optional<std::ptrdiff_t>{parse_result.offset}
                                     : std::nullopt);
    }

    this->settings_loaded = true;
    this->has_settings_part = true;
    return {};
}

std::error_code Document::ensure_settings_part_attached() {
    if (const auto error = this->ensure_settings_loaded()) {
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

    auto settings_relationship =
        find_document_relationship_by_type(relationships, settings_relationship_type);
    if (settings_relationship == pugi::xml_node{}) {
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

        settings_relationship = relationships.append_child("Relationship");
        if (settings_relationship == pugi::xml_node{}) {
            return set_last_error(this->last_error_info,
                                  std::make_error_code(std::errc::not_enough_memory),
                                  "failed to create a document relationship for settings.xml",
                                  std::string{document_relationships_xml_entry});
        }

        settings_relationship.append_attribute("Id").set_value(relationship_id.c_str());
        settings_relationship.append_attribute("Type").set_value(
            settings_relationship_type.data());
        const auto relationship_target =
            make_document_relationship_target(settings_xml_entry);
        settings_relationship.append_attribute("Target").set_value(
            relationship_target.c_str());

        this->has_document_relationships_part = true;
        this->document_relationships_dirty = true;
    }

    const auto override_part_name = make_override_part_name(settings_xml_entry);
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
                              "failed to append a content type override for settings.xml",
                              std::string{content_types_xml_entry});
    }

    auto part_name_attribute = override_node.attribute("PartName");
    if (part_name_attribute == pugi::xml_attribute{}) {
        part_name_attribute = override_node.append_attribute("PartName");
    }
    part_name_attribute.set_value(override_part_name.c_str());

    auto content_type_attribute = override_node.attribute("ContentType");
    if (content_type_attribute == pugi::xml_attribute{}) {
        content_type_attribute = override_node.append_attribute("ContentType");
    }
    content_type_attribute.set_value(settings_content_type.data());

    this->has_settings_part = true;
    this->content_types_dirty = true;
    return {};
}

bool Document::enable_update_fields_on_open() {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before enabling updateFields",
                       std::string{settings_xml_entry});
        return false;
    }
    if (const auto error = this->ensure_settings_part_attached()) {
        return false;
    }

    auto settings_root = this->settings.child("w:settings");
    if (settings_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       document_errc::settings_xml_parse_failed,
                       "word/settings.xml does not contain the expected w:settings root",
                       std::string{settings_xml_entry});
        return false;
    }

    if (ensure_on_off_node_enabled(settings_root, "w:updateFields") ==
        pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to enable w:updateFields in word/settings.xml",
                       std::string{settings_xml_entry});
        return false;
    }

    this->settings_dirty = true;
    this->last_error_info.clear();
    return true;
}

bool Document::clear_update_fields_on_open() {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before clearing updateFields",
                       std::string{settings_xml_entry});
        return false;
    }
    if (const auto error = this->ensure_settings_loaded()) {
        return false;
    }

    auto settings_root = this->settings.child("w:settings");
    if (settings_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       document_errc::settings_xml_parse_failed,
                       "word/settings.xml does not contain the expected w:settings root",
                       std::string{settings_xml_entry});
        return false;
    }

    if (settings_root.child("w:updateFields") != pugi::xml_node{}) {
        settings_root.remove_child("w:updateFields");
        this->settings_dirty = true;
    }

    this->last_error_info.clear();
    return true;
}

Paragraph &Document::ensure_header_paragraphs() {
    return this->ensure_related_part_paragraphs(
        this->header_parts, "w:hdr", "w:headerReference",
        header_relationship_type.data(), header_content_type.data());
}

Paragraph &Document::ensure_footer_paragraphs() {
    return this->ensure_related_part_paragraphs(
        this->footer_parts, "w:ftr", "w:footerReference",
        footer_relationship_type.data(), footer_content_type.data());
}

Paragraph &Document::paragraphs() {
    this->paragraph.set_parent(document.child("w:document").child("w:body"));
    return this->paragraph;
}

Table &Document::tables() {
    this->table.set_owner(this);
    this->table.set_parent(document.child("w:document").child("w:body"));
    return this->table;
}

Table Document::append_table(std::size_t row_count, std::size_t column_count) {
    const auto body = document.child("w:document").child("w:body");
    const auto table_node = detail::append_table_node(body);
    auto created_table = Table(body, table_node);
    created_table.set_owner(this);

    for (std::size_t row_index = 0; row_index < row_count; ++row_index) {
        created_table.append_row(column_count);
    }

    return created_table;
}

#include "document_section_methods.inc"

#include "document_lifecycle_methods.inc"

} // namespace featherdoc
