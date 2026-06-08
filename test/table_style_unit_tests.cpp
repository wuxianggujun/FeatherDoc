#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "basic_docx_archive_test_support.hpp"
#include "basic_document_xml_test_support.hpp"

#include <featherdoc.hpp>
TEST_CASE("table style look can be set saved and reopened") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "table_style_look.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto table = doc.append_table(2, 2);
    auto initial_style_look = table.style_look();
    REQUIRE(initial_style_look.has_value());
    CHECK(initial_style_look->first_row);
    CHECK_FALSE(initial_style_look->last_row);
    CHECK(initial_style_look->first_column);
    CHECK_FALSE(initial_style_look->last_column);
    CHECK(initial_style_look->banded_rows);
    CHECK_FALSE(initial_style_look->banded_columns);

    featherdoc::table_style_look updated_style_look{};
    updated_style_look.first_row = false;
    updated_style_look.last_row = true;
    updated_style_look.first_column = false;
    updated_style_look.last_column = true;
    updated_style_look.banded_rows = false;
    updated_style_look.banded_columns = true;
    CHECK(table.set_style_look(updated_style_look));

    const auto style_look = table.style_look();
    REQUIRE(style_look.has_value());
    CHECK_FALSE(style_look->first_row);
    CHECK(style_look->last_row);
    CHECK_FALSE(style_look->first_column);
    CHECK(style_look->last_column);
    CHECK_FALSE(style_look->banded_rows);
    CHECK(style_look->banded_columns);

    CHECK_FALSE(doc.save());

    const auto xml_text = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(xml_text.c_str()));

    const auto table_look = xml_document.child("w:document")
                                .child("w:body")
                                .child("w:tbl")
                                .child("w:tblPr")
                                .child("w:tblLook");
    REQUIRE(table_look != pugi::xml_node{});
    CHECK_EQ(std::string_view{table_look.attribute("w:val").value()}, "0340");
    CHECK_EQ(std::string_view{table_look.attribute("w:firstRow").value()}, "0");
    CHECK_EQ(std::string_view{table_look.attribute("w:lastRow").value()}, "1");
    CHECK_EQ(std::string_view{table_look.attribute("w:firstColumn").value()}, "0");
    CHECK_EQ(std::string_view{table_look.attribute("w:lastColumn").value()}, "1");
    CHECK_EQ(std::string_view{table_look.attribute("w:noHBand").value()}, "1");
    CHECK_EQ(std::string_view{table_look.attribute("w:noVBand").value()}, "0");

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());

    auto reopened_table = reopened.tables();
    REQUIRE(reopened_table.has_next());
    auto reopened_style_look = reopened_table.style_look();
    REQUIRE(reopened_style_look.has_value());
    CHECK_FALSE(reopened_style_look->first_row);
    CHECK(reopened_style_look->last_row);
    CHECK_FALSE(reopened_style_look->first_column);
    CHECK(reopened_style_look->last_column);
    CHECK_FALSE(reopened_style_look->banded_rows);
    CHECK(reopened_style_look->banded_columns);

    fs::remove(target);
}

TEST_CASE("table style region audit reports empty declared regions") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "table_style_region_audit.docx";
    fs::remove(target);

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body><w:p/><w:sectPr/></w:body>
</w:document>
)";
    const std::string styles_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:styles xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:style w:type="paragraph" w:default="1" w:styleId="Normal">
    <w:name w:val="Normal"/>
  </w:style>
  <w:style w:type="table" w:styleId="SparseTable">
    <w:name w:val="Sparse Table"/>
    <w:tblPr/>
    <w:tblStylePr w:type="firstRow"/>
    <w:tblStylePr w:type="band1Horz">
      <w:tcPr><w:shd w:fill="F2F2F2"/></w:tcPr>
    </w:tblStylePr>
  </w:style>
  <w:style w:type="table" w:styleId="CleanTable">
    <w:name w:val="Clean Table"/>
    <w:tblStylePr w:type="firstRow">
      <w:rPr><w:b/></w:rPr>
    </w:tblStylePr>
  </w:style>
</w:styles>
)";
    write_test_docx_with_styles(target, document_xml, styles_xml);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    const auto report = doc.audit_table_style_regions();
    CHECK_EQ(report.table_style_count, 2U);
    CHECK_EQ(report.region_count, 4U);
    CHECK_FALSE(report.ok());
    REQUIRE_EQ(report.issue_count(), 2U);
    CHECK_EQ(report.issues[0].style_id, "SparseTable");
    CHECK_EQ(report.issues[0].style_name, "Sparse Table");
    CHECK_EQ(report.issues[0].region, "whole_table");
    CHECK_EQ(report.issues[0].issue_type, "empty_region");
    CHECK_EQ(report.issues[0].property_count, 0U);
    CHECK_EQ(report.issues[1].style_id, "SparseTable");
    CHECK_EQ(report.issues[1].region, "first_row");

    const auto filtered = doc.audit_table_style_regions("CleanTable");
    CHECK_EQ(filtered.table_style_count, 1U);
    CHECK_EQ(filtered.region_count, 1U);
    CHECK(filtered.ok());

    fs::remove(target);
}

