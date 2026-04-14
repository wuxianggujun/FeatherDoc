#include "featherdoc.hpp"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
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

auto read_font_family_attribute(pugi::xml_node run_fonts, const char *attribute_name)
    -> std::optional<std::string> {
    if (run_fonts == pugi::xml_node{}) {
        return std::nullopt;
    }

    const auto attribute = run_fonts.attribute(attribute_name);
    if (attribute == pugi::xml_attribute{} || attribute.value()[0] == '\0') {
        return std::nullopt;
    }

    return std::string{attribute.value()};
}

auto read_language_attribute(pugi::xml_node run_language, const char *attribute_name)
    -> std::optional<std::string> {
    if (run_language == pugi::xml_node{}) {
        return std::nullopt;
    }

    const auto attribute = run_language.attribute(attribute_name);
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

auto read_primary_font_family(pugi::xml_node run_properties) -> std::optional<std::string> {
    const auto run_fonts = run_properties.child("w:rFonts");
    if (const auto ascii_font = read_font_family_attribute(run_fonts, "w:ascii")) {
        return ascii_font;
    }
    if (const auto hansi_font = read_font_family_attribute(run_fonts, "w:hAnsi")) {
        return hansi_font;
    }
    return read_font_family_attribute(run_fonts, "w:cs");
}

auto read_east_asia_font_family(pugi::xml_node run_properties) -> std::optional<std::string> {
    return read_font_family_attribute(run_properties.child("w:rFonts"), "w:eastAsia");
}

auto read_primary_language(pugi::xml_node run_properties) -> std::optional<std::string> {
    return read_language_attribute(run_properties.child("w:lang"), "w:val");
}

auto read_east_asia_language(pugi::xml_node run_properties) -> std::optional<std::string> {
    return read_language_attribute(run_properties.child("w:lang"), "w:eastAsia");
}

auto read_bidi_language(pugi::xml_node run_properties) -> std::optional<std::string> {
    return read_language_attribute(run_properties.child("w:lang"), "w:bidi");
}

auto read_run_rtl(pugi::xml_node run_properties) -> std::optional<bool> {
    return read_on_off_value(run_properties.child("w:rtl"));
}

auto read_paragraph_bidi(pugi::xml_node paragraph_properties) -> std::optional<bool> {
    return read_on_off_value(paragraph_properties.child("w:bidi"));
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

auto ensure_style_run_properties_node(pugi::xml_node style) -> pugi::xml_node {
    if (style == pugi::xml_node{}) {
        return {};
    }

    auto run_properties = style.child("w:rPr");
    if (run_properties != pugi::xml_node{}) {
        return run_properties;
    }

    if (const auto paragraph_properties = style.child("w:pPr");
        paragraph_properties != pugi::xml_node{}) {
        return style.insert_child_after("w:rPr", paragraph_properties);
    }

    return style.append_child("w:rPr");
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

auto ensure_run_fonts_node(pugi::xml_node run_properties) -> pugi::xml_node {
    if (run_properties == pugi::xml_node{}) {
        return {};
    }

    auto run_fonts = run_properties.child("w:rFonts");
    if (run_fonts != pugi::xml_node{}) {
        return run_fonts;
    }

    if (const auto run_style = run_properties.child("w:rStyle");
        run_style != pugi::xml_node{}) {
        return run_properties.insert_child_after("w:rFonts", run_style);
    }

    if (const auto first_child = run_properties.first_child(); first_child != pugi::xml_node{}) {
        return run_properties.insert_child_before("w:rFonts", first_child);
    }

    return run_properties.append_child("w:rFonts");
}

auto ensure_run_language_node(pugi::xml_node run_properties) -> pugi::xml_node {
    if (run_properties == pugi::xml_node{}) {
        return {};
    }

    auto run_language = run_properties.child("w:lang");
    if (run_language != pugi::xml_node{}) {
        return run_language;
    }

    if (const auto run_fonts = run_properties.child("w:rFonts");
        run_fonts != pugi::xml_node{}) {
        return run_properties.insert_child_after("w:lang", run_fonts);
    }

    if (const auto run_style = run_properties.child("w:rStyle");
        run_style != pugi::xml_node{}) {
        return run_properties.insert_child_after("w:lang", run_style);
    }

    if (const auto first_child = run_properties.first_child(); first_child != pugi::xml_node{}) {
        return run_properties.insert_child_before("w:lang", first_child);
    }

    return run_properties.append_child("w:lang");
}

auto ensure_run_rtl_node(pugi::xml_node run_properties) -> pugi::xml_node {
    if (run_properties == pugi::xml_node{}) {
        return {};
    }

    auto rtl = run_properties.child("w:rtl");
    if (rtl != pugi::xml_node{}) {
        return rtl;
    }

    if (const auto run_language = run_properties.child("w:lang");
        run_language != pugi::xml_node{}) {
        return run_properties.insert_child_after("w:rtl", run_language);
    }

    if (const auto run_fonts = run_properties.child("w:rFonts");
        run_fonts != pugi::xml_node{}) {
        return run_properties.insert_child_after("w:rtl", run_fonts);
    }

    if (const auto run_style = run_properties.child("w:rStyle");
        run_style != pugi::xml_node{}) {
        return run_properties.insert_child_after("w:rtl", run_style);
    }

    if (const auto first_child = run_properties.first_child(); first_child != pugi::xml_node{}) {
        return run_properties.insert_child_before("w:rtl", first_child);
    }

    return run_properties.append_child("w:rtl");
}

auto ensure_paragraph_bidi_node(pugi::xml_node paragraph_properties) -> pugi::xml_node {
    if (paragraph_properties == pugi::xml_node{}) {
        return {};
    }

    auto bidi = paragraph_properties.child("w:bidi");
    if (bidi != pugi::xml_node{}) {
        return bidi;
    }

    return paragraph_properties.append_child("w:bidi");
}

auto ensure_doc_defaults_node(pugi::xml_node styles_root) -> pugi::xml_node {
    if (styles_root == pugi::xml_node{}) {
        return {};
    }

    auto doc_defaults = styles_root.child("w:docDefaults");
    if (doc_defaults != pugi::xml_node{}) {
        return doc_defaults;
    }

    if (const auto first_child = styles_root.first_child(); first_child != pugi::xml_node{}) {
        return styles_root.insert_child_before("w:docDefaults", first_child);
    }

    return styles_root.append_child("w:docDefaults");
}

auto ensure_doc_defaults_run_properties_node(pugi::xml_node styles_root) -> pugi::xml_node {
    auto doc_defaults = ensure_doc_defaults_node(styles_root);
    if (doc_defaults == pugi::xml_node{}) {
        return {};
    }

    auto run_properties_default = doc_defaults.child("w:rPrDefault");
    if (run_properties_default == pugi::xml_node{}) {
        if (const auto first_child = doc_defaults.first_child(); first_child != pugi::xml_node{}) {
            run_properties_default = doc_defaults.insert_child_before("w:rPrDefault", first_child);
        } else {
            run_properties_default = doc_defaults.append_child("w:rPrDefault");
        }
    }
    if (run_properties_default == pugi::xml_node{}) {
        return {};
    }

    auto run_properties = run_properties_default.child("w:rPr");
    if (run_properties != pugi::xml_node{}) {
        return run_properties;
    }

    if (const auto first_child = run_properties_default.first_child();
        first_child != pugi::xml_node{}) {
        return run_properties_default.insert_child_before("w:rPr", first_child);
    }

    return run_properties_default.append_child("w:rPr");
}

auto ensure_doc_defaults_paragraph_properties_node(pugi::xml_node styles_root)
    -> pugi::xml_node {
    auto doc_defaults = ensure_doc_defaults_node(styles_root);
    if (doc_defaults == pugi::xml_node{}) {
        return {};
    }

    auto paragraph_properties_default = doc_defaults.child("w:pPrDefault");
    if (paragraph_properties_default == pugi::xml_node{}) {
        if (const auto run_properties_default = doc_defaults.child("w:rPrDefault");
            run_properties_default != pugi::xml_node{}) {
            paragraph_properties_default =
                doc_defaults.insert_child_after("w:pPrDefault", run_properties_default);
        } else if (const auto first_child = doc_defaults.first_child();
                   first_child != pugi::xml_node{}) {
            paragraph_properties_default =
                doc_defaults.insert_child_before("w:pPrDefault", first_child);
        } else {
            paragraph_properties_default = doc_defaults.append_child("w:pPrDefault");
        }
    }
    if (paragraph_properties_default == pugi::xml_node{}) {
        return {};
    }

    auto paragraph_properties = paragraph_properties_default.child("w:pPr");
    if (paragraph_properties != pugi::xml_node{}) {
        return paragraph_properties;
    }

    if (const auto first_child = paragraph_properties_default.first_child();
        first_child != pugi::xml_node{}) {
        return paragraph_properties_default.insert_child_before("w:pPr", first_child);
    }

    return paragraph_properties_default.append_child("w:pPr");
}

auto find_style_node(pugi::xml_node styles_root, std::string_view style_id) -> pugi::xml_node;

auto style_properties_anchor(pugi::xml_node style) -> pugi::xml_node {
    if (const auto paragraph_properties = style.child("w:pPr");
        paragraph_properties != pugi::xml_node{}) {
        return paragraph_properties;
    }
    if (const auto run_properties = style.child("w:rPr"); run_properties != pugi::xml_node{}) {
        return run_properties;
    }
    return style.child("w:tblPr");
}

auto ensure_style_child_before_properties(pugi::xml_node style, const char *child_name)
    -> pugi::xml_node {
    if (style == pugi::xml_node{}) {
        return {};
    }

    auto child = style.child(child_name);
    if (child != pugi::xml_node{}) {
        return child;
    }

    if (const auto anchor = style_properties_anchor(style); anchor != pugi::xml_node{}) {
        return style.insert_child_before(child_name, anchor);
    }

    return style.append_child(child_name);
}

auto ensure_style_name_node(pugi::xml_node style) -> pugi::xml_node {
    return ensure_style_child_before_properties(style, "w:name");
}

auto ensure_style_based_on_node(pugi::xml_node style) -> pugi::xml_node {
    if (style == pugi::xml_node{}) {
        return {};
    }

    auto based_on = style.child("w:basedOn");
    if (based_on != pugi::xml_node{}) {
        return based_on;
    }

    if (const auto name = style.child("w:name"); name != pugi::xml_node{}) {
        return style.insert_child_after("w:basedOn", name);
    }

    return ensure_style_child_before_properties(style, "w:basedOn");
}

auto ensure_style_next_node(pugi::xml_node style) -> pugi::xml_node {
    if (style == pugi::xml_node{}) {
        return {};
    }

    auto next = style.child("w:next");
    if (next != pugi::xml_node{}) {
        return next;
    }

    if (const auto based_on = style.child("w:basedOn"); based_on != pugi::xml_node{}) {
        return style.insert_child_after("w:next", based_on);
    }
    if (const auto name = style.child("w:name"); name != pugi::xml_node{}) {
        return style.insert_child_after("w:next", name);
    }

    return ensure_style_child_before_properties(style, "w:next");
}

auto ensure_style_outline_level_node(pugi::xml_node paragraph_properties) -> pugi::xml_node {
    if (paragraph_properties == pugi::xml_node{}) {
        return {};
    }

    auto outline_level = paragraph_properties.child("w:outlineLvl");
    if (outline_level != pugi::xml_node{}) {
        return outline_level;
    }

    if (const auto bidi = paragraph_properties.child("w:bidi"); bidi != pugi::xml_node{}) {
        return paragraph_properties.insert_child_before("w:outlineLvl", bidi);
    }

    if (const auto first_child = paragraph_properties.first_child();
        first_child != pugi::xml_node{}) {
        return paragraph_properties.insert_child_before("w:outlineLvl", first_child);
    }

    return paragraph_properties.append_child("w:outlineLvl");
}

auto ensure_style_node(pugi::xml_node styles_root, std::string_view style_id) -> pugi::xml_node {
    if (styles_root == pugi::xml_node{}) {
        return {};
    }

    if (auto style = find_style_node(styles_root, style_id); style != pugi::xml_node{}) {
        return style;
    }

    auto style = styles_root.append_child("w:style");
    if (style != pugi::xml_node{}) {
        ensure_attribute_value(style, "w:styleId", style_id);
    }
    return style;
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

auto read_on_off_attribute(pugi::xml_node node, const char *attribute_name) -> bool {
    if (node == pugi::xml_node{}) {
        return false;
    }

    const auto attribute = node.attribute(attribute_name);
    if (attribute == pugi::xml_attribute{} || attribute.value()[0] == '\0') {
        return false;
    }

    const auto value = std::string_view{attribute.value()};
    return value != "0" && value != "false" && value != "off";
}

auto decode_style_kind(std::string_view type_name) -> featherdoc::style_kind {
    if (type_name == "paragraph") {
        return featherdoc::style_kind::paragraph;
    }
    if (type_name == "character") {
        return featherdoc::style_kind::character;
    }
    if (type_name == "table") {
        return featherdoc::style_kind::table;
    }
    if (type_name == "numbering") {
        return featherdoc::style_kind::numbering;
    }

    return featherdoc::style_kind::unknown;
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

auto summarize_style_numbering(pugi::xml_node style)
    -> std::optional<featherdoc::style_summary::numbering_summary> {
    const auto numbering_properties = style.child("w:pPr").child("w:numPr");
    if (numbering_properties == pugi::xml_node{}) {
        return std::nullopt;
    }

    auto summary = featherdoc::style_summary::numbering_summary{};
    summary.level = parse_u32_attribute_value(
        numbering_properties.child("w:ilvl").attribute("w:val").value());
    summary.num_id = parse_u32_attribute_value(
        numbering_properties.child("w:numId").attribute("w:val").value());
    return summary;
}

auto style_has_numbering(pugi::xml_node style) -> bool {
    return style.child("w:pPr").child("w:numPr") != pugi::xml_node{};
}

auto summarize_style_node(pugi::xml_node style)
    -> featherdoc::style_summary {
    auto summary = featherdoc::style_summary{};
    summary.style_id = style.attribute("w:styleId").value();
    summary.type_name = style.attribute("w:type").value();
    summary.kind = decode_style_kind(summary.type_name);

    if (const auto name = style.child("w:name").attribute("w:val");
        name != pugi::xml_attribute{}) {
        summary.name = name.value();
    }

    if (const auto based_on = style.child("w:basedOn").attribute("w:val");
        based_on != pugi::xml_attribute{} && based_on.value()[0] != '\0') {
        summary.based_on = std::string{based_on.value()};
    }

    summary.numbering = summarize_style_numbering(style);
    summary.is_default = read_on_off_attribute(style, "w:default");
    summary.is_custom = read_on_off_attribute(style, "w:customStyle");
    summary.is_semi_hidden = read_on_off_value(style.child("w:semiHidden")).value_or(false);
    summary.is_unhide_when_used =
        read_on_off_value(style.child("w:unhideWhenUsed")).value_or(false);
    summary.is_quick_format = read_on_off_value(style.child("w:qFormat")).value_or(false);
    return summary;
}

auto node_style_reference_matches(pugi::xml_node node, const char *properties_name,
                                  const char *style_name, std::string_view style_id)
    -> bool {
    const auto style_value = node.child(properties_name).child(style_name).attribute("w:val");
    if (style_value == pugi::xml_attribute{}) {
        return false;
    }

    return std::string_view{style_value.value()} == style_id;
}

void append_style_usage_hit(std::vector<featherdoc::style_usage_hit> &hits,
                            featherdoc::style_usage_part_kind part_kind,
                            featherdoc::style_usage_hit_kind hit_kind,
                            std::string_view entry_name, std::size_t ordinal,
                            std::optional<std::size_t> section_index = std::nullopt) {
    auto hit = featherdoc::style_usage_hit{};
    hit.part = part_kind;
    hit.kind = hit_kind;
    hit.entry_name = std::string{entry_name};
    hit.ordinal = ordinal;
    hit.section_index = section_index;
    hits.push_back(std::move(hit));
}

void append_style_usage_hit_reference(
    std::vector<featherdoc::style_usage_hit_reference> &references,
    std::size_t section_index, featherdoc::section_reference_kind reference_kind) {
    for (const auto &reference : references) {
        if (reference.section_index == section_index &&
            reference.reference_kind == reference_kind) {
            return;
        }
    }

    auto reference = featherdoc::style_usage_hit_reference{};
    reference.section_index = section_index;
    reference.reference_kind = reference_kind;
    references.push_back(reference);
}

auto style_usage_reference_kind(std::string_view xml_reference_type)
    -> featherdoc::section_reference_kind {
    if (xml_reference_type ==
        featherdoc::to_xml_reference_type(featherdoc::section_reference_kind::first_page)) {
        return featherdoc::section_reference_kind::first_page;
    }

    if (xml_reference_type ==
        featherdoc::to_xml_reference_type(featherdoc::section_reference_kind::even_page)) {
        return featherdoc::section_reference_kind::even_page;
    }

    return featherdoc::section_reference_kind::default_reference;
}

void apply_style_usage_hit_references(
    std::vector<featherdoc::style_usage_hit> &hits,
    const std::unordered_map<std::string,
                             std::vector<featherdoc::style_usage_hit_reference>>
        &header_references,
    const std::unordered_map<std::string,
                             std::vector<featherdoc::style_usage_hit_reference>>
        &footer_references) {
    for (auto &hit : hits) {
        if (hit.part == featherdoc::style_usage_part_kind::body) {
            continue;
        }

        const auto &references =
            hit.part == featherdoc::style_usage_part_kind::header ? header_references
                                                                  : footer_references;
        const auto iterator = references.find(hit.entry_name);
        if (iterator != references.end()) {
            hit.references = iterator->second;
        }
    }
}

void collect_style_usage(pugi::xml_node node, std::string_view style_id,
                         featherdoc::style_usage_breakdown &usage,
                         featherdoc::style_usage_part_kind part_kind,
                         std::string_view entry_name,
                         std::vector<featherdoc::style_usage_hit> *hits,
                         std::size_t &part_ordinal,
                         std::optional<std::size_t> section_index = std::nullopt) {
    if (node == pugi::xml_node{}) {
        return;
    }

    const auto node_name = std::string_view{node.name()};
    if (node_name == "w:p" &&
        node_style_reference_matches(node, "w:pPr", "w:pStyle", style_id)) {
        ++usage.paragraph_count;
        ++part_ordinal;
        if (hits != nullptr) {
            append_style_usage_hit(*hits, part_kind,
                                   featherdoc::style_usage_hit_kind::paragraph, entry_name,
                                   part_ordinal, section_index);
        }
    } else if (node_name == "w:r" &&
               node_style_reference_matches(node, "w:rPr", "w:rStyle", style_id)) {
        ++usage.run_count;
        ++part_ordinal;
        if (hits != nullptr) {
            append_style_usage_hit(*hits, part_kind,
                                   featherdoc::style_usage_hit_kind::run, entry_name, part_ordinal,
                                   section_index);
        }
    } else if (node_name == "w:tbl" &&
               node_style_reference_matches(node, "w:tblPr", "w:tblStyle", style_id)) {
        ++usage.table_count;
        ++part_ordinal;
        if (hits != nullptr) {
            append_style_usage_hit(*hits, part_kind,
                                   featherdoc::style_usage_hit_kind::table, entry_name, part_ordinal,
                                   section_index);
        }
    }

    for (auto child = node.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        collect_style_usage(child, style_id, usage, part_kind, entry_name, hits, part_ordinal,
                            section_index);
    }
}

void collect_body_style_usage(pugi::xml_node body, std::string_view style_id,
                              featherdoc::style_usage_breakdown &usage,
                              std::vector<featherdoc::style_usage_hit> *hits) {
    if (body == pugi::xml_node{}) {
        return;
    }

    auto part_ordinal = std::size_t{0};
    auto section_index = std::size_t{0};
    for (auto child = body.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        const auto child_name = std::string_view{child.name()};
        if (child_name == "w:sectPr") {
            continue;
        }

        collect_style_usage(child, style_id, usage, featherdoc::style_usage_part_kind::body,
                            document_xml_entry, hits, part_ordinal, section_index);
        if (child_name == "w:p" && child.child("w:pPr").child("w:sectPr") != pugi::xml_node{}) {
            ++section_index;
        }
    }
}

void finalize_style_usage_totals(featherdoc::style_usage_summary &usage) {
    usage.paragraph_count =
        usage.body.paragraph_count + usage.header.paragraph_count + usage.footer.paragraph_count;
    usage.run_count = usage.body.run_count + usage.header.run_count + usage.footer.run_count;
    usage.table_count =
        usage.body.table_count + usage.header.table_count + usage.footer.table_count;
}

void remove_empty_run_fonts_node(pugi::xml_node run_properties) {
    if (run_properties == pugi::xml_node{}) {
        return;
    }

    auto run_fonts = run_properties.child("w:rFonts");
    if (run_fonts == pugi::xml_node{}) {
        return;
    }

    if (run_fonts.first_child() == pugi::xml_node{} && !node_has_attributes(run_fonts)) {
        run_properties.remove_child(run_fonts);
    }
}

void remove_empty_run_language_node(pugi::xml_node run_properties) {
    if (run_properties == pugi::xml_node{}) {
        return;
    }

    auto run_language = run_properties.child("w:lang");
    if (run_language == pugi::xml_node{}) {
        return;
    }

    if (run_language.first_child() == pugi::xml_node{} && !node_has_attributes(run_language)) {
        run_properties.remove_child(run_language);
    }
}

auto clear_font_family_attributes(pugi::xml_node run_properties) -> bool {
    if (run_properties == pugi::xml_node{}) {
        return false;
    }

    auto run_fonts = run_properties.child("w:rFonts");
    if (run_fonts == pugi::xml_node{}) {
        return false;
    }

    bool removed = false;
    removed = run_fonts.remove_attribute("w:ascii") || removed;
    removed = run_fonts.remove_attribute("w:hAnsi") || removed;
    removed = run_fonts.remove_attribute("w:cs") || removed;
    removed = run_fonts.remove_attribute("w:eastAsia") || removed;
    remove_empty_run_fonts_node(run_properties);
    return removed;
}

auto clear_language_attributes(pugi::xml_node run_properties) -> bool {
    if (run_properties == pugi::xml_node{}) {
        return false;
    }

    auto run_language = run_properties.child("w:lang");
    if (run_language == pugi::xml_node{}) {
        return false;
    }

    bool removed = false;
    removed = run_language.remove_attribute("w:val") || removed;
    removed = run_language.remove_attribute("w:eastAsia") || removed;
    removed = run_language.remove_attribute("w:bidi") || removed;
    remove_empty_run_language_node(run_properties);
    return removed;
}

void set_on_off_value(pugi::xml_node node, bool enabled) {
    if (node == pugi::xml_node{}) {
        return;
    }

    auto attribute = node.attribute("w:val");
    if (attribute == pugi::xml_attribute{}) {
        attribute = node.append_attribute("w:val");
    }
    attribute.set_value(enabled ? "1" : "0");
}

auto clear_on_off_child(pugi::xml_node parent, const char *child_name) -> bool {
    if (parent == pugi::xml_node{}) {
        return false;
    }

    const auto child = parent.child(child_name);
    return child != pugi::xml_node{} && parent.remove_child(child);
}

void set_style_flag_node(pugi::xml_node style, const char *child_name, bool enabled) {
    if (style == pugi::xml_node{}) {
        return;
    }

    if (!enabled) {
        clear_on_off_child(style, child_name);
        return;
    }

    const auto flag = ensure_style_child_before_properties(style, child_name);
    if (flag == pugi::xml_node{}) {
        return;
    }

    ensure_attribute_value(flag, "w:val", "1");
}

void set_style_custom_flag(pugi::xml_node style, bool enabled) {
    if (style == pugi::xml_node{}) {
        return;
    }

    if (enabled) {
        ensure_attribute_value(style, "w:customStyle", "1");
        return;
    }

    style.remove_attribute("w:customStyle");
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

auto ensure_style_metadata_node(pugi::xml_node style, const char *child_name) -> pugi::xml_node {
    const auto name = std::string_view{child_name};
    if (name == "w:name") {
        return ensure_style_name_node(style);
    }
    if (name == "w:basedOn") {
        return ensure_style_based_on_node(style);
    }
    if (name == "w:next") {
        return ensure_style_next_node(style);
    }
    return ensure_style_child_before_properties(style, child_name);
}

auto set_style_string_node(pugi::xml_node style, const char *child_name,
                           const std::optional<std::string> &value) -> bool {
    if (style == pugi::xml_node{}) {
        return false;
    }

    if (!value.has_value()) {
        if (const auto child = style.child(child_name); child != pugi::xml_node{}) {
            style.remove_child(child);
        }
        return true;
    }

    const auto child = ensure_style_metadata_node(style, child_name);
    if (child == pugi::xml_node{}) {
        return false;
    }

    ensure_attribute_value(child, "w:val", *value);
    return true;
}

auto set_style_outline_level(pugi::xml_node style,
                             const std::optional<std::uint32_t> &outline_level) -> bool {
    if (style == pugi::xml_node{}) {
        return false;
    }

    if (!outline_level.has_value()) {
        if (auto paragraph_properties = style.child("w:pPr");
            paragraph_properties != pugi::xml_node{}) {
            clear_on_off_child(paragraph_properties, "w:outlineLvl");
            remove_empty_style_paragraph_properties(style);
        }
        return true;
    }

    const auto paragraph_properties = ensure_style_paragraph_properties_node(style);
    if (paragraph_properties == pugi::xml_node{}) {
        return false;
    }

    const auto outline = ensure_style_outline_level_node(paragraph_properties);
    if (outline == pugi::xml_node{}) {
        return false;
    }

    ensure_attribute_value(outline, "w:val", std::to_string(*outline_level));
    return true;
}

auto validate_optional_style_value(featherdoc::document_error_info &last_error_info,
                                   const std::optional<std::string> &value,
                                   std::string_view detail) -> bool {
    if (!value.has_value() || !value->empty()) {
        return true;
    }

    set_last_error(last_error_info, std::make_error_code(std::errc::invalid_argument),
                   std::string{detail}, std::string{styles_xml_entry});
    return false;
}

auto apply_common_style_definition(pugi::xml_node style, std::string_view style_id,
                                   std::string_view type_name, std::string_view name,
                                   const std::optional<std::string> &based_on, bool is_custom,
                                   bool is_semi_hidden, bool is_unhide_when_used,
                                   bool is_quick_format) -> bool {
    if (style == pugi::xml_node{}) {
        return false;
    }

    ensure_attribute_value(style, "w:styleId", style_id);
    ensure_attribute_value(style, "w:type", type_name);
    set_style_custom_flag(style, is_custom);

    const auto name_node = ensure_style_name_node(style);
    if (name_node == pugi::xml_node{}) {
        return false;
    }
    ensure_attribute_value(name_node, "w:val", name);
    if (!set_style_string_node(style, "w:basedOn", based_on)) {
        return false;
    }

    set_style_flag_node(style, "w:semiHidden", is_semi_hidden);
    set_style_flag_node(style, "w:unhideWhenUsed", is_unhide_when_used);
    set_style_flag_node(style, "w:qFormat", is_quick_format);
    return true;
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

std::vector<featherdoc::style_summary> Document::list_styles() {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before listing styles",
                       std::string{styles_xml_entry});
        return {};
    }

    if (const auto error = this->ensure_styles_loaded()) {
        return {};
    }

    const auto styles_root = this->styles.child("w:styles");
    if (styles_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info, document_errc::styles_xml_parse_failed,
                       "word/styles.xml does not contain a w:styles root",
                       std::string{styles_xml_entry});
        return {};
    }

    auto numbering_ready = false;
    for (auto style = styles_root.child("w:style"); style != pugi::xml_node{};
         style = style.next_sibling("w:style")) {
        if (!style_has_numbering(style)) {
            continue;
        }
        if (!this->ensure_numbering_loaded()) {
            numbering_ready = this->numbering.child("w:numbering") != pugi::xml_node{};
        }
        break;
    }

    const auto apply_numbering_lookup = [this, numbering_ready](
                                            featherdoc::style_summary &summary) {
        if (!numbering_ready || !summary.numbering.has_value() ||
            !summary.numbering->num_id.has_value()) {
            return;
        }

        const auto lookup = this->find_numbering_instance(*summary.numbering->num_id);
        if (!lookup.has_value()) {
            this->last_error_info.clear();
            return;
        }

        summary.numbering->definition_id = lookup->definition_id;
        if (!lookup->definition_name.empty()) {
            summary.numbering->definition_name = lookup->definition_name;
        }
        summary.numbering->instance = lookup->instance;
        this->last_error_info.clear();
    };

    auto summaries = std::vector<featherdoc::style_summary>{};
    for (auto style = styles_root.child("w:style"); style != pugi::xml_node{};
         style = style.next_sibling("w:style")) {
        auto summary = summarize_style_node(style);
        apply_numbering_lookup(summary);
        summaries.push_back(std::move(summary));
    }

    this->last_error_info.clear();
    return summaries;
}

std::optional<featherdoc::style_summary> Document::find_style(std::string_view style_id) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before reading style metadata",
                       std::string{styles_xml_entry});
        return std::nullopt;
    }

    if (style_id.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id must not be empty when reading style metadata",
                       std::string{styles_xml_entry});
        return std::nullopt;
    }

    if (const auto error = this->ensure_styles_loaded()) {
        return std::nullopt;
    }

    const auto styles_root = this->styles.child("w:styles");
    if (styles_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info, document_errc::styles_xml_parse_failed,
                       "word/styles.xml does not contain a w:styles root",
                       std::string{styles_xml_entry});
        return std::nullopt;
    }

    const auto style = find_style_node(styles_root, style_id);
    if (style == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id '" + std::string{style_id} +
                           "' was not found in word/styles.xml",
                       std::string{styles_xml_entry});
        return std::nullopt;
    }

    const auto numbering_ready =
        style_has_numbering(style) && !this->ensure_numbering_loaded() &&
        this->numbering.child("w:numbering") != pugi::xml_node{};

    auto summary = summarize_style_node(style);
    if (numbering_ready && summary.numbering.has_value() &&
        summary.numbering->num_id.has_value()) {
        const auto lookup = this->find_numbering_instance(*summary.numbering->num_id);
        if (lookup.has_value()) {
            summary.numbering->definition_id = lookup->definition_id;
            if (!lookup->definition_name.empty()) {
                summary.numbering->definition_name = lookup->definition_name;
            }
            summary.numbering->instance = lookup->instance;
        }
    }

    this->last_error_info.clear();
    return summary;
}

