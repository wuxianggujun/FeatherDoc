#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "cli_table_test_support.hpp"

namespace {
TEST_CASE("cli inspect-tables lists body tables in text mode and supports single-table json") {
    const fs::path working_directory = fs::current_path();
    const fs::path source = working_directory / "cli_inspect_tables_source.docx";
    const fs::path inspect_output =
        working_directory / "cli_inspect_tables_output.txt";
    const fs::path single_output =
        working_directory / "cli_inspect_table_single.json";

    remove_if_exists(source);
    remove_if_exists(inspect_output);
    remove_if_exists(single_output);

    create_cli_table_inspection_fixture(source);

    CHECK_EQ(run_cli({"inspect-tables", source.string()}, inspect_output), 0);
    const auto inspect_text = read_text_file(inspect_output);
    CHECK_NE(inspect_text.find("tables: 2"), std::string::npos);
    CHECK_NE(
        inspect_text.find(
            "table[0]: style_id=TableGrid width_twips=7200 layout_mode=none rows=2 columns=2"),
        std::string::npos);
    CHECK_NE(inspect_text.find("column_widths=[1800,5400]"),
             std::string::npos);
    CHECK_NE(inspect_text.find("position=none"), std::string::npos);
    CHECK_NE(inspect_text.find("r0c0"), std::string::npos);
    CHECK_NE(inspect_text.find("merged-top"), std::string::npos);

    CHECK_EQ(run_cli({"inspect-tables", source.string(), "--table", "1", "--json"},
                     single_output),
             0);
    CHECK_EQ(
        read_text_file(single_output),
        std::string{
            "{\"table\":{\"index\":1,\"style_id\":null,\"width_twips\":null,"
            "\"layout_mode\":null,\"alignment\":null,\"indent_twips\":null,"
            "\"cell_spacing_twips\":null,"
            "\"cell_margins\":{\"top\":null,\"left\":null,"
            "\"bottom\":null,\"right\":null},\"borders\":null,"
            "\"position\":null,"
            "\"row_count\":2,\"column_count\":3,"
            "\"column_widths\":[1200,2200,3200],"
            "\"text\":\"merged-top\\ttop-right\\nbottom-left\\tbottom-middle\\t"
            "line1\\nline2\"}}\n"});

    remove_if_exists(source);
    remove_if_exists(inspect_output);
    remove_if_exists(single_output);
}

TEST_CASE("cli table style id commands set and clear body table style references") {
    const fs::path working_directory = fs::current_path();
    const fs::path source = working_directory / "cli_table_style_id_source.docx";
    const fs::path styled = working_directory / "cli_table_style_id_styled.docx";
    const fs::path cleared = working_directory / "cli_table_style_id_cleared.docx";
    const fs::path set_output =
        working_directory / "cli_table_style_id_set_output.json";
    const fs::path clear_output =
        working_directory / "cli_table_style_id_clear_output.json";
    const fs::path inspect_output =
        working_directory / "cli_table_style_id_inspect_output.json";
    const fs::path parse_error_output =
        working_directory / "cli_table_style_id_parse_output.json";

    remove_if_exists(source);
    remove_if_exists(styled);
    remove_if_exists(cleared);
    remove_if_exists(set_output);
    remove_if_exists(clear_output);
    remove_if_exists(inspect_output);
    remove_if_exists(parse_error_output);

    create_cli_table_inspection_fixture(source);

    CHECK_EQ(run_cli({"set-table-style-id",
                      source.string(),
                      "1",
                      "TableGrid",
                      "--output",
                      styled.string(),
                      "--json"},
                     set_output),
             0);
    CHECK_EQ(
        read_text_file(set_output),
        std::string{
            "{\"command\":\"set-table-style-id\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"table_index\":1,\"style_id\":\"TableGrid\"}\n"});

    const auto styled_document_xml = read_docx_entry(styled, "word/document.xml");
    pugi::xml_document styled_xml_document;
    REQUIRE(styled_xml_document.load_string(styled_document_xml.c_str()));
    auto table_index = std::size_t{0U};
    auto styled_table_style = pugi::xml_node{};
    for (auto table_node :
         styled_xml_document.child("w:document").child("w:body").children("w:tbl")) {
        if (table_index == 1U) {
            styled_table_style =
                table_node.child("w:tblPr").child("w:tblStyle");
            break;
        }
        ++table_index;
    }
    REQUIRE(styled_table_style != pugi::xml_node{});
    CHECK_EQ(std::string_view{styled_table_style.attribute("w:val").value()},
             "TableGrid");

    featherdoc::Document reopened_styled(styled);
    REQUIRE_FALSE(reopened_styled.open());
    const auto inspected_styled = reopened_styled.inspect_table(1U);
    REQUIRE(inspected_styled.has_value());
    REQUIRE(inspected_styled->style_id.has_value());
    CHECK_EQ(*inspected_styled->style_id, "TableGrid");

    CHECK_EQ(run_cli({"inspect-tables", styled.string(), "--table", "1", "--json"},
                     inspect_output),
             0);
    CHECK_NE(read_text_file(inspect_output).find("\"style_id\":\"TableGrid\""),
             std::string::npos);

    CHECK_EQ(run_cli({"clear-table-style-id",
                      styled.string(),
                      "1",
                      "--output",
                      cleared.string(),
                      "--json"},
                     clear_output),
             0);
    CHECK_EQ(
        read_text_file(clear_output),
        std::string{
            "{\"command\":\"clear-table-style-id\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"table_index\":1}\n"});

    featherdoc::Document reopened_cleared(cleared);
    REQUIRE_FALSE(reopened_cleared.open());
    const auto inspected_cleared = reopened_cleared.inspect_table(1U);
    REQUIRE(inspected_cleared.has_value());
    CHECK_FALSE(inspected_cleared->style_id.has_value());

    CHECK_EQ(run_cli({"set-table-style-id", source.string(), "x", "TableGrid", "--json"},
                     parse_error_output),
             2);
    CHECK_EQ(
        read_text_file(parse_error_output),
        std::string{
            "{\"command\":\"set-table-style-id\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"invalid table index: x\"}\n"});

    remove_if_exists(source);
    remove_if_exists(styled);
    remove_if_exists(cleared);
    remove_if_exists(set_output);
    remove_if_exists(clear_output);
    remove_if_exists(inspect_output);
    remove_if_exists(parse_error_output);
}

TEST_CASE("cli table style look commands set and clear body table style flags") {
    const fs::path working_directory = fs::current_path();
    const fs::path source = working_directory / "cli_table_style_look_source.docx";
    const fs::path styled = working_directory / "cli_table_style_look_styled.docx";
    const fs::path cleared = working_directory / "cli_table_style_look_cleared.docx";
    const fs::path set_output =
        working_directory / "cli_table_style_look_set_output.json";
    const fs::path clear_output =
        working_directory / "cli_table_style_look_clear_output.json";
    const fs::path parse_error_output =
        working_directory / "cli_table_style_look_parse_output.json";
    const fs::path missing_flag_output =
        working_directory / "cli_table_style_look_missing_flag_output.json";

    remove_if_exists(source);
    remove_if_exists(styled);
    remove_if_exists(cleared);
    remove_if_exists(set_output);
    remove_if_exists(clear_output);
    remove_if_exists(parse_error_output);
    remove_if_exists(missing_flag_output);

    create_cli_table_inspection_fixture(source);

    CHECK_EQ(run_cli({"set-table-style-look",
                      source.string(),
                      "0",
                      "--first-row",
                      "false",
                      "--last-row",
                      "true",
                      "--first-column",
                      "false",
                      "--last-column",
                      "true",
                      "--banded-rows",
                      "false",
                      "--banded-columns",
                      "true",
                      "--output",
                      styled.string(),
                      "--json"},
                     set_output),
             0);
    CHECK_EQ(
        read_text_file(set_output),
        std::string{
            "{\"command\":\"set-table-style-look\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"table_index\":0,\"style_look\":{\"first_row\":false,"
            "\"last_row\":true,\"first_column\":false,\"last_column\":true,"
            "\"banded_rows\":false,\"banded_columns\":true}}\n"});

    const auto styled_document_xml = read_docx_entry(styled, "word/document.xml");
    pugi::xml_document styled_xml_document;
    REQUIRE(styled_xml_document.load_string(styled_document_xml.c_str()));
    const auto table_look = styled_xml_document.child("w:document")
                                .child("w:body")
                                .child("w:tbl")
                                .child("w:tblPr")
                                .child("w:tblLook");
    REQUIRE(table_look != pugi::xml_node{});
    CHECK_EQ(std::string_view{table_look.attribute("w:val").value()}, "0340");
    CHECK_EQ(std::string_view{table_look.attribute("w:firstRow").value()}, "0");
    CHECK_EQ(std::string_view{table_look.attribute("w:lastRow").value()}, "1");
    CHECK_EQ(std::string_view{table_look.attribute("w:firstColumn").value()}, "0");
    CHECK_EQ(std::string_view{table_look.attribute("w:lastColumn").value()}, "1");
    CHECK_EQ(std::string_view{table_look.attribute("w:noHBand").value()}, "1");
    CHECK_EQ(std::string_view{table_look.attribute("w:noVBand").value()}, "0");

    featherdoc::Document reopened_styled(styled);
    REQUIRE_FALSE(reopened_styled.open());
    auto reopened_styled_table = reopened_styled.tables();
    REQUIRE(reopened_styled_table.has_next());
    const auto style_look = reopened_styled_table.style_look();
    REQUIRE(style_look.has_value());
    CHECK_FALSE(style_look->first_row);
    CHECK(style_look->last_row);
    CHECK_FALSE(style_look->first_column);
    CHECK(style_look->last_column);
    CHECK_FALSE(style_look->banded_rows);
    CHECK(style_look->banded_columns);

    CHECK_EQ(run_cli({"clear-table-style-look",
                      styled.string(),
                      "0",
                      "--output",
                      cleared.string(),
                      "--json"},
                     clear_output),
             0);
    CHECK_EQ(
        read_text_file(clear_output),
        std::string{
            "{\"command\":\"clear-table-style-look\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"table_index\":0}\n"});

    featherdoc::Document reopened_cleared(cleared);
    REQUIRE_FALSE(reopened_cleared.open());
    auto reopened_cleared_table = reopened_cleared.tables();
    REQUIRE(reopened_cleared_table.has_next());
    CHECK_FALSE(reopened_cleared_table.style_look().has_value());

    CHECK_EQ(run_cli({"set-table-style-look",
                      source.string(),
                      "0",
                      "--first-row",
                      "maybe",
                      "--json"},
                     parse_error_output),
             2);
    CHECK_EQ(
        read_text_file(parse_error_output),
        std::string{
            "{\"command\":\"set-table-style-look\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"invalid --first-row value: "
            "maybe\"}\n"});

    CHECK_EQ(run_cli({"set-table-style-look", source.string(), "0", "--json"},
                     missing_flag_output),
             2);
    CHECK_EQ(
        read_text_file(missing_flag_output),
        std::string{
            "{\"command\":\"set-table-style-look\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"set-table-style-look requires "
            "at least one style-look flag\"}\n"});

    remove_if_exists(source);
    remove_if_exists(styled);
    remove_if_exists(cleared);
    remove_if_exists(set_output);
    remove_if_exists(clear_output);
    remove_if_exists(parse_error_output);
    remove_if_exists(missing_flag_output);
}

TEST_CASE("cli audit-table-style-regions reports empty table style regions") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_audit_table_style_regions_source.docx";
    const fs::path json_output =
        working_directory / "cli_audit_table_style_regions_output.json";
    const fs::path text_output =
        working_directory / "cli_audit_table_style_regions_output.txt";
    const fs::path clean_output =
        working_directory / "cli_audit_table_style_regions_clean.json";
    const fs::path fail_output =
        working_directory / "cli_audit_table_style_regions_fail.json";

