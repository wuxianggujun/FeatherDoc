#pragma once

#include "cli_test_support.hpp"

namespace {

void create_cli_style_defaults_fixture(const std::filesystem::path &path) {
    remove_if_exists(path);

    featherdoc::Document document(path);
    REQUIRE_FALSE(document.create_empty());
    REQUIRE_FALSE(document.save());
}

void create_cli_style_existing_fixture(const std::filesystem::path &path) {
    remove_if_exists(path);

    constexpr auto relationships_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rId1"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument"
                Target="word/document.xml"/>
</Relationships>
)";
    constexpr auto content_types_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">
  <Default Extension="rels"
           ContentType="application/vnd.openxmlformats-package.relationships+xml"/>
  <Default Extension="xml" ContentType="application/xml"/>
  <Override PartName="/word/document.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml"/>
  <Override PartName="/word/styles.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.styles+xml"/>
</Types>
)";
    constexpr auto document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p><w:r><w:t>seed</w:t></w:r></w:p>
  </w:body>
</w:document>
)";
    constexpr auto document_relationships_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rId2"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/styles"
                Target="styles.xml"/>
</Relationships>
)";
    constexpr auto styles_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:styles xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:style w:type="paragraph" w:default="1" w:styleId="Normal">
    <w:name w:val="Normal"/>
  </w:style>
  <w:style w:type="paragraph" w:styleId="CustomBody" w:customStyle="1">
    <w:name w:val="Custom Body"/>
    <w:basedOn w:val="Normal"/>
    <w:qFormat/>
  </w:style>
  <w:style w:type="numbering" w:styleId="NumberedStyle">
    <w:name w:val="Numbered"/>
    <w:semiHidden/>
    <w:unhideWhenUsed/>
  </w:style>
  <w:style w:type="mystery" w:styleId="MysteryStyle">
    <w:name w:val="Mystery"/>
  </w:style>
</w:styles>
)";

    int zip_error = 0;
    zip_t *archive = zip_openwitherror(path.string().c_str(),
                                       ZIP_DEFAULT_COMPRESSION_LEVEL, 'w',
                                       &zip_error);
    REQUIRE(archive != nullptr);
    REQUIRE(write_archive_entry(archive, "_rels/.rels", relationships_xml));
    REQUIRE(write_archive_entry(archive, "[Content_Types].xml", content_types_xml));
    REQUIRE(write_archive_entry(archive, "word/document.xml", document_xml));
    REQUIRE(write_archive_entry(archive, "word/_rels/document.xml.rels",
                                document_relationships_xml));
    REQUIRE(write_archive_entry(archive, "word/styles.xml", styles_xml));
    zip_close(archive);
}

void create_cli_style_usage_fixture(const fs::path &path) {
    remove_if_exists(path);

    constexpr auto relationships_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rId1"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument"
                Target="word/document.xml"/>
</Relationships>
)";
    constexpr auto content_types_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">
  <Default Extension="rels"
           ContentType="application/vnd.openxmlformats-package.relationships+xml"/>
  <Default Extension="xml" ContentType="application/xml"/>
  <Override PartName="/word/document.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml"/>
  <Override PartName="/word/styles.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.styles+xml"/>
  <Override PartName="/word/header1.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.header+xml"/>
  <Override PartName="/word/footer1.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.footer+xml"/>
</Types>
)";
    constexpr auto document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"
            xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships">
  <w:body>
    <w:p>
      <w:pPr><w:pStyle w:val="CustomBody"/></w:pPr>
      <w:r><w:rPr><w:rStyle w:val="Strong"/></w:rPr><w:t>alpha</w:t></w:r>
    </w:p>
    <w:p>
      <w:pPr><w:pStyle w:val="CustomBody"/></w:pPr>
      <w:r><w:t>beta</w:t></w:r>
      <w:r><w:rPr><w:rStyle w:val="Strong"/></w:rPr><w:t>gamma</w:t></w:r>
    </w:p>
    <w:tbl>
      <w:tblPr>
        <w:tblStyle w:val="ReportTable"/>
        <w:tblW w:w="0" w:type="auto"/>
      </w:tblPr>
      <w:tr><w:tc><w:p><w:r><w:t>cell</w:t></w:r></w:p></w:tc></w:tr>
    </w:tbl>
    <w:sectPr>
      <w:headerReference w:type="default" r:id="rId3"/>
      <w:footerReference w:type="default" r:id="rId4"/>
    </w:sectPr>
  </w:body>
