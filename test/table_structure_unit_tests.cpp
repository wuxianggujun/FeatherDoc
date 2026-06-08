#include <filesystem>
#include <sstream>
#include <string>
#include <string_view>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
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

TEST_CASE("table cells can merge right across following cells") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "table_cell_merge_right.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto table = doc.append_table(1, 3);
    auto row = table.rows();
    REQUIRE(row.has_next());

    auto cell = row.cells();
    REQUIRE(cell.has_next());
    CHECK(cell.paragraphs().add_run("A").has_next());

    cell.next();
    REQUIRE(cell.has_next());
    CHECK(cell.paragraphs().add_run("B").has_next());

    cell.next();
    REQUIRE(cell.has_next());
    CHECK(cell.paragraphs().add_run("C").has_next());

    auto merged = row.cells();
    REQUIRE(merged.has_next());
    CHECK_EQ(merged.column_span(), 1U);
    CHECK(merged.merge_right(1U));
    CHECK_EQ(merged.column_span(), 2U);
    CHECK_FALSE(merged.merge_right(5U));

    CHECK_FALSE(doc.save());

    const auto xml_text = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(xml_text.c_str()));

    const auto table_node = xml_document.child("w:document").child("w:body").child("w:tbl");
    REQUIRE(table_node != pugi::xml_node{});
    const auto table_grid = table_node.child("w:tblGrid");
    REQUIRE(table_grid != pugi::xml_node{});
    CHECK_EQ(count_named_children(table_grid, "w:gridCol"), 3);

    const auto row_node = table_node.child("w:tr");
    REQUIRE(row_node != pugi::xml_node{});
    CHECK_EQ(count_named_children(row_node, "w:tc"), 2);

    const auto first_cell = row_node.child("w:tc");
    REQUIRE(first_cell != pugi::xml_node{});
    const auto grid_span = first_cell.child("w:tcPr").child("w:gridSpan");
    REQUIRE(grid_span != pugi::xml_node{});
    CHECK_EQ(std::string_view{grid_span.attribute("w:val").value()}, "2");

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_table_text(reopened), "A\nC\n");

    auto reopened_table = reopened.tables();
    REQUIRE(reopened_table.has_next());
    auto reopened_row = reopened_table.rows();
    REQUIRE(reopened_row.has_next());
    auto reopened_cell = reopened_row.cells();
    REQUIRE(reopened_cell.has_next());
    CHECK_EQ(reopened_cell.column_span(), 2U);

    reopened_cell.next();
    REQUIRE(reopened_cell.has_next());
    CHECK_EQ(reopened_cell.column_span(), 1U);
    reopened_cell.next();
    CHECK_FALSE(reopened_cell.has_next());

    fs::remove(target);
}

TEST_CASE("table cells can merge down across following rows") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "table_cell_merge_down.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto table = doc.append_table(3, 2);
    auto row = table.rows();
    REQUIRE(row.has_next());

    auto first_row_cell = row.cells();
    REQUIRE(first_row_cell.has_next());
    CHECK(first_row_cell.paragraphs().add_run("A").has_next());
    first_row_cell.next();
    REQUIRE(first_row_cell.has_next());
    CHECK(first_row_cell.paragraphs().add_run("B").has_next());

    row.next();
    REQUIRE(row.has_next());
    auto second_row_cell = row.cells();
    REQUIRE(second_row_cell.has_next());
    CHECK(second_row_cell.paragraphs().add_run("C").has_next());
    second_row_cell.next();
    REQUIRE(second_row_cell.has_next());
    CHECK(second_row_cell.paragraphs().add_run("D").has_next());

    row.next();
    REQUIRE(row.has_next());
    auto third_row_cell = row.cells();
    REQUIRE(third_row_cell.has_next());
    CHECK(third_row_cell.paragraphs().add_run("E").has_next());
    third_row_cell.next();
    REQUIRE(third_row_cell.has_next());
    CHECK(third_row_cell.paragraphs().add_run("F").has_next());

    auto merged_row = table.rows();
    REQUIRE(merged_row.has_next());
    auto merged_cell = merged_row.cells();
    REQUIRE(merged_cell.has_next());
    CHECK(merged_cell.merge_down(2U));
    CHECK_FALSE(merged_cell.merge_down(1U));

    CHECK_FALSE(doc.save());

    const auto xml_text = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(xml_text.c_str()));

    const auto table_node = xml_document.child("w:document").child("w:body").child("w:tbl");
    REQUIRE(table_node != pugi::xml_node{});
    CHECK_EQ(count_named_children(table_node, "w:tr"), 3);

    auto first_row = table_node.child("w:tr");
    REQUIRE(first_row != pugi::xml_node{});
    auto second_row = first_row.next_sibling("w:tr");
    REQUIRE(second_row != pugi::xml_node{});
    auto third_row = second_row.next_sibling("w:tr");
    REQUIRE(third_row != pugi::xml_node{});

    const auto first_cell = first_row.child("w:tc");
    const auto second_cell = second_row.child("w:tc");
    const auto third_cell = third_row.child("w:tc");
    REQUIRE(first_cell != pugi::xml_node{});
    REQUIRE(second_cell != pugi::xml_node{});
    REQUIRE(third_cell != pugi::xml_node{});

    CHECK_EQ(std::string_view{
                 first_cell.child("w:tcPr").child("w:vMerge").attribute("w:val").value()},
             "restart");
    CHECK_EQ(std::string_view{
                 second_cell.child("w:tcPr").child("w:vMerge").attribute("w:val").value()},
             "continue");
    CHECK_EQ(std::string_view{
                 third_cell.child("w:tcPr").child("w:vMerge").attribute("w:val").value()},
             "continue");
    CHECK_EQ(count_named_descendants(second_cell, "w:t"), 0U);
    CHECK_EQ(count_named_descendants(third_cell, "w:t"), 0U);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_table_text(reopened), "A\nB\n\nD\n\nF\n");

    fs::remove(target);
}

