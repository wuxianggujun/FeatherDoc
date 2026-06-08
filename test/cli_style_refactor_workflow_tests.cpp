#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "cli_style_test_support.hpp"

TEST_CASE("cli plan-style-refactor validates batch rename and merge operations") {
    const fs::path working_directory = fs::current_path();
    const fs::path source = working_directory / "cli_style_refactor_plan_source.docx";
    const fs::path clean_output = working_directory / "cli_style_refactor_plan_clean.json";
    const fs::path dirty_output = working_directory / "cli_style_refactor_plan_dirty.json";
    const fs::path clean_plan_file =
        working_directory / "cli_style_refactor_plan_clean.plan.json";
    const fs::path parse_output = working_directory / "cli_style_refactor_plan_parse.json";

    remove_if_exists(source);
    remove_if_exists(clean_output);
    remove_if_exists(dirty_output);
    remove_if_exists(clean_plan_file);
    remove_if_exists(parse_output);

    create_cli_style_usage_fixture(source);

    CHECK_EQ(run_cli({"plan-style-refactor",
                      source.string(),
                      "--rename",
                      "CustomBody:ReviewBody",
                      "--merge",
                      "CustomBody:Normal",
                      "--output-plan",
                      clean_plan_file.string(),
                      "--json"},
                     clean_output),
             0);
    const auto clean_json = read_text_file(clean_output);
    CHECK_NE(clean_json.find(R"("command":"plan-style-refactor")"),
             std::string::npos);
    CHECK_NE(clean_json.find(R"("clean":true)"), std::string::npos);
    CHECK_NE(clean_json.find(R"("operation_count":2)"), std::string::npos);
    CHECK_NE(clean_json.find(R"("applyable_count":2)"), std::string::npos);
    CHECK_NE(clean_json.find(R"("source_reference_count":3)"), std::string::npos);
    CHECK_NE(clean_json.find(
                 R"("command_template":"featherdoc_cli rename-style <input.docx> CustomBody ReviewBody --output <output.docx> --json")"),
             std::string::npos);
    CHECK_NE(clean_json.find(
                 R"("command_template":"featherdoc_cli merge-style <input.docx> CustomBody Normal --output <output.docx> --json")"),
             std::string::npos);
    const auto clean_plan_json = read_text_file(clean_plan_file);
    CHECK_NE(clean_plan_json.find(R"("command":"plan-style-refactor")"),
             std::string::npos);
    CHECK_NE(clean_plan_json.find(R"("operations":[)"), std::string::npos);

    CHECK_EQ(run_cli({"plan-style-refactor",
                      source.string(),
                      "--merge",
                      "Strong:Normal",
                      "--rename",
                      "MissingBody:NewBody",
                      "--rename",
                      "CustomBody:Strong",
                      "--json"},
                     dirty_output),
             1);
    const auto dirty_json = read_text_file(dirty_output);
    CHECK_NE(dirty_json.find(R"("clean":false)"), std::string::npos);
    CHECK_NE(dirty_json.find(R"("operation_count":3)"), std::string::npos);
    CHECK_NE(dirty_json.find(R"("applyable_count":0)"), std::string::npos);
    CHECK_NE(dirty_json.find(R"("issue_count":3)"), std::string::npos);
    CHECK_NE(dirty_json.find(R"("code":"style_type_mismatch")"), std::string::npos);
    CHECK_NE(dirty_json.find(R"("code":"missing_source_style")"), std::string::npos);
    CHECK_NE(dirty_json.find(R"("code":"target_style_exists")"), std::string::npos);

    const auto source_styles_xml = read_docx_entry(source, "word/styles.xml");
    CHECK_NE(source_styles_xml.find(R"(w:styleId="CustomBody")"), std::string::npos);
    CHECK_EQ(source_styles_xml.find(R"(w:styleId="ReviewBody")"), std::string::npos);

    CHECK_EQ(run_cli({"plan-style-refactor", source.string(), "--json"}, parse_output), 2);
    CHECK_NE(read_text_file(parse_output)
                 .find("plan-style-refactor expects at least one --rename or --merge option"),
             std::string::npos);

    remove_if_exists(source);
    remove_if_exists(clean_output);
    remove_if_exists(dirty_output);
    remove_if_exists(clean_plan_file);
    remove_if_exists(parse_output);
}

