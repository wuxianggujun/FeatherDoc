#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
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

TEST_CASE("ensure table style definition writes whole-table and conditional properties") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "ensure_table_style_properties.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto whole_margins = featherdoc::table_style_margins_definition{};
    whole_margins.top_twips = 80U;
    whole_margins.left_twips = 120U;
    whole_margins.bottom_twips = 80U;
    whole_margins.right_twips = 120U;

    auto whole_borders = featherdoc::table_style_borders_definition{};
    whole_borders.top = featherdoc::border_definition{
        featherdoc::border_style::single, 12U, "4472C4", 0U};
    whole_borders.inside_horizontal = featherdoc::border_definition{
        featherdoc::border_style::dotted, 4U, "A5A5A5", 0U};
    whole_borders.inside_vertical = featherdoc::border_definition{
        featherdoc::border_style::dashed, 4U, "A5A5A5", 0U};

    auto whole_table = featherdoc::table_style_region_definition{};
    whole_table.fill_color = std::string{"DDEEFF"};
    whole_table.text_color = std::string{"1F1F1F"};
    whole_table.bold = false;
    whole_table.italic = false;
    whole_table.font_size_points = 11U;
    whole_table.font_family = std::string{"Aptos"};
    whole_table.east_asia_font_family = std::string{"Microsoft YaHei"};
    whole_table.cell_vertical_alignment = featherdoc::cell_vertical_alignment::center;
    whole_table.cell_text_direction =
        featherdoc::cell_text_direction::left_to_right_top_to_bottom;
    whole_table.paragraph_alignment = featherdoc::paragraph_alignment::center;
    auto whole_paragraph_spacing = featherdoc::table_style_paragraph_spacing_definition{};
    whole_paragraph_spacing.before_twips = 120U;
    whole_paragraph_spacing.after_twips = 80U;
    whole_paragraph_spacing.line_twips = 360U;
    whole_paragraph_spacing.line_rule =
        featherdoc::paragraph_line_spacing_rule::exact;
    whole_table.paragraph_spacing = whole_paragraph_spacing;
    whole_table.cell_margins = whole_margins;
    whole_table.borders = whole_borders;

    auto first_row_margins = featherdoc::table_style_margins_definition{};
    first_row_margins.top_twips = 60U;
    first_row_margins.bottom_twips = 60U;

    auto first_row_borders = featherdoc::table_style_borders_definition{};
    first_row_borders.bottom = featherdoc::border_definition{
        featherdoc::border_style::double_line, 8U, "1F4E79", 0U};

    auto first_row = featherdoc::table_style_region_definition{};
    first_row.fill_color = std::string{"1F4E79"};
    first_row.text_color = std::string{"FFFFFF"};
    first_row.bold = true;
    first_row.italic = true;
    first_row.font_size_points = 14U;
    first_row.font_family = std::string{"Aptos Display"};
    first_row.east_asia_font_family = std::string{"SimHei"};
    first_row.cell_vertical_alignment = featherdoc::cell_vertical_alignment::bottom;
    first_row.cell_text_direction =
        featherdoc::cell_text_direction::top_to_bottom_right_to_left;
    first_row.paragraph_alignment = featherdoc::paragraph_alignment::right;
    auto first_row_paragraph_spacing =
        featherdoc::table_style_paragraph_spacing_definition{};
    first_row_paragraph_spacing.after_twips = 120U;
    first_row_paragraph_spacing.line_twips = 240U;
    first_row_paragraph_spacing.line_rule =
        featherdoc::paragraph_line_spacing_rule::at_least;
    first_row.paragraph_spacing = first_row_paragraph_spacing;
    first_row.cell_margins = first_row_margins;
    first_row.borders = first_row_borders;

    auto second_banded_rows = featherdoc::table_style_region_definition{};
    second_banded_rows.fill_color = std::string{"F2F2F2"};
    second_banded_rows.text_color = std::string{"666666"};

    auto second_banded_columns = featherdoc::table_style_region_definition{};
    second_banded_columns.fill_color = std::string{"E2F0D9"};
    second_banded_columns.cell_vertical_alignment =
        featherdoc::cell_vertical_alignment::top;

    auto definition = featherdoc::table_style_definition{};
    definition.name = "Invoice Grid";
    definition.based_on = std::string{"TableGrid"};
    definition.is_quick_format = true;
    definition.whole_table = whole_table;
    definition.first_row = first_row;
    definition.second_banded_rows = second_banded_rows;
    definition.second_banded_columns = second_banded_columns;

    CHECK(doc.ensure_table_style("InvoiceGrid", definition));
    CHECK_FALSE(doc.save());

    const auto saved_styles_xml = read_test_docx_entry(target, "word/styles.xml");
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(saved_styles_xml.c_str()));
    const auto style = find_style_xml_node(xml_document.child("w:styles"), "InvoiceGrid");
    REQUIRE(style != pugi::xml_node{});

    const auto table_properties = style.child("w:tblPr");
    REQUIRE(table_properties != pugi::xml_node{});
    CHECK_EQ(std::string_view{table_properties.child("w:tblCellMar")
                                  .child("w:left")
                                  .attribute("w:w")
                                  .value()},
             "120");
    CHECK_EQ(std::string_view{table_properties.child("w:tblBorders")
                                  .child("w:top")
                                  .attribute("w:color")
                                  .value()},
             "4472C4");
    CHECK_EQ(std::string_view{table_properties.child("w:tblBorders")
                                  .child("w:insideH")
                                  .attribute("w:val")
                                  .value()},
             "dotted");

    const auto whole_cell_properties = style.child("w:tcPr");
    REQUIRE(whole_cell_properties != pugi::xml_node{});
    CHECK_EQ(std::string_view{whole_cell_properties.child("w:shd")
                                  .attribute("w:fill")
                                  .value()},
             "DDEEFF");
    CHECK_EQ(std::string_view{whole_cell_properties.child("w:vAlign")
                                  .attribute("w:val")
                                  .value()},
             "center");
    CHECK_EQ(std::string_view{whole_cell_properties.child("w:textDirection")
                                  .attribute("w:val")
                                  .value()},
             "lrTb");
    CHECK_EQ(std::string_view{style.child("w:pPr")
                                  .child("w:jc")
                                  .attribute("w:val")
                                  .value()},
             "center");
    const auto whole_paragraph_spacing_node =
        style.child("w:pPr").child("w:spacing");
    REQUIRE(whole_paragraph_spacing_node != pugi::xml_node{});
    CHECK_EQ(std::string_view{whole_paragraph_spacing_node.attribute("w:before")
                                  .value()},
             "120");
    CHECK_EQ(std::string_view{whole_paragraph_spacing_node.attribute("w:after")
                                  .value()},
             "80");
    CHECK_EQ(std::string_view{whole_paragraph_spacing_node.attribute("w:line")
                                  .value()},
             "360");
    CHECK_EQ(std::string_view{whole_paragraph_spacing_node.attribute("w:lineRule")
                                  .value()},
             "exact");
    CHECK_EQ(std::string_view{style.child("w:rPr")
                                  .child("w:color")
                                  .attribute("w:val")
                                  .value()},
             "1F1F1F");
    CHECK_EQ(std::string_view{style.child("w:rPr")
                                  .child("w:b")
                                  .attribute("w:val")
                                  .value()},
             "0");
    CHECK_EQ(std::string_view{style.child("w:rPr")
                                  .child("w:i")
                                  .attribute("w:val")
                                  .value()},
             "0");
    CHECK_EQ(std::string_view{style.child("w:rPr")
                                  .child("w:sz")
                                  .attribute("w:val")
                                  .value()},
             "22");
    CHECK_EQ(std::string_view{style.child("w:rPr")
                                  .child("w:szCs")
                                  .attribute("w:val")
                                  .value()},
             "22");
    CHECK_EQ(std::string_view{style.child("w:rPr")
                                  .child("w:rFonts")
                                  .attribute("w:ascii")
                                  .value()},
             "Aptos");
    CHECK_EQ(std::string_view{style.child("w:rPr")
                                  .child("w:rFonts")
                                  .attribute("w:eastAsia")
                                  .value()},
             "Microsoft YaHei");

    const auto first_row_region = find_table_style_region_xml_node(style, "firstRow");
    REQUIRE(first_row_region != pugi::xml_node{});
    const auto first_row_cell_properties = first_row_region.child("w:tcPr");
    REQUIRE(first_row_cell_properties != pugi::xml_node{});
    CHECK_EQ(std::string_view{first_row_cell_properties.child("w:shd")
                                  .attribute("w:fill")
                                  .value()},
             "1F4E79");
    CHECK_EQ(std::string_view{first_row_cell_properties.child("w:vAlign")
                                  .attribute("w:val")
                                  .value()},
             "bottom");
    CHECK_EQ(std::string_view{first_row_cell_properties.child("w:textDirection")
                                  .attribute("w:val")
                                  .value()},
             "tbRl");
    CHECK_EQ(std::string_view{first_row_region.child("w:pPr")
                                  .child("w:jc")
                                  .attribute("w:val")
                                  .value()},
             "right");
    const auto first_row_paragraph_spacing_node =
        first_row_region.child("w:pPr").child("w:spacing");
    REQUIRE(first_row_paragraph_spacing_node != pugi::xml_node{});
    CHECK_EQ(std::string_view{first_row_paragraph_spacing_node.attribute("w:after")
                                  .value()},
             "120");
    CHECK_EQ(std::string_view{first_row_paragraph_spacing_node.attribute("w:line")
                                  .value()},
             "240");
    CHECK_EQ(std::string_view{first_row_paragraph_spacing_node.attribute("w:lineRule")
                                  .value()},
             "atLeast");
    CHECK_EQ(std::string_view{first_row_region.child("w:rPr")
                                  .child("w:color")
                                  .attribute("w:val")
                                  .value()},
             "FFFFFF");
    CHECK_EQ(std::string_view{first_row_region.child("w:rPr")
                                  .child("w:b")
                                  .attribute("w:val")
                                  .value()},
             "1");
    CHECK_EQ(std::string_view{first_row_region.child("w:rPr")
                                  .child("w:i")
                                  .attribute("w:val")
                                  .value()},
             "1");
    CHECK_EQ(std::string_view{first_row_region.child("w:rPr")
                                  .child("w:sz")
                                  .attribute("w:val")
                                  .value()},
             "28");
    CHECK_EQ(std::string_view{first_row_region.child("w:rPr")
                                  .child("w:szCs")
                                  .attribute("w:val")
                                  .value()},
             "28");
    CHECK_EQ(std::string_view{first_row_region.child("w:rPr")
                                  .child("w:rFonts")
                                  .attribute("w:ascii")
                                  .value()},
             "Aptos Display");
    CHECK_EQ(std::string_view{first_row_region.child("w:rPr")
                                  .child("w:rFonts")
                                  .attribute("w:eastAsia")
                                  .value()},
             "SimHei");
    CHECK_EQ(std::string_view{first_row_cell_properties.child("w:tcMar")
                                  .child("w:bottom")
                                  .attribute("w:w")
                                  .value()},
             "60");
    CHECK_EQ(std::string_view{first_row_cell_properties.child("w:tcBorders")
                                  .child("w:bottom")
                                  .attribute("w:val")
                                  .value()},
             "double");

    const auto second_banded_rows_region =
        find_table_style_region_xml_node(style, "band2Horz");
    REQUIRE(second_banded_rows_region != pugi::xml_node{});
    CHECK_EQ(std::string_view{second_banded_rows_region.child("w:tcPr")
                                  .child("w:shd")
                                  .attribute("w:fill")
                                  .value()},
             "F2F2F2");
    CHECK_EQ(std::string_view{second_banded_rows_region.child("w:rPr")
                                  .child("w:color")
                                  .attribute("w:val")
                                  .value()},
             "666666");

    const auto second_banded_columns_region =
        find_table_style_region_xml_node(style, "band2Vert");
    REQUIRE(second_banded_columns_region != pugi::xml_node{});
    CHECK_EQ(std::string_view{second_banded_columns_region.child("w:tcPr")
                                  .child("w:shd")
                                  .attribute("w:fill")
                                  .value()},
             "E2F0D9");
    CHECK_EQ(std::string_view{second_banded_columns_region.child("w:tcPr")
                                  .child("w:vAlign")
                                  .attribute("w:val")
                                  .value()},
             "top");

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    REQUIRE(reopened.find_style("InvoiceGrid").has_value());
    const auto inspected = reopened.find_table_style_definition("InvoiceGrid");
    REQUIRE(inspected.has_value());
    CHECK_EQ(inspected->style.style_id, "InvoiceGrid");
    REQUIRE(inspected->whole_table.has_value());
    CHECK_EQ(inspected->whole_table->fill_color, std::optional<std::string>{"DDEEFF"});
    CHECK_EQ(inspected->whole_table->text_color, std::optional<std::string>{"1F1F1F"});
    CHECK_EQ(inspected->whole_table->bold, std::optional<bool>{false});
    CHECK_EQ(inspected->whole_table->italic, std::optional<bool>{false});
    CHECK_EQ(inspected->whole_table->font_size_points,
             std::optional<std::uint32_t>{11U});
    CHECK_EQ(inspected->whole_table->font_family,
             std::optional<std::string>{"Aptos"});
    CHECK_EQ(inspected->whole_table->east_asia_font_family,
             std::optional<std::string>{"Microsoft YaHei"});
    CHECK_EQ(inspected->whole_table->cell_vertical_alignment,
             std::optional<featherdoc::cell_vertical_alignment>{
                 featherdoc::cell_vertical_alignment::center});
    CHECK_EQ(inspected->whole_table->cell_text_direction,
             std::optional<featherdoc::cell_text_direction>{
                 featherdoc::cell_text_direction::left_to_right_top_to_bottom});
    CHECK_EQ(inspected->whole_table->paragraph_alignment,
             std::optional<featherdoc::paragraph_alignment>{
                 featherdoc::paragraph_alignment::center});
    REQUIRE(inspected->whole_table->paragraph_spacing.has_value());
    CHECK_EQ(inspected->whole_table->paragraph_spacing->before_twips,
             std::optional<std::uint32_t>{120U});
    CHECK_EQ(inspected->whole_table->paragraph_spacing->after_twips,
             std::optional<std::uint32_t>{80U});
    CHECK_EQ(inspected->whole_table->paragraph_spacing->line_twips,
             std::optional<std::uint32_t>{360U});
    CHECK_EQ(inspected->whole_table->paragraph_spacing->line_rule,
             std::optional<featherdoc::paragraph_line_spacing_rule>{
                 featherdoc::paragraph_line_spacing_rule::exact});
    REQUIRE(inspected->whole_table->cell_margins.has_value());
    CHECK_EQ(inspected->whole_table->cell_margins->left_twips,
             std::optional<std::uint32_t>{120U});
    REQUIRE(inspected->whole_table->borders.has_value());
    REQUIRE(inspected->whole_table->borders->top.has_value());
    CHECK_EQ(inspected->whole_table->borders->top->style,
             std::optional<std::string>{"single"});
    CHECK_EQ(inspected->whole_table->borders->top->color,
             std::optional<std::string>{"4472C4"});
    REQUIRE(inspected->first_row.has_value());
    CHECK_EQ(inspected->first_row->fill_color, std::optional<std::string>{"1F4E79"});
    CHECK_EQ(inspected->first_row->text_color, std::optional<std::string>{"FFFFFF"});
    CHECK_EQ(inspected->first_row->bold, std::optional<bool>{true});
    CHECK_EQ(inspected->first_row->italic, std::optional<bool>{true});
    CHECK_EQ(inspected->first_row->font_size_points,
             std::optional<std::uint32_t>{14U});
    CHECK_EQ(inspected->first_row->font_family,
             std::optional<std::string>{"Aptos Display"});
    CHECK_EQ(inspected->first_row->east_asia_font_family,
             std::optional<std::string>{"SimHei"});
    CHECK_EQ(inspected->first_row->cell_vertical_alignment,
             std::optional<featherdoc::cell_vertical_alignment>{
                 featherdoc::cell_vertical_alignment::bottom});
    CHECK_EQ(inspected->first_row->cell_text_direction,
             std::optional<featherdoc::cell_text_direction>{
                 featherdoc::cell_text_direction::top_to_bottom_right_to_left});
    CHECK_EQ(inspected->first_row->paragraph_alignment,
             std::optional<featherdoc::paragraph_alignment>{
                 featherdoc::paragraph_alignment::right});
    REQUIRE(inspected->first_row->paragraph_spacing.has_value());
    CHECK_EQ(inspected->first_row->paragraph_spacing->after_twips,
             std::optional<std::uint32_t>{120U});
    CHECK_EQ(inspected->first_row->paragraph_spacing->line_twips,
             std::optional<std::uint32_t>{240U});
    CHECK_EQ(inspected->first_row->paragraph_spacing->line_rule,
             std::optional<featherdoc::paragraph_line_spacing_rule>{
                 featherdoc::paragraph_line_spacing_rule::at_least});
    REQUIRE(inspected->first_row->borders.has_value());
    REQUIRE(inspected->first_row->borders->bottom.has_value());
    CHECK_EQ(inspected->first_row->borders->bottom->style,
             std::optional<std::string>{"double"});
    REQUIRE(inspected->second_banded_rows.has_value());
    CHECK_EQ(inspected->second_banded_rows->fill_color,
             std::optional<std::string>{"F2F2F2"});
    CHECK_EQ(inspected->second_banded_rows->text_color,
             std::optional<std::string>{"666666"});
    REQUIRE(inspected->second_banded_columns.has_value());
    CHECK_EQ(inspected->second_banded_columns->fill_color,
             std::optional<std::string>{"E2F0D9"});
    CHECK_EQ(inspected->second_banded_columns->cell_vertical_alignment,
             std::optional<featherdoc::cell_vertical_alignment>{
                 featherdoc::cell_vertical_alignment::top});

    fs::remove(target);
}

