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

TEST_CASE("append_table creates a new editable table in an empty document") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "append_table_create_empty.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto table = doc.append_table(2, 2);
    auto row = table.rows();
    REQUIRE(row.has_next());

    auto cell = row.cells();
    REQUIRE(cell.has_next());
    CHECK(cell.paragraphs().add_run("r0c0").has_next());

    cell.next();
    REQUIRE(cell.has_next());
    CHECK(cell.paragraphs().add_run("r0c1").has_next());

    row.next();
    REQUIRE(row.has_next());

    cell = row.cells();
    REQUIRE(cell.has_next());
    CHECK(cell.paragraphs().add_run("r1c0").has_next());

    cell.next();
    REQUIRE(cell.has_next());
    CHECK(cell.paragraphs().add_run("r1c1").has_next());

    CHECK_FALSE(doc.save());

    const auto xml_text = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(xml_text.c_str()));

    std::ostringstream child_order;
    const auto body = xml_document.child("w:document").child("w:body");
    for (auto child = body.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        child_order << child.name() << '\n';
    }
    CHECK_EQ(child_order.str(), "w:p\nw:tbl\n");

    const auto table_node = body.child("w:tbl");
    REQUIRE(table_node != pugi::xml_node{});
    CHECK(table_node.child("w:tblPr") != pugi::xml_node{});
    const auto table_grid = table_node.child("w:tblGrid");
    REQUIRE(table_grid != pugi::xml_node{});
    CHECK_EQ(count_named_children(table_grid, "w:gridCol"), 2);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_table_text(reopened), "r0c0\nr0c1\nr1c0\nr1c1\n");

    fs::remove(target);
}

TEST_CASE("table append_row and append_cell extend existing tables before sectPr") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "append_table_row_and_cell.docx";
    fs::remove(target);

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p><w:r><w:t>outside</w:t></w:r></w:p>
    <w:tbl>
      <w:tr>
        <w:tc>
          <w:p><w:r><w:t>seed</w:t></w:r></w:p>
        </w:tc>
      </w:tr>
    </w:tbl>
    <w:sectPr/>
  </w:body>
</w:document>
)";
    write_test_docx(target, document_xml);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    auto table = doc.tables();
    REQUIRE(table.has_next());

    auto appended_row = table.append_row();
    REQUIRE(appended_row.has_next());

    auto first_cell = appended_row.cells();
    REQUIRE(first_cell.has_next());
    CHECK(first_cell.paragraphs().add_run("appended one").has_next());

    auto second_cell = appended_row.append_cell();
    REQUIRE(second_cell.has_next());
    CHECK(second_cell.paragraphs().add_run("appended two").has_next());

    CHECK_FALSE(doc.save());

    const auto xml_text = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(xml_text.c_str()));

    std::ostringstream child_order;
    const auto body = xml_document.child("w:document").child("w:body");
    for (auto child = body.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        child_order << child.name() << '\n';
    }
    CHECK_EQ(child_order.str(), "w:p\nw:tbl\nw:sectPr\n");

    const auto table_node = body.child("w:tbl");
    REQUIRE(table_node != pugi::xml_node{});
    CHECK(table_node.child("w:tblPr") != pugi::xml_node{});
    const auto table_grid = table_node.child("w:tblGrid");
    REQUIRE(table_grid != pugi::xml_node{});
    CHECK_EQ(count_named_children(table_grid, "w:gridCol"), 2);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_table_text(reopened), "seed\nappended one\nappended two\n");

    fs::remove(target);
}

