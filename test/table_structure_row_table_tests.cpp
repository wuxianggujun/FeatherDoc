#include <array>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "doctest.h"
#include "basic_docx_archive_test_support.hpp"
#include "basic_document_xml_test_support.hpp"

#include <featherdoc.hpp>

TEST_CASE("table rows can insert a formatted row after the current row") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "table_row_insert_after.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto table = doc.append_table(2, 2);
    auto first_row = table.rows();
    REQUIRE(first_row.has_next());
    CHECK(first_row.set_cant_split());

    auto first_cell = first_row.cells();
    REQUIRE(first_cell.has_next());
    CHECK(first_cell.set_width_twips(2400U));
    CHECK(first_cell.set_fill_color("D9EAF7"));
    CHECK(first_cell.set_text("row-1a"));

    auto second_cell = first_cell;
    second_cell.next();
    REQUIRE(second_cell.has_next());
    CHECK(second_cell.set_text("row-1b"));

    auto last_row = first_row;
    last_row.next();
    REQUIRE(last_row.has_next());
    CHECK(last_row.cells().set_text("row-2a"));
    auto last_row_second_cell = last_row.cells();
    last_row_second_cell.next();
    REQUIRE(last_row_second_cell.has_next());
    CHECK(last_row_second_cell.set_text("row-2b"));

    auto inserted_row = table.rows().insert_row_after();
    REQUIRE(inserted_row.has_next());
    CHECK(table.rows().has_next());
    CHECK_EQ(table.rows().cells().get_text(), "row-1a");

    auto inserted_cell = inserted_row.cells();
    REQUIRE(inserted_cell.has_next());
    CHECK_EQ(inserted_cell.get_text(), "");
    CHECK(inserted_cell.set_text("inserted-a"));

    auto inserted_second_cell = inserted_cell;
    inserted_second_cell.next();
    REQUIRE(inserted_second_cell.has_next());
    CHECK_EQ(inserted_second_cell.get_text(), "");
    CHECK(inserted_second_cell.set_text("inserted-b"));

    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_table_text(reopened),
             "row-1a\nrow-1b\ninserted-a\ninserted-b\nrow-2a\nrow-2b\n");

    auto reopened_row = reopened.tables().rows();
    REQUIRE(reopened_row.has_next());
    reopened_row.next();
    REQUIRE(reopened_row.has_next());
    CHECK(reopened_row.cant_split());

    auto reopened_cell = reopened_row.cells();
    REQUIRE(reopened_cell.has_next());
    REQUIRE(reopened_cell.fill_color().has_value());
    CHECK_EQ(*reopened_cell.fill_color(), "D9EAF7");
    REQUIRE(reopened_cell.width_twips().has_value());
    CHECK_EQ(*reopened_cell.width_twips(), 2400U);

    fs::remove(target);
}

TEST_CASE("table rows can insert a formatted row before the current row") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "table_row_insert_before.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto table = doc.append_table(3, 2);
    auto row = table.rows();
    REQUIRE(row.has_next());
    CHECK(row.cells().set_text("row-1a"));
    auto row1_second_cell = row.cells();
    row1_second_cell.next();
    REQUIRE(row1_second_cell.has_next());
    CHECK(row1_second_cell.set_text("row-1b"));

    row.next();
    REQUIRE(row.has_next());
    CHECK(row.set_cant_split());
    auto source_cell = row.cells();
    REQUIRE(source_cell.has_next());
    CHECK(source_cell.set_width_twips(2400U));
    CHECK(source_cell.set_fill_color("FFF2CC"));
    CHECK(source_cell.set_text("source-a"));
    auto source_second_cell = source_cell;
    source_second_cell.next();
    REQUIRE(source_second_cell.has_next());
    CHECK(source_second_cell.set_fill_color("FFF2CC"));
    CHECK(source_second_cell.set_text("source-b"));

    row.next();
    REQUIRE(row.has_next());
    CHECK(row.cells().set_text("row-3a"));
    auto row3_second_cell = row.cells();
    row3_second_cell.next();
    REQUIRE(row3_second_cell.has_next());
    CHECK(row3_second_cell.set_text("row-3b"));

    auto source_row = table.rows();
    source_row.next();
    REQUIRE(source_row.has_next());
    auto inserted_row = source_row.insert_row_before();
    REQUIRE(inserted_row.has_next());
    CHECK(source_row.has_next());
    CHECK_EQ(source_row.cells().get_text(), "");

    auto inserted_cell = inserted_row.cells();
    REQUIRE(inserted_cell.has_next());
    CHECK_EQ(inserted_cell.get_text(), "");
    CHECK(inserted_cell.set_text("inserted-a"));
    auto inserted_second_cell = inserted_cell;
    inserted_second_cell.next();
    REQUIRE(inserted_second_cell.has_next());
    CHECK_EQ(inserted_second_cell.get_text(), "");
    CHECK(inserted_second_cell.set_text("inserted-b"));

    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_table_text(reopened),
             "row-1a\nrow-1b\ninserted-a\ninserted-b\nsource-a\nsource-b\nrow-3a\nrow-3b\n");

    auto reopened_row = reopened.tables().rows();
    REQUIRE(reopened_row.has_next());
    reopened_row.next();
    REQUIRE(reopened_row.has_next());
    CHECK(reopened_row.cant_split());

    auto reopened_cell = reopened_row.cells();
    REQUIRE(reopened_cell.has_next());
    REQUIRE(reopened_cell.fill_color().has_value());
    CHECK_EQ(*reopened_cell.fill_color(), "FFF2CC");
    REQUIRE(reopened_cell.width_twips().has_value());
    CHECK_EQ(*reopened_cell.width_twips(), 2400U);
    CHECK_EQ(reopened_cell.get_text(), "inserted-a");

    fs::remove(target);
}

TEST_CASE("tables can remove a middle table and keep the wrapper usable") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "table_remove.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto first_table = doc.append_table(1, 1);
    REQUIRE(first_table.has_next());
    CHECK(first_table.rows().cells().set_text("table-1"));

    auto second_table = doc.append_table(1, 1);
    REQUIRE(second_table.has_next());
    CHECK(second_table.rows().cells().set_text("table-2"));
    auto removed_table = second_table;

    auto third_table = doc.append_table(1, 1);
    REQUIRE(third_table.has_next());
    CHECK(third_table.rows().cells().set_text("table-3"));

    CHECK(removed_table.remove());
    CHECK(removed_table.has_next());
    CHECK_EQ(removed_table.rows().cells().get_text(), "table-3");

    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_table_text(reopened), "table-1\ntable-3\n");

    fs::remove(target);
}

