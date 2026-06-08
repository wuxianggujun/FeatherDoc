#include <algorithm>
#include <filesystem>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "basic_docx_archive_test_support.hpp"
#include "basic_document_xml_test_support.hpp"

#include <featherdoc.hpp>
TEST_CASE("ensure_style_linked_numbering links multiple paragraph styles to one shared instance") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "style_linked_numbering_roundtrip.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto heading_one_style = featherdoc::paragraph_style_definition{};
    heading_one_style.name = "Legal Heading 1";
    heading_one_style.based_on = std::string{"Heading1"};
    heading_one_style.paragraph_bidi = false;
    CHECK(doc.ensure_paragraph_style("LegalHeading1", heading_one_style));

    auto heading_two_style = featherdoc::paragraph_style_definition{};
    heading_two_style.name = "Legal Heading 2";
    heading_two_style.based_on = std::string{"Heading2"};
    heading_two_style.paragraph_bidi = false;
    CHECK(doc.ensure_paragraph_style("LegalHeading2", heading_two_style));

    auto numbering_definition = featherdoc::numbering_definition{};
    numbering_definition.name = "LegalOutlineShared";
    numbering_definition.levels = {
        featherdoc::numbering_level_definition{
            featherdoc::list_kind::decimal, 1U, 0U, "%1."},
        featherdoc::numbering_level_definition{
            featherdoc::list_kind::decimal, 1U, 1U, "%1.%2."},
    };

    const auto numbering_id = doc.ensure_style_linked_numbering(
        numbering_definition,
        {featherdoc::paragraph_style_numbering_link{"LegalHeading1", 0U},
         featherdoc::paragraph_style_numbering_link{"LegalHeading2", 1U}});
    REQUIRE(numbering_id.has_value());

    const auto heading_one = doc.find_style("LegalHeading1");
    const auto heading_two = doc.find_style("LegalHeading2");
    REQUIRE(heading_one.has_value());
    REQUIRE(heading_two.has_value());
    REQUIRE(heading_one->numbering.has_value());
    REQUIRE(heading_two->numbering.has_value());
    REQUIRE(heading_one->numbering->num_id.has_value());
    REQUIRE(heading_two->numbering->num_id.has_value());
    CHECK_EQ(*heading_one->numbering->num_id, *heading_two->numbering->num_id);
    REQUIRE(heading_one->numbering->definition_id.has_value());
    REQUIRE(heading_two->numbering->definition_id.has_value());
    CHECK_EQ(*heading_one->numbering->definition_id, *numbering_id);
    CHECK_EQ(*heading_two->numbering->definition_id, *numbering_id);
    REQUIRE(heading_one->numbering->level.has_value());
    REQUIRE(heading_two->numbering->level.has_value());
    CHECK_EQ(*heading_one->numbering->level, 0U);
    CHECK_EQ(*heading_two->numbering->level, 1U);

    CHECK_FALSE(doc.save());

    const auto saved_styles_xml = read_test_docx_entry(target, "word/styles.xml");
    pugi::xml_document styles_document;
    REQUIRE(styles_document.load_string(saved_styles_xml.c_str()));
    const auto styles_root = styles_document.child("w:styles");
    REQUIRE(styles_root != pugi::xml_node{});

    const auto heading_one_xml = find_style_xml_node(styles_root, "LegalHeading1");
    const auto heading_two_xml = find_style_xml_node(styles_root, "LegalHeading2");
    REQUIRE(heading_one_xml != pugi::xml_node{});
    REQUIRE(heading_two_xml != pugi::xml_node{});

    const auto heading_one_num_pr = heading_one_xml.child("w:pPr").child("w:numPr");
    const auto heading_two_num_pr = heading_two_xml.child("w:pPr").child("w:numPr");
    REQUIRE(heading_one_num_pr != pugi::xml_node{});
    REQUIRE(heading_two_num_pr != pugi::xml_node{});
    CHECK_EQ(std::string_view{heading_one_num_pr.child("w:ilvl").attribute("w:val").value()},
             "0");
    CHECK_EQ(std::string_view{heading_two_num_pr.child("w:ilvl").attribute("w:val").value()},
             "1");
    CHECK_EQ(std::string{heading_one_num_pr.child("w:numId").attribute("w:val").value()},
             std::string{heading_two_num_pr.child("w:numId").attribute("w:val").value()});

    const auto numbering_xml = read_test_docx_entry(target, "word/numbering.xml");
    CHECK_EQ(count_substring_occurrences(numbering_xml, "<w:num "), 1U);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    const auto reopened_heading_one = reopened.find_style("LegalHeading1");
    const auto reopened_heading_two = reopened.find_style("LegalHeading2");
    REQUIRE(reopened_heading_one.has_value());
    REQUIRE(reopened_heading_two.has_value());
    REQUIRE(reopened_heading_one->numbering.has_value());
    REQUIRE(reopened_heading_two->numbering.has_value());
    REQUIRE(reopened_heading_one->numbering->num_id.has_value());
    REQUIRE(reopened_heading_two->numbering->num_id.has_value());
    CHECK_EQ(*reopened_heading_one->numbering->num_id,
             *reopened_heading_two->numbering->num_id);

    fs::remove(target);
}

TEST_CASE("ensure_style_linked_numbering validates style links before mutating styles") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "style_linked_numbering_validation.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto paragraph_style = featherdoc::paragraph_style_definition{};
    paragraph_style.name = "Legal Heading";
    paragraph_style.based_on = std::string{"Heading1"};
    CHECK(doc.ensure_paragraph_style("LegalHeading", paragraph_style));

    auto numbering_definition = featherdoc::numbering_definition{};
    numbering_definition.name = "LegalOutlineInvalid";
    numbering_definition.levels = {
        featherdoc::numbering_level_definition{
            featherdoc::list_kind::decimal, 1U, 0U, "%1."},
    };

    CHECK_FALSE(doc.ensure_style_linked_numbering(numbering_definition, {}).has_value());
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));

    CHECK_FALSE(doc.ensure_style_linked_numbering(
                    numbering_definition,
                    {featherdoc::paragraph_style_numbering_link{"LegalHeading", 0U},
                     featherdoc::paragraph_style_numbering_link{"LegalHeading", 0U}})
                    .has_value());
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));

    CHECK_FALSE(doc.ensure_style_linked_numbering(
                    numbering_definition,
                    {featherdoc::paragraph_style_numbering_link{"MissingStyle", 0U}})
                    .has_value());
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));

    CHECK_FALSE(doc.ensure_style_linked_numbering(
                    numbering_definition,
                    {featherdoc::paragraph_style_numbering_link{"LegalHeading", 1U}})
                    .has_value());
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));

    fs::remove(target);
}

