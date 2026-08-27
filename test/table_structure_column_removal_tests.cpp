#include <array>
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

TEST_CASE("table column removal rejects invalid geometry atomically") {
    namespace fs = std::filesystem;

    struct geometry_case {
        std::string_view name;
        std::string_view cells_xml;
    };
    constexpr auto geometry_cases = std::array{
        geometry_case{
            "single_span_64",
            R"(<w:tc><w:tcPr><w:gridSpan w:val="64"/></w:tcPr><w:p><w:r><w:t>original</w:t></w:r></w:p></w:tc>)"},
        geometry_case{
            "summed_span_64",
            R"(<w:tc><w:tcPr><w:gridSpan w:val="32"/></w:tcPr><w:p><w:r><w:t>left</w:t></w:r></w:p></w:tc><w:tc><w:tcPr><w:gridSpan w:val="32"/></w:tcPr><w:p><w:r><w:t>right</w:t></w:r></w:p></w:tc>)"},
        geometry_case{
            "zero_span",
            R"(<w:tc><w:tcPr><w:gridSpan w:val="0"/></w:tcPr><w:p><w:r><w:t>original</w:t></w:r></w:p></w:tc>)"},
        geometry_case{
            "missing_span_value",
            R"(<w:tc><w:tcPr><w:gridSpan/></w:tcPr><w:p><w:r><w:t>original</w:t></w:r></w:p></w:tc>)"},
        geometry_case{
            "invalid_span_value",
            R"(<w:tc><w:tcPr><w:gridSpan w:val="invalid"/></w:tcPr><w:p><w:r><w:t>original</w:t></w:r></w:p></w:tc>)"},
    };

    for (const auto &geometry : geometry_cases) {
        const auto target = fs::current_path() /
                            ("table_column_remove_invalid_" +
                             std::string{geometry.name} + ".docx");
        fs::remove(target);
        write_test_docx(
            target,
            std::string{
                R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:body><w:tbl><w:tr>)"} +
                std::string{geometry.cells_xml} +
                R"(</w:tr></w:tbl></w:body></w:document>)");

        featherdoc::Document document(target);
        REQUIRE_FALSE(document.open());
        REQUIRE_FALSE(document.save());
        const auto xml_before =
            read_test_docx_entry(target, test_document_xml_entry);

        auto table = document.tables();
        REQUIRE(table.valid());
        auto row = table.rows();
        REQUIRE(row.valid());
        auto cell = row.cells();
        REQUIRE(cell.valid());
        auto old_cell = cell;
        auto paragraph = cell.paragraphs();
        auto run = paragraph.runs();
        REQUIRE(paragraph.valid());
        REQUIRE(run.valid());

        CHECK_FALSE(cell.remove());
        CHECK(cell.valid());
        CHECK(old_cell.valid());
        CHECK(paragraph.valid());
        CHECK(run.valid());
        CHECK(row.valid());
        CHECK(table.valid());

        REQUIRE_FALSE(document.save());
        CHECK_EQ(read_test_docx_entry(target, test_document_xml_entry),
                 xml_before);

        fs::remove(target);
    }
}

TEST_CASE("table column removal retires every removed cell subtree only") {
    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());

    auto table = document.append_table(3U, 3U);
    REQUIRE(table.valid());
    auto first_row = table.rows();
    auto second_row = first_row;
    second_row.next();
    auto third_row = second_row;
    third_row.next();
    REQUIRE(first_row.valid());
    REQUIRE(second_row.valid());
    REQUIRE(third_row.valid());

    auto first_unchanged = first_row.cells();
    REQUIRE(first_unchanged.set_text("first unchanged"));
    auto first_removed = first_unchanged;
    first_removed.next();
    REQUIRE(first_removed.set_text("first removed"));
    auto first_removed_paragraph = first_removed.paragraphs();
    auto first_removed_run = first_removed_paragraph.runs();
    auto removal_cursor = first_removed;

    auto second_unchanged = second_row.cells();
    REQUIRE(second_unchanged.set_text("second unchanged"));
    auto second_removed = second_unchanged;
    second_removed.next();
    REQUIRE(second_removed.set_text("second removed"));
    auto second_removed_paragraph = second_removed.paragraphs();
    auto second_removed_run = second_removed_paragraph.runs();

    auto third_unchanged = third_row.cells();
    REQUIRE(third_unchanged.set_text("third unchanged"));
    auto third_removed = third_unchanged;
    third_removed.next();
    REQUIRE(third_removed.set_text("third removed"));
    auto third_removed_paragraph = third_removed.paragraphs();
    auto third_removed_run = third_removed_paragraph.runs();

    REQUIRE(removal_cursor.remove());

    CHECK(removal_cursor.valid());
    CHECK_EQ(removal_cursor.get_text(), "");
    CHECK_FALSE(first_removed.valid());
    CHECK_FALSE(first_removed_paragraph.valid());
    CHECK_FALSE(first_removed_run.valid());
    CHECK_FALSE(second_removed.valid());
    CHECK_FALSE(second_removed_paragraph.valid());
    CHECK_FALSE(second_removed_run.valid());
    CHECK_FALSE(third_removed.valid());
    CHECK_FALSE(third_removed_paragraph.valid());
    CHECK_FALSE(third_removed_run.valid());
    CHECK(first_unchanged.valid());
    CHECK_EQ(first_unchanged.get_text(), "first unchanged");
    CHECK(second_unchanged.valid());
    CHECK_EQ(second_unchanged.get_text(), "second unchanged");
    CHECK(third_unchanged.valid());
    CHECK_EQ(third_unchanged.get_text(), "third unchanged");
    CHECK(first_row.valid());
    CHECK(second_row.valid());
    CHECK(third_row.valid());
    CHECK(table.valid());
}