TEST_CASE("ensure style definition APIs validate definitions and reject type mismatches") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "ensure_style_definitions_validation.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto paragraph_definition = featherdoc::paragraph_style_definition{};
    paragraph_definition.name = "Body";
    CHECK_FALSE(doc.ensure_paragraph_style("", paragraph_definition));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));

    paragraph_definition.name.clear();
    CHECK_FALSE(doc.ensure_paragraph_style("BodyText", paragraph_definition));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));

    paragraph_definition.name = "Body";
    paragraph_definition.run_font_family = std::string{};
    CHECK_FALSE(doc.ensure_paragraph_style("BodyText", paragraph_definition));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));

    paragraph_definition.run_font_family = std::string{"Segoe UI"};
    paragraph_definition.outline_level = 9U;
    CHECK_FALSE(doc.ensure_paragraph_style("BodyText", paragraph_definition));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));

    auto table_definition = featherdoc::table_style_definition{};
    table_definition.name = "Wrong Type";
    CHECK_FALSE(doc.ensure_table_style("Heading1", table_definition));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));

    CHECK_FALSE(doc.find_table_style_definition("Heading1").has_value());
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));

    table_definition.name = "Table With Bad Fill";
    table_definition.whole_table = featherdoc::table_style_region_definition{};
    table_definition.whole_table->fill_color = std::string{};
    CHECK_FALSE(doc.ensure_table_style("BadFillTable", table_definition));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));

    table_definition.name = "Table With Bad Text Color";
    table_definition.whole_table->fill_color = std::nullopt;
    table_definition.whole_table->text_color = std::string{};
    CHECK_FALSE(doc.ensure_table_style("BadTextColorTable", table_definition));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));

    table_definition.name = "Table With Bad Font Size";
    table_definition.whole_table->text_color = std::nullopt;
    table_definition.whole_table->font_size_points = 0U;
    CHECK_FALSE(doc.ensure_table_style("BadFontSizeTable", table_definition));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));

    table_definition.name = "Table With Bad Font Family";
    table_definition.whole_table->font_size_points = std::nullopt;
    table_definition.whole_table->font_family = std::string{};
    CHECK_FALSE(doc.ensure_table_style("BadFontFamilyTable", table_definition));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));

    table_definition.name = "Table With Bad East Asia Font Family";
    table_definition.whole_table->font_family = std::nullopt;
    table_definition.whole_table->east_asia_font_family = std::string{};
    CHECK_FALSE(doc.ensure_table_style("BadEastAsiaFontFamilyTable", table_definition));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));

    fs::remove(target);
}

