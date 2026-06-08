#include <array>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
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

TEST_CASE("non-grid edits keep reopened inconsistent fixed cell widths unchanged") {
    namespace fs = std::filesystem;

    const fs::path target =
        fs::current_path() / "table_reopened_fixed_layout_non_grid_edits_keep_stale_widths.docx";
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

    auto row = table.rows();
    REQUIRE(row.has_next());
    auto cell = row.cells();
    REQUIRE(cell.has_next());
    REQUIRE(cell.width_twips().has_value());
    CHECK_EQ(*cell.width_twips(), 7777U);
    CHECK(cell.set_text("merged updated"));
    REQUIRE(cell.width_twips().has_value());
    CHECK_EQ(*cell.width_twips(), 7777U);
    CHECK_EQ(cell.get_text(), "merged updated");

    cell.next();
    REQUIRE(cell.has_next());
    REQUIRE(cell.width_twips().has_value());
    CHECK_EQ(*cell.width_twips(), 5555U);
    CHECK(cell.set_fill_color("FFF2CC"));
    REQUIRE(cell.fill_color().has_value());
    CHECK_EQ(*cell.fill_color(), "FFF2CC");
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
    CHECK(cell.set_border(featherdoc::cell_border_edge::bottom,
                          {featherdoc::border_style::thick, 16U, "00AA00", 0U}));
    REQUIRE(cell.width_twips().has_value());
    CHECK_EQ(*cell.width_twips(), 902U);
    cell.next();
    REQUIRE(cell.has_next());
    REQUIRE(cell.width_twips().has_value());
    CHECK_EQ(*cell.width_twips(), 903U);

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
    CHECK_EQ(first_row_widths[0], "7777");
    CHECK_EQ(first_row_widths[1], "5555");

    const auto second_row_widths = collect_row_cell_width_values(second_row);
    REQUIRE_EQ(second_row_widths.size(), 3U);
    CHECK_EQ(second_row_widths[0], "901");
    CHECK_EQ(second_row_widths[1], "902");
    CHECK_EQ(second_row_widths[2], "903");

    const auto first_row_merged = first_row.child("w:tc");
    REQUIRE(first_row_merged != pugi::xml_node{});
    CHECK_EQ(first_row_merged.child("w:p").child("w:r").child("w:t").text().get(),
             std::string{"merged updated"});

    const auto first_row_tail = first_row_merged.next_sibling("w:tc");
    REQUIRE(first_row_tail != pugi::xml_node{});
    const auto tail_shading = first_row_tail.child("w:tcPr").child("w:shd");
    REQUIRE(tail_shading != pugi::xml_node{});
    CHECK_EQ(std::string_view{tail_shading.attribute("w:fill").value()}, "FFF2CC");

    const auto second_row_middle = second_row.child("w:tc").next_sibling("w:tc");
    REQUIRE(second_row_middle != pugi::xml_node{});
    const auto bottom_border =
        second_row_middle.child("w:tcPr").child("w:tcBorders").child("w:bottom");
    REQUIRE(bottom_border != pugi::xml_node{});
    CHECK_EQ(std::string_view{bottom_border.attribute("w:val").value()}, "thick");
    CHECK_EQ(std::string_view{bottom_border.attribute("w:sz").value()}, "16");
    CHECK_EQ(std::string_view{bottom_border.attribute("w:color").value()}, "00AA00");
    CHECK_EQ(std::string_view{bottom_border.attribute("w:space").value()}, "0");

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    auto reopened_table = reopened.tables();
    REQUIRE(reopened_table.has_next());
    REQUIRE(reopened_table.layout_mode().has_value());
    CHECK_EQ(*reopened_table.layout_mode(), featherdoc::table_layout_mode::fixed);

    auto reopened_row = reopened_table.rows();
    REQUIRE(reopened_row.has_next());
    auto reopened_cell = reopened_row.cells();
    REQUIRE(reopened_cell.has_next());
    REQUIRE(reopened_cell.width_twips().has_value());
    CHECK_EQ(*reopened_cell.width_twips(), 7777U);
    CHECK_EQ(reopened_cell.get_text(), "merged updated");
    reopened_cell.next();
    REQUIRE(reopened_cell.has_next());
    REQUIRE(reopened_cell.width_twips().has_value());
    CHECK_EQ(*reopened_cell.width_twips(), 5555U);
    REQUIRE(reopened_cell.fill_color().has_value());
    CHECK_EQ(*reopened_cell.fill_color(), "FFF2CC");

    reopened_row.next();
    REQUIRE(reopened_row.has_next());
    reopened_cell = reopened_row.cells();
    REQUIRE(reopened_cell.has_next());
    REQUIRE(reopened_cell.width_twips().has_value());
    CHECK_EQ(*reopened_cell.width_twips(), 901U);
    reopened_cell.next();
    REQUIRE(reopened_cell.has_next());
    REQUIRE(reopened_cell.width_twips().has_value());
    CHECK_EQ(*reopened_cell.width_twips(), 902U);
    reopened_cell.next();
    REQUIRE(reopened_cell.has_next());
    REQUIRE(reopened_cell.width_twips().has_value());
    CHECK_EQ(*reopened_cell.width_twips(), 903U);

    fs::remove(target);
}

