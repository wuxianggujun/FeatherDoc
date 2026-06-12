#include "cli_table_test_support.hpp"

namespace {
TEST_CASE("cli table column commands insert and remove body columns") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_table_column_source.docx";
    const fs::path inserted_before =
        working_directory / "cli_table_column_before.docx";
    const fs::path inserted_after =
        working_directory / "cli_table_column_after.docx";
    const fs::path removed =
        working_directory / "cli_table_column_removed.docx";
    const fs::path before_output =
        working_directory / "cli_table_column_before_output.json";
    const fs::path after_output =
        working_directory / "cli_table_column_after_output.json";
    const fs::path remove_output =
        working_directory / "cli_table_column_remove_output.json";

    remove_if_exists(source);
    remove_if_exists(inserted_before);
    remove_if_exists(inserted_after);
    remove_if_exists(removed);
    remove_if_exists(before_output);
    remove_if_exists(after_output);
    remove_if_exists(remove_output);

    create_cli_table_inspection_fixture(source);

    CHECK_EQ(run_cli({"insert-table-column-before",
                      source.string(),
                      "0",
                      "0",
                      "1",
                      "--output",
                      inserted_before.string(),
                      "--json"},
                     before_output),
             0);
    CHECK_EQ(
        read_text_file(before_output),
        std::string{
            "{\"command\":\"insert-table-column-before\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"table_index\":0,\"row_index\":0,\"cell_index\":1,"
            "\"inserted_cell_index\":1,\"inserted_column_index\":1}\n"});

    featherdoc::Document reopened_before(inserted_before);
    REQUIRE_FALSE(reopened_before.open());
    const auto before_table = reopened_before.inspect_table(0U);
    REQUIRE(before_table.has_value());
    CHECK_EQ(before_table->row_count, 2U);
    CHECK_EQ(before_table->column_count, 3U);
    REQUIRE(before_table->column_widths.size() >= 3U);
    REQUIRE(before_table->column_widths[0].has_value());
    REQUIRE(before_table->column_widths[1].has_value());
    REQUIRE(before_table->column_widths[2].has_value());
    CHECK_EQ(*before_table->column_widths[0], 1800U);
    CHECK_EQ(*before_table->column_widths[1], 5400U);
    CHECK_EQ(*before_table->column_widths[2], 5400U);
    const auto before_inserted_header =
        reopened_before.inspect_table_cell(0U, 0U, 1U);
    REQUIRE(before_inserted_header.has_value());
    CHECK_EQ(before_inserted_header->text, "");
    const auto before_shifted_header =
        reopened_before.inspect_table_cell(0U, 0U, 2U);
    REQUIRE(before_shifted_header.has_value());
    CHECK_EQ(before_shifted_header->text, "r0c1");
    const auto before_inserted_body =
        reopened_before.inspect_table_cell(0U, 1U, 1U);
    REQUIRE(before_inserted_body.has_value());
    CHECK_EQ(before_inserted_body->text, "");

    CHECK_EQ(run_cli({"insert-table-column-after",
                      source.string(),
                      "0",
                      "0",
                      "0",
                      "--output",
                      inserted_after.string(),
                      "--json"},
                     after_output),
             0);
    CHECK_EQ(
        read_text_file(after_output),
        std::string{
            "{\"command\":\"insert-table-column-after\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"table_index\":0,\"row_index\":0,\"cell_index\":0,"
            "\"inserted_cell_index\":1,\"inserted_column_index\":1}\n"});

    featherdoc::Document reopened_after(inserted_after);
    REQUIRE_FALSE(reopened_after.open());
    const auto after_table = reopened_after.inspect_table(0U);
    REQUIRE(after_table.has_value());
    CHECK_EQ(after_table->row_count, 2U);
    CHECK_EQ(after_table->column_count, 3U);
    REQUIRE(after_table->column_widths.size() >= 3U);
    REQUIRE(after_table->column_widths[0].has_value());
    REQUIRE(after_table->column_widths[1].has_value());
    REQUIRE(after_table->column_widths[2].has_value());
    CHECK_EQ(*after_table->column_widths[0], 1800U);
    CHECK_EQ(*after_table->column_widths[1], 1800U);
    CHECK_EQ(*after_table->column_widths[2], 5400U);
    const auto after_inserted_header =
        reopened_after.inspect_table_cell(0U, 0U, 1U);
    REQUIRE(after_inserted_header.has_value());
    CHECK_EQ(after_inserted_header->text, "");
    const auto after_shifted_header =
        reopened_after.inspect_table_cell(0U, 0U, 2U);
    REQUIRE(after_shifted_header.has_value());
    CHECK_EQ(after_shifted_header->text, "r0c1");

