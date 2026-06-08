#include "featherdoc.hpp"

#include "document_table_style_helpers.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <limits>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include <zip.h>

namespace {
constexpr auto document_xml_entry = std::string_view{"word/document.xml"};
constexpr auto document_relationships_xml_entry =
    std::string_view{"word/_rels/document.xml.rels"};
constexpr auto content_types_xml_entry =
    std::string_view{"[Content_Types].xml"};
constexpr auto styles_xml_entry = std::string_view{"word/styles.xml"};
constexpr auto styles_relationship_type =
    std::string_view{"http://schemas.openxmlformats.org/officeDocument/2006/"
                     "relationships/styles"};
constexpr auto styles_content_type = std::string_view{
    "application/"
    "vnd.openxmlformats-officedocument.wordprocessingml.styles+xml"};
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

auto initialize_xml_document(pugi::xml_document &xml_document,
                             std::string_view xml_text) -> bool {
    xml_document.reset();
    return static_cast<bool>(
        xml_document.load_buffer(xml_text.data(), xml_text.size()));
}

auto initialize_empty_relationships_document(pugi::xml_document &xml_document)
    -> bool {
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
                          std::move(detail), std::move(entry_name),
                          std::move(xml_offset));
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

auto read_zip_entry_text(zip_t *archive, std::string_view entry_name,
                         std::string &content) -> zip_entry_read_status {
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

    return (std::filesystem::path{"word"} /
            std::filesystem::path{normalized_target})
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

auto make_document_relationship_target(std::string_view entry_name)
    -> std::string {
    const auto normalized_entry =
        std::filesystem::path{std::string{entry_name}}.lexically_normal();
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
        if (std::string_view{relationship.attribute("Type").value()} ==
            relationship_type) {
            return relationship;
        }
    }

    return {};
}

void ensure_attribute_value(pugi::xml_node node, const char *name,
                            std::string_view value) {
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

#include "document_styles_summary_helpers.inc"

#include "document_styles_ensure_helpers.inc"

#include "document_styles_metadata_helpers.inc"

#include "document_styles_reference_helpers.inc"

#include "document_styles_usage_helpers.inc"

#include "document_styles_mutation_helpers.inc"

#include "document_styles_refactor_helpers.inc"
} // namespace

namespace featherdoc {

std::error_code Document::ensure_styles_loaded() {
    if (this->styles_loaded) {
        return {};
    }

    this->styles.reset();
    this->has_styles_part = false;

    if (!this->has_source_archive) {
        const auto parse_result = this->styles.load_buffer(
            default_styles_xml.data(), default_styles_xml.size());
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

    const auto relationships =
        this->document_relationships.child("Relationships");
    if (relationships == pugi::xml_node{}) {
        return set_last_error(this->last_error_info,
                              document_errc::relationships_xml_parse_failed,
                              "word/_rels/document.xml.rels does not contain a "
                              "Relationships root",
                              std::string{document_relationships_xml_entry});
    }

    const auto styles_relationship = find_document_relationship_by_type(
        relationships, styles_relationship_type);
    if (styles_relationship == pugi::xml_node{}) {
        const auto parse_result = this->styles.load_buffer(
            default_styles_xml.data(), default_styles_xml.size());
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

    const auto target =
        std::string_view{styles_relationship.attribute("Target").value()};
    if (target.empty()) {
        return set_last_error(
            this->last_error_info, document_errc::styles_xml_read_failed,
            "styles relationship does not contain a Target attribute",
            std::string{document_relationships_xml_entry});
    }

    int source_zip_error = 0;
    zip_t *source_zip = zip_openwitherror(this->document_path.string().c_str(),
                                          ZIP_DEFAULT_COMPRESSION_LEVEL, 'r',
                                          &source_zip_error);
    if (!source_zip) {
        return set_last_error(
            this->last_error_info, document_errc::source_archive_open_failed,
            "failed to reopen source archive '" + this->document_path.string() +
                "' while loading word/styles.xml: " +
                zip_error_text(source_zip_error));
    }

    const auto styles_entry_name = normalize_word_part_entry(target);
    std::string styles_text;
    const auto read_status =
        read_zip_entry_text(source_zip, styles_entry_name, styles_text);
    zip_close(source_zip);

    if (read_status != zip_entry_read_status::ok) {
        return set_last_error(
            this->last_error_info, document_errc::styles_xml_read_failed,
            "failed to read required styles part '" + styles_entry_name + "'",
            styles_entry_name);
    }

    const auto parse_result =
        this->styles.load_buffer(styles_text.data(), styles_text.size());
    if (!parse_result) {
        return set_last_error(
            this->last_error_info, document_errc::styles_xml_parse_failed,
            parse_result.description(), styles_entry_name,
            parse_result.offset >= 0
                ? std::optional<std::ptrdiff_t>{parse_result.offset}
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

    if (this->document_relationships.child("Relationships") ==
            pugi::xml_node{} &&
        !initialize_empty_relationships_document(
            this->document_relationships)) {
        return set_last_error(this->last_error_info,
                              document_errc::relationships_xml_parse_failed,
                              "failed to initialize default relationships XML",
                              std::string{document_relationships_xml_entry});
    }

    auto relationships = this->document_relationships.child("Relationships");
    if (relationships == pugi::xml_node{}) {
        return set_last_error(this->last_error_info,
                              document_errc::relationships_xml_parse_failed,
                              "word/_rels/document.xml.rels does not contain a "
                              "Relationships root",
                              std::string{document_relationships_xml_entry});
    }

    auto types = this->content_types.child("Types");
    if (types == pugi::xml_node{}) {
        return set_last_error(
            this->last_error_info,
            document_errc::content_types_xml_parse_failed,
            "[Content_Types].xml does not contain a Types root",
            std::string{content_types_xml_entry});
    }

    const bool had_styles_part = this->has_styles_part;

    auto styles_relationship = find_document_relationship_by_type(
        relationships, styles_relationship_type);
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
        for (std::size_t next_index = 1U; relationship_id.empty();
             ++next_index) {
            auto candidate = "rId" + std::to_string(next_index);
            if (!used_relationship_ids.contains(candidate)) {
                relationship_id = std::move(candidate);
            }
        }

        styles_relationship = relationships.append_child("Relationship");
        if (styles_relationship == pugi::xml_node{}) {
            return set_last_error(
                this->last_error_info,
                std::make_error_code(std::errc::not_enough_memory),
                "failed to create a document relationship for styles.xml",
                std::string{document_relationships_xml_entry});
        }

        ensure_attribute_value(styles_relationship, "Id", relationship_id);
        ensure_attribute_value(styles_relationship, "Type",
                               styles_relationship_type);
        ensure_attribute_value(
            styles_relationship, "Target",
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
        return set_last_error(
            this->last_error_info,
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

#include "document_styles_catalog_methods.inc"

#include "document_styles_usage_methods.inc"

#include "document_styles_refactor_methods.inc"

#include "document_styles_ensure_methods.inc"

#include "document_styles_table_style_methods.inc"

#include "document_styles_defaults_methods.inc"

#include "document_styles_property_read_methods.inc"

bool Document::materialize_style_run_properties(std::string_view style_id) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before materializing "
                       "style run properties",
                       std::string{styles_xml_entry});
        return false;
    }

    if (style_id.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id must not be empty when materializing style "
                       "run properties",
                       std::string{styles_xml_entry});
        return false;
    }

    const auto resolved = this->resolve_style_properties(style_id);
    if (!resolved.has_value()) {
        return false;
    }

    if (resolved->kind != featherdoc::style_kind::paragraph &&
        resolved->kind != featherdoc::style_kind::character) {
        set_last_error(
            this->last_error_info,
            std::make_error_code(std::errc::invalid_argument),
            "style id '" + std::string{style_id} + "' with type '" +
                resolved->type_name +
                "' does not support style run property materialization",
            std::string{styles_xml_entry});
        return false;
    }

    const auto style_id_string = std::string{style_id};
    const auto materialize_string =
        [&](const featherdoc::resolved_style_string_property &property,
            auto setter) -> bool {
        if (!property.value.has_value() ||
            !property.source_style_id.has_value() ||
            *property.source_style_id == style_id_string) {
            return true;
        }

        return (this->*setter)(style_id_string, *property.value);
    };

    const auto materialize_bool =
        [&](const featherdoc::resolved_style_bool_property &property,
            auto setter) -> bool {
        if (!property.value.has_value() ||
            !property.source_style_id.has_value() ||
            *property.source_style_id == style_id_string) {
            return true;
        }

        return (this->*setter)(style_id_string, *property.value);
    };

    const auto materialize_double =
        [&](const featherdoc::resolved_style_double_property &property,
            auto setter) -> bool {
        if (!property.value.has_value() ||
            !property.source_style_id.has_value() ||
            *property.source_style_id == style_id_string) {
            return true;
        }

        return (this->*setter)(style_id_string, *property.value);
    };

    const auto materialize_vertical_align = [&]() -> bool {
        const auto inherited_superscript =
            resolved->run_superscript.value.has_value() &&
            resolved->run_superscript.source_style_id.has_value() &&
            *resolved->run_superscript.source_style_id != style_id_string;
        const auto inherited_subscript =
            resolved->run_subscript.value.has_value() &&
            resolved->run_subscript.source_style_id.has_value() &&
            *resolved->run_subscript.source_style_id != style_id_string;
        if (!inherited_superscript && !inherited_subscript) {
            return true;
        }

        if (resolved->run_superscript.value.value_or(false)) {
            return this->set_style_run_superscript(style_id_string, true);
        }
        if (resolved->run_subscript.value.value_or(false)) {
            return this->set_style_run_subscript(style_id_string, true);
        }
        return this->set_style_run_superscript(style_id_string, false);
    };

    if (!materialize_string(resolved->run_text_color,
                            &Document::set_style_run_text_color) ||
        !materialize_bool(resolved->run_bold, &Document::set_style_run_bold) ||
        !materialize_bool(resolved->run_italic,
                          &Document::set_style_run_italic) ||
        !materialize_bool(resolved->run_strikethrough,
                          &Document::set_style_run_strikethrough) ||
        !materialize_bool(resolved->run_underline,
                          &Document::set_style_run_underline) ||
        !materialize_vertical_align() ||
        !materialize_double(resolved->run_font_size_points,
                            &Document::set_style_run_font_size_points) ||
        !materialize_string(resolved->run_font_family,
                            &Document::set_style_run_font_family) ||
        !materialize_string(resolved->run_east_asia_font_family,
                            &Document::set_style_run_east_asia_font_family) ||
        !materialize_string(resolved->run_language,
                            &Document::set_style_run_language) ||
        !materialize_string(resolved->run_east_asia_language,
                            &Document::set_style_run_east_asia_language) ||
        !materialize_string(resolved->run_bidi_language,
                            &Document::set_style_run_bidi_language) ||
        !materialize_bool(resolved->run_rtl, &Document::set_style_run_rtl)) {
        return false;
    }

    if (resolved->kind == featherdoc::style_kind::paragraph &&
        !materialize_bool(resolved->paragraph_bidi,
                          &Document::set_style_paragraph_bidi)) {
        return false;
    }

    this->last_error_info.clear();
    return true;
}

bool Document::rebase_character_style_based_on(std::string_view style_id,
                                               std::string_view based_on) {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before rebasing character styles",
            std::string{styles_xml_entry});
        return false;
    }

    if (style_id.empty()) {
        set_last_error(
            this->last_error_info,
            std::make_error_code(std::errc::invalid_argument),
            "style id must not be empty when rebasing character styles",
            std::string{styles_xml_entry});
        return false;
    }

    if (based_on.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "character style basedOn must not be empty",
                       std::string{styles_xml_entry});
        return false;
    }

    if (style_id == based_on) {
        set_last_error(
            this->last_error_info,
            std::make_error_code(std::errc::invalid_argument),
            "character style basedOn must not reference the same style id",
            std::string{styles_xml_entry});
        return false;
    }

    if (const auto error = this->ensure_styles_loaded()) {
        return false;
    }

    const auto styles_root = this->styles.child("w:styles");
    if (styles_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       document_errc::styles_xml_parse_failed,
                       "word/styles.xml does not contain a w:styles root",
                       std::string{styles_xml_entry});
        return false;
    }