TEST_CASE("column insertion normalizes reopened inconsistent fixed cell widths") {
    namespace fs = std::filesystem;

    const fs::path target =
        fs::current_path() / "table_reopened_fixed_layout_insert_normalizes_stale_widths.docx";
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

    auto inserted = table.rows().cells().insert_cell_after();
    REQUIRE(inserted.has_next());
    CHECK(inserted.set_text("after-merged"));
    REQUIRE(inserted.width_twips().has_value());
    CHECK_EQ(*inserted.width_twips(), 1800U);

    REQUIRE(table.column_width_twips(0U).has_value());
    CHECK_EQ(*table.column_width_twips(0U), 1200U);
    REQUIRE(table.column_width_twips(1U).has_value());
    CHECK_EQ(*table.column_width_twips(1U), 1800U);
    REQUIRE(table.column_width_twips(2U).has_value());
    CHECK_EQ(*table.column_width_twips(2U), 1800U);
    REQUIRE(table.column_width_twips(3U).has_value());
    CHECK_EQ(*table.column_width_twips(3U), 2400U);

    auto row = table.rows();
    REQUIRE(row.has_next());
    auto cell = row.cells();
    REQUIRE(cell.has_next());
    REQUIRE(cell.width_twips().has_value());
    CHECK_EQ(*cell.width_twips(), 3000U);
    cell.next();
    REQUIRE(cell.has_next());
    REQUIRE(cell.width_twips().has_value());
    CHECK_EQ(*cell.width_twips(), 1800U);
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
    const auto grid_widths = collect_table_grid_width_values(table_node);
    REQUIRE_EQ(grid_widths.size(), 4U);
    CHECK_EQ(grid_widths[0], "1200");
    CHECK_EQ(grid_widths[1], "1800");
    CHECK_EQ(grid_widths[2], "1800");
    CHECK_EQ(grid_widths[3], "2400");

    const auto first_row = table_node.child("w:tr");
    REQUIRE(first_row != pugi::xml_node{});
    const auto second_row = first_row.next_sibling("w:tr");
    REQUIRE(second_row != pugi::xml_node{});

    const auto first_row_widths = collect_row_cell_width_values(first_row);
    REQUIRE_EQ(first_row_widths.size(), 3U);
    CHECK_EQ(first_row_widths[0], "3000");
    CHECK_EQ(first_row_widths[1], "1800");
    CHECK_EQ(first_row_widths[2], "2400");

    const auto second_row_widths = collect_row_cell_width_values(second_row);
    REQUIRE_EQ(second_row_widths.size(), 4U);
    CHECK_EQ(second_row_widths[0], "1200");
    CHECK_EQ(second_row_widths[1], "1800");
    CHECK_EQ(second_row_widths[2], "1800");
    CHECK_EQ(second_row_widths[3], "2400");

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
    CHECK_EQ(reopened_cell.get_text(), "after-merged");
    REQUIRE(reopened_cell.width_twips().has_value());
    CHECK_EQ(*reopened_cell.width_twips(), 1800U);
    reopened_cell.next();
    REQUIRE(reopened_cell.has_next());
    REQUIRE(reopened_cell.width_twips().has_value());
    CHECK_EQ(*reopened_cell.width_twips(), 2400U);

    fs::remove(target);
}