TEST_CASE("table style inheritance audit reports invalid based_on chains") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "table_style_inheritance_audit.docx";
    fs::remove(target);

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body><w:p/><w:sectPr/></w:body>
</w:document>
)";
    const std::string styles_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:styles xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:style w:type="paragraph" w:default="1" w:styleId="Normal">
    <w:name w:val="Normal"/>
  </w:style>
  <w:style w:type="table" w:styleId="BaseTable">
    <w:name w:val="Base Table"/>
  </w:style>
  <w:style w:type="table" w:styleId="ChildTable">
    <w:name w:val="Child Table"/>
    <w:basedOn w:val="BaseTable"/>
  </w:style>
  <w:style w:type="table" w:styleId="MissingBaseTable">
    <w:name w:val="Missing Base Table"/>
    <w:basedOn w:val="MissingTable"/>
  </w:style>
  <w:style w:type="table" w:styleId="ParagraphBasedTable">
    <w:name w:val="Paragraph Based Table"/>
    <w:basedOn w:val="Normal"/>
  </w:style>
  <w:style w:type="table" w:styleId="CycleA">
    <w:name w:val="Cycle A"/>
    <w:basedOn w:val="CycleB"/>
  </w:style>
  <w:style w:type="table" w:styleId="CycleB">
    <w:name w:val="Cycle B"/>
    <w:basedOn w:val="CycleA"/>
  </w:style>
</w:styles>
)";
    write_test_docx_with_styles(target, document_xml, styles_xml);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    const auto report = doc.audit_table_style_inheritance();
    CHECK_EQ(report.table_style_count, 6U);
    CHECK_FALSE(report.ok());
    REQUIRE_EQ(report.issue_count(), 4U);

    auto find_issue = [&](std::string_view style_id, std::string_view issue_type)
        -> const featherdoc::table_style_inheritance_audit_issue * {
        for (const auto &issue : report.issues) {
            if (issue.style_id == style_id && issue.issue_type == issue_type) {
                return &issue;
            }
        }
        return nullptr;
    };

    const auto *missing_issue = find_issue("MissingBaseTable", "missing_based_on");
    REQUIRE(missing_issue != nullptr);
    CHECK_EQ(missing_issue->style_name, "Missing Base Table");
    CHECK_EQ(missing_issue->based_on_style_id, "MissingTable");
    CHECK(missing_issue->based_on_style_kind.empty());
    REQUIRE_EQ(missing_issue->inheritance_chain.size(), 1U);
    CHECK_EQ(missing_issue->inheritance_chain[0], "MissingBaseTable");

    const auto *kind_issue = find_issue("ParagraphBasedTable", "based_on_not_table");
    REQUIRE(kind_issue != nullptr);
    CHECK_EQ(kind_issue->based_on_style_id, "Normal");
    CHECK_EQ(kind_issue->based_on_style_kind, "paragraph");
    REQUIRE_EQ(kind_issue->inheritance_chain.size(), 1U);
    CHECK_EQ(kind_issue->inheritance_chain[0], "ParagraphBasedTable");

    const auto *cycle_issue = find_issue("CycleA", "inheritance_cycle");
    REQUIRE(cycle_issue != nullptr);
    CHECK_EQ(cycle_issue->based_on_style_id, "CycleA");
    CHECK_EQ(cycle_issue->based_on_style_kind, "table");
    REQUIRE_EQ(cycle_issue->inheritance_chain.size(), 3U);
    CHECK_EQ(cycle_issue->inheritance_chain[0], "CycleA");
    CHECK_EQ(cycle_issue->inheritance_chain[1], "CycleB");
    CHECK_EQ(cycle_issue->inheritance_chain[2], "CycleA");

    const auto filtered = doc.audit_table_style_inheritance("ChildTable");
    CHECK_EQ(filtered.table_style_count, 1U);
    CHECK(filtered.ok());

    fs::remove(target);
}

TEST_CASE("table style quality audit aggregates region inheritance and look issues") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "table_style_quality_audit.docx";
    fs::remove(target);

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:tbl>
      <w:tblPr>
        <w:tblStyle w:val="QualityLookTable"/>
        <w:tblLook w:val="0000" w:firstRow="0" w:lastRow="0" w:firstColumn="1" w:lastColumn="0" w:noHBand="0" w:noVBand="1"/>
      </w:tblPr>
      <w:tblGrid><w:gridCol w:w="2400"/></w:tblGrid>
      <w:tr>
        <w:tc>
          <w:tcPr><w:tcW w:w="2400" w:type="dxa"/></w:tcPr>
          <w:p><w:r><w:t>seed</w:t></w:r></w:p>
        </w:tc>
      </w:tr>
    </w:tbl>
    <w:sectPr/>
  </w:body>
