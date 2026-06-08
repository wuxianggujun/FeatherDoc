#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "cli_test_support.hpp"

#include <zip.h>

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
            "table[0]: style_id=TableGrid width_twips=7200 rows=2 columns=2"),
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
            "\"alignment\":null,\"indent_twips\":null,"
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

TEST_CASE("cli plans table position preset application") {
    const fs::path working_directory = fs::current_path();
    const fs::path source = working_directory / "cli_table_position_plan_source.docx";
    const fs::path positioned =
        working_directory / "cli_table_position_plan_positioned.docx";
    const fs::path applied_from_plan =
        working_directory / "cli_table_position_plan_applied.docx";
    const fs::path stale_applied_from_plan =
        working_directory / "cli_table_position_plan_stale_applied.docx";
    const fs::path source_plan_output =
        working_directory /
        "cli_table_position_plan_source-table-position-paragraph-callout.docx";
    const fs::path positioned_plan_output =
        working_directory /
        "cli_table_position_plan_positioned-table-position-page-corner.docx";
    const fs::path custom_plan_output =
        working_directory / "cli_table_position_plan_custom_output.docx";
    const fs::path json_output =
        working_directory / "cli_table_position_plan_output.json";
    const fs::path text_output =
        working_directory / "cli_table_position_plan_output.txt";
    const fs::path saved_plan =
        working_directory / "cli_table_position_plan_saved.json";
    const fs::path saved_plan_stdout =
        working_directory / "cli_table_position_plan_saved_stdout.json";
    const fs::path stale_plan =
        working_directory / "cli_table_position_plan_stale.json";
    const fs::path missing_fingerprint_plan =
        working_directory / "cli_table_position_plan_missing_fingerprint.json";
    const fs::path clean_output =
        working_directory / "cli_table_position_plan_clean.json";
    const fs::path fail_output =
        working_directory / "cli_table_position_plan_fail.json";
    const fs::path set_output =
        working_directory / "cli_table_position_plan_set.json";
    const fs::path apply_output =
        working_directory / "cli_table_position_plan_apply.json";
    const fs::path stale_apply_output =
        working_directory / "cli_table_position_plan_stale_apply.json";
    const fs::path dry_run_output =
        working_directory / "cli_table_position_plan_dry_run.json";
    const fs::path missing_fingerprint_apply_output =
        working_directory / "cli_table_position_plan_missing_fingerprint_apply.json";
    const fs::path apply_inspect_output =
        working_directory / "cli_table_position_plan_apply_inspect.json";
    const fs::path replace_output =
        working_directory / "cli_table_position_plan_replace.json";
    const fs::path review_output =
        working_directory / "cli_table_position_plan_review.json";
    const fs::path override_output =
        working_directory / "cli_table_position_plan_override.json";

    remove_if_exists(source);
    remove_if_exists(positioned);
    remove_if_exists(applied_from_plan);
    remove_if_exists(stale_applied_from_plan);
    remove_if_exists(custom_plan_output);
    remove_if_exists(json_output);
    remove_if_exists(text_output);
    remove_if_exists(saved_plan);
    remove_if_exists(saved_plan_stdout);
    remove_if_exists(stale_plan);
    remove_if_exists(missing_fingerprint_plan);
    remove_if_exists(clean_output);
    remove_if_exists(fail_output);
    remove_if_exists(set_output);
    remove_if_exists(apply_output);
    remove_if_exists(stale_apply_output);
    remove_if_exists(dry_run_output);
    remove_if_exists(missing_fingerprint_apply_output);
    remove_if_exists(apply_inspect_output);
    remove_if_exists(replace_output);
    remove_if_exists(review_output);
    remove_if_exists(override_output);

    create_cli_table_inspection_fixture(source);

    CHECK_EQ(run_cli({"plan-table-position-presets",
                      source.string(),
                      "--preset",
                      "paragraph-callout",
                      "--json"},
                     json_output),
             0);
    const auto json = read_text_file(json_output);
    CHECK_NE(json.find(R"("command":"plan-table-position-presets")"),
             std::string::npos);
    CHECK_NE(json.find(R"("ok":false)"), std::string::npos);
    CHECK_NE(json.find(R"("input_path":)"), std::string::npos);
    CHECK_NE(json.find(json_quote(source.string())), std::string::npos);
    CHECK_NE(json.find(R"("preset":"paragraph-callout")"),
             std::string::npos);
    CHECK_NE(json.find(R"("table_count":2)"), std::string::npos);
    CHECK_NE(json.find(R"("set_count":2)"), std::string::npos);
    CHECK_NE(json.find(R"("already_matching_table_indices":[])"),
             std::string::npos);
    CHECK_NE(json.find(R"("review_table_indices":[])"),
             std::string::npos);
    CHECK_NE(json.find(R"("plan_item_count":2)"), std::string::npos);
    CHECK_NE(json.find(R"("automatic_count":2)"), std::string::npos);
    CHECK_NE(json.find(R"("automatic_table_indices":[0,1])"),
             std::string::npos);
    CHECK_NE(json.find(R"("table_fingerprints":[)"), std::string::npos);
    CHECK_NE(json.find(R"("fingerprint":{)"), std::string::npos);
    CHECK_NE(json.find(R"("row_count":2)"), std::string::npos);
    CHECK_NE(json.find(R"("column_count":3)"), std::string::npos);
    CHECK_NE(json.find(
                 R"("recommended_batch_command":"set-table-position <input.docx> all --preset paragraph-callout")"),
             std::string::npos);
    CHECK_NE(json.find("\"resolved_output_path\":" +
                       json_quote(source_plan_output.string())),
             std::string::npos);
    CHECK_NE(json.find("\"resolved_recommended_batch_command\":\"set-table-position " +
                       json_escape_text(source.string()) +
                       " all --preset paragraph-callout --output " +
                       json_escape_text(source_plan_output.string()) + "\""),
             std::string::npos);
    CHECK_NE(json.find(R"("action":"set-preset")"), std::string::npos);
    CHECK_NE(json.find(
                 R"("recommended_command":"set-table-position <input.docx> 0 --preset paragraph-callout")"),
             std::string::npos);
    CHECK_NE(json.find("\"resolved_recommended_command\":\"set-table-position " +
                       json_escape_text(source.string()) +
                       " 0 --preset paragraph-callout --output " +
                       json_escape_text(source_plan_output.string()) + "\""),
             std::string::npos);

    CHECK_EQ(run_cli({"plan-table-position-presets",
                      source.string(),
                      "--preset",
                      "paragraph-callout"},
                     text_output),
             0);
    const auto text = read_text_file(text_output);
    CHECK_NE(text.find("table_position_preset_plan: changes"),
             std::string::npos);
    CHECK_NE(text.find("input_path: " + source.string()),
             std::string::npos);
    CHECK_NE(text.find("set_count: 2"), std::string::npos);
    CHECK_NE(text.find("already_matching_table_indices: []"),
             std::string::npos);
    CHECK_NE(text.find("review_table_indices: []"), std::string::npos);
    CHECK_NE(text.find("automatic_count: 2"), std::string::npos);
    CHECK_NE(text.find("automatic_table_indices: [0,1]"),
             std::string::npos);
    CHECK_NE(text.find("recommended_batch_command: set-table-position <input.docx> all --preset paragraph-callout"),
             std::string::npos);
    CHECK_NE(text.find("resolved_output_path: " + source_plan_output.string()),
             std::string::npos);
    CHECK_NE(text.find("resolved_recommended_batch_command: set-table-position " +
                       source.string() + " all --preset paragraph-callout --output " +
                       source_plan_output.string()),
             std::string::npos);
    CHECK_NE(text.find("recommended_command=\"set-table-position <input.docx> 0 --preset paragraph-callout\""),
             std::string::npos);
    CHECK_NE(text.find("resolved_output_path=\"" +
                       source_plan_output.string() + "\""),
             std::string::npos);
    CHECK_NE(text.find("resolved_recommended_command=\"set-table-position " +
                       source.string() + " 0 --preset paragraph-callout --output " +
                       source_plan_output.string() + "\""),
             std::string::npos);

    CHECK_NE(text.find("fingerprint_rows=2"), std::string::npos);
    CHECK_NE(text.find("fingerprint_columns=3"), std::string::npos);


    CHECK_EQ(run_cli({"plan-table-position-presets",
                      source.string(),
                      "--preset",
                      "paragraph-callout",
                      "--fail-on-change",
                      "--json"},
                     fail_output),
             1);
    CHECK_NE(read_text_file(fail_output).find(R"("fail_on_change":true)"),
             std::string::npos);

    CHECK_EQ(run_cli({"plan-table-position-presets",
                      source.string(),
                      "--preset",
                      "paragraph-callout",
                      "--output-plan",
                      saved_plan.string(),
                      "--json"},
                     saved_plan_stdout),
             0);
    const auto saved_plan_stdout_json = read_text_file(saved_plan_stdout);
    const auto saved_plan_json = read_text_file(saved_plan);
    CHECK_EQ(saved_plan_json, saved_plan_stdout_json);
    CHECK_NE(saved_plan_json.find(R"("input_path":)"), std::string::npos);
    CHECK_NE(saved_plan_json.find(json_quote(source.string())),
             std::string::npos);
    CHECK_NE(saved_plan_json.find(R"("output_plan_path":)"),
             std::string::npos);
    CHECK_NE(saved_plan_json.find(json_quote(saved_plan.string())),
             std::string::npos);
    CHECK_NE(saved_plan_json.find(R"("automatic_table_indices":[0,1])"),
             std::string::npos);
    CHECK_NE(saved_plan_json.find(R"("table_fingerprints":[)"),
             std::string::npos);
    CHECK_NE(saved_plan_json.find(R"("fingerprint":{)"), std::string::npos);
    CHECK_NE(saved_plan_json.find("\"resolved_output_path\":" +
                                  json_quote(source_plan_output.string())),
             std::string::npos);
    CHECK_NE(saved_plan_json.find("\"resolved_recommended_batch_command\":\"set-table-position " +
                                  json_escape_text(source.string()) +
                                  " all --preset paragraph-callout --output " +
                                  json_escape_text(source_plan_output.string()) +
                                  "\""),
             std::string::npos);

    auto stale_plan_json = saved_plan_json;
    const auto stale_text_offset = stale_plan_json.find("r0c0");
    REQUIRE_NE(stale_text_offset, std::string::npos);
    stale_plan_json.replace(stale_text_offset, std::string{"r0c0"}.size(),
                            "r0c0-stale");
    write_binary_file(stale_plan, stale_plan_json);
    CHECK_EQ(run_cli({"apply-table-position-plan",
                      stale_plan.string(),
                      "--output",
                      stale_applied_from_plan.string(),
                      "--json"},
                     stale_apply_output),
             1);
    const auto stale_apply_json = read_text_file(stale_apply_output);
    CHECK_NE(stale_apply_json.find(R"("message":"table fingerprint mismatch")"),
             std::string::npos);
    CHECK_NE(stale_apply_json.find(
                 "table fingerprint changed since plan was generated: table 0"),
             std::string::npos);
    CHECK_NE(stale_apply_json.find("changed fields: text expected=r0c0-stale"),
             std::string::npos);
    CHECK_NE(stale_apply_json.find("current=r0c0"), std::string::npos);
    CHECK_FALSE(fs::exists(stale_applied_from_plan));

    auto missing_fingerprint_plan_json = saved_plan_json;
    const auto fingerprint_key_offset =
        missing_fingerprint_plan_json.find(R"(,"table_fingerprints")");
    REQUIRE_NE(fingerprint_key_offset, std::string::npos);
    const auto fingerprint_next_key_offset =
        missing_fingerprint_plan_json.find(R"(,"items")", fingerprint_key_offset + 1U);
    REQUIRE_NE(fingerprint_next_key_offset, std::string::npos);
    missing_fingerprint_plan_json.erase(
        fingerprint_key_offset, fingerprint_next_key_offset - fingerprint_key_offset);
    write_binary_file(missing_fingerprint_plan, missing_fingerprint_plan_json);
    CHECK_EQ(run_cli({"apply-table-position-plan",
                      missing_fingerprint_plan.string(),
                      "--dry-run",
                      "--json"},
                     missing_fingerprint_apply_output),
             1);
    const auto missing_fingerprint_apply_json =
        read_text_file(missing_fingerprint_apply_output);
    CHECK_NE(missing_fingerprint_apply_json.find(R"("message":"invalid argument")"),
             std::string::npos);
    CHECK_NE(missing_fingerprint_apply_json.find("table_fingerprints"),
             std::string::npos);

    CHECK_EQ(run_cli({"apply-table-position-plan",
                      saved_plan.string(),
                      "--dry-run",
                      "--json"},
                     dry_run_output),
             0);
    const auto dry_run_json = read_text_file(dry_run_output);
    CHECK_NE(dry_run_json.find(R"("command":"apply-table-position-plan")"),
             std::string::npos);
    CHECK_NE(dry_run_json.find(R"("ok":true)"), std::string::npos);
    CHECK_NE(dry_run_json.find(R"("dry_run":true)"), std::string::npos);
    CHECK_NE(dry_run_json.find(R"("table_count":2)"), std::string::npos);
    CHECK_NE(dry_run_json.find(R"("fingerprint_checked_count":2)"),
             std::string::npos);
    CHECK_NE(dry_run_json.find(R"("would_apply_count":2)"),
             std::string::npos);
    CHECK_NE(dry_run_json.find(R"("table_indices":[0,1])"),
             std::string::npos);
    CHECK_NE(dry_run_json.find("\"resolved_output_path\":" +
                               json_quote(source_plan_output.string())),
             std::string::npos);
    CHECK_FALSE(fs::exists(source_plan_output));

    CHECK_EQ(run_cli({"plan-table-position-presets",
                      source.string(),
                      "--preset",
                      "paragraph-callout",
                      "--output",
                      custom_plan_output.string(),
                      "--json"},
                     override_output),
             0);
    const auto override_json = read_text_file(override_output);
    CHECK_NE(override_json.find("\"output_path\":" +
                                json_quote(custom_plan_output.string())),
             std::string::npos);
    CHECK_NE(override_json.find("\"resolved_output_path\":" +
                                json_quote(custom_plan_output.string())),
             std::string::npos);
    CHECK_NE(override_json.find("\"resolved_recommended_batch_command\":\"set-table-position " +
                                json_escape_text(source.string()) +
                                " all --preset paragraph-callout --output " +
                                json_escape_text(custom_plan_output.string()) +
                                "\""),
             std::string::npos);
    CHECK_FALSE(fs::exists(custom_plan_output));

    CHECK_EQ(run_cli({"apply-table-position-plan",
                      saved_plan.string(),
                      "--output",
                      applied_from_plan.string(),
                      "--json"},
                     apply_output),
             0);
    const auto apply_json = read_text_file(apply_output);
    CHECK_NE(apply_json.find(R"("command":"apply-table-position-plan")"),
             std::string::npos);
    CHECK_NE(apply_json.find(R"("applied_count":2)"), std::string::npos);
    CHECK_NE(apply_json.find(R"("table_indices":[0,1])"), std::string::npos);
    CHECK_NE(apply_json.find("\"input_path\":" + json_quote(source.string())),
             std::string::npos);
    CHECK_NE(apply_json.find(R"("preset":"paragraph-callout")"),
             std::string::npos);
    CHECK_NE(apply_json.find(R"("table_count":2)"), std::string::npos);
    CHECK_NE(apply_json.find(R"("fingerprint_checked_count":2)"),
             std::string::npos);
    CHECK(fs::exists(applied_from_plan));
    CHECK_FALSE(fs::exists(source_plan_output));
    CHECK_EQ(run_cli({"inspect-tables", applied_from_plan.string(), "--json"},
                     apply_inspect_output),
             0);
    const auto apply_inspect_json = read_text_file(apply_inspect_output);
    CHECK_NE(apply_inspect_json.find(R"("position":{)"), std::string::npos);
    CHECK_NE(apply_inspect_json.find(R"("horizontal_reference":"column")"),
             std::string::npos);
    CHECK_NE(apply_inspect_json.find(R"("vertical_reference":"paragraph")"),
             std::string::npos);

    CHECK_EQ(run_cli({"set-table-position",
                      source.string(),
                      "all",
                      "--preset",
                      "paragraph-callout",
                      "--output",
                      positioned.string(),
                      "--json"},
                     set_output),
             0);
    CHECK_EQ(run_cli({"plan-table-position-presets",
                      positioned.string(),
                      "--preset",
                      "paragraph-callout",
                      "--json"},
                     clean_output),
             0);
    const auto clean_json = read_text_file(clean_output);
    CHECK_NE(clean_json.find(R"("ok":true)"), std::string::npos);
    CHECK_NE(clean_json.find(R"("already_matching_count":2)"),
             std::string::npos);
    CHECK_NE(clean_json.find(R"("already_matching_table_indices":[0,1])"),
             std::string::npos);
    CHECK_NE(clean_json.find(R"("review_table_indices":[])"),
             std::string::npos);
    CHECK_NE(clean_json.find(R"("automatic_count":0)"), std::string::npos);
    CHECK_NE(clean_json.find(R"("automatic_table_indices":[])"),
             std::string::npos);
    CHECK_NE(clean_json.find(R"("recommended_batch_command":null)"),
             std::string::npos);
    CHECK_NE(clean_json.find(R"("resolved_output_path":null)"),
             std::string::npos);
    CHECK_NE(clean_json.find(R"("resolved_recommended_batch_command":null)"),
             std::string::npos);
    CHECK_NE(clean_json.find(R"("plan_item_count":0)"), std::string::npos);

    CHECK_EQ(run_cli({"plan-table-position-presets",
                      positioned.string(),
                      "--preset",
                      "page-corner",
                      "--json"},
                     review_output),
             0);
    const auto review_json = read_text_file(review_output);
    CHECK_NE(review_json.find(R"("review_count":2)"), std::string::npos);
    CHECK_NE(review_json.find(R"("review_table_indices":[0,1])"),
             std::string::npos);
    CHECK_NE(review_json.find(R"("automatic_table_indices":[])"),
             std::string::npos);
    CHECK_NE(review_json.find(R"("recommended_batch_command":null)"),
             std::string::npos);
    CHECK_NE(review_json.find(R"("resolved_output_path":null)"),
             std::string::npos);
    CHECK_NE(review_json.find(R"("resolved_recommended_batch_command":null)"),
             std::string::npos);
    CHECK_NE(review_json.find(R"("action":"review-existing-position")"),
             std::string::npos);
    CHECK_NE(review_json.find("\"resolved_recommended_command\":\"inspect-tables " +
                              json_escape_text(positioned.string()) +
                              " --table 0 --json\""),
             std::string::npos);
    CHECK_EQ(review_json.find(" --output "), std::string::npos);

    CHECK_EQ(run_cli({"plan-table-position-presets",
                      positioned.string(),
                      "--preset",
                      "page-corner",
                      "--replace-positioned",
                      "--json"},
                     replace_output),
             0);
    const auto replace_json = read_text_file(replace_output);
    CHECK_NE(replace_json.find(R"("replace_positioned":true)"),
             std::string::npos);
    CHECK_NE(replace_json.find(R"("replace_count":2)"), std::string::npos);
    CHECK_NE(replace_json.find(R"("already_matching_table_indices":[])"),
             std::string::npos);
    CHECK_NE(replace_json.find(R"("review_table_indices":[])"),
             std::string::npos);
    CHECK_NE(replace_json.find(R"("automatic_table_indices":[0,1])"),
             std::string::npos);
    CHECK_NE(replace_json.find(
                 R"("recommended_batch_command":"set-table-position <input.docx> all --preset page-corner")"),
             std::string::npos);
    CHECK_NE(replace_json.find("\"resolved_output_path\":" +
                               json_quote(positioned_plan_output.string())),
             std::string::npos);
    CHECK_NE(replace_json.find("\"resolved_recommended_batch_command\":\"set-table-position " +
                               json_escape_text(positioned.string()) +
                               " all --preset page-corner --output " +
                               json_escape_text(positioned_plan_output.string()) +
                               "\""),
             std::string::npos);
    CHECK_NE(replace_json.find(R"("action":"replace-preset")"),
             std::string::npos);

    remove_if_exists(source);
    remove_if_exists(positioned);
    remove_if_exists(applied_from_plan);
    remove_if_exists(stale_applied_from_plan);
    remove_if_exists(custom_plan_output);
    remove_if_exists(json_output);
    remove_if_exists(text_output);
    remove_if_exists(saved_plan);
    remove_if_exists(saved_plan_stdout);
    remove_if_exists(stale_plan);
    remove_if_exists(missing_fingerprint_plan);
    remove_if_exists(clean_output);
    remove_if_exists(fail_output);
    remove_if_exists(set_output);
    remove_if_exists(apply_output);
    remove_if_exists(stale_apply_output);
    remove_if_exists(dry_run_output);
    remove_if_exists(missing_fingerprint_apply_output);
    remove_if_exists(apply_inspect_output);
    remove_if_exists(replace_output);
    remove_if_exists(review_output);
    remove_if_exists(override_output);
}

