#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "cli_template_table_test_support.hpp"

namespace {
TEST_CASE("cli template table selectors support after-text, header cells, and occurrence") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_template_table_selector_source.docx";
    const fs::path after_output =
        working_directory / "cli_template_table_selector_after.json";
    const fs::path header_output =
        working_directory / "cli_template_table_selector_header.json";
    const fs::path combined_output =
        working_directory / "cli_template_table_selector_combined.json";
    const fs::path parse_output =
        working_directory / "cli_template_table_selector_parse.json";

    remove_if_exists(source);
    remove_if_exists(after_output);
    remove_if_exists(header_output);
    remove_if_exists(combined_output);
    remove_if_exists(parse_output);

    create_cli_template_table_selector_fixture(source);

    CHECK_EQ(run_cli({"inspect-template-table-rows",
                      source.string(),
                      "--after-text",
                      "selector target table",
                      "--json"},
                     after_output),
             0);
    const auto after_json = read_text_file(after_output);
    CHECK_NE(after_json.find("\"table_index\":0"), std::string::npos);
    CHECK_NE(after_json.find("\"cell_texts\":[\"Region\",\"Qty\",\"Amount\"]"),
             std::string::npos);
    CHECK_NE(after_json.find("\"cell_texts\":[\"North\",\"7\",\"12\"]"),
             std::string::npos);

    CHECK_EQ(run_cli({"inspect-template-table-rows",
                      source.string(),
                      "--header-cell",
                      "Region",
                      "--header-cell",
                      "Qty",
                      "--header-cell",
                      "Amount",
                      "--occurrence",
                      "1",
                      "--json"},
                     header_output),
             0);
    const auto header_json = read_text_file(header_output);
    CHECK_NE(header_json.find("\"table_index\":2"), std::string::npos);
    CHECK_NE(header_json.find("\"cell_texts\":[\"South\",\"9\",\"24\"]"),
             std::string::npos);

    CHECK_EQ(run_cli({"inspect-template-table-rows",
                      source.string(),
                      "--after-text",
                      "selector target table",
                      "--header-cell",
                      "Region",
                      "--header-cell",
                      "Qty",
                      "--header-cell",
                      "Amount",
                      "--occurrence",
                      "1",
                      "--json"},
                     combined_output),
             0);
    const auto combined_json = read_text_file(combined_output);
    CHECK_NE(combined_json.find("\"table_index\":2"), std::string::npos);
    CHECK_NE(combined_json.find("\"cell_texts\":[\"South\",\"9\",\"24\"]"),
             std::string::npos);

    CHECK_EQ(run_cli({"inspect-template-table-rows",
                      source.string(),
                      "0",
                      "--after-text",
                      "selector target table",
                      "--json"},
                     parse_output),
             2);
    CHECK_EQ(
        read_text_file(parse_output),
        std::string{
            "{\"command\":\"inspect-template-table-rows\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"cannot combine a table index or "
            "--bookmark with --after-text or --header-cell\"}\n"});

    remove_if_exists(source);
    remove_if_exists(after_output);
    remove_if_exists(header_output);
    remove_if_exists(combined_output);
    remove_if_exists(parse_output);
}

TEST_CASE("cli set-template-table-from-json supports selector-based targeting") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_template_table_selector_patch_source.docx";
    const fs::path patch_path =
        working_directory / "cli_template_table_selector_patch.json";
    const fs::path updated =
        working_directory / "cli_template_table_selector_patch_updated.docx";
    const fs::path output =
        working_directory / "cli_template_table_selector_patch_output.json";
    const fs::path parse_output =
        working_directory / "cli_template_table_selector_patch_parse.json";

    remove_if_exists(source);
    remove_if_exists(patch_path);
    remove_if_exists(updated);
    remove_if_exists(output);
    remove_if_exists(parse_output);

    create_cli_template_table_selector_fixture(source);
    write_binary_file(
        patch_path,
        R"({
  "mode": "rows",
  "start_row": 1,
  "rows": [
    ["South-updated", 18, 199]
  ]
})");

    CHECK_EQ(run_cli({"set-template-table-from-json",
                      source.string(),
                      "--after-text",
                      "selector target table",
                      "--header-cell",
                      "Region",
                      "--header-cell",
                      "Qty",
                      "--header-cell",
                      "Amount",
                      "--occurrence",
                      "1",
                      "--patch-file",
                      patch_path.string(),
                      "--output",
                      updated.string(),
                      "--json"},
                     output),
             0);
    const auto output_json = read_text_file(output);
    CHECK_NE(output_json.find("\"command\":\"set-template-table-from-json\""),
             std::string::npos);
    CHECK_NE(output_json.find("\"after_text\":\"selector target table\""),
             std::string::npos);
    CHECK_NE(output_json.find("\"header_cells\":[\"Region\",\"Qty\",\"Amount\"]"),
             std::string::npos);
    CHECK_NE(output_json.find("\"header_row_index\":0"), std::string::npos);
    CHECK_NE(output_json.find("\"occurrence\":1"), std::string::npos);
    CHECK_NE(output_json.find("\"table_index\":2"), std::string::npos);

    featherdoc::Document reopened(updated);
    REQUIRE_FALSE(reopened.open());
    auto body = reopened.body_template();
    REQUIRE(static_cast<bool>(body));
    const auto preserved_first_target = body.inspect_table_cell(0U, 1U, 0U);
    REQUIRE(preserved_first_target.has_value());
    CHECK_EQ(preserved_first_target->text, "North");
    const auto updated_region = body.inspect_table_cell(2U, 1U, 0U);
    REQUIRE(updated_region.has_value());
    CHECK_EQ(updated_region->text, "South-updated");
    const auto updated_qty = body.inspect_table_cell(2U, 1U, 1U);
    REQUIRE(updated_qty.has_value());
    CHECK_EQ(updated_qty->text, "18");
    const auto updated_amount = body.inspect_table_cell(2U, 1U, 2U);
    REQUIRE(updated_amount.has_value());
    CHECK_EQ(updated_amount->text, "199");

    CHECK_EQ(run_cli({"set-template-table-from-json",
                      source.string(),
                      "--header-row",
                      "1",
                      "--patch-file",
                      patch_path.string(),
                      "--json"},
                     parse_output),
             2);
    CHECK_EQ(
        read_text_file(parse_output),
        std::string{
            "{\"command\":\"set-template-table-from-json\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"--header-row requires at least "
            "one --header-cell\"}\n"});

    remove_if_exists(source);
    remove_if_exists(patch_path);
    remove_if_exists(updated);
    remove_if_exists(output);
    remove_if_exists(parse_output);
}

