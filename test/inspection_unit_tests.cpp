#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "basic_docx_archive_test_support.hpp"
#include "basic_document_xml_test_support.hpp"

#include <featherdoc.hpp>
TEST_CASE("inspect paragraph runs returns run style and language metadata") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "inspect_paragraph_runs_roundtrip.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto paragraph = doc.paragraphs();
    REQUIRE(paragraph.has_next());

    auto styled_run = paragraph.add_run("styled");
    REQUIRE(styled_run.has_next());
    CHECK(doc.set_run_style(styled_run, "Strong"));
    CHECK(styled_run.set_font_family("Segoe UI"));
    CHECK(styled_run.set_east_asia_font_family("Microsoft YaHei"));
    CHECK(styled_run.set_language("en-US"));
    CHECK(styled_run.set_east_asia_language("zh-CN"));
    CHECK(styled_run.set_bidi_language("ar-SA"));
    CHECK(styled_run.set_rtl());
    auto strike_run = styled_run.insert_run_after(
        " strike", featherdoc::formatting_flag::strikethrough);
    REQUIRE(strike_run.has_next());
    REQUIRE(strike_run.strikethrough().has_value());
    CHECK(*strike_run.strikethrough());
    auto superscript_run = strike_run.insert_run_after(
        " super", featherdoc::formatting_flag::superscript);
    REQUIRE(superscript_run.has_next());
    REQUIRE(superscript_run.superscript().has_value());
    CHECK(*superscript_run.superscript());
    auto subscript_run = superscript_run.insert_run_after(
        " sub", featherdoc::formatting_flag::subscript);
    REQUIRE(subscript_run.has_next());
    REQUIRE(subscript_run.subscript().has_value());
    CHECK(*subscript_run.subscript());

    auto plain_run = paragraph.add_run(" plain");
    REQUIRE(plain_run.has_next());

    auto second_paragraph = paragraph.insert_paragraph_after("second paragraph");
    REQUIRE(second_paragraph.has_next());

    const auto first_runs = doc.inspect_paragraph_runs(0);
    REQUIRE(first_runs.size() == 5U);

    CHECK_EQ(first_runs[0].index, 0U);
    REQUIRE(first_runs[0].style_id.has_value());
    CHECK_EQ(*first_runs[0].style_id, "Strong");
    REQUIRE(first_runs[0].font_family.has_value());
    CHECK_EQ(*first_runs[0].font_family, "Segoe UI");
    REQUIRE(first_runs[0].east_asia_font_family.has_value());
    CHECK_EQ(*first_runs[0].east_asia_font_family, "Microsoft YaHei");
    REQUIRE(first_runs[0].language.has_value());
    CHECK_EQ(*first_runs[0].language, "en-US");
    REQUIRE(first_runs[0].east_asia_language.has_value());
    CHECK_EQ(*first_runs[0].east_asia_language, "zh-CN");
    REQUIRE(first_runs[0].bidi_language.has_value());
    CHECK_EQ(*first_runs[0].bidi_language, "ar-SA");
    REQUIRE(first_runs[0].rtl.has_value());
    CHECK(*first_runs[0].rtl);
    CHECK_EQ(first_runs[0].text, "styled");

    CHECK_EQ(first_runs[1].index, 1U);
    REQUIRE(first_runs[1].strikethrough.has_value());
    CHECK(*first_runs[1].strikethrough);
    CHECK_FALSE(first_runs[1].superscript.has_value());
    CHECK_FALSE(first_runs[1].subscript.has_value());
    CHECK_EQ(first_runs[1].text, " strike");

    CHECK_EQ(first_runs[2].index, 2U);
    CHECK_FALSE(first_runs[2].strikethrough.has_value());
    REQUIRE(first_runs[2].superscript.has_value());
    CHECK(*first_runs[2].superscript);
    REQUIRE(first_runs[2].subscript.has_value());
    CHECK_FALSE(*first_runs[2].subscript);
    CHECK_EQ(first_runs[2].text, " super");

    CHECK_EQ(first_runs[3].index, 3U);
    CHECK_FALSE(first_runs[3].strikethrough.has_value());
    REQUIRE(first_runs[3].superscript.has_value());
    CHECK_FALSE(*first_runs[3].superscript);
    REQUIRE(first_runs[3].subscript.has_value());
    CHECK(*first_runs[3].subscript);
    CHECK_EQ(first_runs[3].text, " sub");

    CHECK_EQ(first_runs[4].index, 4U);
    CHECK_FALSE(first_runs[4].style_id.has_value());
    CHECK_FALSE(first_runs[4].font_family.has_value());
    CHECK_FALSE(first_runs[4].language.has_value());
    CHECK_FALSE(first_runs[4].rtl.has_value());
    CHECK_FALSE(first_runs[4].strikethrough.has_value());
    CHECK_FALSE(first_runs[4].superscript.has_value());
    CHECK_FALSE(first_runs[4].subscript.has_value());
    CHECK_EQ(first_runs[4].text, " plain");

    const auto second_run = doc.inspect_paragraph_run(1U, 0U);
    REQUIRE(second_run.has_value());
    CHECK_EQ(second_run->index, 0U);
    CHECK_EQ(second_run->text, "second paragraph");

    CHECK(doc.inspect_paragraph_runs(9U).empty());
    CHECK_FALSE(doc.inspect_paragraph_run(0U, 9U).has_value());

    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());

    const auto reopened_runs = reopened.inspect_paragraph_runs(0U);
    REQUIRE(reopened_runs.size() == 5U);
    REQUIRE(reopened_runs[0].style_id.has_value());
    CHECK_EQ(*reopened_runs[0].style_id, "Strong");
    REQUIRE(reopened_runs[0].language.has_value());
    CHECK_EQ(*reopened_runs[0].language, "en-US");
    REQUIRE(reopened_runs[0].east_asia_language.has_value());
    CHECK_EQ(*reopened_runs[0].east_asia_language, "zh-CN");
    REQUIRE(reopened_runs[0].bidi_language.has_value());
    CHECK_EQ(*reopened_runs[0].bidi_language, "ar-SA");
    REQUIRE(reopened_runs[0].rtl.has_value());
    CHECK(*reopened_runs[0].rtl);
    REQUIRE(reopened_runs[1].strikethrough.has_value());
    CHECK(*reopened_runs[1].strikethrough);
    REQUIRE(reopened_runs[2].superscript.has_value());
    CHECK(*reopened_runs[2].superscript);
    REQUIRE(reopened_runs[3].subscript.has_value());
    CHECK(*reopened_runs[3].subscript);
    CHECK_EQ(reopened_runs[4].text, " plain");

    fs::remove(target);
}