    const auto style = find_style_node(styles_root, style_id);
    if (style == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id '" + std::string{style_id} +
                           "' was not found in word/styles.xml",
                       std::string{styles_xml_entry});
        return false;
    }

    const auto type_name = std::string_view{style.attribute("w:type").value()};
    if (!type_name.empty() && type_name != "character") {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id '" + std::string{style_id} +
                           "' is not a character style",
                       std::string{styles_xml_entry});
        return false;
    }

    const auto parent_style = find_style_node(styles_root, based_on);
    if (parent_style == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "basedOn style id '" + std::string{based_on} +
                           "' was not found in word/styles.xml",
                       std::string{styles_xml_entry});
        return false;
    }

    const auto parent_type_name =
        std::string_view{parent_style.attribute("w:type").value()};
    if (!parent_type_name.empty() && parent_type_name != "character") {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "basedOn style id '" + std::string{based_on} +
                           "' is not a character style",
                       std::string{styles_xml_entry});
        return false;
    }

    if (!this->materialize_style_run_properties(style_id)) {
        return false;
    }

    if (!this->set_character_style_based_on(style_id, based_on)) {
        return false;
    }

    this->last_error_info.clear();
    return true;
}

bool Document::set_character_style_based_on(std::string_view style_id,
                                            std::string_view based_on) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before editing character "
                       "style metadata");
        return false;
    }

    if (style_id.empty()) {
        set_last_error(
            this->last_error_info,
            std::make_error_code(std::errc::invalid_argument),
            "style id must not be empty when editing character style metadata",
            std::string{styles_xml_entry});
        return false;
    }

    if (based_on.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "character style basedOn must not be empty",
                       std::string{styles_xml_entry});
        return false;
    }

    if (based_on == style_id) {
        set_last_error(
            this->last_error_info,
            std::make_error_code(std::errc::invalid_argument),
            "character style basedOn must not reference the same style id",
            std::string{styles_xml_entry});
        return false;
    }

    if (const auto error = this->ensure_styles_loaded()) {
        return false;
    }

    auto styles_root = this->styles.child("w:styles");
    if (styles_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       document_errc::styles_xml_parse_failed,
                       "word/styles.xml does not contain a w:styles root",
                       std::string{styles_xml_entry});
        return false;
    }

    auto style = find_style_node(styles_root, style_id);
    if (style == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id '" + std::string{style_id} +
                           "' was not found in word/styles.xml",
                       std::string{styles_xml_entry});
        return false;
    }

    const auto type_name = std::string_view{style.attribute("w:type").value()};
    if (!type_name.empty() && type_name != "character") {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id '" + std::string{style_id} +
                           "' is not a character style",
                       std::string{styles_xml_entry});
        return false;
    }

    const auto parent_style = find_style_node(styles_root, based_on);
    if (parent_style == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "basedOn style id '" + std::string{based_on} +
                           "' was not found in word/styles.xml",
                       std::string{styles_xml_entry});
        return false;
    }

    const auto parent_type_name =
        std::string_view{parent_style.attribute("w:type").value()};
    if (!parent_type_name.empty() && parent_type_name != "character") {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "basedOn style id '" + std::string{based_on} +
                           "' is not a character style",
                       std::string{styles_xml_entry});
        return false;
    }

    if (!this->has_styles_part) {
        if (const auto error = this->ensure_styles_part_attached()) {
            return false;
        }
        styles_root = this->styles.child("w:styles");
        style = find_style_node(styles_root, style_id);
    }

    if (!set_style_string_node(style, "w:basedOn", std::string{based_on})) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to update basedOn for character style '" +
                           std::string{style_id} + "'",
                       std::string{styles_xml_entry});
        return false;
    }

    this->styles_dirty = true;
    this->last_error_info.clear();
    return true;
}

bool Document::clear_character_style_based_on(std::string_view style_id) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before editing character "
                       "style metadata");
        return false;
    }

    if (style_id.empty()) {
        set_last_error(
            this->last_error_info,
            std::make_error_code(std::errc::invalid_argument),
            "style id must not be empty when editing character style metadata",
            std::string{styles_xml_entry});
        return false;
    }

    if (const auto error = this->ensure_styles_loaded()) {
        return false;
    }

    auto styles_root = this->styles.child("w:styles");
    if (styles_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       document_errc::styles_xml_parse_failed,
                       "word/styles.xml does not contain a w:styles root",
                       std::string{styles_xml_entry});
        return false;
    }

    const auto style = find_style_node(styles_root, style_id);
    if (style == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id '" + std::string{style_id} +
                           "' was not found in word/styles.xml",
                       std::string{styles_xml_entry});
        return false;
    }

    const auto type_name = std::string_view{style.attribute("w:type").value()};
    if (!type_name.empty() && type_name != "character") {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id '" + std::string{style_id} +
                           "' is not a character style",
                       std::string{styles_xml_entry});
        return false;
    }

    if (!set_style_string_node(style, "w:basedOn", std::nullopt)) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to clear basedOn for character style '" +
                           std::string{style_id} + "'",
                       std::string{styles_xml_entry});
        return false;
    }

    this->styles_dirty = true;
    this->last_error_info.clear();
    return true;
}

bool Document::rebase_paragraph_style_based_on(std::string_view style_id,
                                               std::string_view based_on) {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before rebasing paragraph styles",
            std::string{styles_xml_entry});
        return false;
    }

    if (style_id.empty()) {
        set_last_error(
            this->last_error_info,
            std::make_error_code(std::errc::invalid_argument),
            "style id must not be empty when rebasing paragraph styles",
            std::string{styles_xml_entry});
        return false;
    }

    if (based_on.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "paragraph style basedOn must not be empty",
                       std::string{styles_xml_entry});
        return false;
    }

    if (style_id == based_on) {
        set_last_error(
            this->last_error_info,
            std::make_error_code(std::errc::invalid_argument),
            "paragraph style basedOn must not reference the same style id",
            std::string{styles_xml_entry});
        return false;
    }

    if (const auto error = this->ensure_styles_loaded()) {
        return false;
    }

    const auto styles_root = this->styles.child("w:styles");
    if (styles_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       document_errc::styles_xml_parse_failed,
                       "word/styles.xml does not contain a w:styles root",
                       std::string{styles_xml_entry});
        return false;
    }

    const auto style = find_style_node(styles_root, style_id);
    if (style == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id '" + std::string{style_id} +
                           "' was not found in word/styles.xml",
                       std::string{styles_xml_entry});
        return false;
    }

    const auto type_name = std::string_view{style.attribute("w:type").value()};
    if (!type_name.empty() && type_name != "paragraph") {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id '" + std::string{style_id} +
                           "' is not a paragraph style",
                       std::string{styles_xml_entry});
        return false;
    }

    const auto parent_style = find_style_node(styles_root, based_on);
    if (parent_style == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "basedOn style id '" + std::string{based_on} +
                           "' was not found in word/styles.xml",
                       std::string{styles_xml_entry});
        return false;
    }

    const auto parent_type_name =
        std::string_view{parent_style.attribute("w:type").value()};
    if (!parent_type_name.empty() && parent_type_name != "paragraph") {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "basedOn style id '" + std::string{based_on} +
                           "' is not a paragraph style",
                       std::string{styles_xml_entry});
        return false;
    }

    if (!this->materialize_style_run_properties(style_id)) {
        return false;
    }

    if (!this->set_paragraph_style_based_on(style_id, based_on)) {
        return false;
    }

    this->last_error_info.clear();
    return true;
}

bool Document::set_paragraph_style_based_on(std::string_view style_id,
                                            std::string_view based_on) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before editing paragraph "
                       "style metadata");
        return false;
    }

    if (style_id.empty()) {
        set_last_error(
            this->last_error_info,
            std::make_error_code(std::errc::invalid_argument),
            "style id must not be empty when editing paragraph style metadata",
            std::string{styles_xml_entry});
        return false;
    }

    if (based_on.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "paragraph style basedOn must not be empty",
                       std::string{styles_xml_entry});
        return false;
    }

    if (based_on == style_id) {
        set_last_error(
            this->last_error_info,
            std::make_error_code(std::errc::invalid_argument),
            "paragraph style basedOn must not reference the same style id",
            std::string{styles_xml_entry});
        return false;
    }

    if (const auto error = this->ensure_styles_loaded()) {
        return false;
    }

    auto styles_root = this->styles.child("w:styles");
    if (styles_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       document_errc::styles_xml_parse_failed,
                       "word/styles.xml does not contain a w:styles root",
                       std::string{styles_xml_entry});
        return false;
    }

    auto style = find_style_node(styles_root, style_id);
    if (style == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id '" + std::string{style_id} +
                           "' was not found in word/styles.xml",
                       std::string{styles_xml_entry});
        return false;
    }

    const auto type_name = std::string_view{style.attribute("w:type").value()};
    if (!type_name.empty() && type_name != "paragraph") {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id '" + std::string{style_id} +
                           "' is not a paragraph style",
                       std::string{styles_xml_entry});
        return false;
    }

    if (!this->has_styles_part) {
        if (const auto error = this->ensure_styles_part_attached()) {
            return false;
        }
        styles_root = this->styles.child("w:styles");
        style = find_style_node(styles_root, style_id);
    }

    if (!set_style_string_node(style, "w:basedOn", std::string{based_on})) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to update basedOn for paragraph style '" +
                           std::string{style_id} + "'",
                       std::string{styles_xml_entry});
        return false;
    }

    this->styles_dirty = true;
    this->last_error_info.clear();
    return true;
}