    CHECK_EQ(run_cli({"remove-table-column",
                      source.string(),
                      "0",
                      "0",
                      "0",
                      "--output",
                      removed.string(),
                      "--json"},
                     remove_output),
             0);
    CHECK_EQ(
        read_text_file(remove_output),
        std::string{
            "{\"command\":\"remove-table-column\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"table_index\":0,\"row_index\":0,\"cell_index\":0,"
            "\"column_index\":0}\n"});

    featherdoc::Document reopened_removed(removed);
    REQUIRE_FALSE(reopened_removed.open());
    const auto removed_table = reopened_removed.inspect_table(0U);
    REQUIRE(removed_table.has_value());
    CHECK_EQ(removed_table->row_count, 2U);
    CHECK_EQ(removed_table->column_count, 1U);
    const auto removed_header = reopened_removed.inspect_table_cell(0U, 0U, 0U);
    REQUIRE(removed_header.has_value());
    CHECK_EQ(removed_header->text, "r0c1");
    const auto removed_body = reopened_removed.inspect_table_cell(0U, 1U, 0U);
    REQUIRE(removed_body.has_value());
    CHECK_EQ(removed_body->text, "r1c1");

    remove_if_exists(source);
    remove_if_exists(inserted_before);
    remove_if_exists(inserted_after);
    remove_if_exists(removed);
    remove_if_exists(before_output);
    remove_if_exists(after_output);
    remove_if_exists(remove_output);
}

TEST_CASE("cli table column commands report merge and last-column errors") {
    const fs::path working_directory = fs::current_path();
    const fs::path merged_source =
        working_directory / "cli_table_column_merge_error_source.docx";
    const fs::path single_column_source =
        working_directory / "cli_table_column_single_error_source.docx";
    const fs::path insert_error_output =
        working_directory / "cli_table_column_insert_error.json";
    const fs::path remove_merge_error_output =
        working_directory / "cli_table_column_remove_merge_error.json";
    const fs::path remove_last_error_output =
        working_directory / "cli_table_column_remove_last_error.json";

    remove_if_exists(merged_source);
    remove_if_exists(single_column_source);
    remove_if_exists(insert_error_output);
    remove_if_exists(remove_merge_error_output);
    remove_if_exists(remove_last_error_output);

    create_cli_table_column_horizontal_merge_fixture(merged_source);

    CHECK_EQ(run_cli({"insert-table-column-before",
                      merged_source.string(),
                      "0",
                      "0",
                      "1",
                      "--json"},
                     insert_error_output),
             1);
    const auto insert_error_json = read_text_file(insert_error_output);
    CHECK_NE(insert_error_json.find("\"command\":\"insert-table-column-before\""),
             std::string::npos);
    CHECK_NE(insert_error_json.find("\"stage\":\"mutate\""), std::string::npos);
    CHECK_NE(
        insert_error_json.find(
            "\"detail\":\"cannot insert a column before cell index '1' at row "
            "index '0' in table index '0' because the insertion boundary "
            "intersects a horizontal merge span\""),
        std::string::npos);

    CHECK_EQ(run_cli({"remove-table-column",
                      merged_source.string(),
                      "0",
                      "0",
                      "0",
                      "--json"},
                     remove_merge_error_output),
             1);
    const auto remove_merge_error_json =
        read_text_file(remove_merge_error_output);
    CHECK_NE(remove_merge_error_json.find("\"command\":\"remove-table-column\""),
             std::string::npos);
    CHECK_NE(remove_merge_error_json.find("\"stage\":\"mutate\""),
             std::string::npos);
    CHECK_NE(
        remove_merge_error_json.find(
            "\"detail\":\"cannot remove column index '0' from table index '0' "
            "because it intersects a horizontal merge span\""),
        std::string::npos);

    featherdoc::Document single_column_document(single_column_source);
    REQUIRE_FALSE(single_column_document.create_empty());
    auto single_column_table = single_column_document.append_table(2, 1);
    REQUIRE(single_column_table.has_next());
    populate_table_cells(single_column_table, {{"only-header"}, {"only-body"}});
    REQUIRE_FALSE(single_column_document.save());

    CHECK_EQ(run_cli({"remove-table-column",
                      single_column_source.string(),
                      "0",
                      "0",
                      "0",
                      "--json"},
                     remove_last_error_output),
             1);
    const auto remove_last_error_json =
        read_text_file(remove_last_error_output);
    CHECK_NE(remove_last_error_json.find("\"command\":\"remove-table-column\""),
             std::string::npos);
    CHECK_NE(remove_last_error_json.find("\"stage\":\"mutate\""),
             std::string::npos);
    CHECK_NE(
        remove_last_error_json.find(
            "\"detail\":\"cannot remove the last column from table index '0'\""),
        std::string::npos);

    remove_if_exists(merged_source);
    remove_if_exists(single_column_source);
    remove_if_exists(insert_error_output);
    remove_if_exists(remove_merge_error_output);
    remove_if_exists(remove_last_error_output);
}

