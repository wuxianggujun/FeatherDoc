#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "cli_table_test_support.hpp"

namespace {
TEST_CASE("cli inspect-table-cells supports row filtering and single-cell json output") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_inspect_table_cells_source.docx";
    const fs::path inspect_output =
        working_directory / "cli_inspect_table_cells_output.json";
    const fs::path single_output =
        working_directory / "cli_inspect_table_cell_single.json";
    const fs::path grid_output =
        working_directory / "cli_inspect_table_cell_grid_column.json";

    remove_if_exists(source);
    remove_if_exists(inspect_output);
    remove_if_exists(single_output);
    remove_if_exists(grid_output);

    create_cli_table_inspection_fixture(source);

    CHECK_EQ(run_cli({"inspect-table-cells",
                      source.string(),
                      "1",
                      "--row",
                      "1",
                      "--json"},
                     inspect_output),
             0);
    CHECK_EQ(
        read_text_file(inspect_output),
        std::string{
            "{\"table_index\":1,\"row_index\":1,\"count\":3,\"cells\":["
            "{\"row_index\":1,\"cell_index\":0,\"column_index\":0,"
            "\"column_span\":1,\"paragraph_count\":1,\"width_twips\":null,"
            "\"vertical_alignment\":null,\"text_direction\":null,"
            "\"text\":\"bottom-left\"},"
            "{\"row_index\":1,\"cell_index\":1,\"column_index\":1,"
            "\"column_span\":1,\"paragraph_count\":1,\"width_twips\":2222,"
            "\"vertical_alignment\":null,\"text_direction\":null,"
            "\"text\":\"bottom-middle\"},"
            "{\"row_index\":1,\"cell_index\":2,\"column_index\":2,"
            "\"column_span\":1,\"paragraph_count\":2,\"width_twips\":null,"
            "\"vertical_alignment\":null,\"text_direction\":null,"
            "\"text\":\"line1\\nline2\"}]}\n"});

    CHECK_EQ(run_cli({"inspect-table-cells",
                      source.string(),
                      "1",
                      "--row",
                      "0",
                      "--cell",
                      "0",
                      "--json"},
                     single_output),
             0);
    CHECK_EQ(
        read_text_file(single_output),
        std::string{
            "{\"table_index\":1,\"cell\":{\"row_index\":0,\"cell_index\":0,"
            "\"column_index\":0,\"column_span\":2,\"paragraph_count\":1,"
            "\"width_twips\":null,\"vertical_alignment\":\"center\","
            "\"text_direction\":\"top_to_bottom_right_to_left\","
            "\"text\":\"merged-top\"}}\n"});

    CHECK_EQ(run_cli({"inspect-table-cells",
                      source.string(),
                      "1",
                      "--row",
                      "0",
                      "--grid-column",
                      "1",
                      "--json"},
                     grid_output),
             0);
    CHECK_EQ(read_text_file(grid_output), read_text_file(single_output));

    remove_if_exists(source);
    remove_if_exists(inspect_output);
    remove_if_exists(single_output);
    remove_if_exists(grid_output);
}

TEST_CASE("cli inspect-table-rows supports list and single-row json output") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_inspect_table_rows_source.docx";
    const fs::path inspect_output =
        working_directory / "cli_inspect_table_rows_output.json";
    const fs::path single_output =
        working_directory / "cli_inspect_table_row_single.json";

    remove_if_exists(source);
    remove_if_exists(inspect_output);
    remove_if_exists(single_output);

    create_cli_table_inspection_fixture(source);

    CHECK_EQ(run_cli({"inspect-table-rows", source.string(), "0", "--json"},
                     inspect_output),
             0);
    CHECK_EQ(
        read_text_file(inspect_output),
        std::string{
            "{\"table_index\":0,\"count\":2,\"rows\":["
            "{\"row_index\":0,\"cell_count\":2,\"height_twips\":null,"
            "\"height_rule\":null,\"cant_split\":false,"
            "\"repeats_header\":false,\"cell_texts\":[\"r0c0\",\"r0c1\"]},"
            "{\"row_index\":1,\"cell_count\":2,\"height_twips\":null,"
            "\"height_rule\":null,\"cant_split\":false,"
            "\"repeats_header\":false,\"cell_texts\":[\"r1c0\",\"r1c1\"]}"
            "]}\n"});

    CHECK_EQ(run_cli({"inspect-table-rows",
                      source.string(),
                      "0",
                      "--row",
                      "1",
                      "--json"},
                     single_output),
             0);
    CHECK_EQ(
        read_text_file(single_output),
        std::string{
            "{\"table_index\":0,\"row\":{\"row_index\":1,\"cell_count\":2,"
            "\"height_twips\":null,\"height_rule\":null,"
            "\"cant_split\":false,\"repeats_header\":false,"
            "\"cell_texts\":[\"r1c0\",\"r1c1\"]}}\n"});

    remove_if_exists(source);
    remove_if_exists(inspect_output);
    remove_if_exists(single_output);
}

