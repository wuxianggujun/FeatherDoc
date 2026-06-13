#include <filesystem>
#include <string>
#include <string_view>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "basic_docx_archive_test_support.hpp"
#include "basic_document_xml_test_support.hpp"

#include <featherdoc.hpp>
TEST_CASE("tables can set and clear alignment and indent") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "table_alignment_indent.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto table = doc.append_table(1, 1);
    CHECK_FALSE(table.alignment().has_value());
    CHECK_FALSE(table.indent_twips().has_value());
    CHECK(table.set_alignment(featherdoc::table_alignment::center));
    CHECK(table.set_indent_twips(360U));
    REQUIRE(table.alignment().has_value());
    CHECK_EQ(*table.alignment(), featherdoc::table_alignment::center);
    REQUIRE(table.indent_twips().has_value());
    CHECK_EQ(*table.indent_twips(), 360U);
    CHECK_FALSE(doc.save());

    auto xml_text = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(xml_text.c_str()));

    auto table_node = xml_document.child("w:document").child("w:body").child("w:tbl");
    REQUIRE(table_node != pugi::xml_node{});
    auto alignment_node = table_node.child("w:tblPr").child("w:jc");
    REQUIRE(alignment_node != pugi::xml_node{});
    CHECK_EQ(std::string_view{alignment_node.attribute("w:val").value()}, "center");
    auto indent_node = table_node.child("w:tblPr").child("w:tblInd");
    REQUIRE(indent_node != pugi::xml_node{});
    CHECK_EQ(std::string_view{indent_node.attribute("w:w").value()}, "360");
    CHECK_EQ(std::string_view{indent_node.attribute("w:type").value()}, "dxa");

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    auto reopened_table = reopened.tables();
    REQUIRE(reopened_table.has_next());
    REQUIRE(reopened_table.alignment().has_value());
    CHECK_EQ(*reopened_table.alignment(), featherdoc::table_alignment::center);
    REQUIRE(reopened_table.indent_twips().has_value());
    CHECK_EQ(*reopened_table.indent_twips(), 360U);
    CHECK(reopened_table.clear_alignment());
    CHECK(reopened_table.clear_indent());
    CHECK_FALSE(reopened_table.alignment().has_value());
    CHECK_FALSE(reopened_table.indent_twips().has_value());
    CHECK_FALSE(reopened.save());

    xml_text = read_test_docx_entry(target, test_document_xml_entry);
    xml_document.reset();
    REQUIRE(xml_document.load_string(xml_text.c_str()));
    table_node = xml_document.child("w:document").child("w:body").child("w:tbl");
    REQUIRE(table_node != pugi::xml_node{});
    CHECK_EQ(table_node.child("w:tblPr").child("w:jc"), pugi::xml_node{});
    CHECK_EQ(table_node.child("w:tblPr").child("w:tblInd"), pugi::xml_node{});

    fs::remove(target);
}

