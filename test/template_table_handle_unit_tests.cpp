#include <filesystem>
#include <string>
#include <string_view>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "basic_docx_archive_test_support.hpp"

#include <featherdoc.hpp>
TEST_CASE("template parts can resolve and mutate a table directly from a bookmark") {
    namespace fs = std::filesystem;

    const fs::path target =
        fs::current_path() / "template_part_find_table_by_bookmark.docx";
    fs::remove(target);

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:tbl>
      <w:tr>
        <w:tc><w:p><w:r><w:t>keep-00</w:t></w:r></w:p></w:tc>
        <w:tc><w:p><w:r><w:t>keep-01</w:t></w:r></w:p></w:tc>
      </w:tr>
    </w:tbl>
    <w:p>
      <w:bookmarkStart w:id="0" w:name="target_before_table"/>
      <w:bookmarkEnd w:id="0"/>
    </w:p>
    <w:tbl>
      <w:tblGrid>
        <w:gridCol w:w="2400"/>
        <w:gridCol w:w="2400"/>
      </w:tblGrid>
      <w:tr>
        <w:tc><w:p><w:r><w:t>target-00</w:t></w:r></w:p></w:tc>
        <w:tc><w:p><w:r><w:t>target-01</w:t></w:r></w:p></w:tc>
      </w:tr>
      <w:tr>
        <w:tc><w:p><w:r><w:t>target-10</w:t></w:r></w:p></w:tc>
        <w:tc>
          <w:p>
            <w:bookmarkStart w:id="1" w:name="target_inside_table"/>
            <w:r><w:t>target-11</w:t></w:r>
            <w:bookmarkEnd w:id="1"/>
          </w:p>
        </w:tc>
      </w:tr>
    </w:tbl>
  </w:body>
</w:document>
)";
    write_test_docx(target, document_xml);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    auto body_template = doc.body_template();
    REQUIRE(static_cast<bool>(body_template));

    const auto before_index =
        body_template.find_table_index_by_bookmark("target_before_table");
    REQUIRE(before_index.has_value());
    CHECK_EQ(*before_index, 1U);

    const auto inside_index =
        body_template.find_table_index_by_bookmark("target_inside_table");
    REQUIRE(inside_index.has_value());
    CHECK_EQ(*inside_index, 1U);

    auto bookmarked_table =
        body_template.find_table_by_bookmark("target_before_table");
    REQUIRE(bookmarked_table.has_value());

    auto same_table_from_inside =
        body_template.find_table_by_bookmark("target_inside_table");
    REQUIRE(same_table_from_inside.has_value());
    auto appended_row = same_table_from_inside->append_row(2U);
    REQUIRE(appended_row.has_next());

    auto target_row = bookmarked_table->rows();
    REQUIRE(target_row.has_next());
    target_row.next();
    REQUIRE(target_row.has_next());

    auto target_cell = target_row.cells();
    REQUIRE(target_cell.has_next());
    target_cell.next();
    REQUIRE(target_cell.has_next());
    CHECK(target_cell.set_text("target-11-updated"));

    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());

    auto reopened_body = reopened.body_template();
    REQUIRE(static_cast<bool>(reopened_body));

    const auto keep_table = reopened_body.inspect_table(0U);
    REQUIRE(keep_table.has_value());
    CHECK_EQ(keep_table->text, "keep-00\tkeep-01");

    const auto target_table = reopened_body.inspect_table(1U);
    REQUIRE(target_table.has_value());
    CHECK_EQ(target_table->row_count, 3U);
    CHECK_EQ(target_table->column_count, 2U);
    CHECK_NE(target_table->text.find("target-11-updated"), std::string::npos);

    CHECK_FALSE(reopened_body.find_table_by_bookmark("missing_table").has_value());

    fs::remove(target);
}