TEST_CASE("fixed-layout merge right synchronizes merged cell widths to tblGrid") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "table_cell_merge_right_fixed_widths.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto table = doc.append_table(2, 3);
    REQUIRE(table.has_next());
    CHECK(table.set_layout_mode(featherdoc::table_layout_mode::fixed));
    CHECK(table.set_column_width_twips(0U, 1200U));
    CHECK(table.set_column_width_twips(1U, 1800U));
    CHECK(table.set_column_width_twips(2U, 2400U));

    auto row = table.rows();
    REQUIRE(row.has_next());
    auto header_cell = row.cells();
    REQUIRE(header_cell.has_next());
    CHECK(header_cell.set_text("A"));
    REQUIRE(header_cell.width_twips().has_value());
    CHECK_EQ(*header_cell.width_twips(), 1200U);

    header_cell.next();
    REQUIRE(header_cell.has_next());
    CHECK(header_cell.set_text("B"));
    REQUIRE(header_cell.width_twips().has_value());
    CHECK_EQ(*header_cell.width_twips(), 1800U);

    header_cell.next();
    REQUIRE(header_cell.has_next());
    CHECK(header_cell.set_text("C"));
    REQUIRE(header_cell.width_twips().has_value());
    CHECK_EQ(*header_cell.width_twips(), 2400U);

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

    auto merged = table.rows().cells();
    REQUIRE(merged.has_next());
    CHECK(merged.merge_right(1U));
    CHECK_EQ(merged.column_span(), 2U);
    REQUIRE(merged.width_twips().has_value());
    CHECK_EQ(*merged.width_twips(), 3000U);

    auto trailing = merged;
    trailing.next();
    REQUIRE(trailing.has_next());
    CHECK_EQ(trailing.get_text(), "C");
    REQUIRE(trailing.width_twips().has_value());
    CHECK_EQ(*trailing.width_twips(), 2400U);

    REQUIRE(table.column_width_twips(0U).has_value());
    CHECK_EQ(*table.column_width_twips(0U), 1200U);
    REQUIRE(table.column_width_twips(1U).has_value());
    CHECK_EQ(*table.column_width_twips(1U), 1800U);
    REQUIRE(table.column_width_twips(2U).has_value());
    CHECK_EQ(*table.column_width_twips(2U), 2400U);

    body_cell = row.cells();
    REQUIRE(body_cell.has_next());
    REQUIRE(body_cell.width_twips().has_value());
    CHECK_EQ(*body_cell.width_twips(), 1200U);
    body_cell.next();
    REQUIRE(body_cell.has_next());
    REQUIRE(body_cell.width_twips().has_value());
    CHECK_EQ(*body_cell.width_twips(), 1800U);
    body_cell.next();
    REQUIRE(body_cell.has_next());
    REQUIRE(body_cell.width_twips().has_value());
    CHECK_EQ(*body_cell.width_twips(), 2400U);

    CHECK_FALSE(doc.save());

    const auto xml_text = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(xml_text.c_str()));

    const auto table_node = xml_document.child("w:document").child("w:body").child("w:tbl");
    REQUIRE(table_node != pugi::xml_node{});

    const auto grid_widths = collect_table_grid_width_values(table_node);
    REQUIRE_EQ(grid_widths.size(), 3U);
    CHECK_EQ(grid_widths[0], "1200");
    CHECK_EQ(grid_widths[1], "1800");
    CHECK_EQ(grid_widths[2], "2400");

    const auto first_row = table_node.child("w:tr");
    REQUIRE(first_row != pugi::xml_node{});
    const auto second_row = first_row.next_sibling("w:tr");
    REQUIRE(second_row != pugi::xml_node{});
    CHECK_EQ(count_named_children(first_row, "w:tc"), 2U);

    const auto first_row_widths = collect_row_cell_width_values(first_row);
    REQUIRE_EQ(first_row_widths.size(), 2U);
    CHECK_EQ(first_row_widths[0], "3000");
    CHECK_EQ(first_row_widths[1], "2400");

    const auto second_row_widths = collect_row_cell_width_values(second_row);
    REQUIRE_EQ(second_row_widths.size(), 3U);
    CHECK_EQ(second_row_widths[0], "1200");
    CHECK_EQ(second_row_widths[1], "1800");
    CHECK_EQ(second_row_widths[2], "2400");

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_table_text(reopened), "A\nC\nr2c1\nr2c2\nr2c3\n");

    auto reopened_table = reopened.tables();
    REQUIRE(reopened_table.has_next());
    auto reopened_header = reopened_table.rows().cells();
    REQUIRE(reopened_header.has_next());
    REQUIRE(reopened_header.width_twips().has_value());
    CHECK_EQ(*reopened_header.width_twips(), 3000U);
    reopened_header.next();
    REQUIRE(reopened_header.has_next());
    REQUIRE(reopened_header.width_twips().has_value());
    CHECK_EQ(*reopened_header.width_twips(), 2400U);

    fs::remove(target);
}