TEST_CASE("tables can set and clear floating position") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "table_floating_position.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto table = doc.append_table(1, 1);
    CHECK_FALSE(table.position().has_value());

    auto position = featherdoc::table_position{};
    position.horizontal_reference = featherdoc::table_position_horizontal_reference::page;
    position.horizontal_offset_twips = 720;
    position.horizontal_spec = featherdoc::table_position_horizontal_spec::center;
    position.vertical_reference = featherdoc::table_position_vertical_reference::paragraph;
    position.vertical_offset_twips = -120;
    position.vertical_spec = featherdoc::table_position_vertical_spec::bottom;
    position.left_from_text_twips = 144U;
    position.right_from_text_twips = 288U;
    position.top_from_text_twips = 72U;
    position.bottom_from_text_twips = 216U;
    position.overlap = featherdoc::table_overlap::never;
    CHECK(table.set_position(position));

    REQUIRE(table.position().has_value());
    CHECK_EQ(table.position()->horizontal_reference,
             featherdoc::table_position_horizontal_reference::page);
    CHECK_EQ(table.position()->horizontal_offset_twips, 720);
    REQUIRE(table.position()->horizontal_spec.has_value());
    CHECK_EQ(*table.position()->horizontal_spec,
             featherdoc::table_position_horizontal_spec::center);
    CHECK_EQ(table.position()->vertical_reference,
             featherdoc::table_position_vertical_reference::paragraph);
    CHECK_EQ(table.position()->vertical_offset_twips, -120);
    REQUIRE(table.position()->vertical_spec.has_value());
    CHECK_EQ(*table.position()->vertical_spec,
             featherdoc::table_position_vertical_spec::bottom);
    REQUIRE(table.position()->left_from_text_twips.has_value());
    CHECK_EQ(*table.position()->left_from_text_twips, 144U);
    REQUIRE(table.position()->right_from_text_twips.has_value());
    CHECK_EQ(*table.position()->right_from_text_twips, 288U);
    REQUIRE(table.position()->top_from_text_twips.has_value());
    CHECK_EQ(*table.position()->top_from_text_twips, 72U);
    REQUIRE(table.position()->bottom_from_text_twips.has_value());
    CHECK_EQ(*table.position()->bottom_from_text_twips, 216U);
    REQUIRE(table.position()->overlap.has_value());
    CHECK_EQ(*table.position()->overlap, featherdoc::table_overlap::never);
    CHECK_FALSE(doc.save());

    auto xml_text = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(xml_text.c_str()));

    auto table_node = xml_document.child("w:document").child("w:body").child("w:tbl");
    REQUIRE(table_node != pugi::xml_node{});
    auto position_node = table_node.child("w:tblPr").child("w:tblpPr");
    REQUIRE(position_node != pugi::xml_node{});
    CHECK_EQ(std::string_view{position_node.attribute("w:horzAnchor").value()}, "page");
    CHECK_EQ(std::string_view{position_node.attribute("w:tblpX").value()}, "720");
    CHECK_EQ(std::string_view{position_node.attribute("w:tblpXSpec").value()}, "center");
    CHECK_EQ(std::string_view{position_node.attribute("w:vertAnchor").value()}, "text");
    CHECK_EQ(std::string_view{position_node.attribute("w:tblpY").value()}, "-120");
    CHECK_EQ(std::string_view{position_node.attribute("w:tblpYSpec").value()}, "bottom");
    CHECK_EQ(std::string_view{position_node.attribute("w:leftFromText").value()}, "144");
    CHECK_EQ(std::string_view{position_node.attribute("w:rightFromText").value()}, "288");
    CHECK_EQ(std::string_view{position_node.attribute("w:topFromText").value()}, "72");
    CHECK_EQ(std::string_view{position_node.attribute("w:bottomFromText").value()}, "216");
    CHECK_EQ(std::string_view{position_node.attribute("w:tblOverlap").value()}, "never");

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    auto reopened_table = reopened.tables();
    REQUIRE(reopened_table.has_next());
    REQUIRE(reopened_table.position().has_value());
    CHECK_EQ(reopened_table.position()->horizontal_reference,
             featherdoc::table_position_horizontal_reference::page);
    CHECK_EQ(reopened_table.position()->horizontal_offset_twips, 720);
    REQUIRE(reopened_table.position()->horizontal_spec.has_value());
    CHECK_EQ(*reopened_table.position()->horizontal_spec,
             featherdoc::table_position_horizontal_spec::center);
    CHECK_EQ(reopened_table.position()->vertical_reference,
             featherdoc::table_position_vertical_reference::paragraph);
    CHECK_EQ(reopened_table.position()->vertical_offset_twips, -120);
    REQUIRE(reopened_table.position()->vertical_spec.has_value());
    CHECK_EQ(*reopened_table.position()->vertical_spec,
             featherdoc::table_position_vertical_spec::bottom);
    REQUIRE(reopened_table.position()->left_from_text_twips.has_value());
    CHECK_EQ(*reopened_table.position()->left_from_text_twips, 144U);
    REQUIRE(reopened_table.position()->right_from_text_twips.has_value());
    CHECK_EQ(*reopened_table.position()->right_from_text_twips, 288U);
    REQUIRE(reopened_table.position()->top_from_text_twips.has_value());
    CHECK_EQ(*reopened_table.position()->top_from_text_twips, 72U);
    REQUIRE(reopened_table.position()->bottom_from_text_twips.has_value());
    CHECK_EQ(*reopened_table.position()->bottom_from_text_twips, 216U);
    REQUIRE(reopened_table.position()->overlap.has_value());
    CHECK_EQ(*reopened_table.position()->overlap, featherdoc::table_overlap::never);
    CHECK(reopened_table.clear_position());
    CHECK_FALSE(reopened_table.position().has_value());
    CHECK_FALSE(reopened.save());

    xml_text = read_test_docx_entry(target, test_document_xml_entry);
    xml_document.reset();
    REQUIRE(xml_document.load_string(xml_text.c_str()));
    table_node = xml_document.child("w:document").child("w:body").child("w:tbl");
    REQUIRE(table_node != pugi::xml_node{});
    CHECK_EQ(table_node.child("w:tblPr").child("w:tblpPr"), pugi::xml_node{});

    fs::remove(target);
}

