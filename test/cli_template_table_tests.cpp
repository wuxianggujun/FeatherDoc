#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "cli_test_support.hpp"

#include <zip.h>

#include "cli_template_table_test_support.hpp"

namespace {
TEST_CASE("cli inspect-template-paragraphs inspects body template paragraphs") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_inspect_template_paragraphs_source.docx";
    const fs::path output =
        working_directory / "cli_inspect_template_paragraphs_output.txt";
    const fs::path json_output =
        working_directory / "cli_inspect_template_paragraph_single.json";

    remove_if_exists(source);
    remove_if_exists(output);
    remove_if_exists(json_output);

    create_cli_template_inspection_fixture(source);

    CHECK_EQ(run_cli({"inspect-template-paragraphs", source.string()}, output), 0);
    const auto inspect_text = read_text_file(output);
    CHECK_NE(inspect_text.find("part: body\n"), std::string::npos);
    CHECK_NE(inspect_text.find("entry_name: word/document.xml\n"),
             std::string::npos);
    CHECK_NE(inspect_text.find("paragraphs: 2"), std::string::npos);
    CHECK_NE(
        inspect_text.find(
            "paragraph[0]: style_id=Heading1 bidi=yes numbering=none runs=1 "
            "text=Body heading"),
        std::string::npos);
    CHECK_NE(inspect_text.find("paragraph[1]: style_id=none bidi=none numbering=num_id="),
             std::string::npos);
    CHECK_NE(inspect_text.find("definition_name=CliTemplateInspectOutline"),
             std::string::npos);
    CHECK_NE(inspect_text.find("text=Body item"), std::string::npos);

    CHECK_EQ(run_cli({"inspect-template-paragraphs",
                      source.string(),
                      "--paragraph",
                      "1",
                      "--json"},
                     json_output),
             0);
    const auto paragraph_json = read_text_file(json_output);
    CHECK_NE(paragraph_json.find("\"part\":\"body\""), std::string::npos);
    CHECK_NE(paragraph_json.find("\"entry_name\":\"word/document.xml\""),
             std::string::npos);
    CHECK_NE(paragraph_json.find("\"paragraph\":{\"index\":1"),
             std::string::npos);
    CHECK_NE(paragraph_json.find("\"definition_name\":\"CliTemplateInspectOutline\""),
             std::string::npos);
    CHECK_NE(paragraph_json.find("\"run_count\":1"), std::string::npos);
    CHECK_NE(paragraph_json.find("\"text\":\"Body item\""), std::string::npos);

    remove_if_exists(source);
    remove_if_exists(output);
    remove_if_exists(json_output);
}

