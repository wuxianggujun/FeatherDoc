#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "cli_template_table_test_support.hpp"

namespace {
TEST_CASE("cli template table commands support bookmark table selectors") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_template_table_bookmark_source.docx";
    const fs::path inspect_output =
        working_directory / "cli_template_table_bookmark_inspect.json";
    const fs::path updated =
        working_directory / "cli_template_table_bookmark_updated.docx";
    const fs::path updated_output =
        working_directory / "cli_template_table_bookmark_updated.json";
    const fs::path rows_updated =
        working_directory / "cli_template_table_bookmark_rows_updated.docx";
    const fs::path rows_output =
        working_directory / "cli_template_table_bookmark_rows_updated.json";
    const fs::path block_updated =
        working_directory / "cli_template_table_bookmark_block_updated.docx";
    const fs::path block_output =
        working_directory / "cli_template_table_bookmark_block_updated.json";
    const fs::path appended =
        working_directory / "cli_template_table_bookmark_appended.docx";
    const fs::path append_output =
        working_directory / "cli_template_table_bookmark_appended.json";
    const fs::path inserted =
        working_directory / "cli_template_table_bookmark_inserted.docx";
    const fs::path insert_output =
        working_directory / "cli_template_table_bookmark_inserted.json";
    const fs::path range_error_output =
        working_directory / "cli_template_table_bookmark_row_range_error.json";
    const fs::path cell_error_output =
        working_directory / "cli_template_table_bookmark_row_cells_error.json";
    const fs::path block_error_output =
        working_directory / "cli_template_table_bookmark_block_error.json";
    const fs::path parse_output =
        working_directory / "cli_template_table_bookmark_parse.json";
    const fs::path row_parse_output =
        working_directory / "cli_template_table_bookmark_row_parse.json";
    const fs::path block_parse_output =
        working_directory / "cli_template_table_bookmark_block_parse.json";

    remove_if_exists(source);
    remove_if_exists(inspect_output);
    remove_if_exists(updated);
    remove_if_exists(updated_output);
    remove_if_exists(rows_updated);
    remove_if_exists(rows_output);
    remove_if_exists(block_updated);
    remove_if_exists(block_output);
    remove_if_exists(appended);
    remove_if_exists(append_output);
    remove_if_exists(inserted);
    remove_if_exists(insert_output);
    remove_if_exists(range_error_output);
    remove_if_exists(cell_error_output);
    remove_if_exists(block_error_output);
    remove_if_exists(parse_output);
    remove_if_exists(row_parse_output);
    remove_if_exists(block_parse_output);

    create_cli_template_table_bookmark_fixture(source);

    CHECK_EQ(run_cli({"inspect-template-table-rows",
                      source.string(),
                      "--bookmark",
                      "target_before_table",
                      "--json"},
                     inspect_output),
             0);
    const auto inspect_json = read_text_file(inspect_output);
    CHECK_NE(inspect_json.find("\"part\":\"body\""), std::string::npos);
    CHECK_NE(inspect_json.find("\"entry_name\":\"word/document.xml\""),
             std::string::npos);
    CHECK_NE(inspect_json.find("\"table_index\":1"), std::string::npos);
    CHECK_NE(inspect_json.find("\"count\":2"), std::string::npos);
    CHECK_NE(inspect_json.find("\"cell_texts\":[\"target-00\",\"target-01\"]"),
             std::string::npos);
    CHECK_NE(inspect_json.find("\"cell_texts\":[\"target-10\",\"target-11\"]"),
             std::string::npos);

    CHECK_EQ(run_cli({"set-template-table-cell-text",
                      source.string(),
                      "--bookmark",
                      "target_before_table",
                      "1",
                      "1",
                      "--text",
                      "updated",
                      "--output",
                      updated.string(),
                      "--json"},
                     updated_output),
             0);
    const auto updated_json = read_text_file(updated_output);
    CHECK_NE(updated_json.find("\"command\":\"set-template-table-cell-text\""),
             std::string::npos);
    CHECK_NE(updated_json.find("\"part\":\"body\""), std::string::npos);
    CHECK_NE(updated_json.find("\"table_index\":1"), std::string::npos);
    CHECK_NE(updated_json.find("\"row_index\":1"), std::string::npos);
    CHECK_NE(updated_json.find("\"cell_index\":1"), std::string::npos);

