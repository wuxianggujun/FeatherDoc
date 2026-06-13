#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "cli_template_table_test_support.hpp"

namespace {
TEST_CASE("cli selector-based template table cell inspection resolves the requested table") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_template_table_selector_cells_source.docx";
    const fs::path cell_output =
        working_directory / "cli_template_table_selector_cells_output.json";
    const fs::path parse_output =
        working_directory / "cli_template_table_selector_cells_parse.json";

    remove_if_exists(source);
    remove_if_exists(cell_output);
    remove_if_exists(parse_output);

    create_cli_template_table_selector_fixture(source);

    CHECK_EQ(run_cli({"inspect-template-table-cells",
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
                      "--row",
                      "1",
                      "--cell",
                      "2",
                      "--json"},
                     cell_output),
             0);
    const auto cell_json = read_text_file(cell_output);
    CHECK_NE(cell_json.find("\"table_index\":2"), std::string::npos);
    CHECK_NE(cell_json.find("\"row_index\":1"), std::string::npos);
    CHECK_NE(cell_json.find("\"cell_index\":2"), std::string::npos);
    CHECK_NE(cell_json.find("\"text\":\"24\""), std::string::npos);

    CHECK_EQ(run_cli({"inspect-template-table-cells",
                      source.string(),
                      "--after-text",
                      "selector target table",
                      "--header-row",
                      "1",
                      "--row",
                      "1",
                      "--json"},
                     parse_output),
             2);
    CHECK_EQ(
        read_text_file(parse_output),
        std::string{
            "{\"command\":\"inspect-template-table-cells\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"--header-row requires at least "
            "one --header-cell\"}\n"});

    remove_if_exists(source);
    remove_if_exists(cell_output);
    remove_if_exists(parse_output);
}