TEST_CASE("cli set-template-tables-from-json applies multiple table patches") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_template_tables_batch_source.docx";
    const fs::path success_patch =
        working_directory / "cli_template_tables_batch_success.json";
    const fs::path parse_patch =
        working_directory / "cli_template_tables_batch_parse.json";
    const fs::path mutate_patch =
        working_directory / "cli_template_tables_batch_mutate.json";
    const fs::path updated =
        working_directory / "cli_template_tables_batch_updated.docx";
    const fs::path success_output =
        working_directory / "cli_template_tables_batch_success_output.json";
    const fs::path parse_output =
        working_directory / "cli_template_tables_batch_parse_output.json";
    const fs::path mutate_output =
        working_directory / "cli_template_tables_batch_mutate_output.json";

    remove_if_exists(source);
    remove_if_exists(success_patch);
    remove_if_exists(parse_patch);
    remove_if_exists(mutate_patch);
    remove_if_exists(updated);
    remove_if_exists(success_output);
    remove_if_exists(parse_output);
    remove_if_exists(mutate_output);

    create_cli_template_table_bookmark_fixture(source);
    write_binary_file(
        success_patch,
        R"({
  "operations": [
    {
      "table_index": 0,
      "mode": "rows",
      "start_row": 0,
      "rows": [
        ["keep-json-00", "keep-json-01"]
      ]
    },
    {
      "bookmark": "target_before_table",
      "start_row_index": 0,
      "start_cell_index": 1,
      "rows": [
        [12],
        [false]
      ]
    }
  ]
})");
    write_binary_file(
        parse_patch,
        R"({
  "operations": [
    {
      "mode": "rows",
      "start_row": 0,
      "rows": [
        ["missing-selector", "value"]
      ]
    }
  ]
})");
    write_binary_file(
        mutate_patch,
        R"({
  "operations": [
    {
      "table_index": 0,
      "mode": "rows",
      "start_row": 0,
      "rows": [
        ["keep-json-00", "keep-json-01"]
      ]
    },
    {
      "bookmark_name": "target_inside_table",
      "mode": "rows",
      "start_row": 1,
      "rows": [
        ["overflow-10", "overflow-11"],
        ["overflow-20", "overflow-21"]
      ]
    }
  ]
})");

    CHECK_EQ(run_cli({"set-template-tables-from-json",
                      source.string(),
                      "--patch-file",
                      success_patch.string(),
                      "--output",
                      updated.string(),
                      "--json"},
                     success_output),
             0);
    const auto success_json = read_text_file(success_output);
    CHECK_NE(success_json.find("\"command\":\"set-template-tables-from-json\""),
             std::string::npos);
    CHECK_NE(success_json.find("\"operation_count\":2"), std::string::npos);
    CHECK_NE(success_json.find("\"operation_index\":0"), std::string::npos);
    CHECK_NE(success_json.find("\"table_index\":0"), std::string::npos);
    CHECK_NE(success_json.find("\"rows\":[[\"keep-json-00\",\"keep-json-01\"]]"),
             std::string::npos);
    CHECK_NE(success_json.find("\"operation_index\":1"), std::string::npos);
    CHECK_NE(success_json.find("\"bookmark_name\":\"target_before_table\""),
             std::string::npos);
    CHECK_NE(success_json.find("\"mode\":\"block\""), std::string::npos);
    CHECK_NE(success_json.find("\"start_cell_index\":1"), std::string::npos);
    CHECK_NE(success_json.find("\"rows\":[[\"12\"],[\"false\"]]"),
             std::string::npos);

    featherdoc::Document reopened_updated(updated);
    REQUIRE_FALSE(reopened_updated.open());
    auto updated_body = reopened_updated.body_template();
    REQUIRE(static_cast<bool>(updated_body));
    const auto first_table_first_cell =
        updated_body.inspect_table_cell(0U, 0U, 0U);
    REQUIRE(first_table_first_cell.has_value());
    CHECK_EQ(first_table_first_cell->text, "keep-json-00");
    const auto first_table_second_cell =
        updated_body.inspect_table_cell(0U, 0U, 1U);
    REQUIRE(first_table_second_cell.has_value());
    CHECK_EQ(first_table_second_cell->text, "keep-json-01");
    const auto second_table_first_region =
        updated_body.inspect_table_cell(1U, 0U, 0U);
    REQUIRE(second_table_first_region.has_value());
    CHECK_EQ(second_table_first_region->text, "target-00");
    const auto second_table_first_value =
        updated_body.inspect_table_cell(1U, 0U, 1U);
    REQUIRE(second_table_first_value.has_value());
    CHECK_EQ(second_table_first_value->text, "12");
    const auto second_table_second_region =
        updated_body.inspect_table_cell(1U, 1U, 0U);
    REQUIRE(second_table_second_region.has_value());
    CHECK_EQ(second_table_second_region->text, "target-10");
    const auto second_table_second_value =
        updated_body.inspect_table_cell(1U, 1U, 1U);
    REQUIRE(second_table_second_value.has_value());
    CHECK_EQ(second_table_second_value->text, "false");

    CHECK_EQ(run_cli({"set-template-tables-from-json",
                      source.string(),
                      "--patch-file",
                      parse_patch.string(),
                      "--json"},
                     parse_output),
             2);
    CHECK_EQ(
        read_text_file(parse_output),
        std::string{
            "{\"command\":\"set-template-tables-from-json\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"operation[0]: expected a table "
            "index, --bookmark <name>, --after-text <text>, or --header-cell "
            "<text>\"}\n"});

    CHECK_EQ(run_cli({"set-template-tables-from-json",
                      source.string(),
                      "--patch-file",
                      mutate_patch.string(),
                      "--json"},
                     mutate_output),
             1);
    const auto mutate_json = read_text_file(mutate_output);
    CHECK_NE(mutate_json.find("\"command\":\"set-template-tables-from-json\""),
             std::string::npos);
    CHECK_NE(mutate_json.find("\"stage\":\"mutate\""), std::string::npos);
    CHECK_NE(mutate_json.find("\"detail\":\"operation[1]: row range starting at row index '1' with count '2' exceeds table index '1' row count '2'\""),
             std::string::npos);

    remove_if_exists(source);
    remove_if_exists(success_patch);
    remove_if_exists(parse_patch);
    remove_if_exists(mutate_patch);
    remove_if_exists(updated);
    remove_if_exists(success_output);
    remove_if_exists(parse_output);
    remove_if_exists(mutate_output);
}

