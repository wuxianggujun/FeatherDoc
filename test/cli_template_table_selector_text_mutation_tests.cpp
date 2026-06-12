#include "cli_template_table_test_support.hpp"

namespace {
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


} // namespace