TEST_CASE("tables can set and clear cell spacing") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "table_cell_spacing.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto table = doc.append_table(1, 1);
    CHECK_FALSE(table.cell_spacing_twips().has_value());
    CHECK(table.set_cell_spacing_twips(180U));
    REQUIRE(table.cell_spacing_twips().has_value());
    CHECK_EQ(*table.cell_spacing_twips(), 180U);
    CHECK_FALSE(doc.save());

    auto xml_text = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(xml_text.c_str()));

    auto table_node = xml_document.child("w:document").child("w:body").child("w:tbl");
    REQUIRE(table_node != pugi::xml_node{});
    auto spacing_node = table_node.child("w:tblPr").child("w:tblCellSpacing");
    REQUIRE(spacing_node != pugi::xml_node{});
    CHECK_EQ(std::string_view{spacing_node.attribute("w:w").value()}, "180");
    CHECK_EQ(std::string_view{spacing_node.attribute("w:type").value()}, "dxa");

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    auto reopened_table = reopened.tables();
    REQUIRE(reopened_table.has_next());
    REQUIRE(reopened_table.cell_spacing_twips().has_value());
    CHECK_EQ(*reopened_table.cell_spacing_twips(), 180U);
    CHECK(reopened_table.clear_cell_spacing());
    CHECK_FALSE(reopened_table.cell_spacing_twips().has_value());
    CHECK_FALSE(reopened.save());

    xml_text = read_test_docx_entry(target, test_document_xml_entry);
    xml_document.reset();
    REQUIRE(xml_document.load_string(xml_text.c_str()));
    table_node = xml_document.child("w:document").child("w:body").child("w:tbl");
    REQUIRE(table_node != pugi::xml_node{});
    CHECK_EQ(table_node.child("w:tblPr").child("w:tblCellSpacing"), pugi::xml_node{});

    fs::remove(target);
}

TEST_CASE("tables can set default cell margins") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "table_default_cell_margins.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto table = doc.append_table(1, 1);
    CHECK_FALSE(table.cell_margin_twips(featherdoc::cell_margin_edge::top).has_value());
    CHECK(table.set_cell_margin_twips(featherdoc::cell_margin_edge::top, 120U));
    CHECK(table.set_cell_margin_twips(featherdoc::cell_margin_edge::left, 240U));
    CHECK(table.set_cell_margin_twips(featherdoc::cell_margin_edge::bottom, 360U));
    CHECK(table.set_cell_margin_twips(featherdoc::cell_margin_edge::right, 480U));
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
    const auto margins = table_node.child("w:tblPr").child("w:tblCellMar");
    REQUIRE(margins != pugi::xml_node{});
    CHECK_EQ(std::string_view{margins.child("w:top").attribute("w:w").value()}, "120");
    CHECK_EQ(std::string_view{margins.child("w:left").attribute("w:w").value()}, "240");
    CHECK_EQ(std::string_view{margins.child("w:bottom").attribute("w:w").value()}, "360");
    CHECK_EQ(std::string_view{margins.child("w:right").attribute("w:w").value()}, "480");
    CHECK_EQ(std::string_view{margins.child("w:right").attribute("w:type").value()}, "dxa");

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    auto reopened_table = reopened.tables();
    REQUIRE(reopened_table.has_next());
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