TEST_CASE("inspect paragraphs returns style bidi numbering run count and text metadata") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "inspect_paragraphs_roundtrip.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto numbering_definition = featherdoc::numbering_definition{};
    numbering_definition.name = "ParagraphInspectOutline";
    numbering_definition.levels = {
        featherdoc::numbering_level_definition{
            featherdoc::list_kind::decimal, 1U, 0U, "%1."},
    };

    const auto numbering_id = doc.ensure_numbering_definition(numbering_definition);
    REQUIRE(numbering_id.has_value());

    auto first = doc.paragraphs();
    REQUIRE(first.has_next());
    CHECK(doc.set_paragraph_style(first, "Heading1"));
    CHECK(first.set_bidi());
    REQUIRE(first.add_run("alpha").has_next());
    REQUIRE(first.add_run(" beta").has_next());

    auto second = first.insert_paragraph_after("item");
    REQUIRE(second.has_next());
    CHECK(doc.set_paragraph_numbering(second, *numbering_id));

    auto third = second.insert_paragraph_after("plain");
    REQUIRE(third.has_next());

    const auto paragraphs = doc.inspect_paragraphs();
    REQUIRE(paragraphs.size() == 3U);

    CHECK_EQ(paragraphs[0].index, 0U);
    REQUIRE(paragraphs[0].style_id.has_value());
    CHECK_EQ(*paragraphs[0].style_id, "Heading1");
    REQUIRE(paragraphs[0].bidi.has_value());
    CHECK(*paragraphs[0].bidi);
    CHECK_FALSE(paragraphs[0].numbering.has_value());
    CHECK_EQ(paragraphs[0].run_count, 2U);
    CHECK_EQ(paragraphs[0].text, "alpha beta");

    CHECK_EQ(paragraphs[1].index, 1U);
    CHECK_FALSE(paragraphs[1].style_id.has_value());
    CHECK_FALSE(paragraphs[1].bidi.has_value());
    REQUIRE(paragraphs[1].numbering.has_value());
    REQUIRE(paragraphs[1].numbering->num_id.has_value());
    CHECK_EQ(paragraphs[1].numbering->level, std::optional<std::uint32_t>{0U});
    REQUIRE(paragraphs[1].numbering->definition_id.has_value());
    CHECK_EQ(*paragraphs[1].numbering->definition_id, *numbering_id);
    REQUIRE(paragraphs[1].numbering->definition_name.has_value());
    CHECK_EQ(*paragraphs[1].numbering->definition_name, "ParagraphInspectOutline");
    CHECK_EQ(paragraphs[1].run_count, 1U);
    CHECK_EQ(paragraphs[1].text, "item");

    CHECK_EQ(paragraphs[2].index, 2U);
    CHECK_FALSE(paragraphs[2].style_id.has_value());
    CHECK_FALSE(paragraphs[2].bidi.has_value());
    CHECK_FALSE(paragraphs[2].numbering.has_value());
    CHECK_EQ(paragraphs[2].run_count, 1U);
    CHECK_EQ(paragraphs[2].text, "plain");

    const auto inspected_second = doc.inspect_paragraph(1U);
    REQUIRE(inspected_second.has_value());
    CHECK_EQ(inspected_second->text, "item");
    REQUIRE(inspected_second->numbering.has_value());
    REQUIRE(inspected_second->numbering->definition_id.has_value());
    CHECK_EQ(*inspected_second->numbering->definition_id, *numbering_id);
    CHECK_FALSE(doc.inspect_paragraph(9U).has_value());

    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());

    const auto reopened_paragraphs = reopened.inspect_paragraphs();
    REQUIRE(reopened_paragraphs.size() == 3U);
    REQUIRE(reopened_paragraphs[0].style_id.has_value());
    CHECK_EQ(*reopened_paragraphs[0].style_id, "Heading1");
    REQUIRE(reopened_paragraphs[1].numbering.has_value());
    REQUIRE(reopened_paragraphs[1].numbering->definition_name.has_value());
    CHECK_EQ(*reopened_paragraphs[1].numbering->definition_name,
             "ParagraphInspectOutline");
    CHECK_EQ(reopened_paragraphs[2].text, "plain");

    fs::remove(target);
}

