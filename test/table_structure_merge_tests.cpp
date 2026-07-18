#include <filesystem>
#include <fstream>
#include <iterator>
#include <limits>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "doctest.h"
#include "basic_docx_archive_test_support.hpp"
#include "basic_document_xml_test_support.hpp"

#include <featherdoc.hpp>

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

TEST_CASE("table span mutations reject oversized grid spans atomically") {
    namespace fs = std::filesystem;

    const fs::path target =
        fs::current_path() / "table_span_mutation_oversized_grid_span.docx";
    fs::remove(target);

    SUBCASE("merge right") {
        write_test_docx(
            target,
            R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body><w:tbl><w:tr>
    <w:tc><w:tcPr><w:gridSpan w:val="64"/></w:tcPr><w:p><w:r><w:t>anchor</w:t></w:r></w:p></w:tc>
    <w:tc><w:p><w:r><w:t>right</w:t></w:r></w:p></w:tc>
  </w:tr></w:tbl></w:body>
</w:document>)");

        featherdoc::Document document(target);
        REQUIRE_FALSE(document.open());
        auto cell = document.tables().rows().cells();
        REQUIRE(cell.has_next());

        CHECK_FALSE(cell.merge_right(1U));
        CHECK_EQ(collect_table_text(document), "anchor\nright\n");
        REQUIRE_FALSE(document.save());

        const auto xml_text =
            read_test_docx_entry(target, test_document_xml_entry);
        pugi::xml_document xml_document;
        REQUIRE(xml_document.load_string(xml_text.c_str()));
        const auto row = xml_document.child("w:document")
                             .child("w:body")
                             .child("w:tbl")
                             .child("w:tr");
        REQUIRE(row != pugi::xml_node{});
        CHECK_EQ(count_named_children(row, "w:tc"), 2U);
        CHECK_EQ(std::string_view{row.child("w:tc")
                                      .child("w:tcPr")
                                      .child("w:gridSpan")
                                      .attribute("w:val")
                                      .value()},
                 "64");
    }

    SUBCASE("merge down") {
        write_test_docx(
            target,
            R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body><w:tbl>
    <w:tr>
      <w:tc><w:tcPr><w:gridSpan w:val="64"/></w:tcPr><w:p><w:r><w:t>top</w:t></w:r></w:p></w:tc>
      <w:tc><w:p><w:r><w:t>top-right</w:t></w:r></w:p></w:tc>
    </w:tr>
    <w:tr>
      <w:tc><w:tcPr><w:gridSpan w:val="64"/></w:tcPr><w:p><w:r><w:t>bottom</w:t></w:r></w:p></w:tc>
      <w:tc><w:p><w:r><w:t>bottom-right</w:t></w:r></w:p></w:tc>
    </w:tr>
  </w:tbl></w:body>
</w:document>)");

        featherdoc::Document document(target);
        REQUIRE_FALSE(document.open());
        auto cell = document.tables().rows().cells();
        REQUIRE(cell.has_next());

        CHECK_FALSE(cell.merge_down(1U));
        CHECK_EQ(collect_table_text(document),
                 "top\ntop-right\nbottom\nbottom-right\n");
        REQUIRE_FALSE(document.save());

        const auto xml_text =
            read_test_docx_entry(target, test_document_xml_entry);
        pugi::xml_document xml_document;
        REQUIRE(xml_document.load_string(xml_text.c_str()));
        auto row = xml_document.child("w:document")
                       .child("w:body")
                       .child("w:tbl")
                       .child("w:tr");
        REQUIRE(row != pugi::xml_node{});
        CHECK(row.child("w:tc").child("w:tcPr").child("w:vMerge") ==
              pugi::xml_node{});
        row = row.next_sibling("w:tr");
        REQUIRE(row != pugi::xml_node{});
        CHECK(row.child("w:tc").child("w:tcPr").child("w:vMerge") ==
              pugi::xml_node{});
    }

    SUBCASE("unmerge right") {
        write_test_docx(
            target,
            R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body><w:tbl><w:tr>
    <w:tc><w:tcPr><w:gridSpan w:val="64"/></w:tcPr><w:p><w:r><w:t>anchor</w:t></w:r></w:p></w:tc>
  </w:tr></w:tbl></w:body>
</w:document>)");

        featherdoc::Document document(target);
        REQUIRE_FALSE(document.open());
        auto cell = document.tables().rows().cells();
        REQUIRE(cell.has_next());

        CHECK_FALSE(cell.unmerge_right());
        CHECK_EQ(collect_table_text(document), "anchor\n");
        REQUIRE_FALSE(document.save());

        const auto xml_text =
            read_test_docx_entry(target, test_document_xml_entry);
        pugi::xml_document xml_document;
        REQUIRE(xml_document.load_string(xml_text.c_str()));
        const auto cell_node = xml_document.child("w:document")
                                   .child("w:body")
                                   .child("w:tbl")
                                   .child("w:tr")
                                   .child("w:tc");
        CHECK_EQ(std::string_view{cell_node.child("w:tcPr")
                                      .child("w:gridSpan")
                                      .attribute("w:val")
                                      .value()},
                 "64");
    }

    SUBCASE("unmerge down") {
        write_test_docx(
            target,
            R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body><w:tbl>
    <w:tr><w:tc><w:tcPr><w:gridSpan w:val="64"/><w:vMerge w:val="restart"/></w:tcPr><w:p><w:r><w:t>top</w:t></w:r></w:p></w:tc></w:tr>
    <w:tr><w:tc><w:tcPr><w:gridSpan w:val="64"/><w:vMerge w:val="continue"/></w:tcPr><w:p><w:r><w:t>bottom</w:t></w:r></w:p></w:tc></w:tr>
  </w:tbl></w:body>
</w:document>)");

        featherdoc::Document document(target);
        REQUIRE_FALSE(document.open());
        auto cell = document.tables().rows().cells();
        REQUIRE(cell.has_next());

        CHECK_FALSE(cell.unmerge_down());
        CHECK_EQ(collect_table_text(document), "top\nbottom\n");
        REQUIRE_FALSE(document.save());

        const auto xml_text =
            read_test_docx_entry(target, test_document_xml_entry);
        pugi::xml_document xml_document;
        REQUIRE(xml_document.load_string(xml_text.c_str()));
        auto row = xml_document.child("w:document")
                       .child("w:body")
                       .child("w:tbl")
                       .child("w:tr");
        REQUIRE(row != pugi::xml_node{});
        CHECK_EQ(std::string_view{row.child("w:tc")
                                      .child("w:tcPr")
                                      .child("w:vMerge")
                                      .attribute("w:val")
                                      .value()},
                 "restart");
        row = row.next_sibling("w:tr");
        REQUIRE(row != pugi::xml_node{});
        CHECK_EQ(std::string_view{row.child("w:tc")
                                      .child("w:tcPr")
                                      .child("w:vMerge")
                                      .attribute("w:val")
                                      .value()},
                 "continue");
    }

    fs::remove(target);
}