std::optional<featherdoc::style_usage_summary> Document::find_style_usage(
    std::string_view style_id) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before reading style usage",
                       std::string{document_xml_entry});
        return std::nullopt;
    }

    if (style_id.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id must not be empty when reading style usage",
                       std::string{document_xml_entry});
        return std::nullopt;
    }

    if (const auto error = this->ensure_styles_loaded()) {
        return std::nullopt;
    }

    const auto styles_root = this->styles.child("w:styles");
    if (styles_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info, document_errc::styles_xml_parse_failed,
                       "word/styles.xml does not contain a w:styles root",
                       std::string{styles_xml_entry});
        return std::nullopt;
    }

    if (find_style_node(styles_root, style_id) == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id '" + std::string{style_id} +
                           "' was not found in word/styles.xml",
                       std::string{styles_xml_entry});
        return std::nullopt;
    }

    const auto document_root = this->document.child("w:document");
    if (document_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info, document_errc::document_xml_parse_failed,
                       "word/document.xml does not contain a w:document root",
                       std::string{document_xml_entry});
        return std::nullopt;
    }

    auto usage = featherdoc::style_usage_summary{};
    usage.style_id = std::string{style_id};
    collect_body_style_usage(document_root.child("w:body"), style_id, usage.body, &usage.hits);

    for (const auto &part : this->header_parts) {
        if (!part) {
            continue;
        }

        auto part_ordinal = std::size_t{0};
        collect_style_usage(part->xml.child("w:hdr"), style_id, usage.header,
                            featherdoc::style_usage_part_kind::header, part->entry_name,
                            &usage.hits, part_ordinal);
    }

    for (const auto &part : this->footer_parts) {
        if (!part) {
            continue;
        }

        auto part_ordinal = std::size_t{0};
        collect_style_usage(part->xml.child("w:ftr"), style_id, usage.footer,
                            featherdoc::style_usage_part_kind::footer, part->entry_name,
                            &usage.hits, part_ordinal);
    }

    const auto build_part_references = [&](const char *reference_name, const auto &parts) {
        auto references_by_relationship_id =
            std::unordered_map<std::string,
                               std::vector<featherdoc::style_usage_hit_reference>>{};
        const auto section_total = this->section_count();
        for (std::size_t section_index = 0; section_index < section_total; ++section_index) {
            const auto section_properties = this->section_properties(section_index);
            if (section_properties == pugi::xml_node{}) {
                continue;
            }

            for (auto reference = section_properties.child(reference_name);
                 reference != pugi::xml_node{};
                 reference = reference.next_sibling(reference_name)) {
                const auto relationship_id =
                    std::string_view{reference.attribute("r:id").value()};
                if (relationship_id.empty()) {
                    continue;
                }

                auto &references =
                    references_by_relationship_id[std::string{relationship_id}];
                append_style_usage_hit_reference(
                    references, section_index,
                    style_usage_reference_kind(reference.attribute("w:type").value()));
            }
        }

        auto references_by_entry_name =
            std::unordered_map<std::string,
                               std::vector<featherdoc::style_usage_hit_reference>>{};
        for (const auto &part : parts) {
            if (!part || part->entry_name.empty() || part->relationship_id.empty()) {
                continue;
            }

            const auto iterator =
                references_by_relationship_id.find(part->relationship_id);
            if (iterator == references_by_relationship_id.end()) {
                continue;
            }

            references_by_entry_name.emplace(part->entry_name, iterator->second);
        }

        return references_by_entry_name;
    };

    const auto header_references =
        build_part_references("w:headerReference", this->header_parts);
    const auto footer_references =
        build_part_references("w:footerReference", this->footer_parts);
    apply_style_usage_hit_references(usage.hits, header_references, footer_references);

    finalize_style_usage_totals(usage);

    this->last_error_info.clear();
    return usage;
}