TEST_CASE("cli inspect-table commands report parse and inspect errors") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_inspect_table_error_source.docx";
    const fs::path parse_output =
        working_directory / "cli_inspect_table_parse_output.json";
    const fs::path table_error_output =
        working_directory / "cli_inspect_table_error_output.json";
    const fs::path row_error_output =
        working_directory / "cli_inspect_table_row_error_output.json";
    const fs::path cell_error_output =
        working_directory / "cli_inspect_table_cell_error_output.json";

    remove_if_exists(source);
    remove_if_exists(parse_output);
    remove_if_exists(table_error_output);
    remove_if_exists(row_error_output);
    remove_if_exists(cell_error_output);

    create_cli_table_inspection_fixture(source);

    CHECK_EQ(run_cli({"inspect-table-cells",
                      source.string(),
                      "1",
                      "--cell",
                      "0",
                      "--json"},
                     parse_output),
             2);
    CHECK_EQ(
        read_text_file(parse_output),
        std::string{
            "{\"command\":\"inspect-table-cells\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"--cell requires --row\"}\n"});

    CHECK_EQ(run_cli({"inspect-tables", source.string(), "--table", "9", "--json"},
                     table_error_output),
             1);
    const auto table_error_json = read_text_file(table_error_output);
    CHECK_NE(table_error_json.find("\"command\":\"inspect-tables\""),
             std::string::npos);
    CHECK_NE(table_error_json.find("\"stage\":\"inspect\""), std::string::npos);
    CHECK_NE(table_error_json.find("\"detail\":\"table index '9' is out of range\""),
             std::string::npos);

    CHECK_EQ(run_cli({"inspect-table-cells",
                      source.string(),
                      "1",
                      "--row",
                      "9",
                      "--json"},
                     row_error_output),
             1);
    const auto row_error_json = read_text_file(row_error_output);
    CHECK_NE(row_error_json.find("\"command\":\"inspect-table-cells\""),
             std::string::npos);
    CHECK_NE(row_error_json.find("\"detail\":\"row index '9' is out of range for "
                                 "table index '1'\""),
             std::string::npos);

    CHECK_EQ(run_cli({"inspect-table-cells",
                      source.string(),
                      "1",
                      "--row",
                      "1",
                      "--cell",
                      "9",
                      "--json"},
                     cell_error_output),
             1);
    const auto cell_error_json = read_text_file(cell_error_output);
    CHECK_NE(cell_error_json.find("\"command\":\"inspect-table-cells\""),
             std::string::npos);
    CHECK_NE(
        cell_error_json.find(
            "\"detail\":\"cell index '9' is out of range for row index '1' in "
            "table index '1'\""),
        std::string::npos);

    remove_if_exists(source);
    remove_if_exists(parse_output);
    remove_if_exists(table_error_output);
    remove_if_exists(row_error_output);
    remove_if_exists(cell_error_output);
}