TEST_CASE("cli table position commands set inspect and clear body table position") {
    const fs::path working_directory = fs::current_path();
    const fs::path source = working_directory / "cli_table_position_source.docx";
    const fs::path positioned = working_directory / "cli_table_position_set.docx";
    const fs::path cleared = working_directory / "cli_table_position_cleared.docx";
    const fs::path preset_positioned =
        working_directory / "cli_table_position_preset_set.docx";
    const fs::path all_positioned =
        working_directory / "cli_table_position_all_set.docx";
    const fs::path list_positioned =
        working_directory / "cli_table_position_list_set.docx";
    const fs::path all_cleared =
        working_directory / "cli_table_position_all_cleared.docx";
    const fs::path list_cleared =
        working_directory / "cli_table_position_list_cleared.docx";
    const fs::path set_output =
        working_directory / "cli_table_position_set_output.json";
    const fs::path inspect_output =
        working_directory / "cli_table_position_inspect_output.json";
    const fs::path clear_output =
        working_directory / "cli_table_position_clear_output.json";
    const fs::path inspect_cleared_output =
        working_directory / "cli_table_position_inspect_cleared_output.json";
    const fs::path preset_output =
        working_directory / "cli_table_position_preset_output.json";
    const fs::path all_output =
        working_directory / "cli_table_position_all_output.json";
    const fs::path list_output =
        working_directory / "cli_table_position_list_output.json";
    const fs::path clear_all_output =
        working_directory / "cli_table_position_clear_all_output.json";
    const fs::path clear_list_output =
        working_directory / "cli_table_position_clear_list_output.json";
    const fs::path parse_error_output =
        working_directory / "cli_table_position_parse_error.json";

    remove_if_exists(source);
    remove_if_exists(positioned);
    remove_if_exists(cleared);
    remove_if_exists(preset_positioned);
    remove_if_exists(all_positioned);
    remove_if_exists(list_positioned);
    remove_if_exists(all_cleared);
    remove_if_exists(list_cleared);
    remove_if_exists(set_output);
    remove_if_exists(inspect_output);
    remove_if_exists(clear_output);
    remove_if_exists(inspect_cleared_output);
    remove_if_exists(preset_output);
    remove_if_exists(all_output);
    remove_if_exists(list_output);
    remove_if_exists(clear_all_output);
    remove_if_exists(clear_list_output);
    remove_if_exists(parse_error_output);

    create_cli_table_inspection_fixture(source);

    CHECK_EQ(run_cli({"set-table-position",
                      source.string(),
                      "0",
                      "--horizontal-reference",
                      "page",
                      "--horizontal-offset",
                      "720",
                      "--horizontal-spec",
                      "center",
                      "--vertical-reference",
                      "paragraph",
                      "--vertical-offset",
                      "-120",
                      "--vertical-spec",
                      "bottom",
                      "--left-from-text",
                      "144",
                      "--right-from-text",
                      "288",
                      "--top-from-text",
                      "72",
                      "--bottom-from-text",
                      "216",
                      "--overlap",
                      "never",
                      "--output",
                      positioned.string(),
                      "--json"},
                     set_output),
             0);
    CHECK_EQ(
        read_text_file(set_output),
        std::string{
            "{\"command\":\"set-table-position\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"table_indices\":[0],\"positions\":[{\"horizontal_reference\":\"page\","
            "\"horizontal_offset_twips\":720,\"horizontal_spec\":\"center\","
            "\"vertical_reference\":\"paragraph\",\"vertical_offset_twips\":-120,"
            "\"vertical_spec\":\"bottom\",\"left_from_text_twips\":144,"
            "\"right_from_text_twips\":288,\"top_from_text_twips\":72,"
            "\"bottom_from_text_twips\":216,\"overlap\":\"never\"}]}\n"});

    const auto positioned_document_xml = read_docx_entry(positioned, "word/document.xml");
    pugi::xml_document positioned_xml_document;
    REQUIRE(positioned_xml_document.load_string(positioned_document_xml.c_str()));
    const auto positioned_table_properties = positioned_xml_document.child("w:document")
                                                 .child("w:body")
                                                 .child("w:tbl")
                                                 .child("w:tblPr");
    REQUIRE(positioned_table_properties != pugi::xml_node{});
    const auto table_position = positioned_table_properties.child("w:tblpPr");
    REQUIRE(table_position != pugi::xml_node{});
    CHECK_EQ(std::string_view{table_position.attribute("w:horzAnchor").value()},
             "page");
    CHECK_EQ(std::string_view{table_position.attribute("w:tblpX").value()},
             "720");
    CHECK_EQ(std::string_view{table_position.attribute("w:tblpXSpec").value()},
             "center");
    CHECK_EQ(std::string_view{table_position.attribute("w:vertAnchor").value()},
             "text");
    CHECK_EQ(std::string_view{table_position.attribute("w:tblpY").value()},
             "-120");
    CHECK_EQ(std::string_view{table_position.attribute("w:tblpYSpec").value()},
             "bottom");
    CHECK_EQ(std::string_view{table_position.attribute("w:leftFromText").value()},
             "144");
    CHECK_EQ(std::string_view{table_position.attribute("w:rightFromText").value()},
             "288");
    CHECK_EQ(std::string_view{table_position.attribute("w:topFromText").value()},
             "72");
    CHECK_EQ(std::string_view{table_position.attribute("w:bottomFromText").value()},
             "216");
    CHECK_EQ(std::string_view{table_position.attribute("w:tblOverlap").value()},
             "never");

    featherdoc::Document reopened(positioned);
    REQUIRE_FALSE(reopened.open());
    auto reopened_table = reopened.tables();
    REQUIRE(reopened_table.has_next());
    const auto reopened_position = reopened_table.position();
    REQUIRE(reopened_position.has_value());
    CHECK_EQ(reopened_position->horizontal_reference,
             featherdoc::table_position_horizontal_reference::page);
    CHECK_EQ(reopened_position->horizontal_offset_twips, 720);
    REQUIRE(reopened_position->horizontal_spec.has_value());
    CHECK_EQ(*reopened_position->horizontal_spec,
             featherdoc::table_position_horizontal_spec::center);
    CHECK_EQ(reopened_position->vertical_reference,
             featherdoc::table_position_vertical_reference::paragraph);
    CHECK_EQ(reopened_position->vertical_offset_twips, -120);
    REQUIRE(reopened_position->vertical_spec.has_value());
    CHECK_EQ(*reopened_position->vertical_spec,
             featherdoc::table_position_vertical_spec::bottom);
    REQUIRE(reopened_position->left_from_text_twips.has_value());
    CHECK_EQ(*reopened_position->left_from_text_twips, 144U);
    REQUIRE(reopened_position->right_from_text_twips.has_value());
    CHECK_EQ(*reopened_position->right_from_text_twips, 288U);
    REQUIRE(reopened_position->top_from_text_twips.has_value());
    CHECK_EQ(*reopened_position->top_from_text_twips, 72U);
    REQUIRE(reopened_position->bottom_from_text_twips.has_value());
    CHECK_EQ(*reopened_position->bottom_from_text_twips, 216U);
    REQUIRE(reopened_position->overlap.has_value());
    CHECK_EQ(*reopened_position->overlap, featherdoc::table_overlap::never);

    CHECK_EQ(run_cli({"inspect-tables", positioned.string(), "--table", "0", "--json"},
                     inspect_output),
             0);
    const auto inspect_json = read_text_file(inspect_output);
    CHECK_NE(inspect_json.find("\"position\":{\"horizontal_reference\":\"page\","),
             std::string::npos);
    CHECK_NE(inspect_json.find("\"horizontal_offset_twips\":720"),
             std::string::npos);
    CHECK_NE(inspect_json.find("\"horizontal_spec\":\"center\""),
             std::string::npos);
    CHECK_NE(inspect_json.find("\"vertical_reference\":\"paragraph\""),
             std::string::npos);
    CHECK_NE(inspect_json.find("\"vertical_offset_twips\":-120"),
             std::string::npos);
    CHECK_NE(inspect_json.find("\"vertical_spec\":\"bottom\""),
             std::string::npos);

    CHECK_EQ(run_cli({"set-table-position",
                      source.string(),
                      "0",
                      "--preset",
                      "page-corner",
                      "--horizontal-offset",
                      "360",
                      "--bottom-from-text",
                      "288",
                      "--output",
                      preset_positioned.string(),
                      "--json"},
                     preset_output),
             0);
    CHECK_EQ(
        read_text_file(preset_output),
        std::string{
            "{\"command\":\"set-table-position\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"table_indices\":[0],\"positions\":[{\"horizontal_reference\":\"page\","
            "\"horizontal_offset_twips\":360,\"horizontal_spec\":null,"
            "\"vertical_reference\":\"page\",\"vertical_offset_twips\":720,"
            "\"vertical_spec\":null,\"left_from_text_twips\":144,"
            "\"right_from_text_twips\":144,\"top_from_text_twips\":144,"
            "\"bottom_from_text_twips\":288,\"overlap\":\"never\"}]}\n"});

    const auto preset_document_xml =
        read_docx_entry(preset_positioned, "word/document.xml");
    pugi::xml_document preset_xml_document;
    REQUIRE(preset_xml_document.load_string(preset_document_xml.c_str()));
    const auto preset_table_position = preset_xml_document.child("w:document")
                                           .child("w:body")
                                           .child("w:tbl")
                                           .child("w:tblPr")
                                           .child("w:tblpPr");
    REQUIRE(preset_table_position != pugi::xml_node{});
    CHECK_EQ(
        std::string_view{preset_table_position.attribute("w:horzAnchor").value()},
        "page");
    CHECK_EQ(std::string_view{preset_table_position.attribute("w:tblpX").value()},
             "360");
    CHECK_EQ(
        std::string_view{preset_table_position.attribute("w:vertAnchor").value()},
        "page");
    CHECK_EQ(std::string_view{preset_table_position.attribute("w:tblpY").value()},
             "720");
    CHECK_EQ(
        std::string_view{preset_table_position.attribute("w:bottomFromText").value()},
        "288");
    CHECK_EQ(
        std::string_view{preset_table_position.attribute("w:tblOverlap").value()},
        "never");

    CHECK_EQ(run_cli({"set-table-position",
                      source.string(),
                      "all",
                      "--preset",
                      "paragraph-callout",
                      "--output",
                      all_positioned.string(),
                      "--json"},
                     all_output),
             0);
    CHECK_EQ(
        read_text_file(all_output),
        std::string{
            "{\"command\":\"set-table-position\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"table_indices\":[0,1],\"positions\":["
            "{\"horizontal_reference\":\"column\",\"horizontal_offset_twips\":0,"
            "\"horizontal_spec\":null,\"vertical_reference\":\"paragraph\","
            "\"vertical_offset_twips\":0,\"vertical_spec\":null,"
            "\"left_from_text_twips\":144,\"right_from_text_twips\":144,"
            "\"top_from_text_twips\":72,\"bottom_from_text_twips\":72,"
            "\"overlap\":\"never\"},"
            "{\"horizontal_reference\":\"column\",\"horizontal_offset_twips\":0,"
            "\"horizontal_spec\":null,\"vertical_reference\":\"paragraph\","
            "\"vertical_offset_twips\":0,\"vertical_spec\":null,"
            "\"left_from_text_twips\":144,\"right_from_text_twips\":144,"
            "\"top_from_text_twips\":72,\"bottom_from_text_twips\":72,"
            "\"overlap\":\"never\"}]}\n"});

    const auto all_document_xml = read_docx_entry(all_positioned, "word/document.xml");
    pugi::xml_document all_xml_document;
    REQUIRE(all_xml_document.load_string(all_document_xml.c_str()));
    const auto all_body = all_xml_document.child("w:document").child("w:body");
    std::size_t all_positioned_table_count = 0U;
    for (auto table_node = all_body.child("w:tbl"); table_node != pugi::xml_node{};
         table_node = table_node.next_sibling("w:tbl")) {
        const auto table_position = table_node.child("w:tblPr").child("w:tblpPr");
        REQUIRE(table_position != pugi::xml_node{});
        CHECK_EQ(std::string_view{table_position.attribute("w:horzAnchor").value()},
                 "text");
        CHECK_EQ(std::string_view{table_position.attribute("w:vertAnchor").value()},
                 "text");
        CHECK_EQ(std::string_view{table_position.attribute("w:tblOverlap").value()},
                 "never");
        ++all_positioned_table_count;
    }
    CHECK_EQ(all_positioned_table_count, 2U);

    CHECK_EQ(run_cli({"set-table-position",
                      source.string(),
                      "0",
                      "--table",
                      "1",
                      "--preset",
                      "margin-anchor",
                      "--output",
                      list_positioned.string(),
                      "--json"},
                     list_output),
             0);
    CHECK_EQ(
        read_text_file(list_output),
        std::string{
            "{\"command\":\"set-table-position\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"table_indices\":[0,1],\"positions\":["
            "{\"horizontal_reference\":\"margin\",\"horizontal_offset_twips\":0,"
            "\"horizontal_spec\":null,\"vertical_reference\":\"paragraph\","
            "\"vertical_offset_twips\":0,\"vertical_spec\":null,"
            "\"left_from_text_twips\":144,\"right_from_text_twips\":144,"
            "\"top_from_text_twips\":72,\"bottom_from_text_twips\":72,"
            "\"overlap\":\"never\"},"
            "{\"horizontal_reference\":\"margin\",\"horizontal_offset_twips\":0,"
            "\"horizontal_spec\":null,\"vertical_reference\":\"paragraph\","
            "\"vertical_offset_twips\":0,\"vertical_spec\":null,"
            "\"left_from_text_twips\":144,\"right_from_text_twips\":144,"
            "\"top_from_text_twips\":72,\"bottom_from_text_twips\":72,"
            "\"overlap\":\"never\"}]}\n"});

    CHECK_EQ(run_cli({"clear-table-position",
                      positioned.string(),
                      "0",
                      "--output",
                      cleared.string(),
                      "--json"},
                     clear_output),
             0);
    CHECK_EQ(
        read_text_file(clear_output),
        std::string{
            "{\"command\":\"clear-table-position\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"table_indices\":[0],\"positions\":[null]}\n"});

    const auto cleared_document_xml = read_docx_entry(cleared, "word/document.xml");
    pugi::xml_document cleared_xml_document;
    REQUIRE(cleared_xml_document.load_string(cleared_document_xml.c_str()));
    const auto cleared_table_properties = cleared_xml_document.child("w:document")
                                              .child("w:body")
                                              .child("w:tbl")
                                              .child("w:tblPr");
    REQUIRE(cleared_table_properties != pugi::xml_node{});
    CHECK(cleared_table_properties.child("w:tblpPr") == pugi::xml_node{});

    CHECK_EQ(run_cli({"inspect-tables", cleared.string(), "--table", "0", "--json"},
                     inspect_cleared_output),
             0);
    CHECK_NE(read_text_file(inspect_cleared_output).find("\"position\":null"),
             std::string::npos);

    CHECK_EQ(run_cli({"set-table-position",
                      source.string(),
                      "0",
                      "--horizontal-reference",
                      "page",
                      "--horizontal-offset",
                      "0",
                      "--horizontal-spec",
                      "middle",
                      "--vertical-reference",
                      "paragraph",
                      "--vertical-offset",
                      "0",
                      "--json"},
                     parse_error_output),
             2);
    CHECK_EQ(
        read_text_file(parse_error_output),
        std::string{
            "{\"command\":\"set-table-position\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"invalid horizontal spec: middle\"}\n"});


    CHECK_EQ(run_cli({"clear-table-position",
                      all_positioned.string(),
                      "all",
                      "--output",
                      all_cleared.string(),
                      "--json"},
                     clear_all_output),
             0);
    CHECK_EQ(
        read_text_file(clear_all_output),
        std::string{
            "{\"command\":\"clear-table-position\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"table_indices\":[0,1],\"positions\":[null,null]}\n"});

    const auto all_cleared_document_xml =
        read_docx_entry(all_cleared, "word/document.xml");
    pugi::xml_document all_cleared_xml_document;
    REQUIRE(all_cleared_xml_document.load_string(all_cleared_document_xml.c_str()));
    const auto all_cleared_body =
        all_cleared_xml_document.child("w:document").child("w:body");
    std::size_t all_cleared_table_count = 0U;
    for (auto table_node = all_cleared_body.child("w:tbl");
         table_node != pugi::xml_node{};
         table_node = table_node.next_sibling("w:tbl")) {
        CHECK(table_node.child("w:tblPr").child("w:tblpPr") == pugi::xml_node{});
        ++all_cleared_table_count;
    }
    CHECK_EQ(all_cleared_table_count, 2U);

    CHECK_EQ(run_cli({"clear-table-position",
                      list_positioned.string(),
                      "0",
                      "--table",
                      "1",
                      "--output",
                      list_cleared.string(),
                      "--json"},
                     clear_list_output),
             0);
    CHECK_EQ(
        read_text_file(clear_list_output),
        std::string{
            "{\"command\":\"clear-table-position\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"table_indices\":[0,1],\"positions\":[null,null]}\n"});

    const auto list_cleared_document_xml =
        read_docx_entry(list_cleared, "word/document.xml");
    pugi::xml_document list_cleared_xml_document;
    REQUIRE(
        list_cleared_xml_document.load_string(list_cleared_document_xml.c_str()));
    const auto list_cleared_body =
        list_cleared_xml_document.child("w:document").child("w:body");
    std::size_t list_cleared_table_count = 0U;
    for (auto table_node = list_cleared_body.child("w:tbl");
         table_node != pugi::xml_node{};
         table_node = table_node.next_sibling("w:tbl")) {
        CHECK(table_node.child("w:tblPr").child("w:tblpPr") == pugi::xml_node{});
        ++list_cleared_table_count;
    }
    CHECK_EQ(list_cleared_table_count, 2U);
    remove_if_exists(source);
    remove_if_exists(positioned);
    remove_if_exists(cleared);
    remove_if_exists(preset_positioned);
    remove_if_exists(all_positioned);
    remove_if_exists(list_positioned);
    remove_if_exists(all_cleared);
    remove_if_exists(list_cleared);
    remove_if_exists(set_output);
    remove_if_exists(inspect_output);
    remove_if_exists(clear_output);
    remove_if_exists(inspect_cleared_output);
    remove_if_exists(preset_output);
    remove_if_exists(all_output);
    remove_if_exists(list_output);
    remove_if_exists(clear_all_output);
    remove_if_exists(clear_list_output);
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

TEST_CASE("cli table cell fill commands set and clear body cell fill") {
    const fs::path working_directory = fs::current_path();
    const fs::path source = working_directory / "cli_table_cell_fill_source.docx";
    const fs::path styled = working_directory / "cli_table_cell_fill_styled.docx";
    const fs::path cleared = working_directory / "cli_table_cell_fill_cleared.docx";
    const fs::path set_output =
        working_directory / "cli_table_cell_fill_set_output.json";
    const fs::path clear_output =
        working_directory / "cli_table_cell_fill_clear_output.json";

    remove_if_exists(source);
    remove_if_exists(styled);
    remove_if_exists(cleared);
    remove_if_exists(set_output);
    remove_if_exists(clear_output);

    create_cli_table_cell_style_fixture(source);

    CHECK_EQ(run_cli({"set-table-cell-fill",
                      source.string(),
                      "0",
                      "0",
                      "0",
                      "A1B2C3",
                      "--output",
                      styled.string(),
                      "--json"},
                     set_output),
             0);
    CHECK_EQ(
        read_text_file(set_output),
        std::string{
            "{\"command\":\"set-table-cell-fill\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"table_index\":0,\"row_index\":0,\"cell_index\":0,"
            "\"fill_color\":\"A1B2C3\"}\n"});

    const auto source_document_xml = read_docx_entry(source, "word/document.xml");
    pugi::xml_document source_xml_document;
    REQUIRE(source_xml_document.load_string(source_document_xml.c_str()));
    const auto source_cell_node =
        source_xml_document.child("w:document").child("w:body").child("w:tbl").child("w:tr").child("w:tc");
    REQUIRE(source_cell_node != pugi::xml_node{});
    CHECK(source_cell_node.child("w:tcPr").child("w:shd") == pugi::xml_node{});

    const auto styled_document_xml = read_docx_entry(styled, "word/document.xml");
    pugi::xml_document styled_xml_document;
    REQUIRE(styled_xml_document.load_string(styled_document_xml.c_str()));
    const auto styled_cell_node =
        styled_xml_document.child("w:document").child("w:body").child("w:tbl").child("w:tr").child("w:tc");
    REQUIRE(styled_cell_node != pugi::xml_node{});
    const auto shading = styled_cell_node.child("w:tcPr").child("w:shd");
    REQUIRE(shading != pugi::xml_node{});
    CHECK_EQ(std::string_view{shading.attribute("w:val").value()}, "clear");
    CHECK_EQ(std::string_view{shading.attribute("w:color").value()}, "auto");
    CHECK_EQ(std::string_view{shading.attribute("w:fill").value()}, "A1B2C3");

    featherdoc::Document reopened(styled);
    REQUIRE_FALSE(reopened.open());
    auto reopened_cell = reopened.tables().rows().cells();
    REQUIRE(reopened_cell.has_next());
    REQUIRE(reopened_cell.fill_color().has_value());
    CHECK_EQ(*reopened_cell.fill_color(), "A1B2C3");

    CHECK_EQ(run_cli({"clear-table-cell-fill",
                      styled.string(),
                      "0",
                      "0",
                      "0",
                      "--output",
                      cleared.string(),
                      "--json"},
                     clear_output),
             0);
    CHECK_EQ(
        read_text_file(clear_output),
        std::string{
            "{\"command\":\"clear-table-cell-fill\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"table_index\":0,\"row_index\":0,\"cell_index\":0}\n"});

    const auto cleared_document_xml = read_docx_entry(cleared, "word/document.xml");
    pugi::xml_document cleared_xml_document;
    REQUIRE(cleared_xml_document.load_string(cleared_document_xml.c_str()));
    const auto cleared_cell_node =
        cleared_xml_document.child("w:document").child("w:body").child("w:tbl").child("w:tr").child("w:tc");
    REQUIRE(cleared_cell_node != pugi::xml_node{});
    CHECK(cleared_cell_node.child("w:tcPr").child("w:shd") == pugi::xml_node{});

    featherdoc::Document reopened_cleared(cleared);
    REQUIRE_FALSE(reopened_cleared.open());
    auto reopened_cleared_cell = reopened_cleared.tables().rows().cells();
    REQUIRE(reopened_cleared_cell.has_next());
    CHECK_FALSE(reopened_cleared_cell.fill_color().has_value());

    remove_if_exists(source);
    remove_if_exists(styled);
    remove_if_exists(cleared);
    remove_if_exists(set_output);
    remove_if_exists(clear_output);
}

TEST_CASE("cli table cell vertical alignment commands set and clear body cell alignment") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_table_cell_vertical_alignment_source.docx";
    const fs::path aligned =
        working_directory / "cli_table_cell_vertical_alignment_aligned.docx";
    const fs::path cleared =
        working_directory / "cli_table_cell_vertical_alignment_cleared.docx";
    const fs::path set_output =
        working_directory / "cli_table_cell_vertical_alignment_set_output.json";
    const fs::path clear_output =
        working_directory / "cli_table_cell_vertical_alignment_clear_output.json";

    remove_if_exists(source);
    remove_if_exists(aligned);
    remove_if_exists(cleared);
    remove_if_exists(set_output);
    remove_if_exists(clear_output);

    create_cli_table_cell_style_fixture(source);

    CHECK_EQ(run_cli({"set-table-cell-vertical-alignment",
                      source.string(),
                      "0",
                      "0",
                      "0",
                      "center",
                      "--output",
                      aligned.string(),
                      "--json"},
                     set_output),
             0);
    CHECK_EQ(
        read_text_file(set_output),
        std::string{
            "{\"command\":\"set-table-cell-vertical-alignment\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"table_index\":0,\"row_index\":0,\"cell_index\":0,"
            "\"vertical_alignment\":\"center\"}\n"});

    const auto source_document_xml = read_docx_entry(source, "word/document.xml");
    pugi::xml_document source_xml_document;
    REQUIRE(source_xml_document.load_string(source_document_xml.c_str()));
    const auto source_cell_node =
        source_xml_document.child("w:document").child("w:body").child("w:tbl").child("w:tr").child("w:tc");
    REQUIRE(source_cell_node != pugi::xml_node{});
    CHECK(source_cell_node.child("w:tcPr").child("w:vAlign") == pugi::xml_node{});

    const auto aligned_document_xml = read_docx_entry(aligned, "word/document.xml");
    pugi::xml_document aligned_xml_document;
    REQUIRE(aligned_xml_document.load_string(aligned_document_xml.c_str()));
    const auto aligned_cell_node =
        aligned_xml_document.child("w:document").child("w:body").child("w:tbl").child("w:tr").child("w:tc");
    REQUIRE(aligned_cell_node != pugi::xml_node{});
    const auto vertical_alignment = aligned_cell_node.child("w:tcPr").child("w:vAlign");
    REQUIRE(vertical_alignment != pugi::xml_node{});
    CHECK_EQ(std::string_view{vertical_alignment.attribute("w:val").value()},
             "center");

    featherdoc::Document reopened(aligned);
    REQUIRE_FALSE(reopened.open());
    auto reopened_cell = reopened.tables().rows().cells();
    REQUIRE(reopened_cell.has_next());
    REQUIRE(reopened_cell.vertical_alignment().has_value());
    CHECK_EQ(*reopened_cell.vertical_alignment(),
             featherdoc::cell_vertical_alignment::center);

    CHECK_EQ(run_cli({"clear-table-cell-vertical-alignment",
                      aligned.string(),
                      "0",
                      "0",
                      "0",
                      "--output",
                      cleared.string(),
                      "--json"},
                     clear_output),
             0);
    CHECK_EQ(
        read_text_file(clear_output),
        std::string{
            "{\"command\":\"clear-table-cell-vertical-alignment\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"table_index\":0,\"row_index\":0,\"cell_index\":0}\n"});

    const auto cleared_document_xml = read_docx_entry(cleared, "word/document.xml");
    pugi::xml_document cleared_xml_document;
    REQUIRE(cleared_xml_document.load_string(cleared_document_xml.c_str()));
    const auto cleared_cell_node =
        cleared_xml_document.child("w:document").child("w:body").child("w:tbl").child("w:tr").child("w:tc");
    REQUIRE(cleared_cell_node != pugi::xml_node{});
    CHECK(cleared_cell_node.child("w:tcPr").child("w:vAlign") == pugi::xml_node{});

    featherdoc::Document reopened_cleared(cleared);
    REQUIRE_FALSE(reopened_cleared.open());
    auto reopened_cleared_cell = reopened_cleared.tables().rows().cells();
    REQUIRE(reopened_cleared_cell.has_next());
    CHECK_FALSE(reopened_cleared_cell.vertical_alignment().has_value());

    remove_if_exists(source);
    remove_if_exists(aligned);
    remove_if_exists(cleared);
    remove_if_exists(set_output);
    remove_if_exists(clear_output);
}

