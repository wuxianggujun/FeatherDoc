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
