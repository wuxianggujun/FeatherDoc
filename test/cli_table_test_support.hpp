#pragma once

#include "cli_test_support.hpp"

#include <zip.h>

namespace {
void create_cli_table_inspection_fixture(const fs::path &path) {
    remove_if_exists(path);

    featherdoc::Document document(path);
    REQUIRE_FALSE(document.create_empty());

    auto table = document.append_table(2, 2);
    REQUIRE(table.has_next());
    REQUIRE(table.set_width_twips(7200U));
    REQUIRE(table.set_style_id("TableGrid"));
    REQUIRE(table.set_column_width_twips(0U, 1800U));
    REQUIRE(table.set_column_width_twips(1U, 5400U));

    auto row = table.rows();
    REQUIRE(row.has_next());
    auto cell = row.cells();
    REQUIRE(cell.has_next());
    REQUIRE(cell.set_text("r0c0"));
    cell.next();
    REQUIRE(cell.has_next());
    REQUIRE(cell.set_text("r0c1"));

    row.next();
    REQUIRE(row.has_next());
    cell = row.cells();
    REQUIRE(cell.has_next());
    REQUIRE(cell.set_text("r1c0"));
    cell.next();
    REQUIRE(cell.has_next());
    REQUIRE(cell.set_text("r1c1"));

    auto second_table = document.append_table(2, 3);
    REQUIRE(second_table.has_next());
    REQUIRE(second_table.set_column_width_twips(0U, 1200U));
    REQUIRE(second_table.set_column_width_twips(1U, 2200U));
    REQUIRE(second_table.set_column_width_twips(2U, 3200U));

    row = second_table.rows();
    REQUIRE(row.has_next());
    cell = row.cells();
    REQUIRE(cell.has_next());
    REQUIRE(cell.set_text("merged-top"));
    REQUIRE(cell.merge_right());
    REQUIRE(cell.set_vertical_alignment(
        featherdoc::cell_vertical_alignment::center));
    REQUIRE(cell.set_text_direction(
        featherdoc::cell_text_direction::top_to_bottom_right_to_left));

    cell.next();
    REQUIRE(cell.has_next());
    REQUIRE(cell.set_text("top-right"));

    row.next();
    REQUIRE(row.has_next());
    cell = row.cells();
    REQUIRE(cell.has_next());
    REQUIRE(cell.set_text("bottom-left"));

    cell.next();
    REQUIRE(cell.has_next());
    REQUIRE(cell.set_width_twips(2222U));
    REQUIRE(cell.set_text("bottom-middle"));

    cell.next();
    REQUIRE(cell.has_next());
    REQUIRE(cell.set_text("line1"));
    auto paragraph = cell.paragraphs();
    REQUIRE(paragraph.has_next());
    auto inserted_paragraph = paragraph.insert_paragraph_after("line2");
    REQUIRE(inserted_paragraph.has_next());

    REQUIRE_FALSE(document.save());
}

void create_cli_table_style_region_audit_fixture(const fs::path &path) {
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
  <w:body><w:p/><w:sectPr/></w:body>
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
    <w:tblStylePr w:type="firstRow"><w:rPr><w:b/></w:rPr></w:tblStylePr>
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

void create_cli_table_style_inheritance_audit_fixture(const fs::path &path) {
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
  <w:body><w:p/><w:sectPr/></w:body>
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

void create_cli_table_style_quality_audit_fixture(const fs::path &path) {
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

void create_cli_table_style_look_check_fixture(const fs::path &path) {
    remove_if_exists(path);

    featherdoc::Document document(path);
    REQUIRE_FALSE(document.create_empty());

    auto first_row = featherdoc::table_style_region_definition{};
    first_row.fill_color = std::string{"1F4E79"};

    auto second_banded_rows = featherdoc::table_style_region_definition{};
    second_banded_rows.fill_color = std::string{"F2F2F2"};

    auto table_style = featherdoc::table_style_definition{};
    table_style.name = "Look Check Table";
    table_style.first_row = first_row;
    table_style.second_banded_rows = second_banded_rows;
    REQUIRE(document.ensure_table_style("LookCheckTable", table_style));

    auto table = document.append_table(2U, 2U);
    REQUIRE(table.has_next());
    REQUIRE(table.set_style_id("LookCheckTable"));

    auto style_look = featherdoc::table_style_look{};
    style_look.first_row = false;
    style_look.first_column = false;
    style_look.banded_rows = false;
    REQUIRE(table.set_style_look(style_look));

    REQUIRE_FALSE(document.save());
}

void populate_table_cells(featherdoc::Table table,
                          const std::vector<std::vector<std::string>> &rows) {
    auto row = table.rows();
    for (std::size_t row_index = 0; row_index < rows.size(); ++row_index) {
        REQUIRE(row.has_next());
        auto cell = row.cells();
        for (std::size_t cell_index = 0; cell_index < rows[row_index].size();
             ++cell_index) {
            REQUIRE(cell.has_next());
            REQUIRE(cell.set_text(rows[row_index][cell_index]));
            if (cell_index + 1U < rows[row_index].size()) {
                cell.next();
            }
        }
        if (row_index + 1U < rows.size()) {
            row.next();
        }
    }
}

void create_cli_table_cell_style_fixture(const fs::path &path) {
    remove_if_exists(path);

    featherdoc::Document document(path);
    REQUIRE_FALSE(document.create_empty());

    auto table = document.append_table(1, 1);
    REQUIRE(table.has_next());
    auto cell = table.rows().cells();
    REQUIRE(cell.has_next());
    REQUIRE(cell.set_text("styled-cell"));

    REQUIRE_FALSE(document.save());
}

void create_cli_table_row_style_fixture(const fs::path &path) {
    remove_if_exists(path);

    featherdoc::Document document(path);
    REQUIRE_FALSE(document.create_empty());

    auto table = document.append_table(2, 1);
    REQUIRE(table.has_next());

    auto row = table.rows();
    REQUIRE(row.has_next());
    auto cell = row.cells();
    REQUIRE(cell.has_next());
    REQUIRE(cell.set_text("header-row"));

    row.next();
    REQUIRE(row.has_next());
    cell = row.cells();
    REQUIRE(cell.has_next());
    REQUIRE(cell.set_text("body-row"));

    REQUIRE_FALSE(document.save());
}

void create_cli_table_merge_fixture(const fs::path &path) {
    remove_if_exists(path);

    featherdoc::Document document(path);
    REQUIRE_FALSE(document.create_empty());

    auto table = document.append_table(3, 3);
    REQUIRE(table.has_next());

    const std::vector<std::vector<std::string>> rows = {
        {"A", "B", "C"},
        {"D", "E", "F"},
        {"G", "H", "I"},
    };

    auto row = table.rows();
    for (std::size_t row_index = 0; row_index < rows.size(); ++row_index) {
        REQUIRE(row.has_next());
        auto cell = row.cells();
        for (std::size_t cell_index = 0; cell_index < rows[row_index].size();
             ++cell_index) {
            REQUIRE(cell.has_next());
            REQUIRE(cell.set_text(rows[row_index][cell_index]));
            if (cell_index + 1U < rows[row_index].size()) {
                cell.next();
            }
        }
        if (row_index + 1U < rows.size()) {
            row.next();
        }
    }

    REQUIRE_FALSE(document.save());
}

void create_cli_table_column_horizontal_merge_fixture(const fs::path &path) {
    remove_if_exists(path);

    featherdoc::Document document(path);
    REQUIRE_FALSE(document.create_empty());

    auto table = document.append_table(2, 3);
    REQUIRE(table.has_next());
    populate_table_cells(table,
                         {{"h00", "h01", "h02"},
                          {"m00", "m01", "m02"}});

    auto row = table.rows();
    REQUIRE(row.has_next());
    row.next();
    REQUIRE(row.has_next());
    auto cell = row.cells();
    REQUIRE(cell.has_next());
    REQUIRE(cell.merge_right(1U));

    REQUIRE_FALSE(document.save());
}

void create_cli_table_unmerge_right_fixture(const fs::path &path) {
    remove_if_exists(path);

    featherdoc::Document document(path);
    REQUIRE_FALSE(document.create_empty());

    auto table = document.append_table(1, 3);
    REQUIRE(table.has_next());

    auto row = table.rows();
    REQUIRE(row.has_next());

    auto cell = row.cells();
    REQUIRE(cell.has_next());
    REQUIRE(cell.set_text("A"));
    cell.next();
    REQUIRE(cell.has_next());
    REQUIRE(cell.set_text("B"));
    cell.next();
    REQUIRE(cell.has_next());
    REQUIRE(cell.set_text("C"));

    cell = row.cells();
    REQUIRE(cell.has_next());
    REQUIRE(cell.merge_right(1U));

    REQUIRE_FALSE(document.save());
}

void create_cli_table_unmerge_down_fixture(const fs::path &path) {
    remove_if_exists(path);

    featherdoc::Document document(path);
    REQUIRE_FALSE(document.create_empty());

    auto table = document.append_table(3, 2);
    REQUIRE(table.has_next());

    const std::vector<std::vector<std::string>> rows = {
        {"A", "B"},
        {"C", "D"},
        {"E", "F"},
    };

    auto row = table.rows();
    for (std::size_t row_index = 0; row_index < rows.size(); ++row_index) {
        REQUIRE(row.has_next());
        auto cell = row.cells();
        for (std::size_t cell_index = 0; cell_index < rows[row_index].size();
             ++cell_index) {
            REQUIRE(cell.has_next());
            REQUIRE(cell.set_text(rows[row_index][cell_index]));
            if (cell_index + 1U < rows[row_index].size()) {
                cell.next();
            }
        }
        if (row_index + 1U < rows.size()) {
            row.next();
        }
    }

    auto anchor = table.rows().cells();
    REQUIRE(anchor.has_next());
    REQUIRE(anchor.merge_down(2U));

    REQUIRE_FALSE(document.save());
}

} // namespace