TEST_CASE("set_paragraph_style and set_run_style create styles parts and round-trip") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "paragraph_run_style_roundtrip.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto paragraph = doc.paragraphs();
    REQUIRE(paragraph.has_next());
    CHECK(doc.set_paragraph_style(paragraph, "Heading1"));

    auto run = paragraph.add_run("Styled text");
    REQUIRE(run.has_next());
    CHECK(doc.set_run_style(run, "Strong"));

    CHECK_FALSE(doc.save());
    CHECK(test_docx_entry_exists(target, "word/styles.xml"));

    const auto saved_relationships =
        read_test_docx_entry(target, "word/_rels/document.xml.rels");
    CHECK_NE(saved_relationships.find(
                 "Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/styles\""),
             std::string::npos);
    CHECK_NE(saved_relationships.find("Target=\"styles.xml\""), std::string::npos);

    const auto saved_content_types = read_test_docx_entry(target, test_content_types_xml_entry);
    CHECK_NE(saved_content_types.find("PartName=\"/word/styles.xml\""), std::string::npos);
    CHECK_NE(saved_content_types.find(
                 "application/vnd.openxmlformats-officedocument.wordprocessingml.styles+xml"),
             std::string::npos);

    const auto saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    CHECK_NE(saved_document_xml.find("<w:pStyle w:val=\"Heading1\""), std::string::npos);
    CHECK_NE(saved_document_xml.find("<w:rStyle w:val=\"Strong\""), std::string::npos);

    const auto saved_styles_xml = read_test_docx_entry(target, "word/styles.xml");
    CHECK_NE(saved_styles_xml.find("w:styleId=\"Heading1\""), std::string::npos);
    CHECK_NE(saved_styles_xml.find("w:styleId=\"Strong\""), std::string::npos);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK(reopened.paragraphs().add_run(" tail").has_next());
    CHECK_FALSE(reopened.save());
    CHECK(test_docx_entry_exists(target, "word/styles.xml"));

    fs::remove(target);
}

TEST_CASE("list_styles exposes the generated default styles catalog") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "style_catalog_defaults.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    const auto styles = doc.list_styles();
    CHECK_EQ(styles.size(), 9U);

    const auto *normal = find_style_summary(styles, "Normal");
    REQUIRE(normal != nullptr);
    CHECK_EQ(normal->kind, featherdoc::style_kind::paragraph);
    CHECK_EQ(normal->type_name, "paragraph");
    CHECK_EQ(normal->name, "Normal");
    CHECK_FALSE(normal->numbering.has_value());
    CHECK(normal->is_default);
    CHECK_FALSE(normal->based_on.has_value());
    CHECK(normal->is_quick_format);

    const auto *default_paragraph_font = find_style_summary(styles, "DefaultParagraphFont");
    REQUIRE(default_paragraph_font != nullptr);
    CHECK_EQ(default_paragraph_font->kind, featherdoc::style_kind::character);
    CHECK(default_paragraph_font->is_default);
    CHECK(default_paragraph_font->is_semi_hidden);
    CHECK(default_paragraph_font->is_unhide_when_used);

    const auto *table_grid = find_style_summary(styles, "TableGrid");
    REQUIRE(table_grid != nullptr);
    CHECK_EQ(table_grid->kind, featherdoc::style_kind::table);
    REQUIRE(table_grid->based_on.has_value());
    CHECK_EQ(*table_grid->based_on, "TableNormal");

    const auto strong = doc.find_style("Strong");
    REQUIRE(strong.has_value());
    CHECK_EQ(strong->kind, featherdoc::style_kind::character);
    CHECK_EQ(strong->type_name, "character");
    CHECK_EQ(strong->name, "Strong");
    REQUIRE(strong->based_on.has_value());
    CHECK_EQ(*strong->based_on, "DefaultParagraphFont");
    CHECK(strong->is_quick_format);

    CHECK_FALSE(doc.find_style("MissingStyle").has_value());
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));

    CHECK_FALSE(doc.save());
    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    const auto reopened_styles = reopened.list_styles();
    CHECK_EQ(reopened_styles.size(), styles.size());

    fs::remove(target);
}

TEST_CASE("list_styles reads existing styles metadata and preserves unknown kinds") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "style_catalog_existing.docx";
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

    const auto styles = doc.list_styles();
    CHECK_EQ(styles.size(), 4U);

    const auto *custom_body = find_style_summary(styles, "CustomBody");
    REQUIRE(custom_body != nullptr);
    CHECK_EQ(custom_body->kind, featherdoc::style_kind::paragraph);
    CHECK_EQ(custom_body->type_name, "paragraph");
    CHECK_EQ(custom_body->name, "Custom Body");
    CHECK_FALSE(custom_body->numbering.has_value());
    REQUIRE(custom_body->based_on.has_value());
    CHECK_EQ(*custom_body->based_on, "Normal");
    CHECK(custom_body->is_custom);
    CHECK(custom_body->is_quick_format);

    const auto numbered = doc.find_style("NumberedStyle");
    REQUIRE(numbered.has_value());
    CHECK_EQ(numbered->kind, featherdoc::style_kind::numbering);
    CHECK_EQ(numbered->type_name, "numbering");
    CHECK(numbered->is_semi_hidden);
    CHECK(numbered->is_unhide_when_used);

    const auto *mystery = find_style_summary(styles, "MysteryStyle");
    REQUIRE(mystery != nullptr);
    CHECK_EQ(mystery->kind, featherdoc::style_kind::unknown);
    CHECK_EQ(mystery->type_name, "mystery");
    CHECK_EQ(mystery->name, "Mystery");

    CHECK_FALSE(doc.save());
    const auto saved_styles_xml = read_test_docx_entry(target, "word/styles.xml");
    CHECK_NE(saved_styles_xml.find("w:styleId=\"CustomBody\""), std::string::npos);
    CHECK_NE(saved_styles_xml.find("w:type=\"mystery\""), std::string::npos);
    CHECK_EQ(count_substring_occurrences(saved_styles_xml, "w:styleId=\"TableGrid\""), 0);

    fs::remove(target);
}