TEST_CASE("tables can clear default cell margins") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "table_default_cell_margins_clear.docx";
    fs::remove(target);

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:tbl>
      <w:tblPr>
        <w:tblCellMar>
          <w:top w:w="120" w:type="dxa"/>
          <w:left w:w="240" w:type="dxa"/>
          <w:bottom w:w="360" w:type="dxa"/>
          <w:right w:w="480" w:type="dxa"/>
        </w:tblCellMar>
      </w:tblPr>
      <w:tblGrid>
        <w:gridCol w:w="0"/>
      </w:tblGrid>
      <w:tr>
        <w:tc>
          <w:tcPr>
            <w:tcW w:w="0" w:type="auto"/>
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
    REQUIRE(table.cell_margin_twips(featherdoc::cell_margin_edge::top).has_value());
    CHECK_EQ(*table.cell_margin_twips(featherdoc::cell_margin_edge::top), 120U);
    CHECK(table.clear_cell_margin(featherdoc::cell_margin_edge::top));
    CHECK(table.clear_cell_margin(featherdoc::cell_margin_edge::left));
    CHECK(table.clear_cell_margin(featherdoc::cell_margin_edge::bottom));
    CHECK(table.clear_cell_margin(featherdoc::cell_margin_edge::right));
    CHECK_FALSE(table.cell_margin_twips(featherdoc::cell_margin_edge::top).has_value());
    CHECK_FALSE(table.cell_margin_twips(featherdoc::cell_margin_edge::left).has_value());
    CHECK_FALSE(table.cell_margin_twips(featherdoc::cell_margin_edge::bottom).has_value());
    CHECK_FALSE(table.cell_margin_twips(featherdoc::cell_margin_edge::right).has_value());

    CHECK_FALSE(doc.save());

    const auto xml_text = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(xml_text.c_str()));

    const auto table_properties =
        xml_document.child("w:document").child("w:body").child("w:tbl").child("w:tblPr");
    REQUIRE(table_properties != pugi::xml_node{});
    CHECK_EQ(table_properties.child("w:tblCellMar"), pugi::xml_node{});

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    auto reopened_table = reopened.tables();
    REQUIRE(reopened_table.has_next());
    CHECK_FALSE(reopened_table.cell_margin_twips(featherdoc::cell_margin_edge::top).has_value());
    CHECK_FALSE(reopened_table.cell_margin_twips(featherdoc::cell_margin_edge::left).has_value());

    fs::remove(target);
}

