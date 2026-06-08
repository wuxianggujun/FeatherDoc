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

auto ensure_paragraph_properties_node(pugi::xml_node paragraph)
    -> pugi::xml_node {
    if (paragraph == pugi::xml_node{}) {
        return {};
    }

    auto paragraph_properties = paragraph.child("w:pPr");
    if (paragraph_properties != pugi::xml_node{}) {
        return paragraph_properties;
    }

    if (const auto first_child = paragraph.first_child();
        first_child != pugi::xml_node{}) {
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

    if (const auto first_child = run.first_child();
        first_child != pugi::xml_node{}) {
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

auto ensure_style_paragraph_properties_node(pugi::xml_node style)
    -> pugi::xml_node {
    if (style == pugi::xml_node{}) {
        return {};
    }

    auto paragraph_properties = style.child("w:pPr");
    if (paragraph_properties != pugi::xml_node{}) {
        return paragraph_properties;
    }

    if (const auto run_properties = style.child("w:rPr");
        run_properties != pugi::xml_node{}) {
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

    if (const auto first_child = run_properties.first_child();
        first_child != pugi::xml_node{}) {
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

    if (const auto first_child = run_properties.first_child();
        first_child != pugi::xml_node{}) {
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

    if (const auto first_child = run_properties.first_child();
        first_child != pugi::xml_node{}) {
        return run_properties.insert_child_before("w:rtl", first_child);
    }

    return run_properties.append_child("w:rtl");
}

auto ensure_run_text_color_node(pugi::xml_node run_properties)
    -> pugi::xml_node {
    if (run_properties == pugi::xml_node{}) {
        return {};
    }

    auto color = run_properties.child("w:color");
    if (color != pugi::xml_node{}) {
        return color;
    }

    return run_properties.append_child("w:color");
}

auto ensure_run_bold_node(pugi::xml_node run_properties) -> pugi::xml_node {
    if (run_properties == pugi::xml_node{}) {
        return {};
    }

    auto bold = run_properties.child("w:b");
    if (bold != pugi::xml_node{}) {
        return bold;
    }

    if (const auto italic = run_properties.child("w:i");
        italic != pugi::xml_node{}) {
        return run_properties.insert_child_before("w:b", italic);
    }
    if (const auto color = run_properties.child("w:color");
        color != pugi::xml_node{}) {
        return run_properties.insert_child_before("w:b", color);
    }

    return run_properties.append_child("w:b");
}

auto ensure_run_italic_node(pugi::xml_node run_properties) -> pugi::xml_node {
    if (run_properties == pugi::xml_node{}) {
        return {};
    }

    auto italic = run_properties.child("w:i");
    if (italic != pugi::xml_node{}) {
        return italic;
    }

    if (const auto bold = run_properties.child("w:b");
        bold != pugi::xml_node{}) {
        return run_properties.insert_child_after("w:i", bold);
    }
    if (const auto color = run_properties.child("w:color");
        color != pugi::xml_node{}) {
        return run_properties.insert_child_before("w:i", color);
    }
    if (const auto size = run_properties.child("w:sz");
        size != pugi::xml_node{}) {
        return run_properties.insert_child_before("w:i", size);
    }

    return run_properties.append_child("w:i");
}

auto ensure_run_underline_node(pugi::xml_node run_properties)
    -> pugi::xml_node {
    if (run_properties == pugi::xml_node{}) {
        return {};
    }

    auto underline = run_properties.child("w:u");
    if (underline != pugi::xml_node{}) {
        return underline;
    }

    if (const auto size = run_properties.child("w:sz");
        size != pugi::xml_node{}) {
        return run_properties.insert_child_before("w:u", size);
    }
    if (const auto italic = run_properties.child("w:i");
        italic != pugi::xml_node{}) {
        return run_properties.insert_child_after("w:u", italic);
    }
    if (const auto bold = run_properties.child("w:b");
        bold != pugi::xml_node{}) {
        return run_properties.insert_child_after("w:u", bold);
    }
    if (const auto color = run_properties.child("w:color");
        color != pugi::xml_node{}) {
        return run_properties.insert_child_after("w:u", color);
    }

    return run_properties.append_child("w:u");
}

auto ensure_run_strikethrough_node(pugi::xml_node run_properties)
    -> pugi::xml_node {
    if (run_properties == pugi::xml_node{}) {
        return {};
    }

    auto strike = run_properties.child("w:strike");
    if (strike != pugi::xml_node{}) {
        return strike;
    }

    if (const auto underline = run_properties.child("w:u");
        underline != pugi::xml_node{}) {
        return run_properties.insert_child_before("w:strike", underline);
    }
    if (const auto size = run_properties.child("w:sz");
        size != pugi::xml_node{}) {
        return run_properties.insert_child_before("w:strike", size);
    }
    if (const auto italic = run_properties.child("w:i");
        italic != pugi::xml_node{}) {
        return run_properties.insert_child_after("w:strike", italic);
    }
    if (const auto bold = run_properties.child("w:b");
        bold != pugi::xml_node{}) {
        return run_properties.insert_child_after("w:strike", bold);
    }

    return run_properties.append_child("w:strike");
}

auto ensure_run_vertical_align_node(pugi::xml_node run_properties)
    -> pugi::xml_node {
    if (run_properties == pugi::xml_node{}) {
        return {};
    }

    auto vertical_align = run_properties.child("w:vertAlign");
    if (vertical_align != pugi::xml_node{}) {
        return vertical_align;
    }

    if (const auto font_size = run_properties.child("w:szCs");
        font_size != pugi::xml_node{}) {
        return run_properties.insert_child_after("w:vertAlign", font_size);
    }
    if (const auto font_size = run_properties.child("w:sz");
        font_size != pugi::xml_node{}) {
        return run_properties.insert_child_after("w:vertAlign", font_size);
    }
    if (const auto underline = run_properties.child("w:u");
        underline != pugi::xml_node{}) {
        return run_properties.insert_child_after("w:vertAlign", underline);
    }

    return run_properties.append_child("w:vertAlign");
}

auto ensure_run_font_size_node(pugi::xml_node run_properties,
                               const char *child_name) -> pugi::xml_node {
    if (run_properties == pugi::xml_node{}) {
        return {};
    }

    auto font_size = run_properties.child(child_name);
    if (font_size != pugi::xml_node{}) {
        return font_size;
    }

    const auto name = std::string_view{child_name};
    if (name == "w:szCs") {
        if (const auto primary_size = run_properties.child("w:sz");
            primary_size != pugi::xml_node{}) {
            return run_properties.insert_child_after(child_name, primary_size);
        }
    } else if (const auto complex_size = run_properties.child("w:szCs");
               complex_size != pugi::xml_node{}) {
        return run_properties.insert_child_before(child_name, complex_size);
    }

    if (const auto underline = run_properties.child("w:u");
        underline != pugi::xml_node{}) {
        return run_properties.insert_child_after(child_name, underline);
    }
    if (const auto color = run_properties.child("w:color");
        color != pugi::xml_node{}) {
        return run_properties.insert_child_after(child_name, color);
    }
    if (const auto italic = run_properties.child("w:i");
        italic != pugi::xml_node{}) {
        return run_properties.insert_child_after(child_name, italic);
    }
    if (const auto bold = run_properties.child("w:b");
        bold != pugi::xml_node{}) {
        return run_properties.insert_child_after(child_name, bold);
    }

    return run_properties.append_child(child_name);
}

auto ensure_paragraph_bidi_node(pugi::xml_node paragraph_properties)
    -> pugi::xml_node {
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

    if (const auto first_child = styles_root.first_child();
        first_child != pugi::xml_node{}) {
        return styles_root.insert_child_before("w:docDefaults", first_child);
    }

    return styles_root.append_child("w:docDefaults");
}

auto ensure_doc_defaults_run_properties_node(pugi::xml_node styles_root)
    -> pugi::xml_node {
    auto doc_defaults = ensure_doc_defaults_node(styles_root);
    if (doc_defaults == pugi::xml_node{}) {
        return {};
    }

    auto run_properties_default = doc_defaults.child("w:rPrDefault");
    if (run_properties_default == pugi::xml_node{}) {
        if (const auto first_child = doc_defaults.first_child();
            first_child != pugi::xml_node{}) {
            run_properties_default =
                doc_defaults.insert_child_before("w:rPrDefault", first_child);
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
        if (const auto run_properties_default =
                doc_defaults.child("w:rPrDefault");
            run_properties_default != pugi::xml_node{}) {
            paragraph_properties_default = doc_defaults.insert_child_after(
                "w:pPrDefault", run_properties_default);
        } else if (const auto first_child = doc_defaults.first_child();
                   first_child != pugi::xml_node{}) {
            paragraph_properties_default =
                doc_defaults.insert_child_before("w:pPrDefault", first_child);
        } else {
            paragraph_properties_default =
                doc_defaults.append_child("w:pPrDefault");
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
        return paragraph_properties_default.insert_child_before("w:pPr",
                                                                first_child);
    }

    return paragraph_properties_default.append_child("w:pPr");
}

auto find_style_node(pugi::xml_node styles_root,
                     std::string_view style_id) -> pugi::xml_node;

auto style_properties_anchor(pugi::xml_node style) -> pugi::xml_node {
    if (const auto paragraph_properties = style.child("w:pPr");
        paragraph_properties != pugi::xml_node{}) {
        return paragraph_properties;
    }
    if (const auto run_properties = style.child("w:rPr");
        run_properties != pugi::xml_node{}) {
        return run_properties;
    }
    if (const auto table_properties = style.child("w:tblPr");
        table_properties != pugi::xml_node{}) {
        return table_properties;
    }
    if (const auto table_cell_properties = style.child("w:tcPr");
        table_cell_properties != pugi::xml_node{}) {
        return table_cell_properties;
    }
    return style.child("w:tblStylePr");
}

auto ensure_style_child_before_properties(
    pugi::xml_node style, const char *child_name) -> pugi::xml_node {
    if (style == pugi::xml_node{}) {
        return {};
    }

    auto child = style.child(child_name);
    if (child != pugi::xml_node{}) {
        return child;
    }

    if (const auto anchor = style_properties_anchor(style);
        anchor != pugi::xml_node{}) {
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

    if (const auto based_on = style.child("w:basedOn");
        based_on != pugi::xml_node{}) {
        return style.insert_child_after("w:next", based_on);
    }
    if (const auto name = style.child("w:name"); name != pugi::xml_node{}) {
        return style.insert_child_after("w:next", name);
    }

    return ensure_style_child_before_properties(style, "w:next");
}

auto ensure_style_outline_level_node(pugi::xml_node paragraph_properties)
    -> pugi::xml_node {
    if (paragraph_properties == pugi::xml_node{}) {
        return {};
    }

    auto outline_level = paragraph_properties.child("w:outlineLvl");
    if (outline_level != pugi::xml_node{}) {
        return outline_level;
    }

    if (const auto bidi = paragraph_properties.child("w:bidi");
        bidi != pugi::xml_node{}) {
        return paragraph_properties.insert_child_before("w:outlineLvl", bidi);
    }

    if (const auto first_child = paragraph_properties.first_child();
        first_child != pugi::xml_node{}) {
        return paragraph_properties.insert_child_before("w:outlineLvl",
                                                        first_child);
    }

    return paragraph_properties.append_child("w:outlineLvl");
}

auto ensure_style_node(pugi::xml_node styles_root,
                       std::string_view style_id) -> pugi::xml_node {
    if (styles_root == pugi::xml_node{}) {
        return {};
    }

    if (auto style = find_style_node(styles_root, style_id);
        style != pugi::xml_node{}) {
        return style;
    }

    auto style = styles_root.append_child("w:style");
    if (style != pugi::xml_node{}) {
        ensure_attribute_value(style, "w:styleId", style_id);
    }
    return style;
}

auto find_style_node(pugi::xml_node styles_root,
                     std::string_view style_id) -> pugi::xml_node {
    for (auto style = styles_root.child("w:style"); style != pugi::xml_node{};
         style = style.next_sibling("w:style")) {
        if (std::string_view{style.attribute("w:styleId").value()} ==
            style_id) {
            return style;
        }
    }

    return {};
}

auto style_node_id(pugi::xml_node style) -> std::string_view {
    return style.attribute("w:styleId").value();
}

auto build_style_inheritance_chain(
    pugi::xml_node styles_root, pugi::xml_node style,
    featherdoc::document_error_info &last_error_info,
    std::vector<pugi::xml_node> &chain) -> bool {
    chain.clear();
    while (style != pugi::xml_node{}) {
        const auto current_style_id = style_node_id(style);
        for (const auto &visited_style : chain) {
            if (style_node_id(visited_style) == current_style_id) {
                set_last_error(
                    last_error_info,
                    std::make_error_code(std::errc::invalid_argument),
                    "style inheritance cycle detected at style '" +
                        std::string{current_style_id} + "'",
                    std::string{styles_xml_entry});
                return false;
            }
        }

        chain.push_back(style);

        const auto based_on = std::string_view{
            style.child("w:basedOn").attribute("w:val").value()};
        if (based_on.empty()) {
            return true;
        }

        const auto parent_style = find_style_node(styles_root, based_on);
        if (parent_style == pugi::xml_node{}) {
            set_last_error(last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           "style id '" + std::string{current_style_id} +
                               "' references missing basedOn style '" +
                               std::string{based_on} + "'",
                           std::string{styles_xml_entry});
            return false;
        }

        style = parent_style;
    }

    return true;
}

template <typename Reader>
auto resolve_style_string_property(const std::vector<pugi::xml_node> &chain,
                                   Reader &&reader)
    -> featherdoc::resolved_style_string_property {
    for (const auto &style : chain) {
        if (const auto value = reader(style); value.has_value()) {
            featherdoc::resolved_style_string_property property{};
            property.value = value;
            property.source_style_id = std::string{style_node_id(style)};
            return property;
        }
    }

    return {};
}

template <typename Reader>
auto resolve_style_bool_property(const std::vector<pugi::xml_node> &chain,
                                 Reader &&reader)
    -> featherdoc::resolved_style_bool_property {
    for (const auto &style : chain) {
        if (const auto value = reader(style); value.has_value()) {
            featherdoc::resolved_style_bool_property property{};
            property.value = value;
            property.source_style_id = std::string{style_node_id(style)};
            return property;
        }
    }

    return {};
}

template <typename Reader>
auto resolve_style_double_property(const std::vector<pugi::xml_node> &chain,
                                   Reader &&reader)
    -> featherdoc::resolved_style_double_property {
    for (const auto &style : chain) {
        if (const auto value = reader(style); value.has_value()) {
            featherdoc::resolved_style_double_property property{};
            property.value = value;
            property.source_style_id = std::string{style_node_id(style)};
            return property;
        }
    }

    return {};
}

auto read_on_off_attribute(pugi::xml_node node,
                           const char *attribute_name) -> bool {
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

auto parse_u32_attribute_value(const char *text)
    -> std::optional<std::uint32_t> {
    if (text == nullptr || *text == '\0') {
        return std::nullopt;
    }

    char *end = nullptr;
    const auto value = std::strtoul(text, &end, 10);
    if (end == text || *end != '\0' ||
        value > static_cast<unsigned long>(
                    std::numeric_limits<std::uint32_t>::max())) {
        return std::nullopt;
    }

    return static_cast<std::uint32_t>(value);
}

auto parse_half_point_size(const char *text) -> std::optional<double> {
    if (text == nullptr || *text == '\0') {
        return std::nullopt;
    }

    char *end = nullptr;
    const auto value = std::strtod(text, &end);
    if (end == text || *end != '\0' || value <= 0.0) {
        return std::nullopt;
    }

    return value / 2.0;
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

auto summarize_style_node(pugi::xml_node style) -> featherdoc::style_summary {
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
    summary.is_semi_hidden =
        read_on_off_value(style.child("w:semiHidden")).value_or(false);
    summary.is_unhide_when_used =
        read_on_off_value(style.child("w:unhideWhenUsed")).value_or(false);
    summary.is_quick_format =
        read_on_off_value(style.child("w:qFormat")).value_or(false);
    return summary;
}

auto node_style_reference_matches(pugi::xml_node node,
                                  const char *properties_name,
                                  const char *style_name,
                                  std::string_view style_id) -> bool {
    const auto style_value =
        node.child(properties_name).child(style_name).attribute("w:val");
    if (style_value == pugi::xml_attribute{}) {
        return false;
    }

    return std::string_view{style_value.value()} == style_id;
}

auto rewrite_style_reference_value(pugi::xml_attribute value_attribute,
                                   std::string_view old_style_id,
                                   std::string_view new_style_id) -> bool {
    if (value_attribute == pugi::xml_attribute{} ||
        std::string_view{value_attribute.value()} != old_style_id) {
        return false;
    }

    value_attribute.set_value(std::string{new_style_id}.c_str());
    return true;
}

auto rewrite_style_reference_child(pugi::xml_node node,
                                   const char *properties_name,
                                   const char *style_name,
                                   std::string_view old_style_id,
                                   std::string_view new_style_id) -> bool {
    return rewrite_style_reference_value(
        node.child(properties_name).child(style_name).attribute("w:val"),
        old_style_id, new_style_id);
}

auto rewrite_style_metadata_reference(pugi::xml_node style,
                                      const char *child_name,
                                      std::string_view old_style_id,
                                      std::string_view new_style_id) -> bool {
    return rewrite_style_reference_value(
        style.child(child_name).attribute("w:val"), old_style_id, new_style_id);
}

auto rewrite_style_metadata_references_in_styles(
    pugi::xml_node styles_root, std::string_view old_style_id,
    std::string_view new_style_id) -> bool {
    auto changed = false;
    for (auto style = styles_root.child("w:style"); style != pugi::xml_node{};
         style = style.next_sibling("w:style")) {
        changed = rewrite_style_metadata_reference(
                      style, "w:basedOn", old_style_id, new_style_id) ||
                  changed;
        changed = rewrite_style_metadata_reference(
                      style, "w:next", old_style_id, new_style_id) ||
                  changed;
        changed = rewrite_style_metadata_reference(
                      style, "w:link", old_style_id, new_style_id) ||
                  changed;
    }
    return changed;
}

void collect_style_reference_value(pugi::xml_attribute value_attribute,
                                   std::unordered_set<std::string> &style_ids) {
    if (value_attribute == pugi::xml_attribute{} ||
        value_attribute.value()[0] == '\0') {
        return;
    }

    style_ids.emplace(value_attribute.value());
}

void collect_style_reference_child(pugi::xml_node node,
                                   const char *properties_name,
                                   const char *style_name,
                                   std::unordered_set<std::string> &style_ids) {
    collect_style_reference_value(
        node.child(properties_name).child(style_name).attribute("w:val"),
        style_ids);
}

void collect_style_references_in_tree(
    pugi::xml_node node, std::unordered_set<std::string> &style_ids) {
    if (node == pugi::xml_node{}) {
        return;
    }

    const auto node_name = std::string_view{node.name()};
    if (node_name == "w:p") {
        collect_style_reference_child(node, "w:pPr", "w:pStyle", style_ids);
    } else if (node_name == "w:r") {
        collect_style_reference_child(node, "w:rPr", "w:rStyle", style_ids);
    } else if (node_name == "w:tbl") {
        collect_style_reference_child(node, "w:tblPr", "w:tblStyle", style_ids);
    }

    for (auto child = node.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        collect_style_references_in_tree(child, style_ids);
    }
}

void append_style_dependency_ids(pugi::xml_node style,
                                 std::vector<std::string> &dependencies) {
    for (const auto *child_name : {"w:basedOn", "w:next", "w:link"}) {
        const auto value = style.child(child_name).attribute("w:val");
        if (value == pugi::xml_attribute{} || value.value()[0] == '\0') {
            continue;
        }
        dependencies.emplace_back(value.value());
    }
}

auto rewrite_style_references_in_tree(pugi::xml_node node,
                                      std::string_view old_style_id,
                                      std::string_view new_style_id) -> bool {
    if (node == pugi::xml_node{}) {
        return false;
    }

    auto changed = false;
    const auto node_name = std::string_view{node.name()};
    if (node_name == "w:p") {
        changed = rewrite_style_reference_child(node, "w:pPr", "w:pStyle",
                                                old_style_id, new_style_id) ||
                  changed;
    } else if (node_name == "w:r") {
        changed = rewrite_style_reference_child(node, "w:rPr", "w:rStyle",
                                                old_style_id, new_style_id) ||
                  changed;
    } else if (node_name == "w:tbl") {
        changed = rewrite_style_reference_child(node, "w:tblPr", "w:tblStyle",
                                                old_style_id, new_style_id) ||
                  changed;
    }

    for (auto child = node.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        changed = rewrite_style_references_in_tree(child, old_style_id,
                                                   new_style_id) ||
                  changed;
    }

    return changed;
}

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
        set_last_error(this->last_error_info,
                       document_errc::styles_xml_parse_failed,
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
            numbering_ready =
                this->numbering.child("w:numbering") != pugi::xml_node{};
        }
        break;
    }

    const auto apply_numbering_lookup =
        [this, numbering_ready](featherdoc::style_summary &summary) {
            if (!numbering_ready || !summary.numbering.has_value() ||
                !summary.numbering->num_id.has_value()) {
                return;
            }

            const auto lookup =
                this->find_numbering_instance(*summary.numbering->num_id);
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

std::optional<featherdoc::style_summary>
Document::find_style(std::string_view style_id) {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
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

    auto styles_root = this->styles.child("w:styles");
    if (styles_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       document_errc::styles_xml_parse_failed,
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
        const auto lookup =
            this->find_numbering_instance(*summary.numbering->num_id);
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

std::optional<featherdoc::resolved_style_properties_summary>
Document::resolve_style_properties(std::string_view style_id) {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before resolving style properties",
            std::string{styles_xml_entry});
        return std::nullopt;
    }

    if (style_id.empty()) {
        set_last_error(
            this->last_error_info,
            std::make_error_code(std::errc::invalid_argument),
            "style id must not be empty when resolving style properties",
            std::string{styles_xml_entry});
        return std::nullopt;
    }

    if (const auto error = this->ensure_styles_loaded()) {
        return std::nullopt;
    }

    const auto styles_root = this->styles.child("w:styles");
    if (styles_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       document_errc::styles_xml_parse_failed,
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

    std::vector<pugi::xml_node> chain;
    if (!build_style_inheritance_chain(styles_root, style,
                                       this->last_error_info, chain)) {
        return std::nullopt;
    }

    featherdoc::resolved_style_properties_summary summary{};
    summary.style_id = std::string{style_id};
    summary.type_name = style.attribute("w:type").value();
    summary.kind = decode_style_kind(summary.type_name);
    if (const auto based_on = style.child("w:basedOn").attribute("w:val");
        based_on != pugi::xml_attribute{} && based_on.value()[0] != '\0') {
        summary.based_on = std::string{based_on.value()};
    }

    summary.inheritance_chain.reserve(chain.size());
    for (const auto &visited_style : chain) {
        summary.inheritance_chain.emplace_back(
            std::string{style_node_id(visited_style)});
    }

    summary.run_font_family =
        resolve_style_string_property(chain, [](pugi::xml_node node) {
            return read_primary_font_family(node.child("w:rPr"));
        });
    summary.run_east_asia_font_family =
        resolve_style_string_property(chain, [](pugi::xml_node node) {
            return read_east_asia_font_family(node.child("w:rPr"));
        });
    summary.run_text_color =
        resolve_style_string_property(chain, [](pugi::xml_node node) {
            return read_run_text_color(node.child("w:rPr"));
        });
    summary.run_bold =
        resolve_style_bool_property(chain, [](pugi::xml_node node) {
            return read_run_bold(node.child("w:rPr"));
        });
    summary.run_italic =
        resolve_style_bool_property(chain, [](pugi::xml_node node) {
            return read_run_italic(node.child("w:rPr"));
        });
    summary.run_strikethrough =
        resolve_style_bool_property(chain, [](pugi::xml_node node) {
            return read_run_strikethrough(node.child("w:rPr"));
        });
    summary.run_underline =
        resolve_style_bool_property(chain, [](pugi::xml_node node) {
            return read_run_underline(node.child("w:rPr"));
        });
    summary.run_superscript =
        resolve_style_bool_property(chain, [](pugi::xml_node node) {
            return read_vertical_align_value(
                node.child("w:rPr").child("w:vertAlign"), "superscript");
        });
    summary.run_subscript =
        resolve_style_bool_property(chain, [](pugi::xml_node node) {
            return read_vertical_align_value(
                node.child("w:rPr").child("w:vertAlign"), "subscript");
        });
    summary.run_font_size_points =
        resolve_style_double_property(chain, [](pugi::xml_node node) {
            return read_run_font_size_points(node.child("w:rPr"));
        });
    summary.run_language =
        resolve_style_string_property(chain, [](pugi::xml_node node) {
            return read_primary_language(node.child("w:rPr"));
        });
    summary.run_east_asia_language =
        resolve_style_string_property(chain, [](pugi::xml_node node) {
            return read_east_asia_language(node.child("w:rPr"));
        });
    summary.run_bidi_language =
        resolve_style_string_property(chain, [](pugi::xml_node node) {
            return read_bidi_language(node.child("w:rPr"));
        });
    summary.run_rtl =
        resolve_style_bool_property(chain, [](pugi::xml_node node) {
            return read_run_rtl(node.child("w:rPr"));
        });
    summary.paragraph_bidi =
        resolve_style_bool_property(chain, [](pugi::xml_node node) {
            return read_paragraph_bidi(node.child("w:pPr"));
        });

    this->last_error_info.clear();
    return summary;
}

std::optional<featherdoc::style_usage_summary>
Document::find_style_usage(std::string_view style_id) {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
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
        set_last_error(this->last_error_info,
                       document_errc::styles_xml_parse_failed,
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
        set_last_error(this->last_error_info,
                       document_errc::document_xml_parse_failed,
                       "word/document.xml does not contain a w:document root",
                       std::string{document_xml_entry});
        return std::nullopt;
    }

    auto usage = featherdoc::style_usage_summary{};
    usage.style_id = std::string{style_id};
    collect_body_style_usage(document_root.child("w:body"), style_id,
                             usage.body, &usage.hits);

    for (const auto &part : this->header_parts) {
        if (!part) {
            continue;
        }

        auto part_ordinal = std::size_t{0};
        auto node_ordinals = style_usage_node_ordinals{};
        collect_style_usage(part->xml.child("w:hdr"), style_id, usage.header,
                            featherdoc::style_usage_part_kind::header,
                            part->entry_name, &usage.hits, part_ordinal,
                            node_ordinals);
    }

    for (const auto &part : this->footer_parts) {
        if (!part) {
            continue;
        }

        auto part_ordinal = std::size_t{0};
        auto node_ordinals = style_usage_node_ordinals{};
        collect_style_usage(part->xml.child("w:ftr"), style_id, usage.footer,
                            featherdoc::style_usage_part_kind::footer,
                            part->entry_name, &usage.hits, part_ordinal,
                            node_ordinals);
    }

    const auto build_part_references = [&](const char *reference_name,
                                           const auto &parts) {
        auto references_by_relationship_id = std::unordered_map<
            std::string, std::vector<featherdoc::style_usage_hit_reference>>{};
        const auto section_total = this->section_count();
        for (std::size_t section_index = 0; section_index < section_total;
             ++section_index) {
            const auto section_properties =
                this->section_properties(section_index);
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
                    style_usage_reference_kind(
                        reference.attribute("w:type").value()));
            }
        }

        auto references_by_entry_name = std::unordered_map<
            std::string, std::vector<featherdoc::style_usage_hit_reference>>{};
        for (const auto &part : parts) {
            if (!part || part->entry_name.empty() ||
                part->relationship_id.empty()) {
                continue;
            }

            const auto iterator =
                references_by_relationship_id.find(part->relationship_id);
            if (iterator == references_by_relationship_id.end()) {
                continue;
            }

            references_by_entry_name.emplace(part->entry_name,
                                             iterator->second);
        }

        return references_by_entry_name;
    };

    const auto header_references =
        build_part_references("w:headerReference", this->header_parts);
    const auto footer_references =
        build_part_references("w:footerReference", this->footer_parts);
    apply_style_usage_hit_references(usage.hits, header_references,
                                     footer_references);

    finalize_style_usage_totals(usage);

    this->last_error_info.clear();
    return usage;
}

std::optional<featherdoc::style_usage_report> Document::list_style_usage() {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before listing style usage",
            std::string{styles_xml_entry});
        return std::nullopt;
    }

    auto styles = this->list_styles();
    if (const auto &error_info = this->last_error(); error_info.code) {
        return std::nullopt;
    }

    auto report = featherdoc::style_usage_report{};
    report.style_count = styles.size();
    report.entries.reserve(styles.size());

    for (auto &style : styles) {
        auto usage = featherdoc::style_usage_summary{};
        usage.style_id = style.style_id;

        if (!style.style_id.empty()) {
            auto style_usage = this->find_style_usage(style.style_id);
            if (!style_usage.has_value()) {
                return std::nullopt;
            }
            usage = std::move(*style_usage);
        }

        const auto reference_count = usage.total_count();
        report.total_reference_count += reference_count;
        if (reference_count == 0U) {
            ++report.unused_style_count;
        } else {
            ++report.used_style_count;
        }

        report.entries.push_back(featherdoc::style_usage_report_entry{
            std::move(style), std::move(usage)});
    }

    this->last_error_info.clear();
    return report;
}

std::optional<featherdoc::style_refactor_plan> Document::plan_style_refactor(
    const std::vector<featherdoc::style_refactor_request> &requests) {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before planning style refactors",
            std::string{styles_xml_entry});
        return std::nullopt;
    }

    auto styles = this->list_styles();
    if (const auto &error_info = this->last_error(); error_info.code) {
        return std::nullopt;
    }

    auto styles_by_id =
        std::unordered_map<std::string, featherdoc::style_summary>{};
    styles_by_id.reserve(styles.size());
    for (auto &style : styles) {
        if (!style.style_id.empty()) {
            styles_by_id.emplace(style.style_id, std::move(style));
        }
    }

    auto plan = featherdoc::style_refactor_plan{};
    plan.operation_count = requests.size();
    plan.operations.reserve(requests.size());

    const auto append_issue =
        [](featherdoc::style_refactor_operation_plan &operation,
           std::string code, std::string message) {
            operation.issues.push_back(featherdoc::style_refactor_issue{
                std::move(code), std::move(message)});
        };

    for (const auto &request : requests) {
        auto operation = featherdoc::style_refactor_operation_plan{};
        operation.action = request.action;
        operation.source_style_id = request.source_style_id;
        operation.target_style_id = request.target_style_id;

        if (operation.source_style_id.empty()) {
            append_issue(operation, "empty_source_style_id",
                         "source style id must not be empty");
        }
        if (operation.target_style_id.empty()) {
            append_issue(operation, "empty_target_style_id",
                         "target style id must not be empty");
        }
        if (!operation.source_style_id.empty() &&
            operation.source_style_id == operation.target_style_id) {
            append_issue(operation, "same_style_id",
                         "source and target style ids must be different");
        }

        if (!operation.source_style_id.empty()) {
            const auto source_iterator =
                styles_by_id.find(operation.source_style_id);
            if (source_iterator == styles_by_id.end()) {
                append_issue(operation, "missing_source_style",
                             "source style id '" + operation.source_style_id +
                                 "' was not found in word/styles.xml");
            } else {
                operation.source_style = source_iterator->second;
                if (source_iterator->second.is_default) {
                    append_issue(operation, "default_source_style",
                                 "default source style id '" +
                                     operation.source_style_id +
                                     "' cannot be refactored");
                }

                auto source_usage =
                    this->find_style_usage(operation.source_style_id);
                if (!source_usage.has_value()) {
                    return std::nullopt;
                }
                operation.source_usage = std::move(*source_usage);
            }
        }

        if (!operation.target_style_id.empty()) {
            const auto target_iterator =
                styles_by_id.find(operation.target_style_id);
            if (target_iterator != styles_by_id.end()) {
                operation.target_style = target_iterator->second;
            }
        }

        if (operation.action == featherdoc::style_refactor_action::rename) {
            if (operation.target_style.has_value()) {
                append_issue(operation, "target_style_exists",
                             "target style id '" + operation.target_style_id +
                                 "' already exists in word/styles.xml");
            }
        } else {
            if (!operation.target_style.has_value() &&
                !operation.target_style_id.empty()) {
                append_issue(operation, "missing_target_style",
                             "target style id '" + operation.target_style_id +
                                 "' was not found in word/styles.xml");
            }
            if (operation.source_style.has_value() &&
                operation.target_style.has_value() &&
                (operation.source_style->kind != operation.target_style->kind ||
                 operation.source_style->type_name !=
                     operation.target_style->type_name)) {
                append_issue(operation, "style_type_mismatch",
                             "source style id '" + operation.source_style_id +
                                 "' has type '" +
                                 operation.source_style->type_name +
                                 "' but target style id '" +
                                 operation.target_style_id + "' has type '" +
                                 operation.target_style->type_name + "'");
            }
        }

        operation.applyable = operation.issues.empty();
        if (operation.applyable) {
            ++plan.applyable_count;
        }
        plan.issue_count += operation.issues.size();
        plan.operations.push_back(std::move(operation));
    }

    this->last_error_info.clear();
    return plan;
}

std::optional<featherdoc::style_refactor_plan>
Document::suggest_style_merges() {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before suggesting style merges",
            std::string{styles_xml_entry});
        return std::nullopt;
    }

    auto styles = this->list_styles();
    if (const auto &error_info = this->last_error(); error_info.code) {
        return std::nullopt;
    }

    const auto styles_root = this->styles.child("w:styles");
    if (styles_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "word/styles.xml does not contain a w:styles root",
                       std::string{styles_xml_entry});
        return std::nullopt;
    }

    struct duplicate_candidate {
        featherdoc::style_summary style{};
        featherdoc::style_usage_summary usage{};
        pugi::xml_node style_node{};
    };

    auto candidates_by_signature =
        std::unordered_map<std::string, std::vector<duplicate_candidate>>{};

    for (const auto &style : styles) {
        if (!style_duplicate_merge_candidate(style)) {
            continue;
        }

        const auto resolved = this->resolve_style_properties(style.style_id);
        if (!resolved.has_value()) {
            return std::nullopt;
        }
        const auto usage = this->find_style_usage(style.style_id);
        if (!usage.has_value()) {
            return std::nullopt;
        }
        const auto style_node = find_style_node(styles_root, style.style_id);

        candidates_by_signature[style_duplicate_signature(style, *resolved)]
            .push_back(duplicate_candidate{style, *usage, style_node});
    }

    auto requests = std::vector<featherdoc::style_refactor_request>{};
    auto suggestions = std::vector<featherdoc::style_refactor_suggestion>{};
    for (auto &entry : candidates_by_signature) {
        auto &candidates = entry.second;
        if (candidates.size() < 2U) {
            continue;
        }

        std::sort(candidates.begin(), candidates.end(),
                  [](const auto &left, const auto &right) {
                      const auto left_count = left.usage.total_count();
                      const auto right_count = right.usage.total_count();
                      if (left_count != right_count) {
                          return left_count > right_count;
                      }
                      return left.style.style_id < right.style.style_id;
                  });

        const auto target_style_id = candidates.front().style.style_id;
        const auto target_reference_count =
            candidates.front().usage.total_count();
        const auto target_style_node = candidates.front().style_node;
        for (std::size_t index = 1U; index < candidates.size(); ++index) {
            requests.push_back({featherdoc::style_refactor_action::merge,
                                candidates[index].style.style_id,
                                target_style_id});
            suggestions.push_back(style_duplicate_merge_suggestion(
                target_reference_count > candidates[index].usage.total_count(),
                compare_style_duplicate_xml(candidates[index].style_node,
                                            target_style_node)));
        }
    }

    auto plan = this->plan_style_refactor(requests);
    if (!plan.has_value()) {
        return std::nullopt;
    }

    for (std::size_t index = 0U;
         index < plan->operations.size() && index < suggestions.size();
         ++index) {
        auto &operation = plan->operations[index];
        if (operation.action == featherdoc::style_refactor_action::merge &&
            operation.source_style_id == requests[index].source_style_id &&
            operation.target_style_id == requests[index].target_style_id) {
            operation.suggestion = suggestions[index];
        }
    }

    return plan;
}

std::optional<featherdoc::style_refactor_apply_result>
Document::apply_style_refactor(
    const std::vector<featherdoc::style_refactor_request> &requests) {
    auto plan = this->plan_style_refactor(requests);
    if (!plan.has_value()) {
        return std::nullopt;
    }

    add_style_refactor_apply_conflicts(*plan);

    auto result = featherdoc::style_refactor_apply_result{};
    result.requested_count = requests.size();
    result.plan = std::move(*plan);

    if (!result.plan.clean()) {
        this->last_error_info.clear();
        return result;
    }

    for (const auto &operation : result.plan.operations) {
        auto rollback = featherdoc::style_refactor_rollback_entry{};
        if (operation.action == featherdoc::style_refactor_action::rename) {
            rollback.action = featherdoc::style_refactor_action::rename;
            rollback.source_style_id = operation.target_style_id;
            rollback.target_style_id = operation.source_style_id;
            rollback.automatic = true;
            rollback.restorable = true;
            rollback.note = "rename style back to its previous style id";
        } else {
            rollback.action = featherdoc::style_refactor_action::merge;
            rollback.source_style_id = operation.source_style_id;
            rollback.target_style_id = operation.target_style_id;
            rollback.automatic = false;
            rollback.source_usage = operation.source_usage;
            if (const auto styles_root = this->styles.child("w:styles");
                styles_root != pugi::xml_node{}) {
                rollback.source_style_xml = style_xml_snapshot(
                    find_style_node(styles_root, operation.source_style_id));
            }
            rollback.restorable = !rollback.source_style_xml.empty() &&
                                  rollback.source_usage.has_value();
            rollback.note =
                rollback.restorable
                    ? "merge source style XML and original source references "
                      "were captured for a future restore workflow"
                    : "merge removed the source style definition; restore the "
                      "source document or a backup before replaying dependent "
                      "references";
        }

        const auto applied =
            operation.action == featherdoc::style_refactor_action::rename
                ? this->rename_style(operation.source_style_id,
                                     operation.target_style_id)
                : this->merge_style(operation.source_style_id,
                                    operation.target_style_id);
        if (!applied) {
            return std::nullopt;
        }
        result.rollback_entries.push_back(std::move(rollback));
        ++result.applied_count;
    }

    result.changed = result.applied_count > 0U;
    this->last_error_info.clear();
    return result;
}

std::optional<featherdoc::style_refactor_restore_result>
Document::plan_style_refactor_restore(
    const std::vector<featherdoc::style_refactor_rollback_entry>
        &rollback_entries) {
    return this->restore_style_refactor(rollback_entries, false);
}

std::optional<featherdoc::style_refactor_restore_result>
Document::restore_style_refactor(
    const std::vector<featherdoc::style_refactor_rollback_entry>
        &rollback_entries) {
    return this->restore_style_refactor(rollback_entries, true);
}

std::optional<featherdoc::style_refactor_restore_result>
Document::restore_style_refactor(
    const std::vector<featherdoc::style_refactor_rollback_entry>
        &rollback_entries,
    bool apply_changes) {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before restoring style refactors",
            std::string{styles_xml_entry});
        return std::nullopt;
    }

    if (const auto error = this->ensure_styles_loaded()) {
        return std::nullopt;
    }

    auto styles_root = this->styles.child("w:styles");
    if (styles_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       document_errc::styles_xml_parse_failed,
                       "word/styles.xml does not contain a w:styles root",
                       std::string{styles_xml_entry});
        return std::nullopt;
    }

    const auto document_root = this->document.child("w:document");
    if (document_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       document_errc::document_xml_parse_failed,
                       "word/document.xml does not contain a w:document root",
                       std::string{document_xml_entry});
        return std::nullopt;
    }

    auto result = featherdoc::style_refactor_restore_result{};
    result.requested_count = rollback_entries.size();
    result.dry_run = !apply_changes;
    result.operations.reserve(rollback_entries.size());

    const auto find_hit_root = [&](const featherdoc::style_usage_hit &hit) {
        if (hit.part == featherdoc::style_usage_part_kind::body) {
            return document_root.child("w:body");
        }

        if (hit.part == featherdoc::style_usage_part_kind::header) {
            for (const auto &part : this->header_parts) {
                if (part && part->entry_name == hit.entry_name) {
                    return part->xml.child("w:hdr");
                }
            }
            return pugi::xml_node{};
        }

        for (const auto &part : this->footer_parts) {
            if (part && part->entry_name == hit.entry_name) {
                return part->xml.child("w:ftr");
            }
        }
        return pugi::xml_node{};
    };

    for (const auto &entry : rollback_entries) {
        auto operation = featherdoc::style_refactor_restore_operation_result{};
        operation.action = entry.action;
        operation.source_style_id = entry.source_style_id;
        operation.target_style_id = entry.target_style_id;
        operation.restorable =
            entry.restorable ||
            (!entry.source_style_xml.empty() && entry.source_usage.has_value());

        if (entry.action != featherdoc::style_refactor_action::merge) {
            append_style_refactor_restore_issue(
                operation.issues, "unsupported_rollback_action",
                "only merge rollback entries can be restored from captured "
                "XML");
        }
        if (entry.source_style_id.empty()) {
            append_style_refactor_restore_issue(
                operation.issues, "empty_source_style_id",
                "merge rollback source style id must not be empty");
        }
        if (entry.target_style_id.empty()) {
            append_style_refactor_restore_issue(
                operation.issues, "empty_target_style_id",
                "merge rollback target style id must not be empty");
        }
        if (!operation.restorable) {
            append_style_refactor_restore_issue(
                operation.issues, "not_restorable",
                "merge rollback entry does not contain a restorable style XML "
                "and usage snapshot");
        }
        if (entry.source_style_xml.empty()) {
            append_style_refactor_restore_issue(
                operation.issues, "missing_source_style_xml",
                "merge rollback entry does not contain source style XML");
        }
        if (!entry.source_usage.has_value()) {
            append_style_refactor_restore_issue(
                operation.issues, "missing_source_usage",
                "merge rollback entry does not contain original source usage "
                "hits");
        } else if (!entry.source_usage->style_id.empty() &&
                   entry.source_usage->style_id != entry.source_style_id) {
            append_style_refactor_restore_issue(
                operation.issues, "source_usage_style_mismatch",
                "merge rollback source usage style id does not match the "
                "rollback source style id");
        }
        if (find_style_node(styles_root, entry.source_style_id) !=
            pugi::xml_node{}) {
            append_style_refactor_restore_issue(
                operation.issues, "source_style_exists",
                "source style id '" + entry.source_style_id +
                    "' already exists in word/styles.xml");
        }
        if (find_style_node(styles_root, entry.target_style_id) ==
            pugi::xml_node{}) {
            append_style_refactor_restore_issue(
                operation.issues, "missing_target_style",
                "target style id '" + entry.target_style_id +
                    "' was not found in word/styles.xml");
        }

        auto source_style_document = pugi::xml_document{};
        pugi::xml_node source_style;
        if (!entry.source_style_xml.empty()) {
            const auto parsed = source_style_document.load_string(
                entry.source_style_xml.c_str(), pugi::parse_default);
            if (!parsed) {
                append_style_refactor_restore_issue(
                    operation.issues, "invalid_source_style_xml",
                    "source style XML snapshot could not be parsed");
            } else {
                source_style = source_style_document.child("w:style");
                if (source_style == pugi::xml_node{}) {
                    source_style = source_style_document.first_child();
                }
                if (source_style == pugi::xml_node{} ||
                    std::string_view{source_style.name()} != "w:style") {
                    append_style_refactor_restore_issue(
                        operation.issues, "invalid_source_style_xml",
                        "source style XML snapshot must contain a w:style "
                        "root");
                } else if (std::string_view{
                               source_style.attribute("w:styleId").value()} !=
                           entry.source_style_id) {
                    append_style_refactor_restore_issue(
                        operation.issues, "source_style_xml_mismatch",
                        "source style XML snapshot style id does not match the "
                        "rollback source style id");
                }
            }
        }

        if (!operation.issues.empty()) {
            result.operations.push_back(std::move(operation));
            continue;
        }

        for (const auto &hit : entry.source_usage->hits) {
            const auto root = find_hit_root(hit);
            if (root == pugi::xml_node{}) {
                append_style_refactor_restore_issue(
                    operation.issues, "missing_usage_part",
                    "usage hit part '" + hit.entry_name +
                        "' was not found while restoring source references");
                continue;
            }

            const auto hit_result = restore_style_reference_hit(
                root, hit, entry.target_style_id, entry.source_style_id, false);
            if (!hit_result.found) {
                append_style_refactor_restore_issue(
                    operation.issues, "missing_usage_hit",
                    "usage hit ordinal could not be found while restoring "
                    "source references");
                continue;
            }
            if (!hit_result.changed) {
                append_style_refactor_restore_issue(
                    operation.issues, "usage_hit_not_target_style",
                    "usage hit no longer references the merge target style id");
            }
        }

        if (!operation.issues.empty()) {
            result.operations.push_back(std::move(operation));
            continue;
        }

        if (!apply_changes) {
            operation.style_restored = true;
            operation.restored_reference_count =
                entry.source_usage->hits.size();
            operation.restored = true;
            ++result.restored_count;
            ++result.restored_style_count;
            result.restored_reference_count +=
                operation.restored_reference_count;
            result.operations.push_back(std::move(operation));
            continue;
        }

        if (styles_root.append_copy(source_style) == pugi::xml_node{}) {
            append_style_refactor_restore_issue(
                operation.issues, "source_style_insert_failed",
                "source style XML snapshot could not be inserted into "
                "word/styles.xml");
            result.operations.push_back(std::move(operation));
            continue;
        }

        operation.style_restored = true;
        ++result.restored_style_count;

        for (const auto &hit : entry.source_usage->hits) {
            const auto root = find_hit_root(hit);
            if (root == pugi::xml_node{}) {
                append_style_refactor_restore_issue(
                    operation.issues, "missing_usage_part",
                    "usage hit part '" + hit.entry_name +
                        "' was not found while restoring source references");
                continue;
            }

            const auto hit_result = restore_style_reference_hit(
                root, hit, entry.target_style_id, entry.source_style_id);
            if (!hit_result.found) {
                append_style_refactor_restore_issue(
                    operation.issues, "missing_usage_hit",
                    "usage hit ordinal could not be found while restoring "
                    "source references");
                continue;
            }
            if (!hit_result.changed) {
                append_style_refactor_restore_issue(
                    operation.issues, "usage_hit_not_target_style",
                    "usage hit no longer references the merge target style id");
                continue;
            }

            ++operation.restored_reference_count;
            ++result.restored_reference_count;
        }

        operation.restored = operation.issues.empty();
        if (operation.restored) {
            ++result.restored_count;
        }
        if (apply_changes) {
            result.changed = result.changed || operation.style_restored ||
                             operation.restored_reference_count > 0U;
        }
        result.operations.push_back(std::move(operation));
    }

    if (apply_changes && result.changed) {
        this->styles_dirty = true;
    }

    this->last_error_info.clear();
    return result;
}