bool Document::clear_paragraph_style_based_on(std::string_view style_id) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before editing paragraph "
                       "style metadata");
        return false;
    }

    if (style_id.empty()) {
        set_last_error(
            this->last_error_info,
            std::make_error_code(std::errc::invalid_argument),
            "style id must not be empty when editing paragraph style metadata",
            std::string{styles_xml_entry});
        return false;
    }

    if (const auto error = this->ensure_styles_loaded()) {
        return false;
    }

    auto styles_root = this->styles.child("w:styles");
    if (styles_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       document_errc::styles_xml_parse_failed,
                       "word/styles.xml does not contain a w:styles root",
                       std::string{styles_xml_entry});
        return false;
    }

    auto style = find_style_node(styles_root, style_id);
    if (style == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id '" + std::string{style_id} +
                           "' was not found in word/styles.xml",
                       std::string{styles_xml_entry});
        return false;
    }

    const auto type_name = std::string_view{style.attribute("w:type").value()};
    if (!type_name.empty() && type_name != "paragraph") {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id '" + std::string{style_id} +
                           "' is not a paragraph style",
                       std::string{styles_xml_entry});
        return false;
    }

    if (!set_style_string_node(style, "w:basedOn", std::nullopt)) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to clear basedOn for paragraph style '" +
                           std::string{style_id} + "'",
                       std::string{styles_xml_entry});
        return false;
    }

    this->styles_dirty = true;
    this->last_error_info.clear();
    return true;
}

bool Document::set_paragraph_style_next_style(std::string_view style_id,
                                              std::string_view next_style) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before editing paragraph "
                       "style metadata");
        return false;
    }

    if (style_id.empty()) {
        set_last_error(
            this->last_error_info,
            std::make_error_code(std::errc::invalid_argument),
            "style id must not be empty when editing paragraph style metadata",
            std::string{styles_xml_entry});
        return false;
    }

    if (next_style.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "paragraph style next style must not be empty",
                       std::string{styles_xml_entry});
        return false;
    }

    if (const auto error = this->ensure_styles_loaded()) {
        return false;
    }

    auto styles_root = this->styles.child("w:styles");
    if (styles_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       document_errc::styles_xml_parse_failed,
                       "word/styles.xml does not contain a w:styles root",
                       std::string{styles_xml_entry});
        return false;
    }

    auto style = find_style_node(styles_root, style_id);
    if (style == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id '" + std::string{style_id} +
                           "' was not found in word/styles.xml",
                       std::string{styles_xml_entry});
        return false;
    }

    const auto type_name = std::string_view{style.attribute("w:type").value()};
    if (!type_name.empty() && type_name != "paragraph") {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id '" + std::string{style_id} +
                           "' is not a paragraph style",
                       std::string{styles_xml_entry});
        return false;
    }

    if (!this->has_styles_part) {
        if (const auto error = this->ensure_styles_part_attached()) {
            return false;
        }
        styles_root = this->styles.child("w:styles");
        style = find_style_node(styles_root, style_id);
    }

    if (!set_style_string_node(style, "w:next", std::string{next_style})) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to update next style for paragraph style '" +
                           std::string{style_id} + "'",
                       std::string{styles_xml_entry});
        return false;
    }

    this->styles_dirty = true;
    this->last_error_info.clear();
    return true;
}

bool Document::clear_paragraph_style_next_style(std::string_view style_id) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before editing paragraph "
                       "style metadata");
        return false;
    }

    if (style_id.empty()) {
        set_last_error(
            this->last_error_info,
            std::make_error_code(std::errc::invalid_argument),
            "style id must not be empty when editing paragraph style metadata",
            std::string{styles_xml_entry});
        return false;
    }

    if (const auto error = this->ensure_styles_loaded()) {
        return false;
    }

    const auto styles_root = this->styles.child("w:styles");
    if (styles_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       document_errc::styles_xml_parse_failed,
                       "word/styles.xml does not contain a w:styles root",
                       std::string{styles_xml_entry});
        return false;
    }

    auto style = find_style_node(styles_root, style_id);
    if (style == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id '" + std::string{style_id} +
                           "' was not found in word/styles.xml",
                       std::string{styles_xml_entry});
        return false;
    }

    const auto type_name = std::string_view{style.attribute("w:type").value()};
    if (!type_name.empty() && type_name != "paragraph") {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id '" + std::string{style_id} +
                           "' is not a paragraph style",
                       std::string{styles_xml_entry});
        return false;
    }

    if (!set_style_string_node(style, "w:next", std::nullopt)) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to clear next style for paragraph style '" +
                           std::string{style_id} + "'",
                       std::string{styles_xml_entry});
        return false;
    }

    this->styles_dirty = true;
    this->last_error_info.clear();
    return true;
}

bool Document::set_paragraph_style_outline_level(std::string_view style_id,
                                                 std::uint32_t outline_level) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before editing paragraph "
                       "style metadata");
        return false;
    }

    if (style_id.empty()) {
        set_last_error(
            this->last_error_info,
            std::make_error_code(std::errc::invalid_argument),
            "style id must not be empty when editing paragraph style metadata",
            std::string{styles_xml_entry});
        return false;
    }

    if (outline_level > 8U) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "paragraph style outline level must be between 0 and 8",
                       std::string{styles_xml_entry});
        return false;
    }

    if (const auto error = this->ensure_styles_loaded()) {
        return false;
    }

    auto styles_root = this->styles.child("w:styles");
    if (styles_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       document_errc::styles_xml_parse_failed,
                       "word/styles.xml does not contain a w:styles root",
                       std::string{styles_xml_entry});
        return false;
    }

    auto style = find_style_node(styles_root, style_id);
    if (style == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id '" + std::string{style_id} +
                           "' was not found in word/styles.xml",
                       std::string{styles_xml_entry});
        return false;
    }

    const auto type_name = std::string_view{style.attribute("w:type").value()};
    if (!type_name.empty() && type_name != "paragraph") {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id '" + std::string{style_id} +
                           "' is not a paragraph style",
                       std::string{styles_xml_entry});
        return false;
    }

    if (!this->has_styles_part) {
        if (const auto error = this->ensure_styles_part_attached()) {
            return false;
        }
        styles_root = this->styles.child("w:styles");
        style = find_style_node(styles_root, style_id);
    }

    if (!set_style_outline_level(style, outline_level)) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to update outline level for paragraph style '" +
                           std::string{style_id} + "'",
                       std::string{styles_xml_entry});
        return false;
    }

    this->styles_dirty = true;
    this->last_error_info.clear();
    return true;
}

bool Document::clear_paragraph_style_outline_level(std::string_view style_id) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before editing paragraph "
                       "style metadata");
        return false;
    }

    if (style_id.empty()) {
        set_last_error(
            this->last_error_info,
            std::make_error_code(std::errc::invalid_argument),
            "style id must not be empty when editing paragraph style metadata",
            std::string{styles_xml_entry});
        return false;
    }

    if (const auto error = this->ensure_styles_loaded()) {
        return false;
    }

    const auto styles_root = this->styles.child("w:styles");
    if (styles_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       document_errc::styles_xml_parse_failed,
                       "word/styles.xml does not contain a w:styles root",
                       std::string{styles_xml_entry});
        return false;
    }

    auto style = find_style_node(styles_root, style_id);
    if (style == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id '" + std::string{style_id} +
                           "' was not found in word/styles.xml",
                       std::string{styles_xml_entry});
        return false;
    }

    const auto type_name = std::string_view{style.attribute("w:type").value()};
    if (!type_name.empty() && type_name != "paragraph") {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id '" + std::string{style_id} +
                           "' is not a paragraph style",
                       std::string{styles_xml_entry});
        return false;
    }

    if (!set_style_outline_level(style, std::nullopt)) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to clear outline level for paragraph style '" +
                           std::string{style_id} + "'",
                       std::string{styles_xml_entry});
        return false;
    }

    this->styles_dirty = true;
    this->last_error_info.clear();
    return true;
}

bool Document::set_style_run_text_color(std::string_view style_id,
                                        std::string_view text_color) {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before editing style run color",
            std::string{styles_xml_entry});
        return false;
    }

    if (style_id.empty() || text_color.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id and run text color must not be empty",
                       std::string{styles_xml_entry});
        return false;
    }

    if (const auto error = this->ensure_styles_part_attached()) {
        return false;
    }

    auto styles_root = this->styles.child("w:styles");
    auto style = find_style_node(styles_root, style_id);
    if (style == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id '" + std::string{style_id} +
                           "' was not found in word/styles.xml",
                       std::string{styles_xml_entry});
        return false;
    }

    const auto run_properties = ensure_style_run_properties_node(style);
    const auto color = ensure_run_text_color_node(run_properties);
    if (color == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to create w:color for style '" +
                           std::string{style_id} + "'",
                       std::string{styles_xml_entry});
        return false;
    }

    ensure_attribute_value(color, "w:val", text_color);
    this->styles_dirty = true;
    this->last_error_info.clear();
    return true;
}

bool Document::set_style_run_bold(std::string_view style_id, bool enabled) {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before editing style run bold",
            std::string{styles_xml_entry});
        return false;
    }
    if (style_id.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id must not be empty when editing style run bold",
                       std::string{styles_xml_entry});
        return false;
    }
    if (const auto error = this->ensure_styles_part_attached()) {
        return false;
    }
    auto style = find_style_node(this->styles.child("w:styles"), style_id);
    if (style == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id '" + std::string{style_id} +
                           "' was not found in word/styles.xml",
                       std::string{styles_xml_entry});
        return false;
    }
    const auto bold =
        ensure_run_bold_node(ensure_style_run_properties_node(style));
    if (bold == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to create w:b for style '" +
                           std::string{style_id} + "'",
                       std::string{styles_xml_entry});
        return false;
    }
    set_on_off_value(bold, enabled);
    this->styles_dirty = true;
    this->last_error_info.clear();
    return true;
}