TEST_CASE("table rows can set and clear heights") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "table_row_height.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto table = doc.append_table(1, 1);
    auto row = table.rows();
    REQUIRE(row.has_next());

    CHECK_FALSE(row.height_twips().has_value());
    CHECK_FALSE(row.height_rule().has_value());
    CHECK(row.set_height_twips(360U, featherdoc::row_height_rule::exact));
    REQUIRE(row.height_twips().has_value());
    CHECK_EQ(*row.height_twips(), 360U);
    REQUIRE(row.height_rule().has_value());
    CHECK_EQ(*row.height_rule(), featherdoc::row_height_rule::exact);
    CHECK_FALSE(doc.save());

    auto xml_text = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(xml_text.c_str()));

    auto row_node = xml_document.child("w:document").child("w:body").child("w:tbl").child("w:tr");
    REQUIRE(row_node != pugi::xml_node{});
    auto row_height = row_node.child("w:trPr").child("w:trHeight");
    REQUIRE(row_height != pugi::xml_node{});
    CHECK_EQ(std::string_view{row_height.attribute("w:val").value()}, "360");
    CHECK_EQ(std::string_view{row_height.attribute("w:hRule").value()}, "exact");

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    auto reopened_table = reopened.tables();
    REQUIRE(reopened_table.has_next());
    auto reopened_row = reopened_table.rows();
    REQUIRE(reopened_row.has_next());
    REQUIRE(reopened_row.height_twips().has_value());
    CHECK_EQ(*reopened_row.height_twips(), 360U);
    REQUIRE(reopened_row.height_rule().has_value());
    CHECK_EQ(*reopened_row.height_rule(), featherdoc::row_height_rule::exact);
    CHECK(reopened_row.clear_height());
    CHECK_FALSE(reopened_row.height_twips().has_value());
    CHECK_FALSE(reopened_row.height_rule().has_value());
    CHECK_FALSE(reopened.save());

    xml_text = read_test_docx_entry(target, test_document_xml_entry);
    xml_document.reset();
    REQUIRE(xml_document.load_string(xml_text.c_str()));
    row_node = xml_document.child("w:document").child("w:body").child("w:tbl").child("w:tr");
    REQUIRE(row_node != pugi::xml_node{});
    CHECK_EQ(row_node.child("w:trPr"), pugi::xml_node{});

    fs::remove(target);
}

TEST_CASE("table rows can set and clear repeating headers") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "table_row_repeat_header.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto table = doc.append_table(2, 1);
    auto row = table.rows();
    REQUIRE(row.has_next());

    CHECK_FALSE(row.repeats_header());
    CHECK(row.set_repeats_header());
    CHECK(row.repeats_header());
    CHECK_FALSE(doc.save());

    auto xml_text = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(xml_text.c_str()));

    auto row_node = xml_document.child("w:document").child("w:body").child("w:tbl").child("w:tr");
    REQUIRE(row_node != pugi::xml_node{});
    auto table_header = row_node.child("w:trPr").child("w:tblHeader");
    REQUIRE(table_header != pugi::xml_node{});
    CHECK_EQ(std::string_view{table_header.attribute("w:val").value()}, "1");

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    auto reopened_table = reopened.tables();
    REQUIRE(reopened_table.has_next());
    auto reopened_row = reopened_table.rows();
    REQUIRE(reopened_row.has_next());
    CHECK(reopened_row.repeats_header());
    CHECK(reopened_row.clear_repeats_header());
    CHECK_FALSE(reopened_row.repeats_header());
    CHECK_FALSE(reopened.save());

    xml_text = read_test_docx_entry(target, test_document_xml_entry);
    xml_document.reset();
    REQUIRE(xml_document.load_string(xml_text.c_str()));
    row_node = xml_document.child("w:document").child("w:body").child("w:tbl").child("w:tr");
    REQUIRE(row_node != pugi::xml_node{});
    CHECK_EQ(row_node.child("w:trPr"), pugi::xml_node{});

    fs::remove(target);
}

TEST_CASE("table rows can set and clear cant-split") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "table_row_cant_split.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto table = doc.append_table(2, 1);
    auto row = table.rows();
    REQUIRE(row.has_next());

    CHECK_FALSE(row.cant_split());
    CHECK(row.set_cant_split());
    CHECK(row.cant_split());
    CHECK_FALSE(doc.save());

    auto xml_text = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(xml_text.c_str()));

    auto row_node = xml_document.child("w:document").child("w:body").child("w:tbl").child("w:tr");
    REQUIRE(row_node != pugi::xml_node{});
    auto cant_split = row_node.child("w:trPr").child("w:cantSplit");
    REQUIRE(cant_split != pugi::xml_node{});
    CHECK_EQ(std::string_view{cant_split.attribute("w:val").value()}, "1");

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    auto reopened_table = reopened.tables();
    REQUIRE(reopened_table.has_next());
    auto reopened_row = reopened_table.rows();
    REQUIRE(reopened_row.has_next());
    CHECK(reopened_row.cant_split());
    CHECK(reopened_row.clear_cant_split());
    CHECK_FALSE(reopened_row.cant_split());
    CHECK_FALSE(reopened.save());

    xml_text = read_test_docx_entry(target, test_document_xml_entry);
    xml_document.reset();
    REQUIRE(xml_document.load_string(xml_text.c_str()));
    row_node = xml_document.child("w:document").child("w:body").child("w:tbl").child("w:tr");
    REQUIRE(row_node != pugi::xml_node{});
    CHECK_EQ(row_node.child("w:trPr"), pugi::xml_node{});

    fs::remove(target);
}