bool Document::ensure_paragraph_style(std::string_view style_id,
                                      const featherdoc::paragraph_style_definition &definition) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before defining paragraph styles");
        return false;
    }

    if (style_id.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id must not be empty when defining paragraph styles",
                       std::string{styles_xml_entry});
        return false;
    }

    if (definition.name.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "paragraph style name must not be empty",
                       std::string{styles_xml_entry});
        return false;
    }

    if (!validate_optional_style_value(this->last_error_info, definition.based_on,
                                       "paragraph style basedOn must not be empty") ||
        !validate_optional_style_value(this->last_error_info, definition.next_style,
                                       "paragraph style next style must not be empty") ||
        !validate_optional_style_value(this->last_error_info, definition.run_font_family,
                                       "paragraph style run font family must not be empty") ||
        !validate_optional_style_value(
            this->last_error_info, definition.run_east_asia_font_family,
            "paragraph style eastAsia font family must not be empty") ||
        !validate_optional_style_value(this->last_error_info, definition.run_language,
                                       "paragraph style run language must not be empty") ||
        !validate_optional_style_value(
            this->last_error_info, definition.run_east_asia_language,
            "paragraph style eastAsia language must not be empty") ||
        !validate_optional_style_value(this->last_error_info, definition.run_bidi_language,
                                       "paragraph style bidi language must not be empty")) {
        return false;
    }

    if (definition.outline_level.has_value() && *definition.outline_level > 8U) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "paragraph style outline level must be between 0 and 8",
                       std::string{styles_xml_entry});
        return false;
    }

    if (const auto error = this->ensure_styles_part_attached()) {
        return false;
    }

    const auto styles_root = this->styles.child("w:styles");
    if (styles_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info, document_errc::styles_xml_parse_failed,
                       "word/styles.xml does not contain a w:styles root",
                       std::string{styles_xml_entry});
        return false;
    }

    if (const auto existing = find_style_node(styles_root, style_id);
        existing != pugi::xml_node{}) {
        const auto existing_type = std::string_view{existing.attribute("w:type").value()};
        if (!existing_type.empty() && existing_type != "paragraph") {
            set_last_error(this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           "style id '" + std::string{style_id} + "' already exists with type '" +
                               std::string{existing_type} + "'",
                           std::string{styles_xml_entry});
            return false;
        }
    }

    const auto style = ensure_style_node(styles_root, style_id);
    if (style == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to create paragraph style '" + std::string{style_id} + "'",
                       std::string{styles_xml_entry});
        return false;
    }

    if (!apply_common_style_definition(style, style_id, "paragraph", definition.name,
                                       definition.based_on, definition.is_custom,
                                       definition.is_semi_hidden,
                                       definition.is_unhide_when_used,
                                       definition.is_quick_format) ||
        !set_style_string_node(style, "w:next", definition.next_style) ||
        !set_style_outline_level(style, definition.outline_level)) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to update paragraph style '" + std::string{style_id} + "'",
                       std::string{styles_xml_entry});
        return false;
    }

    this->styles_dirty = true;

    if (!this->clear_style_run_font_family(style_id)) {
        return false;
    }
    if (definition.run_font_family.has_value() &&
        !this->set_style_run_font_family(style_id, *definition.run_font_family)) {
        return false;
    }
    if (definition.run_east_asia_font_family.has_value() &&
        !this->set_style_run_east_asia_font_family(style_id,
                                                   *definition.run_east_asia_font_family)) {
        return false;
    }

    if (!this->clear_style_run_language(style_id)) {
        return false;
    }
    if (definition.run_language.has_value() &&
        !this->set_style_run_language(style_id, *definition.run_language)) {
        return false;
    }
    if (definition.run_east_asia_language.has_value() &&
        !this->set_style_run_east_asia_language(style_id,
                                                *definition.run_east_asia_language)) {
        return false;
    }
    if (definition.run_bidi_language.has_value() &&
        !this->set_style_run_bidi_language(style_id, *definition.run_bidi_language)) {
        return false;
    }

    if (definition.run_rtl.has_value()) {
        if (!this->set_style_run_rtl(style_id, *definition.run_rtl)) {
            return false;
        }
    } else if (!this->clear_style_run_rtl(style_id)) {
        return false;
    }

    if (definition.paragraph_bidi.has_value()) {
        if (!this->set_style_paragraph_bidi(style_id, *definition.paragraph_bidi)) {
            return false;
        }
    } else if (!this->clear_style_paragraph_bidi(style_id)) {
        return false;
    }

    this->last_error_info.clear();
    return true;
}