TEST_CASE("fixed-layout merge down preserves cell widths on merged columns") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "table_cell_merge_down_fixed_widths.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto table = doc.append_table(3, 2);
    REQUIRE(table.has_next());
    CHECK(table.set_layout_mode(featherdoc::table_layout_mode::fixed));
    CHECK(table.set_column_width_twips(0U, 1200U));
    CHECK(table.set_column_width_twips(1U, 1800U));

    auto row = table.rows();
    REQUIRE(row.has_next());
    auto cell = row.cells();
    REQUIRE(cell.has_next());
    CHECK(cell.set_text("A"));
    REQUIRE(cell.width_twips().has_value());
    CHECK_EQ(*cell.width_twips(), 1200U);
    cell.next();
    REQUIRE(cell.has_next());
    CHECK(cell.set_text("B"));
    REQUIRE(cell.width_twips().has_value());
    CHECK_EQ(*cell.width_twips(), 1800U);

    row.next();
    REQUIRE(row.has_next());
    cell = row.cells();
    REQUIRE(cell.has_next());
    CHECK(cell.set_text("C"));
    REQUIRE(cell.width_twips().has_value());
    CHECK_EQ(*cell.width_twips(), 1200U);
    cell.next();
    REQUIRE(cell.has_next());
    CHECK(cell.set_text("D"));
    REQUIRE(cell.width_twips().has_value());
    CHECK_EQ(*cell.width_twips(), 1800U);

    row.next();
    REQUIRE(row.has_next());
    cell = row.cells();
    REQUIRE(cell.has_next());
    CHECK(cell.set_text("E"));
    REQUIRE(cell.width_twips().has_value());
    CHECK_EQ(*cell.width_twips(), 1200U);
    cell.next();
    REQUIRE(cell.has_next());
    CHECK(cell.set_text("F"));
    REQUIRE(cell.width_twips().has_value());
    CHECK_EQ(*cell.width_twips(), 1800U);

    auto merged_anchor = table.rows().cells();
    REQUIRE(merged_anchor.has_next());
    CHECK(merged_anchor.merge_down(2U));
    REQUIRE(merged_anchor.width_twips().has_value());
    CHECK_EQ(*merged_anchor.width_twips(), 1200U);

    auto second_row = table.rows();
    second_row.next();
    REQUIRE(second_row.has_next());
    auto continuation = second_row.cells();
    REQUIRE(continuation.has_next());
    REQUIRE(continuation.width_twips().has_value());
    CHECK_EQ(*continuation.width_twips(), 1200U);
    continuation.next();
    REQUIRE(continuation.has_next());
    REQUIRE(continuation.width_twips().has_value());
    CHECK_EQ(*continuation.width_twips(), 1800U);

    auto third_row = second_row;
    third_row.next();
    REQUIRE(third_row.has_next());
    auto bottom = third_row.cells();
    REQUIRE(bottom.has_next());
    REQUIRE(bottom.width_twips().has_value());
    CHECK_EQ(*bottom.width_twips(), 1200U);
    bottom.next();
    REQUIRE(bottom.has_next());
    REQUIRE(bottom.width_twips().has_value());
    CHECK_EQ(*bottom.width_twips(), 1800U);

    CHECK_FALSE(doc.save());

    const auto xml_text = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(xml_text.c_str()));

    const auto table_node = xml_document.child("w:document").child("w:body").child("w:tbl");
    REQUIRE(table_node != pugi::xml_node{});
    auto first_row = table_node.child("w:tr");
    REQUIRE(first_row != pugi::xml_node{});
    auto second_xml_row = first_row.next_sibling("w:tr");
    REQUIRE(second_xml_row != pugi::xml_node{});
    auto third_xml_row = second_xml_row.next_sibling("w:tr");
    REQUIRE(third_xml_row != pugi::xml_node{});

    const auto first_row_widths = collect_row_cell_width_values(first_row);
    REQUIRE_EQ(first_row_widths.size(), 2U);
    CHECK_EQ(first_row_widths[0], "1200");
    CHECK_EQ(first_row_widths[1], "1800");

    const auto second_row_widths = collect_row_cell_width_values(second_xml_row);
    REQUIRE_EQ(second_row_widths.size(), 2U);
    CHECK_EQ(second_row_widths[0], "1200");
    CHECK_EQ(second_row_widths[1], "1800");

    const auto third_row_widths = collect_row_cell_width_values(third_xml_row);
    REQUIRE_EQ(third_row_widths.size(), 2U);
    CHECK_EQ(third_row_widths[0], "1200");
    CHECK_EQ(third_row_widths[1], "1800");

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_table_text(reopened), "A\nB\n\nD\n\nF\n");

    auto reopened_table = reopened.tables();
    REQUIRE(reopened_table.has_next());
    auto reopened_row = reopened_table.rows();
    REQUIRE(reopened_row.has_next());
    auto reopened_cell = reopened_row.cells();
    REQUIRE(reopened_cell.has_next());
    REQUIRE(reopened_cell.width_twips().has_value());
    CHECK_EQ(*reopened_cell.width_twips(), 1200U);
    reopened_cell.next();
    REQUIRE(reopened_cell.has_next());
    REQUIRE(reopened_cell.width_twips().has_value());
    CHECK_EQ(*reopened_cell.width_twips(), 1800U);

    reopened_row.next();
    REQUIRE(reopened_row.has_next());
    reopened_cell = reopened_row.cells();
    REQUIRE(reopened_cell.has_next());
    REQUIRE(reopened_cell.width_twips().has_value());
    CHECK_EQ(*reopened_cell.width_twips(), 1200U);
    reopened_cell.next();
    REQUIRE(reopened_cell.has_next());
    REQUIRE(reopened_cell.width_twips().has_value());
    CHECK_EQ(*reopened_cell.width_twips(), 1800U);

    reopened_row.next();
    REQUIRE(reopened_row.has_next());
    reopened_cell = reopened_row.cells();
    REQUIRE(reopened_cell.has_next());
    REQUIRE(reopened_cell.width_twips().has_value());
    CHECK_EQ(*reopened_cell.width_twips(), 1200U);
    reopened_cell.next();
    REQUIRE(reopened_cell.has_next());
    REQUIRE(reopened_cell.width_twips().has_value());
    CHECK_EQ(*reopened_cell.width_twips(), 1800U);

    fs::remove(target);
}