    featherdoc::Document reopened_updated(updated);
    REQUIRE_FALSE(reopened_updated.open());
    auto updated_body = reopened_updated.body_template();
    REQUIRE(static_cast<bool>(updated_body));
    const auto preserved_cell = updated_body.inspect_table_cell(0U, 0U, 0U);
    REQUIRE(preserved_cell.has_value());
    CHECK_EQ(preserved_cell->text, "keep-00");
    const auto updated_cell = updated_body.inspect_table_cell(1U, 1U, 1U);
    REQUIRE(updated_cell.has_value());
    CHECK_EQ(updated_cell->text, "updated");

    CHECK_EQ(run_cli({"set-template-table-row-texts",
                      source.string(),
                      "--bookmark",
                      "target_before_table",
                      "0",
                      "--row",
                      "batch-00",
                      "--cell",
                      "batch-01",
                      "--row",
                      "batch-10",
                      "--cell",
                      "batch-11",
                      "--output",
                      rows_updated.string(),
                      "--json"},
                     rows_output),
             0);
    const auto rows_json = read_text_file(rows_output);
    CHECK_NE(rows_json.find("\"command\":\"set-template-table-row-texts\""),
             std::string::npos);
    CHECK_NE(rows_json.find("\"part\":\"body\""), std::string::npos);
    CHECK_NE(rows_json.find("\"bookmark_name\":\"target_before_table\""),
             std::string::npos);
    CHECK_NE(rows_json.find("\"table_index\":1"), std::string::npos);
    CHECK_NE(rows_json.find("\"start_row_index\":0"), std::string::npos);
    CHECK_NE(rows_json.find("\"row_count\":2"), std::string::npos);
    CHECK_NE(rows_json.find("\"rows\":[[\"batch-00\",\"batch-01\"],[\"batch-10\",\"batch-11\"]]"),
             std::string::npos);

    featherdoc::Document reopened_rows(rows_updated);
    REQUIRE_FALSE(reopened_rows.open());
    auto rows_body = reopened_rows.body_template();
    REQUIRE(static_cast<bool>(rows_body));
    const auto preserved_batch_cell = rows_body.inspect_table_cell(0U, 0U, 0U);
    REQUIRE(preserved_batch_cell.has_value());
    CHECK_EQ(preserved_batch_cell->text, "keep-00");
    const auto batch_first_cell = rows_body.inspect_table_cell(1U, 0U, 0U);
    REQUIRE(batch_first_cell.has_value());
    CHECK_EQ(batch_first_cell->text, "batch-00");
    const auto batch_second_cell = rows_body.inspect_table_cell(1U, 0U, 1U);
    REQUIRE(batch_second_cell.has_value());
    CHECK_EQ(batch_second_cell->text, "batch-01");
    const auto batch_third_cell = rows_body.inspect_table_cell(1U, 1U, 0U);
    REQUIRE(batch_third_cell.has_value());
    CHECK_EQ(batch_third_cell->text, "batch-10");
    const auto batch_fourth_cell = rows_body.inspect_table_cell(1U, 1U, 1U);
    REQUIRE(batch_fourth_cell.has_value());
    CHECK_EQ(batch_fourth_cell->text, "batch-11");

    CHECK_EQ(run_cli({"set-template-table-row-texts",
                      source.string(),
                      "--bookmark",
                      "target_inside_table",
                      "1",
                      "--row",
                      "overflow-10",
                      "--cell",
                      "overflow-11",
                      "--row",
                      "overflow-20",
                      "--cell",
                      "overflow-21",
                      "--json"},
                     range_error_output),
             1);
    const auto range_error_json = read_text_file(range_error_output);
    CHECK_NE(range_error_json.find("\"command\":\"set-template-table-row-texts\""),
             std::string::npos);
    CHECK_NE(range_error_json.find("\"stage\":\"mutate\""),
             std::string::npos);
    CHECK_NE(range_error_json.find("\"detail\":\"row range starting at row index '1' with count '2' exceeds table index '1' row count '2'\""),
             std::string::npos);

    CHECK_EQ(run_cli({"set-template-table-row-texts",
                      source.string(),
                      "--bookmark",
                      "target_before_table",
                      "0",
                      "--row",
                      "only-one-cell",
                      "--json"},
                     cell_error_output),
             1);
    const auto cell_error_json = read_text_file(cell_error_output);
    CHECK_NE(cell_error_json.find("\"command\":\"set-template-table-row-texts\""),
             std::string::npos);
    CHECK_NE(cell_error_json.find("\"stage\":\"mutate\""),
             std::string::npos);
    CHECK_NE(cell_error_json.find("\"detail\":\"replacement row at offset '0' contains '1' cells but target row index '0' contains '2' cells\""),
             std::string::npos);