TEST_CASE("tables can insert a new table before the selected body table and keep the wrapper usable") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "table_insert_before.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto paragraph = doc.paragraphs();
    REQUIRE(paragraph.has_next());
    CHECK(paragraph.set_text("body paragraph"));

    auto first_table = doc.append_table(1, 1);
    REQUIRE(first_table.has_next());
    CHECK(first_table.rows().cells().set_text("table-1"));

    auto last_table = doc.append_table(1, 1);
    REQUIRE(last_table.has_next());
    CHECK(last_table.rows().cells().set_text("table-3"));

    auto inserted_table = last_table.insert_table_before(1, 1);
    REQUIRE(inserted_table.has_next());
    CHECK(last_table.has_next());
    CHECK(last_table.rows().cells().set_text("table-2"));
    CHECK_EQ(inserted_table.rows().cells().get_text(), "table-2");

    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_document_text(reopened), "body paragraph\n");
    CHECK_EQ(collect_table_text(reopened), "table-1\ntable-2\ntable-3\n");

    const auto xml_text = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(xml_text.c_str()));
    const auto body_node = xml_document.child("w:document").child("w:body");
    REQUIRE(body_node != pugi::xml_node{});
    CHECK_EQ(count_named_children(body_node, "w:tbl"), 3U);

    fs::remove(target);
}

TEST_CASE("tables can insert a new table after the last body table before section properties") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "table_insert_after_last.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto paragraph = doc.paragraphs();
    REQUIRE(paragraph.has_next());
    CHECK(paragraph.set_text("body paragraph"));

    auto &header = doc.ensure_section_header_paragraphs(0);
    REQUIRE(header.has_next());
    CHECK(header.set_text("header"));

    auto last_table = doc.append_table(1, 1);
    REQUIRE(last_table.has_next());
    CHECK(last_table.rows().cells().set_text("table-1"));

    auto inserted_table = last_table.insert_table_after(1, 1);
    REQUIRE(inserted_table.has_next());
    CHECK(last_table.has_next());
    CHECK(last_table.rows().cells().set_text("table-2"));

    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_document_text(reopened), "body paragraph\n");
    CHECK_EQ(collect_table_text(reopened), "table-1\ntable-2\n");

    const auto xml_text = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(xml_text.c_str()));
    const auto body_node = xml_document.child("w:document").child("w:body");
    REQUIRE(body_node != pugi::xml_node{});
    CHECK_EQ(count_named_children(body_node, "w:tbl"), 2U);
    CHECK_EQ(std::string_view{body_node.last_child().name()}, "w:sectPr");

    fs::remove(target);
}

TEST_CASE("tables can insert a styled table clone before the selected body table") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "table_insert_like_before.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto paragraph = doc.paragraphs();
    REQUIRE(paragraph.has_next());
    CHECK(paragraph.set_text("body paragraph"));

    auto first_table = doc.append_table(2, 2);
    REQUIRE(first_table.has_next());
    REQUIRE(configure_clone_template_table(first_table, "seed"));

    auto anchor_table = doc.append_table(2, 2);
    REQUIRE(anchor_table.has_next());
    REQUIRE(configure_clone_template_table(anchor_table, "anchor"));

    auto inserted_table = anchor_table.insert_table_like_before();
    REQUIRE(inserted_table.has_next());
    CHECK(anchor_table.has_next());

    const auto inserted_width = inserted_table.width_twips();
    REQUIRE(inserted_width.has_value());
    CHECK_EQ(*inserted_width, 7200U);

    const auto inserted_style = inserted_table.style_id();
    REQUIRE(inserted_style.has_value());
    CHECK_EQ(*inserted_style, "TableGrid");

    const auto inserted_layout = inserted_table.layout_mode();
    REQUIRE(inserted_layout.has_value());
    CHECK_EQ(*inserted_layout, featherdoc::table_layout_mode::fixed);

    auto inserted_row = inserted_table.rows();
    REQUIRE(inserted_row.has_next());
    CHECK(inserted_row.repeats_header());
    CHECK_EQ(inserted_row.height_twips().value_or(0U), 360U);

    auto inserted_cell = inserted_row.cells();
    REQUIRE(inserted_cell.has_next());
    CHECK_EQ(inserted_cell.fill_color().value_or(""), "D9EAF7");
    CHECK_EQ(inserted_cell.get_text(), "");

    inserted_cell.next();
    REQUIRE(inserted_cell.has_next());
    CHECK_EQ(inserted_cell.fill_color().value_or(""), "D9EAF7");
    CHECK_EQ(inserted_cell.get_text(), "");

    inserted_row.next();
    REQUIRE(inserted_row.has_next());
    CHECK_EQ(inserted_row.height_twips().value_or(0U), 420U);
    inserted_cell = inserted_row.cells();
    REQUIRE(inserted_cell.has_next());
    CHECK_EQ(inserted_cell.fill_color().value_or(""), "FCE4D6");
    CHECK_EQ(inserted_cell.margin_twips(featherdoc::cell_margin_edge::left).value_or(0U),
             160U);
    CHECK_EQ(inserted_cell.get_text(), "");

    inserted_cell.next();
    REQUIRE(inserted_cell.has_next());
    CHECK_EQ(inserted_cell.fill_color().value_or(""), "FFF2CC");
    CHECK_EQ(inserted_cell.margin_twips(featherdoc::cell_margin_edge::right).value_or(0U),
             160U);
    CHECK_EQ(inserted_cell.get_text(), "");

    inserted_row = inserted_table.rows();
    REQUIRE(inserted_row.has_next());
    CHECK(set_two_cell_row_text(inserted_row, "clone-h1", "clone-h2"));
    inserted_row.next();
    REQUIRE(inserted_row.has_next());
    CHECK(set_two_cell_row_text(inserted_row, "clone-b1", "clone-b2"));

    anchor_table.next();
    REQUIRE(anchor_table.has_next());
    CHECK_EQ(anchor_table.rows().cells().get_text(), "anchor-h1");

    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_document_text(reopened), "body paragraph\n");
    CHECK_EQ(collect_table_text(reopened),
             "seed-h1\nseed-h2\nseed-b1\nseed-b2\n"
             "clone-h1\nclone-h2\nclone-b1\nclone-b2\n"
             "anchor-h1\nanchor-h2\nanchor-b1\nanchor-b2\n");

    const auto xml_text = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(xml_text.c_str()));
    const auto body_node = xml_document.child("w:document").child("w:body");
    REQUIRE(body_node != pugi::xml_node{});
    CHECK_EQ(count_named_children(body_node, "w:tbl"), 3U);

    fs::remove(target);
}