bool Document::ensure_character_style(std::string_view style_id,
                                      const featherdoc::character_style_definition &definition) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before defining character styles");
        return false;
    }

    if (style_id.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id must not be empty when defining character styles",
                       std::string{styles_xml_entry});
        return false;
    }

    if (definition.name.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "character style name must not be empty",
                       std::string{styles_xml_entry});
        return false;
    }

    if (!validate_optional_style_value(this->last_error_info, definition.based_on,
                                       "character style basedOn must not be empty") ||
        !validate_optional_style_value(this->last_error_info, definition.run_font_family,
                                       "character style run font family must not be empty") ||
        !validate_optional_style_value(
            this->last_error_info, definition.run_east_asia_font_family,
            "character style eastAsia font family must not be empty") ||
        !validate_optional_style_value(this->last_error_info, definition.run_language,
                                       "character style run language must not be empty") ||
        !validate_optional_style_value(
            this->last_error_info, definition.run_east_asia_language,
            "character style eastAsia language must not be empty") ||
        !validate_optional_style_value(this->last_error_info, definition.run_bidi_language,
                                       "character style bidi language must not be empty")) {
        return false;
    }

    if (const auto error = this->ensure_styles_part_attached()) {
        return false;
    }

    const auto styles_root = this->styles.child("w:styles");
    if (styles_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info, document_errc::styles_xml_parse_failed,
                       "word/styles.xml does not contain a w:styles root",
                       std::string{styles_xml_entry});
        return false;
    }

    if (const auto existing = find_style_node(styles_root, style_id);
        existing != pugi::xml_node{}) {
        const auto existing_type = std::string_view{existing.attribute("w:type").value()};
        if (!existing_type.empty() && existing_type != "character") {
            set_last_error(this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           "style id '" + std::string{style_id} + "' already exists with type '" +
                               std::string{existing_type} + "'",
                           std::string{styles_xml_entry});
            return false;
        }
    }

    const auto style = ensure_style_node(styles_root, style_id);
    if (style == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to create character style '" + std::string{style_id} + "'",
                       std::string{styles_xml_entry});
        return false;
    }

    if (!apply_common_style_definition(style, style_id, "character", definition.name,
                                       definition.based_on, definition.is_custom,
                                       definition.is_semi_hidden,
                                       definition.is_unhide_when_used,
                                       definition.is_quick_format)) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to update character style '" + std::string{style_id} + "'",
                       std::string{styles_xml_entry});
        return false;
    }

    this->styles_dirty = true;

    if (!this->clear_style_run_font_family(style_id)) {
        return false;
    }
    if (definition.run_font_family.has_value() &&
        !this->set_style_run_font_family(style_id, *definition.run_font_family)) {
        return false;
    }
    if (definition.run_east_asia_font_family.has_value() &&
        !this->set_style_run_east_asia_font_family(style_id,
                                                   *definition.run_east_asia_font_family)) {
        return false;
    }

    if (!this->clear_style_run_language(style_id)) {
        return false;
    }
    if (definition.run_language.has_value() &&
        !this->set_style_run_language(style_id, *definition.run_language)) {
        return false;
    }
    if (definition.run_east_asia_language.has_value() &&
        !this->set_style_run_east_asia_language(style_id,
                                                *definition.run_east_asia_language)) {
        return false;
    }
    if (definition.run_bidi_language.has_value() &&
        !this->set_style_run_bidi_language(style_id, *definition.run_bidi_language)) {
        return false;
    }

    if (definition.run_rtl.has_value()) {
        if (!this->set_style_run_rtl(style_id, *definition.run_rtl)) {
            return false;
        }
    } else if (!this->clear_style_run_rtl(style_id)) {
        return false;
    }

    this->last_error_info.clear();
    return true;
}