bool Document::set_style_run_italic(std::string_view style_id, bool enabled) {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before editing style run italic",
            std::string{styles_xml_entry});
        return false;
    }
    if (style_id.empty()) {
        set_last_error(
            this->last_error_info,
            std::make_error_code(std::errc::invalid_argument),
            "style id must not be empty when editing style run italic",
            std::string{styles_xml_entry});
        return false;
    }
    if (const auto error = this->ensure_styles_part_attached()) {
        return false;
    }
    auto style = find_style_node(this->styles.child("w:styles"), style_id);
    if (style == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id '" + std::string{style_id} +
                           "' was not found in word/styles.xml",
                       std::string{styles_xml_entry});
        return false;
    }
    const auto italic =
        ensure_run_italic_node(ensure_style_run_properties_node(style));
    if (italic == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to create w:i for style '" +
                           std::string{style_id} + "'",
                       std::string{styles_xml_entry});
        return false;
    }
    set_on_off_value(italic, enabled);
    this->styles_dirty = true;
    this->last_error_info.clear();
    return true;
}

bool Document::set_style_run_strikethrough(std::string_view style_id,
                                           bool enabled) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before editing style "
                       "run strikethrough",
                       std::string{styles_xml_entry});
        return false;
    }
    if (style_id.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id must not be empty when editing style run "
                       "strikethrough",
                       std::string{styles_xml_entry});
        return false;
    }
    if (const auto error = this->ensure_styles_part_attached()) {
        return false;
    }
    auto style = find_style_node(this->styles.child("w:styles"), style_id);
    if (style == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id '" + std::string{style_id} +
                           "' was not found in word/styles.xml",
                       std::string{styles_xml_entry});
        return false;
    }
    const auto strike =
        ensure_run_strikethrough_node(ensure_style_run_properties_node(style));
    if (strike == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to create w:strike for style '" +
                           std::string{style_id} + "'",
                       std::string{styles_xml_entry});
        return false;
    }
    set_on_off_value(strike, enabled);
    this->styles_dirty = true;
    this->last_error_info.clear();
    return true;
}

bool Document::set_style_run_underline(std::string_view style_id,
                                       bool enabled) {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before editing style run underline",
            std::string{styles_xml_entry});
        return false;
    }
    if (style_id.empty()) {
        set_last_error(
            this->last_error_info,
            std::make_error_code(std::errc::invalid_argument),
            "style id must not be empty when editing style run underline",
            std::string{styles_xml_entry});
        return false;
    }
    if (const auto error = this->ensure_styles_part_attached()) {
        return false;
    }
    auto style = find_style_node(this->styles.child("w:styles"), style_id);
    if (style == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id '" + std::string{style_id} +
                           "' was not found in word/styles.xml",
                       std::string{styles_xml_entry});
        return false;
    }
    const auto underline =
        ensure_run_underline_node(ensure_style_run_properties_node(style));
    if (underline == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to create w:u for style '" +
                           std::string{style_id} + "'",
                       std::string{styles_xml_entry});
        return false;
    }
    ensure_attribute_value(underline, "w:val", enabled ? "single" : "none");
    this->styles_dirty = true;
    this->last_error_info.clear();
    return true;
}

bool Document::set_style_run_superscript(std::string_view style_id,
                                         bool enabled) {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before editing style run "
            "superscript",
            std::string{styles_xml_entry});
        return false;
    }
    if (style_id.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id must not be empty when editing style run "
                       "superscript",
                       std::string{styles_xml_entry});
        return false;
    }
    if (const auto error = this->ensure_styles_part_attached()) {
        return false;
    }
    auto style = find_style_node(this->styles.child("w:styles"), style_id);
    if (style == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id '" + std::string{style_id} +
                           "' was not found in word/styles.xml",
                       std::string{styles_xml_entry});
        return false;
    }
    const auto vertical_align =
        ensure_run_vertical_align_node(ensure_style_run_properties_node(style));
    if (vertical_align == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to create w:vertAlign for style '" +
                           std::string{style_id} + "'",
                       std::string{styles_xml_entry});
        return false;
    }
    ensure_attribute_value(vertical_align, "w:val",
                           enabled ? "superscript" : "baseline");
    this->styles_dirty = true;
    this->last_error_info.clear();
    return true;
}

bool Document::set_style_run_subscript(std::string_view style_id,
                                       bool enabled) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before editing style "
                       "run subscript",
                       std::string{styles_xml_entry});
        return false;
    }
    if (style_id.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id must not be empty when editing style run "
                       "subscript",
                       std::string{styles_xml_entry});
        return false;
    }
    if (const auto error = this->ensure_styles_part_attached()) {
        return false;
    }
    auto style = find_style_node(this->styles.child("w:styles"), style_id);
    if (style == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id '" + std::string{style_id} +
                           "' was not found in word/styles.xml",
                       std::string{styles_xml_entry});
        return false;
    }
    const auto vertical_align =
        ensure_run_vertical_align_node(ensure_style_run_properties_node(style));
    if (vertical_align == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to create w:vertAlign for style '" +
                           std::string{style_id} + "'",
                       std::string{styles_xml_entry});
        return false;
    }
    ensure_attribute_value(vertical_align, "w:val",
                           enabled ? "subscript" : "baseline");
    this->styles_dirty = true;
    this->last_error_info.clear();
    return true;
}

bool Document::set_style_run_font_size_points(std::string_view style_id,
                                              double font_size_points) {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before editing style run font size",
            std::string{styles_xml_entry});
        return false;
    }
    if (style_id.empty() || font_size_points <= 0.0) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id must not be empty and style run font size "
                       "points must be greater than zero",
                       std::string{styles_xml_entry});
        return false;
    }
    if (const auto error = this->ensure_styles_part_attached()) {
        return false;
    }
    auto style = find_style_node(this->styles.child("w:styles"), style_id);
    if (style == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id '" + std::string{style_id} +
                           "' was not found in word/styles.xml",
                       std::string{styles_xml_entry});
        return false;
    }
    const auto run_properties = ensure_style_run_properties_node(style);
    const auto primary_size = ensure_run_font_size_node(run_properties, "w:sz");
    const auto complex_size =
        ensure_run_font_size_node(run_properties, "w:szCs");
    if (primary_size == pugi::xml_node{} || complex_size == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to create font size nodes for style '" +
                           std::string{style_id} + "'",
                       std::string{styles_xml_entry});
        return false;
    }
    const auto half_points =
        std::to_string(std::llround(font_size_points * 2.0));
    ensure_attribute_value(primary_size, "w:val", half_points);
    ensure_attribute_value(complex_size, "w:val", half_points);
    this->styles_dirty = true;
    this->last_error_info.clear();
    return true;
}

bool Document::set_style_run_font_family(std::string_view style_id,
                                         std::string_view font_family) {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before editing style run fonts");
        return false;
    }

    if (style_id.empty()) {
        set_last_error(
            this->last_error_info,
            std::make_error_code(std::errc::invalid_argument),
            "style id must not be empty when editing style run fonts",
            std::string{styles_xml_entry});
        return false;
    }

    if (font_family.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style run font family must not be empty",
                       std::string{styles_xml_entry});
        return false;
    }

    if (const auto error = this->ensure_styles_loaded()) {
        return false;
    }

    auto styles_root = this->styles.child("w:styles");
    if (styles_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       document_errc::styles_xml_parse_failed,
                       "word/styles.xml does not contain a w:styles root",
                       std::string{styles_xml_entry});
        return false;
    }

    auto style = find_style_node(styles_root, style_id);
    if (style == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id '" + std::string{style_id} +
                           "' was not found in word/styles.xml",
                       std::string{styles_xml_entry});
        return false;
    }

    if (!this->has_styles_part) {
        if (const auto error = this->ensure_styles_part_attached()) {
            return false;
        }
        styles_root = this->styles.child("w:styles");
        style = find_style_node(styles_root, style_id);
    }

    const auto run_properties = ensure_style_run_properties_node(style);
    if (run_properties == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to create w:rPr for style '" +
                           std::string{style_id} + "'",
                       std::string{styles_xml_entry});
        return false;
    }

    const auto run_fonts = ensure_run_fonts_node(run_properties);
    if (run_fonts == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to create w:rFonts for style '" +
                           std::string{style_id} + "'",
                       std::string{styles_xml_entry});
        return false;
    }

    ensure_attribute_value(run_fonts, "w:ascii", font_family);
    ensure_attribute_value(run_fonts, "w:hAnsi", font_family);
    ensure_attribute_value(run_fonts, "w:cs", font_family);
    this->styles_dirty = true;
    this->last_error_info.clear();
    return true;
}

bool Document::set_style_run_east_asia_font_family(
    std::string_view style_id, std::string_view font_family) {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before editing style run fonts");
        return false;
    }

    if (style_id.empty()) {
        set_last_error(
            this->last_error_info,
            std::make_error_code(std::errc::invalid_argument),
            "style id must not be empty when editing style run fonts",
            std::string{styles_xml_entry});
        return false;
    }

    if (font_family.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style eastAsia font family must not be empty",
                       std::string{styles_xml_entry});
        return false;
    }

    if (const auto error = this->ensure_styles_loaded()) {
        return false;
    }

    auto styles_root = this->styles.child("w:styles");
    if (styles_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       document_errc::styles_xml_parse_failed,
                       "word/styles.xml does not contain a w:styles root",
                       std::string{styles_xml_entry});
        return false;
    }

    auto style = find_style_node(styles_root, style_id);
    if (style == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id '" + std::string{style_id} +
                           "' was not found in word/styles.xml",
                       std::string{styles_xml_entry});
        return false;
    }

    if (!this->has_styles_part) {
        if (const auto error = this->ensure_styles_part_attached()) {
            return false;
        }
        styles_root = this->styles.child("w:styles");
        style = find_style_node(styles_root, style_id);
    }

    const auto run_properties = ensure_style_run_properties_node(style);
    if (run_properties == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to create w:rPr for style '" +
                           std::string{style_id} + "'",
                       std::string{styles_xml_entry});
        return false;
    }

    const auto run_fonts = ensure_run_fonts_node(run_properties);
    if (run_fonts == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to create w:rFonts for style '" +
                           std::string{style_id} + "'",
                       std::string{styles_xml_entry});
        return false;
    }

    ensure_attribute_value(run_fonts, "w:eastAsia", font_family);
    this->styles_dirty = true;
    this->last_error_info.clear();
    return true;
}

