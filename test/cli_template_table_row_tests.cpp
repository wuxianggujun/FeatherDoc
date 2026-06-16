#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "cli_template_table_test_support.hpp"

namespace {
TEST_CASE("cli template table row commands mutate header template tables") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_template_table_row_source.docx";
    const fs::path appended =
        working_directory / "cli_template_table_row_appended.docx";
    const fs::path inserted_before =
        working_directory / "cli_template_table_row_before.docx";
    const fs::path inserted_after =
        working_directory / "cli_template_table_row_after.docx";
    const fs::path removed =
        working_directory / "cli_template_table_row_removed.docx";
    const fs::path append_output =
        working_directory / "cli_template_table_row_append.json";
    const fs::path before_output =
        working_directory / "cli_template_table_row_before.json";
    const fs::path after_output =
        working_directory / "cli_template_table_row_after.json";
    const fs::path remove_output =
        working_directory / "cli_template_table_row_remove.json";

    remove_if_exists(source);
    remove_if_exists(appended);
    remove_if_exists(inserted_before);
    remove_if_exists(inserted_after);
    remove_if_exists(removed);
    remove_if_exists(append_output);
    remove_if_exists(before_output);
    remove_if_exists(after_output);
    remove_if_exists(remove_output);

    create_cli_template_inspection_fixture(source);

    CHECK_EQ(run_cli({"append-template-table-row",
                      source.string(),
                      "0",
                      "--part",
                      "header",
                      "--index",
                      "0",
                      "--output",
                      appended.string(),
                      "--json"},
                     append_output),
             0);
    const auto append_json = read_text_file(append_output);
    CHECK_NE(append_json.find("\"command\":\"append-template-table-row\""),
             std::string::npos);
    CHECK_NE(append_json.find("\"part\":\"header\""), std::string::npos);
    CHECK_NE(append_json.find("\"part_index\":0"), std::string::npos);
    CHECK_NE(append_json.find("\"entry_name\":\"word/header1.xml\""),
             std::string::npos);
    CHECK_NE(append_json.find("\"table_index\":0"), std::string::npos);
    CHECK_NE(append_json.find("\"row_index\":2"), std::string::npos);
    CHECK_NE(append_json.find("\"cell_count\":2"), std::string::npos);

    featherdoc::Document reopened_appended(appended);
    REQUIRE_FALSE(reopened_appended.open());
    auto appended_header = reopened_appended.header_template(0U);
    REQUIRE(static_cast<bool>(appended_header));
    const auto appended_table = appended_header.inspect_table(0U);
    REQUIRE(appended_table.has_value());
    CHECK_EQ(appended_table->row_count, 3U);
    CHECK_EQ(appended_table->column_count, 2U);
    const auto appended_first_cell = appended_header.inspect_table_cell(0U, 2U, 0U);
    REQUIRE(appended_first_cell.has_value());
    CHECK_EQ(appended_first_cell->text, "");
    const auto appended_second_cell = appended_header.inspect_table_cell(0U, 2U, 1U);
    REQUIRE(appended_second_cell.has_value());
    CHECK_EQ(appended_second_cell->text, "");

    CHECK_EQ(run_cli({"insert-template-table-row-before",
                      source.string(),
                      "0",
                      "1",
                      "--part",
                      "header",
                      "--index",
                      "0",
                      "--output",
                      inserted_before.string(),
                      "--json"},
                     before_output),
             0);
    const auto before_json = read_text_file(before_output);
    CHECK_NE(before_json.find("\"command\":\"insert-template-table-row-before\""),
             std::string::npos);
    CHECK_NE(before_json.find("\"inserted_row_index\":1"), std::string::npos);

    featherdoc::Document reopened_before(inserted_before);
    REQUIRE_FALSE(reopened_before.open());
    auto before_header = reopened_before.header_template(0U);
    REQUIRE(static_cast<bool>(before_header));
    const auto before_table = before_header.inspect_table(0U);
    REQUIRE(before_table.has_value());
    CHECK_EQ(before_table->row_count, 3U);
    const auto before_inserted_first_cell =
        before_header.inspect_table_cell(0U, 1U, 0U);
    REQUIRE(before_inserted_first_cell.has_value());
    CHECK_EQ(before_inserted_first_cell->text, "");
    const auto before_inserted_second_cell =
        before_header.inspect_table_cell(0U, 1U, 1U);
    REQUIRE(before_inserted_second_cell.has_value());
    CHECK_EQ(before_inserted_second_cell->text, "");
    const auto before_shifted_first_cell =
        before_header.inspect_table_cell(0U, 2U, 0U);
    REQUIRE(before_shifted_first_cell.has_value());
    CHECK_EQ(before_shifted_first_cell->text, "h10");