TEST_CASE("cli suggest-style-merges writes reviewable duplicate merge plan") {
    const fs::path working_directory = fs::current_path();
    const fs::path source = working_directory / "cli_suggest_style_merges_source.docx";
    const fs::path output = working_directory / "cli_suggest_style_merges_output.json";
    const fs::path filtered_output =
        working_directory / "cli_suggest_style_merges_filtered.json";
    const fs::path gate_output =
        working_directory / "cli_suggest_style_merges_gate.json";
    const fs::path filtered_gate_output =
        working_directory / "cli_suggest_style_merges_filtered_gate.json";
    const fs::path plan_file =
        working_directory / "cli_suggest_style_merges_plan.json";
    const fs::path filtered_plan_file =
        working_directory / "cli_suggest_style_merges_filtered.plan.json";
    const fs::path applied = working_directory / "cli_suggest_style_merges_applied.docx";
    const fs::path apply_output =
        working_directory / "cli_suggest_style_merges_apply.json";
    const fs::path rollback_file =
        working_directory / "cli_suggest_style_merges_rollback.json";
    const fs::path restored =
        working_directory / "cli_suggest_style_merges_restored.docx";
    const fs::path restore_output =
        working_directory / "cli_suggest_style_merges_restore.json";
    const fs::path dry_run_output =
        working_directory / "cli_suggest_style_merges_restore_dry_run.json";
    const fs::path parse_output =
        working_directory / "cli_suggest_style_merges_parse.json";

    remove_if_exists(source);
    remove_if_exists(output);
    remove_if_exists(filtered_output);
    remove_if_exists(gate_output);
    remove_if_exists(filtered_gate_output);
    remove_if_exists(plan_file);
    remove_if_exists(filtered_plan_file);
    remove_if_exists(applied);
    remove_if_exists(apply_output);
    remove_if_exists(rollback_file);
    remove_if_exists(restored);
    remove_if_exists(restore_output);
    remove_if_exists(dry_run_output);
    remove_if_exists(parse_output);

    create_cli_duplicate_style_fixture(source);

    CHECK_EQ(run_cli({"suggest-style-merges",
                      source.string(),
                      "--output-plan",
                      plan_file.string(),
                      "--json"},
                     output),
             0);

    const auto suggestion_json = read_text_file(output);
    CHECK_NE(suggestion_json.find(R"("command":"suggest-style-merges")"),
             std::string::npos);
    CHECK_NE(suggestion_json.find(R"("fail_on_suggestion":false)"),
             std::string::npos);
    CHECK_NE(suggestion_json.find(R"("suggestion_gate_failed":false)"),
             std::string::npos);
    CHECK_NE(suggestion_json.find(R"("operation_count":1)"), std::string::npos);
    CHECK_NE(suggestion_json.find(R"("action":"merge")"), std::string::npos);
    CHECK_NE(suggestion_json.find(R"("source_style_id":"DuplicateBodyB")"),
             std::string::npos);
    CHECK_NE(suggestion_json.find(R"("target_style_id":"DuplicateBodyA")"),
             std::string::npos);
    CHECK_NE(suggestion_json.find(
                 R"("reason_code":"matching_style_signature_and_xml")"),
             std::string::npos);
    CHECK_NE(suggestion_json.find(R"("confidence":95)"), std::string::npos);
    CHECK_NE(suggestion_json.find(R"("target_has_more_references")"),
             std::string::npos);
    CHECK_NE(suggestion_json.find(R"("style_definition_xml_matches")"),
             std::string::npos);
    CHECK_NE(suggestion_json.find(R"("differences":[])"), std::string::npos);
    CHECK_NE(suggestion_json.find(
                 R"("suggestion_confidence_summary":{"suggestion_count":1)"),
             std::string::npos);
    CHECK_NE(suggestion_json.find(R"("min_confidence":95)"),
             std::string::npos);
    CHECK_NE(suggestion_json.find(R"("max_confidence":95)"),
             std::string::npos);
    CHECK_NE(suggestion_json.find(R"("exact_xml_match_count":1)"),
             std::string::npos);
    CHECK_NE(suggestion_json.find(R"("xml_difference_count":0)"),
             std::string::npos);
    CHECK_NE(suggestion_json.find(R"("recommended_min_confidence":95)"),
             std::string::npos);
    CHECK_NE(suggestion_json.find("automation gates"), std::string::npos);

    CHECK_EQ(run_cli({"suggest-style-merges",
                      source.string(),
                      "--fail-on-suggestion",
                      "--json"},
                     gate_output),
             1);
    const auto gate_json = read_text_file(gate_output);
    CHECK_NE(gate_json.find(R"("command":"suggest-style-merges")"),
             std::string::npos);
    CHECK_NE(gate_json.find(R"("fail_on_suggestion":true)"),
             std::string::npos);
    CHECK_NE(gate_json.find(R"("suggestion_gate_failed":true)"),
             std::string::npos);
    CHECK_NE(gate_json.find(R"("clean":true)"), std::string::npos);
    CHECK_NE(gate_json.find(R"("operation_count":1)"), std::string::npos);
    CHECK_NE(gate_json.find(R"("source_style_id":"DuplicateBodyB")"),
             std::string::npos);

    const auto plan_json = read_text_file(plan_file);
    CHECK_NE(plan_json.find(R"("command":"suggest-style-merges")"),
             std::string::npos);
    CHECK_NE(plan_json.find(R"("operations":[)"), std::string::npos);
    CHECK_NE(plan_json.find(R"("source_style_id":"DuplicateBodyB")"),
             std::string::npos);
    CHECK_NE(plan_json.find(R"("target_style_id":"DuplicateBodyA")"),
             std::string::npos);
    CHECK_NE(plan_json.find(R"("confidence":95)"), std::string::npos);
    CHECK_NE(plan_json.find(R"("recommended_min_confidence":95)"),
             std::string::npos);
    CHECK_EQ(plan_json.find(R"("fail_on_suggestion")"), std::string::npos);
    CHECK_EQ(plan_json.find(R"("suggestion_gate_failed")"), std::string::npos);

    CHECK_EQ(run_cli({"suggest-style-merges",
                      source.string(),
                      "--min-confidence",
                      "96",
                      "--output-plan",
                      filtered_plan_file.string(),
                      "--json"},
                     filtered_output),
             0);
    const auto filtered_json = read_text_file(filtered_output);
    CHECK_NE(filtered_json.find(R"("command":"suggest-style-merges")"),
             std::string::npos);
    CHECK_NE(filtered_json.find(R"("operation_count":0)"), std::string::npos);
    CHECK_NE(filtered_json.find(R"("applyable_count":0)"), std::string::npos);
    CHECK_NE(filtered_json.find(R"("suggestion_confidence_summary":{"suggestion_count":0)"),
             std::string::npos);
    CHECK_NE(filtered_json.find(R"("min_confidence":null)"),
             std::string::npos);
    CHECK_NE(filtered_json.find(R"("max_confidence":null)"),
             std::string::npos);
    CHECK_NE(filtered_json.find(R"("recommended_min_confidence":null)"),
             std::string::npos);
    CHECK_NE(filtered_json.find(R"("operations":[])"), std::string::npos);
    CHECK_EQ(filtered_json.find(R"("source_style_id":"DuplicateBodyB")"),
             std::string::npos);

    CHECK_EQ(run_cli({"suggest-style-merges",
                      source.string(),
                      "--min-confidence",
                      "96",
                      "--fail-on-suggestion",
                      "--json"},
                     filtered_gate_output),
             0);
    const auto filtered_gate_json = read_text_file(filtered_gate_output);
    CHECK_NE(filtered_gate_json.find(R"("fail_on_suggestion":true)"),
             std::string::npos);
    CHECK_NE(filtered_gate_json.find(R"("suggestion_gate_failed":false)"),
             std::string::npos);
    CHECK_NE(filtered_gate_json.find(R"("operation_count":0)"),
             std::string::npos);
    CHECK_NE(filtered_gate_json.find(R"("operations":[])"), std::string::npos);

    const auto filtered_plan_json = read_text_file(filtered_plan_file);
    CHECK_NE(filtered_plan_json.find(R"("operation_count":0)"),
             std::string::npos);
    CHECK_NE(filtered_plan_json.find(R"("recommended_min_confidence":null)"),
             std::string::npos);
    CHECK_NE(filtered_plan_json.find(R"("operations":[])"), std::string::npos);

    CHECK_EQ(run_cli({"apply-style-refactor",
                      source.string(),
                      "--plan-file",
                      plan_file.string(),
                      "--rollback-plan",
                      rollback_file.string(),
                      "--output",
                      applied.string(),
                      "--json"},
                     apply_output),
             0);
    CHECK_NE(read_text_file(apply_output).find(R"("command":"apply-style-refactor")"),
             std::string::npos);
    const auto suggestion_rollback_json = read_text_file(rollback_file);
    CHECK_NE(suggestion_rollback_json.find(R"("rollback_count":1)"),
             std::string::npos);
    CHECK_NE(suggestion_rollback_json.find(R"("restorable":true)"),
             std::string::npos);
    CHECK_NE(suggestion_rollback_json.find(R"("source_reference_count":1)"),
             std::string::npos);
    CHECK_NE(suggestion_rollback_json.find(R"("source_style_xml":")"),
             std::string::npos);
    CHECK_NE(suggestion_rollback_json.find(R"("node_ordinal":3)"),
             std::string::npos);
    CHECK_NE(suggestion_rollback_json.find(R"(w:styleId=\"DuplicateBodyB\")"),
             std::string::npos);
    {
        featherdoc::Document document(applied);
        REQUIRE_FALSE(document.open());
        CHECK(document.find_style("DuplicateBodyA").has_value());
        CHECK_FALSE(document.find_style("DuplicateBodyB").has_value());
    }

    CHECK_EQ(run_cli({"restore-style-merge",
                      applied.string(),
                      "--rollback-plan",
                      rollback_file.string(),
                      "--entry",
                      "0",
                      "--dry-run",
                      "--json"},
                     dry_run_output),
             0);
    const auto dry_run_json = read_text_file(dry_run_output);
    CHECK_NE(dry_run_json.find(R"("command":"restore-style-merge")"),
             std::string::npos);
    CHECK_NE(dry_run_json.find(R"("dry_run":true)"), std::string::npos);
    CHECK_NE(dry_run_json.find(R"("changed":false)"), std::string::npos);
    CHECK_NE(dry_run_json.find(R"("issue_count":0)"), std::string::npos);
    CHECK_NE(dry_run_json.find(R"("issue_summary":[])"), std::string::npos);
    CHECK_NE(dry_run_json.find(R"("restored_count":1)"), std::string::npos);
    CHECK_NE(dry_run_json.find(R"("restored_style_count":1)"),
             std::string::npos);
    CHECK_NE(dry_run_json.find(R"("restored_reference_count":1)"),
             std::string::npos);
    CHECK_NE(dry_run_json.find(R"("entry_index":0)"), std::string::npos);
    {
        featherdoc::Document document(applied);
        REQUIRE_FALSE(document.open());
        CHECK(document.find_style("DuplicateBodyA").has_value());
        CHECK_FALSE(document.find_style("DuplicateBodyB").has_value());
        const auto target_usage = document.find_style_usage("DuplicateBodyA");
        REQUIRE(target_usage.has_value());
        CHECK_EQ(target_usage->paragraph_count, 3U);
    }

    CHECK_EQ(run_cli({"restore-style-merge",
                      applied.string(),
                      "--rollback-plan",
                      rollback_file.string(),
                      "--entry",
                      "0",
                      "--output",
                      restored.string(),
                      "--json"},
                     restore_output),
             0);
    const auto restore_json = read_text_file(restore_output);
    CHECK_NE(restore_json.find(R"("command":"restore-style-merge")"),
             std::string::npos);
    CHECK_NE(restore_json.find(R"("ok":true)"), std::string::npos);
    CHECK_NE(restore_json.find(R"("restored_count":1)"), std::string::npos);
    CHECK_NE(restore_json.find(R"("restored_style_count":1)"),
             std::string::npos);
    CHECK_NE(restore_json.find(R"("restored_reference_count":1)"),
             std::string::npos);
    {
        featherdoc::Document document(restored);
        REQUIRE_FALSE(document.open());
        CHECK(document.find_style("DuplicateBodyA").has_value());
        CHECK(document.find_style("DuplicateBodyB").has_value());
        const auto target_usage = document.find_style_usage("DuplicateBodyA");
        REQUIRE(target_usage.has_value());
        CHECK_EQ(target_usage->paragraph_count, 2U);
        const auto source_usage = document.find_style_usage("DuplicateBodyB");
        REQUIRE(source_usage.has_value());
        CHECK_EQ(source_usage->paragraph_count, 1U);
    }

    CHECK_EQ(run_cli({"restore-style-merge",
                      restored.string(),
                      "--rollback-plan",
                      rollback_file.string(),
                      "--entry",
                      "0",
                      "--dry-run",
                      "--json"},
                     parse_output),
             1);
    const auto restore_conflict_json = read_text_file(parse_output);
    CHECK_NE(restore_conflict_json.find(R"("ok":false)"), std::string::npos);
    CHECK_NE(restore_conflict_json.find(R"("issue_count":1)"),
             std::string::npos);
    CHECK_NE(restore_conflict_json.find(
                 R"("issue_summary":[{"code":"source_style_exists","count":1)"),
             std::string::npos);
    CHECK_NE(restore_conflict_json.find(R"("code":"source_style_exists")"),
             std::string::npos);
    CHECK_NE(restore_conflict_json.find(R"("suggestion":"The source style already exists;)"),
             std::string::npos);
    CHECK_NE(restore_conflict_json.find("skip this rollback entry"),
             std::string::npos);

    CHECK_EQ(run_cli({"restore-style-merge",
                      applied.string(),
                      "--rollback-plan",
                      rollback_file.string(),
                      "--dry-run",
                      "--output",
                      restored.string(),
                      "--json"},
                     parse_output),
             2);
    CHECK_NE(read_text_file(parse_output)
                 .find("restore-style-merge --dry-run cannot be combined with --output"),
             std::string::npos);

    CHECK_EQ(run_cli({"suggest-style-merges",
                      source.string(),
                      "--min-confidence",
                      "101",
                      "--json"},
                     parse_output),
             2);
    CHECK_NE(read_text_file(parse_output)
                 .find("--min-confidence expects an integer from 0 to 100"),
             std::string::npos);

    CHECK_EQ(run_cli({"suggest-style-merges", source.string(), "--bad", "--json"},
                     parse_output),
             2);
    CHECK_NE(read_text_file(parse_output).find(R"("stage":"parse")"),
             std::string::npos);

    remove_if_exists(source);
    remove_if_exists(output);
    remove_if_exists(filtered_output);
    remove_if_exists(gate_output);
    remove_if_exists(filtered_gate_output);
    remove_if_exists(plan_file);
    remove_if_exists(filtered_plan_file);
    remove_if_exists(applied);
    remove_if_exists(apply_output);
    remove_if_exists(rollback_file);
    remove_if_exists(restored);
    remove_if_exists(restore_output);
    remove_if_exists(dry_run_output);
    remove_if_exists(parse_output);
}

