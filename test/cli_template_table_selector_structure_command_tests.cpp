#include "cli_template_table_test_support.hpp"

namespace {
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