bool Document::set_style_run_language(std::string_view style_id,
                                      std::string_view language) {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before editing style run language",
            std::string{styles_xml_entry});
        return false;
    }

    if (style_id.empty()) {
        set_last_error(
            this->last_error_info,
            std::make_error_code(std::errc::invalid_argument),
            "style id must not be empty when editing style run language",
            std::string{styles_xml_entry});
        return false;
    }

    if (language.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style run language must not be empty",
                       std::string{styles_xml_entry});
        return false;
    }

    if (const auto error = this->ensure_styles_loaded()) {
        return false;
    }

    auto styles_root = this->styles.child("w:styles");
    if (styles_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       document_errc::styles_xml_parse_failed,
                       "word/styles.xml does not contain a w:styles root",
                       std::string{styles_xml_entry});
        return false;
    }

    auto style = find_style_node(styles_root, style_id);
    if (style == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id '" + std::string{style_id} +
                           "' was not found in word/styles.xml",
                       std::string{styles_xml_entry});
        return false;
    }

    if (!this->has_styles_part) {
        if (const auto error = this->ensure_styles_part_attached()) {
            return false;
        }
        styles_root = this->styles.child("w:styles");
        style = find_style_node(styles_root, style_id);
    }

    const auto run_properties = ensure_style_run_properties_node(style);
    if (run_properties == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to create w:rPr for style '" +
                           std::string{style_id} + "'",
                       std::string{styles_xml_entry});
        return false;
    }

    const auto run_language = ensure_run_language_node(run_properties);
    if (run_language == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to create w:lang for style '" +
                           std::string{style_id} + "'",
                       std::string{styles_xml_entry});
        return false;
    }

    ensure_attribute_value(run_language, "w:val", language);
    this->styles_dirty = true;
    this->last_error_info.clear();
    return true;
}

bool Document::set_style_run_east_asia_language(std::string_view style_id,
                                                std::string_view language) {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before editing style run language",
            std::string{styles_xml_entry});
        return false;
    }

    if (style_id.empty()) {
        set_last_error(
            this->last_error_info,
            std::make_error_code(std::errc::invalid_argument),
            "style id must not be empty when editing style run language",
            std::string{styles_xml_entry});
        return false;
    }

    if (language.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style eastAsia language must not be empty",
                       std::string{styles_xml_entry});
        return false;
    }

    if (const auto error = this->ensure_styles_loaded()) {
        return false;
    }

    auto styles_root = this->styles.child("w:styles");
    if (styles_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       document_errc::styles_xml_parse_failed,
                       "word/styles.xml does not contain a w:styles root",
                       std::string{styles_xml_entry});
        return false;
    }

    auto style = find_style_node(styles_root, style_id);
    if (style == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id '" + std::string{style_id} +
                           "' was not found in word/styles.xml",
                       std::string{styles_xml_entry});
        return false;
    }

    if (!this->has_styles_part) {
        if (const auto error = this->ensure_styles_part_attached()) {
            return false;
        }
        styles_root = this->styles.child("w:styles");
        style = find_style_node(styles_root, style_id);
    }

    const auto run_properties = ensure_style_run_properties_node(style);
    if (run_properties == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to create w:rPr for style '" +
                           std::string{style_id} + "'",
                       std::string{styles_xml_entry});
        return false;
    }

    const auto run_language = ensure_run_language_node(run_properties);
    if (run_language == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to create w:lang for style '" +
                           std::string{style_id} + "'",
                       std::string{styles_xml_entry});
        return false;
    }

    ensure_attribute_value(run_language, "w:eastAsia", language);
    this->styles_dirty = true;
    this->last_error_info.clear();
    return true;
}

bool Document::set_style_run_bidi_language(std::string_view style_id,
                                           std::string_view language) {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before editing style run language",
            std::string{styles_xml_entry});
        return false;
    }

    if (style_id.empty()) {
        set_last_error(
            this->last_error_info,
            std::make_error_code(std::errc::invalid_argument),
            "style id must not be empty when editing style run language",
            std::string{styles_xml_entry});
        return false;
    }

    if (language.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style bidi language must not be empty",
                       std::string{styles_xml_entry});
        return false;
    }

    if (const auto error = this->ensure_styles_loaded()) {
        return false;
    }

    auto styles_root = this->styles.child("w:styles");
    if (styles_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       document_errc::styles_xml_parse_failed,
                       "word/styles.xml does not contain a w:styles root",
                       std::string{styles_xml_entry});
        return false;
    }

    auto style = find_style_node(styles_root, style_id);
    if (style == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id '" + std::string{style_id} +
                           "' was not found in word/styles.xml",
                       std::string{styles_xml_entry});
        return false;
    }

    if (!this->has_styles_part) {
        if (const auto error = this->ensure_styles_part_attached()) {
            return false;
        }
        styles_root = this->styles.child("w:styles");
        style = find_style_node(styles_root, style_id);
    }

    const auto run_properties = ensure_style_run_properties_node(style);
    if (run_properties == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to create w:rPr for style '" +
                           std::string{style_id} + "'",
                       std::string{styles_xml_entry});
        return false;
    }

    const auto run_language = ensure_run_language_node(run_properties);
    if (run_language == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to create w:lang for style '" +
                           std::string{style_id} + "'",
                       std::string{styles_xml_entry});
        return false;
    }

    ensure_attribute_value(run_language, "w:bidi", language);
    this->styles_dirty = true;
    this->last_error_info.clear();
    return true;
}

bool Document::set_style_run_rtl(std::string_view style_id, bool enabled) {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before editing style run direction",
            std::string{styles_xml_entry});
        return false;
    }

    if (style_id.empty()) {
        set_last_error(
            this->last_error_info,
            std::make_error_code(std::errc::invalid_argument),
            "style id must not be empty when editing style run direction",
            std::string{styles_xml_entry});
        return false;
    }

    if (const auto error = this->ensure_styles_loaded()) {
        return false;
    }

    auto styles_root = this->styles.child("w:styles");
    if (styles_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       document_errc::styles_xml_parse_failed,
                       "word/styles.xml does not contain a w:styles root",
                       std::string{styles_xml_entry});
        return false;
    }

    auto style = find_style_node(styles_root, style_id);
    if (style == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id '" + std::string{style_id} +
                           "' was not found in word/styles.xml",
                       std::string{styles_xml_entry});
        return false;
    }

    if (!this->has_styles_part) {
        if (const auto error = this->ensure_styles_part_attached()) {
            return false;
        }
        styles_root = this->styles.child("w:styles");
        style = find_style_node(styles_root, style_id);
    }

    const auto run_properties = ensure_style_run_properties_node(style);
    if (run_properties == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to create w:rPr for style '" +
                           std::string{style_id} + "'",
                       std::string{styles_xml_entry});
        return false;
    }

    const auto rtl = ensure_run_rtl_node(run_properties);
    if (rtl == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to create w:rtl for style '" +
                           std::string{style_id} + "'",
                       std::string{styles_xml_entry});
        return false;
    }

    set_on_off_value(rtl, enabled);
    this->styles_dirty = true;
    this->last_error_info.clear();
    return true;
}

bool Document::set_style_paragraph_bidi(std::string_view style_id,
                                        bool enabled) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before editing style "
                       "paragraph direction",
                       std::string{styles_xml_entry});
        return false;
    }

    if (style_id.empty()) {
        set_last_error(
            this->last_error_info,
            std::make_error_code(std::errc::invalid_argument),
            "style id must not be empty when editing style paragraph direction",
            std::string{styles_xml_entry});
        return false;
    }

    if (const auto error = this->ensure_styles_loaded()) {
        return false;
    }

    auto styles_root = this->styles.child("w:styles");
    if (styles_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       document_errc::styles_xml_parse_failed,
                       "word/styles.xml does not contain a w:styles root",
                       std::string{styles_xml_entry});
        return false;
    }

    auto style = find_style_node(styles_root, style_id);
    if (style == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id '" + std::string{style_id} +
                           "' was not found in word/styles.xml",
                       std::string{styles_xml_entry});
        return false;
    }

    if (!this->has_styles_part) {
        if (const auto error = this->ensure_styles_part_attached()) {
            return false;
        }
        styles_root = this->styles.child("w:styles");
        style = find_style_node(styles_root, style_id);
    }

    const auto paragraph_properties =
        ensure_style_paragraph_properties_node(style);
    if (paragraph_properties == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to create w:pPr for style '" +
                           std::string{style_id} + "'",
                       std::string{styles_xml_entry});
        return false;
    }

    const auto bidi = ensure_paragraph_bidi_node(paragraph_properties);
    if (bidi == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to create w:bidi for style '" +
                           std::string{style_id} + "'",
                       std::string{styles_xml_entry});
        return false;
    }

    set_on_off_value(bidi, enabled);
    this->styles_dirty = true;
    this->last_error_info.clear();
    return true;
}

bool Document::clear_style_run_text_color(std::string_view style_id) {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before editing style run color",
            std::string{styles_xml_entry});
        return false;
    }
    if (style_id.empty()) {
        set_last_error(
            this->last_error_info,
            std::make_error_code(std::errc::invalid_argument),
            "style id must not be empty when editing style run color",
            std::string{styles_xml_entry});
        return false;
    }
    if (const auto error = this->ensure_styles_loaded()) {
        return false;
    }
    const auto styles_root = this->styles.child("w:styles");
    auto style = find_style_node(styles_root, style_id);
    if (style == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id '" + std::string{style_id} +
                           "' was not found in word/styles.xml",
                       std::string{styles_xml_entry});
        return false;
    }
    if (clear_run_text_color(style.child("w:rPr"))) {
        remove_empty_style_run_properties(style);
        this->styles_dirty = true;
    }
    this->last_error_info.clear();
    return true;
}

bool Document::clear_style_run_bold(std::string_view style_id) {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before editing style run bold",
            std::string{styles_xml_entry});
        return false;
    }
    if (style_id.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id must not be empty when editing style run bold",
                       std::string{styles_xml_entry});
        return false;
    }
    if (const auto error = this->ensure_styles_loaded()) {
        return false;
    }
    const auto styles_root = this->styles.child("w:styles");
    auto style = find_style_node(styles_root, style_id);
    if (style == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id '" + std::string{style_id} +
                           "' was not found in word/styles.xml",
                       std::string{styles_xml_entry});
        return false;
    }
    if (clear_run_bold(style.child("w:rPr"))) {
        remove_empty_style_run_properties(style);
        this->styles_dirty = true;
    }
    this->last_error_info.clear();
    return true;
}

bool Document::clear_style_run_italic(std::string_view style_id) {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before editing style run italic",
            std::string{styles_xml_entry});
        return false;
    }
    if (style_id.empty()) {
        set_last_error(
            this->last_error_info,
            std::make_error_code(std::errc::invalid_argument),
            "style id must not be empty when editing style run italic",
            std::string{styles_xml_entry});
        return false;
    }
    if (const auto error = this->ensure_styles_loaded()) {
        return false;
    }
    const auto styles_root = this->styles.child("w:styles");
    auto style = find_style_node(styles_root, style_id);
    if (style == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id '" + std::string{style_id} +
                           "' was not found in word/styles.xml",
                       std::string{styles_xml_entry});
        return false;
    }
    if (clear_run_italic(style.child("w:rPr"))) {
        remove_empty_style_run_properties(style);
        this->styles_dirty = true;
    }
    this->last_error_info.clear();
    return true;
}

