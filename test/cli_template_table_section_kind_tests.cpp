#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "cli_template_table_test_support.hpp"

namespace {
TEST_CASE("cli set-template-table-cell-text updates section kind template cells") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_set_section_kind_template_cell_text_source.docx";
    const fs::path even_updated =
        working_directory / "cli_set_section_kind_even_header_updated.docx";
    const fs::path first_updated =
        working_directory / "cli_set_section_kind_first_footer_updated.docx";
    const fs::path even_output =
        working_directory / "cli_set_section_kind_even_header_output.json";
    const fs::path first_output =
        working_directory / "cli_set_section_kind_first_footer_output.json";

    remove_if_exists(source);
    remove_if_exists(even_updated);
    remove_if_exists(first_updated);
    remove_if_exists(even_output);
    remove_if_exists(first_output);

    create_cli_template_table_section_kind_fixture(source);

    CHECK_EQ(run_cli({"set-template-table-cell-text",
                      source.string(),
                      "0",
                      "1",
                      "1",
                      "--part",
                      "section-header",
                      "--section",
                      "0",
                      "--kind",
                      "even",
                      "--text",
                      "updated even header cell",
                      "--output",
                      even_updated.string(),
                      "--json"},
                     even_output),
             0);
    const auto even_json = read_text_file(even_output);
    CHECK_NE(even_json.find("\"command\":\"set-template-table-cell-text\""),
             std::string::npos);
    CHECK_NE(even_json.find("\"part\":\"section-header\""),
             std::string::npos);
    CHECK_NE(even_json.find("\"section\":0"), std::string::npos);
    CHECK_NE(even_json.find("\"kind\":\"even\""), std::string::npos);

    featherdoc::Document reopened_even(even_updated);
    REQUIRE_FALSE(reopened_even.open());
    auto even_header_template = reopened_even.section_header_template(
        0U, featherdoc::section_reference_kind::even_page);
    REQUIRE(static_cast<bool>(even_header_template));
    const auto even_cell =
        even_header_template.inspect_table_cell(0U, 1U, 1U);
    REQUIRE(even_cell.has_value());
    CHECK_EQ(even_cell->text, "updated even header cell");

    CHECK_EQ(run_cli({"set-template-table-cell-text",
                      source.string(),
                      "0",
                      "1",
                      "1",
                      "--part",
                      "section-footer",
                      "--section",
                      "0",
                      "--kind",
                      "first",
                      "--text",
                      "updated first footer cell",
                      "--output",
                      first_updated.string(),
                      "--json"},
                     first_output),
             0);
    const auto first_json = read_text_file(first_output);
    CHECK_NE(first_json.find("\"command\":\"set-template-table-cell-text\""),
             std::string::npos);
    CHECK_NE(first_json.find("\"part\":\"section-footer\""),
             std::string::npos);
    CHECK_NE(first_json.find("\"section\":0"), std::string::npos);
    CHECK_NE(first_json.find("\"kind\":\"first\""), std::string::npos);

    featherdoc::Document reopened_first(first_updated);
    REQUIRE_FALSE(reopened_first.open());
    auto first_footer_template = reopened_first.section_footer_template(
        0U, featherdoc::section_reference_kind::first_page);
    REQUIRE(static_cast<bool>(first_footer_template));
    const auto first_cell =
        first_footer_template.inspect_table_cell(0U, 1U, 1U);
    REQUIRE(first_cell.has_value());
    CHECK_EQ(first_cell->text, "updated first footer cell");

    remove_if_exists(source);
    remove_if_exists(even_updated);
    remove_if_exists(first_updated);
    remove_if_exists(even_output);
    remove_if_exists(first_output);
}