TEST_CASE(
    "cli selector-based template table merge and unmerge commands target the requested table") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_template_table_selector_merge_source.docx";
    const fs::path merged =
        working_directory / "cli_template_table_selector_merged.docx";
    const fs::path merged_multiline =
        working_directory / "cli_template_table_selector_merged_multiline.docx";
    const fs::path unmerged =
        working_directory / "cli_template_table_selector_unmerged.docx";
    const fs::path unmerged_multiline =
        working_directory / "cli_template_table_selector_unmerged_multiline.docx";
    const fs::path merge_output =
        working_directory / "cli_template_table_selector_merge_output.json";
    const fs::path grid_inspect_output =
        working_directory / "cli_template_table_selector_grid_inspect_output.json";
    const fs::path merge_multiline_output =
        working_directory / "cli_template_table_selector_merge_multiline_output.json";
    const fs::path unmerge_output =
        working_directory / "cli_template_table_selector_unmerge_output.json";
    const fs::path unmerge_multiline_output =
        working_directory / "cli_template_table_selector_unmerge_multiline_output.json";
    const fs::path parse_output =
        working_directory / "cli_template_table_selector_merge_parse.json";

    remove_if_exists(source);
    remove_if_exists(merged);
    remove_if_exists(merged_multiline);
    remove_if_exists(unmerged);
    remove_if_exists(unmerged_multiline);
    remove_if_exists(merge_output);
    remove_if_exists(grid_inspect_output);
    remove_if_exists(merge_multiline_output);
    remove_if_exists(unmerge_output);
    remove_if_exists(unmerge_multiline_output);
    remove_if_exists(parse_output);

    create_cli_template_table_selector_fixture(source);

    CHECK_EQ(run_cli({"merge-template-table-cells",
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
                      "0",
                      "--direction",
                      "right",
                      "--output",
                      merged.string(),
                      "--json"},
                     merge_output),
             0);
    const auto merge_json = read_text_file(merge_output);
    CHECK_NE(merge_json.find("\"command\":\"merge-template-table-cells\""),
             std::string::npos);
    CHECK_NE(merge_json.find("\"table_index\":2"), std::string::npos);
    CHECK_NE(merge_json.find("\"row_index\":1"), std::string::npos);
    CHECK_NE(merge_json.find("\"cell_index\":0"), std::string::npos);
    CHECK_NE(merge_json.find("\"direction\":\"right\""), std::string::npos);
    CHECK_NE(merge_json.find("\"count\":1"), std::string::npos);

    featherdoc::Document reopened_merged(merged);
    REQUIRE_FALSE(reopened_merged.open());
    auto merged_body = reopened_merged.body_template();
    REQUIRE(static_cast<bool>(merged_body));
    const auto preserved_first_target = merged_body.inspect_table_cell(0U, 1U, 0U);
    REQUIRE(preserved_first_target.has_value());
    CHECK_EQ(preserved_first_target->text, "North");
    CHECK_EQ(preserved_first_target->column_span, 1U);
    const auto merged_anchor = merged_body.inspect_table_cell(2U, 1U, 0U);
    REQUIRE(merged_anchor.has_value());
    CHECK_EQ(merged_anchor->text, "South");
    CHECK_EQ(merged_anchor->column_span, 2U);

    CHECK_EQ(run_cli({"inspect-template-table-cells",
                      merged.string(),
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
                      "--row",
                      "1",
                      "--grid-column",
                      "1",
                      "--json"},
                     grid_inspect_output),
             0);
    const auto grid_inspect_json = read_text_file(grid_inspect_output);
    CHECK_NE(grid_inspect_json.find("\"table_index\":2"), std::string::npos);
    CHECK_NE(grid_inspect_json.find("\"row_index\":1"), std::string::npos);
    CHECK_NE(grid_inspect_json.find("\"cell_index\":0"), std::string::npos);
    CHECK_NE(grid_inspect_json.find("\"column_index\":0"), std::string::npos);
    CHECK_NE(grid_inspect_json.find("\"column_span\":2"), std::string::npos);
    CHECK_NE(grid_inspect_json.find("\"text\":\"South\""), std::string::npos);

    CHECK_EQ(run_cli({"set-template-table-cell-text",
                      merged.string(),
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
                      "--grid-column",
                      "1",
                      "--text",
                      "South\npending approval",
                      "--output",
                      merged_multiline.string(),
                      "--json"},
                     merge_multiline_output),
             0);
    const auto merged_multiline_json = read_text_file(merge_multiline_output);
    CHECK_NE(merged_multiline_json.find("\"command\":\"set-template-table-cell-text\""),
             std::string::npos);
    CHECK_NE(merged_multiline_json.find("\"table_index\":2"), std::string::npos);
    CHECK_NE(merged_multiline_json.find("\"row_index\":1"), std::string::npos);
    CHECK_NE(merged_multiline_json.find("\"grid_column\":1"), std::string::npos);
    CHECK_NE(merged_multiline_json.find("\"cell_index\":0"), std::string::npos);
    CHECK_NE(merged_multiline_json.find("\"column_index\":0"), std::string::npos);
    CHECK_NE(merged_multiline_json.find("\"column_span\":2"), std::string::npos);

    featherdoc::Document reopened_merged_multiline(merged_multiline);
    REQUIRE_FALSE(reopened_merged_multiline.open());
    auto merged_multiline_body = reopened_merged_multiline.body_template();
    REQUIRE(static_cast<bool>(merged_multiline_body));
    const auto merged_multiline_anchor =
        merged_multiline_body.inspect_table_cell(2U, 1U, 0U);
    REQUIRE(merged_multiline_anchor.has_value());
    CHECK_EQ(merged_multiline_anchor->text, "South\npending approval");
    CHECK_EQ(merged_multiline_anchor->column_span, 2U);

    CHECK_EQ(run_cli({"unmerge-template-table-cells",
                      merged.string(),
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
                      "0",
                      "--direction",
                      "right",
                      "--output",
                      unmerged.string(),
                      "--json"},
                     unmerge_output),
             0);
    const auto unmerge_json = read_text_file(unmerge_output);
    CHECK_NE(unmerge_json.find("\"command\":\"unmerge-template-table-cells\""),
             std::string::npos);
    CHECK_NE(unmerge_json.find("\"table_index\":2"), std::string::npos);
    CHECK_NE(unmerge_json.find("\"row_index\":1"), std::string::npos);
    CHECK_NE(unmerge_json.find("\"cell_index\":0"), std::string::npos);
    CHECK_NE(unmerge_json.find("\"direction\":\"right\""), std::string::npos);

    featherdoc::Document reopened_unmerged(unmerged);
    REQUIRE_FALSE(reopened_unmerged.open());
    auto unmerged_body = reopened_unmerged.body_template();
    REQUIRE(static_cast<bool>(unmerged_body));
    const auto restored_anchor = unmerged_body.inspect_table_cell(2U, 1U, 0U);
    REQUIRE(restored_anchor.has_value());
    CHECK_EQ(restored_anchor->text, "South");
    CHECK_EQ(restored_anchor->column_span, 1U);
    const auto restored_cell = unmerged_body.inspect_table_cell(2U, 1U, 1U);
    REQUIRE(restored_cell.has_value());
    CHECK_EQ(restored_cell->text, "");
    CHECK_EQ(restored_cell->column_span, 1U);

    CHECK_EQ(run_cli({"set-template-table-cell-text",
                      unmerged.string(),
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
                      "9 units\nhold",
                      "--output",
                      unmerged_multiline.string(),
                      "--json"},
                     unmerge_multiline_output),
             0);
    const auto unmerged_multiline_json = read_text_file(unmerge_multiline_output);
    CHECK_NE(unmerged_multiline_json.find("\"command\":\"set-template-table-cell-text\""),
             std::string::npos);
    CHECK_NE(unmerged_multiline_json.find("\"table_index\":2"), std::string::npos);
    CHECK_NE(unmerged_multiline_json.find("\"row_index\":1"), std::string::npos);
    CHECK_NE(unmerged_multiline_json.find("\"cell_index\":1"), std::string::npos);

    featherdoc::Document reopened_unmerged_multiline(unmerged_multiline);
    REQUIRE_FALSE(reopened_unmerged_multiline.open());
    auto unmerged_multiline_body = reopened_unmerged_multiline.body_template();
    REQUIRE(static_cast<bool>(unmerged_multiline_body));
    const auto restored_multiline_anchor =
        unmerged_multiline_body.inspect_table_cell(2U, 1U, 0U);
    REQUIRE(restored_multiline_anchor.has_value());
    CHECK_EQ(restored_multiline_anchor->text, "South");
    CHECK_EQ(restored_multiline_anchor->column_span, 1U);
    const auto restored_multiline_cell =
        unmerged_multiline_body.inspect_table_cell(2U, 1U, 1U);
    REQUIRE(restored_multiline_cell.has_value());
    CHECK_EQ(restored_multiline_cell->text, "9 units\nhold");
    CHECK_EQ(restored_multiline_cell->column_span, 1U);

    CHECK_EQ(run_cli({"merge-template-table-cells",
                      source.string(),
                      "--after-text",
                      "selector target table",
                      "--direction",
                      "right",
                      "--json"},
                     parse_output),
             2);
    CHECK_EQ(
        read_text_file(parse_output),
        std::string{
            "{\"command\":\"merge-template-table-cells\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"missing row index after table "
            "selector\"}\n"});

    remove_if_exists(source);
    remove_if_exists(merged);
    remove_if_exists(merged_multiline);
    remove_if_exists(unmerged);
    remove_if_exists(unmerged_multiline);
    remove_if_exists(merge_output);
    remove_if_exists(grid_inspect_output);
    remove_if_exists(merge_multiline_output);
    remove_if_exists(unmerge_output);
    remove_if_exists(unmerge_multiline_output);
    remove_if_exists(parse_output);
}