TEST_CASE("table cells can set and clear explicit widths") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "table_cell_widths.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto table = doc.append_table(1, 2);
    auto row = table.rows();
    REQUIRE(row.has_next());

    auto cell = row.cells();
    REQUIRE(cell.has_next());
    CHECK_FALSE(cell.width_twips().has_value());
    CHECK(cell.set_width_twips(2400U));
    REQUIRE(cell.width_twips().has_value());
    CHECK_EQ(*cell.width_twips(), 2400U);
    CHECK(cell.paragraphs().add_run("left").has_next());

    cell.next();
    REQUIRE(cell.has_next());
    CHECK_FALSE(cell.width_twips().has_value());
    CHECK(cell.set_width_twips(1800U));
    REQUIRE(cell.width_twips().has_value());
    CHECK_EQ(*cell.width_twips(), 1800U);
    CHECK(cell.clear_width());
    CHECK_FALSE(cell.width_twips().has_value());
    CHECK(cell.paragraphs().add_run("right").has_next());

    CHECK_FALSE(doc.save());

    const auto xml_text = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(xml_text.c_str()));

    const auto table_node = xml_document.child("w:document").child("w:body").child("w:tbl");
    REQUIRE(table_node != pugi::xml_node{});

    const auto row_node = table_node.child("w:tr");
    REQUIRE(row_node != pugi::xml_node{});

    const auto first_cell = row_node.child("w:tc");
    REQUIRE(first_cell != pugi::xml_node{});
    const auto first_width = first_cell.child("w:tcPr").child("w:tcW");
    REQUIRE(first_width != pugi::xml_node{});
    CHECK_EQ(std::string_view{first_width.attribute("w:type").value()}, "dxa");
    CHECK_EQ(std::string_view{first_width.attribute("w:w").value()}, "2400");

    const auto second_cell = first_cell.next_sibling("w:tc");
    REQUIRE(second_cell != pugi::xml_node{});
    CHECK_EQ(second_cell.child("w:tcPr").child("w:tcW"), pugi::xml_node{});

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());

    auto reopened_table = reopened.tables();
    REQUIRE(reopened_table.has_next());
    auto reopened_row = reopened_table.rows();
    REQUIRE(reopened_row.has_next());
    auto reopened_cell = reopened_row.cells();
    REQUIRE(reopened_cell.has_next());
    REQUIRE(reopened_cell.width_twips().has_value());
    CHECK_EQ(*reopened_cell.width_twips(), 2400U);

    reopened_cell.next();
    REQUIRE(reopened_cell.has_next());
    CHECK_FALSE(reopened_cell.width_twips().has_value());

    fs::remove(target);
}

TEST_CASE("table cells can replace text while preserving cell properties") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "table_cell_set_text.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto cell = doc.append_table(1, 1).rows().cells();
    REQUIRE(cell.has_next());
    CHECK(cell.set_fill_color("D9EAF7"));
    CHECK(cell.set_margin_twips(featherdoc::cell_margin_edge::left, 120U));

    auto paragraph = cell.paragraphs();
    REQUIRE(paragraph.has_next());
    CHECK(paragraph.set_text("before"));
    auto extra_paragraph = paragraph.insert_paragraph_after("after");
    REQUIRE(extra_paragraph.has_next());
    CHECK_EQ(cell.get_text(), "before\nafter");

    CHECK(cell.set_text("updated"));
    CHECK_EQ(cell.get_text(), "updated");
    CHECK(cell.set_text("top line\nbottom line"));
    CHECK_EQ(cell.get_text(), "top line\nbottom line");

    REQUIRE(cell.fill_color().has_value());
    CHECK_EQ(*cell.fill_color(), "D9EAF7");
    REQUIRE(cell.margin_twips(featherdoc::cell_margin_edge::left).has_value());
    CHECK_EQ(*cell.margin_twips(featherdoc::cell_margin_edge::left), 120U);

    CHECK_FALSE(doc.save());

    const auto xml_text = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(xml_text.c_str()));

    const auto cell_node = xml_document.child("w:document")
                               .child("w:body")
                               .child("w:tbl")
                               .child("w:tr")
                               .child("w:tc");
    REQUIRE(cell_node != pugi::xml_node{});
    CHECK_EQ(count_named_children(cell_node, "w:p"), 1);
    CHECK_EQ(std::string_view{
                 cell_node.child("w:tcPr").child("w:shd").attribute("w:fill").value()},
             "D9EAF7");
    const auto cell_run = cell_node.child("w:p").child("w:r");
    REQUIRE(cell_run != pugi::xml_node{});
    CHECK_EQ(std::string{cell_run.child("w:t").text().get()}, "top line");
    REQUIRE(cell_run.child("w:br") != pugi::xml_node{});

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());

    auto reopened_cell = reopened.tables().rows().cells();
    REQUIRE(reopened_cell.has_next());
    CHECK_EQ(reopened_cell.get_text(), "top line\nbottom line");
    REQUIRE(reopened_cell.fill_color().has_value());
    CHECK_EQ(*reopened_cell.fill_color(), "D9EAF7");
    REQUIRE(reopened_cell.margin_twips(featherdoc::cell_margin_edge::left).has_value());
    CHECK_EQ(*reopened_cell.margin_twips(featherdoc::cell_margin_edge::left), 120U);

    fs::remove(target);
}