TEST_CASE("cli table cell text direction commands set and clear body cell direction") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_table_cell_text_direction_source.docx";
    const fs::path directed =
        working_directory / "cli_table_cell_text_direction_directed.docx";
    const fs::path cleared =
        working_directory / "cli_table_cell_text_direction_cleared.docx";
    const fs::path set_output =
        working_directory / "cli_table_cell_text_direction_set_output.json";
    const fs::path clear_output =
        working_directory / "cli_table_cell_text_direction_clear_output.json";

    remove_if_exists(source);
    remove_if_exists(directed);
    remove_if_exists(cleared);
    remove_if_exists(set_output);
    remove_if_exists(clear_output);

    create_cli_table_cell_style_fixture(source);

    CHECK_EQ(run_cli({"set-table-cell-text-direction",
                      source.string(),
                      "0",
                      "0",
                      "0",
                      "top_to_bottom_right_to_left",
                      "--output",
                      directed.string(),
                      "--json"},
                     set_output),
             0);
    CHECK_EQ(
        read_text_file(set_output),
        std::string{
            "{\"command\":\"set-table-cell-text-direction\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"table_index\":0,\"row_index\":0,\"cell_index\":0,"
            "\"text_direction\":\"top_to_bottom_right_to_left\"}\n"});

    const auto source_document_xml = read_docx_entry(source, "word/document.xml");
    pugi::xml_document source_xml_document;
    REQUIRE(source_xml_document.load_string(source_document_xml.c_str()));
    const auto source_cell_node =
        source_xml_document.child("w:document").child("w:body").child("w:tbl").child("w:tr").child("w:tc");
    REQUIRE(source_cell_node != pugi::xml_node{});
    CHECK(source_cell_node.child("w:tcPr").child("w:textDirection") ==
          pugi::xml_node{});

    const auto directed_document_xml = read_docx_entry(directed, "word/document.xml");
    pugi::xml_document directed_xml_document;
    REQUIRE(directed_xml_document.load_string(directed_document_xml.c_str()));
    const auto directed_cell_node =
        directed_xml_document.child("w:document").child("w:body").child("w:tbl").child("w:tr").child("w:tc");
    REQUIRE(directed_cell_node != pugi::xml_node{});
    const auto text_direction =
        directed_cell_node.child("w:tcPr").child("w:textDirection");
    REQUIRE(text_direction != pugi::xml_node{});
    CHECK_EQ(std::string_view{text_direction.attribute("w:val").value()}, "tbRl");

    featherdoc::Document reopened(directed);
    REQUIRE_FALSE(reopened.open());
    auto reopened_cell = reopened.tables().rows().cells();
    REQUIRE(reopened_cell.has_next());
    REQUIRE(reopened_cell.text_direction().has_value());
    CHECK_EQ(*reopened_cell.text_direction(),
             featherdoc::cell_text_direction::top_to_bottom_right_to_left);

    CHECK_EQ(run_cli({"clear-table-cell-text-direction",
                      directed.string(),
                      "0",
                      "0",
                      "0",
                      "--output",
                      cleared.string(),
                      "--json"},
                     clear_output),
             0);
    CHECK_EQ(
        read_text_file(clear_output),
        std::string{
            "{\"command\":\"clear-table-cell-text-direction\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"table_index\":0,\"row_index\":0,\"cell_index\":0}\n"});

    const auto cleared_document_xml = read_docx_entry(cleared, "word/document.xml");
    pugi::xml_document cleared_xml_document;
    REQUIRE(cleared_xml_document.load_string(cleared_document_xml.c_str()));
    const auto cleared_cell_node =
        cleared_xml_document.child("w:document").child("w:body").child("w:tbl").child("w:tr").child("w:tc");
    REQUIRE(cleared_cell_node != pugi::xml_node{});
    CHECK(cleared_cell_node.child("w:tcPr").child("w:textDirection") ==
          pugi::xml_node{});

    featherdoc::Document reopened_cleared(cleared);
    REQUIRE_FALSE(reopened_cleared.open());
    auto reopened_cleared_cell = reopened_cleared.tables().rows().cells();
    REQUIRE(reopened_cleared_cell.has_next());
    CHECK_FALSE(reopened_cleared_cell.text_direction().has_value());

    remove_if_exists(source);
    remove_if_exists(directed);
    remove_if_exists(cleared);
    remove_if_exists(set_output);
    remove_if_exists(clear_output);
}

