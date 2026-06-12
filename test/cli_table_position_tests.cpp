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


} // namespace