bool Document::rename_style(std::string_view old_style_id,
                            std::string_view new_style_id) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before renaming styles",
                       std::string{styles_xml_entry});
        return false;
    }

    if (old_style_id.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "old style id must not be empty when renaming styles",
                       std::string{styles_xml_entry});
        return false;
    }

    if (new_style_id.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "new style id must not be empty when renaming styles",
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

    const auto style = find_style_node(styles_root, old_style_id);
    if (style == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id '" + std::string{old_style_id} +
                           "' was not found in word/styles.xml",
                       std::string{styles_xml_entry});
        return false;
    }

    if (find_style_node(styles_root, new_style_id) != pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "target style id '" + std::string{new_style_id} +
                           "' already exists in word/styles.xml",
                       std::string{styles_xml_entry});
        return false;
    }

    if (read_on_off_attribute(style, "w:default")) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "default style id '" + std::string{old_style_id} +
                           "' cannot be renamed",
                       std::string{styles_xml_entry});
        return false;
    }

    const auto document_root = this->document.child("w:document");
    if (document_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       document_errc::document_xml_parse_failed,
                       "word/document.xml does not contain a w:document root",
                       std::string{document_xml_entry});
        return false;
    }

    if (const auto error = this->ensure_styles_part_attached()) {
        return false;
    }

    auto changed = true;
    ensure_attribute_value(style, "w:styleId", new_style_id);

    changed = rewrite_style_metadata_references_in_styles(
                  styles_root, old_style_id, new_style_id) ||
              changed;

    changed = rewrite_style_references_in_tree(document_root, old_style_id,
                                               new_style_id) ||
              changed;

    for (const auto &part : this->header_parts) {
        if (part) {
            changed =
                rewrite_style_references_in_tree(part->xml.child("w:hdr"),
                                                 old_style_id, new_style_id) ||
                changed;
        }
    }

    for (const auto &part : this->footer_parts) {
        if (part) {
            changed =
                rewrite_style_references_in_tree(part->xml.child("w:ftr"),
                                                 old_style_id, new_style_id) ||
                changed;
        }
    }

    if (changed) {
        this->styles_dirty = true;
    }

    this->last_error_info.clear();
    return true;
}