TEST_CASE("paragraph snapshot inspection avoids reopening the source archive") {
    namespace fs = std::filesystem;

    const fs::path target =
        fs::current_path() / "inspect_paragraph_snapshot_no_reopen.docx";
    fs::remove(target);

    featherdoc::Document source(target);
    REQUIRE_FALSE(source.create_empty());
    auto paragraph = source.paragraphs();
    REQUIRE(paragraph.has_next());
    REQUIRE(paragraph.add_run("中文前缀 ").has_next());
    CHECK_EQ(source.append_hyperlink("链接", "https://example.com/zh"), 1U);
    CHECK(source.set_paragraph_list(paragraph,
                                    featherdoc::list_kind::decimal, 0U));
    REQUIRE_FALSE(source.save());

    featherdoc::Document reopened(target);
    REQUIRE_FALSE(reopened.open());
    REQUIRE(fs::remove(target));

    auto options = featherdoc::paragraph_inspection_options{};
    options.resolve_numbering_metadata = false;
    const auto paragraphs = reopened.inspect_paragraphs_with_options(options);
    CHECK_FALSE(reopened.last_error());
    REQUIRE_EQ(paragraphs.size(), 2U);
    CHECK_EQ(paragraphs[0].text, "中文前缀 ");
    REQUIRE(paragraphs[0].numbering.has_value());
    REQUIRE(paragraphs[0].numbering->num_id.has_value());
    CHECK_FALSE(paragraphs[0].numbering->definition_id.has_value());
    CHECK_FALSE(paragraphs[0].numbering->definition_name.has_value());
    CHECK_EQ(paragraphs[1].text, "链接");

    const auto resolved_paragraphs = reopened.inspect_paragraphs();
    CHECK(resolved_paragraphs.empty());
    CHECK(reopened.last_error());
    CHECK_EQ(reopened.last_error().code,
             featherdoc::make_error_code(
                 featherdoc::document_errc::source_archive_changed));
    CHECK_EQ(reopened.last_error().entry_name, "word/numbering.xml");

    const auto resolved_paragraph = reopened.inspect_paragraph(0U);
    CHECK_FALSE(resolved_paragraph.has_value());
    CHECK(reopened.last_error());
    CHECK_EQ(reopened.last_error().code,
             featherdoc::make_error_code(
                 featherdoc::document_errc::source_archive_changed));
}