TEST_CASE("table cells can set and clear vertical alignment") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "table_cell_vertical_alignment.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto table = doc.append_table(1, 1);
    auto row = table.rows();
    REQUIRE(row.has_next());
    auto cell = row.cells();
    REQUIRE(cell.has_next());

    CHECK_FALSE(cell.vertical_alignment().has_value());
    CHECK(cell.set_vertical_alignment(featherdoc::cell_vertical_alignment::center));
    REQUIRE(cell.vertical_alignment().has_value());
    CHECK_EQ(*cell.vertical_alignment(), featherdoc::cell_vertical_alignment::center);
    CHECK_FALSE(doc.save());

    auto xml_text = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(xml_text.c_str()));

    auto cell_node =
        xml_document.child("w:document").child("w:body").child("w:tbl").child("w:tr").child("w:tc");
    REQUIRE(cell_node != pugi::xml_node{});
    auto vertical_alignment = cell_node.child("w:tcPr").child("w:vAlign");
    REQUIRE(vertical_alignment != pugi::xml_node{});
    CHECK_EQ(std::string_view{vertical_alignment.attribute("w:val").value()}, "center");

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    auto reopened_table = reopened.tables();
    REQUIRE(reopened_table.has_next());
    auto reopened_row = reopened_table.rows();
    REQUIRE(reopened_row.has_next());
    auto reopened_cell = reopened_row.cells();
    REQUIRE(reopened_cell.has_next());
    REQUIRE(reopened_cell.vertical_alignment().has_value());
    CHECK_EQ(*reopened_cell.vertical_alignment(),
             featherdoc::cell_vertical_alignment::center);
    CHECK(reopened_cell.clear_vertical_alignment());
    CHECK_FALSE(reopened_cell.vertical_alignment().has_value());
    CHECK_FALSE(reopened.save());

    xml_text = read_test_docx_entry(target, test_document_xml_entry);
    xml_document.reset();
    REQUIRE(xml_document.load_string(xml_text.c_str()));
    cell_node =
        xml_document.child("w:document").child("w:body").child("w:tbl").child("w:tr").child("w:tc");
    REQUIRE(cell_node != pugi::xml_node{});
    CHECK_EQ(cell_node.child("w:tcPr").child("w:vAlign"), pugi::xml_node{});

    fs::remove(target);
}