TEST_CASE("column insertion before normalizes reopened inconsistent fixed cell widths") {
    namespace fs = std::filesystem;

    const fs::path target =
        fs::current_path() / "table_reopened_fixed_layout_insert_before_normalizes_stale_widths.docx";
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

    auto row = table.rows();
    REQUIRE(row.has_next());
    auto tail = row.cells();
    tail.next();
    REQUIRE(tail.has_next());
    auto inserted = tail.insert_cell_before();
    REQUIRE(inserted.has_next());
    CHECK(inserted.set_text("before-tail"));
    REQUIRE(inserted.width_twips().has_value());
    CHECK_EQ(*inserted.width_twips(), 2400U);

    REQUIRE(table.column_width_twips(0U).has_value());
    CHECK_EQ(*table.column_width_twips(0U), 1200U);
    REQUIRE(table.column_width_twips(1U).has_value());
    CHECK_EQ(*table.column_width_twips(1U), 1800U);
    REQUIRE(table.column_width_twips(2U).has_value());
    CHECK_EQ(*table.column_width_twips(2U), 2400U);
    REQUIRE(table.column_width_twips(3U).has_value());
    CHECK_EQ(*table.column_width_twips(3U), 2400U);

    row = table.rows();
    REQUIRE(row.has_next());
    auto cell = row.cells();
    REQUIRE(cell.has_next());
    REQUIRE(cell.width_twips().has_value());
    CHECK_EQ(*cell.width_twips(), 3000U);
    cell.next();
    REQUIRE(cell.has_next());
    REQUIRE(cell.width_twips().has_value());
    CHECK_EQ(*cell.width_twips(), 2400U);
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
    const auto grid_widths = collect_table_grid_width_values(table_node);
    REQUIRE_EQ(grid_widths.size(), 4U);
    CHECK_EQ(grid_widths[0], "1200");
    CHECK_EQ(grid_widths[1], "1800");
    CHECK_EQ(grid_widths[2], "2400");
    CHECK_EQ(grid_widths[3], "2400");

    const auto first_row = table_node.child("w:tr");
    REQUIRE(first_row != pugi::xml_node{});
    const auto second_row = first_row.next_sibling("w:tr");
    REQUIRE(second_row != pugi::xml_node{});

    const auto first_row_widths = collect_row_cell_width_values(first_row);
    REQUIRE_EQ(first_row_widths.size(), 3U);
    CHECK_EQ(first_row_widths[0], "3000");
    CHECK_EQ(first_row_widths[1], "2400");
    CHECK_EQ(first_row_widths[2], "2400");

    const auto second_row_widths = collect_row_cell_width_values(second_row);
    REQUIRE_EQ(second_row_widths.size(), 4U);
    CHECK_EQ(second_row_widths[0], "1200");
    CHECK_EQ(second_row_widths[1], "1800");
    CHECK_EQ(second_row_widths[2], "2400");
    CHECK_EQ(second_row_widths[3], "2400");

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
    CHECK_EQ(reopened_cell.get_text(), "before-tail");
    REQUIRE(reopened_cell.width_twips().has_value());
    CHECK_EQ(*reopened_cell.width_twips(), 2400U);
    reopened_cell.next();
    REQUIRE(reopened_cell.has_next());
    REQUIRE(reopened_cell.width_twips().has_value());
    CHECK_EQ(*reopened_cell.width_twips(), 2400U);

    fs::remove(target);
}

TEST_CASE("column removal normalizes reopened inconsistent fixed cell widths") {
    namespace fs = std::filesystem;

    const fs::path target =
        fs::current_path() / "table_reopened_fixed_layout_remove_normalizes_stale_widths.docx";
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

    auto row = table.rows();
    row.next();
    REQUIRE(row.has_next());
    auto removable = row.cells();
    removable.next();
    removable.next();
    REQUIRE(removable.has_next());
    REQUIRE(removable.width_twips().has_value());
    CHECK_EQ(*removable.width_twips(), 903U);
    CHECK(removable.remove());
    CHECK(removable.has_next());
    CHECK_EQ(removable.get_text(), "r2c2");
    REQUIRE(removable.width_twips().has_value());
    CHECK_EQ(*removable.width_twips(), 1800U);

    REQUIRE(table.column_width_twips(0U).has_value());
    CHECK_EQ(*table.column_width_twips(0U), 1200U);
    REQUIRE(table.column_width_twips(1U).has_value());
    CHECK_EQ(*table.column_width_twips(1U), 1800U);
    CHECK_FALSE(table.column_width_twips(2U).has_value());

    row = table.rows();
    REQUIRE(row.has_next());
    auto cell = row.cells();
    REQUIRE(cell.has_next());
    REQUIRE(cell.width_twips().has_value());
    CHECK_EQ(*cell.width_twips(), 3000U);
    CHECK_FALSE(cell.next().has_next());

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
    CHECK_FALSE(cell.next().has_next());

    CHECK_FALSE(doc.save());

    const auto xml_text = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(xml_text.c_str()));

    const auto table_node = xml_document.child("w:document").child("w:body").child("w:tbl");
    REQUIRE(table_node != pugi::xml_node{});
    const auto grid_widths = collect_table_grid_width_values(table_node);
    REQUIRE_EQ(grid_widths.size(), 2U);
    CHECK_EQ(grid_widths[0], "1200");
    CHECK_EQ(grid_widths[1], "1800");

    const auto first_row = table_node.child("w:tr");
    REQUIRE(first_row != pugi::xml_node{});
    const auto second_row = first_row.next_sibling("w:tr");
    REQUIRE(second_row != pugi::xml_node{});
    CHECK_EQ(count_named_children(first_row, "w:tc"), 1U);
    CHECK_EQ(count_named_children(second_row, "w:tc"), 2U);

    const auto first_row_widths = collect_row_cell_width_values(first_row);
    REQUIRE_EQ(first_row_widths.size(), 1U);
    CHECK_EQ(first_row_widths[0], "3000");

    const auto second_row_widths = collect_row_cell_width_values(second_row);
    REQUIRE_EQ(second_row_widths.size(), 2U);
    CHECK_EQ(second_row_widths[0], "1200");
    CHECK_EQ(second_row_widths[1], "1800");

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    auto reopened_table = reopened.tables();
    REQUIRE(reopened_table.has_next());
    auto reopened_row = reopened_table.rows();
    reopened_row.next();
    REQUIRE(reopened_row.has_next());
    auto reopened_cell = reopened_row.cells();
    reopened_cell.next();
    REQUIRE(reopened_cell.has_next());
    CHECK_EQ(reopened_cell.get_text(), "r2c2");
    REQUIRE(reopened_cell.width_twips().has_value());
    CHECK_EQ(*reopened_cell.width_twips(), 1800U);
    CHECK_FALSE(reopened_cell.next().has_next());

    fs::remove(target);
}