TEST_CASE("cli inspect-template-runs inspects section footer runs and errors") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_inspect_template_runs_source.docx";
    const fs::path output =
        working_directory / "cli_inspect_template_runs_output.txt";
    const fs::path missing_output =
        working_directory / "cli_inspect_template_runs_missing.json";
    const fs::path parse_output =
        working_directory / "cli_inspect_template_runs_parse.json";

    remove_if_exists(source);
    remove_if_exists(output);
    remove_if_exists(missing_output);
    remove_if_exists(parse_output);

    create_cli_template_inspection_fixture(source);

    CHECK_EQ(run_cli({"inspect-template-runs",
                      source.string(),
                      "1",
                      "--part",
                      "section-footer",
                      "--section",
                      "0",
                      "--run",
                      "0"},
                     output),
             0);
    const auto run_text = read_text_file(output);
    CHECK_NE(run_text.find("part: section-footer\n"), std::string::npos);
    CHECK_NE(run_text.find("section: 0\n"), std::string::npos);
    CHECK_NE(run_text.find("kind: default\n"), std::string::npos);
    CHECK_NE(run_text.find("entry_name: word/footer1.xml\n"), std::string::npos);
    CHECK_NE(run_text.find("paragraph_index: 1\n"), std::string::npos);
    CHECK_NE(run_text.find("index: 0\n"), std::string::npos);
    CHECK_NE(run_text.find("style_id: Strong\n"), std::string::npos);
    CHECK_NE(run_text.find("font_family: Segoe UI\n"), std::string::npos);
    CHECK_NE(run_text.find("text_color: none\n"), std::string::npos);
    CHECK_NE(run_text.find("bold: none\n"), std::string::npos);
    CHECK_NE(run_text.find("italic: none\n"), std::string::npos);
    CHECK_NE(run_text.find("underline: none\n"), std::string::npos);
    CHECK_NE(run_text.find("font_size_points: none\n"), std::string::npos);
    CHECK_NE(run_text.find("language: en-US\n"), std::string::npos);
    CHECK_NE(run_text.find("text: Footer styled\n"), std::string::npos);

    CHECK_EQ(run_cli({"inspect-template-runs",
                      source.string(),
                      "1",
                      "--part",
                      "section-footer",
                      "--section",
                      "0",
                      "--run",
                      "9",
                      "--json"},
                     missing_output),
             1);
    const auto missing_json = read_text_file(missing_output);
    CHECK_NE(missing_json.find("\"command\":\"inspect-template-runs\""),
             std::string::npos);
    CHECK_NE(missing_json.find("\"stage\":\"inspect\""), std::string::npos);
    CHECK_NE(missing_json.find(
                 "\"detail\":\"run index '9' is out of range for paragraph "
                 "index '1'\""),
             std::string::npos);
    CHECK_NE(missing_json.find("\"entry\":\"word/footer1.xml\""),
             std::string::npos);

    CHECK_EQ(run_cli({"inspect-template-runs",
                      source.string(),
                      "1",
                      "--part",
                      "section-footer",
                      "--json"},
                     parse_output),
             2);
    CHECK_EQ(
        read_text_file(parse_output),
        std::string{
            "{\"command\":\"inspect-template-runs\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"section-header/section-footer "
            "inspection requires --section <index>\"}\n"});

    remove_if_exists(source);
    remove_if_exists(output);
    remove_if_exists(missing_output);
    remove_if_exists(parse_output);
}