TEST_CASE("table cells can set and clear text direction") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "table_cell_text_direction.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto table = doc.append_table(1, 1);
    auto row = table.rows();
    REQUIRE(row.has_next());
    auto cell = row.cells();
    REQUIRE(cell.has_next());

    CHECK_FALSE(cell.text_direction().has_value());
    CHECK(cell.set_text_direction(
        featherdoc::cell_text_direction::top_to_bottom_right_to_left));
    REQUIRE(cell.text_direction().has_value());
    CHECK_EQ(*cell.text_direction(),
             featherdoc::cell_text_direction::top_to_bottom_right_to_left);
    CHECK_FALSE(doc.save());

    auto xml_text = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(xml_text.c_str()));

    auto cell_node =
        xml_document.child("w:document").child("w:body").child("w:tbl").child("w:tr").child("w:tc");
    REQUIRE(cell_node != pugi::xml_node{});
    auto text_direction = cell_node.child("w:tcPr").child("w:textDirection");
    REQUIRE(text_direction != pugi::xml_node{});
    CHECK_EQ(std::string_view{text_direction.attribute("w:val").value()}, "tbRl");

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    auto reopened_table = reopened.tables();
    REQUIRE(reopened_table.has_next());
    auto reopened_row = reopened_table.rows();
    REQUIRE(reopened_row.has_next());
    auto reopened_cell = reopened_row.cells();
    REQUIRE(reopened_cell.has_next());
    REQUIRE(reopened_cell.text_direction().has_value());
    CHECK_EQ(*reopened_cell.text_direction(),
             featherdoc::cell_text_direction::top_to_bottom_right_to_left);
    CHECK(reopened_cell.clear_text_direction());
    CHECK_FALSE(reopened_cell.text_direction().has_value());
    CHECK_FALSE(reopened.save());

    xml_text = read_test_docx_entry(target, test_document_xml_entry);
    xml_document.reset();
    REQUIRE(xml_document.load_string(xml_text.c_str()));
    cell_node =
        xml_document.child("w:document").child("w:body").child("w:tbl").child("w:tr").child("w:tc");
    REQUIRE(cell_node != pugi::xml_node{});
    CHECK_EQ(cell_node.child("w:tcPr").child("w:textDirection"), pugi::xml_node{});

    fs::remove(target);
}

TEST_CASE("table cells can set fill colors and margins") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "table_cell_fill_margins.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto table = doc.append_table(1, 1);
    auto row = table.rows();
    REQUIRE(row.has_next());
    auto cell = row.cells();
    REQUIRE(cell.has_next());

    CHECK_FALSE(cell.fill_color().has_value());
    CHECK_FALSE(cell.margin_twips(featherdoc::cell_margin_edge::top).has_value());
    CHECK(cell.set_fill_color("A1B2C3"));
    REQUIRE(cell.fill_color().has_value());
    CHECK_EQ(*cell.fill_color(), "A1B2C3");
    CHECK(cell.set_margin_twips(featherdoc::cell_margin_edge::top, 120U));
    CHECK(cell.set_margin_twips(featherdoc::cell_margin_edge::left, 240U));
    CHECK(cell.set_margin_twips(featherdoc::cell_margin_edge::bottom, 360U));
    CHECK(cell.set_margin_twips(featherdoc::cell_margin_edge::right, 480U));
    CHECK(cell.paragraphs().add_run("seed").has_next());

    CHECK_FALSE(doc.save());

    const auto xml_text = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(xml_text.c_str()));

    const auto cell_node =
        xml_document.child("w:document").child("w:body").child("w:tbl").child("w:tr").child("w:tc");
    REQUIRE(cell_node != pugi::xml_node{});

    const auto shading = cell_node.child("w:tcPr").child("w:shd");
    REQUIRE(shading != pugi::xml_node{});
    CHECK_EQ(std::string_view{shading.attribute("w:val").value()}, "clear");
    CHECK_EQ(std::string_view{shading.attribute("w:color").value()}, "auto");
    CHECK_EQ(std::string_view{shading.attribute("w:fill").value()}, "A1B2C3");

    const auto margins = cell_node.child("w:tcPr").child("w:tcMar");
    REQUIRE(margins != pugi::xml_node{});
    CHECK_EQ(std::string_view{margins.child("w:top").attribute("w:w").value()}, "120");
    CHECK_EQ(std::string_view{margins.child("w:left").attribute("w:w").value()}, "240");
    CHECK_EQ(std::string_view{margins.child("w:bottom").attribute("w:w").value()}, "360");
    CHECK_EQ(std::string_view{margins.child("w:right").attribute("w:w").value()}, "480");
    CHECK_EQ(std::string_view{margins.child("w:right").attribute("w:type").value()}, "dxa");

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    auto reopened_table = reopened.tables();
    REQUIRE(reopened_table.has_next());
    auto reopened_row = reopened_table.rows();
    REQUIRE(reopened_row.has_next());
    auto reopened_cell = reopened_row.cells();
    REQUIRE(reopened_cell.has_next());
    REQUIRE(reopened_cell.fill_color().has_value());
    CHECK_EQ(*reopened_cell.fill_color(), "A1B2C3");
    REQUIRE(reopened_cell.margin_twips(featherdoc::cell_margin_edge::top).has_value());
    CHECK_EQ(*reopened_cell.margin_twips(featherdoc::cell_margin_edge::top), 120U);
    REQUIRE(reopened_cell.margin_twips(featherdoc::cell_margin_edge::left).has_value());
    CHECK_EQ(*reopened_cell.margin_twips(featherdoc::cell_margin_edge::left), 240U);
    REQUIRE(reopened_cell.margin_twips(featherdoc::cell_margin_edge::bottom).has_value());
    CHECK_EQ(*reopened_cell.margin_twips(featherdoc::cell_margin_edge::bottom), 360U);
    REQUIRE(reopened_cell.margin_twips(featherdoc::cell_margin_edge::right).has_value());
    CHECK_EQ(*reopened_cell.margin_twips(featherdoc::cell_margin_edge::right), 480U);

    fs::remove(target);
}

