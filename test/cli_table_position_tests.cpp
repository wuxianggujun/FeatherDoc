#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "cli_table_test_support.hpp"

namespace {

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

} // namespace