TEST_CASE("cli set-table-cell-text updates a cell and preserves inspection metadata") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_set_table_cell_text_source.docx";
    const fs::path updated =
        working_directory / "cli_set_table_cell_text_updated.docx";
    const fs::path mutate_output =
        working_directory / "cli_set_table_cell_text_output.json";

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(mutate_output);

    create_cli_table_inspection_fixture(source);

    CHECK_EQ(run_cli({"set-table-cell-text",
                      source.string(),
                      "1",
                      "0",
                      "--grid-column",
                      "1",
                      "--text",
                      "updated merged",
                      "--output",
                      updated.string(),
                      "--json"},
                     mutate_output),
             0);
    CHECK_EQ(
        read_text_file(mutate_output),
        std::string{
            "{\"command\":\"set-table-cell-text\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"table_index\":1,\"row_index\":0,\"grid_column\":1,"
            "\"cell_index\":0,\"column_index\":0,\"column_span\":2}\n"});

    featherdoc::Document reopened(updated);
    REQUIRE_FALSE(reopened.open());

    const auto cell = reopened.inspect_table_cell(1U, 0U, 0U);
    REQUIRE(cell.has_value());
    CHECK_EQ(cell->text, "updated merged");
    CHECK_EQ(cell->column_span, 2U);
    REQUIRE(cell->vertical_alignment.has_value());
    CHECK_EQ(*cell->vertical_alignment,
             featherdoc::cell_vertical_alignment::center);
    REQUIRE(cell->text_direction.has_value());
    CHECK_EQ(*cell->text_direction,
             featherdoc::cell_text_direction::top_to_bottom_right_to_left);

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(mutate_output);
}

TEST_CASE("cli set-table-cell-text supports file input and reports errors") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_set_table_cell_text_file_source.docx";
    const fs::path text_source =
        working_directory / "cli_set_table_cell_text_input.txt";
    const fs::path parse_output =
        working_directory / "cli_set_table_cell_text_parse_output.json";
    const fs::path mutate_output =
        working_directory / "cli_set_table_cell_text_error_output.json";

    remove_if_exists(source);
    remove_if_exists(text_source);
    remove_if_exists(parse_output);
    remove_if_exists(mutate_output);

    create_cli_table_inspection_fixture(source);

    {
        std::ofstream stream(text_source);
        REQUIRE(stream.good());
        stream << "from file";
    }

    CHECK_EQ(run_cli({"set-table-cell-text",
                      source.string(),
                      "0",
                      "1",
                      "1",
                      "--text-file",
                      text_source.string()}),
             0);

    featherdoc::Document reopened(source);
    REQUIRE_FALSE(reopened.open());
    const auto updated_cell = reopened.inspect_table_cell(0U, 1U, 1U);
    REQUIRE(updated_cell.has_value());
    CHECK_EQ(updated_cell->text, "from file");

    CHECK_EQ(run_cli({"set-table-cell-text",
                      source.string(),
                      "1",
                      "0",
                      "0",
                      "--json"},
                     parse_output),
             2);
    CHECK_EQ(
        read_text_file(parse_output),
        std::string{
            "{\"command\":\"set-table-cell-text\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"expected --text <text> or "
            "--text-file <path>\"}\n"});

    CHECK_EQ(run_cli({"set-table-cell-text",
                      source.string(),
                      "1",
                      "1",
                      "9",
                      "--text",
                      "x",
                      "--json"},
                     mutate_output),
             1);
    const auto mutate_error_json = read_text_file(mutate_output);
    CHECK_NE(mutate_error_json.find("\"command\":\"set-table-cell-text\""),
             std::string::npos);
    CHECK_NE(mutate_error_json.find("\"stage\":\"mutate\""), std::string::npos);
    CHECK_NE(
        mutate_error_json.find(
            "\"detail\":\"cell index '9' is out of range for row index '1' in "
            "table index '1'\""),
        std::string::npos);

    remove_if_exists(source);
    remove_if_exists(text_source);
    remove_if_exists(parse_output);
    remove_if_exists(mutate_output);
}