TEST_CASE("cli template table merge and unmerge commands support bookmark selectors") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_template_table_bookmark_merge_source.docx";
    const fs::path merged =
        working_directory / "cli_template_table_bookmark_merged.docx";
    const fs::path unmerged =
        working_directory / "cli_template_table_bookmark_unmerged.docx";
    const fs::path merge_output =
        working_directory / "cli_template_table_bookmark_merge.json";
    const fs::path unmerge_output =
        working_directory / "cli_template_table_bookmark_unmerge.json";

    remove_if_exists(source);
    remove_if_exists(merged);
    remove_if_exists(unmerged);
    remove_if_exists(merge_output);
    remove_if_exists(unmerge_output);

    create_cli_template_table_bookmark_fixture(source);

    CHECK_EQ(run_cli({"merge-template-table-cells",
                      source.string(),
                      "--bookmark",
                      "target_inside_table",
                      "0",
                      "0",
                      "--direction",
                      "right",
                      "--output",
                      merged.string(),
                      "--json"},
                     merge_output),
             0);
    const auto merge_json = read_text_file(merge_output);
    CHECK_NE(merge_json.find("\"command\":\"merge-template-table-cells\""),
             std::string::npos);
    CHECK_NE(merge_json.find("\"part\":\"body\""), std::string::npos);
    CHECK_NE(merge_json.find("\"table_index\":1"), std::string::npos);
    CHECK_NE(merge_json.find("\"row_index\":0"), std::string::npos);
    CHECK_NE(merge_json.find("\"cell_index\":0"), std::string::npos);
    CHECK_NE(merge_json.find("\"direction\":\"right\""), std::string::npos);
    CHECK_NE(merge_json.find("\"count\":1"), std::string::npos);

    featherdoc::Document reopened_merged(merged);
    REQUIRE_FALSE(reopened_merged.open());
    auto merged_body = reopened_merged.body_template();
    REQUIRE(static_cast<bool>(merged_body));
    const auto merged_cell = merged_body.inspect_table_cell(1U, 0U, 0U);
    REQUIRE(merged_cell.has_value());
    CHECK_EQ(merged_cell->text, "target-00");
    CHECK_EQ(merged_cell->column_span, 2U);

    CHECK_EQ(run_cli({"unmerge-template-table-cells",
                      merged.string(),
                      "--bookmark",
                      "target_before_table",
                      "0",
                      "0",
                      "--direction",
                      "right",
                      "--output",
                      unmerged.string(),
                      "--json"},
                     unmerge_output),
             0);
    const auto unmerge_json = read_text_file(unmerge_output);
    CHECK_NE(unmerge_json.find("\"command\":\"unmerge-template-table-cells\""),
             std::string::npos);
    CHECK_NE(unmerge_json.find("\"part\":\"body\""), std::string::npos);
    CHECK_NE(unmerge_json.find("\"table_index\":1"), std::string::npos);
    CHECK_NE(unmerge_json.find("\"row_index\":0"), std::string::npos);
    CHECK_NE(unmerge_json.find("\"cell_index\":0"), std::string::npos);
    CHECK_NE(unmerge_json.find("\"direction\":\"right\""), std::string::npos);

    featherdoc::Document reopened_unmerged(unmerged);
    REQUIRE_FALSE(reopened_unmerged.open());
    auto unmerged_body = reopened_unmerged.body_template();
    REQUIRE(static_cast<bool>(unmerged_body));
    const auto first_cell = unmerged_body.inspect_table_cell(1U, 0U, 0U);
    REQUIRE(first_cell.has_value());
    CHECK_EQ(first_cell->text, "target-00");
    CHECK_EQ(first_cell->column_span, 1U);
    const auto restored_cell = unmerged_body.inspect_table_cell(1U, 0U, 1U);
    REQUIRE(restored_cell.has_value());
    CHECK_EQ(restored_cell->text, "");

    remove_if_exists(source);
    remove_if_exists(merged);
    remove_if_exists(unmerged);
    remove_if_exists(merge_output);
    remove_if_exists(unmerge_output);
}