TEST_CASE("paragraph inspection preserves unresolved numbering errors") {
    namespace fs = std::filesystem;

    const auto target =
        fs::current_path() / "inspect_unresolved_numbering_instance.docx";
    fs::remove(target);

    const auto content_types_xml = std::string{
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">
  <Default Extension="rels" ContentType="application/vnd.openxmlformats-package.relationships+xml"/>
  <Default Extension="xml" ContentType="application/xml"/>
  <Override PartName="/word/document.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml"/>
  <Override PartName="/word/numbering.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.numbering+xml"/>
</Types>
)"};
    const auto document_xml = std::string{
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body><w:p><w:pPr><w:numPr><w:ilvl w:val="0"/><w:numId w:val="99"/></w:numPr></w:pPr><w:r><w:t>orphan list item</w:t></w:r></w:p></w:body>
</w:document>
)"};
    const auto document_relationships_xml = std::string{
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rIdNumbering"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/numbering"
                Target="numbering.xml"/>
</Relationships>
)"};
    const auto numbering_xml = std::string{
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:numbering xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:abstractNum w:abstractNumId="1"><w:lvl w:ilvl="0"><w:numFmt w:val="decimal"/></w:lvl></w:abstractNum>
  <w:num w:numId="1"><w:abstractNumId w:val="1"/></w:num>
</w:numbering>
)"};

    write_test_archive_entries(
        target,
        {{test_content_types_xml_entry, content_types_xml},
         {test_relationships_xml_entry, test_relationships_xml},
         {test_document_xml_entry, document_xml},
         {"word/_rels/document.xml.rels", document_relationships_xml},
         {"word/numbering.xml", numbering_xml}});

    featherdoc::Document document(target);
    REQUIRE_FALSE(document.open());

    CHECK(document.inspect_paragraphs().empty());
    CHECK(document.last_error());
    CHECK_EQ(document.last_error().code,
             std::make_error_code(std::errc::invalid_argument));
    CHECK_NE(document.last_error().detail.find("99"), std::string::npos);

    CHECK_FALSE(document.inspect_paragraph(0U).has_value());
    CHECK(document.last_error());
    CHECK_EQ(document.last_error().code,
             std::make_error_code(std::errc::invalid_argument));

    fs::remove(target);
}

TEST_CASE("template part paragraph snapshot inspection avoids reopening the source archive") {
    namespace fs = std::filesystem;

    const fs::path target =
        fs::current_path() / "inspect_template_part_snapshot_no_reopen.docx";
    fs::remove(target);

    featherdoc::Document source(target);
    REQUIRE_FALSE(source.create_empty());
    auto body = source.body_template();
    REQUIRE(static_cast<bool>(body));
    auto paragraph = body.paragraphs();
    REQUIRE(paragraph.has_next());
    CHECK(paragraph.set_text("template item"));
    CHECK(source.set_paragraph_list(paragraph,
                                    featherdoc::list_kind::decimal, 0U));
    REQUIRE_FALSE(source.save());

    featherdoc::Document reopened(target);
    REQUIRE_FALSE(reopened.open());
    auto reopened_body = reopened.body_template();
    REQUIRE(static_cast<bool>(reopened_body));
    REQUIRE(fs::remove(target));

    auto options = featherdoc::paragraph_inspection_options{};
    options.resolve_numbering_metadata = false;
    const auto paragraphs =
        reopened_body.inspect_paragraphs_with_options(options);
    CHECK_FALSE(reopened.last_error());
    REQUIRE_EQ(paragraphs.size(), 1U);
    CHECK_EQ(paragraphs[0].text, "template item");
    REQUIRE(paragraphs[0].numbering.has_value());
    REQUIRE(paragraphs[0].numbering->num_id.has_value());
    CHECK_FALSE(paragraphs[0].numbering->definition_id.has_value());
    CHECK_FALSE(paragraphs[0].numbering->definition_name.has_value());

    const auto paragraph_snapshot =
        reopened_body.inspect_paragraph_with_options(0U, options);
    CHECK_FALSE(reopened.last_error());
    REQUIRE(paragraph_snapshot.has_value());
    REQUIRE(paragraph_snapshot->numbering.has_value());
    CHECK_FALSE(paragraph_snapshot->numbering->definition_id.has_value());
    CHECK_FALSE(paragraph_snapshot->numbering->definition_name.has_value());

    const auto resolved_paragraphs = reopened_body.inspect_paragraphs();
    CHECK(resolved_paragraphs.empty());
    CHECK(reopened.last_error());
    CHECK_EQ(reopened.last_error().code,
             featherdoc::make_error_code(
                  featherdoc::document_errc::source_archive_changed));
    CHECK_EQ(reopened.last_error().entry_name, "word/numbering.xml");

    const auto resolved_paragraph = reopened_body.inspect_paragraph(0U);
    CHECK_FALSE(resolved_paragraph.has_value());
    CHECK(reopened.last_error());
    CHECK_EQ(reopened.last_error().code,
             featherdoc::make_error_code(
                  featherdoc::document_errc::source_archive_changed));
}