TEST_CASE("cli merge-table-cells merges horizontally and reports json output") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_merge_table_cells_right_source.docx";
    const fs::path merged =
        working_directory / "cli_merge_table_cells_right_output.docx";
    const fs::path mutate_output =
        working_directory / "cli_merge_table_cells_right_output.json";

    remove_if_exists(source);
    remove_if_exists(merged);
    remove_if_exists(mutate_output);

    create_cli_table_merge_fixture(source);

    CHECK_EQ(run_cli({"merge-table-cells",
                      source.string(),
                      "0",
                      "0",
                      "0",
                      "--direction",
                      "right",
                      "--output",
                      merged.string(),
                      "--json"},
                     mutate_output),
             0);
    CHECK_EQ(
        read_text_file(mutate_output),
        std::string{
            "{\"command\":\"merge-table-cells\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"table_index\":0,\"row_index\":0,\"cell_index\":0,"
            "\"direction\":\"right\",\"count\":1}\n"});

    featherdoc::Document reopened(merged);
    REQUIRE_FALSE(reopened.open());

    const auto merged_cell = reopened.inspect_table_cell(0U, 0U, 0U);
    REQUIRE(merged_cell.has_value());
    CHECK_EQ(merged_cell->text, "A");
    CHECK_EQ(merged_cell->column_span, 2U);
    CHECK_EQ(merged_cell->column_index, 0U);

    const auto remaining_cell = reopened.inspect_table_cell(0U, 0U, 1U);
    REQUIRE(remaining_cell.has_value());
    CHECK_EQ(remaining_cell->text, "C");
    CHECK_EQ(remaining_cell->column_index, 2U);

    const auto merged_table = reopened.inspect_table(0U);
    REQUIRE(merged_table.has_value());
    CHECK_EQ(merged_table->text, "A\tC\nD\tE\tF\nG\tH\tI");

    const auto document_xml = read_docx_entry(merged, "word/document.xml");
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(document_xml.c_str()));

    const auto table_node = xml_document.child("w:document").child("w:body").child("w:tbl");
    REQUIRE(table_node != pugi::xml_node{});
    const auto first_row = table_node.child("w:tr");
    REQUIRE(first_row != pugi::xml_node{});

    std::size_t first_row_cell_count = 0U;
    for (auto cell = first_row.child("w:tc"); cell != pugi::xml_node{};
         cell = cell.next_sibling("w:tc")) {
        ++first_row_cell_count;
    }
    CHECK_EQ(first_row_cell_count, 2U);

    const auto first_cell = first_row.child("w:tc");
    REQUIRE(first_cell != pugi::xml_node{});
    const auto grid_span = first_cell.child("w:tcPr").child("w:gridSpan");
    REQUIRE(grid_span != pugi::xml_node{});
    CHECK_EQ(std::string_view{grid_span.attribute("w:val").value()}, "2");

    remove_if_exists(source);
    remove_if_exists(merged);
    remove_if_exists(mutate_output);
}

TEST_CASE("cli merge-table-cells merges down and reports parse/mutate errors") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_merge_table_cells_down_source.docx";
    const fs::path parse_output =
        working_directory / "cli_merge_table_cells_parse_output.json";
    const fs::path mutate_output =
        working_directory / "cli_merge_table_cells_error_output.json";

    remove_if_exists(source);
    remove_if_exists(parse_output);
    remove_if_exists(mutate_output);

    create_cli_table_merge_fixture(source);

    CHECK_EQ(run_cli({"merge-table-cells",
                      source.string(),
                      "0",
                      "0",
                      "0",
                      "--direction",
                      "down",
                      "--count",
                      "2"}),
             0);

    featherdoc::Document reopened(source);
    REQUIRE_FALSE(reopened.open());

    const auto anchor_cell = reopened.inspect_table_cell(0U, 0U, 0U);
    REQUIRE(anchor_cell.has_value());
    CHECK_EQ(anchor_cell->text, "A");

    const auto second_row_cell = reopened.inspect_table_cell(0U, 1U, 0U);
    REQUIRE(second_row_cell.has_value());
    CHECK_EQ(second_row_cell->text, "");

    const auto third_row_cell = reopened.inspect_table_cell(0U, 2U, 0U);
    REQUIRE(third_row_cell.has_value());
    CHECK_EQ(third_row_cell->text, "");

    const auto document_xml = read_docx_entry(source, "word/document.xml");
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(document_xml.c_str()));

    const auto table_node = xml_document.child("w:document").child("w:body").child("w:tbl");
    REQUIRE(table_node != pugi::xml_node{});
    const auto first_row = table_node.child("w:tr");
    REQUIRE(first_row != pugi::xml_node{});
    const auto second_row = first_row.next_sibling("w:tr");
    REQUIRE(second_row != pugi::xml_node{});
    const auto third_row = second_row.next_sibling("w:tr");
    REQUIRE(third_row != pugi::xml_node{});

    const auto first_cell = first_row.child("w:tc");
    const auto second_cell = second_row.child("w:tc");
    const auto third_cell = third_row.child("w:tc");
    REQUIRE(first_cell != pugi::xml_node{});
    REQUIRE(second_cell != pugi::xml_node{});
    REQUIRE(third_cell != pugi::xml_node{});

    CHECK_EQ(std::string_view{
                 first_cell.child("w:tcPr").child("w:vMerge").attribute("w:val").value()},
             "restart");
    CHECK_EQ(std::string_view{
                 second_cell.child("w:tcPr").child("w:vMerge").attribute("w:val").value()},
             "continue");
    CHECK_EQ(std::string_view{
                 third_cell.child("w:tcPr").child("w:vMerge").attribute("w:val").value()},
             "continue");

    CHECK_EQ(run_cli({"merge-table-cells",
                      source.string(),
                      "0",
                      "0",
                      "0",
                      "--json"},
                     parse_output),
             2);
    CHECK_EQ(
        read_text_file(parse_output),
        std::string{
            "{\"command\":\"merge-table-cells\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"missing required --direction "
            "<right|down>\"}\n"});

    CHECK_EQ(run_cli({"merge-table-cells",
                      source.string(),
                      "0",
                      "0",
                      "0",
                      "--direction",
                      "right",
                      "--count",
                      "5",
                      "--json"},
                     mutate_output),
             1);
    const auto mutate_error_json = read_text_file(mutate_output);
    CHECK_NE(mutate_error_json.find("\"command\":\"merge-table-cells\""),
             std::string::npos);
    CHECK_NE(mutate_error_json.find("\"stage\":\"mutate\""), std::string::npos);
    CHECK_NE(
        mutate_error_json.find(
            "\"detail\":\"cell at table index '0', row index '0', and cell "
            "index '0' could not be merged towards 'right' with count '5'\""),
        std::string::npos);

    remove_if_exists(source);
    remove_if_exists(parse_output);
    remove_if_exists(mutate_output);
}

