#include <array>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <string_view>
#include <vector>

#include "doctest.h"
#include "basic_docx_archive_test_support.hpp"
#include "basic_document_xml_test_support.hpp"

#include <featherdoc.hpp>

TEST_CASE("table-level properties survive append_row") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "table_append_row_property_regression.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto table = doc.append_table(1, 2);
    CHECK(table.set_width_twips(7200U));
    CHECK(table.set_style_id("TableGrid"));
    CHECK(table.set_layout_mode(featherdoc::table_layout_mode::fixed));
    CHECK(table.set_alignment(featherdoc::table_alignment::center));
    CHECK(table.set_indent_twips(360U));
    CHECK(table.set_cell_spacing_twips(180U));
    CHECK(table.set_cell_margin_twips(featherdoc::cell_margin_edge::top, 120U));
    CHECK(table.set_cell_margin_twips(featherdoc::cell_margin_edge::left, 240U));
    CHECK(table.set_cell_margin_twips(featherdoc::cell_margin_edge::bottom, 360U));
    CHECK(table.set_cell_margin_twips(featherdoc::cell_margin_edge::right, 480U));

    auto appended_row = table.append_row(2);
    REQUIRE(appended_row.has_next());
    auto appended_cell = appended_row.cells();
    REQUIRE(appended_cell.has_next());
    CHECK(appended_cell.paragraphs().add_run("left").has_next());
    appended_cell.next();
    REQUIRE(appended_cell.has_next());
    CHECK(appended_cell.paragraphs().add_run("right").has_next());

    REQUIRE(table.width_twips().has_value());
    CHECK_EQ(*table.width_twips(), 7200U);
    REQUIRE(table.style_id().has_value());
    CHECK_EQ(*table.style_id(), "TableGrid");
    REQUIRE(table.layout_mode().has_value());
    CHECK_EQ(*table.layout_mode(), featherdoc::table_layout_mode::fixed);
    REQUIRE(table.alignment().has_value());
    CHECK_EQ(*table.alignment(), featherdoc::table_alignment::center);
    REQUIRE(table.indent_twips().has_value());
    CHECK_EQ(*table.indent_twips(), 360U);
    REQUIRE(table.cell_spacing_twips().has_value());
    CHECK_EQ(*table.cell_spacing_twips(), 180U);
    REQUIRE(table.cell_margin_twips(featherdoc::cell_margin_edge::top).has_value());
    CHECK_EQ(*table.cell_margin_twips(featherdoc::cell_margin_edge::top), 120U);
    REQUIRE(table.cell_margin_twips(featherdoc::cell_margin_edge::left).has_value());
    CHECK_EQ(*table.cell_margin_twips(featherdoc::cell_margin_edge::left), 240U);
    REQUIRE(table.cell_margin_twips(featherdoc::cell_margin_edge::bottom).has_value());
    CHECK_EQ(*table.cell_margin_twips(featherdoc::cell_margin_edge::bottom), 360U);
    REQUIRE(table.cell_margin_twips(featherdoc::cell_margin_edge::right).has_value());
    CHECK_EQ(*table.cell_margin_twips(featherdoc::cell_margin_edge::right), 480U);

    CHECK_FALSE(doc.save());

    const auto xml_text = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(xml_text.c_str()));

    const auto table_node = xml_document.child("w:document").child("w:body").child("w:tbl");
    REQUIRE(table_node != pugi::xml_node{});
    const auto table_properties = table_node.child("w:tblPr");
    REQUIRE(table_properties != pugi::xml_node{});
    CHECK_EQ(std::string_view{table_properties.child("w:tblW").attribute("w:w").value()}, "7200");
    CHECK_EQ(std::string_view{table_properties.child("w:tblW").attribute("w:type").value()},
             "dxa");
    CHECK_EQ(std::string_view{table_properties.child("w:tblStyle").attribute("w:val").value()},
             "TableGrid");
    CHECK_EQ(std::string_view{table_properties.child("w:tblLayout").attribute("w:type").value()},
             "fixed");
    CHECK_EQ(std::string_view{table_properties.child("w:jc").attribute("w:val").value()},
             "center");
    CHECK_EQ(std::string_view{table_properties.child("w:tblInd").attribute("w:w").value()},
             "360");
    CHECK_EQ(std::string_view{table_properties.child("w:tblCellSpacing")
                                  .attribute("w:w")
                                  .value()},
             "180");
    CHECK_EQ(std::string_view{table_properties.child("w:tblCellSpacing")
                                  .attribute("w:type")
                                  .value()},
             "dxa");
    CHECK_EQ(std::string_view{table_properties.child("w:tblCellMar")
                                  .child("w:top")
                                  .attribute("w:w")
                                  .value()},
             "120");
    CHECK_EQ(std::string_view{table_properties.child("w:tblCellMar")
                                  .child("w:left")
                                  .attribute("w:w")
                                  .value()},
             "240");
    CHECK_EQ(std::string_view{table_properties.child("w:tblCellMar")
                                  .child("w:bottom")
                                  .attribute("w:w")
                                  .value()},
             "360");
    CHECK_EQ(std::string_view{table_properties.child("w:tblCellMar")
                                  .child("w:right")
                                  .attribute("w:w")
                                  .value()},
             "480");
    CHECK_EQ(count_named_children(table_node, "w:tr"), 2U);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    auto reopened_table = reopened.tables();
    REQUIRE(reopened_table.has_next());
    REQUIRE(reopened_table.width_twips().has_value());
    CHECK_EQ(*reopened_table.width_twips(), 7200U);
    REQUIRE(reopened_table.style_id().has_value());
    CHECK_EQ(*reopened_table.style_id(), "TableGrid");
    REQUIRE(reopened_table.layout_mode().has_value());
    CHECK_EQ(*reopened_table.layout_mode(), featherdoc::table_layout_mode::fixed);
    REQUIRE(reopened_table.alignment().has_value());
    CHECK_EQ(*reopened_table.alignment(), featherdoc::table_alignment::center);
    REQUIRE(reopened_table.indent_twips().has_value());
    CHECK_EQ(*reopened_table.indent_twips(), 360U);
    REQUIRE(reopened_table.cell_spacing_twips().has_value());
    CHECK_EQ(*reopened_table.cell_spacing_twips(), 180U);
    REQUIRE(reopened_table.cell_margin_twips(featherdoc::cell_margin_edge::top).has_value());
    CHECK_EQ(*reopened_table.cell_margin_twips(featherdoc::cell_margin_edge::top), 120U);
    REQUIRE(reopened_table.cell_margin_twips(featherdoc::cell_margin_edge::left).has_value());
    CHECK_EQ(*reopened_table.cell_margin_twips(featherdoc::cell_margin_edge::left), 240U);
    REQUIRE(reopened_table.cell_margin_twips(featherdoc::cell_margin_edge::bottom).has_value());
    CHECK_EQ(*reopened_table.cell_margin_twips(featherdoc::cell_margin_edge::bottom), 360U);
    REQUIRE(reopened_table.cell_margin_twips(featherdoc::cell_margin_edge::right).has_value());
    CHECK_EQ(*reopened_table.cell_margin_twips(featherdoc::cell_margin_edge::right), 480U);

    fs::remove(target);
}