TEST_CASE("tables can insert a styled table clone after the last body table before section properties") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "table_insert_like_after_last.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto paragraph = doc.paragraphs();
    REQUIRE(paragraph.has_next());
    CHECK(paragraph.set_text("body paragraph"));

    auto &header = doc.ensure_section_header_paragraphs(0);
    REQUIRE(header.has_next());
    CHECK(header.set_text("header"));

    auto anchor_table = doc.append_table(2, 2);
    REQUIRE(anchor_table.has_next());
    REQUIRE(configure_clone_template_table(anchor_table, "anchor"));

    auto inserted_table = anchor_table.insert_table_like_after();
    REQUIRE(inserted_table.has_next());
    CHECK(anchor_table.has_next());

    auto inserted_row = inserted_table.rows();
    REQUIRE(inserted_row.has_next());
    CHECK(inserted_row.repeats_header());
    CHECK_EQ(inserted_row.cells().fill_color().value_or(""), "D9EAF7");
    CHECK_EQ(inserted_row.cells().get_text(), "");

    inserted_row = inserted_table.rows();
    REQUIRE(inserted_row.has_next());
    CHECK(set_two_cell_row_text(inserted_row, "clone-h1", "clone-h2"));
    inserted_row.next();
    REQUIRE(inserted_row.has_next());
    CHECK(set_two_cell_row_text(inserted_row, "clone-b1", "clone-b2"));

    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_document_text(reopened), "body paragraph\n");
    CHECK_EQ(collect_table_text(reopened),
             "anchor-h1\nanchor-h2\nanchor-b1\nanchor-b2\n"
             "clone-h1\nclone-h2\nclone-b1\nclone-b2\n");

    const auto xml_text = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(xml_text.c_str()));
    const auto body_node = xml_document.child("w:document").child("w:body");
    REQUIRE(body_node != pugi::xml_node{});
    CHECK_EQ(count_named_children(body_node, "w:tbl"), 2U);
    CHECK_EQ(std::string_view{body_node.last_child().name()}, "w:sectPr");

    fs::remove(target);
}

TEST_CASE("header template part tables can insert a styled table clone after the selected header table") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "header_template_table_insert_like_after.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto body_paragraph = doc.paragraphs();
    REQUIRE(body_paragraph.has_next());
    CHECK(body_paragraph.set_text("body paragraph"));

    auto &header_paragraph = doc.ensure_section_header_paragraphs(0);
    REQUIRE(header_paragraph.has_next());
    CHECK(header_paragraph.set_text("Header intro"));

    auto header_template = doc.section_header_template(0);
    REQUIRE(static_cast<bool>(header_template));

    auto anchor_table = header_template.append_table(2, 2);
    REQUIRE(anchor_table.has_next());
    REQUIRE(configure_clone_template_table(anchor_table, "anchor"));

    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());

    header_template = reopened.section_header_template(0);
    REQUIRE(static_cast<bool>(header_template));
    auto selected_table = header_template.tables();
    REQUIRE(selected_table.has_next());

    auto inserted_table = selected_table.insert_table_like_after();
    REQUIRE(inserted_table.has_next());
    CHECK(selected_table.has_next());

    auto inserted_row = inserted_table.rows();
    REQUIRE(inserted_row.has_next());
    CHECK(inserted_row.repeats_header());
    CHECK_EQ(inserted_row.cells().fill_color().value_or(""), "D9EAF7");
    CHECK_EQ(inserted_row.cells().get_text(), "");

    inserted_row = inserted_table.rows();
    REQUIRE(inserted_row.has_next());
    CHECK(set_two_cell_row_text(inserted_row, "clone-h1", "clone-h2"));
    inserted_row.next();
    REQUIRE(inserted_row.has_next());
    CHECK(set_two_cell_row_text(inserted_row, "clone-b1", "clone-b2"));

    CHECK_FALSE(reopened.save());

    featherdoc::Document reopened_again(target);
    CHECK_FALSE(reopened_again.open());
    header_template = reopened_again.section_header_template(0);
    REQUIRE(static_cast<bool>(header_template));
    CHECK_EQ(collect_template_part_text(header_template), "Header intro\n");
    CHECK_EQ(collect_template_part_table_text(header_template),
             "anchor-h1\nanchor-h2\nanchor-b1\nanchor-b2\n"
             "clone-h1\nclone-h2\nclone-b1\nclone-b2\n");

    const auto header_xml = read_test_docx_entry(target, "word/header1.xml");
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(header_xml.c_str()));
    const auto header_node = xml_document.child("w:hdr");
    REQUIRE(header_node != pugi::xml_node{});
    CHECK_EQ(count_named_children(header_node, "w:tbl"), 2U);

    fs::remove(target);
}

TEST_CASE("table remove rejects removing the last block item in the document body") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "table_remove_last_block.docx";
    fs::remove(target);

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:tbl>
      <w:tr>
        <w:tc>
          <w:p>
            <w:r><w:t>only table</w:t></w:r>
          </w:p>
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
    CHECK_FALSE(table.remove());
    CHECK(table.has_next());

    fs::remove(target);
}

TEST_CASE("tables can remove the only table when body paragraphs remain") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "table_remove_keep_body_paragraph.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto paragraph = doc.paragraphs();
    REQUIRE(paragraph.has_next());
    CHECK(paragraph.set_text("body paragraph"));

    auto table = doc.append_table(1, 1);
    REQUIRE(table.has_next());
    CHECK(table.rows().cells().set_text("table-1"));

    CHECK(table.remove());
    CHECK_FALSE(table.has_next());

    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_document_text(reopened), "body paragraph\n");
    CHECK_EQ(collect_table_text(reopened), "");

    const auto xml_text = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(xml_text.c_str()));
    const auto body_node = xml_document.child("w:document").child("w:body");
    REQUIRE(body_node != pugi::xml_node{});
    CHECK_EQ(count_named_children(body_node, "w:tbl"), 0U);
    CHECK_EQ(count_named_children(body_node, "w:p"), 1U);

    fs::remove(target);
}

TEST_CASE("tables can remove the last table and keep the wrapper usable") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "table_remove_last_table.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto paragraph = doc.paragraphs();
    REQUIRE(paragraph.has_next());
    CHECK(paragraph.set_text("body paragraph"));

    auto first_table = doc.append_table(1, 1);
    REQUIRE(first_table.has_next());
    CHECK(first_table.rows().cells().set_text("table-1"));

    auto last_table = doc.append_table(1, 1);
    REQUIRE(last_table.has_next());
    CHECK(last_table.rows().cells().set_text("table-2"));

    CHECK(last_table.remove());
    CHECK(last_table.has_next());
    CHECK_EQ(last_table.rows().cells().get_text(), "table-1");
    CHECK(last_table.rows().cells().set_text("table-1-updated"));

    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_document_text(reopened), "body paragraph\n");
    CHECK_EQ(collect_table_text(reopened), "table-1-updated\n");

    const auto xml_text = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(xml_text.c_str()));
    const auto body_node = xml_document.child("w:document").child("w:body");
    REQUIRE(body_node != pugi::xml_node{});
    CHECK_EQ(count_named_children(body_node, "w:tbl"), 1U);

    fs::remove(target);
}