TEST_CASE("cli set-template-tables-from-json supports selector-based operations") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_template_tables_selector_source.docx";
    const fs::path success_patch =
        working_directory / "cli_template_tables_selector_success.json";
    const fs::path parse_patch =
        working_directory / "cli_template_tables_selector_parse.json";
    const fs::path updated =
        working_directory / "cli_template_tables_selector_updated.docx";
    const fs::path success_output =
        working_directory / "cli_template_tables_selector_success_output.json";
    const fs::path parse_output =
        working_directory / "cli_template_tables_selector_parse_output.json";

    remove_if_exists(source);
    remove_if_exists(success_patch);
    remove_if_exists(parse_patch);
    remove_if_exists(updated);
    remove_if_exists(success_output);
    remove_if_exists(parse_output);

    create_cli_template_table_selector_fixture(source);
    write_binary_file(
        success_patch,
        R"({
  "operations": [
    {
      "after_text": "selector target table",
      "header_cells": ["Region", "Qty", "Amount"],
      "occurrence": 1,
      "mode": "rows",
      "start_row": 1,
      "rows": [
        ["South-batch", 21, 240]
      ]
    },
    {
      "header_cell_texts": ["Label", "Value"],
      "mode": "rows",
      "start_row": 1,
      "rows": [
        ["Other-batch", 42]
      ]
    }
  ]
})");
    write_binary_file(
        parse_patch,
        R"({
  "operations": [
    {
      "table_index": 2,
      "after_text": "selector target table",
      "mode": "rows",
      "start_row": 1,
      "rows": [
        ["invalid-selector", 1, 2]
      ]
    }
  ]
})");

    CHECK_EQ(run_cli({"set-template-tables-from-json",
                      source.string(),
                      "--patch-file",
                      success_patch.string(),
                      "--output",
                      updated.string(),
                      "--json"},
                     success_output),
             0);
    const auto success_json = read_text_file(success_output);
    CHECK_NE(success_json.find("\"command\":\"set-template-tables-from-json\""),
             std::string::npos);
    CHECK_NE(success_json.find("\"after_text\":\"selector target table\""),
             std::string::npos);
    CHECK_NE(success_json.find("\"header_cells\":[\"Region\",\"Qty\",\"Amount\"]"),
             std::string::npos);
    CHECK_NE(success_json.find("\"occurrence\":1"), std::string::npos);
    CHECK_NE(success_json.find("\"table_index\":2"), std::string::npos);
    CHECK_NE(success_json.find("\"header_cells\":[\"Label\",\"Value\"]"),
             std::string::npos);

    featherdoc::Document reopened(updated);
    REQUIRE_FALSE(reopened.open());
    auto body = reopened.body_template();
    REQUIRE(static_cast<bool>(body));
    const auto updated_other_left = body.inspect_table_cell(1U, 1U, 0U);
    REQUIRE(updated_other_left.has_value());
    CHECK_EQ(updated_other_left->text, "Other-batch");
    const auto updated_other_right = body.inspect_table_cell(1U, 1U, 1U);
    REQUIRE(updated_other_right.has_value());
    CHECK_EQ(updated_other_right->text, "42");
    const auto updated_target_region = body.inspect_table_cell(2U, 1U, 0U);
    REQUIRE(updated_target_region.has_value());
    CHECK_EQ(updated_target_region->text, "South-batch");
    const auto updated_target_qty = body.inspect_table_cell(2U, 1U, 1U);
    REQUIRE(updated_target_qty.has_value());
    CHECK_EQ(updated_target_qty->text, "21");
    const auto updated_target_amount = body.inspect_table_cell(2U, 1U, 2U);
    REQUIRE(updated_target_amount.has_value());
    CHECK_EQ(updated_target_amount->text, "240");

    CHECK_EQ(run_cli({"set-template-tables-from-json",
                      source.string(),
                      "--patch-file",
                      parse_patch.string(),
                      "--json"},
                     parse_output),
             2);
    CHECK_EQ(
        read_text_file(parse_output),
        std::string{
            "{\"command\":\"set-template-tables-from-json\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"operation[0]: cannot combine a "
            "table index or --bookmark with --after-text or --header-cell\"}\n"});

    remove_if_exists(source);
    remove_if_exists(success_patch);
    remove_if_exists(parse_patch);
    remove_if_exists(updated);
    remove_if_exists(success_output);
    remove_if_exists(parse_output);
}

