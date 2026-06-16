#include <filesystem>
#include <string>
#include <string_view>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "basic_docx_archive_test_support.hpp"
#include "basic_document_xml_test_support.hpp"

#include <featherdoc.hpp>
TEST_CASE("tables can set widths style ids and borders") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "table_width_style_borders.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto table = doc.append_table(1, 2);
    CHECK_FALSE(table.width_twips().has_value());
    CHECK_FALSE(table.style_id().has_value());

    CHECK(table.set_width_twips(7200U));
    REQUIRE(table.width_twips().has_value());
    CHECK_EQ(*table.width_twips(), 7200U);

    CHECK(table.set_style_id("TableGrid"));
    REQUIRE(table.style_id().has_value());
    CHECK_EQ(*table.style_id(), "TableGrid");

    CHECK(table.set_border(featherdoc::table_border_edge::top,
                           {featherdoc::border_style::single, 12U, "FF0000", 0U}));
    CHECK(table.set_border(featherdoc::table_border_edge::inside_vertical,
                           {featherdoc::border_style::dashed, 8U, "808080", 0U}));

    auto row = table.rows();
    REQUIRE(row.has_next());
    auto cell = row.cells();
    REQUIRE(cell.has_next());
    CHECK(cell.set_border(featherdoc::cell_border_edge::bottom,
                          {featherdoc::border_style::thick, 16U, "00AA00", 0U}));
    CHECK(cell.paragraphs().add_run("left").has_next());

    cell.next();
    REQUIRE(cell.has_next());
    CHECK(cell.set_border(featherdoc::cell_border_edge::right,
                          {featherdoc::border_style::dotted, 6U, "0000FF", 1U}));
    CHECK(cell.paragraphs().add_run("right").has_next());

    CHECK_FALSE(doc.save());
    CHECK(test_docx_entry_exists(target, "word/styles.xml"));
    CHECK(collect_empty_zip64_extra_locations(target).empty());
    CHECK(collect_zero_version_needed_locations(target).empty());

    const auto xml_text = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(xml_text.c_str()));

    const auto table_node = xml_document.child("w:document").child("w:body").child("w:tbl");
    REQUIRE(table_node != pugi::xml_node{});

    const auto table_width = table_node.child("w:tblPr").child("w:tblW");
    REQUIRE(table_width != pugi::xml_node{});
    CHECK_EQ(std::string_view{table_width.attribute("w:type").value()}, "dxa");
    CHECK_EQ(std::string_view{table_width.attribute("w:w").value()}, "7200");

    const auto table_style = table_node.child("w:tblPr").child("w:tblStyle");
    REQUIRE(table_style != pugi::xml_node{});
    CHECK_EQ(std::string_view{table_style.attribute("w:val").value()}, "TableGrid");

    const auto saved_relationships =
        read_test_docx_entry(target, "word/_rels/document.xml.rels");
    CHECK_NE(saved_relationships.find(
                 "Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/styles\""),
             std::string::npos);
    CHECK_NE(saved_relationships.find("Target=\"styles.xml\""), std::string::npos);

    const auto saved_content_types = read_test_docx_entry(target, test_content_types_xml_entry);
    CHECK_NE(saved_content_types.find("PartName=\"/word/styles.xml\""), std::string::npos);
    CHECK_NE(saved_content_types.find(
                 "application/vnd.openxmlformats-officedocument.wordprocessingml.styles+xml"),
             std::string::npos);

    const auto saved_styles_xml = read_test_docx_entry(target, "word/styles.xml");
    CHECK_NE(saved_styles_xml.find("w:styleId=\"TableGrid\""), std::string::npos);

    const auto table_borders = table_node.child("w:tblPr").child("w:tblBorders");
    REQUIRE(table_borders != pugi::xml_node{});
    const auto top_border = table_borders.child("w:top");
    REQUIRE(top_border != pugi::xml_node{});
    CHECK_EQ(std::string_view{top_border.attribute("w:val").value()}, "single");
    CHECK_EQ(std::string_view{top_border.attribute("w:sz").value()}, "12");
    CHECK_EQ(std::string_view{top_border.attribute("w:color").value()}, "FF0000");

    const auto inside_vertical = table_borders.child("w:insideV");
    REQUIRE(inside_vertical != pugi::xml_node{});
    CHECK_EQ(std::string_view{inside_vertical.attribute("w:val").value()}, "dashed");
    CHECK_EQ(std::string_view{inside_vertical.attribute("w:sz").value()}, "8");
    CHECK_EQ(std::string_view{inside_vertical.attribute("w:color").value()}, "808080");

    const auto first_cell = table_node.child("w:tr").child("w:tc");
    REQUIRE(first_cell != pugi::xml_node{});
    const auto first_cell_bottom = first_cell.child("w:tcPr").child("w:tcBorders").child("w:bottom");
    REQUIRE(first_cell_bottom != pugi::xml_node{});
    CHECK_EQ(std::string_view{first_cell_bottom.attribute("w:val").value()}, "thick");
    CHECK_EQ(std::string_view{first_cell_bottom.attribute("w:sz").value()}, "16");
    CHECK_EQ(std::string_view{first_cell_bottom.attribute("w:color").value()}, "00AA00");

    const auto second_cell = first_cell.next_sibling("w:tc");
    REQUIRE(second_cell != pugi::xml_node{});
    const auto second_cell_right = second_cell.child("w:tcPr").child("w:tcBorders").child("w:right");
    REQUIRE(second_cell_right != pugi::xml_node{});
    CHECK_EQ(std::string_view{second_cell_right.attribute("w:val").value()}, "dotted");
    CHECK_EQ(std::string_view{second_cell_right.attribute("w:sz").value()}, "6");
    CHECK_EQ(std::string_view{second_cell_right.attribute("w:space").value()}, "1");
    CHECK_EQ(std::string_view{second_cell_right.attribute("w:color").value()}, "0000FF");

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());

    auto reopened_table = reopened.tables();
    REQUIRE(reopened_table.has_next());
    REQUIRE(reopened_table.width_twips().has_value());
    CHECK_EQ(*reopened_table.width_twips(), 7200U);
    REQUIRE(reopened_table.style_id().has_value());
    CHECK_EQ(*reopened_table.style_id(), "TableGrid");

    fs::remove(target);
}