TEST_CASE("table rows can remove a middle row and keep the wrapper usable") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "table_row_remove.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto table = doc.append_table(3, 1);
    auto row = table.rows();
    REQUIRE(row.has_next());
    CHECK(row.cells().set_text("row-1"));

    row.next();
    REQUIRE(row.has_next());
    CHECK(row.cells().set_text("row-2"));
    auto removed_row = row;

    row.next();
    REQUIRE(row.has_next());
    CHECK(row.cells().set_text("row-3"));

    CHECK(removed_row.remove());
    CHECK(removed_row.has_next());
    CHECK_EQ(removed_row.cells().get_text(), "row-3");

    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_table_text(reopened), "row-1\nrow-3\n");

    fs::remove(target);
}

TEST_CASE("table cells can insert a formatted column after the current column") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "table_column_insert_after.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto table = doc.append_table(2, 2);
    auto row = table.rows();
    REQUIRE(row.has_next());

    auto source_cell = row.cells();
    REQUIRE(source_cell.has_next());
    CHECK(source_cell.set_width_twips(2400U));
    CHECK(source_cell.set_fill_color("D9EAF7"));
    CHECK(source_cell.set_text("left-header"));

    auto right_cell = source_cell;
    right_cell.next();
    REQUIRE(right_cell.has_next());
    CHECK(right_cell.set_text("right-header"));

    row.next();
    REQUIRE(row.has_next());
    auto body_left = row.cells();
    REQUIRE(body_left.has_next());
    CHECK(body_left.set_text("left-body"));
    auto body_right = body_left;
    body_right.next();
    REQUIRE(body_right.has_next());
    CHECK(body_right.set_text("right-body"));

    auto inserted_cell = source_cell.insert_cell_after();
    REQUIRE(inserted_cell.has_next());
    CHECK(source_cell.has_next());
    CHECK_EQ(source_cell.get_text(), "");
    REQUIRE(source_cell.fill_color().has_value());
    CHECK_EQ(*source_cell.fill_color(), "D9EAF7");
    REQUIRE(source_cell.width_twips().has_value());
    CHECK_EQ(*source_cell.width_twips(), 2400U);
    CHECK(inserted_cell.set_text("inserted-header"));

    auto body_row = table.rows();
    body_row.next();
    REQUIRE(body_row.has_next());
    auto inserted_body = body_row.cells();
    inserted_body.next();
    REQUIRE(inserted_body.has_next());
    CHECK_EQ(inserted_body.get_text(), "");
    CHECK(inserted_body.set_text("inserted-body"));

    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_table_text(reopened),
             "left-header\ninserted-header\nright-header\nleft-body\ninserted-body\nright-body\n");

    const auto xml_text = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(xml_text.c_str()));
    const auto table_node = xml_document.child("w:document").child("w:body").child("w:tbl");
    REQUIRE(table_node != pugi::xml_node{});
    CHECK_EQ(count_named_children(table_node.child("w:tblGrid"), "w:gridCol"), 3U);
    auto first_row = table_node.child("w:tr");
    REQUIRE(first_row != pugi::xml_node{});
    auto second_row = first_row.next_sibling("w:tr");
    REQUIRE(second_row != pugi::xml_node{});
    CHECK_EQ(count_named_children(first_row, "w:tc"), 3U);
    CHECK_EQ(count_named_children(second_row, "w:tc"), 3U);

    fs::remove(target);
}