TEST_CASE("table layout mode can be set and cleared") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "table_layout_mode.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto table = doc.append_table(1, 1);
    CHECK_FALSE(table.layout_mode().has_value());
    CHECK(table.set_layout_mode(featherdoc::table_layout_mode::fixed));
    REQUIRE(table.layout_mode().has_value());
    CHECK_EQ(*table.layout_mode(), featherdoc::table_layout_mode::fixed);
    CHECK_FALSE(doc.save());

    auto xml_text = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(xml_text.c_str()));

    auto table_node = xml_document.child("w:document").child("w:body").child("w:tbl");
    REQUIRE(table_node != pugi::xml_node{});
    auto layout_node = table_node.child("w:tblPr").child("w:tblLayout");
    REQUIRE(layout_node != pugi::xml_node{});
    CHECK_EQ(std::string_view{layout_node.attribute("w:type").value()}, "fixed");

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    auto reopened_table = reopened.tables();
    REQUIRE(reopened_table.has_next());
    REQUIRE(reopened_table.layout_mode().has_value());
    CHECK_EQ(*reopened_table.layout_mode(), featherdoc::table_layout_mode::fixed);
    CHECK(reopened_table.clear_layout_mode());
    CHECK_FALSE(reopened_table.layout_mode().has_value());
    CHECK_FALSE(reopened.save());

    xml_text = read_test_docx_entry(target, test_document_xml_entry);
    xml_document.reset();
    REQUIRE(xml_document.load_string(xml_text.c_str()));
    table_node = xml_document.child("w:document").child("w:body").child("w:tbl");
    REQUIRE(table_node != pugi::xml_node{});
    CHECK_EQ(table_node.child("w:tblPr").child("w:tblLayout"), pugi::xml_node{});

    fs::remove(target);
}