bool Document::ensure_table_style(std::string_view style_id,
                                  const featherdoc::table_style_definition &definition) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before defining table styles");
        return false;
    }

    if (style_id.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id must not be empty when defining table styles",
                       std::string{styles_xml_entry});
        return false;
    }

    if (definition.name.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "table style name must not be empty",
                       std::string{styles_xml_entry});
        return false;
    }

    if (!validate_optional_style_value(this->last_error_info, definition.based_on,
                                       "table style basedOn must not be empty")) {
        return false;
    }

    if (const auto error = this->ensure_styles_part_attached()) {
        return false;
    }

    const auto styles_root = this->styles.child("w:styles");
    if (styles_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info, document_errc::styles_xml_parse_failed,
                       "word/styles.xml does not contain a w:styles root",
                       std::string{styles_xml_entry});
        return false;
    }

    if (const auto existing = find_style_node(styles_root, style_id);
        existing != pugi::xml_node{}) {
        const auto existing_type = std::string_view{existing.attribute("w:type").value()};
        if (!existing_type.empty() && existing_type != "table") {
            set_last_error(this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           "style id '" + std::string{style_id} + "' already exists with type '" +
                               std::string{existing_type} + "'",
                           std::string{styles_xml_entry});
            return false;
        }
    }

    const auto style = ensure_style_node(styles_root, style_id);
    if (style == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to create table style '" + std::string{style_id} + "'",
                       std::string{styles_xml_entry});
        return false;
    }

    if (!apply_common_style_definition(style, style_id, "table", definition.name,
                                       definition.based_on, definition.is_custom,
                                       definition.is_semi_hidden,
                                       definition.is_unhide_when_used,
                                       definition.is_quick_format)) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to update table style '" + std::string{style_id} + "'",
                       std::string{styles_xml_entry});
        return false;
    }

    this->styles_dirty = true;
    this->last_error_info.clear();
    return true;
}