TEST_CASE("cli suggest-style-merges applies confidence profiles") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_suggest_style_merges_profiles_source.docx";
    const fs::path recommended_output =
        working_directory / "cli_suggest_style_merges_profiles_recommended.json";
    const fs::path strict_output =
        working_directory / "cli_suggest_style_merges_profiles_strict.json";
    const fs::path review_output =
        working_directory / "cli_suggest_style_merges_profiles_review.json";
    const fs::path parse_output =
        working_directory / "cli_suggest_style_merges_profiles_parse.json";

    remove_if_exists(source);
    remove_if_exists(recommended_output);
    remove_if_exists(strict_output);
    remove_if_exists(review_output);
    remove_if_exists(parse_output);

    create_cli_duplicate_style_profile_fixture(source);

    CHECK_EQ(run_cli({"suggest-style-merges",
                      source.string(),
                      "--confidence-profile",
                      "recommended",
                      "--json"},
                     recommended_output),
             0);
    const auto recommended_json = read_text_file(recommended_output);
    CHECK_NE(recommended_json.find(R"("operation_count":1)"), std::string::npos);
    CHECK_NE(recommended_json.find(R"("source_style_id":"DuplicateBodyB")"),
             std::string::npos);
    CHECK_EQ(recommended_json.find(R"("source_style_id":"DuplicateBodyC")"),
             std::string::npos);
    CHECK_NE(recommended_json.find(R"("min_confidence":95)"),
             std::string::npos);
    CHECK_NE(recommended_json.find(R"("recommended_min_confidence":95)"),
             std::string::npos);
    CHECK_NE(recommended_json.find("automation gates"), std::string::npos);

    CHECK_EQ(run_cli({"suggest-style-merges",
                      source.string(),
                      "--confidence-profile",
                      "strict",
                      "--json"},
                     strict_output),
             0);
    const auto strict_json = read_text_file(strict_output);
    CHECK_NE(strict_json.find(R"("operation_count":1)"), std::string::npos);
    CHECK_NE(strict_json.find(R"("source_style_id":"DuplicateBodyB")"),
             std::string::npos);
    CHECK_EQ(strict_json.find(R"("source_style_id":"DuplicateBodyC")"),
             std::string::npos);
    CHECK_NE(strict_json.find(R"("min_confidence":95)"), std::string::npos);
    CHECK_NE(strict_json.find(R"("recommended_min_confidence":95)"),
             std::string::npos);
    CHECK_NE(strict_json.find(R"("exact_xml_match_count":1)"),
             std::string::npos);
    CHECK_NE(strict_json.find(R"("xml_difference_count":0)"),
             std::string::npos);

    CHECK_EQ(run_cli({"suggest-style-merges",
                      source.string(),
                      "--confidence-profile",
                      "review",
                      "--json"},
                     review_output),
             0);
    const auto review_json = read_text_file(review_output);
    CHECK_NE(review_json.find(R"("operation_count":2)"), std::string::npos);
    CHECK_NE(review_json.find(R"("source_style_id":"DuplicateBodyB")"),
             std::string::npos);
    CHECK_NE(review_json.find(R"("source_style_id":"DuplicateBodyC")"),
             std::string::npos);
    CHECK_NE(review_json.find(R"("confidence":80)"), std::string::npos);
    CHECK_NE(review_json.find(R"("min_confidence":80)"), std::string::npos);
    CHECK_NE(review_json.find(R"("exact_xml_match_count":1)"),
             std::string::npos);
    CHECK_NE(review_json.find(R"("xml_difference_count":1)"),
             std::string::npos);
    CHECK_NE(review_json.find("review lower-confidence XML differences manually"),
             std::string::npos);

    CHECK_EQ(run_cli({"suggest-style-merges",
                      source.string(),
                      "--confidence-profile",
                      "strict",
                      "--min-confidence",
                      "80",
                      "--json"},
                     parse_output),
             2);
    CHECK_NE(read_text_file(parse_output)
                 .find("--confidence-profile cannot be combined with --min-confidence"),
             std::string::npos);

    CHECK_EQ(run_cli({"suggest-style-merges",
                      source.string(),
                      "--confidence-profile",
                      "unsafe",
                      "--json"},
                     parse_output),
             2);
    CHECK_NE(read_text_file(parse_output)
                 .find("--confidence-profile expects recommended, strict, review, or exploratory"),
             std::string::npos);

    remove_if_exists(source);
    remove_if_exists(recommended_output);
    remove_if_exists(strict_output);
    remove_if_exists(review_output);
    remove_if_exists(parse_output);
}


