#include "cli_test_support.hpp"

namespace {
TEST_CASE("cli review mutation plan updates comment metadata") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_review_mutation_plan_comment_metadata_source.docx";
    const fs::path applied =
        working_directory / "cli_review_mutation_plan_comment_metadata_output.docx";
    const fs::path mismatch_output =
        working_directory / "cli_review_mutation_plan_comment_metadata_mismatch.docx";
    const fs::path parent_output =
        working_directory / "cli_review_mutation_plan_comment_metadata_parent.docx";
    const fs::path bounds_output =
        working_directory / "cli_review_mutation_plan_comment_metadata_bounds.docx";
    const fs::path success_plan =
        working_directory / "cli_review_mutation_plan_comment_metadata.json";
    const fs::path mismatch_plan =
        working_directory / "cli_review_mutation_plan_comment_metadata_mismatch.json";
    const fs::path parent_plan =
        working_directory / "cli_review_mutation_plan_comment_metadata_parent.json";
    const fs::path conflict_plan =
        working_directory / "cli_review_mutation_plan_comment_metadata_conflict.json";
    const fs::path bounds_plan =
        working_directory / "cli_review_mutation_plan_comment_metadata_bounds.json";
    const fs::path output =
        working_directory / "cli_review_mutation_plan_comment_metadata_output.json";
    const fs::path inspect_output =
        working_directory / "cli_review_mutation_plan_comment_metadata_inspect.json";

    remove_if_exists(source);
    remove_if_exists(applied);
    remove_if_exists(mismatch_output);
    remove_if_exists(parent_output);
    remove_if_exists(bounds_output);
    remove_if_exists(success_plan);
    remove_if_exists(mismatch_plan);
    remove_if_exists(parent_plan);
    remove_if_exists(conflict_plan);
    remove_if_exists(bounds_plan);
    remove_if_exists(output);
    remove_if_exists(inspect_output);

    featherdoc::Document source_document(source);
    REQUIRE_FALSE(source_document.create_empty());
    auto paragraph = source_document.paragraphs();
    REQUIRE(paragraph.has_next());
    REQUIRE(paragraph.add_run("Alpha ").has_next());
    REQUIRE(paragraph.add_run("Beta").has_next());
    REQUIRE_FALSE(source_document.save());
    REQUIRE_EQ(run_cli({"append-paragraph-text-comment",
                        source.string(),
                        "0",
                        "0",
                        "10",
                        "--comment-text",
                        "Parent metadata comment.",
                        "--author",
                        "Parent Reviewer",
                        "--initials",
                        "PR",
                        "--date",
                        "2026-05-03T16:30:00Z",
                        "--json"},
                       output),
               0);
    REQUIRE_EQ(run_cli({"append-comment-reply",
                        source.string(),
                        "0",
                        "--comment-text",
                        "Reply metadata body.",
                        "--author",
                        "Reply Reviewer",
                        "--initials",
                        "RR",
                        "--date",
                        "2026-05-03T16:40:00Z",
                        "--json"},
                       output),
               0);