TEST_CASE("cli template table row commands mutate section kind template tables") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_section_kind_template_table_row_source.docx";
    const fs::path even_appended =
        working_directory / "cli_section_kind_template_table_even_appended.docx";
    const fs::path first_removed =
        working_directory / "cli_section_kind_template_table_first_removed.docx";
    const fs::path append_output =
        working_directory / "cli_section_kind_template_table_even_append.json";
    const fs::path remove_output =
        working_directory / "cli_section_kind_template_table_first_remove.json";

    remove_if_exists(source);
    remove_if_exists(even_appended);
    remove_if_exists(first_removed);
    remove_if_exists(append_output);
    remove_if_exists(remove_output);

    create_cli_template_table_section_kind_fixture(source);

    CHECK_EQ(run_cli({"append-template-table-row",
                      source.string(),
                      "0",
                      "--part",
                      "section-header",
                      "--section",
                      "0",
                      "--kind",
                      "even",
                      "--output",
                      even_appended.string(),
                      "--json"},
                     append_output),
             0);
    const auto append_json = read_text_file(append_output);
    CHECK_NE(append_json.find("\"command\":\"append-template-table-row\""),
             std::string::npos);
    CHECK_NE(append_json.find("\"part\":\"section-header\""),
             std::string::npos);
    CHECK_NE(append_json.find("\"section\":0"), std::string::npos);
    CHECK_NE(append_json.find("\"kind\":\"even\""), std::string::npos);
    CHECK_NE(append_json.find("\"row_index\":2"), std::string::npos);

    featherdoc::Document reopened_even(even_appended);
    REQUIRE_FALSE(reopened_even.open());
    auto even_header_template = reopened_even.section_header_template(
        0U, featherdoc::section_reference_kind::even_page);
    REQUIRE(static_cast<bool>(even_header_template));
    const auto appended_table = even_header_template.inspect_table(0U);
    REQUIRE(appended_table.has_value());
    CHECK_EQ(appended_table->row_count, 3U);
    const auto appended_cell =
        even_header_template.inspect_table_cell(0U, 2U, 0U);
    REQUIRE(appended_cell.has_value());
    CHECK_EQ(appended_cell->text, "");

    CHECK_EQ(run_cli({"remove-template-table-row",
                      source.string(),
                      "0",
                      "0",
                      "--part",
                      "section-footer",
                      "--section",
                      "0",
                      "--kind",
                      "first",
                      "--output",
                      first_removed.string(),
                      "--json"},
                     remove_output),
             0);
    const auto remove_json = read_text_file(remove_output);
    CHECK_NE(remove_json.find("\"command\":\"remove-template-table-row\""),
             std::string::npos);
    CHECK_NE(remove_json.find("\"part\":\"section-footer\""),
             std::string::npos);
    CHECK_NE(remove_json.find("\"section\":0"), std::string::npos);
    CHECK_NE(remove_json.find("\"kind\":\"first\""), std::string::npos);
    CHECK_NE(remove_json.find("\"row_index\":0"), std::string::npos);

    featherdoc::Document reopened_first(first_removed);
    REQUIRE_FALSE(reopened_first.open());
    auto first_footer_template = reopened_first.section_footer_template(
        0U, featherdoc::section_reference_kind::first_page);
    REQUIRE(static_cast<bool>(first_footer_template));
    const auto removed_table = first_footer_template.inspect_table(0U);
    REQUIRE(removed_table.has_value());
    CHECK_EQ(removed_table->row_count, 1U);
    const auto removed_first_cell =
        first_footer_template.inspect_table_cell(0U, 0U, 0U);
    REQUIRE(removed_first_cell.has_value());
    CHECK_EQ(removed_first_cell->text, "ff10");

    remove_if_exists(source);
    remove_if_exists(even_appended);
    remove_if_exists(first_removed);
    remove_if_exists(append_output);
    remove_if_exists(remove_output);
}