TEST_CASE("cli unmerge-table-cells splits a horizontal merge and reports json output") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_unmerge_table_cells_right_source.docx";
    const fs::path updated =
        working_directory / "cli_unmerge_table_cells_right_output.docx";
    const fs::path mutate_output =
        working_directory / "cli_unmerge_table_cells_right_output.json";

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(mutate_output);

    create_cli_table_unmerge_right_fixture(source);

    CHECK_EQ(run_cli({"unmerge-table-cells",
                      source.string(),
                      "0",
                      "0",
                      "0",
                      "--direction",
                      "right",
                      "--output",
                      updated.string(),
                      "--json"},
                     mutate_output),
             0);
    CHECK_EQ(
        read_text_file(mutate_output),
        std::string{
            "{\"command\":\"unmerge-table-cells\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"table_index\":0,\"row_index\":0,\"cell_index\":0,"
            "\"direction\":\"right\"}\n"});

    featherdoc::Document reopened(updated);
    REQUIRE_FALSE(reopened.open());

    const auto first_cell = reopened.inspect_table_cell(0U, 0U, 0U);
    REQUIRE(first_cell.has_value());
    CHECK_EQ(first_cell->text, "A");
    CHECK_EQ(first_cell->column_span, 1U);

    const auto restored_cell = reopened.inspect_table_cell(0U, 0U, 1U);
    REQUIRE(restored_cell.has_value());
    CHECK_EQ(restored_cell->text, "");
    CHECK_EQ(restored_cell->column_span, 1U);

    const auto last_cell = reopened.inspect_table_cell(0U, 0U, 2U);
    REQUIRE(last_cell.has_value());
    CHECK_EQ(last_cell->text, "C");

    const auto document_xml = read_docx_entry(updated, "word/document.xml");
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(document_xml.c_str()));

    const auto table_node = xml_document.child("w:document").child("w:body").child("w:tbl");
    REQUIRE(table_node != pugi::xml_node{});
    const auto row_node = table_node.child("w:tr");
    REQUIRE(row_node != pugi::xml_node{});

    std::size_t row_cell_count = 0U;
    for (auto cell = row_node.child("w:tc"); cell != pugi::xml_node{};
         cell = cell.next_sibling("w:tc")) {
        ++row_cell_count;
    }
    CHECK_EQ(row_cell_count, 3U);

    const auto first_cell_node = row_node.child("w:tc");
    REQUIRE(first_cell_node != pugi::xml_node{});
    CHECK(first_cell_node.child("w:tcPr").child("w:gridSpan") == pugi::xml_node{});

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(mutate_output);
}