TEST_CASE("header template part tables can insert a new table after the selected header table") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "header_template_table_insert_after.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto body_paragraph = doc.paragraphs();
    REQUIRE(body_paragraph.has_next());
    CHECK(body_paragraph.set_text("body paragraph"));

    auto &header_paragraph = doc.ensure_section_header_paragraphs(0);
    REQUIRE(header_paragraph.has_next());
    CHECK(header_paragraph.set_text("Header intro"));

    auto header_template = doc.section_header_template(0);
    REQUIRE(static_cast<bool>(header_template));

    auto first_table = header_template.append_table(1, 1);
    REQUIRE(first_table.has_next());
    CHECK(first_table.rows().cells().set_text("header-1"));

    auto last_table = header_template.append_table(1, 1);
    REQUIRE(last_table.has_next());
    CHECK(last_table.rows().cells().set_text("header-3"));

    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());

    header_template = reopened.section_header_template(0);
    REQUIRE(static_cast<bool>(header_template));
    auto selected_table = header_template.tables();
    REQUIRE(selected_table.has_next());

    auto inserted_table = selected_table.insert_table_after(1, 1);
    REQUIRE(inserted_table.has_next());
    CHECK(selected_table.has_next());
    CHECK(selected_table.rows().cells().set_text("header-2"));

    CHECK_FALSE(reopened.save());

    featherdoc::Document reopened_again(target);
    CHECK_FALSE(reopened_again.open());
    header_template = reopened_again.section_header_template(0);
    REQUIRE(static_cast<bool>(header_template));
    CHECK_EQ(collect_template_part_text(header_template), "Header intro\n");
    CHECK_EQ(collect_template_part_table_text(header_template),
             "header-1\nheader-2\nheader-3\n");

    const auto header_xml = read_test_docx_entry(target, "word/header1.xml");
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(header_xml.c_str()));
    const auto header_node = xml_document.child("w:hdr");
    REQUIRE(header_node != pugi::xml_node{});
    CHECK_EQ(count_named_children(header_node, "w:tbl"), 3U);

    fs::remove(target);
}

TEST_CASE("table row remove rejects removing the last table row") {
    featherdoc::Document doc;
    CHECK_FALSE(doc.create_empty());

    auto row = doc.append_table(1, 1).rows();
    REQUIRE(row.has_next());
    CHECK_FALSE(row.remove());
    CHECK(row.has_next());
}

TEST_CASE("table row insert after rejects vertical-merge rows") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "table_row_insert_after_vertical_merge.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto row = doc.append_table(2, 1).rows();
    REQUIRE(row.has_next());
    auto cell = row.cells();
    REQUIRE(cell.has_next());
    CHECK(cell.set_text("merged"));
    CHECK(cell.merge_down(1U));

    auto inserted = row.insert_row_after();
    CHECK_FALSE(inserted.has_next());
    CHECK(row.has_next());
    CHECK_EQ(row.cells().get_text(), "merged");

    CHECK_FALSE(doc.save());

    const auto xml_text = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(xml_text.c_str()));
    const auto table_node = xml_document.child("w:document").child("w:body").child("w:tbl");
    REQUIRE(table_node != pugi::xml_node{});
    CHECK_EQ(count_named_children(table_node, "w:tr"), 2);

    fs::remove(target);
}

TEST_CASE("table row insert before rejects vertical-merge rows") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "table_row_insert_before_vertical_merge.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto row = doc.append_table(2, 1).rows();
    REQUIRE(row.has_next());
    auto cell = row.cells();
    REQUIRE(cell.has_next());
    CHECK(cell.set_text("merged"));
    CHECK(cell.merge_down(1U));

    auto inserted = row.insert_row_before();
    CHECK_FALSE(inserted.has_next());
    CHECK(row.has_next());
    CHECK_EQ(row.cells().get_text(), "merged");

    CHECK_FALSE(doc.save());

    const auto xml_text = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(xml_text.c_str()));
    const auto table_node = xml_document.child("w:document").child("w:body").child("w:tbl");
    REQUIRE(table_node != pugi::xml_node{});
    CHECK_EQ(count_named_children(table_node, "w:tr"), 2);

    fs::remove(target);
}

TEST_CASE("table row remove promotes the next vertical-merge continuation row") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "table_row_remove_vertical_merge.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto table = doc.append_table(3, 1);
    auto first_row = table.rows();
    REQUIRE(first_row.has_next());

    auto merged_cell = first_row.cells();
    REQUIRE(merged_cell.has_next());
    CHECK(merged_cell.set_text("merged"));
    CHECK(merged_cell.merge_down(2U));

    CHECK(first_row.remove());
    CHECK(first_row.has_next());
    CHECK_EQ(first_row.cells().get_text(), "merged");

    CHECK_FALSE(doc.save());

    const auto xml_text = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(xml_text.c_str()));

    const auto table_node = xml_document.child("w:document").child("w:body").child("w:tbl");
    REQUIRE(table_node != pugi::xml_node{});
    CHECK_EQ(count_named_children(table_node, "w:tr"), 2);

    const auto promoted_cell = table_node.child("w:tr").child("w:tc");
    REQUIRE(promoted_cell != pugi::xml_node{});
    CHECK_EQ(std::string_view{
                 promoted_cell.child("w:tcPr").child("w:vMerge").attribute("w:val").value()},
             "restart");
    CHECK_EQ(std::string_view{
                 promoted_cell.child("w:p").child("w:r").child("w:t").text().get()},
             "merged");

    const auto trailing_cell = table_node.child("w:tr").next_sibling("w:tr").child("w:tc");
    REQUIRE(trailing_cell != pugi::xml_node{});
    CHECK_EQ(std::string_view{
                 trailing_cell.child("w:tcPr").child("w:vMerge").attribute("w:val").value()},
             "continue");

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_table_text(reopened), "merged\n\n");

    fs::remove(target);
}

TEST_CASE("table row remove rejects invalid column geometry atomically") {
    namespace fs = std::filesystem;

    struct geometry_case {
        std::string_view name;
        std::string_view cells_xml;
    };
    constexpr auto geometry_cases = std::array{
        geometry_case{
            "single_span_64",
            R"(<w:tc><w:tcPr><w:gridSpan w:val="64"/></w:tcPr><w:p><w:r><w:t>original</w:t></w:r></w:p></w:tc>)"},
        geometry_case{
            "summed_span_64",
            R"(<w:tc><w:tcPr><w:gridSpan w:val="32"/></w:tcPr><w:p><w:r><w:t>left</w:t></w:r></w:p></w:tc><w:tc><w:tcPr><w:gridSpan w:val="32"/></w:tcPr><w:p><w:r><w:t>right</w:t></w:r></w:p></w:tc>)"},
        geometry_case{
            "zero_span",
            R"(<w:tc><w:tcPr><w:gridSpan w:val="0"/></w:tcPr><w:p><w:r><w:t>original</w:t></w:r></w:p></w:tc>)"},
        geometry_case{
            "missing_span_value",
            R"(<w:tc><w:tcPr><w:gridSpan/></w:tcPr><w:p><w:r><w:t>original</w:t></w:r></w:p></w:tc>)"},
        geometry_case{
            "invalid_span_value",
            R"(<w:tc><w:tcPr><w:gridSpan w:val="invalid"/></w:tcPr><w:p><w:r><w:t>original</w:t></w:r></w:p></w:tc>)"},
    };

    for (const auto &geometry : geometry_cases) {
        const auto target = fs::current_path() /
                            ("table_row_remove_invalid_" +
                             std::string{geometry.name} + ".docx");
        fs::remove(target);
        write_test_docx(
            target,
            std::string{
                R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:body><w:tbl><w:tr>)"} +
                std::string{geometry.cells_xml} +
                R"(</w:tr><w:tr><w:tc><w:p><w:r><w:t>keep</w:t></w:r></w:p></w:tc></w:tr></w:tbl></w:body></w:document>)");

        featherdoc::Document document(target);
        REQUIRE_FALSE(document.open());
        REQUIRE_FALSE(document.save());
        const auto xml_before =
            read_test_docx_entry(target, test_document_xml_entry);

        auto table = document.tables();
        REQUIRE(table.valid());
        auto row = table.rows();
        REQUIRE(row.valid());
        auto old_row = row;
        auto cell = row.cells();
        REQUIRE(cell.valid());
        auto paragraph = cell.paragraphs();
        auto run = paragraph.runs();
        REQUIRE(paragraph.valid());
        REQUIRE(run.valid());

        CHECK_FALSE(row.remove());
        CHECK(row.valid());
        CHECK(old_row.valid());
        CHECK(cell.valid());
        CHECK(paragraph.valid());
        CHECK(run.valid());
        CHECK(table.valid());

        REQUIRE_FALSE(document.save());
        CHECK_EQ(read_test_docx_entry(target, test_document_xml_entry),
                 xml_before);

        fs::remove(target);
    }
}