bool Document::merge_style(std::string_view source_style_id,
                           std::string_view target_style_id) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before merging styles",
                       std::string{styles_xml_entry});
        return false;
    }

    if (source_style_id.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "source style id must not be empty when merging styles",
                       std::string{styles_xml_entry});
        return false;
    }

    if (target_style_id.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "target style id must not be empty when merging styles",
                       std::string{styles_xml_entry});
        return false;
    }

    if (source_style_id == target_style_id) {
        set_last_error(
            this->last_error_info,
            std::make_error_code(std::errc::invalid_argument),
            "source and target style ids must be different when merging styles",
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

    const auto source_style = find_style_node(styles_root, source_style_id);
    if (source_style == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "source style id '" + std::string{source_style_id} +
                           "' was not found in word/styles.xml",
                       std::string{styles_xml_entry});
        return false;
    }

    const auto target_style = find_style_node(styles_root, target_style_id);
    if (target_style == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "target style id '" + std::string{target_style_id} +
                           "' was not found in word/styles.xml",
                       std::string{styles_xml_entry});
        return false;
    }

    if (read_on_off_attribute(source_style, "w:default")) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "default style id '" + std::string{source_style_id} +
                           "' cannot be merged",
                       std::string{styles_xml_entry});
        return false;
    }

    const auto source_summary = summarize_style_node(source_style);
    const auto target_summary = summarize_style_node(target_style);
    if (source_summary.kind != target_summary.kind ||
        source_summary.type_name != target_summary.type_name) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "source style id '" + std::string{source_style_id} +
                           "' has type '" + source_summary.type_name +
                           "' but target style id '" +
                           std::string{target_style_id} + "' has type '" +
                           target_summary.type_name + "'",
                       std::string{styles_xml_entry});
        return false;
    }

    const auto document_root = this->document.child("w:document");
    if (document_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       document_errc::document_xml_parse_failed,
                       "word/document.xml does not contain a w:document root",
                       std::string{document_xml_entry});
        return false;
    }

    if (const auto error = this->ensure_styles_part_attached()) {
        return false;
    }

    rewrite_style_metadata_references_in_styles(styles_root, source_style_id,
                                                target_style_id);
    rewrite_style_references_in_tree(document_root, source_style_id,
                                     target_style_id);

    for (const auto &part : this->header_parts) {
        if (part) {
            rewrite_style_references_in_tree(part->xml.child("w:hdr"),
                                             source_style_id, target_style_id);
        }
    }

    for (const auto &part : this->footer_parts) {
        if (part) {
            rewrite_style_references_in_tree(part->xml.child("w:ftr"),
                                             source_style_id, target_style_id);
        }
    }

    if (!styles_root.remove_child(source_style)) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "failed to remove source style id '" +
                           std::string{source_style_id} +
                           "' from word/styles.xml",
                       std::string{styles_xml_entry});
        return false;
    }

    this->styles_dirty = true;
    this->last_error_info.clear();
    return true;
}

