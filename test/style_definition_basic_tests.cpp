#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

#include "doctest.h"
#include "basic_docx_archive_test_support.hpp"
#include "basic_document_xml_test_support.hpp"

#include <featherdoc.hpp>

TEST_CASE("ensure style definition APIs create paragraph character and table styles") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "ensure_style_definitions_roundtrip.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto paragraph_definition = featherdoc::paragraph_style_definition{};
    paragraph_definition.name = "Body Text";
    paragraph_definition.based_on = std::string{"Normal"};
    paragraph_definition.next_style = std::string{"BodyText"};
    paragraph_definition.is_quick_format = true;
    paragraph_definition.run_font_family = std::string{"Segoe UI"};
    paragraph_definition.run_east_asia_font_family = std::string{"Microsoft YaHei"};
    paragraph_definition.run_language = std::string{"en-US"};
    paragraph_definition.run_east_asia_language = std::string{"zh-CN"};
    paragraph_definition.run_bidi_language = std::string{"ar-SA"};
    paragraph_definition.run_rtl = false;
    paragraph_definition.paragraph_bidi = false;
    paragraph_definition.outline_level = 2U;
    CHECK(doc.ensure_paragraph_style("BodyText", paragraph_definition));

    auto character_definition = featherdoc::character_style_definition{};
    character_definition.name = "Accent Strong";
    character_definition.based_on = std::string{"DefaultParagraphFont"};
    character_definition.is_quick_format = true;
    character_definition.run_font_family = std::string{"Aptos"};
    character_definition.run_east_asia_font_family = std::string{"SimSun"};
    character_definition.run_language = std::string{"fr-FR"};
    character_definition.run_east_asia_language = std::string{"ja-JP"};
    character_definition.run_bidi_language = std::string{"he-IL"};
    character_definition.run_rtl = true;
    CHECK(doc.ensure_character_style("AccentStrong", character_definition));

    auto table_definition = featherdoc::table_style_definition{};
    table_definition.name = "Report Table";
    table_definition.based_on = std::string{"TableGrid"};
    table_definition.is_quick_format = true;
    CHECK(doc.ensure_table_style("ReportTable", table_definition));

    const auto styles = doc.list_styles();

    const auto *body_text = find_style_summary(styles, "BodyText");
    REQUIRE(body_text != nullptr);
    CHECK_EQ(body_text->kind, featherdoc::style_kind::paragraph);
    CHECK_EQ(body_text->name, "Body Text");
    CHECK(body_text->is_custom);
    CHECK(body_text->is_quick_format);
    REQUIRE(body_text->based_on.has_value());
    CHECK_EQ(*body_text->based_on, "Normal");

    const auto *accent_strong = find_style_summary(styles, "AccentStrong");
    REQUIRE(accent_strong != nullptr);
    CHECK_EQ(accent_strong->kind, featherdoc::style_kind::character);
    CHECK_EQ(accent_strong->name, "Accent Strong");
    CHECK(accent_strong->is_custom);
    CHECK(accent_strong->is_quick_format);

    const auto *report_table = find_style_summary(styles, "ReportTable");
    REQUIRE(report_table != nullptr);
    CHECK_EQ(report_table->kind, featherdoc::style_kind::table);
    CHECK_EQ(report_table->name, "Report Table");
    CHECK(report_table->is_custom);
    CHECK(report_table->is_quick_format);

    const auto body_font = doc.style_run_font_family("BodyText");
    REQUIRE(body_font.has_value());
    CHECK_EQ(*body_font, "Segoe UI");
    const auto body_east_asia_font = doc.style_run_east_asia_font_family("BodyText");
    REQUIRE(body_east_asia_font.has_value());
    CHECK_EQ(*body_east_asia_font, "Microsoft YaHei");
    const auto body_language = doc.style_run_language("BodyText");
    REQUIRE(body_language.has_value());
    CHECK_EQ(*body_language, "en-US");
    const auto body_east_asia_language = doc.style_run_east_asia_language("BodyText");
    REQUIRE(body_east_asia_language.has_value());
    CHECK_EQ(*body_east_asia_language, "zh-CN");
    const auto body_bidi_language = doc.style_run_bidi_language("BodyText");
    REQUIRE(body_bidi_language.has_value());
    CHECK_EQ(*body_bidi_language, "ar-SA");
    const auto body_rtl = doc.style_run_rtl("BodyText");
    REQUIRE(body_rtl.has_value());
    CHECK_FALSE(*body_rtl);
    const auto body_bidi = doc.style_paragraph_bidi("BodyText");
    REQUIRE(body_bidi.has_value());
    CHECK_FALSE(*body_bidi);

    const auto accent_font = doc.style_run_font_family("AccentStrong");
    REQUIRE(accent_font.has_value());
    CHECK_EQ(*accent_font, "Aptos");
    const auto accent_rtl = doc.style_run_rtl("AccentStrong");
    REQUIRE(accent_rtl.has_value());
    CHECK(*accent_rtl);

    auto paragraph = doc.paragraphs();
    REQUIRE(paragraph.has_next());
    CHECK(doc.set_paragraph_style(paragraph, "BodyText"));
    auto run = paragraph.add_run("styled body text");
    REQUIRE(run.has_next());
    CHECK(doc.set_run_style(run, "AccentStrong"));

    auto table = doc.append_table(1, 1);
    REQUIRE(table.has_next());
    CHECK(table.set_style_id("ReportTable"));

    CHECK_FALSE(doc.save());

    const auto saved_styles_xml = read_test_docx_entry(target, "word/styles.xml");
    CHECK_NE(saved_styles_xml.find("w:styleId=\"BodyText\""), std::string::npos);
    CHECK_NE(saved_styles_xml.find("w:styleId=\"AccentStrong\""), std::string::npos);
    CHECK_NE(saved_styles_xml.find("w:styleId=\"ReportTable\""), std::string::npos);
    CHECK_NE(saved_styles_xml.find("w:outlineLvl w:val=\"2\""), std::string::npos);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());

    const auto reopened_body = reopened.find_style("BodyText");
    REQUIRE(reopened_body.has_value());
    CHECK_EQ(reopened_body->name, "Body Text");
    const auto reopened_body_font = reopened.style_run_font_family("BodyText");
    REQUIRE(reopened_body_font.has_value());
    CHECK_EQ(*reopened_body_font, "Segoe UI");
    const auto reopened_accent_rtl = reopened.style_run_rtl("AccentStrong");
    REQUIRE(reopened_accent_rtl.has_value());
    CHECK(*reopened_accent_rtl);

    auto reopened_table = reopened.tables();
    REQUIRE(reopened_table.has_next());
    REQUIRE(reopened_table.style_id().has_value());
    CHECK_EQ(*reopened_table.style_id(), "ReportTable");

    fs::remove(target);
}