TEST_CASE("column width edits normalize reopened inconsistent fixed cell widths") {
    namespace fs = std::filesystem;

    const fs::path target =
        fs::current_path() / "table_reopened_fixed_layout_set_column_width_normalizes_stale_widths.docx";
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

    CHECK(table.set_column_width_twips(1U, 2100U));
    REQUIRE(table.column_width_twips(0U).has_value());
    CHECK_EQ(*table.column_width_twips(0U), 1200U);
    REQUIRE(table.column_width_twips(1U).has_value());
    CHECK_EQ(*table.column_width_twips(1U), 2100U);
    REQUIRE(table.column_width_twips(2U).has_value());
    CHECK_EQ(*table.column_width_twips(2U), 2400U);

    auto row = table.rows();
    REQUIRE(row.has_next());
    auto cell = row.cells();
    REQUIRE(cell.has_next());
    REQUIRE(cell.width_twips().has_value());
    CHECK_EQ(*cell.width_twips(), 3300U);
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
    CHECK_EQ(*cell.width_twips(), 2100U);
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
    const auto grid_widths = collect_table_grid_width_values(table_node);
    REQUIRE_EQ(grid_widths.size(), 3U);
    CHECK_EQ(grid_widths[0], "1200");
    CHECK_EQ(grid_widths[1], "2100");
    CHECK_EQ(grid_widths[2], "2400");

    const auto first_row = table_node.child("w:tr");
    REQUIRE(first_row != pugi::xml_node{});
    const auto second_row = first_row.next_sibling("w:tr");
    REQUIRE(second_row != pugi::xml_node{});

    const auto first_row_widths = collect_row_cell_width_values(first_row);
    REQUIRE_EQ(first_row_widths.size(), 2U);
    CHECK_EQ(first_row_widths[0], "3300");
    CHECK_EQ(first_row_widths[1], "2400");

    const auto second_row_widths = collect_row_cell_width_values(second_row);
    REQUIRE_EQ(second_row_widths.size(), 3U);
    CHECK_EQ(second_row_widths[0], "1200");
    CHECK_EQ(second_row_widths[1], "2100");
    CHECK_EQ(second_row_widths[2], "2400");

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    auto reopened_table = reopened.tables();
    REQUIRE(reopened_table.has_next());
    REQUIRE(reopened_table.column_width_twips(0U).has_value());
    CHECK_EQ(*reopened_table.column_width_twips(0U), 1200U);
    REQUIRE(reopened_table.column_width_twips(1U).has_value());
    CHECK_EQ(*reopened_table.column_width_twips(1U), 2100U);
    REQUIRE(reopened_table.column_width_twips(2U).has_value());
    CHECK_EQ(*reopened_table.column_width_twips(2U), 2400U);

    auto reopened_row = reopened_table.rows();
    REQUIRE(reopened_row.has_next());
    auto reopened_cell = reopened_row.cells();
    REQUIRE(reopened_cell.has_next());
    REQUIRE(reopened_cell.width_twips().has_value());
    CHECK_EQ(*reopened_cell.width_twips(), 3300U);
    reopened_cell.next();
    REQUIRE(reopened_cell.has_next());
    REQUIRE(reopened_cell.width_twips().has_value());
    CHECK_EQ(*reopened_cell.width_twips(), 2400U);

    fs::remove(target);
}