TEST_CASE("table merges reject extreme requested ranges without allocation failures") {
    namespace fs = std::filesystem;

    const fs::path target =
        fs::current_path() / "table_merge_extreme_requested_range.docx";
    fs::remove(target);

    featherdoc::Document document(target);
    REQUIRE_FALSE(document.create_empty());
    auto table = document.append_table(3U, 2U);
    REQUIRE(table.has_next());
    auto anchor = table.rows().cells();
    REQUIRE(anchor.has_next());

    const auto extreme_count = std::numeric_limits<std::size_t>::max();
    auto merge_right_result = true;
    CHECK_NOTHROW(merge_right_result = anchor.merge_right(extreme_count));
    CHECK_FALSE(merge_right_result);
    auto merge_down_result = true;
    CHECK_NOTHROW(merge_down_result = anchor.merge_down(extreme_count));
    CHECK_FALSE(merge_down_result);

    REQUIRE_FALSE(document.save());
    const auto xml_text = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(xml_text.c_str()));
    const auto table_node =
        xml_document.child("w:document").child("w:body").child("w:tbl");
    REQUIRE(table_node != pugi::xml_node{});
    CHECK_EQ(count_named_children(table_node, "w:tr"), 3U);
    for (auto row = table_node.child("w:tr"); row != pugi::xml_node{};
         row = row.next_sibling("w:tr")) {
        CHECK_EQ(count_named_children(row, "w:tc"), 2U);
        CHECK(row.child("w:tc").child("w:tcPr").child("w:gridSpan") ==
              pugi::xml_node{});
        CHECK(row.child("w:tc").child("w:tcPr").child("w:vMerge") ==
              pugi::xml_node{});
    }

    fs::remove(target);
}