</w:document>
)";
    const std::string styles_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:styles xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:style w:type="paragraph" w:default="1" w:styleId="Normal">
    <w:name w:val="Normal"/>
  </w:style>
  <w:style w:type="table" w:styleId="SparseTable">
    <w:name w:val="Sparse Table"/>
    <w:tblPr/>
  </w:style>
  <w:style w:type="table" w:styleId="MissingBaseTable">
    <w:name w:val="Missing Base Table"/>
    <w:basedOn w:val="MissingTable"/>
  </w:style>
  <w:style w:type="table" w:styleId="QualityLookTable">
    <w:name w:val="Quality Look Table"/>
    <w:tblStylePr w:type="firstRow"><w:rPr><w:b/></w:rPr></w:tblStylePr>
  </w:style>
</w:styles>
)";
    write_test_docx_with_styles(target, document_xml, styles_xml);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    const auto report = doc.audit_table_style_quality();
    CHECK_FALSE(report.ok());
    CHECK_EQ(report.issue_count(), 3U);
    CHECK_EQ(report.region_audit.table_style_count, 3U);
    CHECK_EQ(report.region_audit.issue_count(), 1U);
    CHECK_EQ(report.region_audit.issues[0].style_id, "SparseTable");
    CHECK_EQ(report.inheritance_audit.table_style_count, 3U);
    CHECK_EQ(report.inheritance_audit.issue_count(), 1U);
    CHECK_EQ(report.inheritance_audit.issues[0].style_id, "MissingBaseTable");
    CHECK_EQ(report.inheritance_audit.issues[0].issue_type, "missing_based_on");
    CHECK_EQ(report.style_look.table_count, 1U);
    CHECK_EQ(report.style_look.issue_count(), 1U);
    CHECK_EQ(report.style_look.issues[0].style_id, "QualityLookTable");
    CHECK_EQ(report.style_look.issues[0].issue_type, "style_look_disabled");
    CHECK_EQ(report.style_look.issues[0].region, "first_row");

    const auto plan = doc.plan_table_style_quality_fixes();
    CHECK_FALSE(plan.ok());
    CHECK_EQ(plan.issue_count(), 3U);
    CHECK_EQ(plan.items.size(), 3U);
    CHECK_EQ(plan.automatic_fix_count(), 1U);
    CHECK_EQ(plan.manual_fix_count(), 2U);
    CHECK_EQ(plan.items[0].source, "region_audit");
    CHECK_EQ(plan.items[0].action, "edit_table_style_region");
    CHECK_FALSE(plan.items[0].automatic);
    CHECK_EQ(plan.items[1].source, "inheritance_audit");
    CHECK_EQ(plan.items[1].action, "create_or_clear_based_on");
    CHECK_FALSE(plan.items[1].automatic);
    CHECK_EQ(plan.items[2].source, "style_look");
    CHECK_EQ(plan.items[2].action, "repair_table_style_look");
    CHECK(plan.items[2].automatic);
    CHECK_EQ(plan.items[2].command, "repair-table-style-look --plan-only");
    CHECK_EQ(plan.items[2].table_index, std::optional<std::size_t>{0U});

    fs::remove(target);
}