TEST_CASE("table cell insert after inherits the source grid-column width") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "table_column_insert_after_widths.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto table = doc.append_table(2, 3);
    REQUIRE(table.has_next());
    CHECK(table.set_layout_mode(featherdoc::table_layout_mode::fixed));
    CHECK(table.set_column_width_twips(0U, 1200U));
    CHECK(table.set_column_width_twips(1U, 2200U));
    CHECK(table.set_column_width_twips(2U, 3200U));

    auto row = table.rows();
    REQUIRE(row.has_next());
    auto cell = row.cells();
    REQUIRE(cell.has_next());
    CHECK(cell.set_text("left"));
    cell.next();
    REQUIRE(cell.has_next());
    CHECK(cell.set_text("middle"));
    cell.next();
    REQUIRE(cell.has_next());
    CHECK(cell.set_text("right"));

    row.next();
    REQUIRE(row.has_next());
    cell = row.cells();
    REQUIRE(cell.has_next());
    CHECK(cell.set_text("row2-left"));
    cell.next();
    REQUIRE(cell.has_next());
    CHECK(cell.set_text("row2-middle"));
    cell.next();
    REQUIRE(cell.has_next());
    CHECK(cell.set_text("row2-right"));

    auto inserted = table.rows().cells().insert_cell_after();
    REQUIRE(inserted.has_next());
    CHECK(inserted.set_text("inserted"));
    REQUIRE(inserted.width_twips().has_value());
    CHECK_EQ(*inserted.width_twips(), 1200U);

    REQUIRE(table.column_width_twips(0U).has_value());
    CHECK_EQ(*table.column_width_twips(0U), 1200U);
    REQUIRE(table.column_width_twips(1U).has_value());
    CHECK_EQ(*table.column_width_twips(1U), 1200U);
    REQUIRE(table.column_width_twips(2U).has_value());
    CHECK_EQ(*table.column_width_twips(2U), 2200U);
    REQUIRE(table.column_width_twips(3U).has_value());
    CHECK_EQ(*table.column_width_twips(3U), 3200U);

    CHECK_FALSE(doc.save());

    const auto xml_text = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(xml_text.c_str()));
    const auto table_node = xml_document.child("w:document").child("w:body").child("w:tbl");
    REQUIRE(table_node != pugi::xml_node{});

    const auto grid_widths = collect_table_grid_width_values(table_node);
    REQUIRE_EQ(grid_widths.size(), 4U);
    CHECK_EQ(grid_widths[0], "1200");
    CHECK_EQ(grid_widths[1], "1200");
    CHECK_EQ(grid_widths[2], "2200");
    CHECK_EQ(grid_widths[3], "3200");

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    auto reopened_table = reopened.tables();
    REQUIRE(reopened_table.has_next());
    auto reopened_inserted = reopened_table.rows().cells();
    reopened_inserted.next();
    REQUIRE(reopened_inserted.has_next());
    REQUIRE(reopened_inserted.width_twips().has_value());
    CHECK_EQ(*reopened_inserted.width_twips(), 1200U);
    REQUIRE(reopened_table.column_width_twips(0U).has_value());
    CHECK_EQ(*reopened_table.column_width_twips(0U), 1200U);
    REQUIRE(reopened_table.column_width_twips(1U).has_value());
    CHECK_EQ(*reopened_table.column_width_twips(1U), 1200U);
    REQUIRE(reopened_table.column_width_twips(2U).has_value());
    CHECK_EQ(*reopened_table.column_width_twips(2U), 2200U);
    REQUIRE(reopened_table.column_width_twips(3U).has_value());
    CHECK_EQ(*reopened_table.column_width_twips(3U), 3200U);

    fs::remove(target);
}