TEST_CASE("cli merge-template-table-cells merges template cells and reports errors") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_merge_template_table_cells_source.docx";
    const fs::path merged =
        working_directory / "cli_merge_template_table_cells_output.docx";
    const fs::path success_output =
        working_directory / "cli_merge_template_table_cells_success.json";
    const fs::path parse_output =
        working_directory / "cli_merge_template_table_cells_parse.json";
    const fs::path mutate_output =
        working_directory / "cli_merge_template_table_cells_error.json";

    remove_if_exists(source);
    remove_if_exists(merged);
    remove_if_exists(success_output);
    remove_if_exists(parse_output);
    remove_if_exists(mutate_output);

    create_cli_template_table_merge_unmerge_fixture(source);

    CHECK_EQ(run_cli({"merge-template-table-cells",
                      source.string(),
                      "0",
                      "0",
                      "0",
                      "--part",
                      "header",
                      "--index",
                      "0",
                      "--direction",
                      "right",
                      "--output",
                      merged.string(),
                      "--json"},
                     success_output),
             0);
    const auto success_json = read_text_file(success_output);
    CHECK_NE(success_json.find("\"command\":\"merge-template-table-cells\""),
             std::string::npos);
    CHECK_NE(success_json.find("\"part\":\"header\""), std::string::npos);
    CHECK_NE(success_json.find("\"part_index\":0"), std::string::npos);
    CHECK_NE(success_json.find("\"entry_name\":\"word/header1.xml\""),
             std::string::npos);
    CHECK_NE(success_json.find("\"table_index\":0"), std::string::npos);
    CHECK_NE(success_json.find("\"row_index\":0"), std::string::npos);
    CHECK_NE(success_json.find("\"cell_index\":0"), std::string::npos);
    CHECK_NE(success_json.find("\"direction\":\"right\""), std::string::npos);
    CHECK_NE(success_json.find("\"count\":1"), std::string::npos);

    featherdoc::Document reopened(merged);
    REQUIRE_FALSE(reopened.open());
    auto header_template = reopened.header_template(0U);
    REQUIRE(static_cast<bool>(header_template));
    const auto merged_cell = header_template.inspect_table_cell(0U, 0U, 0U);
    REQUIRE(merged_cell.has_value());
    CHECK_EQ(merged_cell->text, "hA");
    CHECK_EQ(merged_cell->column_span, 2U);

    const auto remaining_cell = header_template.inspect_table_cell(0U, 0U, 1U);
    REQUIRE(remaining_cell.has_value());
    CHECK_EQ(remaining_cell->text, "hC");
    CHECK_EQ(remaining_cell->column_index, 2U);

    const auto header_xml = read_docx_entry(merged, "word/header1.xml");
    pugi::xml_document header_doc;
    REQUIRE(header_doc.load_string(header_xml.c_str()));

    const auto table_node = header_doc.child("w:hdr").child("w:tbl");
    REQUIRE(table_node != pugi::xml_node{});
    const auto first_row = table_node.child("w:tr");
    REQUIRE(first_row != pugi::xml_node{});

    std::size_t cell_count = 0U;
    for (auto cell = first_row.child("w:tc"); cell != pugi::xml_node{};
         cell = cell.next_sibling("w:tc")) {
        ++cell_count;
    }
    CHECK_EQ(cell_count, 2U);

    const auto grid_span = first_row.child("w:tc").child("w:tcPr").child("w:gridSpan");
    REQUIRE(grid_span != pugi::xml_node{});
    CHECK_EQ(std::string_view{grid_span.attribute("w:val").value()}, "2");

    CHECK_EQ(run_cli({"merge-template-table-cells",
                      source.string(),
                      "0",
                      "0",
                      "0",
                      "--part",
                      "section-header",
                      "--direction",
                      "right",
                      "--json"},
                     parse_output),
             2);
    CHECK_EQ(
        read_text_file(parse_output),
        std::string{
            "{\"command\":\"merge-template-table-cells\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"section-header/section-footer "
            "mutation requires --section <index>\"}\n"});

    CHECK_EQ(run_cli({"merge-template-table-cells",
                      source.string(),
                      "0",
                      "0",
                      "0",
                      "--part",
                      "header",
                      "--index",
                      "0",
                      "--direction",
                      "right",
                      "--count",
                      "5",
                      "--json"},
                     mutate_output),
             1);
    const auto mutate_error_json = read_text_file(mutate_output);
    CHECK_NE(mutate_error_json.find("\"command\":\"merge-template-table-cells\""),
             std::string::npos);
    CHECK_NE(mutate_error_json.find("\"stage\":\"mutate\""), std::string::npos);
    CHECK_NE(mutate_error_json.find("\"entry\":\"word/header1.xml\""),
             std::string::npos);
    CHECK_NE(
        mutate_error_json.find(
            "\"detail\":\"cell at table index '0', row index '0', and cell "
            "index '0' could not be merged towards 'right' with count '5'\""),
        std::string::npos);

    remove_if_exists(source);
    remove_if_exists(merged);
    remove_if_exists(success_output);
    remove_if_exists(parse_output);
    remove_if_exists(mutate_output);
}