TEST_CASE("cli suggest-style-merges filters by style ids") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_suggest_style_merges_filters_source.docx";
    const fs::path source_filter_output =
        working_directory / "cli_suggest_style_merges_filters_source.json";
    const fs::path target_filter_output =
        working_directory / "cli_suggest_style_merges_filters_target.json";
    const fs::path missing_filter_output =
        working_directory / "cli_suggest_style_merges_filters_missing.json";
    const fs::path parse_output =
        working_directory / "cli_suggest_style_merges_filters_parse.json";

    remove_if_exists(source);
    remove_if_exists(source_filter_output);
    remove_if_exists(target_filter_output);
    remove_if_exists(missing_filter_output);
    remove_if_exists(parse_output);

    create_cli_duplicate_style_profile_fixture(source);

    CHECK_EQ(run_cli({"suggest-style-merges",
                      source.string(),
                      "--source-style",
                      "DuplicateBodyC",
                      "--json"},
                     source_filter_output),
             0);
    const auto source_filter_json = read_text_file(source_filter_output);
    CHECK_NE(source_filter_json.find(R"("operation_count":1)"),
             std::string::npos);
    CHECK_NE(source_filter_json.find(R"("source_style_id":"DuplicateBodyC")"),
             std::string::npos);
    CHECK_EQ(source_filter_json.find(R"("source_style_id":"DuplicateBodyB")"),
             std::string::npos);
    CHECK_NE(source_filter_json.find(R"("target_style_id":"DuplicateBodyA")"),
             std::string::npos);
    CHECK_NE(source_filter_json.find(R"("confidence":80)"), std::string::npos);
    CHECK_NE(source_filter_json.find(R"("exact_xml_match_count":0)"),
             std::string::npos);
    CHECK_NE(source_filter_json.find(R"("xml_difference_count":1)"),
             std::string::npos);

    CHECK_EQ(run_cli({"suggest-style-merges",
                      source.string(),
                      "--target-style",
                      "DuplicateBodyA",
                      "--json"},
                     target_filter_output),
             0);
    const auto target_filter_json = read_text_file(target_filter_output);
    CHECK_NE(target_filter_json.find(R"("operation_count":2)"),
             std::string::npos);
    CHECK_NE(target_filter_json.find(R"("source_style_id":"DuplicateBodyB")"),
             std::string::npos);
    CHECK_NE(target_filter_json.find(R"("source_style_id":"DuplicateBodyC")"),
             std::string::npos);
    CHECK_NE(target_filter_json.find(R"("target_style_id":"DuplicateBodyA")"),
             std::string::npos);

    CHECK_EQ(run_cli({"suggest-style-merges",
                      source.string(),
                      "--source-style",
                      "MissingStyle",
                      "--json"},
                     missing_filter_output),
             0);
    const auto missing_filter_json = read_text_file(missing_filter_output);
    CHECK_NE(missing_filter_json.find(R"("operation_count":0)"),
             std::string::npos);
    CHECK_EQ(missing_filter_json.find(R"("source_style_id":"DuplicateBodyB")"),
             std::string::npos);
    CHECK_EQ(missing_filter_json.find(R"("source_style_id":"DuplicateBodyC")"),
             std::string::npos);

    CHECK_EQ(run_cli({"suggest-style-merges",
                      source.string(),
                      "--source-style",
                      "DuplicateBodyB",
                      "--source-style",
                      "DuplicateBodyB",
                      "--json"},
                     parse_output),
             2);
    CHECK_NE(read_text_file(parse_output).find("duplicate --source-style id"),
             std::string::npos);

    CHECK_EQ(run_cli({"suggest-style-merges",
                      source.string(),
                      "--target-style",
                      "DuplicateBodyA",
                      "--target-style",
                      "DuplicateBodyA",
                      "--json"},
                     parse_output),
             2);
    CHECK_NE(read_text_file(parse_output).find("duplicate --target-style id"),
             std::string::npos);

    remove_if_exists(source);
    remove_if_exists(source_filter_output);
    remove_if_exists(target_filter_output);
    remove_if_exists(missing_filter_output);
    remove_if_exists(parse_output);
}

