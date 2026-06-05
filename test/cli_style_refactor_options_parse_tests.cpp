#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "featherdoc_cli_style_refactor_options_parse.hpp"

#include <string>
#include <string_view>

TEST_CASE("cli style refactor options parse accepts plan and suggestion commands") {
    featherdoc_cli::style_refactor_plan_options plan;
    std::string error;
    CHECK(featherdoc_cli::parse_style_refactor_plan_options(
        {"plan-style-refactor", "input.docx", "--rename", "OldStyle:NewStyle",
         "--merge", "BodyA:BodyB", "--output-plan", "plan.json", "--json"},
        2U, plan, error));
    REQUIRE_EQ(plan.requests.size(), 2U);
    CHECK(plan.requests[0].action == featherdoc::style_refactor_action::rename);
    CHECK_EQ(plan.requests[0].source_style_id, "OldStyle");
    CHECK_EQ(plan.requests[0].target_style_id, "NewStyle");
    CHECK(plan.requests[1].action == featherdoc::style_refactor_action::merge);
    CHECK_EQ(plan.requests[1].source_style_id, "BodyA");
    CHECK_EQ(plan.requests[1].target_style_id, "BodyB");
    REQUIRE(plan.output_plan_path.has_value());
    CHECK(plan.output_plan_path->filename().string() == "plan.json");
    CHECK(plan.json_output);

    featherdoc_cli::style_merge_suggestion_options suggestion;
    error.clear();
    CHECK(featherdoc_cli::parse_style_merge_suggestion_options(
        {"style-merge-suggestion", "input.docx", "--output-plan", "suggest.json",
         "--confidence-profile", "review", "--source-style", "BodyA",
         "--target-style", "BodyB", "--fail-on-suggestion", "--json"},
        2U, suggestion, error));
    REQUIRE(suggestion.output_plan_path.has_value());
    CHECK(suggestion.output_plan_path->filename().string() == "suggest.json");
    REQUIRE(suggestion.confidence_profile.has_value());
    CHECK(*suggestion.confidence_profile == "review");
    REQUIRE_EQ(suggestion.source_style_ids.size(), 1U);
    REQUIRE_EQ(suggestion.target_style_ids.size(), 1U);
    CHECK_EQ(suggestion.source_style_ids[0], "BodyA");
    CHECK_EQ(suggestion.target_style_ids[0], "BodyB");
    CHECK(suggestion.fail_on_suggestion);
    CHECK(suggestion.json_output);
}

TEST_CASE("cli style refactor options parse accepts apply and restore commands") {
    featherdoc_cli::style_refactor_apply_options apply_requests;
    std::string error;
    CHECK(featherdoc_cli::parse_style_refactor_apply_options(
        {"apply-style-refactor", "input.docx", "--rename", "OldStyle:NewStyle",
         "--merge", "BodyA:BodyB", "--output", "out.docx", "--json"},
        2U, apply_requests, error));
    REQUIRE_EQ(apply_requests.requests.size(), 2U);
    REQUIRE(apply_requests.output_path.has_value());
    CHECK(apply_requests.output_path->filename().string() == "out.docx");
    CHECK(apply_requests.json_output);

    featherdoc_cli::style_refactor_apply_options apply_plan;
    error.clear();
    CHECK(featherdoc_cli::parse_style_refactor_apply_options(
        {"apply-style-refactor", "input.docx", "--plan-file", "plan.json",
         "--output", "out.docx", "--json"},
        2U, apply_plan, error));
    REQUIRE(apply_plan.plan_file_path.has_value());
    CHECK(apply_plan.plan_file_path->filename().string() == "plan.json");
    CHECK(apply_plan.requests.empty());
    CHECK(apply_plan.output_path->filename().string() == "out.docx");
    CHECK(apply_plan.json_output);

    featherdoc_cli::style_merge_restore_options restore_ids;
    error.clear();
    CHECK(featherdoc_cli::parse_style_merge_restore_options(
        {"restore-style-merge", "input.docx", "--rollback-plan", "rollback.json",
         "--source-style", "BodyA", "--target-style", "BodyB", "--output",
         "restored.docx", "--json"},
        2U, restore_ids, error));
    REQUIRE(restore_ids.rollback_plan_path.has_value());
    CHECK(restore_ids.rollback_plan_path->filename().string() == "rollback.json");
    REQUIRE_EQ(restore_ids.source_style_ids.size(), 1U);
    REQUIRE_EQ(restore_ids.target_style_ids.size(), 1U);
    CHECK_EQ(restore_ids.source_style_ids[0], "BodyA");
    CHECK_EQ(restore_ids.target_style_ids[0], "BodyB");
    REQUIRE(restore_ids.output_path.has_value());
    CHECK(restore_ids.output_path->filename().string() == "restored.docx");
    CHECK(restore_ids.json_output);
}

TEST_CASE("cli style refactor options parse validates conflicts and required flags") {
    featherdoc_cli::style_refactor_plan_options plan;
    std::string error;
    CHECK_FALSE(featherdoc_cli::parse_style_refactor_plan_options(
        {"plan-style-refactor", "input.docx", "--json"}, 2U, plan, error));
    CHECK(error ==
          "plan-style-refactor expects at least one --rename or --merge option");

    featherdoc_cli::style_merge_suggestion_options suggestion;
    error.clear();
    CHECK_FALSE(featherdoc_cli::parse_style_merge_suggestion_options(
        {"style-merge-suggestion", "input.docx", "--min-confidence", "80",
         "--confidence-profile", "review"},
        2U, suggestion, error));
    CHECK(error ==
          "--confidence-profile cannot be combined with --min-confidence");

    featherdoc_cli::style_refactor_apply_options apply;
    error.clear();
    CHECK_FALSE(featherdoc_cli::parse_style_refactor_apply_options(
        {"apply-style-refactor", "input.docx", "--plan-file", "plan.json",
         "--rename", "OldStyle:NewStyle"},
        2U, apply, error));
    CHECK(error ==
          "apply-style-refactor cannot combine --plan-file with --rename or --merge");

    featherdoc_cli::style_merge_restore_options restore;
    error.clear();
    CHECK_FALSE(featherdoc_cli::parse_style_merge_restore_options(
        {"restore-style-merge", "input.docx", "--output", "restored.docx"},
        2U, restore, error));
    CHECK(error == "restore-style-merge expects --rollback-plan");
}
