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