bool Document::clear_style_run_strikethrough(std::string_view style_id) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before editing style "
                       "run strikethrough",
                       std::string{styles_xml_entry});
        return false;
    }
    if (style_id.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id must not be empty when editing style run "
                       "strikethrough",
                       std::string{styles_xml_entry});
        return false;
    }
    if (const auto error = this->ensure_styles_loaded()) {
        return false;
    }
    const auto styles_root = this->styles.child("w:styles");
    auto style = find_style_node(styles_root, style_id);
    if (style == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id '" + std::string{style_id} +
                           "' was not found in word/styles.xml",
                       std::string{styles_xml_entry});
        return false;
    }
    if (clear_run_strikethrough(style.child("w:rPr"))) {
        remove_empty_style_run_properties(style);
        this->styles_dirty = true;
    }
    this->last_error_info.clear();
    return true;
}

bool Document::clear_style_run_underline(std::string_view style_id) {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before editing style run underline",
            std::string{styles_xml_entry});
        return false;
    }
    if (style_id.empty()) {
        set_last_error(
            this->last_error_info,
            std::make_error_code(std::errc::invalid_argument),
            "style id must not be empty when editing style run underline",
            std::string{styles_xml_entry});
        return false;
    }
    if (const auto error = this->ensure_styles_loaded()) {
        return false;
    }
    const auto styles_root = this->styles.child("w:styles");
    auto style = find_style_node(styles_root, style_id);
    if (style == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id '" + std::string{style_id} +
                           "' was not found in word/styles.xml",
                       std::string{styles_xml_entry});
        return false;
    }
    if (clear_run_underline(style.child("w:rPr"))) {
        remove_empty_style_run_properties(style);
        this->styles_dirty = true;
    }
    this->last_error_info.clear();
    return true;
}

bool Document::clear_style_run_superscript(std::string_view style_id) {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before editing style run "
            "superscript",
            std::string{styles_xml_entry});
        return false;
    }
    if (style_id.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id must not be empty when editing style run "
                       "superscript",
                       std::string{styles_xml_entry});
        return false;
    }
    if (const auto error = this->ensure_styles_loaded()) {
        return false;
    }
    const auto styles_root = this->styles.child("w:styles");
    auto style = find_style_node(styles_root, style_id);
    if (style == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id '" + std::string{style_id} +
                           "' was not found in word/styles.xml",
                       std::string{styles_xml_entry});
        return false;
    }
    if (clear_run_vertical_align(style.child("w:rPr"))) {
        remove_empty_style_run_properties(style);
        this->styles_dirty = true;
    }
    this->last_error_info.clear();
    return true;
}

bool Document::clear_style_run_subscript(std::string_view style_id) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before editing style "
                       "run subscript",
                       std::string{styles_xml_entry});
        return false;
    }
    if (style_id.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id must not be empty when editing style run "
                       "subscript",
                       std::string{styles_xml_entry});
        return false;
    }
    if (const auto error = this->ensure_styles_loaded()) {
        return false;
    }
    const auto styles_root = this->styles.child("w:styles");
    auto style = find_style_node(styles_root, style_id);
    if (style == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id '" + std::string{style_id} +
                           "' was not found in word/styles.xml",
                       std::string{styles_xml_entry});
        return false;
    }
    if (clear_run_vertical_align(style.child("w:rPr"))) {
        remove_empty_style_run_properties(style);
        this->styles_dirty = true;
    }
    this->last_error_info.clear();
    return true;
}

bool Document::clear_style_run_font_size_points(std::string_view style_id) {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before editing style run font size",
            std::string{styles_xml_entry});
        return false;
    }
    if (style_id.empty()) {
        set_last_error(
            this->last_error_info,
            std::make_error_code(std::errc::invalid_argument),
            "style id must not be empty when editing style run font size",
            std::string{styles_xml_entry});
        return false;
    }
    if (const auto error = this->ensure_styles_loaded()) {
        return false;
    }
    const auto styles_root = this->styles.child("w:styles");
    auto style = find_style_node(styles_root, style_id);
    if (style == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id '" + std::string{style_id} +
                           "' was not found in word/styles.xml",
                       std::string{styles_xml_entry});
        return false;
    }
    if (clear_run_font_size_points(style.child("w:rPr"))) {
        remove_empty_style_run_properties(style);
        this->styles_dirty = true;
    }
    this->last_error_info.clear();
    return true;
}

bool Document::clear_style_run_font_family(std::string_view style_id) {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before editing style run fonts");
        return false;
    }

    if (style_id.empty()) {
        set_last_error(
            this->last_error_info,
            std::make_error_code(std::errc::invalid_argument),
            "style id must not be empty when editing style run fonts",
            std::string{styles_xml_entry});
        return false;
    }

    if (const auto error = this->ensure_styles_loaded()) {
        return false;
    }

    const auto styles_root = this->styles.child("w:styles");
    if (styles_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       document_errc::styles_xml_parse_failed,
                       "word/styles.xml does not contain a w:styles root",
                       std::string{styles_xml_entry});
        return false;
    }

    auto style = find_style_node(styles_root, style_id);
    if (style == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id '" + std::string{style_id} +
                           "' was not found in word/styles.xml",
                       std::string{styles_xml_entry});
        return false;
    }

    auto run_properties = style.child("w:rPr");
    if (clear_font_family_attributes(run_properties)) {
        if (run_properties.first_child() == pugi::xml_node{} &&
            !node_has_attributes(run_properties)) {
            style.remove_child(run_properties);
        }
        this->styles_dirty = true;
    }

    this->last_error_info.clear();
    return true;
}

bool Document::clear_style_run_east_asia_font_family(
    std::string_view style_id) {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before editing style run fonts");
        return false;
    }

    if (style_id.empty()) {
        set_last_error(
            this->last_error_info,
            std::make_error_code(std::errc::invalid_argument),
            "style id must not be empty when editing style run fonts",
            std::string{styles_xml_entry});
        return false;
    }

    if (const auto error = this->ensure_styles_loaded()) {
        return false;
    }

    const auto styles_root = this->styles.child("w:styles");
    if (styles_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       document_errc::styles_xml_parse_failed,
                       "word/styles.xml does not contain a w:styles root",
                       std::string{styles_xml_entry});
        return false;
    }

    auto style = find_style_node(styles_root, style_id);
    if (style == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id '" + std::string{style_id} +
                           "' was not found in word/styles.xml",
                       std::string{styles_xml_entry});
        return false;
    }

    auto run_properties = style.child("w:rPr");
    if (clear_font_family_attribute(run_properties, "w:eastAsia")) {
        if (run_properties.first_child() == pugi::xml_node{} &&
            !node_has_attributes(run_properties)) {
            style.remove_child(run_properties);
        }
        this->styles_dirty = true;
    }

    this->last_error_info.clear();
    return true;
}

bool Document::clear_style_run_primary_language(std::string_view style_id) {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before editing style run language",
            std::string{styles_xml_entry});
        return false;
    }

    if (style_id.empty()) {
        set_last_error(
            this->last_error_info,
            std::make_error_code(std::errc::invalid_argument),
            "style id must not be empty when editing style run language",
            std::string{styles_xml_entry});
        return false;
    }

    if (const auto error = this->ensure_styles_loaded()) {
        return false;
    }

    const auto styles_root = this->styles.child("w:styles");
    if (styles_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       document_errc::styles_xml_parse_failed,
                       "word/styles.xml does not contain a w:styles root",
                       std::string{styles_xml_entry});
        return false;
    }

    auto style = find_style_node(styles_root, style_id);
    if (style == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id '" + std::string{style_id} +
                           "' was not found in word/styles.xml",
                       std::string{styles_xml_entry});
        return false;
    }

    auto run_properties = style.child("w:rPr");
    if (clear_language_attribute(run_properties, "w:val")) {
        if (run_properties.first_child() == pugi::xml_node{} &&
            !node_has_attributes(run_properties)) {
            style.remove_child(run_properties);
        }
        this->styles_dirty = true;
    }

    this->last_error_info.clear();
    return true;
}

bool Document::clear_style_run_language(std::string_view style_id) {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before editing style run language",
            std::string{styles_xml_entry});
        return false;
    }

    if (style_id.empty()) {
        set_last_error(
            this->last_error_info,
            std::make_error_code(std::errc::invalid_argument),
            "style id must not be empty when editing style run language",
            std::string{styles_xml_entry});
        return false;
    }

    if (const auto error = this->ensure_styles_loaded()) {
        return false;
    }

    const auto styles_root = this->styles.child("w:styles");
    if (styles_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       document_errc::styles_xml_parse_failed,
                       "word/styles.xml does not contain a w:styles root",
                       std::string{styles_xml_entry});
        return false;
    }

    auto style = find_style_node(styles_root, style_id);
    if (style == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id '" + std::string{style_id} +
                           "' was not found in word/styles.xml",
                       std::string{styles_xml_entry});
        return false;
    }

    auto run_properties = style.child("w:rPr");
    if (clear_language_attributes(run_properties)) {
        if (run_properties.first_child() == pugi::xml_node{} &&
            !node_has_attributes(run_properties)) {
            style.remove_child(run_properties);
        }
        this->styles_dirty = true;
    }

    this->last_error_info.clear();
    return true;
}

bool Document::clear_style_run_east_asia_language(std::string_view style_id) {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before editing style run language",
            std::string{styles_xml_entry});
        return false;
    }

    if (style_id.empty()) {
        set_last_error(
            this->last_error_info,
            std::make_error_code(std::errc::invalid_argument),
            "style id must not be empty when editing style run language",
            std::string{styles_xml_entry});
        return false;
    }

    if (const auto error = this->ensure_styles_loaded()) {
        return false;
    }

    const auto styles_root = this->styles.child("w:styles");
    if (styles_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       document_errc::styles_xml_parse_failed,
                       "word/styles.xml does not contain a w:styles root",
                       std::string{styles_xml_entry});
        return false;
    }

    auto style = find_style_node(styles_root, style_id);
    if (style == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id '" + std::string{style_id} +
                           "' was not found in word/styles.xml",
                       std::string{styles_xml_entry});
        return false;
    }

    auto run_properties = style.child("w:rPr");
    if (clear_language_attribute(run_properties, "w:eastAsia")) {
        if (run_properties.first_child() == pugi::xml_node{} &&
            !node_has_attributes(run_properties)) {
            style.remove_child(run_properties);
        }
        this->styles_dirty = true;
    }

    this->last_error_info.clear();
    return true;
}

