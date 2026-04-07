#include "featherdoc.hpp"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
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
constexpr auto styles_xml_entry = std::string_view{"word/styles.xml"};
constexpr auto styles_relationship_type = std::string_view{
    "http://schemas.openxmlformats.org/officeDocument/2006/relationships/styles"};
constexpr auto styles_content_type = std::string_view{
    "application/vnd.openxmlformats-officedocument.wordprocessingml.styles+xml"};
constexpr auto empty_relationships_xml = std::string_view{
    R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
</Relationships>
)"}; 
constexpr auto default_styles_xml = std::string_view{
    R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:styles xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:docDefaults>
    <w:rPrDefault><w:rPr/></w:rPrDefault>
    <w:pPrDefault><w:pPr/></w:pPrDefault>
  </w:docDefaults>
  <w:latentStyles w:defLockedState="0"
                  w:defUIPriority="99"
                  w:defSemiHidden="0"
                  w:defUnhideWhenUsed="0"
                  w:defQFormat="0"
                  w:count="0"/>
  <w:style w:type="paragraph" w:default="1" w:styleId="Normal">
    <w:name w:val="Normal"/>
    <w:qFormat/>
  </w:style>
  <w:style w:type="paragraph" w:styleId="Heading1">
    <w:name w:val="heading 1"/>
    <w:basedOn w:val="Normal"/>
    <w:next w:val="Normal"/>
    <w:uiPriority w:val="9"/>
    <w:qFormat/>
    <w:pPr><w:outlineLvl w:val="0"/></w:pPr>
    <w:rPr><w:b/><w:sz w:val="32"/></w:rPr>
  </w:style>
  <w:style w:type="paragraph" w:styleId="Heading2">
    <w:name w:val="heading 2"/>
    <w:basedOn w:val="Normal"/>
    <w:next w:val="Normal"/>
    <w:uiPriority w:val="9"/>
    <w:qFormat/>
    <w:pPr><w:outlineLvl w:val="1"/></w:pPr>
    <w:rPr><w:b/><w:sz w:val="28"/></w:rPr>
  </w:style>
  <w:style w:type="paragraph" w:styleId="Quote">
    <w:name w:val="Quote"/>
    <w:basedOn w:val="Normal"/>
    <w:uiPriority w:val="29"/>
    <w:qFormat/>
    <w:rPr><w:i/></w:rPr>
  </w:style>
  <w:style w:type="character" w:default="1" w:styleId="DefaultParagraphFont">
    <w:name w:val="Default Paragraph Font"/>
    <w:uiPriority w:val="1"/>
    <w:semiHidden/>
    <w:unhideWhenUsed/>
  </w:style>
  <w:style w:type="character" w:styleId="Emphasis">
    <w:name w:val="Emphasis"/>
    <w:basedOn w:val="DefaultParagraphFont"/>
    <w:uiPriority w:val="20"/>
    <w:qFormat/>
    <w:rPr><w:i/></w:rPr>
  </w:style>
  <w:style w:type="character" w:styleId="Strong">
    <w:name w:val="Strong"/>
    <w:basedOn w:val="DefaultParagraphFont"/>
    <w:uiPriority w:val="21"/>
    <w:qFormat/>
    <w:rPr><w:b/></w:rPr>
  </w:style>
  <w:style w:type="table" w:default="1" w:styleId="TableNormal">
    <w:name w:val="Normal Table"/>
    <w:uiPriority w:val="99"/>
    <w:semiHidden/>
  </w:style>
  <w:style w:type="table" w:styleId="TableGrid">
    <w:name w:val="Table Grid"/>
    <w:basedOn w:val="TableNormal"/>
    <w:uiPriority w:val="59"/>
    <w:tblPr>
      <w:tblBorders>
        <w:top w:val="single" w:sz="4" w:space="0" w:color="auto"/>
        <w:left w:val="single" w:sz="4" w:space="0" w:color="auto"/>
        <w:bottom w:val="single" w:sz="4" w:space="0" w:color="auto"/>
        <w:right w:val="single" w:sz="4" w:space="0" w:color="auto"/>
        <w:insideH w:val="single" w:sz="4" w:space="0" w:color="auto"/>
        <w:insideV w:val="single" w:sz="4" w:space="0" w:color="auto"/>
      </w:tblBorders>
    </w:tblPr>
  </w:style>
</w:styles>
)"}; 

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