TEST_CASE("table cells can insert a formatted column before the current column") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "table_column_insert_before.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto table = doc.append_table(2, 2);
    auto row = table.rows();
    REQUIRE(row.has_next());

    auto first_cell = row.cells();
    REQUIRE(first_cell.has_next());
    CHECK(first_cell.set_text("left-header"));

    auto source_cell = first_cell;
    source_cell.next();
    REQUIRE(source_cell.has_next());
    CHECK(source_cell.set_width_twips(3000U));
    CHECK(source_cell.set_fill_color("FFF2CC"));
    CHECK(source_cell.set_text("right-header"));

    row.next();
    REQUIRE(row.has_next());
    auto body_left = row.cells();
    REQUIRE(body_left.has_next());
    CHECK(body_left.set_text("left-body"));
    auto body_source = body_left;
    body_source.next();
    REQUIRE(body_source.has_next());
    CHECK(body_source.set_text("right-body"));

    auto inserted_cell = source_cell.insert_cell_before();
    REQUIRE(inserted_cell.has_next());
    CHECK(source_cell.has_next());
    CHECK_EQ(source_cell.get_text(), "");
    REQUIRE(source_cell.fill_color().has_value());
    CHECK_EQ(*source_cell.fill_color(), "FFF2CC");
    REQUIRE(source_cell.width_twips().has_value());
    CHECK_EQ(*source_cell.width_twips(), 3000U);
    CHECK(inserted_cell.set_text("inserted-header"));

    auto body_row = table.rows();
    body_row.next();
    REQUIRE(body_row.has_next());
    auto inserted_body = body_row.cells();
    inserted_body.next();
    REQUIRE(inserted_body.has_next());
    CHECK_EQ(inserted_body.get_text(), "");
    CHECK(inserted_body.set_text("inserted-body"));

    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_table_text(reopened),
             "left-header\ninserted-header\nright-header\nleft-body\ninserted-body\nright-body\n");

    fs::remove(target);
}

TEST_CASE("table cells can insert after a merged block and normalize the new cell span") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "table_column_insert_after_merged.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto table = doc.append_table(2, 3);
    auto row = table.rows();
    REQUIRE(row.has_next());
    auto merged_cell = row.cells();
    REQUIRE(merged_cell.has_next());
    CHECK(merged_cell.set_text("merged"));
    CHECK(merged_cell.merge_right(1U));
    auto trailing_cell = merged_cell;
    trailing_cell.next();
    REQUIRE(trailing_cell.has_next());
    CHECK(trailing_cell.set_text("tail"));

    row.next();
    REQUIRE(row.has_next());
    auto body_cell = row.cells();
    REQUIRE(body_cell.has_next());
    CHECK(body_cell.set_text("row2c1"));
    body_cell.next();
    REQUIRE(body_cell.has_next());
    CHECK(body_cell.set_text("row2c2"));
    body_cell.next();
    REQUIRE(body_cell.has_next());
    CHECK(body_cell.set_text("row2c3"));

    auto inserted_cell = merged_cell.insert_cell_after();
    REQUIRE(inserted_cell.has_next());
    CHECK_EQ(inserted_cell.column_span(), 1U);
    CHECK(inserted_cell.set_text("after-merged"));

    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_table_text(reopened),
             "merged\nafter-merged\ntail\nrow2c1\nrow2c2\n\nrow2c3\n");

    const auto xml_text = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(xml_text.c_str()));
    const auto table_node = xml_document.child("w:document").child("w:body").child("w:tbl");
    REQUIRE(table_node != pugi::xml_node{});
    CHECK_EQ(count_named_children(table_node.child("w:tblGrid"), "w:gridCol"), 4U);
    const auto first_row = table_node.child("w:tr");
    REQUIRE(first_row != pugi::xml_node{});
    const auto first_inserted_cell = first_row.child("w:tc").next_sibling("w:tc");
    REQUIRE(first_inserted_cell != pugi::xml_node{});
    const auto inserted_cell_properties = first_inserted_cell.child("w:tcPr");
    CHECK(inserted_cell_properties.child("w:gridSpan") == pugi::xml_node{});
    CHECK(inserted_cell_properties.child("w:vMerge") == pugi::xml_node{});

    fs::remove(target);
}