TEST_CASE("tables can set and read grid column widths") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "table_grid_column_widths.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto table = doc.append_table(2, 3);
    REQUIRE(table.has_next());
    CHECK(table.set_width_twips(7200U));
    CHECK(table.set_layout_mode(featherdoc::table_layout_mode::fixed));

    CHECK_FALSE(table.set_column_width_twips(3U, 3600U));
    CHECK(table.set_column_width_twips(0U, 1800U));
    CHECK(table.set_column_width_twips(1U, 2400U));
    CHECK(table.set_column_width_twips(2U, 3000U));
    REQUIRE(table.column_width_twips(0U).has_value());
    CHECK_EQ(*table.column_width_twips(0U), 1800U);
    REQUIRE(table.column_width_twips(1U).has_value());
    CHECK_EQ(*table.column_width_twips(1U), 2400U);
    REQUIRE(table.column_width_twips(2U).has_value());
    CHECK_EQ(*table.column_width_twips(2U), 3000U);
    CHECK_FALSE(table.column_width_twips(3U).has_value());

    CHECK_FALSE(doc.save());

    const auto xml_text = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(xml_text.c_str()));

    const auto table_node = xml_document.child("w:document").child("w:body").child("w:tbl");
    REQUIRE(table_node != pugi::xml_node{});
    const auto table_grid = table_node.child("w:tblGrid");
    REQUIRE(table_grid != pugi::xml_node{});
    CHECK_EQ(count_named_children(table_grid, "w:gridCol"), 3U);

    const auto first_grid_column = table_grid.child("w:gridCol");
    REQUIRE(first_grid_column != pugi::xml_node{});
    CHECK_EQ(std::string_view{first_grid_column.attribute("w:w").value()}, "1800");

    const auto second_grid_column = first_grid_column.next_sibling("w:gridCol");
    REQUIRE(second_grid_column != pugi::xml_node{});
    CHECK_EQ(std::string_view{second_grid_column.attribute("w:w").value()}, "2400");

    const auto third_grid_column = second_grid_column.next_sibling("w:gridCol");
    REQUIRE(third_grid_column != pugi::xml_node{});
    CHECK_EQ(std::string_view{third_grid_column.attribute("w:w").value()}, "3000");

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());

    auto reopened_table = reopened.tables();
    REQUIRE(reopened_table.has_next());
    REQUIRE(reopened_table.column_width_twips(0U).has_value());
    CHECK_EQ(*reopened_table.column_width_twips(0U), 1800U);
    REQUIRE(reopened_table.column_width_twips(1U).has_value());
    CHECK_EQ(*reopened_table.column_width_twips(1U), 2400U);
    REQUIRE(reopened_table.column_width_twips(2U).has_value());
    CHECK_EQ(*reopened_table.column_width_twips(2U), 3000U);
    CHECK_FALSE(reopened_table.column_width_twips(3U).has_value());

    fs::remove(target);
}