std::optional<std::string> Document::default_run_font_family() {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before reading default run fonts");
        return std::nullopt;
    }

    if (const auto error = this->ensure_styles_loaded()) {
        return std::nullopt;
    }

    const auto styles_root = this->styles.child("w:styles");
    if (styles_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info, document_errc::styles_xml_parse_failed,
                       "word/styles.xml does not contain a w:styles root",
                       std::string{styles_xml_entry});
        return std::nullopt;
    }

    this->last_error_info.clear();
    return read_primary_font_family(
        styles_root.child("w:docDefaults").child("w:rPrDefault").child("w:rPr"));
}

std::optional<std::string> Document::default_run_east_asia_font_family() {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before reading default run fonts");
        return std::nullopt;
    }

    if (const auto error = this->ensure_styles_loaded()) {
        return std::nullopt;
    }

    const auto styles_root = this->styles.child("w:styles");
    if (styles_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info, document_errc::styles_xml_parse_failed,
                       "word/styles.xml does not contain a w:styles root",
                       std::string{styles_xml_entry});
        return std::nullopt;
    }

    this->last_error_info.clear();
    return read_east_asia_font_family(
        styles_root.child("w:docDefaults").child("w:rPrDefault").child("w:rPr"));
}

std::optional<std::string> Document::default_run_language() {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before reading default run language",
                       std::string{styles_xml_entry});
        return std::nullopt;
    }

    if (const auto error = this->ensure_styles_loaded()) {
        return std::nullopt;
    }

    const auto styles_root = this->styles.child("w:styles");
    if (styles_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info, document_errc::styles_xml_parse_failed,
                       "word/styles.xml does not contain a w:styles root",
                       std::string{styles_xml_entry});
        return std::nullopt;
    }

    this->last_error_info.clear();
    return read_primary_language(
        styles_root.child("w:docDefaults").child("w:rPrDefault").child("w:rPr"));
}

std::optional<std::string> Document::default_run_east_asia_language() {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before reading default run language",
                       std::string{styles_xml_entry});
        return std::nullopt;
    }

    if (const auto error = this->ensure_styles_loaded()) {
        return std::nullopt;
    }

    const auto styles_root = this->styles.child("w:styles");
    if (styles_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info, document_errc::styles_xml_parse_failed,
                       "word/styles.xml does not contain a w:styles root",
                       std::string{styles_xml_entry});
        return std::nullopt;
    }

    this->last_error_info.clear();
    return read_east_asia_language(
        styles_root.child("w:docDefaults").child("w:rPrDefault").child("w:rPr"));
}

std::optional<std::string> Document::default_run_bidi_language() {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before reading default run language",
                       std::string{styles_xml_entry});
        return std::nullopt;
    }

    if (const auto error = this->ensure_styles_loaded()) {
        return std::nullopt;
    }

    const auto styles_root = this->styles.child("w:styles");
    if (styles_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info, document_errc::styles_xml_parse_failed,
                       "word/styles.xml does not contain a w:styles root",
                       std::string{styles_xml_entry});
        return std::nullopt;
    }

    this->last_error_info.clear();
    return read_bidi_language(
        styles_root.child("w:docDefaults").child("w:rPrDefault").child("w:rPr"));
}

std::optional<bool> Document::default_run_rtl() {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before reading default run direction",
                       std::string{styles_xml_entry});
        return std::nullopt;
    }

    if (const auto error = this->ensure_styles_loaded()) {
        return std::nullopt;
    }

    const auto styles_root = this->styles.child("w:styles");
    if (styles_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info, document_errc::styles_xml_parse_failed,
                       "word/styles.xml does not contain a w:styles root",
                       std::string{styles_xml_entry});
        return std::nullopt;
    }

    this->last_error_info.clear();
    return read_run_rtl(
        styles_root.child("w:docDefaults").child("w:rPrDefault").child("w:rPr"));
}

std::optional<bool> Document::default_paragraph_bidi() {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before reading default paragraph direction",
                       std::string{styles_xml_entry});
        return std::nullopt;
    }

    if (const auto error = this->ensure_styles_loaded()) {
        return std::nullopt;
    }

    const auto styles_root = this->styles.child("w:styles");
    if (styles_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info, document_errc::styles_xml_parse_failed,
                       "word/styles.xml does not contain a w:styles root",
                       std::string{styles_xml_entry});
        return std::nullopt;
    }

    this->last_error_info.clear();
    return read_paragraph_bidi(
        styles_root.child("w:docDefaults").child("w:pPrDefault").child("w:pPr"));
}

bool Document::set_default_run_font_family(std::string_view font_family) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before editing default run fonts");
        return false;
    }

    if (font_family.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "default run font family must not be empty",
                       std::string{styles_xml_entry});
        return false;
    }

    if (const auto error = this->ensure_styles_part_attached()) {
        return false;
    }

    const auto styles_root = this->styles.child("w:styles");
    if (styles_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info, document_errc::styles_xml_parse_failed,
                       "word/styles.xml does not contain a w:styles root",
                       std::string{styles_xml_entry});
        return false;
    }

    const auto run_properties = ensure_doc_defaults_run_properties_node(styles_root);
    if (run_properties == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to create docDefaults/w:rPrDefault/w:rPr",
                       std::string{styles_xml_entry});
        return false;
    }

    const auto run_fonts = ensure_run_fonts_node(run_properties);
    if (run_fonts == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to create docDefaults default w:rFonts",
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

bool Document::set_default_run_east_asia_font_family(std::string_view font_family) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before editing default run fonts");
        return false;
    }

    if (font_family.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "default eastAsia font family must not be empty",
                       std::string{styles_xml_entry});
        return false;
    }

    if (const auto error = this->ensure_styles_part_attached()) {
        return false;
    }

    const auto styles_root = this->styles.child("w:styles");
    if (styles_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info, document_errc::styles_xml_parse_failed,
                       "word/styles.xml does not contain a w:styles root",
                       std::string{styles_xml_entry});
        return false;
    }

    const auto run_properties = ensure_doc_defaults_run_properties_node(styles_root);
    if (run_properties == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to create docDefaults/w:rPrDefault/w:rPr",
                       std::string{styles_xml_entry});
        return false;
    }

    const auto run_fonts = ensure_run_fonts_node(run_properties);
    if (run_fonts == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to create docDefaults default w:rFonts",
                       std::string{styles_xml_entry});
        return false;
    }

    ensure_attribute_value(run_fonts, "w:eastAsia", font_family);
    this->styles_dirty = true;
    this->last_error_info.clear();
    return true;
}

bool Document::set_default_run_language(std::string_view language) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before editing default run language",
                       std::string{styles_xml_entry});
        return false;
    }

    if (language.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "default run language must not be empty",
                       std::string{styles_xml_entry});
        return false;
    }

    if (const auto error = this->ensure_styles_part_attached()) {
        return false;
    }

    const auto styles_root = this->styles.child("w:styles");
    if (styles_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info, document_errc::styles_xml_parse_failed,
                       "word/styles.xml does not contain a w:styles root",
                       std::string{styles_xml_entry});
        return false;
    }

    const auto run_properties = ensure_doc_defaults_run_properties_node(styles_root);
    if (run_properties == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to create docDefaults/w:rPrDefault/w:rPr",
                       std::string{styles_xml_entry});
        return false;
    }

    const auto run_language = ensure_run_language_node(run_properties);
    if (run_language == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to create docDefaults default w:lang",
                       std::string{styles_xml_entry});
        return false;
    }

    ensure_attribute_value(run_language, "w:val", language);
    this->styles_dirty = true;
    this->last_error_info.clear();
    return true;
}

bool Document::set_default_run_east_asia_language(std::string_view language) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before editing default run language",
                       std::string{styles_xml_entry});
        return false;
    }

    if (language.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "default eastAsia language must not be empty",
                       std::string{styles_xml_entry});
        return false;
    }

    if (const auto error = this->ensure_styles_part_attached()) {
        return false;
    }

    const auto styles_root = this->styles.child("w:styles");
    if (styles_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info, document_errc::styles_xml_parse_failed,
                       "word/styles.xml does not contain a w:styles root",
                       std::string{styles_xml_entry});
        return false;
    }

    const auto run_properties = ensure_doc_defaults_run_properties_node(styles_root);
    if (run_properties == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to create docDefaults/w:rPrDefault/w:rPr",
                       std::string{styles_xml_entry});
        return false;
    }

    const auto run_language = ensure_run_language_node(run_properties);
    if (run_language == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to create docDefaults default w:lang",
                       std::string{styles_xml_entry});
        return false;
    }

    ensure_attribute_value(run_language, "w:eastAsia", language);
    this->styles_dirty = true;
    this->last_error_info.clear();
    return true;
}