TEST_CASE("table style look consistency reports disabled conditional regions") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "table_style_look_consistency.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto first_row = featherdoc::table_style_region_definition{};
    first_row.fill_color = std::string{"1F4E79"};

    auto second_banded_rows = featherdoc::table_style_region_definition{};
    second_banded_rows.fill_color = std::string{"F2F2F2"};

    auto table_style = featherdoc::table_style_definition{};
    table_style.name = "Look Check Table";
    table_style.first_row = first_row;
    table_style.second_banded_rows = second_banded_rows;
    REQUIRE(doc.ensure_table_style("LookCheckTable", table_style));

    auto table = doc.append_table(2U, 2U);
    REQUIRE(table.has_next());
    REQUIRE(table.set_style_id("LookCheckTable"));

    auto style_look = featherdoc::table_style_look{};
    style_look.first_row = false;
    style_look.first_column = false;
    style_look.banded_rows = false;
    REQUIRE(table.set_style_look(style_look));

    const auto report = doc.check_table_style_look_consistency();
    CHECK(report.table_count == 1U);
    CHECK_FALSE(report.ok());
    REQUIRE(report.issues.size() == 2U);
    CHECK_EQ(report.issues[0].table_index, 0U);
    CHECK_EQ(report.issues[0].style_id, "LookCheckTable");
    CHECK_EQ(report.issues[0].issue_type, "style_look_disabled");
    CHECK_EQ(report.issues[0].region, "first_row");
    CHECK_EQ(report.issues[0].required_style_look_flag, "first_row");
    CHECK_EQ(report.issues[0].actual_value, std::optional<bool>{false});
    CHECK_EQ(report.issues[1].region, "second_banded_rows");
    CHECK_EQ(report.issues[1].required_style_look_flag, "banded_rows");

    const auto repair = doc.repair_table_style_look_consistency();
    CHECK(repair.changed());
    CHECK_EQ(repair.changed_table_count, 1U);
    CHECK_EQ(repair.before.issue_count(), 2U);
    CHECK(repair.after.ok());
    CHECK_EQ(repair.after.issue_count(), 0U);

    auto repaired_table = doc.tables();
    REQUIRE(repaired_table.has_next());
    const auto repaired_style_look = repaired_table.style_look();
    REQUIRE(repaired_style_look.has_value());
    CHECK(repaired_style_look->first_row);
    CHECK_FALSE(repaired_style_look->first_column);
    CHECK(repaired_style_look->banded_rows);

    fs::remove(target);
}

TEST_CASE("table style look repair writes missing style look defaults") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "table_style_look_repair_missing.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto first_row = featherdoc::table_style_region_definition{};
    first_row.fill_color = std::string{"1F4E79"};

    auto second_banded_rows = featherdoc::table_style_region_definition{};
    second_banded_rows.fill_color = std::string{"F2F2F2"};

    auto table_style = featherdoc::table_style_definition{};
    table_style.name = "Missing Look Table";
    table_style.first_row = first_row;
    table_style.second_banded_rows = second_banded_rows;
    REQUIRE(doc.ensure_table_style("MissingLookTable", table_style));

    auto table = doc.append_table(2U, 2U);
    REQUIRE(table.has_next());
    REQUIRE(table.set_style_id("MissingLookTable"));
    REQUIRE(table.clear_style_look());
    CHECK_FALSE(table.style_look().has_value());

    const auto report = doc.check_table_style_look_consistency();
    CHECK_FALSE(report.ok());
    REQUIRE_EQ(report.issue_count(), 2U);
    CHECK_EQ(report.issues[0].issue_type, "style_look_missing");
    CHECK_FALSE(report.issues[0].actual_value.has_value());

    const auto repair = doc.repair_table_style_look_consistency();
    CHECK(repair.ok());
    CHECK_EQ(repair.changed_table_count, 1U);
    CHECK_EQ(repair.after.issue_count(), 0U);

    auto repaired_table = doc.tables();
    REQUIRE(repaired_table.has_next());
    const auto repaired_style_look = repaired_table.style_look();
    REQUIRE(repaired_style_look.has_value());
    CHECK(repaired_style_look->first_row);
    CHECK(repaired_style_look->banded_rows);

    fs::remove(target);
}

TEST_CASE("table style look can be read from val-only XML and cleared") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "table_style_look_val_only.docx";
    fs::remove(target);

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:tbl>
      <w:tblPr>
        <w:tblLook w:val="0260"/>
      </w:tblPr>
      <w:tblGrid>
        <w:gridCol w:w="2400"/>
      </w:tblGrid>
      <w:tr>
        <w:tc>
          <w:tcPr>
            <w:tcW w:w="2400" w:type="dxa"/>
          </w:tcPr>
          <w:p><w:r><w:t>seed</w:t></w:r></w:p>
        </w:tc>
      </w:tr>
    </w:tbl>
  </w:body>
</w:document>
)";
    write_test_docx(target, document_xml);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    auto table = doc.tables();
    REQUIRE(table.has_next());
    const auto style_look = table.style_look();
    REQUIRE(style_look.has_value());
    CHECK(style_look->first_row);
    CHECK(style_look->last_row);
    CHECK_FALSE(style_look->first_column);
    CHECK_FALSE(style_look->last_column);
    CHECK_FALSE(style_look->banded_rows);
    CHECK(style_look->banded_columns);

    CHECK(table.clear_style_look());
    CHECK_FALSE(doc.save());

    const auto xml_text = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(xml_text.c_str()));

    const auto table_properties =
        xml_document.child("w:document").child("w:body").child("w:tbl").child("w:tblPr");
    REQUIRE(table_properties != pugi::xml_node{});
    CHECK_EQ(table_properties.child("w:tblLook"), pugi::xml_node{});

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());

    auto reopened_table = reopened.tables();
    REQUIRE(reopened_table.has_next());
    CHECK_FALSE(reopened_table.style_look().has_value());

    fs::remove(target);
}