TEST_CASE("fixed-layout grid column widths synchronize cell widths") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "table_grid_column_widths_sync_cells.docx";
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
    const auto first_row = table_node.child("w:tr");
    REQUIRE(first_row != pugi::xml_node{});
    const auto second_row = first_row.next_sibling("w:tr");
    REQUIRE(second_row != pugi::xml_node{});

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

    auto reopened_body_row = reopened_table.rows();
    reopened_body_row.next();
    REQUIRE(reopened_body_row.has_next());
    auto reopened_body = reopened_body_row.cells();
    REQUIRE(reopened_body.has_next());
    REQUIRE(reopened_body.width_twips().has_value());
    CHECK_EQ(*reopened_body.width_twips(), 1200U);
    reopened_body.next();
    REQUIRE(reopened_body.has_next());
    REQUIRE(reopened_body.width_twips().has_value());
    CHECK_EQ(*reopened_body.width_twips(), 1800U);
    reopened_body.next();
    REQUIRE(reopened_body.has_next());
    REQUIRE(reopened_body.width_twips().has_value());
    CHECK_EQ(*reopened_body.width_twips(), 2400U);

    fs::remove(target);
}

TEST_CASE("switching to fixed layout synchronizes complete grid widths into tcW") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "table_layout_mode_fixed_syncs_grid_widths.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto table = doc.append_table(2, 3);
    REQUIRE(table.has_next());

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

    CHECK_FALSE(merged_header.width_twips().has_value());
    CHECK_FALSE(header_tail.width_twips().has_value());

    body_cell = body_row.cells();
    REQUIRE(body_cell.has_next());
    CHECK_FALSE(body_cell.width_twips().has_value());
    body_cell.next();
    REQUIRE(body_cell.has_next());
    CHECK_FALSE(body_cell.width_twips().has_value());
    body_cell.next();
    REQUIRE(body_cell.has_next());
    CHECK_FALSE(body_cell.width_twips().has_value());

    CHECK(table.set_layout_mode(featherdoc::table_layout_mode::fixed));

    REQUIRE(merged_header.width_twips().has_value());
    CHECK_EQ(*merged_header.width_twips(), 3000U);
    REQUIRE(header_tail.width_twips().has_value());
    CHECK_EQ(*header_tail.width_twips(), 2400U);

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
    const auto first_row = table_node.child("w:tr");
    REQUIRE(first_row != pugi::xml_node{});
    const auto second_row = first_row.next_sibling("w:tr");
    REQUIRE(second_row != pugi::xml_node{});
    CHECK_EQ(std::string_view{table_node.child("w:tblPr")
                                  .child("w:tblLayout")
                                  .attribute("w:type")
                                  .value()},
             "fixed");

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
    auto reopened_table = reopened.tables();
    REQUIRE(reopened_table.has_next());
    REQUIRE(reopened_table.layout_mode().has_value());
    CHECK_EQ(*reopened_table.layout_mode(), featherdoc::table_layout_mode::fixed);

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

