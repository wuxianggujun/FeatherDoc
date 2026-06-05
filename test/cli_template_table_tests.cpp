#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "cli_test_support.hpp"

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

TEST_CASE("cli inspect-template-paragraphs inspects body template paragraphs") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_inspect_template_paragraphs_source.docx";
    const fs::path output =
        working_directory / "cli_inspect_template_paragraphs_output.txt";
    const fs::path json_output =
        working_directory / "cli_inspect_template_paragraph_single.json";

    remove_if_exists(source);
    remove_if_exists(output);
    remove_if_exists(json_output);

    create_cli_template_inspection_fixture(source);

    CHECK_EQ(run_cli({"inspect-template-paragraphs", source.string()}, output), 0);
    const auto inspect_text = read_text_file(output);
    CHECK_NE(inspect_text.find("part: body\n"), std::string::npos);
    CHECK_NE(inspect_text.find("entry_name: word/document.xml\n"),
             std::string::npos);
    CHECK_NE(inspect_text.find("paragraphs: 2"), std::string::npos);
    CHECK_NE(
        inspect_text.find(
            "paragraph[0]: style_id=Heading1 bidi=yes numbering=none runs=1 "
            "text=Body heading"),
        std::string::npos);
    CHECK_NE(inspect_text.find("paragraph[1]: style_id=none bidi=none numbering=num_id="),
             std::string::npos);
    CHECK_NE(inspect_text.find("definition_name=CliTemplateInspectOutline"),
             std::string::npos);
    CHECK_NE(inspect_text.find("text=Body item"), std::string::npos);

    CHECK_EQ(run_cli({"inspect-template-paragraphs",
                      source.string(),
                      "--paragraph",
                      "1",
                      "--json"},
                     json_output),
             0);
    const auto paragraph_json = read_text_file(json_output);
    CHECK_NE(paragraph_json.find("\"part\":\"body\""), std::string::npos);
    CHECK_NE(paragraph_json.find("\"entry_name\":\"word/document.xml\""),
             std::string::npos);
    CHECK_NE(paragraph_json.find("\"paragraph\":{\"index\":1"),
             std::string::npos);
    CHECK_NE(paragraph_json.find("\"definition_name\":\"CliTemplateInspectOutline\""),
             std::string::npos);
    CHECK_NE(paragraph_json.find("\"run_count\":1"), std::string::npos);
    CHECK_NE(paragraph_json.find("\"text\":\"Body item\""), std::string::npos);

    remove_if_exists(source);
    remove_if_exists(output);
    remove_if_exists(json_output);
}

TEST_CASE("cli inspect-template-runs inspects section footer runs and errors") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_inspect_template_runs_source.docx";
    const fs::path output =
        working_directory / "cli_inspect_template_runs_output.txt";
    const fs::path missing_output =
        working_directory / "cli_inspect_template_runs_missing.json";
    const fs::path parse_output =
        working_directory / "cli_inspect_template_runs_parse.json";

    remove_if_exists(source);
    remove_if_exists(output);
    remove_if_exists(missing_output);
    remove_if_exists(parse_output);

    create_cli_template_inspection_fixture(source);

    CHECK_EQ(run_cli({"inspect-template-runs",
                      source.string(),
                      "1",
                      "--part",
                      "section-footer",
                      "--section",
                      "0",
                      "--run",
                      "0"},
                     output),
             0);
    const auto run_text = read_text_file(output);
    CHECK_NE(run_text.find("part: section-footer\n"), std::string::npos);
    CHECK_NE(run_text.find("section: 0\n"), std::string::npos);
    CHECK_NE(run_text.find("kind: default\n"), std::string::npos);
    CHECK_NE(run_text.find("entry_name: word/footer1.xml\n"), std::string::npos);
    CHECK_NE(run_text.find("paragraph_index: 1\n"), std::string::npos);
    CHECK_NE(run_text.find("index: 0\n"), std::string::npos);
    CHECK_NE(run_text.find("style_id: Strong\n"), std::string::npos);
    CHECK_NE(run_text.find("font_family: Segoe UI\n"), std::string::npos);
    CHECK_NE(run_text.find("text_color: none\n"), std::string::npos);
    CHECK_NE(run_text.find("bold: none\n"), std::string::npos);
    CHECK_NE(run_text.find("italic: none\n"), std::string::npos);
    CHECK_NE(run_text.find("underline: none\n"), std::string::npos);
    CHECK_NE(run_text.find("font_size_points: none\n"), std::string::npos);
    CHECK_NE(run_text.find("language: en-US\n"), std::string::npos);
    CHECK_NE(run_text.find("text: Footer styled\n"), std::string::npos);

    CHECK_EQ(run_cli({"inspect-template-runs",
                      source.string(),
                      "1",
                      "--part",
                      "section-footer",
                      "--section",
                      "0",
                      "--run",
                      "9",
                      "--json"},
                     missing_output),
             1);
    const auto missing_json = read_text_file(missing_output);
    CHECK_NE(missing_json.find("\"command\":\"inspect-template-runs\""),
             std::string::npos);
    CHECK_NE(missing_json.find("\"stage\":\"inspect\""), std::string::npos);
    CHECK_NE(missing_json.find(
                 "\"detail\":\"run index '9' is out of range for paragraph "
                 "index '1'\""),
             std::string::npos);
    CHECK_NE(missing_json.find("\"entry\":\"word/footer1.xml\""),
             std::string::npos);

    CHECK_EQ(run_cli({"inspect-template-runs",
                      source.string(),
                      "1",
                      "--part",
                      "section-footer",
                      "--json"},
                     parse_output),
             2);
    CHECK_EQ(
        read_text_file(parse_output),
        std::string{
            "{\"command\":\"inspect-template-runs\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"section-header/section-footer "
            "inspection requires --section <index>\"}\n"});

    remove_if_exists(source);
    remove_if_exists(output);
    remove_if_exists(missing_output);
    remove_if_exists(parse_output);
}

TEST_CASE("cli inspect-template-table commands inspect header parts and report errors") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_inspect_template_tables_source.docx";
    const fs::path tables_output =
        working_directory / "cli_inspect_template_tables_output.json";
    const fs::path cell_output =
        working_directory / "cli_inspect_template_table_cell_output.json";
    const fs::path parse_output =
        working_directory / "cli_inspect_template_table_parse.json";
    const fs::path row_error_output =
        working_directory / "cli_inspect_template_table_row_error.json";

    remove_if_exists(source);
    remove_if_exists(tables_output);
    remove_if_exists(cell_output);
    remove_if_exists(parse_output);
    remove_if_exists(row_error_output);

    create_cli_template_inspection_fixture(source);

    CHECK_EQ(run_cli({"inspect-template-tables",
                      source.string(),
                      "--part",
                      "header",
                      "--index",
                      "0",
                      "--json"},
                     tables_output),
             0);
    const auto tables_json = read_text_file(tables_output);
    CHECK_NE(tables_json.find("\"part\":\"header\""), std::string::npos);
    CHECK_NE(tables_json.find("\"part_index\":0"), std::string::npos);
    CHECK_NE(tables_json.find("\"entry_name\":\"word/header1.xml\""),
             std::string::npos);
    CHECK_NE(tables_json.find("\"count\":1"), std::string::npos);
    CHECK_NE(tables_json.find("\"style_id\":\"TableGrid\""), std::string::npos);
    CHECK_NE(tables_json.find("\"column_widths\":[1800,3600]"),
             std::string::npos);
    CHECK_NE(tables_json.find("\"text\":\"h00\\th01\\nh10\\th11\""),
             std::string::npos);

    CHECK_EQ(run_cli({"inspect-template-table-cells",
                      source.string(),
                      "0",
                      "--part",
                      "header",
                      "--index",
                      "0",
                      "--row",
                      "1",
                      "--cell",
                      "1",
                      "--json"},
                     cell_output),
             0);
    const auto cell_json = read_text_file(cell_output);
    CHECK_NE(cell_json.find("\"part\":\"header\""), std::string::npos);
    CHECK_NE(cell_json.find("\"part_index\":0"), std::string::npos);
    CHECK_NE(cell_json.find("\"entry_name\":\"word/header1.xml\""),
             std::string::npos);
    CHECK_NE(cell_json.find("\"table_index\":0"), std::string::npos);
    CHECK_NE(cell_json.find("\"row_index\":1"), std::string::npos);
    CHECK_NE(cell_json.find("\"cell_index\":1"), std::string::npos);
    CHECK_NE(cell_json.find("\"width_twips\":2222"), std::string::npos);
    CHECK_NE(cell_json.find("\"text\":\"h11\""), std::string::npos);

    CHECK_EQ(run_cli({"inspect-template-table-cells",
                      source.string(),
                      "0",
                      "--part",
                      "header",
                      "--index",
                      "0",
                      "--cell",
                      "1",
                      "--json"},
                     parse_output),
             2);
    CHECK_EQ(
        read_text_file(parse_output),
        std::string{
            "{\"command\":\"inspect-template-table-cells\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"--cell requires --row\"}\n"});

    CHECK_EQ(run_cli({"inspect-template-table-cells",
                      source.string(),
                      "0",
                      "--part",
                      "header",
                      "--index",
                      "0",
                      "--row",
                      "9",
                      "--json"},
                     row_error_output),
             1);
    const auto row_error_json = read_text_file(row_error_output);
    CHECK_NE(row_error_json.find("\"command\":\"inspect-template-table-cells\""),
             std::string::npos);
    CHECK_NE(row_error_json.find("\"detail\":\"row index '9' is out of range "
                                 "for table index '0'\""),
             std::string::npos);
    CHECK_NE(row_error_json.find("\"entry\":\"word/header1.xml\""),
             std::string::npos);

    remove_if_exists(source);
    remove_if_exists(tables_output);
    remove_if_exists(cell_output);
    remove_if_exists(parse_output);
    remove_if_exists(row_error_output);
}

TEST_CASE("cli inspect-template-table-rows inspects header rows and reports errors") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_inspect_template_table_rows_source.docx";
    const fs::path list_output =
        working_directory / "cli_inspect_template_table_rows_output.json";
    const fs::path single_output =
        working_directory / "cli_inspect_template_table_row_single.json";
    const fs::path error_output =
        working_directory / "cli_inspect_template_table_row_error.json";

    remove_if_exists(source);
    remove_if_exists(list_output);
    remove_if_exists(single_output);
    remove_if_exists(error_output);

    create_cli_template_inspection_fixture(source);

    CHECK_EQ(run_cli({"inspect-template-table-rows",
                      source.string(),
                      "0",
                      "--part",
                      "header",
                      "--index",
                      "0",
                      "--json"},
                     list_output),
             0);
    CHECK_EQ(
        read_text_file(list_output),
        std::string{
            "{\"part\":\"header\",\"part_index\":0,"
            "\"entry_name\":\"word/header1.xml\",\"table_index\":0,"
            "\"count\":2,\"rows\":["
            "{\"row_index\":0,\"cell_count\":2,\"height_twips\":null,"
            "\"height_rule\":null,\"cant_split\":false,"
            "\"repeats_header\":false,\"cell_texts\":[\"h00\",\"h01\"]},"
            "{\"row_index\":1,\"cell_count\":2,\"height_twips\":null,"
            "\"height_rule\":null,\"cant_split\":false,"
            "\"repeats_header\":false,\"cell_texts\":[\"h10\",\"h11\"]}"
            "]}\n"});

    CHECK_EQ(run_cli({"inspect-template-table-rows",
                      source.string(),
                      "0",
                      "--part",
                      "header",
                      "--index",
                      "0",
                      "--row",
                      "1",
                      "--json"},
                     single_output),
             0);
    CHECK_EQ(
        read_text_file(single_output),
        std::string{
            "{\"part\":\"header\",\"part_index\":0,"
            "\"entry_name\":\"word/header1.xml\",\"table_index\":0,"
            "\"row\":{\"row_index\":1,\"cell_count\":2,"
            "\"height_twips\":null,\"height_rule\":null,"
            "\"cant_split\":false,\"repeats_header\":false,"
            "\"cell_texts\":[\"h10\",\"h11\"]}}\n"});

    CHECK_EQ(run_cli({"inspect-template-table-rows",
                      source.string(),
                      "0",
                      "--part",
                      "header",
                      "--index",
                      "0",
                      "--row",
                      "9",
                      "--json"},
                     error_output),
             1);
    const auto error_json = read_text_file(error_output);
    CHECK_NE(error_json.find("\"command\":\"inspect-template-table-rows\""),
             std::string::npos);
    CHECK_NE(error_json.find("\"stage\":\"inspect\""), std::string::npos);
    CHECK_NE(error_json.find("\"detail\":\"row index '9' is out of range for "
                             "table index '0'\""),
             std::string::npos);
    CHECK_NE(error_json.find("\"entry\":\"word/header1.xml\""),
             std::string::npos);

    remove_if_exists(source);
    remove_if_exists(list_output);
    remove_if_exists(single_output);
    remove_if_exists(error_output);
}

TEST_CASE("cli set-template-table-cell-text updates header template cells and reports errors") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_set_template_table_cell_text_source.docx";
    const fs::path updated =
        working_directory / "cli_set_template_table_cell_text_updated.docx";
    const fs::path mutate_output =
        working_directory / "cli_set_template_table_cell_text_output.json";
    const fs::path parse_output =
        working_directory / "cli_set_template_table_cell_text_parse.json";
    const fs::path error_output =
        working_directory / "cli_set_template_table_cell_text_error.json";

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(mutate_output);
    remove_if_exists(parse_output);
    remove_if_exists(error_output);

    create_cli_template_inspection_fixture(source);

    CHECK_EQ(run_cli({"set-template-table-cell-text",
                      source.string(),
                      "0",
                      "1",
                      "1",
                      "--part",
                      "header",
                      "--index",
                      "0",
                      "--text",
                      "updated header cell",
                      "--output",
                      updated.string(),
                      "--json"},
                     mutate_output),
             0);

    const auto mutate_json = read_text_file(mutate_output);
    CHECK_NE(mutate_json.find("\"command\":\"set-template-table-cell-text\""),
             std::string::npos);
    CHECK_NE(mutate_json.find("\"part\":\"header\""), std::string::npos);
    CHECK_NE(mutate_json.find("\"part_index\":0"), std::string::npos);
    CHECK_NE(mutate_json.find("\"entry_name\":\"word/header1.xml\""),
             std::string::npos);
    CHECK_NE(mutate_json.find("\"table_index\":0"), std::string::npos);
    CHECK_NE(mutate_json.find("\"row_index\":1"), std::string::npos);
    CHECK_NE(mutate_json.find("\"cell_index\":1"), std::string::npos);

    featherdoc::Document reopened(updated);
    REQUIRE_FALSE(reopened.open());

    auto header_template = reopened.header_template(0U);
    REQUIRE(static_cast<bool>(header_template));
    const auto updated_cell = header_template.inspect_table_cell(0U, 1U, 1U);
    REQUIRE(updated_cell.has_value());
    CHECK_EQ(updated_cell->text, "updated header cell");
    REQUIRE(updated_cell->width_twips.has_value());
    CHECK_EQ(*updated_cell->width_twips, 2222U);

    CHECK_EQ(run_cli({"set-template-table-cell-text",
                      source.string(),
                      "0",
                      "0",
                      "0",
                      "--part",
                      "section-header",
                      "--text",
                      "x",
                      "--json"},
                     parse_output),
             2);
    CHECK_EQ(
        read_text_file(parse_output),
        std::string{
            "{\"command\":\"set-template-table-cell-text\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"section-header/section-footer "
            "mutation requires --section <index>\"}\n"});

    CHECK_EQ(run_cli({"set-template-table-cell-text",
                      source.string(),
                      "0",
                      "9",
                      "0",
                      "--part",
                      "header",
                      "--index",
                      "0",
                      "--text",
                      "x",
                      "--json"},
                     error_output),
             1);
    const auto error_json = read_text_file(error_output);
    CHECK_NE(error_json.find("\"command\":\"set-template-table-cell-text\""),
             std::string::npos);
    CHECK_NE(error_json.find("\"stage\":\"mutate\""), std::string::npos);
    CHECK_NE(error_json.find("\"entry\":\"word/header1.xml\""),
             std::string::npos);
    CHECK_NE(error_json.find("\"detail\":\"row index '9' is out of range for "
                             "table index '0'\""),
             std::string::npos);

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(mutate_output);
    remove_if_exists(parse_output);
    remove_if_exists(error_output);
}

TEST_CASE("cli set-template-table-cell-text updates direct footer template cells and reports errors") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_set_direct_footer_template_cell_text_source.docx";
    const fs::path updated =
        working_directory / "cli_set_direct_footer_template_cell_text_updated.docx";
    const fs::path mutate_output =
        working_directory / "cli_set_direct_footer_template_cell_text_output.json";
    const fs::path error_output =
        working_directory / "cli_set_direct_footer_template_cell_text_error.json";

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(mutate_output);
    remove_if_exists(error_output);

    create_cli_template_table_direct_column_fixture(source);

    CHECK_EQ(run_cli({"set-template-table-cell-text",
                      source.string(),
                      "0",
                      "1",
                      "1",
                      "--part",
                      "footer",
                      "--index",
                      "0",
                      "--text",
                      "updated footer cell",
                      "--output",
                      updated.string(),
                      "--json"},
                     mutate_output),
             0);

    const auto mutate_json = read_text_file(mutate_output);
    CHECK_NE(mutate_json.find("\"command\":\"set-template-table-cell-text\""),
             std::string::npos);
    CHECK_NE(mutate_json.find("\"part\":\"footer\""), std::string::npos);
    CHECK_NE(mutate_json.find("\"part_index\":0"), std::string::npos);
    CHECK_NE(mutate_json.find("\"entry_name\":\"word/footer1.xml\""),
             std::string::npos);
    CHECK_NE(mutate_json.find("\"table_index\":0"), std::string::npos);
    CHECK_NE(mutate_json.find("\"row_index\":1"), std::string::npos);
    CHECK_NE(mutate_json.find("\"cell_index\":1"), std::string::npos);

    featherdoc::Document reopened(updated);
    REQUIRE_FALSE(reopened.open());

    auto footer_template = reopened.footer_template(0U);
    REQUIRE(static_cast<bool>(footer_template));
    const auto updated_cell = footer_template.inspect_table_cell(0U, 1U, 1U);
    REQUIRE(updated_cell.has_value());
    CHECK_EQ(updated_cell->text, "updated footer cell");

    CHECK_EQ(run_cli({"set-template-table-cell-text",
                      source.string(),
                      "0",
                      "9",
                      "0",
                      "--part",
                      "footer",
                      "--index",
                      "0",
                      "--text",
                      "x",
                      "--json"},
                     error_output),
             1);
    const auto error_json = read_text_file(error_output);
    CHECK_NE(error_json.find("\"command\":\"set-template-table-cell-text\""),
             std::string::npos);
    CHECK_NE(error_json.find("\"stage\":\"mutate\""), std::string::npos);
    CHECK_NE(error_json.find("\"entry\":\"word/footer1.xml\""),
             std::string::npos);
    CHECK_NE(error_json.find("\"detail\":\"row index '9' is out of range for table index '0'\""),
             std::string::npos);

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(mutate_output);
    remove_if_exists(error_output);
}

TEST_CASE("cli set-template-table-cell-text updates section kind template cells") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_set_section_kind_template_cell_text_source.docx";
    const fs::path even_updated =
        working_directory / "cli_set_section_kind_even_header_updated.docx";
    const fs::path first_updated =
        working_directory / "cli_set_section_kind_first_footer_updated.docx";
    const fs::path even_output =
        working_directory / "cli_set_section_kind_even_header_output.json";
    const fs::path first_output =
        working_directory / "cli_set_section_kind_first_footer_output.json";

    remove_if_exists(source);
    remove_if_exists(even_updated);
    remove_if_exists(first_updated);
    remove_if_exists(even_output);
    remove_if_exists(first_output);

    create_cli_template_table_section_kind_fixture(source);

    CHECK_EQ(run_cli({"set-template-table-cell-text",
                      source.string(),
                      "0",
                      "1",
                      "1",
                      "--part",
                      "section-header",
                      "--section",
                      "0",
                      "--kind",
                      "even",
                      "--text",
                      "updated even header cell",
                      "--output",
                      even_updated.string(),
                      "--json"},
                     even_output),
             0);
    const auto even_json = read_text_file(even_output);
    CHECK_NE(even_json.find("\"command\":\"set-template-table-cell-text\""),
             std::string::npos);
    CHECK_NE(even_json.find("\"part\":\"section-header\""),
             std::string::npos);
    CHECK_NE(even_json.find("\"section\":0"), std::string::npos);
    CHECK_NE(even_json.find("\"kind\":\"even\""), std::string::npos);

    featherdoc::Document reopened_even(even_updated);
    REQUIRE_FALSE(reopened_even.open());
    auto even_header_template = reopened_even.section_header_template(
        0U, featherdoc::section_reference_kind::even_page);
    REQUIRE(static_cast<bool>(even_header_template));
    const auto even_cell =
        even_header_template.inspect_table_cell(0U, 1U, 1U);
    REQUIRE(even_cell.has_value());
    CHECK_EQ(even_cell->text, "updated even header cell");

    CHECK_EQ(run_cli({"set-template-table-cell-text",
                      source.string(),
                      "0",
                      "1",
                      "1",
                      "--part",
                      "section-footer",
                      "--section",
                      "0",
                      "--kind",
                      "first",
                      "--text",
                      "updated first footer cell",
                      "--output",
                      first_updated.string(),
                      "--json"},
                     first_output),
             0);
    const auto first_json = read_text_file(first_output);
    CHECK_NE(first_json.find("\"command\":\"set-template-table-cell-text\""),
             std::string::npos);
    CHECK_NE(first_json.find("\"part\":\"section-footer\""),
             std::string::npos);
    CHECK_NE(first_json.find("\"section\":0"), std::string::npos);
    CHECK_NE(first_json.find("\"kind\":\"first\""), std::string::npos);

    featherdoc::Document reopened_first(first_updated);
    REQUIRE_FALSE(reopened_first.open());
    auto first_footer_template = reopened_first.section_footer_template(
        0U, featherdoc::section_reference_kind::first_page);
    REQUIRE(static_cast<bool>(first_footer_template));
    const auto first_cell =
        first_footer_template.inspect_table_cell(0U, 1U, 1U);
    REQUIRE(first_cell.has_value());
    CHECK_EQ(first_cell->text, "updated first footer cell");

    remove_if_exists(source);
    remove_if_exists(even_updated);
    remove_if_exists(first_updated);
    remove_if_exists(even_output);
    remove_if_exists(first_output);
}

