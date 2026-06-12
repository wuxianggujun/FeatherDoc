#include <algorithm>
#include <filesystem>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

#include "doctest.h"
#include "basic_docx_archive_test_support.hpp"
#include "basic_document_xml_test_support.hpp"

#include <featherdoc.hpp>

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