auto node_has_attributes(pugi::xml_node node) -> bool {
    return node.first_attribute() != pugi::xml_attribute{};
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

auto ensure_run_properties_node(pugi::xml_node run) -> pugi::xml_node {
    if (run == pugi::xml_node{}) {
        return {};
    }

    auto run_properties = run.child("w:rPr");
    if (run_properties != pugi::xml_node{}) {
        return run_properties;
    }

    if (const auto first_child = run.first_child(); first_child != pugi::xml_node{}) {
        return run.insert_child_before("w:rPr", first_child);
    }

    return run.append_child("w:rPr");
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

void remove_empty_run_properties(pugi::xml_node run) {
    if (run == pugi::xml_node{}) {
        return;
    }

    auto run_properties = run.child("w:rPr");
    if (run_properties == pugi::xml_node{}) {
        return;
    }

    if (run_properties.first_child() == pugi::xml_node{} &&
        !node_has_attributes(run_properties)) {
        run.remove_child(run_properties);
    }
}
} // namespace

namespace featherdoc {

std::error_code Document::ensure_styles_loaded() {
    if (this->styles_loaded) {
        return {};
    }

    this->styles.reset();
    this->has_styles_part = false;

    if (!this->has_source_archive) {
        const auto parse_result =
            this->styles.load_buffer(default_styles_xml.data(), default_styles_xml.size());
        if (!parse_result) {
            return set_last_error(
                this->last_error_info, document_errc::styles_xml_parse_failed,
                parse_result.description(), std::string{styles_xml_entry},
                parse_result.offset >= 0
                    ? std::optional<std::ptrdiff_t>{parse_result.offset}
                    : std::nullopt);
        }

        this->styles_loaded = true;
        return {};
    }

    const auto relationships = this->document_relationships.child("Relationships");
    if (relationships == pugi::xml_node{}) {
        return set_last_error(this->last_error_info,
                              document_errc::relationships_xml_parse_failed,
                              "word/_rels/document.xml.rels does not contain a Relationships root",
                              std::string{document_relationships_xml_entry});
    }

    const auto styles_relationship =
        find_document_relationship_by_type(relationships, styles_relationship_type);
    if (styles_relationship == pugi::xml_node{}) {
        const auto parse_result =
            this->styles.load_buffer(default_styles_xml.data(), default_styles_xml.size());
        if (!parse_result) {
            return set_last_error(
                this->last_error_info, document_errc::styles_xml_parse_failed,
                parse_result.description(), std::string{styles_xml_entry},
                parse_result.offset >= 0
                    ? std::optional<std::ptrdiff_t>{parse_result.offset}
                    : std::nullopt);
        }

        this->styles_loaded = true;
        return {};
    }

    const auto target = std::string_view{styles_relationship.attribute("Target").value()};
    if (target.empty()) {
        return set_last_error(this->last_error_info, document_errc::styles_xml_read_failed,
                              "styles relationship does not contain a Target attribute",
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
                                  "' while loading word/styles.xml: " +
                                  zip_error_text(source_zip_error));
    }

    const auto styles_entry_name = normalize_word_part_entry(target);
    std::string styles_text;
    const auto read_status = read_zip_entry_text(source_zip, styles_entry_name, styles_text);
    zip_close(source_zip);

    if (read_status != zip_entry_read_status::ok) {
        return set_last_error(this->last_error_info,
                              document_errc::styles_xml_read_failed,
                              "failed to read required styles part '" + styles_entry_name + "'",
                              styles_entry_name);
    }

    const auto parse_result = this->styles.load_buffer(styles_text.data(), styles_text.size());
    if (!parse_result) {
        return set_last_error(
            this->last_error_info, document_errc::styles_xml_parse_failed,
            parse_result.description(), styles_entry_name,
            parse_result.offset >= 0 ? std::optional<std::ptrdiff_t>{parse_result.offset}
                                     : std::nullopt);
    }

    this->styles_loaded = true;
    this->has_styles_part = true;
    return {};
}

std::error_code Document::ensure_styles_part_attached() {
    if (const auto error = this->ensure_styles_loaded()) {
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

    const bool had_styles_part = this->has_styles_part;

    auto styles_relationship =
        find_document_relationship_by_type(relationships, styles_relationship_type);
    if (styles_relationship == pugi::xml_node{}) {
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

        styles_relationship = relationships.append_child("Relationship");
        if (styles_relationship == pugi::xml_node{}) {
            return set_last_error(this->last_error_info,
                                  std::make_error_code(std::errc::not_enough_memory),
                                  "failed to create a document relationship for styles.xml",
                                  std::string{document_relationships_xml_entry});
        }

        ensure_attribute_value(styles_relationship, "Id", relationship_id);
        ensure_attribute_value(styles_relationship, "Type", styles_relationship_type);
        ensure_attribute_value(styles_relationship, "Target",
                               make_document_relationship_target(styles_xml_entry));

        this->has_document_relationships_part = true;
        this->document_relationships_dirty = true;
    }

    const auto override_part_name = make_override_part_name(styles_xml_entry);
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
                              "failed to append a content type override for styles.xml",
                              std::string{content_types_xml_entry});
    }

    ensure_attribute_value(override_node, "PartName", override_part_name);
    ensure_attribute_value(override_node, "ContentType", styles_content_type);

    this->has_styles_part = true;
    this->content_types_dirty = true;
    if (!had_styles_part) {
        this->styles_dirty = true;
    }
    return {};
}

bool Document::set_paragraph_style(Paragraph paragraph_handle, std::string_view style_id) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before editing paragraph styles");
        return false;
    }

    if (style_id.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "paragraph style id must not be empty",
                       std::string{document_xml_entry});
        return false;
    }

    if (!paragraph_handle.has_next() || paragraph_handle.current == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "target paragraph handle is not valid",
                       std::string{document_xml_entry});
        return false;
    }

    if (const auto error = this->ensure_styles_part_attached()) {
        return false;
    }

    auto paragraph_properties = ensure_paragraph_properties_node(paragraph_handle.current);
    if (paragraph_properties == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to create w:pPr for the target paragraph",
                       std::string{document_xml_entry});
        return false;
    }

    auto style_node = paragraph_properties.child("w:pStyle");
    if (style_node == pugi::xml_node{}) {
        if (const auto first_child = paragraph_properties.first_child();
            first_child != pugi::xml_node{}) {
            style_node = paragraph_properties.insert_child_before("w:pStyle", first_child);
        } else {
            style_node = paragraph_properties.append_child("w:pStyle");
        }
    }
    if (style_node == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to create w:pStyle for the target paragraph",
                       std::string{document_xml_entry});
        return false;
    }

    ensure_attribute_value(style_node, "w:val", style_id);
    this->last_error_info.clear();
    return true;
}