TEST_CASE("inspect tables returns style width grid and text metadata") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "inspect_tables_roundtrip.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto table = doc.append_table(2, 2);
    CHECK(table.set_width_twips(7200U));
    CHECK(table.set_style_id("TableGrid"));
    CHECK(table.set_column_width_twips(0U, 1800U));
    CHECK(table.set_column_width_twips(1U, 5400U));

    auto row = table.rows();
    REQUIRE(row.has_next());
    auto cell = row.cells();
    REQUIRE(cell.has_next());
    CHECK(cell.set_text("r0c0"));
    cell.next();
    REQUIRE(cell.has_next());
    CHECK(cell.set_text("r0c1"));

    row.next();
    REQUIRE(row.has_next());
    cell = row.cells();
    REQUIRE(cell.has_next());
    CHECK(cell.set_text("r1c0"));
    cell.next();
    REQUIRE(cell.has_next());
    CHECK(cell.set_text("r1c1"));

    auto second_table = doc.append_table(1, 1);
    REQUIRE(second_table.has_next());
    auto second_row = second_table.rows();
    REQUIRE(second_row.has_next());
    auto second_cell = second_row.cells();
    REQUIRE(second_cell.has_next());
    CHECK(second_cell.set_text("tail"));

    const auto tables = doc.inspect_tables();
    REQUIRE(tables.size() == 2U);

    CHECK_EQ(tables[0].index, 0U);
    REQUIRE(tables[0].style_id.has_value());
    CHECK_EQ(*tables[0].style_id, "TableGrid");
    REQUIRE(tables[0].width_twips.has_value());
    CHECK_EQ(*tables[0].width_twips, 7200U);
    CHECK_EQ(tables[0].row_count, 2U);
    CHECK_EQ(tables[0].column_count, 2U);
    REQUIRE(tables[0].column_widths.size() == 2U);
    REQUIRE(tables[0].column_widths[0].has_value());
    CHECK_EQ(*tables[0].column_widths[0], 1800U);
    REQUIRE(tables[0].column_widths[1].has_value());
    CHECK_EQ(*tables[0].column_widths[1], 5400U);
    CHECK_EQ(tables[0].text, "r0c0\tr0c1\nr1c0\tr1c1");

    CHECK_EQ(tables[1].index, 1U);
    CHECK_FALSE(tables[1].style_id.has_value());
    CHECK_FALSE(tables[1].width_twips.has_value());
    CHECK_EQ(tables[1].row_count, 1U);
    CHECK_EQ(tables[1].column_count, 1U);
    REQUIRE(tables[1].column_widths.size() == 1U);
    CHECK_EQ(tables[1].text, "tail");

    const auto inspected_table = doc.inspect_table(1U);
    REQUIRE(inspected_table.has_value());
    CHECK_EQ(inspected_table->text, "tail");
    CHECK_FALSE(doc.inspect_table(9U).has_value());

    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());

    const auto reopened_tables = reopened.inspect_tables();
    REQUIRE(reopened_tables.size() == 2U);
    REQUIRE(reopened_tables[0].style_id.has_value());
    CHECK_EQ(*reopened_tables[0].style_id, "TableGrid");
    REQUIRE(reopened_tables[0].width_twips.has_value());
    CHECK_EQ(*reopened_tables[0].width_twips, 7200U);
    REQUIRE(reopened_tables[0].column_widths.size() == 2U);
    REQUIRE(reopened_tables[0].column_widths[0].has_value());
    CHECK_EQ(*reopened_tables[0].column_widths[0], 1800U);
    REQUIRE(reopened_tables[0].column_widths[1].has_value());
    CHECK_EQ(*reopened_tables[0].column_widths[1], 5400U);
    CHECK_EQ(reopened_tables[0].text, "r0c0\tr0c1\nr1c0\tr1c1");
    CHECK_EQ(reopened_tables[1].column_widths, tables[1].column_widths);
    CHECK_EQ(reopened_tables[1].text, "tail");

    fs::remove(target);
}