    CHECK_EQ(run_cli({"set-template-table-cell-block-texts",
                      source.string(),
                      "--bookmark",
                      "target_before_table",
                      "0",
                      "1",
                      "--row",
                      "block-01",
                      "--row",
                      "block-11",
                      "--output",
                      block_updated.string(),
                      "--json"},
                     block_output),
             0);
    const auto block_json = read_text_file(block_output);
    CHECK_NE(block_json.find("\"command\":\"set-template-table-cell-block-texts\""),
             std::string::npos);
    CHECK_NE(block_json.find("\"part\":\"body\""), std::string::npos);
    CHECK_NE(block_json.find("\"bookmark_name\":\"target_before_table\""),
             std::string::npos);
    CHECK_NE(block_json.find("\"table_index\":1"), std::string::npos);
    CHECK_NE(block_json.find("\"start_row_index\":0"), std::string::npos);
    CHECK_NE(block_json.find("\"start_cell_index\":1"), std::string::npos);
    CHECK_NE(block_json.find("\"row_count\":2"), std::string::npos);
    CHECK_NE(block_json.find("\"rows\":[[\"block-01\"],[\"block-11\"]]"),
             std::string::npos);

    featherdoc::Document reopened_block(block_updated);
    REQUIRE_FALSE(reopened_block.open());
    auto block_body = reopened_block.body_template();
    REQUIRE(static_cast<bool>(block_body));
    const auto preserved_block_first = block_body.inspect_table_cell(1U, 0U, 0U);
    REQUIRE(preserved_block_first.has_value());
    CHECK_EQ(preserved_block_first->text, "target-00");
    const auto updated_block_first = block_body.inspect_table_cell(1U, 0U, 1U);
    REQUIRE(updated_block_first.has_value());
    CHECK_EQ(updated_block_first->text, "block-01");
    const auto preserved_block_second = block_body.inspect_table_cell(1U, 1U, 0U);
    REQUIRE(preserved_block_second.has_value());
    CHECK_EQ(preserved_block_second->text, "target-10");
    const auto updated_block_second = block_body.inspect_table_cell(1U, 1U, 1U);
    REQUIRE(updated_block_second.has_value());
    CHECK_EQ(updated_block_second->text, "block-11");

    CHECK_EQ(run_cli({"set-template-table-cell-block-texts",
                      source.string(),
                      "--bookmark",
                      "target_inside_table",
                      "0",
                      "1",
                      "--row",
                      "too-wide-01",
                      "--cell",
                      "too-wide-02",
                      "--json"},
                     block_error_output),
             1);
    const auto block_error_json = read_text_file(block_error_output);
    CHECK_NE(block_error_json.find("\"command\":\"set-template-table-cell-block-texts\""),
             std::string::npos);
    CHECK_NE(block_error_json.find("\"stage\":\"mutate\""),
             std::string::npos);
    CHECK_NE(block_error_json.find("\"detail\":\"replacement row at offset '0' starting at cell index '1' with count '2' exceeds target row index '0' cell count '2'\""),
             std::string::npos);

    CHECK_EQ(run_cli({"append-template-table-row",
                      source.string(),
                      "--bookmark",
                      "target_inside_table",
                      "--output",
                      appended.string(),
                      "--json"},
                     append_output),
             0);
    const auto append_json = read_text_file(append_output);
    CHECK_NE(append_json.find("\"command\":\"append-template-table-row\""),
             std::string::npos);
    CHECK_NE(append_json.find("\"part\":\"body\""), std::string::npos);
    CHECK_NE(append_json.find("\"table_index\":1"), std::string::npos);
    CHECK_NE(append_json.find("\"row_index\":2"), std::string::npos);
    CHECK_NE(append_json.find("\"cell_count\":2"), std::string::npos);

    featherdoc::Document reopened_appended(appended);
    REQUIRE_FALSE(reopened_appended.open());
    auto appended_body = reopened_appended.body_template();
    REQUIRE(static_cast<bool>(appended_body));
    const auto appended_table = appended_body.inspect_table(1U);
    REQUIRE(appended_table.has_value());
    CHECK_EQ(appended_table->row_count, 3U);
    const auto appended_first_cell = appended_body.inspect_table_cell(1U, 2U, 0U);
    REQUIRE(appended_first_cell.has_value());
    CHECK_EQ(appended_first_cell->text, "");
    const auto appended_second_cell = appended_body.inspect_table_cell(1U, 2U, 1U);
    REQUIRE(appended_second_cell.has_value());
    CHECK_EQ(appended_second_cell->text, "");