TEST_CASE("table handles can update bookmark-targeted rows and cells by index") {
    namespace fs = std::filesystem;

    const fs::path target =
        fs::current_path() / "table_indexed_edit_from_bookmark.docx";
    fs::remove(target);

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p>
      <w:bookmarkStart w:id="0" w:name="page3_target_table"/>
      <w:bookmarkEnd w:id="0"/>
    </w:p>
    <w:tbl>
      <w:tr>
        <w:tc><w:p><w:r><w:t>row0-col0</w:t></w:r></w:p></w:tc>
        <w:tc><w:p><w:r><w:t>row0-col1</w:t></w:r></w:p></w:tc>
      </w:tr>
      <w:tr>
        <w:tc><w:p><w:r><w:t>row1-col0</w:t></w:r></w:p></w:tc>
        <w:tc><w:p><w:r><w:t>row1-col1</w:t></w:r></w:p></w:tc>
      </w:tr>
    </w:tbl>
  </w:body>
</w:document>
)";
    write_test_docx(target, document_xml);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    auto body_template = doc.body_template();
    REQUIRE(static_cast<bool>(body_template));

    auto table = body_template.find_table_by_bookmark("page3_target_table");
    REQUIRE(table.has_value());

    auto first_row = table->find_row(0U);
    REQUIRE(first_row.has_value());
    auto second_cell = first_row->find_cell(1U);
    REQUIRE(second_cell.has_value());
    CHECK_EQ(second_cell->get_text(), "row0-col1");

    auto first_cell = first_row->find_cell(0U);
    REQUIRE(first_cell.has_value());
    CHECK(first_cell->merge_right());
    const auto covered_first_cell = first_row->find_cell_by_grid_column(1U);
    REQUIRE(covered_first_cell.has_value());
    CHECK_EQ(covered_first_cell->get_text(), "row0-col0");
    const auto covered_first_column_index = covered_first_cell->column_index();
    REQUIRE(covered_first_column_index.has_value());
    CHECK_EQ(*covered_first_column_index, 0U);
    CHECK_EQ(covered_first_cell->column_span(), 2U);
    CHECK_FALSE(first_row->find_cell_by_grid_column(9U).has_value());
    CHECK(table->set_cell_text_by_grid_column(0U, 1U, "header-updated"));
    CHECK_EQ(first_cell->get_text(), "header-updated");
    CHECK_FALSE(table->set_cell_text_by_grid_column(0U, 9U, "out-of-range"));

    CHECK(table->set_row_texts(1U, {"body-updated-0", "body-updated-1"}));

    auto second_row = table->find_row(1U);
    REQUIRE(second_row.has_value());
    CHECK(second_row->set_texts({"body-rewritten-0", "body-rewritten-1"}));

    CHECK_FALSE(table->find_row(9U).has_value());
    CHECK_FALSE(table->find_cell(0U, 9U).has_value());
    CHECK_FALSE(table->find_cell(9U, 0U).has_value());
    CHECK_FALSE(table->set_cell_text(9U, 0U, "out-of-range"));
    CHECK_FALSE(table->set_row_texts(0U, {"too", "many", "cells"}));

    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());

    auto reopened_body = reopened.body_template();
    REQUIRE(static_cast<bool>(reopened_body));
    const auto summary = reopened_body.inspect_table(0U);
    REQUIRE(summary.has_value());
    CHECK_EQ(summary->row_count, 2U);
    CHECK_EQ(summary->column_count, 2U);
    CHECK_EQ(summary->text,
             "header-updated\nbody-rewritten-0\tbody-rewritten-1");

    fs::remove(target);
}

TEST_CASE("table handles can batch-update bookmark-targeted row ranges and cell blocks") {
    namespace fs = std::filesystem;

    const fs::path target =
        fs::current_path() / "table_batch_edit_from_bookmark.docx";
    fs::remove(target);

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p>
      <w:bookmarkStart w:id="0" w:name="page3_target_table"/>
      <w:bookmarkEnd w:id="0"/>
    </w:p>
    <w:tbl>
      <w:tr>
        <w:tc><w:p><w:r><w:t>r0c0</w:t></w:r></w:p></w:tc>
        <w:tc><w:p><w:r><w:t>r0c1</w:t></w:r></w:p></w:tc>
        <w:tc><w:p><w:r><w:t>r0c2</w:t></w:r></w:p></w:tc>
      </w:tr>
      <w:tr>
        <w:tc><w:p><w:r><w:t>r1c0</w:t></w:r></w:p></w:tc>
        <w:tc><w:p><w:r><w:t>r1c1</w:t></w:r></w:p></w:tc>
        <w:tc><w:p><w:r><w:t>r1c2</w:t></w:r></w:p></w:tc>
      </w:tr>
      <w:tr>
        <w:tc><w:p><w:r><w:t>r2c0</w:t></w:r></w:p></w:tc>
        <w:tc><w:p><w:r><w:t>r2c1</w:t></w:r></w:p></w:tc>
        <w:tc><w:p><w:r><w:t>r2c2</w:t></w:r></w:p></w:tc>
      </w:tr>
    </w:tbl>
  </w:body>
</w:document>
)";
    write_test_docx(target, document_xml);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    auto body_template = doc.body_template();
    REQUIRE(static_cast<bool>(body_template));

    auto table = body_template.find_table_by_bookmark("page3_target_table");
    REQUIRE(table.has_value());

    CHECK(table->set_rows_texts(
        1U, {{"row1-a", "row1-b", "row1-c"}, {"row2-a", "row2-b", "row2-c"}}));
    CHECK(table->set_cell_block_texts(
        0U, 1U, {{"block-00", "block-01"}, {"block-10", "block-11"}}));

    CHECK(table->set_rows_texts(0U, {}));
    CHECK(table->set_cell_block_texts(0U, 0U, {}));

    CHECK_FALSE(table->set_rows_texts(
        2U, {{"overflow-row-a", "overflow-row-b", "overflow-row-c"},
             {"past-end-a", "past-end-b", "past-end-c"}}));
    CHECK_FALSE(table->set_rows_texts(0U, {{"too-short"}}));
    CHECK_FALSE(table->set_cell_block_texts(0U, 2U, {{"overflow-a", "overflow-b"}}));
    CHECK_FALSE(table->set_cell_block_texts(9U, 0U, {{"missing-row"}}));

    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());

    auto reopened_body = reopened.body_template();
    REQUIRE(static_cast<bool>(reopened_body));
    const auto summary = reopened_body.inspect_table(0U);
    REQUIRE(summary.has_value());
    CHECK_EQ(summary->row_count, 3U);
    CHECK_EQ(summary->column_count, 3U);
    CHECK_EQ(summary->text,
             "r0c0\tblock-00\tblock-01\nrow1-a\tblock-10\tblock-11\nrow2-a\trow2-b\trow2-c");

    fs::remove(target);
}