    write_binary_file(
        success_plan,
        R"({"operations":[{"kind":"set_comment_metadata","comment_index":1,"author":"Plan Updated Responder","clear_initials":true,"date":"2026-05-03T17:10:00Z","expected_comment_text":"Reply metadata body.","expected_parent_index":0,"expected_resolved":false}]})");
    CHECK_EQ(run_cli({"preview-review-mutation-plan",
                      source.string(),
                      "--plan-file",
                      success_plan.string(),
                      "--json"},
                     output),
             0);
    auto output_json = read_text_file(output);
    CHECK_NE(output_json.find(R"("kind":"set_comment_metadata")"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("comment_index":1)"), std::string::npos);
    CHECK_NE(output_json.find(R"("expected_comment_text":"Reply metadata body.")"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("actual_comment_text":"Reply metadata body.")"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("expected_parent_index":0)"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("actual_parent_index":0)"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("actual_resolved":false)"),
             std::string::npos);

    CHECK_EQ(run_cli({"apply-review-mutation-plan",
                      source.string(),
                      "--plan-file",
                      success_plan.string(),
                      "--output",
                      applied.string(),
                      "--json"},
                     output),
             0);
    output_json = read_text_file(output);
    CHECK_NE(output_json.find(R"("applied_count":1)"), std::string::npos);
    CHECK_EQ(run_cli({"inspect-review", applied.string(), "--json"},
                     inspect_output),
             0);
    const auto inspect_json = read_text_file(inspect_output);
    CHECK_NE(inspect_json.find(R"("comments_count":2)"), std::string::npos);
    CHECK_NE(inspect_json.find(R"("text":"Reply metadata body.")"),
             std::string::npos);
    CHECK_NE(inspect_json.find(R"("author":"Plan Updated Responder")"),
             std::string::npos);
    CHECK_NE(inspect_json.find(R"("initials":null)"), std::string::npos);
    CHECK_NE(inspect_json.find(R"("date":"2026-05-03T17:10:00Z")"),
             std::string::npos);
    CHECK_NE(inspect_json.find(R"("parent_index":0)"), std::string::npos);

    write_binary_file(
        mismatch_plan,
        R"({"operations":[{"kind":"set-comment-metadata","comment_index":1,"author":"Wrong target","expected_comment_text":"Wrong reply body.","expected_parent_index":0}]})");
    CHECK_EQ(run_cli({"apply-review-mutation-plan",
                      source.string(),
                      "--plan-file",
                      mismatch_plan.string(),
                      "--output",
                      mismatch_output.string(),
                      "--json"},
                     output),
             1);
    output_json = read_text_file(output);
    CHECK_NE(output_json.find(R"("stage":"preflight")"), std::string::npos);
    CHECK_NE(output_json.find("expected comment text did not match comment body"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("actual_comment_text":"Reply metadata body.")"),
             std::string::npos);
    CHECK_FALSE(fs::exists(mismatch_output));

    write_binary_file(
        parent_plan,
        R"({"operations":[{"kind":"set_comment_metadata","comment_index":1,"clear_author":true,"expected_comment_text":"Reply metadata body.","expected_parent_index":3}]})");
    CHECK_EQ(run_cli({"apply-review-mutation-plan",
                      source.string(),
                      "--plan-file",
                      parent_plan.string(),
                      "--output",
                      parent_output.string(),
                      "--json"},
                     output),
             1);
    output_json = read_text_file(output);
    CHECK_NE(output_json.find("expected parent index did not match comment parent"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("actual_parent_index":0)"),
             std::string::npos);
    CHECK_FALSE(fs::exists(parent_output));

    write_binary_file(
        conflict_plan,
        R"({"operations":[{"kind":"set_comment_metadata","comment_index":1,"author":"Conflicting","clear_author":true}]})");
    CHECK_EQ(run_cli({"preview-review-mutation-plan",
                      source.string(),
                      "--plan-file",
                      conflict_plan.string(),
                      "--json"},
                     output),
             2);
    output_json = read_text_file(output);
    CHECK_NE(output_json.find(
                 "cannot set and clear the same metadata field"),
             std::string::npos);

    write_binary_file(
        bounds_plan,
        R"({"operations":[{"kind":"set_comment_metadata","comment_index":9,"author":"Out of range"}]})");
    CHECK_EQ(run_cli({"preview-review-mutation-plan",
                      source.string(),
                      "--plan-file",
                      bounds_plan.string(),
                      "--json"},
                     output),
             1);
    output_json = read_text_file(output);
    CHECK_NE(output_json.find("comment index is out of range"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("comment_index":9)"), std::string::npos);
    CHECK_FALSE(fs::exists(bounds_output));

    remove_if_exists(source);
    remove_if_exists(applied);
    remove_if_exists(mismatch_output);
    remove_if_exists(parent_output);
    remove_if_exists(bounds_output);
    remove_if_exists(success_plan);
    remove_if_exists(mismatch_plan);
    remove_if_exists(parent_plan);
    remove_if_exists(conflict_plan);
    remove_if_exists(bounds_plan);
    remove_if_exists(output);
    remove_if_exists(inspect_output);
}

TEST_CASE("cli review mutation plan replaces and removes comments") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_review_mutation_plan_comment_body_source.docx";
    const fs::path applied =
        working_directory / "cli_review_mutation_plan_comment_body_output.docx";
    const fs::path parent_removed =
        working_directory / "cli_review_mutation_plan_comment_body_parent_removed.docx";
    const fs::path mismatch_output =
        working_directory / "cli_review_mutation_plan_comment_body_mismatch.docx";
    const fs::path parent_mismatch_output =
        working_directory / "cli_review_mutation_plan_comment_body_parent_mismatch.docx";
    const fs::path success_plan =
        working_directory / "cli_review_mutation_plan_comment_body.json";
    const fs::path parent_remove_plan =
        working_directory / "cli_review_mutation_plan_comment_body_parent_remove.json";
    const fs::path mismatch_plan =
        working_directory / "cli_review_mutation_plan_comment_body_mismatch.json";
    const fs::path parent_mismatch_plan =
        working_directory / "cli_review_mutation_plan_comment_body_parent_mismatch.json";
    const fs::path output =
        working_directory / "cli_review_mutation_plan_comment_body_output.json";
    const fs::path inspect_output =
        working_directory / "cli_review_mutation_plan_comment_body_inspect.json";

    remove_if_exists(source);
    remove_if_exists(applied);
    remove_if_exists(parent_removed);
    remove_if_exists(mismatch_output);
    remove_if_exists(parent_mismatch_output);
    remove_if_exists(success_plan);
    remove_if_exists(parent_remove_plan);
    remove_if_exists(mismatch_plan);
    remove_if_exists(parent_mismatch_plan);
    remove_if_exists(output);
    remove_if_exists(inspect_output);

    featherdoc::Document source_document(source);
    REQUIRE_FALSE(source_document.create_empty());
    auto paragraph = source_document.paragraphs();
    REQUIRE(paragraph.has_next());
    REQUIRE(paragraph.add_run("Alpha ").has_next());
    REQUIRE(paragraph.add_run("Beta").has_next());
    REQUIRE_FALSE(source_document.save());
    REQUIRE_EQ(run_cli({"append-paragraph-text-comment",
                        source.string(),
                        "0",
                        "0",
                        "10",
                        "--comment-text",
                        "Original parent body.",
                        "--author",
                        "Parent Reviewer",
                        "--initials",
                        "PR",
                        "--date",
                        "2026-05-03T17:40:00Z",
                        "--json"},
                       output),
               0);
    REQUIRE_EQ(run_cli({"append-comment-reply",
                        source.string(),
                        "0",
                        "--comment-text",
                        "Reply body to remove.",
                        "--author",
                        "Reply Reviewer",
                        "--initials",
                        "RR",
                        "--date",
                        "2026-05-03T17:45:00Z",
                        "--json"},
                       output),
               0);

    write_binary_file(
        success_plan,
        R"({"operations":[{"kind":"replace_comment","comment_index":0,"comment_text":"Plan replaced parent body.","expected_text":"Alpha Beta","expected_comment_text":"Original parent body.","expected_resolved":false},{"kind":"remove_comment","comment_index":1,"expected_comment_text":"Reply body to remove.","expected_parent_index":0}]})");
    CHECK_EQ(run_cli({"preview-review-mutation-plan",
                      source.string(),
                      "--plan-file",
                      success_plan.string(),
                      "--json"},
                     output),
             0);
    auto output_json = read_text_file(output);
    CHECK_NE(output_json.find(R"("kind":"replace_comment")"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("kind":"remove_comment")"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("actual_text":"Alpha Beta")"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("actual_comment_text":"Original parent body.")"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("actual_comment_text":"Reply body to remove.")"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("actual_parent_index":0)"),
             std::string::npos);

    CHECK_EQ(run_cli({"apply-review-mutation-plan",
                      source.string(),
                      "--plan-file",
                      success_plan.string(),
                      "--output",
                      applied.string(),
                      "--json"},
                     output),
             0);
    output_json = read_text_file(output);
    CHECK_NE(output_json.find(R"("applied_count":2)"), std::string::npos);
    CHECK_EQ(run_cli({"inspect-review", applied.string(), "--json"},
                     inspect_output),
             0);
    auto inspect_json = read_text_file(inspect_output);
    CHECK_NE(inspect_json.find(R"("comments_count":1)"), std::string::npos);
    CHECK_NE(inspect_json.find(R"("anchor_text":"Alpha Beta")"),
             std::string::npos);
    CHECK_NE(inspect_json.find(R"("text":"Plan replaced parent body.")"),
             std::string::npos);
    CHECK_EQ(inspect_json.find(R"("text":"Reply body to remove.")"),
             std::string::npos);

    write_binary_file(
        parent_remove_plan,
        R"({"operations":[{"kind":"remove_comment","comment_index":0,"expected_text":"Alpha Beta","expected_comment_text":"Original parent body.","expected_resolved":false}]})");
    CHECK_EQ(run_cli({"apply-review-mutation-plan",
                      source.string(),
                      "--plan-file",
                      parent_remove_plan.string(),
                      "--output",
                      parent_removed.string(),
                      "--json"},
                     output),
             0);
    output_json = read_text_file(output);
    CHECK_NE(output_json.find(R"("applied_count":1)"), std::string::npos);
    CHECK_EQ(run_cli({"inspect-review", parent_removed.string(), "--json"},
                     inspect_output),
             0);
    inspect_json = read_text_file(inspect_output);
    CHECK_NE(inspect_json.find(R"("comments_count":0)"), std::string::npos);

    write_binary_file(
        mismatch_plan,
        R"({"operations":[{"kind":"replace-comment","comment_index":0,"comment_text":"Wrong replacement target.","expected_text":"Wrong anchor","expected_comment_text":"Original parent body."}]})");
    CHECK_EQ(run_cli({"apply-review-mutation-plan",
                      source.string(),
                      "--plan-file",
                      mismatch_plan.string(),
                      "--output",
                      mismatch_output.string(),
                      "--json"},
                     output),
             1);
    output_json = read_text_file(output);
    CHECK_NE(output_json.find("expected text did not match comment anchor text"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("actual_text":"Alpha Beta")"),
             std::string::npos);
    CHECK_FALSE(fs::exists(mismatch_output));

    write_binary_file(
        parent_mismatch_plan,
        R"({"operations":[{"kind":"remove_comment","comment_index":1,"expected_comment_text":"Reply body to remove.","expected_parent_index":5}]})");
    CHECK_EQ(run_cli({"apply-review-mutation-plan",
                      source.string(),
                      "--plan-file",
                      parent_mismatch_plan.string(),
                      "--output",
                      parent_mismatch_output.string(),
                      "--json"},
                     output),
             1);
    output_json = read_text_file(output);
    CHECK_NE(output_json.find("expected parent index did not match comment parent"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("actual_parent_index":0)"),
             std::string::npos);
    CHECK_FALSE(fs::exists(parent_mismatch_output));

    remove_if_exists(source);
    remove_if_exists(applied);
    remove_if_exists(parent_removed);
    remove_if_exists(mismatch_output);
    remove_if_exists(parent_mismatch_output);
    remove_if_exists(success_plan);
    remove_if_exists(parent_remove_plan);
    remove_if_exists(mismatch_plan);
    remove_if_exists(parent_mismatch_plan);
    remove_if_exists(output);
    remove_if_exists(inspect_output);
}


} // namespace