TEST_CASE("find_style_usage scans paragraph run and table references from the main document") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "style_usage_existing.docx";
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
    <w:p>
      <w:pPr>
        <w:pStyle w:val="CustomBody"/>
        <w:sectPr/>
      </w:pPr>
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
    <w:sectPr/>
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

    const auto paragraph_usage = doc.find_style_usage("CustomBody");
    REQUIRE(paragraph_usage.has_value());
    CHECK_EQ(paragraph_usage->style_id, "CustomBody");
    CHECK_EQ(paragraph_usage->paragraph_count, 2U);
    CHECK_EQ(paragraph_usage->run_count, 0U);
    CHECK_EQ(paragraph_usage->table_count, 0U);
    CHECK_EQ(paragraph_usage->total_count(), 2U);
    CHECK_EQ(paragraph_usage->body.paragraph_count, 2U);
    CHECK_EQ(paragraph_usage->body.total_count(), 2U);
    CHECK_EQ(paragraph_usage->header.total_count(), 0U);
    CHECK_EQ(paragraph_usage->footer.total_count(), 0U);
    REQUIRE_EQ(paragraph_usage->hits.size(), 2U);
    CHECK_EQ(paragraph_usage->hits[0].part, featherdoc::style_usage_part_kind::body);
    CHECK_EQ(paragraph_usage->hits[0].kind, featherdoc::style_usage_hit_kind::paragraph);
    CHECK_EQ(paragraph_usage->hits[0].entry_name, "word/document.xml");
    CHECK_EQ(paragraph_usage->hits[0].ordinal, 1U);
    REQUIRE(paragraph_usage->hits[0].section_index.has_value());
    CHECK_EQ(*paragraph_usage->hits[0].section_index, 0U);
    CHECK(paragraph_usage->hits[0].references.empty());
    CHECK_EQ(paragraph_usage->hits[1].part, featherdoc::style_usage_part_kind::body);
    CHECK_EQ(paragraph_usage->hits[1].kind, featherdoc::style_usage_hit_kind::paragraph);
    CHECK_EQ(paragraph_usage->hits[1].entry_name, "word/document.xml");
    CHECK_EQ(paragraph_usage->hits[1].ordinal, 2U);
    REQUIRE(paragraph_usage->hits[1].section_index.has_value());
    CHECK_EQ(*paragraph_usage->hits[1].section_index, 1U);
    CHECK(paragraph_usage->hits[1].references.empty());

    const auto run_usage = doc.find_style_usage("Strong");
    REQUIRE(run_usage.has_value());
    CHECK_EQ(run_usage->style_id, "Strong");
    CHECK_EQ(run_usage->paragraph_count, 0U);
    CHECK_EQ(run_usage->run_count, 2U);
    CHECK_EQ(run_usage->table_count, 0U);
    CHECK_EQ(run_usage->total_count(), 2U);
    CHECK_EQ(run_usage->body.run_count, 2U);
    CHECK_EQ(run_usage->header.total_count(), 0U);
    CHECK_EQ(run_usage->footer.total_count(), 0U);
    REQUIRE_EQ(run_usage->hits.size(), 2U);
    CHECK_EQ(run_usage->hits[0].part, featherdoc::style_usage_part_kind::body);
    CHECK_EQ(run_usage->hits[0].kind, featherdoc::style_usage_hit_kind::run);
    CHECK_EQ(run_usage->hits[0].entry_name, "word/document.xml");
    CHECK_EQ(run_usage->hits[0].ordinal, 1U);
    REQUIRE(run_usage->hits[0].section_index.has_value());
    CHECK_EQ(*run_usage->hits[0].section_index, 0U);
    CHECK(run_usage->hits[0].references.empty());
    CHECK_EQ(run_usage->hits[1].part, featherdoc::style_usage_part_kind::body);
    CHECK_EQ(run_usage->hits[1].kind, featherdoc::style_usage_hit_kind::run);
    CHECK_EQ(run_usage->hits[1].entry_name, "word/document.xml");
    CHECK_EQ(run_usage->hits[1].ordinal, 2U);
    REQUIRE(run_usage->hits[1].section_index.has_value());
    CHECK_EQ(*run_usage->hits[1].section_index, 1U);
    CHECK(run_usage->hits[1].references.empty());

    const auto table_usage = doc.find_style_usage("ReportTable");
    REQUIRE(table_usage.has_value());
    CHECK_EQ(table_usage->style_id, "ReportTable");
    CHECK_EQ(table_usage->paragraph_count, 0U);
    CHECK_EQ(table_usage->run_count, 0U);
    CHECK_EQ(table_usage->table_count, 1U);
    CHECK_EQ(table_usage->total_count(), 1U);
    CHECK_EQ(table_usage->body.table_count, 1U);
    CHECK_EQ(table_usage->header.total_count(), 0U);
    CHECK_EQ(table_usage->footer.total_count(), 0U);
    REQUIRE_EQ(table_usage->hits.size(), 1U);
    CHECK_EQ(table_usage->hits[0].part, featherdoc::style_usage_part_kind::body);
    CHECK_EQ(table_usage->hits[0].kind, featherdoc::style_usage_hit_kind::table);
    CHECK_EQ(table_usage->hits[0].entry_name, "word/document.xml");
    CHECK_EQ(table_usage->hits[0].ordinal, 1U);
    REQUIRE(table_usage->hits[0].section_index.has_value());
    CHECK_EQ(*table_usage->hits[0].section_index, 1U);
    CHECK(table_usage->hits[0].references.empty());

    const auto unused_usage = doc.find_style_usage("Normal");
    REQUIRE(unused_usage.has_value());
    CHECK_EQ(unused_usage->style_id, "Normal");
    CHECK_EQ(unused_usage->total_count(), 0U);
    CHECK(unused_usage->hits.empty());

    const auto report = doc.list_style_usage();
    REQUIRE(report.has_value());
    CHECK_EQ(report->style_count, 4U);
    CHECK_EQ(report->used_style_count, 3U);
    CHECK_EQ(report->unused_style_count, 1U);
    CHECK_EQ(report->total_reference_count, 5U);
    REQUIRE_EQ(report->entries.size(), 4U);

    const auto find_usage_entry = [&report](std::string_view style_id)
        -> const featherdoc::style_usage_report_entry * {
        const auto iterator = std::find_if(
            report->entries.begin(), report->entries.end(),
            [style_id](const featherdoc::style_usage_report_entry &entry) {
                return entry.style.style_id == style_id;
            });
        return iterator == report->entries.end() ? nullptr : &*iterator;
    };

    const auto *custom_body_entry = find_usage_entry("CustomBody");
    REQUIRE(custom_body_entry != nullptr);
    CHECK_EQ(custom_body_entry->style.name, "Custom Body");
    CHECK_EQ(custom_body_entry->usage.total_count(), 2U);

    const auto *normal_entry = find_usage_entry("Normal");
    REQUIRE(normal_entry != nullptr);
    CHECK_EQ(normal_entry->usage.total_count(), 0U);
    CHECK(normal_entry->usage.hits.empty());

    CHECK_FALSE(doc.find_style_usage("MissingStyle").has_value());
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));

    fs::remove(target);
}

