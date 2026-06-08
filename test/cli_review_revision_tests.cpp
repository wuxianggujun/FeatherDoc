#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "cli_review_test_support.hpp"

namespace {

TEST_CASE("cli revision authoring appends insertion and deletion revisions") {
    const fs::path working_directory = fs::current_path();
    const fs::path source = working_directory / "cli_revision_authoring_source.docx";
    const fs::path inserted = working_directory / "cli_revision_authoring_inserted.docx";
    const fs::path deleted = working_directory / "cli_revision_authoring_deleted.docx";
    const fs::path metadata_updated =
        working_directory / "cli_revision_authoring_metadata_updated.docx";
    const fs::path metadata_cleared =
        working_directory / "cli_revision_authoring_metadata_cleared.docx";
    const fs::path output = working_directory / "cli_revision_authoring.json";
    const fs::path inspect_output = working_directory / "cli_revision_authoring_inspect.json";

    remove_if_exists(source);
    remove_if_exists(inserted);
    remove_if_exists(deleted);
    remove_if_exists(metadata_updated);
    remove_if_exists(metadata_cleared);
    remove_if_exists(output);
    remove_if_exists(inspect_output);

    create_cli_review_inspection_fixture(source);

    CHECK_EQ(run_cli({"append-insertion-revision",
                      source.string(),
                      "--text",
                      "CLI inserted revision",
                      "--author",
                      "CLI Author",
                      "--date",
                      "2026-05-02T12:00:00Z",
                      "--output",
                      inserted.string(),
                      "--json"},
                     output),
             0);
    CHECK_NE(read_text_file(output).find(R"("affected":1)"),
             std::string::npos);

    CHECK_EQ(run_cli({"append-deletion-revision",
                      inserted.string(),
                      "--text",
                      "CLI deleted revision",
                      "--author",
                      "CLI Reviewer",
                      "--date",
                      "2026-05-02T13:00:00Z",
                      "--output",
                      deleted.string(),
                      "--json"},
                     output),
             0);
    CHECK_NE(read_text_file(output).find(R"("affected":1)"),
             std::string::npos);

    CHECK_EQ(run_cli({"inspect-review", deleted.string(), "--json"},
                     inspect_output),
             0);
    const auto inspect_json = read_text_file(inspect_output);
    CHECK_NE(inspect_json.find(R"("revisions_count":4)"), std::string::npos);
    CHECK_NE(inspect_json.find(R"("kind":"insertion")"), std::string::npos);
    CHECK_NE(inspect_json.find(R"("kind":"deletion")"), std::string::npos);
    CHECK_NE(inspect_json.find(R"("author":"CLI Author")"), std::string::npos);
    CHECK_NE(inspect_json.find(R"("author":"CLI Reviewer")"), std::string::npos);
    CHECK_NE(inspect_json.find(R"("text":"CLI inserted revision")"),
             std::string::npos);
    CHECK_NE(inspect_json.find(R"("text":"CLI deleted revision")"),
             std::string::npos);

    CHECK_EQ(run_cli({"set-revision-metadata",
                      deleted.string(),
                      "2",
                      "--author",
                      "CLI Updated Author",
                      "--date",
                      "2026-05-02T12:30:00Z",
                      "--output",
                      metadata_updated.string(),
                      "--json"},
                     output),
             0);
    const auto metadata_output_json = read_text_file(output);
    CHECK_NE(metadata_output_json.find(R"("affected":1)"), std::string::npos);
    CHECK_NE(metadata_output_json.find(R"("revision_index":2)"),
             std::string::npos);

    CHECK_EQ(run_cli({"set-revision-metadata",
                      metadata_updated.string(),
                      "3",
                      "--clear-author",
                      "--date",
                      "2026-05-02T13:30:00Z",
                      "--output",
                      metadata_cleared.string(),
                      "--json"},
                     output),
             0);

    CHECK_EQ(run_cli({"inspect-review", metadata_cleared.string(), "--json"},
                     inspect_output),
             0);
    const auto metadata_inspect_json = read_text_file(inspect_output);
    CHECK_NE(metadata_inspect_json.find(R"("revisions_count":4)"),
             std::string::npos);
    CHECK_NE(metadata_inspect_json.find(R"("author":"CLI Updated Author")"),
             std::string::npos);
    CHECK_NE(metadata_inspect_json.find(R"("date":"2026-05-02T12:30:00Z")"),
             std::string::npos);
    CHECK_NE(metadata_inspect_json.find(R"("date":"2026-05-02T13:30:00Z")"),
             std::string::npos);
    CHECK_EQ(metadata_inspect_json.find(R"("author":"CLI Reviewer")"),
             std::string::npos);

    remove_if_exists(source);
    remove_if_exists(inserted);
    remove_if_exists(deleted);
    remove_if_exists(metadata_updated);
    remove_if_exists(metadata_cleared);
    remove_if_exists(output);
    remove_if_exists(inspect_output);
}

TEST_CASE("cli run revision authoring creates in-place revisions") {
    const fs::path working_directory = fs::current_path();
    const fs::path source = working_directory / "cli_run_revision_authoring_source.docx";
    const fs::path inserted = working_directory / "cli_run_revision_authoring_inserted.docx";
    const fs::path deleted = working_directory / "cli_run_revision_authoring_deleted.docx";
    const fs::path replaced = working_directory / "cli_run_revision_authoring_replaced.docx";
    const fs::path output = working_directory / "cli_run_revision_authoring.json";
    const fs::path inspect_output = working_directory / "cli_run_revision_authoring_inspect.json";

    remove_if_exists(source);
    remove_if_exists(inserted);
    remove_if_exists(deleted);
    remove_if_exists(replaced);
    remove_if_exists(output);
    remove_if_exists(inspect_output);

    featherdoc::Document source_document(source);
    REQUIRE_FALSE(source_document.create_empty());
    auto source_paragraph = source_document.paragraphs();
    REQUIRE(source_paragraph.has_next());
    REQUIRE(source_paragraph.add_run("Left ").has_next());
    REQUIRE(source_paragraph.add_run("Middle").has_next());
    REQUIRE(source_paragraph.add_run(" Right").has_next());
    REQUIRE_FALSE(source_document.save());

    CHECK_EQ(run_cli({"insert-run-revision-after",
                      source.string(),
                      "0",
                      "0",
                      "--text",
                      "CLI in-place insertion",
                      "--author",
                      "CLI Run Author",
                      "--date",
                      "2026-05-02T17:00:00Z",
                      "--output",
                      inserted.string(),
                      "--json"},
                     output),
             0);
    CHECK_NE(read_text_file(output).find(R"("paragraph_index":0)"),
             std::string::npos);
    CHECK_NE(read_text_file(output).find(R"("run_index":0)"),
             std::string::npos);

    CHECK_EQ(run_cli({"delete-run-revision",
                      inserted.string(),
                      "0",
                      "1",
                      "--author",
                      "CLI Run Reviewer",
                      "--date",
                      "2026-05-02T18:00:00Z",
                      "--output",
                      deleted.string(),
                      "--json"},
                     output),
             0);

    CHECK_EQ(run_cli({"replace-run-revision",
                      deleted.string(),
                      "0",
                      "1",
                      "--text",
                      "CLI replacement",
                      "--author",
                      "CLI Run Editor",
                      "--date",
                      "2026-05-02T19:00:00Z",
                      "--output",
                      replaced.string(),
                      "--json"},
                     output),
             0);

    CHECK_EQ(run_cli({"inspect-review", replaced.string(), "--json"},
                     inspect_output),
             0);
    const auto inspect_json = read_text_file(inspect_output);
    CHECK_NE(inspect_json.find(R"("revisions_count":4)"), std::string::npos);
    CHECK_NE(inspect_json.find(R"("text":"CLI in-place insertion")"),
             std::string::npos);
    CHECK_NE(inspect_json.find(R"("text":"Middle")"), std::string::npos);
    CHECK_NE(inspect_json.find(R"("text":" Right")"), std::string::npos);
    CHECK_NE(inspect_json.find(R"("text":"CLI replacement")"),
             std::string::npos);
    CHECK_NE(inspect_json.find(R"("author":"CLI Run Author")"),
             std::string::npos);
    CHECK_NE(inspect_json.find(R"("author":"CLI Run Reviewer")"),
             std::string::npos);
    CHECK_NE(inspect_json.find(R"("author":"CLI Run Editor")"),
             std::string::npos);

    remove_if_exists(source);
    remove_if_exists(inserted);
    remove_if_exists(deleted);
    remove_if_exists(replaced);
    remove_if_exists(output);
    remove_if_exists(inspect_output);
}

TEST_CASE("cli paragraph text revision authoring creates range revisions") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_paragraph_text_revision_authoring_source.docx";
    const fs::path inserted =
        working_directory / "cli_paragraph_text_revision_authoring_inserted.docx";
    const fs::path deleted =
        working_directory / "cli_paragraph_text_revision_authoring_deleted.docx";
    const fs::path replaced =
        working_directory / "cli_paragraph_text_revision_authoring_replaced.docx";
    const fs::path output =
        working_directory / "cli_paragraph_text_revision_authoring.json";
    const fs::path inspect_output =
        working_directory / "cli_paragraph_text_revision_authoring_inspect.json";

