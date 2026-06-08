#pragma once

#include "cli_test_support.hpp"

#include <zip.h>

namespace {
void create_cli_template_table_bookmark_fixture(const fs::path &path) {
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
</Types>
)";
    constexpr auto document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p><w:r><w:t>bookmark template intro</w:t></w:r></w:p>
    <w:tbl>
      <w:tblGrid>
        <w:gridCol w:w="2400"/>
        <w:gridCol w:w="2400"/>
      </w:tblGrid>
      <w:tr>
        <w:tc><w:p><w:r><w:t>keep-00</w:t></w:r></w:p></w:tc>
        <w:tc><w:p><w:r><w:t>keep-01</w:t></w:r></w:p></w:tc>
      </w:tr>
    </w:tbl>
    <w:p>
      <w:bookmarkStart w:id="0" w:name="target_before_table"/>
      <w:r><w:t>before target table</w:t></w:r>
      <w:bookmarkEnd w:id="0"/>
    </w:p>
    <w:tbl>
      <w:tblGrid>
        <w:gridCol w:w="2400"/>
        <w:gridCol w:w="2400"/>
      </w:tblGrid>
      <w:tr>
        <w:tc>
          <w:p>
            <w:bookmarkStart w:id="1" w:name="target_inside_table"/>
            <w:r><w:t>target-00</w:t></w:r>
            <w:bookmarkEnd w:id="1"/>
          </w:p>
        </w:tc>
        <w:tc><w:p><w:r><w:t>target-01</w:t></w:r></w:p></w:tc>
      </w:tr>
      <w:tr>
        <w:tc><w:p><w:r><w:t>target-10</w:t></w:r></w:p></w:tc>
        <w:tc><w:p><w:r><w:t>target-11</w:t></w:r></w:p></w:tc>
      </w:tr>
    </w:tbl>
    <w:p><w:r><w:t>bookmark template outro</w:t></w:r></w:p>
  </w:body>
</w:document>
)";

    int zip_error = 0;
    zip_t *archive = zip_openwitherror(path.string().c_str(),
                                       ZIP_DEFAULT_COMPRESSION_LEVEL, 'w',
                                       &zip_error);
    REQUIRE(archive != nullptr);
    REQUIRE(write_archive_entry(archive, "_rels/.rels", relationships_xml));
    REQUIRE(write_archive_entry(archive, "[Content_Types].xml", content_types_xml));
    REQUIRE(write_archive_entry(archive, "word/document.xml", document_xml));
    zip_close(archive);
}

void create_cli_template_table_selector_fixture(const fs::path &path) {
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
</Types>
)";
    constexpr auto document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p><w:r><w:t>selector intro</w:t></w:r></w:p>
    <w:p><w:r><w:t>selector target table</w:t></w:r></w:p>
    <w:tbl>
      <w:tblGrid>
        <w:gridCol w:w="2400"/>
        <w:gridCol w:w="2400"/>
        <w:gridCol w:w="2400"/>
      </w:tblGrid>
      <w:tr>
        <w:tc><w:p><w:r><w:t>Region</w:t></w:r></w:p></w:tc>
        <w:tc><w:p><w:r><w:t>Qty</w:t></w:r></w:p></w:tc>
        <w:tc><w:p><w:r><w:t>Amount</w:t></w:r></w:p></w:tc>
      </w:tr>
      <w:tr>
        <w:tc><w:p><w:r><w:t>North</w:t></w:r></w:p></w:tc>
        <w:tc><w:p><w:r><w:t>7</w:t></w:r></w:p></w:tc>
        <w:tc><w:p><w:r><w:t>12</w:t></w:r></w:p></w:tc>
      </w:tr>
    </w:tbl>
    <w:p><w:r><w:t>selector middle</w:t></w:r></w:p>
    <w:tbl>
      <w:tblGrid>
        <w:gridCol w:w="2400"/>
        <w:gridCol w:w="2400"/>
      </w:tblGrid>
      <w:tr>
        <w:tc><w:p><w:r><w:t>Label</w:t></w:r></w:p></w:tc>
        <w:tc><w:p><w:r><w:t>Value</w:t></w:r></w:p></w:tc>
      </w:tr>
      <w:tr>
        <w:tc><w:p><w:r><w:t>Ignore</w:t></w:r></w:p></w:tc>
        <w:tc><w:p><w:r><w:t>Keep</w:t></w:r></w:p></w:tc>
      </w:tr>
    </w:tbl>
    <w:p><w:r><w:t>selector target table</w:t></w:r></w:p>
    <w:tbl>
      <w:tblGrid>
        <w:gridCol w:w="2400"/>
        <w:gridCol w:w="2400"/>
        <w:gridCol w:w="2400"/>
      </w:tblGrid>
      <w:tr>
        <w:tc><w:p><w:r><w:t>Region</w:t></w:r></w:p></w:tc>
        <w:tc><w:p><w:r><w:t>Qty</w:t></w:r></w:p></w:tc>
        <w:tc><w:p><w:r><w:t>Amount</w:t></w:r></w:p></w:tc>
      </w:tr>
      <w:tr>
        <w:tc><w:p><w:r><w:t>South</w:t></w:r></w:p></w:tc>
        <w:tc><w:p><w:r><w:t>9</w:t></w:r></w:p></w:tc>
        <w:tc><w:p><w:r><w:t>24</w:t></w:r></w:p></w:tc>
      </w:tr>
    </w:tbl>
    <w:p><w:r><w:t>selector outro</w:t></w:r></w:p>
  </w:body>