TEST_CASE("cli template table row commands mutate section kind template tables") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_section_kind_template_table_row_source.docx";
    const fs::path even_appended =
        working_directory / "cli_section_kind_template_table_even_appended.docx";
    const fs::path first_removed =
        working_directory / "cli_section_kind_template_table_first_removed.docx";
    const fs::path append_output =
        working_directory / "cli_section_kind_template_table_even_append.json";
    const fs::path remove_output =
        working_directory / "cli_section_kind_template_table_first_remove.json";

    remove_if_exists(source);
    remove_if_exists(even_appended);
    remove_if_exists(first_removed);
    remove_if_exists(append_output);
    remove_if_exists(remove_output);

    create_cli_template_table_section_kind_fixture(source);

    CHECK_EQ(run_cli({"append-template-table-row",
                      source.string(),
                      "0",
                      "--part",
                      "section-header",
                      "--section",
                      "0",
                      "--kind",
                      "even",
                      "--output",
                      even_appended.string(),
                      "--json"},
                     append_output),
             0);
    const auto append_json = read_text_file(append_output);
    CHECK_NE(append_json.find("\"command\":\"append-template-table-row\""),
             std::string::npos);
    CHECK_NE(append_json.find("\"part\":\"section-header\""),
             std::string::npos);
    CHECK_NE(append_json.find("\"section\":0"), std::string::npos);
    CHECK_NE(append_json.find("\"kind\":\"even\""), std::string::npos);
    CHECK_NE(append_json.find("\"row_index\":2"), std::string::npos);

    featherdoc::Document reopened_even(even_appended);
    REQUIRE_FALSE(reopened_even.open());
    auto even_header_template = reopened_even.section_header_template(
        0U, featherdoc::section_reference_kind::even_page);
    REQUIRE(static_cast<bool>(even_header_template));
    const auto appended_table = even_header_template.inspect_table(0U);
    REQUIRE(appended_table.has_value());
    CHECK_EQ(appended_table->row_count, 3U);
    const auto appended_cell =
        even_header_template.inspect_table_cell(0U, 2U, 0U);
    REQUIRE(appended_cell.has_value());
    CHECK_EQ(appended_cell->text, "");

    CHECK_EQ(run_cli({"remove-template-table-row",
                      source.string(),
                      "0",
                      "0",
                      "--part",
                      "section-footer",
                      "--section",
                      "0",
                      "--kind",
                      "first",
                      "--output",
                      first_removed.string(),
                      "--json"},
                     remove_output),
             0);
    const auto remove_json = read_text_file(remove_output);
    CHECK_NE(remove_json.find("\"command\":\"remove-template-table-row\""),
             std::string::npos);
    CHECK_NE(remove_json.find("\"part\":\"section-footer\""),
             std::string::npos);
    CHECK_NE(remove_json.find("\"section\":0"), std::string::npos);
    CHECK_NE(remove_json.find("\"kind\":\"first\""), std::string::npos);
    CHECK_NE(remove_json.find("\"row_index\":0"), std::string::npos);

    featherdoc::Document reopened_first(first_removed);
    REQUIRE_FALSE(reopened_first.open());
    auto first_footer_template = reopened_first.section_footer_template(
        0U, featherdoc::section_reference_kind::first_page);
    REQUIRE(static_cast<bool>(first_footer_template));
    const auto removed_table = first_footer_template.inspect_table(0U);
    REQUIRE(removed_table.has_value());
    CHECK_EQ(removed_table->row_count, 1U);
    const auto removed_first_cell =
        first_footer_template.inspect_table_cell(0U, 0U, 0U);
    REQUIRE(removed_first_cell.has_value());
    CHECK_EQ(removed_first_cell->text, "ff10");

    remove_if_exists(source);
    remove_if_exists(even_appended);
    remove_if_exists(first_removed);
    remove_if_exists(append_output);
    remove_if_exists(remove_output);
}

TEST_CASE("cli template table column commands mutate section kind template tables") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_section_kind_template_table_column_source.docx";
    const fs::path even_inserted =
        working_directory / "cli_section_kind_template_table_even_column_before.docx";
    const fs::path first_removed =
        working_directory / "cli_section_kind_template_table_first_column_removed.docx";
    const fs::path before_output =
        working_directory / "cli_section_kind_template_table_even_column_before.json";
    const fs::path remove_output =
        working_directory / "cli_section_kind_template_table_first_column_removed.json";

    remove_if_exists(source);
    remove_if_exists(even_inserted);
    remove_if_exists(first_removed);
    remove_if_exists(before_output);
    remove_if_exists(remove_output);

    create_cli_template_table_section_kind_fixture(source);

    CHECK_EQ(run_cli({"insert-template-table-column-before",
                      source.string(),
                      "0",
                      "0",
                      "1",
                      "--part",
                      "section-header",
                      "--section",
                      "0",
                      "--kind",
                      "even",
                      "--output",
                      even_inserted.string(),
                      "--json"},
                     before_output),
             0);
    const auto before_json = read_text_file(before_output);
    CHECK_NE(before_json.find("\"command\":\"insert-template-table-column-before\""),
             std::string::npos);
    CHECK_NE(before_json.find("\"part\":\"section-header\""),
             std::string::npos);
    CHECK_NE(before_json.find("\"section\":0"), std::string::npos);
    CHECK_NE(before_json.find("\"kind\":\"even\""), std::string::npos);
    CHECK_NE(before_json.find("\"inserted_cell_index\":1"),
             std::string::npos);
    CHECK_NE(before_json.find("\"inserted_column_index\":1"),
             std::string::npos);

    featherdoc::Document reopened_even(even_inserted);
    REQUIRE_FALSE(reopened_even.open());
    auto even_header_template = reopened_even.section_header_template(
        0U, featherdoc::section_reference_kind::even_page);
    REQUIRE(static_cast<bool>(even_header_template));
    const auto even_table = even_header_template.inspect_table(0U);
    REQUIRE(even_table.has_value());
    CHECK_EQ(even_table->column_count, 3U);
    REQUIRE(even_table->column_widths.size() >= 3U);
    CHECK(even_table->column_widths[0].has_value());
    CHECK(even_table->column_widths[1].has_value());
    CHECK(even_table->column_widths[2].has_value());
    CHECK_EQ(*even_table->column_widths[0], 1800U);
    CHECK_EQ(*even_table->column_widths[1], 3600U);
    CHECK_EQ(*even_table->column_widths[2], 3600U);
    const auto even_inserted_header =
        even_header_template.inspect_table_cell(0U, 0U, 1U);
    REQUIRE(even_inserted_header.has_value());
    CHECK_EQ(even_inserted_header->text, "");
    const auto even_shifted_header =
        even_header_template.inspect_table_cell(0U, 0U, 2U);
    REQUIRE(even_shifted_header.has_value());
    CHECK_EQ(even_shifted_header->text, "eh01");
    const auto even_inserted_body =
        even_header_template.inspect_table_cell(0U, 1U, 1U);
    REQUIRE(even_inserted_body.has_value());
    CHECK_EQ(even_inserted_body->text, "");

    CHECK_EQ(run_cli({"remove-template-table-column",
                      source.string(),
                      "0",
                      "0",
                      "1",
                      "--part",
                      "section-footer",
                      "--section",
                      "0",
                      "--kind",
                      "first",
                      "--output",
                      first_removed.string(),
                      "--json"},
                     remove_output),
             0);
    const auto remove_json = read_text_file(remove_output);
    CHECK_NE(remove_json.find("\"command\":\"remove-template-table-column\""),
             std::string::npos);
    CHECK_NE(remove_json.find("\"part\":\"section-footer\""),
             std::string::npos);
    CHECK_NE(remove_json.find("\"section\":0"), std::string::npos);
    CHECK_NE(remove_json.find("\"kind\":\"first\""), std::string::npos);
    CHECK_NE(remove_json.find("\"column_index\":1"), std::string::npos);

    featherdoc::Document reopened_first(first_removed);
    REQUIRE_FALSE(reopened_first.open());
    auto first_footer_template = reopened_first.section_footer_template(
        0U, featherdoc::section_reference_kind::first_page);
    REQUIRE(static_cast<bool>(first_footer_template));
    const auto first_table = first_footer_template.inspect_table(0U);
    REQUIRE(first_table.has_value());
    CHECK_EQ(first_table->column_count, 1U);
    const auto first_header_cell =
        first_footer_template.inspect_table_cell(0U, 0U, 0U);
    REQUIRE(first_header_cell.has_value());
    CHECK_EQ(first_header_cell->text, "ff00");
    const auto first_body_cell =
        first_footer_template.inspect_table_cell(0U, 1U, 0U);
    REQUIRE(first_body_cell.has_value());
    CHECK_EQ(first_body_cell->text, "ff10");

    remove_if_exists(source);
    remove_if_exists(even_inserted);
    remove_if_exists(first_removed);
    remove_if_exists(before_output);
    remove_if_exists(remove_output);
}

TEST_CASE("cli template table merge and unmerge commands mutate section kind template tables") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_section_kind_template_table_merge_source.docx";
    const fs::path even_merged =
        working_directory / "cli_section_kind_template_table_even_merged.docx";
    const fs::path first_unmerged =
        working_directory /
        "cli_section_kind_template_table_first_unmerged.docx";
    const fs::path merge_output =
        working_directory / "cli_section_kind_template_table_even_merge.json";
    const fs::path unmerge_output =
        working_directory /
        "cli_section_kind_template_table_first_unmerge.json";

    remove_if_exists(source);
    remove_if_exists(even_merged);
    remove_if_exists(first_unmerged);
    remove_if_exists(merge_output);
    remove_if_exists(unmerge_output);

    create_cli_template_table_section_kind_merge_unmerge_fixture(source);

    CHECK_EQ(run_cli({"merge-template-table-cells",
                      source.string(),
                      "0",
                      "0",
                      "0",
                      "--part",
                      "section-header",
                      "--section",
                      "0",
                      "--kind",
                      "even",
                      "--direction",
                      "right",
                      "--count",
                      "1",
                      "--output",
                      even_merged.string(),
                      "--json"},
                     merge_output),
             0);
    const auto merge_json = read_text_file(merge_output);
    CHECK_NE(merge_json.find("\"command\":\"merge-template-table-cells\""),
             std::string::npos);
    CHECK_NE(merge_json.find("\"part\":\"section-header\""),
             std::string::npos);
    CHECK_NE(merge_json.find("\"section\":0"), std::string::npos);
    CHECK_NE(merge_json.find("\"kind\":\"even\""), std::string::npos);
    CHECK_NE(merge_json.find("\"direction\":\"right\""), std::string::npos);
    CHECK_NE(merge_json.find("\"count\":1"), std::string::npos);

    featherdoc::Document reopened_even(even_merged);
    REQUIRE_FALSE(reopened_even.open());
    auto even_header_template = reopened_even.section_header_template(
        0U, featherdoc::section_reference_kind::even_page);
    REQUIRE(static_cast<bool>(even_header_template));
    const auto merged_even_cell =
        even_header_template.inspect_table_cell(0U, 0U, 0U);
    REQUIRE(merged_even_cell.has_value());
    CHECK_EQ(merged_even_cell->text, "eh00");
    CHECK_EQ(merged_even_cell->column_span, 2U);
    const auto merged_even_tail =
        even_header_template.inspect_table_cell(0U, 0U, 1U);
    REQUIRE(merged_even_tail.has_value());
    CHECK_EQ(merged_even_tail->text, "eh02");
    CHECK_EQ(merged_even_tail->column_index, 2U);

    CHECK_EQ(run_cli({"unmerge-template-table-cells",
                      source.string(),
                      "0",
                      "1",
                      "0",
                      "--part",
                      "section-footer",
                      "--section",
                      "0",
                      "--kind",
                      "first",
                      "--direction",
                      "down",
                      "--output",
                      first_unmerged.string(),
                      "--json"},
                     unmerge_output),
             0);
    const auto unmerge_json = read_text_file(unmerge_output);
    CHECK_NE(unmerge_json.find("\"command\":\"unmerge-template-table-cells\""),
             std::string::npos);
    CHECK_NE(unmerge_json.find("\"part\":\"section-footer\""),
             std::string::npos);
    CHECK_NE(unmerge_json.find("\"section\":0"), std::string::npos);
    CHECK_NE(unmerge_json.find("\"kind\":\"first\""), std::string::npos);
    CHECK_NE(unmerge_json.find("\"direction\":\"down\""), std::string::npos);

    featherdoc::Document reopened_first(first_unmerged);
    REQUIRE_FALSE(reopened_first.open());
    auto first_footer_template = reopened_first.section_footer_template(
        0U, featherdoc::section_reference_kind::first_page);
    REQUIRE(static_cast<bool>(first_footer_template));
    const auto first_top_cell =
        first_footer_template.inspect_table_cell(0U, 0U, 0U);
    REQUIRE(first_top_cell.has_value());
    CHECK_EQ(first_top_cell->text, "ff00");
    const auto first_middle_cell =
        first_footer_template.inspect_table_cell(0U, 1U, 0U);
    REQUIRE(first_middle_cell.has_value());
    CHECK_EQ(first_middle_cell->text, "");
    const auto first_bottom_cell =
        first_footer_template.inspect_table_cell(0U, 2U, 0U);
    REQUIRE(first_bottom_cell.has_value());
    CHECK_EQ(first_bottom_cell->text, "");

    remove_if_exists(source);
    remove_if_exists(even_merged);
    remove_if_exists(first_unmerged);
    remove_if_exists(merge_output);
    remove_if_exists(unmerge_output);
}

TEST_CASE("cli template table row commands mutate header template tables") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_template_table_row_source.docx";
    const fs::path appended =
        working_directory / "cli_template_table_row_appended.docx";
    const fs::path inserted_before =
        working_directory / "cli_template_table_row_before.docx";
    const fs::path inserted_after =
        working_directory / "cli_template_table_row_after.docx";
    const fs::path removed =
        working_directory / "cli_template_table_row_removed.docx";
    const fs::path append_output =
        working_directory / "cli_template_table_row_append.json";
    const fs::path before_output =
        working_directory / "cli_template_table_row_before.json";
    const fs::path after_output =
        working_directory / "cli_template_table_row_after.json";
    const fs::path remove_output =
        working_directory / "cli_template_table_row_remove.json";

    remove_if_exists(source);
    remove_if_exists(appended);
    remove_if_exists(inserted_before);
    remove_if_exists(inserted_after);
    remove_if_exists(removed);
    remove_if_exists(append_output);
    remove_if_exists(before_output);
    remove_if_exists(after_output);
    remove_if_exists(remove_output);

    create_cli_template_inspection_fixture(source);

    CHECK_EQ(run_cli({"append-template-table-row",
                      source.string(),
                      "0",
                      "--part",
                      "header",
                      "--index",
                      "0",
                      "--output",
                      appended.string(),
                      "--json"},
                     append_output),
             0);
    const auto append_json = read_text_file(append_output);
    CHECK_NE(append_json.find("\"command\":\"append-template-table-row\""),
             std::string::npos);
    CHECK_NE(append_json.find("\"part\":\"header\""), std::string::npos);
    CHECK_NE(append_json.find("\"part_index\":0"), std::string::npos);
    CHECK_NE(append_json.find("\"entry_name\":\"word/header1.xml\""),
             std::string::npos);
    CHECK_NE(append_json.find("\"table_index\":0"), std::string::npos);
    CHECK_NE(append_json.find("\"row_index\":2"), std::string::npos);
    CHECK_NE(append_json.find("\"cell_count\":2"), std::string::npos);

    featherdoc::Document reopened_appended(appended);
    REQUIRE_FALSE(reopened_appended.open());
    auto appended_header = reopened_appended.header_template(0U);
    REQUIRE(static_cast<bool>(appended_header));
    const auto appended_table = appended_header.inspect_table(0U);
    REQUIRE(appended_table.has_value());
    CHECK_EQ(appended_table->row_count, 3U);
    CHECK_EQ(appended_table->column_count, 2U);
    const auto appended_first_cell = appended_header.inspect_table_cell(0U, 2U, 0U);
    REQUIRE(appended_first_cell.has_value());
    CHECK_EQ(appended_first_cell->text, "");
    const auto appended_second_cell = appended_header.inspect_table_cell(0U, 2U, 1U);
    REQUIRE(appended_second_cell.has_value());
    CHECK_EQ(appended_second_cell->text, "");

    CHECK_EQ(run_cli({"insert-template-table-row-before",
                      source.string(),
                      "0",
                      "1",
                      "--part",
                      "header",
                      "--index",
                      "0",
                      "--output",
                      inserted_before.string(),
                      "--json"},
                     before_output),
             0);
    const auto before_json = read_text_file(before_output);
    CHECK_NE(before_json.find("\"command\":\"insert-template-table-row-before\""),
             std::string::npos);
    CHECK_NE(before_json.find("\"inserted_row_index\":1"), std::string::npos);

    featherdoc::Document reopened_before(inserted_before);
    REQUIRE_FALSE(reopened_before.open());
    auto before_header = reopened_before.header_template(0U);
    REQUIRE(static_cast<bool>(before_header));
    const auto before_table = before_header.inspect_table(0U);
    REQUIRE(before_table.has_value());
    CHECK_EQ(before_table->row_count, 3U);
    const auto before_inserted_first_cell =
        before_header.inspect_table_cell(0U, 1U, 0U);
    REQUIRE(before_inserted_first_cell.has_value());
    CHECK_EQ(before_inserted_first_cell->text, "");
    const auto before_inserted_second_cell =
        before_header.inspect_table_cell(0U, 1U, 1U);
    REQUIRE(before_inserted_second_cell.has_value());
    CHECK_EQ(before_inserted_second_cell->text, "");
    const auto before_shifted_first_cell =
        before_header.inspect_table_cell(0U, 2U, 0U);
    REQUIRE(before_shifted_first_cell.has_value());
    CHECK_EQ(before_shifted_first_cell->text, "h10");

    CHECK_EQ(run_cli({"insert-template-table-row-after",
                      source.string(),
                      "0",
                      "0",
                      "--part",
                      "header",
                      "--index",
                      "0",
                      "--output",
                      inserted_after.string(),
                      "--json"},
                     after_output),
             0);
    const auto after_json = read_text_file(after_output);
    CHECK_NE(after_json.find("\"command\":\"insert-template-table-row-after\""),
             std::string::npos);
    CHECK_NE(after_json.find("\"inserted_row_index\":1"), std::string::npos);

    featherdoc::Document reopened_after(inserted_after);
    REQUIRE_FALSE(reopened_after.open());
    auto after_header = reopened_after.header_template(0U);
    REQUIRE(static_cast<bool>(after_header));
    const auto after_table = after_header.inspect_table(0U);
    REQUIRE(after_table.has_value());
    CHECK_EQ(after_table->row_count, 3U);
    const auto after_inserted_first_cell =
        after_header.inspect_table_cell(0U, 1U, 0U);
    REQUIRE(after_inserted_first_cell.has_value());
    CHECK_EQ(after_inserted_first_cell->text, "");
    const auto after_inserted_second_cell =
        after_header.inspect_table_cell(0U, 1U, 1U);
    REQUIRE(after_inserted_second_cell.has_value());
    CHECK_EQ(after_inserted_second_cell->text, "");
    const auto after_shifted_first_cell =
        after_header.inspect_table_cell(0U, 2U, 0U);
    REQUIRE(after_shifted_first_cell.has_value());
    CHECK_EQ(after_shifted_first_cell->text, "h10");

    CHECK_EQ(run_cli({"remove-template-table-row",
                      source.string(),
                      "0",
                      "0",
                      "--part",
                      "header",
                      "--index",
                      "0",
                      "--output",
                      removed.string(),
                      "--json"},
                     remove_output),
             0);
    const auto remove_json = read_text_file(remove_output);
    CHECK_NE(remove_json.find("\"command\":\"remove-template-table-row\""),
             std::string::npos);
    CHECK_NE(remove_json.find("\"table_index\":0"), std::string::npos);
    CHECK_NE(remove_json.find("\"row_index\":0"), std::string::npos);

    featherdoc::Document reopened_removed(removed);
    REQUIRE_FALSE(reopened_removed.open());
    auto removed_header = reopened_removed.header_template(0U);
    REQUIRE(static_cast<bool>(removed_header));
    const auto removed_table = removed_header.inspect_table(0U);
    REQUIRE(removed_table.has_value());
    CHECK_EQ(removed_table->row_count, 1U);
    CHECK_EQ(removed_table->column_count, 2U);
    const auto removed_first_cell = removed_header.inspect_table_cell(0U, 0U, 0U);
    REQUIRE(removed_first_cell.has_value());
    CHECK_EQ(removed_first_cell->text, "h10");
    const auto removed_second_cell = removed_header.inspect_table_cell(0U, 0U, 1U);
    REQUIRE(removed_second_cell.has_value());
    CHECK_EQ(removed_second_cell->text, "h11");

    remove_if_exists(source);
    remove_if_exists(appended);
    remove_if_exists(inserted_before);
    remove_if_exists(inserted_after);
    remove_if_exists(removed);
    remove_if_exists(append_output);
    remove_if_exists(before_output);
    remove_if_exists(after_output);
    remove_if_exists(remove_output);
}

