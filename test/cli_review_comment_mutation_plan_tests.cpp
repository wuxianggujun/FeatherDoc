#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "cli_test_support.hpp"

namespace {

TEST_CASE("cli review mutation plan allows overlapping comment anchors") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_review_mutation_plan_comment_overlap_source.docx";
    const fs::path applied =
        working_directory / "cli_review_mutation_plan_comment_overlap_output.docx";
    const fs::path plan =
        working_directory / "cli_review_mutation_plan_comment_overlap.json";
    const fs::path output =
        working_directory / "cli_review_mutation_plan_comment_overlap_output.json";
    const fs::path inspect_output =
        working_directory / "cli_review_mutation_plan_comment_overlap_inspect.json";

    remove_if_exists(source);
    remove_if_exists(applied);
    remove_if_exists(plan);
    remove_if_exists(output);
    remove_if_exists(inspect_output);

    featherdoc::Document source_document(source);
    REQUIRE_FALSE(source_document.create_empty());
    auto paragraph = source_document.paragraphs();
    REQUIRE(paragraph.has_next());
    REQUIRE(paragraph.add_run("Alpha ").has_next());
    REQUIRE(paragraph.add_run("Beta").has_next());
    REQUIRE_FALSE(source_document.save());

    write_binary_file(
        plan,
        R"({"operations":[)"
        R"({"kind":"append_paragraph_text_comment","paragraph_index":0,"text_offset":0,"text_length":10,"comment_text":"Check the full phrase.","expected_text":"Alpha Beta","author":"Outer Commenter","initials":"OC","date":"2026-05-03T14:00:00Z"},)"
        R"({"kind":"append_paragraph_text_comment","paragraph_index":0,"text_offset":6,"text_length":4,"comment_text":"Check the nested word.","expected_text":"Beta","author":"Inner Commenter","initials":"IC","date":"2026-05-03T14:01:00Z"}]})");
    CHECK_EQ(run_cli({"apply-review-mutation-plan",
                      source.string(),
                      "--plan-file",
                      plan.string(),
                      "--output",
                      applied.string(),
                      "--json"},
                     output),
             0);
    auto output_json = read_text_file(output);
    CHECK_NE(output_json.find(R"("ok":true)"), std::string::npos);
    CHECK_NE(output_json.find(R"("operations_count":2)"), std::string::npos);
    CHECK_NE(output_json.find(R"("applied_count":2)"), std::string::npos);
    CHECK_NE(output_json.find(R"("failed_count":0)"), std::string::npos);
    CHECK(fs::exists(applied));

    CHECK_EQ(run_cli({"inspect-review", applied.string(), "--json"},
                     inspect_output),
             0);
    const auto inspect_json = read_text_file(inspect_output);
    CHECK_NE(inspect_json.find(R"("comments_count":2)"), std::string::npos);
    CHECK_NE(inspect_json.find(R"("anchor_text":"Alpha Beta")"),
             std::string::npos);
    CHECK_NE(inspect_json.find(R"("anchor_text":"Beta")"), std::string::npos);
    CHECK_NE(inspect_json.find(R"("text":"Check the full phrase.")"),
             std::string::npos);
    CHECK_NE(inspect_json.find(R"("text":"Check the nested word.")"),
             std::string::npos);
    CHECK_NE(inspect_json.find(R"("initials":"OC")"), std::string::npos);
    CHECK_NE(inspect_json.find(R"("initials":"IC")"), std::string::npos);

    remove_if_exists(source);
    remove_if_exists(applied);
    remove_if_exists(plan);
    remove_if_exists(output);
    remove_if_exists(inspect_output);
}