TEST_CASE("cli apply-style-refactor applies clean batch and blocks conflicts") {
    const fs::path working_directory = fs::current_path();
    const fs::path source = working_directory / "cli_style_refactor_apply_source.docx";
    const fs::path applied = working_directory / "cli_style_refactor_apply_output.docx";
    const fs::path dirty_output =
        working_directory / "cli_style_refactor_apply_dirty.docx";
    const fs::path output = working_directory / "cli_style_refactor_apply.json";
    const fs::path plan_file =
        working_directory / "cli_style_refactor_apply.plan.json";
    const fs::path plan_output =
        working_directory / "cli_style_refactor_apply_plan_output.json";
    const fs::path rollback_file =
        working_directory / "cli_style_refactor_apply.rollback.json";
    const fs::path restore_dry_run_output =
        working_directory / "cli_style_refactor_apply_restore_dry_run.json";
    const fs::path dirty_json =
        working_directory / "cli_style_refactor_apply_dirty.json";
    const fs::path parse_output =
        working_directory / "cli_style_refactor_apply_parse.json";

    remove_if_exists(source);
    remove_if_exists(applied);
    remove_if_exists(dirty_output);
    remove_if_exists(output);
    remove_if_exists(plan_file);
    remove_if_exists(plan_output);
    remove_if_exists(rollback_file);
    remove_if_exists(restore_dry_run_output);
    remove_if_exists(dirty_json);
    remove_if_exists(parse_output);

    create_cli_style_usage_fixture(source);
    {
        featherdoc::Document document(source);
        REQUIRE_FALSE(document.open());
        featherdoc::paragraph_style_definition archive_body;
        archive_body.name = "Archive Body";
        archive_body.based_on = "Normal";
        REQUIRE(document.ensure_paragraph_style("ArchiveBody", archive_body));
        featherdoc::paragraph_style_definition archive_note;
        archive_note.name = "Archive Note";
        archive_note.based_on = "Normal";
        REQUIRE(document.ensure_paragraph_style("ArchiveNote", archive_note));
        REQUIRE_FALSE(document.save());
    }

    CHECK_EQ(run_cli({"plan-style-refactor",
                      source.string(),
                      "--rename",
                      "CustomBody:ReviewBody",
                      "--merge",
                      "ArchiveBody:Normal",
                      "--merge",
                      "ArchiveNote:Normal",
                      "--output-plan",
                      plan_file.string(),
                      "--json"},
                     plan_output),
             0);
    const auto plan_json = read_text_file(plan_file);
    CHECK_NE(plan_json.find(R"("source_style_id":"ArchiveBody")"),
             std::string::npos);
    CHECK_NE(plan_json.find(R"("source_style_id":"ArchiveNote")"),
             std::string::npos);

    CHECK_EQ(run_cli({"apply-style-refactor",
                      source.string(),
                      "--plan-file",
                      plan_file.string(),
                      "--rollback-plan",
                      rollback_file.string(),
                      "--output",
                      applied.string(),
                      "--json"},
                     output),
             0);
    const auto apply_json = read_text_file(output);
    CHECK_NE(apply_json.find(R"("command":"apply-style-refactor")"),
             std::string::npos);
    CHECK_NE(apply_json.find(R"("ok":true)"), std::string::npos);
    CHECK_NE(apply_json.find(R"("changed":true)"), std::string::npos);
    CHECK_NE(apply_json.find(R"("requested_count":3)"), std::string::npos);
    CHECK_NE(apply_json.find(R"("applied_count":3)"), std::string::npos);
    CHECK_NE(apply_json.find(R"("skipped_count":0)"), std::string::npos);
    CHECK_NE(apply_json.find(R"("rollback_count":3)"), std::string::npos);
    CHECK_NE(apply_json.find(R"("plan_file":)"), std::string::npos);
    CHECK_NE(apply_json.find(R"("rollback_plan_file":)"), std::string::npos);
    CHECK_NE(apply_json.find(R"("clean":true)"), std::string::npos);
    const auto rollback_json = read_text_file(rollback_file);
    CHECK_NE(rollback_json.find(R"("rollback_count":3)"), std::string::npos);
    CHECK_NE(rollback_json.find(
                 R"("command_template":"featherdoc_cli rename-style <input.docx> ReviewBody CustomBody --output <output.docx> --json")"),
             std::string::npos);
    CHECK_NE(rollback_json.find(R"("automatic":false)"), std::string::npos);
    CHECK_NE(rollback_json.find(R"("restorable":true)"), std::string::npos);
    CHECK_NE(rollback_json.find(R"("source_style_xml":")"), std::string::npos);
    CHECK_NE(rollback_json.find(R"(w:styleId=\"ArchiveBody\")"),
             std::string::npos);
    CHECK_NE(rollback_json.find(R"(w:styleId=\"ArchiveNote\")"),
             std::string::npos);

    const auto styles_xml = read_docx_entry(applied, "word/styles.xml");
    CHECK_EQ(styles_xml.find(R"(w:styleId="CustomBody")"), std::string::npos);
    CHECK_NE(styles_xml.find(R"(w:styleId="ReviewBody")"),
             std::string::npos);
    CHECK_EQ(styles_xml.find(R"(w:styleId="ArchiveBody")"),
             std::string::npos);
    CHECK_EQ(styles_xml.find(R"(w:styleId="ArchiveNote")"),
             std::string::npos);
    const auto document_xml = read_docx_entry(applied, "word/document.xml");
    CHECK_NE(document_xml.find(R"(w:pStyle w:val="ReviewBody")"),
             std::string::npos);
    CHECK_EQ(document_xml.find(R"(w:pStyle w:val="CustomBody")"),
             std::string::npos);

    CHECK_EQ(run_cli({"restore-style-merge",
                      applied.string(),
                      "--rollback-plan",
                      rollback_file.string(),
                      "--entry",
                      "1",
                      "--entry",
                      "2",
                      "--dry-run",
                      "--json"},
                     restore_dry_run_output),
             0);
    const auto restore_dry_run_json = read_text_file(restore_dry_run_output);
    CHECK_NE(restore_dry_run_json.find(R"("command":"restore-style-merge")"),
             std::string::npos);
    CHECK_NE(restore_dry_run_json.find(R"("dry_run":true)"), std::string::npos);
    CHECK_NE(restore_dry_run_json.find(R"("requested_count":2)"),
             std::string::npos);
    CHECK_NE(restore_dry_run_json.find(R"("restored_count":2)"),
             std::string::npos);
    CHECK_NE(restore_dry_run_json.find(R"("entry_indexes":[1,2])"),
             std::string::npos);
    {
        featherdoc::Document document(applied);
        REQUIRE_FALSE(document.open());
        CHECK_FALSE(document.find_style("ArchiveBody").has_value());
        CHECK_FALSE(document.find_style("ArchiveNote").has_value());
    }

    CHECK_EQ(run_cli({"restore-style-merge",
                      applied.string(),
                      "--rollback-plan",
                      rollback_file.string(),
                      "--source-style",
                      "ArchiveNote",
                      "--target-style",
                      "Normal",
                      "--plan-only",
                      "--json"},
                     restore_dry_run_output),
             0);
    const auto restore_filter_json = read_text_file(restore_dry_run_output);
    CHECK_NE(restore_filter_json.find(R"("command":"restore-style-merge")"),
             std::string::npos);
    CHECK_NE(restore_filter_json.find(R"("dry_run":true)"), std::string::npos);
    CHECK_NE(restore_filter_json.find(R"("requested_count":1)"),
             std::string::npos);
    CHECK_NE(restore_filter_json.find(R"("restored_count":1)"),
             std::string::npos);
    CHECK_NE(restore_filter_json.find(R"("source_style_ids":["ArchiveNote"])"),
             std::string::npos);
    CHECK_NE(restore_filter_json.find(R"("target_style_ids":["Normal"])"),
             std::string::npos);
    CHECK_NE(restore_filter_json.find(R"("source_style_id":"ArchiveNote")"),
             std::string::npos);

    CHECK_EQ(run_cli({"apply-style-refactor",
                      source.string(),
                      "--rename",
                      "CustomBody:ReviewBody",
                      "--merge",
                      "CustomBody:Normal",
                      "--output",
                      dirty_output.string(),
                      "--json"},
                     dirty_json),
             1);
    const auto dirty_text = read_text_file(dirty_json);
    CHECK_NE(dirty_text.find(R"("ok":false)"), std::string::npos);
    CHECK_NE(dirty_text.find(R"("changed":false)"), std::string::npos);
    CHECK_NE(dirty_text.find(R"("applied_count":0)"), std::string::npos);
    CHECK_NE(dirty_text.find(R"("skipped_count":2)"), std::string::npos);
    CHECK_NE(dirty_text.find(R"("code":"duplicate_source_operation")"),
             std::string::npos);
    CHECK_FALSE(fs::exists(dirty_output));

    CHECK_EQ(run_cli({"restore-style-merge",
                      applied.string(),
                      "--rollback-plan",
                      rollback_file.string(),
                      "--entry",
                      "1",
                      "--source-style",
                      "ArchiveBody",
                      "--dry-run",
                      "--json"},
                     parse_output),
             2);
    CHECK_NE(read_text_file(parse_output)
                 .find("restore-style-merge --entry cannot be combined with --source-style or --target-style"),
             std::string::npos);

    CHECK_EQ(run_cli({"apply-style-refactor", source.string(), "--json"},
                     parse_output),
             2);
    CHECK_NE(read_text_file(parse_output)
                 .find("apply-style-refactor expects --plan-file or at least one --rename or --merge option"),
             std::string::npos);

    remove_if_exists(source);
    remove_if_exists(applied);
    remove_if_exists(dirty_output);
    remove_if_exists(output);
    remove_if_exists(plan_file);
    remove_if_exists(plan_output);
    remove_if_exists(rollback_file);
    remove_if_exists(restore_dry_run_output);
    remove_if_exists(dirty_json);
    remove_if_exists(parse_output);
}
