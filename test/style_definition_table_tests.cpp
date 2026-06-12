#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

#include "doctest.h"
#include "basic_docx_archive_test_support.hpp"
#include "basic_document_xml_test_support.hpp"

#include <featherdoc.hpp>

TEST_CASE("ensure table style definition writes whole-table and conditional properties") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "ensure_table_style_properties.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto whole_margins = featherdoc::table_style_margins_definition{};
    whole_margins.top_twips = 80U;
    whole_margins.left_twips = 120U;
    whole_margins.bottom_twips = 80U;
    whole_margins.right_twips = 120U;

    auto whole_borders = featherdoc::table_style_borders_definition{};
    whole_borders.top = featherdoc::border_definition{
        featherdoc::border_style::single, 12U, "4472C4", 0U};
    whole_borders.inside_horizontal = featherdoc::border_definition{
        featherdoc::border_style::dotted, 4U, "A5A5A5", 0U};
    whole_borders.inside_vertical = featherdoc::border_definition{
        featherdoc::border_style::dashed, 4U, "A5A5A5", 0U};

    auto whole_table = featherdoc::table_style_region_definition{};
    whole_table.fill_color = std::string{"DDEEFF"};
    whole_table.text_color = std::string{"1F1F1F"};
    whole_table.bold = false;
    whole_table.italic = false;
    whole_table.font_size_points = 11U;
    whole_table.font_family = std::string{"Aptos"};
    whole_table.east_asia_font_family = std::string{"Microsoft YaHei"};
    whole_table.cell_vertical_alignment = featherdoc::cell_vertical_alignment::center;
    whole_table.cell_text_direction =
        featherdoc::cell_text_direction::left_to_right_top_to_bottom;
    whole_table.paragraph_alignment = featherdoc::paragraph_alignment::center;
    auto whole_paragraph_spacing = featherdoc::table_style_paragraph_spacing_definition{};
    whole_paragraph_spacing.before_twips = 120U;
    whole_paragraph_spacing.after_twips = 80U;
    whole_paragraph_spacing.line_twips = 360U;
    whole_paragraph_spacing.line_rule =
        featherdoc::paragraph_line_spacing_rule::exact;
    whole_table.paragraph_spacing = whole_paragraph_spacing;
    whole_table.cell_margins = whole_margins;
    whole_table.borders = whole_borders;

    auto first_row_margins = featherdoc::table_style_margins_definition{};
    first_row_margins.top_twips = 60U;
    first_row_margins.bottom_twips = 60U;

    auto first_row_borders = featherdoc::table_style_borders_definition{};
    first_row_borders.bottom = featherdoc::border_definition{
        featherdoc::border_style::double_line, 8U, "1F4E79", 0U};

    auto first_row = featherdoc::table_style_region_definition{};
    first_row.fill_color = std::string{"1F4E79"};
    first_row.text_color = std::string{"FFFFFF"};
    first_row.bold = true;
    first_row.italic = true;
    first_row.font_size_points = 14U;
    first_row.font_family = std::string{"Aptos Display"};
    first_row.east_asia_font_family = std::string{"SimHei"};
    first_row.cell_vertical_alignment = featherdoc::cell_vertical_alignment::bottom;
    first_row.cell_text_direction =
        featherdoc::cell_text_direction::top_to_bottom_right_to_left;
    first_row.paragraph_alignment = featherdoc::paragraph_alignment::right;
    auto first_row_paragraph_spacing =
        featherdoc::table_style_paragraph_spacing_definition{};
    first_row_paragraph_spacing.after_twips = 120U;
    first_row_paragraph_spacing.line_twips = 240U;
    first_row_paragraph_spacing.line_rule =
        featherdoc::paragraph_line_spacing_rule::at_least;
    first_row.paragraph_spacing = first_row_paragraph_spacing;
    first_row.cell_margins = first_row_margins;
    first_row.borders = first_row_borders;

    auto second_banded_rows = featherdoc::table_style_region_definition{};
    second_banded_rows.fill_color = std::string{"F2F2F2"};
    second_banded_rows.text_color = std::string{"666666"};

    auto second_banded_columns = featherdoc::table_style_region_definition{};
    second_banded_columns.fill_color = std::string{"E2F0D9"};
    second_banded_columns.cell_vertical_alignment =
        featherdoc::cell_vertical_alignment::top;

    auto definition = featherdoc::table_style_definition{};
    definition.name = "Invoice Grid";
    definition.based_on = std::string{"TableGrid"};
    definition.is_quick_format = true;
    definition.whole_table = whole_table;
    definition.first_row = first_row;
    definition.second_banded_rows = second_banded_rows;
    definition.second_banded_columns = second_banded_columns;

    CHECK(doc.ensure_table_style("InvoiceGrid", definition));
    CHECK_FALSE(doc.save());

    const auto saved_styles_xml = read_test_docx_entry(target, "word/styles.xml");
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(saved_styles_xml.c_str()));
    const auto style = find_style_xml_node(xml_document.child("w:styles"), "InvoiceGrid");
    REQUIRE(style != pugi::xml_node{});

    const auto table_properties = style.child("w:tblPr");
    REQUIRE(table_properties != pugi::xml_node{});
    CHECK_EQ(std::string_view{table_properties.child("w:tblCellMar")
                                  .child("w:left")
                                  .attribute("w:w")
                                  .value()},
             "120");
    CHECK_EQ(std::string_view{table_properties.child("w:tblBorders")
                                  .child("w:top")
                                  .attribute("w:color")
                                  .value()},
             "4472C4");
    CHECK_EQ(std::string_view{table_properties.child("w:tblBorders")
                                  .child("w:insideH")
                                  .attribute("w:val")
                                  .value()},
             "dotted");

    const auto whole_cell_properties = style.child("w:tcPr");
    REQUIRE(whole_cell_properties != pugi::xml_node{});
    CHECK_EQ(std::string_view{whole_cell_properties.child("w:shd")
                                  .attribute("w:fill")
                                  .value()},
             "DDEEFF");
    CHECK_EQ(std::string_view{whole_cell_properties.child("w:vAlign")
                                  .attribute("w:val")
                                  .value()},
             "center");
    CHECK_EQ(std::string_view{whole_cell_properties.child("w:textDirection")
                                  .attribute("w:val")
                                  .value()},
             "lrTb");
    CHECK_EQ(std::string_view{style.child("w:pPr")
                                  .child("w:jc")
                                  .attribute("w:val")
                                  .value()},
             "center");
    const auto whole_paragraph_spacing_node =
        style.child("w:pPr").child("w:spacing");
    REQUIRE(whole_paragraph_spacing_node != pugi::xml_node{});
    CHECK_EQ(std::string_view{whole_paragraph_spacing_node.attribute("w:before")
                                  .value()},
             "120");
    CHECK_EQ(std::string_view{whole_paragraph_spacing_node.attribute("w:after")
                                  .value()},
             "80");
    CHECK_EQ(std::string_view{whole_paragraph_spacing_node.attribute("w:line")
                                  .value()},
             "360");
    CHECK_EQ(std::string_view{whole_paragraph_spacing_node.attribute("w:lineRule")
                                  .value()},
             "exact");
    CHECK_EQ(std::string_view{style.child("w:rPr")
                                  .child("w:color")
                                  .attribute("w:val")
                                  .value()},
             "1F1F1F");
    CHECK_EQ(std::string_view{style.child("w:rPr")
                                  .child("w:b")
                                  .attribute("w:val")
                                  .value()},
             "0");
    CHECK_EQ(std::string_view{style.child("w:rPr")
                                  .child("w:i")
                                  .attribute("w:val")
                                  .value()},
             "0");
    CHECK_EQ(std::string_view{style.child("w:rPr")
                                  .child("w:sz")
                                  .attribute("w:val")
                                  .value()},
             "22");
    CHECK_EQ(std::string_view{style.child("w:rPr")
                                  .child("w:szCs")
                                  .attribute("w:val")
                                  .value()},
             "22");
    CHECK_EQ(std::string_view{style.child("w:rPr")
                                  .child("w:rFonts")
                                  .attribute("w:ascii")
                                  .value()},
             "Aptos");
    CHECK_EQ(std::string_view{style.child("w:rPr")
                                  .child("w:rFonts")
                                  .attribute("w:eastAsia")
                                  .value()},
             "Microsoft YaHei");

    const auto first_row_region = find_table_style_region_xml_node(style, "firstRow");
    REQUIRE(first_row_region != pugi::xml_node{});
    const auto first_row_cell_properties = first_row_region.child("w:tcPr");
    REQUIRE(first_row_cell_properties != pugi::xml_node{});
    CHECK_EQ(std::string_view{first_row_cell_properties.child("w:shd")
                                  .attribute("w:fill")
                                  .value()},
             "1F4E79");
    CHECK_EQ(std::string_view{first_row_cell_properties.child("w:vAlign")
                                  .attribute("w:val")
                                  .value()},
             "bottom");
    CHECK_EQ(std::string_view{first_row_cell_properties.child("w:textDirection")
                                  .attribute("w:val")
                                  .value()},
             "tbRl");
    CHECK_EQ(std::string_view{first_row_region.child("w:pPr")
                                  .child("w:jc")
                                  .attribute("w:val")
                                  .value()},
             "right");
    const auto first_row_paragraph_spacing_node =
        first_row_region.child("w:pPr").child("w:spacing");
    REQUIRE(first_row_paragraph_spacing_node != pugi::xml_node{});
    CHECK_EQ(std::string_view{first_row_paragraph_spacing_node.attribute("w:after")
                                  .value()},
             "120");
    CHECK_EQ(std::string_view{first_row_paragraph_spacing_node.attribute("w:line")
                                  .value()},
             "240");
    CHECK_EQ(std::string_view{first_row_paragraph_spacing_node.attribute("w:lineRule")
                                  .value()},
             "atLeast");
    CHECK_EQ(std::string_view{first_row_region.child("w:rPr")
                                  .child("w:color")
                                  .attribute("w:val")
                                  .value()},
             "FFFFFF");
    CHECK_EQ(std::string_view{first_row_region.child("w:rPr")
                                  .child("w:b")
                                  .attribute("w:val")
                                  .value()},
             "1");
    CHECK_EQ(std::string_view{first_row_region.child("w:rPr")
                                  .child("w:i")
                                  .attribute("w:val")
                                  .value()},
             "1");
    CHECK_EQ(std::string_view{first_row_region.child("w:rPr")
                                  .child("w:sz")
                                  .attribute("w:val")
                                  .value()},
             "28");
    CHECK_EQ(std::string_view{first_row_region.child("w:rPr")
                                  .child("w:szCs")
                                  .attribute("w:val")
                                  .value()},
             "28");
    CHECK_EQ(std::string_view{first_row_region.child("w:rPr")
                                  .child("w:rFonts")
                                  .attribute("w:ascii")
                                  .value()},
             "Aptos Display");
    CHECK_EQ(std::string_view{first_row_region.child("w:rPr")
                                  .child("w:rFonts")
                                  .attribute("w:eastAsia")
                                  .value()},
             "SimHei");
    CHECK_EQ(std::string_view{first_row_cell_properties.child("w:tcMar")
                                  .child("w:bottom")
                                  .attribute("w:w")
                                  .value()},
             "60");
    CHECK_EQ(std::string_view{first_row_cell_properties.child("w:tcBorders")
                                  .child("w:bottom")
                                  .attribute("w:val")
                                  .value()},
             "double");

    const auto second_banded_rows_region =
        find_table_style_region_xml_node(style, "band2Horz");
    REQUIRE(second_banded_rows_region != pugi::xml_node{});
    CHECK_EQ(std::string_view{second_banded_rows_region.child("w:tcPr")
                                  .child("w:shd")
                                  .attribute("w:fill")
                                  .value()},
             "F2F2F2");
    CHECK_EQ(std::string_view{second_banded_rows_region.child("w:rPr")
                                  .child("w:color")
                                  .attribute("w:val")
                                  .value()},
             "666666");

    const auto second_banded_columns_region =
        find_table_style_region_xml_node(style, "band2Vert");
    REQUIRE(second_banded_columns_region != pugi::xml_node{});
    CHECK_EQ(std::string_view{second_banded_columns_region.child("w:tcPr")
                                  .child("w:shd")
                                  .attribute("w:fill")
                                  .value()},
             "E2F0D9");
    CHECK_EQ(std::string_view{second_banded_columns_region.child("w:tcPr")
                                  .child("w:vAlign")
                                  .attribute("w:val")
                                  .value()},
             "top");

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    REQUIRE(reopened.find_style("InvoiceGrid").has_value());
    const auto inspected = reopened.find_table_style_definition("InvoiceGrid");
    REQUIRE(inspected.has_value());
    CHECK_EQ(inspected->style.style_id, "InvoiceGrid");
    REQUIRE(inspected->whole_table.has_value());
    CHECK_EQ(inspected->whole_table->fill_color, std::optional<std::string>{"DDEEFF"});
    CHECK_EQ(inspected->whole_table->text_color, std::optional<std::string>{"1F1F1F"});
    CHECK_EQ(inspected->whole_table->bold, std::optional<bool>{false});
    CHECK_EQ(inspected->whole_table->italic, std::optional<bool>{false});
    CHECK_EQ(inspected->whole_table->font_size_points,
             std::optional<std::uint32_t>{11U});
    CHECK_EQ(inspected->whole_table->font_family,
             std::optional<std::string>{"Aptos"});
    CHECK_EQ(inspected->whole_table->east_asia_font_family,
             std::optional<std::string>{"Microsoft YaHei"});
    CHECK_EQ(inspected->whole_table->cell_vertical_alignment,
             std::optional<featherdoc::cell_vertical_alignment>{
                 featherdoc::cell_vertical_alignment::center});
    CHECK_EQ(inspected->whole_table->cell_text_direction,
             std::optional<featherdoc::cell_text_direction>{
                 featherdoc::cell_text_direction::left_to_right_top_to_bottom});
    CHECK_EQ(inspected->whole_table->paragraph_alignment,
             std::optional<featherdoc::paragraph_alignment>{
                 featherdoc::paragraph_alignment::center});
    REQUIRE(inspected->whole_table->paragraph_spacing.has_value());
    CHECK_EQ(inspected->whole_table->paragraph_spacing->before_twips,
             std::optional<std::uint32_t>{120U});
    CHECK_EQ(inspected->whole_table->paragraph_spacing->after_twips,
             std::optional<std::uint32_t>{80U});
    CHECK_EQ(inspected->whole_table->paragraph_spacing->line_twips,
             std::optional<std::uint32_t>{360U});
    CHECK_EQ(inspected->whole_table->paragraph_spacing->line_rule,
             std::optional<featherdoc::paragraph_line_spacing_rule>{
                 featherdoc::paragraph_line_spacing_rule::exact});
    REQUIRE(inspected->whole_table->cell_margins.has_value());
    CHECK_EQ(inspected->whole_table->cell_margins->left_twips,
             std::optional<std::uint32_t>{120U});
    REQUIRE(inspected->whole_table->borders.has_value());
    REQUIRE(inspected->whole_table->borders->top.has_value());
    CHECK_EQ(inspected->whole_table->borders->top->style,
             std::optional<std::string>{"single"});
    CHECK_EQ(inspected->whole_table->borders->top->color,
             std::optional<std::string>{"4472C4"});
    REQUIRE(inspected->first_row.has_value());
    CHECK_EQ(inspected->first_row->fill_color, std::optional<std::string>{"1F4E79"});
    CHECK_EQ(inspected->first_row->text_color, std::optional<std::string>{"FFFFFF"});
    CHECK_EQ(inspected->first_row->bold, std::optional<bool>{true});
    CHECK_EQ(inspected->first_row->italic, std::optional<bool>{true});
    CHECK_EQ(inspected->first_row->font_size_points,
             std::optional<std::uint32_t>{14U});
    CHECK_EQ(inspected->first_row->font_family,
             std::optional<std::string>{"Aptos Display"});
    CHECK_EQ(inspected->first_row->east_asia_font_family,
             std::optional<std::string>{"SimHei"});
    CHECK_EQ(inspected->first_row->cell_vertical_alignment,
             std::optional<featherdoc::cell_vertical_alignment>{
                 featherdoc::cell_vertical_alignment::bottom});
    CHECK_EQ(inspected->first_row->cell_text_direction,
             std::optional<featherdoc::cell_text_direction>{
                 featherdoc::cell_text_direction::top_to_bottom_right_to_left});
    CHECK_EQ(inspected->first_row->paragraph_alignment,
             std::optional<featherdoc::paragraph_alignment>{
                 featherdoc::paragraph_alignment::right});
    REQUIRE(inspected->first_row->paragraph_spacing.has_value());
    CHECK_EQ(inspected->first_row->paragraph_spacing->after_twips,
             std::optional<std::uint32_t>{120U});
    CHECK_EQ(inspected->first_row->paragraph_spacing->line_twips,
             std::optional<std::uint32_t>{240U});
    CHECK_EQ(inspected->first_row->paragraph_spacing->line_rule,
             std::optional<featherdoc::paragraph_line_spacing_rule>{
                 featherdoc::paragraph_line_spacing_rule::at_least});
    REQUIRE(inspected->first_row->borders.has_value());
    REQUIRE(inspected->first_row->borders->bottom.has_value());
    CHECK_EQ(inspected->first_row->borders->bottom->style,
             std::optional<std::string>{"double"});
    REQUIRE(inspected->second_banded_rows.has_value());
    CHECK_EQ(inspected->second_banded_rows->fill_color,
             std::optional<std::string>{"F2F2F2"});
    CHECK_EQ(inspected->second_banded_rows->text_color,
             std::optional<std::string>{"666666"});
    REQUIRE(inspected->second_banded_columns.has_value());
    CHECK_EQ(inspected->second_banded_columns->fill_color,
             std::optional<std::string>{"E2F0D9"});
    CHECK_EQ(inspected->second_banded_columns->cell_vertical_alignment,
             std::optional<featherdoc::cell_vertical_alignment>{
                 featherdoc::cell_vertical_alignment::top});

    fs::remove(target);
}