TEST_CASE("autofit and clear_layout_mode preserve explicit grid and cell widths") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "table_layout_mode_preserves_widths.docx";
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
    CHECK(table.set_layout_mode(featherdoc::table_layout_mode::fixed));

    REQUIRE(merged_header.width_twips().has_value());
    CHECK_EQ(*merged_header.width_twips(), 3000U);
    REQUIRE(header_tail.width_twips().has_value());
    CHECK_EQ(*header_tail.width_twips(), 2400U);

    CHECK(table.set_layout_mode(featherdoc::table_layout_mode::autofit));
    REQUIRE(table.layout_mode().has_value());
    CHECK_EQ(*table.layout_mode(), featherdoc::table_layout_mode::autofit);
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

    auto xml_text = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(xml_text.c_str()));

    auto table_node = xml_document.child("w:document").child("w:body").child("w:tbl");
    REQUIRE(table_node != pugi::xml_node{});
    CHECK_EQ(std::string_view{table_node.child("w:tblPr")
                                  .child("w:tblLayout")
                                  .attribute("w:type")
                                  .value()},
             "autofit");

    auto first_row = table_node.child("w:tr");
    REQUIRE(first_row != pugi::xml_node{});
    auto second_row = first_row.next_sibling("w:tr");
    REQUIRE(second_row != pugi::xml_node{});

    auto first_row_widths = collect_row_cell_width_values(first_row);
    REQUIRE_EQ(first_row_widths.size(), 2U);
    CHECK_EQ(first_row_widths[0], "3000");
    CHECK_EQ(first_row_widths[1], "2400");

    auto second_row_widths = collect_row_cell_width_values(second_row);
    REQUIRE_EQ(second_row_widths.size(), 3U);
    CHECK_EQ(second_row_widths[0], "1200");
    CHECK_EQ(second_row_widths[1], "1800");
    CHECK_EQ(second_row_widths[2], "2400");

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());

    auto reopened_table = reopened.tables();
    REQUIRE(reopened_table.has_next());
    REQUIRE(reopened_table.layout_mode().has_value());
    CHECK_EQ(*reopened_table.layout_mode(), featherdoc::table_layout_mode::autofit);
    REQUIRE(reopened_table.column_width_twips(0U).has_value());
    CHECK_EQ(*reopened_table.column_width_twips(0U), 1200U);
    REQUIRE(reopened_table.column_width_twips(1U).has_value());
    CHECK_EQ(*reopened_table.column_width_twips(1U), 1800U);
    REQUIRE(reopened_table.column_width_twips(2U).has_value());
    CHECK_EQ(*reopened_table.column_width_twips(2U), 2400U);

    auto reopened_header = reopened_table.rows().cells();
    REQUIRE(reopened_header.has_next());
    REQUIRE(reopened_header.width_twips().has_value());
    CHECK_EQ(*reopened_header.width_twips(), 3000U);
    reopened_header.next();
    REQUIRE(reopened_header.has_next());
    REQUIRE(reopened_header.width_twips().has_value());
    CHECK_EQ(*reopened_header.width_twips(), 2400U);

    CHECK(reopened_table.clear_layout_mode());
    CHECK_FALSE(reopened_table.layout_mode().has_value());
    REQUIRE(reopened_header.width_twips().has_value());
    CHECK_EQ(*reopened_header.width_twips(), 2400U);
    CHECK_FALSE(reopened.save());

    xml_text = read_test_docx_entry(target, test_document_xml_entry);
    xml_document.reset();
    REQUIRE(xml_document.load_string(xml_text.c_str()));

    table_node = xml_document.child("w:document").child("w:body").child("w:tbl");
    REQUIRE(table_node != pugi::xml_node{});
    CHECK_EQ(table_node.child("w:tblPr").child("w:tblLayout"), pugi::xml_node{});

    first_row = table_node.child("w:tr");
    REQUIRE(first_row != pugi::xml_node{});
    second_row = first_row.next_sibling("w:tr");
    REQUIRE(second_row != pugi::xml_node{});

    first_row_widths = collect_row_cell_width_values(first_row);
    REQUIRE_EQ(first_row_widths.size(), 2U);
    CHECK_EQ(first_row_widths[0], "3000");
    CHECK_EQ(first_row_widths[1], "2400");

    second_row_widths = collect_row_cell_width_values(second_row);
    REQUIRE_EQ(second_row_widths.size(), 3U);
    CHECK_EQ(second_row_widths[0], "1200");
    CHECK_EQ(second_row_widths[1], "1800");
    CHECK_EQ(second_row_widths[2], "2400");

    featherdoc::Document reopened_cleared(target);
    CHECK_FALSE(reopened_cleared.open());
    auto reopened_cleared_table = reopened_cleared.tables();
    REQUIRE(reopened_cleared_table.has_next());
    CHECK_FALSE(reopened_cleared_table.layout_mode().has_value());
    REQUIRE(reopened_cleared_table.column_width_twips(0U).has_value());
    CHECK_EQ(*reopened_cleared_table.column_width_twips(0U), 1200U);
    REQUIRE(reopened_cleared_table.column_width_twips(1U).has_value());
    CHECK_EQ(*reopened_cleared_table.column_width_twips(1U), 1800U);
    REQUIRE(reopened_cleared_table.column_width_twips(2U).has_value());
    CHECK_EQ(*reopened_cleared_table.column_width_twips(2U), 2400U);

    fs::remove(target);
}