TEST_CASE("inspect table cells returns width span layout and text metadata") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "inspect_table_cells_roundtrip.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto table = doc.append_table(2, 3);
    CHECK(table.set_column_width_twips(0U, 1200U));
    CHECK(table.set_column_width_twips(1U, 2200U));
    CHECK(table.set_column_width_twips(2U, 3200U));

    auto row = table.rows();
    REQUIRE(row.has_next());
    auto cell = row.cells();
    REQUIRE(cell.has_next());
    CHECK(cell.set_text("merged-top"));
    CHECK(cell.merge_right());
    CHECK(cell.set_vertical_alignment(featherdoc::cell_vertical_alignment::center));
    CHECK(cell.set_text_direction(
        featherdoc::cell_text_direction::top_to_bottom_right_to_left));

    cell.next();
    REQUIRE(cell.has_next());
    CHECK(cell.set_text("top-right"));

    row.next();
    REQUIRE(row.has_next());
    cell = row.cells();
    REQUIRE(cell.has_next());
    CHECK(cell.set_text("bottom-left"));

    cell.next();
    REQUIRE(cell.has_next());
    CHECK(cell.set_width_twips(2222U));
    CHECK(cell.set_text("bottom-middle"));

    cell.next();
    REQUIRE(cell.has_next());
    CHECK(cell.set_text("line1"));
    auto paragraph = cell.paragraphs();
    REQUIRE(paragraph.has_next());
    auto inserted_paragraph = paragraph.insert_paragraph_after("line2");
    REQUIRE(inserted_paragraph.has_next());

    const auto cells = doc.inspect_table_cells(0U);
    REQUIRE(cells.size() == 5U);

    CHECK_EQ(cells[0].row_index, 0U);
    CHECK_EQ(cells[0].cell_index, 0U);
    CHECK_EQ(cells[0].column_index, 0U);
    CHECK_EQ(cells[0].column_span, 2U);
    CHECK_EQ(cells[0].paragraph_count, 1U);
    REQUIRE(cells[0].vertical_alignment.has_value());
    CHECK_EQ(*cells[0].vertical_alignment, featherdoc::cell_vertical_alignment::center);
    REQUIRE(cells[0].text_direction.has_value());
    CHECK_EQ(*cells[0].text_direction,
             featherdoc::cell_text_direction::top_to_bottom_right_to_left);
    CHECK_EQ(cells[0].text, "merged-top");

    CHECK_EQ(cells[1].row_index, 0U);
    CHECK_EQ(cells[1].cell_index, 1U);
    CHECK_EQ(cells[1].column_index, 2U);
    CHECK_EQ(cells[1].column_span, 1U);
    CHECK_EQ(cells[1].text, "top-right");

    CHECK_EQ(cells[2].row_index, 1U);
    CHECK_EQ(cells[2].cell_index, 0U);
    CHECK_EQ(cells[2].column_index, 0U);
    CHECK_EQ(cells[2].text, "bottom-left");

    CHECK_EQ(cells[3].row_index, 1U);
    CHECK_EQ(cells[3].cell_index, 1U);
    CHECK_EQ(cells[3].column_index, 1U);
    REQUIRE(cells[3].width_twips.has_value());
    CHECK_EQ(*cells[3].width_twips, 2222U);
    CHECK_EQ(cells[3].text, "bottom-middle");

    CHECK_EQ(cells[4].row_index, 1U);
    CHECK_EQ(cells[4].cell_index, 2U);
    CHECK_EQ(cells[4].column_index, 2U);
    CHECK_EQ(cells[4].column_span, 1U);
    CHECK_EQ(cells[4].paragraph_count, 2U);
    CHECK_EQ(cells[4].text, "line1\nline2");

    const auto inspected_cell = doc.inspect_table_cell(0U, 1U, 2U);
    REQUIRE(inspected_cell.has_value());
    CHECK_EQ(inspected_cell->text, "line1\nline2");
    const auto visually_inspected_cell =
        doc.inspect_table_cell_by_grid_column(0U, 0U, 1U);
    REQUIRE(visually_inspected_cell.has_value());
    CHECK_EQ(visually_inspected_cell->cell_index, 0U);
    CHECK_EQ(visually_inspected_cell->column_index, 0U);
    CHECK_EQ(visually_inspected_cell->column_span, 2U);
    CHECK_EQ(visually_inspected_cell->text, "merged-top");
    CHECK(doc.inspect_table_cells(9U).empty());
    CHECK_FALSE(doc.inspect_table_cell(0U, 9U, 0U).has_value());
    CHECK_FALSE(doc.inspect_table_cell(0U, 0U, 9U).has_value());
    CHECK_FALSE(doc.inspect_table_cell_by_grid_column(0U, 0U, 9U).has_value());

    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());

    const auto reopened_cells = reopened.inspect_table_cells(0U);
    REQUIRE(reopened_cells.size() == 5U);
    REQUIRE(reopened_cells[0].vertical_alignment.has_value());
    CHECK_EQ(*reopened_cells[0].vertical_alignment,
             featherdoc::cell_vertical_alignment::center);
    REQUIRE(reopened_cells[0].text_direction.has_value());
    CHECK_EQ(*reopened_cells[0].text_direction,
             featherdoc::cell_text_direction::top_to_bottom_right_to_left);
    REQUIRE(reopened_cells[3].width_twips.has_value());
    CHECK_EQ(*reopened_cells[3].width_twips, 2222U);
    CHECK_EQ(reopened_cells[4].paragraph_count, 2U);
    CHECK_EQ(reopened_cells[4].text, "line1\nline2");

    fs::remove(target);
}