TEST_CASE("cli inspect-template-table commands inspect header parts and report errors") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_inspect_template_tables_source.docx";
    const fs::path tables_output =
        working_directory / "cli_inspect_template_tables_output.json";
    const fs::path cell_output =
        working_directory / "cli_inspect_template_table_cell_output.json";
    const fs::path parse_output =
        working_directory / "cli_inspect_template_table_parse.json";
    const fs::path row_error_output =
        working_directory / "cli_inspect_template_table_row_error.json";

    remove_if_exists(source);
    remove_if_exists(tables_output);
    remove_if_exists(cell_output);
    remove_if_exists(parse_output);
    remove_if_exists(row_error_output);

    create_cli_template_inspection_fixture(source);

    CHECK_EQ(run_cli({"inspect-template-tables",
                      source.string(),
                      "--part",
                      "header",
                      "--index",
                      "0",
                      "--json"},
                     tables_output),
             0);
    const auto tables_json = read_text_file(tables_output);
    CHECK_NE(tables_json.find("\"part\":\"header\""), std::string::npos);
    CHECK_NE(tables_json.find("\"part_index\":0"), std::string::npos);
    CHECK_NE(tables_json.find("\"entry_name\":\"word/header1.xml\""),
             std::string::npos);
    CHECK_NE(tables_json.find("\"count\":1"), std::string::npos);
    CHECK_NE(tables_json.find("\"style_id\":\"TableGrid\""), std::string::npos);
    CHECK_NE(tables_json.find("\"column_widths\":[1800,3600]"),
             std::string::npos);
    CHECK_NE(tables_json.find("\"text\":\"h00\\th01\\nh10\\th11\""),
             std::string::npos);

    CHECK_EQ(run_cli({"inspect-template-table-cells",
                      source.string(),
                      "0",
                      "--part",
                      "header",
                      "--index",
                      "0",
                      "--row",
                      "1",
                      "--cell",
                      "1",
                      "--json"},
                     cell_output),
             0);
    const auto cell_json = read_text_file(cell_output);
    CHECK_NE(cell_json.find("\"part\":\"header\""), std::string::npos);
    CHECK_NE(cell_json.find("\"part_index\":0"), std::string::npos);
    CHECK_NE(cell_json.find("\"entry_name\":\"word/header1.xml\""),
             std::string::npos);
    CHECK_NE(cell_json.find("\"table_index\":0"), std::string::npos);
    CHECK_NE(cell_json.find("\"row_index\":1"), std::string::npos);
    CHECK_NE(cell_json.find("\"cell_index\":1"), std::string::npos);
    CHECK_NE(cell_json.find("\"width_twips\":2222"), std::string::npos);
    CHECK_NE(cell_json.find("\"text\":\"h11\""), std::string::npos);

    CHECK_EQ(run_cli({"inspect-template-table-cells",
                      source.string(),
                      "0",
                      "--part",
                      "header",
                      "--index",
                      "0",
                      "--cell",
                      "1",
                      "--json"},
                     parse_output),
             2);
    CHECK_EQ(
        read_text_file(parse_output),
        std::string{
            "{\"command\":\"inspect-template-table-cells\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"--cell requires --row\"}\n"});

    CHECK_EQ(run_cli({"inspect-template-table-cells",
                      source.string(),
                      "0",
                      "--part",
                      "header",
                      "--index",
                      "0",
                      "--row",
                      "9",
                      "--json"},
                     row_error_output),
             1);
    const auto row_error_json = read_text_file(row_error_output);
    CHECK_NE(row_error_json.find("\"command\":\"inspect-template-table-cells\""),
             std::string::npos);
    CHECK_NE(row_error_json.find("\"detail\":\"row index '9' is out of range "
                                 "for table index '0'\""),
             std::string::npos);
    CHECK_NE(row_error_json.find("\"entry\":\"word/header1.xml\""),
             std::string::npos);

    remove_if_exists(source);
    remove_if_exists(tables_output);
    remove_if_exists(cell_output);
    remove_if_exists(parse_output);
    remove_if_exists(row_error_output);
}

TEST_CASE("cli inspect-template-table-rows inspects header rows and reports errors") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_inspect_template_table_rows_source.docx";
    const fs::path list_output =
        working_directory / "cli_inspect_template_table_rows_output.json";
    const fs::path single_output =
        working_directory / "cli_inspect_template_table_row_single.json";
    const fs::path error_output =
        working_directory / "cli_inspect_template_table_row_error.json";

    remove_if_exists(source);
    remove_if_exists(list_output);
    remove_if_exists(single_output);
    remove_if_exists(error_output);

    create_cli_template_inspection_fixture(source);

    CHECK_EQ(run_cli({"inspect-template-table-rows",
                      source.string(),
                      "0",
                      "--part",
                      "header",
                      "--index",
                      "0",
                      "--json"},
                     list_output),
             0);
    CHECK_EQ(
        read_text_file(list_output),
        std::string{
            "{\"part\":\"header\",\"part_index\":0,"
            "\"entry_name\":\"word/header1.xml\",\"table_index\":0,"
            "\"count\":2,\"rows\":["
            "{\"row_index\":0,\"cell_count\":2,\"height_twips\":null,"
            "\"height_rule\":null,\"cant_split\":false,"
            "\"repeats_header\":false,\"cell_texts\":[\"h00\",\"h01\"]},"
            "{\"row_index\":1,\"cell_count\":2,\"height_twips\":null,"
            "\"height_rule\":null,\"cant_split\":false,"
            "\"repeats_header\":false,\"cell_texts\":[\"h10\",\"h11\"]}"
            "]}\n"});

    CHECK_EQ(run_cli({"inspect-template-table-rows",
                      source.string(),
                      "0",
                      "--part",
                      "header",
                      "--index",
                      "0",
                      "--row",
                      "1",
                      "--json"},
                     single_output),
             0);
    CHECK_EQ(
        read_text_file(single_output),
        std::string{
            "{\"part\":\"header\",\"part_index\":0,"
            "\"entry_name\":\"word/header1.xml\",\"table_index\":0,"
            "\"row\":{\"row_index\":1,\"cell_count\":2,"
            "\"height_twips\":null,\"height_rule\":null,"
            "\"cant_split\":false,\"repeats_header\":false,"
            "\"cell_texts\":[\"h10\",\"h11\"]}}\n"});

    CHECK_EQ(run_cli({"inspect-template-table-rows",
                      source.string(),
                      "0",
                      "--part",
                      "header",
                      "--index",
                      "0",
                      "--row",
                      "9",
                      "--json"},
                     error_output),
             1);
    const auto error_json = read_text_file(error_output);
    CHECK_NE(error_json.find("\"command\":\"inspect-template-table-rows\""),
             std::string::npos);
    CHECK_NE(error_json.find("\"stage\":\"inspect\""), std::string::npos);
    CHECK_NE(error_json.find("\"detail\":\"row index '9' is out of range for "
                             "table index '0'\""),
             std::string::npos);
    CHECK_NE(error_json.find("\"entry\":\"word/header1.xml\""),
             std::string::npos);

    remove_if_exists(source);
    remove_if_exists(list_output);
    remove_if_exists(single_output);
    remove_if_exists(error_output);
}