    remove_if_exists(source);
    remove_if_exists(json_output);
    remove_if_exists(text_output);
    remove_if_exists(clean_output);
    remove_if_exists(fail_output);

    create_cli_table_style_region_audit_fixture(source);

    CHECK_EQ(run_cli({"audit-table-style-regions", source.string(), "--json"},
                     json_output),
             0);
    const auto json = read_text_file(json_output);
    CHECK_NE(json.find(R"("command":"audit-table-style-regions")"),
             std::string::npos);
    CHECK_NE(json.find(R"("ok":false)"), std::string::npos);
    CHECK_NE(json.find(R"("table_style_count":2)"), std::string::npos);
    CHECK_NE(json.find(R"("region_count":4)"), std::string::npos);
    CHECK_NE(json.find(R"("issue_count":2)"), std::string::npos);
    CHECK_NE(json.find(R"("style_id":"SparseTable")"), std::string::npos);
    CHECK_NE(json.find(R"("region":"whole_table")"), std::string::npos);
    CHECK_NE(json.find(R"("region":"first_row")"), std::string::npos);

    CHECK_EQ(run_cli({"audit-table-style-regions", source.string()}, text_output), 0);
    const auto text = read_text_file(text_output);
    CHECK_NE(text.find("table_style_region_audit: issues"), std::string::npos);
    CHECK_NE(text.find("issue_count: 2"), std::string::npos);