TEST_CASE("cli template table row commands mutate direct footer template tables") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_direct_footer_template_table_row_source.docx";
    const fs::path appended =
        working_directory / "cli_direct_footer_template_table_row_appended.docx";
    const fs::path inserted_before =
        working_directory / "cli_direct_footer_template_table_row_before.docx";
    const fs::path inserted_after =
        working_directory / "cli_direct_footer_template_table_row_after.docx";
    const fs::path removed =
        working_directory / "cli_direct_footer_template_table_row_removed.docx";
    const fs::path append_output =
        working_directory / "cli_direct_footer_template_table_row_append.json";
    const fs::path before_output =
        working_directory / "cli_direct_footer_template_table_row_before.json";
    const fs::path after_output =
        working_directory / "cli_direct_footer_template_table_row_after.json";
    const fs::path remove_output =
        working_directory / "cli_direct_footer_template_table_row_remove.json";

    remove_if_exists(source);
    remove_if_exists(appended);
    remove_if_exists(inserted_before);
    remove_if_exists(inserted_after);
    remove_if_exists(removed);
    remove_if_exists(append_output);
    remove_if_exists(before_output);
    remove_if_exists(after_output);
    remove_if_exists(remove_output);

    create_cli_template_table_direct_column_fixture(source);

    CHECK_EQ(run_cli({"append-template-table-row",
                      source.string(),
                      "0",
                      "--part",
                      "footer",
                      "--index",
                      "0",
                      "--output",
                      appended.string(),
                      "--json"},
                     append_output),
             0);
    const auto append_json = read_text_file(append_output);
    CHECK_NE(append_json.find("\"command\":\"append-template-table-row\""),
             std::string::npos);
    CHECK_NE(append_json.find("\"part\":\"footer\""), std::string::npos);
    CHECK_NE(append_json.find("\"part_index\":0"), std::string::npos);
    CHECK_NE(append_json.find("\"entry_name\":\"word/footer1.xml\""),
             std::string::npos);
    CHECK_NE(append_json.find("\"table_index\":0"), std::string::npos);
    CHECK_NE(append_json.find("\"row_index\":2"), std::string::npos);
    CHECK_NE(append_json.find("\"cell_count\":2"), std::string::npos);

    featherdoc::Document reopened_appended(appended);
    REQUIRE_FALSE(reopened_appended.open());
    auto appended_footer = reopened_appended.footer_template(0U);
    REQUIRE(static_cast<bool>(appended_footer));
    const auto appended_table = appended_footer.inspect_table(0U);
    REQUIRE(appended_table.has_value());
    CHECK_EQ(appended_table->row_count, 3U);
    CHECK_EQ(appended_table->column_count, 2U);
    const auto appended_first_cell = appended_footer.inspect_table_cell(0U, 2U, 0U);
    REQUIRE(appended_first_cell.has_value());
    CHECK_EQ(appended_first_cell->text, "");
    const auto appended_second_cell = appended_footer.inspect_table_cell(0U, 2U, 1U);
    REQUIRE(appended_second_cell.has_value());
    CHECK_EQ(appended_second_cell->text, "");

    CHECK_EQ(run_cli({"insert-template-table-row-before",
                      source.string(),
                      "0",
                      "1",
                      "--part",
                      "footer",
                      "--index",
                      "0",
                      "--output",
                      inserted_before.string(),
                      "--json"},
                     before_output),
             0);
    const auto before_json = read_text_file(before_output);
    CHECK_NE(before_json.find("\"command\":\"insert-template-table-row-before\""),
             std::string::npos);
    CHECK_NE(before_json.find("\"inserted_row_index\":1"), std::string::npos);

    featherdoc::Document reopened_before(inserted_before);
    REQUIRE_FALSE(reopened_before.open());
    auto before_footer = reopened_before.footer_template(0U);
    REQUIRE(static_cast<bool>(before_footer));
    const auto before_table = before_footer.inspect_table(0U);
    REQUIRE(before_table.has_value());
    CHECK_EQ(before_table->row_count, 3U);
    const auto before_inserted_first_cell =
        before_footer.inspect_table_cell(0U, 1U, 0U);
    REQUIRE(before_inserted_first_cell.has_value());
    CHECK_EQ(before_inserted_first_cell->text, "");
    const auto before_inserted_second_cell =
        before_footer.inspect_table_cell(0U, 1U, 1U);
    REQUIRE(before_inserted_second_cell.has_value());
    CHECK_EQ(before_inserted_second_cell->text, "");
    const auto before_shifted_first_cell =
        before_footer.inspect_table_cell(0U, 2U, 0U);
    REQUIRE(before_shifted_first_cell.has_value());
    CHECK_EQ(before_shifted_first_cell->text, "f10");

    CHECK_EQ(run_cli({"insert-template-table-row-after",
                      source.string(),
                      "0",
                      "0",
                      "--part",
                      "footer",
                      "--index",
                      "0",
                      "--output",
                      inserted_after.string(),
                      "--json"},
                     after_output),
             0);
    const auto after_json = read_text_file(after_output);
    CHECK_NE(after_json.find("\"command\":\"insert-template-table-row-after\""),
             std::string::npos);
    CHECK_NE(after_json.find("\"inserted_row_index\":1"), std::string::npos);

    featherdoc::Document reopened_after(inserted_after);
    REQUIRE_FALSE(reopened_after.open());
    auto after_footer = reopened_after.footer_template(0U);
    REQUIRE(static_cast<bool>(after_footer));
    const auto after_table = after_footer.inspect_table(0U);
    REQUIRE(after_table.has_value());
    CHECK_EQ(after_table->row_count, 3U);
    const auto after_inserted_first_cell =
        after_footer.inspect_table_cell(0U, 1U, 0U);
    REQUIRE(after_inserted_first_cell.has_value());
    CHECK_EQ(after_inserted_first_cell->text, "");
    const auto after_inserted_second_cell =
        after_footer.inspect_table_cell(0U, 1U, 1U);
    REQUIRE(after_inserted_second_cell.has_value());
    CHECK_EQ(after_inserted_second_cell->text, "");
    const auto after_shifted_first_cell =
        after_footer.inspect_table_cell(0U, 2U, 0U);
    REQUIRE(after_shifted_first_cell.has_value());
    CHECK_EQ(after_shifted_first_cell->text, "f10");

    CHECK_EQ(run_cli({"remove-template-table-row",
                      source.string(),
                      "0",
                      "0",
                      "--part",
                      "footer",
                      "--index",
                      "0",
                      "--output",
                      removed.string(),
                      "--json"},
                     remove_output),
             0);
    const auto remove_json = read_text_file(remove_output);
    CHECK_NE(remove_json.find("\"command\":\"remove-template-table-row\""),
             std::string::npos);
    CHECK_NE(remove_json.find("\"part\":\"footer\""), std::string::npos);
    CHECK_NE(remove_json.find("\"part_index\":0"), std::string::npos);
    CHECK_NE(remove_json.find("\"table_index\":0"), std::string::npos);
    CHECK_NE(remove_json.find("\"row_index\":0"), std::string::npos);

    featherdoc::Document reopened_removed(removed);
    REQUIRE_FALSE(reopened_removed.open());
    auto removed_footer = reopened_removed.footer_template(0U);
    REQUIRE(static_cast<bool>(removed_footer));
    const auto removed_table = removed_footer.inspect_table(0U);
    REQUIRE(removed_table.has_value());
    CHECK_EQ(removed_table->row_count, 1U);
    CHECK_EQ(removed_table->column_count, 2U);
    const auto removed_first_cell = removed_footer.inspect_table_cell(0U, 0U, 0U);
    REQUIRE(removed_first_cell.has_value());
    CHECK_EQ(removed_first_cell->text, "f10");
    const auto removed_second_cell = removed_footer.inspect_table_cell(0U, 0U, 1U);
    REQUIRE(removed_second_cell.has_value());
    CHECK_EQ(removed_second_cell->text, "f11");

    remove_if_exists(source);
    remove_if_exists(appended);
    remove_if_exists(inserted_before);
    remove_if_exists(inserted_after);
    remove_if_exists(removed);
    remove_if_exists(append_output);
    remove_if_exists(before_output);
    remove_if_exists(after_output);
    remove_if_exists(remove_output);
}

TEST_CASE("cli template table row commands report parse and mutate errors") {
    const fs::path working_directory = fs::current_path();
    const fs::path parse_source =
        working_directory / "cli_template_table_row_parse_source.docx";
    const fs::path merged_source =
        working_directory / "cli_template_table_row_merge_source.docx";
    const fs::path single_row_source =
        working_directory / "cli_template_table_row_single_source.docx";
    const fs::path parse_output =
        working_directory / "cli_template_table_row_parse.json";
    const fs::path merge_error_output =
        working_directory / "cli_template_table_row_merge_error.json";
    const fs::path remove_error_output =
        working_directory / "cli_template_table_row_remove_error.json";

    remove_if_exists(parse_source);
    remove_if_exists(merged_source);
    remove_if_exists(single_row_source);
    remove_if_exists(parse_output);
    remove_if_exists(merge_error_output);
    remove_if_exists(remove_error_output);

    create_cli_template_inspection_fixture(parse_source);

    CHECK_EQ(run_cli({"append-template-table-row",
                      parse_source.string(),
                      "0",
                      "--part",
                      "section-header",
                      "--json"},
                     parse_output),
             2);
    CHECK_EQ(
        read_text_file(parse_output),
        std::string{
            "{\"command\":\"append-template-table-row\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"section-header/section-footer "
            "mutation requires --section <index>\"}\n"});

    featherdoc::Document merged_document(merged_source);
    REQUIRE_FALSE(merged_document.create_empty());
    auto &merged_header = merged_document.ensure_header_paragraphs();
    REQUIRE(merged_header.has_next());
    auto merged_template = merged_document.header_template(0U);
    REQUIRE(static_cast<bool>(merged_template));
    auto merged_table = merged_template.append_table(3U, 1U);
    REQUIRE(merged_table.has_next());
    auto merged_row = merged_table.rows();
    REQUIRE(merged_row.has_next());
    auto merged_cell = merged_row.cells();
    REQUIRE(merged_cell.has_next());
    REQUIRE(merged_cell.set_text("merged-root"));
    REQUIRE(merged_cell.merge_down(2U));
    REQUIRE_FALSE(merged_document.save());

    CHECK_EQ(run_cli({"insert-template-table-row-after",
                      merged_source.string(),
                      "0",
                      "0",
                      "--part",
                      "header",
                      "--index",
                      "0",
                      "--json"},
                     merge_error_output),
             1);
    const auto merge_error_json = read_text_file(merge_error_output);
    CHECK_NE(merge_error_json.find("\"command\":\"insert-template-table-row-after\""),
             std::string::npos);
    CHECK_NE(merge_error_json.find("\"stage\":\"mutate\""), std::string::npos);
    CHECK_NE(merge_error_json.find("\"entry\":\"word/header1.xml\""),
             std::string::npos);
    CHECK_NE(
        merge_error_json.find(
            "\"detail\":\"cannot insert a row adjacent to row index '0' in "
            "table index '0' because the row participates in a vertical "
            "merge\""),
        std::string::npos);

    featherdoc::Document single_row_document(single_row_source);
    REQUIRE_FALSE(single_row_document.create_empty());
    auto &single_header = single_row_document.ensure_header_paragraphs();
    REQUIRE(single_header.has_next());
    auto single_template = single_row_document.header_template(0U);
    REQUIRE(static_cast<bool>(single_template));
    auto single_table = single_template.append_table(1U, 1U);
    REQUIRE(single_table.has_next());
    auto single_row = single_table.rows();
    REQUIRE(single_row.has_next());
    auto only_cell = single_row.cells();
    REQUIRE(only_cell.has_next());
    REQUIRE(only_cell.set_text("only-row"));
    REQUIRE_FALSE(single_row_document.save());

    CHECK_EQ(run_cli({"remove-template-table-row",
                      single_row_source.string(),
                      "0",
                      "0",
                      "--part",
                      "header",
                      "--index",
                      "0",
                      "--json"},
                     remove_error_output),
             1);
    const auto remove_error_json = read_text_file(remove_error_output);
    CHECK_NE(remove_error_json.find("\"command\":\"remove-template-table-row\""),
             std::string::npos);
    CHECK_NE(remove_error_json.find("\"stage\":\"mutate\""), std::string::npos);
    CHECK_NE(remove_error_json.find("\"entry\":\"word/header1.xml\""),
             std::string::npos);
    CHECK_NE(
        remove_error_json.find(
            "\"detail\":\"cannot remove the last row from table index '0'\""),
        std::string::npos);

    remove_if_exists(parse_source);
    remove_if_exists(merged_source);
    remove_if_exists(single_row_source);
    remove_if_exists(parse_output);
    remove_if_exists(merge_error_output);
    remove_if_exists(remove_error_output);
}

TEST_CASE("cli template table commands support bookmark table selectors") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_template_table_bookmark_source.docx";
    const fs::path inspect_output =
        working_directory / "cli_template_table_bookmark_inspect.json";
    const fs::path updated =
        working_directory / "cli_template_table_bookmark_updated.docx";
    const fs::path updated_output =
        working_directory / "cli_template_table_bookmark_updated.json";
    const fs::path rows_updated =
        working_directory / "cli_template_table_bookmark_rows_updated.docx";
    const fs::path rows_output =
        working_directory / "cli_template_table_bookmark_rows_updated.json";
    const fs::path block_updated =
        working_directory / "cli_template_table_bookmark_block_updated.docx";
    const fs::path block_output =
        working_directory / "cli_template_table_bookmark_block_updated.json";
    const fs::path appended =
        working_directory / "cli_template_table_bookmark_appended.docx";
    const fs::path append_output =
        working_directory / "cli_template_table_bookmark_appended.json";
    const fs::path inserted =
        working_directory / "cli_template_table_bookmark_inserted.docx";
    const fs::path insert_output =
        working_directory / "cli_template_table_bookmark_inserted.json";
    const fs::path range_error_output =
        working_directory / "cli_template_table_bookmark_row_range_error.json";
    const fs::path cell_error_output =
        working_directory / "cli_template_table_bookmark_row_cells_error.json";
    const fs::path block_error_output =
        working_directory / "cli_template_table_bookmark_block_error.json";
    const fs::path parse_output =
        working_directory / "cli_template_table_bookmark_parse.json";
    const fs::path row_parse_output =
        working_directory / "cli_template_table_bookmark_row_parse.json";
    const fs::path block_parse_output =
        working_directory / "cli_template_table_bookmark_block_parse.json";

    remove_if_exists(source);
    remove_if_exists(inspect_output);
    remove_if_exists(updated);
    remove_if_exists(updated_output);
    remove_if_exists(rows_updated);
    remove_if_exists(rows_output);
    remove_if_exists(block_updated);
    remove_if_exists(block_output);
    remove_if_exists(appended);
    remove_if_exists(append_output);
    remove_if_exists(inserted);
    remove_if_exists(insert_output);
    remove_if_exists(range_error_output);
    remove_if_exists(cell_error_output);
    remove_if_exists(block_error_output);
    remove_if_exists(parse_output);
    remove_if_exists(row_parse_output);
    remove_if_exists(block_parse_output);

    create_cli_template_table_bookmark_fixture(source);

    CHECK_EQ(run_cli({"inspect-template-table-rows",
                      source.string(),
                      "--bookmark",
                      "target_before_table",
                      "--json"},
                     inspect_output),
             0);
    const auto inspect_json = read_text_file(inspect_output);
    CHECK_NE(inspect_json.find("\"part\":\"body\""), std::string::npos);
    CHECK_NE(inspect_json.find("\"entry_name\":\"word/document.xml\""),
             std::string::npos);
    CHECK_NE(inspect_json.find("\"table_index\":1"), std::string::npos);
    CHECK_NE(inspect_json.find("\"count\":2"), std::string::npos);
    CHECK_NE(inspect_json.find("\"cell_texts\":[\"target-00\",\"target-01\"]"),
             std::string::npos);
    CHECK_NE(inspect_json.find("\"cell_texts\":[\"target-10\",\"target-11\"]"),
             std::string::npos);

    CHECK_EQ(run_cli({"set-template-table-cell-text",
                      source.string(),
                      "--bookmark",
                      "target_before_table",
                      "1",
                      "1",
                      "--text",
                      "updated",
                      "--output",
                      updated.string(),
                      "--json"},
                     updated_output),
             0);
    const auto updated_json = read_text_file(updated_output);
    CHECK_NE(updated_json.find("\"command\":\"set-template-table-cell-text\""),
             std::string::npos);
    CHECK_NE(updated_json.find("\"part\":\"body\""), std::string::npos);
    CHECK_NE(updated_json.find("\"table_index\":1"), std::string::npos);
    CHECK_NE(updated_json.find("\"row_index\":1"), std::string::npos);
    CHECK_NE(updated_json.find("\"cell_index\":1"), std::string::npos);

    featherdoc::Document reopened_updated(updated);
    REQUIRE_FALSE(reopened_updated.open());
    auto updated_body = reopened_updated.body_template();
    REQUIRE(static_cast<bool>(updated_body));
    const auto preserved_cell = updated_body.inspect_table_cell(0U, 0U, 0U);
    REQUIRE(preserved_cell.has_value());
    CHECK_EQ(preserved_cell->text, "keep-00");
    const auto updated_cell = updated_body.inspect_table_cell(1U, 1U, 1U);
    REQUIRE(updated_cell.has_value());
    CHECK_EQ(updated_cell->text, "updated");

    CHECK_EQ(run_cli({"set-template-table-row-texts",
                      source.string(),
                      "--bookmark",
                      "target_before_table",
                      "0",
                      "--row",
                      "batch-00",
                      "--cell",
                      "batch-01",
                      "--row",
                      "batch-10",
                      "--cell",
                      "batch-11",
                      "--output",
                      rows_updated.string(),
                      "--json"},
                     rows_output),
             0);
    const auto rows_json = read_text_file(rows_output);
    CHECK_NE(rows_json.find("\"command\":\"set-template-table-row-texts\""),
             std::string::npos);
    CHECK_NE(rows_json.find("\"part\":\"body\""), std::string::npos);
    CHECK_NE(rows_json.find("\"bookmark_name\":\"target_before_table\""),
             std::string::npos);
    CHECK_NE(rows_json.find("\"table_index\":1"), std::string::npos);
    CHECK_NE(rows_json.find("\"start_row_index\":0"), std::string::npos);
    CHECK_NE(rows_json.find("\"row_count\":2"), std::string::npos);
    CHECK_NE(rows_json.find("\"rows\":[[\"batch-00\",\"batch-01\"],[\"batch-10\",\"batch-11\"]]"),
             std::string::npos);

    featherdoc::Document reopened_rows(rows_updated);
    REQUIRE_FALSE(reopened_rows.open());
    auto rows_body = reopened_rows.body_template();
    REQUIRE(static_cast<bool>(rows_body));
    const auto preserved_batch_cell = rows_body.inspect_table_cell(0U, 0U, 0U);
    REQUIRE(preserved_batch_cell.has_value());
    CHECK_EQ(preserved_batch_cell->text, "keep-00");
    const auto batch_first_cell = rows_body.inspect_table_cell(1U, 0U, 0U);
    REQUIRE(batch_first_cell.has_value());
    CHECK_EQ(batch_first_cell->text, "batch-00");
    const auto batch_second_cell = rows_body.inspect_table_cell(1U, 0U, 1U);
    REQUIRE(batch_second_cell.has_value());
    CHECK_EQ(batch_second_cell->text, "batch-01");
    const auto batch_third_cell = rows_body.inspect_table_cell(1U, 1U, 0U);
    REQUIRE(batch_third_cell.has_value());
    CHECK_EQ(batch_third_cell->text, "batch-10");
    const auto batch_fourth_cell = rows_body.inspect_table_cell(1U, 1U, 1U);
    REQUIRE(batch_fourth_cell.has_value());
    CHECK_EQ(batch_fourth_cell->text, "batch-11");

    CHECK_EQ(run_cli({"set-template-table-row-texts",
                      source.string(),
                      "--bookmark",
                      "target_inside_table",
                      "1",
                      "--row",
                      "overflow-10",
                      "--cell",
                      "overflow-11",
                      "--row",
                      "overflow-20",
                      "--cell",
                      "overflow-21",
                      "--json"},
                     range_error_output),
             1);
    const auto range_error_json = read_text_file(range_error_output);
    CHECK_NE(range_error_json.find("\"command\":\"set-template-table-row-texts\""),
             std::string::npos);
    CHECK_NE(range_error_json.find("\"stage\":\"mutate\""),
             std::string::npos);
    CHECK_NE(range_error_json.find("\"detail\":\"row range starting at row index '1' with count '2' exceeds table index '1' row count '2'\""),
             std::string::npos);

    CHECK_EQ(run_cli({"set-template-table-row-texts",
                      source.string(),
                      "--bookmark",
                      "target_before_table",
                      "0",
                      "--row",
                      "only-one-cell",
                      "--json"},
                     cell_error_output),
             1);
    const auto cell_error_json = read_text_file(cell_error_output);
    CHECK_NE(cell_error_json.find("\"command\":\"set-template-table-row-texts\""),
             std::string::npos);
    CHECK_NE(cell_error_json.find("\"stage\":\"mutate\""),
             std::string::npos);
    CHECK_NE(cell_error_json.find("\"detail\":\"replacement row at offset '0' contains '1' cells but target row index '0' contains '2' cells\""),
             std::string::npos);

    CHECK_EQ(run_cli({"set-template-table-cell-block-texts",
                      source.string(),
                      "--bookmark",
                      "target_before_table",
                      "0",
                      "1",
                      "--row",
                      "block-01",
                      "--row",
                      "block-11",
                      "--output",
                      block_updated.string(),
                      "--json"},
                     block_output),
             0);
    const auto block_json = read_text_file(block_output);
    CHECK_NE(block_json.find("\"command\":\"set-template-table-cell-block-texts\""),
             std::string::npos);
    CHECK_NE(block_json.find("\"part\":\"body\""), std::string::npos);
    CHECK_NE(block_json.find("\"bookmark_name\":\"target_before_table\""),
             std::string::npos);
    CHECK_NE(block_json.find("\"table_index\":1"), std::string::npos);
    CHECK_NE(block_json.find("\"start_row_index\":0"), std::string::npos);
    CHECK_NE(block_json.find("\"start_cell_index\":1"), std::string::npos);
    CHECK_NE(block_json.find("\"row_count\":2"), std::string::npos);
    CHECK_NE(block_json.find("\"rows\":[[\"block-01\"],[\"block-11\"]]"),
             std::string::npos);

    featherdoc::Document reopened_block(block_updated);
    REQUIRE_FALSE(reopened_block.open());
    auto block_body = reopened_block.body_template();
    REQUIRE(static_cast<bool>(block_body));
    const auto preserved_block_first = block_body.inspect_table_cell(1U, 0U, 0U);
    REQUIRE(preserved_block_first.has_value());
    CHECK_EQ(preserved_block_first->text, "target-00");
    const auto updated_block_first = block_body.inspect_table_cell(1U, 0U, 1U);
    REQUIRE(updated_block_first.has_value());
    CHECK_EQ(updated_block_first->text, "block-01");
    const auto preserved_block_second = block_body.inspect_table_cell(1U, 1U, 0U);
    REQUIRE(preserved_block_second.has_value());
    CHECK_EQ(preserved_block_second->text, "target-10");
    const auto updated_block_second = block_body.inspect_table_cell(1U, 1U, 1U);
    REQUIRE(updated_block_second.has_value());
    CHECK_EQ(updated_block_second->text, "block-11");

    CHECK_EQ(run_cli({"set-template-table-cell-block-texts",
                      source.string(),
                      "--bookmark",
                      "target_inside_table",
                      "0",
                      "1",
                      "--row",
                      "too-wide-01",
                      "--cell",
                      "too-wide-02",
                      "--json"},
                     block_error_output),
             1);
    const auto block_error_json = read_text_file(block_error_output);
    CHECK_NE(block_error_json.find("\"command\":\"set-template-table-cell-block-texts\""),
             std::string::npos);
    CHECK_NE(block_error_json.find("\"stage\":\"mutate\""),
             std::string::npos);
    CHECK_NE(block_error_json.find("\"detail\":\"replacement row at offset '0' starting at cell index '1' with count '2' exceeds target row index '0' cell count '2'\""),
             std::string::npos);

    CHECK_EQ(run_cli({"append-template-table-row",
                      source.string(),
                      "--bookmark",
                      "target_inside_table",
                      "--output",
                      appended.string(),
                      "--json"},
                     append_output),
             0);
    const auto append_json = read_text_file(append_output);
    CHECK_NE(append_json.find("\"command\":\"append-template-table-row\""),
             std::string::npos);
    CHECK_NE(append_json.find("\"part\":\"body\""), std::string::npos);
    CHECK_NE(append_json.find("\"table_index\":1"), std::string::npos);
    CHECK_NE(append_json.find("\"row_index\":2"), std::string::npos);
    CHECK_NE(append_json.find("\"cell_count\":2"), std::string::npos);

    featherdoc::Document reopened_appended(appended);
    REQUIRE_FALSE(reopened_appended.open());
    auto appended_body = reopened_appended.body_template();
    REQUIRE(static_cast<bool>(appended_body));
    const auto appended_table = appended_body.inspect_table(1U);
    REQUIRE(appended_table.has_value());
    CHECK_EQ(appended_table->row_count, 3U);
    const auto appended_first_cell = appended_body.inspect_table_cell(1U, 2U, 0U);
    REQUIRE(appended_first_cell.has_value());
    CHECK_EQ(appended_first_cell->text, "");
    const auto appended_second_cell = appended_body.inspect_table_cell(1U, 2U, 1U);
    REQUIRE(appended_second_cell.has_value());
    CHECK_EQ(appended_second_cell->text, "");

    CHECK_EQ(run_cli({"insert-template-table-row-before",
                      source.string(),
                      "--bookmark",
                      "target_before_table",
                      "1",
                      "--output",
                      inserted.string(),
                      "--json"},
                     insert_output),
             0);
    const auto insert_json = read_text_file(insert_output);
    CHECK_NE(insert_json.find("\"command\":\"insert-template-table-row-before\""),
             std::string::npos);
    CHECK_NE(insert_json.find("\"part\":\"body\""), std::string::npos);
    CHECK_NE(insert_json.find("\"table_index\":1"), std::string::npos);
    CHECK_NE(insert_json.find("\"row_index\":1"), std::string::npos);
    CHECK_NE(insert_json.find("\"inserted_row_index\":1"), std::string::npos);

    featherdoc::Document reopened_inserted(inserted);
    REQUIRE_FALSE(reopened_inserted.open());
    auto inserted_body = reopened_inserted.body_template();
    REQUIRE(static_cast<bool>(inserted_body));
    const auto inserted_table = inserted_body.inspect_table(1U);
    REQUIRE(inserted_table.has_value());
    CHECK_EQ(inserted_table->row_count, 3U);
    const auto inserted_first_cell = inserted_body.inspect_table_cell(1U, 1U, 0U);
    REQUIRE(inserted_first_cell.has_value());
    CHECK_EQ(inserted_first_cell->text, "");
    const auto inserted_second_cell = inserted_body.inspect_table_cell(1U, 1U, 1U);
    REQUIRE(inserted_second_cell.has_value());
    CHECK_EQ(inserted_second_cell->text, "");
    const auto shifted_row_cell = inserted_body.inspect_table_cell(1U, 2U, 0U);
    REQUIRE(shifted_row_cell.has_value());
    CHECK_EQ(shifted_row_cell->text, "target-10");

    CHECK_EQ(run_cli({"append-template-table-row",
                      source.string(),
                      "1",
                      "--bookmark",
                      "target_inside_table",
                      "--json"},
                     parse_output),
             2);
    CHECK_EQ(
        read_text_file(parse_output),
        std::string{
            "{\"command\":\"append-template-table-row\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"cannot combine a table index "
            "with --bookmark\"}\n"});

    CHECK_EQ(run_cli({"set-template-table-row-texts",
                      source.string(),
                      "1",
                      "0",
                      "--bookmark",
                      "target_inside_table",
                      "--row",
                      "dup-00",
                      "--cell",
                      "dup-01",
                      "--json"},
                     row_parse_output),
             2);
    CHECK_EQ(
        read_text_file(row_parse_output),
        std::string{
            "{\"command\":\"set-template-table-row-texts\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"cannot combine a table index "
            "with --bookmark\"}\n"});

    CHECK_EQ(run_cli({"set-template-table-cell-block-texts",
                      source.string(),
                      "1",
                      "0",
                      "1",
                      "--bookmark",
                      "target_inside_table",
                      "--row",
                      "dup-block-01",
                      "--json"},
                     block_parse_output),
             2);
    CHECK_EQ(
        read_text_file(block_parse_output),
        std::string{
            "{\"command\":\"set-template-table-cell-block-texts\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"cannot combine a table index "
            "with --bookmark\"}\n"});

    remove_if_exists(source);
    remove_if_exists(inspect_output);
    remove_if_exists(updated);
    remove_if_exists(updated_output);
    remove_if_exists(rows_updated);
    remove_if_exists(rows_output);
    remove_if_exists(block_updated);
    remove_if_exists(block_output);
    remove_if_exists(appended);
    remove_if_exists(append_output);
    remove_if_exists(inserted);
    remove_if_exists(insert_output);
    remove_if_exists(range_error_output);
    remove_if_exists(cell_error_output);
    remove_if_exists(block_error_output);
    remove_if_exists(parse_output);
    remove_if_exists(row_parse_output);
    remove_if_exists(block_parse_output);
}