TEST_CASE("cli template table column commands mutate section kind template tables") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_section_kind_template_table_column_source.docx";
    const fs::path even_inserted =
        working_directory / "cli_section_kind_template_table_even_column_before.docx";
    const fs::path first_removed =
        working_directory / "cli_section_kind_template_table_first_column_removed.docx";
    const fs::path before_output =
        working_directory / "cli_section_kind_template_table_even_column_before.json";
    const fs::path remove_output =
        working_directory / "cli_section_kind_template_table_first_column_removed.json";

    remove_if_exists(source);
    remove_if_exists(even_inserted);
    remove_if_exists(first_removed);
    remove_if_exists(before_output);
    remove_if_exists(remove_output);

    create_cli_template_table_section_kind_fixture(source);

    CHECK_EQ(run_cli({"insert-template-table-column-before",
                      source.string(),
                      "0",
                      "0",
                      "1",
                      "--part",
                      "section-header",
                      "--section",
                      "0",
                      "--kind",
                      "even",
                      "--output",
                      even_inserted.string(),
                      "--json"},
                     before_output),
             0);
    const auto before_json = read_text_file(before_output);
    CHECK_NE(before_json.find("\"command\":\"insert-template-table-column-before\""),
             std::string::npos);
    CHECK_NE(before_json.find("\"part\":\"section-header\""),
             std::string::npos);
    CHECK_NE(before_json.find("\"section\":0"), std::string::npos);
    CHECK_NE(before_json.find("\"kind\":\"even\""), std::string::npos);
    CHECK_NE(before_json.find("\"inserted_cell_index\":1"),
             std::string::npos);
    CHECK_NE(before_json.find("\"inserted_column_index\":1"),
             std::string::npos);

    featherdoc::Document reopened_even(even_inserted);
    REQUIRE_FALSE(reopened_even.open());
    auto even_header_template = reopened_even.section_header_template(
        0U, featherdoc::section_reference_kind::even_page);
    REQUIRE(static_cast<bool>(even_header_template));
    const auto even_table = even_header_template.inspect_table(0U);
    REQUIRE(even_table.has_value());
    CHECK_EQ(even_table->column_count, 3U);
    REQUIRE(even_table->column_widths.size() >= 3U);
    CHECK(even_table->column_widths[0].has_value());
    CHECK(even_table->column_widths[1].has_value());
    CHECK(even_table->column_widths[2].has_value());
    CHECK_EQ(*even_table->column_widths[0], 1800U);
    CHECK_EQ(*even_table->column_widths[1], 3600U);
    CHECK_EQ(*even_table->column_widths[2], 3600U);
    const auto even_inserted_header =
        even_header_template.inspect_table_cell(0U, 0U, 1U);
    REQUIRE(even_inserted_header.has_value());
    CHECK_EQ(even_inserted_header->text, "");
    const auto even_shifted_header =
        even_header_template.inspect_table_cell(0U, 0U, 2U);
    REQUIRE(even_shifted_header.has_value());
    CHECK_EQ(even_shifted_header->text, "eh01");
    const auto even_inserted_body =
        even_header_template.inspect_table_cell(0U, 1U, 1U);
    REQUIRE(even_inserted_body.has_value());
    CHECK_EQ(even_inserted_body->text, "");

    CHECK_EQ(run_cli({"remove-template-table-column",
                      source.string(),
                      "0",
                      "0",
                      "1",
                      "--part",
                      "section-footer",
                      "--section",
                      "0",
                      "--kind",
                      "first",
                      "--output",
                      first_removed.string(),
                      "--json"},
                     remove_output),
             0);
    const auto remove_json = read_text_file(remove_output);
    CHECK_NE(remove_json.find("\"command\":\"remove-template-table-column\""),
             std::string::npos);
    CHECK_NE(remove_json.find("\"part\":\"section-footer\""),
             std::string::npos);
    CHECK_NE(remove_json.find("\"section\":0"), std::string::npos);
    CHECK_NE(remove_json.find("\"kind\":\"first\""), std::string::npos);
    CHECK_NE(remove_json.find("\"column_index\":1"), std::string::npos);

    featherdoc::Document reopened_first(first_removed);
    REQUIRE_FALSE(reopened_first.open());
    auto first_footer_template = reopened_first.section_footer_template(
        0U, featherdoc::section_reference_kind::first_page);
    REQUIRE(static_cast<bool>(first_footer_template));
    const auto first_table = first_footer_template.inspect_table(0U);
    REQUIRE(first_table.has_value());
    CHECK_EQ(first_table->column_count, 1U);
    const auto first_header_cell =
        first_footer_template.inspect_table_cell(0U, 0U, 0U);
    REQUIRE(first_header_cell.has_value());
    CHECK_EQ(first_header_cell->text, "ff00");
    const auto first_body_cell =
        first_footer_template.inspect_table_cell(0U, 1U, 0U);
    REQUIRE(first_body_cell.has_value());
    CHECK_EQ(first_body_cell->text, "ff10");

    remove_if_exists(source);
    remove_if_exists(even_inserted);
    remove_if_exists(first_removed);
    remove_if_exists(before_output);
    remove_if_exists(remove_output);
}

