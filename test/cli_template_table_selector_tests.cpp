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


} // namespace