TEST_CASE("ensure style definition APIs update existing styles and preserve unrelated markup") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "ensure_existing_style_definitions.docx";
    fs::remove(target);

    const std::string content_types_xml =
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
    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p><w:r><w:t>seed</w:t></w:r></w:p>
    <w:tbl>
      <w:tr><w:tc><w:p><w:r><w:t>cell</w:t></w:r></w:p></w:tc></w:tr>
    </w:tbl>
  </w:body>
</w:document>
)";
    const std::string document_relationships_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rId2"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/styles"
                Target="styles.xml"/>
</Relationships>
)";
    const std::string styles_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:styles xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:style w:type="paragraph" w:styleId="ExistingPara">
    <w:name w:val="Old Paragraph"/>
    <w:uiPriority w:val="30"/>
    <w:qFormat/>
    <w:pPr><w:keepNext/></w:pPr>
    <w:rPr><w:b/></w:rPr>
  </w:style>
  <w:style w:type="paragraph" w:styleId="ExistingRebasedPara">
    <w:name w:val="Old Rebased Paragraph"/>
    <w:basedOn w:val="Normal"/>
    <w:pPr><w:keepLines/></w:pPr>
    <w:rPr>
      <w:b/>
      <w:rFonts w:ascii="Courier New" w:hAnsi="Courier New"/>
      <w:lang w:val="en-US"/>
      <w:rtl/>
    </w:rPr>
  </w:style>
  <w:style w:type="character" w:styleId="ExistingChar">
    <w:name w:val="Old Character"/>
    <w:rPr><w:i/></w:rPr>
  </w:style>
  <w:style w:type="character" w:styleId="ExistingRebasedChar">
    <w:name w:val="Old Rebased Character"/>
    <w:basedOn w:val="DefaultParagraphFont"/>
    <w:rPr>
      <w:i/>
      <w:rFonts w:ascii="Courier New" w:hAnsi="Courier New"/>
      <w:lang w:val="en-US"/>
      <w:rtl/>
    </w:rPr>
  </w:style>
  <w:style w:type="table" w:styleId="ExistingTable">
    <w:name w:val="Old Table"/>
    <w:semiHidden/>
    <w:tblPr>
      <w:tblBorders>
        <w:top w:val="single" w:sz="8" w:space="0" w:color="auto"/>
      </w:tblBorders>
    </w:tblPr>
  </w:style>