TEST_CASE("table cells can clear fill colors and margins") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "table_cell_fill_margins_clear.docx";
    fs::remove(target);

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:tbl>
      <w:tblGrid>
        <w:gridCol w:w="0"/>
      </w:tblGrid>
      <w:tr>
        <w:tc>
          <w:tcPr>
            <w:tcW w:w="0" w:type="auto"/>
            <w:shd w:val="clear" w:color="auto" w:fill="ABCDEF"/>
            <w:tcMar>
              <w:top w:w="120" w:type="dxa"/>
              <w:left w:w="240" w:type="dxa"/>
              <w:bottom w:w="360" w:type="dxa"/>
              <w:right w:w="480" w:type="dxa"/>
            </w:tcMar>
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
    auto row = table.rows();
    REQUIRE(row.has_next());
    auto cell = row.cells();
    REQUIRE(cell.has_next());

    REQUIRE(cell.fill_color().has_value());
    CHECK_EQ(*cell.fill_color(), "ABCDEF");
    CHECK(cell.clear_fill_color());
    CHECK_FALSE(cell.fill_color().has_value());
    CHECK(cell.clear_margin(featherdoc::cell_margin_edge::top));
    CHECK(cell.clear_margin(featherdoc::cell_margin_edge::left));
    CHECK(cell.clear_margin(featherdoc::cell_margin_edge::bottom));
    CHECK(cell.clear_margin(featherdoc::cell_margin_edge::right));
    CHECK_FALSE(cell.margin_twips(featherdoc::cell_margin_edge::top).has_value());
    CHECK_FALSE(cell.margin_twips(featherdoc::cell_margin_edge::left).has_value());
    CHECK_FALSE(cell.margin_twips(featherdoc::cell_margin_edge::bottom).has_value());
    CHECK_FALSE(cell.margin_twips(featherdoc::cell_margin_edge::right).has_value());

    CHECK_FALSE(doc.save());

    const auto xml_text = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(xml_text.c_str()));

    const auto cell_properties =
        xml_document.child("w:document").child("w:body").child("w:tbl").child("w:tr").child("w:tc").child("w:tcPr");
    REQUIRE(cell_properties != pugi::xml_node{});
    CHECK_EQ(cell_properties.child("w:shd"), pugi::xml_node{});
    CHECK_EQ(cell_properties.child("w:tcMar"), pugi::xml_node{});

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    auto reopened_table = reopened.tables();
    REQUIRE(reopened_table.has_next());
    auto reopened_row = reopened_table.rows();
    REQUIRE(reopened_row.has_next());
    auto reopened_cell = reopened_row.cells();
    REQUIRE(reopened_cell.has_next());
    CHECK_FALSE(reopened_cell.fill_color().has_value());
    CHECK_FALSE(reopened_cell.margin_twips(featherdoc::cell_margin_edge::top).has_value());

    fs::remove(target);
}