TEST_CASE("cli review mutation plan sets comment resolved state") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_review_mutation_plan_comment_resolved_source.docx";
    const fs::path applied =
        working_directory / "cli_review_mutation_plan_comment_resolved_output.docx";
    const fs::path mismatch_output =
        working_directory / "cli_review_mutation_plan_comment_resolved_mismatch.docx";
    const fs::path state_output =
        working_directory / "cli_review_mutation_plan_comment_resolved_state.docx";
    const fs::path bounds_output =
        working_directory / "cli_review_mutation_plan_comment_resolved_bounds.docx";
    const fs::path success_plan =
        working_directory / "cli_review_mutation_plan_comment_resolved.json";
    const fs::path mismatch_plan =
        working_directory / "cli_review_mutation_plan_comment_resolved_mismatch.json";
    const fs::path state_plan =
        working_directory / "cli_review_mutation_plan_comment_resolved_state.json";
    const fs::path bounds_plan =
        working_directory / "cli_review_mutation_plan_comment_resolved_bounds.json";
    const fs::path output =
        working_directory / "cli_review_mutation_plan_comment_resolved_output.json";
    const fs::path inspect_output =
        working_directory / "cli_review_mutation_plan_comment_resolved_inspect.json";

    remove_if_exists(source);
    remove_if_exists(applied);
    remove_if_exists(mismatch_output);
    remove_if_exists(state_output);
    remove_if_exists(bounds_output);
    remove_if_exists(success_plan);
    remove_if_exists(mismatch_plan);
    remove_if_exists(state_plan);
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
                        "Resolve this comment.",
                        "--author",
                        "Plan Reviewer",
                        "--initials",
                        "PR",
                        "--date",
                        "2026-05-03T15:00:00Z",
                        "--json"},
                       output),
               0);

    write_binary_file(
        success_plan,
        R"({"operations":[{"kind":"set_comment_resolved","comment_index":0,"resolved":true,"expected_resolved":false,"expected_text":"Alpha Beta"}]})");
    CHECK_EQ(run_cli({"preview-review-mutation-plan",
                      source.string(),
                      "--plan-file",
                      success_plan.string(),
                      "--json"},
                     output),
             0);
    auto output_json = read_text_file(output);
    CHECK_NE(output_json.find(R"("kind":"set_comment_resolved")"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("comment_index":0)"), std::string::npos);
    CHECK_NE(output_json.find(R"("expected_resolved":false)"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("actual_resolved":false)"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("actual_text":"Alpha Beta")"),
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
    CHECK_NE(inspect_json.find(R"("comments_count":1)"), std::string::npos);
    CHECK_NE(inspect_json.find(R"("resolved":true)"), std::string::npos);
    CHECK_NE(inspect_json.find(R"("anchor_text":"Alpha Beta")"),
             std::string::npos);

    write_binary_file(
        mismatch_plan,
        R"({"operations":[{"kind":"set-comment-resolved","comment_index":0,"resolved":true,"expected_resolved":false,"expected_text":"Wrong anchor"}]})");
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
    CHECK_NE(output_json.find("expected text did not match comment anchor text"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("actual_text":"Alpha Beta")"),
             std::string::npos);
    CHECK_FALSE(fs::exists(mismatch_output));

    write_binary_file(
        state_plan,
        R"({"operations":[{"kind":"set_comment_resolved","comment_index":0,"resolved":true,"expected_resolved":true,"expected_text":"Alpha Beta"}]})");
    CHECK_EQ(run_cli({"apply-review-mutation-plan",
                      source.string(),
                      "--plan-file",
                      state_plan.string(),
                      "--output",
                      state_output.string(),
                      "--json"},
                     output),
             1);
    output_json = read_text_file(output);
    CHECK_NE(output_json.find("expected resolved state did not match comment state"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("actual_resolved":false)"),
             std::string::npos);
    CHECK_FALSE(fs::exists(state_output));

    write_binary_file(
        bounds_plan,
        R"({"operations":[{"kind":"set_comment_resolved","comment_index":5,"resolved":true}]})");
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
    CHECK_NE(output_json.find(R"("comment_index":5)"), std::string::npos);
    CHECK_FALSE(fs::exists(bounds_output));

    remove_if_exists(source);
    remove_if_exists(applied);
    remove_if_exists(mismatch_output);
    remove_if_exists(state_output);
    remove_if_exists(bounds_output);
    remove_if_exists(success_plan);
    remove_if_exists(mismatch_plan);
    remove_if_exists(state_plan);
    remove_if_exists(bounds_plan);
    remove_if_exists(output);
    remove_if_exists(inspect_output);
}

TEST_CASE("cli review mutation plan appends comment replies") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_review_mutation_plan_comment_reply_source.docx";
    const fs::path applied =
        working_directory / "cli_review_mutation_plan_comment_reply_output.docx";
    const fs::path mismatch_output =
        working_directory / "cli_review_mutation_plan_comment_reply_mismatch.docx";
    const fs::path state_output =
        working_directory / "cli_review_mutation_plan_comment_reply_state.docx";
    const fs::path bounds_output =
        working_directory / "cli_review_mutation_plan_comment_reply_bounds.docx";
    const fs::path success_plan =
        working_directory / "cli_review_mutation_plan_comment_reply.json";
    const fs::path mismatch_plan =
        working_directory / "cli_review_mutation_plan_comment_reply_mismatch.json";
    const fs::path state_plan =
        working_directory / "cli_review_mutation_plan_comment_reply_state.json";
    const fs::path bounds_plan =
        working_directory / "cli_review_mutation_plan_comment_reply_bounds.json";
    const fs::path build_request =
        working_directory / "cli_review_mutation_plan_comment_reply_build_request.json";
    const fs::path built_plan =
        working_directory / "cli_review_mutation_plan_comment_reply_built.json";
    const fs::path output =
        working_directory / "cli_review_mutation_plan_comment_reply_output.json";
    const fs::path inspect_output =
        working_directory / "cli_review_mutation_plan_comment_reply_inspect.json";

    remove_if_exists(source);
    remove_if_exists(applied);
    remove_if_exists(mismatch_output);
    remove_if_exists(state_output);
    remove_if_exists(bounds_output);
    remove_if_exists(success_plan);
    remove_if_exists(mismatch_plan);
    remove_if_exists(state_plan);
    remove_if_exists(bounds_plan);
    remove_if_exists(build_request);
    remove_if_exists(built_plan);
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
                        "Parent comment.",
                        "--author",
                        "Plan Reviewer",
                        "--initials",
                        "PR",
                        "--date",
                        "2026-05-03T15:30:00Z",
                        "--json"},
                       output),
               0);

    write_binary_file(
        success_plan,
        R"({"operations":[{"kind":"append_comment_reply","comment_index":0,"comment_text":"Reply from plan.","author":"Reply Reviewer","initials":"RR","date":"2026-05-03T16:00:00Z","expected_resolved":false,"expected_text":"Alpha Beta"}]})");
    CHECK_EQ(run_cli({"preview-review-mutation-plan",
                      source.string(),
                      "--plan-file",
                      success_plan.string(),
                      "--json"},
                     output),
             0);
    auto output_json = read_text_file(output);
    CHECK_NE(output_json.find(R"("kind":"append_comment_reply")"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("comment_index":0)"), std::string::npos);
    CHECK_NE(output_json.find(R"("expected_resolved":false)"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("actual_resolved":false)"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("actual_text":"Alpha Beta")"),
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
    CHECK_NE(inspect_json.find(R"("anchor_text":"Alpha Beta")"),
             std::string::npos);
    CHECK_NE(inspect_json.find(R"("text":"Reply from plan.")"),
             std::string::npos);
    CHECK_NE(inspect_json.find(R"("author":"Reply Reviewer")"),
             std::string::npos);
    CHECK_NE(inspect_json.find(R"("initials":"RR")"), std::string::npos);
    CHECK_NE(inspect_json.find(R"("date":"2026-05-03T16:00:00Z")"),
             std::string::npos);
    CHECK_NE(inspect_json.find(R"("parent_index":0)"), std::string::npos);

    write_binary_file(
        mismatch_plan,
        R"({"operations":[{"kind":"append-comment-reply","comment_index":0,"comment_text":"Reply from mismatch plan.","expected_resolved":false,"expected_text":"Wrong anchor"}]})");
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
    CHECK_NE(output_json.find("expected text did not match comment anchor text"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("actual_text":"Alpha Beta")"),
             std::string::npos);
    CHECK_FALSE(fs::exists(mismatch_output));

    write_binary_file(
        state_plan,
        R"({"operations":[{"kind":"append_comment_reply","comment_index":0,"comment_text":"Reply from state plan.","expected_resolved":true,"expected_text":"Alpha Beta"}]})");
    CHECK_EQ(run_cli({"apply-review-mutation-plan",
                      source.string(),
                      "--plan-file",
                      state_plan.string(),
                      "--output",
                      state_output.string(),
                      "--json"},
                     output),
             1);
    output_json = read_text_file(output);
    CHECK_NE(output_json.find("expected resolved state did not match comment state"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("actual_resolved":false)"),
             std::string::npos);
    CHECK_FALSE(fs::exists(state_output));

    write_binary_file(
        bounds_plan,
        R"({"operations":[{"kind":"append_comment_reply","comment_index":5,"comment_text":"Out of range reply."}]})");
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
    CHECK_NE(output_json.find(R"("comment_index":5)"), std::string::npos);
    CHECK_FALSE(fs::exists(bounds_output));

    write_binary_file(
        build_request,
        R"({"operations":[{"kind":"append_comment_reply","find_text":"Alpha Beta","comment_text":"Reply from build request."}]})");
    CHECK_EQ(run_cli({"build-review-mutation-plan",
                      source.string(),
                      "--request-file",
                      build_request.string(),
                      "--output-plan",
                      built_plan.string(),
                      "--json"},
                     output),
             2);
    output_json = read_text_file(output);
    CHECK_NE(output_json.find(
                 "does not support direct comment-index operations"),
             std::string::npos);
    CHECK_FALSE(fs::exists(built_plan));

    remove_if_exists(source);
    remove_if_exists(applied);
    remove_if_exists(mismatch_output);
    remove_if_exists(state_output);
    remove_if_exists(bounds_output);
    remove_if_exists(success_plan);
    remove_if_exists(mismatch_plan);
    remove_if_exists(state_plan);
    remove_if_exists(bounds_plan);
    remove_if_exists(build_request);
    remove_if_exists(built_plan);
    remove_if_exists(output);
    remove_if_exists(inspect_output);
}


} // namespace