bool Document::clear_paragraph_style(Paragraph paragraph_handle) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before editing paragraph styles");
        return false;
    }

    if (!paragraph_handle.has_next() || paragraph_handle.current == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "target paragraph handle is not valid",
                       std::string{document_xml_entry});
        return false;
    }

    if (auto paragraph_properties = paragraph_handle.current.child("w:pPr");
        paragraph_properties != pugi::xml_node{}) {
        paragraph_properties.remove_child("w:pStyle");
        remove_empty_paragraph_properties(paragraph_handle.current);
    }

    this->last_error_info.clear();
    return true;
}

bool Document::set_run_style(Run run_handle, std::string_view style_id) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before editing run styles");
        return false;
    }

    if (style_id.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "run style id must not be empty",
                       std::string{document_xml_entry});
        return false;
    }

    if (!run_handle.has_next() || run_handle.current == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "target run handle is not valid",
                       std::string{document_xml_entry});
        return false;
    }

    if (const auto error = this->ensure_styles_part_attached()) {
        return false;
    }

    auto run_properties = ensure_run_properties_node(run_handle.current);
    if (run_properties == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to create w:rPr for the target run",
                       std::string{document_xml_entry});
        return false;
    }

    auto style_node = run_properties.child("w:rStyle");
    if (style_node == pugi::xml_node{}) {
        if (const auto first_child = run_properties.first_child();
            first_child != pugi::xml_node{}) {
            style_node = run_properties.insert_child_before("w:rStyle", first_child);
        } else {
            style_node = run_properties.append_child("w:rStyle");
        }
    }
    if (style_node == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to create w:rStyle for the target run",
                       std::string{document_xml_entry});
        return false;
    }

    ensure_attribute_value(style_node, "w:val", style_id);
    this->last_error_info.clear();
    return true;
}

bool Document::clear_run_style(Run run_handle) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before editing run styles");
        return false;
    }

    if (!run_handle.has_next() || run_handle.current == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "target run handle is not valid",
                       std::string{document_xml_entry});
        return false;
    }

    if (auto run_properties = run_handle.current.child("w:rPr");
        run_properties != pugi::xml_node{}) {
        run_properties.remove_child("w:rStyle");
        remove_empty_run_properties(run_handle.current);
    }

    this->last_error_info.clear();
    return true;
}

} // namespace featherdoc