    CHECK_EQ(run_cli({"audit-table-style-regions",
                      source.string(),
                      "--style",
                      "CleanTable",
                      "--json"},
                     clean_output),
             0);
    const auto clean_json = read_text_file(clean_output);
    CHECK_NE(clean_json.find(R"("ok":true)"), std::string::npos);
    CHECK_NE(clean_json.find(R"("style_id":"CleanTable")"), std::string::npos);
    CHECK_NE(clean_json.find(R"("issue_count":0)"), std::string::npos);

    CHECK_EQ(run_cli({"audit-table-style-regions",
                      source.string(),
                      "--fail-on-issue",
                      "--json"},
                     fail_output),
             1);
    CHECK_NE(read_text_file(fail_output).find(R"("fail_on_issue":true)"),
             std::string::npos);

    remove_if_exists(source);
    remove_if_exists(json_output);
    remove_if_exists(text_output);
    remove_if_exists(clean_output);
    remove_if_exists(fail_output);
}

TEST_CASE("cli audit-table-style-inheritance reports invalid based_on chains") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_audit_table_style_inheritance_source.docx";
    const fs::path json_output =
        working_directory / "cli_audit_table_style_inheritance_output.json";
    const fs::path text_output =
        working_directory / "cli_audit_table_style_inheritance_output.txt";
    const fs::path clean_output =
        working_directory / "cli_audit_table_style_inheritance_clean.json";
    const fs::path fail_output =
        working_directory / "cli_audit_table_style_inheritance_fail.json";

    remove_if_exists(source);
    remove_if_exists(json_output);
    remove_if_exists(text_output);
    remove_if_exists(clean_output);
    remove_if_exists(fail_output);

    create_cli_table_style_inheritance_audit_fixture(source);

    CHECK_EQ(run_cli({"audit-table-style-inheritance", source.string(), "--json"},
                     json_output),
             0);
    const auto json = read_text_file(json_output);
    CHECK_NE(json.find(R"("command":"audit-table-style-inheritance")"),
             std::string::npos);
    CHECK_NE(json.find(R"("ok":false)"), std::string::npos);
    CHECK_NE(json.find(R"("table_style_count":6)"), std::string::npos);
    CHECK_NE(json.find(R"("issue_count":4)"), std::string::npos);
    CHECK_NE(json.find(R"("issue_type":"missing_based_on")"),
             std::string::npos);
    CHECK_NE(json.find(R"("issue_type":"based_on_not_table")"),
             std::string::npos);
    CHECK_NE(json.find(R"("issue_type":"inheritance_cycle")"),
             std::string::npos);
    CHECK_NE(json.find(R"("based_on_style_id":"MissingTable")"),
             std::string::npos);
    CHECK_NE(json.find(R"("based_on_style_kind":"paragraph")"),
             std::string::npos);
    CHECK_NE(json.find(R"("inheritance_chain":["CycleA","CycleB","CycleA"])"),
             std::string::npos);

    CHECK_EQ(run_cli({"audit-table-style-inheritance", source.string()}, text_output),
             0);
    const auto text = read_text_file(text_output);
    CHECK_NE(text.find("table_style_inheritance_audit: issues"),
             std::string::npos);
    CHECK_NE(text.find("chain=CycleA -> CycleB -> CycleA"), std::string::npos);

    CHECK_EQ(run_cli({"audit-table-style-inheritance",
                      source.string(),
                      "--style",
                      "ChildTable",
                      "--json"},
                     clean_output),
             0);
    const auto clean_json = read_text_file(clean_output);
    CHECK_NE(clean_json.find(R"("ok":true)"), std::string::npos);
    CHECK_NE(clean_json.find(R"("style_id":"ChildTable")"), std::string::npos);
    CHECK_NE(clean_json.find(R"("issue_count":0)"), std::string::npos);

    CHECK_EQ(run_cli({"audit-table-style-inheritance",
                      source.string(),
                      "--fail-on-issue",
                      "--json"},
                     fail_output),
             1);
    CHECK_NE(read_text_file(fail_output).find(R"("fail_on_issue":true)"),
             std::string::npos);

    remove_if_exists(source);
    remove_if_exists(json_output);
    remove_if_exists(text_output);
    remove_if_exists(clean_output);
    remove_if_exists(fail_output);
}