TEST_CASE("cli set-template-table-from-json applies bookmark-targeted patches") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_template_table_json_patch_source.docx";
    const fs::path rows_patch =
        working_directory / "cli_template_table_rows_patch.json";
    const fs::path block_patch =
        working_directory / "cli_template_table_block_patch.json";
    const fs::path parse_patch =
        working_directory / "cli_template_table_parse_patch.json";
    const fs::path mutate_patch =
        working_directory / "cli_template_table_mutate_patch.json";
    const fs::path rows_updated =
        working_directory / "cli_template_table_rows_patch_updated.docx";
    const fs::path block_updated =
        working_directory / "cli_template_table_block_patch_updated.docx";
    const fs::path rows_output =
        working_directory / "cli_template_table_rows_patch_output.json";
    const fs::path block_output =
        working_directory / "cli_template_table_block_patch_output.json";
    const fs::path parse_output =
        working_directory / "cli_template_table_parse_patch_output.json";
    const fs::path mutate_output =
        working_directory / "cli_template_table_mutate_patch_output.json";
    const fs::path selector_parse_output =
        working_directory / "cli_template_table_selector_patch_output.json";

    remove_if_exists(source);
    remove_if_exists(rows_patch);
    remove_if_exists(block_patch);
    remove_if_exists(parse_patch);
    remove_if_exists(mutate_patch);
    remove_if_exists(rows_updated);
    remove_if_exists(block_updated);
    remove_if_exists(rows_output);
    remove_if_exists(block_output);
    remove_if_exists(parse_output);
    remove_if_exists(mutate_output);
    remove_if_exists(selector_parse_output);

    create_cli_template_table_bookmark_fixture(source);
    write_binary_file(
        rows_patch,
        R"({
  "mode": "rows",
  "start_row": 0,
  "rows": [
    ["json-00", 12],
    ["json-10", true]
  ]
})");
    write_binary_file(
        block_patch,
        R"({
  "command": "set-template-table-cell-block-texts",
  "start_row_index": 0,
  "start_cell_index": 1,
  "row_count": 2,
  "rows": [
    [12],
    [false]
  ]
})");
    write_binary_file(
        parse_patch,
        R"({
  "mode": "rows",
  "rows": [
    ["missing-start-row", "value"]
  ]
})");
    write_binary_file(
        mutate_patch,
        R"({
  "mode": "rows",
  "start_row": 1,
  "rows": [
    ["overflow-10", "overflow-11"],
    ["overflow-20", "overflow-21"]
  ]
})");

    CHECK_EQ(run_cli({"set-template-table-from-json",
                      source.string(),
                      "--bookmark",
                      "target_before_table",
                      "--patch-file",
                      rows_patch.string(),
                      "--output",
                      rows_updated.string(),
                      "--json"},
                     rows_output),
             0);
    const auto rows_json = read_text_file(rows_output);
    CHECK_NE(rows_json.find("\"command\":\"set-template-table-from-json\""),
             std::string::npos);
    CHECK_NE(rows_json.find("\"part\":\"body\""), std::string::npos);
    CHECK_NE(rows_json.find("\"bookmark_name\":\"target_before_table\""),
             std::string::npos);
    CHECK_NE(rows_json.find("\"mode\":\"rows\""), std::string::npos);
    CHECK_NE(rows_json.find("\"table_index\":1"), std::string::npos);
    CHECK_NE(rows_json.find("\"start_row_index\":0"), std::string::npos);
    CHECK_NE(rows_json.find("\"row_count\":2"), std::string::npos);
    CHECK_NE(rows_json.find("\"rows\":[[\"json-00\",\"12\"],[\"json-10\",\"true\"]]"),
             std::string::npos);

    featherdoc::Document reopened_rows(rows_updated);
    REQUIRE_FALSE(reopened_rows.open());
    auto rows_body = reopened_rows.body_template();
    REQUIRE(static_cast<bool>(rows_body));
    const auto preserved_rows_cell = rows_body.inspect_table_cell(0U, 0U, 0U);
    REQUIRE(preserved_rows_cell.has_value());
    CHECK_EQ(preserved_rows_cell->text, "keep-00");
    const auto json_row_first = rows_body.inspect_table_cell(1U, 0U, 0U);
    REQUIRE(json_row_first.has_value());
    CHECK_EQ(json_row_first->text, "json-00");
    const auto json_row_second = rows_body.inspect_table_cell(1U, 0U, 1U);
    REQUIRE(json_row_second.has_value());
    CHECK_EQ(json_row_second->text, "12");
    const auto json_row_third = rows_body.inspect_table_cell(1U, 1U, 0U);
    REQUIRE(json_row_third.has_value());
    CHECK_EQ(json_row_third->text, "json-10");
    const auto json_row_fourth = rows_body.inspect_table_cell(1U, 1U, 1U);
    REQUIRE(json_row_fourth.has_value());
    CHECK_EQ(json_row_fourth->text, "true");

    CHECK_EQ(run_cli({"set-template-table-from-json",
                      source.string(),
                      "--bookmark",
                      "target_inside_table",
                      "--patch-file",
                      block_patch.string(),
                      "--output",
                      block_updated.string(),
                      "--json"},
                     block_output),
             0);
    const auto block_json = read_text_file(block_output);
    CHECK_NE(block_json.find("\"command\":\"set-template-table-from-json\""),
             std::string::npos);
    CHECK_NE(block_json.find("\"bookmark_name\":\"target_inside_table\""),
             std::string::npos);
    CHECK_NE(block_json.find("\"mode\":\"block\""), std::string::npos);
    CHECK_NE(block_json.find("\"table_index\":1"), std::string::npos);
    CHECK_NE(block_json.find("\"start_row_index\":0"), std::string::npos);
    CHECK_NE(block_json.find("\"start_cell_index\":1"), std::string::npos);
    CHECK_NE(block_json.find("\"row_count\":2"), std::string::npos);
    CHECK_NE(block_json.find("\"rows\":[[\"12\"],[\"false\"]]"),
             std::string::npos);

    featherdoc::Document reopened_block(block_updated);
    REQUIRE_FALSE(reopened_block.open());
    auto block_body = reopened_block.body_template();
    REQUIRE(static_cast<bool>(block_body));
    const auto preserved_block_first = block_body.inspect_table_cell(1U, 0U, 0U);
    REQUIRE(preserved_block_first.has_value());
    CHECK_EQ(preserved_block_first->text, "target-00");
    const auto updated_block_first = block_body.inspect_table_cell(1U, 0U, 1U);
    REQUIRE(updated_block_first.has_value());
    CHECK_EQ(updated_block_first->text, "12");
    const auto preserved_block_second = block_body.inspect_table_cell(1U, 1U, 0U);
    REQUIRE(preserved_block_second.has_value());
    CHECK_EQ(preserved_block_second->text, "target-10");
    const auto updated_block_second = block_body.inspect_table_cell(1U, 1U, 1U);
    REQUIRE(updated_block_second.has_value());
    CHECK_EQ(updated_block_second->text, "false");

    CHECK_EQ(run_cli({"set-template-table-from-json",
                      source.string(),
                      "--bookmark",
                      "target_before_table",
                      "--patch-file",
                      parse_patch.string(),
                      "--json"},
                     parse_output),
             2);
    CHECK_EQ(
        read_text_file(parse_output),
        std::string{
            "{\"command\":\"set-template-table-from-json\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"JSON patch must contain "
            "'start_row' or 'start_row_index'\"}\n"});

    CHECK_EQ(run_cli({"set-template-table-from-json",
                      source.string(),
                      "--bookmark",
                      "target_inside_table",
                      "--patch-file",
                      mutate_patch.string(),
                      "--json"},
                     mutate_output),
             1);
    const auto mutate_json = read_text_file(mutate_output);
    CHECK_NE(mutate_json.find("\"command\":\"set-template-table-from-json\""),
             std::string::npos);
    CHECK_NE(mutate_json.find("\"stage\":\"mutate\""), std::string::npos);
    CHECK_NE(mutate_json.find("\"detail\":\"row range starting at row index '1' with count '2' exceeds table index '1' row count '2'\""),
             std::string::npos);

    CHECK_EQ(run_cli({"set-template-table-from-json",
                      source.string(),
                      "1",
                      "--bookmark",
                      "target_inside_table",
                      "--patch-file",
                      rows_patch.string(),
                      "--json"},
                     selector_parse_output),
             2);
    CHECK_EQ(
        read_text_file(selector_parse_output),
        std::string{
            "{\"command\":\"set-template-table-from-json\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"cannot combine a table index "
            "with --bookmark\"}\n"});

    remove_if_exists(source);
    remove_if_exists(rows_patch);
    remove_if_exists(block_patch);
    remove_if_exists(parse_patch);
    remove_if_exists(mutate_patch);
    remove_if_exists(rows_updated);
    remove_if_exists(block_updated);
    remove_if_exists(rows_output);
    remove_if_exists(block_output);
    remove_if_exists(parse_output);
    remove_if_exists(mutate_output);
    remove_if_exists(selector_parse_output);
}

TEST_CASE("cli template table selectors support after-text, header cells, and occurrence") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_template_table_selector_source.docx";
    const fs::path after_output =
        working_directory / "cli_template_table_selector_after.json";
    const fs::path header_output =
        working_directory / "cli_template_table_selector_header.json";
    const fs::path combined_output =
        working_directory / "cli_template_table_selector_combined.json";
    const fs::path parse_output =
        working_directory / "cli_template_table_selector_parse.json";

    remove_if_exists(source);
    remove_if_exists(after_output);
    remove_if_exists(header_output);
    remove_if_exists(combined_output);
    remove_if_exists(parse_output);

    create_cli_template_table_selector_fixture(source);

    CHECK_EQ(run_cli({"inspect-template-table-rows",
                      source.string(),
                      "--after-text",
                      "selector target table",
                      "--json"},
                     after_output),
             0);
    const auto after_json = read_text_file(after_output);
    CHECK_NE(after_json.find("\"table_index\":0"), std::string::npos);
    CHECK_NE(after_json.find("\"cell_texts\":[\"Region\",\"Qty\",\"Amount\"]"),
             std::string::npos);
    CHECK_NE(after_json.find("\"cell_texts\":[\"North\",\"7\",\"12\"]"),
             std::string::npos);

    CHECK_EQ(run_cli({"inspect-template-table-rows",
                      source.string(),
                      "--header-cell",
                      "Region",
                      "--header-cell",
                      "Qty",
                      "--header-cell",
                      "Amount",
                      "--occurrence",
                      "1",
                      "--json"},
                     header_output),
             0);
    const auto header_json = read_text_file(header_output);
    CHECK_NE(header_json.find("\"table_index\":2"), std::string::npos);
    CHECK_NE(header_json.find("\"cell_texts\":[\"South\",\"9\",\"24\"]"),
             std::string::npos);

    CHECK_EQ(run_cli({"inspect-template-table-rows",
                      source.string(),
                      "--after-text",
                      "selector target table",
                      "--header-cell",
                      "Region",
                      "--header-cell",
                      "Qty",
                      "--header-cell",
                      "Amount",
                      "--occurrence",
                      "1",
                      "--json"},
                     combined_output),
             0);
    const auto combined_json = read_text_file(combined_output);
    CHECK_NE(combined_json.find("\"table_index\":2"), std::string::npos);
    CHECK_NE(combined_json.find("\"cell_texts\":[\"South\",\"9\",\"24\"]"),
             std::string::npos);

    CHECK_EQ(run_cli({"inspect-template-table-rows",
                      source.string(),
                      "0",
                      "--after-text",
                      "selector target table",
                      "--json"},
                     parse_output),
             2);
    CHECK_EQ(
        read_text_file(parse_output),
        std::string{
            "{\"command\":\"inspect-template-table-rows\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"cannot combine a table index or "
            "--bookmark with --after-text or --header-cell\"}\n"});

    remove_if_exists(source);
    remove_if_exists(after_output);
    remove_if_exists(header_output);
    remove_if_exists(combined_output);
    remove_if_exists(parse_output);
}

TEST_CASE("cli set-template-table-from-json supports selector-based targeting") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_template_table_selector_patch_source.docx";
    const fs::path patch_path =
        working_directory / "cli_template_table_selector_patch.json";
    const fs::path updated =
        working_directory / "cli_template_table_selector_patch_updated.docx";
    const fs::path output =
        working_directory / "cli_template_table_selector_patch_output.json";
    const fs::path parse_output =
        working_directory / "cli_template_table_selector_patch_parse.json";

    remove_if_exists(source);
    remove_if_exists(patch_path);
    remove_if_exists(updated);
    remove_if_exists(output);
    remove_if_exists(parse_output);

    create_cli_template_table_selector_fixture(source);
    write_binary_file(
        patch_path,
        R"({
  "mode": "rows",
  "start_row": 1,
  "rows": [
    ["South-updated", 18, 199]
  ]
})");

    CHECK_EQ(run_cli({"set-template-table-from-json",
                      source.string(),
                      "--after-text",
                      "selector target table",
                      "--header-cell",
                      "Region",
                      "--header-cell",
                      "Qty",
                      "--header-cell",
                      "Amount",
                      "--occurrence",
                      "1",
                      "--patch-file",
                      patch_path.string(),
                      "--output",
                      updated.string(),
                      "--json"},
                     output),
             0);
    const auto output_json = read_text_file(output);
    CHECK_NE(output_json.find("\"command\":\"set-template-table-from-json\""),
             std::string::npos);
    CHECK_NE(output_json.find("\"after_text\":\"selector target table\""),
             std::string::npos);
    CHECK_NE(output_json.find("\"header_cells\":[\"Region\",\"Qty\",\"Amount\"]"),
             std::string::npos);
    CHECK_NE(output_json.find("\"header_row_index\":0"), std::string::npos);
    CHECK_NE(output_json.find("\"occurrence\":1"), std::string::npos);
    CHECK_NE(output_json.find("\"table_index\":2"), std::string::npos);

    featherdoc::Document reopened(updated);
    REQUIRE_FALSE(reopened.open());
    auto body = reopened.body_template();
    REQUIRE(static_cast<bool>(body));
    const auto preserved_first_target = body.inspect_table_cell(0U, 1U, 0U);
    REQUIRE(preserved_first_target.has_value());
    CHECK_EQ(preserved_first_target->text, "North");
    const auto updated_region = body.inspect_table_cell(2U, 1U, 0U);
    REQUIRE(updated_region.has_value());
    CHECK_EQ(updated_region->text, "South-updated");
    const auto updated_qty = body.inspect_table_cell(2U, 1U, 1U);
    REQUIRE(updated_qty.has_value());
    CHECK_EQ(updated_qty->text, "18");
    const auto updated_amount = body.inspect_table_cell(2U, 1U, 2U);
    REQUIRE(updated_amount.has_value());
    CHECK_EQ(updated_amount->text, "199");

    CHECK_EQ(run_cli({"set-template-table-from-json",
                      source.string(),
                      "--header-row",
                      "1",
                      "--patch-file",
                      patch_path.string(),
                      "--json"},
                     parse_output),
             2);
    CHECK_EQ(
        read_text_file(parse_output),
        std::string{
            "{\"command\":\"set-template-table-from-json\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"--header-row requires at least "
            "one --header-cell\"}\n"});

    remove_if_exists(source);
    remove_if_exists(patch_path);
    remove_if_exists(updated);
    remove_if_exists(output);
    remove_if_exists(parse_output);
}