TEST_CASE("table cells can unmerge right into standalone cells") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "table_cell_unmerge_right.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto table = doc.append_table(1, 3);
    auto row = table.rows();
    REQUIRE(row.has_next());

    auto cell = row.cells();
    REQUIRE(cell.has_next());
    CHECK(cell.set_text("A"));
    cell.next();
    REQUIRE(cell.has_next());
    CHECK(cell.set_text("B"));
    cell.next();
    REQUIRE(cell.has_next());
    CHECK(cell.set_text("C"));

    auto merged = row.cells();
    REQUIRE(merged.has_next());
    CHECK(merged.merge_right(1U));
    CHECK_EQ(merged.column_span(), 2U);
    CHECK(merged.unmerge_right());
    CHECK_EQ(merged.column_span(), 1U);
    CHECK_FALSE(merged.unmerge_right());

    auto restored = merged;
    restored.next();
    REQUIRE(restored.has_next());
    CHECK_EQ(restored.column_span(), 1U);
    CHECK_EQ(restored.get_text(), "");
    CHECK(restored.set_text("restored"));

    restored.next();
    REQUIRE(restored.has_next());
    CHECK_EQ(restored.get_text(), "C");

    CHECK_FALSE(doc.save());

    const auto xml_text = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(xml_text.c_str()));

    const auto table_node = xml_document.child("w:document").child("w:body").child("w:tbl");
    REQUIRE(table_node != pugi::xml_node{});
    const auto row_node = table_node.child("w:tr");
    REQUIRE(row_node != pugi::xml_node{});
    CHECK_EQ(count_named_children(row_node, "w:tc"), 3);

    const auto first_cell = row_node.child("w:tc");
    REQUIRE(first_cell != pugi::xml_node{});
    CHECK(first_cell.child("w:tcPr").child("w:gridSpan") == pugi::xml_node{});

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_table_text(reopened), "A\nrestored\nC\n");

    fs::remove(target);
}