TEST_CASE("table width changes preserve explicit grid and cell widths") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "table_width_preserves_grid_and_cell_widths.docx";
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
    CHECK(table.set_layout_mode(featherdoc::table_layout_mode::fixed));
    CHECK(table.set_width_twips(7200U));
    REQUIRE(table.width_twips().has_value());
    CHECK_EQ(*table.width_twips(), 7200U);

    REQUIRE(merged_header.width_twips().has_value());
    CHECK_EQ(*merged_header.width_twips(), 3000U);
    REQUIRE(header_tail.width_twips().has_value());
    CHECK_EQ(*header_tail.width_twips(), 2400U);

    CHECK(table.clear_width());
    CHECK_FALSE(table.width_twips().has_value());
    REQUIRE(table.column_width_twips(0U).has_value());
    CHECK_EQ(*table.column_width_twips(0U), 1200U);
    REQUIRE(table.column_width_twips(1U).has_value());
    CHECK_EQ(*table.column_width_twips(1U), 1800U);
    REQUIRE(table.column_width_twips(2U).has_value());
    CHECK_EQ(*table.column_width_twips(2U), 2400U);
    REQUIRE(merged_header.width_twips().has_value());
    CHECK_EQ(*merged_header.width_twips(), 3000U);
    REQUIRE(header_tail.width_twips().has_value());
    CHECK_EQ(*header_tail.width_twips(), 2400U);

    CHECK_FALSE(doc.save());

    auto xml_text = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(xml_text.c_str()));

    auto table_node = xml_document.child("w:document").child("w:body").child("w:tbl");
    REQUIRE(table_node != pugi::xml_node{});
    CHECK_EQ(table_node.child("w:tblPr").child("w:tblW"), pugi::xml_node{});

    auto grid_widths = collect_table_grid_width_values(table_node);
    REQUIRE_EQ(grid_widths.size(), 3U);
    CHECK_EQ(grid_widths[0], "1200");
    CHECK_EQ(grid_widths[1], "1800");
    CHECK_EQ(grid_widths[2], "2400");

    auto first_row = table_node.child("w:tr");
    REQUIRE(first_row != pugi::xml_node{});
    auto second_row = first_row.next_sibling("w:tr");
    REQUIRE(second_row != pugi::xml_node{});

    auto first_row_widths = collect_row_cell_width_values(first_row);
    REQUIRE_EQ(first_row_widths.size(), 2U);
    CHECK_EQ(first_row_widths[0], "3000");
    CHECK_EQ(first_row_widths[1], "2400");

    auto second_row_widths = collect_row_cell_width_values(second_row);
    REQUIRE_EQ(second_row_widths.size(), 3U);
    CHECK_EQ(second_row_widths[0], "1200");
    CHECK_EQ(second_row_widths[1], "1800");
    CHECK_EQ(second_row_widths[2], "2400");

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    auto reopened_table = reopened.tables();
    REQUIRE(reopened_table.has_next());
    CHECK_FALSE(reopened_table.width_twips().has_value());
    REQUIRE(reopened_table.column_width_twips(0U).has_value());
    CHECK_EQ(*reopened_table.column_width_twips(0U), 1200U);
    REQUIRE(reopened_table.column_width_twips(1U).has_value());
    CHECK_EQ(*reopened_table.column_width_twips(1U), 1800U);
    REQUIRE(reopened_table.column_width_twips(2U).has_value());
    CHECK_EQ(*reopened_table.column_width_twips(2U), 2400U);

    auto reopened_header = reopened_table.rows().cells();
    REQUIRE(reopened_header.has_next());
    REQUIRE(reopened_header.width_twips().has_value());
    CHECK_EQ(*reopened_header.width_twips(), 3000U);
    reopened_header.next();
    REQUIRE(reopened_header.has_next());
    REQUIRE(reopened_header.width_twips().has_value());
    CHECK_EQ(*reopened_header.width_twips(), 2400U);

    CHECK(reopened_table.set_width_twips(7800U));
    REQUIRE(reopened_table.width_twips().has_value());
    CHECK_EQ(*reopened_table.width_twips(), 7800U);
    REQUIRE(reopened_table.column_width_twips(0U).has_value());
    CHECK_EQ(*reopened_table.column_width_twips(0U), 1200U);
    REQUIRE(reopened_table.column_width_twips(1U).has_value());
    CHECK_EQ(*reopened_table.column_width_twips(1U), 1800U);
    REQUIRE(reopened_table.column_width_twips(2U).has_value());
    CHECK_EQ(*reopened_table.column_width_twips(2U), 2400U);
    REQUIRE(reopened_header.width_twips().has_value());
    CHECK_EQ(*reopened_header.width_twips(), 2400U);
    CHECK_FALSE(reopened.save());

    xml_text = read_test_docx_entry(target, test_document_xml_entry);
    xml_document.reset();
    REQUIRE(xml_document.load_string(xml_text.c_str()));

    table_node = xml_document.child("w:document").child("w:body").child("w:tbl");
    REQUIRE(table_node != pugi::xml_node{});
    const auto table_width = table_node.child("w:tblPr").child("w:tblW");
    REQUIRE(table_width != pugi::xml_node{});
    CHECK_EQ(std::string_view{table_width.attribute("w:w").value()}, "7800");
    CHECK_EQ(std::string_view{table_width.attribute("w:type").value()}, "dxa");

    grid_widths = collect_table_grid_width_values(table_node);
    REQUIRE_EQ(grid_widths.size(), 3U);
    CHECK_EQ(grid_widths[0], "1200");
    CHECK_EQ(grid_widths[1], "1800");
    CHECK_EQ(grid_widths[2], "2400");

    first_row = table_node.child("w:tr");
    REQUIRE(first_row != pugi::xml_node{});
    second_row = first_row.next_sibling("w:tr");
    REQUIRE(second_row != pugi::xml_node{});

    first_row_widths = collect_row_cell_width_values(first_row);
    REQUIRE_EQ(first_row_widths.size(), 2U);
    CHECK_EQ(first_row_widths[0], "3000");
    CHECK_EQ(first_row_widths[1], "2400");

    second_row_widths = collect_row_cell_width_values(second_row);
    REQUIRE_EQ(second_row_widths.size(), 3U);
    CHECK_EQ(second_row_widths[0], "1200");
    CHECK_EQ(second_row_widths[1], "1800");
    CHECK_EQ(second_row_widths[2], "2400");

    featherdoc::Document reopened_with_width(target);
    CHECK_FALSE(reopened_with_width.open());
    auto reopened_with_width_table = reopened_with_width.tables();
    REQUIRE(reopened_with_width_table.has_next());
    REQUIRE(reopened_with_width_table.width_twips().has_value());
    CHECK_EQ(*reopened_with_width_table.width_twips(), 7800U);
    REQUIRE(reopened_with_width_table.column_width_twips(0U).has_value());
    CHECK_EQ(*reopened_with_width_table.column_width_twips(0U), 1200U);
    REQUIRE(reopened_with_width_table.column_width_twips(1U).has_value());
    CHECK_EQ(*reopened_with_width_table.column_width_twips(1U), 1800U);
    REQUIRE(reopened_with_width_table.column_width_twips(2U).has_value());
    CHECK_EQ(*reopened_with_width_table.column_width_twips(2U), 2400U);

    fs::remove(target);
}