TEST_CASE("merge_style rewrites references removes source style and round-trips") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "style_merge_round_trip.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    featherdoc::paragraph_style_definition source_style;
    source_style.name = "Source Style";
    source_style.based_on = "Normal";
    source_style.next_style = "SourceStyle";
    source_style.is_quick_format = true;
    CHECK(doc.ensure_paragraph_style("SourceStyle", source_style));

    featherdoc::paragraph_style_definition target_style;
    target_style.name = "Target Style";
    target_style.based_on = "Normal";
    CHECK(doc.ensure_paragraph_style("TargetStyle", target_style));

    auto paragraph = doc.paragraphs();
    REQUIRE(paragraph.has_next());
    REQUIRE(paragraph.add_run("merged style paragraph").has_next());
    CHECK(doc.set_paragraph_style(paragraph, "SourceStyle"));

    CHECK(doc.merge_style("SourceStyle", "TargetStyle"));
    CHECK_FALSE(doc.find_style("SourceStyle").has_value());
    const auto merged_style = doc.find_style("TargetStyle");
    REQUIRE(merged_style.has_value());
    CHECK_EQ(merged_style->style_id, "TargetStyle");
    CHECK_EQ(merged_style->name, "Target Style");

    const auto usage = doc.find_style_usage("TargetStyle");
    REQUIRE(usage.has_value());
    CHECK_EQ(usage->paragraph_count, 1U);
    CHECK_EQ(usage->total_count(), 1U);

    CHECK_FALSE(doc.merge_style("TargetStyle", "TargetStyle"));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));

    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_FALSE(reopened.find_style("SourceStyle").has_value());
    const auto reopened_usage = reopened.find_style_usage("TargetStyle");
    REQUIRE(reopened_usage.has_value());
    CHECK_EQ(reopened_usage->paragraph_count, 1U);
    CHECK_EQ(reopened_usage->total_count(), 1U);

    const auto styles_xml = read_test_docx_entry(target, "word/styles.xml");
    CHECK_EQ(styles_xml.find(R"(w:styleId="SourceStyle")"), std::string::npos);
    CHECK_NE(styles_xml.find(R"(w:styleId="TargetStyle")"), std::string::npos);
    const auto document_xml = read_test_docx_entry(target, "word/document.xml");
    CHECK_NE(document_xml.find(R"(w:pStyle w:val="TargetStyle")"), std::string::npos);
    CHECK_EQ(document_xml.find(R"(w:pStyle w:val="SourceStyle")"), std::string::npos);

    fs::remove(target);
}

TEST_CASE("plan_style_refactor validates rename and merge operations without mutating") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "style_refactor_plan.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    featherdoc::paragraph_style_definition source_style;
    source_style.name = "Source Style";
    source_style.based_on = "Normal";
    CHECK(doc.ensure_paragraph_style("SourceStyle", source_style));

    featherdoc::paragraph_style_definition target_style;
    target_style.name = "Target Style";
    target_style.based_on = "Normal";
    CHECK(doc.ensure_paragraph_style("TargetStyle", target_style));

    featherdoc::character_style_definition character_target;
    character_target.name = "Character Target";
    CHECK(doc.ensure_character_style("CharacterTarget", character_target));

    auto paragraph = doc.paragraphs();
    REQUIRE(paragraph.has_next());
    REQUIRE(paragraph.add_run("refactor plan paragraph").has_next());
    CHECK(doc.set_paragraph_style(paragraph, "SourceStyle"));

    const auto plan = doc.plan_style_refactor(
        {
            {featherdoc::style_refactor_action::rename, "SourceStyle", "RenamedStyle"},
            {featherdoc::style_refactor_action::merge, "SourceStyle", "TargetStyle"},
            {featherdoc::style_refactor_action::merge, "SourceStyle", "CharacterTarget"},
            {featherdoc::style_refactor_action::rename, "MissingStyle", "NewStyle"},
            {featherdoc::style_refactor_action::rename, "SourceStyle", "TargetStyle"},
        });
    REQUIRE(plan.has_value());
    CHECK_FALSE(plan->clean());
    CHECK_EQ(plan->operation_count, 5U);
    CHECK_EQ(plan->applyable_count, 2U);
    CHECK_EQ(plan->issue_count, 3U);
    REQUIRE_EQ(plan->operations.size(), 5U);

    CHECK_EQ(plan->operations[0].action, featherdoc::style_refactor_action::rename);
    CHECK(plan->operations[0].applyable);
    REQUIRE(plan->operations[0].source_usage.has_value());
    CHECK_EQ(plan->operations[0].source_usage->total_count(), 1U);
    CHECK_FALSE(plan->operations[0].target_style.has_value());

    CHECK_EQ(plan->operations[1].action, featherdoc::style_refactor_action::merge);
    CHECK(plan->operations[1].applyable);
    REQUIRE(plan->operations[1].target_style.has_value());
    CHECK_EQ(plan->operations[1].target_style->kind, featherdoc::style_kind::paragraph);

    REQUIRE_EQ(plan->operations[2].issues.size(), 1U);
    CHECK_EQ(plan->operations[2].issues[0].code, "style_type_mismatch");
    CHECK_FALSE(plan->operations[2].applyable);

    REQUIRE_EQ(plan->operations[3].issues.size(), 1U);
    CHECK_EQ(plan->operations[3].issues[0].code, "missing_source_style");
    CHECK_FALSE(plan->operations[3].applyable);

    REQUIRE_EQ(plan->operations[4].issues.size(), 1U);
    CHECK_EQ(plan->operations[4].issues[0].code, "target_style_exists");
    CHECK_FALSE(plan->operations[4].applyable);

    CHECK(doc.find_style("SourceStyle").has_value());
    CHECK(doc.find_style("TargetStyle").has_value());
    CHECK_FALSE(doc.find_style("RenamedStyle").has_value());

    fs::remove(target);
}