TEST_CASE("table cells can unmerge down from a continuation cell") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "table_cell_unmerge_down.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto table = doc.append_table(3, 2);
    auto row = table.rows();
    REQUIRE(row.has_next());

    auto cell = row.cells();
    REQUIRE(cell.has_next());
    CHECK(cell.set_text("A"));
    cell.next();
    REQUIRE(cell.has_next());
    CHECK(cell.set_text("B"));

    row.next();
    REQUIRE(row.has_next());
    cell = row.cells();
    REQUIRE(cell.has_next());
    CHECK(cell.set_text("C"));
    cell.next();
    REQUIRE(cell.has_next());
    CHECK(cell.set_text("D"));

    row.next();
    REQUIRE(row.has_next());
    cell = row.cells();
    REQUIRE(cell.has_next());
    CHECK(cell.set_text("E"));
    cell.next();
    REQUIRE(cell.has_next());
    CHECK(cell.set_text("F"));

    auto merged_anchor = table.rows().cells();
    REQUIRE(merged_anchor.has_next());
    CHECK(merged_anchor.merge_down(2U));

    auto second_row = table.rows();
    second_row.next();
    REQUIRE(second_row.has_next());
    auto continuation_cell = second_row.cells();
    REQUIRE(continuation_cell.has_next());
    CHECK(continuation_cell.unmerge_down());
    CHECK_FALSE(continuation_cell.unmerge_down());
    CHECK_EQ(continuation_cell.get_text(), "");
    CHECK(continuation_cell.set_text("restored-middle"));

    auto third_row = second_row;
    third_row.next();
    REQUIRE(third_row.has_next());
    auto restored_bottom = third_row.cells();
    REQUIRE(restored_bottom.has_next());
    CHECK_EQ(restored_bottom.get_text(), "");
    CHECK(restored_bottom.set_text("restored-bottom"));

    CHECK_FALSE(doc.save());

    const auto xml_text = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(xml_text.c_str()));

    const auto table_node = xml_document.child("w:document").child("w:body").child("w:tbl");
    REQUIRE(table_node != pugi::xml_node{});
    auto first_row = table_node.child("w:tr");
    REQUIRE(first_row != pugi::xml_node{});
    auto second_xml_row = first_row.next_sibling("w:tr");
    REQUIRE(second_xml_row != pugi::xml_node{});
    auto third_xml_row = second_xml_row.next_sibling("w:tr");
    REQUIRE(third_xml_row != pugi::xml_node{});

    CHECK(first_row.child("w:tc").child("w:tcPr").child("w:vMerge") == pugi::xml_node{});
    CHECK(second_xml_row.child("w:tc").child("w:tcPr").child("w:vMerge") == pugi::xml_node{});
    CHECK(third_xml_row.child("w:tc").child("w:tcPr").child("w:vMerge") == pugi::xml_node{});

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_table_text(reopened), "A\nB\nrestored-middle\nD\nrestored-bottom\nF\n");

    fs::remove(target);
}