TEST_CASE("cli set-template-tables-from-json applies multiple table patches") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_template_tables_batch_source.docx";
    const fs::path success_patch =
        working_directory / "cli_template_tables_batch_success.json";
    const fs::path parse_patch =
        working_directory / "cli_template_tables_batch_parse.json";
    const fs::path mutate_patch =
        working_directory / "cli_template_tables_batch_mutate.json";
    const fs::path updated =
        working_directory / "cli_template_tables_batch_updated.docx";
    const fs::path success_output =
        working_directory / "cli_template_tables_batch_success_output.json";
    const fs::path parse_output =
        working_directory / "cli_template_tables_batch_parse_output.json";
    const fs::path mutate_output =
        working_directory / "cli_template_tables_batch_mutate_output.json";

    remove_if_exists(source);
    remove_if_exists(success_patch);
    remove_if_exists(parse_patch);
    remove_if_exists(mutate_patch);
    remove_if_exists(updated);
    remove_if_exists(success_output);
    remove_if_exists(parse_output);
    remove_if_exists(mutate_output);

    create_cli_template_table_bookmark_fixture(source);
    write_binary_file(
        success_patch,
        R"({
  "operations": [
    {
      "table_index": 0,
      "mode": "rows",
      "start_row": 0,
      "rows": [
        ["keep-json-00", "keep-json-01"]
      ]
    },
    {
      "bookmark": "target_before_table",
      "start_row_index": 0,
      "start_cell_index": 1,
      "rows": [
        [12],
        [false]
      ]
    }
  ]
})");
    write_binary_file(
        parse_patch,
        R"({
  "operations": [
    {
      "mode": "rows",
      "start_row": 0,
      "rows": [
        ["missing-selector", "value"]
      ]
    }
  ]
})");
    write_binary_file(
        mutate_patch,
        R"({
  "operations": [
    {
      "table_index": 0,
      "mode": "rows",
      "start_row": 0,
      "rows": [
        ["keep-json-00", "keep-json-01"]
      ]
    },
    {
      "bookmark_name": "target_inside_table",
      "mode": "rows",
      "start_row": 1,
      "rows": [
        ["overflow-10", "overflow-11"],
        ["overflow-20", "overflow-21"]
      ]
    }
  ]
})");

    CHECK_EQ(run_cli({"set-template-tables-from-json",
                      source.string(),
                      "--patch-file",
                      success_patch.string(),
                      "--output",
                      updated.string(),
                      "--json"},
                     success_output),
             0);
    const auto success_json = read_text_file(success_output);
    CHECK_NE(success_json.find("\"command\":\"set-template-tables-from-json\""),
             std::string::npos);
    CHECK_NE(success_json.find("\"operation_count\":2"), std::string::npos);
    CHECK_NE(success_json.find("\"operation_index\":0"), std::string::npos);
    CHECK_NE(success_json.find("\"table_index\":0"), std::string::npos);
    CHECK_NE(success_json.find("\"rows\":[[\"keep-json-00\",\"keep-json-01\"]]"),
             std::string::npos);
    CHECK_NE(success_json.find("\"operation_index\":1"), std::string::npos);
    CHECK_NE(success_json.find("\"bookmark_name\":\"target_before_table\""),
             std::string::npos);
    CHECK_NE(success_json.find("\"mode\":\"block\""), std::string::npos);
    CHECK_NE(success_json.find("\"start_cell_index\":1"), std::string::npos);
    CHECK_NE(success_json.find("\"rows\":[[\"12\"],[\"false\"]]"),
             std::string::npos);

    featherdoc::Document reopened_updated(updated);
    REQUIRE_FALSE(reopened_updated.open());
    auto updated_body = reopened_updated.body_template();
    REQUIRE(static_cast<bool>(updated_body));
    const auto first_table_first_cell =
        updated_body.inspect_table_cell(0U, 0U, 0U);
    REQUIRE(first_table_first_cell.has_value());
    CHECK_EQ(first_table_first_cell->text, "keep-json-00");
    const auto first_table_second_cell =
        updated_body.inspect_table_cell(0U, 0U, 1U);
    REQUIRE(first_table_second_cell.has_value());
    CHECK_EQ(first_table_second_cell->text, "keep-json-01");
    const auto second_table_first_region =
        updated_body.inspect_table_cell(1U, 0U, 0U);
    REQUIRE(second_table_first_region.has_value());
    CHECK_EQ(second_table_first_region->text, "target-00");
    const auto second_table_first_value =
        updated_body.inspect_table_cell(1U, 0U, 1U);
    REQUIRE(second_table_first_value.has_value());
    CHECK_EQ(second_table_first_value->text, "12");
    const auto second_table_second_region =
        updated_body.inspect_table_cell(1U, 1U, 0U);
    REQUIRE(second_table_second_region.has_value());
    CHECK_EQ(second_table_second_region->text, "target-10");
    const auto second_table_second_value =
        updated_body.inspect_table_cell(1U, 1U, 1U);
    REQUIRE(second_table_second_value.has_value());
    CHECK_EQ(second_table_second_value->text, "false");

    CHECK_EQ(run_cli({"set-template-tables-from-json",
                      source.string(),
                      "--patch-file",
                      parse_patch.string(),
                      "--json"},
                     parse_output),
             2);
    CHECK_EQ(
        read_text_file(parse_output),
        std::string{
            "{\"command\":\"set-template-tables-from-json\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"operation[0]: expected a table "
            "index, --bookmark <name>, --after-text <text>, or --header-cell "
            "<text>\"}\n"});

    CHECK_EQ(run_cli({"set-template-tables-from-json",
                      source.string(),
                      "--patch-file",
                      mutate_patch.string(),
                      "--json"},
                     mutate_output),
             1);
    const auto mutate_json = read_text_file(mutate_output);
    CHECK_NE(mutate_json.find("\"command\":\"set-template-tables-from-json\""),
             std::string::npos);
    CHECK_NE(mutate_json.find("\"stage\":\"mutate\""), std::string::npos);
    CHECK_NE(mutate_json.find("\"detail\":\"operation[1]: row range starting at row index '1' with count '2' exceeds table index '1' row count '2'\""),
             std::string::npos);

    remove_if_exists(source);
    remove_if_exists(success_patch);
    remove_if_exists(parse_patch);
    remove_if_exists(mutate_patch);
    remove_if_exists(updated);
    remove_if_exists(success_output);
    remove_if_exists(parse_output);
    remove_if_exists(mutate_output);
}

TEST_CASE("cli set-template-tables-from-json supports selector-based operations") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_template_tables_selector_source.docx";
    const fs::path success_patch =
        working_directory / "cli_template_tables_selector_success.json";
    const fs::path parse_patch =
        working_directory / "cli_template_tables_selector_parse.json";
    const fs::path updated =
        working_directory / "cli_template_tables_selector_updated.docx";
    const fs::path success_output =
        working_directory / "cli_template_tables_selector_success_output.json";
    const fs::path parse_output =
        working_directory / "cli_template_tables_selector_parse_output.json";

    remove_if_exists(source);
    remove_if_exists(success_patch);
    remove_if_exists(parse_patch);
    remove_if_exists(updated);
    remove_if_exists(success_output);
    remove_if_exists(parse_output);

    create_cli_template_table_selector_fixture(source);
    write_binary_file(
        success_patch,
        R"({
  "operations": [
    {
      "after_text": "selector target table",
      "header_cells": ["Region", "Qty", "Amount"],
      "occurrence": 1,
      "mode": "rows",
      "start_row": 1,
      "rows": [
        ["South-batch", 21, 240]
      ]
    },
    {
      "header_cell_texts": ["Label", "Value"],
      "mode": "rows",
      "start_row": 1,
      "rows": [
        ["Other-batch", 42]
      ]
    }
  ]
})");
    write_binary_file(
        parse_patch,
        R"({
  "operations": [
    {
      "table_index": 2,
      "after_text": "selector target table",
      "mode": "rows",
      "start_row": 1,
      "rows": [
        ["invalid-selector", 1, 2]
      ]
    }
  ]
})");

    CHECK_EQ(run_cli({"set-template-tables-from-json",
                      source.string(),
                      "--patch-file",
                      success_patch.string(),
                      "--output",
                      updated.string(),
                      "--json"},
                     success_output),
             0);
    const auto success_json = read_text_file(success_output);
    CHECK_NE(success_json.find("\"command\":\"set-template-tables-from-json\""),
             std::string::npos);
    CHECK_NE(success_json.find("\"after_text\":\"selector target table\""),
             std::string::npos);
    CHECK_NE(success_json.find("\"header_cells\":[\"Region\",\"Qty\",\"Amount\"]"),
             std::string::npos);
    CHECK_NE(success_json.find("\"occurrence\":1"), std::string::npos);
    CHECK_NE(success_json.find("\"table_index\":2"), std::string::npos);
    CHECK_NE(success_json.find("\"header_cells\":[\"Label\",\"Value\"]"),
             std::string::npos);

    featherdoc::Document reopened(updated);
    REQUIRE_FALSE(reopened.open());
    auto body = reopened.body_template();
    REQUIRE(static_cast<bool>(body));
    const auto updated_other_left = body.inspect_table_cell(1U, 1U, 0U);
    REQUIRE(updated_other_left.has_value());
    CHECK_EQ(updated_other_left->text, "Other-batch");
    const auto updated_other_right = body.inspect_table_cell(1U, 1U, 1U);
    REQUIRE(updated_other_right.has_value());
    CHECK_EQ(updated_other_right->text, "42");
    const auto updated_target_region = body.inspect_table_cell(2U, 1U, 0U);
    REQUIRE(updated_target_region.has_value());
    CHECK_EQ(updated_target_region->text, "South-batch");
    const auto updated_target_qty = body.inspect_table_cell(2U, 1U, 1U);
    REQUIRE(updated_target_qty.has_value());
    CHECK_EQ(updated_target_qty->text, "21");
    const auto updated_target_amount = body.inspect_table_cell(2U, 1U, 2U);
    REQUIRE(updated_target_amount.has_value());
    CHECK_EQ(updated_target_amount->text, "240");

    CHECK_EQ(run_cli({"set-template-tables-from-json",
                      source.string(),
                      "--patch-file",
                      parse_patch.string(),
                      "--json"},
                     parse_output),
             2);
    CHECK_EQ(
        read_text_file(parse_output),
        std::string{
            "{\"command\":\"set-template-tables-from-json\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"operation[0]: cannot combine a "
            "table index or --bookmark with --after-text or --header-cell\"}\n"});

    remove_if_exists(source);
    remove_if_exists(success_patch);
    remove_if_exists(parse_patch);
    remove_if_exists(updated);
    remove_if_exists(success_output);
    remove_if_exists(parse_output);
}

TEST_CASE("cli selector-based template table text mutations target the requested table") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_template_table_selector_direct_text_source.docx";
    const fs::path cell_updated =
        working_directory / "cli_template_table_selector_direct_cell_updated.docx";
    const fs::path multiline_cell_updated =
        working_directory / "cli_template_table_selector_direct_multiline_cell_updated.docx";
    const fs::path row_updated =
        working_directory / "cli_template_table_selector_direct_row_updated.docx";
    const fs::path block_updated =
        working_directory / "cli_template_table_selector_direct_block_updated.docx";
    const fs::path multiline_block_updated =
        working_directory / "cli_template_table_selector_direct_multiline_block_updated.docx";
    const fs::path cell_output =
        working_directory / "cli_template_table_selector_direct_cell_output.json";
    const fs::path multiline_cell_output =
        working_directory / "cli_template_table_selector_direct_multiline_cell_output.json";
    const fs::path row_output =
        working_directory / "cli_template_table_selector_direct_row_output.json";
    const fs::path block_output =
        working_directory / "cli_template_table_selector_direct_block_output.json";
    const fs::path multiline_block_output =
        working_directory / "cli_template_table_selector_direct_multiline_block_output.json";

    remove_if_exists(source);
    remove_if_exists(cell_updated);
    remove_if_exists(multiline_cell_updated);
    remove_if_exists(row_updated);
    remove_if_exists(block_updated);
    remove_if_exists(multiline_block_updated);
    remove_if_exists(cell_output);
    remove_if_exists(multiline_cell_output);
    remove_if_exists(row_output);
    remove_if_exists(block_output);
    remove_if_exists(multiline_block_output);

    create_cli_template_table_selector_fixture(source);

    CHECK_EQ(run_cli({"set-template-table-cell-text",
                      source.string(),
                      "--after-text",
                      "selector target table",
                      "--header-cell",
                      "Region",
                      "--header-cell",
                      "Qty",
                      "--header-cell",
                      "Amount",
                      "--occurrence",
                      "1",
                      "1",
                      "1",
                      "--text",
                      "42",
                      "--output",
                      cell_updated.string(),
                      "--json"},
                     cell_output),
             0);
    const auto cell_json = read_text_file(cell_output);
    CHECK_NE(cell_json.find("\"command\":\"set-template-table-cell-text\""),
             std::string::npos);
    CHECK_NE(cell_json.find("\"table_index\":2"), std::string::npos);
    CHECK_NE(cell_json.find("\"row_index\":1"), std::string::npos);
    CHECK_NE(cell_json.find("\"cell_index\":1"), std::string::npos);

    featherdoc::Document reopened_cell(cell_updated);
    REQUIRE_FALSE(reopened_cell.open());
    auto cell_body = reopened_cell.body_template();
    REQUIRE(static_cast<bool>(cell_body));
    const auto preserved_first_qty = cell_body.inspect_table_cell(0U, 1U, 1U);
    REQUIRE(preserved_first_qty.has_value());
    CHECK_EQ(preserved_first_qty->text, "7");
    const auto updated_target_qty = cell_body.inspect_table_cell(2U, 1U, 1U);
    REQUIRE(updated_target_qty.has_value());
    CHECK_EQ(updated_target_qty->text, "42");

    CHECK_EQ(run_cli({"set-template-table-cell-text",
                      source.string(),
                      "--after-text",
                      "selector target table",
                      "--header-cell",
                      "Region",
                      "--header-cell",
                      "Qty",
                      "--header-cell",
                      "Amount",
                      "--occurrence",
                      "1",
                      "1",
                      "2",
                      "--text",
                      "240 total\nfinance review",
                      "--output",
                      multiline_cell_updated.string(),
                      "--json"},
                     multiline_cell_output),
             0);
    const auto multiline_cell_json = read_text_file(multiline_cell_output);
    CHECK_NE(multiline_cell_json.find("\"command\":\"set-template-table-cell-text\""),
             std::string::npos);
    CHECK_NE(multiline_cell_json.find("\"table_index\":2"), std::string::npos);
    CHECK_NE(multiline_cell_json.find("\"row_index\":1"), std::string::npos);
    CHECK_NE(multiline_cell_json.find("\"cell_index\":2"), std::string::npos);

    featherdoc::Document reopened_multiline_cell(multiline_cell_updated);
    REQUIRE_FALSE(reopened_multiline_cell.open());
    auto multiline_cell_body = reopened_multiline_cell.body_template();
    REQUIRE(static_cast<bool>(multiline_cell_body));
    const auto preserved_first_amount =
        multiline_cell_body.inspect_table_cell(0U, 1U, 2U);
    REQUIRE(preserved_first_amount.has_value());
    CHECK_EQ(preserved_first_amount->text, "12");
    const auto updated_multiline_cell_amount =
        multiline_cell_body.inspect_table_cell(2U, 1U, 2U);
    REQUIRE(updated_multiline_cell_amount.has_value());
    CHECK_EQ(updated_multiline_cell_amount->text, "240 total\nfinance review");

    CHECK_EQ(run_cli({"set-template-table-row-texts",
                      source.string(),
                      "--after-text",
                      "selector target table",
                      "--header-cell",
                      "Region",
                      "--header-cell",
                      "Qty",
                      "--header-cell",
                      "Amount",
                      "--occurrence",
                      "1",
                      "1",
                      "--row",
                      "South revised",
                      "--cell",
                      "18",
                      "--cell",
                      "199",
                      "--output",
                      row_updated.string(),
                      "--json"},
                     row_output),
             0);
    const auto row_json = read_text_file(row_output);
    CHECK_NE(row_json.find("\"command\":\"set-template-table-row-texts\""),
             std::string::npos);
    CHECK_NE(row_json.find("\"table_index\":2"), std::string::npos);
    CHECK_NE(row_json.find("\"start_row_index\":1"), std::string::npos);

    featherdoc::Document reopened_row(row_updated);
    REQUIRE_FALSE(reopened_row.open());
    auto row_body = reopened_row.body_template();
    REQUIRE(static_cast<bool>(row_body));
    const auto preserved_first_region = row_body.inspect_table_cell(0U, 1U, 0U);
    REQUIRE(preserved_first_region.has_value());
    CHECK_EQ(preserved_first_region->text, "North");
    const auto updated_target_region = row_body.inspect_table_cell(2U, 1U, 0U);
    REQUIRE(updated_target_region.has_value());
    CHECK_EQ(updated_target_region->text, "South revised");
    const auto updated_target_amount = row_body.inspect_table_cell(2U, 1U, 2U);
    REQUIRE(updated_target_amount.has_value());
    CHECK_EQ(updated_target_amount->text, "199");

    CHECK_EQ(run_cli({"set-template-table-cell-block-texts",
                      source.string(),
                      "--after-text",
                      "selector target table",
                      "--header-cell",
                      "Region",
                      "--header-cell",
                      "Qty",
                      "--header-cell",
                      "Amount",
                      "--occurrence",
                      "1",
                      "1",
                      "1",
                      "--row",
                      "18",
                      "--cell",
                      "240",
                      "--output",
                      block_updated.string(),
                      "--json"},
                     block_output),
             0);
    const auto block_json = read_text_file(block_output);
    CHECK_NE(block_json.find("\"command\":\"set-template-table-cell-block-texts\""),
             std::string::npos);
    CHECK_NE(block_json.find("\"table_index\":2"), std::string::npos);
    CHECK_NE(block_json.find("\"start_row_index\":1"), std::string::npos);
    CHECK_NE(block_json.find("\"start_cell_index\":1"), std::string::npos);

    featherdoc::Document reopened_block(block_updated);
    REQUIRE_FALSE(reopened_block.open());
    auto block_body = reopened_block.body_template();
    REQUIRE(static_cast<bool>(block_body));
    const auto preserved_target_region = block_body.inspect_table_cell(2U, 1U, 0U);
    REQUIRE(preserved_target_region.has_value());
    CHECK_EQ(preserved_target_region->text, "South");
    const auto updated_block_qty = block_body.inspect_table_cell(2U, 1U, 1U);
    REQUIRE(updated_block_qty.has_value());
    CHECK_EQ(updated_block_qty->text, "18");
    const auto updated_block_amount = block_body.inspect_table_cell(2U, 1U, 2U);
    REQUIRE(updated_block_amount.has_value());
    CHECK_EQ(updated_block_amount->text, "240");

    CHECK_EQ(run_cli({"set-template-table-cell-block-texts",
                      source.string(),
                      "--after-text",
                      "selector target table",
                      "--header-cell",
                      "Region",
                      "--header-cell",
                      "Qty",
                      "--header-cell",
                      "Amount",
                      "--occurrence",
                      "1",
                      "1",
                      "1",
                      "--row",
                      "18 units\npending approval",
                      "--cell",
                      "240 total\nfinance review",
                      "--output",
                      multiline_block_updated.string(),
                      "--json"},
                     multiline_block_output),
             0);
    const auto multiline_block_json = read_text_file(multiline_block_output);
    CHECK_NE(multiline_block_json.find("\"command\":\"set-template-table-cell-block-texts\""),
             std::string::npos);
    CHECK_NE(multiline_block_json.find("\"table_index\":2"), std::string::npos);

    featherdoc::Document reopened_multiline_block(multiline_block_updated);
    REQUIRE_FALSE(reopened_multiline_block.open());
    auto multiline_block_body = reopened_multiline_block.body_template();
    REQUIRE(static_cast<bool>(multiline_block_body));
    const auto updated_multiline_qty =
        multiline_block_body.inspect_table_cell(2U, 1U, 1U);
    REQUIRE(updated_multiline_qty.has_value());
    CHECK_EQ(updated_multiline_qty->text, "18 units\npending approval");
    const auto updated_multiline_amount =
        multiline_block_body.inspect_table_cell(2U, 1U, 2U);
    REQUIRE(updated_multiline_amount.has_value());
    CHECK_EQ(updated_multiline_amount->text, "240 total\nfinance review");

    remove_if_exists(source);
    remove_if_exists(cell_updated);
    remove_if_exists(multiline_cell_updated);
    remove_if_exists(row_updated);
    remove_if_exists(block_updated);
    remove_if_exists(multiline_block_updated);
    remove_if_exists(cell_output);
    remove_if_exists(multiline_cell_output);
    remove_if_exists(row_output);
    remove_if_exists(block_output);
    remove_if_exists(multiline_block_output);
}