TEST_CASE("cli audit-table-style-quality aggregates table style gates") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_audit_table_style_quality_source.docx";
    const fs::path json_output =
        working_directory / "cli_audit_table_style_quality_output.json";
    const fs::path text_output =
        working_directory / "cli_audit_table_style_quality_output.txt";
    const fs::path fail_output =
        working_directory / "cli_audit_table_style_quality_fail.json";
    const fs::path plan_json_output =
        working_directory / "cli_plan_table_style_quality_fixes_output.json";
    const fs::path plan_text_output =
        working_directory / "cli_plan_table_style_quality_fixes_output.txt";
    const fs::path plan_fail_output =
        working_directory / "cli_plan_table_style_quality_fixes_fail.json";
    const fs::path applied =
        working_directory / "cli_apply_table_style_quality_fixes_applied.docx";
    const fs::path apply_output =
        working_directory / "cli_apply_table_style_quality_fixes_output.json";
    const fs::path after_quality_output =
        working_directory / "cli_apply_table_style_quality_fixes_after.json";

    remove_if_exists(source);
    remove_if_exists(json_output);
    remove_if_exists(text_output);
    remove_if_exists(fail_output);
    remove_if_exists(plan_json_output);
    remove_if_exists(plan_text_output);
    remove_if_exists(plan_fail_output);
    remove_if_exists(applied);
    remove_if_exists(apply_output);
    remove_if_exists(after_quality_output);
    remove_if_exists(applied);
    remove_if_exists(apply_output);
    remove_if_exists(after_quality_output);

    create_cli_table_style_quality_audit_fixture(source);

    CHECK_EQ(run_cli({"audit-table-style-quality", source.string(), "--json"},
                     json_output),
             0);
    const auto json = read_text_file(json_output);
    CHECK_NE(json.find(R"("command":"audit-table-style-quality")"),
             std::string::npos);
    CHECK_NE(json.find(R"("ok":false)"), std::string::npos);
    CHECK_NE(json.find(R"("issue_count":3)"), std::string::npos);
    CHECK_NE(json.find(R"("region_issue_count":1)"), std::string::npos);
    CHECK_NE(json.find(R"("inheritance_issue_count":1)"), std::string::npos);
    CHECK_NE(json.find(R"("style_look_issue_count":1)"), std::string::npos);
    CHECK_NE(json.find(R"("region_audit")"), std::string::npos);
    CHECK_NE(json.find(R"("inheritance_audit")"), std::string::npos);
    CHECK_NE(json.find(R"("style_look")"), std::string::npos);
    CHECK_NE(json.find(R"("issue_type":"empty_region")"), std::string::npos);
    CHECK_NE(json.find(R"("issue_type":"missing_based_on")"), std::string::npos);
    CHECK_NE(json.find(R"("issue_type":"style_look_disabled")"),
             std::string::npos);

    CHECK_EQ(run_cli({"audit-table-style-quality", source.string()}, text_output),
             0);
    const auto text = read_text_file(text_output);
    CHECK_NE(text.find("table_style_quality_audit: issues"), std::string::npos);
    CHECK_NE(text.find("region_issue_count: 1"), std::string::npos);
    CHECK_NE(text.find("inheritance_issue_count: 1"), std::string::npos);
    CHECK_NE(text.find("style_look_issue_count: 1"), std::string::npos);

    CHECK_EQ(run_cli({"audit-table-style-quality",
                      source.string(),
                      "--fail-on-issue",
                      "--json"},
                     fail_output),
             1);
    CHECK_NE(read_text_file(fail_output).find(R"("fail_on_issue":true)"),
             std::string::npos);

    CHECK_EQ(run_cli({"plan-table-style-quality-fixes", source.string(), "--json"},
                     plan_json_output),
             0);
    const auto plan_json = read_text_file(plan_json_output);
    CHECK_NE(plan_json.find(R"("command":"plan-table-style-quality-fixes")"),
             std::string::npos);
    CHECK_NE(plan_json.find(R"("plan_item_count":3)"), std::string::npos);
    CHECK_NE(plan_json.find(R"("automatic_fix_count":1)"), std::string::npos);
    CHECK_NE(plan_json.find(R"("manual_fix_count":2)"), std::string::npos);
    CHECK_NE(plan_json.find(R"("action":"edit_table_style_region")"),
             std::string::npos);
    CHECK_NE(plan_json.find(R"("action":"create_or_clear_based_on")"),
             std::string::npos);
    CHECK_NE(plan_json.find(R"("action":"repair_table_style_look")"),
             std::string::npos);
    CHECK_NE(plan_json.find(R"("recommended_command":"repair-table-style-look --plan-only")"),
             std::string::npos);

    CHECK_EQ(run_cli({"plan-table-style-quality-fixes", source.string()},
                     plan_text_output),
             0);
    const auto plan_text = read_text_file(plan_text_output);
    CHECK_NE(plan_text.find("table_style_quality_fix_plan: issues"),
             std::string::npos);
    CHECK_NE(plan_text.find("automatic_fix_count: 1"), std::string::npos);
    CHECK_NE(plan_text.find("manual_fix_count: 2"), std::string::npos);

    CHECK_EQ(run_cli({"plan-table-style-quality-fixes",
                      source.string(),
                      "--fail-on-issue",
                      "--json"},
                     plan_fail_output),
             1);
    CHECK_NE(read_text_file(plan_fail_output).find(R"("fail_on_issue":true)"),
             std::string::npos);

    CHECK_EQ(run_cli({"apply-table-style-quality-fixes",
                      source.string(),
                      "--look-only",
                      "--output",
                      applied.string(),
                      "--json"},
                     apply_output),
             0);
    const auto apply_json = read_text_file(apply_output);
    CHECK_NE(apply_json.find(R"("command":"apply-table-style-quality-fixes")"),
             std::string::npos);
    CHECK_NE(apply_json.find(R"("mode":"look_only")"), std::string::npos);
    CHECK_NE(apply_json.find(R"("before_issue_count":3)"), std::string::npos);
    CHECK_NE(apply_json.find(R"("after_issue_count":2)"), std::string::npos);
    CHECK_NE(apply_json.find(R"("before_automatic_fix_count":1)"),
             std::string::npos);
    CHECK_NE(apply_json.find(R"("after_automatic_fix_count":0)"),
             std::string::npos);
    CHECK_NE(apply_json.find(R"("changed_table_count":1)"), std::string::npos);

    CHECK_EQ(run_cli({"audit-table-style-quality", applied.string(), "--json"},
                     after_quality_output),
             0);
    const auto after_quality_json = read_text_file(after_quality_output);
    CHECK_NE(after_quality_json.find(R"("issue_count":2)"), std::string::npos);
    CHECK_NE(after_quality_json.find(R"("region_issue_count":1)"),
             std::string::npos);
    CHECK_NE(after_quality_json.find(R"("inheritance_issue_count":1)"),
             std::string::npos);
    CHECK_NE(after_quality_json.find(R"("style_look_issue_count":0)"),
             std::string::npos);

    remove_if_exists(source);
    remove_if_exists(json_output);
    remove_if_exists(text_output);
    remove_if_exists(fail_output);
    remove_if_exists(plan_json_output);
    remove_if_exists(plan_text_output);
    remove_if_exists(plan_fail_output);
}