TEST_CASE("suggest_style_merges recommends duplicate custom styles by usage") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "style_merge_suggestions.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    featherdoc::paragraph_style_definition duplicate_a;
    duplicate_a.name = "Duplicate Body A";
    duplicate_a.based_on = "Normal";
    duplicate_a.run_font_family = "Aptos";
    CHECK(doc.ensure_paragraph_style("DuplicateBodyA", duplicate_a));

    featherdoc::paragraph_style_definition duplicate_b;
    duplicate_b.name = "Duplicate Body B";
    duplicate_b.based_on = "Normal";
    duplicate_b.run_font_family = "Aptos";
    CHECK(doc.ensure_paragraph_style("DuplicateBodyB", duplicate_b));

    featherdoc::paragraph_style_definition duplicate_c;
    duplicate_c.name = "Duplicate Body C";
    duplicate_c.based_on = "Normal";
    duplicate_c.next_style = "Normal";
    duplicate_c.run_font_family = "Aptos";
    CHECK(doc.ensure_paragraph_style("DuplicateBodyC", duplicate_c));

    auto first_paragraph = doc.paragraphs();
    REQUIRE(first_paragraph.has_next());
    REQUIRE(first_paragraph.add_run("first duplicate style reference").has_next());
    CHECK(doc.set_paragraph_style(first_paragraph, "DuplicateBodyA"));
    auto second_paragraph = first_paragraph.insert_paragraph_after("second");
    CHECK(doc.set_paragraph_style(second_paragraph, "DuplicateBodyA"));
    auto third_paragraph = second_paragraph.insert_paragraph_after("third");
    CHECK(doc.set_paragraph_style(third_paragraph, "DuplicateBodyB"));
    auto fourth_paragraph = third_paragraph.insert_paragraph_after("fourth");
    CHECK(doc.set_paragraph_style(fourth_paragraph, "DuplicateBodyC"));

    const auto plan = doc.suggest_style_merges();
    REQUIRE(plan.has_value());
    CHECK(plan->clean());
    CHECK_EQ(plan->operation_count, 2U);
    CHECK_EQ(plan->applyable_count, 2U);
    CHECK_EQ(plan->issue_count, 0U);
    REQUIRE_EQ(plan->operations.size(), 2U);

    const auto confidence_summary = plan->suggestion_confidence_summary();
    CHECK_EQ(confidence_summary.suggestion_count, 2U);
    CHECK_EQ(confidence_summary.exact_xml_match_count, 1U);
    CHECK_EQ(confidence_summary.xml_difference_count, 1U);
    REQUIRE(confidence_summary.min_confidence.has_value());
    CHECK_EQ(*confidence_summary.min_confidence, 80U);
    REQUIRE(confidence_summary.max_confidence.has_value());
    CHECK_EQ(*confidence_summary.max_confidence, 95U);
    REQUIRE(confidence_summary.recommended_min_confidence.has_value());
    CHECK_EQ(*confidence_summary.recommended_min_confidence, 95U);
    CHECK_NE(confidence_summary.recommendation.find("review lower-confidence"),
             std::string::npos);

    const auto &exact_operation = plan->operations[0];
    CHECK_EQ(exact_operation.action, featherdoc::style_refactor_action::merge);
    CHECK_EQ(exact_operation.source_style_id, "DuplicateBodyB");
    CHECK_EQ(exact_operation.target_style_id, "DuplicateBodyA");
    REQUIRE(exact_operation.source_usage.has_value());
    CHECK_EQ(exact_operation.source_usage->paragraph_count, 1U);
    REQUIRE(exact_operation.suggestion.has_value());
    CHECK_EQ(exact_operation.suggestion->reason_code,
             "matching_style_signature_and_xml");
    CHECK_EQ(exact_operation.suggestion->confidence, 95U);
    CHECK(exact_operation.suggestion->differences.empty());
    CHECK_NE(std::find(exact_operation.suggestion->evidence.begin(),
                       exact_operation.suggestion->evidence.end(),
                       "style_definition_xml_matches"),
             exact_operation.suggestion->evidence.end());
    REQUIRE(exact_operation.target_style.has_value());
    CHECK_EQ(exact_operation.target_style->kind, featherdoc::style_kind::paragraph);

    const auto &diff_operation = plan->operations[1];
    CHECK_EQ(diff_operation.action, featherdoc::style_refactor_action::merge);
    CHECK_EQ(diff_operation.source_style_id, "DuplicateBodyC");
    CHECK_EQ(diff_operation.target_style_id, "DuplicateBodyA");
    REQUIRE(diff_operation.suggestion.has_value());
    CHECK_EQ(diff_operation.suggestion->reason_code,
             "matching_resolved_style_signature");
    CHECK_EQ(diff_operation.suggestion->confidence, 80U);
    CHECK_NE(std::find(diff_operation.suggestion->evidence.begin(),
                       diff_operation.suggestion->evidence.end(),
                       "style_definition_xml_differs"),
             diff_operation.suggestion->evidence.end());
    CHECK_NE(std::find(diff_operation.suggestion->differences.begin(),
                       diff_operation.suggestion->differences.end(), "w:next"),
             diff_operation.suggestion->differences.end());

    CHECK(doc.find_style("DuplicateBodyA").has_value());
    CHECK(doc.find_style("DuplicateBodyB").has_value());
    CHECK(doc.find_style("DuplicateBodyC").has_value());

    fs::remove(target);
}