std::optional<featherdoc::style_prune_plan>
Document::plan_prune_unused_styles() {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before planning unused "
                       "style pruning",
                       std::string{styles_xml_entry});
        return std::nullopt;
    }

    if (const auto error = this->ensure_styles_loaded()) {
        return std::nullopt;
    }

    const auto styles_root = this->styles.child("w:styles");
    if (styles_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       document_errc::styles_xml_parse_failed,
                       "word/styles.xml does not contain a w:styles root",
                       std::string{styles_xml_entry});
        return std::nullopt;
    }

    const auto document_root = this->document.child("w:document");
    if (document_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       document_errc::document_xml_parse_failed,
                       "word/document.xml does not contain a w:document root",
                       std::string{document_xml_entry});
        return std::nullopt;
    }

    auto plan = featherdoc::style_prune_plan{};
    auto style_nodes = std::unordered_map<std::string, pugi::xml_node>{};
    auto protected_style_ids = std::unordered_set<std::string>{};
    auto traversal_queue = std::vector<std::string>{};

    const auto protect_style_id = [&](std::string_view style_id) {
        if (style_id.empty()) {
            return;
        }
        auto [iterator, inserted] = protected_style_ids.emplace(style_id);
        if (inserted) {
            traversal_queue.push_back(*iterator);
        }
    };

    for (auto style = styles_root.child("w:style"); style != pugi::xml_node{};
         style = style.next_sibling("w:style")) {
        const auto style_id =
            std::string_view{style.attribute("w:styleId").value()};
        if (style_id.empty()) {
            continue;
        }
        ++plan.scanned_style_count;
        style_nodes.emplace(std::string{style_id}, style);
        if (read_on_off_attribute(style, "w:default") ||
            !read_on_off_attribute(style, "w:customStyle")) {
            protect_style_id(style_id);
        }
    }

    auto directly_used_style_ids = std::unordered_set<std::string>{};
    collect_style_references_in_tree(document_root, directly_used_style_ids);
    for (const auto &part : this->header_parts) {
        if (part) {
            collect_style_references_in_tree(part->xml.child("w:hdr"),
                                             directly_used_style_ids);
        }
    }
    for (const auto &part : this->footer_parts) {
        if (part) {
            collect_style_references_in_tree(part->xml.child("w:ftr"),
                                             directly_used_style_ids);
        }
    }
    for (const auto &style_id : directly_used_style_ids) {
        protect_style_id(style_id);
    }

    for (std::size_t index = 0U; index < traversal_queue.size(); ++index) {
        const auto iterator = style_nodes.find(traversal_queue[index]);
        if (iterator == style_nodes.end()) {
            continue;
        }

        auto dependencies = std::vector<std::string>{};
        append_style_dependency_ids(iterator->second, dependencies);
        for (const auto &dependency : dependencies) {
            if (style_nodes.contains(dependency)) {
                protect_style_id(dependency);
            }
        }
    }

    plan.protected_style_count = protected_style_ids.size();

    for (auto style = styles_root.child("w:style"); style != pugi::xml_node{};
         style = style.next_sibling("w:style")) {
        const auto style_id = std::string{style.attribute("w:styleId").value()};
        if (!style_id.empty() &&
            read_on_off_attribute(style, "w:customStyle") &&
            !read_on_off_attribute(style, "w:default") &&
            !protected_style_ids.contains(style_id)) {
            plan.removable_style_ids.push_back(style_id);
        }
    }

    this->last_error_info.clear();
    return plan;
}