TEST_CASE("cli selector-based template table row commands mutate only the matched table") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_template_table_selector_row_commands_source.docx";
    const fs::path appended =
        working_directory / "cli_template_table_selector_row_appended.docx";
    const fs::path inserted_before =
        working_directory / "cli_template_table_selector_row_before.docx";
    const fs::path inserted_after =
        working_directory / "cli_template_table_selector_row_after.docx";
    const fs::path removed =
        working_directory / "cli_template_table_selector_row_removed.docx";
    const fs::path append_output =
        working_directory / "cli_template_table_selector_row_append.json";
    const fs::path before_output =
        working_directory / "cli_template_table_selector_row_before.json";
    const fs::path after_output =
        working_directory / "cli_template_table_selector_row_after.json";
    const fs::path remove_output =
        working_directory / "cli_template_table_selector_row_remove.json";
    const fs::path parse_output =
        working_directory / "cli_template_table_selector_row_parse.json";

    remove_if_exists(source);
    remove_if_exists(appended);
    remove_if_exists(inserted_before);
    remove_if_exists(inserted_after);
    remove_if_exists(removed);
    remove_if_exists(append_output);
    remove_if_exists(before_output);
    remove_if_exists(after_output);
    remove_if_exists(remove_output);
    remove_if_exists(parse_output);

    create_cli_template_table_selector_fixture(source);

    CHECK_EQ(run_cli({"append-template-table-row",
                      source.string(),
                      "--after-text",
                      "selector target table",
                      "--header-cell",
                      "Region",
                      "--header-cell",
                      "Qty",
                      "--header-cell",
                      "Amount",
                      "--occurrence",
                      "1",
                      "--cell-count",
                      "3",
                      "--output",
                      appended.string(),
                      "--json"},
                     append_output),
             0);
    const auto append_json = read_text_file(append_output);
    CHECK_NE(append_json.find("\"command\":\"append-template-table-row\""),
             std::string::npos);
    CHECK_NE(append_json.find("\"table_index\":2"), std::string::npos);
    CHECK_NE(append_json.find("\"row_index\":2"), std::string::npos);

    featherdoc::Document reopened_append(appended);
    REQUIRE_FALSE(reopened_append.open());
    auto append_body = reopened_append.body_template();
    REQUIRE(static_cast<bool>(append_body));
    const auto preserved_first_table = append_body.inspect_table(0U);
    REQUIRE(preserved_first_table.has_value());
    CHECK_EQ(preserved_first_table->row_count, 2U);
    const auto appended_target_table = append_body.inspect_table(2U);
    REQUIRE(appended_target_table.has_value());
    CHECK_EQ(appended_target_table->row_count, 3U);
    const auto appended_target_cell = append_body.inspect_table_cell(2U, 2U, 0U);
    REQUIRE(appended_target_cell.has_value());
    CHECK_EQ(appended_target_cell->text, "");

    CHECK_EQ(run_cli({"insert-template-table-row-before",
                      source.string(),
                      "--after-text",
                      "selector target table",
                      "--header-cell",
                      "Region",
                      "--header-cell",
                      "Qty",
                      "--header-cell",
                      "Amount",
                      "--occurrence",
                      "1",
                      "1",
                      "--output",
                      inserted_before.string(),
                      "--json"},
                     before_output),
             0);
    const auto before_json = read_text_file(before_output);
    CHECK_NE(before_json.find("\"command\":\"insert-template-table-row-before\""),
             std::string::npos);
    CHECK_NE(before_json.find("\"table_index\":2"), std::string::npos);
    CHECK_NE(before_json.find("\"inserted_row_index\":1"), std::string::npos);

    featherdoc::Document reopened_before(inserted_before);
    REQUIRE_FALSE(reopened_before.open());
    auto before_body = reopened_before.body_template();
    REQUIRE(static_cast<bool>(before_body));
    const auto before_target_table = before_body.inspect_table(2U);
    REQUIRE(before_target_table.has_value());
    CHECK_EQ(before_target_table->row_count, 3U);
    const auto before_inserted_cell = before_body.inspect_table_cell(2U, 1U, 0U);
    REQUIRE(before_inserted_cell.has_value());
    CHECK_EQ(before_inserted_cell->text, "");
    const auto before_shifted_cell = before_body.inspect_table_cell(2U, 2U, 0U);
    REQUIRE(before_shifted_cell.has_value());
    CHECK_EQ(before_shifted_cell->text, "South");

    CHECK_EQ(run_cli({"insert-template-table-row-after",
                      source.string(),
                      "--after-text",
                      "selector target table",
                      "--header-cell",
                      "Region",
                      "--header-cell",
                      "Qty",
                      "--header-cell",
                      "Amount",
                      "--occurrence",
                      "1",
                      "0",
                      "--output",
                      inserted_after.string(),
                      "--json"},
                     after_output),
             0);
    const auto after_json = read_text_file(after_output);
    CHECK_NE(after_json.find("\"command\":\"insert-template-table-row-after\""),
             std::string::npos);
    CHECK_NE(after_json.find("\"table_index\":2"), std::string::npos);
    CHECK_NE(after_json.find("\"inserted_row_index\":1"), std::string::npos);

    featherdoc::Document reopened_after(inserted_after);
    REQUIRE_FALSE(reopened_after.open());
    auto after_body = reopened_after.body_template();
    REQUIRE(static_cast<bool>(after_body));
    const auto after_target_table = after_body.inspect_table(2U);
    REQUIRE(after_target_table.has_value());
    CHECK_EQ(after_target_table->row_count, 3U);
    const auto after_inserted_cell = after_body.inspect_table_cell(2U, 1U, 0U);
    REQUIRE(after_inserted_cell.has_value());
    CHECK_EQ(after_inserted_cell->text, "");
    const auto after_shifted_cell = after_body.inspect_table_cell(2U, 2U, 0U);
    REQUIRE(after_shifted_cell.has_value());
    CHECK_EQ(after_shifted_cell->text, "South");

    CHECK_EQ(run_cli({"remove-template-table-row",
                      source.string(),
                      "--after-text",
                      "selector target table",
                      "--header-cell",
                      "Region",
                      "--header-cell",
                      "Qty",
                      "--header-cell",
                      "Amount",
                      "--occurrence",
                      "1",
                      "1",
                      "--output",
                      removed.string(),
                      "--json"},
                     remove_output),
             0);
    const auto remove_json = read_text_file(remove_output);
    CHECK_NE(remove_json.find("\"command\":\"remove-template-table-row\""),
             std::string::npos);
    CHECK_NE(remove_json.find("\"table_index\":2"), std::string::npos);
    CHECK_NE(remove_json.find("\"row_index\":1"), std::string::npos);

    featherdoc::Document reopened_remove(removed);
    REQUIRE_FALSE(reopened_remove.open());
    auto remove_body = reopened_remove.body_template();
    REQUIRE(static_cast<bool>(remove_body));
    const auto removed_target_table = remove_body.inspect_table(2U);
    REQUIRE(removed_target_table.has_value());
    CHECK_EQ(removed_target_table->row_count, 1U);
    const auto removed_header_cell = remove_body.inspect_table_cell(2U, 0U, 0U);
    REQUIRE(removed_header_cell.has_value());
    CHECK_EQ(removed_header_cell->text, "Region");

    CHECK_EQ(run_cli({"insert-template-table-row-before",
                      source.string(),
                      "--after-text",
                      "selector target table",
                      "--header-cell",
                      "Region",
                      "--header-cell",
                      "Qty",
                      "--header-cell",
                      "Amount",
                      "--occurrence",
                      "1",
                      "--json"},
                     parse_output),
             2);
    CHECK_EQ(
        read_text_file(parse_output),
        std::string{
            "{\"command\":\"insert-template-table-row-before\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"missing row index after table "
            "selector\"}\n"});

    remove_if_exists(source);
    remove_if_exists(appended);
    remove_if_exists(inserted_before);
    remove_if_exists(inserted_after);
    remove_if_exists(removed);
    remove_if_exists(append_output);
    remove_if_exists(before_output);
    remove_if_exists(after_output);
    remove_if_exists(remove_output);
    remove_if_exists(parse_output);
}

TEST_CASE("cli selector-based template table column commands mutate only the matched table") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_template_table_selector_column_source.docx";
    const fs::path inserted_before =
        working_directory / "cli_template_table_selector_column_before.docx";
    const fs::path inserted_after =
        working_directory / "cli_template_table_selector_column_after.docx";
    const fs::path removed =
        working_directory / "cli_template_table_selector_column_removed.docx";
    const fs::path before_output =
        working_directory / "cli_template_table_selector_column_before.json";
    const fs::path after_output =
        working_directory / "cli_template_table_selector_column_after.json";
    const fs::path remove_output =
        working_directory / "cli_template_table_selector_column_removed.json";

    remove_if_exists(source);
    remove_if_exists(inserted_before);
    remove_if_exists(inserted_after);
    remove_if_exists(removed);
    remove_if_exists(before_output);
    remove_if_exists(after_output);
    remove_if_exists(remove_output);

    create_cli_template_table_selector_fixture(source);

    CHECK_EQ(run_cli({"insert-template-table-column-before",
                      source.string(),
                      "--after-text",
                      "selector target table",
                      "--header-cell",
                      "Region",
                      "--header-cell",
                      "Qty",
                      "--header-cell",
                      "Amount",
                      "--occurrence",
                      "1",
                      "0",
                      "1",
                      "--output",
                      inserted_before.string(),
                      "--json"},
                     before_output),
             0);
    const auto before_json = read_text_file(before_output);
    CHECK_NE(before_json.find("\"command\":\"insert-template-table-column-before\""),
             std::string::npos);
    CHECK_NE(before_json.find("\"table_index\":2"), std::string::npos);
    CHECK_NE(before_json.find("\"inserted_column_index\":1"),
             std::string::npos);

    featherdoc::Document reopened_before(inserted_before);
    REQUIRE_FALSE(reopened_before.open());
    auto before_body = reopened_before.body_template();
    REQUIRE(static_cast<bool>(before_body));
    const auto before_first_table = before_body.inspect_table(0U);
    REQUIRE(before_first_table.has_value());
    CHECK_EQ(before_first_table->column_count, 3U);
    const auto before_target_table = before_body.inspect_table(2U);
    REQUIRE(before_target_table.has_value());
    CHECK_EQ(before_target_table->column_count, 4U);
    const auto before_inserted_header = before_body.inspect_table_cell(2U, 0U, 1U);
    REQUIRE(before_inserted_header.has_value());
    CHECK_EQ(before_inserted_header->text, "");
    const auto before_shifted_header = before_body.inspect_table_cell(2U, 0U, 2U);
    REQUIRE(before_shifted_header.has_value());
    CHECK_EQ(before_shifted_header->text, "Qty");

    CHECK_EQ(run_cli({"insert-template-table-column-after",
                      source.string(),
                      "--after-text",
                      "selector target table",
                      "--header-cell",
                      "Region",
                      "--header-cell",
                      "Qty",
                      "--header-cell",
                      "Amount",
                      "--occurrence",
                      "1",
                      "0",
                      "0",
                      "--output",
                      inserted_after.string(),
                      "--json"},
                     after_output),
             0);
    const auto after_json = read_text_file(after_output);
    CHECK_NE(after_json.find("\"command\":\"insert-template-table-column-after\""),
             std::string::npos);
    CHECK_NE(after_json.find("\"table_index\":2"), std::string::npos);
    CHECK_NE(after_json.find("\"inserted_column_index\":1"),
             std::string::npos);

    featherdoc::Document reopened_after(inserted_after);
    REQUIRE_FALSE(reopened_after.open());
    auto after_body = reopened_after.body_template();
    REQUIRE(static_cast<bool>(after_body));
    const auto after_target_table = after_body.inspect_table(2U);
    REQUIRE(after_target_table.has_value());
    CHECK_EQ(after_target_table->column_count, 4U);
    const auto after_inserted_header = after_body.inspect_table_cell(2U, 0U, 1U);
    REQUIRE(after_inserted_header.has_value());
    CHECK_EQ(after_inserted_header->text, "");
    const auto after_shifted_header = after_body.inspect_table_cell(2U, 0U, 2U);
    REQUIRE(after_shifted_header.has_value());
    CHECK_EQ(after_shifted_header->text, "Qty");

    CHECK_EQ(run_cli({"remove-template-table-column",
                      source.string(),
                      "--after-text",
                      "selector target table",
                      "--header-cell",
                      "Region",
                      "--header-cell",
                      "Qty",
                      "--header-cell",
                      "Amount",
                      "--occurrence",
                      "1",
                      "0",
                      "1",
                      "--output",
                      removed.string(),
                      "--json"},
                     remove_output),
             0);
    const auto remove_json = read_text_file(remove_output);
    CHECK_NE(remove_json.find("\"command\":\"remove-template-table-column\""),
             std::string::npos);
    CHECK_NE(remove_json.find("\"table_index\":2"), std::string::npos);
    CHECK_NE(remove_json.find("\"column_index\":1"), std::string::npos);

    featherdoc::Document reopened_remove(removed);
    REQUIRE_FALSE(reopened_remove.open());
    auto remove_body = reopened_remove.body_template();
    REQUIRE(static_cast<bool>(remove_body));
    const auto removed_target_table = remove_body.inspect_table(2U);
    REQUIRE(removed_target_table.has_value());
    CHECK_EQ(removed_target_table->column_count, 2U);
    const auto removed_header = remove_body.inspect_table_cell(2U, 0U, 1U);
    REQUIRE(removed_header.has_value());
    CHECK_EQ(removed_header->text, "Amount");
    const auto removed_value = remove_body.inspect_table_cell(2U, 1U, 1U);
    REQUIRE(removed_value.has_value());
    CHECK_EQ(removed_value->text, "24");

    remove_if_exists(source);
    remove_if_exists(inserted_before);
    remove_if_exists(inserted_after);
    remove_if_exists(removed);
    remove_if_exists(before_output);
    remove_if_exists(after_output);
    remove_if_exists(remove_output);
}

TEST_CASE("cli template table column commands mutate section and body template tables") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_template_table_column_source.docx";
    const fs::path inserted_before =
        working_directory / "cli_template_table_column_before.docx";
    const fs::path inserted_after =
        working_directory / "cli_template_table_column_after.docx";
    const fs::path removed =
        working_directory / "cli_template_table_column_removed.docx";
    const fs::path before_output =
        working_directory / "cli_template_table_column_before.json";
    const fs::path after_output =
        working_directory / "cli_template_table_column_after.json";
    const fs::path remove_output =
        working_directory / "cli_template_table_column_removed.json";

    remove_if_exists(source);
    remove_if_exists(inserted_before);
    remove_if_exists(inserted_after);
    remove_if_exists(removed);
    remove_if_exists(before_output);
    remove_if_exists(after_output);
    remove_if_exists(remove_output);

    create_cli_template_table_column_fixture(source);

    CHECK_EQ(run_cli({"insert-template-table-column-before",
                      source.string(),
                      "0",
                      "0",
                      "1",
                      "--part",
                      "section-header",
                      "--section",
                      "0",
                      "--output",
                      inserted_before.string(),
                      "--json"},
                     before_output),
             0);
    const auto before_json = read_text_file(before_output);
    CHECK_NE(before_json.find("\"command\":\"insert-template-table-column-before\""),
             std::string::npos);
    CHECK_NE(before_json.find("\"part\":\"section-header\""), std::string::npos);
    CHECK_NE(before_json.find("\"section\":0"), std::string::npos);
    CHECK_NE(before_json.find("\"entry_name\":\"word/header1.xml\""),
             std::string::npos);
    CHECK_NE(before_json.find("\"inserted_cell_index\":1"),
             std::string::npos);
    CHECK_NE(before_json.find("\"inserted_column_index\":1"),
             std::string::npos);

    featherdoc::Document reopened_before(inserted_before);
    REQUIRE_FALSE(reopened_before.open());
    auto before_template = reopened_before.section_header_template(0U);
    REQUIRE(static_cast<bool>(before_template));
    const auto before_table = before_template.inspect_table(0U);
    REQUIRE(before_table.has_value());
    CHECK_EQ(before_table->column_count, 3U);
    REQUIRE(before_table->column_widths.size() >= 3U);
    CHECK(before_table->column_widths[0].has_value());
    CHECK(before_table->column_widths[1].has_value());
    CHECK(before_table->column_widths[2].has_value());
    CHECK_EQ(*before_table->column_widths[0], 1800U);
    CHECK_EQ(*before_table->column_widths[1], 3600U);
    CHECK_EQ(*before_table->column_widths[2], 3600U);
    const auto before_inserted_header =
        before_template.inspect_table_cell(0U, 0U, 1U);
    REQUIRE(before_inserted_header.has_value());
    CHECK_EQ(before_inserted_header->text, "");
    const auto before_shifted_header =
        before_template.inspect_table_cell(0U, 0U, 2U);
    REQUIRE(before_shifted_header.has_value());
    CHECK_EQ(before_shifted_header->text, "h01");
    const auto before_inserted_body =
        before_template.inspect_table_cell(0U, 1U, 1U);
    REQUIRE(before_inserted_body.has_value());
    CHECK_EQ(before_inserted_body->text, "");

    CHECK_EQ(run_cli({"insert-template-table-column-after",
                      source.string(),
                      "0",
                      "0",
                      "0",
                      "--part",
                      "section-footer",
                      "--section",
                      "0",
                      "--output",
                      inserted_after.string(),
                      "--json"},
                     after_output),
             0);
    const auto after_json = read_text_file(after_output);
    CHECK_NE(after_json.find("\"command\":\"insert-template-table-column-after\""),
             std::string::npos);
    CHECK_NE(after_json.find("\"part\":\"section-footer\""), std::string::npos);
    CHECK_NE(after_json.find("\"section\":0"), std::string::npos);
    CHECK_NE(after_json.find("\"entry_name\":\"word/footer1.xml\""),
             std::string::npos);
    CHECK_NE(after_json.find("\"inserted_cell_index\":1"),
             std::string::npos);
    CHECK_NE(after_json.find("\"inserted_column_index\":1"),
             std::string::npos);

    featherdoc::Document reopened_after(inserted_after);
    REQUIRE_FALSE(reopened_after.open());
    auto after_template = reopened_after.section_footer_template(0U);
    REQUIRE(static_cast<bool>(after_template));
    const auto after_table = after_template.inspect_table(0U);
    REQUIRE(after_table.has_value());
    CHECK_EQ(after_table->column_count, 3U);
    REQUIRE(after_table->column_widths.size() >= 3U);
    CHECK(after_table->column_widths[0].has_value());
    CHECK(after_table->column_widths[1].has_value());
    CHECK(after_table->column_widths[2].has_value());
    CHECK_EQ(*after_table->column_widths[0], 1600U);
    CHECK_EQ(*after_table->column_widths[1], 1600U);
    CHECK_EQ(*after_table->column_widths[2], 2800U);
    const auto after_inserted_header =
        after_template.inspect_table_cell(0U, 0U, 1U);
    REQUIRE(after_inserted_header.has_value());
    CHECK_EQ(after_inserted_header->text, "");
    const auto after_shifted_header =
        after_template.inspect_table_cell(0U, 0U, 2U);
    REQUIRE(after_shifted_header.has_value());
    CHECK_EQ(after_shifted_header->text, "f01");
    const auto after_inserted_body =
        after_template.inspect_table_cell(0U, 1U, 1U);
    REQUIRE(after_inserted_body.has_value());
    CHECK_EQ(after_inserted_body->text, "");

    CHECK_EQ(run_cli({"remove-template-table-column",
                      source.string(),
                      "0",
                      "0",
                      "1",
                      "--part",
                      "body",
                      "--output",
                      removed.string(),
                      "--json"},
                     remove_output),
             0);
    const auto remove_json = read_text_file(remove_output);
    CHECK_NE(remove_json.find("\"command\":\"remove-template-table-column\""),
             std::string::npos);
    CHECK_NE(remove_json.find("\"part\":\"body\""), std::string::npos);
    CHECK_NE(remove_json.find("\"entry_name\":\"word/document.xml\""),
             std::string::npos);
    CHECK_NE(remove_json.find("\"column_index\":1"), std::string::npos);

    featherdoc::Document reopened_removed(removed);
    REQUIRE_FALSE(reopened_removed.open());
    auto removed_template = reopened_removed.body_template();
    REQUIRE(static_cast<bool>(removed_template));
    const auto removed_table = removed_template.inspect_table(0U);
    REQUIRE(removed_table.has_value());
    CHECK_EQ(removed_table->column_count, 2U);
    const auto removed_first_header =
        removed_template.inspect_table_cell(0U, 0U, 0U);
    REQUIRE(removed_first_header.has_value());
    CHECK_EQ(removed_first_header->text, "b00");
    const auto removed_second_header =
        removed_template.inspect_table_cell(0U, 0U, 1U);
    REQUIRE(removed_second_header.has_value());
    CHECK_EQ(removed_second_header->text, "b02");
    const auto removed_second_body =
        removed_template.inspect_table_cell(0U, 1U, 1U);
    REQUIRE(removed_second_body.has_value());
    CHECK_EQ(removed_second_body->text, "b12");

    remove_if_exists(source);
    remove_if_exists(inserted_before);
    remove_if_exists(inserted_after);
    remove_if_exists(removed);
    remove_if_exists(before_output);
    remove_if_exists(after_output);
    remove_if_exists(remove_output);
}

