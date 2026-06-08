#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "cli_template_table_test_support.hpp"

namespace {
TEST_CASE("cli template table column commands mutate section and body template tables") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_template_table_column_source.docx";
    const fs::path inserted_before =
        working_directory / "cli_template_table_column_before.docx";
    const fs::path inserted_after =
        working_directory / "cli_template_table_column_after.docx";
    const fs::path removed =
        working_directory / "cli_template_table_column_removed.docx";
    const fs::path before_output =
        working_directory / "cli_template_table_column_before.json";
    const fs::path after_output =
        working_directory / "cli_template_table_column_after.json";
    const fs::path remove_output =
        working_directory / "cli_template_table_column_removed.json";

    remove_if_exists(source);
    remove_if_exists(inserted_before);
    remove_if_exists(inserted_after);
    remove_if_exists(removed);
    remove_if_exists(before_output);
    remove_if_exists(after_output);
    remove_if_exists(remove_output);

    create_cli_template_table_column_fixture(source);

    CHECK_EQ(run_cli({"insert-template-table-column-before",
                      source.string(),
                      "0",
                      "0",
                      "1",
                      "--part",
                      "section-header",
                      "--section",
                      "0",
                      "--output",
                      inserted_before.string(),
                      "--json"},
                     before_output),
             0);
    const auto before_json = read_text_file(before_output);
    CHECK_NE(before_json.find("\"command\":\"insert-template-table-column-before\""),
             std::string::npos);
    CHECK_NE(before_json.find("\"part\":\"section-header\""), std::string::npos);
    CHECK_NE(before_json.find("\"section\":0"), std::string::npos);
    CHECK_NE(before_json.find("\"entry_name\":\"word/header1.xml\""),
             std::string::npos);
    CHECK_NE(before_json.find("\"inserted_cell_index\":1"),
             std::string::npos);
    CHECK_NE(before_json.find("\"inserted_column_index\":1"),
             std::string::npos);

    featherdoc::Document reopened_before(inserted_before);
    REQUIRE_FALSE(reopened_before.open());
    auto before_template = reopened_before.section_header_template(0U);
    REQUIRE(static_cast<bool>(before_template));
    const auto before_table = before_template.inspect_table(0U);
    REQUIRE(before_table.has_value());
    CHECK_EQ(before_table->column_count, 3U);
    REQUIRE(before_table->column_widths.size() >= 3U);
    CHECK(before_table->column_widths[0].has_value());
    CHECK(before_table->column_widths[1].has_value());
    CHECK(before_table->column_widths[2].has_value());
    CHECK_EQ(*before_table->column_widths[0], 1800U);
    CHECK_EQ(*before_table->column_widths[1], 3600U);
    CHECK_EQ(*before_table->column_widths[2], 3600U);
    const auto before_inserted_header =
        before_template.inspect_table_cell(0U, 0U, 1U);
    REQUIRE(before_inserted_header.has_value());
    CHECK_EQ(before_inserted_header->text, "");
    const auto before_shifted_header =
        before_template.inspect_table_cell(0U, 0U, 2U);
    REQUIRE(before_shifted_header.has_value());
    CHECK_EQ(before_shifted_header->text, "h01");
    const auto before_inserted_body =
        before_template.inspect_table_cell(0U, 1U, 1U);
    REQUIRE(before_inserted_body.has_value());
    CHECK_EQ(before_inserted_body->text, "");

    CHECK_EQ(run_cli({"insert-template-table-column-after",
                      source.string(),
                      "0",
                      "0",
                      "0",
                      "--part",
                      "section-footer",
                      "--section",
                      "0",
                      "--output",
                      inserted_after.string(),
                      "--json"},
                     after_output),
             0);
    const auto after_json = read_text_file(after_output);
    CHECK_NE(after_json.find("\"command\":\"insert-template-table-column-after\""),
             std::string::npos);
    CHECK_NE(after_json.find("\"part\":\"section-footer\""), std::string::npos);
    CHECK_NE(after_json.find("\"section\":0"), std::string::npos);
    CHECK_NE(after_json.find("\"entry_name\":\"word/footer1.xml\""),
             std::string::npos);
    CHECK_NE(after_json.find("\"inserted_cell_index\":1"),
             std::string::npos);
    CHECK_NE(after_json.find("\"inserted_column_index\":1"),
             std::string::npos);