TEST_CASE("table append row rejects oversized grid spans without changing the table") {
    namespace fs = std::filesystem;

    const fs::path target =
        fs::current_path() / "table_append_row_oversized_grid_span.docx";
    fs::remove(target);

    write_test_docx(
        target,
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:tbl>
      <w:tr>
        <w:tc>
          <w:tcPr><w:gridSpan w:val="64"/></w:tcPr>
          <w:p><w:r><w:t>original</w:t></w:r></w:p>
        </w:tc>
      </w:tr>
    </w:tbl>
  </w:body>
</w:document>)");

    featherdoc::Document doc(target);
    REQUIRE_FALSE(doc.open());
    auto table = doc.tables();
    REQUIRE(table.has_next());

    const auto appended = table.append_row();
    CHECK_FALSE(appended.has_next());
    CHECK_EQ(collect_table_text(doc), "original\n");

    CHECK_FALSE(doc.save());
    const auto xml_text = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(xml_text.c_str()));
    const auto table_node =
        xml_document.child("w:document").child("w:body").child("w:tbl");
    REQUIRE(table_node != pugi::xml_node{});
    CHECK_EQ(count_named_children(table_node, "w:tr"), 1U);
    CHECK(table_node.child("w:tblGrid") == pugi::xml_node{});

    fs::remove(target);
}

TEST_CASE("table append row rejects a checked grid span sum above the safety limit") {
    namespace fs = std::filesystem;

    const fs::path target =
        fs::current_path() / "table_append_row_oversized_grid_span_sum.docx";
    fs::remove(target);

    write_test_docx(
        target,
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:tbl>
      <w:tr>
        <w:tc>
          <w:tcPr><w:gridSpan w:val="32"/></w:tcPr>
          <w:p><w:r><w:t>left</w:t></w:r></w:p>
        </w:tc>
        <w:tc>
          <w:tcPr><w:gridSpan w:val="32"/></w:tcPr>
          <w:p><w:r><w:t>right</w:t></w:r></w:p>
        </w:tc>
      </w:tr>
    </w:tbl>
  </w:body>
</w:document>)");

    featherdoc::Document doc(target);
    REQUIRE_FALSE(doc.open());
    auto table = doc.tables();
    REQUIRE(table.has_next());

    const auto appended = table.append_row();
    CHECK_FALSE(appended.has_next());
    CHECK_EQ(collect_table_text(doc), "left\nright\n");

    CHECK_FALSE(doc.save());
    const auto xml_text = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(xml_text.c_str()));
    const auto table_node =
        xml_document.child("w:document").child("w:body").child("w:tbl");
    REQUIRE(table_node != pugi::xml_node{});
    CHECK_EQ(count_named_children(table_node, "w:tr"), 1U);
    CHECK(table_node.child("w:tblGrid") == pugi::xml_node{});

    fs::remove(target);
}

TEST_CASE(
    "table append cell preserves existing handles and extends the grid atomically") {
    namespace fs = std::filesystem;

    const fs::path target =
        fs::current_path() / "table_append_cell_handles.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    REQUIRE_FALSE(doc.create_empty());
    auto table = doc.append_table(1U, 1U);
    REQUIRE(table.valid());
    auto row = table.rows();
    REQUIRE(row.valid());
    auto original_cell = row.cells();
    REQUIRE(original_cell.valid());
    REQUIRE(original_cell.set_text("original"));
    auto original_paragraph = original_cell.paragraphs();
    auto original_run = original_paragraph.runs();
    auto original_row = row;
    REQUIRE(original_paragraph.valid());
    REQUIRE(original_run.valid());

    auto appended = row.append_cell();
    REQUIRE(appended.valid());
    REQUIRE(appended.set_text("appended"));
    CHECK(table.valid());
    CHECK(row.valid());
    CHECK(original_cell.valid());
    CHECK(original_paragraph.valid());
    CHECK(original_run.valid());
    CHECK_EQ(original_cell.get_text(), "original");
    CHECK_EQ(appended.get_text(), "appended");

    row.next();
    REQUIRE_FALSE(row.valid());
    auto new_row_cell = row.append_cell();
    REQUIRE(new_row_cell.valid());
    REQUIRE(new_row_cell.set_text("new row"));
    CHECK(row.valid());
    CHECK(original_row.valid());
    CHECK(original_cell.valid());
    CHECK(original_paragraph.valid());
    CHECK(original_run.valid());

    REQUIRE_FALSE(doc.save());
    const auto xml_text = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(xml_text.c_str()));
    const auto table_node =
        xml_document.child("w:document").child("w:body").child("w:tbl");
    CHECK_EQ(count_named_children(table_node.child("w:tblGrid"), "w:gridCol"),
             2U);
    CHECK_EQ(count_named_children(table_node, "w:tr"), 2U);
    CHECK_EQ(count_named_children(table_node.child("w:tr"), "w:tc"), 2U);
    CHECK_EQ(collect_table_text(doc), "original\nappended\nnew row\n");

    fs::remove(target);
}