TEST_CASE("cli table cell width commands set and clear body cell width") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_table_cell_width_source.docx";
    const fs::path sized =
        working_directory / "cli_table_cell_width_sized.docx";
    const fs::path cleared =
        working_directory / "cli_table_cell_width_cleared.docx";
    const fs::path set_output =
        working_directory / "cli_table_cell_width_set_output.json";
    const fs::path clear_output =
        working_directory / "cli_table_cell_width_clear_output.json";

    remove_if_exists(source);
    remove_if_exists(sized);
    remove_if_exists(cleared);
    remove_if_exists(set_output);
    remove_if_exists(clear_output);

    create_cli_table_cell_style_fixture(source);

    CHECK_EQ(run_cli({"set-table-cell-width",
                      source.string(),
                      "0",
                      "0",
                      "0",
                      "2222",
                      "--output",
                      sized.string(),
                      "--json"},
                     set_output),
             0);
    CHECK_EQ(
        read_text_file(set_output),
        std::string{
            "{\"command\":\"set-table-cell-width\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"table_index\":0,\"row_index\":0,\"cell_index\":0,"
            "\"width_twips\":2222}\n"});

    const auto sized_document_xml = read_docx_entry(sized, "word/document.xml");
    pugi::xml_document sized_xml_document;
    REQUIRE(sized_xml_document.load_string(sized_document_xml.c_str()));
    const auto sized_cell_node =
        sized_xml_document.child("w:document").child("w:body").child("w:tbl").child("w:tr").child("w:tc");
    REQUIRE(sized_cell_node != pugi::xml_node{});
    const auto width_node = sized_cell_node.child("w:tcPr").child("w:tcW");
    REQUIRE(width_node != pugi::xml_node{});
    CHECK_EQ(std::string_view{width_node.attribute("w:type").value()}, "dxa");
    CHECK_EQ(std::string_view{width_node.attribute("w:w").value()}, "2222");

    featherdoc::Document reopened(sized);
    REQUIRE_FALSE(reopened.open());
    auto reopened_cell = reopened.tables().rows().cells();
    REQUIRE(reopened_cell.has_next());
    REQUIRE(reopened_cell.width_twips().has_value());
    CHECK_EQ(*reopened_cell.width_twips(), 2222U);

    CHECK_EQ(run_cli({"clear-table-cell-width",
                      sized.string(),
                      "0",
                      "0",
                      "0",
                      "--output",
                      cleared.string(),
                      "--json"},
                     clear_output),
             0);
    CHECK_EQ(
        read_text_file(clear_output),
        std::string{
            "{\"command\":\"clear-table-cell-width\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"table_index\":0,\"row_index\":0,\"cell_index\":0}\n"});

    const auto cleared_document_xml = read_docx_entry(cleared, "word/document.xml");
    pugi::xml_document cleared_xml_document;
    REQUIRE(cleared_xml_document.load_string(cleared_document_xml.c_str()));
    const auto cleared_cell_node =
        cleared_xml_document.child("w:document").child("w:body").child("w:tbl").child("w:tr").child("w:tc");
    REQUIRE(cleared_cell_node != pugi::xml_node{});
    CHECK(cleared_cell_node.child("w:tcPr").child("w:tcW") == pugi::xml_node{});

    featherdoc::Document reopened_cleared(cleared);
    REQUIRE_FALSE(reopened_cleared.open());
    auto reopened_cleared_cell = reopened_cleared.tables().rows().cells();
    REQUIRE(reopened_cleared_cell.has_next());
    CHECK_FALSE(reopened_cleared_cell.width_twips().has_value());

    remove_if_exists(source);
    remove_if_exists(sized);
    remove_if_exists(cleared);
    remove_if_exists(set_output);
    remove_if_exists(clear_output);
}