TEST_CASE("resolve_style_properties follows basedOn chains and reports value sources") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "resolve_style_properties_roundtrip.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    CHECK(doc.set_style_run_bidi_language("Normal", "ar-SA"));

    auto base_definition = featherdoc::paragraph_style_definition{};
    base_definition.name = "Base Style";
    base_definition.based_on = std::string{"Normal"};
    base_definition.run_font_family = std::string{"Segoe UI"};
    base_definition.run_east_asia_language = std::string{"zh-CN"};
    base_definition.run_rtl = true;
    CHECK(doc.ensure_paragraph_style("BaseStyle", base_definition));

    auto child_definition = featherdoc::paragraph_style_definition{};
    child_definition.name = "Child Style";
    child_definition.based_on = std::string{"BaseStyle"};
    child_definition.run_language = std::string{"en-US"};
    child_definition.paragraph_bidi = true;
    CHECK(doc.ensure_paragraph_style("ChildStyle", child_definition));

    const auto resolved = doc.resolve_style_properties("ChildStyle");
    REQUIRE(resolved.has_value());
    CHECK_EQ(resolved->style_id, "ChildStyle");
    CHECK_EQ(resolved->type_name, "paragraph");
    CHECK_EQ(resolved->kind, featherdoc::style_kind::paragraph);
    REQUIRE(resolved->based_on.has_value());
    CHECK_EQ(*resolved->based_on, "BaseStyle");
    CHECK_EQ(resolved->inheritance_chain.size(), 3U);
    CHECK_EQ(resolved->inheritance_chain[0], "ChildStyle");
    CHECK_EQ(resolved->inheritance_chain[1], "BaseStyle");
    CHECK_EQ(resolved->inheritance_chain[2], "Normal");

    REQUIRE(resolved->run_font_family.value.has_value());
    REQUIRE(resolved->run_font_family.source_style_id.has_value());
    CHECK_EQ(*resolved->run_font_family.value, "Segoe UI");
    CHECK_EQ(*resolved->run_font_family.source_style_id, "BaseStyle");

    CHECK_FALSE(resolved->run_east_asia_font_family.value.has_value());
    CHECK_FALSE(resolved->run_east_asia_font_family.source_style_id.has_value());

    REQUIRE(resolved->run_language.value.has_value());
    REQUIRE(resolved->run_language.source_style_id.has_value());
    CHECK_EQ(*resolved->run_language.value, "en-US");
    CHECK_EQ(*resolved->run_language.source_style_id, "ChildStyle");

    REQUIRE(resolved->run_east_asia_language.value.has_value());
    REQUIRE(resolved->run_east_asia_language.source_style_id.has_value());
    CHECK_EQ(*resolved->run_east_asia_language.value, "zh-CN");
    CHECK_EQ(*resolved->run_east_asia_language.source_style_id, "BaseStyle");

    REQUIRE(resolved->run_bidi_language.value.has_value());
    REQUIRE(resolved->run_bidi_language.source_style_id.has_value());
    CHECK_EQ(*resolved->run_bidi_language.value, "ar-SA");
    CHECK_EQ(*resolved->run_bidi_language.source_style_id, "Normal");

    REQUIRE(resolved->run_rtl.value.has_value());
    REQUIRE(resolved->run_rtl.source_style_id.has_value());
    CHECK(*resolved->run_rtl.value);
    CHECK_EQ(*resolved->run_rtl.source_style_id, "BaseStyle");

    REQUIRE(resolved->paragraph_bidi.value.has_value());
    REQUIRE(resolved->paragraph_bidi.source_style_id.has_value());
    CHECK(*resolved->paragraph_bidi.value);
    CHECK_EQ(*resolved->paragraph_bidi.source_style_id, "ChildStyle");

    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    const auto reopened_resolved = reopened.resolve_style_properties("ChildStyle");
    REQUIRE(reopened_resolved.has_value());
    REQUIRE(reopened_resolved->run_bidi_language.value.has_value());
    REQUIRE(reopened_resolved->run_bidi_language.source_style_id.has_value());
    CHECK_EQ(*reopened_resolved->run_bidi_language.value, "ar-SA");
    CHECK_EQ(*reopened_resolved->run_bidi_language.source_style_id, "Normal");

    fs::remove(target);
}