TEST_CASE("fixed-layout unmerge right splits merged cell widths back to tblGrid columns") {
    namespace fs = std::filesystem;

    const fs::path target =
        fs::current_path() / "table_cell_unmerge_right_fixed_widths.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto table = doc.append_table(2, 3);
    REQUIRE(table.has_next());
    CHECK(table.set_layout_mode(featherdoc::table_layout_mode::fixed));

    auto header_row = table.rows();
    REQUIRE(header_row.has_next());
    auto merged_header = header_row.cells();
    REQUIRE(merged_header.has_next());
    CHECK(merged_header.set_text("merged"));
    CHECK(merged_header.merge_right(1U));
    auto header_tail = merged_header;
    header_tail.next();
    REQUIRE(header_tail.has_next());
    CHECK(header_tail.set_text("tail"));

    auto body_row = header_row;
    body_row.next();
    REQUIRE(body_row.has_next());
    auto body_cell = body_row.cells();
    REQUIRE(body_cell.has_next());
    CHECK(body_cell.set_text("r2c1"));
    body_cell.next();
    REQUIRE(body_cell.has_next());
    CHECK(body_cell.set_text("r2c2"));
    body_cell.next();
    REQUIRE(body_cell.has_next());
    CHECK(body_cell.set_text("r2c3"));

    CHECK(table.set_column_width_twips(0U, 1200U));
    CHECK(table.set_column_width_twips(1U, 1800U));
    CHECK(table.set_column_width_twips(2U, 2400U));

    REQUIRE(merged_header.width_twips().has_value());
    CHECK_EQ(*merged_header.width_twips(), 3000U);
    REQUIRE(header_tail.width_twips().has_value());
    CHECK_EQ(*header_tail.width_twips(), 2400U);

    CHECK(merged_header.unmerge_right());
    CHECK_EQ(merged_header.column_span(), 1U);
    REQUIRE(merged_header.width_twips().has_value());
    CHECK_EQ(*merged_header.width_twips(), 1200U);

    auto restored_header = merged_header;
    restored_header.next();
    REQUIRE(restored_header.has_next());
    CHECK_EQ(restored_header.column_span(), 1U);
    REQUIRE(restored_header.width_twips().has_value());
    CHECK_EQ(*restored_header.width_twips(), 1800U);
    CHECK_EQ(restored_header.get_text(), "");
    CHECK(restored_header.set_text("restored"));

    header_tail = restored_header;
    header_tail.next();
    REQUIRE(header_tail.has_next());
    CHECK_EQ(header_tail.get_text(), "tail");
    REQUIRE(header_tail.width_twips().has_value());
    CHECK_EQ(*header_tail.width_twips(), 2400U);

    REQUIRE(table.column_width_twips(0U).has_value());
    CHECK_EQ(*table.column_width_twips(0U), 1200U);
    REQUIRE(table.column_width_twips(1U).has_value());
    CHECK_EQ(*table.column_width_twips(1U), 1800U);
    REQUIRE(table.column_width_twips(2U).has_value());
    CHECK_EQ(*table.column_width_twips(2U), 2400U);

    body_cell = body_row.cells();
    REQUIRE(body_cell.has_next());
    REQUIRE(body_cell.width_twips().has_value());
    CHECK_EQ(*body_cell.width_twips(), 1200U);
    body_cell.next();
    REQUIRE(body_cell.has_next());
    REQUIRE(body_cell.width_twips().has_value());
    CHECK_EQ(*body_cell.width_twips(), 1800U);
    body_cell.next();
    REQUIRE(body_cell.has_next());
    REQUIRE(body_cell.width_twips().has_value());
    CHECK_EQ(*body_cell.width_twips(), 2400U);

    CHECK_FALSE(doc.save());

    const auto xml_text = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(xml_text.c_str()));

    const auto table_node = xml_document.child("w:document").child("w:body").child("w:tbl");
    REQUIRE(table_node != pugi::xml_node{});

    const auto grid_widths = collect_table_grid_width_values(table_node);
    REQUIRE_EQ(grid_widths.size(), 3U);
    CHECK_EQ(grid_widths[0], "1200");
    CHECK_EQ(grid_widths[1], "1800");
    CHECK_EQ(grid_widths[2], "2400");

    const auto first_row = table_node.child("w:tr");
    REQUIRE(first_row != pugi::xml_node{});
    const auto second_row = first_row.next_sibling("w:tr");
    REQUIRE(second_row != pugi::xml_node{});
    CHECK_EQ(count_named_children(first_row, "w:tc"), 3U);
    CHECK(first_row.child("w:tc").child("w:tcPr").child("w:gridSpan") == pugi::xml_node{});

    const auto first_row_widths = collect_row_cell_width_values(first_row);
    REQUIRE_EQ(first_row_widths.size(), 3U);
    CHECK_EQ(first_row_widths[0], "1200");
    CHECK_EQ(first_row_widths[1], "1800");
    CHECK_EQ(first_row_widths[2], "2400");

    const auto second_row_widths = collect_row_cell_width_values(second_row);
    REQUIRE_EQ(second_row_widths.size(), 3U);
    CHECK_EQ(second_row_widths[0], "1200");
    CHECK_EQ(second_row_widths[1], "1800");
    CHECK_EQ(second_row_widths[2], "2400");

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_table_text(reopened), "merged\nrestored\ntail\nr2c1\nr2c2\nr2c3\n");

    auto reopened_table = reopened.tables();
    REQUIRE(reopened_table.has_next());
    auto reopened_header = reopened_table.rows().cells();
    REQUIRE(reopened_header.has_next());
    REQUIRE(reopened_header.width_twips().has_value());
    CHECK_EQ(*reopened_header.width_twips(), 1200U);
    reopened_header.next();
    REQUIRE(reopened_header.has_next());
    REQUIRE(reopened_header.width_twips().has_value());
    CHECK_EQ(*reopened_header.width_twips(), 1800U);
    reopened_header.next();
    REQUIRE(reopened_header.has_next());
    REQUIRE(reopened_header.width_twips().has_value());
    CHECK_EQ(*reopened_header.width_twips(), 2400U);

    REQUIRE(reopened_table.column_width_twips(0U).has_value());
    CHECK_EQ(*reopened_table.column_width_twips(0U), 1200U);
    REQUIRE(reopened_table.column_width_twips(1U).has_value());
    CHECK_EQ(*reopened_table.column_width_twips(1U), 1800U);
    REQUIRE(reopened_table.column_width_twips(2U).has_value());
    CHECK_EQ(*reopened_table.column_width_twips(2U), 2400U);

    fs::remove(target);
}