bool Document::set_default_run_bidi_language(std::string_view language) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before editing default run language",
                       std::string{styles_xml_entry});
        return false;
    }

    if (language.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "default bidi language must not be empty",
                       std::string{styles_xml_entry});
        return false;
    }

    if (const auto error = this->ensure_styles_part_attached()) {
        return false;
    }

    const auto styles_root = this->styles.child("w:styles");
    if (styles_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info, document_errc::styles_xml_parse_failed,
                       "word/styles.xml does not contain a w:styles root",
                       std::string{styles_xml_entry});
        return false;
    }

    const auto run_properties = ensure_doc_defaults_run_properties_node(styles_root);
    if (run_properties == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to create docDefaults/w:rPrDefault/w:rPr",
                       std::string{styles_xml_entry});
        return false;
    }

    const auto run_language = ensure_run_language_node(run_properties);
    if (run_language == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to create docDefaults default w:lang",
                       std::string{styles_xml_entry});
        return false;
    }

    ensure_attribute_value(run_language, "w:bidi", language);
    this->styles_dirty = true;
    this->last_error_info.clear();
    return true;
}

bool Document::set_default_run_rtl(bool enabled) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before editing default run direction",
                       std::string{styles_xml_entry});
        return false;
    }

    if (const auto error = this->ensure_styles_part_attached()) {
        return false;
    }

    const auto styles_root = this->styles.child("w:styles");
    if (styles_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info, document_errc::styles_xml_parse_failed,
                       "word/styles.xml does not contain a w:styles root",
                       std::string{styles_xml_entry});
        return false;
    }

    const auto run_properties = ensure_doc_defaults_run_properties_node(styles_root);
    if (run_properties == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to create docDefaults/w:rPrDefault/w:rPr",
                       std::string{styles_xml_entry});
        return false;
    }

    const auto rtl = ensure_run_rtl_node(run_properties);
    if (rtl == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to create docDefaults default w:rtl",
                       std::string{styles_xml_entry});
        return false;
    }

    set_on_off_value(rtl, enabled);
    this->styles_dirty = true;
    this->last_error_info.clear();
    return true;
}

bool Document::set_default_paragraph_bidi(bool enabled) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before editing default paragraph direction",
                       std::string{styles_xml_entry});
        return false;
    }

    if (const auto error = this->ensure_styles_part_attached()) {
        return false;
    }

    const auto styles_root = this->styles.child("w:styles");
    if (styles_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info, document_errc::styles_xml_parse_failed,
                       "word/styles.xml does not contain a w:styles root",
                       std::string{styles_xml_entry});
        return false;
    }

    const auto paragraph_properties = ensure_doc_defaults_paragraph_properties_node(styles_root);
    if (paragraph_properties == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to create docDefaults/w:pPrDefault/w:pPr",
                       std::string{styles_xml_entry});
        return false;
    }

    const auto bidi = ensure_paragraph_bidi_node(paragraph_properties);
    if (bidi == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to create docDefaults default w:bidi",
                       std::string{styles_xml_entry});
        return false;
    }

    set_on_off_value(bidi, enabled);
    this->styles_dirty = true;
    this->last_error_info.clear();
    return true;
}

bool Document::clear_default_run_font_family() {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before editing default run fonts");
        return false;
    }

    if (const auto error = this->ensure_styles_loaded()) {
        return false;
    }

    const auto styles_root = this->styles.child("w:styles");
    if (styles_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info, document_errc::styles_xml_parse_failed,
                       "word/styles.xml does not contain a w:styles root",
                       std::string{styles_xml_entry});
        return false;
    }

    if (clear_font_family_attributes(
            styles_root.child("w:docDefaults").child("w:rPrDefault").child("w:rPr"))) {
        this->styles_dirty = true;
    }

    this->last_error_info.clear();
    return true;
}

bool Document::clear_default_run_language() {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before editing default run language",
                       std::string{styles_xml_entry});
        return false;
    }

    if (const auto error = this->ensure_styles_loaded()) {
        return false;
    }

    const auto styles_root = this->styles.child("w:styles");
    if (styles_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info, document_errc::styles_xml_parse_failed,
                       "word/styles.xml does not contain a w:styles root",
                       std::string{styles_xml_entry});
        return false;
    }

    if (clear_language_attributes(
            styles_root.child("w:docDefaults").child("w:rPrDefault").child("w:rPr"))) {
        this->styles_dirty = true;
    }

    this->last_error_info.clear();
    return true;
}

bool Document::clear_default_run_rtl() {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before editing default run direction",
                       std::string{styles_xml_entry});
        return false;
    }

    if (const auto error = this->ensure_styles_loaded()) {
        return false;
    }

    const auto styles_root = this->styles.child("w:styles");
    if (styles_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info, document_errc::styles_xml_parse_failed,
                       "word/styles.xml does not contain a w:styles root",
                       std::string{styles_xml_entry});
        return false;
    }

    if (clear_on_off_child(styles_root.child("w:docDefaults").child("w:rPrDefault").child("w:rPr"),
                           "w:rtl")) {
        this->styles_dirty = true;
    }

    this->last_error_info.clear();
    return true;
}

bool Document::clear_default_paragraph_bidi() {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before editing default paragraph direction",
                       std::string{styles_xml_entry});
        return false;
    }

    if (const auto error = this->ensure_styles_loaded()) {
        return false;
    }

    const auto styles_root = this->styles.child("w:styles");
    if (styles_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info, document_errc::styles_xml_parse_failed,
                       "word/styles.xml does not contain a w:styles root",
                       std::string{styles_xml_entry});
        return false;
    }

    if (clear_on_off_child(styles_root.child("w:docDefaults").child("w:pPrDefault").child("w:pPr"),
                           "w:bidi")) {
        this->styles_dirty = true;
    }

    this->last_error_info.clear();
    return true;
}

std::optional<std::string> Document::style_run_font_family(std::string_view style_id) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before reading style run fonts");
        return std::nullopt;
    }

    if (style_id.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id must not be empty when reading style run fonts",
                       std::string{styles_xml_entry});
        return std::nullopt;
    }

    if (const auto error = this->ensure_styles_loaded()) {
        return std::nullopt;
    }

    const auto styles_root = this->styles.child("w:styles");
    if (styles_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info, document_errc::styles_xml_parse_failed,
                       "word/styles.xml does not contain a w:styles root",
                       std::string{styles_xml_entry});
        return std::nullopt;
    }

    auto style = find_style_node(styles_root, style_id);
    if (style == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id '" + std::string{style_id} +
                           "' was not found in word/styles.xml",
                       std::string{styles_xml_entry});
        return std::nullopt;
    }

    this->last_error_info.clear();
    return read_primary_font_family(style.child("w:rPr"));
}

std::optional<std::string> Document::style_run_east_asia_font_family(
    std::string_view style_id) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before reading style run fonts");
        return std::nullopt;
    }

    if (style_id.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id must not be empty when reading style run fonts",
                       std::string{styles_xml_entry});
        return std::nullopt;
    }

    if (const auto error = this->ensure_styles_loaded()) {
        return std::nullopt;
    }

    const auto styles_root = this->styles.child("w:styles");
    if (styles_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info, document_errc::styles_xml_parse_failed,
                       "word/styles.xml does not contain a w:styles root",
                       std::string{styles_xml_entry});
        return std::nullopt;
    }

    auto style = find_style_node(styles_root, style_id);
    if (style == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id '" + std::string{style_id} +
                           "' was not found in word/styles.xml",
                       std::string{styles_xml_entry});
        return std::nullopt;
    }

    this->last_error_info.clear();
    return read_east_asia_font_family(style.child("w:rPr"));
}

std::optional<std::string> Document::style_run_language(std::string_view style_id) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before reading style run language",
                       std::string{styles_xml_entry});
        return std::nullopt;
    }

    if (style_id.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id must not be empty when reading style run language",
                       std::string{styles_xml_entry});
        return std::nullopt;
    }

    if (const auto error = this->ensure_styles_loaded()) {
        return std::nullopt;
    }

    const auto styles_root = this->styles.child("w:styles");
    if (styles_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info, document_errc::styles_xml_parse_failed,
                       "word/styles.xml does not contain a w:styles root",
                       std::string{styles_xml_entry});
        return std::nullopt;
    }

    auto style = find_style_node(styles_root, style_id);
    if (style == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id '" + std::string{style_id} +
                           "' was not found in word/styles.xml",
                       std::string{styles_xml_entry});
        return std::nullopt;
    }

    this->last_error_info.clear();
    return read_primary_language(style.child("w:rPr"));
}