</w:styles>
)";

    write_test_archive_entries(
        target,
        {
            {test_content_types_xml_entry, content_types_xml},
            {test_relationships_xml_entry, test_relationships_xml},
            {test_document_xml_entry, document_xml},
            {"word/_rels/document.xml.rels", document_relationships_xml},
            {"word/styles.xml", styles_xml},
        });

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    auto paragraph_definition = featherdoc::paragraph_style_definition{};
    paragraph_definition.name = "Body Paragraph";
    paragraph_definition.based_on = std::string{"Normal"};
    paragraph_definition.next_style = std::string{"ExistingPara"};
    paragraph_definition.is_custom = true;
    paragraph_definition.is_quick_format = false;
    paragraph_definition.is_unhide_when_used = true;
    paragraph_definition.run_font_family = std::string{"Calibri"};
    paragraph_definition.run_language = std::string{"en-US"};
    paragraph_definition.paragraph_bidi = true;
    paragraph_definition.outline_level = 3U;
    CHECK(doc.ensure_paragraph_style("ExistingPara", paragraph_definition));

    auto rebased_paragraph_definition = featherdoc::paragraph_style_definition{};
    rebased_paragraph_definition.name = "Rebased Paragraph";
    rebased_paragraph_definition.based_on = std::string{"Heading2"};
    rebased_paragraph_definition.next_style = std::string{"ExistingRebasedPara"};
    rebased_paragraph_definition.is_custom = true;
    rebased_paragraph_definition.is_quick_format = true;
    CHECK(doc.ensure_paragraph_style("ExistingRebasedPara",
                                     rebased_paragraph_definition));

    auto character_definition = featherdoc::character_style_definition{};
    character_definition.name = "Accent Character";
    character_definition.based_on = std::string{"DefaultParagraphFont"};
    character_definition.is_custom = true;
    character_definition.is_quick_format = true;
    character_definition.is_semi_hidden = true;
    character_definition.run_language = std::string{"fr-FR"};
    character_definition.run_rtl = false;
    CHECK(doc.ensure_character_style("ExistingChar", character_definition));

    auto rebased_character_definition = featherdoc::character_style_definition{};
    rebased_character_definition.name = "Rebased Character";
    rebased_character_definition.based_on = std::string{"Emphasis"};
    rebased_character_definition.is_custom = true;
    rebased_character_definition.is_quick_format = true;
    CHECK(doc.ensure_character_style("ExistingRebasedChar",
                                     rebased_character_definition));

    auto table_definition = featherdoc::table_style_definition{};
    table_definition.name = "Report Table";
    table_definition.based_on = std::string{"TableGrid"};
    table_definition.is_custom = true;
    table_definition.is_quick_format = true;
    table_definition.is_semi_hidden = false;
    table_definition.is_unhide_when_used = true;
    CHECK(doc.ensure_table_style("ExistingTable", table_definition));

    CHECK_FALSE(doc.save());

    const auto saved_styles_xml = read_test_docx_entry(target, "word/styles.xml");
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(saved_styles_xml.c_str()));
    const auto styles_root = xml_document.child("w:styles");

    const auto paragraph_style = find_style_xml_node(styles_root, "ExistingPara");
    REQUIRE(paragraph_style != pugi::xml_node{});
    CHECK_EQ(std::string_view{paragraph_style.child("w:name").attribute("w:val").value()},
             "Body Paragraph");
    CHECK_EQ(std::string_view{paragraph_style.child("w:basedOn").attribute("w:val").value()},
             "Normal");
    CHECK_EQ(std::string_view{paragraph_style.child("w:next").attribute("w:val").value()},
             "ExistingPara");
    CHECK_EQ(std::string_view{paragraph_style.child("w:uiPriority").attribute("w:val").value()},
             "30");
    CHECK_EQ(paragraph_style.child("w:qFormat"), pugi::xml_node{});
    CHECK(paragraph_style.child("w:unhideWhenUsed") != pugi::xml_node{});
    CHECK(paragraph_style.child("w:pPr").child("w:keepNext") != pugi::xml_node{});
    CHECK(paragraph_style.child("w:pPr").child("w:outlineLvl") != pugi::xml_node{});
    CHECK_EQ(std::string_view{paragraph_style.child("w:pPr")
                                  .child("w:outlineLvl")
                                  .attribute("w:val")
                                  .value()},
             "3");
    CHECK(paragraph_style.child("w:pPr").child("w:bidi") != pugi::xml_node{});
    CHECK(paragraph_style.child("w:rPr").child("w:b") != pugi::xml_node{});
    CHECK(paragraph_style.child("w:rPr").child("w:rFonts") != pugi::xml_node{});
    CHECK(paragraph_style.child("w:rPr").child("w:lang") != pugi::xml_node{});

    const auto rebased_paragraph_style =
        find_style_xml_node(styles_root, "ExistingRebasedPara");
    REQUIRE(rebased_paragraph_style != pugi::xml_node{});
    CHECK_EQ(std::string_view{
                 rebased_paragraph_style.child("w:name").attribute("w:val").value()},
             "Rebased Paragraph");
    CHECK_EQ(std::string_view{rebased_paragraph_style.child("w:basedOn")
                                  .attribute("w:val")
                                  .value()},
             "Heading2");
    CHECK(rebased_paragraph_style.child("w:pPr").child("w:keepLines") !=
          pugi::xml_node{});
    CHECK(rebased_paragraph_style.child("w:rPr").child("w:b") !=
          pugi::xml_node{});
    CHECK(rebased_paragraph_style.child("w:rPr").child("w:rFonts") ==
          pugi::xml_node{});
    CHECK(rebased_paragraph_style.child("w:rPr").child("w:lang") ==
          pugi::xml_node{});
    CHECK(rebased_paragraph_style.child("w:rPr").child("w:rtl") ==
          pugi::xml_node{});

    const auto character_style = find_style_xml_node(styles_root, "ExistingChar");
    REQUIRE(character_style != pugi::xml_node{});
    CHECK_EQ(std::string_view{character_style.child("w:name").attribute("w:val").value()},
             "Accent Character");
    CHECK_EQ(std::string_view{character_style.child("w:basedOn").attribute("w:val").value()},
             "DefaultParagraphFont");
    CHECK(character_style.child("w:semiHidden") != pugi::xml_node{});
    CHECK(character_style.child("w:qFormat") != pugi::xml_node{});
    CHECK(character_style.child("w:rPr").child("w:i") != pugi::xml_node{});
    CHECK(character_style.child("w:rPr").child("w:lang") != pugi::xml_node{});
    CHECK(character_style.child("w:rPr").child("w:rtl") != pugi::xml_node{});
    CHECK_EQ(std::string_view{character_style.child("w:rPr")
                                  .child("w:lang")
                                  .attribute("w:val")
                                  .value()},
             "fr-FR");
    CHECK_EQ(std::string_view{character_style.child("w:rPr")
                                  .child("w:rtl")
                                  .attribute("w:val")
                                  .value()},
             "0");

    const auto rebased_character_style =
        find_style_xml_node(styles_root, "ExistingRebasedChar");
    REQUIRE(rebased_character_style != pugi::xml_node{});
    CHECK_EQ(std::string_view{
                 rebased_character_style.child("w:name").attribute("w:val").value()},
             "Rebased Character");
    CHECK_EQ(std::string_view{rebased_character_style.child("w:basedOn")
                                  .attribute("w:val")
                                  .value()},
             "Emphasis");
    CHECK(rebased_character_style.child("w:rPr").child("w:i") !=
          pugi::xml_node{});
    CHECK(rebased_character_style.child("w:rPr").child("w:rFonts") ==
          pugi::xml_node{});
    CHECK(rebased_character_style.child("w:rPr").child("w:lang") ==
          pugi::xml_node{});
    CHECK(rebased_character_style.child("w:rPr").child("w:rtl") ==
          pugi::xml_node{});

    const auto table_style = find_style_xml_node(styles_root, "ExistingTable");
    REQUIRE(table_style != pugi::xml_node{});
    CHECK_EQ(std::string_view{table_style.child("w:name").attribute("w:val").value()},
             "Report Table");
    CHECK_EQ(std::string_view{table_style.child("w:basedOn").attribute("w:val").value()},
             "TableGrid");
    CHECK_EQ(table_style.child("w:semiHidden"), pugi::xml_node{});
    CHECK(table_style.child("w:qFormat") != pugi::xml_node{});
    CHECK(table_style.child("w:unhideWhenUsed") != pugi::xml_node{});
    CHECK(table_style.child("w:tblPr").child("w:tblBorders") != pugi::xml_node{});
    CHECK(table_style.child("w:tblPr").child("w:tblBorders").child("w:top") !=
          pugi::xml_node{});

    fs::remove(target);
}