TEST_CASE("merged tables keep grid widths consistent across insert and remove") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "table_merge_insert_remove_widths.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto table = doc.append_table(2, 4);
    REQUIRE(table.has_next());
    CHECK(table.set_layout_mode(featherdoc::table_layout_mode::fixed));
    CHECK(table.set_column_width_twips(0U, 1200U));
    CHECK(table.set_column_width_twips(1U, 1800U));
    CHECK(table.set_column_width_twips(2U, 2400U));
    CHECK(table.set_column_width_twips(3U, 3000U));

    auto row = table.rows();
    REQUIRE(row.has_next());
    auto merged_header = row.cells();
    REQUIRE(merged_header.has_next());
    CHECK(merged_header.set_text("merged"));
    CHECK(merged_header.merge_right(1U));
    auto header_tail = merged_header;
    header_tail.next();
    REQUIRE(header_tail.has_next());
    CHECK(header_tail.set_text("header-3"));
    header_tail.next();
    REQUIRE(header_tail.has_next());
    CHECK(header_tail.set_text("header-4"));

    row.next();
    REQUIRE(row.has_next());
    auto body_cell = row.cells();
    REQUIRE(body_cell.has_next());
    CHECK(body_cell.set_text("r2c1"));
    body_cell.next();
    REQUIRE(body_cell.has_next());
    CHECK(body_cell.set_text("r2c2"));
    body_cell.next();
    REQUIRE(body_cell.has_next());
    CHECK(body_cell.set_text("r2c3"));
    body_cell.next();
    REQUIRE(body_cell.has_next());
    CHECK(body_cell.set_text("r2c4"));

    auto inserted = table.rows().cells().insert_cell_after();
    REQUIRE(inserted.has_next());
    CHECK(inserted.set_text("inserted"));
    REQUIRE(inserted.width_twips().has_value());
    CHECK_EQ(*inserted.width_twips(), 1800U);
    REQUIRE(table.rows().cells().width_twips().has_value());
    CHECK_EQ(*table.rows().cells().width_twips(), 3000U);

    REQUIRE(table.column_width_twips(0U).has_value());
    CHECK_EQ(*table.column_width_twips(0U), 1200U);
    REQUIRE(table.column_width_twips(1U).has_value());
    CHECK_EQ(*table.column_width_twips(1U), 1800U);
    REQUIRE(table.column_width_twips(2U).has_value());
    CHECK_EQ(*table.column_width_twips(2U), 1800U);
    REQUIRE(table.column_width_twips(3U).has_value());
    CHECK_EQ(*table.column_width_twips(3U), 2400U);
    REQUIRE(table.column_width_twips(4U).has_value());
    CHECK_EQ(*table.column_width_twips(4U), 3000U);

    auto body_row = table.rows();
    body_row.next();
    REQUIRE(body_row.has_next());
    auto removable = body_row.cells();
    removable.next();
    removable.next();
    REQUIRE(removable.has_next());
    CHECK(removable.remove());
    CHECK(removable.has_next());
    CHECK_EQ(removable.get_text(), "r2c3");
    REQUIRE(removable.width_twips().has_value());
    CHECK_EQ(*removable.width_twips(), 2400U);

    REQUIRE(table.column_width_twips(0U).has_value());
    CHECK_EQ(*table.column_width_twips(0U), 1200U);
    REQUIRE(table.column_width_twips(1U).has_value());
    CHECK_EQ(*table.column_width_twips(1U), 1800U);
    REQUIRE(table.column_width_twips(2U).has_value());
    CHECK_EQ(*table.column_width_twips(2U), 2400U);
    REQUIRE(table.column_width_twips(3U).has_value());
    CHECK_EQ(*table.column_width_twips(3U), 3000U);
    CHECK_FALSE(table.column_width_twips(4U).has_value());

    CHECK_FALSE(doc.save());

    const auto xml_text = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(xml_text.c_str()));
    const auto table_node = xml_document.child("w:document").child("w:body").child("w:tbl");
    REQUIRE(table_node != pugi::xml_node{});

    const auto grid_widths = collect_table_grid_width_values(table_node);
    REQUIRE_EQ(grid_widths.size(), 4U);
    CHECK_EQ(grid_widths[0], "1200");
    CHECK_EQ(grid_widths[1], "1800");
    CHECK_EQ(grid_widths[2], "2400");
    CHECK_EQ(grid_widths[3], "3000");

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    auto reopened_table = reopened.tables();
    REQUIRE(reopened_table.has_next());
    auto reopened_merged = reopened_table.rows().cells();
    REQUIRE(reopened_merged.has_next());
    REQUIRE(reopened_merged.width_twips().has_value());
    CHECK_EQ(*reopened_merged.width_twips(), 3000U);
    REQUIRE(reopened_table.column_width_twips(0U).has_value());
    CHECK_EQ(*reopened_table.column_width_twips(0U), 1200U);
    REQUIRE(reopened_table.column_width_twips(1U).has_value());
    CHECK_EQ(*reopened_table.column_width_twips(1U), 1800U);
    REQUIRE(reopened_table.column_width_twips(2U).has_value());
    CHECK_EQ(*reopened_table.column_width_twips(2U), 2400U);
    REQUIRE(reopened_table.column_width_twips(3U).has_value());
    CHECK_EQ(*reopened_table.column_width_twips(3U), 3000U);
    CHECK_FALSE(reopened_table.column_width_twips(4U).has_value());

    fs::remove(target);
}