TEST_CASE("cli template table merge and unmerge commands mutate section kind template tables") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_section_kind_template_table_merge_source.docx";
    const fs::path even_merged =
        working_directory / "cli_section_kind_template_table_even_merged.docx";
    const fs::path first_unmerged =
        working_directory /
        "cli_section_kind_template_table_first_unmerged.docx";
    const fs::path merge_output =
        working_directory / "cli_section_kind_template_table_even_merge.json";
    const fs::path unmerge_output =
        working_directory /
        "cli_section_kind_template_table_first_unmerge.json";

    remove_if_exists(source);
    remove_if_exists(even_merged);
    remove_if_exists(first_unmerged);
    remove_if_exists(merge_output);
    remove_if_exists(unmerge_output);

    create_cli_template_table_section_kind_merge_unmerge_fixture(source);

    CHECK_EQ(run_cli({"merge-template-table-cells",
                      source.string(),
                      "0",
                      "0",
                      "0",
                      "--part",
                      "section-header",
                      "--section",
                      "0",
                      "--kind",
                      "even",
                      "--direction",
                      "right",
                      "--count",
                      "1",
                      "--output",
                      even_merged.string(),
                      "--json"},
                     merge_output),
             0);
    const auto merge_json = read_text_file(merge_output);
    CHECK_NE(merge_json.find("\"command\":\"merge-template-table-cells\""),
             std::string::npos);
    CHECK_NE(merge_json.find("\"part\":\"section-header\""),
             std::string::npos);
    CHECK_NE(merge_json.find("\"section\":0"), std::string::npos);
    CHECK_NE(merge_json.find("\"kind\":\"even\""), std::string::npos);
    CHECK_NE(merge_json.find("\"direction\":\"right\""), std::string::npos);
    CHECK_NE(merge_json.find("\"count\":1"), std::string::npos);

    featherdoc::Document reopened_even(even_merged);
    REQUIRE_FALSE(reopened_even.open());
    auto even_header_template = reopened_even.section_header_template(
        0U, featherdoc::section_reference_kind::even_page);
    REQUIRE(static_cast<bool>(even_header_template));
    const auto merged_even_cell =
        even_header_template.inspect_table_cell(0U, 0U, 0U);
    REQUIRE(merged_even_cell.has_value());
    CHECK_EQ(merged_even_cell->text, "eh00");
    CHECK_EQ(merged_even_cell->column_span, 2U);
    const auto merged_even_tail =
        even_header_template.inspect_table_cell(0U, 0U, 1U);
    REQUIRE(merged_even_tail.has_value());
    CHECK_EQ(merged_even_tail->text, "eh02");
    CHECK_EQ(merged_even_tail->column_index, 2U);

    CHECK_EQ(run_cli({"unmerge-template-table-cells",
                      source.string(),
                      "0",
                      "1",
                      "0",
                      "--part",
                      "section-footer",
                      "--section",
                      "0",
                      "--kind",
                      "first",
                      "--direction",
                      "down",
                      "--output",
                      first_unmerged.string(),
                      "--json"},
                     unmerge_output),
             0);
    const auto unmerge_json = read_text_file(unmerge_output);
    CHECK_NE(unmerge_json.find("\"command\":\"unmerge-template-table-cells\""),
             std::string::npos);
    CHECK_NE(unmerge_json.find("\"part\":\"section-footer\""),
             std::string::npos);
    CHECK_NE(unmerge_json.find("\"section\":0"), std::string::npos);
    CHECK_NE(unmerge_json.find("\"kind\":\"first\""), std::string::npos);
    CHECK_NE(unmerge_json.find("\"direction\":\"down\""), std::string::npos);

    featherdoc::Document reopened_first(first_unmerged);
    REQUIRE_FALSE(reopened_first.open());
    auto first_footer_template = reopened_first.section_footer_template(
        0U, featherdoc::section_reference_kind::first_page);
    REQUIRE(static_cast<bool>(first_footer_template));
    const auto first_top_cell =
        first_footer_template.inspect_table_cell(0U, 0U, 0U);
    REQUIRE(first_top_cell.has_value());
    CHECK_EQ(first_top_cell->text, "ff00");
    const auto first_middle_cell =
        first_footer_template.inspect_table_cell(0U, 1U, 0U);
    REQUIRE(first_middle_cell.has_value());
    CHECK_EQ(first_middle_cell->text, "");
    const auto first_bottom_cell =
        first_footer_template.inspect_table_cell(0U, 2U, 0U);
    REQUIRE(first_bottom_cell.has_value());
    CHECK_EQ(first_bottom_cell->text, "");

    remove_if_exists(source);
    remove_if_exists(even_merged);
    remove_if_exists(first_unmerged);
    remove_if_exists(merge_output);
    remove_if_exists(unmerge_output);
}

} // namespace
