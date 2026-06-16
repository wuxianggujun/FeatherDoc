#include <filesystem>
#include <fstream>
#include <iterator>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "basic_docx_archive_test_support.hpp"
#include "basic_document_xml_test_support.hpp"

#include <featherdoc.hpp>

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
