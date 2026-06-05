#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "featherdoc_cli_style_refactor_plan.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace {

auto make_operation(featherdoc::style_refactor_action action,
                    std::string source_style_id, std::string target_style_id,
                    std::optional<std::uint32_t> confidence, bool applyable,
                    std::size_t issue_count)
    -> featherdoc::style_refactor_operation_plan {
    featherdoc::style_refactor_operation_plan operation;
    operation.action = action;
    operation.source_style_id = std::move(source_style_id);
    operation.target_style_id = std::move(target_style_id);
    operation.applyable = applyable;
    if (confidence.has_value()) {
        featherdoc::style_refactor_suggestion suggestion;
        suggestion.reason_code = "matching_resolved_style_signature";
        suggestion.reason = "Similar resolved styles";
        suggestion.confidence = *confidence;
        operation.suggestion = std::move(suggestion);
    }
    for (std::size_t index = 0U; index < issue_count; ++index) {
        featherdoc::style_refactor_issue issue;
        issue.code = "issue";
        issue.message = "Issue " + std::to_string(index);
        operation.issues.push_back(std::move(issue));
    }
    return operation;
}

auto make_plan(std::vector<featherdoc::style_refactor_operation_plan> operations)
    -> featherdoc::style_refactor_plan {
    featherdoc::style_refactor_plan plan;
    plan.operations = std::move(operations);
    plan.operation_count = plan.operations.size();
    for (const auto &operation : plan.operations) {
        if (operation.applyable) {
            ++plan.applyable_count;
        }
        plan.issue_count += operation.issues.size();
    }
    return plan;
}

} // namespace

TEST_CASE("cli style refactor plan command templates match CLI commands") {
    const auto rename = make_operation(featherdoc::style_refactor_action::rename,
                                       "OldStyle", "NewStyle", 95U, true, 0U);
    CHECK(featherdoc_cli::style_refactor_command_template(rename) ==
          "featherdoc_cli rename-style <input.docx> OldStyle NewStyle "
          "--output <output.docx> --json");

    const auto merge = make_operation(featherdoc::style_refactor_action::merge,
                                      "SourceStyle", "TargetStyle", 90U, true,
                                      0U);
    CHECK(featherdoc_cli::style_refactor_action_name(merge.action) ==
          std::string{"merge"});
    CHECK(featherdoc_cli::style_refactor_command_template(merge) ==
          "featherdoc_cli merge-style <input.docx> SourceStyle TargetStyle "
          "--output <output.docx> --json");

    featherdoc::style_refactor_rollback_entry rollback;
    rollback.action = featherdoc::style_refactor_action::merge;
    rollback.source_style_id = "SourceStyle";
    rollback.target_style_id = "TargetStyle";
    rollback.automatic = true;
    CHECK(featherdoc_cli::style_refactor_rollback_command_template(rollback) ==
          "featherdoc_cli merge-style <input.docx> SourceStyle TargetStyle "
          "--output <output.docx> --json");

    rollback.automatic = false;
    CHECK(featherdoc_cli::style_refactor_rollback_command_template(rollback)
              .empty());
}

TEST_CASE("cli style refactor plan confidence profiles map documented thresholds") {
    const auto strict =
        featherdoc_cli::style_merge_suggestion_confidence_profile_min_confidence(
            "strict");
    REQUIRE(strict.has_value());
    CHECK(*strict == 90U);

    const auto review =
        featherdoc_cli::style_merge_suggestion_confidence_profile_min_confidence(
            "review");
    REQUIRE(review.has_value());
    CHECK(*review == 75U);

    const auto exploratory =
        featherdoc_cli::style_merge_suggestion_confidence_profile_min_confidence(
            "exploratory");
    REQUIRE(exploratory.has_value());
    CHECK(*exploratory == 0U);

    CHECK_FALSE(
        featherdoc_cli::style_merge_suggestion_confidence_profile_min_confidence(
            "recommended")
            .has_value());
    CHECK(featherdoc_cli::style_merge_suggestion_confidence_profile_is_valid(
        "recommended"));
    CHECK_FALSE(featherdoc_cli::style_merge_suggestion_confidence_profile_is_valid(
        "loose"));
}

TEST_CASE("cli style refactor plan filters by minimum confidence and recounts") {
    auto plan = make_plan({
        make_operation(featherdoc::style_refactor_action::merge, "A", "B", 95U,
                       true, 0U),
        make_operation(featherdoc::style_refactor_action::merge, "C", "D", 80U,
                       false, 2U),
        make_operation(featherdoc::style_refactor_action::merge, "E", "F",
                       std::nullopt, true, 1U),
    });

    const auto unfiltered =
        featherdoc_cli::filter_style_refactor_plan_by_min_confidence(plan, 0U);
    CHECK(unfiltered.operation_count == 3U);
    CHECK(unfiltered.applyable_count == 2U);
    CHECK(unfiltered.issue_count == 3U);

    const auto filtered =
        featherdoc_cli::filter_style_refactor_plan_by_min_confidence(
            std::move(plan), 90U);
    REQUIRE(filtered.operations.size() == 1U);
    CHECK(filtered.operation_count == 1U);
    CHECK(filtered.applyable_count == 1U);
    CHECK(filtered.issue_count == 0U);
    CHECK(filtered.operations.front().source_style_id == "A");
}

TEST_CASE("cli style refactor plan filters source and target as AND conditions") {
    auto plan = make_plan({
        make_operation(featherdoc::style_refactor_action::merge, "SourceA",
                       "TargetA", 95U, true, 0U),
        make_operation(featherdoc::style_refactor_action::merge, "SourceA",
                       "TargetB", 90U, false, 1U),
        make_operation(featherdoc::style_refactor_action::merge, "SourceB",
                       "TargetA", 85U, true, 0U),
    });

    const auto source_and_target =
        featherdoc_cli::filter_style_refactor_plan_by_style_ids(
            plan, {"SourceA"}, {"TargetB"});
    REQUIRE(source_and_target.operations.size() == 1U);
    CHECK(source_and_target.operations.front().source_style_id == "SourceA");
    CHECK(source_and_target.operations.front().target_style_id == "TargetB");
    CHECK(source_and_target.operation_count == 1U);
    CHECK(source_and_target.applyable_count == 0U);
    CHECK(source_and_target.issue_count == 1U);

    const auto target_only =
        featherdoc_cli::filter_style_refactor_plan_by_style_ids(
            std::move(plan), {}, {"TargetA"});
    REQUIRE(target_only.operations.size() == 2U);
    CHECK(target_only.operation_count == 2U);
    CHECK(target_only.applyable_count == 2U);
    CHECK(target_only.issue_count == 0U);
}