    featherdoc::Document reopened_after(inserted_after);
    REQUIRE_FALSE(reopened_after.open());
    auto after_template = reopened_after.section_footer_template(0U);
    REQUIRE(static_cast<bool>(after_template));
    const auto after_table = after_template.inspect_table(0U);
    REQUIRE(after_table.has_value());
    CHECK_EQ(after_table->column_count, 3U);
    REQUIRE(after_table->column_widths.size() >= 3U);
    CHECK(after_table->column_widths[0].has_value());
    CHECK(after_table->column_widths[1].has_value());
    CHECK(after_table->column_widths[2].has_value());
    CHECK_EQ(*after_table->column_widths[0], 1600U);
    CHECK_EQ(*after_table->column_widths[1], 1600U);
    CHECK_EQ(*after_table->column_widths[2], 2800U);
    const auto after_inserted_header =
        after_template.inspect_table_cell(0U, 0U, 1U);
    REQUIRE(after_inserted_header.has_value());
    CHECK_EQ(after_inserted_header->text, "");
    const auto after_shifted_header =
        after_template.inspect_table_cell(0U, 0U, 2U);
    REQUIRE(after_shifted_header.has_value());
    CHECK_EQ(after_shifted_header->text, "f01");
    const auto after_inserted_body =
        after_template.inspect_table_cell(0U, 1U, 1U);
    REQUIRE(after_inserted_body.has_value());
    CHECK_EQ(after_inserted_body->text, "");

    CHECK_EQ(run_cli({"remove-template-table-column",
                      source.string(),
                      "0",
                      "0",
                      "1",
                      "--part",
                      "body",
                      "--output",
                      removed.string(),
                      "--json"},
                     remove_output),
             0);
    const auto remove_json = read_text_file(remove_output);
    CHECK_NE(remove_json.find("\"command\":\"remove-template-table-column\""),
             std::string::npos);
    CHECK_NE(remove_json.find("\"part\":\"body\""), std::string::npos);
    CHECK_NE(remove_json.find("\"entry_name\":\"word/document.xml\""),
             std::string::npos);
    CHECK_NE(remove_json.find("\"column_index\":1"), std::string::npos);

    featherdoc::Document reopened_removed(removed);
    REQUIRE_FALSE(reopened_removed.open());
    auto removed_template = reopened_removed.body_template();
    REQUIRE(static_cast<bool>(removed_template));
    const auto removed_table = removed_template.inspect_table(0U);
    REQUIRE(removed_table.has_value());
    CHECK_EQ(removed_table->column_count, 2U);
    const auto removed_first_header =
        removed_template.inspect_table_cell(0U, 0U, 0U);
    REQUIRE(removed_first_header.has_value());
    CHECK_EQ(removed_first_header->text, "b00");
    const auto removed_second_header =
        removed_template.inspect_table_cell(0U, 0U, 1U);
    REQUIRE(removed_second_header.has_value());
    CHECK_EQ(removed_second_header->text, "b02");
    const auto removed_second_body =
        removed_template.inspect_table_cell(0U, 1U, 1U);
    REQUIRE(removed_second_body.has_value());
    CHECK_EQ(removed_second_body->text, "b12");

    remove_if_exists(source);
    remove_if_exists(inserted_before);
    remove_if_exists(inserted_after);
    remove_if_exists(removed);
    remove_if_exists(before_output);
    remove_if_exists(after_output);
    remove_if_exists(remove_output);
}