TEST_CASE("cli table column width commands set and clear body column widths") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_table_column_width_source.docx";
    const fs::path sized =
        working_directory / "cli_table_column_width_sized.docx";
    const fs::path cleared =
        working_directory / "cli_table_column_width_cleared.docx";
    const fs::path set_output =
        working_directory / "cli_table_column_width_set_output.json";
    const fs::path clear_output =
        working_directory / "cli_table_column_width_clear_output.json";
    const fs::path error_output =
        working_directory / "cli_table_column_width_error_output.json";

    remove_if_exists(source);
    remove_if_exists(sized);
    remove_if_exists(cleared);
    remove_if_exists(set_output);
    remove_if_exists(clear_output);
    remove_if_exists(error_output);

    create_cli_table_inspection_fixture(source);

    CHECK_EQ(run_cli({"set-table-column-width",
                      source.string(),
                      "0",
                      "1",
                      "2400",
                      "--output",
                      sized.string(),
                      "--json"},
                     set_output),
             0);
    CHECK_EQ(
        read_text_file(set_output),
        std::string{
            "{\"command\":\"set-table-column-width\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"table_index\":0,\"column_index\":1,\"width_twips\":2400}\n"});

    featherdoc::Document reopened_sized(sized);
    REQUIRE_FALSE(reopened_sized.open());
    const auto sized_table = reopened_sized.inspect_table(0U);
    REQUIRE(sized_table.has_value());
    CHECK_EQ(sized_table->column_count, 2U);
    REQUIRE(sized_table->column_widths.size() >= 2U);
    REQUIRE(sized_table->column_widths[0].has_value());
    REQUIRE(sized_table->column_widths[1].has_value());
    CHECK_EQ(*sized_table->column_widths[0], 1800U);
    CHECK_EQ(*sized_table->column_widths[1], 2400U);

    CHECK_EQ(run_cli({"clear-table-column-width",
                      sized.string(),
                      "0",
                      "1",
                      "--output",
                      cleared.string(),
                      "--json"},
                     clear_output),
             0);
    CHECK_EQ(
        read_text_file(clear_output),
        std::string{
            "{\"command\":\"clear-table-column-width\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"table_index\":0,\"column_index\":1}\n"});

    featherdoc::Document reopened_cleared(cleared);
    REQUIRE_FALSE(reopened_cleared.open());
    const auto cleared_table = reopened_cleared.inspect_table(0U);
    REQUIRE(cleared_table.has_value());
    CHECK_EQ(cleared_table->column_count, 2U);
    REQUIRE(cleared_table->column_widths.size() >= 2U);
    REQUIRE(cleared_table->column_widths[0].has_value());
    CHECK_EQ(*cleared_table->column_widths[0], 1800U);
    CHECK_FALSE(cleared_table->column_widths[1].has_value());

    CHECK_EQ(run_cli({"set-table-column-width",
                      source.string(),
                      "0",
                      "9",
                      "2400",
                      "--json"},
                     error_output),
             1);
    const auto error_json = read_text_file(error_output);
    CHECK_NE(error_json.find("\"command\":\"set-table-column-width\""),
             std::string::npos);
    CHECK_NE(error_json.find("\"stage\":\"mutate\""), std::string::npos);
    CHECK_NE(
        error_json.find(
            "\"detail\":\"column index '9' is out of range for table index '0'\""),
        std::string::npos);

    remove_if_exists(source);
    remove_if_exists(sized);
    remove_if_exists(cleared);
    remove_if_exists(set_output);
    remove_if_exists(clear_output);
    remove_if_exists(error_output);
}