TEST_CASE("reopened fixed-layout column width workflow keeps tblGrid and tcW aligned") {
    namespace fs = std::filesystem;

    const fs::path target =
        fs::current_path() / "table_reopened_fixed_layout_column_width_workflow.docx";
    fs::remove(target);
    const std::array<const char *, 3> header_texts = {
        "Key",
        "Review state",
        "Evidence and implementation note",
    };
    const std::array<const char *, 3> first_body_texts = {
        "A-7",
        "Waiting",
        "This column will become the widest one after reopening the document.",
    };
    const std::array<const char *, 3> second_body_texts = {
        "B-2",
        "Ready",
        "Use Table::set_column_width_twips(...) to widen this detail column while keeping the table layout fixed.",
    };

    auto fill_row = [](featherdoc::TableRow row,
                       const std::array<const char *, 3> &texts) {
        if (!row.has_next()) {
            return false;
        }

        auto cell = row.cells();
        for (const auto *text : texts) {
            if (!cell.has_next() || !cell.set_text(text)) {
                return false;
            }
            cell.next();
        }

        return true;
    };

    featherdoc::Document seed(target);
    CHECK_FALSE(seed.create_empty());

    auto paragraph = seed.paragraphs();
    REQUIRE(paragraph.has_next());
    CHECK(paragraph.set_text("Seed a three-column table, reopen it, and edit tblGrid column widths "
                             "without touching raw XML."));

    auto table = seed.append_table(3, 3);
    REQUIRE(table.has_next());
    CHECK(table.set_width_twips(7800U));
    CHECK(table.set_layout_mode(featherdoc::table_layout_mode::fixed));
    CHECK(table.set_alignment(featherdoc::table_alignment::center));
    CHECK(table.set_style_id("TableGrid"));

    auto row = table.rows();
    CHECK(fill_row(row, header_texts));
    row.next();
    CHECK(fill_row(row, first_body_texts));
    row.next();
    CHECK(fill_row(row, second_body_texts));

    CHECK_FALSE(seed.save());

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    paragraph = doc.paragraphs();
    REQUIRE(paragraph.has_next());
    REQUIRE(paragraph
                .add_run(" The reopened table now uses column widths 1200 / 2200 / 4400 twips "
                         "through Table::set_column_width_twips(...), then re-applies fixed layout "
                         "to normalize any stale cell tcW values back to tblGrid.")
                .has_next());

    table = doc.tables();
    REQUIRE(table.has_next());
    CHECK(table.set_column_width_twips(0U, 1200U));
    CHECK(table.set_column_width_twips(1U, 2200U));
    CHECK(table.set_column_width_twips(2U, 4400U));
    CHECK(table.clear_column_width(1U));
    CHECK_FALSE(table.column_width_twips(1U).has_value());
    CHECK(table.set_column_width_twips(1U, 2200U));
    CHECK(table.set_layout_mode(featherdoc::table_layout_mode::fixed));

    REQUIRE(table.layout_mode().has_value());
    CHECK_EQ(*table.layout_mode(), featherdoc::table_layout_mode::fixed);
    REQUIRE(table.alignment().has_value());
    CHECK_EQ(*table.alignment(), featherdoc::table_alignment::center);
    REQUIRE(table.column_width_twips(0U).has_value());
    CHECK_EQ(*table.column_width_twips(0U), 1200U);
    REQUIRE(table.column_width_twips(1U).has_value());
    CHECK_EQ(*table.column_width_twips(1U), 2200U);
    REQUIRE(table.column_width_twips(2U).has_value());
    CHECK_EQ(*table.column_width_twips(2U), 4400U);

    row = table.rows();
    const std::array<std::uint32_t, 3> expected_widths = {1200U, 2200U, 4400U};
    for (std::size_t row_index = 0; row_index < 3U; ++row_index) {
        REQUIRE(row.has_next());
        auto cell = row.cells();
        for (const auto expected_width : expected_widths) {
            REQUIRE(cell.has_next());
            REQUIRE(cell.width_twips().has_value());
            CHECK_EQ(*cell.width_twips(), expected_width);
            cell.next();
        }
        row.next();
    }

    CHECK_FALSE(doc.save());

    const auto xml_text = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(xml_text.c_str()));

    const auto table_node = xml_document.child("w:document").child("w:body").child("w:tbl");
    REQUIRE(table_node != pugi::xml_node{});
    CHECK_EQ(table_node.child("w:tblPr").child("w:tblLayout").attribute("w:type").value(),
             std::string{"fixed"});
    CHECK_EQ(table_node.child("w:tblPr").child("w:tblW").attribute("w:w").value(), std::string{"7800"});

    const auto table_grid_widths = collect_table_grid_width_values(table_node);
    REQUIRE_EQ(table_grid_widths.size(), 3U);
    CHECK_EQ(table_grid_widths[0], "1200");
    CHECK_EQ(table_grid_widths[1], "2200");
    CHECK_EQ(table_grid_widths[2], "4400");

    auto xml_row = table_node.child("w:tr");
    for (std::size_t row_index = 0; row_index < 3U; ++row_index) {
        REQUIRE(xml_row != pugi::xml_node{});
        const auto row_widths = collect_row_cell_width_values(xml_row);
        REQUIRE_EQ(row_widths.size(), 3U);
        CHECK_EQ(row_widths[0], "1200");
        CHECK_EQ(row_widths[1], "2200");
        CHECK_EQ(row_widths[2], "4400");
        xml_row = xml_row.next_sibling("w:tr");
    }

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_NE(collect_document_text(reopened).find("1200 / 2200 / 4400 twips"), std::string::npos);

    auto reopened_table = reopened.tables();
    REQUIRE(reopened_table.has_next());
    REQUIRE(reopened_table.layout_mode().has_value());
    CHECK_EQ(*reopened_table.layout_mode(), featherdoc::table_layout_mode::fixed);
    REQUIRE(reopened_table.alignment().has_value());
    CHECK_EQ(*reopened_table.alignment(), featherdoc::table_alignment::center);
    REQUIRE(reopened_table.column_width_twips(0U).has_value());
    CHECK_EQ(*reopened_table.column_width_twips(0U), 1200U);
    REQUIRE(reopened_table.column_width_twips(1U).has_value());
    CHECK_EQ(*reopened_table.column_width_twips(1U), 2200U);
    REQUIRE(reopened_table.column_width_twips(2U).has_value());
    CHECK_EQ(*reopened_table.column_width_twips(2U), 4400U);

    auto reopened_row = reopened_table.rows();
    for (std::size_t row_index = 0; row_index < 3U; ++row_index) {
        REQUIRE(reopened_row.has_next());
        auto reopened_cell = reopened_row.cells();
        for (const auto expected_width : expected_widths) {
            REQUIRE(reopened_cell.has_next());
            REQUIRE(reopened_cell.width_twips().has_value());
            CHECK_EQ(*reopened_cell.width_twips(), expected_width);
            reopened_cell.next();
        }
        reopened_row.next();
    }

    fs::remove(target);
}