TEST_CASE("manual cell width overrides are re-normalized by fixed-grid synchronization") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "table_cell_width_override_normalization.docx";
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
    auto overridden_cell = body_cell;
    body_cell.next();
    REQUIRE(body_cell.has_next());
    CHECK(body_cell.set_text("r2c3"));
    auto cleared_cell = body_cell;

    CHECK(table.set_column_width_twips(0U, 1200U));
    CHECK(table.set_column_width_twips(1U, 1800U));
    CHECK(table.set_column_width_twips(2U, 2400U));
    CHECK(table.set_layout_mode(featherdoc::table_layout_mode::fixed));

    REQUIRE(merged_header.width_twips().has_value());
    CHECK_EQ(*merged_header.width_twips(), 3000U);
    REQUIRE(header_tail.width_twips().has_value());
    CHECK_EQ(*header_tail.width_twips(), 2400U);
    REQUIRE(overridden_cell.width_twips().has_value());
    CHECK_EQ(*overridden_cell.width_twips(), 1800U);
    REQUIRE(cleared_cell.width_twips().has_value());
    CHECK_EQ(*cleared_cell.width_twips(), 2400U);

    CHECK(overridden_cell.set_width_twips(900U));
    REQUIRE(overridden_cell.width_twips().has_value());
    CHECK_EQ(*overridden_cell.width_twips(), 900U);
    CHECK(cleared_cell.clear_width());
    CHECK_FALSE(cleared_cell.width_twips().has_value());

    CHECK(table.set_layout_mode(featherdoc::table_layout_mode::fixed));

    REQUIRE(merged_header.width_twips().has_value());
    CHECK_EQ(*merged_header.width_twips(), 3000U);
    REQUIRE(header_tail.width_twips().has_value());
    CHECK_EQ(*header_tail.width_twips(), 2400U);
    REQUIRE(overridden_cell.width_twips().has_value());
    CHECK_EQ(*overridden_cell.width_twips(), 1800U);
    REQUIRE(cleared_cell.width_twips().has_value());
    CHECK_EQ(*cleared_cell.width_twips(), 2400U);

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
    auto reopened_body_row = reopened_table.rows();
    reopened_body_row.next();
    REQUIRE(reopened_body_row.has_next());
    auto reopened_body = reopened_body_row.cells();
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