    CHECK_EQ(run_cli({"insert-template-table-row-before",
                      source.string(),
                      "--bookmark",
                      "target_before_table",
                      "1",
                      "--output",
                      inserted.string(),
                      "--json"},
                     insert_output),
             0);
    const auto insert_json = read_text_file(insert_output);
    CHECK_NE(insert_json.find("\"command\":\"insert-template-table-row-before\""),
             std::string::npos);
    CHECK_NE(insert_json.find("\"part\":\"body\""), std::string::npos);
    CHECK_NE(insert_json.find("\"table_index\":1"), std::string::npos);
    CHECK_NE(insert_json.find("\"row_index\":1"), std::string::npos);
    CHECK_NE(insert_json.find("\"inserted_row_index\":1"), std::string::npos);

    featherdoc::Document reopened_inserted(inserted);
    REQUIRE_FALSE(reopened_inserted.open());
    auto inserted_body = reopened_inserted.body_template();
    REQUIRE(static_cast<bool>(inserted_body));
    const auto inserted_table = inserted_body.inspect_table(1U);
    REQUIRE(inserted_table.has_value());
    CHECK_EQ(inserted_table->row_count, 3U);
    const auto inserted_first_cell = inserted_body.inspect_table_cell(1U, 1U, 0U);
    REQUIRE(inserted_first_cell.has_value());
    CHECK_EQ(inserted_first_cell->text, "");
    const auto inserted_second_cell = inserted_body.inspect_table_cell(1U, 1U, 1U);
    REQUIRE(inserted_second_cell.has_value());
    CHECK_EQ(inserted_second_cell->text, "");
    const auto shifted_row_cell = inserted_body.inspect_table_cell(1U, 2U, 0U);
    REQUIRE(shifted_row_cell.has_value());
    CHECK_EQ(shifted_row_cell->text, "target-10");

    CHECK_EQ(run_cli({"append-template-table-row",
                      source.string(),
                      "1",
                      "--bookmark",
                      "target_inside_table",
                      "--json"},
                     parse_output),
             2);
    CHECK_EQ(
        read_text_file(parse_output),
        std::string{
            "{\"command\":\"append-template-table-row\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"cannot combine a table index "
            "with --bookmark\"}\n"});

    CHECK_EQ(run_cli({"set-template-table-row-texts",
                      source.string(),
                      "1",
                      "0",
                      "--bookmark",
                      "target_inside_table",
                      "--row",
                      "dup-00",
                      "--cell",
                      "dup-01",
                      "--json"},
                     row_parse_output),
             2);
    CHECK_EQ(
        read_text_file(row_parse_output),
        std::string{
            "{\"command\":\"set-template-table-row-texts\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"cannot combine a table index "
            "with --bookmark\"}\n"});

    CHECK_EQ(run_cli({"set-template-table-cell-block-texts",
                      source.string(),
                      "1",
                      "0",
                      "1",
                      "--bookmark",
                      "target_inside_table",
                      "--row",
                      "dup-block-01",
                      "--json"},
                     block_parse_output),
             2);
    CHECK_EQ(
        read_text_file(block_parse_output),
        std::string{
            "{\"command\":\"set-template-table-cell-block-texts\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"cannot combine a table index "
            "with --bookmark\"}\n"});

    remove_if_exists(source);
    remove_if_exists(inspect_output);
    remove_if_exists(updated);
    remove_if_exists(updated_output);
    remove_if_exists(rows_updated);
    remove_if_exists(rows_output);
    remove_if_exists(block_updated);
    remove_if_exists(block_output);
    remove_if_exists(appended);
    remove_if_exists(append_output);
    remove_if_exists(inserted);
    remove_if_exists(insert_output);
    remove_if_exists(range_error_output);
    remove_if_exists(cell_error_output);
    remove_if_exists(block_error_output);
    remove_if_exists(parse_output);
    remove_if_exists(row_parse_output);
    remove_if_exists(block_parse_output);
}