</w:document>
)";
    constexpr auto document_relationships_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rId2"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/styles"
                Target="styles.xml"/>
  <Relationship Id="rId3"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/header"
                Target="header1.xml"/>
  <Relationship Id="rId4"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/footer"
                Target="footer1.xml"/>
</Relationships>
)";
    constexpr auto styles_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:styles xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:style w:type="paragraph" w:default="1" w:styleId="Normal">
    <w:name w:val="Normal"/>
  </w:style>
  <w:style w:type="paragraph" w:styleId="CustomBody" w:customStyle="1">
    <w:name w:val="Custom Body"/>
    <w:basedOn w:val="Normal"/>
    <w:qFormat/>
  </w:style>
  <w:style w:type="character" w:styleId="Strong">
    <w:name w:val="Strong"/>
  </w:style>
  <w:style w:type="table" w:styleId="ReportTable">
    <w:name w:val="Report Table"/>
  </w:style>
</w:styles>
)";
    constexpr auto header_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:hdr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p>
    <w:pPr><w:pStyle w:val="CustomBody"/></w:pPr>
    <w:r><w:t>header paragraph</w:t></w:r>
  </w:p>
  <w:tbl>
    <w:tblPr>
      <w:tblStyle w:val="ReportTable"/>
      <w:tblW w:w="0" w:type="auto"/>
    </w:tblPr>
    <w:tr><w:tc><w:p><w:r><w:t>header cell</w:t></w:r></w:p></w:tc></w:tr>
  </w:tbl>
</w:hdr>
)";
    constexpr auto footer_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:ftr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p>
    <w:r><w:rPr><w:rStyle w:val="Strong"/></w:rPr><w:t>footer run</w:t></w:r>
  </w:p>
</w:ftr>
)";

    int zip_error = 0;
    zip_t *archive = zip_openwitherror(path.string().c_str(),
                                       ZIP_DEFAULT_COMPRESSION_LEVEL, 'w',
                                       &zip_error);
    REQUIRE(archive != nullptr);
    REQUIRE(write_archive_entry(archive, "_rels/.rels", relationships_xml));
    REQUIRE(write_archive_entry(archive, "[Content_Types].xml", content_types_xml));
    REQUIRE(write_archive_entry(archive, "word/document.xml", document_xml));
    REQUIRE(write_archive_entry(archive, "word/_rels/document.xml.rels",
                                document_relationships_xml));
    REQUIRE(write_archive_entry(archive, "word/styles.xml", styles_xml));
    REQUIRE(write_archive_entry(archive, "word/header1.xml", header_xml));
    REQUIRE(write_archive_entry(archive, "word/footer1.xml", footer_xml));
    zip_close(archive);
}


void create_cli_duplicate_style_fixture(const std::filesystem::path &path) {
    remove_if_exists(path);

    featherdoc::Document document(path);
    REQUIRE_FALSE(document.create_empty());

    featherdoc::paragraph_style_definition duplicate_a;
    duplicate_a.name = "Duplicate Body A";
    duplicate_a.based_on = "Normal";
    duplicate_a.run_font_family = "Aptos";
    REQUIRE(document.ensure_paragraph_style("DuplicateBodyA", duplicate_a));

    featherdoc::paragraph_style_definition duplicate_b;
    duplicate_b.name = "Duplicate Body B";
    duplicate_b.based_on = "Normal";
    duplicate_b.run_font_family = "Aptos";
    REQUIRE(document.ensure_paragraph_style("DuplicateBodyB", duplicate_b));

    auto first_paragraph = document.paragraphs();
    REQUIRE(first_paragraph.has_next());
    REQUIRE(first_paragraph.add_run("first duplicate style reference").has_next());
    REQUIRE(document.set_paragraph_style(first_paragraph, "DuplicateBodyA"));
    auto second_paragraph = first_paragraph.insert_paragraph_after("second");
    REQUIRE(document.set_paragraph_style(second_paragraph, "DuplicateBodyA"));
    auto third_paragraph = second_paragraph.insert_paragraph_after("third");
    REQUIRE(document.set_paragraph_style(third_paragraph, "DuplicateBodyB"));

    REQUIRE_FALSE(document.save());
}