TEST_CASE("clearing a grid column on reopened inconsistent fixed tables clears only affected tcW") {
    namespace fs = std::filesystem;

    const fs::path target =
        fs::current_path() / "table_reopened_fixed_layout_clear_column_keeps_unaffected_stale_widths.docx";
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

    CHECK(table.clear_column_width(1U));
    CHECK_FALSE(table.column_width_twips(1U).has_value());
    REQUIRE(table.column_width_twips(0U).has_value());
    CHECK_EQ(*table.column_width_twips(0U), 1200U);
    REQUIRE(table.column_width_twips(2U).has_value());
    CHECK_EQ(*table.column_width_twips(2U), 2400U);

    auto row = table.rows();
    REQUIRE(row.has_next());
    auto cell = row.cells();
    REQUIRE(cell.has_next());
    CHECK_FALSE(cell.width_twips().has_value());
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
    CHECK_FALSE(cell.width_twips().has_value());
    cell.next();
    REQUIRE(cell.has_next());
    REQUIRE(cell.width_twips().has_value());
    CHECK_EQ(*cell.width_twips(), 903U);

    CHECK_FALSE(doc.save());

    const auto xml_text = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(xml_text.c_str()));

    const auto table_node = xml_document.child("w:document").child("w:body").child("w:tbl");
    REQUIRE(table_node != pugi::xml_node{});
    const auto grid_widths = collect_table_grid_width_values(table_node);
    REQUIRE_EQ(grid_widths.size(), 3U);
    CHECK_EQ(grid_widths[0], "1200");
    CHECK_EQ(grid_widths[1], "");
    CHECK_EQ(grid_widths[2], "2400");

    const auto first_row = table_node.child("w:tr");
    REQUIRE(first_row != pugi::xml_node{});
    const auto second_row = first_row.next_sibling("w:tr");
    REQUIRE(second_row != pugi::xml_node{});

    const auto first_row_widths = collect_row_cell_width_values(first_row);
    REQUIRE_EQ(first_row_widths.size(), 2U);
    CHECK_EQ(first_row_widths[0], "");
    CHECK_EQ(first_row_widths[1], "5555");

    const auto second_row_widths = collect_row_cell_width_values(second_row);
    REQUIRE_EQ(second_row_widths.size(), 3U);
    CHECK_EQ(second_row_widths[0], "901");
    CHECK_EQ(second_row_widths[1], "");
    CHECK_EQ(second_row_widths[2], "903");

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    auto reopened_table = reopened.tables();
    REQUIRE(reopened_table.has_next());
    CHECK_FALSE(reopened_table.column_width_twips(1U).has_value());
    REQUIRE(reopened_table.column_width_twips(0U).has_value());
    CHECK_EQ(*reopened_table.column_width_twips(0U), 1200U);
    REQUIRE(reopened_table.column_width_twips(2U).has_value());
    CHECK_EQ(*reopened_table.column_width_twips(2U), 2400U);

    auto reopened_row = reopened_table.rows();
    REQUIRE(reopened_row.has_next());
    auto reopened_cell = reopened_row.cells();
    REQUIRE(reopened_cell.has_next());
    CHECK_FALSE(reopened_cell.width_twips().has_value());
    reopened_cell.next();
    REQUIRE(reopened_cell.has_next());
    REQUIRE(reopened_cell.width_twips().has_value());
    CHECK_EQ(*reopened_cell.width_twips(), 5555U);

    reopened_row.next();
    REQUIRE(reopened_row.has_next());
    reopened_cell = reopened_row.cells();
    REQUIRE(reopened_cell.has_next());
    REQUIRE(reopened_cell.width_twips().has_value());
    CHECK_EQ(*reopened_cell.width_twips(), 901U);
    reopened_cell.next();
    REQUIRE(reopened_cell.has_next());
    CHECK_FALSE(reopened_cell.width_twips().has_value());
    reopened_cell.next();
    REQUIRE(reopened_cell.has_next());
    REQUIRE(reopened_cell.width_twips().has_value());
    CHECK_EQ(*reopened_cell.width_twips(), 903U);

    fs::remove(target);
}