TEST_CASE("table cell insert rejects columns that intersect horizontal merges") {
    featherdoc::Document doc;
    CHECK_FALSE(doc.create_empty());

    auto table = doc.append_table(2, 3);
    auto row = table.rows();
    REQUIRE(row.has_next());
    auto merged_cell = row.cells();
    REQUIRE(merged_cell.has_next());
    CHECK(merged_cell.set_text("merged"));
    CHECK(merged_cell.merge_right(1U));

    row.next();
    REQUIRE(row.has_next());
    auto blocked_cell = row.cells();
    REQUIRE(blocked_cell.has_next());
    CHECK(blocked_cell.set_text("row2c1"));

    auto inserted = blocked_cell.insert_cell_after();
    CHECK_FALSE(inserted.has_next());
    CHECK(blocked_cell.has_next());
    CHECK_EQ(blocked_cell.get_text(), "row2c1");
}

TEST_CASE("table cells can remove a middle column and keep the wrapper usable") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "table_column_remove.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto table = doc.append_table(2, 3);
    auto row = table.rows();
    REQUIRE(row.has_next());

    auto cell = row.cells();
    REQUIRE(cell.has_next());
    CHECK(cell.set_text("r1c1"));
    cell.next();
    REQUIRE(cell.has_next());
    CHECK(cell.set_text("r1c2"));
    auto removed_cell = cell;
    cell.next();
    REQUIRE(cell.has_next());
    CHECK(cell.set_text("r1c3"));

    row.next();
    REQUIRE(row.has_next());
    cell = row.cells();
    REQUIRE(cell.has_next());
    CHECK(cell.set_text("r2c1"));
    cell.next();
    REQUIRE(cell.has_next());
    CHECK(cell.set_text("r2c2"));
    cell.next();
    REQUIRE(cell.has_next());
    CHECK(cell.set_text("r2c3"));

    CHECK(removed_cell.remove());
    CHECK(removed_cell.has_next());
    CHECK_EQ(removed_cell.get_text(), "r1c3");
    CHECK(removed_cell.set_text("r1c3-updated"));

    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_table_text(reopened), "r1c1\nr1c3-updated\nr2c1\nr2c3\n");

    const auto xml_text = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(xml_text.c_str()));
    const auto table_node = xml_document.child("w:document").child("w:body").child("w:tbl");
    REQUIRE(table_node != pugi::xml_node{});
    CHECK_EQ(count_named_children(table_node.child("w:tblGrid"), "w:gridCol"), 2U);
    auto first_row = table_node.child("w:tr");
    REQUIRE(first_row != pugi::xml_node{});
    auto second_row = first_row.next_sibling("w:tr");
    REQUIRE(second_row != pugi::xml_node{});
    CHECK_EQ(count_named_children(first_row, "w:tc"), 2U);
    CHECK_EQ(count_named_children(second_row, "w:tc"), 2U);

    fs::remove(target);
}