TEST_CASE("cli table geometry commands set and clear body table width and layout mode") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_table_geometry_source.docx";
    const fs::path width_set =
        working_directory / "cli_table_geometry_width_set.docx";
    const fs::path width_cleared =
        working_directory / "cli_table_geometry_width_cleared.docx";
    const fs::path layout_set =
        working_directory / "cli_table_geometry_layout_set.docx";
    const fs::path layout_cleared =
        working_directory / "cli_table_geometry_layout_cleared.docx";
    const fs::path width_set_output =
        working_directory / "cli_table_geometry_width_set_output.json";
    const fs::path width_clear_output =
        working_directory / "cli_table_geometry_width_clear_output.json";
    const fs::path layout_set_output =
        working_directory / "cli_table_geometry_layout_set_output.json";
    const fs::path layout_clear_output =
        working_directory / "cli_table_geometry_layout_clear_output.json";
    const fs::path parse_error_output =
        working_directory / "cli_table_geometry_parse_error_output.json";
    const fs::path range_error_output =
        working_directory / "cli_table_geometry_range_error_output.json";

    remove_if_exists(source);
    remove_if_exists(width_set);
    remove_if_exists(width_cleared);
    remove_if_exists(layout_set);
    remove_if_exists(layout_cleared);
    remove_if_exists(width_set_output);
    remove_if_exists(width_clear_output);
    remove_if_exists(layout_set_output);
    remove_if_exists(layout_clear_output);
    remove_if_exists(parse_error_output);
    remove_if_exists(range_error_output);

    create_cli_table_inspection_fixture(source);

    CHECK_EQ(run_cli({"set-table-width",
                      source.string(),
                      "0",
                      "6600",
                      "--output",
                      width_set.string(),
                      "--json"},
                     width_set_output),
             0);
    CHECK_EQ(
        read_text_file(width_set_output),
        std::string{
            "{\"command\":\"set-table-width\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"table_index\":0,\"width_twips\":6600}\n"});

    featherdoc::Document reopened_width_set(width_set);
    REQUIRE_FALSE(reopened_width_set.open());
    const auto sized_table = reopened_width_set.inspect_table(0U);
    REQUIRE(sized_table.has_value());
    REQUIRE(sized_table->width_twips.has_value());
    CHECK_EQ(*sized_table->width_twips, 6600U);

    CHECK_EQ(run_cli({"clear-table-width",
                      width_set.string(),
                      "0",
                      "--output",
                      width_cleared.string(),
                      "--json"},
                     width_clear_output),
             0);
    CHECK_EQ(
        read_text_file(width_clear_output),
        std::string{
            "{\"command\":\"clear-table-width\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"table_index\":0}\n"});

    featherdoc::Document reopened_width_cleared(width_cleared);
    REQUIRE_FALSE(reopened_width_cleared.open());
    const auto cleared_width_table = reopened_width_cleared.inspect_table(0U);
    REQUIRE(cleared_width_table.has_value());
    CHECK_FALSE(cleared_width_table->width_twips.has_value());

    CHECK_EQ(run_cli({"set-table-layout-mode",
                      width_cleared.string(),
                      "0",
                      "fixed",
                      "--output",
                      layout_set.string(),
                      "--json"},
                     layout_set_output),
             0);
    CHECK_EQ(
        read_text_file(layout_set_output),
        std::string{
            "{\"command\":\"set-table-layout-mode\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"table_index\":0,\"layout_mode\":\"fixed\"}\n"});

    featherdoc::Document reopened_layout_set(layout_set);
    REQUIRE_FALSE(reopened_layout_set.open());
    auto layout_table = reopened_layout_set.tables();
    REQUIRE(layout_table.has_next());
    REQUIRE(layout_table.layout_mode().has_value());
    CHECK_EQ(*layout_table.layout_mode(), featherdoc::table_layout_mode::fixed);

    CHECK_EQ(run_cli({"clear-table-layout-mode",
                      layout_set.string(),
                      "0",
                      "--output",
                      layout_cleared.string(),
                      "--json"},
                     layout_clear_output),
             0);
    CHECK_EQ(
        read_text_file(layout_clear_output),
        std::string{
            "{\"command\":\"clear-table-layout-mode\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"table_index\":0}\n"});

    featherdoc::Document reopened_layout_cleared(layout_cleared);
    REQUIRE_FALSE(reopened_layout_cleared.open());
    auto cleared_layout_table = reopened_layout_cleared.tables();
    REQUIRE(cleared_layout_table.has_next());
    CHECK_FALSE(cleared_layout_table.layout_mode().has_value());

    CHECK_EQ(run_cli({"set-table-layout-mode",
                      source.string(),
                      "0",
                      "fluid",
                      "--json"},
                     parse_error_output),
             2);
    CHECK_EQ(
        read_text_file(parse_error_output),
        std::string{
            "{\"command\":\"set-table-layout-mode\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"invalid table layout mode: fluid\"}\n"});

    CHECK_EQ(
        run_cli({"clear-table-width", source.string(), "9", "--json"},
                range_error_output),
        1);
    const auto range_error_json = read_text_file(range_error_output);
    CHECK_NE(range_error_json.find("\"command\":\"clear-table-width\""),
             std::string::npos);
    CHECK_NE(range_error_json.find("\"stage\":\"mutate\""), std::string::npos);
    CHECK_NE(range_error_json.find("\"detail\":\"table index '9' is out of range\""),
             std::string::npos);

    remove_if_exists(source);
    remove_if_exists(width_set);
    remove_if_exists(width_cleared);
    remove_if_exists(layout_set);
    remove_if_exists(layout_cleared);
    remove_if_exists(width_set_output);
    remove_if_exists(width_clear_output);
    remove_if_exists(layout_set_output);
    remove_if_exists(layout_clear_output);
    remove_if_exists(parse_error_output);
    remove_if_exists(range_error_output);
}

TEST_CASE("cli table placement commands set and clear body table metadata") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_table_placement_source.docx";
    const fs::path aligned =
        working_directory / "cli_table_placement_aligned.docx";
    const fs::path alignment_cleared =
        working_directory / "cli_table_placement_alignment_cleared.docx";
    const fs::path indented =
        working_directory / "cli_table_placement_indented.docx";
    const fs::path indent_cleared =
        working_directory / "cli_table_placement_indent_cleared.docx";
    const fs::path spaced =
        working_directory / "cli_table_placement_spaced.docx";
    const fs::path spacing_cleared =
        working_directory / "cli_table_placement_spacing_cleared.docx";
    const fs::path alignment_output =
        working_directory / "cli_table_placement_alignment_output.json";
    const fs::path alignment_clear_output =
        working_directory / "cli_table_placement_alignment_clear_output.json";
    const fs::path indent_output =
        working_directory / "cli_table_placement_indent_output.json";
    const fs::path indent_clear_output =
        working_directory / "cli_table_placement_indent_clear_output.json";
    const fs::path spacing_output =
        working_directory / "cli_table_placement_spacing_output.json";
    const fs::path spacing_clear_output =
        working_directory / "cli_table_placement_spacing_clear_output.json";
    const fs::path parse_error_output =
        working_directory / "cli_table_placement_parse_error_output.json";

    remove_if_exists(source);
    remove_if_exists(aligned);
    remove_if_exists(alignment_cleared);
    remove_if_exists(indented);
    remove_if_exists(indent_cleared);
    remove_if_exists(spaced);
    remove_if_exists(spacing_cleared);
    remove_if_exists(alignment_output);
    remove_if_exists(alignment_clear_output);
    remove_if_exists(indent_output);
    remove_if_exists(indent_clear_output);
    remove_if_exists(spacing_output);
    remove_if_exists(spacing_clear_output);
    remove_if_exists(parse_error_output);

    create_cli_table_inspection_fixture(source);

    CHECK_EQ(run_cli({"set-table-alignment",
                      source.string(),
                      "0",
                      "center",
                      "--output",
                      aligned.string(),
                      "--json"},
                     alignment_output),
             0);
    CHECK_EQ(
        read_text_file(alignment_output),
        std::string{
            "{\"command\":\"set-table-alignment\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"table_index\":0,\"alignment\":\"center\"}\n"});

    featherdoc::Document reopened_aligned(aligned);
    REQUIRE_FALSE(reopened_aligned.open());
    const auto aligned_table = reopened_aligned.inspect_table(0U);
    REQUIRE(aligned_table.has_value());
    REQUIRE(aligned_table->alignment.has_value());
    CHECK_EQ(*aligned_table->alignment, featherdoc::table_alignment::center);

    CHECK_EQ(run_cli({"clear-table-alignment",
                      aligned.string(),
                      "0",
                      "--output",
                      alignment_cleared.string(),
                      "--json"},
                     alignment_clear_output),
             0);
    CHECK_EQ(
        read_text_file(alignment_clear_output),
        std::string{
            "{\"command\":\"clear-table-alignment\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"table_index\":0}\n"});

    featherdoc::Document reopened_alignment_cleared(alignment_cleared);
    REQUIRE_FALSE(reopened_alignment_cleared.open());
    const auto alignment_cleared_table =
        reopened_alignment_cleared.inspect_table(0U);
    REQUIRE(alignment_cleared_table.has_value());
    CHECK_FALSE(alignment_cleared_table->alignment.has_value());

    CHECK_EQ(run_cli({"set-table-indent",
                      source.string(),
                      "0",
                      "360",
                      "--output",
                      indented.string(),
                      "--json"},
                     indent_output),
             0);
    CHECK_EQ(
        read_text_file(indent_output),
        std::string{
            "{\"command\":\"set-table-indent\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"table_index\":0,\"indent_twips\":360}\n"});

    featherdoc::Document reopened_indented(indented);
    REQUIRE_FALSE(reopened_indented.open());
    const auto indented_table = reopened_indented.inspect_table(0U);
    REQUIRE(indented_table.has_value());
    REQUIRE(indented_table->indent_twips.has_value());
    CHECK_EQ(*indented_table->indent_twips, 360U);

    CHECK_EQ(run_cli({"clear-table-indent",
                      indented.string(),
                      "0",
                      "--output",
                      indent_cleared.string(),
                      "--json"},
                     indent_clear_output),
             0);
    CHECK_EQ(
        read_text_file(indent_clear_output),
        std::string{
            "{\"command\":\"clear-table-indent\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"table_index\":0}\n"});

    featherdoc::Document reopened_indent_cleared(indent_cleared);
    REQUIRE_FALSE(reopened_indent_cleared.open());
    const auto indent_cleared_table = reopened_indent_cleared.inspect_table(0U);
    REQUIRE(indent_cleared_table.has_value());
    CHECK_FALSE(indent_cleared_table->indent_twips.has_value());

    CHECK_EQ(run_cli({"set-table-cell-spacing",
                      source.string(),
                      "0",
                      "180",
                      "--output",
                      spaced.string(),
                      "--json"},
                     spacing_output),
             0);
    CHECK_EQ(
        read_text_file(spacing_output),
        std::string{
            "{\"command\":\"set-table-cell-spacing\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"table_index\":0,\"cell_spacing_twips\":180}\n"});

    featherdoc::Document reopened_spaced(spaced);
    REQUIRE_FALSE(reopened_spaced.open());
    const auto spaced_table = reopened_spaced.inspect_table(0U);
    REQUIRE(spaced_table.has_value());
    REQUIRE(spaced_table->cell_spacing_twips.has_value());
    CHECK_EQ(*spaced_table->cell_spacing_twips, 180U);

    CHECK_EQ(run_cli({"clear-table-cell-spacing",
                      spaced.string(),
                      "0",
                      "--output",
                      spacing_cleared.string(),
                      "--json"},
                     spacing_clear_output),
             0);
    CHECK_EQ(
        read_text_file(spacing_clear_output),
        std::string{
            "{\"command\":\"clear-table-cell-spacing\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"table_index\":0}\n"});

    featherdoc::Document reopened_spacing_cleared(spacing_cleared);
    REQUIRE_FALSE(reopened_spacing_cleared.open());
    const auto spacing_cleared_table =
        reopened_spacing_cleared.inspect_table(0U);
    REQUIRE(spacing_cleared_table.has_value());
    CHECK_FALSE(spacing_cleared_table->cell_spacing_twips.has_value());

    CHECK_EQ(run_cli({"set-table-alignment",
                      source.string(),
                      "0",
                      "middle",
                      "--json"},
                     parse_error_output),
             2);
    CHECK_EQ(
        read_text_file(parse_error_output),
        std::string{
            "{\"command\":\"set-table-alignment\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"invalid table alignment: middle\"}\n"});

    remove_if_exists(source);
    remove_if_exists(aligned);
    remove_if_exists(alignment_cleared);
    remove_if_exists(indented);
    remove_if_exists(indent_cleared);
    remove_if_exists(spaced);
    remove_if_exists(spacing_cleared);
    remove_if_exists(alignment_output);
    remove_if_exists(alignment_clear_output);
    remove_if_exists(indent_output);
    remove_if_exists(indent_clear_output);
    remove_if_exists(spacing_output);
    remove_if_exists(spacing_clear_output);
    remove_if_exists(parse_error_output);
}