void create_cli_duplicate_style_profile_fixture(const std::filesystem::path &path) {
    remove_if_exists(path);

    featherdoc::Document document(path);
    REQUIRE_FALSE(document.create_empty());

    featherdoc::paragraph_style_definition duplicate_a;
    duplicate_a.name = "Duplicate Body A";
    duplicate_a.based_on = "Normal";
    duplicate_a.run_font_family = "Aptos";
    REQUIRE(document.ensure_paragraph_style("DuplicateBodyA", duplicate_a));

    featherdoc::paragraph_style_definition duplicate_b;
    duplicate_b.name = "Duplicate Body B";
    duplicate_b.based_on = "Normal";
    duplicate_b.run_font_family = "Aptos";
    REQUIRE(document.ensure_paragraph_style("DuplicateBodyB", duplicate_b));

    featherdoc::paragraph_style_definition duplicate_c;
    duplicate_c.name = "Duplicate Body C";
    duplicate_c.based_on = "Normal";
    duplicate_c.next_style = "Normal";
    duplicate_c.run_font_family = "Aptos";
    REQUIRE(document.ensure_paragraph_style("DuplicateBodyC", duplicate_c));

    auto first_paragraph = document.paragraphs();
    REQUIRE(first_paragraph.has_next());
    REQUIRE(first_paragraph.add_run("first duplicate style reference").has_next());
    REQUIRE(document.set_paragraph_style(first_paragraph, "DuplicateBodyA"));
    auto second_paragraph = first_paragraph.insert_paragraph_after("second");
    REQUIRE(document.set_paragraph_style(second_paragraph, "DuplicateBodyA"));
    auto third_paragraph = second_paragraph.insert_paragraph_after("third");
    REQUIRE(document.set_paragraph_style(third_paragraph, "DuplicateBodyB"));
    auto fourth_paragraph = third_paragraph.insert_paragraph_after("fourth");
    REQUIRE(document.set_paragraph_style(fourth_paragraph, "DuplicateBodyC"));

    REQUIRE_FALSE(document.save());
}

auto find_style_xml_node(pugi::xml_node styles_root, std::string_view style_id)
    -> pugi::xml_node {
    for (auto style = styles_root.child("w:style"); style != pugi::xml_node{};
         style = style.next_sibling("w:style")) {
        if (std::string_view{style.attribute("w:styleId").value()} == style_id) {
            return style;
        }
    }

    return {};
}

void create_cli_paragraph_style_fixture(const fs::path &path,
                                        std::string_view style_id,
                                        std::string_view style_name,
                                        std::string_view based_on,
                                        bool paragraph_bidi) {
    remove_if_exists(path);

    featherdoc::Document document(path);
    REQUIRE_FALSE(document.create_empty());

    auto style = featherdoc::paragraph_style_definition{};
    style.name = std::string(style_name);
    style.based_on = std::string(based_on);
    style.is_quick_format = true;
    style.paragraph_bidi = paragraph_bidi;
    REQUIRE(document.ensure_paragraph_style(std::string(style_id), style));
    REQUIRE_FALSE(document.save());
}

void create_cli_style_numbering_audit_broken_fixture(const fs::path &path) {
    remove_if_exists(path);

    constexpr auto relationships_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rId1"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument"
                Target="word/document.xml"/>
</Relationships>
)";
    constexpr auto content_types_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">
  <Default Extension="rels"
           ContentType="application/vnd.openxmlformats-package.relationships+xml"/>
  <Default Extension="xml" ContentType="application/xml"/>
  <Override PartName="/word/document.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml"/>
  <Override PartName="/word/styles.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.styles+xml"/>
</Types>
)";
    constexpr auto document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p><w:r><w:t>seed</w:t></w:r></w:p>
  </w:body>
</w:document>
)";
    constexpr auto document_relationships_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rId2"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/styles"
                Target="styles.xml"/>
</Relationships>
)";
    constexpr auto styles_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:styles xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:style w:type="paragraph" w:default="1" w:styleId="Normal">
    <w:name w:val="Normal"/>
  </w:style>
  <w:style w:type="paragraph" w:styleId="MissingNumId" w:customStyle="1">
    <w:name w:val="Missing NumId"/>
    <w:basedOn w:val="Normal"/>
    <w:pPr>
      <w:numPr><w:ilvl w:val="0"/></w:numPr>
    </w:pPr>
  </w:style>
  <w:style w:type="paragraph" w:styleId="OrphanNumId" w:customStyle="1">
    <w:name w:val="Orphan NumId"/>
    <w:basedOn w:val="Normal"/>
    <w:pPr>
      <w:numPr><w:ilvl w:val="0"/><w:numId w:val="777"/></w:numPr>
    </w:pPr>
  </w:style>
  <w:style w:type="paragraph" w:styleId="PlainBody" w:customStyle="1">
    <w:name w:val="Plain Body"/>
    <w:basedOn w:val="Normal"/>
  </w:style>