TEST_CASE("cli template table column commands mutate direct header and footer template tables") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_template_table_direct_column_source.docx";
    const fs::path inserted_before =
        working_directory / "cli_template_table_direct_column_before.docx";
    const fs::path inserted_after =
        working_directory / "cli_template_table_direct_column_after.docx";
    const fs::path removed =
        working_directory / "cli_template_table_direct_column_removed.docx";
    const fs::path before_output =
        working_directory / "cli_template_table_direct_column_before.json";
    const fs::path after_output =
        working_directory / "cli_template_table_direct_column_after.json";
    const fs::path remove_output =
        working_directory / "cli_template_table_direct_column_removed.json";

    remove_if_exists(source);
    remove_if_exists(inserted_before);
    remove_if_exists(inserted_after);
    remove_if_exists(removed);
    remove_if_exists(before_output);
    remove_if_exists(after_output);
    remove_if_exists(remove_output);

    create_cli_template_table_direct_column_fixture(source);

    CHECK_EQ(run_cli({"insert-template-table-column-before",
                      source.string(),
                      "0",
                      "0",
                      "1",
                      "--part",
                      "header",
                      "--index",
                      "0",
                      "--output",
                      inserted_before.string(),
                      "--json"},
                     before_output),
             0);
    const auto before_json = read_text_file(before_output);
    CHECK_NE(before_json.find("\"command\":\"insert-template-table-column-before\""),
             std::string::npos);
    CHECK_NE(before_json.find("\"part\":\"header\""), std::string::npos);
    CHECK_NE(before_json.find("\"part_index\":0"), std::string::npos);
    CHECK_NE(before_json.find("\"entry_name\":\"word/header1.xml\""),
             std::string::npos);
    CHECK_NE(before_json.find("\"inserted_cell_index\":1"),
             std::string::npos);
    CHECK_NE(before_json.find("\"inserted_column_index\":1"),
             std::string::npos);

    featherdoc::Document reopened_before(inserted_before);
    REQUIRE_FALSE(reopened_before.open());
    auto before_template = reopened_before.header_template(0U);
    REQUIRE(static_cast<bool>(before_template));
    const auto before_table = before_template.inspect_table(0U);
    REQUIRE(before_table.has_value());
    CHECK_EQ(before_table->column_count, 3U);
    REQUIRE(before_table->column_widths.size() >= 3U);
    CHECK(before_table->column_widths[0].has_value());
    CHECK(before_table->column_widths[1].has_value());
    CHECK(before_table->column_widths[2].has_value());
    CHECK_EQ(*before_table->column_widths[0], 1800U);
    CHECK_EQ(*before_table->column_widths[1], 3600U);
    CHECK_EQ(*before_table->column_widths[2], 3600U);
    const auto before_inserted_header =
        before_template.inspect_table_cell(0U, 0U, 1U);
    REQUIRE(before_inserted_header.has_value());
    CHECK_EQ(before_inserted_header->text, "");
    const auto before_shifted_header =
        before_template.inspect_table_cell(0U, 0U, 2U);
    REQUIRE(before_shifted_header.has_value());
    CHECK_EQ(before_shifted_header->text, "h01");
    const auto before_inserted_body =
        before_template.inspect_table_cell(0U, 1U, 1U);
    REQUIRE(before_inserted_body.has_value());
    CHECK_EQ(before_inserted_body->text, "");

    CHECK_EQ(run_cli({"insert-template-table-column-after",
                      source.string(),
                      "0",
                      "0",
                      "0",
                      "--part",
                      "footer",
                      "--index",
                      "0",
                      "--output",
                      inserted_after.string(),
                      "--json"},
                     after_output),
             0);
    const auto after_json = read_text_file(after_output);
    CHECK_NE(after_json.find("\"command\":\"insert-template-table-column-after\""),
             std::string::npos);
    CHECK_NE(after_json.find("\"part\":\"footer\""), std::string::npos);
    CHECK_NE(after_json.find("\"part_index\":0"), std::string::npos);
    CHECK_NE(after_json.find("\"entry_name\":\"word/footer1.xml\""),
             std::string::npos);
    CHECK_NE(after_json.find("\"inserted_cell_index\":1"),
             std::string::npos);
    CHECK_NE(after_json.find("\"inserted_column_index\":1"),
             std::string::npos);

    featherdoc::Document reopened_after(inserted_after);
    REQUIRE_FALSE(reopened_after.open());
    auto after_template = reopened_after.footer_template(0U);
    REQUIRE(static_cast<bool>(after_template));
    const auto after_table = after_template.inspect_table(0U);
    REQUIRE(after_table.has_value());
    CHECK_EQ(after_table->column_count, 3U);
    REQUIRE(after_table->column_widths.size() >= 3U);
    CHECK(after_table->column_widths[0].has_value());
    CHECK(after_table->column_widths[1].has_value());
    CHECK(after_table->column_widths[2].has_value());
    CHECK_EQ(*after_table->column_widths[0], 1600U);
    CHECK_EQ(*after_table->column_widths[1], 1600U);
    CHECK_EQ(*after_table->column_widths[2], 2800U);
    const auto after_inserted_header =
        after_template.inspect_table_cell(0U, 0U, 1U);
    REQUIRE(after_inserted_header.has_value());
    CHECK_EQ(after_inserted_header->text, "");
    const auto after_shifted_header =
        after_template.inspect_table_cell(0U, 0U, 2U);
    REQUIRE(after_shifted_header.has_value());
    CHECK_EQ(after_shifted_header->text, "f01");
    const auto after_inserted_body =
        after_template.inspect_table_cell(0U, 1U, 1U);
    REQUIRE(after_inserted_body.has_value());
    CHECK_EQ(after_inserted_body->text, "");

    CHECK_EQ(run_cli({"remove-template-table-column",
                      source.string(),
                      "0",
                      "0",
                      "1",
                      "--part",
                      "body",
                      "--output",
                      removed.string(),
                      "--json"},
                     remove_output),
             0);
    const auto remove_json = read_text_file(remove_output);
    CHECK_NE(remove_json.find("\"command\":\"remove-template-table-column\""),
             std::string::npos);
    CHECK_NE(remove_json.find("\"part\":\"body\""), std::string::npos);
    CHECK_NE(remove_json.find("\"entry_name\":\"word/document.xml\""),
             std::string::npos);
    CHECK_NE(remove_json.find("\"column_index\":1"), std::string::npos);

    featherdoc::Document reopened_removed(removed);
    REQUIRE_FALSE(reopened_removed.open());
    auto removed_template = reopened_removed.body_template();
    REQUIRE(static_cast<bool>(removed_template));
    const auto removed_table = removed_template.inspect_table(0U);
    REQUIRE(removed_table.has_value());
    CHECK_EQ(removed_table->column_count, 2U);
    const auto removed_first_header =
        removed_template.inspect_table_cell(0U, 0U, 0U);
    REQUIRE(removed_first_header.has_value());
    CHECK_EQ(removed_first_header->text, "b00");
    const auto removed_second_header =
        removed_template.inspect_table_cell(0U, 0U, 1U);
    REQUIRE(removed_second_header.has_value());
    CHECK_EQ(removed_second_header->text, "b02");
    const auto removed_second_body =
        removed_template.inspect_table_cell(0U, 1U, 1U);
    REQUIRE(removed_second_body.has_value());
    CHECK_EQ(removed_second_body->text, "b12");

    remove_if_exists(source);
    remove_if_exists(inserted_before);
    remove_if_exists(inserted_after);
    remove_if_exists(removed);
    remove_if_exists(before_output);
    remove_if_exists(after_output);
    remove_if_exists(remove_output);
}

TEST_CASE("cli template table column commands report parse and mutate errors") {
    const fs::path working_directory = fs::current_path();
    const fs::path parse_source =
        working_directory / "cli_template_table_column_parse_source.docx";
    const fs::path merged_source =
        working_directory / "cli_template_table_column_merge_source.docx";
    const fs::path single_column_source =
        working_directory / "cli_template_table_column_single_source.docx";
    const fs::path parse_output =
        working_directory / "cli_template_table_column_parse.json";
    const fs::path merge_error_output =
        working_directory / "cli_template_table_column_merge_error.json";
    const fs::path remove_error_output =
        working_directory / "cli_template_table_column_remove_error.json";

    remove_if_exists(parse_source);
    remove_if_exists(merged_source);
    remove_if_exists(single_column_source);
    remove_if_exists(parse_output);
    remove_if_exists(merge_error_output);
    remove_if_exists(remove_error_output);

    create_cli_template_table_column_fixture(parse_source);

    CHECK_EQ(run_cli({"insert-template-table-column-before",
                      parse_source.string(),
                      "0",
                      "0",
                      "1",
                      "--part",
                      "section-header",
                      "--json"},
                     parse_output),
             2);
    CHECK_EQ(
        read_text_file(parse_output),
        std::string{
            "{\"command\":\"insert-template-table-column-before\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"section-header/section-footer "
            "mutation requires --section <index>\"}\n"});

    featherdoc::Document merged_document(merged_source);
    REQUIRE_FALSE(merged_document.create_empty());
    auto &merged_header = merged_document.ensure_section_header_paragraphs(0U);
    REQUIRE(merged_header.has_next());
    REQUIRE(merged_header.set_text("Section header merged column fixture"));
    auto merged_template = merged_document.section_header_template(0U);
    REQUIRE(static_cast<bool>(merged_template));
    auto merged_table = merged_template.append_table(2U, 3U);
    configure_cli_template_table_fixture(merged_table, {1600U, 1800U, 2200U},
                                         {{"h00", "h01", "h02"},
                                          {"m00", "m01", "m02"}});
    auto merged_row = merged_table.rows();
    REQUIRE(merged_row.has_next());
    merged_row.next();
    REQUIRE(merged_row.has_next());
    auto merged_cell = merged_row.cells();
    REQUIRE(merged_cell.has_next());
    REQUIRE(merged_cell.merge_right(1U));
    REQUIRE_FALSE(merged_document.save());

    CHECK_EQ(run_cli({"insert-template-table-column-before",
                      merged_source.string(),
                      "0",
                      "0",
                      "1",
                      "--part",
                      "section-header",
                      "--section",
                      "0",
                      "--json"},
                     merge_error_output),
             1);
    const auto merge_error_json = read_text_file(merge_error_output);
    CHECK_NE(
        merge_error_json.find("\"command\":\"insert-template-table-column-before\""),
        std::string::npos);
    CHECK_NE(merge_error_json.find("\"stage\":\"mutate\""), std::string::npos);
    CHECK_NE(merge_error_json.find("\"entry\":\"word/header1.xml\""),
             std::string::npos);
    CHECK_NE(
        merge_error_json.find(
            "\"detail\":\"cannot insert a column before cell index '1' at row "
            "index '0' in table index '0' because the insertion boundary "
            "intersects a horizontal merge span\""),
        std::string::npos);

    featherdoc::Document single_column_document(single_column_source);
    REQUIRE_FALSE(single_column_document.create_empty());
    auto body_template = single_column_document.body_template();
    REQUIRE(static_cast<bool>(body_template));
    auto body_paragraph = body_template.paragraphs();
    REQUIRE(body_paragraph.has_next());
    REQUIRE(body_paragraph.set_text("Single-column body fixture"));
    auto single_table = body_template.append_table(1U, 1U);
    configure_cli_template_table_fixture(single_table, {2400U}, {{"only"}});
    REQUIRE_FALSE(single_column_document.save());

    CHECK_EQ(run_cli({"remove-template-table-column",
                      single_column_source.string(),
                      "0",
                      "0",
                      "0",
                      "--part",
                      "body",
                      "--json"},
                     remove_error_output),
             1);
    const auto remove_error_json = read_text_file(remove_error_output);
    CHECK_NE(remove_error_json.find("\"command\":\"remove-template-table-column\""),
             std::string::npos);
    CHECK_NE(remove_error_json.find("\"stage\":\"mutate\""), std::string::npos);
    CHECK_NE(remove_error_json.find("\"entry\":\"word/document.xml\""),
             std::string::npos);
    CHECK_NE(
        remove_error_json.find(
            "\"detail\":\"cannot remove the last column from table index '0'\""),
        std::string::npos);

    remove_if_exists(parse_source);
    remove_if_exists(merged_source);
    remove_if_exists(single_column_source);
    remove_if_exists(parse_output);
    remove_if_exists(merge_error_output);
    remove_if_exists(remove_error_output);
}

TEST_CASE("cli selector-based template table cell inspection resolves the requested table") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_template_table_selector_cells_source.docx";
    const fs::path cell_output =
        working_directory / "cli_template_table_selector_cells_output.json";
    const fs::path parse_output =
        working_directory / "cli_template_table_selector_cells_parse.json";

    remove_if_exists(source);
    remove_if_exists(cell_output);
    remove_if_exists(parse_output);

    create_cli_template_table_selector_fixture(source);

    CHECK_EQ(run_cli({"inspect-template-table-cells",
                      source.string(),
                      "--after-text",
                      "selector target table",
                      "--header-cell",
                      "Region",
                      "--header-cell",
                      "Qty",
                      "--header-cell",
                      "Amount",
                      "--occurrence",
                      "1",
                      "--row",
                      "1",
                      "--cell",
                      "2",
                      "--json"},
                     cell_output),
             0);
    const auto cell_json = read_text_file(cell_output);
    CHECK_NE(cell_json.find("\"table_index\":2"), std::string::npos);
    CHECK_NE(cell_json.find("\"row_index\":1"), std::string::npos);
    CHECK_NE(cell_json.find("\"cell_index\":2"), std::string::npos);
    CHECK_NE(cell_json.find("\"text\":\"24\""), std::string::npos);

    CHECK_EQ(run_cli({"inspect-template-table-cells",
                      source.string(),
                      "--after-text",
                      "selector target table",
                      "--header-row",
                      "1",
                      "--row",
                      "1",
                      "--json"},
                     parse_output),
             2);
    CHECK_EQ(
        read_text_file(parse_output),
        std::string{
            "{\"command\":\"inspect-template-table-cells\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"--header-row requires at least "
            "one --header-cell\"}\n"});

    remove_if_exists(source);
    remove_if_exists(cell_output);
    remove_if_exists(parse_output);
}

TEST_CASE(
    "cli selector-based template table merge and unmerge commands target the requested table") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_template_table_selector_merge_source.docx";
    const fs::path merged =
        working_directory / "cli_template_table_selector_merged.docx";
    const fs::path merged_multiline =
        working_directory / "cli_template_table_selector_merged_multiline.docx";
    const fs::path unmerged =
        working_directory / "cli_template_table_selector_unmerged.docx";
    const fs::path unmerged_multiline =
        working_directory / "cli_template_table_selector_unmerged_multiline.docx";
    const fs::path merge_output =
        working_directory / "cli_template_table_selector_merge_output.json";
    const fs::path grid_inspect_output =
        working_directory / "cli_template_table_selector_grid_inspect_output.json";
    const fs::path merge_multiline_output =
        working_directory / "cli_template_table_selector_merge_multiline_output.json";
    const fs::path unmerge_output =
        working_directory / "cli_template_table_selector_unmerge_output.json";
    const fs::path unmerge_multiline_output =
        working_directory / "cli_template_table_selector_unmerge_multiline_output.json";
    const fs::path parse_output =
        working_directory / "cli_template_table_selector_merge_parse.json";

    remove_if_exists(source);
    remove_if_exists(merged);
    remove_if_exists(merged_multiline);
    remove_if_exists(unmerged);
    remove_if_exists(unmerged_multiline);
    remove_if_exists(merge_output);
    remove_if_exists(grid_inspect_output);
    remove_if_exists(merge_multiline_output);
    remove_if_exists(unmerge_output);
    remove_if_exists(unmerge_multiline_output);
    remove_if_exists(parse_output);

    create_cli_template_table_selector_fixture(source);

    CHECK_EQ(run_cli({"merge-template-table-cells",
                      source.string(),
                      "--after-text",
                      "selector target table",
                      "--header-cell",
                      "Region",
                      "--header-cell",
                      "Qty",
                      "--header-cell",
                      "Amount",
                      "--occurrence",
                      "1",
                      "1",
                      "0",
                      "--direction",
                      "right",
                      "--output",
                      merged.string(),
                      "--json"},
                     merge_output),
             0);
    const auto merge_json = read_text_file(merge_output);
    CHECK_NE(merge_json.find("\"command\":\"merge-template-table-cells\""),
             std::string::npos);
    CHECK_NE(merge_json.find("\"table_index\":2"), std::string::npos);
    CHECK_NE(merge_json.find("\"row_index\":1"), std::string::npos);
    CHECK_NE(merge_json.find("\"cell_index\":0"), std::string::npos);
    CHECK_NE(merge_json.find("\"direction\":\"right\""), std::string::npos);
    CHECK_NE(merge_json.find("\"count\":1"), std::string::npos);

    featherdoc::Document reopened_merged(merged);
    REQUIRE_FALSE(reopened_merged.open());
    auto merged_body = reopened_merged.body_template();
    REQUIRE(static_cast<bool>(merged_body));
    const auto preserved_first_target = merged_body.inspect_table_cell(0U, 1U, 0U);
    REQUIRE(preserved_first_target.has_value());
    CHECK_EQ(preserved_first_target->text, "North");
    CHECK_EQ(preserved_first_target->column_span, 1U);
    const auto merged_anchor = merged_body.inspect_table_cell(2U, 1U, 0U);
    REQUIRE(merged_anchor.has_value());
    CHECK_EQ(merged_anchor->text, "South");
    CHECK_EQ(merged_anchor->column_span, 2U);

    CHECK_EQ(run_cli({"inspect-template-table-cells",
                      merged.string(),
                      "--after-text",
                      "selector target table",
                      "--header-cell",
                      "Region",
                      "--header-cell",
                      "Qty",
                      "--header-cell",
                      "Amount",
                      "--occurrence",
                      "1",
                      "--row",
                      "1",
                      "--grid-column",
                      "1",
                      "--json"},
                     grid_inspect_output),
             0);
    const auto grid_inspect_json = read_text_file(grid_inspect_output);
    CHECK_NE(grid_inspect_json.find("\"table_index\":2"), std::string::npos);
    CHECK_NE(grid_inspect_json.find("\"row_index\":1"), std::string::npos);
    CHECK_NE(grid_inspect_json.find("\"cell_index\":0"), std::string::npos);
    CHECK_NE(grid_inspect_json.find("\"column_index\":0"), std::string::npos);
    CHECK_NE(grid_inspect_json.find("\"column_span\":2"), std::string::npos);
    CHECK_NE(grid_inspect_json.find("\"text\":\"South\""), std::string::npos);

    CHECK_EQ(run_cli({"set-template-table-cell-text",
                      merged.string(),
                      "--after-text",
                      "selector target table",
                      "--header-cell",
                      "Region",
                      "--header-cell",
                      "Qty",
                      "--header-cell",
                      "Amount",
                      "--occurrence",
                      "1",
                      "1",
                      "--grid-column",
                      "1",
                      "--text",
                      "South\npending approval",
                      "--output",
                      merged_multiline.string(),
                      "--json"},
                     merge_multiline_output),
             0);
    const auto merged_multiline_json = read_text_file(merge_multiline_output);
    CHECK_NE(merged_multiline_json.find("\"command\":\"set-template-table-cell-text\""),
             std::string::npos);
    CHECK_NE(merged_multiline_json.find("\"table_index\":2"), std::string::npos);
    CHECK_NE(merged_multiline_json.find("\"row_index\":1"), std::string::npos);
    CHECK_NE(merged_multiline_json.find("\"grid_column\":1"), std::string::npos);
    CHECK_NE(merged_multiline_json.find("\"cell_index\":0"), std::string::npos);
    CHECK_NE(merged_multiline_json.find("\"column_index\":0"), std::string::npos);
    CHECK_NE(merged_multiline_json.find("\"column_span\":2"), std::string::npos);

    featherdoc::Document reopened_merged_multiline(merged_multiline);
    REQUIRE_FALSE(reopened_merged_multiline.open());
    auto merged_multiline_body = reopened_merged_multiline.body_template();
    REQUIRE(static_cast<bool>(merged_multiline_body));
    const auto merged_multiline_anchor =
        merged_multiline_body.inspect_table_cell(2U, 1U, 0U);
    REQUIRE(merged_multiline_anchor.has_value());
    CHECK_EQ(merged_multiline_anchor->text, "South\npending approval");
    CHECK_EQ(merged_multiline_anchor->column_span, 2U);

    CHECK_EQ(run_cli({"unmerge-template-table-cells",
                      merged.string(),
                      "--after-text",
                      "selector target table",
                      "--header-cell",
                      "Region",
                      "--header-cell",
                      "Qty",
                      "--header-cell",
                      "Amount",
                      "--occurrence",
                      "1",
                      "1",
                      "0",
                      "--direction",
                      "right",
                      "--output",
                      unmerged.string(),
                      "--json"},
                     unmerge_output),
             0);
    const auto unmerge_json = read_text_file(unmerge_output);
    CHECK_NE(unmerge_json.find("\"command\":\"unmerge-template-table-cells\""),
             std::string::npos);
    CHECK_NE(unmerge_json.find("\"table_index\":2"), std::string::npos);
    CHECK_NE(unmerge_json.find("\"row_index\":1"), std::string::npos);
    CHECK_NE(unmerge_json.find("\"cell_index\":0"), std::string::npos);
    CHECK_NE(unmerge_json.find("\"direction\":\"right\""), std::string::npos);

    featherdoc::Document reopened_unmerged(unmerged);
    REQUIRE_FALSE(reopened_unmerged.open());
    auto unmerged_body = reopened_unmerged.body_template();
    REQUIRE(static_cast<bool>(unmerged_body));
    const auto restored_anchor = unmerged_body.inspect_table_cell(2U, 1U, 0U);
    REQUIRE(restored_anchor.has_value());
    CHECK_EQ(restored_anchor->text, "South");
    CHECK_EQ(restored_anchor->column_span, 1U);
    const auto restored_cell = unmerged_body.inspect_table_cell(2U, 1U, 1U);
    REQUIRE(restored_cell.has_value());
    CHECK_EQ(restored_cell->text, "");
    CHECK_EQ(restored_cell->column_span, 1U);

    CHECK_EQ(run_cli({"set-template-table-cell-text",
                      unmerged.string(),
                      "--after-text",
                      "selector target table",
                      "--header-cell",
                      "Region",
                      "--header-cell",
                      "Qty",
                      "--header-cell",
                      "Amount",
                      "--occurrence",
                      "1",
                      "1",
                      "1",
                      "--text",
                      "9 units\nhold",
                      "--output",
                      unmerged_multiline.string(),
                      "--json"},
                     unmerge_multiline_output),
             0);
    const auto unmerged_multiline_json = read_text_file(unmerge_multiline_output);
    CHECK_NE(unmerged_multiline_json.find("\"command\":\"set-template-table-cell-text\""),
             std::string::npos);
    CHECK_NE(unmerged_multiline_json.find("\"table_index\":2"), std::string::npos);
    CHECK_NE(unmerged_multiline_json.find("\"row_index\":1"), std::string::npos);
    CHECK_NE(unmerged_multiline_json.find("\"cell_index\":1"), std::string::npos);

    featherdoc::Document reopened_unmerged_multiline(unmerged_multiline);
    REQUIRE_FALSE(reopened_unmerged_multiline.open());
    auto unmerged_multiline_body = reopened_unmerged_multiline.body_template();
    REQUIRE(static_cast<bool>(unmerged_multiline_body));
    const auto restored_multiline_anchor =
        unmerged_multiline_body.inspect_table_cell(2U, 1U, 0U);
    REQUIRE(restored_multiline_anchor.has_value());
    CHECK_EQ(restored_multiline_anchor->text, "South");
    CHECK_EQ(restored_multiline_anchor->column_span, 1U);
    const auto restored_multiline_cell =
        unmerged_multiline_body.inspect_table_cell(2U, 1U, 1U);
    REQUIRE(restored_multiline_cell.has_value());
    CHECK_EQ(restored_multiline_cell->text, "9 units\nhold");
    CHECK_EQ(restored_multiline_cell->column_span, 1U);

    CHECK_EQ(run_cli({"merge-template-table-cells",
                      source.string(),
                      "--after-text",
                      "selector target table",
                      "--direction",
                      "right",
                      "--json"},
                     parse_output),
             2);
    CHECK_EQ(
        read_text_file(parse_output),
        std::string{
            "{\"command\":\"merge-template-table-cells\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"missing row index after table "
            "selector\"}\n"});

    remove_if_exists(source);
    remove_if_exists(merged);
    remove_if_exists(merged_multiline);
    remove_if_exists(unmerged);
    remove_if_exists(unmerged_multiline);
    remove_if_exists(merge_output);
    remove_if_exists(grid_inspect_output);
    remove_if_exists(merge_multiline_output);
    remove_if_exists(unmerge_output);
    remove_if_exists(unmerge_multiline_output);
    remove_if_exists(parse_output);
}