TEST_CASE("cli check-table-style-look reports disabled conditional flags") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_check_table_style_look_source.docx";
    const fs::path json_output =
        working_directory / "cli_check_table_style_look_output.json";
    const fs::path text_output =
        working_directory / "cli_check_table_style_look_output.txt";
    const fs::path fail_output =
        working_directory / "cli_check_table_style_look_fail.json";

    remove_if_exists(source);
    remove_if_exists(json_output);
    remove_if_exists(text_output);
    remove_if_exists(fail_output);

    create_cli_table_style_look_check_fixture(source);

    CHECK_EQ(run_cli({"check-table-style-look", source.string(), "--json"},
                     json_output),
             0);
    const auto json = read_text_file(json_output);
    CHECK_NE(json.find(R"("command":"check-table-style-look")"),
             std::string::npos);
    CHECK_NE(json.find(R"("ok":false)"), std::string::npos);
    CHECK_NE(json.find(R"("table_count":1)"), std::string::npos);
    CHECK_NE(json.find(R"("issue_count":2)"), std::string::npos);
    CHECK_NE(json.find(R"("region":"first_row")"), std::string::npos);
    CHECK_NE(json.find(R"("required_style_look_flag":"first_row")"),
             std::string::npos);
    CHECK_NE(json.find(R"("actual_value":false)"), std::string::npos);
    CHECK_NE(json.find(R"("region":"second_banded_rows")"),
             std::string::npos);
    CHECK_NE(json.find(R"("required_style_look_flag":"banded_rows")"),
             std::string::npos);

    CHECK_EQ(run_cli({"check-table-style-look", source.string()}, text_output), 0);
    const auto text = read_text_file(text_output);
    CHECK_NE(text.find("table_style_look_consistency: issues"),
             std::string::npos);
    CHECK_NE(text.find("issue_count: 2"), std::string::npos);
    CHECK_NE(text.find("region=second_banded_rows"), std::string::npos);

    CHECK_EQ(run_cli({"check-table-style-look",
                      source.string(),
                      "--fail-on-issue",
                      "--json"},
                     fail_output),
             1);
    CHECK_NE(read_text_file(fail_output).find(R"("fail_on_issue":true)"),
             std::string::npos);

    remove_if_exists(source);
    remove_if_exists(json_output);
    remove_if_exists(text_output);
    remove_if_exists(fail_output);
}