std::optional<std::string> Document::style_run_east_asia_language(
    std::string_view style_id) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before reading style run language",
                       std::string{styles_xml_entry});
        return std::nullopt;
    }

    if (style_id.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id must not be empty when reading style run language",
                       std::string{styles_xml_entry});
        return std::nullopt;
    }

    if (const auto error = this->ensure_styles_loaded()) {
        return std::nullopt;
    }

    const auto styles_root = this->styles.child("w:styles");
    if (styles_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info, document_errc::styles_xml_parse_failed,
                       "word/styles.xml does not contain a w:styles root",
                       std::string{styles_xml_entry});
        return std::nullopt;
    }

    auto style = find_style_node(styles_root, style_id);
    if (style == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id '" + std::string{style_id} +
                           "' was not found in word/styles.xml",
                       std::string{styles_xml_entry});
        return std::nullopt;
    }

    this->last_error_info.clear();
    return read_east_asia_language(style.child("w:rPr"));
}

std::optional<std::string> Document::style_run_bidi_language(std::string_view style_id) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before reading style run language",
                       std::string{styles_xml_entry});
        return std::nullopt;
    }

    if (style_id.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id must not be empty when reading style run language",
                       std::string{styles_xml_entry});
        return std::nullopt;
    }

    if (const auto error = this->ensure_styles_loaded()) {
        return std::nullopt;
    }

    const auto styles_root = this->styles.child("w:styles");
    if (styles_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info, document_errc::styles_xml_parse_failed,
                       "word/styles.xml does not contain a w:styles root",
                       std::string{styles_xml_entry});
        return std::nullopt;
    }

    auto style = find_style_node(styles_root, style_id);
    if (style == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id '" + std::string{style_id} +
                           "' was not found in word/styles.xml",
                       std::string{styles_xml_entry});
        return std::nullopt;
    }

    this->last_error_info.clear();
    return read_bidi_language(style.child("w:rPr"));
}

std::optional<bool> Document::style_run_rtl(std::string_view style_id) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before reading style run direction",
                       std::string{styles_xml_entry});
        return std::nullopt;
    }

    if (style_id.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id must not be empty when reading style run direction",
                       std::string{styles_xml_entry});
        return std::nullopt;
    }

    if (const auto error = this->ensure_styles_loaded()) {
        return std::nullopt;
    }

    const auto styles_root = this->styles.child("w:styles");
    if (styles_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info, document_errc::styles_xml_parse_failed,
                       "word/styles.xml does not contain a w:styles root",
                       std::string{styles_xml_entry});
        return std::nullopt;
    }

    const auto style = find_style_node(styles_root, style_id);
    if (style == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id '" + std::string{style_id} +
                           "' was not found in word/styles.xml",
                       std::string{styles_xml_entry});
        return std::nullopt;
    }

    this->last_error_info.clear();
    return read_run_rtl(style.child("w:rPr"));
}

std::optional<bool> Document::style_paragraph_bidi(std::string_view style_id) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before reading style paragraph direction",
                       std::string{styles_xml_entry});
        return std::nullopt;
    }

    if (style_id.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id must not be empty when reading style paragraph direction",
                       std::string{styles_xml_entry});
        return std::nullopt;
    }

    if (const auto error = this->ensure_styles_loaded()) {
        return std::nullopt;
    }

    const auto styles_root = this->styles.child("w:styles");
    if (styles_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info, document_errc::styles_xml_parse_failed,
                       "word/styles.xml does not contain a w:styles root",
                       std::string{styles_xml_entry});
        return std::nullopt;
    }

    const auto style = find_style_node(styles_root, style_id);
    if (style == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id '" + std::string{style_id} +
                           "' was not found in word/styles.xml",
                       std::string{styles_xml_entry});
        return std::nullopt;
    }

    this->last_error_info.clear();
    return read_paragraph_bidi(style.child("w:pPr"));
}

bool Document::set_style_run_font_family(std::string_view style_id,
                                         std::string_view font_family) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before editing style run fonts");
        return false;
    }

    if (style_id.empty()) {
        set_last_error(this->last_error_info,
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
        set_last_error(this->last_error_info, document_errc::styles_xml_parse_failed,
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
                       "failed to create w:rPr for style '" + std::string{style_id} + "'",
                       std::string{styles_xml_entry});
        return false;
    }

    const auto run_fonts = ensure_run_fonts_node(run_properties);
    if (run_fonts == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to create w:rFonts for style '" + std::string{style_id} + "'",
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

bool Document::set_style_run_east_asia_font_family(std::string_view style_id,
                                                   std::string_view font_family) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before editing style run fonts");
        return false;
    }

    if (style_id.empty()) {
        set_last_error(this->last_error_info,
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
        set_last_error(this->last_error_info, document_errc::styles_xml_parse_failed,
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
                       "failed to create w:rPr for style '" + std::string{style_id} + "'",
                       std::string{styles_xml_entry});
        return false;
    }

    const auto run_fonts = ensure_run_fonts_node(run_properties);
    if (run_fonts == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to create w:rFonts for style '" + std::string{style_id} + "'",
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
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before editing style run language",
                       std::string{styles_xml_entry});
        return false;
    }

    if (style_id.empty()) {
        set_last_error(this->last_error_info,
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
        set_last_error(this->last_error_info, document_errc::styles_xml_parse_failed,
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
                       "failed to create w:rPr for style '" + std::string{style_id} + "'",
                       std::string{styles_xml_entry});
        return false;
    }

    const auto run_language = ensure_run_language_node(run_properties);
    if (run_language == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to create w:lang for style '" + std::string{style_id} + "'",
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
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before editing style run language",
                       std::string{styles_xml_entry});
        return false;
    }

    if (style_id.empty()) {
        set_last_error(this->last_error_info,
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
        set_last_error(this->last_error_info, document_errc::styles_xml_parse_failed,
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
                       "failed to create w:rPr for style '" + std::string{style_id} + "'",
                       std::string{styles_xml_entry});
        return false;
    }

    const auto run_language = ensure_run_language_node(run_properties);
    if (run_language == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to create w:lang for style '" + std::string{style_id} + "'",
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
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before editing style run language",
                       std::string{styles_xml_entry});
        return false;
    }

    if (style_id.empty()) {
        set_last_error(this->last_error_info,
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
        set_last_error(this->last_error_info, document_errc::styles_xml_parse_failed,
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
                       "failed to create w:rPr for style '" + std::string{style_id} + "'",
                       std::string{styles_xml_entry});
        return false;
    }

    const auto run_language = ensure_run_language_node(run_properties);
    if (run_language == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to create w:lang for style '" + std::string{style_id} + "'",
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
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before editing style run direction",
                       std::string{styles_xml_entry});
        return false;
    }

    if (style_id.empty()) {
        set_last_error(this->last_error_info,
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
        set_last_error(this->last_error_info, document_errc::styles_xml_parse_failed,
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
                       "failed to create w:rPr for style '" + std::string{style_id} + "'",
                       std::string{styles_xml_entry});
        return false;
    }

    const auto rtl = ensure_run_rtl_node(run_properties);
    if (rtl == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to create w:rtl for style '" + std::string{style_id} + "'",
                       std::string{styles_xml_entry});
        return false;
    }

    set_on_off_value(rtl, enabled);
    this->styles_dirty = true;
    this->last_error_info.clear();
    return true;
}

bool Document::set_style_paragraph_bidi(std::string_view style_id, bool enabled) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before editing style paragraph direction",
                       std::string{styles_xml_entry});
        return false;
    }

    if (style_id.empty()) {
        set_last_error(this->last_error_info,
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
        set_last_error(this->last_error_info, document_errc::styles_xml_parse_failed,
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

    const auto paragraph_properties = ensure_style_paragraph_properties_node(style);
    if (paragraph_properties == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to create w:pPr for style '" + std::string{style_id} + "'",
                       std::string{styles_xml_entry});
        return false;
    }

    const auto bidi = ensure_paragraph_bidi_node(paragraph_properties);
    if (bidi == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to create w:bidi for style '" + std::string{style_id} + "'",
                       std::string{styles_xml_entry});
        return false;
    }

    set_on_off_value(bidi, enabled);
    this->styles_dirty = true;
    this->last_error_info.clear();
    return true;
}

bool Document::clear_style_run_font_family(std::string_view style_id) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before editing style run fonts");
        return false;
    }

    if (style_id.empty()) {
        set_last_error(this->last_error_info,
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
        set_last_error(this->last_error_info, document_errc::styles_xml_parse_failed,
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

bool Document::clear_style_run_language(std::string_view style_id) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before editing style run language",
                       std::string{styles_xml_entry});
        return false;
    }

    if (style_id.empty()) {
        set_last_error(this->last_error_info,
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
        set_last_error(this->last_error_info, document_errc::styles_xml_parse_failed,
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

bool Document::clear_style_run_rtl(std::string_view style_id) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before editing style run direction",
                       std::string{styles_xml_entry});
        return false;
    }

    if (style_id.empty()) {
        set_last_error(this->last_error_info,
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
        set_last_error(this->last_error_info, document_errc::styles_xml_parse_failed,
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
                       "call open() or create_empty() before editing style paragraph direction",
                       std::string{styles_xml_entry});
        return false;
    }

    if (style_id.empty()) {
        set_last_error(this->last_error_info,
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
        set_last_error(this->last_error_info, document_errc::styles_xml_parse_failed,
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