TEST_CASE("apply_style_refactor applies clean batches and rejects conflicts") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "style_refactor_apply.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    featherdoc::paragraph_style_definition source_a;
    source_a.name = "Source A";
    source_a.based_on = "Normal";
    CHECK(doc.ensure_paragraph_style("SourceA", source_a));

    featherdoc::paragraph_style_definition source_b;
    source_b.name = "Source B";
    source_b.based_on = "Normal";
    CHECK(doc.ensure_paragraph_style("SourceB", source_b));

    featherdoc::paragraph_style_definition target_style;
    target_style.name = "Target Style";
    target_style.based_on = "Normal";
    CHECK(doc.ensure_paragraph_style("TargetStyle", target_style));

    auto first_paragraph = doc.paragraphs();
    REQUIRE(first_paragraph.has_next());
    REQUIRE(first_paragraph.add_run("first").has_next());
    CHECK(doc.set_paragraph_style(first_paragraph, "SourceA"));
    auto second_paragraph = first_paragraph.insert_paragraph_after("second");
    CHECK(doc.set_paragraph_style(second_paragraph, "SourceB"));
    auto target_paragraph = second_paragraph.insert_paragraph_after("target");
    CHECK(doc.set_paragraph_style(target_paragraph, "TargetStyle"));

    const auto result = doc.apply_style_refactor(
        {
            {featherdoc::style_refactor_action::rename, "SourceA", "RenamedA"},
            {featherdoc::style_refactor_action::merge, "SourceB", "TargetStyle"},
        });
    REQUIRE(result.has_value());
    CHECK(result->applied());
    CHECK(result->changed);
    CHECK_EQ(result->requested_count, 2U);
    CHECK_EQ(result->applied_count, 2U);
    CHECK_EQ(result->skipped_count(), 0U);
    REQUIRE_EQ(result->rollback_entries.size(), 2U);
    CHECK(result->rollback_entries[0].automatic);
    CHECK_EQ(result->rollback_entries[0].action,
             featherdoc::style_refactor_action::rename);
    CHECK_EQ(result->rollback_entries[0].source_style_id, "RenamedA");
    CHECK_EQ(result->rollback_entries[0].target_style_id, "SourceA");
    CHECK_FALSE(result->rollback_entries[1].automatic);
    CHECK(result->rollback_entries[1].restorable);
    CHECK_EQ(result->rollback_entries[1].action,
             featherdoc::style_refactor_action::merge);
    CHECK_EQ(result->rollback_entries[1].source_style_id, "SourceB");
    CHECK_EQ(result->rollback_entries[1].target_style_id, "TargetStyle");
    CHECK_NE(result->rollback_entries[1].source_style_xml.find(
                 R"(w:styleId="SourceB")"),
             std::string::npos);
    REQUIRE(result->rollback_entries[1].source_usage.has_value());
    CHECK_EQ(result->rollback_entries[1].source_usage->paragraph_count, 1U);
    REQUIRE_EQ(result->rollback_entries[1].source_usage->hits.size(), 1U);
    CHECK_EQ(result->rollback_entries[1].source_usage->hits[0].node_ordinal, 2U);

    CHECK_FALSE(doc.find_style("SourceA").has_value());
    CHECK_FALSE(doc.find_style("SourceB").has_value());
    CHECK(doc.find_style("RenamedA").has_value());
    CHECK(doc.find_style("TargetStyle").has_value());

    const auto renamed_usage = doc.find_style_usage("RenamedA");
    REQUIRE(renamed_usage.has_value());
    CHECK_EQ(renamed_usage->paragraph_count, 1U);
    const auto target_usage = doc.find_style_usage("TargetStyle");
    REQUIRE(target_usage.has_value());
    CHECK_EQ(target_usage->paragraph_count, 2U);

    const auto restore_plan =
        doc.plan_style_refactor_restore({result->rollback_entries[1]});
    REQUIRE(restore_plan.has_value());
    CHECK(restore_plan->restored());
    CHECK_FALSE(restore_plan->changed);
    CHECK(restore_plan->dry_run);
    CHECK_EQ(restore_plan->requested_count, 1U);
    CHECK_EQ(restore_plan->restored_count, 1U);
    CHECK_EQ(restore_plan->issue_count(), 0U);
    CHECK(restore_plan->issue_summary().empty());
    CHECK_EQ(restore_plan->restored_style_count, 1U);
    CHECK_EQ(restore_plan->restored_reference_count, 1U);
    REQUIRE_EQ(restore_plan->operations.size(), 1U);
    CHECK(restore_plan->operations[0].restored);
    CHECK(restore_plan->operations[0].style_restored);
    CHECK_EQ(restore_plan->operations[0].restored_reference_count, 1U);
    CHECK_FALSE(doc.find_style("SourceB").has_value());
    const auto dry_run_target_usage = doc.find_style_usage("TargetStyle");
    REQUIRE(dry_run_target_usage.has_value());
    CHECK_EQ(dry_run_target_usage->paragraph_count, 2U);

    const auto restore = doc.restore_style_refactor({result->rollback_entries[1]});
    REQUIRE(restore.has_value());
    CHECK(restore->restored());
    CHECK(restore->changed);
    CHECK_EQ(restore->requested_count, 1U);
    CHECK_EQ(restore->restored_count, 1U);
    CHECK_EQ(restore->restored_style_count, 1U);
    CHECK_EQ(restore->restored_reference_count, 1U);
    REQUIRE_EQ(restore->operations.size(), 1U);
    CHECK(restore->operations[0].restored);
    CHECK(restore->operations[0].style_restored);
    CHECK_EQ(restore->operations[0].restored_reference_count, 1U);
    CHECK(doc.find_style("SourceB").has_value());
    const auto restored_source_usage = doc.find_style_usage("SourceB");
    REQUIRE(restored_source_usage.has_value());
    CHECK_EQ(restored_source_usage->paragraph_count, 1U);
    const auto restored_target_usage = doc.find_style_usage("TargetStyle");
    REQUIRE(restored_target_usage.has_value());
    CHECK_EQ(restored_target_usage->paragraph_count, 1U);

    const auto restore_conflict_plan =
        doc.plan_style_refactor_restore({result->rollback_entries[1]});
    REQUIRE(restore_conflict_plan.has_value());
    CHECK_FALSE(restore_conflict_plan->restored());
    CHECK(restore_conflict_plan->dry_run);
    CHECK_EQ(restore_conflict_plan->issue_count(), 1U);
    const auto restore_conflict_summary = restore_conflict_plan->issue_summary();
    REQUIRE_EQ(restore_conflict_summary.size(), 1U);
    CHECK_EQ(restore_conflict_summary[0].code, "source_style_exists");
    CHECK_EQ(restore_conflict_summary[0].count, 1U);
    CHECK_NE(restore_conflict_summary[0].suggestion.find(
                 "skip this rollback entry"),
             std::string::npos);
    REQUIRE_EQ(restore_conflict_plan->operations.size(), 1U);
    REQUIRE_EQ(restore_conflict_plan->operations[0].issues.size(), 1U);
    CHECK_EQ(restore_conflict_plan->operations[0].issues[0].code,
             "source_style_exists");
    CHECK_NE(restore_conflict_plan->operations[0].issues[0].suggestion.find(
                 "skip this rollback entry"),
             std::string::npos);

    const auto conflict = doc.apply_style_refactor(
        {
            {featherdoc::style_refactor_action::rename, "TargetStyle", "FinalTarget"},
            {featherdoc::style_refactor_action::merge, "TargetStyle", "RenamedA"},
        });
    REQUIRE(conflict.has_value());
    CHECK_FALSE(conflict->applied());
    CHECK_FALSE(conflict->changed);
    CHECK_EQ(conflict->applied_count, 0U);
    CHECK_EQ(conflict->skipped_count(), 2U);
    CHECK_EQ(conflict->plan.issue_count, 2U);
    CHECK_EQ(conflict->plan.operations[0].issues[0].code,
             "duplicate_source_operation");
    CHECK_EQ(conflict->plan.operations[1].issues[0].code,
             "duplicate_source_operation");
    CHECK(doc.find_style("TargetStyle").has_value());
    CHECK_FALSE(doc.find_style("FinalTarget").has_value());

    fs::remove(target);
}