TEST_CASE("cli table row structure commands report parse and mutate errors") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_table_row_structure_error_source.docx";
    const fs::path single_row_source =
        working_directory / "cli_table_row_structure_single_row_source.docx";
    const fs::path parse_output =
        working_directory / "cli_table_row_structure_parse_output.json";
    const fs::path merge_error_output =
        working_directory / "cli_table_row_structure_merge_error_output.json";
    const fs::path remove_error_output =
        working_directory / "cli_table_row_structure_remove_error_output.json";

    remove_if_exists(source);
    remove_if_exists(single_row_source);
    remove_if_exists(parse_output);
    remove_if_exists(merge_error_output);
    remove_if_exists(remove_error_output);

    create_cli_table_unmerge_down_fixture(source);

    CHECK_EQ(run_cli({"append-table-row",
                      source.string(),
                      "0",
                      "--cell-count",
                      "0",
                      "--json"},
                     parse_output),
             2);
    CHECK_EQ(
        read_text_file(parse_output),
        std::string{
            "{\"command\":\"append-table-row\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"cell count must be greater than "
            "0\"}\n"});

    CHECK_EQ(run_cli({"insert-table-row-after",
                      source.string(),
                      "0",
                      "0",
                      "--json"},
                     merge_error_output),
             1);
    const auto merge_error_json = read_text_file(merge_error_output);
    CHECK_NE(merge_error_json.find("\"command\":\"insert-table-row-after\""),
             std::string::npos);
    CHECK_NE(merge_error_json.find("\"stage\":\"mutate\""), std::string::npos);
    CHECK_NE(
        merge_error_json.find(
            "\"detail\":\"cannot insert a row adjacent to row index '0' in "
            "table index '0' because the row participates in a vertical "
            "merge\""),
        std::string::npos);

    featherdoc::Document single_row_document(single_row_source);
    REQUIRE_FALSE(single_row_document.create_empty());
    auto single_row_table = single_row_document.append_table(1, 1);
    REQUIRE(single_row_table.has_next());
    auto single_row = single_row_table.rows();
    REQUIRE(single_row.has_next());
    auto only_cell = single_row.cells();
    REQUIRE(only_cell.has_next());
    REQUIRE(only_cell.set_text("only-row"));
    REQUIRE_FALSE(single_row_document.save());

    CHECK_EQ(run_cli({"remove-table-row",
                      single_row_source.string(),
                      "0",
                      "0",
                      "--json"},
                     remove_error_output),
             1);
    const auto remove_error_json = read_text_file(remove_error_output);
    CHECK_NE(remove_error_json.find("\"command\":\"remove-table-row\""),
             std::string::npos);
    CHECK_NE(remove_error_json.find("\"stage\":\"mutate\""), std::string::npos);
    CHECK_NE(
        remove_error_json.find(
            "\"detail\":\"cannot remove the last row from table index '0'\""),
        std::string::npos);

    remove_if_exists(source);
    remove_if_exists(single_row_source);
    remove_if_exists(parse_output);
    remove_if_exists(merge_error_output);
    remove_if_exists(remove_error_output);
}

} // namespace