TEST_CASE("cli table default margin and border commands set and clear body table metadata") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_table_default_style_source.docx";
    const fs::path margined =
        working_directory / "cli_table_default_style_margined.docx";
    const fs::path margin_cleared =
        working_directory / "cli_table_default_style_margin_cleared.docx";
    const fs::path bordered =
        working_directory / "cli_table_default_style_bordered.docx";
    const fs::path border_cleared =
        working_directory / "cli_table_default_style_border_cleared.docx";
    const fs::path margin_output =
        working_directory / "cli_table_default_margin_set_output.json";
    const fs::path margin_clear_output =
        working_directory / "cli_table_default_margin_clear_output.json";
    const fs::path border_output =
        working_directory / "cli_table_border_set_output.json";
    const fs::path border_clear_output =
        working_directory / "cli_table_border_clear_output.json";
    const fs::path inspect_output =
        working_directory / "cli_table_default_style_inspect_output.json";
    const fs::path parse_error_output =
        working_directory / "cli_table_border_parse_output.json";

    remove_if_exists(source);
    remove_if_exists(margined);
    remove_if_exists(margin_cleared);
    remove_if_exists(bordered);
    remove_if_exists(border_cleared);
    remove_if_exists(margin_output);
    remove_if_exists(margin_clear_output);
    remove_if_exists(border_output);
    remove_if_exists(border_clear_output);
    remove_if_exists(inspect_output);
    remove_if_exists(parse_error_output);

    create_cli_table_cell_style_fixture(source);

    CHECK_EQ(run_cli({"set-table-default-cell-margin",
                      source.string(),
                      "0",
                      "left",
                      "240",
                      "--output",
                      margined.string(),
                      "--json"},
                     margin_output),
             0);
    CHECK_EQ(
        read_text_file(margin_output),
        std::string{
            "{\"command\":\"set-table-default-cell-margin\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"table_index\":0,\"edge\":\"left\",\"margin_twips\":240}\n"});

    const auto margined_document_xml =
        read_docx_entry(margined, "word/document.xml");
    pugi::xml_document margined_xml_document;
    REQUIRE(margined_xml_document.load_string(margined_document_xml.c_str()));
    const auto table_left_margin =
        margined_xml_document.child("w:document")
            .child("w:body")
            .child("w:tbl")
            .child("w:tblPr")
            .child("w:tblCellMar")
            .child("w:left");
    REQUIRE(table_left_margin != pugi::xml_node{});
    CHECK_EQ(std::string_view{table_left_margin.attribute("w:w").value()},
             "240");
    CHECK_EQ(std::string_view{table_left_margin.attribute("w:type").value()},
             "dxa");

    featherdoc::Document reopened_margined(margined);
    REQUIRE_FALSE(reopened_margined.open());
    const auto margined_table = reopened_margined.inspect_table(0U);
    REQUIRE(margined_table.has_value());
    REQUIRE(margined_table->cell_margin_left_twips.has_value());
    CHECK_EQ(*margined_table->cell_margin_left_twips, 240U);

    CHECK_EQ(run_cli({"inspect-tables", margined.string(), "--table", "0", "--json"},
                     inspect_output),
             0);
    const auto margined_inspect_json = read_text_file(inspect_output);
    CHECK_NE(
        margined_inspect_json.find(
            "\"cell_margins\":{\"top\":null,\"left\":240,"
            "\"bottom\":null,\"right\":null}"),
        std::string::npos);

    CHECK_EQ(run_cli({"clear-table-default-cell-margin",
                      margined.string(),
                      "0",
                      "left",
                      "--output",
                      margin_cleared.string(),
                      "--json"},
                     margin_clear_output),
             0);
    CHECK_EQ(
        read_text_file(margin_clear_output),
        std::string{
            "{\"command\":\"clear-table-default-cell-margin\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"table_index\":0,\"edge\":\"left\"}\n"});

    featherdoc::Document reopened_margin_cleared(margin_cleared);
    REQUIRE_FALSE(reopened_margin_cleared.open());
    const auto margin_cleared_table =
        reopened_margin_cleared.inspect_table(0U);
    REQUIRE(margin_cleared_table.has_value());
    CHECK_FALSE(margin_cleared_table->cell_margin_left_twips.has_value());

    CHECK_EQ(run_cli({"set-table-border",
                      source.string(),
                      "0",
                      "inside_horizontal",
                      "--style",
                      "dashed",
                      "--size",
                      "8",
                      "--color",
                      "00AA00",
                      "--space",
                      "2",
                      "--output",
                      bordered.string(),
                      "--json"},
                     border_output),
             0);
    CHECK_EQ(
        read_text_file(border_output),
        std::string{
            "{\"command\":\"set-table-border\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"table_index\":0,\"edge\":\"inside_horizontal\","
            "\"style\":\"dashed\",\"size_eighth_points\":8,"
            "\"color\":\"00AA00\",\"space_points\":2}\n"});

    const auto bordered_document_xml =
        read_docx_entry(bordered, "word/document.xml");
    pugi::xml_document bordered_xml_document;
    REQUIRE(bordered_xml_document.load_string(bordered_document_xml.c_str()));
    const auto inside_horizontal_border =
        bordered_xml_document.child("w:document")
            .child("w:body")
            .child("w:tbl")
            .child("w:tblPr")
            .child("w:tblBorders")
            .child("w:insideH");
    REQUIRE(inside_horizontal_border != pugi::xml_node{});
    CHECK_EQ(std::string_view{inside_horizontal_border.attribute("w:val").value()},
             "dashed");
    CHECK_EQ(std::string_view{inside_horizontal_border.attribute("w:sz").value()},
             "8");
    CHECK_EQ(
        std::string_view{inside_horizontal_border.attribute("w:space").value()},
        "2");
    CHECK_EQ(
        std::string_view{inside_horizontal_border.attribute("w:color").value()},
        "00AA00");

    featherdoc::Document reopened_bordered(bordered);
    REQUIRE_FALSE(reopened_bordered.open());
    const auto bordered_table = reopened_bordered.inspect_table(0U);
    REQUIRE(bordered_table.has_value());
    REQUIRE(bordered_table->borders.has_value());
    REQUIRE(bordered_table->borders->inside_horizontal.has_value());
    CHECK_EQ(bordered_table->borders->inside_horizontal->style,
             featherdoc::border_style::dashed);
    CHECK_EQ(bordered_table->borders->inside_horizontal->size_eighth_points,
             8U);
    CHECK_EQ(bordered_table->borders->inside_horizontal->color, "00AA00");
    CHECK_EQ(bordered_table->borders->inside_horizontal->space_points, 2U);

    CHECK_EQ(run_cli({"inspect-tables", bordered.string(), "--table", "0", "--json"},
                     inspect_output),
             0);
    const auto bordered_inspect_json = read_text_file(inspect_output);
    CHECK_NE(
        bordered_inspect_json.find(
            "\"inside_horizontal\":{\"style\":\"dashed\","
            "\"size_eighth_points\":8,\"color\":\"00AA00\","
            "\"space_points\":2}"),
        std::string::npos);

    CHECK_EQ(run_cli({"clear-table-border",
                      bordered.string(),
                      "0",
                      "inside_horizontal",
                      "--output",
                      border_cleared.string(),
                      "--json"},
                     border_clear_output),
             0);
    CHECK_EQ(
        read_text_file(border_clear_output),
        std::string{
            "{\"command\":\"clear-table-border\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"table_index\":0,\"edge\":\"inside_horizontal\"}\n"});

    featherdoc::Document reopened_border_cleared(border_cleared);
    REQUIRE_FALSE(reopened_border_cleared.open());
    const auto border_cleared_table =
        reopened_border_cleared.inspect_table(0U);
    REQUIRE(border_cleared_table.has_value());
    CHECK_FALSE(border_cleared_table->borders.has_value());

    CHECK_EQ(run_cli({"set-table-border",
                      source.string(),
                      "0",
                      "diagonal",
                      "--style",
                      "single",
                      "--json"},
                     parse_error_output),
             2);
    CHECK_EQ(
        read_text_file(parse_error_output),
        std::string{
            "{\"command\":\"set-table-border\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"invalid table border edge: "
            "diagonal\"}\n"});

    remove_if_exists(source);
    remove_if_exists(margined);
    remove_if_exists(margin_cleared);
    remove_if_exists(bordered);
    remove_if_exists(border_cleared);
    remove_if_exists(margin_output);
    remove_if_exists(margin_clear_output);
    remove_if_exists(border_output);
    remove_if_exists(border_clear_output);
    remove_if_exists(inspect_output);
    remove_if_exists(parse_error_output);
}