TEST_CASE("materialize_style_run_properties freezes inherited style metadata on the child style") {
    namespace fs = std::filesystem;

    const fs::path target =
        fs::current_path() / "materialize_style_run_properties_roundtrip.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    CHECK(doc.set_style_run_bidi_language("Normal", "ar-SA"));

    auto base_definition = featherdoc::paragraph_style_definition{};
    base_definition.name = "Base Style";
    base_definition.based_on = std::string{"Normal"};
    base_definition.run_font_family = std::string{"Segoe UI"};
    base_definition.run_east_asia_language = std::string{"zh-CN"};
    base_definition.run_rtl = true;
    base_definition.paragraph_bidi = true;
    CHECK(doc.ensure_paragraph_style("BaseStyle", base_definition));

    auto child_definition = featherdoc::paragraph_style_definition{};
    child_definition.name = "Child Style";
    child_definition.based_on = std::string{"BaseStyle"};
    CHECK(doc.ensure_paragraph_style("ChildStyle", child_definition));

    const auto before = doc.resolve_style_properties("ChildStyle");
    REQUIRE(before.has_value());
    REQUIRE(before->run_font_family.source_style_id.has_value());
    CHECK_EQ(*before->run_font_family.source_style_id, "BaseStyle");
    REQUIRE(before->run_bidi_language.source_style_id.has_value());
    CHECK_EQ(*before->run_bidi_language.source_style_id, "Normal");

    CHECK(doc.materialize_style_run_properties("ChildStyle"));

    REQUIRE(doc.style_run_font_family("ChildStyle").has_value());
    CHECK_EQ(*doc.style_run_font_family("ChildStyle"), "Segoe UI");
    REQUIRE(doc.style_run_east_asia_language("ChildStyle").has_value());
    CHECK_EQ(*doc.style_run_east_asia_language("ChildStyle"), "zh-CN");
    REQUIRE(doc.style_run_bidi_language("ChildStyle").has_value());
    CHECK_EQ(*doc.style_run_bidi_language("ChildStyle"), "ar-SA");
    REQUIRE(doc.style_run_rtl("ChildStyle").has_value());
    CHECK(*doc.style_run_rtl("ChildStyle"));
    REQUIRE(doc.style_paragraph_bidi("ChildStyle").has_value());
    CHECK(*doc.style_paragraph_bidi("ChildStyle"));

    CHECK(doc.set_style_run_font_family("BaseStyle", "Arial"));
    CHECK(doc.set_style_run_east_asia_language("BaseStyle", "ja-JP"));
    CHECK(doc.set_style_run_rtl("BaseStyle", false));
    CHECK(doc.set_style_paragraph_bidi("BaseStyle", false));
    CHECK(doc.set_style_run_bidi_language("Normal", "he-IL"));

    const auto after = doc.resolve_style_properties("ChildStyle");
    REQUIRE(after.has_value());
    REQUIRE(after->run_font_family.value.has_value());
    REQUIRE(after->run_font_family.source_style_id.has_value());
    CHECK_EQ(*after->run_font_family.value, "Segoe UI");
    CHECK_EQ(*after->run_font_family.source_style_id, "ChildStyle");
    REQUIRE(after->run_east_asia_language.value.has_value());
    REQUIRE(after->run_east_asia_language.source_style_id.has_value());
    CHECK_EQ(*after->run_east_asia_language.value, "zh-CN");
    CHECK_EQ(*after->run_east_asia_language.source_style_id, "ChildStyle");
    REQUIRE(after->run_bidi_language.value.has_value());
    REQUIRE(after->run_bidi_language.source_style_id.has_value());
    CHECK_EQ(*after->run_bidi_language.value, "ar-SA");
    CHECK_EQ(*after->run_bidi_language.source_style_id, "ChildStyle");
    REQUIRE(after->run_rtl.value.has_value());
    REQUIRE(after->run_rtl.source_style_id.has_value());
    CHECK(*after->run_rtl.value);
    CHECK_EQ(*after->run_rtl.source_style_id, "ChildStyle");
    REQUIRE(after->paragraph_bidi.value.has_value());
    REQUIRE(after->paragraph_bidi.source_style_id.has_value());
    CHECK(*after->paragraph_bidi.value);
    CHECK_EQ(*after->paragraph_bidi.source_style_id, "ChildStyle");

    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    const auto reopened_resolved = reopened.resolve_style_properties("ChildStyle");
    REQUIRE(reopened_resolved.has_value());
    REQUIRE(reopened_resolved->run_font_family.source_style_id.has_value());
    CHECK_EQ(*reopened_resolved->run_font_family.source_style_id, "ChildStyle");
    REQUIRE(reopened_resolved->run_bidi_language.source_style_id.has_value());
    CHECK_EQ(*reopened_resolved->run_bidi_language.source_style_id, "ChildStyle");

    fs::remove(target);
}