TEST_CASE("cli template table merge and unmerge commands support bookmark selectors") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_template_table_bookmark_merge_source.docx";
    const fs::path merged =
        working_directory / "cli_template_table_bookmark_merged.docx";
    const fs::path unmerged =
        working_directory / "cli_template_table_bookmark_unmerged.docx";
    const fs::path merge_output =
        working_directory / "cli_template_table_bookmark_merge.json";
    const fs::path unmerge_output =
        working_directory / "cli_template_table_bookmark_unmerge.json";

    remove_if_exists(source);
    remove_if_exists(merged);
    remove_if_exists(unmerged);
    remove_if_exists(merge_output);
    remove_if_exists(unmerge_output);

    create_cli_template_table_bookmark_fixture(source);

    CHECK_EQ(run_cli({"merge-template-table-cells",
                      source.string(),
                      "--bookmark",
                      "target_inside_table",
                      "0",
                      "0",
                      "--direction",
                      "right",
                      "--output",
                      merged.string(),
                      "--json"},
                     merge_output),
             0);
    const auto merge_json = read_text_file(merge_output);
    CHECK_NE(merge_json.find("\"command\":\"merge-template-table-cells\""),
             std::string::npos);
    CHECK_NE(merge_json.find("\"part\":\"body\""), std::string::npos);
    CHECK_NE(merge_json.find("\"table_index\":1"), std::string::npos);
    CHECK_NE(merge_json.find("\"row_index\":0"), std::string::npos);
    CHECK_NE(merge_json.find("\"cell_index\":0"), std::string::npos);
    CHECK_NE(merge_json.find("\"direction\":\"right\""), std::string::npos);
    CHECK_NE(merge_json.find("\"count\":1"), std::string::npos);

    featherdoc::Document reopened_merged(merged);
    REQUIRE_FALSE(reopened_merged.open());
    auto merged_body = reopened_merged.body_template();
    REQUIRE(static_cast<bool>(merged_body));
    const auto merged_cell = merged_body.inspect_table_cell(1U, 0U, 0U);
    REQUIRE(merged_cell.has_value());
    CHECK_EQ(merged_cell->text, "target-00");
    CHECK_EQ(merged_cell->column_span, 2U);

    CHECK_EQ(run_cli({"unmerge-template-table-cells",
                      merged.string(),
                      "--bookmark",
                      "target_before_table",
                      "0",
                      "0",
                      "--direction",
                      "right",
                      "--output",
                      unmerged.string(),
                      "--json"},
                     unmerge_output),
             0);
    const auto unmerge_json = read_text_file(unmerge_output);
    CHECK_NE(unmerge_json.find("\"command\":\"unmerge-template-table-cells\""),
             std::string::npos);
    CHECK_NE(unmerge_json.find("\"part\":\"body\""), std::string::npos);
    CHECK_NE(unmerge_json.find("\"table_index\":1"), std::string::npos);
    CHECK_NE(unmerge_json.find("\"row_index\":0"), std::string::npos);
    CHECK_NE(unmerge_json.find("\"cell_index\":0"), std::string::npos);
    CHECK_NE(unmerge_json.find("\"direction\":\"right\""), std::string::npos);

    featherdoc::Document reopened_unmerged(unmerged);
    REQUIRE_FALSE(reopened_unmerged.open());
    auto unmerged_body = reopened_unmerged.body_template();
    REQUIRE(static_cast<bool>(unmerged_body));
    const auto first_cell = unmerged_body.inspect_table_cell(1U, 0U, 0U);
    REQUIRE(first_cell.has_value());
    CHECK_EQ(first_cell->text, "target-00");
    CHECK_EQ(first_cell->column_span, 1U);
    const auto restored_cell = unmerged_body.inspect_table_cell(1U, 0U, 1U);
    REQUIRE(restored_cell.has_value());
    CHECK_EQ(restored_cell->text, "");

    remove_if_exists(source);
    remove_if_exists(merged);
    remove_if_exists(unmerged);
    remove_if_exists(merge_output);
    remove_if_exists(unmerge_output);
}

TEST_CASE("cli merge-template-table-cells merges template cells and reports errors") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_merge_template_table_cells_source.docx";
    const fs::path merged =
        working_directory / "cli_merge_template_table_cells_output.docx";
    const fs::path success_output =
        working_directory / "cli_merge_template_table_cells_success.json";
    const fs::path parse_output =
        working_directory / "cli_merge_template_table_cells_parse.json";
    const fs::path mutate_output =
        working_directory / "cli_merge_template_table_cells_error.json";

    remove_if_exists(source);
    remove_if_exists(merged);
    remove_if_exists(success_output);
    remove_if_exists(parse_output);
    remove_if_exists(mutate_output);

    create_cli_template_table_merge_unmerge_fixture(source);

    CHECK_EQ(run_cli({"merge-template-table-cells",
                      source.string(),
                      "0",
                      "0",
                      "0",
                      "--part",
                      "header",
                      "--index",
                      "0",
                      "--direction",
                      "right",
                      "--output",
                      merged.string(),
                      "--json"},
                     success_output),
             0);
    const auto success_json = read_text_file(success_output);
    CHECK_NE(success_json.find("\"command\":\"merge-template-table-cells\""),
             std::string::npos);
    CHECK_NE(success_json.find("\"part\":\"header\""), std::string::npos);
    CHECK_NE(success_json.find("\"part_index\":0"), std::string::npos);
    CHECK_NE(success_json.find("\"entry_name\":\"word/header1.xml\""),
             std::string::npos);
    CHECK_NE(success_json.find("\"table_index\":0"), std::string::npos);
    CHECK_NE(success_json.find("\"row_index\":0"), std::string::npos);
    CHECK_NE(success_json.find("\"cell_index\":0"), std::string::npos);
    CHECK_NE(success_json.find("\"direction\":\"right\""), std::string::npos);
    CHECK_NE(success_json.find("\"count\":1"), std::string::npos);

    featherdoc::Document reopened(merged);
    REQUIRE_FALSE(reopened.open());
    auto header_template = reopened.header_template(0U);
    REQUIRE(static_cast<bool>(header_template));
    const auto merged_cell = header_template.inspect_table_cell(0U, 0U, 0U);
    REQUIRE(merged_cell.has_value());
    CHECK_EQ(merged_cell->text, "hA");
    CHECK_EQ(merged_cell->column_span, 2U);

    const auto remaining_cell = header_template.inspect_table_cell(0U, 0U, 1U);
    REQUIRE(remaining_cell.has_value());
    CHECK_EQ(remaining_cell->text, "hC");
    CHECK_EQ(remaining_cell->column_index, 2U);

    const auto header_xml = read_docx_entry(merged, "word/header1.xml");
    pugi::xml_document header_doc;
    REQUIRE(header_doc.load_string(header_xml.c_str()));

    const auto table_node = header_doc.child("w:hdr").child("w:tbl");
    REQUIRE(table_node != pugi::xml_node{});
    const auto first_row = table_node.child("w:tr");
    REQUIRE(first_row != pugi::xml_node{});

    std::size_t cell_count = 0U;
    for (auto cell = first_row.child("w:tc"); cell != pugi::xml_node{};
         cell = cell.next_sibling("w:tc")) {
        ++cell_count;
    }
    CHECK_EQ(cell_count, 2U);

    const auto grid_span = first_row.child("w:tc").child("w:tcPr").child("w:gridSpan");
    REQUIRE(grid_span != pugi::xml_node{});
    CHECK_EQ(std::string_view{grid_span.attribute("w:val").value()}, "2");

    CHECK_EQ(run_cli({"merge-template-table-cells",
                      source.string(),
                      "0",
                      "0",
                      "0",
                      "--part",
                      "section-header",
                      "--direction",
                      "right",
                      "--json"},
                     parse_output),
             2);
    CHECK_EQ(
        read_text_file(parse_output),
        std::string{
            "{\"command\":\"merge-template-table-cells\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"section-header/section-footer "
            "mutation requires --section <index>\"}\n"});

    CHECK_EQ(run_cli({"merge-template-table-cells",
                      source.string(),
                      "0",
                      "0",
                      "0",
                      "--part",
                      "header",
                      "--index",
                      "0",
                      "--direction",
                      "right",
                      "--count",
                      "5",
                      "--json"},
                     mutate_output),
             1);
    const auto mutate_error_json = read_text_file(mutate_output);
    CHECK_NE(mutate_error_json.find("\"command\":\"merge-template-table-cells\""),
             std::string::npos);
    CHECK_NE(mutate_error_json.find("\"stage\":\"mutate\""), std::string::npos);
    CHECK_NE(mutate_error_json.find("\"entry\":\"word/header1.xml\""),
             std::string::npos);
    CHECK_NE(
        mutate_error_json.find(
            "\"detail\":\"cell at table index '0', row index '0', and cell "
            "index '0' could not be merged towards 'right' with count '5'\""),
        std::string::npos);

    remove_if_exists(source);
    remove_if_exists(merged);
    remove_if_exists(success_output);
    remove_if_exists(parse_output);
    remove_if_exists(mutate_output);
}

TEST_CASE("cli unmerge-template-table-cells splits template merges and reports errors") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_unmerge_template_table_cells_source.docx";
    const fs::path unmerged =
        working_directory / "cli_unmerge_template_table_cells_output.docx";
    const fs::path direct_footer_unmerged =
        working_directory /
        "cli_unmerge_template_table_cells_direct_footer_output.docx";
    const fs::path success_output =
        working_directory / "cli_unmerge_template_table_cells_success.json";
    const fs::path direct_footer_success_output =
        working_directory /
        "cli_unmerge_template_table_cells_direct_footer_success.json";
    const fs::path footer_success_output =
        working_directory / "cli_unmerge_template_table_cells_footer_success.json";
    const fs::path parse_output =
        working_directory / "cli_unmerge_template_table_cells_parse.json";
    const fs::path mutate_output =
        working_directory / "cli_unmerge_template_table_cells_error.json";

    remove_if_exists(source);
    remove_if_exists(unmerged);
    remove_if_exists(direct_footer_unmerged);
    remove_if_exists(success_output);
    remove_if_exists(direct_footer_success_output);
    remove_if_exists(footer_success_output);
    remove_if_exists(parse_output);
    remove_if_exists(mutate_output);

    create_cli_template_table_merge_unmerge_fixture(source);

    CHECK_EQ(run_cli({"unmerge-template-table-cells",
                      source.string(),
                      "0",
                      "0",
                      "0",
                      "--part",
                      "body",
                      "--direction",
                      "right",
                      "--output",
                      unmerged.string(),
                      "--json"},
                     success_output),
             0);
    const auto success_json = read_text_file(success_output);
    CHECK_NE(success_json.find("\"command\":\"unmerge-template-table-cells\""),
             std::string::npos);
    CHECK_NE(success_json.find("\"part\":\"body\""), std::string::npos);
    CHECK_NE(success_json.find("\"entry_name\":\"word/document.xml\""),
             std::string::npos);
    CHECK_NE(success_json.find("\"table_index\":0"), std::string::npos);
    CHECK_NE(success_json.find("\"row_index\":0"), std::string::npos);
    CHECK_NE(success_json.find("\"cell_index\":0"), std::string::npos);
    CHECK_NE(success_json.find("\"direction\":\"right\""), std::string::npos);

    featherdoc::Document reopened(unmerged);
    REQUIRE_FALSE(reopened.open());
    auto body_template = reopened.body_template();
    REQUIRE(static_cast<bool>(body_template));
    const auto first_cell = body_template.inspect_table_cell(0U, 0U, 0U);
    REQUIRE(first_cell.has_value());
    CHECK_EQ(first_cell->text, "A");
    CHECK_EQ(first_cell->column_span, 1U);
    const auto restored_cell = body_template.inspect_table_cell(0U, 0U, 1U);
    REQUIRE(restored_cell.has_value());
    CHECK_EQ(restored_cell->text, "");
    CHECK_EQ(restored_cell->column_span, 1U);
    const auto last_cell = body_template.inspect_table_cell(0U, 0U, 2U);
    REQUIRE(last_cell.has_value());
    CHECK_EQ(last_cell->text, "C");

    const auto document_xml = read_docx_entry(unmerged, "word/document.xml");
    pugi::xml_document document_xml_doc;
    REQUIRE(document_xml_doc.load_string(document_xml.c_str()));

    const auto body_table_node = document_xml_doc.child("w:document").child("w:body").child("w:tbl");
    REQUIRE(body_table_node != pugi::xml_node{});
    const auto body_row_node = body_table_node.child("w:tr");
    REQUIRE(body_row_node != pugi::xml_node{});

    std::size_t row_cell_count = 0U;
    for (auto cell = body_row_node.child("w:tc"); cell != pugi::xml_node{};
         cell = cell.next_sibling("w:tc")) {
        ++row_cell_count;
    }
    CHECK_EQ(row_cell_count, 3U);
    CHECK(body_row_node.child("w:tc").child("w:tcPr").child("w:gridSpan") ==
          pugi::xml_node{});

    CHECK_EQ(run_cli({"unmerge-template-table-cells",
                      source.string(),
                      "1",
                      "1",
                      "0",
                      "--part",
                      "footer",
                      "--index",
                      "0",
                      "--direction",
                      "down",
                      "--output",
                      direct_footer_unmerged.string(),
                      "--json"},
                     direct_footer_success_output),
             0);
    const auto direct_footer_success_json =
        read_text_file(direct_footer_success_output);
    CHECK_NE(direct_footer_success_json.find(
                 "\"command\":\"unmerge-template-table-cells\""),
             std::string::npos);
    CHECK_NE(direct_footer_success_json.find("\"part\":\"footer\""),
             std::string::npos);
    CHECK_NE(direct_footer_success_json.find("\"part_index\":0"),
             std::string::npos);
    CHECK_NE(direct_footer_success_json.find("\"entry_name\":\"word/footer1.xml\""),
             std::string::npos);
    CHECK_NE(direct_footer_success_json.find("\"table_index\":1"),
             std::string::npos);
    CHECK_NE(direct_footer_success_json.find("\"direction\":\"down\""),
             std::string::npos);

    featherdoc::Document direct_footer_reopened(direct_footer_unmerged);
    REQUIRE_FALSE(direct_footer_reopened.open());
    auto direct_footer_template = direct_footer_reopened.footer_template(0U);
    REQUIRE(static_cast<bool>(direct_footer_template));
    const auto direct_footer_top =
        direct_footer_template.inspect_table_cell(1U, 0U, 0U);
    REQUIRE(direct_footer_top.has_value());
    CHECK_EQ(direct_footer_top->text, "dfA");
    const auto direct_footer_middle =
        direct_footer_template.inspect_table_cell(1U, 1U, 0U);
    REQUIRE(direct_footer_middle.has_value());
    CHECK_EQ(direct_footer_middle->text, "");
    const auto direct_footer_bottom =
        direct_footer_template.inspect_table_cell(1U, 2U, 0U);
    REQUIRE(direct_footer_bottom.has_value());
    CHECK_EQ(direct_footer_bottom->text, "");

    const auto direct_footer_xml =
        read_docx_entry(direct_footer_unmerged, "word/footer1.xml");
    pugi::xml_document direct_footer_doc;
    REQUIRE(direct_footer_doc.load_string(direct_footer_xml.c_str()));

    auto direct_footer_table_node =
        direct_footer_doc.child("w:ftr").child("w:tbl");
    REQUIRE(direct_footer_table_node != pugi::xml_node{});
    direct_footer_table_node = direct_footer_table_node.next_sibling("w:tbl");
    REQUIRE(direct_footer_table_node != pugi::xml_node{});
    for (auto direct_footer_row = direct_footer_table_node.child("w:tr");
         direct_footer_row != pugi::xml_node{};
         direct_footer_row = direct_footer_row.next_sibling("w:tr")) {
        const auto cell = direct_footer_row.child("w:tc");
        REQUIRE(cell != pugi::xml_node{});
        CHECK(cell.child("w:tcPr").child("w:vMerge") == pugi::xml_node{});
    }

    CHECK_EQ(run_cli({"unmerge-template-table-cells",
                      source.string(),
                      "0",
                      "1",
                      "0",
                      "--part",
                      "section-footer",
                      "--direction",
                      "down",
                      "--json"},
                     parse_output),
             2);
    CHECK_EQ(
        read_text_file(parse_output),
        std::string{
            "{\"command\":\"unmerge-template-table-cells\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"section-header/section-footer "
            "mutation requires --section <index>\"}\n"});

    CHECK_EQ(run_cli({"unmerge-template-table-cells",
                      source.string(),
                      "0",
                      "1",
                      "0",
                      "--part",
                      "section-footer",
                      "--section",
                      "0",
                      "--direction",
                      "down",
                      "--json"},
                     footer_success_output),
             0);
    const auto footer_success_json = read_text_file(footer_success_output);
    CHECK_NE(footer_success_json.find("\"part\":\"section-footer\""),
             std::string::npos);
    CHECK_NE(footer_success_json.find("\"section\":0"), std::string::npos);
    CHECK_NE(footer_success_json.find("\"entry_name\":\"word/footer1.xml\""),
             std::string::npos);
    CHECK_NE(footer_success_json.find("\"direction\":\"down\""),
             std::string::npos);

    featherdoc::Document footer_reopened(source);
    REQUIRE_FALSE(footer_reopened.open());
    auto footer_template = footer_reopened.section_footer_template(0U);
    REQUIRE(static_cast<bool>(footer_template));
    const auto footer_top = footer_template.inspect_table_cell(0U, 0U, 0U);
    REQUIRE(footer_top.has_value());
    CHECK_EQ(footer_top->text, "fA");
    const auto footer_middle = footer_template.inspect_table_cell(0U, 1U, 0U);
    REQUIRE(footer_middle.has_value());
    CHECK_EQ(footer_middle->text, "");
    const auto footer_bottom = footer_template.inspect_table_cell(0U, 2U, 0U);
    REQUIRE(footer_bottom.has_value());
    CHECK_EQ(footer_bottom->text, "");

    const auto footer_xml = read_docx_entry(source, "word/footer1.xml");
    pugi::xml_document footer_doc;
    REQUIRE(footer_doc.load_string(footer_xml.c_str()));

    const auto footer_table_node = footer_doc.child("w:ftr").child("w:tbl");
    REQUIRE(footer_table_node != pugi::xml_node{});
    auto footer_row = footer_table_node.child("w:tr");
    for (; footer_row != pugi::xml_node{}; footer_row = footer_row.next_sibling("w:tr")) {
        const auto cell = footer_row.child("w:tc");
        REQUIRE(cell != pugi::xml_node{});
        CHECK(cell.child("w:tcPr").child("w:vMerge") == pugi::xml_node{});
    }

    CHECK_EQ(run_cli({"unmerge-template-table-cells",
                      source.string(),
                      "0",
                      "1",
                      "0",
                      "--part",
                      "section-footer",
                      "--section",
                      "0",
                      "--direction",
                      "down",
                      "--json"},
                     mutate_output),
             1);
    const auto mutate_error_json = read_text_file(mutate_output);
    CHECK_NE(mutate_error_json.find("\"command\":\"unmerge-template-table-cells\""),
             std::string::npos);
    CHECK_NE(mutate_error_json.find("\"stage\":\"mutate\""), std::string::npos);
    CHECK_NE(mutate_error_json.find("\"entry\":\"word/footer1.xml\""),
             std::string::npos);
    CHECK_NE(
        mutate_error_json.find(
            "\"detail\":\"cell at table index '0', row index '1', and cell "
            "index '0' could not be unmerged towards 'down'\""),
        std::string::npos);

    remove_if_exists(source);
    remove_if_exists(unmerged);
    remove_if_exists(direct_footer_unmerged);
    remove_if_exists(success_output);
    remove_if_exists(direct_footer_success_output);
    remove_if_exists(footer_success_output);
    remove_if_exists(parse_output);
    remove_if_exists(mutate_output);
}

} // namespace