TEST_CASE("cli set-template-table-cell-text updates header template cells and reports errors") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_set_template_table_cell_text_source.docx";
    const fs::path updated =
        working_directory / "cli_set_template_table_cell_text_updated.docx";
    const fs::path mutate_output =
        working_directory / "cli_set_template_table_cell_text_output.json";
    const fs::path parse_output =
        working_directory / "cli_set_template_table_cell_text_parse.json";
    const fs::path error_output =
        working_directory / "cli_set_template_table_cell_text_error.json";

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(mutate_output);
    remove_if_exists(parse_output);
    remove_if_exists(error_output);

    create_cli_template_inspection_fixture(source);

    CHECK_EQ(run_cli({"set-template-table-cell-text",
                      source.string(),
                      "0",
                      "1",
                      "1",
                      "--part",
                      "header",
                      "--index",
                      "0",
                      "--text",
                      "updated header cell",
                      "--output",
                      updated.string(),
                      "--json"},
                     mutate_output),
             0);

    const auto mutate_json = read_text_file(mutate_output);
    CHECK_NE(mutate_json.find("\"command\":\"set-template-table-cell-text\""),
             std::string::npos);
    CHECK_NE(mutate_json.find("\"part\":\"header\""), std::string::npos);
    CHECK_NE(mutate_json.find("\"part_index\":0"), std::string::npos);
    CHECK_NE(mutate_json.find("\"entry_name\":\"word/header1.xml\""),
             std::string::npos);
    CHECK_NE(mutate_json.find("\"table_index\":0"), std::string::npos);
    CHECK_NE(mutate_json.find("\"row_index\":1"), std::string::npos);
    CHECK_NE(mutate_json.find("\"cell_index\":1"), std::string::npos);

    featherdoc::Document reopened(updated);
    REQUIRE_FALSE(reopened.open());

    auto header_template = reopened.header_template(0U);
    REQUIRE(static_cast<bool>(header_template));
    const auto updated_cell = header_template.inspect_table_cell(0U, 1U, 1U);
    REQUIRE(updated_cell.has_value());
    CHECK_EQ(updated_cell->text, "updated header cell");
    REQUIRE(updated_cell->width_twips.has_value());
    CHECK_EQ(*updated_cell->width_twips, 2222U);

    CHECK_EQ(run_cli({"set-template-table-cell-text",
                      source.string(),
                      "0",
                      "0",
                      "0",
                      "--part",
                      "section-header",
                      "--text",
                      "x",
                      "--json"},
                     parse_output),
             2);
    CHECK_EQ(
        read_text_file(parse_output),
        std::string{
            "{\"command\":\"set-template-table-cell-text\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"section-header/section-footer "
            "mutation requires --section <index>\"}\n"});

    CHECK_EQ(run_cli({"set-template-table-cell-text",
                      source.string(),
                      "0",
                      "9",
                      "0",
                      "--part",
                      "header",
                      "--index",
                      "0",
                      "--text",
                      "x",
                      "--json"},
                     error_output),
             1);
    const auto error_json = read_text_file(error_output);
    CHECK_NE(error_json.find("\"command\":\"set-template-table-cell-text\""),
             std::string::npos);
    CHECK_NE(error_json.find("\"stage\":\"mutate\""), std::string::npos);
    CHECK_NE(error_json.find("\"entry\":\"word/header1.xml\""),
             std::string::npos);
    CHECK_NE(error_json.find("\"detail\":\"row index '9' is out of range for "
                             "table index '0'\""),
             std::string::npos);

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(mutate_output);
    remove_if_exists(parse_output);
    remove_if_exists(error_output);
}