TEST_CASE("row insertion on reopened inconsistent fixed tables preserves stale tcW") {
    namespace fs = std::filesystem;

    const fs::path target =
        fs::current_path() / "table_reopened_fixed_layout_row_insert_preserves_stale_widths.docx";
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

    auto row = table.rows();
    REQUIRE(row.has_next());
    auto inserted = row.insert_row_after();
    REQUIRE(inserted.has_next());
    CHECK(inserted.cells().set_text("inserted-merged"));

    auto inserted_cell = inserted.cells();
    REQUIRE(inserted_cell.has_next());
    REQUIRE(inserted_cell.width_twips().has_value());
    CHECK_EQ(*inserted_cell.width_twips(), 7777U);
    inserted_cell.next();
    REQUIRE(inserted_cell.has_next());
    REQUIRE(inserted_cell.width_twips().has_value());
    CHECK_EQ(*inserted_cell.width_twips(), 5555U);

    row = table.rows();
    REQUIRE(row.has_next());
    auto original_first = row.cells();
    REQUIRE(original_first.has_next());
    REQUIRE(original_first.width_twips().has_value());
    CHECK_EQ(*original_first.width_twips(), 7777U);

    row.next();
    REQUIRE(row.has_next());
    auto inserted_row_cell = row.cells();
    REQUIRE(inserted_row_cell.has_next());
    REQUIRE(inserted_row_cell.width_twips().has_value());
    CHECK_EQ(*inserted_row_cell.width_twips(), 7777U);

    row.next();
    REQUIRE(row.has_next());
    auto body_cell = row.cells();
    REQUIRE(body_cell.has_next());
    REQUIRE(body_cell.width_twips().has_value());
    CHECK_EQ(*body_cell.width_twips(), 901U);
    body_cell.next();
    REQUIRE(body_cell.has_next());
    REQUIRE(body_cell.width_twips().has_value());
    CHECK_EQ(*body_cell.width_twips(), 902U);
    body_cell.next();
    REQUIRE(body_cell.has_next());
    REQUIRE(body_cell.width_twips().has_value());
    CHECK_EQ(*body_cell.width_twips(), 903U);

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
    const auto third_row = second_row.next_sibling("w:tr");
    REQUIRE(third_row != pugi::xml_node{});

    const auto first_row_widths = collect_row_cell_width_values(first_row);
    REQUIRE_EQ(first_row_widths.size(), 2U);
    CHECK_EQ(first_row_widths[0], "7777");
    CHECK_EQ(first_row_widths[1], "5555");

    const auto second_row_widths = collect_row_cell_width_values(second_row);
    REQUIRE_EQ(second_row_widths.size(), 2U);
    CHECK_EQ(second_row_widths[0], "7777");
    CHECK_EQ(second_row_widths[1], "5555");

    const auto third_row_widths = collect_row_cell_width_values(third_row);
    REQUIRE_EQ(third_row_widths.size(), 3U);
    CHECK_EQ(third_row_widths[0], "901");
    CHECK_EQ(third_row_widths[1], "902");
    CHECK_EQ(third_row_widths[2], "903");

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    auto reopened_table = reopened.tables();
    REQUIRE(reopened_table.has_next());
    auto reopened_row = reopened_table.rows();
    reopened_row.next();
    REQUIRE(reopened_row.has_next());
    auto reopened_cell = reopened_row.cells();
    REQUIRE(reopened_cell.has_next());
    CHECK_EQ(reopened_cell.get_text(), "inserted-merged");
    REQUIRE(reopened_cell.width_twips().has_value());
    CHECK_EQ(*reopened_cell.width_twips(), 7777U);
    reopened_cell.next();
    REQUIRE(reopened_cell.has_next());
    REQUIRE(reopened_cell.width_twips().has_value());
    CHECK_EQ(*reopened_cell.width_twips(), 5555U);

    fs::remove(target);
}