    CHECK_EQ(run_cli({"insert-template-table-row-after",
                      source.string(),
                      "0",
                      "0",
                      "--part",
                      "header",
                      "--index",
                      "0",
                      "--output",
                      inserted_after.string(),
                      "--json"},
                     after_output),
             0);
    const auto after_json = read_text_file(after_output);
    CHECK_NE(after_json.find("\"command\":\"insert-template-table-row-after\""),
             std::string::npos);
    CHECK_NE(after_json.find("\"inserted_row_index\":1"), std::string::npos);

    featherdoc::Document reopened_after(inserted_after);
    REQUIRE_FALSE(reopened_after.open());
    auto after_header = reopened_after.header_template(0U);
    REQUIRE(static_cast<bool>(after_header));
    const auto after_table = after_header.inspect_table(0U);
    REQUIRE(after_table.has_value());
    CHECK_EQ(after_table->row_count, 3U);
    const auto after_inserted_first_cell =
        after_header.inspect_table_cell(0U, 1U, 0U);
    REQUIRE(after_inserted_first_cell.has_value());
    CHECK_EQ(after_inserted_first_cell->text, "");
    const auto after_inserted_second_cell =
        after_header.inspect_table_cell(0U, 1U, 1U);
    REQUIRE(after_inserted_second_cell.has_value());
    CHECK_EQ(after_inserted_second_cell->text, "");
    const auto after_shifted_first_cell =
        after_header.inspect_table_cell(0U, 2U, 0U);
    REQUIRE(after_shifted_first_cell.has_value());
    CHECK_EQ(after_shifted_first_cell->text, "h10");

    CHECK_EQ(run_cli({"remove-template-table-row",
                      source.string(),
                      "0",
                      "0",
                      "--part",
                      "header",
                      "--index",
                      "0",
                      "--output",
                      removed.string(),
                      "--json"},
                     remove_output),
             0);
    const auto remove_json = read_text_file(remove_output);
    CHECK_NE(remove_json.find("\"command\":\"remove-template-table-row\""),
             std::string::npos);
    CHECK_NE(remove_json.find("\"table_index\":0"), std::string::npos);
    CHECK_NE(remove_json.find("\"row_index\":0"), std::string::npos);

    featherdoc::Document reopened_removed(removed);
    REQUIRE_FALSE(reopened_removed.open());
    auto removed_header = reopened_removed.header_template(0U);
    REQUIRE(static_cast<bool>(removed_header));
    const auto removed_table = removed_header.inspect_table(0U);
    REQUIRE(removed_table.has_value());
    CHECK_EQ(removed_table->row_count, 1U);
    CHECK_EQ(removed_table->column_count, 2U);
    const auto removed_first_cell = removed_header.inspect_table_cell(0U, 0U, 0U);
    REQUIRE(removed_first_cell.has_value());
    CHECK_EQ(removed_first_cell->text, "h10");
    const auto removed_second_cell = removed_header.inspect_table_cell(0U, 0U, 1U);
    REQUIRE(removed_second_cell.has_value());
    CHECK_EQ(removed_second_cell->text, "h11");

    remove_if_exists(source);
    remove_if_exists(appended);
    remove_if_exists(inserted_before);
    remove_if_exists(inserted_after);
    remove_if_exists(removed);
    remove_if_exists(append_output);
    remove_if_exists(before_output);
    remove_if_exists(after_output);
    remove_if_exists(remove_output);
}