TEST_CASE("template part inspection returns body header and footer metadata") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "template_part_inspection_roundtrip.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto numbering_definition = featherdoc::numbering_definition{};
    numbering_definition.name = "TemplatePartInspectOutline";
    numbering_definition.levels = {
        featherdoc::numbering_level_definition{
            featherdoc::list_kind::decimal, 1U, 0U, "%1."},
    };

    const auto numbering_id = doc.ensure_numbering_definition(numbering_definition);
    REQUIRE(numbering_id.has_value());

    auto body_template = doc.body_template();
    REQUIRE(static_cast<bool>(body_template));
    auto body_paragraph = body_template.paragraphs();
    REQUIRE(body_paragraph.has_next());
    CHECK(doc.set_paragraph_style(body_paragraph, "Heading1"));
    CHECK(body_paragraph.set_bidi());
    REQUIRE(body_paragraph.add_run("Body heading").has_next());

    auto numbered_body_paragraph = body_paragraph.insert_paragraph_after("Body item");
    REQUIRE(numbered_body_paragraph.has_next());
    CHECK(doc.set_paragraph_numbering(numbered_body_paragraph, *numbering_id));

    auto &header_paragraph = doc.ensure_section_header_paragraphs(0U);
    REQUIRE(header_paragraph.has_next());
    CHECK(header_paragraph.set_text("Header intro"));

    auto header_template = doc.section_header_template(0U);
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

    auto &footer_paragraph = doc.ensure_section_footer_paragraphs(0U);
    REQUIRE(footer_paragraph.has_next());
    CHECK(footer_paragraph.set_text("Footer intro"));

    auto footer_template = doc.section_footer_template(0U);
    REQUIRE(static_cast<bool>(footer_template));
    auto footer_inspected_paragraph = footer_template.append_paragraph();
    REQUIRE(footer_inspected_paragraph.has_next());
    auto footer_styled_run = footer_inspected_paragraph.add_run("Footer styled");
    REQUIRE(footer_styled_run.has_next());
    CHECK(doc.set_run_style(footer_styled_run, "Strong"));
    CHECK(footer_styled_run.set_font_family("Segoe UI"));
    CHECK(footer_styled_run.set_language("en-US"));
    auto footer_plain_run = footer_inspected_paragraph.add_run(" tail");
    REQUIRE(footer_plain_run.has_next());

    const auto body_paragraphs = body_template.inspect_paragraphs();
    REQUIRE(body_paragraphs.size() == 2U);
    CHECK_EQ(body_paragraphs[0].index, 0U);
    REQUIRE(body_paragraphs[0].style_id.has_value());
    CHECK_EQ(*body_paragraphs[0].style_id, "Heading1");
    REQUIRE(body_paragraphs[0].bidi.has_value());
    CHECK(*body_paragraphs[0].bidi);
    CHECK_EQ(body_paragraphs[0].run_count, 1U);
    CHECK_EQ(body_paragraphs[0].text, "Body heading");
    REQUIRE(body_paragraphs[1].numbering.has_value());
    REQUIRE(body_paragraphs[1].numbering->definition_id.has_value());
    CHECK_EQ(*body_paragraphs[1].numbering->definition_id, *numbering_id);
    REQUIRE(body_paragraphs[1].numbering->definition_name.has_value());
    CHECK_EQ(*body_paragraphs[1].numbering->definition_name,
             "TemplatePartInspectOutline");
    CHECK_EQ(body_paragraphs[1].text, "Body item");
    CHECK_FALSE(body_template.inspect_paragraph(9U).has_value());
    CHECK(body_template.inspect_paragraph_runs(9U).empty());

    const auto header_tables = header_template.inspect_tables();
    REQUIRE(header_tables.size() == 1U);
    CHECK_EQ(header_tables[0].index, 0U);
    REQUIRE(header_tables[0].style_id.has_value());
    CHECK_EQ(*header_tables[0].style_id, "TableGrid");
    CHECK_EQ(header_tables[0].row_count, 2U);
    CHECK_EQ(header_tables[0].column_count, 2U);
    REQUIRE(header_tables[0].column_widths.size() == 2U);
    REQUIRE(header_tables[0].column_widths[0].has_value());
    CHECK_EQ(*header_tables[0].column_widths[0], 1800U);
    CHECK_EQ(header_tables[0].text, "h00\th01\nh10\th11");

    const auto header_cells = header_template.inspect_table_cells(0U);
    REQUIRE(header_cells.size() == 4U);
    CHECK_EQ(header_cells[3].row_index, 1U);
    CHECK_EQ(header_cells[3].cell_index, 1U);
    REQUIRE(header_cells[3].width_twips.has_value());
    CHECK_EQ(*header_cells[3].width_twips, 2222U);
    CHECK_EQ(header_cells[3].text, "h11");
    REQUIRE(header_template.inspect_table_cell(0U, 1U, 1U).has_value());
    CHECK_FALSE(header_template.inspect_table(9U).has_value());
    CHECK(header_template.inspect_table_cells(9U).empty());

    const auto footer_runs = footer_template.inspect_paragraph_runs(1U);
    REQUIRE(footer_runs.size() == 2U);
    CHECK_EQ(footer_runs[0].index, 0U);
    REQUIRE(footer_runs[0].style_id.has_value());
    CHECK_EQ(*footer_runs[0].style_id, "Strong");
    REQUIRE(footer_runs[0].font_family.has_value());
    CHECK_EQ(*footer_runs[0].font_family, "Segoe UI");
    REQUIRE(footer_runs[0].language.has_value());
    CHECK_EQ(*footer_runs[0].language, "en-US");
    CHECK_EQ(footer_runs[0].text, "Footer styled");
    CHECK_EQ(footer_runs[1].text, " tail");
    REQUIRE(footer_template.inspect_paragraph_run(1U, 0U).has_value());
    CHECK_FALSE(footer_template.inspect_paragraph_run(1U, 9U).has_value());

    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());

    auto reopened_body = reopened.body_template();
    REQUIRE(static_cast<bool>(reopened_body));
    const auto reopened_body_paragraphs = reopened_body.inspect_paragraphs();
    REQUIRE(reopened_body_paragraphs.size() == 2U);
    REQUIRE(reopened_body_paragraphs[0].style_id.has_value());
    CHECK_EQ(*reopened_body_paragraphs[0].style_id, "Heading1");
    REQUIRE(reopened_body_paragraphs[1].numbering.has_value());
    REQUIRE(reopened_body_paragraphs[1].numbering->definition_name.has_value());
    CHECK_EQ(*reopened_body_paragraphs[1].numbering->definition_name,
             "TemplatePartInspectOutline");

    auto reopened_header = reopened.section_header_template(0U);
    REQUIRE(static_cast<bool>(reopened_header));
    const auto reopened_header_tables = reopened_header.inspect_tables();
    REQUIRE(reopened_header_tables.size() == 1U);
    CHECK_EQ(reopened_header_tables[0].text, "h00\th01\nh10\th11");
    const auto reopened_header_cells = reopened_header.inspect_table_cells(0U);
    REQUIRE(reopened_header_cells.size() == 4U);
    REQUIRE(reopened_header_cells[3].width_twips.has_value());
    CHECK_EQ(*reopened_header_cells[3].width_twips, 2222U);

    auto reopened_footer = reopened.section_footer_template(0U);
    REQUIRE(static_cast<bool>(reopened_footer));
    const auto reopened_footer_runs = reopened_footer.inspect_paragraph_runs(1U);
    REQUIRE(reopened_footer_runs.size() == 2U);
    REQUIRE(reopened_footer_runs[0].style_id.has_value());
    CHECK_EQ(*reopened_footer_runs[0].style_id, "Strong");
    REQUIRE(reopened_footer_runs[0].language.has_value());
    CHECK_EQ(*reopened_footer_runs[0].language, "en-US");
    CHECK_EQ(reopened_footer_runs[1].text, " tail");

    fs::remove(target);
}