bool Document::clear_style_run_bidi_language(std::string_view style_id) {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before editing style run language",
            std::string{styles_xml_entry});
        return false;
    }

    if (style_id.empty()) {
        set_last_error(
            this->last_error_info,
            std::make_error_code(std::errc::invalid_argument),
            "style id must not be empty when editing style run language",
            std::string{styles_xml_entry});
        return false;
    }

    if (const auto error = this->ensure_styles_loaded()) {
        return false;
    }

    const auto styles_root = this->styles.child("w:styles");
    if (styles_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       document_errc::styles_xml_parse_failed,
                       "word/styles.xml does not contain a w:styles root",
                       std::string{styles_xml_entry});
        return false;
    }

    auto style = find_style_node(styles_root, style_id);
    if (style == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id '" + std::string{style_id} +
                           "' was not found in word/styles.xml",
                       std::string{styles_xml_entry});
        return false;
    }

    auto run_properties = style.child("w:rPr");
    if (clear_language_attribute(run_properties, "w:bidi")) {
        if (run_properties.first_child() == pugi::xml_node{} &&
            !node_has_attributes(run_properties)) {
            style.remove_child(run_properties);
        }
        this->styles_dirty = true;
    }

    this->last_error_info.clear();
    return true;
}

bool Document::clear_style_run_rtl(std::string_view style_id) {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before editing style run direction",
            std::string{styles_xml_entry});
        return false;
    }

    if (style_id.empty()) {
        set_last_error(
            this->last_error_info,
            std::make_error_code(std::errc::invalid_argument),
            "style id must not be empty when editing style run direction",
            std::string{styles_xml_entry});
        return false;
    }

    if (const auto error = this->ensure_styles_loaded()) {
        return false;
    }

    const auto styles_root = this->styles.child("w:styles");
    if (styles_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       document_errc::styles_xml_parse_failed,
                       "word/styles.xml does not contain a w:styles root",
                       std::string{styles_xml_entry});
        return false;
    }

    auto style = find_style_node(styles_root, style_id);
    if (style == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id '" + std::string{style_id} +
                           "' was not found in word/styles.xml",
                       std::string{styles_xml_entry});
        return false;
    }

    auto run_properties = style.child("w:rPr");
    if (clear_on_off_child(run_properties, "w:rtl")) {
        if (run_properties.first_child() == pugi::xml_node{} &&
            !node_has_attributes(run_properties)) {
            style.remove_child(run_properties);
        }
        this->styles_dirty = true;
    }

    this->last_error_info.clear();
    return true;
}

bool Document::clear_style_paragraph_bidi(std::string_view style_id) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before editing style "
                       "paragraph direction",
                       std::string{styles_xml_entry});
        return false;
    }

    if (style_id.empty()) {
        set_last_error(
            this->last_error_info,
            std::make_error_code(std::errc::invalid_argument),
            "style id must not be empty when editing style paragraph direction",
            std::string{styles_xml_entry});
        return false;
    }

    if (const auto error = this->ensure_styles_loaded()) {
        return false;
    }

    const auto styles_root = this->styles.child("w:styles");
    if (styles_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       document_errc::styles_xml_parse_failed,
                       "word/styles.xml does not contain a w:styles root",
                       std::string{styles_xml_entry});
        return false;
    }

    auto style = find_style_node(styles_root, style_id);
    if (style == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id '" + std::string{style_id} +
                           "' was not found in word/styles.xml",
                       std::string{styles_xml_entry});
        return false;
    }

    auto paragraph_properties = style.child("w:pPr");
    if (clear_on_off_child(paragraph_properties, "w:bidi")) {
        if (paragraph_properties.first_child() == pugi::xml_node{} &&
            !node_has_attributes(paragraph_properties)) {
            style.remove_child(paragraph_properties);
        }
        this->styles_dirty = true;
    }

    this->last_error_info.clear();
    return true;
}

bool Document::set_paragraph_style(Paragraph paragraph_handle,
                                   std::string_view style_id) {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
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

    if (!paragraph_handle.has_next() ||
        paragraph_handle.current == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "target paragraph handle is not valid",
                       std::string{document_xml_entry});
        return false;
    }

    if (const auto error = this->ensure_styles_part_attached()) {
        return false;
    }

    auto paragraph_properties =
        ensure_paragraph_properties_node(paragraph_handle.current);
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
            style_node = paragraph_properties.insert_child_before("w:pStyle",
                                                                  first_child);
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
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before editing paragraph styles");
        return false;
    }

    if (!paragraph_handle.has_next() ||
        paragraph_handle.current == pugi::xml_node{}) {
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
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
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
            style_node =
                run_properties.insert_child_before("w:rStyle", first_child);
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
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
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

std::vector<featherdoc::table_inspection_summary> Document::inspect_tables() {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before inspecting tables",
                       std::string{document_xml_entry});
        return {};
    }

    auto summaries = std::vector<featherdoc::table_inspection_summary>{};
    auto table_handle = this->tables();
    for (std::size_t table_index = 0U; table_handle.has_next();
         ++table_index, table_handle.next()) {
        summaries.push_back(summarize_table_handle(table_handle, table_index));
    }

    this->last_error_info.clear();
    return summaries;
}

std::optional<featherdoc::table_inspection_summary>
Document::inspect_table(std::size_t table_index) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before inspecting tables",
                       std::string{document_xml_entry});
        return std::nullopt;
    }

    auto table_handle = this->tables();
    for (std::size_t current_index = 0U;
         current_index < table_index && table_handle.has_next();
         ++current_index) {
        table_handle.next();
    }

    if (!table_handle.has_next()) {
        this->last_error_info.clear();
        return std::nullopt;
    }

    auto summary = summarize_table_handle(table_handle, table_index);
    this->last_error_info.clear();
    return summary;
}

std::vector<featherdoc::body_block_inspection_summary>
Document::inspect_body_blocks() {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before inspecting body blocks",
            std::string{document_xml_entry});
        return {};
    }

    const auto body = this->document.child("w:document").child("w:body");
    if (body == pugi::xml_node{}) {
        set_last_error(
            this->last_error_info, document_errc::document_xml_parse_failed,
            "word/document.xml does not contain a w:document/w:body root",
            std::string{document_xml_entry});
        return {};
    }

    auto summaries = std::vector<featherdoc::body_block_inspection_summary>{};
    auto paragraph_index = std::size_t{0U};
    auto table_index = std::size_t{0U};
    auto block_index = std::size_t{0U};
    auto section_index = std::size_t{0U};
    for (auto child = body.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        const auto child_name = std::string_view{child.name()};
        if (child_name == "w:p") {
            summaries.push_back(featherdoc::body_block_inspection_summary{
                featherdoc::body_block_kind::paragraph, block_index++,
                paragraph_index++, section_index});
            if (child.child("w:pPr").child("w:sectPr") != pugi::xml_node{}) {
                ++section_index;
            }
        } else if (child_name == "w:tbl") {
            summaries.push_back(featherdoc::body_block_inspection_summary{
                featherdoc::body_block_kind::table, block_index++,
                table_index++, section_index});
        }
    }

    this->last_error_info.clear();
    return summaries;
}

table_style_look_consistency_report
Document::check_table_style_look_consistency() {
    auto report = table_style_look_consistency_report{};
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before checking table style look",
            std::string{document_xml_entry});
        return report;
    }

    if (const auto error = this->ensure_styles_loaded()) {
        return report;
    }

    const auto styles_root = this->styles.child("w:styles");
    if (styles_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       document_errc::styles_xml_parse_failed,
                       "word/styles.xml does not contain a w:styles root",
                       std::string{styles_xml_entry});
        return report;
    }

    auto add_issue = [&](std::size_t table_index, const std::string &style_id,
                         std::string issue_type, std::string region,
                         std::string required_flag,
                         std::optional<bool> actual_value,
                         std::string suggestion) {
        auto issue = table_style_look_consistency_issue{};
        issue.table_index = table_index;
        issue.style_id = style_id;
        issue.issue_type = std::move(issue_type);
        issue.region = std::move(region);
        issue.required_style_look_flag = std::move(required_flag);
        issue.actual_value = actual_value;
        issue.expected_value = true;
        issue.suggestion = std::move(suggestion);
        report.issues.push_back(std::move(issue));
    };

    auto required_flag_value = [](const table_style_look &style_look,
                                  std::string_view flag_name) -> bool {
        if (flag_name == "first_row") {
            return style_look.first_row;
        }
        if (flag_name == "last_row") {
            return style_look.last_row;
        }
        if (flag_name == "first_column") {
            return style_look.first_column;
        }
        if (flag_name == "last_column") {
            return style_look.last_column;
        }
        if (flag_name == "banded_rows") {
            return style_look.banded_rows;
        }
        if (flag_name == "banded_columns") {
            return style_look.banded_columns;
        }
        return false;
    };

    auto table_handle = this->tables();
    for (std::size_t table_index = 0U; table_handle.has_next();
         ++table_index, table_handle.next()) {
        ++report.table_count;
        const auto style_id = table_handle.style_id();
        if (!style_id.has_value() || style_id->empty()) {
            continue;
        }

        const auto style = find_style_node(styles_root, *style_id);
        if (style == pugi::xml_node{}) {
            add_issue(table_index, *style_id, "missing_style", {}, {},
                      std::nullopt,
                      "create table style '" + *style_id +
                          "' or change this table to an existing table style");
            continue;
        }

        const auto style_summary = summarize_style_node(style);
        if (style_summary.kind != style_kind::table) {
            add_issue(table_index, *style_id, "style_not_table", {}, {},
                      std::nullopt,
                      "replace style id '" + *style_id +
                          "' with a table style before relying on tblLook");
            continue;
        }

        const auto style_look = table_handle.style_look();
        auto check_region = [&](std::string_view xml_region,
                                std::string region_name,
                                std::string flag_name) {
            if (detail::find_table_style_conditional_node(style, xml_region) ==
                pugi::xml_node{}) {
                return;
            }

            auto actual_value = std::optional<bool>{};
            if (style_look.has_value()) {
                actual_value = required_flag_value(*style_look, flag_name);
            }
            if (actual_value.value_or(false)) {
                return;
            }

            const auto issue_type = actual_value.has_value()
                                        ? "style_look_disabled"
                                        : "style_look_missing";
            add_issue(table_index, *style_id, issue_type,
                      std::move(region_name), std::move(flag_name),
                      actual_value,
                      "enable the required tblLook flag for this table");
        };

        check_region("firstRow", "first_row", "first_row");
        check_region("lastRow", "last_row", "last_row");
        check_region("firstCol", "first_column", "first_column");
        check_region("lastCol", "last_column", "last_column");
        check_region("band1Horz", "banded_rows", "banded_rows");
        check_region("band2Horz", "second_banded_rows", "banded_rows");
        check_region("band1Vert", "banded_columns", "banded_columns");
        check_region("band2Vert", "second_banded_columns", "banded_columns");
    }

    this->last_error_info.clear();
    return report;
}

