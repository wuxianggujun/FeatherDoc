#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "cli_test_support.hpp"

namespace {

TEST_CASE("cli builds review mutation plans from found text") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_build_review_mutation_plan_source.docx";
    const fs::path request =
        working_directory / "cli_build_review_mutation_plan_request.json";
    const fs::path missing_request =
        working_directory / "cli_build_review_mutation_plan_missing.json";
    const fs::path context_request =
        working_directory / "cli_build_review_mutation_plan_context.json";
    const fs::path context_missing_request =
        working_directory /
        "cli_build_review_mutation_plan_context_missing.json";
    const fs::path unique_request =
        working_directory / "cli_build_review_mutation_plan_unique.json";
    const fs::path insert_request =
        working_directory / "cli_build_review_mutation_plan_insert.json";
    const fs::path comment_request =
        working_directory / "cli_build_review_mutation_plan_comment.json";
    const fs::path paragraph_request =
        working_directory / "cli_build_review_mutation_plan_paragraph.json";
    const fs::path plan =
        working_directory / "cli_build_review_mutation_plan_output.json";
    const fs::path context_plan =
        working_directory / "cli_build_review_mutation_plan_context_output.json";
    const fs::path insert_plan =
        working_directory / "cli_build_review_mutation_plan_insert_output.json";
    const fs::path comment_plan =
        working_directory / "cli_build_review_mutation_plan_comment_output.json";
    const fs::path applied =
        working_directory / "cli_build_review_mutation_plan_applied.docx";
    const fs::path insert_applied =
        working_directory / "cli_build_review_mutation_plan_insert_applied.docx";
    const fs::path comment_applied =
        working_directory / "cli_build_review_mutation_plan_comment_applied.docx";
    const fs::path output =
        working_directory / "cli_build_review_mutation_plan_result.json";
    const fs::path inspect_output =
        working_directory / "cli_build_review_mutation_plan_inspect.json";
    const fs::path insert_inspect_output =
        working_directory /
        "cli_build_review_mutation_plan_insert_inspect.json";
    const fs::path comment_inspect_output =
        working_directory /
        "cli_build_review_mutation_plan_comment_inspect.json";

    remove_if_exists(source);
    remove_if_exists(request);
    remove_if_exists(missing_request);
    remove_if_exists(context_request);
    remove_if_exists(context_missing_request);
    remove_if_exists(unique_request);
    remove_if_exists(insert_request);
    remove_if_exists(comment_request);
    remove_if_exists(paragraph_request);
    remove_if_exists(plan);
    remove_if_exists(context_plan);
    remove_if_exists(insert_plan);
    remove_if_exists(comment_plan);
    remove_if_exists(applied);
    remove_if_exists(insert_applied);
    remove_if_exists(comment_applied);
    remove_if_exists(output);
    remove_if_exists(inspect_output);
    remove_if_exists(insert_inspect_output);
    remove_if_exists(comment_inspect_output);

    featherdoc::Document source_document(source);
    REQUIRE_FALSE(source_document.create_empty());
    auto first = source_document.paragraphs();
    REQUIRE(first.has_next());
    REQUIRE(first.add_run("Alpha ").has_next());
    REQUIRE(first.add_run("Beta").has_next());
    auto second = first.insert_paragraph_after("Middle ");
    REQUIRE(second.has_next());
    REQUIRE(second.add_run("Text").has_next());
    auto third = second.insert_paragraph_after("Beta ");
    REQUIRE(third.has_next());
    REQUIRE(third.add_run("Tail").has_next());
    REQUIRE_FALSE(source_document.save());