TEST_CASE("cli unmerge-table-cells splits a vertical merge and reports errors") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_unmerge_table_cells_down_source.docx";
    const fs::path parse_output =
        working_directory / "cli_unmerge_table_cells_parse_output.json";
    const fs::path mutate_output =
        working_directory / "cli_unmerge_table_cells_error_output.json";
    const fs::path success_output =
        working_directory / "cli_unmerge_table_cells_success_output.json";

    remove_if_exists(source);
    remove_if_exists(parse_output);
    remove_if_exists(mutate_output);
    remove_if_exists(success_output);

    create_cli_table_unmerge_down_fixture(source);

    CHECK_EQ(run_cli({"unmerge-table-cells",
                      source.string(),
                      "0",
                      "1",
                      "0",
                      "--json"},
                     parse_output),
             2);
    CHECK_EQ(
        read_text_file(parse_output),
        std::string{
            "{\"command\":\"unmerge-table-cells\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"missing required --direction "
            "<right|down>\"}\n"});

    CHECK_EQ(run_cli({"unmerge-table-cells",
                      source.string(),
                      "0",
                      "1",
                      "0",
                      "--direction",
                      "down",
                      "--json"},
                     success_output),
             0);
    CHECK_EQ(
        read_text_file(success_output),
        std::string{
            "{\"command\":\"unmerge-table-cells\",\"ok\":true,"
            "\"in_place\":true,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"table_index\":0,\"row_index\":1,\"cell_index\":0,"
            "\"direction\":\"down\"}\n"});

    featherdoc::Document reopened(source);
    REQUIRE_FALSE(reopened.open());

    const auto first_cell = reopened.inspect_table_cell(0U, 0U, 0U);
    REQUIRE(first_cell.has_value());
    CHECK_EQ(first_cell->text, "A");

    const auto second_cell = reopened.inspect_table_cell(0U, 1U, 0U);
    REQUIRE(second_cell.has_value());
    CHECK_EQ(second_cell->text, "");

    const auto third_cell = reopened.inspect_table_cell(0U, 2U, 0U);
    REQUIRE(third_cell.has_value());
    CHECK_EQ(third_cell->text, "");

    const auto document_xml = read_docx_entry(source, "word/document.xml");
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(document_xml.c_str()));

    const auto table_node = xml_document.child("w:document").child("w:body").child("w:tbl");
    REQUIRE(table_node != pugi::xml_node{});
    const auto first_row = table_node.child("w:tr");
    REQUIRE(first_row != pugi::xml_node{});
    const auto second_row = first_row.next_sibling("w:tr");
    REQUIRE(second_row != pugi::xml_node{});
    const auto third_row = second_row.next_sibling("w:tr");
    REQUIRE(third_row != pugi::xml_node{});

    const auto first_cell_node = first_row.child("w:tc");
    const auto second_cell_node = second_row.child("w:tc");
    const auto third_cell_node = third_row.child("w:tc");
    REQUIRE(first_cell_node != pugi::xml_node{});
    REQUIRE(second_cell_node != pugi::xml_node{});
    REQUIRE(third_cell_node != pugi::xml_node{});

    CHECK(first_cell_node.child("w:tcPr").child("w:vMerge") == pugi::xml_node{});
    CHECK(second_cell_node.child("w:tcPr").child("w:vMerge") == pugi::xml_node{});
    CHECK(third_cell_node.child("w:tcPr").child("w:vMerge") == pugi::xml_node{});

    CHECK_EQ(run_cli({"unmerge-table-cells",
                      source.string(),
                      "0",
                      "1",
                      "0",
                      "--direction",
                      "down",
                      "--json"},
                     mutate_output),
             1);
    const auto mutate_error_json = read_text_file(mutate_output);
    CHECK_NE(mutate_error_json.find("\"command\":\"unmerge-table-cells\""),
             std::string::npos);
    CHECK_NE(mutate_error_json.find("\"stage\":\"mutate\""), std::string::npos);
    CHECK_NE(
        mutate_error_json.find(
            "\"detail\":\"cell at table index '0', row index '1', and cell "
            "index '0' could not be unmerged towards 'down'\""),
        std::string::npos);

    remove_if_exists(source);
    remove_if_exists(parse_output);
    remove_if_exists(mutate_output);
    remove_if_exists(success_output);
}

} // namespace