</w:styles>
)";

    int zip_error = 0;
    zip_t *archive = zip_openwitherror(path.string().c_str(),
                                       ZIP_DEFAULT_COMPRESSION_LEVEL, 'w',
                                       &zip_error);
    REQUIRE(archive != nullptr);
    REQUIRE(write_archive_entry(archive, "_rels/.rels", relationships_xml));
    REQUIRE(write_archive_entry(archive, "[Content_Types].xml", content_types_xml));
    REQUIRE(write_archive_entry(archive, "word/document.xml", document_xml));
    REQUIRE(write_archive_entry(archive, "word/_rels/document.xml.rels",
                                document_relationships_xml));
    REQUIRE(write_archive_entry(archive, "word/styles.xml", styles_xml));
    zip_close(archive);
}

void create_cli_style_numbering_based_on_mismatch_fixture(const fs::path &path) {
    remove_if_exists(path);

    featherdoc::Document document(path);
    REQUIRE_FALSE(document.create_empty());

    auto base_style = featherdoc::paragraph_style_definition{};
    base_style.name = "Base Numbered";
    base_style.based_on = std::string{"Normal"};
    base_style.is_quick_format = true;
    REQUIRE(document.ensure_paragraph_style("BaseNumbered", base_style));

    auto child_style = featherdoc::paragraph_style_definition{};
    child_style.name = "Child Numbered";
    child_style.based_on = std::string{"BaseNumbered"};
    child_style.is_quick_format = true;
    REQUIRE(document.ensure_paragraph_style("ChildNumbered", child_style));

    auto base_definition = featherdoc::numbering_definition{};
    base_definition.name = "BaseOutline";
    base_definition.levels = {
        featherdoc::numbering_level_definition{
            featherdoc::list_kind::decimal, 1U, 0U, "%1."},
    };
    const auto base_definition_id =
        document.ensure_numbering_definition(base_definition);
    REQUIRE(base_definition_id.has_value());

    auto child_definition = featherdoc::numbering_definition{};
    child_definition.name = "ChildOutline";
    child_definition.levels = {
        featherdoc::numbering_level_definition{
            featherdoc::list_kind::decimal, 1U, 0U, "%1)"},
    };
    const auto child_definition_id =
        document.ensure_numbering_definition(child_definition);
    REQUIRE(child_definition_id.has_value());

    REQUIRE(document.set_paragraph_style_numbering("BaseNumbered",
                                                   *base_definition_id, 0U));
    REQUIRE(document.set_paragraph_style_numbering("ChildNumbered",
                                                   *child_definition_id, 0U));
    REQUIRE_FALSE(document.save());
}

void create_cli_style_numbering_missing_level_relink_fixture(
    const fs::path &path) {
    remove_if_exists(path);

    constexpr auto relationships_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rId1"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument"
                Target="word/document.xml"/>
</Relationships>
)";
    constexpr auto content_types_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">
  <Default Extension="rels"
           ContentType="application/vnd.openxmlformats-package.relationships+xml"/>
  <Default Extension="xml" ContentType="application/xml"/>
  <Override PartName="/word/document.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml"/>
  <Override PartName="/word/styles.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.styles+xml"/>
  <Override PartName="/word/numbering.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.numbering+xml"/>
</Types>
)";
    constexpr auto document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p><w:r><w:t>seed</w:t></w:r></w:p>
  </w:body>
</w:document>
)";
    constexpr auto document_relationships_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rId2"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/styles"
                Target="styles.xml"/>
  <Relationship Id="rId3"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/numbering"
                Target="numbering.xml"/>
</Relationships>
)";
    constexpr auto styles_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:styles xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:style w:type="paragraph" w:default="1" w:styleId="Normal">
    <w:name w:val="Normal"/>
  </w:style>
  <w:style w:type="paragraph" w:styleId="NeedsRelink" w:customStyle="1">
    <w:name w:val="Needs Relink"/>
    <w:basedOn w:val="Normal"/>
    <w:pPr>
      <w:numPr><w:ilvl w:val="1"/><w:numId w:val="7"/></w:numPr>
    </w:pPr>
  </w:style>