TEST_CASE("table append cell rejects invalid geometry atomically") {
    namespace fs = std::filesystem;

    struct geometry_case {
        std::string_view name;
        std::string_view table_xml;
    };
    constexpr auto geometry_cases = std::array{
        geometry_case{
            "single_span_64",
            R"(<w:tr><w:tc><w:tcPr><w:gridSpan w:val="64"/></w:tcPr><w:p><w:r><w:t>original</w:t></w:r></w:p></w:tc></w:tr>)"},
        geometry_case{
            "summed_span_64",
            R"(<w:tr><w:tc><w:tcPr><w:gridSpan w:val="32"/></w:tcPr><w:p><w:r><w:t>original</w:t></w:r></w:p></w:tc><w:tc><w:tcPr><w:gridSpan w:val="32"/></w:tcPr><w:p><w:r><w:t>right</w:t></w:r></w:p></w:tc></w:tr>)"},
        geometry_case{
            "zero_span",
            R"(<w:tr><w:tc><w:tcPr><w:gridSpan w:val="0"/></w:tcPr><w:p><w:r><w:t>original</w:t></w:r></w:p></w:tc></w:tr>)"},
        geometry_case{
            "missing_span_value",
            R"(<w:tr><w:tc><w:tcPr><w:gridSpan/></w:tcPr><w:p><w:r><w:t>original</w:t></w:r></w:p></w:tc></w:tr>)"},
        geometry_case{
            "invalid_span_value",
            R"(<w:tr><w:tc><w:tcPr><w:gridSpan w:val="invalid"/></w:tcPr><w:p><w:r><w:t>original</w:t></w:r></w:p></w:tc></w:tr>)"},
        geometry_case{
            "duplicate_span",
            R"(<w:tr><w:tc><w:tcPr><w:gridSpan w:val="1"/><w:gridSpan w:val="1"/></w:tcPr><w:p><w:r><w:t>original</w:t></w:r></w:p></w:tc></w:tr>)"},
        geometry_case{
            "duplicate_cell_properties",
            R"(<w:tr><w:tc><w:tcPr/><w:tcPr/><w:p><w:r><w:t>original</w:t></w:r></w:p></w:tc></w:tr>)"},
        geometry_case{
            "duplicate_table_properties",
            R"(<w:tblPr/><w:tblPr/><w:tr><w:tc><w:p><w:r><w:t>original</w:t></w:r></w:p></w:tc></w:tr>)"},
        geometry_case{
            "duplicate_table_grid",
            R"(<w:tblGrid><w:gridCol/></w:tblGrid><w:tblGrid><w:gridCol/></w:tblGrid><w:tr><w:tc><w:p><w:r><w:t>original</w:t></w:r></w:p></w:tc></w:tr>)"},
    };

    for (const auto &geometry : geometry_cases) {
        CAPTURE(geometry.name);
        const auto target =
            fs::current_path() /
            ("table_append_cell_invalid_" + std::string{geometry.name} +
             ".docx");
        fs::remove(target);
        write_test_docx(
            target,
            std::string{
                R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:body><w:tbl>)"} +
                std::string{geometry.table_xml} +
                R"(</w:tbl></w:body></w:document>)");

        featherdoc::Document document(target);
        REQUIRE_FALSE(document.open());
        REQUIRE_FALSE(document.save());
        const auto xml_before =
            read_test_docx_entry(target, test_document_xml_entry);

        auto table = document.tables();
        REQUIRE(table.valid());
        auto row = table.rows();
        REQUIRE(row.valid());
        auto original_cell = row.cells();
        REQUIRE(original_cell.valid());
        auto original_paragraph = original_cell.paragraphs();
        auto original_run = original_paragraph.runs();
        REQUIRE(original_paragraph.valid());
        REQUIRE(original_run.valid());

        const auto appended = row.append_cell();
        CHECK_FALSE(appended.valid());
        CHECK(table.valid());
        CHECK(row.valid());
        CHECK(original_cell.valid());
        CHECK(original_paragraph.valid());
        CHECK(original_run.valid());

        REQUIRE_FALSE(document.save());
        CHECK_EQ(read_test_docx_entry(target, test_document_xml_entry),
                 xml_before);

        fs::remove(target);
    }
}

TEST_CASE("table row and cell append reject column counts above the safety limit") {
    namespace fs = std::filesystem;

    const fs::path target =
        fs::current_path() / "table_append_column_safety_limit.docx";
    fs::remove(target);

    constexpr auto safe_column_limit = std::size_t{63U};
    featherdoc::Document doc(target);
    REQUIRE_FALSE(doc.create_empty());
    auto table = doc.append_table(1U, safe_column_limit);
    REQUIRE(table.has_next());

    const auto rejected_row = table.append_row(safe_column_limit + 1U);
    CHECK_FALSE(rejected_row.has_next());
    auto row = table.rows();
    REQUIRE(row.has_next());
    const auto rejected_cell = row.append_cell();
    CHECK_FALSE(rejected_cell.has_next());

    CHECK_FALSE(doc.save());
    const auto xml_text = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(xml_text.c_str()));
    const auto table_node =
        xml_document.child("w:document").child("w:body").child("w:tbl");
    REQUIRE(table_node != pugi::xml_node{});
    CHECK_EQ(count_named_children(table_node, "w:tr"), 1U);
    CHECK_EQ(count_named_children(table_node.child("w:tr"), "w:tc"),
             safe_column_limit);
    CHECK_EQ(count_named_children(table_node.child("w:tblGrid"), "w:gridCol"),
             safe_column_limit);

    fs::remove(target);
}

TEST_CASE("document append table rejects oversized column counts atomically") {
    namespace fs = std::filesystem;

    const fs::path target =
        fs::current_path() / "document_append_table_column_safety_limit.docx";
    fs::remove(target);

    constexpr auto safe_column_limit = std::size_t{63U};
    featherdoc::Document doc(target);
    REQUIRE_FALSE(doc.create_empty());
    REQUIRE(doc.paragraphs().set_text("keep body unchanged"));
    REQUIRE_FALSE(doc.save());
    const auto xml_before_rejection =
        read_test_docx_entry(target, test_document_xml_entry);

    const auto rejected = doc.append_table(1U, safe_column_limit + 1U);
    CHECK_FALSE(rejected.has_next());
    CHECK_FALSE(doc.tables().has_next());
    REQUIRE_FALSE(doc.save());
    CHECK_EQ(read_test_docx_entry(target, test_document_xml_entry),
             xml_before_rejection);

    auto accepted = doc.append_table(1U, safe_column_limit);
    REQUIRE(accepted.has_next());
    auto accepted_row = accepted.rows();
    REQUIRE(accepted_row.has_next());
    auto accepted_cell = accepted_row.cells();
    auto cell_count = std::size_t{0U};
    while (accepted_cell.has_next()) {
        ++cell_count;
        accepted_cell.next();
    }
    CHECK_EQ(cell_count, safe_column_limit);

    fs::remove(target);
}