std::optional<featherdoc::style_prune_summary> Document::prune_unused_styles() {
    const auto plan = this->plan_prune_unused_styles();
    if (!plan.has_value()) {
        return std::nullopt;
    }

    auto summary = featherdoc::style_prune_summary{};
    summary.scanned_style_count = plan->scanned_style_count;
    summary.protected_style_count = plan->protected_style_count;

    if (!plan->removable_style_ids.empty()) {
        if (const auto error = this->ensure_styles_part_attached()) {
            return std::nullopt;
        }
        auto styles_root = this->styles.child("w:styles");
        for (const auto &style_id : plan->removable_style_ids) {
            const auto style = find_style_node(styles_root, style_id);
            if (style == pugi::xml_node{}) {
                continue;
            }
            if (styles_root.remove_child(style)) {
                summary.removed_style_ids.push_back(style_id);
            }
        }
        this->styles_dirty = true;
    }

    this->last_error_info.clear();
    return summary;
}

bool Document::ensure_paragraph_style(
    std::string_view style_id,
    const featherdoc::paragraph_style_definition &definition) {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before defining paragraph styles");
        return false;
    }

    if (style_id.empty()) {
        set_last_error(
            this->last_error_info,
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

    if (!validate_optional_style_value(
            this->last_error_info, definition.based_on,
            "paragraph style basedOn must not be empty") ||
        !validate_optional_style_value(
            this->last_error_info, definition.next_style,
            "paragraph style next style must not be empty") ||
        !validate_optional_style_value(
            this->last_error_info, definition.run_font_family,
            "paragraph style run font family must not be empty") ||
        !validate_optional_style_value(
            this->last_error_info, definition.run_east_asia_font_family,
            "paragraph style eastAsia font family must not be empty") ||
        !validate_optional_style_value(
            this->last_error_info, definition.run_language,
            "paragraph style run language must not be empty") ||
        !validate_optional_style_value(
            this->last_error_info, definition.run_east_asia_language,
            "paragraph style eastAsia language must not be empty") ||
        !validate_optional_style_value(
            this->last_error_info, definition.run_bidi_language,
            "paragraph style bidi language must not be empty")) {
        return false;
    }

    if (definition.outline_level.has_value() &&
        *definition.outline_level > 8U) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "paragraph style outline level must be between 0 and 8",
                       std::string{styles_xml_entry});
        return false;
    }
    if (definition.run_text_color.has_value() &&
        definition.run_text_color->empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "paragraph style run text color must not be empty",
                       std::string{styles_xml_entry});
        return false;
    }
    if (definition.run_font_size_points.has_value() &&
        *definition.run_font_size_points <= 0.0) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "paragraph style run font size points must be greater "
                       "than zero",
                       std::string{styles_xml_entry});
        return false;
    }

    if (const auto error = this->ensure_styles_part_attached()) {
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

    bool reset_inherited_run_properties = false;
    if (const auto existing = find_style_node(styles_root, style_id);
        existing != pugi::xml_node{}) {
        const auto existing_type =
            std::string_view{existing.attribute("w:type").value()};
        if (!existing_type.empty() && existing_type != "paragraph") {
            set_last_error(this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           "style id '" + std::string{style_id} +
                               "' already exists with type '" +
                               std::string{existing_type} + "'",
                           std::string{styles_xml_entry});
            return false;
        }

        if (definition.based_on.has_value()) {
            const auto existing_based_on =
                std::string_view{
                    existing.child("w:basedOn").attribute("w:val").value()};
            reset_inherited_run_properties =
                existing_based_on !=
                std::string_view{definition.based_on->data(),
                                 definition.based_on->size()};
        }
    }

    const auto style = ensure_style_node(styles_root, style_id);
    if (style == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to create paragraph style '" +
                           std::string{style_id} + "'",
                       std::string{styles_xml_entry});
        return false;
    }

    if (!apply_common_style_definition(
            style, style_id, "paragraph", definition.name, definition.based_on,
            definition.is_custom, definition.is_semi_hidden,
            definition.is_unhide_when_used, definition.is_quick_format) ||
        !set_style_string_node(style, "w:next", definition.next_style) ||
        !set_style_outline_level(style, definition.outline_level)) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to update paragraph style '" +
                           std::string{style_id} + "'",
                       std::string{styles_xml_entry});
        return false;
    }

    this->styles_dirty = true;

    if (definition.run_text_color.has_value() &&
        (!this->clear_style_run_text_color(style_id) ||
         !this->set_style_run_text_color(style_id,
                                         *definition.run_text_color))) {
        return false;
    }
    if (definition.run_bold.has_value() &&
        (!this->clear_style_run_bold(style_id) ||
         !this->set_style_run_bold(style_id, *definition.run_bold))) {
        return false;
    }
    if (definition.run_italic.has_value() &&
        (!this->clear_style_run_italic(style_id) ||
         !this->set_style_run_italic(style_id, *definition.run_italic))) {
        return false;
    }
    if (definition.run_strikethrough.has_value() &&
        (!this->clear_style_run_strikethrough(style_id) ||
         !this->set_style_run_strikethrough(
             style_id, *definition.run_strikethrough))) {
        return false;
    }
    if (definition.run_underline.has_value() &&
        (!this->clear_style_run_underline(style_id) ||
         !this->set_style_run_underline(style_id,
                                        *definition.run_underline))) {
        return false;
    }
    if (definition.run_font_size_points.has_value() &&
        (!this->clear_style_run_font_size_points(style_id) ||
         !this->set_style_run_font_size_points(
             style_id, *definition.run_font_size_points))) {
        return false;
    }

    if (reset_inherited_run_properties ||
        definition.run_font_family.has_value() ||
        definition.run_east_asia_font_family.has_value()) {
        if (!this->clear_style_run_font_family(style_id)) {
            return false;
        }
        if (definition.run_font_family.has_value() &&
            !this->set_style_run_font_family(style_id,
                                             *definition.run_font_family)) {
            return false;
        }
        if (definition.run_east_asia_font_family.has_value() &&
            !this->set_style_run_east_asia_font_family(
                style_id, *definition.run_east_asia_font_family)) {
            return false;
        }
    }

    if (reset_inherited_run_properties || definition.run_language.has_value() ||
        definition.run_east_asia_language.has_value() ||
        definition.run_bidi_language.has_value()) {
        if (!this->clear_style_run_language(style_id)) {
            return false;
        }
        if (definition.run_language.has_value() &&
            !this->set_style_run_language(style_id, *definition.run_language)) {
            return false;
        }
        if (definition.run_east_asia_language.has_value() &&
            !this->set_style_run_east_asia_language(
                style_id, *definition.run_east_asia_language)) {
            return false;
        }
        if (definition.run_bidi_language.has_value() &&
            !this->set_style_run_bidi_language(
                style_id, *definition.run_bidi_language)) {
            return false;
        }
    }

    if (definition.run_rtl.has_value()) {
        if (!this->set_style_run_rtl(style_id, *definition.run_rtl)) {
            return false;
        }
    } else if (reset_inherited_run_properties &&
               !this->clear_style_run_rtl(style_id)) {
        return false;
    }

    if (definition.paragraph_bidi.has_value()) {
        if (!this->set_style_paragraph_bidi(style_id,
                                            *definition.paragraph_bidi)) {
            return false;
        }
    }

    this->last_error_info.clear();
    return true;
}