TEST_CASE("rebase_paragraph_style_based_on preserves resolved inherited values while switching parent") {
    namespace fs = std::filesystem;

    const fs::path target =
        fs::current_path() / "rebase_paragraph_style_based_on_roundtrip.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto base_a = featherdoc::paragraph_style_definition{};
    base_a.name = "Base A";
    base_a.based_on = std::string{"Normal"};
    base_a.run_font_family = std::string{"Segoe UI"};
    base_a.run_east_asia_language = std::string{"zh-CN"};
    base_a.run_rtl = true;
    CHECK(doc.ensure_paragraph_style("BaseA", base_a));

    auto base_b = featherdoc::paragraph_style_definition{};
    base_b.name = "Base B";
    base_b.based_on = std::string{"Normal"};
    base_b.run_font_family = std::string{"Arial"};
    base_b.run_east_asia_language = std::string{"ja-JP"};
    base_b.run_rtl = false;
    CHECK(doc.ensure_paragraph_style("BaseB", base_b));

    auto child_definition = featherdoc::paragraph_style_definition{};
    child_definition.name = "Child Style";
    child_definition.based_on = std::string{"BaseA"};
    child_definition.run_language = std::string{"en-US"};
    CHECK(doc.ensure_paragraph_style("ChildStyle", child_definition));

    const auto before = doc.resolve_style_properties("ChildStyle");
    REQUIRE(before.has_value());
    REQUIRE(before->run_font_family.value.has_value());
    CHECK_EQ(*before->run_font_family.value, "Segoe UI");
    REQUIRE(before->run_font_family.source_style_id.has_value());
    CHECK_EQ(*before->run_font_family.source_style_id, "BaseA");

    CHECK(doc.rebase_paragraph_style_based_on("ChildStyle", "BaseB"));

    const auto summary = doc.find_style("ChildStyle");
    REQUIRE(summary.has_value());
    REQUIRE(summary->based_on.has_value());
    CHECK_EQ(*summary->based_on, "BaseB");

    const auto after = doc.resolve_style_properties("ChildStyle");
    REQUIRE(after.has_value());
    REQUIRE(after->based_on.has_value());
    CHECK_EQ(*after->based_on, "BaseB");
    REQUIRE(after->run_font_family.value.has_value());
    REQUIRE(after->run_font_family.source_style_id.has_value());
    CHECK_EQ(*after->run_font_family.value, "Segoe UI");
    CHECK_EQ(*after->run_font_family.source_style_id, "ChildStyle");
    REQUIRE(after->run_east_asia_language.value.has_value());
    REQUIRE(after->run_east_asia_language.source_style_id.has_value());
    CHECK_EQ(*after->run_east_asia_language.value, "zh-CN");
    CHECK_EQ(*after->run_east_asia_language.source_style_id, "ChildStyle");
    REQUIRE(after->run_language.value.has_value());
    REQUIRE(after->run_language.source_style_id.has_value());
    CHECK_EQ(*after->run_language.value, "en-US");
    CHECK_EQ(*after->run_language.source_style_id, "ChildStyle");
    REQUIRE(after->run_rtl.value.has_value());
    REQUIRE(after->run_rtl.source_style_id.has_value());
    CHECK(*after->run_rtl.value);
    CHECK_EQ(*after->run_rtl.source_style_id, "ChildStyle");

    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    const auto reopened_summary = reopened.find_style("ChildStyle");
    REQUIRE(reopened_summary.has_value());
    REQUIRE(reopened_summary->based_on.has_value());
    CHECK_EQ(*reopened_summary->based_on, "BaseB");
    const auto reopened_after = reopened.resolve_style_properties("ChildStyle");
    REQUIRE(reopened_after.has_value());
    REQUIRE(reopened_after->run_font_family.source_style_id.has_value());
    CHECK_EQ(*reopened_after->run_font_family.source_style_id, "ChildStyle");

    fs::remove(target);
}