TEST_CASE("template part append table rejects oversized column counts atomically") {
    namespace fs = std::filesystem;

    const fs::path target =
        fs::current_path() / "template_append_table_column_safety_limit.docx";
    fs::remove(target);

    constexpr auto safe_column_limit = std::size_t{63U};
    featherdoc::Document doc(target);
    REQUIRE_FALSE(doc.create_empty());
    auto body = doc.body_template();
    REQUIRE(static_cast<bool>(body));
    REQUIRE(body.paragraphs().set_text("keep template body unchanged"));
    REQUIRE_FALSE(doc.save());
    const auto xml_before_rejection =
        read_test_docx_entry(target, test_document_xml_entry);

    const auto rejected = body.append_table(2U, safe_column_limit + 1U);
    CHECK_FALSE(rejected.has_next());
    CHECK_FALSE(body.tables().has_next());
    REQUIRE_FALSE(doc.save());
    CHECK_EQ(read_test_docx_entry(target, test_document_xml_entry),
             xml_before_rejection);

    auto accepted = body.append_table(1U, safe_column_limit);
    REQUIRE(accepted.has_next());
    auto accepted_row = accepted.rows();
    REQUIRE(accepted_row.has_next());
    auto accepted_cell = accepted_row.cells();
    auto cell_count = std::size_t{0U};
    while (accepted_cell.has_next()) {
        ++cell_count;
        accepted_cell.next();
    }
    CHECK_EQ(cell_count, safe_column_limit);

    fs::remove(target);
}

TEST_CASE("table clone insertions reject invalid column geometry atomically") {
    namespace fs = std::filesystem;

    struct geometry_case {
        std::string_view name;
        std::string_view cells_xml;
    };
    constexpr auto geometry_cases = std::array{
        geometry_case{
            "single_span_64",
            R"(<w:tc><w:tcPr><w:gridSpan w:val="64"/></w:tcPr><w:p><w:r><w:t>original</w:t></w:r></w:p></w:tc>)"},
        geometry_case{
            "summed_span_64",
            R"(<w:tc><w:tcPr><w:gridSpan w:val="32"/></w:tcPr><w:p><w:r><w:t>left</w:t></w:r></w:p></w:tc><w:tc><w:tcPr><w:gridSpan w:val="32"/></w:tcPr><w:p><w:r><w:t>right</w:t></w:r></w:p></w:tc>)"},
        geometry_case{
            "zero_span",
            R"(<w:tc><w:tcPr><w:gridSpan w:val="0"/></w:tcPr><w:p><w:r><w:t>original</w:t></w:r></w:p></w:tc>)"},
        geometry_case{
            "missing_span_value",
            R"(<w:tc><w:tcPr><w:gridSpan/></w:tcPr><w:p><w:r><w:t>original</w:t></w:r></w:p></w:tc>)"},
        geometry_case{
            "invalid_span_value",
            R"(<w:tc><w:tcPr><w:gridSpan w:val="invalid"/></w:tcPr><w:p><w:r><w:t>original</w:t></w:r></w:p></w:tc>)"},
    };
    constexpr auto operation_names = std::array{
        std::string_view{"insert_row_before"},
        std::string_view{"insert_row_after"},
        std::string_view{"insert_table_like_before"},
        std::string_view{"insert_table_like_after"},
        std::string_view{"insert_cell_before"},
        std::string_view{"insert_cell_after"},
    };

    for (const auto &geometry : geometry_cases) {
        const auto source = fs::current_path() /
                            ("table_clone_invalid_" +
                             std::string{geometry.name} + ".docx");
        fs::remove(source);
        write_test_docx(
            source,
            std::string{
                R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:body><w:tbl><w:tr>)"} +
                std::string{geometry.cells_xml} +
                R"(</w:tr></w:tbl></w:body></w:document>)");

        for (std::size_t operation_index = 0U;
             operation_index < operation_names.size(); ++operation_index) {
            const auto stem = "table_clone_invalid_" +
                              std::string{geometry.name} + "_" +
                              std::string{operation_names[operation_index]};
            const auto before = fs::current_path() / (stem + ".before.docx");
            const auto after = fs::current_path() / (stem + ".after.docx");
            fs::remove(before);
            fs::remove(after);

            featherdoc::Document document(source);
            REQUIRE_FALSE(document.open());
            REQUIRE_FALSE(document.save_as(before));

            auto table = document.tables();
            REQUIRE(table.has_next());
            if (operation_index < 2U) {
                auto row = table.rows();
                REQUIRE(row.has_next());
                const auto inserted = operation_index == 0U
                                          ? row.insert_row_before()
                                          : row.insert_row_after();
                CHECK_FALSE(inserted.has_next());
            } else if (operation_index < 4U) {
                const auto inserted = operation_index == 2U
                                          ? table.insert_table_like_before()
                                          : table.insert_table_like_after();
                CHECK_FALSE(inserted.has_next());
            } else {
                auto row = table.rows();
                REQUIRE(row.has_next());
                auto cell = row.cells();
                REQUIRE(cell.has_next());
                const auto inserted = operation_index == 4U
                                          ? cell.insert_cell_before()
                                          : cell.insert_cell_after();
                CHECK_FALSE(inserted.valid());
            }

            REQUIRE_FALSE(document.save_as(after));
            CHECK_EQ(read_test_docx_entry(after, test_document_xml_entry),
                     read_test_docx_entry(before, test_document_xml_entry));

            fs::remove(before);
            fs::remove(after);
        }
        fs::remove(source);
    }
}

TEST_CASE("table clone insertions reject zero-dimensional source structures") {
    namespace fs = std::filesystem;

    const auto verify_unchanged = [](const fs::path &source,
                                     bool clone_row) {
        featherdoc::Document document(source);
        REQUIRE_FALSE(document.open());
        REQUIRE_FALSE(document.save());
        const auto xml_before =
            read_test_docx_entry(source, test_document_xml_entry);

        auto table = document.tables();
        REQUIRE(table.valid());
        auto old_table = table;
        if (clone_row) {
            auto row = table.rows();
            REQUIRE(row.valid());
            auto old_row = row;
            CHECK_FALSE(row.insert_row_before().valid());
            CHECK_FALSE(row.insert_row_after().valid());
            CHECK(row.valid());
            CHECK(old_row.valid());
        }
        CHECK_FALSE(table.insert_table_like_before().valid());
        CHECK_FALSE(table.insert_table_like_after().valid());
        CHECK(table.valid());
        CHECK(old_table.valid());

        REQUIRE_FALSE(document.save());
        CHECK_EQ(read_test_docx_entry(source, test_document_xml_entry),
                 xml_before);
    };

    const auto empty_row_source =
        fs::current_path() / "table_clone_empty_row_source.docx";
    fs::remove(empty_row_source);
    write_test_docx(
        empty_row_source,
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body><w:tbl><w:tblGrid><w:gridCol w:w="1200"/></w:tblGrid>
    <w:tr><w:trPr/></w:tr>
  </w:tbl></w:body>
</w:document>)");
    verify_unchanged(empty_row_source, true);
    fs::remove(empty_row_source);

    const auto empty_table_source =
        fs::current_path() / "table_clone_empty_table_source.docx";
    fs::remove(empty_table_source);
    write_test_docx(
        empty_table_source,
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body><w:tbl><w:tblGrid><w:gridCol w:w="1200"/></w:tblGrid>
  </w:tbl></w:body>
</w:document>)");
    verify_unchanged(empty_table_source, false);
    fs::remove(empty_table_source);
}

