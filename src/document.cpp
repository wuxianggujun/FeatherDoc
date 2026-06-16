#include "featherdoc.hpp"
#include "document_section_xml_helpers.hpp"
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

using detail::append_section_reference;
using detail::clear_section_header_footer_references;
using detail::collect_section_snapshots;
using detail::document_has_reference_type;
using detail::ensure_on_off_node_enabled;
using detail::ensure_section_property_node;
using detail::ensure_section_title_page_node;
using detail::ensure_xml_uint32_attribute;
using detail::find_section_reference;
using detail::read_on_off_value;
using detail::rebuild_body_from_section_snapshots;
using detail::remove_empty_node;
using detail::remove_empty_paragraph;
using detail::replace_section_properties_contents;
using detail::section_break_paragraph_for;
using detail::section_has_reference_type;

#include "document_settings_methods.inc"
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