TEST_CASE("cli set-template-table-from-json applies bookmark-targeted patches") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_template_table_json_patch_source.docx";
    const fs::path rows_patch =
        working_directory / "cli_template_table_rows_patch.json";
    const fs::path block_patch =
        working_directory / "cli_template_table_block_patch.json";
    const fs::path parse_patch =
        working_directory / "cli_template_table_parse_patch.json";
    const fs::path mutate_patch =
        working_directory / "cli_template_table_mutate_patch.json";
    const fs::path rows_updated =
        working_directory / "cli_template_table_rows_patch_updated.docx";
    const fs::path block_updated =
        working_directory / "cli_template_table_block_patch_updated.docx";
    const fs::path rows_output =
        working_directory / "cli_template_table_rows_patch_output.json";
    const fs::path block_output =
        working_directory / "cli_template_table_block_patch_output.json";
    const fs::path parse_output =
        working_directory / "cli_template_table_parse_patch_output.json";
    const fs::path mutate_output =
        working_directory / "cli_template_table_mutate_patch_output.json";
    const fs::path selector_parse_output =
        working_directory /
        "cli_template_table_bookmark_selector_parse_output.json";

    remove_if_exists(source);
    remove_if_exists(rows_patch);
    remove_if_exists(block_patch);
    remove_if_exists(parse_patch);
    remove_if_exists(mutate_patch);
    remove_if_exists(rows_updated);
    remove_if_exists(block_updated);
    remove_if_exists(rows_output);
    remove_if_exists(block_output);
    remove_if_exists(parse_output);
    remove_if_exists(mutate_output);
    remove_if_exists(selector_parse_output);

    create_cli_template_table_bookmark_fixture(source);
    write_binary_file(
        rows_patch,
        R"({
  "mode": "rows",
  "start_row": 0,
  "rows": [
    ["json-00", 12],
    ["json-10", true]
  ]
})");
    write_binary_file(
        block_patch,
        R"({
  "command": "set-template-table-cell-block-texts",
  "start_row_index": 0,
  "start_cell_index": 1,
  "row_count": 2,
  "rows": [
    [12],
    [false]
  ]
})");
    write_binary_file(
        parse_patch,
        R"({
  "mode": "rows",
  "rows": [
    ["missing-start-row", "value"]
  ]
})");
    write_binary_file(
        mutate_patch,
        R"({
  "mode": "rows",
  "start_row": 1,
  "rows": [
    ["overflow-10", "overflow-11"],
    ["overflow-20", "overflow-21"]
  ]
})");

    CHECK_EQ(run_cli({"set-template-table-from-json",
                      source.string(),
                      "--bookmark",
                      "target_before_table",
                      "--patch-file",
                      rows_patch.string(),
                      "--output",
                      rows_updated.string(),
                      "--json"},
                     rows_output),
             0);
    const auto rows_json = read_text_file(rows_output);
    CHECK_NE(rows_json.find("\"command\":\"set-template-table-from-json\""),
             std::string::npos);
    CHECK_NE(rows_json.find("\"part\":\"body\""), std::string::npos);
    CHECK_NE(rows_json.find("\"bookmark_name\":\"target_before_table\""),
             std::string::npos);
    CHECK_NE(rows_json.find("\"mode\":\"rows\""), std::string::npos);
    CHECK_NE(rows_json.find("\"table_index\":1"), std::string::npos);
    CHECK_NE(rows_json.find("\"start_row_index\":0"), std::string::npos);
    CHECK_NE(rows_json.find("\"row_count\":2"), std::string::npos);
    CHECK_NE(rows_json.find("\"rows\":[[\"json-00\",\"12\"],[\"json-10\",\"true\"]]"),
             std::string::npos);

    featherdoc::Document reopened_rows(rows_updated);
    REQUIRE_FALSE(reopened_rows.open());
    auto rows_body = reopened_rows.body_template();
    REQUIRE(static_cast<bool>(rows_body));
    const auto preserved_rows_cell = rows_body.inspect_table_cell(0U, 0U, 0U);
    REQUIRE(preserved_rows_cell.has_value());
    CHECK_EQ(preserved_rows_cell->text, "keep-00");
    const auto json_row_first = rows_body.inspect_table_cell(1U, 0U, 0U);
    REQUIRE(json_row_first.has_value());
    CHECK_EQ(json_row_first->text, "json-00");
    const auto json_row_second = rows_body.inspect_table_cell(1U, 0U, 1U);
    REQUIRE(json_row_second.has_value());
    CHECK_EQ(json_row_second->text, "12");
    const auto json_row_third = rows_body.inspect_table_cell(1U, 1U, 0U);
    REQUIRE(json_row_third.has_value());
    CHECK_EQ(json_row_third->text, "json-10");
    const auto json_row_fourth = rows_body.inspect_table_cell(1U, 1U, 1U);
    REQUIRE(json_row_fourth.has_value());
    CHECK_EQ(json_row_fourth->text, "true");

    CHECK_EQ(run_cli({"set-template-table-from-json",
                      source.string(),
                      "--bookmark",
                      "target_inside_table",
                      "--patch-file",
                      block_patch.string(),
                      "--output",
                      block_updated.string(),
                      "--json"},
                     block_output),
             0);
    const auto block_json = read_text_file(block_output);
    CHECK_NE(block_json.find("\"command\":\"set-template-table-from-json\""),
             std::string::npos);
    CHECK_NE(block_json.find("\"bookmark_name\":\"target_inside_table\""),
             std::string::npos);
    CHECK_NE(block_json.find("\"mode\":\"block\""), std::string::npos);
    CHECK_NE(block_json.find("\"table_index\":1"), std::string::npos);
    CHECK_NE(block_json.find("\"start_row_index\":0"), std::string::npos);
    CHECK_NE(block_json.find("\"start_cell_index\":1"), std::string::npos);
    CHECK_NE(block_json.find("\"row_count\":2"), std::string::npos);
    CHECK_NE(block_json.find("\"rows\":[[\"12\"],[\"false\"]]"),
             std::string::npos);

    featherdoc::Document reopened_block(block_updated);
    REQUIRE_FALSE(reopened_block.open());
    auto block_body = reopened_block.body_template();
    REQUIRE(static_cast<bool>(block_body));
    const auto preserved_block_first = block_body.inspect_table_cell(1U, 0U, 0U);
    REQUIRE(preserved_block_first.has_value());
    CHECK_EQ(preserved_block_first->text, "target-00");
    const auto updated_block_first = block_body.inspect_table_cell(1U, 0U, 1U);
    REQUIRE(updated_block_first.has_value());
    CHECK_EQ(updated_block_first->text, "12");
    const auto preserved_block_second = block_body.inspect_table_cell(1U, 1U, 0U);
    REQUIRE(preserved_block_second.has_value());
    CHECK_EQ(preserved_block_second->text, "target-10");
    const auto updated_block_second = block_body.inspect_table_cell(1U, 1U, 1U);
    REQUIRE(updated_block_second.has_value());
    CHECK_EQ(updated_block_second->text, "false");

    CHECK_EQ(run_cli({"set-template-table-from-json",
                      source.string(),
                      "--bookmark",
                      "target_before_table",
                      "--patch-file",
                      parse_patch.string(),
                      "--json"},
                     parse_output),
             2);
    CHECK_EQ(
        read_text_file(parse_output),
        std::string{
            "{\"command\":\"set-template-table-from-json\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"JSON patch must contain "
            "'start_row' or 'start_row_index'\"}\n"});

    CHECK_EQ(run_cli({"set-template-table-from-json",
                      source.string(),
                      "--bookmark",
                      "target_inside_table",
                      "--patch-file",
                      mutate_patch.string(),
                      "--json"},
                     mutate_output),
             1);
    const auto mutate_json = read_text_file(mutate_output);
    CHECK_NE(mutate_json.find("\"command\":\"set-template-table-from-json\""),
             std::string::npos);
    CHECK_NE(mutate_json.find("\"stage\":\"mutate\""), std::string::npos);
    CHECK_NE(mutate_json.find("\"detail\":\"row range starting at row index '1' with count '2' exceeds table index '1' row count '2'\""),
             std::string::npos);

    CHECK_EQ(run_cli({"set-template-table-from-json",
                      source.string(),
                      "1",
                      "--bookmark",
                      "target_inside_table",
                      "--patch-file",
                      rows_patch.string(),
                      "--json"},
                     selector_parse_output),
             2);
    CHECK_EQ(
        read_text_file(selector_parse_output),
        std::string{
            "{\"command\":\"set-template-table-from-json\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"cannot combine a table index "
            "with --bookmark\"}\n"});

    remove_if_exists(source);
    remove_if_exists(rows_patch);
    remove_if_exists(block_patch);
    remove_if_exists(parse_patch);
    remove_if_exists(mutate_patch);
    remove_if_exists(rows_updated);
    remove_if_exists(block_updated);
    remove_if_exists(rows_output);
    remove_if_exists(block_output);
    remove_if_exists(parse_output);
    remove_if_exists(mutate_output);
    remove_if_exists(selector_parse_output);
}

} // namespace