</w:document>
)";

    int zip_error = 0;
    zip_t *archive = zip_openwitherror(path.string().c_str(),
                                       ZIP_DEFAULT_COMPRESSION_LEVEL, 'w',
                                       &zip_error);
    REQUIRE(archive != nullptr);
    REQUIRE(write_archive_entry(archive, "_rels/.rels", relationships_xml));
    REQUIRE(write_archive_entry(archive, "[Content_Types].xml", content_types_xml));
    REQUIRE(write_archive_entry(archive, "word/document.xml", document_xml));
    zip_close(archive);
}

void create_cli_template_inspection_fixture(const fs::path &path) {
    remove_if_exists(path);

    featherdoc::Document document(path);
    REQUIRE_FALSE(document.create_empty());

    auto numbering_definition = featherdoc::numbering_definition{};
    numbering_definition.name = "CliTemplateInspectOutline";
    numbering_definition.levels = {
        featherdoc::numbering_level_definition{
            featherdoc::list_kind::decimal, 1U, 0U, "%1."},
    };

    const auto numbering_id =
        document.ensure_numbering_definition(numbering_definition);
    REQUIRE(numbering_id.has_value());

    auto body_template = document.body_template();
    REQUIRE(static_cast<bool>(body_template));
    auto body_paragraph = body_template.paragraphs();
    REQUIRE(body_paragraph.has_next());
    CHECK(document.set_paragraph_style(body_paragraph, "Heading1"));
    CHECK(body_paragraph.set_bidi());
    REQUIRE(body_paragraph.add_run("Body heading").has_next());

    auto numbered_body_paragraph = body_paragraph.insert_paragraph_after("Body item");
    REQUIRE(numbered_body_paragraph.has_next());
    CHECK(document.set_paragraph_numbering(numbered_body_paragraph, *numbering_id));

    auto &header_paragraph = document.ensure_header_paragraphs();
    REQUIRE(header_paragraph.has_next());
    CHECK(header_paragraph.set_text("Header intro"));

    auto header_template = document.header_template(0U);
    REQUIRE(static_cast<bool>(header_template));
    auto header_table = header_template.append_table(2U, 2U);
    REQUIRE(header_table.has_next());
    CHECK(header_table.set_style_id("TableGrid"));
    CHECK(header_table.set_column_width_twips(0U, 1800U));
    CHECK(header_table.set_column_width_twips(1U, 3600U));

    auto header_row = header_table.rows();
    REQUIRE(header_row.has_next());
    auto header_cell = header_row.cells();
    REQUIRE(header_cell.has_next());
    CHECK(header_cell.set_text("h00"));
    header_cell.next();
    REQUIRE(header_cell.has_next());
    CHECK(header_cell.set_text("h01"));

    header_row.next();
    REQUIRE(header_row.has_next());
    header_cell = header_row.cells();
    REQUIRE(header_cell.has_next());
    CHECK(header_cell.set_text("h10"));
    header_cell.next();
    REQUIRE(header_cell.has_next());
    CHECK(header_cell.set_width_twips(2222U));
    CHECK(header_cell.set_text("h11"));

    auto &footer_paragraph = document.ensure_section_footer_paragraphs(0U);
    REQUIRE(footer_paragraph.has_next());
    CHECK(footer_paragraph.set_text("Footer intro"));

    auto footer_template = document.section_footer_template(0U);
    REQUIRE(static_cast<bool>(footer_template));
    auto footer_inspected_paragraph = footer_template.append_paragraph();
    REQUIRE(footer_inspected_paragraph.has_next());
    auto footer_styled_run = footer_inspected_paragraph.add_run("Footer styled");
    REQUIRE(footer_styled_run.has_next());
    CHECK(document.set_run_style(footer_styled_run, "Strong"));
    CHECK(footer_styled_run.set_font_family("Segoe UI"));
    CHECK(footer_styled_run.set_language("en-US"));
    auto footer_plain_run = footer_inspected_paragraph.add_run(" tail");
    REQUIRE(footer_plain_run.has_next());

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

void create_cli_template_table_merge_unmerge_fixture(const fs::path &path) {
    remove_if_exists(path);

    featherdoc::Document document(path);
    REQUIRE_FALSE(document.create_empty());

    auto body_template = document.body_template();
    REQUIRE(static_cast<bool>(body_template));
    auto body_paragraph = body_template.paragraphs();
    REQUIRE(body_paragraph.has_next());
    REQUIRE(body_paragraph.set_text("Body template merge fixture"));

    auto body_table = body_template.append_table(1U, 3U);
    REQUIRE(body_table.has_next());
    populate_table_cells(body_table, {{"A", "B", "C"}});
    auto body_cell = body_table.rows().cells();
    REQUIRE(body_cell.has_next());
    REQUIRE(body_cell.merge_right(1U));

    auto &header_paragraph = document.ensure_header_paragraphs();
    REQUIRE(header_paragraph.has_next());
    REQUIRE(header_paragraph.set_text("Header template merge fixture"));

    auto header_template = document.header_template(0U);
    REQUIRE(static_cast<bool>(header_template));
    auto header_table = header_template.append_table(3U, 3U);
    REQUIRE(header_table.has_next());
    populate_table_cells(header_table,
                         {{"hA", "hB", "hC"},
                          {"hD", "hE", "hF"},
                          {"hG", "hH", "hI"}});

    auto &footer_paragraph = document.ensure_section_footer_paragraphs(0U);
    REQUIRE(footer_paragraph.has_next());
    REQUIRE(footer_paragraph.set_text("Section footer merge fixture"));

    auto footer_template = document.section_footer_template(0U);
    REQUIRE(static_cast<bool>(footer_template));
    auto footer_table = footer_template.append_table(3U, 2U);
    REQUIRE(footer_table.has_next());
    populate_table_cells(footer_table,
                         {{"fA", "fB"},
                          {"fC", "fD"},
                          {"fE", "fF"}});
    auto footer_cell = footer_table.rows().cells();
    REQUIRE(footer_cell.has_next());
    REQUIRE(footer_cell.merge_down(2U));

    auto direct_footer_template = document.footer_template(0U);
    REQUIRE(static_cast<bool>(direct_footer_template));
    auto direct_footer_table = direct_footer_template.append_table(3U, 2U);
    REQUIRE(direct_footer_table.has_next());
    populate_table_cells(direct_footer_table,
                         {{"dfA", "dfB"},
                          {"dfC", "dfD"},
                          {"dfE", "dfF"}});
    auto direct_footer_cell = direct_footer_table.rows().cells();
    REQUIRE(direct_footer_cell.has_next());
    REQUIRE(direct_footer_cell.merge_down(2U));

    REQUIRE_FALSE(document.save());
}

void configure_cli_template_table_fixture(
    featherdoc::Table table, const std::vector<std::uint32_t> &column_widths,
    const std::vector<std::vector<std::string>> &rows) {
    REQUIRE(table.has_next());
    REQUIRE_FALSE(column_widths.empty());

    std::uint32_t total_width = 0U;
    for (const auto width : column_widths) {
        total_width += width;
    }

    CHECK(table.set_style_id("TableGrid"));
    CHECK(table.set_width_twips(total_width));
    CHECK(table.set_layout_mode(featherdoc::table_layout_mode::fixed));
    for (std::size_t column_index = 0; column_index < column_widths.size();
         ++column_index) {
        CHECK(table.set_column_width_twips(column_index,
                                           column_widths[column_index]));
    }

    populate_table_cells(table, rows);
}

void create_cli_template_table_column_fixture(const fs::path &path) {
    remove_if_exists(path);

    featherdoc::Document document(path);
    REQUIRE_FALSE(document.create_empty());

    auto body_template = document.body_template();
    REQUIRE(static_cast<bool>(body_template));
    auto body_paragraph = body_template.paragraphs();
    REQUIRE(body_paragraph.has_next());
    REQUIRE(body_paragraph.set_text("Body template column fixture"));
    auto body_table = body_template.append_table(2U, 3U);
    configure_cli_template_table_fixture(body_table, {1200U, 2200U, 2800U},
                                         {{"b00", "b01", "b02"},
                                          {"b10", "b11", "b12"}});

    auto &header_paragraph = document.ensure_section_header_paragraphs(0U);
    REQUIRE(header_paragraph.has_next());
    REQUIRE(header_paragraph.set_text("Section header template column fixture"));
    auto header_template = document.section_header_template(0U);
    REQUIRE(static_cast<bool>(header_template));
    auto header_table = header_template.append_table(2U, 2U);
    configure_cli_template_table_fixture(header_table, {1800U, 3600U},
                                         {{"h00", "h01"},
                                          {"h10", "h11"}});

    auto &footer_paragraph = document.ensure_section_footer_paragraphs(0U);
    REQUIRE(footer_paragraph.has_next());
    REQUIRE(footer_paragraph.set_text("Section footer template column fixture"));
    auto footer_template = document.section_footer_template(0U);
    REQUIRE(static_cast<bool>(footer_template));
    auto footer_table = footer_template.append_table(2U, 2U);
    configure_cli_template_table_fixture(footer_table, {1600U, 2800U},
                                         {{"f00", "f01"},
                                          {"f10", "f11"}});

    REQUIRE_FALSE(document.save());
}

void create_cli_template_table_direct_column_fixture(const fs::path &path) {
    remove_if_exists(path);

    featherdoc::Document document(path);
    REQUIRE_FALSE(document.create_empty());

    auto body_template = document.body_template();
    REQUIRE(static_cast<bool>(body_template));
    auto body_paragraph = body_template.paragraphs();
    REQUIRE(body_paragraph.has_next());
    REQUIRE(body_paragraph.set_text("Body template direct column fixture"));
    auto body_table = body_template.append_table(2U, 3U);
    configure_cli_template_table_fixture(body_table, {1200U, 2200U, 2800U},
                                         {{"b00", "b01", "b02"},
                                          {"b10", "b11", "b12"}});

    auto &header_paragraph = document.ensure_header_paragraphs();
    REQUIRE(header_paragraph.has_next());
    REQUIRE(header_paragraph.set_text("Direct header template column fixture"));
    auto header_template = document.header_template(0U);
    REQUIRE(static_cast<bool>(header_template));
    auto header_table = header_template.append_table(2U, 2U);
    configure_cli_template_table_fixture(header_table, {1800U, 3600U},
                                         {{"h00", "h01"},
                                          {"h10", "h11"}});

    auto &footer_paragraph = document.ensure_footer_paragraphs();
    REQUIRE(footer_paragraph.has_next());
    REQUIRE(footer_paragraph.set_text("Direct footer template column fixture"));
    auto footer_template = document.footer_template(0U);
    REQUIRE(static_cast<bool>(footer_template));
    auto footer_table = footer_template.append_table(2U, 2U);
    configure_cli_template_table_fixture(footer_table, {1600U, 2800U},
                                         {{"f00", "f01"},
                                          {"f10", "f11"}});

    REQUIRE_FALSE(document.save());
}

void create_cli_template_table_section_kind_fixture(const fs::path &path) {
    remove_if_exists(path);

    featherdoc::Document document(path);
    REQUIRE_FALSE(document.create_empty());

    auto body_template = document.body_template();
    REQUIRE(static_cast<bool>(body_template));
    auto body_paragraph = body_template.paragraphs();
    REQUIRE(body_paragraph.has_next());
    REQUIRE(body_paragraph.set_text("Body template section kind fixture"));

    auto &default_header = document.ensure_section_header_paragraphs(0U);
    REQUIRE(default_header.has_next());
    REQUIRE(default_header.set_text("Default section header kind fixture"));
    auto default_header_template = document.section_header_template(0U);
    REQUIRE(static_cast<bool>(default_header_template));
    auto default_header_table = default_header_template.append_table(2U, 2U);
    configure_cli_template_table_fixture(default_header_table, {1800U, 3600U},
                                         {{"dh00", "dh01"},
                                          {"dh10", "dh11"}});

    auto &even_header = document.ensure_section_header_paragraphs(
        0U, featherdoc::section_reference_kind::even_page);
    REQUIRE(even_header.has_next());
    REQUIRE(even_header.set_text("Even section header kind fixture"));
    auto even_header_template = document.section_header_template(
        0U, featherdoc::section_reference_kind::even_page);
    REQUIRE(static_cast<bool>(even_header_template));
    auto even_header_table = even_header_template.append_table(2U, 2U);
    configure_cli_template_table_fixture(even_header_table, {1800U, 3600U},
                                         {{"eh00", "eh01"},
                                          {"eh10", "eh11"}});

    auto &default_footer = document.ensure_section_footer_paragraphs(0U);
    REQUIRE(default_footer.has_next());
    REQUIRE(default_footer.set_text("Default section footer kind fixture"));
    auto default_footer_template = document.section_footer_template(0U);
    REQUIRE(static_cast<bool>(default_footer_template));
    auto default_footer_table = default_footer_template.append_table(2U, 2U);
    configure_cli_template_table_fixture(default_footer_table, {1600U, 2800U},
                                         {{"df00", "df01"},
                                          {"df10", "df11"}});

    auto &first_footer = document.ensure_section_footer_paragraphs(
        0U, featherdoc::section_reference_kind::first_page);
    REQUIRE(first_footer.has_next());
    REQUIRE(first_footer.set_text("First section footer kind fixture"));
    auto first_footer_template = document.section_footer_template(
        0U, featherdoc::section_reference_kind::first_page);
    REQUIRE(static_cast<bool>(first_footer_template));
    auto first_footer_table = first_footer_template.append_table(2U, 2U);
    configure_cli_template_table_fixture(first_footer_table, {1600U, 2800U},
                                         {{"ff00", "ff01"},
                                          {"ff10", "ff11"}});

    REQUIRE_FALSE(document.save());
}

void create_cli_template_table_section_kind_merge_unmerge_fixture(
    const fs::path &path) {
    remove_if_exists(path);

    featherdoc::Document document(path);
    REQUIRE_FALSE(document.create_empty());

    auto body_template = document.body_template();
    REQUIRE(static_cast<bool>(body_template));
    auto body_paragraph = body_template.paragraphs();
    REQUIRE(body_paragraph.has_next());
    REQUIRE(body_paragraph.set_text("Body template section kind merge fixture"));

    auto &default_header = document.ensure_section_header_paragraphs(0U);
    REQUIRE(default_header.has_next());
    REQUIRE(default_header.set_text("Default section header merge fixture"));
    auto default_header_template = document.section_header_template(0U);
    REQUIRE(static_cast<bool>(default_header_template));
    auto default_header_table = default_header_template.append_table(2U, 3U);
    configure_cli_template_table_fixture(default_header_table,
                                         {1600U, 1800U, 3000U},
                                         {{"dh00", "dh01", "dh02"},
                                          {"dh10", "dh11", "dh12"}});

    auto &even_header = document.ensure_section_header_paragraphs(
        0U, featherdoc::section_reference_kind::even_page);
    REQUIRE(even_header.has_next());
    REQUIRE(even_header.set_text("Even section header merge fixture"));
    auto even_header_template = document.section_header_template(
        0U, featherdoc::section_reference_kind::even_page);
    REQUIRE(static_cast<bool>(even_header_template));
    auto even_header_table = even_header_template.append_table(2U, 3U);
    configure_cli_template_table_fixture(even_header_table,
                                         {1600U, 1800U, 3000U},
                                         {{"eh00", "eh01", "eh02"},
                                          {"eh10", "eh11", "eh12"}});

    auto &default_footer = document.ensure_section_footer_paragraphs(0U);
    REQUIRE(default_footer.has_next());
    REQUIRE(default_footer.set_text("Default section footer merge fixture"));
    auto default_footer_template = document.section_footer_template(0U);
    REQUIRE(static_cast<bool>(default_footer_template));
    auto default_footer_table = default_footer_template.append_table(3U, 2U);
    configure_cli_template_table_fixture(default_footer_table, {1800U, 4200U},
                                         {{"df00", "df01"},
                                          {"df10", "df11"},
                                          {"df20", "df21"}});
    auto default_footer_cell = default_footer_table.rows().cells();
    REQUIRE(default_footer_cell.has_next());
    REQUIRE(default_footer_cell.merge_down(2U));

    auto &first_footer = document.ensure_section_footer_paragraphs(
        0U, featherdoc::section_reference_kind::first_page);
    REQUIRE(first_footer.has_next());
    REQUIRE(first_footer.set_text("First section footer merge fixture"));
    auto first_footer_template = document.section_footer_template(
        0U, featherdoc::section_reference_kind::first_page);
    REQUIRE(static_cast<bool>(first_footer_template));
    auto first_footer_table = first_footer_template.append_table(3U, 2U);
    configure_cli_template_table_fixture(first_footer_table, {1800U, 4200U},
                                         {{"ff00", "ff01"},
                                          {"ff10", "ff11"},
                                          {"ff20", "ff21"}});
    auto first_footer_cell = first_footer_table.rows().cells();
    REQUIRE(first_footer_cell.has_next());
    REQUIRE(first_footer_cell.merge_down(2U));

    REQUIRE_FALSE(document.save());
}

} // namespace