TEST_CASE("cli selector-based template table text mutations target the requested table") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_template_table_selector_direct_text_source.docx";
    const fs::path cell_updated =
        working_directory / "cli_template_table_selector_direct_cell_updated.docx";
    const fs::path multiline_cell_updated =
        working_directory / "cli_template_table_selector_direct_multiline_cell_updated.docx";
    const fs::path row_updated =
        working_directory / "cli_template_table_selector_direct_row_updated.docx";
    const fs::path block_updated =
        working_directory / "cli_template_table_selector_direct_block_updated.docx";
    const fs::path multiline_block_updated =
        working_directory / "cli_template_table_selector_direct_multiline_block_updated.docx";
    const fs::path cell_output =
        working_directory / "cli_template_table_selector_direct_cell_output.json";
    const fs::path multiline_cell_output =
        working_directory / "cli_template_table_selector_direct_multiline_cell_output.json";
    const fs::path row_output =
        working_directory / "cli_template_table_selector_direct_row_output.json";
    const fs::path block_output =
        working_directory / "cli_template_table_selector_direct_block_output.json";
    const fs::path multiline_block_output =
        working_directory / "cli_template_table_selector_direct_multiline_block_output.json";

    remove_if_exists(source);
    remove_if_exists(cell_updated);
    remove_if_exists(multiline_cell_updated);
    remove_if_exists(row_updated);
    remove_if_exists(block_updated);
    remove_if_exists(multiline_block_updated);
    remove_if_exists(cell_output);
    remove_if_exists(multiline_cell_output);
    remove_if_exists(row_output);
    remove_if_exists(block_output);
    remove_if_exists(multiline_block_output);

    create_cli_template_table_selector_fixture(source);

    CHECK_EQ(run_cli({"set-template-table-cell-text",
                      source.string(),
                      "--after-text",
                      "selector target table",
                      "--header-cell",
                      "Region",
                      "--header-cell",
                      "Qty",
                      "--header-cell",
                      "Amount",
                      "--occurrence",
                      "1",
                      "1",
                      "1",
                      "--text",
                      "42",
                      "--output",
                      cell_updated.string(),
                      "--json"},
                     cell_output),
             0);
    const auto cell_json = read_text_file(cell_output);
    CHECK_NE(cell_json.find("\"command\":\"set-template-table-cell-text\""),
             std::string::npos);
    CHECK_NE(cell_json.find("\"table_index\":2"), std::string::npos);
    CHECK_NE(cell_json.find("\"row_index\":1"), std::string::npos);
    CHECK_NE(cell_json.find("\"cell_index\":1"), std::string::npos);

    featherdoc::Document reopened_cell(cell_updated);
    REQUIRE_FALSE(reopened_cell.open());
    auto cell_body = reopened_cell.body_template();
    REQUIRE(static_cast<bool>(cell_body));
    const auto preserved_first_qty = cell_body.inspect_table_cell(0U, 1U, 1U);
    REQUIRE(preserved_first_qty.has_value());
    CHECK_EQ(preserved_first_qty->text, "7");
    const auto updated_target_qty = cell_body.inspect_table_cell(2U, 1U, 1U);
    REQUIRE(updated_target_qty.has_value());
    CHECK_EQ(updated_target_qty->text, "42");

    CHECK_EQ(run_cli({"set-template-table-cell-text",
                      source.string(),
                      "--after-text",
                      "selector target table",
                      "--header-cell",
                      "Region",
                      "--header-cell",
                      "Qty",
                      "--header-cell",
                      "Amount",
                      "--occurrence",
                      "1",
                      "1",
                      "2",
                      "--text",
                      "240 total\nfinance review",
                      "--output",
                      multiline_cell_updated.string(),
                      "--json"},
                     multiline_cell_output),
             0);
    const auto multiline_cell_json = read_text_file(multiline_cell_output);
    CHECK_NE(multiline_cell_json.find("\"command\":\"set-template-table-cell-text\""),
             std::string::npos);
    CHECK_NE(multiline_cell_json.find("\"table_index\":2"), std::string::npos);
    CHECK_NE(multiline_cell_json.find("\"row_index\":1"), std::string::npos);
    CHECK_NE(multiline_cell_json.find("\"cell_index\":2"), std::string::npos);

    featherdoc::Document reopened_multiline_cell(multiline_cell_updated);
    REQUIRE_FALSE(reopened_multiline_cell.open());
    auto multiline_cell_body = reopened_multiline_cell.body_template();
    REQUIRE(static_cast<bool>(multiline_cell_body));
    const auto preserved_first_amount =
        multiline_cell_body.inspect_table_cell(0U, 1U, 2U);
    REQUIRE(preserved_first_amount.has_value());
    CHECK_EQ(preserved_first_amount->text, "12");
    const auto updated_multiline_cell_amount =
        multiline_cell_body.inspect_table_cell(2U, 1U, 2U);
    REQUIRE(updated_multiline_cell_amount.has_value());
    CHECK_EQ(updated_multiline_cell_amount->text, "240 total\nfinance review");

    CHECK_EQ(run_cli({"set-template-table-row-texts",
                      source.string(),
                      "--after-text",
                      "selector target table",
                      "--header-cell",
                      "Region",
                      "--header-cell",
                      "Qty",
                      "--header-cell",
                      "Amount",
                      "--occurrence",
                      "1",
                      "1",
                      "--row",
                      "South revised",
                      "--cell",
                      "18",
                      "--cell",
                      "199",
                      "--output",
                      row_updated.string(),
                      "--json"},
                     row_output),
             0);
    const auto row_json = read_text_file(row_output);
    CHECK_NE(row_json.find("\"command\":\"set-template-table-row-texts\""),
             std::string::npos);
    CHECK_NE(row_json.find("\"table_index\":2"), std::string::npos);
    CHECK_NE(row_json.find("\"start_row_index\":1"), std::string::npos);

    featherdoc::Document reopened_row(row_updated);
    REQUIRE_FALSE(reopened_row.open());
    auto row_body = reopened_row.body_template();
    REQUIRE(static_cast<bool>(row_body));
    const auto preserved_first_region = row_body.inspect_table_cell(0U, 1U, 0U);
    REQUIRE(preserved_first_region.has_value());
    CHECK_EQ(preserved_first_region->text, "North");
    const auto updated_target_region = row_body.inspect_table_cell(2U, 1U, 0U);
    REQUIRE(updated_target_region.has_value());
    CHECK_EQ(updated_target_region->text, "South revised");
    const auto updated_target_amount = row_body.inspect_table_cell(2U, 1U, 2U);
    REQUIRE(updated_target_amount.has_value());
    CHECK_EQ(updated_target_amount->text, "199");

    CHECK_EQ(run_cli({"set-template-table-cell-block-texts",
                      source.string(),
                      "--after-text",
                      "selector target table",
                      "--header-cell",
                      "Region",
                      "--header-cell",
                      "Qty",
                      "--header-cell",
                      "Amount",
                      "--occurrence",
                      "1",
                      "1",
                      "1",
                      "--row",
                      "18",
                      "--cell",
                      "240",
                      "--output",
                      block_updated.string(),
                      "--json"},
                     block_output),
             0);
    const auto block_json = read_text_file(block_output);
    CHECK_NE(block_json.find("\"command\":\"set-template-table-cell-block-texts\""),
             std::string::npos);
    CHECK_NE(block_json.find("\"table_index\":2"), std::string::npos);
    CHECK_NE(block_json.find("\"start_row_index\":1"), std::string::npos);
    CHECK_NE(block_json.find("\"start_cell_index\":1"), std::string::npos);

    featherdoc::Document reopened_block(block_updated);
    REQUIRE_FALSE(reopened_block.open());
    auto block_body = reopened_block.body_template();
    REQUIRE(static_cast<bool>(block_body));
    const auto preserved_target_region = block_body.inspect_table_cell(2U, 1U, 0U);
    REQUIRE(preserved_target_region.has_value());
    CHECK_EQ(preserved_target_region->text, "South");
    const auto updated_block_qty = block_body.inspect_table_cell(2U, 1U, 1U);
    REQUIRE(updated_block_qty.has_value());
    CHECK_EQ(updated_block_qty->text, "18");
    const auto updated_block_amount = block_body.inspect_table_cell(2U, 1U, 2U);
    REQUIRE(updated_block_amount.has_value());
    CHECK_EQ(updated_block_amount->text, "240");

    CHECK_EQ(run_cli({"set-template-table-cell-block-texts",
                      source.string(),
                      "--after-text",
                      "selector target table",
                      "--header-cell",
                      "Region",
                      "--header-cell",
                      "Qty",
                      "--header-cell",
                      "Amount",
                      "--occurrence",
                      "1",
                      "1",
                      "1",
                      "--row",
                      "18 units\npending approval",
                      "--cell",
                      "240 total\nfinance review",
                      "--output",
                      multiline_block_updated.string(),
                      "--json"},
                     multiline_block_output),
             0);
    const auto multiline_block_json = read_text_file(multiline_block_output);
    CHECK_NE(multiline_block_json.find("\"command\":\"set-template-table-cell-block-texts\""),
             std::string::npos);
    CHECK_NE(multiline_block_json.find("\"table_index\":2"), std::string::npos);

    featherdoc::Document reopened_multiline_block(multiline_block_updated);
    REQUIRE_FALSE(reopened_multiline_block.open());
    auto multiline_block_body = reopened_multiline_block.body_template();
    REQUIRE(static_cast<bool>(multiline_block_body));
    const auto updated_multiline_qty =
        multiline_block_body.inspect_table_cell(2U, 1U, 1U);
    REQUIRE(updated_multiline_qty.has_value());
    CHECK_EQ(updated_multiline_qty->text, "18 units\npending approval");
    const auto updated_multiline_amount =
        multiline_block_body.inspect_table_cell(2U, 1U, 2U);
    REQUIRE(updated_multiline_amount.has_value());
    CHECK_EQ(updated_multiline_amount->text, "240 total\nfinance review");

    remove_if_exists(source);
    remove_if_exists(cell_updated);
    remove_if_exists(multiline_cell_updated);
    remove_if_exists(row_updated);
    remove_if_exists(block_updated);
    remove_if_exists(multiline_block_updated);
    remove_if_exists(cell_output);
    remove_if_exists(multiline_cell_output);
    remove_if_exists(row_output);
    remove_if_exists(block_output);
    remove_if_exists(multiline_block_output);
}