TEST_CASE("cli template table column commands mutate direct header and footer template tables") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_template_table_direct_column_source.docx";
    const fs::path inserted_before =
        working_directory / "cli_template_table_direct_column_before.docx";
    const fs::path inserted_after =
        working_directory / "cli_template_table_direct_column_after.docx";
    const fs::path removed =
        working_directory / "cli_template_table_direct_column_removed.docx";
    const fs::path before_output =
        working_directory / "cli_template_table_direct_column_before.json";
    const fs::path after_output =
        working_directory / "cli_template_table_direct_column_after.json";
    const fs::path remove_output =
        working_directory / "cli_template_table_direct_column_removed.json";

    remove_if_exists(source);
    remove_if_exists(inserted_before);
    remove_if_exists(inserted_after);
    remove_if_exists(removed);
    remove_if_exists(before_output);
    remove_if_exists(after_output);
    remove_if_exists(remove_output);

    create_cli_template_table_direct_column_fixture(source);

    CHECK_EQ(run_cli({"insert-template-table-column-before",
                      source.string(),
                      "0",
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
    CHECK_NE(before_json.find("\"command\":\"insert-template-table-column-before\""),
             std::string::npos);
    CHECK_NE(before_json.find("\"part\":\"header\""), std::string::npos);
    CHECK_NE(before_json.find("\"part_index\":0"), std::string::npos);
    CHECK_NE(before_json.find("\"entry_name\":\"word/header1.xml\""),
             std::string::npos);
    CHECK_NE(before_json.find("\"inserted_cell_index\":1"),
             std::string::npos);
    CHECK_NE(before_json.find("\"inserted_column_index\":1"),
             std::string::npos);

    featherdoc::Document reopened_before(inserted_before);
    REQUIRE_FALSE(reopened_before.open());
    auto before_template = reopened_before.header_template(0U);
    REQUIRE(static_cast<bool>(before_template));
    const auto before_table = before_template.inspect_table(0U);
    REQUIRE(before_table.has_value());
    CHECK_EQ(before_table->column_count, 3U);
    REQUIRE(before_table->column_widths.size() >= 3U);
    CHECK(before_table->column_widths[0].has_value());
    CHECK(before_table->column_widths[1].has_value());
    CHECK(before_table->column_widths[2].has_value());
    CHECK_EQ(*before_table->column_widths[0], 1800U);
    CHECK_EQ(*before_table->column_widths[1], 3600U);
    CHECK_EQ(*before_table->column_widths[2], 3600U);
    const auto before_inserted_header =
        before_template.inspect_table_cell(0U, 0U, 1U);
    REQUIRE(before_inserted_header.has_value());
    CHECK_EQ(before_inserted_header->text, "");
    const auto before_shifted_header =
        before_template.inspect_table_cell(0U, 0U, 2U);
    REQUIRE(before_shifted_header.has_value());
    CHECK_EQ(before_shifted_header->text, "h01");
    const auto before_inserted_body =
        before_template.inspect_table_cell(0U, 1U, 1U);
    REQUIRE(before_inserted_body.has_value());
    CHECK_EQ(before_inserted_body->text, "");

    CHECK_EQ(run_cli({"insert-template-table-column-after",
                      source.string(),
                      "0",
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
    CHECK_NE(after_json.find("\"command\":\"insert-template-table-column-after\""),
             std::string::npos);
    CHECK_NE(after_json.find("\"part\":\"footer\""), std::string::npos);
    CHECK_NE(after_json.find("\"part_index\":0"), std::string::npos);
    CHECK_NE(after_json.find("\"entry_name\":\"word/footer1.xml\""),
             std::string::npos);
    CHECK_NE(after_json.find("\"inserted_cell_index\":1"),
             std::string::npos);
    CHECK_NE(after_json.find("\"inserted_column_index\":1"),
             std::string::npos);

    featherdoc::Document reopened_after(inserted_after);
    REQUIRE_FALSE(reopened_after.open());
    auto after_template = reopened_after.footer_template(0U);
    REQUIRE(static_cast<bool>(after_template));
    const auto after_table = after_template.inspect_table(0U);
    REQUIRE(after_table.has_value());
    CHECK_EQ(after_table->column_count, 3U);
    REQUIRE(after_table->column_widths.size() >= 3U);
    CHECK(after_table->column_widths[0].has_value());
    CHECK(after_table->column_widths[1].has_value());
    CHECK(after_table->column_widths[2].has_value());
    CHECK_EQ(*after_table->column_widths[0], 1600U);
    CHECK_EQ(*after_table->column_widths[1], 1600U);
    CHECK_EQ(*after_table->column_widths[2], 2800U);
    const auto after_inserted_header =
        after_template.inspect_table_cell(0U, 0U, 1U);
    REQUIRE(after_inserted_header.has_value());
    CHECK_EQ(after_inserted_header->text, "");
    const auto after_shifted_header =
        after_template.inspect_table_cell(0U, 0U, 2U);
    REQUIRE(after_shifted_header.has_value());
    CHECK_EQ(after_shifted_header->text, "f01");
    const auto after_inserted_body =
        after_template.inspect_table_cell(0U, 1U, 1U);
    REQUIRE(after_inserted_body.has_value());
    CHECK_EQ(after_inserted_body->text, "");

    CHECK_EQ(run_cli({"remove-template-table-column",
                      source.string(),
                      "0",
                      "0",
                      "1",
                      "--part",
                      "body",
                      "--output",
                      removed.string(),
                      "--json"},
                     remove_output),
             0);
    const auto remove_json = read_text_file(remove_output);
    CHECK_NE(remove_json.find("\"command\":\"remove-template-table-column\""),
             std::string::npos);
    CHECK_NE(remove_json.find("\"part\":\"body\""), std::string::npos);
    CHECK_NE(remove_json.find("\"entry_name\":\"word/document.xml\""),
             std::string::npos);
    CHECK_NE(remove_json.find("\"column_index\":1"), std::string::npos);

    featherdoc::Document reopened_removed(removed);
    REQUIRE_FALSE(reopened_removed.open());
    auto removed_template = reopened_removed.body_template();
    REQUIRE(static_cast<bool>(removed_template));
    const auto removed_table = removed_template.inspect_table(0U);
    REQUIRE(removed_table.has_value());
    CHECK_EQ(removed_table->column_count, 2U);
    const auto removed_first_header =
        removed_template.inspect_table_cell(0U, 0U, 0U);
    REQUIRE(removed_first_header.has_value());
    CHECK_EQ(removed_first_header->text, "b00");
    const auto removed_second_header =
        removed_template.inspect_table_cell(0U, 0U, 1U);
    REQUIRE(removed_second_header.has_value());
    CHECK_EQ(removed_second_header->text, "b02");
    const auto removed_second_body =
        removed_template.inspect_table_cell(0U, 1U, 1U);
    REQUIRE(removed_second_body.has_value());
    CHECK_EQ(removed_second_body->text, "b12");

    remove_if_exists(source);
    remove_if_exists(inserted_before);
    remove_if_exists(inserted_after);
    remove_if_exists(removed);
    remove_if_exists(before_output);
    remove_if_exists(after_output);
    remove_if_exists(remove_output);
}

TEST_CASE("cli template table column commands report parse and mutate errors") {
    const fs::path working_directory = fs::current_path();
    const fs::path parse_source =
        working_directory / "cli_template_table_column_parse_source.docx";
    const fs::path merged_source =
        working_directory / "cli_template_table_column_merge_source.docx";
    const fs::path single_column_source =
        working_directory / "cli_template_table_column_single_source.docx";
    const fs::path parse_output =
        working_directory / "cli_template_table_column_parse.json";
    const fs::path merge_error_output =
        working_directory / "cli_template_table_column_merge_error.json";
    const fs::path remove_error_output =
        working_directory / "cli_template_table_column_remove_error.json";

    remove_if_exists(parse_source);
    remove_if_exists(merged_source);
    remove_if_exists(single_column_source);
    remove_if_exists(parse_output);
    remove_if_exists(merge_error_output);
    remove_if_exists(remove_error_output);

    create_cli_template_table_column_fixture(parse_source);

    CHECK_EQ(run_cli({"insert-template-table-column-before",
                      parse_source.string(),
                      "0",
                      "0",
                      "1",
                      "--part",
                      "section-header",
                      "--json"},
                     parse_output),
             2);
    CHECK_EQ(
        read_text_file(parse_output),
        std::string{
            "{\"command\":\"insert-template-table-column-before\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"section-header/section-footer "
            "mutation requires --section <index>\"}\n"});

    featherdoc::Document merged_document(merged_source);
    REQUIRE_FALSE(merged_document.create_empty());
    auto &merged_header = merged_document.ensure_section_header_paragraphs(0U);
    REQUIRE(merged_header.has_next());
    REQUIRE(merged_header.set_text("Section header merged column fixture"));
    auto merged_template = merged_document.section_header_template(0U);
    REQUIRE(static_cast<bool>(merged_template));
    auto merged_table = merged_template.append_table(2U, 3U);
    configure_cli_template_table_fixture(merged_table, {1600U, 1800U, 2200U},
                                         {{"h00", "h01", "h02"},
                                          {"m00", "m01", "m02"}});
    auto merged_row = merged_table.rows();
    REQUIRE(merged_row.has_next());
    merged_row.next();
    REQUIRE(merged_row.has_next());
    auto merged_cell = merged_row.cells();
    REQUIRE(merged_cell.has_next());
    REQUIRE(merged_cell.merge_right(1U));
    REQUIRE_FALSE(merged_document.save());

    CHECK_EQ(run_cli({"insert-template-table-column-before",
                      merged_source.string(),
                      "0",
                      "0",
                      "1",
                      "--part",
                      "section-header",
                      "--section",
                      "0",
                      "--json"},
                     merge_error_output),
             1);
    const auto merge_error_json = read_text_file(merge_error_output);
    CHECK_NE(
        merge_error_json.find("\"command\":\"insert-template-table-column-before\""),
        std::string::npos);
    CHECK_NE(merge_error_json.find("\"stage\":\"mutate\""), std::string::npos);
    CHECK_NE(merge_error_json.find("\"entry\":\"word/header1.xml\""),
             std::string::npos);
    CHECK_NE(
        merge_error_json.find(
            "\"detail\":\"cannot insert a column before cell index '1' at row "
            "index '0' in table index '0' because the insertion boundary "
            "intersects a horizontal merge span\""),
        std::string::npos);

    featherdoc::Document single_column_document(single_column_source);
    REQUIRE_FALSE(single_column_document.create_empty());
    auto body_template = single_column_document.body_template();
    REQUIRE(static_cast<bool>(body_template));
    auto body_paragraph = body_template.paragraphs();
    REQUIRE(body_paragraph.has_next());
    REQUIRE(body_paragraph.set_text("Single-column body fixture"));
    auto single_table = body_template.append_table(1U, 1U);
    configure_cli_template_table_fixture(single_table, {2400U}, {{"only"}});
    REQUIRE_FALSE(single_column_document.save());

    CHECK_EQ(run_cli({"remove-template-table-column",
                      single_column_source.string(),
                      "0",
                      "0",
                      "0",
                      "--part",
                      "body",
                      "--json"},
                     remove_error_output),
             1);
    const auto remove_error_json = read_text_file(remove_error_output);
    CHECK_NE(remove_error_json.find("\"command\":\"remove-template-table-column\""),
             std::string::npos);
    CHECK_NE(remove_error_json.find("\"stage\":\"mutate\""), std::string::npos);
    CHECK_NE(remove_error_json.find("\"entry\":\"word/document.xml\""),
             std::string::npos);
    CHECK_NE(
        remove_error_json.find(
            "\"detail\":\"cannot remove the last column from table index '0'\""),
        std::string::npos);

    remove_if_exists(parse_source);
    remove_if_exists(merged_source);
    remove_if_exists(single_column_source);
    remove_if_exists(parse_output);
    remove_if_exists(merge_error_output);
    remove_if_exists(remove_error_output);
}

} // namespace