    remove_if_exists(source);
    remove_if_exists(inserted);
    remove_if_exists(deleted);
    remove_if_exists(replaced);
    remove_if_exists(output);
    remove_if_exists(inspect_output);

    featherdoc::Document source_document(source);
    REQUIRE_FALSE(source_document.create_empty());
    auto source_paragraph = source_document.paragraphs();
    REQUIRE(source_paragraph.has_next());
    REQUIRE(source_paragraph.add_run("Left ").has_next());
    REQUIRE(source_paragraph.add_run("Middle").has_next());
    REQUIRE(source_paragraph.add_run(" Right").has_next());
    REQUIRE_FALSE(source_document.save());

    CHECK_EQ(run_cli({"insert-paragraph-text-revision",
                      source.string(),
                      "0",
                      "5",
                      "--text",
                      "CLI range insertion ",
                      "--author",
                      "CLI Range Author",
                      "--date",
                      "2026-05-02T20:00:00Z",
                      "--output",
                      inserted.string(),
                      "--json"},
                     output),
             0);
    auto output_json = read_text_file(output);
    CHECK_NE(output_json.find(R"("paragraph_index":0)"), std::string::npos);
    CHECK_NE(output_json.find(R"("text_offset":5)"), std::string::npos);

    CHECK_EQ(run_cli({"insert-paragraph-text-revision",
                      source.string(),
                      "0",
                      "5",
                      "--text",
                      "CLI range insertion ",
                      "--expected-text",
                      "Middle",
                      "--json"},
                     output),
             2);
    output_json = read_text_file(output);
    CHECK_NE(output_json.find("insert-paragraph-text-revision does not accept --expected-text"),
             std::string::npos);