TEST_CASE("row removal on reopened inconsistent fixed tables preserves surviving stale tcW") {
    namespace fs = std::filesystem;

    const fs::path target =
        fs::current_path() / "table_reopened_fixed_layout_row_remove_preserves_stale_widths.docx";
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
          <w:p><w:r><w:t>remove-1</w:t></w:r></w:p>
        </w:tc>
        <w:tc>
          <w:tcPr>
            <w:tcW w:w="902" w:type="dxa"/>
          </w:tcPr>
          <w:p><w:r><w:t>remove-2</w:t></w:r></w:p>
        </w:tc>
        <w:tc>
          <w:tcPr>
            <w:tcW w:w="903" w:type="dxa"/>
          </w:tcPr>
          <w:p><w:r><w:t>remove-3</w:t></w:r></w:p>
        </w:tc>
      </w:tr>
      <w:tr>
        <w:tc>
          <w:tcPr>
            <w:tcW w:w="1001" w:type="dxa"/>
          </w:tcPr>
          <w:p><w:r><w:t>keep-1</w:t></w:r></w:p>
        </w:tc>
        <w:tc>
          <w:tcPr>
            <w:tcW w:w="1002" w:type="dxa"/>
          </w:tcPr>
          <w:p><w:r><w:t>keep-2</w:t></w:r></w:p>
        </w:tc>
        <w:tc>
          <w:tcPr>
            <w:tcW w:w="1003" w:type="dxa"/>
          </w:tcPr>
          <w:p><w:r><w:t>keep-3</w:t></w:r></w:p>
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

    auto row = table.rows();
    row.next();
    REQUIRE(row.has_next());
    CHECK(row.remove());
    CHECK(row.has_next());

    auto current_cell = row.cells();
    REQUIRE(current_cell.has_next());
    CHECK_EQ(current_cell.get_text(), "keep-1");
    REQUIRE(current_cell.width_twips().has_value());
    CHECK_EQ(*current_cell.width_twips(), 1001U);
    current_cell.next();
    REQUIRE(current_cell.has_next());
    REQUIRE(current_cell.width_twips().has_value());
    CHECK_EQ(*current_cell.width_twips(), 1002U);
    current_cell.next();
    REQUIRE(current_cell.has_next());
    REQUIRE(current_cell.width_twips().has_value());
    CHECK_EQ(*current_cell.width_twips(), 1003U);

    auto first_row = table.rows();
    REQUIRE(first_row.has_next());
    auto first_cell = first_row.cells();
    REQUIRE(first_cell.has_next());
    REQUIRE(first_cell.width_twips().has_value());
    CHECK_EQ(*first_cell.width_twips(), 7777U);
    first_cell.next();
    REQUIRE(first_cell.has_next());
    REQUIRE(first_cell.width_twips().has_value());
    CHECK_EQ(*first_cell.width_twips(), 5555U);

    CHECK_FALSE(doc.save());

    const auto xml_text = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(xml_text.c_str()));

    const auto table_node = xml_document.child("w:document").child("w:body").child("w:tbl");
    REQUIRE(table_node != pugi::xml_node{});
    const auto first_saved_row = table_node.child("w:tr");
    REQUIRE(first_saved_row != pugi::xml_node{});
    const auto second_saved_row = first_saved_row.next_sibling("w:tr");
    REQUIRE(second_saved_row != pugi::xml_node{});
    CHECK_EQ(second_saved_row.next_sibling("w:tr"), pugi::xml_node{});

    const auto first_row_widths = collect_row_cell_width_values(first_saved_row);
    REQUIRE_EQ(first_row_widths.size(), 2U);
    CHECK_EQ(first_row_widths[0], "7777");
    CHECK_EQ(first_row_widths[1], "5555");

    const auto second_row_widths = collect_row_cell_width_values(second_saved_row);
    REQUIRE_EQ(second_row_widths.size(), 3U);
    CHECK_EQ(second_row_widths[0], "1001");
    CHECK_EQ(second_row_widths[1], "1002");
    CHECK_EQ(second_row_widths[2], "1003");

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    auto reopened_table = reopened.tables();
    REQUIRE(reopened_table.has_next());
    auto reopened_row = reopened_table.rows();
    reopened_row.next();
    REQUIRE(reopened_row.has_next());
    auto reopened_cell = reopened_row.cells();
    REQUIRE(reopened_cell.has_next());
    CHECK_EQ(reopened_cell.get_text(), "keep-1");
    REQUIRE(reopened_cell.width_twips().has_value());
    CHECK_EQ(*reopened_cell.width_twips(), 1001U);
    reopened_cell.next();
    REQUIRE(reopened_cell.has_next());
    REQUIRE(reopened_cell.width_twips().has_value());
    CHECK_EQ(*reopened_cell.width_twips(), 1002U);
    reopened_cell.next();
    REQUIRE(reopened_cell.has_next());
    REQUIRE(reopened_cell.width_twips().has_value());
    CHECK_EQ(*reopened_cell.width_twips(), 1003U);

    fs::remove(target);
}