TEST_CASE("rebase_character_style_based_on preserves resolved inherited values while switching parent") {
    namespace fs = std::filesystem;

    const fs::path target =
        fs::current_path() / "rebase_character_style_based_on_roundtrip.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto base_a = featherdoc::character_style_definition{};
    base_a.name = "Base A";
    base_a.based_on = std::string{"DefaultParagraphFont"};
    base_a.run_font_family = std::string{"Segoe UI"};
    base_a.run_east_asia_language = std::string{"zh-CN"};
    base_a.run_strikethrough = true;
    base_a.run_superscript = true;
    base_a.run_rtl = true;
    CHECK(doc.ensure_character_style("BaseA", base_a));

    auto base_b = featherdoc::character_style_definition{};
    base_b.name = "Base B";
    base_b.based_on = std::string{"DefaultParagraphFont"};
    base_b.run_font_family = std::string{"Arial"};
    base_b.run_east_asia_language = std::string{"ja-JP"};
    base_b.run_strikethrough = false;
    base_b.run_subscript = true;
    base_b.run_rtl = false;
    CHECK(doc.ensure_character_style("BaseB", base_b));

    auto child_definition = featherdoc::character_style_definition{};
    child_definition.name = "Child Style";
    child_definition.based_on = std::string{"BaseA"};
    child_definition.run_language = std::string{"en-US"};
    CHECK(doc.ensure_character_style("ChildStyle", child_definition));

    const auto before = doc.resolve_style_properties("ChildStyle");
    REQUIRE(before.has_value());
    REQUIRE(before->run_font_family.value.has_value());
    CHECK_EQ(*before->run_font_family.value, "Segoe UI");
    REQUIRE(before->run_font_family.source_style_id.has_value());
    CHECK_EQ(*before->run_font_family.source_style_id, "BaseA");
    REQUIRE(before->run_strikethrough.value.has_value());
    CHECK(*before->run_strikethrough.value);
    REQUIRE(before->run_strikethrough.source_style_id.has_value());
    CHECK_EQ(*before->run_strikethrough.source_style_id, "BaseA");
    REQUIRE(before->run_superscript.value.has_value());
    CHECK(*before->run_superscript.value);
    REQUIRE(before->run_superscript.source_style_id.has_value());
    CHECK_EQ(*before->run_superscript.source_style_id, "BaseA");

    CHECK(doc.rebase_character_style_based_on("ChildStyle", "BaseB"));

    const auto summary = doc.find_style("ChildStyle");
    REQUIRE(summary.has_value());
    REQUIRE(summary->based_on.has_value());
    CHECK_EQ(*summary->based_on, "BaseB");

    const auto after = doc.resolve_style_properties("ChildStyle");
    REQUIRE(after.has_value());
    REQUIRE(after->based_on.has_value());
    CHECK_EQ(*after->based_on, "BaseB");
    REQUIRE(after->run_font_family.value.has_value());
    REQUIRE(after->run_font_family.source_style_id.has_value());
    CHECK_EQ(*after->run_font_family.value, "Segoe UI");
    CHECK_EQ(*after->run_font_family.source_style_id, "ChildStyle");
    REQUIRE(after->run_east_asia_language.value.has_value());
    REQUIRE(after->run_east_asia_language.source_style_id.has_value());
    CHECK_EQ(*after->run_east_asia_language.value, "zh-CN");
    CHECK_EQ(*after->run_east_asia_language.source_style_id, "ChildStyle");
    REQUIRE(after->run_language.value.has_value());
    REQUIRE(after->run_language.source_style_id.has_value());
    CHECK_EQ(*after->run_language.value, "en-US");
    CHECK_EQ(*after->run_language.source_style_id, "ChildStyle");
    REQUIRE(after->run_strikethrough.value.has_value());
    REQUIRE(after->run_strikethrough.source_style_id.has_value());
    CHECK(*after->run_strikethrough.value);
    CHECK_EQ(*after->run_strikethrough.source_style_id, "ChildStyle");
    REQUIRE(after->run_superscript.value.has_value());
    REQUIRE(after->run_superscript.source_style_id.has_value());
    CHECK(*after->run_superscript.value);
    CHECK_EQ(*after->run_superscript.source_style_id, "ChildStyle");
    REQUIRE(after->run_subscript.value.has_value());
    CHECK_FALSE(*after->run_subscript.value);
    REQUIRE(after->run_rtl.value.has_value());
    REQUIRE(after->run_rtl.source_style_id.has_value());
    CHECK(*after->run_rtl.value);
    CHECK_EQ(*after->run_rtl.source_style_id, "ChildStyle");

    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    const auto reopened_summary = reopened.find_style("ChildStyle");
    REQUIRE(reopened_summary.has_value());
    REQUIRE(reopened_summary->based_on.has_value());
    CHECK_EQ(*reopened_summary->based_on, "BaseB");
    const auto reopened_after = reopened.resolve_style_properties("ChildStyle");
    REQUIRE(reopened_after.has_value());
    REQUIRE(reopened_after->run_font_family.source_style_id.has_value());
    CHECK_EQ(*reopened_after->run_font_family.source_style_id, "ChildStyle");
    REQUIRE(reopened_after->run_strikethrough.source_style_id.has_value());
    CHECK_EQ(*reopened_after->run_strikethrough.source_style_id, "ChildStyle");
    REQUIRE(reopened_after->run_superscript.source_style_id.has_value());
    CHECK_EQ(*reopened_after->run_superscript.source_style_id, "ChildStyle");

    fs::remove(target);
}

