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