TEST_CASE("cli unmerge-template-table-cells splits template merges and reports errors") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_unmerge_template_table_cells_source.docx";
    const fs::path unmerged =
        working_directory / "cli_unmerge_template_table_cells_output.docx";
    const fs::path direct_footer_unmerged =
        working_directory /
        "cli_unmerge_template_table_cells_direct_footer_output.docx";
    const fs::path success_output =
        working_directory / "cli_unmerge_template_table_cells_success.json";
    const fs::path direct_footer_success_output =
        working_directory /
        "cli_unmerge_template_table_cells_direct_footer_success.json";
    const fs::path footer_success_output =
        working_directory / "cli_unmerge_template_table_cells_footer_success.json";
    const fs::path parse_output =
        working_directory / "cli_unmerge_template_table_cells_parse.json";
    const fs::path mutate_output =
        working_directory / "cli_unmerge_template_table_cells_error.json";

    remove_if_exists(source);
    remove_if_exists(unmerged);
    remove_if_exists(direct_footer_unmerged);
    remove_if_exists(success_output);
    remove_if_exists(direct_footer_success_output);
    remove_if_exists(footer_success_output);
    remove_if_exists(parse_output);
    remove_if_exists(mutate_output);

    create_cli_template_table_merge_unmerge_fixture(source);

    CHECK_EQ(run_cli({"unmerge-template-table-cells",
                      source.string(),
                      "0",
                      "0",
                      "0",
                      "--part",
                      "body",
                      "--direction",
                      "right",
                      "--output",
                      unmerged.string(),
                      "--json"},
                     success_output),
             0);
    const auto success_json = read_text_file(success_output);
    CHECK_NE(success_json.find("\"command\":\"unmerge-template-table-cells\""),
             std::string::npos);
    CHECK_NE(success_json.find("\"part\":\"body\""), std::string::npos);
    CHECK_NE(success_json.find("\"entry_name\":\"word/document.xml\""),
             std::string::npos);
    CHECK_NE(success_json.find("\"table_index\":0"), std::string::npos);
    CHECK_NE(success_json.find("\"row_index\":0"), std::string::npos);
    CHECK_NE(success_json.find("\"cell_index\":0"), std::string::npos);
    CHECK_NE(success_json.find("\"direction\":\"right\""), std::string::npos);

    featherdoc::Document reopened(unmerged);
    REQUIRE_FALSE(reopened.open());
    auto body_template = reopened.body_template();
    REQUIRE(static_cast<bool>(body_template));
    const auto first_cell = body_template.inspect_table_cell(0U, 0U, 0U);
    REQUIRE(first_cell.has_value());
    CHECK_EQ(first_cell->text, "A");
    CHECK_EQ(first_cell->column_span, 1U);
    const auto restored_cell = body_template.inspect_table_cell(0U, 0U, 1U);
    REQUIRE(restored_cell.has_value());
    CHECK_EQ(restored_cell->text, "");
    CHECK_EQ(restored_cell->column_span, 1U);
    const auto last_cell = body_template.inspect_table_cell(0U, 0U, 2U);
    REQUIRE(last_cell.has_value());
    CHECK_EQ(last_cell->text, "C");

    const auto document_xml = read_docx_entry(unmerged, "word/document.xml");
    pugi::xml_document document_xml_doc;
    REQUIRE(document_xml_doc.load_string(document_xml.c_str()));

    const auto body_table_node = document_xml_doc.child("w:document").child("w:body").child("w:tbl");
    REQUIRE(body_table_node != pugi::xml_node{});
    const auto body_row_node = body_table_node.child("w:tr");
    REQUIRE(body_row_node != pugi::xml_node{});

    std::size_t row_cell_count = 0U;
    for (auto cell = body_row_node.child("w:tc"); cell != pugi::xml_node{};
         cell = cell.next_sibling("w:tc")) {
        ++row_cell_count;
    }
    CHECK_EQ(row_cell_count, 3U);
    CHECK(body_row_node.child("w:tc").child("w:tcPr").child("w:gridSpan") ==
          pugi::xml_node{});

    CHECK_EQ(run_cli({"unmerge-template-table-cells",
                      source.string(),
                      "1",
                      "1",
                      "0",
                      "--part",
                      "footer",
                      "--index",
                      "0",
                      "--direction",
                      "down",
                      "--output",
                      direct_footer_unmerged.string(),
                      "--json"},
                     direct_footer_success_output),
             0);
    const auto direct_footer_success_json =
        read_text_file(direct_footer_success_output);
    CHECK_NE(direct_footer_success_json.find(
                 "\"command\":\"unmerge-template-table-cells\""),
             std::string::npos);
    CHECK_NE(direct_footer_success_json.find("\"part\":\"footer\""),
             std::string::npos);
    CHECK_NE(direct_footer_success_json.find("\"part_index\":0"),
             std::string::npos);
    CHECK_NE(direct_footer_success_json.find("\"entry_name\":\"word/footer1.xml\""),
             std::string::npos);
    CHECK_NE(direct_footer_success_json.find("\"table_index\":1"),
             std::string::npos);
    CHECK_NE(direct_footer_success_json.find("\"direction\":\"down\""),
             std::string::npos);

    featherdoc::Document direct_footer_reopened(direct_footer_unmerged);
    REQUIRE_FALSE(direct_footer_reopened.open());
    auto direct_footer_template = direct_footer_reopened.footer_template(0U);
    REQUIRE(static_cast<bool>(direct_footer_template));
    const auto direct_footer_top =
        direct_footer_template.inspect_table_cell(1U, 0U, 0U);
    REQUIRE(direct_footer_top.has_value());
    CHECK_EQ(direct_footer_top->text, "dfA");
    const auto direct_footer_middle =
        direct_footer_template.inspect_table_cell(1U, 1U, 0U);
    REQUIRE(direct_footer_middle.has_value());
    CHECK_EQ(direct_footer_middle->text, "");
    const auto direct_footer_bottom =
        direct_footer_template.inspect_table_cell(1U, 2U, 0U);
    REQUIRE(direct_footer_bottom.has_value());
    CHECK_EQ(direct_footer_bottom->text, "");

    const auto direct_footer_xml =
        read_docx_entry(direct_footer_unmerged, "word/footer1.xml");
    pugi::xml_document direct_footer_doc;
    REQUIRE(direct_footer_doc.load_string(direct_footer_xml.c_str()));

    auto direct_footer_table_node =
        direct_footer_doc.child("w:ftr").child("w:tbl");
    REQUIRE(direct_footer_table_node != pugi::xml_node{});
    direct_footer_table_node = direct_footer_table_node.next_sibling("w:tbl");
    REQUIRE(direct_footer_table_node != pugi::xml_node{});
    for (auto direct_footer_row = direct_footer_table_node.child("w:tr");
         direct_footer_row != pugi::xml_node{};
         direct_footer_row = direct_footer_row.next_sibling("w:tr")) {
        const auto cell = direct_footer_row.child("w:tc");
        REQUIRE(cell != pugi::xml_node{});
        CHECK(cell.child("w:tcPr").child("w:vMerge") == pugi::xml_node{});
    }

    CHECK_EQ(run_cli({"unmerge-template-table-cells",
                      source.string(),
                      "0",
                      "1",
                      "0",
                      "--part",
                      "section-footer",
                      "--direction",
                      "down",
                      "--json"},
                     parse_output),
             2);
    CHECK_EQ(
        read_text_file(parse_output),
        std::string{
            "{\"command\":\"unmerge-template-table-cells\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"section-header/section-footer "
            "mutation requires --section <index>\"}\n"});

    CHECK_EQ(run_cli({"unmerge-template-table-cells",
                      source.string(),
                      "0",
                      "1",
                      "0",
                      "--part",
                      "section-footer",
                      "--section",
                      "0",
                      "--direction",
                      "down",
                      "--json"},
                     footer_success_output),
             0);
    const auto footer_success_json = read_text_file(footer_success_output);
    CHECK_NE(footer_success_json.find("\"part\":\"section-footer\""),
             std::string::npos);
    CHECK_NE(footer_success_json.find("\"section\":0"), std::string::npos);
    CHECK_NE(footer_success_json.find("\"entry_name\":\"word/footer1.xml\""),
             std::string::npos);
    CHECK_NE(footer_success_json.find("\"direction\":\"down\""),
             std::string::npos);

    featherdoc::Document footer_reopened(source);
    REQUIRE_FALSE(footer_reopened.open());
    auto footer_template = footer_reopened.section_footer_template(0U);
    REQUIRE(static_cast<bool>(footer_template));
    const auto footer_top = footer_template.inspect_table_cell(0U, 0U, 0U);
    REQUIRE(footer_top.has_value());
    CHECK_EQ(footer_top->text, "fA");
    const auto footer_middle = footer_template.inspect_table_cell(0U, 1U, 0U);
    REQUIRE(footer_middle.has_value());
    CHECK_EQ(footer_middle->text, "");
    const auto footer_bottom = footer_template.inspect_table_cell(0U, 2U, 0U);
    REQUIRE(footer_bottom.has_value());
    CHECK_EQ(footer_bottom->text, "");

    const auto footer_xml = read_docx_entry(source, "word/footer1.xml");
    pugi::xml_document footer_doc;
    REQUIRE(footer_doc.load_string(footer_xml.c_str()));

    const auto footer_table_node = footer_doc.child("w:ftr").child("w:tbl");
    REQUIRE(footer_table_node != pugi::xml_node{});
    auto footer_row = footer_table_node.child("w:tr");
    for (; footer_row != pugi::xml_node{}; footer_row = footer_row.next_sibling("w:tr")) {
        const auto cell = footer_row.child("w:tc");
        REQUIRE(cell != pugi::xml_node{});
        CHECK(cell.child("w:tcPr").child("w:vMerge") == pugi::xml_node{});
    }

    CHECK_EQ(run_cli({"unmerge-template-table-cells",
                      source.string(),
                      "0",
                      "1",
                      "0",
                      "--part",
                      "section-footer",
                      "--section",
                      "0",
                      "--direction",
                      "down",
                      "--json"},
                     mutate_output),
             1);
    const auto mutate_error_json = read_text_file(mutate_output);
    CHECK_NE(mutate_error_json.find("\"command\":\"unmerge-template-table-cells\""),
             std::string::npos);
    CHECK_NE(mutate_error_json.find("\"stage\":\"mutate\""), std::string::npos);
    CHECK_NE(mutate_error_json.find("\"entry\":\"word/footer1.xml\""),
             std::string::npos);
    CHECK_NE(
        mutate_error_json.find(
            "\"detail\":\"cell at table index '0', row index '1', and cell "
            "index '0' could not be unmerged towards 'down'\""),
        std::string::npos);

    remove_if_exists(source);
    remove_if_exists(unmerged);
    remove_if_exists(direct_footer_unmerged);
    remove_if_exists(success_output);
    remove_if_exists(direct_footer_success_output);
    remove_if_exists(footer_success_output);
    remove_if_exists(parse_output);
    remove_if_exists(mutate_output);
}

} // namespace