TEST_CASE("prune_unused_styles removes unreachable custom styles and preserves dependencies") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "style_prune_round_trip.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    featherdoc::paragraph_style_definition dependency_style;
    dependency_style.name = "Dependency Style";
    dependency_style.based_on = "Normal";
    CHECK(doc.ensure_paragraph_style("DependencyStyle", dependency_style));

    featherdoc::paragraph_style_definition used_style;
    used_style.name = "Used Style";
    used_style.based_on = "DependencyStyle";
    CHECK(doc.ensure_paragraph_style("UsedStyle", used_style));

    featherdoc::paragraph_style_definition unused_base;
    unused_base.name = "Unused Base";
    unused_base.based_on = "Normal";
    CHECK(doc.ensure_paragraph_style("UnusedBase", unused_base));

    featherdoc::paragraph_style_definition unused_child;
    unused_child.name = "Unused Child";
    unused_child.based_on = "UnusedBase";
    unused_child.next_style = "UnusedBase";
    CHECK(doc.ensure_paragraph_style("UnusedChild", unused_child));

    auto paragraph = doc.paragraphs();
    REQUIRE(paragraph.has_next());
    REQUIRE(paragraph.add_run("prune style paragraph").has_next());
    CHECK(doc.set_paragraph_style(paragraph, "UsedStyle"));

    const auto plan = doc.plan_prune_unused_styles();
    REQUIRE(plan.has_value());
    CHECK(plan->changed());
    CHECK_GE(plan->scanned_style_count, 4U);
    CHECK_GE(plan->protected_style_count, 2U);
    CHECK_EQ(plan->removable_style_ids.size(), 2U);
    CHECK(std::find(plan->removable_style_ids.begin(), plan->removable_style_ids.end(),
                    "UnusedBase") != plan->removable_style_ids.end());
    CHECK(std::find(plan->removable_style_ids.begin(), plan->removable_style_ids.end(),
                    "UnusedChild") != plan->removable_style_ids.end());
    CHECK(doc.find_style("UnusedBase").has_value());
    CHECK(doc.find_style("UnusedChild").has_value());

    const auto summary = doc.prune_unused_styles();
    REQUIRE(summary.has_value());
    CHECK(summary->changed());
    CHECK_GE(summary->scanned_style_count, 4U);
    CHECK_GE(summary->protected_style_count, 2U);
    CHECK_EQ(summary->removed_style_ids.size(), 2U);
    CHECK(std::find(summary->removed_style_ids.begin(), summary->removed_style_ids.end(),
                    "UnusedBase") != summary->removed_style_ids.end());
    CHECK(std::find(summary->removed_style_ids.begin(), summary->removed_style_ids.end(),
                    "UnusedChild") != summary->removed_style_ids.end());

    CHECK(doc.find_style("UsedStyle").has_value());
    CHECK(doc.find_style("DependencyStyle").has_value());
    CHECK_FALSE(doc.find_style("UnusedBase").has_value());
    CHECK_FALSE(doc.find_style("UnusedChild").has_value());

    const auto usage = doc.find_style_usage("UsedStyle");
    REQUIRE(usage.has_value());
    CHECK_EQ(usage->paragraph_count, 1U);
    CHECK_EQ(usage->total_count(), 1U);

    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK(reopened.find_style("UsedStyle").has_value());
    CHECK(reopened.find_style("DependencyStyle").has_value());
    CHECK_FALSE(reopened.find_style("UnusedBase").has_value());
    CHECK_FALSE(reopened.find_style("UnusedChild").has_value());

    const auto styles_xml = read_test_docx_entry(target, "word/styles.xml");
    CHECK_NE(styles_xml.find(R"(w:styleId="UsedStyle")"), std::string::npos);
    CHECK_NE(styles_xml.find(R"(w:styleId="DependencyStyle")"), std::string::npos);
    CHECK_EQ(styles_xml.find(R"(w:styleId="UnusedBase")"), std::string::npos);
    CHECK_EQ(styles_xml.find(R"(w:styleId="UnusedChild")"), std::string::npos);

    fs::remove(target);
}

TEST_CASE("find_style_usage also scans header and footer parts") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "style_usage_header_footer.docx";
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
  <Override PartName="/word/header1.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.header+xml"/>
  <Override PartName="/word/footer1.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.footer+xml"/>
</Types>
)";
    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"
            xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships">
  <w:body>
    <w:p><w:r><w:t>section 0 body</w:t></w:r></w:p>
    <w:p>
      <w:pPr>
        <w:sectPr>
          <w:headerReference w:type="default" r:id="rId3"/>
          <w:footerReference w:type="default" r:id="rId4"/>
        </w:sectPr>
      </w:pPr>
      <w:r><w:t>section break</w:t></w:r>
    </w:p>
    <w:p><w:r><w:t>section 1 body</w:t></w:r></w:p>
    <w:sectPr>
      <w:headerReference w:type="default" r:id="rId3"/>
      <w:footerReference w:type="first" r:id="rId4"/>
      <w:titlePg/>
    </w:sectPr>
  </w:body>
</w:document>
)";
    const std::string document_relationships_xml =
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
    const std::string styles_xml =
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
    const std::string header_xml =
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
    const std::string footer_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:ftr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p>
    <w:r>
      <w:rPr><w:rStyle w:val="Strong"/></w:rPr>
      <w:t>footer run</w:t>
    </w:r>
  </w:p>
  <w:tbl>
    <w:tblPr>
      <w:tblStyle w:val="ReportTable"/>
      <w:tblW w:w="0" w:type="auto"/>
    </w:tblPr>
    <w:tr><w:tc><w:p><w:r><w:t>footer cell</w:t></w:r></w:p></w:tc></w:tr>
  </w:tbl>