TEST_CASE("paragraph style property mutators update metadata without clearing direct run properties") {
    namespace fs = std::filesystem;

    const fs::path target =
        fs::current_path() / "paragraph_style_property_mutators_roundtrip.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto definition = featherdoc::paragraph_style_definition{};
    definition.name = "Working Style";
    definition.based_on = std::string{"Normal"};
    definition.next_style = std::string{"WorkingStyle"};
    definition.run_font_family = std::string{"Consolas"};
    definition.run_strikethrough = true;
    CHECK(doc.ensure_paragraph_style("WorkingStyle", definition));

    REQUIRE(doc.paragraph_style_next_style("WorkingStyle").has_value());
    CHECK_EQ(*doc.paragraph_style_next_style("WorkingStyle"), "WorkingStyle");
    CHECK_FALSE(doc.paragraph_style_outline_level("WorkingStyle").has_value());
    REQUIRE(doc.style_run_font_family("WorkingStyle").has_value());
    CHECK_EQ(*doc.style_run_font_family("WorkingStyle"), "Consolas");
    REQUIRE(doc.style_run_strikethrough("WorkingStyle").has_value());
    CHECK(*doc.style_run_strikethrough("WorkingStyle"));

    CHECK(doc.set_paragraph_style_based_on("WorkingStyle", "Heading1"));
    CHECK(doc.set_paragraph_style_next_style("WorkingStyle", "BodyText"));
    CHECK(doc.set_paragraph_style_outline_level("WorkingStyle", 2U));

    const auto summary = doc.find_style("WorkingStyle");
    REQUIRE(summary.has_value());
    REQUIRE(summary->based_on.has_value());
    CHECK_EQ(*summary->based_on, "Heading1");
    REQUIRE(doc.paragraph_style_next_style("WorkingStyle").has_value());
    CHECK_EQ(*doc.paragraph_style_next_style("WorkingStyle"), "BodyText");
    REQUIRE(doc.paragraph_style_outline_level("WorkingStyle").has_value());
    CHECK_EQ(*doc.paragraph_style_outline_level("WorkingStyle"), 2U);
    REQUIRE(doc.style_run_font_family("WorkingStyle").has_value());
    CHECK_EQ(*doc.style_run_font_family("WorkingStyle"), "Consolas");
    REQUIRE(doc.style_run_strikethrough("WorkingStyle").has_value());
    CHECK(*doc.style_run_strikethrough("WorkingStyle"));

    CHECK(doc.clear_paragraph_style_based_on("WorkingStyle"));
    CHECK(doc.clear_paragraph_style_next_style("WorkingStyle"));
    CHECK(doc.clear_paragraph_style_outline_level("WorkingStyle"));

    const auto cleared_summary = doc.find_style("WorkingStyle");
    REQUIRE(cleared_summary.has_value());
    CHECK_FALSE(cleared_summary->based_on.has_value());
    CHECK_FALSE(doc.paragraph_style_next_style("WorkingStyle").has_value());
    CHECK_FALSE(doc.paragraph_style_outline_level("WorkingStyle").has_value());
    REQUIRE(doc.style_run_font_family("WorkingStyle").has_value());
    CHECK_EQ(*doc.style_run_font_family("WorkingStyle"), "Consolas");

    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    const auto reopened_summary = reopened.find_style("WorkingStyle");
    REQUIRE(reopened_summary.has_value());
    CHECK_FALSE(reopened_summary->based_on.has_value());
    CHECK_FALSE(reopened.paragraph_style_next_style("WorkingStyle").has_value());
    CHECK_FALSE(reopened.paragraph_style_outline_level("WorkingStyle").has_value());
    REQUIRE(reopened.style_run_font_family("WorkingStyle").has_value());
    CHECK_EQ(*reopened.style_run_font_family("WorkingStyle"), "Consolas");
    REQUIRE(reopened.style_run_strikethrough("WorkingStyle").has_value());
    CHECK(*reopened.style_run_strikethrough("WorkingStyle"));

    fs::remove(target);
}