bool Document::ensure_character_style(
    std::string_view style_id,
    const featherdoc::character_style_definition &definition) {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before defining character styles");
        return false;
    }

    if (style_id.empty()) {
        set_last_error(
            this->last_error_info,
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

    if (!validate_optional_style_value(
            this->last_error_info, definition.based_on,
            "character style basedOn must not be empty") ||
        !validate_optional_style_value(
            this->last_error_info, definition.run_font_family,
            "character style run font family must not be empty") ||
        !validate_optional_style_value(
            this->last_error_info, definition.run_east_asia_font_family,
            "character style eastAsia font family must not be empty") ||
        !validate_optional_style_value(
            this->last_error_info, definition.run_language,
            "character style run language must not be empty") ||
        !validate_optional_style_value(
            this->last_error_info, definition.run_east_asia_language,
            "character style eastAsia language must not be empty") ||
        !validate_optional_style_value(
            this->last_error_info, definition.run_bidi_language,
            "character style bidi language must not be empty")) {
        return false;
    }
    if (definition.run_text_color.has_value() &&
        definition.run_text_color->empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "character style run text color must not be empty",
                       std::string{styles_xml_entry});
        return false;
    }
    if (definition.run_font_size_points.has_value() &&
        *definition.run_font_size_points <= 0.0) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "character style run font size points must be greater "
                       "than zero",
                       std::string{styles_xml_entry});
        return false;
    }
    if (definition.run_superscript.value_or(false) &&
        definition.run_subscript.value_or(false)) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "character style run superscript and subscript cannot "
                       "both be true",
                       std::string{styles_xml_entry});
        return false;
    }

    if (const auto error = this->ensure_styles_part_attached()) {
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

    bool reset_inherited_run_properties = false;
    if (const auto existing = find_style_node(styles_root, style_id);
        existing != pugi::xml_node{}) {
        const auto existing_type =
            std::string_view{existing.attribute("w:type").value()};
        if (!existing_type.empty() && existing_type != "character") {
            set_last_error(this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           "style id '" + std::string{style_id} +
                               "' already exists with type '" +
                               std::string{existing_type} + "'",
                           std::string{styles_xml_entry});
            return false;
        }

        if (definition.based_on.has_value()) {
            const auto existing_based_on =
                std::string_view{
                    existing.child("w:basedOn").attribute("w:val").value()};
            reset_inherited_run_properties =
                existing_based_on !=
                std::string_view{definition.based_on->data(),
                                 definition.based_on->size()};
        }
    }

    const auto style = ensure_style_node(styles_root, style_id);
    if (style == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to create character style '" +
                           std::string{style_id} + "'",
                       std::string{styles_xml_entry});
        return false;
    }

    if (!apply_common_style_definition(
            style, style_id, "character", definition.name, definition.based_on,
            definition.is_custom, definition.is_semi_hidden,
            definition.is_unhide_when_used, definition.is_quick_format)) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to update character style '" +
                           std::string{style_id} + "'",
                       std::string{styles_xml_entry});
        return false;
    }

    this->styles_dirty = true;

    if (definition.run_text_color.has_value() &&
        (!this->clear_style_run_text_color(style_id) ||
         !this->set_style_run_text_color(style_id,
                                         *definition.run_text_color))) {
        return false;
    }
    if (definition.run_bold.has_value() &&
        (!this->clear_style_run_bold(style_id) ||
         !this->set_style_run_bold(style_id, *definition.run_bold))) {
        return false;
    }
    if (definition.run_italic.has_value() &&
        (!this->clear_style_run_italic(style_id) ||
         !this->set_style_run_italic(style_id, *definition.run_italic))) {
        return false;
    }
    if (definition.run_strikethrough.has_value() &&
        (!this->clear_style_run_strikethrough(style_id) ||
         !this->set_style_run_strikethrough(
             style_id, *definition.run_strikethrough))) {
        return false;
    }
    if (definition.run_underline.has_value() &&
        (!this->clear_style_run_underline(style_id) ||
         !this->set_style_run_underline(style_id,
                                        *definition.run_underline))) {
        return false;
    }
    if (definition.run_superscript.has_value() ||
        definition.run_subscript.has_value()) {
        if (!this->clear_style_run_superscript(style_id)) {
            return false;
        }
        if (definition.run_superscript.value_or(false)) {
            if (!this->set_style_run_superscript(style_id, true)) {
                return false;
            }
        } else if (definition.run_subscript.value_or(false)) {
            if (!this->set_style_run_subscript(style_id, true)) {
                return false;
            }
        } else if (!this->set_style_run_superscript(style_id, false)) {
            return false;
        }
    }
    if (definition.run_font_size_points.has_value() &&
        (!this->clear_style_run_font_size_points(style_id) ||
         !this->set_style_run_font_size_points(
             style_id, *definition.run_font_size_points))) {
        return false;
    }

    if (reset_inherited_run_properties ||
        definition.run_font_family.has_value() ||
        definition.run_east_asia_font_family.has_value()) {
        if (!this->clear_style_run_font_family(style_id)) {
            return false;
        }
        if (definition.run_font_family.has_value() &&
            !this->set_style_run_font_family(style_id,
                                             *definition.run_font_family)) {
            return false;
        }
        if (definition.run_east_asia_font_family.has_value() &&
            !this->set_style_run_east_asia_font_family(
                style_id, *definition.run_east_asia_font_family)) {
            return false;
        }
    }

    if (reset_inherited_run_properties || definition.run_language.has_value() ||
        definition.run_east_asia_language.has_value() ||
        definition.run_bidi_language.has_value()) {
        if (!this->clear_style_run_language(style_id)) {
            return false;
        }
        if (definition.run_language.has_value() &&
            !this->set_style_run_language(style_id, *definition.run_language)) {
            return false;
        }
        if (definition.run_east_asia_language.has_value() &&
            !this->set_style_run_east_asia_language(
                style_id, *definition.run_east_asia_language)) {
            return false;
        }
        if (definition.run_bidi_language.has_value() &&
            !this->set_style_run_bidi_language(
                style_id, *definition.run_bidi_language)) {
            return false;
        }
    }

    if (definition.run_rtl.has_value()) {
        if (!this->set_style_run_rtl(style_id, *definition.run_rtl)) {
            return false;
        }
    } else if (reset_inherited_run_properties &&
               !this->clear_style_run_rtl(style_id)) {
        return false;
    }

    this->last_error_info.clear();
    return true;
}

bool Document::ensure_table_style(
    std::string_view style_id,
    const featherdoc::table_style_definition &definition) {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
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

    if (!validate_optional_style_value(
            this->last_error_info, definition.based_on,
            "table style basedOn must not be empty")) {
        return false;
    }

    if (!detail::validate_table_style_regions(this->last_error_info, definition)) {
        return false;
    }

    if (const auto error = this->ensure_styles_part_attached()) {
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

    if (const auto existing = find_style_node(styles_root, style_id);
        existing != pugi::xml_node{}) {
        const auto existing_type =
            std::string_view{existing.attribute("w:type").value()};
        if (!existing_type.empty() && existing_type != "table") {
            set_last_error(this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           "style id '" + std::string{style_id} +
                               "' already exists with type '" +
                               std::string{existing_type} + "'",
                           std::string{styles_xml_entry});
            return false;
        }
    }

    const auto style = ensure_style_node(styles_root, style_id);
    if (style == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to create table style '" +
                           std::string{style_id} + "'",
                       std::string{styles_xml_entry});
        return false;
    }

    if (!apply_common_style_definition(
            style, style_id, "table", definition.name, definition.based_on,
            definition.is_custom, definition.is_semi_hidden,
            definition.is_unhide_when_used, definition.is_quick_format)) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to update table style '" +
                           std::string{style_id} + "'",
                       std::string{styles_xml_entry});
        return false;
    }

    if (!detail::apply_table_style_regions(style, definition)) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to update table style properties for '" +
                           std::string{style_id} + "'",
                       std::string{styles_xml_entry});
        return false;
    }

    this->styles_dirty = true;
    this->last_error_info.clear();
    return true;
}

std::optional<featherdoc::table_style_definition_summary>
Document::find_table_style_definition(std::string_view style_id) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before reading table "
                       "style definitions",
                       std::string{styles_xml_entry});
        return std::nullopt;
    }

    if (style_id.empty()) {
        set_last_error(
            this->last_error_info,
            std::make_error_code(std::errc::invalid_argument),
            "style id must not be empty when reading table style definitions",
            std::string{styles_xml_entry});
        return std::nullopt;
    }

    if (const auto error = this->ensure_styles_loaded()) {
        return std::nullopt;
    }

    const auto styles_root = this->styles.child("w:styles");
    if (styles_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       document_errc::styles_xml_parse_failed,
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

    auto summary = featherdoc::table_style_definition_summary{};
    summary.style = summarize_style_node(style);
    if (summary.style.kind != featherdoc::style_kind::table) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id '" + std::string{style_id} +
                           "' is not a table style",
                       std::string{styles_xml_entry});
        return std::nullopt;
    }

    summary.whole_table = detail::read_table_style_region(style, true);
    summary.first_row = detail::read_table_style_conditional_region(style, "firstRow");
    summary.last_row = detail::read_table_style_conditional_region(style, "lastRow");
    summary.first_column =
        detail::read_table_style_conditional_region(style, "firstCol");
    summary.last_column = detail::read_table_style_conditional_region(style, "lastCol");
    summary.banded_rows =
        detail::read_table_style_conditional_region(style, "band1Horz");
    summary.banded_columns =
        detail::read_table_style_conditional_region(style, "band1Vert");
    summary.second_banded_rows =
        detail::read_table_style_conditional_region(style, "band2Horz");
    summary.second_banded_columns =
        detail::read_table_style_conditional_region(style, "band2Vert");

    this->last_error_info.clear();
    return summary;
}