</w:ftr>
)";

    write_test_archive_entries(
        target,
        {
            {test_content_types_xml_entry, content_types_xml},
            {test_relationships_xml_entry, test_relationships_xml},
            {test_document_xml_entry, document_xml},
            {"word/_rels/document.xml.rels", document_relationships_xml},
            {"word/styles.xml", styles_xml},
            {"word/header1.xml", header_xml},
            {"word/footer1.xml", footer_xml},
        });

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    const auto paragraph_usage = doc.find_style_usage("CustomBody");
    REQUIRE(paragraph_usage.has_value());
    CHECK_EQ(paragraph_usage->style_id, "CustomBody");
    CHECK_EQ(paragraph_usage->paragraph_count, 1U);
    CHECK_EQ(paragraph_usage->run_count, 0U);
    CHECK_EQ(paragraph_usage->table_count, 0U);
    CHECK_EQ(paragraph_usage->total_count(), 1U);
    CHECK_EQ(paragraph_usage->body.total_count(), 0U);
    CHECK_EQ(paragraph_usage->header.paragraph_count, 1U);
    CHECK_EQ(paragraph_usage->header.total_count(), 1U);
    CHECK_EQ(paragraph_usage->footer.total_count(), 0U);
    REQUIRE_EQ(paragraph_usage->hits.size(), 1U);
    CHECK_EQ(paragraph_usage->hits[0].part, featherdoc::style_usage_part_kind::header);
    CHECK_EQ(paragraph_usage->hits[0].kind, featherdoc::style_usage_hit_kind::paragraph);
    CHECK_EQ(paragraph_usage->hits[0].entry_name, "word/header1.xml");
    CHECK_EQ(paragraph_usage->hits[0].ordinal, 1U);
    CHECK_FALSE(paragraph_usage->hits[0].section_index.has_value());
    REQUIRE_EQ(paragraph_usage->hits[0].references.size(), 2U);
    CHECK_EQ(paragraph_usage->hits[0].references[0].section_index, 0U);
    CHECK_EQ(paragraph_usage->hits[0].references[0].reference_kind,
             featherdoc::section_reference_kind::default_reference);
    CHECK_EQ(paragraph_usage->hits[0].references[1].section_index, 1U);
    CHECK_EQ(paragraph_usage->hits[0].references[1].reference_kind,
             featherdoc::section_reference_kind::default_reference);

    const auto run_usage = doc.find_style_usage("Strong");
    REQUIRE(run_usage.has_value());
    CHECK_EQ(run_usage->style_id, "Strong");
    CHECK_EQ(run_usage->paragraph_count, 0U);
    CHECK_EQ(run_usage->run_count, 1U);
    CHECK_EQ(run_usage->table_count, 0U);
    CHECK_EQ(run_usage->total_count(), 1U);
    CHECK_EQ(run_usage->body.total_count(), 0U);
    CHECK_EQ(run_usage->header.total_count(), 0U);
    CHECK_EQ(run_usage->footer.run_count, 1U);
    CHECK_EQ(run_usage->footer.total_count(), 1U);
    REQUIRE_EQ(run_usage->hits.size(), 1U);
    CHECK_EQ(run_usage->hits[0].part, featherdoc::style_usage_part_kind::footer);
    CHECK_EQ(run_usage->hits[0].kind, featherdoc::style_usage_hit_kind::run);
    CHECK_EQ(run_usage->hits[0].entry_name, "word/footer1.xml");
    CHECK_EQ(run_usage->hits[0].ordinal, 1U);
    CHECK_FALSE(run_usage->hits[0].section_index.has_value());
    REQUIRE_EQ(run_usage->hits[0].references.size(), 2U);
    CHECK_EQ(run_usage->hits[0].references[0].section_index, 0U);
    CHECK_EQ(run_usage->hits[0].references[0].reference_kind,
             featherdoc::section_reference_kind::default_reference);
    CHECK_EQ(run_usage->hits[0].references[1].section_index, 1U);
    CHECK_EQ(run_usage->hits[0].references[1].reference_kind,
             featherdoc::section_reference_kind::first_page);

    const auto table_usage = doc.find_style_usage("ReportTable");
    REQUIRE(table_usage.has_value());
    CHECK_EQ(table_usage->style_id, "ReportTable");
    CHECK_EQ(table_usage->paragraph_count, 0U);
    CHECK_EQ(table_usage->run_count, 0U);
    CHECK_EQ(table_usage->table_count, 2U);
    CHECK_EQ(table_usage->total_count(), 2U);
    CHECK_EQ(table_usage->body.total_count(), 0U);
    CHECK_EQ(table_usage->header.table_count, 1U);
    CHECK_EQ(table_usage->footer.table_count, 1U);
    CHECK_EQ(table_usage->header.total_count(), 1U);
    CHECK_EQ(table_usage->footer.total_count(), 1U);
    REQUIRE_EQ(table_usage->hits.size(), 2U);
    CHECK_EQ(table_usage->hits[0].part, featherdoc::style_usage_part_kind::header);
    CHECK_EQ(table_usage->hits[0].kind, featherdoc::style_usage_hit_kind::table);
    CHECK_EQ(table_usage->hits[0].entry_name, "word/header1.xml");
    CHECK_EQ(table_usage->hits[0].ordinal, 1U);
    CHECK_FALSE(table_usage->hits[0].section_index.has_value());
    REQUIRE_EQ(table_usage->hits[0].references.size(), 2U);
    CHECK_EQ(table_usage->hits[0].references[0].section_index, 0U);
    CHECK_EQ(table_usage->hits[0].references[0].reference_kind,
             featherdoc::section_reference_kind::default_reference);
    CHECK_EQ(table_usage->hits[0].references[1].section_index, 1U);
    CHECK_EQ(table_usage->hits[0].references[1].reference_kind,
             featherdoc::section_reference_kind::default_reference);
    CHECK_EQ(table_usage->hits[1].part, featherdoc::style_usage_part_kind::footer);
    CHECK_EQ(table_usage->hits[1].kind, featherdoc::style_usage_hit_kind::table);
    CHECK_EQ(table_usage->hits[1].entry_name, "word/footer1.xml");
    CHECK_EQ(table_usage->hits[1].ordinal, 1U);
    CHECK_FALSE(table_usage->hits[1].section_index.has_value());
    REQUIRE_EQ(table_usage->hits[1].references.size(), 2U);
    CHECK_EQ(table_usage->hits[1].references[0].section_index, 0U);
    CHECK_EQ(table_usage->hits[1].references[0].reference_kind,
             featherdoc::section_reference_kind::default_reference);
    CHECK_EQ(table_usage->hits[1].references[1].section_index, 1U);
    CHECK_EQ(table_usage->hits[1].references[1].reference_kind,
             featherdoc::section_reference_kind::first_page);

    const auto unused_usage = doc.find_style_usage("Normal");
    REQUIRE(unused_usage.has_value());
    CHECK_EQ(unused_usage->style_id, "Normal");
    CHECK_EQ(unused_usage->total_count(), 0U);
    CHECK_EQ(unused_usage->body.total_count(), 0U);
    CHECK_EQ(unused_usage->header.total_count(), 0U);
    CHECK_EQ(unused_usage->footer.total_count(), 0U);
    CHECK(unused_usage->hits.empty());

    fs::remove(target);
}

TEST_CASE("clear_paragraph_style and clear_run_style remove markup and reject empty ids") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "paragraph_run_style_clear.docx";
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
    <w:p>
      <w:pPr><w:pStyle w:val="Heading1"/></w:pPr>
      <w:r>
        <w:rPr><w:rStyle w:val="Strong"/></w:rPr>
        <w:t>seed</w:t>
      </w:r>
    </w:p>
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
  <w:style w:type="paragraph" w:default="1" w:styleId="Normal">
    <w:name w:val="Normal"/>
  </w:style>
  <w:style w:type="paragraph" w:styleId="Heading1">
    <w:name w:val="heading 1"/>
    <w:basedOn w:val="Normal"/>
  </w:style>
  <w:style w:type="character" w:default="1" w:styleId="DefaultParagraphFont">
    <w:name w:val="Default Paragraph Font"/>
  </w:style>
  <w:style w:type="character" w:styleId="Strong">
    <w:name w:val="Strong"/>
    <w:basedOn w:val="DefaultParagraphFont"/>
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

    auto paragraph = doc.paragraphs();
    REQUIRE(paragraph.has_next());
    CHECK_FALSE(doc.set_paragraph_style(paragraph, ""));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));

    auto run = paragraph.runs();
    REQUIRE(run.has_next());
    CHECK_FALSE(doc.set_run_style(run, ""));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));

    CHECK(doc.clear_paragraph_style(paragraph));
    CHECK(doc.clear_run_style(run));
    CHECK_FALSE(doc.save());

    const auto saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    CHECK_EQ(count_substring_occurrences(saved_document_xml, "<w:pStyle"), 0);
    CHECK_EQ(count_substring_occurrences(saved_document_xml, "<w:rStyle"), 0);
    CHECK_EQ(count_substring_occurrences(saved_document_xml, "<w:pPr>"), 0);
    CHECK_EQ(count_substring_occurrences(saved_document_xml, "<w:rPr>"), 0);

    CHECK(test_docx_entry_exists(target, "word/styles.xml"));

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_document_text(reopened), "seed\n");

    fs::remove(target);
}