TEST_CASE("table cells can remove the last column and fall back to the previous cell") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "table_column_remove_last.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto table = doc.append_table(2, 3);
    auto row = table.rows();
    REQUIRE(row.has_next());

    auto cell = row.cells();
    REQUIRE(cell.has_next());
    CHECK(cell.set_text("r1c1"));
    cell.next();
    REQUIRE(cell.has_next());
    CHECK(cell.set_text("r1c2"));
    cell.next();
    REQUIRE(cell.has_next());
    CHECK(cell.set_text("r1c3"));
    auto removed_cell = cell;

    row.next();
    REQUIRE(row.has_next());
    cell = row.cells();
    REQUIRE(cell.has_next());
    CHECK(cell.set_text("r2c1"));
    cell.next();
    REQUIRE(cell.has_next());
    CHECK(cell.set_text("r2c2"));
    cell.next();
    REQUIRE(cell.has_next());
    CHECK(cell.set_text("r2c3"));

    CHECK(removed_cell.remove());
    CHECK(removed_cell.has_next());
    CHECK_EQ(removed_cell.get_text(), "r1c2");
    CHECK(removed_cell.set_text("r1c2-updated"));

    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_table_text(reopened), "r1c1\nr1c2-updated\nr2c1\nr2c2\n");

    fs::remove(target);
}

TEST_CASE("table cell remove rejects removing the last remaining column") {
    featherdoc::Document doc;
    CHECK_FALSE(doc.create_empty());

    auto cell = doc.append_table(2, 1).rows().cells();
    REQUIRE(cell.has_next());
    CHECK_FALSE(cell.remove());
    CHECK(cell.has_next());
}

TEST_CASE("table cell remove rejects columns that intersect horizontal merges") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "table_column_remove_merged.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto table = doc.append_table(2, 3);
    auto row = table.rows();
    REQUIRE(row.has_next());
    auto merged_cell = row.cells();
    REQUIRE(merged_cell.has_next());
    CHECK(merged_cell.set_text("merged"));
    CHECK(merged_cell.merge_right(1U));
    auto trailing_cell = row.cells();
    trailing_cell.next();
    REQUIRE(trailing_cell.has_next());
    CHECK(trailing_cell.set_text("tail"));

    row.next();
    REQUIRE(row.has_next());
    auto second_row_cell = row.cells();
    REQUIRE(second_row_cell.has_next());
    CHECK(second_row_cell.set_text("r2c1"));
    second_row_cell.next();
    REQUIRE(second_row_cell.has_next());
    CHECK(second_row_cell.set_text("r2c2"));
    auto blocked_cell = second_row_cell;
    second_row_cell.next();
    REQUIRE(second_row_cell.has_next());
    CHECK(second_row_cell.set_text("r2c3"));

    CHECK_FALSE(blocked_cell.remove());
    CHECK(blocked_cell.has_next());
    CHECK_EQ(blocked_cell.get_text(), "r2c2");

    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_table_text(reopened), "merged\ntail\nr2c1\nr2c2\nr2c3\n");

    const auto xml_text = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(xml_text.c_str()));
    const auto table_node = xml_document.child("w:document").child("w:body").child("w:tbl");
    REQUIRE(table_node != pugi::xml_node{});
    CHECK_EQ(count_named_children(table_node.child("w:tblGrid"), "w:gridCol"), 3U);

    fs::remove(target);
}