TEST_CASE("cli selector-based template table row commands mutate only the matched table") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_template_table_selector_row_commands_source.docx";
    const fs::path appended =
        working_directory / "cli_template_table_selector_row_appended.docx";
    const fs::path inserted_before =
        working_directory / "cli_template_table_selector_row_before.docx";
    const fs::path inserted_after =
        working_directory / "cli_template_table_selector_row_after.docx";
    const fs::path removed =
        working_directory / "cli_template_table_selector_row_removed.docx";
    const fs::path append_output =
        working_directory / "cli_template_table_selector_row_append.json";
    const fs::path before_output =
        working_directory / "cli_template_table_selector_row_before.json";
    const fs::path after_output =
        working_directory / "cli_template_table_selector_row_after.json";
    const fs::path remove_output =
        working_directory / "cli_template_table_selector_row_remove.json";
    const fs::path parse_output =
        working_directory / "cli_template_table_selector_row_parse.json";

    remove_if_exists(source);
    remove_if_exists(appended);
    remove_if_exists(inserted_before);
    remove_if_exists(inserted_after);
    remove_if_exists(removed);
    remove_if_exists(append_output);
    remove_if_exists(before_output);
    remove_if_exists(after_output);
    remove_if_exists(remove_output);
    remove_if_exists(parse_output);

    create_cli_template_table_selector_fixture(source);

    CHECK_EQ(run_cli({"append-template-table-row",
                      source.string(),
                      "--after-text",
                      "selector target table",
                      "--header-cell",
                      "Region",
                      "--header-cell",
                      "Qty",
                      "--header-cell",
                      "Amount",
                      "--occurrence",
                      "1",
                      "--cell-count",
                      "3",
                      "--output",
                      appended.string(),
                      "--json"},
                     append_output),
             0);
    const auto append_json = read_text_file(append_output);
    CHECK_NE(append_json.find("\"command\":\"append-template-table-row\""),
             std::string::npos);
    CHECK_NE(append_json.find("\"table_index\":2"), std::string::npos);
    CHECK_NE(append_json.find("\"row_index\":2"), std::string::npos);

    featherdoc::Document reopened_append(appended);
    REQUIRE_FALSE(reopened_append.open());
    auto append_body = reopened_append.body_template();
    REQUIRE(static_cast<bool>(append_body));
    const auto preserved_first_table = append_body.inspect_table(0U);
    REQUIRE(preserved_first_table.has_value());
    CHECK_EQ(preserved_first_table->row_count, 2U);
    const auto appended_target_table = append_body.inspect_table(2U);
    REQUIRE(appended_target_table.has_value());
    CHECK_EQ(appended_target_table->row_count, 3U);
    const auto appended_target_cell = append_body.inspect_table_cell(2U, 2U, 0U);
    REQUIRE(appended_target_cell.has_value());
    CHECK_EQ(appended_target_cell->text, "");

    CHECK_EQ(run_cli({"insert-template-table-row-before",
                      source.string(),
                      "--after-text",
                      "selector target table",
                      "--header-cell",
                      "Region",
                      "--header-cell",
                      "Qty",
                      "--header-cell",
                      "Amount",
                      "--occurrence",
                      "1",
                      "1",
                      "--output",
                      inserted_before.string(),
                      "--json"},
                     before_output),
             0);
    const auto before_json = read_text_file(before_output);
    CHECK_NE(before_json.find("\"command\":\"insert-template-table-row-before\""),
             std::string::npos);
    CHECK_NE(before_json.find("\"table_index\":2"), std::string::npos);
    CHECK_NE(before_json.find("\"inserted_row_index\":1"), std::string::npos);

    featherdoc::Document reopened_before(inserted_before);
    REQUIRE_FALSE(reopened_before.open());
    auto before_body = reopened_before.body_template();
    REQUIRE(static_cast<bool>(before_body));
    const auto before_target_table = before_body.inspect_table(2U);
    REQUIRE(before_target_table.has_value());
    CHECK_EQ(before_target_table->row_count, 3U);
    const auto before_inserted_cell = before_body.inspect_table_cell(2U, 1U, 0U);
    REQUIRE(before_inserted_cell.has_value());
    CHECK_EQ(before_inserted_cell->text, "");
    const auto before_shifted_cell = before_body.inspect_table_cell(2U, 2U, 0U);
    REQUIRE(before_shifted_cell.has_value());
    CHECK_EQ(before_shifted_cell->text, "South");

    CHECK_EQ(run_cli({"insert-template-table-row-after",
                      source.string(),
                      "--after-text",
                      "selector target table",
                      "--header-cell",
                      "Region",
                      "--header-cell",
                      "Qty",
                      "--header-cell",
                      "Amount",
                      "--occurrence",
                      "1",
                      "0",
                      "--output",
                      inserted_after.string(),
                      "--json"},
                     after_output),
             0);
    const auto after_json = read_text_file(after_output);
    CHECK_NE(after_json.find("\"command\":\"insert-template-table-row-after\""),
             std::string::npos);
    CHECK_NE(after_json.find("\"table_index\":2"), std::string::npos);
    CHECK_NE(after_json.find("\"inserted_row_index\":1"), std::string::npos);

    featherdoc::Document reopened_after(inserted_after);
    REQUIRE_FALSE(reopened_after.open());
    auto after_body = reopened_after.body_template();
    REQUIRE(static_cast<bool>(after_body));
    const auto after_target_table = after_body.inspect_table(2U);
    REQUIRE(after_target_table.has_value());
    CHECK_EQ(after_target_table->row_count, 3U);
    const auto after_inserted_cell = after_body.inspect_table_cell(2U, 1U, 0U);
    REQUIRE(after_inserted_cell.has_value());
    CHECK_EQ(after_inserted_cell->text, "");
    const auto after_shifted_cell = after_body.inspect_table_cell(2U, 2U, 0U);
    REQUIRE(after_shifted_cell.has_value());
    CHECK_EQ(after_shifted_cell->text, "South");

    CHECK_EQ(run_cli({"remove-template-table-row",
                      source.string(),
                      "--after-text",
                      "selector target table",
                      "--header-cell",
                      "Region",
                      "--header-cell",
                      "Qty",
                      "--header-cell",
                      "Amount",
                      "--occurrence",
                      "1",
                      "1",
                      "--output",
                      removed.string(),
                      "--json"},
                     remove_output),
             0);
    const auto remove_json = read_text_file(remove_output);
    CHECK_NE(remove_json.find("\"command\":\"remove-template-table-row\""),
             std::string::npos);
    CHECK_NE(remove_json.find("\"table_index\":2"), std::string::npos);
    CHECK_NE(remove_json.find("\"row_index\":1"), std::string::npos);

    featherdoc::Document reopened_remove(removed);
    REQUIRE_FALSE(reopened_remove.open());
    auto remove_body = reopened_remove.body_template();
    REQUIRE(static_cast<bool>(remove_body));
    const auto removed_target_table = remove_body.inspect_table(2U);
    REQUIRE(removed_target_table.has_value());
    CHECK_EQ(removed_target_table->row_count, 1U);
    const auto removed_header_cell = remove_body.inspect_table_cell(2U, 0U, 0U);
    REQUIRE(removed_header_cell.has_value());
    CHECK_EQ(removed_header_cell->text, "Region");

    CHECK_EQ(run_cli({"insert-template-table-row-before",
                      source.string(),
                      "--after-text",
                      "selector target table",
                      "--header-cell",
                      "Region",
                      "--header-cell",
                      "Qty",
                      "--header-cell",
                      "Amount",
                      "--occurrence",
                      "1",
                      "--json"},
                     parse_output),
             2);
    CHECK_EQ(
        read_text_file(parse_output),
        std::string{
            "{\"command\":\"insert-template-table-row-before\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"missing row index after table "
            "selector\"}\n"});

    remove_if_exists(source);
    remove_if_exists(appended);
    remove_if_exists(inserted_before);
    remove_if_exists(inserted_after);
    remove_if_exists(removed);
    remove_if_exists(append_output);
    remove_if_exists(before_output);
    remove_if_exists(after_output);
    remove_if_exists(remove_output);
    remove_if_exists(parse_output);
}