table_style_region_audit_report
Document::audit_table_style_regions(std::optional<std::string_view> style_id) {
    auto report = table_style_region_audit_report{};
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before auditing table style regions",
            std::string{styles_xml_entry});
        return report;
    }

    if (style_id.has_value() && style_id->empty()) {
        set_last_error(
            this->last_error_info,
            std::make_error_code(std::errc::invalid_argument),
            "style id must not be empty when auditing table style regions",
            std::string{styles_xml_entry});
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

    auto add_issue = [&](const style_summary &style, std::string region,
                         std::size_t property_count) {
        auto issue = table_style_region_audit_issue{};
        issue.style_id = style.style_id;
        issue.style_name = style.name;
        issue.region = std::move(region);
        issue.issue_type = "empty_region";
        issue.property_count = property_count;
        issue.suggestion = "remove this empty table style region or add typed "
                           "visual properties";
        report.issues.push_back(std::move(issue));
    };

    auto audit_region = [&](const style_summary &style,
                            std::string_view region_name,
                            pugi::xml_node region_node, bool whole_table,
                            bool declared) {
        if (!declared) {
            return;
        }
        ++report.region_count;
        const auto region = detail::read_table_style_region(region_node, whole_table);
        const auto property_count =
            region.has_value() ? detail::table_style_region_property_count(*region)
                               : 0U;
        if (property_count == 0U) {
            add_issue(style, std::string{region_name}, property_count);
        }
    };

    auto audit_style = [&](pugi::xml_node style) {
        const auto summary = summarize_style_node(style);
        if (summary.kind != style_kind::table) {
            return;
        }
        ++report.table_style_count;
        audit_region(summary, "whole_table", style, true,
                     detail::table_style_whole_region_declared(style));

        auto audit_conditional = [&](std::string_view region_name,
                                     std::string_view xml_region) {
            const auto conditional =
                detail::find_table_style_conditional_node(style, xml_region);
            audit_region(summary, region_name, conditional, false,
                         conditional != pugi::xml_node{});
        };
        audit_conditional("first_row", "firstRow");
        audit_conditional("last_row", "lastRow");
        audit_conditional("first_column", "firstCol");
        audit_conditional("last_column", "lastCol");
        audit_conditional("banded_rows", "band1Horz");
        audit_conditional("banded_columns", "band1Vert");
        audit_conditional("second_banded_rows", "band2Horz");
        audit_conditional("second_banded_columns", "band2Vert");
    };

    if (style_id.has_value()) {
        const auto style = find_style_node(styles_root, *style_id);
        if (style == pugi::xml_node{}) {
            set_last_error(this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           "style id '" + std::string{*style_id} +
                               "' was not found in word/styles.xml",
                           std::string{styles_xml_entry});
            return report;
        }
        const auto summary = summarize_style_node(style);
        if (summary.kind != style_kind::table) {
            set_last_error(this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           "style id '" + std::string{*style_id} +
                               "' is not a table style",
                           std::string{styles_xml_entry});
            return report;
        }
        audit_style(style);
    } else {
        for (auto style = styles_root.child("w:style");
             style != pugi::xml_node{}; style = style.next_sibling("w:style")) {
            audit_style(style);
        }
    }

    this->last_error_info.clear();
    return report;
}

table_style_inheritance_audit_report Document::audit_table_style_inheritance(
    std::optional<std::string_view> style_id) {
    auto report = table_style_inheritance_audit_report{};
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before auditing table "
                       "style inheritance",
                       std::string{styles_xml_entry});
        return report;
    }

    if (style_id.has_value() && style_id->empty()) {
        set_last_error(
            this->last_error_info,
            std::make_error_code(std::errc::invalid_argument),
            "style id must not be empty when auditing table style inheritance",
            std::string{styles_xml_entry});
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

    auto add_issue =
        [&](const style_summary &style, std::string based_on_style_id,
            std::string based_on_style_kind, std::string issue_type,
            const std::vector<std::string> &chain, std::string suggestion) {
            auto issue = table_style_inheritance_audit_issue{};
            issue.style_id = style.style_id;
            issue.style_name = style.name;
            issue.based_on_style_id = std::move(based_on_style_id);
            issue.based_on_style_kind = std::move(based_on_style_kind);
            issue.issue_type = std::move(issue_type);
            issue.inheritance_chain = chain;
            issue.suggestion = std::move(suggestion);
            report.issues.push_back(std::move(issue));
        };

    auto audit_style = [&](pugi::xml_node style) {
        const auto root_summary = summarize_style_node(style);
        if (root_summary.kind != style_kind::table) {
            return;
        }
        ++report.table_style_count;

        auto visited = std::unordered_set<std::string>{};
        auto chain = std::vector<std::string>{};
        auto current_style = style;
        auto current_summary = root_summary;
        while (current_style != pugi::xml_node{}) {
            const auto current_id = current_summary.style_id;
            if (!visited.insert(current_id).second) {
                chain.push_back(current_id);
                add_issue(root_summary, current_id, "table",
                          "inheritance_cycle", chain,
                          "break the table style based_on cycle before "
                          "resolving inheritance");
                return;
            }
            chain.push_back(current_id);

            if (!current_summary.based_on.has_value() ||
                current_summary.based_on->empty()) {
                return;
            }

            const auto parent_style =
                find_style_node(styles_root, *current_summary.based_on);
            if (parent_style == pugi::xml_node{}) {
                add_issue(
                    root_summary, *current_summary.based_on, {},
                    "missing_based_on", chain,
                    "create the referenced table style or clear based_on");
                return;
            }

            const auto parent_summary = summarize_style_node(parent_style);
            if (parent_summary.kind != style_kind::table) {
                add_issue(root_summary, parent_summary.style_id,
                          parent_summary.type_name, "based_on_not_table", chain,
                          "rebase this table style to another table style");
                return;
            }

            current_style = parent_style;
            current_summary = parent_summary;
        }
    };

    if (style_id.has_value()) {
        const auto style = find_style_node(styles_root, *style_id);
        if (style == pugi::xml_node{}) {
            set_last_error(this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           "style id '" + std::string{*style_id} +
                               "' was not found in word/styles.xml",
                           std::string{styles_xml_entry});
            return report;
        }
        const auto summary = summarize_style_node(style);
        if (summary.kind != style_kind::table) {
            set_last_error(this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           "style id '" + std::string{*style_id} +
                               "' is not a table style",
                           std::string{styles_xml_entry});
            return report;
        }
        audit_style(style);
    } else {
        for (auto style = styles_root.child("w:style");
             style != pugi::xml_node{}; style = style.next_sibling("w:style")) {
            audit_style(style);
        }
    }

    this->last_error_info.clear();
    return report;
}

std::optional<std::string> Document::default_run_font_family() {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before reading default run fonts");
        return std::nullopt;
    }

    if (const auto error = this->ensure_styles_loaded()) {
        return std::nullopt;
    }

    const auto styles_root = this->styles.child("w:styles");
    if (styles_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       document_errc::styles_xml_parse_failed,
                       "word/styles.xml does not contain a w:styles root",
                       std::string{styles_xml_entry});
        return std::nullopt;
    }

    this->last_error_info.clear();
    return read_primary_font_family(styles_root.child("w:docDefaults")
                                        .child("w:rPrDefault")
                                        .child("w:rPr"));
}

std::optional<std::string> Document::default_run_east_asia_font_family() {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before reading default run fonts");
        return std::nullopt;
    }

    if (const auto error = this->ensure_styles_loaded()) {
        return std::nullopt;
    }

    const auto styles_root = this->styles.child("w:styles");
    if (styles_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       document_errc::styles_xml_parse_failed,
                       "word/styles.xml does not contain a w:styles root",
                       std::string{styles_xml_entry});
        return std::nullopt;
    }

    this->last_error_info.clear();
    return read_east_asia_font_family(styles_root.child("w:docDefaults")
                                          .child("w:rPrDefault")
                                          .child("w:rPr"));
}

std::optional<std::string> Document::default_run_language() {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before reading default run language",
            std::string{styles_xml_entry});
        return std::nullopt;
    }

    if (const auto error = this->ensure_styles_loaded()) {
        return std::nullopt;
    }

    const auto styles_root = this->styles.child("w:styles");
    if (styles_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       document_errc::styles_xml_parse_failed,
                       "word/styles.xml does not contain a w:styles root",
                       std::string{styles_xml_entry});
        return std::nullopt;
    }

    this->last_error_info.clear();
    return read_primary_language(styles_root.child("w:docDefaults")
                                     .child("w:rPrDefault")
                                     .child("w:rPr"));
}

std::optional<std::string> Document::default_run_east_asia_language() {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before reading default run language",
            std::string{styles_xml_entry});
        return std::nullopt;
    }

    if (const auto error = this->ensure_styles_loaded()) {
        return std::nullopt;
    }

    const auto styles_root = this->styles.child("w:styles");
    if (styles_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       document_errc::styles_xml_parse_failed,
                       "word/styles.xml does not contain a w:styles root",
                       std::string{styles_xml_entry});
        return std::nullopt;
    }

    this->last_error_info.clear();
    return read_east_asia_language(styles_root.child("w:docDefaults")
                                       .child("w:rPrDefault")
                                       .child("w:rPr"));
}

std::optional<std::string> Document::default_run_bidi_language() {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before reading default run language",
            std::string{styles_xml_entry});
        return std::nullopt;
    }

    if (const auto error = this->ensure_styles_loaded()) {
        return std::nullopt;
    }

    const auto styles_root = this->styles.child("w:styles");
    if (styles_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       document_errc::styles_xml_parse_failed,
                       "word/styles.xml does not contain a w:styles root",
                       std::string{styles_xml_entry});
        return std::nullopt;
    }

    this->last_error_info.clear();
    return read_bidi_language(styles_root.child("w:docDefaults")
                                  .child("w:rPrDefault")
                                  .child("w:rPr"));
}

std::optional<bool> Document::default_run_rtl() {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before reading default "
                       "run direction",
                       std::string{styles_xml_entry});
        return std::nullopt;
    }

    if (const auto error = this->ensure_styles_loaded()) {
        return std::nullopt;
    }

    const auto styles_root = this->styles.child("w:styles");
    if (styles_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       document_errc::styles_xml_parse_failed,
                       "word/styles.xml does not contain a w:styles root",
                       std::string{styles_xml_entry});
        return std::nullopt;
    }

    this->last_error_info.clear();
    return read_run_rtl(styles_root.child("w:docDefaults")
                            .child("w:rPrDefault")
                            .child("w:rPr"));
}

std::optional<bool> Document::default_paragraph_bidi() {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before reading default "
                       "paragraph direction",
                       std::string{styles_xml_entry});
        return std::nullopt;
    }

    if (const auto error = this->ensure_styles_loaded()) {
        return std::nullopt;
    }

    const auto styles_root = this->styles.child("w:styles");
    if (styles_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       document_errc::styles_xml_parse_failed,
                       "word/styles.xml does not contain a w:styles root",
                       std::string{styles_xml_entry});
        return std::nullopt;
    }

    this->last_error_info.clear();
    return read_paragraph_bidi(styles_root.child("w:docDefaults")
                                   .child("w:pPrDefault")
                                   .child("w:pPr"));
}

bool Document::set_default_run_font_family(std::string_view font_family) {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
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
        set_last_error(this->last_error_info,
                       document_errc::styles_xml_parse_failed,
                       "word/styles.xml does not contain a w:styles root",
                       std::string{styles_xml_entry});
        return false;
    }

    const auto run_properties =
        ensure_doc_defaults_run_properties_node(styles_root);
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

bool Document::set_default_run_east_asia_font_family(
    std::string_view font_family) {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
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
        set_last_error(this->last_error_info,
                       document_errc::styles_xml_parse_failed,
                       "word/styles.xml does not contain a w:styles root",
                       std::string{styles_xml_entry});
        return false;
    }

    const auto run_properties =
        ensure_doc_defaults_run_properties_node(styles_root);
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
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
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
        set_last_error(this->last_error_info,
                       document_errc::styles_xml_parse_failed,
                       "word/styles.xml does not contain a w:styles root",
                       std::string{styles_xml_entry});
        return false;
    }

    const auto run_properties =
        ensure_doc_defaults_run_properties_node(styles_root);
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
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
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
        set_last_error(this->last_error_info,
                       document_errc::styles_xml_parse_failed,
                       "word/styles.xml does not contain a w:styles root",
                       std::string{styles_xml_entry});
        return false;
    }

    const auto run_properties =
        ensure_doc_defaults_run_properties_node(styles_root);
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
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
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
        set_last_error(this->last_error_info,
                       document_errc::styles_xml_parse_failed,
                       "word/styles.xml does not contain a w:styles root",
                       std::string{styles_xml_entry});
        return false;
    }

    const auto run_properties =
        ensure_doc_defaults_run_properties_node(styles_root);
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
                       "call open() or create_empty() before editing default "
                       "run direction",
                       std::string{styles_xml_entry});
        return false;
    }

    if (const auto error = this->ensure_styles_part_attached()) {
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

    const auto run_properties =
        ensure_doc_defaults_run_properties_node(styles_root);
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
                       "call open() or create_empty() before editing default "
                       "paragraph direction",
                       std::string{styles_xml_entry});
        return false;
    }

    if (const auto error = this->ensure_styles_part_attached()) {
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

    const auto paragraph_properties =
        ensure_doc_defaults_paragraph_properties_node(styles_root);
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
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before editing default run fonts");
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

    if (clear_font_family_attributes(styles_root.child("w:docDefaults")
                                         .child("w:rPrDefault")
                                         .child("w:rPr"))) {
        this->styles_dirty = true;
    }

    this->last_error_info.clear();
    return true;
}