table_style_look_repair_report Document::repair_table_style_look_consistency() {
    auto repair = table_style_look_repair_report{};
    repair.before = this->check_table_style_look_consistency();
    if (this->last_error_info.code) {
        return repair;
    }

    auto required_flags_by_table =
        std::unordered_map<std::size_t, std::vector<std::string>>{};
    for (const auto &issue : repair.before.issues) {
        if ((issue.issue_type != "style_look_missing" &&
             issue.issue_type != "style_look_disabled") ||
            issue.required_style_look_flag.empty()) {
            continue;
        }
        required_flags_by_table[issue.table_index].push_back(
            issue.required_style_look_flag);
    }

    auto enable_required_flag = [](table_style_look &style_look,
                                   std::string_view flag_name) -> bool {
        auto changed = false;
        auto enable_flag = [&changed](bool &value) {
            if (!value) {
                value = true;
                changed = true;
            }
        };

        if (flag_name == "first_row") {
            enable_flag(style_look.first_row);
        } else if (flag_name == "last_row") {
            enable_flag(style_look.last_row);
        } else if (flag_name == "first_column") {
            enable_flag(style_look.first_column);
        } else if (flag_name == "last_column") {
            enable_flag(style_look.last_column);
        } else if (flag_name == "banded_rows") {
            enable_flag(style_look.banded_rows);
        } else if (flag_name == "banded_columns") {
            enable_flag(style_look.banded_columns);
        }

        return changed;
    };

    auto table_handle = this->tables();
    for (std::size_t table_index = 0U; table_handle.has_next();
         ++table_index, table_handle.next()) {
        const auto required_flags = required_flags_by_table.find(table_index);
        if (required_flags == required_flags_by_table.end()) {
            continue;
        }

        const auto existing_style_look = table_handle.style_look();
        auto style_look = existing_style_look.value_or(table_style_look{});
        auto changed = !existing_style_look.has_value();
        for (const auto &flag_name : required_flags->second) {
            changed = enable_required_flag(style_look, flag_name) || changed;
        }
        if (!changed) {
            continue;
        }
        if (!table_handle.set_style_look(style_look)) {
            set_last_error(this->last_error_info,
                           document_errc::document_xml_parse_failed,
                           "failed to update table style look flags",
                           std::string{document_xml_entry});
            return repair;
        }
        ++repair.changed_table_count;
    }

    repair.after = this->check_table_style_look_consistency();
    if (this->last_error_info.code) {
        return repair;
    }

    this->last_error_info.clear();
    return repair;
}

table_style_quality_audit_report Document::audit_table_style_quality() {
    auto report = table_style_quality_audit_report{};
    report.region_audit = this->audit_table_style_regions();
    if (this->last_error_info.code) {
        return report;
    }

    report.inheritance_audit = this->audit_table_style_inheritance();
    if (this->last_error_info.code) {
        return report;
    }

    report.style_look = this->check_table_style_look_consistency();
    if (this->last_error_info.code) {
        return report;
    }

    this->last_error_info.clear();
    return report;
}

table_style_quality_fix_plan Document::plan_table_style_quality_fixes() {
    auto plan = table_style_quality_fix_plan{};
    plan.audit = this->audit_table_style_quality();
    if (this->last_error_info.code) {
        return plan;
    }

    for (const auto &issue : plan.audit.region_audit.issues) {
        auto item = table_style_quality_fix_item{};
        item.source = "region_audit";
        item.issue_type = issue.issue_type;
        item.style_id = issue.style_id;
        item.style_name = issue.style_name;
        item.region = issue.region;
        item.action = "edit_table_style_region";
        item.automatic = false;
        item.suggestion = issue.suggestion;
        plan.items.push_back(std::move(item));
    }

    for (const auto &issue : plan.audit.inheritance_audit.issues) {
        auto item = table_style_quality_fix_item{};
        item.source = "inheritance_audit";
        item.issue_type = issue.issue_type;
        item.style_id = issue.style_id;
        item.style_name = issue.style_name;
        item.action = issue.issue_type == "inheritance_cycle"
                          ? "break_based_on_cycle"
                          : (issue.issue_type == "based_on_not_table"
                                 ? "rebase_table_style"
                                 : "create_or_clear_based_on");
        item.automatic = false;
        item.suggestion = issue.suggestion;
        plan.items.push_back(std::move(item));
    }

    for (const auto &issue : plan.audit.style_look.issues) {
        auto item = table_style_quality_fix_item{};
        item.source = "style_look";
        item.issue_type = issue.issue_type;
        item.table_index = issue.table_index;
        item.style_id = issue.style_id;
        item.region = issue.region;
        item.automatic = (issue.issue_type == "style_look_missing" ||
                          issue.issue_type == "style_look_disabled") &&
                         !issue.required_style_look_flag.empty();
        item.action = item.automatic ? "repair_table_style_look"
                                     : "fix_table_style_reference";
        item.command =
            item.automatic ? "repair-table-style-look --plan-only" : "";
        item.suggestion = issue.suggestion;
        plan.items.push_back(std::move(item));
    }

    this->last_error_info.clear();
    return plan;
}

std::vector<featherdoc::table_cell_inspection_summary>
Document::inspect_table_cells(std::size_t table_index) {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before inspecting table cells",
            std::string{document_xml_entry});
        return {};
    }

    auto table_handle = this->tables();
    for (std::size_t current_index = 0U;
         current_index < table_index && table_handle.has_next();
         ++current_index) {
        table_handle.next();
    }

    if (!table_handle.has_next()) {
        this->last_error_info.clear();
        return {};
    }

    auto summaries = collect_table_cell_summaries(table_handle);
    this->last_error_info.clear();
    return summaries;
}

std::optional<featherdoc::table_cell_inspection_summary>
Document::inspect_table_cell(std::size_t table_index, std::size_t row_index,
                             std::size_t cell_index) {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before inspecting table cells",
            std::string{document_xml_entry});
        return std::nullopt;
    }

    const auto cells = this->inspect_table_cells(table_index);
    for (const auto &cell : cells) {
        if (cell.row_index == row_index && cell.cell_index == cell_index) {
            this->last_error_info.clear();
            return cell;
        }
    }

    this->last_error_info.clear();
    return std::nullopt;
}

std::optional<featherdoc::table_cell_inspection_summary>
Document::inspect_table_cell_by_grid_column(std::size_t table_index,
                                            std::size_t row_index,
                                            std::size_t grid_column) {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before inspecting table cells",
            std::string{document_xml_entry});
        return std::nullopt;
    }

    const auto cells = this->inspect_table_cells(table_index);
    for (const auto &cell : cells) {
        if (cell.row_index == row_index && grid_column >= cell.column_index &&
            grid_column - cell.column_index < cell.column_span) {
            this->last_error_info.clear();
            return cell;
        }
    }

    this->last_error_info.clear();
    return std::nullopt;
}

std::vector<featherdoc::paragraph_inspection_summary>
Document::inspect_paragraphs() {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before inspecting paragraphs",
            std::string{document_xml_entry});
        return {};
    }

    auto summaries = std::vector<featherdoc::paragraph_inspection_summary>{};
    auto paragraph_handle = this->paragraphs();
    auto numbering_lookup_resolved = false;
    auto numbering_lookup_ready = false;
    for (std::size_t paragraph_index = 0U; paragraph_handle.has_next();
         ++paragraph_index, paragraph_handle.next()) {
        auto summary =
            summarize_paragraph_node(paragraph_handle.current, paragraph_index);
        if (summary.numbering.has_value() &&
            summary.numbering->num_id.has_value()) {
            if (!numbering_lookup_resolved) {
                numbering_lookup_ready =
                    !this->ensure_numbering_loaded() &&
                    this->numbering.child("w:numbering") != pugi::xml_node{};
                numbering_lookup_resolved = true;
            }

            if (numbering_lookup_ready) {
                const auto lookup =
                    this->find_numbering_instance(*summary.numbering->num_id);
                if (lookup.has_value()) {
                    summary.numbering->definition_id = lookup->definition_id;
                    if (!lookup->definition_name.empty()) {
                        summary.numbering->definition_name =
                            lookup->definition_name;
                    }
                }
            }
        }

        summaries.push_back(std::move(summary));
    }

    this->last_error_info.clear();
    return summaries;
}

std::optional<featherdoc::paragraph_inspection_summary>
Document::inspect_paragraph(std::size_t paragraph_index) {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before inspecting paragraphs",
            std::string{document_xml_entry});
        return std::nullopt;
    }

    auto paragraph_handle = this->paragraphs();
    for (std::size_t current_index = 0U;
         current_index < paragraph_index && paragraph_handle.has_next();
         ++current_index) {
        paragraph_handle.next();
    }

    if (!paragraph_handle.has_next()) {
        this->last_error_info.clear();
        return std::nullopt;
    }

    auto summary =
        summarize_paragraph_node(paragraph_handle.current, paragraph_index);
    if (summary.numbering.has_value() &&
        summary.numbering->num_id.has_value()) {
        const auto numbering_lookup_ready =
            !this->ensure_numbering_loaded() &&
            this->numbering.child("w:numbering") != pugi::xml_node{};
        if (numbering_lookup_ready) {
            const auto lookup =
                this->find_numbering_instance(*summary.numbering->num_id);
            if (lookup.has_value()) {
                summary.numbering->definition_id = lookup->definition_id;
                if (!lookup->definition_name.empty()) {
                    summary.numbering->definition_name =
                        lookup->definition_name;
                }
            }
        }
    }

    this->last_error_info.clear();
    return summary;
}

std::vector<featherdoc::run_inspection_summary>
Document::inspect_paragraph_runs(std::size_t paragraph_index) {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before inspecting paragraph runs",
            std::string{document_xml_entry});
        return {};
    }

    auto paragraph_handle = this->paragraphs();
    for (std::size_t current_index = 0U;
         current_index < paragraph_index && paragraph_handle.has_next();
         ++current_index) {
        paragraph_handle.next();
    }

    if (!paragraph_handle.has_next()) {
        this->last_error_info.clear();
        return {};
    }

    auto summaries = std::vector<featherdoc::run_inspection_summary>{};
    auto run = paragraph_handle.runs();
    for (std::size_t run_index = 0U; run.has_next(); ++run_index, run.next()) {
        summaries.push_back(summarize_run_handle(run, run_index));
    }

    this->last_error_info.clear();
    return summaries;
}

std::optional<featherdoc::run_inspection_summary>
Document::inspect_paragraph_run(std::size_t paragraph_index,
                                std::size_t run_index) {
    const auto runs = this->inspect_paragraph_runs(paragraph_index);
    if (run_index >= runs.size()) {
        this->last_error_info.clear();
        return std::nullopt;
    }

    return runs[run_index];
}

} // namespace featherdoc