TEST_CASE("table grid column widths can be cleared") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "table_grid_column_widths_clear.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto table = doc.append_table(1, 3);
    REQUIRE(table.has_next());
    CHECK(table.set_column_width_twips(0U, 1500U));
    CHECK(table.set_column_width_twips(1U, 2100U));
    CHECK(table.set_column_width_twips(2U, 3600U));
    REQUIRE(table.column_width_twips(1U).has_value());
    CHECK_EQ(*table.column_width_twips(1U), 2100U);

    CHECK(table.clear_column_width(1U));
    CHECK_FALSE(table.column_width_twips(1U).has_value());
    CHECK_FALSE(table.clear_column_width(3U));

    CHECK_FALSE(doc.save());

    const auto xml_text = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(xml_text.c_str()));

    const auto table_node = xml_document.child("w:document").child("w:body").child("w:tbl");
    REQUIRE(table_node != pugi::xml_node{});
    const auto table_grid = table_node.child("w:tblGrid");
    REQUIRE(table_grid != pugi::xml_node{});

    const auto first_grid_column = table_grid.child("w:gridCol");
    REQUIRE(first_grid_column != pugi::xml_node{});
    CHECK_EQ(std::string_view{first_grid_column.attribute("w:w").value()}, "1500");

    const auto second_grid_column = first_grid_column.next_sibling("w:gridCol");
    REQUIRE(second_grid_column != pugi::xml_node{});
    CHECK_EQ(second_grid_column.attribute("w:w"), pugi::xml_attribute{});

    const auto third_grid_column = second_grid_column.next_sibling("w:gridCol");
    REQUIRE(third_grid_column != pugi::xml_node{});
    CHECK_EQ(std::string_view{third_grid_column.attribute("w:w").value()}, "3600");

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());

    auto reopened_table = reopened.tables();
    REQUIRE(reopened_table.has_next());
    REQUIRE(reopened_table.column_width_twips(0U).has_value());
    CHECK_EQ(*reopened_table.column_width_twips(0U), 1500U);
    CHECK_FALSE(reopened_table.column_width_twips(1U).has_value());
    REQUIRE(reopened_table.column_width_twips(2U).has_value());
    CHECK_EQ(*reopened_table.column_width_twips(2U), 3600U);

    fs::remove(target);
}

TEST_CASE("clearing a fixed-layout grid column clears tcW on affected cells") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "table_grid_column_widths_clear_cells.docx";
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

    CHECK(table.set_column_width_twips(0U, 1500U));
    CHECK(table.set_column_width_twips(1U, 2100U));
    CHECK(table.set_column_width_twips(2U, 3600U));

    REQUIRE(merged_header.width_twips().has_value());
    CHECK_EQ(*merged_header.width_twips(), 3600U);

    CHECK(table.clear_column_width(1U));
    CHECK_FALSE(table.column_width_twips(1U).has_value());
    CHECK_FALSE(merged_header.width_twips().has_value());
    REQUIRE(header_tail.width_twips().has_value());
    CHECK_EQ(*header_tail.width_twips(), 3600U);

    body_cell = body_row.cells();
    REQUIRE(body_cell.has_next());
    REQUIRE(body_cell.width_twips().has_value());
    CHECK_EQ(*body_cell.width_twips(), 1500U);
    body_cell.next();
    REQUIRE(body_cell.has_next());
    CHECK_FALSE(body_cell.width_twips().has_value());
    body_cell.next();
    REQUIRE(body_cell.has_next());
    REQUIRE(body_cell.width_twips().has_value());
    CHECK_EQ(*body_cell.width_twips(), 3600U);

    CHECK_FALSE(doc.save());

    const auto xml_text = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(xml_text.c_str()));

    const auto table_node = xml_document.child("w:document").child("w:body").child("w:tbl");
    REQUIRE(table_node != pugi::xml_node{});
    const auto first_row = table_node.child("w:tr");
    REQUIRE(first_row != pugi::xml_node{});
    const auto second_row = first_row.next_sibling("w:tr");
    REQUIRE(second_row != pugi::xml_node{});

    const auto grid_widths = collect_table_grid_width_values(table_node);
    REQUIRE_EQ(grid_widths.size(), 3U);
    CHECK_EQ(grid_widths[0], "1500");
    CHECK_EQ(grid_widths[1], "");
    CHECK_EQ(grid_widths[2], "3600");

    const auto first_row_widths = collect_row_cell_width_values(first_row);
    REQUIRE_EQ(first_row_widths.size(), 2U);
    CHECK_EQ(first_row_widths[0], "");
    CHECK_EQ(first_row_widths[1], "3600");

    const auto second_row_widths = collect_row_cell_width_values(second_row);
    REQUIRE_EQ(second_row_widths.size(), 3U);
    CHECK_EQ(second_row_widths[0], "1500");
    CHECK_EQ(second_row_widths[1], "");
    CHECK_EQ(second_row_widths[2], "3600");

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    auto reopened_table = reopened.tables();
    REQUIRE(reopened_table.has_next());
    auto reopened_header = reopened_table.rows().cells();
    REQUIRE(reopened_header.has_next());
    CHECK_FALSE(reopened_header.width_twips().has_value());
    reopened_header.next();
    REQUIRE(reopened_header.has_next());
    REQUIRE(reopened_header.width_twips().has_value());
    CHECK_EQ(*reopened_header.width_twips(), 3600U);

    auto reopened_body_row = reopened_table.rows();
    reopened_body_row.next();
    REQUIRE(reopened_body_row.has_next());
    auto reopened_body = reopened_body_row.cells();
    REQUIRE(reopened_body.has_next());
    REQUIRE(reopened_body.width_twips().has_value());
    CHECK_EQ(*reopened_body.width_twips(), 1500U);
    reopened_body.next();
    REQUIRE(reopened_body.has_next());
    CHECK_FALSE(reopened_body.width_twips().has_value());
    reopened_body.next();
    REQUIRE(reopened_body.has_next());
    REQUIRE(reopened_body.width_twips().has_value());
    CHECK_EQ(*reopened_body.width_twips(), 3600U);

    fs::remove(target);
}