TEST_CASE("reapplying fixed layout normalizes reopened inconsistent grid and cell widths") {
    namespace fs = std::filesystem;

    const fs::path target =
        fs::current_path() / "table_reopened_fixed_layout_normalizes_inconsistent_widths.docx";
    fs::remove(target);

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:tbl>
      <w:tblPr>
        <w:tblLayout w:type="fixed"/>
      </w:tblPr>
      <w:tblGrid>
        <w:gridCol w:w="1200"/>
        <w:gridCol w:w="1800"/>
        <w:gridCol w:w="2400"/>
      </w:tblGrid>
      <w:tr>
        <w:tc>
          <w:tcPr>
            <w:tcW w:w="7777" w:type="dxa"/>
            <w:gridSpan w:val="2"/>
          </w:tcPr>
          <w:p><w:r><w:t>merged</w:t></w:r></w:p>
        </w:tc>
        <w:tc>
          <w:tcPr>
            <w:tcW w:w="5555" w:type="dxa"/>
          </w:tcPr>
          <w:p><w:r><w:t>tail</w:t></w:r></w:p>
        </w:tc>
      </w:tr>
      <w:tr>
        <w:tc>
          <w:tcPr>
            <w:tcW w:w="901" w:type="dxa"/>
          </w:tcPr>
          <w:p><w:r><w:t>r2c1</w:t></w:r></w:p>
        </w:tc>
        <w:tc>
          <w:tcPr>
            <w:tcW w:w="902" w:type="dxa"/>
          </w:tcPr>
          <w:p><w:r><w:t>r2c2</w:t></w:r></w:p>
        </w:tc>
        <w:tc>
          <w:tcPr>
            <w:tcW w:w="903" w:type="dxa"/>
          </w:tcPr>
          <w:p><w:r><w:t>r2c3</w:t></w:r></w:p>
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
    REQUIRE(table.layout_mode().has_value());
    CHECK_EQ(*table.layout_mode(), featherdoc::table_layout_mode::fixed);
    REQUIRE(table.column_width_twips(0U).has_value());
    CHECK_EQ(*table.column_width_twips(0U), 1200U);
    REQUIRE(table.column_width_twips(1U).has_value());
    CHECK_EQ(*table.column_width_twips(1U), 1800U);
    REQUIRE(table.column_width_twips(2U).has_value());
    CHECK_EQ(*table.column_width_twips(2U), 2400U);

    auto row = table.rows();
    REQUIRE(row.has_next());
    auto cell = row.cells();
    REQUIRE(cell.has_next());
    REQUIRE(cell.width_twips().has_value());
    CHECK_EQ(*cell.width_twips(), 7777U);
    cell.next();
    REQUIRE(cell.has_next());
    REQUIRE(cell.width_twips().has_value());
    CHECK_EQ(*cell.width_twips(), 5555U);

    row.next();
    REQUIRE(row.has_next());
    cell = row.cells();
    REQUIRE(cell.has_next());
    REQUIRE(cell.width_twips().has_value());
    CHECK_EQ(*cell.width_twips(), 901U);
    cell.next();
    REQUIRE(cell.has_next());
    REQUIRE(cell.width_twips().has_value());
    CHECK_EQ(*cell.width_twips(), 902U);
    cell.next();
    REQUIRE(cell.has_next());
    REQUIRE(cell.width_twips().has_value());
    CHECK_EQ(*cell.width_twips(), 903U);

    CHECK(table.set_layout_mode(featherdoc::table_layout_mode::fixed));

    row = table.rows();
    REQUIRE(row.has_next());
    cell = row.cells();
    REQUIRE(cell.has_next());
    REQUIRE(cell.width_twips().has_value());
    CHECK_EQ(*cell.width_twips(), 3000U);
    cell.next();
    REQUIRE(cell.has_next());
    REQUIRE(cell.width_twips().has_value());
    CHECK_EQ(*cell.width_twips(), 2400U);

    row.next();
    REQUIRE(row.has_next());
    cell = row.cells();
    REQUIRE(cell.has_next());
    REQUIRE(cell.width_twips().has_value());
    CHECK_EQ(*cell.width_twips(), 1200U);
    cell.next();
    REQUIRE(cell.has_next());
    REQUIRE(cell.width_twips().has_value());
    CHECK_EQ(*cell.width_twips(), 1800U);
    cell.next();
    REQUIRE(cell.has_next());
    REQUIRE(cell.width_twips().has_value());
    CHECK_EQ(*cell.width_twips(), 2400U);

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

    fs::remove(target);
}