</w:styles>
)";
    constexpr auto numbering_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:numbering xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:abstractNum w:abstractNumId="1">
    <w:multiLevelType w:val="multilevel"/>
    <w:name w:val="SharedOutline"/>
    <w:lvl w:ilvl="0">
      <w:start w:val="1"/>
      <w:numFmt w:val="decimal"/>
      <w:lvlText w:val="%1."/>
    </w:lvl>
  </w:abstractNum>
  <w:abstractNum w:abstractNumId="2">
    <w:multiLevelType w:val="multilevel"/>
    <w:name w:val="SharedOutline"/>
    <w:lvl w:ilvl="0">
      <w:start w:val="1"/>
      <w:numFmt w:val="decimal"/>
      <w:lvlText w:val="%1."/>
    </w:lvl>
    <w:lvl w:ilvl="1">
      <w:start w:val="1"/>
      <w:numFmt w:val="decimal"/>
      <w:lvlText w:val="%1.%2."/>
    </w:lvl>
  </w:abstractNum>
  <w:num w:numId="7"><w:abstractNumId w:val="1"/></w:num>
  <w:num w:numId="8"><w:abstractNumId w:val="2"/></w:num>
</w:numbering>
)";

    int zip_error = 0;
    zip_t *archive = zip_openwitherror(path.string().c_str(),
                                       ZIP_DEFAULT_COMPRESSION_LEVEL, 'w',
                                       &zip_error);
    REQUIRE(archive != nullptr);
    REQUIRE(write_archive_entry(archive, "_rels/.rels", relationships_xml));
    REQUIRE(write_archive_entry(archive, "[Content_Types].xml", content_types_xml));
    REQUIRE(write_archive_entry(archive, "word/document.xml", document_xml));
    REQUIRE(write_archive_entry(archive, "word/_rels/document.xml.rels",
                                document_relationships_xml));
    REQUIRE(write_archive_entry(archive, "word/styles.xml", styles_xml));
    REQUIRE(write_archive_entry(archive, "word/numbering.xml", numbering_xml));
    zip_close(archive);
}

void create_cli_style_numbering_catalog_repair_fixture(const fs::path &path) {
    remove_if_exists(path);

    constexpr auto relationships_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rId1"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument"
                Target="word/document.xml"/>
</Relationships>
)";
    constexpr auto content_types_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">
  <Default Extension="rels"
           ContentType="application/vnd.openxmlformats-package.relationships+xml"/>
  <Default Extension="xml" ContentType="application/xml"/>
  <Override PartName="/word/document.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml"/>
  <Override PartName="/word/styles.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.styles+xml"/>
  <Override PartName="/word/numbering.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.numbering+xml"/>
</Types>
)";
    constexpr auto document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p><w:r><w:t>seed</w:t></w:r></w:p>
  </w:body>
</w:document>
)";
    constexpr auto document_relationships_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rId2"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/styles"
                Target="styles.xml"/>
  <Relationship Id="rId3"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/numbering"
                Target="numbering.xml"/>
</Relationships>
)";
    constexpr auto styles_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:styles xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:style w:type="paragraph" w:default="1" w:styleId="Normal">
    <w:name w:val="Normal"/>
  </w:style>
  <w:style w:type="paragraph" w:styleId="CatalogRepaired" w:customStyle="1">
    <w:name w:val="Catalog Repaired"/>
    <w:basedOn w:val="Normal"/>
    <w:pPr>
      <w:numPr><w:ilvl w:val="1"/><w:numId w:val="7"/></w:numPr>
    </w:pPr>
  </w:style>
</w:styles>
)";
    constexpr auto numbering_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:numbering xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:abstractNum w:abstractNumId="1">
    <w:multiLevelType w:val="multilevel"/>
    <w:name w:val="CatalogOutline"/>
    <w:lvl w:ilvl="0">
      <w:start w:val="1"/>
      <w:numFmt w:val="decimal"/>
      <w:lvlText w:val="%1."/>
    </w:lvl>
  </w:abstractNum>
  <w:num w:numId="7"><w:abstractNumId w:val="1"/></w:num>
</w:numbering>
)";

    int zip_error = 0;
    zip_t *archive = zip_openwitherror(path.string().c_str(),
                                       ZIP_DEFAULT_COMPRESSION_LEVEL, 'w',
                                       &zip_error);
    REQUIRE(archive != nullptr);
    REQUIRE(write_archive_entry(archive, "_rels/.rels", relationships_xml));
    REQUIRE(write_archive_entry(archive, "[Content_Types].xml", content_types_xml));
    REQUIRE(write_archive_entry(archive, "word/document.xml", document_xml));
    REQUIRE(write_archive_entry(archive, "word/_rels/document.xml.rels",
                                document_relationships_xml));
    REQUIRE(write_archive_entry(archive, "word/styles.xml", styles_xml));
    REQUIRE(write_archive_entry(archive, "word/numbering.xml", numbering_xml));
    zip_close(archive);
}

} // namespace