TEST_CASE("table widths style ids and borders can be cleared") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "table_width_style_clear.docx";
    fs::remove(target);

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:tbl>
      <w:tblPr>
        <w:tblStyle w:val="TableGrid"/>
        <w:tblW w:w="7200" w:type="dxa"/>
        <w:tblBorders>
          <w:top w:val="single" w:sz="8" w:space="0" w:color="000000"/>
        </w:tblBorders>
      </w:tblPr>
      <w:tblGrid>
        <w:gridCol w:w="0"/>
      </w:tblGrid>
      <w:tr>
        <w:tc>
          <w:tcPr>
            <w:tcW w:w="0" w:type="auto"/>
            <w:tcBorders>
              <w:bottom w:val="single" w:sz="8" w:space="0" w:color="000000"/>
            </w:tcBorders>
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
    REQUIRE(table.width_twips().has_value());
    CHECK_EQ(*table.width_twips(), 7200U);
    REQUIRE(table.style_id().has_value());
    CHECK_EQ(*table.style_id(), "TableGrid");

    CHECK(table.clear_width());
    CHECK(table.clear_style_id());
    CHECK(table.clear_border(featherdoc::table_border_edge::top));

    auto row = table.rows();
    REQUIRE(row.has_next());
    auto cell = row.cells();
    REQUIRE(cell.has_next());
    CHECK(cell.clear_border(featherdoc::cell_border_edge::bottom));

    CHECK_FALSE(doc.save());

    const auto xml_text = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(xml_text.c_str()));

    const auto table_node = xml_document.child("w:document").child("w:body").child("w:tbl");
    REQUIRE(table_node != pugi::xml_node{});
    const auto table_properties = table_node.child("w:tblPr");
    REQUIRE(table_properties != pugi::xml_node{});
    CHECK_EQ(table_properties.child("w:tblW"), pugi::xml_node{});
    CHECK_EQ(table_properties.child("w:tblStyle"), pugi::xml_node{});
    CHECK_EQ(table_properties.child("w:tblBorders"), pugi::xml_node{});

    const auto cell_properties = table_node.child("w:tr").child("w:tc").child("w:tcPr");
    REQUIRE(cell_properties != pugi::xml_node{});
    CHECK_EQ(cell_properties.child("w:tcBorders"), pugi::xml_node{});

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());

    auto reopened_table = reopened.tables();
    REQUIRE(reopened_table.has_next());
    CHECK_FALSE(reopened_table.width_twips().has_value());
    CHECK_FALSE(reopened_table.style_id().has_value());

    fs::remove(target);
}