TEST_CASE("fixed-layout unmerge down preserves spanning cell widths from tblGrid") {
    namespace fs = std::filesystem;

    const fs::path target =
        fs::current_path() / "table_cell_unmerge_down_fixed_widths.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto table = doc.append_table(3, 3);
    REQUIRE(table.has_next());
    CHECK(table.set_layout_mode(featherdoc::table_layout_mode::fixed));

    auto row = table.rows();
    REQUIRE(row.has_next());
    auto cell = row.cells();
    REQUIRE(cell.has_next());
    CHECK(cell.set_text("top-span"));
    CHECK(cell.merge_right(1U));
    auto row_tail = cell;
    row_tail.next();
    REQUIRE(row_tail.has_next());
    CHECK(row_tail.set_text("top-tail"));

    row.next();
    REQUIRE(row.has_next());
    cell = row.cells();
    REQUIRE(cell.has_next());
    CHECK(cell.set_text("middle-span"));
    CHECK(cell.merge_right(1U));
    row_tail = cell;
    row_tail.next();
    REQUIRE(row_tail.has_next());
    CHECK(row_tail.set_text("middle-tail"));

    row.next();
    REQUIRE(row.has_next());
    cell = row.cells();
    REQUIRE(cell.has_next());
    CHECK(cell.set_text("bottom-span"));
    CHECK(cell.merge_right(1U));
    row_tail = cell;
    row_tail.next();
    REQUIRE(row_tail.has_next());
    CHECK(row_tail.set_text("bottom-tail"));

    CHECK(table.set_column_width_twips(0U, 1200U));
    CHECK(table.set_column_width_twips(1U, 1800U));
    CHECK(table.set_column_width_twips(2U, 2400U));

    auto merged_anchor = table.rows().cells();
    REQUIRE(merged_anchor.has_next());
    REQUIRE(merged_anchor.width_twips().has_value());
    CHECK_EQ(*merged_anchor.width_twips(), 3000U);
    CHECK(merged_anchor.merge_down(2U));

    auto second_row = table.rows();
    second_row.next();
    REQUIRE(second_row.has_next());
    auto continuation_cell = second_row.cells();
    REQUIRE(continuation_cell.has_next());
    REQUIRE(continuation_cell.width_twips().has_value());
    CHECK_EQ(*continuation_cell.width_twips(), 3000U);
    CHECK(continuation_cell.unmerge_down());
    CHECK_FALSE(continuation_cell.unmerge_down());
    REQUIRE(continuation_cell.width_twips().has_value());
    CHECK_EQ(*continuation_cell.width_twips(), 3000U);
    CHECK_EQ(continuation_cell.get_text(), "");
    CHECK(continuation_cell.set_text("restored-middle"));

    auto third_row = second_row;
    third_row.next();
    REQUIRE(third_row.has_next());
    auto restored_bottom = third_row.cells();
    REQUIRE(restored_bottom.has_next());
    REQUIRE(restored_bottom.width_twips().has_value());
    CHECK_EQ(*restored_bottom.width_twips(), 3000U);
    CHECK_EQ(restored_bottom.get_text(), "");
    CHECK(restored_bottom.set_text("restored-bottom"));

    auto top_tail = table.rows().cells();
    top_tail.next();
    REQUIRE(top_tail.has_next());
    REQUIRE(top_tail.width_twips().has_value());
    CHECK_EQ(*top_tail.width_twips(), 2400U);

    auto middle_tail = second_row.cells();
    middle_tail.next();
    REQUIRE(middle_tail.has_next());
    REQUIRE(middle_tail.width_twips().has_value());
    CHECK_EQ(*middle_tail.width_twips(), 2400U);

    auto bottom_tail = third_row.cells();
    bottom_tail.next();
    REQUIRE(bottom_tail.has_next());
    REQUIRE(bottom_tail.width_twips().has_value());
    CHECK_EQ(*bottom_tail.width_twips(), 2400U);

    REQUIRE(table.column_width_twips(0U).has_value());
    CHECK_EQ(*table.column_width_twips(0U), 1200U);
    REQUIRE(table.column_width_twips(1U).has_value());
    CHECK_EQ(*table.column_width_twips(1U), 1800U);
    REQUIRE(table.column_width_twips(2U).has_value());
    CHECK_EQ(*table.column_width_twips(2U), 2400U);

    CHECK_FALSE(doc.save());

    const auto xml_text = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(xml_text.c_str()));

    const auto table_node = xml_document.child("w:document").child("w:body").child("w:tbl");
    REQUIRE(table_node != pugi::xml_node{});

    const auto grid_widths = collect_table_grid_width_values(table_node);
    REQUIRE_EQ(grid_widths.size(), 3U);
    CHECK_EQ(grid_widths[0], "1200");
    CHECK_EQ(grid_widths[1], "1800");
    CHECK_EQ(grid_widths[2], "2400");

    auto first_row = table_node.child("w:tr");
    REQUIRE(first_row != pugi::xml_node{});
    auto second_xml_row = first_row.next_sibling("w:tr");
    REQUIRE(second_xml_row != pugi::xml_node{});
    auto third_xml_row = second_xml_row.next_sibling("w:tr");
    REQUIRE(third_xml_row != pugi::xml_node{});

    CHECK(first_row.child("w:tc").child("w:tcPr").child("w:vMerge") == pugi::xml_node{});
    CHECK(second_xml_row.child("w:tc").child("w:tcPr").child("w:vMerge") ==
          pugi::xml_node{});
    CHECK(third_xml_row.child("w:tc").child("w:tcPr").child("w:vMerge") == pugi::xml_node{});

    const auto first_row_widths = collect_row_cell_width_values(first_row);
    REQUIRE_EQ(first_row_widths.size(), 2U);
    CHECK_EQ(first_row_widths[0], "3000");
    CHECK_EQ(first_row_widths[1], "2400");

    const auto second_row_widths = collect_row_cell_width_values(second_xml_row);
    REQUIRE_EQ(second_row_widths.size(), 2U);
    CHECK_EQ(second_row_widths[0], "3000");
    CHECK_EQ(second_row_widths[1], "2400");

    const auto third_row_widths = collect_row_cell_width_values(third_xml_row);
    REQUIRE_EQ(third_row_widths.size(), 2U);
    CHECK_EQ(third_row_widths[0], "3000");
    CHECK_EQ(third_row_widths[1], "2400");

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_table_text(reopened),
             "top-span\ntop-tail\nrestored-middle\nmiddle-tail\nrestored-bottom\nbottom-tail\n");

    auto reopened_table = reopened.tables();
    REQUIRE(reopened_table.has_next());
    auto reopened_row = reopened_table.rows();
    REQUIRE(reopened_row.has_next());
    auto reopened_cell = reopened_row.cells();
    REQUIRE(reopened_cell.has_next());
    REQUIRE(reopened_cell.width_twips().has_value());
    CHECK_EQ(*reopened_cell.width_twips(), 3000U);
    reopened_cell.next();
    REQUIRE(reopened_cell.has_next());
    REQUIRE(reopened_cell.width_twips().has_value());
    CHECK_EQ(*reopened_cell.width_twips(), 2400U);

    reopened_row.next();
    REQUIRE(reopened_row.has_next());
    reopened_cell = reopened_row.cells();
    REQUIRE(reopened_cell.has_next());
    REQUIRE(reopened_cell.width_twips().has_value());
    CHECK_EQ(*reopened_cell.width_twips(), 3000U);
    reopened_cell.next();
    REQUIRE(reopened_cell.has_next());
    REQUIRE(reopened_cell.width_twips().has_value());
    CHECK_EQ(*reopened_cell.width_twips(), 2400U);

    reopened_row.next();
    REQUIRE(reopened_row.has_next());
    reopened_cell = reopened_row.cells();
    REQUIRE(reopened_cell.has_next());
    REQUIRE(reopened_cell.width_twips().has_value());
    CHECK_EQ(*reopened_cell.width_twips(), 3000U);
    reopened_cell.next();
    REQUIRE(reopened_cell.has_next());
    REQUIRE(reopened_cell.width_twips().has_value());
    CHECK_EQ(*reopened_cell.width_twips(), 2400U);

    fs::remove(target);
}