TEST_CASE("table merge boundaries through 63 columns remain supported") {
    namespace fs = std::filesystem;

    const fs::path target =
        fs::current_path() / "table_merge_63_column_boundary.docx";
    fs::remove(target);

    featherdoc::Document document(target);
    REQUIRE_FALSE(document.create_empty());
    auto table = document.append_table(1U, 63U);
    REQUIRE(table.has_next());
    auto cell = table.rows().cells();
    REQUIRE(cell.has_next());
    CHECK(cell.merge_right(62U));
    CHECK_EQ(cell.column_span(), 63U);

    auto remaining_cells = std::size_t{0U};
    for (auto cursor = table.rows().cells(); cursor.has_next(); cursor.next()) {
        ++remaining_cells;
    }
    CHECK_EQ(remaining_cells, 1U);

    CHECK(cell.unmerge_right());
    CHECK_EQ(cell.column_span(), 1U);
    auto restored_cells = std::size_t{0U};
    for (auto cursor = table.rows().cells(); cursor.has_next(); cursor.next()) {
        ++restored_cells;
    }
    CHECK_EQ(restored_cells, 63U);

    fs::remove(target);
}

TEST_CASE("table horizontal merge retires only the removed cell subtrees") {
    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());

    auto table = document.append_table(1U, 4U);
    REQUIRE(table.valid());
    auto row = table.rows();
    REQUIRE(row.valid());

    auto anchor = row.cells();
    REQUIRE(anchor.set_text("anchor"));

    auto removed = anchor;
    removed.next();
    REQUIRE(removed.valid());
    REQUIRE(removed.set_text("removed"));
    auto removed_paragraph = removed.paragraphs();
    auto removed_run = removed_paragraph.runs();
    REQUIRE(removed_paragraph.valid());
    REQUIRE(removed_run.valid());

    auto unaffected = removed;
    unaffected.next();
    REQUIRE(unaffected.valid());
    REQUIRE(unaffected.set_text("unaffected"));
    auto unaffected_paragraph = unaffected.paragraphs();
    auto unaffected_run = unaffected_paragraph.runs();
    REQUIRE(unaffected_paragraph.valid());
    REQUIRE(unaffected_run.valid());

    CHECK(anchor.merge_right());

    CHECK_FALSE(removed.valid());
    CHECK_FALSE(removed_paragraph.valid());
    CHECK_FALSE(removed_run.valid());
    CHECK(anchor.valid());
    CHECK(row.valid());
    CHECK(table.valid());
    CHECK(unaffected.valid());
    CHECK(unaffected_paragraph.valid());
    CHECK(unaffected_run.valid());
    CHECK_EQ(unaffected.get_text(), "unaffected");
}

TEST_CASE("table vertical merge retires replaced cell contents only") {
    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());

    auto table = document.append_table(2U, 2U);
    REQUIRE(table.valid());
    auto first_row = table.rows();
    REQUIRE(first_row.valid());
    auto second_row = first_row;
    second_row.next();
    REQUIRE(second_row.valid());

    auto anchor = first_row.cells();
    REQUIRE(anchor.set_text("anchor"));
    auto anchor_paragraph = anchor.paragraphs();
    auto anchor_run = anchor_paragraph.runs();
    REQUIRE(anchor_paragraph.valid());
    REQUIRE(anchor_run.valid());

    auto replaced_cell = second_row.cells();
    REQUIRE(replaced_cell.set_text("replace me"));
    auto replaced_paragraph = replaced_cell.paragraphs();
    auto replaced_run = replaced_paragraph.runs();
    REQUIRE(replaced_paragraph.valid());
    REQUIRE(replaced_run.valid());

    auto unaffected_cell = replaced_cell;
    unaffected_cell.next();
    REQUIRE(unaffected_cell.valid());
    REQUIRE(unaffected_cell.set_text("unaffected"));
    auto unaffected_paragraph = unaffected_cell.paragraphs();
    auto unaffected_run = unaffected_paragraph.runs();
    REQUIRE(unaffected_paragraph.valid());
    REQUIRE(unaffected_run.valid());

    CHECK(anchor.merge_down());

    CHECK(replaced_cell.valid());
    CHECK_FALSE(replaced_paragraph.valid());
    CHECK_FALSE(replaced_run.valid());
    CHECK_EQ(replaced_cell.get_text(), "");
    CHECK(replaced_cell.paragraphs().valid());
    CHECK(anchor.valid());
    CHECK(anchor_paragraph.valid());
    CHECK(anchor_run.valid());
    CHECK(first_row.valid());
    CHECK(second_row.valid());
    CHECK(table.valid());
    CHECK(unaffected_cell.valid());
    CHECK(unaffected_paragraph.valid());
    CHECK(unaffected_run.valid());
    CHECK_EQ(unaffected_cell.get_text(), "unaffected");
}