    write_binary_file(
        request,
        R"({"operations":[)"
        R"({"kind":"replace_text_range_revision","find_text":"BetaMiddle TextBeta","occurrence":0,"text":"Range ","author":"Plan Builder","date":"2026-05-03T11:00:00Z"},)"
        R"({"kind":"delete_text_range_revision","find_text":"Tail","occurrence":0,"author":"Plan Cleaner","date":"2026-05-03T11:05:00Z"}]})");
    CHECK_EQ(run_cli({"build-review-mutation-plan",
                      source.string(),
                      "--request-file",
                      request.string(),
                      "--output-plan",
                      plan.string(),
                      "--json"},
                     output),
             0);
    auto output_json = read_text_file(output);
    CHECK_NE(output_json.find(R"("command":"build-review-mutation-plan")"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("ok":true)"), std::string::npos);
    CHECK_NE(output_json.find(R"("operations_count":2)"), std::string::npos);
    CHECK_NE(output_json.find(R"("output_plan_path":")"), std::string::npos);
    CHECK_NE(output_json.find(R"("find_text":"BetaMiddle TextBeta")"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("matches_count":1)"), std::string::npos);
    CHECK_NE(output_json.find(R"("start_paragraph_index":0)"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("end_paragraph_index":2)"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("end_text_offset":4)"), std::string::npos);
    CHECK(fs::exists(plan));
    const auto plan_json = read_text_file(plan);
    CHECK_NE(plan_json.find(R"("kind":"replace_text_range_revision")"),
             std::string::npos);
    CHECK_NE(plan_json.find(R"("expected_text":"BetaMiddle TextBeta")"),
             std::string::npos);
    CHECK_NE(plan_json.find(R"("text":"Range ")"), std::string::npos);
    CHECK_NE(plan_json.find(R"("author":"Plan Cleaner")"),
             std::string::npos);

    CHECK_EQ(run_cli({"preview-review-mutation-plan",
                      source.string(),
                      "--plan-file",
                      plan.string(),
                      "--json"},
                     output),
             0);

    CHECK_EQ(run_cli({"apply-review-mutation-plan",
                      source.string(),
                      "--plan-file",
                      plan.string(),
                      "--output",
                      applied.string(),
                      "--json"},
                     output),
             0);
    CHECK_EQ(run_cli({"inspect-review", applied.string(), "--json"},
                     inspect_output),
             0);
    const auto inspect_json = read_text_file(inspect_output);
    CHECK_NE(inspect_json.find(R"("text":"Range ")"), std::string::npos);
    CHECK_NE(inspect_json.find(R"("text":"Beta")"), std::string::npos);
    CHECK_NE(inspect_json.find(R"("text":"Middle Text")"),
             std::string::npos);
    CHECK_NE(inspect_json.find(R"("text":"Tail")"), std::string::npos);
    CHECK_NE(inspect_json.find(R"("author":"Plan Builder")"),
             std::string::npos);
    CHECK_NE(inspect_json.find(R"("author":"Plan Cleaner")"),
             std::string::npos);

    write_binary_file(
        context_request,
        R"({"operations":[{"kind":"delete_text_range_revision","find_text":"Beta","before_text":"Middle Text","after_text":" Tail","require_unique":true,"occurrence":0,"author":"Context Picker","date":"2026-05-03T11:10:00Z"}]})");
    CHECK_EQ(run_cli({"build-review-mutation-plan",
                      source.string(),
                      "--request-file",
                      context_request.string(),
                      "--output-plan",
                      context_plan.string(),
                      "--json"},
                     output),
             0);
    output_json = read_text_file(output);
    CHECK_NE(output_json.find(R"("raw_matches_count":2)"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("matches_count":1)"), std::string::npos);
    CHECK_NE(output_json.find(R"("selected_match_index":1)"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("require_unique":true)"), std::string::npos);
    CHECK_NE(output_json.find(R"("before_text":"Middle Text")"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("after_text":" Tail")"), std::string::npos);
    CHECK_NE(output_json.find(R"("start_paragraph_index":2)"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("start_text_offset":0)"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("end_text_offset":4)"), std::string::npos);
    const auto context_plan_json = read_text_file(context_plan);
    CHECK_NE(context_plan_json.find(R"("expected_text":"Beta")"),
             std::string::npos);
    CHECK_NE(context_plan_json.find(R"("author":"Context Picker")"),
             std::string::npos);

    write_binary_file(
        insert_request,
        R"({"operations":[)"
        R"({"kind":"insert_text_range_revision","find_text":"Beta","before_text":"Middle Text","after_text":" Tail","require_unique":true,"insert_after_match":true,"text":" inserted-after ","author":"Insert Builder","date":"2026-05-03T12:40:00Z"},)"
        R"({"kind":"insert_paragraph_text_revision","find_text":"Alpha","require_unique":true,"insert_after_match":false,"text":"Start ","author":"Paragraph Inserter","date":"2026-05-03T12:41:00Z"}]})");
    CHECK_EQ(run_cli({"build-review-mutation-plan",
                      source.string(),
                      "--request-file",
                      insert_request.string(),
                      "--output-plan",
                      insert_plan.string(),
                      "--json"},
                     output),
             0);
    output_json = read_text_file(output);
    CHECK_NE(output_json.find(R"("operations_count":2)"), std::string::npos);
    CHECK_NE(output_json.find(R"("kind":"insert_text_range_revision")"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("kind":"insert_paragraph_text_revision")"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("insert_after_match":true)"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("start_paragraph_index":2)"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("start_text_offset":4)"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("paragraph_index":0)"), std::string::npos);
    CHECK_NE(output_json.find(R"("text_offset":0)"), std::string::npos);
    CHECK(fs::exists(insert_plan));
    const auto insert_plan_json = read_text_file(insert_plan);
    CHECK_NE(insert_plan_json.find(R"("kind":"insert_text_range_revision")"),
             std::string::npos);
    CHECK_NE(
        insert_plan_json.find(R"("kind":"insert_paragraph_text_revision")"),
        std::string::npos);
    CHECK_NE(insert_plan_json.find(R"("text":" inserted-after ")"),
             std::string::npos);
    CHECK_NE(insert_plan_json.find(R"("text":"Start ")"), std::string::npos);
    CHECK_EQ(insert_plan_json.find(R"("expected_text")"), std::string::npos);
    CHECK_EQ(insert_plan_json.find(R"("text_length")"), std::string::npos);
    CHECK_EQ(insert_plan_json.find(R"("end_paragraph_index")"),
             std::string::npos);
    CHECK_EQ(insert_plan_json.find(R"("end_text_offset")"), std::string::npos);

    CHECK_EQ(run_cli({"preview-review-mutation-plan",
                      source.string(),
                      "--plan-file",
                      insert_plan.string(),
                      "--json"},
                     output),
             0);
    CHECK_EQ(run_cli({"apply-review-mutation-plan",
                      source.string(),
                      "--plan-file",
                      insert_plan.string(),
                      "--output",
                      insert_applied.string(),
                      "--json"},
                     output),
             0);
    CHECK_EQ(run_cli({"inspect-review", insert_applied.string(), "--json"},
                     insert_inspect_output),
             0);
    const auto insert_inspect_json = read_text_file(insert_inspect_output);
    CHECK_NE(insert_inspect_json.find(R"("text":"Start ")"),
             std::string::npos);
    CHECK_NE(insert_inspect_json.find(R"("text":" inserted-after ")"),
             std::string::npos);
    CHECK_NE(insert_inspect_json.find(R"("author":"Paragraph Inserter")"),
             std::string::npos);
    CHECK_NE(insert_inspect_json.find(R"("author":"Insert Builder")"),
             std::string::npos);

    write_binary_file(
        comment_request,
        R"({"operations":[)"
        R"({"kind":"append_text_range_comment","find_text":"BetaMiddle TextBeta","comment_text":"Review this cross-paragraph span.","author":"Comment Builder","initials":"CB","date":"2026-05-03T12:45:00Z"},)"
        R"({"kind":"append_paragraph_text_comment","find_text":"Tail","comment_text":"Check final tail.","author":"Paragraph Commenter","initials":"PC","date":"2026-05-03T12:46:00Z"}]})");
    CHECK_EQ(run_cli({"build-review-mutation-plan",
                      source.string(),
                      "--request-file",
                      comment_request.string(),
                      "--output-plan",
                      comment_plan.string(),
                      "--json"},
                     output),
             0);
    output_json = read_text_file(output);
    CHECK_NE(output_json.find(R"("kind":"append_text_range_comment")"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("kind":"append_paragraph_text_comment")"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("text":"BetaMiddle TextBeta")"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("text":"Tail")"), std::string::npos);
    CHECK(fs::exists(comment_plan));
    const auto comment_plan_json = read_text_file(comment_plan);
    CHECK_NE(comment_plan_json.find(R"("comment_text":"Review this cross-paragraph span.")"),
             std::string::npos);
    CHECK_NE(comment_plan_json.find(R"("comment_text":"Check final tail.")"),
             std::string::npos);
    CHECK_NE(comment_plan_json.find(R"("expected_text":"BetaMiddle TextBeta")"),
             std::string::npos);
    CHECK_NE(comment_plan_json.find(R"("expected_text":"Tail")"),
             std::string::npos);
    CHECK_NE(comment_plan_json.find(R"("initials":"CB")"),
             std::string::npos);
    CHECK_EQ(comment_plan_json.find(R"("insert_after_match")"),
             std::string::npos);

    CHECK_EQ(run_cli({"preview-review-mutation-plan",
                      source.string(),
                      "--plan-file",
                      comment_plan.string(),
                      "--json"},
                     output),
             0);
    CHECK_EQ(run_cli({"apply-review-mutation-plan",
                      source.string(),
                      "--plan-file",
                      comment_plan.string(),
                      "--output",
                      comment_applied.string(),
                      "--json"},
                     output),
             0);
    CHECK_EQ(run_cli({"inspect-review", comment_applied.string(), "--json"},
                     comment_inspect_output),
             0);
    const auto comment_inspect_json = read_text_file(comment_inspect_output);
    CHECK_NE(comment_inspect_json.find(R"("comments_count":2)"),
             std::string::npos);
    CHECK_NE(comment_inspect_json.find(R"("text":"Review this cross-paragraph span.")"),
             std::string::npos);
    CHECK_NE(comment_inspect_json.find(R"("text":"Check final tail.")"),
             std::string::npos);
    CHECK_NE(comment_inspect_json.find(R"("anchor_text":"BetaMiddle TextBeta")"),
             std::string::npos);
    CHECK_NE(comment_inspect_json.find(R"("anchor_text":"Tail")"),
             std::string::npos);
    CHECK_NE(comment_inspect_json.find(R"("initials":"CB")"),
             std::string::npos);
    CHECK_NE(comment_inspect_json.find(R"("initials":"PC")"),
             std::string::npos);

    write_binary_file(
        context_missing_request,
        R"({"operations":[{"kind":"delete_text_range_revision","find_text":"Beta","before_text":"Missing context","occurrence":0}]})");
    CHECK_EQ(run_cli({"build-review-mutation-plan",
                      source.string(),
                      "--request-file",
                      context_missing_request.string(),
                      "--json"},
                     output),
             1);
    output_json = read_text_file(output);
    CHECK_NE(output_json.find(R"("matches_count":0)"), std::string::npos);
    CHECK_NE(output_json.find(R"("raw_matches_count":2)"),
             std::string::npos);
    CHECK_NE(output_json.find("requested text occurrence was not found"),
             std::string::npos);

    write_binary_file(
        unique_request,
        R"({"operations":[{"kind":"delete_text_range_revision","find_text":"Beta","require_unique":true}]})");
    CHECK_EQ(run_cli({"build-review-mutation-plan",
                      source.string(),
                      "--request-file",
                      unique_request.string(),
                      "--json"},
                     output),
             1);
    output_json = read_text_file(output);
    CHECK_NE(output_json.find(R"("matches_count":2)"), std::string::npos);
    CHECK_NE(output_json.find(R"("raw_matches_count":2)"),
             std::string::npos);
    CHECK_NE(output_json.find("requested text did not resolve to a unique match"),
             std::string::npos);

    write_binary_file(
        missing_request,
        R"({"operations":[{"kind":"delete_text_range_revision","find_text":"Missing","occurrence":0}]})");
    CHECK_EQ(run_cli({"build-review-mutation-plan",
                      source.string(),
                      "--request-file",
                      missing_request.string(),
                      "--json"},
                     output),
             1);
    output_json = read_text_file(output);
    CHECK_NE(output_json.find(R"("stage":"resolve")"), std::string::npos);
    CHECK_NE(output_json.find(R"("matches_count":0)"), std::string::npos);
    CHECK_NE(output_json.find("requested text occurrence was not found"),
             std::string::npos);

    write_binary_file(
        paragraph_request,
        R"({"operations":[{"kind":"replace_paragraph_text_revision","find_text":"BetaMiddle TextBeta","text":"Range "}]})");
    CHECK_EQ(run_cli({"build-review-mutation-plan",
                      source.string(),
                      "--request-file",
                      paragraph_request.string(),
                      "--json"},
                     output),
             1);
    output_json = read_text_file(output);
    CHECK_NE(output_json.find("matched text crosses paragraphs"),
             std::string::npos);

    remove_if_exists(source);
    remove_if_exists(request);
    remove_if_exists(missing_request);
    remove_if_exists(context_request);
    remove_if_exists(context_missing_request);
    remove_if_exists(unique_request);
    remove_if_exists(insert_request);
    remove_if_exists(comment_request);
    remove_if_exists(paragraph_request);
    remove_if_exists(plan);
    remove_if_exists(context_plan);
    remove_if_exists(insert_plan);
    remove_if_exists(comment_plan);
    remove_if_exists(applied);
    remove_if_exists(insert_applied);
    remove_if_exists(comment_applied);
    remove_if_exists(output);
    remove_if_exists(inspect_output);
    remove_if_exists(insert_inspect_output);
    remove_if_exists(comment_inspect_output);
}

TEST_CASE("cli review mutation plan previews expected text guards") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_review_mutation_plan_preview_source.docx";
    const fs::path success_plan =
        working_directory / "cli_review_mutation_plan_preview_success.json";
    const fs::path mismatch_plan =
        working_directory / "cli_review_mutation_plan_preview_mismatch.json";
    const fs::path invalid_plan =
        working_directory / "cli_review_mutation_plan_preview_invalid.json";
    const fs::path output =
        working_directory / "cli_review_mutation_plan_preview_output.json";

    remove_if_exists(source);
    remove_if_exists(success_plan);
    remove_if_exists(mismatch_plan);
    remove_if_exists(invalid_plan);
    remove_if_exists(output);

    featherdoc::Document source_document(source);
    REQUIRE_FALSE(source_document.create_empty());
    auto first = source_document.paragraphs();
    REQUIRE(first.has_next());
    REQUIRE(first.add_run("Alpha ").has_next());
    REQUIRE(first.add_run("Beta").has_next());
    auto second = first.insert_paragraph_after("Middle ");
    REQUIRE(second.has_next());
    REQUIRE(second.add_run("Text").has_next());
    auto third = second.insert_paragraph_after("Gamma ");
    REQUIRE(third.has_next());
    REQUIRE(third.add_run("Delta").has_next());
    REQUIRE_FALSE(source_document.save());

    write_binary_file(
        success_plan,
        R"({"operations":[)"
        R"({"kind":"delete_paragraph_text_revision","paragraph_index":1,"text_offset":0,"text_length":11,"expected_text":"Middle Text"},)"
        R"({"kind":"replace-text-range-revision","start_paragraph_index":0,"start_text_offset":6,"end_paragraph_index":2,"end_text_offset":5,"text":"Range ","expected_text":"BetaMiddle TextGamma"}]})");
    CHECK_EQ(run_cli({"preview-review-mutation-plan",
                      source.string(),
                      "--plan-file",
                      success_plan.string(),
                      "--json"},
                     output),
             0);
    auto output_json = read_text_file(output);
    CHECK_NE(output_json.find(R"("command":"preview-review-mutation-plan")"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("ok":true)"), std::string::npos);
    CHECK_NE(output_json.find(R"("operations_count":2)"), std::string::npos);
    CHECK_NE(output_json.find(R"("failed_count":0)"), std::string::npos);
    CHECK_NE(output_json.find(R"("kind":"delete_paragraph_text_revision")"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("kind":"replace_text_range_revision")"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("expected_text":"Middle Text")"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("actual_text":"Middle Text")"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("actual_text":"BetaMiddle TextGamma")"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("paragraph_index":1,"text_offset":0,"text_length":11,"text":"Middle Text")"),
             std::string::npos);

    write_binary_file(
        mismatch_plan,
        R"({"operations":[)"
        R"({"kind":"delete_text_range_revision","start_paragraph_index":0,"start_text_offset":6,"end_paragraph_index":2,"end_text_offset":5,"expected_text":"Wrong selected text"}]})");
    CHECK_EQ(run_cli({"preview-review-mutation-plan",
                      source.string(),
                      "--plan-file",
                      mismatch_plan.string(),
                      "--json"},
                     output),
             1);
    output_json = read_text_file(output);
    CHECK_NE(output_json.find(R"("ok":false)"), std::string::npos);
    CHECK_NE(output_json.find(R"("failed_count":1)"), std::string::npos);
    CHECK_NE(output_json.find("expected text did not match selected text"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("expected_text":"Wrong selected text")"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("actual_text":"BetaMiddle TextGamma")"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("preview":{)"), std::string::npos);

    write_binary_file(
        invalid_plan,
        R"({"operations":[{"kind":"replace_paragraph_text_revision","paragraph_index":0,"text_offset":0,"text_length":5}]})");
    CHECK_EQ(run_cli({"preview-review-mutation-plan",
                      source.string(),
                      "--plan-file",
                      invalid_plan.string(),
                      "--json"},
                     output),
             2);
    output_json = read_text_file(output);
    CHECK_NE(output_json.find(R"("stage":"parse")"), std::string::npos);
    CHECK_NE(output_json.find("replace_paragraph_text_revision operation must contain"),
             std::string::npos);

    remove_if_exists(source);
    remove_if_exists(success_plan);
    remove_if_exists(mismatch_plan);
    remove_if_exists(invalid_plan);
    remove_if_exists(output);
}

TEST_CASE("cli review mutation plan applies guarded revisions atomically") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_review_mutation_plan_apply_source.docx";
    const fs::path applied =
        working_directory / "cli_review_mutation_plan_apply_output.docx";
    const fs::path mismatch_output =
        working_directory / "cli_review_mutation_plan_apply_mismatch.docx";
    const fs::path overlap_output =
        working_directory / "cli_review_mutation_plan_apply_overlap.docx";
    const fs::path success_plan =
        working_directory / "cli_review_mutation_plan_apply_success.json";
    const fs::path mismatch_plan =
        working_directory / "cli_review_mutation_plan_apply_mismatch.json";
    const fs::path overlap_plan =
        working_directory / "cli_review_mutation_plan_apply_overlap.json";
    const fs::path output =
        working_directory / "cli_review_mutation_plan_apply_output.json";
    const fs::path inspect_output =
        working_directory / "cli_review_mutation_plan_apply_inspect.json";

    remove_if_exists(source);
    remove_if_exists(applied);
    remove_if_exists(mismatch_output);
    remove_if_exists(overlap_output);
    remove_if_exists(success_plan);
    remove_if_exists(mismatch_plan);
    remove_if_exists(overlap_plan);
    remove_if_exists(output);
    remove_if_exists(inspect_output);

    featherdoc::Document source_document(source);
    REQUIRE_FALSE(source_document.create_empty());
    auto first = source_document.paragraphs();
    REQUIRE(first.has_next());
    REQUIRE(first.add_run("Alpha ").has_next());
    REQUIRE(first.add_run("Beta").has_next());
    auto second = first.insert_paragraph_after("Middle ");
    REQUIRE(second.has_next());
    REQUIRE(second.add_run("Text").has_next());
    auto third = second.insert_paragraph_after("Gamma ");
    REQUIRE(third.has_next());
    REQUIRE(third.add_run("Delta").has_next());
    auto fourth = third.insert_paragraph_after("Tail ");
    REQUIRE(fourth.has_next());
    REQUIRE(fourth.add_run("End").has_next());
    REQUIRE_FALSE(source_document.save());

    write_binary_file(
        success_plan,
        R"({"operations":[)"
        R"({"kind":"replace_paragraph_text_revision","paragraph_index":0,"text_offset":6,"text_length":4,"text":"BETA","expected_text":"Beta","author":"Plan Editor","date":"2026-05-03T10:00:00Z"},)"
        R"({"kind":"delete-paragraph-text-revision","paragraph_index":1,"text_offset":0,"text_length":11,"expected_text":"Middle Text","author":"Plan Reviewer","date":"2026-05-03T10:05:00Z"},)"
        R"({"kind":"replace_text_range_revision","start_paragraph_index":2,"start_text_offset":0,"end_paragraph_index":3,"end_text_offset":4,"text":"Cross ","expected_text":"Gamma DeltaTail","author":"Plan Cross Editor","date":"2026-05-03T10:10:00Z"}]})");
    CHECK_EQ(run_cli({"apply-review-mutation-plan",
                      source.string(),
                      "--plan-file",
                      success_plan.string(),
                      "--output",
                      applied.string(),
                      "--json"},
                     output),
             0);
    auto output_json = read_text_file(output);
    CHECK_NE(output_json.find(R"("command":"apply-review-mutation-plan")"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("ok":true)"), std::string::npos);
    CHECK_NE(output_json.find(R"("operations_count":3)"), std::string::npos);
    CHECK_NE(output_json.find(R"("applied_count":3)"), std::string::npos);
    CHECK_NE(output_json.find(R"("failed_count":0)"), std::string::npos);
    CHECK(fs::exists(applied));

    CHECK_EQ(run_cli({"inspect-review", applied.string(), "--json"},
                     inspect_output),
             0);
    const auto inspect_json = read_text_file(inspect_output);
    CHECK_NE(inspect_json.find(R"("text":"Beta")"), std::string::npos);
    CHECK_NE(inspect_json.find(R"("text":"BETA")"), std::string::npos);
    CHECK_NE(inspect_json.find(R"("text":"Middle Text")"),
             std::string::npos);
    CHECK_NE(inspect_json.find(R"("text":"Gamma Delta")"),
             std::string::npos);
    CHECK_NE(inspect_json.find(R"("text":"Tail")"), std::string::npos);
    CHECK_NE(inspect_json.find(R"("text":"Cross ")"), std::string::npos);
    CHECK_NE(inspect_json.find(R"("author":"Plan Editor")"),
             std::string::npos);
    CHECK_NE(inspect_json.find(R"("author":"Plan Reviewer")"),
             std::string::npos);
    CHECK_NE(inspect_json.find(R"("author":"Plan Cross Editor")"),
             std::string::npos);

    write_binary_file(
        mismatch_plan,
        R"({"operations":[{"kind":"replace_paragraph_text_revision","paragraph_index":0,"text_offset":6,"text_length":4,"text":"BETA","expected_text":"Wrong"}]})");
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
    CHECK_NE(output_json.find(R"("failed_count":1)"), std::string::npos);
    CHECK_NE(output_json.find("expected text did not match selected text"),
             std::string::npos);
    CHECK_FALSE(fs::exists(mismatch_output));

    write_binary_file(
        overlap_plan,
        R"({"operations":[)"
        R"({"kind":"delete_paragraph_text_revision","paragraph_index":1,"text_offset":0,"text_length":11,"expected_text":"Middle Text"},)"
        R"({"kind":"replace_text_range_revision","start_paragraph_index":0,"start_text_offset":6,"end_paragraph_index":2,"end_text_offset":5,"text":"Range ","expected_text":"BetaMiddle TextGamma"}]})");
    CHECK_EQ(run_cli({"apply-review-mutation-plan",
                      source.string(),
                      "--plan-file",
                      overlap_plan.string(),
                      "--output",
                      overlap_output.string(),
                      "--json"},
                     output),
             1);
    output_json = read_text_file(output);
    CHECK_NE(output_json.find(R"("stage":"validate")"), std::string::npos);
    CHECK_NE(output_json.find("operation ranges overlap"), std::string::npos);
    CHECK_FALSE(fs::exists(overlap_output));

    remove_if_exists(source);
    remove_if_exists(applied);
    remove_if_exists(mismatch_output);
    remove_if_exists(overlap_output);
    remove_if_exists(success_plan);
    remove_if_exists(mismatch_plan);
    remove_if_exists(overlap_plan);
    remove_if_exists(output);
    remove_if_exists(inspect_output);
}

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