    CHECK_EQ(run_cli({"delete-paragraph-text-revision",
                      inserted.string(),
                      "0",
                      "5",
                      "6",
                      "--expected-text",
                      "Middle",
                      "--expected-text",
                      "Middle",
                      "--json"},
                     output),
             2);
    output_json = read_text_file(output);
    CHECK_NE(output_json.find("duplicate --expected-text option"),
             std::string::npos);

    CHECK_EQ(run_cli({"delete-paragraph-text-revision",
                      inserted.string(),
                      "0",
                      "5",
                      "6",
                      "--expected-text"},
                     output),
             2);
    output_json = read_text_file(output);
    CHECK_NE(output_json.find("missing value after --expected-text"),
             std::string::npos);

    CHECK_EQ(run_cli({"delete-paragraph-text-revision",
                      inserted.string(),
                      "0",
                      "5",
                      "6",
                      "--expected-text",
                      "Wrong paragraph text",
                      "--output",
                      deleted.string(),
                     "--json"},
                     output),
             1);
    output_json = read_text_file(output);
    CHECK_NE(output_json.find(R"("stage":"validate")"), std::string::npos);
    CHECK_NE(output_json.find(R"("expected_text":"Wrong paragraph text")"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("actual_text":"Middle")"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("preview":{)"), std::string::npos);
    CHECK_NE(output_json.find(R"("start_paragraph_index":0)"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("text_length":6)"), std::string::npos);
    CHECK_NE(output_json.find(R"("paragraph_index":0,"text_offset":5,"text_length":6,"text":"Middle")"),
             std::string::npos);

    CHECK_EQ(run_cli({"delete-paragraph-text-revision",
                      inserted.string(),
                      "0",
                      "5",
                      "6",
                      "--expected-text",
                      "Middle",
                      "--author",
                      "CLI Range Reviewer",
                      "--date",
                      "2026-05-02T21:00:00Z",
                      "--output",
                      deleted.string(),
                      "--json"},
                     output),
             0);
    output_json = read_text_file(output);
    CHECK_NE(output_json.find(R"("text_length":6)"), std::string::npos);

    CHECK_EQ(run_cli({"replace-paragraph-text-revision",
                      deleted.string(),
                      "0",
                      "5",
                      "6",
                      "--text",
                      "CLI range replacement",
                      "--author",
                      "CLI Range Editor",
                      "--date",
                      "2026-05-02T22:00:00Z",
                      "--output",
                      replaced.string(),
                      "--json"},
                     output),
             0);

    CHECK_EQ(run_cli({"inspect-review", replaced.string(), "--json"},
                     inspect_output),
             0);
    const auto inspect_json = read_text_file(inspect_output);
    CHECK_NE(inspect_json.find(R"("revisions_count":4)"), std::string::npos);
    CHECK_NE(inspect_json.find(R"("text":"CLI range insertion ")"),
             std::string::npos);
    CHECK_NE(inspect_json.find(R"("text":"Middle")"), std::string::npos);
    CHECK_NE(inspect_json.find(R"("text":" Right")"), std::string::npos);
    CHECK_NE(inspect_json.find(R"("text":"CLI range replacement")"),
             std::string::npos);
    CHECK_NE(inspect_json.find(R"("author":"CLI Range Author")"),
             std::string::npos);
    CHECK_NE(inspect_json.find(R"("author":"CLI Range Reviewer")"),
             std::string::npos);
    CHECK_NE(inspect_json.find(R"("author":"CLI Range Editor")"),
             std::string::npos);

    remove_if_exists(source);
    remove_if_exists(inserted);
    remove_if_exists(deleted);
    remove_if_exists(replaced);
    remove_if_exists(output);
    remove_if_exists(inspect_output);
}

TEST_CASE("cli text range revision authoring creates cross-paragraph revisions") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_text_range_revision_authoring_source.docx";
    const fs::path inserted =
        working_directory / "cli_text_range_revision_authoring_inserted.docx";
    const fs::path deleted =
        working_directory / "cli_text_range_revision_authoring_deleted.docx";
    const fs::path replaced =
        working_directory / "cli_text_range_revision_authoring_replaced.docx";
    const fs::path output =
        working_directory / "cli_text_range_revision_authoring.json";
    const fs::path inspect_output =
        working_directory / "cli_text_range_revision_authoring_inspect.json";

    remove_if_exists(source);
    remove_if_exists(inserted);
    remove_if_exists(deleted);
    remove_if_exists(replaced);
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
    REQUIRE_FALSE(source_document.save());

    CHECK_EQ(run_cli({"preview-text-range",
                      source.string(),
                      "0",
                      "6",
                      "2",
                      "5",
                      "--json"},
                     output),
             0);
    auto output_json = read_text_file(output);
    CHECK_NE(output_json.find(R"("command":"preview-text-range")"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("start_paragraph_index":0)"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("end_paragraph_index":2)"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("text_length":20)"), std::string::npos);
    CHECK_NE(output_json.find(R"("plain_text_runs_supported":true)"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("text":"BetaMiddle TextGamma")"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("paragraph_index":1,"text_offset":0,"text_length":11,"text":"Middle Text")"),
             std::string::npos);

    CHECK_EQ(run_cli({"preview-text-range",
                      source.string(),
                      "2",
                      "0",
                      "1",
                      "1",
                      "--json"},
                     output),
             1);
    output_json = read_text_file(output);
    CHECK_NE(output_json.find(R"("stage":"preview")"), std::string::npos);
    CHECK_NE(output_json.find("text range start must not be after end"),
             std::string::npos);

    CHECK_EQ(run_cli({"insert-text-range-revision",
                      source.string(),
                      "1",
                      "7",
                      "--text",
                      "CLI cross insertion ",
                      "--expected-text",
                      "Text",
                      "--json"},
                     output),
             2);
    output_json = read_text_file(output);
    CHECK_NE(output_json.find("insert-text-range-revision does not accept --expected-text"),
             std::string::npos);

    CHECK_EQ(run_cli({"insert-text-range-revision",
                      source.string(),
                      "1",
                      "7",
                      "--text",
                      "CLI cross insertion ",
                      "--author",
                      "CLI Cross Author",
                      "--date",
                      "2026-05-03T02:00:00Z",
                      "--output",
                      inserted.string(),
                      "--json"},
                     output),
             0);
    output_json = read_text_file(output);
    CHECK_NE(output_json.find(R"("start_paragraph_index":1)"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("start_text_offset":7)"),
             std::string::npos);

    CHECK_EQ(run_cli({"delete-text-range-revision",
                      source.string(),
                      "0",
                      "6",
                      "2",
                      "5",
                      "--expected-text",
                      "Wrong selected text",
                      "--output",
                      deleted.string(),
                      "--json"},
                     output),
             1);
    output_json = read_text_file(output);
    CHECK_NE(output_json.find(R"("stage":"validate")"), std::string::npos);
    CHECK_NE(output_json.find(R"("expected_text":"Wrong selected text")"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("actual_text":"BetaMiddle TextGamma")"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("preview":{)"), std::string::npos);
    CHECK_NE(output_json.find(R"("start_paragraph_index":0)"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("end_paragraph_index":2)"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("paragraph_index":1,"text_offset":0,"text_length":11,"text":"Middle Text")"),
             std::string::npos);

    CHECK_EQ(run_cli({"delete-text-range-revision",
                      source.string(),
                      "0",
                      "6",
                      "2",
                      "5",
                      "--expected-text",
                      "BetaMiddle TextGamma",
                      "--author",
                      "CLI Cross Reviewer",
                      "--date",
                      "2026-05-03T03:00:00Z",
                      "--output",
                      deleted.string(),
                      "--json"},
                     output),
             0);
    output_json = read_text_file(output);
    CHECK_NE(output_json.find(R"("end_paragraph_index":2)"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("end_text_offset":5)"),
             std::string::npos);

    CHECK_EQ(run_cli({"replace-text-range-revision",
                      source.string(),
                      "0",
                      "6",
                      "2",
                      "5",
                      "--text",
                      "CLI cross replacement ",
                      "--expected-text",
                      "BetaMiddle TextGamma",
                      "--author",
                      "CLI Cross Editor",
                      "--date",
                      "2026-05-03T04:00:00Z",
                      "--output",
                      replaced.string(),
                      "--json"},
                     output),
             0);

    CHECK_EQ(run_cli({"inspect-review", replaced.string(), "--json"},
                     inspect_output),
             0);
    const auto inspect_json = read_text_file(inspect_output);
    CHECK_NE(inspect_json.find(R"("revisions_count":4)"), std::string::npos);
    CHECK_NE(inspect_json.find(R"("text":"Beta")"), std::string::npos);
    CHECK_NE(inspect_json.find(R"("text":"CLI cross replacement ")"),
             std::string::npos);
    CHECK_NE(inspect_json.find(R"("text":"Middle Text")"), std::string::npos);
    CHECK_NE(inspect_json.find(R"("text":"Gamma")"), std::string::npos);
    CHECK_NE(inspect_json.find(R"("author":"CLI Cross Editor")"),
             std::string::npos);

    remove_if_exists(source);
    remove_if_exists(inserted);
    remove_if_exists(deleted);
    remove_if_exists(replaced);
    remove_if_exists(output);
    remove_if_exists(inspect_output);
}

TEST_CASE("cli find text ranges reports paragraph offsets") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_find_text_ranges_source.docx";
    const fs::path output =
        working_directory / "cli_find_text_ranges_output.json";

    remove_if_exists(source);
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
    auto third = second.insert_paragraph_after("Beta ");
    REQUIRE(third.has_next());
    REQUIRE(third.add_run("Tail").has_next());
    REQUIRE_FALSE(source_document.save());

    CHECK_EQ(run_cli({"find-text-ranges",
                      source.string(),
                      "--text",
                      "Beta",
                      "--json"},
                     output),
             0);
    auto output_json = read_text_file(output);
    CHECK_NE(output_json.find(R"("command":"find-text-ranges")"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("query":"Beta")"), std::string::npos);
    CHECK_NE(output_json.find(R"("matches_count":2)"), std::string::npos);
    CHECK_NE(output_json.find(R"("start_paragraph_index":0)"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("start_text_offset":6)"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("start_paragraph_index":2)"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("start_text_offset":0)"),
             std::string::npos);

    CHECK_EQ(run_cli({"find-text-ranges",
                      source.string(),
                      "--text",
                      "BetaMiddle TextBeta",
                      "--json"},
                     output),
             0);
    output_json = read_text_file(output);
    CHECK_NE(output_json.find(R"("matches_count":1)"), std::string::npos);
    CHECK_NE(output_json.find(R"("text":"BetaMiddle TextBeta")"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("end_paragraph_index":2)"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("end_text_offset":4)"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("paragraph_index":1,"text_offset":0,"text_length":11,"text":"Middle Text")"),
             std::string::npos);

    CHECK_EQ(run_cli({"find-text-ranges", source.string(), "--json"}, output),
             2);
    output_json = read_text_file(output);
    CHECK_NE(output_json.find("missing --text"), std::string::npos);

    CHECK_EQ(run_cli({"find-text-ranges",
                      source.string(),
                      "--text",
                      "",
                      "--json"},
                     output),
             1);
    output_json = read_text_file(output);
    CHECK_NE(output_json.find(R"("stage":"find")"), std::string::npos);
    CHECK_NE(output_json.find("search text must not be empty"),
             std::string::npos);

    remove_if_exists(source);
    remove_if_exists(output);
}

TEST_CASE("cli run-recipe executes batch replace") {
    const fs::path working_directory = fs::current_path();
    const fs::path source_dir =
        working_directory / "cli_run_recipe_batch_replace_source";
    const fs::path output_dir =
        working_directory / "cli_run_recipe_batch_replace_output";
    const fs::path source = source_dir / "input.docx";
    const fs::path replaced = output_dir / "input_replaced.docx";
    const fs::path recipe =
        working_directory / "cli_run_recipe_batch_replace_recipe.json";
    const fs::path inputs =
        working_directory / "cli_run_recipe_batch_replace_inputs.json";
    const fs::path output =
        working_directory / "cli_run_recipe_batch_replace_result.json";
    const fs::path find_output =
        working_directory / "cli_run_recipe_batch_replace_find.json";

    std::error_code cleanup_error;
    fs::remove_all(source_dir, cleanup_error);
    fs::remove_all(output_dir, cleanup_error);
    remove_if_exists(recipe);
    remove_if_exists(inputs);
    remove_if_exists(output);
    remove_if_exists(find_output);

    REQUIRE(fs::create_directories(source_dir));

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

    write_binary_file(recipe, R"({"id":"batch_replace"})");
    write_binary_file(
        inputs,
        R"({"inputs":{"source_dir":")" + json_escape_text(source_dir.string()) +
            R"(","find_text":"Beta","replace_text":"Omega"}})");

    CHECK_EQ(run_cli({"run-recipe",
                      "--recipe",
                      recipe.string(),
                      "--inputs",
                      inputs.string(),
                      "--output",
                      output_dir.string(),
                      "--json"},
                     output),
             0);

    const auto output_json = read_text_file(output);
    CHECK_NE(output_json.find(R"("command":"run-recipe")"), std::string::npos);
    CHECK_NE(output_json.find(R"("ok":true)"), std::string::npos);
    CHECK_NE(output_json.find(R"("recipe_id":"batch_replace")"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("documents_count":1)"), std::string::npos);
    CHECK_NE(output_json.find(R"("changed_count":1)"), std::string::npos);
    CHECK_NE(output_json.find(R"("replacements_count":2)"),
             std::string::npos);
    CHECK(fs::exists(replaced));

    CHECK_EQ(run_cli({"find-text-ranges",
                      replaced.string(),
                      "--text",
                      "Omega",
                      "--json"},
                     find_output),
             0);
    const auto find_json = read_text_file(find_output);
    CHECK_NE(find_json.find(R"("matches_count":2)"), std::string::npos);

    fs::remove_all(source_dir, cleanup_error);
    fs::remove_all(output_dir, cleanup_error);
    remove_if_exists(recipe);
    remove_if_exists(inputs);
    remove_if_exists(output);
    remove_if_exists(find_output);
}

TEST_CASE("cli revision mutation accepts and rejects revisions") {
    const fs::path working_directory = fs::current_path();
    const fs::path source = working_directory / "cli_revision_mutation_source.docx";
    const fs::path accepted = working_directory / "cli_revision_mutation_accepted.docx";
    const fs::path clean = working_directory / "cli_revision_mutation_clean.docx";
    const fs::path output = working_directory / "cli_revision_mutation.json";
    const fs::path inspect_output = working_directory / "cli_revision_mutation_inspect.json";

    remove_if_exists(source);
    remove_if_exists(accepted);
    remove_if_exists(clean);
    remove_if_exists(output);
    remove_if_exists(inspect_output);

    create_cli_review_inspection_fixture(source);

    CHECK_EQ(run_cli({"accept-revision",
                      source.string(),
                      "0",
                      "--output",
                      accepted.string(),
                      "--json"},
                     output),
             0);
    CHECK_NE(read_text_file(output).find(R"("affected":1)"),
             std::string::npos);

    CHECK_EQ(run_cli({"inspect-review", accepted.string(), "--json"},
                     inspect_output),
             0);
    CHECK_NE(read_text_file(inspect_output).find(R"("revisions_count":1)"),
             std::string::npos);

    CHECK_EQ(run_cli({"reject-all-revisions",
                      accepted.string(),
                      "--output",
                      clean.string(),
                      "--json"},
                     output),
             0);
    CHECK_NE(read_text_file(output).find(R"("affected":1)"),
             std::string::npos);

    CHECK_EQ(run_cli({"inspect-review", clean.string(), "--json"},
                     inspect_output),
             0);
    CHECK_NE(read_text_file(inspect_output).find(R"("revisions_count":0)"),
             std::string::npos);

    remove_if_exists(source);
    remove_if_exists(accepted);
    remove_if_exists(clean);
    remove_if_exists(output);
    remove_if_exists(inspect_output);
}

} // namespace