bool Document::clear_default_run_east_asia_font_family() {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before editing default run fonts");
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

    if (clear_font_family_attribute(styles_root.child("w:docDefaults")
                                        .child("w:rPrDefault")
                                        .child("w:rPr"),
                                    "w:eastAsia")) {
        this->styles_dirty = true;
    }

    this->last_error_info.clear();
    return true;
}

bool Document::clear_default_run_primary_language() {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before editing default run language",
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

    if (clear_language_attribute(styles_root.child("w:docDefaults")
                                     .child("w:rPrDefault")
                                     .child("w:rPr"),
                                 "w:val")) {
        this->styles_dirty = true;
    }

    this->last_error_info.clear();
    return true;
}

bool Document::clear_default_run_language() {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before editing default run language",
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

    if (clear_language_attributes(styles_root.child("w:docDefaults")
                                      .child("w:rPrDefault")
                                      .child("w:rPr"))) {
        this->styles_dirty = true;
    }

    this->last_error_info.clear();
    return true;
}

bool Document::clear_default_run_east_asia_language() {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before editing default run language",
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

    if (clear_language_attribute(styles_root.child("w:docDefaults")
                                     .child("w:rPrDefault")
                                     .child("w:rPr"),
                                 "w:eastAsia")) {
        this->styles_dirty = true;
    }

    this->last_error_info.clear();
    return true;
}

bool Document::clear_default_run_bidi_language() {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before editing default run language",
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

    if (clear_language_attribute(styles_root.child("w:docDefaults")
                                     .child("w:rPrDefault")
                                     .child("w:rPr"),
                                 "w:bidi")) {
        this->styles_dirty = true;
    }

    this->last_error_info.clear();
    return true;
}

bool Document::clear_default_run_rtl() {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before editing default "
                       "run direction",
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

    if (clear_on_off_child(styles_root.child("w:docDefaults")
                               .child("w:rPrDefault")
                               .child("w:rPr"),
                           "w:rtl")) {
        this->styles_dirty = true;
    }

    this->last_error_info.clear();
    return true;
}

bool Document::clear_default_paragraph_bidi() {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before editing default "
                       "paragraph direction",
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

    if (clear_on_off_child(styles_root.child("w:docDefaults")
                               .child("w:pPrDefault")
                               .child("w:pPr"),
                           "w:bidi")) {
        this->styles_dirty = true;
    }

    this->last_error_info.clear();
    return true;
}

std::optional<std::string>
Document::style_run_text_color(std::string_view style_id) {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before reading style run color",
            std::string{styles_xml_entry});
        return std::nullopt;
    }
    if (style_id.empty()) {
        set_last_error(
            this->last_error_info,
            std::make_error_code(std::errc::invalid_argument),
            "style id must not be empty when reading style run color",
            std::string{styles_xml_entry});
        return std::nullopt;
    }
    if (const auto error = this->ensure_styles_loaded()) {
        return std::nullopt;
    }
    const auto styles_root = this->styles.child("w:styles");
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
    return read_run_text_color(style.child("w:rPr"));
}

std::optional<bool> Document::style_run_bold(std::string_view style_id) {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before reading style run bold",
            std::string{styles_xml_entry});
        return std::nullopt;
    }
    if (style_id.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id must not be empty when reading style run bold",
                       std::string{styles_xml_entry});
        return std::nullopt;
    }
    if (const auto error = this->ensure_styles_loaded()) {
        return std::nullopt;
    }
    const auto style =
        find_style_node(this->styles.child("w:styles"), style_id);
    if (style == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id '" + std::string{style_id} +
                           "' was not found in word/styles.xml",
                       std::string{styles_xml_entry});
        return std::nullopt;
    }
    this->last_error_info.clear();
    return read_run_bold(style.child("w:rPr"));
}

std::optional<bool> Document::style_run_italic(std::string_view style_id) {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before reading style run italic",
            std::string{styles_xml_entry});
        return std::nullopt;
    }
    if (style_id.empty()) {
        set_last_error(
            this->last_error_info,
            std::make_error_code(std::errc::invalid_argument),
            "style id must not be empty when reading style run italic",
            std::string{styles_xml_entry});
        return std::nullopt;
    }
    if (const auto error = this->ensure_styles_loaded()) {
        return std::nullopt;
    }
    const auto style =
        find_style_node(this->styles.child("w:styles"), style_id);
    if (style == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id '" + std::string{style_id} +
                           "' was not found in word/styles.xml",
                       std::string{styles_xml_entry});
        return std::nullopt;
    }
    this->last_error_info.clear();
    return read_run_italic(style.child("w:rPr"));
}

std::optional<bool> Document::style_run_underline(std::string_view style_id) {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before reading style run underline",
            std::string{styles_xml_entry});
        return std::nullopt;
    }
    if (style_id.empty()) {
        set_last_error(
            this->last_error_info,
            std::make_error_code(std::errc::invalid_argument),
            "style id must not be empty when reading style run underline",
            std::string{styles_xml_entry});
        return std::nullopt;
    }
    if (const auto error = this->ensure_styles_loaded()) {
        return std::nullopt;
    }
    const auto style =
        find_style_node(this->styles.child("w:styles"), style_id);
    if (style == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id '" + std::string{style_id} +
                           "' was not found in word/styles.xml",
                       std::string{styles_xml_entry});
        return std::nullopt;
    }
    this->last_error_info.clear();
    return read_run_underline(style.child("w:rPr"));
}

std::optional<bool>
Document::style_run_strikethrough(std::string_view style_id) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before reading style "
                       "run strikethrough",
                       std::string{styles_xml_entry});
        return std::nullopt;
    }
    if (style_id.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id must not be empty when reading style run "
                       "strikethrough",
                       std::string{styles_xml_entry});
        return std::nullopt;
    }
    if (const auto error = this->ensure_styles_loaded()) {
        return std::nullopt;
    }
    const auto style =
        find_style_node(this->styles.child("w:styles"), style_id);
    if (style == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id '" + std::string{style_id} +
                           "' was not found in word/styles.xml",
                       std::string{styles_xml_entry});
        return std::nullopt;
    }
    this->last_error_info.clear();
    return read_run_strikethrough(style.child("w:rPr"));
}

std::optional<double>
Document::style_run_font_size_points(std::string_view style_id) {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before reading style run font size",
            std::string{styles_xml_entry});
        return std::nullopt;
    }
    if (style_id.empty()) {
        set_last_error(
            this->last_error_info,
            std::make_error_code(std::errc::invalid_argument),
            "style id must not be empty when reading style run font size",
            std::string{styles_xml_entry});
        return std::nullopt;
    }
    if (const auto error = this->ensure_styles_loaded()) {
        return std::nullopt;
    }
    const auto style =
        find_style_node(this->styles.child("w:styles"), style_id);
    if (style == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id '" + std::string{style_id} +
                           "' was not found in word/styles.xml",
                       std::string{styles_xml_entry});
        return std::nullopt;
    }
    this->last_error_info.clear();
    return read_run_font_size_points(style.child("w:rPr"));
}

std::optional<std::string>
Document::style_run_font_family(std::string_view style_id) {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before reading style run fonts");
        return std::nullopt;
    }

    if (style_id.empty()) {
        set_last_error(
            this->last_error_info,
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
        set_last_error(this->last_error_info,
                       document_errc::styles_xml_parse_failed,
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

std::optional<std::string>
Document::style_run_east_asia_font_family(std::string_view style_id) {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before reading style run fonts");
        return std::nullopt;
    }

    if (style_id.empty()) {
        set_last_error(
            this->last_error_info,
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
        set_last_error(this->last_error_info,
                       document_errc::styles_xml_parse_failed,
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

std::optional<std::string>
Document::style_run_language(std::string_view style_id) {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before reading style run language",
            std::string{styles_xml_entry});
        return std::nullopt;
    }

    if (style_id.empty()) {
        set_last_error(
            this->last_error_info,
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
        set_last_error(this->last_error_info,
                       document_errc::styles_xml_parse_failed,
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

std::optional<std::string>
Document::style_run_east_asia_language(std::string_view style_id) {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before reading style run language",
            std::string{styles_xml_entry});
        return std::nullopt;
    }

    if (style_id.empty()) {
        set_last_error(
            this->last_error_info,
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
        set_last_error(this->last_error_info,
                       document_errc::styles_xml_parse_failed,
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

std::optional<std::string>
Document::style_run_bidi_language(std::string_view style_id) {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before reading style run language",
            std::string{styles_xml_entry});
        return std::nullopt;
    }

    if (style_id.empty()) {
        set_last_error(
            this->last_error_info,
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
        set_last_error(this->last_error_info,
                       document_errc::styles_xml_parse_failed,
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
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before reading style run direction",
            std::string{styles_xml_entry});
        return std::nullopt;
    }

    if (style_id.empty()) {
        set_last_error(
            this->last_error_info,
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
        set_last_error(this->last_error_info,
                       document_errc::styles_xml_parse_failed,
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
                       "call open() or create_empty() before reading style "
                       "paragraph direction",
                       std::string{styles_xml_entry});
        return std::nullopt;
    }

    if (style_id.empty()) {
        set_last_error(
            this->last_error_info,
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
        set_last_error(this->last_error_info,
                       document_errc::styles_xml_parse_failed,
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

std::optional<std::string>
Document::paragraph_style_next_style(std::string_view style_id) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before reading paragraph "
                       "style next style",
                       std::string{styles_xml_entry});
        return std::nullopt;
    }

    if (style_id.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id must not be empty when reading paragraph "
                       "style next style",
                       std::string{styles_xml_entry});
        return std::nullopt;
    }

    if (const auto error = this->ensure_styles_loaded()) {
        return std::nullopt;
    }

    const auto styles_root = this->styles.child("w:styles");
    if (styles_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       document_errc::styles_xml_parse_failed,
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

    const auto type_name = std::string_view{style.attribute("w:type").value()};
    if (!type_name.empty() && type_name != "paragraph") {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id '" + std::string{style_id} +
                           "' is not a paragraph style",
                       std::string{styles_xml_entry});
        return std::nullopt;
    }

    if (const auto next = style.child("w:next").attribute("w:val");
        next != pugi::xml_attribute{} && next.value()[0] != '\0') {
        this->last_error_info.clear();
        return std::string{next.value()};
    }

    this->last_error_info.clear();
    return std::nullopt;
}

std::optional<std::uint32_t>
Document::paragraph_style_outline_level(std::string_view style_id) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before reading paragraph "
                       "style outline level",
                       std::string{styles_xml_entry});
        return std::nullopt;
    }

    if (style_id.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id must not be empty when reading paragraph "
                       "style outline level",
                       std::string{styles_xml_entry});
        return std::nullopt;
    }

    if (const auto error = this->ensure_styles_loaded()) {
        return std::nullopt;
    }

    const auto styles_root = this->styles.child("w:styles");
    if (styles_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       document_errc::styles_xml_parse_failed,
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

    const auto type_name = std::string_view{style.attribute("w:type").value()};
    if (!type_name.empty() && type_name != "paragraph") {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id '" + std::string{style_id} +
                           "' is not a paragraph style",
                       std::string{styles_xml_entry});
        return std::nullopt;
    }

    this->last_error_info.clear();
    return parse_u32_attribute_value(
        style.child("w:pPr").child("w:outlineLvl").attribute("w:val").value());
}

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