TEST_CASE("cli set-template-table-cell-text updates direct footer template cells and reports errors") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_set_direct_footer_template_cell_text_source.docx";
    const fs::path updated =
        working_directory / "cli_set_direct_footer_template_cell_text_updated.docx";
    const fs::path mutate_output =
        working_directory / "cli_set_direct_footer_template_cell_text_output.json";
    const fs::path error_output =
        working_directory / "cli_set_direct_footer_template_cell_text_error.json";

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(mutate_output);
    remove_if_exists(error_output);

    create_cli_template_table_direct_column_fixture(source);

    CHECK_EQ(run_cli({"set-template-table-cell-text",
                      source.string(),
                      "0",
                      "1",
                      "1",
                      "--part",
                      "footer",
                      "--index",
                      "0",
                      "--text",
                      "updated footer cell",
                      "--output",
                      updated.string(),
                      "--json"},
                     mutate_output),
             0);

    const auto mutate_json = read_text_file(mutate_output);
    CHECK_NE(mutate_json.find("\"command\":\"set-template-table-cell-text\""),
             std::string::npos);
    CHECK_NE(mutate_json.find("\"part\":\"footer\""), std::string::npos);
    CHECK_NE(mutate_json.find("\"part_index\":0"), std::string::npos);
    CHECK_NE(mutate_json.find("\"entry_name\":\"word/footer1.xml\""),
             std::string::npos);
    CHECK_NE(mutate_json.find("\"table_index\":0"), std::string::npos);
    CHECK_NE(mutate_json.find("\"row_index\":1"), std::string::npos);
    CHECK_NE(mutate_json.find("\"cell_index\":1"), std::string::npos);

    featherdoc::Document reopened(updated);
    REQUIRE_FALSE(reopened.open());

    auto footer_template = reopened.footer_template(0U);
    REQUIRE(static_cast<bool>(footer_template));
    const auto updated_cell = footer_template.inspect_table_cell(0U, 1U, 1U);
    REQUIRE(updated_cell.has_value());
    CHECK_EQ(updated_cell->text, "updated footer cell");

    CHECK_EQ(run_cli({"set-template-table-cell-text",
                      source.string(),
                      "0",
                      "9",
                      "0",
                      "--part",
                      "footer",
                      "--index",
                      "0",
                      "--text",
                      "x",
                      "--json"},
                     error_output),
             1);
    const auto error_json = read_text_file(error_output);
    CHECK_NE(error_json.find("\"command\":\"set-template-table-cell-text\""),
             std::string::npos);
    CHECK_NE(error_json.find("\"stage\":\"mutate\""), std::string::npos);
    CHECK_NE(error_json.find("\"entry\":\"word/footer1.xml\""),
             std::string::npos);
    CHECK_NE(error_json.find("\"detail\":\"row index '9' is out of range for table index '0'\""),
             std::string::npos);

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(mutate_output);
    remove_if_exists(error_output);
}

} // namespace