TEST_CASE("cli table cell margin commands set and clear body cell margins") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_table_cell_margin_source.docx";
    const fs::path margined =
        working_directory / "cli_table_cell_margin_margined.docx";
    const fs::path cleared =
        working_directory / "cli_table_cell_margin_cleared.docx";
    const fs::path set_output =
        working_directory / "cli_table_cell_margin_set_output.json";
    const fs::path clear_output =
        working_directory / "cli_table_cell_margin_clear_output.json";

    remove_if_exists(source);
    remove_if_exists(margined);
    remove_if_exists(cleared);
    remove_if_exists(set_output);
    remove_if_exists(clear_output);

    create_cli_table_cell_style_fixture(source);

    CHECK_EQ(run_cli({"set-table-cell-margin",
                      source.string(),
                      "0",
                      "0",
                      "0",
                      "left",
                      "240",
                      "--output",
                      margined.string(),
                      "--json"},
                     set_output),
             0);
    CHECK_EQ(
        read_text_file(set_output),
        std::string{
            "{\"command\":\"set-table-cell-margin\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"table_index\":0,\"row_index\":0,\"cell_index\":0,"
            "\"edge\":\"left\",\"margin_twips\":240}\n"});

    const auto margined_document_xml =
        read_docx_entry(margined, "word/document.xml");
    pugi::xml_document margined_xml_document;
    REQUIRE(margined_xml_document.load_string(margined_document_xml.c_str()));
    const auto margined_cell_node =
        margined_xml_document.child("w:document").child("w:body").child("w:tbl").child("w:tr").child("w:tc");
    REQUIRE(margined_cell_node != pugi::xml_node{});
    const auto left_margin =
        margined_cell_node.child("w:tcPr").child("w:tcMar").child("w:left");
    REQUIRE(left_margin != pugi::xml_node{});
    CHECK_EQ(std::string_view{left_margin.attribute("w:w").value()}, "240");
    CHECK_EQ(std::string_view{left_margin.attribute("w:type").value()}, "dxa");

    featherdoc::Document reopened(margined);
    REQUIRE_FALSE(reopened.open());
    auto reopened_cell = reopened.tables().rows().cells();
    REQUIRE(reopened_cell.has_next());
    REQUIRE(reopened_cell.margin_twips(featherdoc::cell_margin_edge::left).has_value());
    CHECK_EQ(*reopened_cell.margin_twips(featherdoc::cell_margin_edge::left),
             240U);

    CHECK_EQ(run_cli({"clear-table-cell-margin",
                      margined.string(),
                      "0",
                      "0",
                      "0",
                      "left",
                      "--output",
                      cleared.string(),
                      "--json"},
                     clear_output),
             0);
    CHECK_EQ(
        read_text_file(clear_output),
        std::string{
            "{\"command\":\"clear-table-cell-margin\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"table_index\":0,\"row_index\":0,\"cell_index\":0,"
            "\"edge\":\"left\"}\n"});

    const auto cleared_document_xml = read_docx_entry(cleared, "word/document.xml");
    pugi::xml_document cleared_xml_document;
    REQUIRE(cleared_xml_document.load_string(cleared_document_xml.c_str()));
    const auto cleared_cell_node =
        cleared_xml_document.child("w:document").child("w:body").child("w:tbl").child("w:tr").child("w:tc");
    REQUIRE(cleared_cell_node != pugi::xml_node{});
    CHECK(cleared_cell_node.child("w:tcPr").child("w:tcMar") == pugi::xml_node{});

    featherdoc::Document reopened_cleared(cleared);
    REQUIRE_FALSE(reopened_cleared.open());
    auto reopened_cleared_cell = reopened_cleared.tables().rows().cells();
    REQUIRE(reopened_cleared_cell.has_next());
    CHECK_FALSE(reopened_cleared_cell.margin_twips(
        featherdoc::cell_margin_edge::left).has_value());

    remove_if_exists(source);
    remove_if_exists(margined);
    remove_if_exists(cleared);
    remove_if_exists(set_output);
    remove_if_exists(clear_output);
}

TEST_CASE("cli table cell border commands set and clear body cell borders") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_table_cell_border_source.docx";
    const fs::path bordered =
        working_directory / "cli_table_cell_border_bordered.docx";
    const fs::path cleared =
        working_directory / "cli_table_cell_border_cleared.docx";
    const fs::path set_output =
        working_directory / "cli_table_cell_border_set_output.json";
    const fs::path clear_output =
        working_directory / "cli_table_cell_border_clear_output.json";

    remove_if_exists(source);
    remove_if_exists(bordered);
    remove_if_exists(cleared);
    remove_if_exists(set_output);
    remove_if_exists(clear_output);

    create_cli_table_cell_style_fixture(source);

    CHECK_EQ(run_cli({"set-table-cell-border",
                      source.string(),
                      "0",
                      "0",
                      "0",
                      "right",
                      "--style",
                      "dotted",
                      "--size",
                      "6",
                      "--color",
                      "0000FF",
                      "--space",
                      "1",
                      "--output",
                      bordered.string(),
                      "--json"},
                     set_output),
             0);
    CHECK_EQ(
        read_text_file(set_output),
        std::string{
            "{\"command\":\"set-table-cell-border\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"table_index\":0,\"row_index\":0,\"cell_index\":0,"
            "\"edge\":\"right\",\"style\":\"dotted\","
            "\"size_eighth_points\":6,\"color\":\"0000FF\","
            "\"space_points\":1}\n"});

    const auto bordered_document_xml =
        read_docx_entry(bordered, "word/document.xml");
    pugi::xml_document bordered_xml_document;
    REQUIRE(bordered_xml_document.load_string(bordered_document_xml.c_str()));
    const auto bordered_cell_node =
        bordered_xml_document.child("w:document").child("w:body").child("w:tbl").child("w:tr").child("w:tc");
    REQUIRE(bordered_cell_node != pugi::xml_node{});
    const auto right_border =
        bordered_cell_node.child("w:tcPr").child("w:tcBorders").child("w:right");
    REQUIRE(right_border != pugi::xml_node{});
    CHECK_EQ(std::string_view{right_border.attribute("w:val").value()}, "dotted");
    CHECK_EQ(std::string_view{right_border.attribute("w:sz").value()}, "6");
    CHECK_EQ(std::string_view{right_border.attribute("w:space").value()}, "1");
    CHECK_EQ(std::string_view{right_border.attribute("w:color").value()}, "0000FF");

    featherdoc::Document reopened(bordered);
    REQUIRE_FALSE(reopened.open());
    auto reopened_cell = reopened.tables().rows().cells();
    REQUIRE(reopened_cell.has_next());

    CHECK_EQ(run_cli({"clear-table-cell-border",
                      bordered.string(),
                      "0",
                      "0",
                      "0",
                      "right",
                      "--output",
                      cleared.string(),
                      "--json"},
                     clear_output),
             0);
    CHECK_EQ(
        read_text_file(clear_output),
        std::string{
            "{\"command\":\"clear-table-cell-border\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"table_index\":0,\"row_index\":0,\"cell_index\":0,"
            "\"edge\":\"right\"}\n"});

    const auto cleared_document_xml = read_docx_entry(cleared, "word/document.xml");
    pugi::xml_document cleared_xml_document;
    REQUIRE(cleared_xml_document.load_string(cleared_document_xml.c_str()));
    const auto cleared_cell_node =
        cleared_xml_document.child("w:document").child("w:body").child("w:tbl").child("w:tr").child("w:tc");
    REQUIRE(cleared_cell_node != pugi::xml_node{});
    CHECK(cleared_cell_node.child("w:tcPr").child("w:tcBorders").child("w:right") ==
          pugi::xml_node{});

    featherdoc::Document reopened_cleared(cleared);
    REQUIRE_FALSE(reopened_cleared.open());
    auto reopened_cleared_cell = reopened_cleared.tables().rows().cells();
    REQUIRE(reopened_cleared_cell.has_next());

    remove_if_exists(source);
    remove_if_exists(bordered);
    remove_if_exists(cleared);
    remove_if_exists(set_output);
    remove_if_exists(clear_output);
}

TEST_CASE("cli table cell style and geometry commands report parse and mutate errors") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_table_cell_style_error_source.docx";
    const fs::path parse_output =
        working_directory / "cli_table_cell_style_parse_output.json";
    const fs::path mutate_output =
        working_directory / "cli_table_cell_style_mutate_output.json";

    remove_if_exists(source);
    remove_if_exists(parse_output);
    remove_if_exists(mutate_output);

    create_cli_table_cell_style_fixture(source);

    CHECK_EQ(run_cli({"set-table-cell-vertical-alignment",
                      source.string(),
                      "0",
                      "0",
                      "0",
                      "middle",
                      "--json"},
                     parse_output),
             2);
    CHECK_EQ(
        read_text_file(parse_output),
        std::string{
            "{\"command\":\"set-table-cell-vertical-alignment\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"invalid vertical alignment: "
            "middle\"}\n"});

    CHECK_EQ(run_cli({"set-table-cell-border",
                      source.string(),
                      "0",
                      "0",
                      "0",
                      "right",
                      "--style",
                      "groove",
                      "--json"},
                     parse_output),
             2);
    CHECK_EQ(
        read_text_file(parse_output),
        std::string{
            "{\"command\":\"set-table-cell-border\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"invalid border style: "
            "groove\"}\n"});

    CHECK_EQ(run_cli({"set-table-cell-text-direction",
                      source.string(),
                      "0",
                      "0",
                      "9",
                      "top_to_bottom_right_to_left",
                      "--json"},
                     mutate_output),
             1);
    const auto mutate_json = read_text_file(mutate_output);
    CHECK_NE(mutate_json.find("\"command\":\"set-table-cell-text-direction\""),
             std::string::npos);
    CHECK_NE(mutate_json.find("\"stage\":\"mutate\""), std::string::npos);
    CHECK_NE(
        mutate_json.find(
            "\"detail\":\"cell index '9' is out of range for row index '0' in "
            "table index '0'\""),
        std::string::npos);
    CHECK_NE(mutate_json.find("\"entry\":\"word/document.xml\""),
             std::string::npos);

    CHECK_EQ(run_cli({"set-table-cell-width",
                      source.string(),
                      "0",
                      "0",
                      "9",
                      "2400",
                      "--json"},
                     mutate_output),
             1);
    const auto width_mutate_json = read_text_file(mutate_output);
    CHECK_NE(width_mutate_json.find("\"command\":\"set-table-cell-width\""),
             std::string::npos);
    CHECK_NE(width_mutate_json.find("\"stage\":\"mutate\""), std::string::npos);
    CHECK_NE(
        width_mutate_json.find(
            "\"detail\":\"cell index '9' is out of range for row index '0' in "
            "table index '0'\""),
        std::string::npos);
    CHECK_NE(width_mutate_json.find("\"entry\":\"word/document.xml\""),
             std::string::npos);

    remove_if_exists(source);
    remove_if_exists(parse_output);
    remove_if_exists(mutate_output);
}

} // namespace
