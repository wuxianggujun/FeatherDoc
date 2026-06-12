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