TEST_CASE("cli selector-based template table column commands mutate only the matched table") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_template_table_selector_column_source.docx";
    const fs::path inserted_before =
        working_directory / "cli_template_table_selector_column_before.docx";
    const fs::path inserted_after =
        working_directory / "cli_template_table_selector_column_after.docx";
    const fs::path removed =
        working_directory / "cli_template_table_selector_column_removed.docx";
    const fs::path before_output =
        working_directory / "cli_template_table_selector_column_before.json";
    const fs::path after_output =
        working_directory / "cli_template_table_selector_column_after.json";
    const fs::path remove_output =
        working_directory / "cli_template_table_selector_column_removed.json";

    remove_if_exists(source);
    remove_if_exists(inserted_before);
    remove_if_exists(inserted_after);
    remove_if_exists(removed);
    remove_if_exists(before_output);
    remove_if_exists(after_output);
    remove_if_exists(remove_output);

    create_cli_template_table_selector_fixture(source);

    CHECK_EQ(run_cli({"insert-template-table-column-before",
                      source.string(),
                      "--after-text",
                      "selector target table",
                      "--header-cell",
                      "Region",
                      "--header-cell",
                      "Qty",
                      "--header-cell",
                      "Amount",
                      "--occurrence",
                      "1",
                      "0",
                      "1",
                      "--output",
                      inserted_before.string(),
                      "--json"},
                     before_output),
             0);
    const auto before_json = read_text_file(before_output);
    CHECK_NE(before_json.find("\"command\":\"insert-template-table-column-before\""),
             std::string::npos);
    CHECK_NE(before_json.find("\"table_index\":2"), std::string::npos);
    CHECK_NE(before_json.find("\"inserted_column_index\":1"),
             std::string::npos);

    featherdoc::Document reopened_before(inserted_before);
    REQUIRE_FALSE(reopened_before.open());
    auto before_body = reopened_before.body_template();
    REQUIRE(static_cast<bool>(before_body));
    const auto before_first_table = before_body.inspect_table(0U);
    REQUIRE(before_first_table.has_value());
    CHECK_EQ(before_first_table->column_count, 3U);
    const auto before_target_table = before_body.inspect_table(2U);
    REQUIRE(before_target_table.has_value());
    CHECK_EQ(before_target_table->column_count, 4U);
    const auto before_inserted_header = before_body.inspect_table_cell(2U, 0U, 1U);
    REQUIRE(before_inserted_header.has_value());
    CHECK_EQ(before_inserted_header->text, "");
    const auto before_shifted_header = before_body.inspect_table_cell(2U, 0U, 2U);
    REQUIRE(before_shifted_header.has_value());
    CHECK_EQ(before_shifted_header->text, "Qty");

    CHECK_EQ(run_cli({"insert-template-table-column-after",
                      source.string(),
                      "--after-text",
                      "selector target table",
                      "--header-cell",
                      "Region",
                      "--header-cell",
                      "Qty",
                      "--header-cell",
                      "Amount",
                      "--occurrence",
                      "1",
                      "0",
                      "0",
                      "--output",
                      inserted_after.string(),
                      "--json"},
                     after_output),
             0);
    const auto after_json = read_text_file(after_output);
    CHECK_NE(after_json.find("\"command\":\"insert-template-table-column-after\""),
             std::string::npos);
    CHECK_NE(after_json.find("\"table_index\":2"), std::string::npos);
    CHECK_NE(after_json.find("\"inserted_column_index\":1"),
             std::string::npos);

    featherdoc::Document reopened_after(inserted_after);
    REQUIRE_FALSE(reopened_after.open());
    auto after_body = reopened_after.body_template();
    REQUIRE(static_cast<bool>(after_body));
    const auto after_target_table = after_body.inspect_table(2U);
    REQUIRE(after_target_table.has_value());
    CHECK_EQ(after_target_table->column_count, 4U);
    const auto after_inserted_header = after_body.inspect_table_cell(2U, 0U, 1U);
    REQUIRE(after_inserted_header.has_value());
    CHECK_EQ(after_inserted_header->text, "");
    const auto after_shifted_header = after_body.inspect_table_cell(2U, 0U, 2U);
    REQUIRE(after_shifted_header.has_value());
    CHECK_EQ(after_shifted_header->text, "Qty");

    CHECK_EQ(run_cli({"remove-template-table-column",
                      source.string(),
                      "--after-text",
                      "selector target table",
                      "--header-cell",
                      "Region",
                      "--header-cell",
                      "Qty",
                      "--header-cell",
                      "Amount",
                      "--occurrence",
                      "1",
                      "0",
                      "1",
                      "--output",
                      removed.string(),
                      "--json"},
                     remove_output),
             0);
    const auto remove_json = read_text_file(remove_output);
    CHECK_NE(remove_json.find("\"command\":\"remove-template-table-column\""),
             std::string::npos);
    CHECK_NE(remove_json.find("\"table_index\":2"), std::string::npos);
    CHECK_NE(remove_json.find("\"column_index\":1"), std::string::npos);

    featherdoc::Document reopened_remove(removed);
    REQUIRE_FALSE(reopened_remove.open());
    auto remove_body = reopened_remove.body_template();
    REQUIRE(static_cast<bool>(remove_body));
    const auto removed_target_table = remove_body.inspect_table(2U);
    REQUIRE(removed_target_table.has_value());
    CHECK_EQ(removed_target_table->column_count, 2U);
    const auto removed_header = remove_body.inspect_table_cell(2U, 0U, 1U);
    REQUIRE(removed_header.has_value());
    CHECK_EQ(removed_header->text, "Amount");
    const auto removed_value = remove_body.inspect_table_cell(2U, 1U, 1U);
    REQUIRE(removed_value.has_value());
    CHECK_EQ(removed_value->text, "24");

    remove_if_exists(source);
    remove_if_exists(inserted_before);
    remove_if_exists(inserted_after);
    remove_if_exists(removed);
    remove_if_exists(before_output);
    remove_if_exists(after_output);
    remove_if_exists(remove_output);
}

} // namespace