TEST_CASE("table row insertion after stays before intervening markers") {
    namespace fs = std::filesystem;

    const auto target =
        fs::current_path() / "table_row_after_marker_order_utf8.docx";
    fs::remove(target);
    write_test_docx(
        target,
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body><w:tbl><w:tblGrid><w:gridCol w:w="1200"/></w:tblGrid>
    <w:tr><w:tc><w:p><w:r><w:t>锚点😀</w:t></w:r></w:p></w:tc></w:tr>
    <w:bookmarkStart w:id="31" w:name="行间书签"/>
    <w:bookmarkEnd w:id="31"/>
    <w:tr><w:tc><w:p><w:r><w:t>尾行中文</w:t></w:r></w:p></w:tc></w:tr>
  </w:tbl></w:body>
</w:document>)");

    featherdoc::Document document(target);
    REQUIRE_FALSE(document.open());
    auto anchor = document.tables().rows();
    auto old_anchor = anchor;
    auto tail = anchor;
    tail.next();
    REQUIRE(tail.valid());

    auto inserted = anchor.insert_row_after();
    REQUIRE(inserted.valid());
    REQUIRE(inserted.cells().set_text("新行中文😀"));
    CHECK(old_anchor.valid());
    CHECK(tail.valid());
    CHECK_EQ(old_anchor.cells().get_text(), "锚点😀");
    CHECK_EQ(tail.cells().get_text(), "尾行中文");
    REQUIRE_FALSE(document.save());

    const auto xml = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document parsed;
    REQUIRE(parsed.load_string(xml.c_str()));
    const auto table =
        parsed.child("w:document").child("w:body").child("w:tbl");
    auto first = table.child("w:tr");
    auto second = first.next_sibling();
    auto bookmark_start = second.next_sibling();
    auto bookmark_end = bookmark_start.next_sibling();
    auto last = bookmark_end.next_sibling();
    REQUIRE(last != pugi::xml_node{});
    CHECK_EQ(std::string_view{first.name()}, "w:tr");
    CHECK_EQ(std::string_view{second.name()}, "w:tr");
    CHECK_EQ(std::string_view{bookmark_start.name()}, "w:bookmarkStart");
    CHECK_EQ(std::string_view{bookmark_end.name()}, "w:bookmarkEnd");
    CHECK_EQ(std::string_view{last.name()}, "w:tr");
    CHECK_EQ(std::string_view{second.child("w:tc")
                                  .child("w:p")
                                  .child("w:r")
                                  .child("w:t")
                                  .text()
                                  .get()},
             "新行中文😀");

    fs::remove(target);
}

TEST_CASE("document append table reports unopened document state") {
    featherdoc::Document document;

    CHECK_FALSE(document.append_table(1U, 1U).valid());
    CHECK_EQ(document.last_error().code,
             featherdoc::make_error_code(
                 featherdoc::document_errc::document_not_open));
    CHECK_EQ(document.last_error().entry_name, test_document_xml_entry);
    CHECK_NE(document.last_error().detail.find("open() or create_empty()"),
             std::string::npos);
}

TEST_CASE("table cell text replacement retires only the old cell contents") {
    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());

    auto table = document.append_table(1U, 2U);
    REQUIRE(table.valid());
    auto row = table.rows();
    REQUIRE(row.valid());
    auto replaced_cell = row.cells();
    REQUIRE(replaced_cell.set_text("old text"));
    auto old_paragraph = replaced_cell.paragraphs();
    auto old_run = old_paragraph.runs();
    REQUIRE(old_paragraph.valid());
    REQUIRE(old_run.valid());

    auto unaffected_cell = replaced_cell;
    unaffected_cell.next();
    REQUIRE(unaffected_cell.valid());
    REQUIRE(unaffected_cell.set_text("unaffected"));
    auto unaffected_paragraph = unaffected_cell.paragraphs();
    auto unaffected_run = unaffected_paragraph.runs();
    REQUIRE(unaffected_paragraph.valid());
    REQUIRE(unaffected_run.valid());

    CHECK(replaced_cell.set_text("new text"));

    CHECK(replaced_cell.valid());
    CHECK_FALSE(old_paragraph.valid());
    CHECK_FALSE(old_run.valid());
    CHECK_EQ(replaced_cell.get_text(), "new text");
    CHECK(row.valid());
    CHECK(table.valid());
    CHECK(unaffected_cell.valid());
    CHECK(unaffected_paragraph.valid());
    CHECK(unaffected_run.valid());
    CHECK_EQ(unaffected_cell.get_text(), "unaffected");
}

TEST_CASE("table row removal retires the removed row and promoted cell contents") {
    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());

    auto table = document.append_table(3U, 2U);
    REQUIRE(table.valid());
    auto first_row = table.rows();
    REQUIRE(first_row.valid());
    auto removed_row = first_row;

    auto source_cell = first_row.cells();
    REQUIRE(source_cell.set_text("promoted text"));
    auto source_paragraph = source_cell.paragraphs();
    auto source_run = source_paragraph.runs();
    REQUIRE(source_paragraph.valid());
    REQUIRE(source_run.valid());
    REQUIRE(source_cell.merge_down(2U));

    auto promoted_row = first_row;
    promoted_row.next();
    REQUIRE(promoted_row.valid());
    auto promoted_cell = promoted_row.cells();
    REQUIRE(promoted_cell.valid());
    REQUIRE(promoted_cell.set_text("old continuation contents"));
    auto replaced_paragraph = promoted_cell.paragraphs();
    auto replaced_run = replaced_paragraph.runs();
    REQUIRE(replaced_paragraph.valid());
    REQUIRE(replaced_run.valid());

    auto unaffected_cell = promoted_cell;
    unaffected_cell.next();
    REQUIRE(unaffected_cell.valid());
    REQUIRE(unaffected_cell.set_text("unaffected"));
    auto unaffected_paragraph = unaffected_cell.paragraphs();
    auto unaffected_run = unaffected_paragraph.runs();
    REQUIRE(unaffected_paragraph.valid());
    REQUIRE(unaffected_run.valid());

    CHECK(first_row.remove());

    CHECK_FALSE(removed_row.valid());
    CHECK_FALSE(source_cell.valid());
    CHECK_FALSE(source_paragraph.valid());
    CHECK_FALSE(source_run.valid());
    CHECK(promoted_row.valid());
    CHECK(promoted_cell.valid());
    CHECK_FALSE(replaced_paragraph.valid());
    CHECK_FALSE(replaced_run.valid());
    CHECK_EQ(promoted_cell.get_text(), "promoted text");
    CHECK_EQ(promoted_cell.vertical_merge(),
             featherdoc::cell_vertical_merge::restart);
    CHECK(unaffected_cell.valid());
    CHECK(unaffected_paragraph.valid());
    CHECK(unaffected_run.valid());
    CHECK_EQ(unaffected_cell.get_text(), "unaffected");
    CHECK(table.valid());
    CHECK(first_row.valid());
}