TEST_CASE("cli repair-table-style-look applies conditional flags") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_repair_table_style_look_source.docx";
    const fs::path repaired =
        working_directory / "cli_repair_table_style_look_repaired.docx";
    const fs::path plan_output =
        working_directory / "cli_repair_table_style_look_plan.json";
    const fs::path apply_output =
        working_directory / "cli_repair_table_style_look_apply.json";
    const fs::path check_output =
        working_directory / "cli_repair_table_style_look_check.json";

    remove_if_exists(source);
    remove_if_exists(repaired);
    remove_if_exists(plan_output);
    remove_if_exists(apply_output);
    remove_if_exists(check_output);

    create_cli_table_style_look_check_fixture(source);

    CHECK_EQ(run_cli({"repair-table-style-look",
                      source.string(),
                      "--plan-only",
                      "--json"},
                     plan_output),
             0);
    const auto plan_json = read_text_file(plan_output);
    CHECK_NE(plan_json.find(R"("command":"repair-table-style-look")"),
             std::string::npos);
    CHECK_NE(plan_json.find(R"("mode":"plan")"), std::string::npos);
    CHECK_NE(plan_json.find(R"("before_issue_count":2)"), std::string::npos);
    CHECK_NE(plan_json.find(R"("after_issue_count":null)"), std::string::npos);
    CHECK_NE(plan_json.find(R"("applicable_issue_count":2)"),
             std::string::npos);

    CHECK_EQ(run_cli({"repair-table-style-look",
                      source.string(),
                      "--apply",
                      "--output",
                      repaired.string(),
                      "--json"},
                     apply_output),
             0);
    const auto apply_json = read_text_file(apply_output);
    CHECK_NE(apply_json.find(R"("mode":"apply")"), std::string::npos);
    CHECK_NE(apply_json.find(R"("ok":true)"), std::string::npos);
    CHECK_NE(apply_json.find(R"("before_issue_count":2)"), std::string::npos);
    CHECK_NE(apply_json.find(R"("after_issue_count":0)"), std::string::npos);
    CHECK_NE(apply_json.find(R"("changed_table_count":1)"), std::string::npos);

    CHECK_EQ(run_cli({"check-table-style-look", repaired.string(), "--json"},
                     check_output),
             0);
    const auto check_json = read_text_file(check_output);
    CHECK_NE(check_json.find(R"("ok":true)"), std::string::npos);
    CHECK_NE(check_json.find(R"("issue_count":0)"), std::string::npos);

    remove_if_exists(source);
    remove_if_exists(repaired);
    remove_if_exists(plan_output);
    remove_if_exists(apply_output);
    remove_if_exists(check_output);
}

} // namespace