TEST_CASE("cli template table row commands mutate direct footer template tables") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_direct_footer_template_table_row_source.docx";
    const fs::path appended =
        working_directory / "cli_direct_footer_template_table_row_appended.docx";
    const fs::path inserted_before =
        working_directory / "cli_direct_footer_template_table_row_before.docx";
    const fs::path inserted_after =
        working_directory / "cli_direct_footer_template_table_row_after.docx";
    const fs::path removed =
        working_directory / "cli_direct_footer_template_table_row_removed.docx";
    const fs::path append_output =
        working_directory / "cli_direct_footer_template_table_row_append.json";
    const fs::path before_output =
        working_directory / "cli_direct_footer_template_table_row_before.json";
    const fs::path after_output =
        working_directory / "cli_direct_footer_template_table_row_after.json";
    const fs::path remove_output =
        working_directory / "cli_direct_footer_template_table_row_remove.json";

    remove_if_exists(source);
    remove_if_exists(appended);
    remove_if_exists(inserted_before);
    remove_if_exists(inserted_after);
    remove_if_exists(removed);
    remove_if_exists(append_output);
    remove_if_exists(before_output);
    remove_if_exists(after_output);
    remove_if_exists(remove_output);

    create_cli_template_table_direct_column_fixture(source);

    CHECK_EQ(run_cli({"append-template-table-row",
                      source.string(),
                      "0",
                      "--part",
                      "footer",
                      "--index",
                      "0",
                      "--output",
                      appended.string(),
                      "--json"},
                     append_output),
             0);
    const auto append_json = read_text_file(append_output);
    CHECK_NE(append_json.find("\"command\":\"append-template-table-row\""),
             std::string::npos);
    CHECK_NE(append_json.find("\"part\":\"footer\""), std::string::npos);
    CHECK_NE(append_json.find("\"part_index\":0"), std::string::npos);
    CHECK_NE(append_json.find("\"entry_name\":\"word/footer1.xml\""),
             std::string::npos);
    CHECK_NE(append_json.find("\"table_index\":0"), std::string::npos);
    CHECK_NE(append_json.find("\"row_index\":2"), std::string::npos);
    CHECK_NE(append_json.find("\"cell_count\":2"), std::string::npos);

    featherdoc::Document reopened_appended(appended);
    REQUIRE_FALSE(reopened_appended.open());
    auto appended_footer = reopened_appended.footer_template(0U);
    REQUIRE(static_cast<bool>(appended_footer));
    const auto appended_table = appended_footer.inspect_table(0U);
    REQUIRE(appended_table.has_value());
    CHECK_EQ(appended_table->row_count, 3U);
    CHECK_EQ(appended_table->column_count, 2U);
    const auto appended_first_cell = appended_footer.inspect_table_cell(0U, 2U, 0U);
    REQUIRE(appended_first_cell.has_value());
    CHECK_EQ(appended_first_cell->text, "");
    const auto appended_second_cell = appended_footer.inspect_table_cell(0U, 2U, 1U);
    REQUIRE(appended_second_cell.has_value());
    CHECK_EQ(appended_second_cell->text, "");

    CHECK_EQ(run_cli({"insert-template-table-row-before",
                      source.string(),
                      "0",
                      "1",
                      "--part",
                      "footer",
                      "--index",
                      "0",
                      "--output",
                      inserted_before.string(),
                      "--json"},
                     before_output),
             0);
    const auto before_json = read_text_file(before_output);
    CHECK_NE(before_json.find("\"command\":\"insert-template-table-row-before\""),
             std::string::npos);
    CHECK_NE(before_json.find("\"inserted_row_index\":1"), std::string::npos);

    featherdoc::Document reopened_before(inserted_before);
    REQUIRE_FALSE(reopened_before.open());
    auto before_footer = reopened_before.footer_template(0U);
    REQUIRE(static_cast<bool>(before_footer));
    const auto before_table = before_footer.inspect_table(0U);
    REQUIRE(before_table.has_value());
    CHECK_EQ(before_table->row_count, 3U);
    const auto before_inserted_first_cell =
        before_footer.inspect_table_cell(0U, 1U, 0U);
    REQUIRE(before_inserted_first_cell.has_value());
    CHECK_EQ(before_inserted_first_cell->text, "");
    const auto before_inserted_second_cell =
        before_footer.inspect_table_cell(0U, 1U, 1U);
    REQUIRE(before_inserted_second_cell.has_value());
    CHECK_EQ(before_inserted_second_cell->text, "");
    const auto before_shifted_first_cell =
        before_footer.inspect_table_cell(0U, 2U, 0U);
    REQUIRE(before_shifted_first_cell.has_value());
    CHECK_EQ(before_shifted_first_cell->text, "f10");

    CHECK_EQ(run_cli({"insert-template-table-row-after",
                      source.string(),
                      "0",
                      "0",
                      "--part",
                      "footer",
                      "--index",
                      "0",
                      "--output",
                      inserted_after.string(),
                      "--json"},
                     after_output),
             0);
    const auto after_json = read_text_file(after_output);
    CHECK_NE(after_json.find("\"command\":\"insert-template-table-row-after\""),
             std::string::npos);
    CHECK_NE(after_json.find("\"inserted_row_index\":1"), std::string::npos);

    featherdoc::Document reopened_after(inserted_after);
    REQUIRE_FALSE(reopened_after.open());
    auto after_footer = reopened_after.footer_template(0U);
    REQUIRE(static_cast<bool>(after_footer));
    const auto after_table = after_footer.inspect_table(0U);
    REQUIRE(after_table.has_value());
    CHECK_EQ(after_table->row_count, 3U);
    const auto after_inserted_first_cell =
        after_footer.inspect_table_cell(0U, 1U, 0U);
    REQUIRE(after_inserted_first_cell.has_value());
    CHECK_EQ(after_inserted_first_cell->text, "");
    const auto after_inserted_second_cell =
        after_footer.inspect_table_cell(0U, 1U, 1U);
    REQUIRE(after_inserted_second_cell.has_value());
    CHECK_EQ(after_inserted_second_cell->text, "");
    const auto after_shifted_first_cell =
        after_footer.inspect_table_cell(0U, 2U, 0U);
    REQUIRE(after_shifted_first_cell.has_value());
    CHECK_EQ(after_shifted_first_cell->text, "f10");

    CHECK_EQ(run_cli({"remove-template-table-row",
                      source.string(),
                      "0",
                      "0",
                      "--part",
                      "footer",
                      "--index",
                      "0",
                      "--output",
                      removed.string(),
                      "--json"},
                     remove_output),
             0);
    const auto remove_json = read_text_file(remove_output);
    CHECK_NE(remove_json.find("\"command\":\"remove-template-table-row\""),
             std::string::npos);
    CHECK_NE(remove_json.find("\"part\":\"footer\""), std::string::npos);
    CHECK_NE(remove_json.find("\"part_index\":0"), std::string::npos);
    CHECK_NE(remove_json.find("\"table_index\":0"), std::string::npos);
    CHECK_NE(remove_json.find("\"row_index\":0"), std::string::npos);

    featherdoc::Document reopened_removed(removed);
    REQUIRE_FALSE(reopened_removed.open());
    auto removed_footer = reopened_removed.footer_template(0U);
    REQUIRE(static_cast<bool>(removed_footer));
    const auto removed_table = removed_footer.inspect_table(0U);
    REQUIRE(removed_table.has_value());
    CHECK_EQ(removed_table->row_count, 1U);
    CHECK_EQ(removed_table->column_count, 2U);
    const auto removed_first_cell = removed_footer.inspect_table_cell(0U, 0U, 0U);
    REQUIRE(removed_first_cell.has_value());
    CHECK_EQ(removed_first_cell->text, "f10");
    const auto removed_second_cell = removed_footer.inspect_table_cell(0U, 0U, 1U);
    REQUIRE(removed_second_cell.has_value());
    CHECK_EQ(removed_second_cell->text, "f11");

    remove_if_exists(source);
    remove_if_exists(appended);
    remove_if_exists(inserted_before);
    remove_if_exists(inserted_after);
    remove_if_exists(removed);
    remove_if_exists(append_output);
    remove_if_exists(before_output);
    remove_if_exists(after_output);
    remove_if_exists(remove_output);
}

TEST_CASE("cli template table row commands report parse and mutate errors") {
    const fs::path working_directory = fs::current_path();
    const fs::path parse_source =
        working_directory / "cli_template_table_row_parse_source.docx";
    const fs::path merged_source =
        working_directory / "cli_template_table_row_merge_source.docx";
    const fs::path single_row_source =
        working_directory / "cli_template_table_row_single_source.docx";
    const fs::path parse_output =
        working_directory / "cli_template_table_row_parse.json";
    const fs::path merge_error_output =
        working_directory / "cli_template_table_row_merge_error.json";
    const fs::path remove_error_output =
        working_directory / "cli_template_table_row_remove_error.json";

    remove_if_exists(parse_source);
    remove_if_exists(merged_source);
    remove_if_exists(single_row_source);
    remove_if_exists(parse_output);
    remove_if_exists(merge_error_output);
    remove_if_exists(remove_error_output);

    create_cli_template_inspection_fixture(parse_source);

    CHECK_EQ(run_cli({"append-template-table-row",
                      parse_source.string(),
                      "0",
                      "--part",
                      "section-header",
                      "--json"},
                     parse_output),
             2);
    CHECK_EQ(
        read_text_file(parse_output),
        std::string{
            "{\"command\":\"append-template-table-row\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"section-header/section-footer "
            "mutation requires --section <index>\"}\n"});

    featherdoc::Document merged_document(merged_source);
    REQUIRE_FALSE(merged_document.create_empty());
    auto &merged_header = merged_document.ensure_header_paragraphs();
    REQUIRE(merged_header.has_next());
    auto merged_template = merged_document.header_template(0U);
    REQUIRE(static_cast<bool>(merged_template));
    auto merged_table = merged_template.append_table(3U, 1U);
    REQUIRE(merged_table.has_next());
    auto merged_row = merged_table.rows();
    REQUIRE(merged_row.has_next());
    auto merged_cell = merged_row.cells();
    REQUIRE(merged_cell.has_next());
    REQUIRE(merged_cell.set_text("merged-root"));
    REQUIRE(merged_cell.merge_down(2U));
    REQUIRE_FALSE(merged_document.save());

    CHECK_EQ(run_cli({"insert-template-table-row-after",
                      merged_source.string(),
                      "0",
                      "0",
                      "--part",
                      "header",
                      "--index",
                      "0",
                      "--json"},
                     merge_error_output),
             1);
    const auto merge_error_json = read_text_file(merge_error_output);
    CHECK_NE(merge_error_json.find("\"command\":\"insert-template-table-row-after\""),
             std::string::npos);
    CHECK_NE(merge_error_json.find("\"stage\":\"mutate\""), std::string::npos);
    CHECK_NE(merge_error_json.find("\"entry\":\"word/header1.xml\""),
             std::string::npos);
    CHECK_NE(
        merge_error_json.find(
            "\"detail\":\"cannot insert a row adjacent to row index '0' in "
            "table index '0' because the row participates in a vertical "
            "merge\""),
        std::string::npos);

    featherdoc::Document single_row_document(single_row_source);
    REQUIRE_FALSE(single_row_document.create_empty());
    auto &single_header = single_row_document.ensure_header_paragraphs();
    REQUIRE(single_header.has_next());
    auto single_template = single_row_document.header_template(0U);
    REQUIRE(static_cast<bool>(single_template));
    auto single_table = single_template.append_table(1U, 1U);
    REQUIRE(single_table.has_next());
    auto single_row = single_table.rows();
    REQUIRE(single_row.has_next());
    auto only_cell = single_row.cells();
    REQUIRE(only_cell.has_next());
    REQUIRE(only_cell.set_text("only-row"));
    REQUIRE_FALSE(single_row_document.save());

    CHECK_EQ(run_cli({"remove-template-table-row",
                      single_row_source.string(),
                      "0",
                      "0",
                      "--part",
                      "header",
                      "--index",
                      "0",
                      "--json"},
                     remove_error_output),
             1);
    const auto remove_error_json = read_text_file(remove_error_output);
    CHECK_NE(remove_error_json.find("\"command\":\"remove-template-table-row\""),
             std::string::npos);
    CHECK_NE(remove_error_json.find("\"stage\":\"mutate\""), std::string::npos);
    CHECK_NE(remove_error_json.find("\"entry\":\"word/header1.xml\""),
             std::string::npos);
    CHECK_NE(
        remove_error_json.find(
            "\"detail\":\"cannot remove the last row from table index '0'\""),
        std::string::npos);

    remove_if_exists(parse_source);
    remove_if_exists(merged_source);
    remove_if_exists(single_row_source);
    remove_if_exists(parse_output);
    remove_if_exists(merge_error_output);
    remove_if_exists(remove_error_output);
}

} // namespace
