#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "cli_review_test_support.hpp"

namespace {
TEST_CASE("cli inspect-hyperlinks lists document hyperlinks") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_inspect_hyperlinks_source.docx";
    const fs::path text_output =
        working_directory / "cli_inspect_hyperlinks.txt";
    const fs::path json_output =
        working_directory / "cli_inspect_hyperlinks.json";

    remove_if_exists(source);
    remove_if_exists(text_output);
    remove_if_exists(json_output);

    create_cli_review_inspection_fixture(source);

    CHECK_EQ(run_cli({"inspect-hyperlinks", source.string()}, text_output), 0);
    const auto text = read_text_file(text_output);
    CHECK_NE(text.find("hyperlinks: 1"), std::string::npos);
    CHECK_NE(text.find("Open docs"), std::string::npos);
    CHECK_NE(text.find("https://example.com/docs"), std::string::npos);
    CHECK_NE(text.find("external=yes"), std::string::npos);

    CHECK_EQ(run_cli({"inspect-hyperlinks", source.string(), "--json"},
                     json_output),
             0);
    const auto json = read_text_file(json_output);
    CHECK_NE(json.find(R"("count":1)"), std::string::npos);
    CHECK_NE(json.find(R"("text":"Open docs")"), std::string::npos);
    CHECK_NE(json.find(R"("relationship_id":"rHyperlink")"),
             std::string::npos);
    CHECK_NE(json.find(R"("target":"https://example.com/docs")"),
             std::string::npos);
    CHECK_NE(json.find(R"("external":true)"), std::string::npos);

    remove_if_exists(source);
    remove_if_exists(text_output);
    remove_if_exists(json_output);
}

TEST_CASE("cli inspect-review lists notes comments and revisions") {
    const fs::path working_directory = fs::current_path();
    const fs::path source = working_directory / "cli_inspect_review_source.docx";
    const fs::path text_output = working_directory / "cli_inspect_review.txt";
    const fs::path json_output = working_directory / "cli_inspect_review.json";

    remove_if_exists(source);
    remove_if_exists(text_output);
    remove_if_exists(json_output);

    create_cli_review_inspection_fixture(source);

    CHECK_EQ(run_cli({"inspect-review", source.string()}, text_output), 0);
    const auto text = read_text_file(text_output);
    CHECK_NE(text.find("footnotes: 1"), std::string::npos);
    CHECK_NE(text.find("Footnote body"), std::string::npos);
    CHECK_NE(text.find("endnotes: 1"), std::string::npos);
    CHECK_NE(text.find("Endnote body"), std::string::npos);
    CHECK_NE(text.find("comments: 1"), std::string::npos);
    CHECK_NE(text.find("Reviewer note"), std::string::npos);
    CHECK_NE(text.find("anchor_text=Commented text"), std::string::npos);
    CHECK_NE(text.find("revisions: 2"), std::string::npos);
    CHECK_NE(text.find("kind=insertion"), std::string::npos);
    CHECK_NE(text.find("kind=deletion"), std::string::npos);

    CHECK_EQ(run_cli({"inspect-review", source.string(), "--json"},
                     json_output),
             0);
    const auto json = read_text_file(json_output);
    CHECK_NE(json.find(R"("footnotes_count":1)"), std::string::npos);
    CHECK_NE(json.find(R"("endnotes_count":1)"), std::string::npos);
    CHECK_NE(json.find(R"("comments_count":1)"), std::string::npos);
    CHECK_NE(json.find(R"("revisions_count":2)"), std::string::npos);
    CHECK_NE(json.find(R"("kind":"comment")"), std::string::npos);
    CHECK_NE(json.find(R"("author":"Reviewer")"), std::string::npos);
    CHECK_NE(json.find(R"("anchor_text":"Commented text")"),
             std::string::npos);
    CHECK_NE(json.find(R"("text":"Inserted review text")"),
             std::string::npos);
    CHECK_NE(json.find(R"("text":"Deleted review text")"),
             std::string::npos);

    remove_if_exists(source);
    remove_if_exists(text_output);
    remove_if_exists(json_output);
}

TEST_CASE("cli inspect-omml lists inline and display formulas") {
    const fs::path working_directory = fs::current_path();
    const fs::path source = working_directory / "cli_inspect_omml_source.docx";
    const fs::path text_output = working_directory / "cli_inspect_omml.txt";
    const fs::path json_output = working_directory / "cli_inspect_omml.json";

    remove_if_exists(source);
    remove_if_exists(text_output);
    remove_if_exists(json_output);

    create_cli_review_inspection_fixture(source);

    CHECK_EQ(run_cli({"inspect-omml", source.string()}, text_output), 0);
    const auto text = read_text_file(text_output);
    CHECK_NE(text.find("formulas: 2"), std::string::npos);
    CHECK_NE(text.find("display=no"), std::string::npos);
    CHECK_NE(text.find("display=yes"), std::string::npos);
    CHECK_NE(text.find("x+1"), std::string::npos);
    CHECK_NE(text.find("y=2"), std::string::npos);

    CHECK_EQ(run_cli({"inspect-omml", source.string(), "--json"}, json_output),
             0);
    const auto json = read_text_file(json_output);
    CHECK_NE(json.find(R"("count":2)"), std::string::npos);
    CHECK_NE(json.find(R"("display":false)"), std::string::npos);
    CHECK_NE(json.find(R"("display":true)"), std::string::npos);
    CHECK_NE(json.find(R"("text":"x+1")"), std::string::npos);
    CHECK_NE(json.find(R"("text":"y=2")"), std::string::npos);
    CHECK_NE(json.find("<m:oMath"), std::string::npos);

    remove_if_exists(source);
    remove_if_exists(text_output);
    remove_if_exists(json_output);
}

TEST_CASE("cli hyperlink mutation appends replaces and removes links") {
    const fs::path working_directory = fs::current_path();
    const fs::path source = working_directory / "cli_hyperlink_mutation_source.docx";
    const fs::path appended = working_directory / "cli_hyperlink_mutation_appended.docx";
    const fs::path replaced = working_directory / "cli_hyperlink_mutation_replaced.docx";
    const fs::path removed = working_directory / "cli_hyperlink_mutation_removed.docx";
    const fs::path output = working_directory / "cli_hyperlink_mutation.json";
    const fs::path inspect_output = working_directory / "cli_hyperlink_mutation_inspect.json";

    remove_if_exists(source);
    remove_if_exists(appended);
    remove_if_exists(replaced);
    remove_if_exists(removed);
    remove_if_exists(output);
    remove_if_exists(inspect_output);

    create_cli_review_inspection_fixture(source);

    CHECK_EQ(run_cli({"append-hyperlink",
                      source.string(),
                      "--text",
                      "Added link",
                      "--target",
                      "https://example.com/added",
                      "--output",
                      appended.string(),
                      "--json"},
                     output),
             0);
    CHECK_NE(read_text_file(output).find(R"("affected":1)"),
             std::string::npos);

    CHECK_EQ(run_cli({"replace-hyperlink",
                      appended.string(),
                      "0",
                      "--text",
                      "Changed docs",
                      "--target",
                      "https://example.com/changed",
                      "--output",
                      replaced.string(),
                      "--json"},
                     output),
             0);
    CHECK_NE(read_text_file(output).find(R"("command":"replace-hyperlink")"),
             std::string::npos);

    CHECK_EQ(run_cli({"remove-hyperlink",
                      replaced.string(),
                      "1",
                      "--output",
                      removed.string(),
                      "--json"},
                     output),
             0);

    CHECK_EQ(run_cli({"inspect-hyperlinks", removed.string(), "--json"},
                     inspect_output),
             0);
    const auto inspect_json = read_text_file(inspect_output);
    CHECK_NE(inspect_json.find(R"("count":1)"), std::string::npos);
    CHECK_NE(inspect_json.find(R"("text":"Changed docs")"),
             std::string::npos);
    CHECK_NE(inspect_json.find(R"("target":"https://example.com/changed")"),
             std::string::npos);
    CHECK_EQ(inspect_json.find("https://example.com/added"), std::string::npos);

    remove_if_exists(source);
    remove_if_exists(appended);
    remove_if_exists(replaced);
    remove_if_exists(removed);
    remove_if_exists(output);
    remove_if_exists(inspect_output);
}

TEST_CASE("cli omml mutation appends replaces and removes formulas") {
    const fs::path working_directory = fs::current_path();
    const fs::path source = working_directory / "cli_omml_mutation_source.docx";
    const fs::path appended = working_directory / "cli_omml_mutation_appended.docx";
    const fs::path replaced = working_directory / "cli_omml_mutation_replaced.docx";
    const fs::path removed = working_directory / "cli_omml_mutation_removed.docx";
    const fs::path output = working_directory / "cli_omml_mutation.json";
    const fs::path inspect_output = working_directory / "cli_omml_mutation_inspect.json";

    remove_if_exists(source);
    remove_if_exists(appended);
    remove_if_exists(replaced);
    remove_if_exists(removed);
    remove_if_exists(output);
    remove_if_exists(inspect_output);

    create_cli_review_inspection_fixture(source);

    const std::string appended_xml =
        R"(<m:oMath xmlns:m="http://schemas.openxmlformats.org/officeDocument/2006/math"><m:r><m:t>z=3</m:t></m:r></m:oMath>)";
    const std::string replacement_xml =
        R"(<m:oMath xmlns:m="http://schemas.openxmlformats.org/officeDocument/2006/math"><m:r><m:t>q=4</m:t></m:r></m:oMath>)";

    CHECK_EQ(run_cli({"append-omml",
                      source.string(),
                      "--xml",
                      appended_xml,
                      "--output",
                      appended.string(),
                      "--json"},
                     output),
             0);
    CHECK_NE(read_text_file(output).find(R"("affected":1)"),
             std::string::npos);

    CHECK_EQ(run_cli({"replace-omml",
                      appended.string(),
                      "0",
                      "--xml",
                      replacement_xml,
                      "--output",
                      replaced.string(),
                      "--json"},
                     output),
             0);

    CHECK_EQ(run_cli({"remove-omml",
                      replaced.string(),
                      "1",
                      "--output",
                      removed.string(),
                      "--json"},
                     output),
             0);

    CHECK_EQ(run_cli({"inspect-omml", removed.string(), "--json"},
                     inspect_output),
             0);
    const auto inspect_json = read_text_file(inspect_output);
    CHECK_NE(inspect_json.find(R"("count":2)"), std::string::npos);
    CHECK_NE(inspect_json.find(R"("text":"q=4")"), std::string::npos);
    CHECK_NE(inspect_json.find(R"("text":"z=3")"), std::string::npos);
    CHECK_EQ(inspect_json.find(R"("text":"y=2")"), std::string::npos);

    remove_if_exists(source);
    remove_if_exists(appended);
    remove_if_exists(replaced);
    remove_if_exists(removed);
    remove_if_exists(output);
    remove_if_exists(inspect_output);
}

TEST_CASE("cli review note mutation updates notes and comments") {
    const fs::path working_directory = fs::current_path();
    const fs::path source = working_directory / "cli_review_note_mutation_source.docx";
    const fs::path footnote_added = working_directory / "cli_review_note_footnote_added.docx";
    const fs::path footnote_replaced = working_directory / "cli_review_note_footnote_replaced.docx";
    const fs::path footnote_removed = working_directory / "cli_review_note_footnote_removed.docx";
    const fs::path endnote_added = working_directory / "cli_review_note_endnote_added.docx";
    const fs::path endnote_replaced = working_directory / "cli_review_note_endnote_replaced.docx";
    const fs::path endnote_removed = working_directory / "cli_review_note_endnote_removed.docx";
    const fs::path comment_added = working_directory / "cli_review_note_comment_added.docx";
    const fs::path comment_replaced = working_directory / "cli_review_note_comment_replaced.docx";
    const fs::path final_doc = working_directory / "cli_review_note_final.docx";
    const fs::path output = working_directory / "cli_review_note_mutation.json";
    const fs::path inspect_output = working_directory / "cli_review_note_inspect.json";

    remove_if_exists(source);
    remove_if_exists(footnote_added);
    remove_if_exists(footnote_replaced);
    remove_if_exists(footnote_removed);
    remove_if_exists(endnote_added);
    remove_if_exists(endnote_replaced);
    remove_if_exists(endnote_removed);
    remove_if_exists(comment_added);
    remove_if_exists(comment_replaced);
    remove_if_exists(final_doc);
    remove_if_exists(output);
    remove_if_exists(inspect_output);

    create_cli_review_inspection_fixture(source);

    CHECK_EQ(run_cli({"append-footnote",
                      source.string(),
                      "--reference-text",
                      "Added footnote mark ",
                      "--note-text",
                      "Added footnote body",
                      "--output",
                      footnote_added.string(),
                      "--json"},
                     output),
             0);
    CHECK_EQ(run_cli({"replace-footnote",
                      footnote_added.string(),
                      "0",
                      "--note-text",
                      "Changed footnote body",
                      "--output",
                      footnote_replaced.string(),
                      "--json"},
                     output),
             0);
    CHECK_EQ(run_cli({"remove-footnote",
                      footnote_replaced.string(),
                      "1",
                      "--output",
                      footnote_removed.string(),
                      "--json"},
                     output),
             0);

    CHECK_EQ(run_cli({"append-endnote",
                      footnote_removed.string(),
                      "--reference-text",
                      "Added endnote mark ",
                      "--note-text",
                      "Added endnote body",
                      "--output",
                      endnote_added.string(),
                      "--json"},
                     output),
             0);
    CHECK_EQ(run_cli({"replace-endnote",
                      endnote_added.string(),
                      "0",
                      "--note-text",
                      "Changed endnote body",
                      "--output",
                      endnote_replaced.string(),
                      "--json"},
                     output),
             0);
    CHECK_EQ(run_cli({"remove-endnote",
                      endnote_replaced.string(),
                      "1",
                      "--output",
                      endnote_removed.string(),
                      "--json"},
                     output),
             0);

    CHECK_EQ(run_cli({"append-comment",
                      endnote_removed.string(),
                      "--selected-text",
                      "New commented text",
                      "--comment-text",
                      "Added comment body",
                      "--author",
                      "CLI Reviewer",
                      "--initials",
                      "CR",
                      "--date",
                      "2026-05-02T11:00:00Z",
                      "--output",
                      comment_added.string(),
                      "--json"},
                     output),
             0);
    CHECK_EQ(run_cli({"inspect-review", comment_added.string(), "--json"},
                     inspect_output),
             0);
    const auto appended_comment_json = read_text_file(inspect_output);
    CHECK_NE(appended_comment_json.find(R"("comments_count":2)"),
             std::string::npos);
    CHECK_NE(appended_comment_json.find(R"("anchor_text":"New commented text")"),
             std::string::npos);
    CHECK_NE(appended_comment_json.find(R"("text":"Added comment body")"),
             std::string::npos);
    CHECK_NE(appended_comment_json.find(R"("date":"2026-05-02T11:00:00Z")"),
             std::string::npos);
    CHECK_EQ(run_cli({"replace-comment",
                      comment_added.string(),
                      "0",
                      "--comment-text",
                      "Changed comment body",
                      "--output",
                      comment_replaced.string(),
                      "--json"},
                     output),
             0);
    CHECK_EQ(run_cli({"remove-comment",
                      comment_replaced.string(),
                      "1",
                      "--output",
                      final_doc.string(),
                      "--json"},
                     output),
             0);

    CHECK_EQ(run_cli({"inspect-review", final_doc.string(), "--json"},
                     inspect_output),
             0);
    const auto inspect_json = read_text_file(inspect_output);
    CHECK_NE(inspect_json.find(R"("footnotes_count":1)"), std::string::npos);
    CHECK_NE(inspect_json.find(R"("endnotes_count":1)"), std::string::npos);
    CHECK_NE(inspect_json.find(R"("comments_count":1)"), std::string::npos);
    CHECK_NE(inspect_json.find(R"("text":"Changed footnote body")"),
             std::string::npos);
    CHECK_NE(inspect_json.find(R"("text":"Changed endnote body")"),
             std::string::npos);
    CHECK_NE(inspect_json.find(R"("anchor_text":"Commented text")"),
             std::string::npos);
    CHECK_NE(inspect_json.find(R"("text":"Changed comment body")"),
             std::string::npos);
    CHECK_EQ(inspect_json.find("Added footnote body"), std::string::npos);
    CHECK_EQ(inspect_json.find("Added endnote body"), std::string::npos);
    CHECK_EQ(inspect_json.find("Added comment body"), std::string::npos);

    remove_if_exists(source);
    remove_if_exists(footnote_added);
    remove_if_exists(footnote_replaced);
    remove_if_exists(footnote_removed);
    remove_if_exists(endnote_added);
    remove_if_exists(endnote_replaced);
    remove_if_exists(endnote_removed);
    remove_if_exists(comment_added);
    remove_if_exists(comment_replaced);
    remove_if_exists(final_doc);
    remove_if_exists(output);
    remove_if_exists(inspect_output);
}

TEST_CASE("cli comment range authoring creates in-place comments") {
    const fs::path working_directory = fs::current_path();
    const fs::path source = working_directory / "cli_comment_range_source.docx";
    const fs::path paragraph_comment =
        working_directory / "cli_comment_range_paragraph.docx";
    const fs::path cross_comment =
        working_directory / "cli_comment_range_cross.docx";
    const fs::path paragraph_moved =
        working_directory / "cli_comment_range_paragraph_moved.docx";
    const fs::path range_moved =
        working_directory / "cli_comment_range_moved.docx";
    const fs::path resolved =
        working_directory / "cli_comment_range_resolved.docx";
    const fs::path threaded =
        working_directory / "cli_comment_range_threaded.docx";
    const fs::path metadata_updated =
        working_directory / "cli_comment_range_metadata.docx";
    const fs::path thread_removed =
        working_directory / "cli_comment_range_thread_removed.docx";
    const fs::path output = working_directory / "cli_comment_range.json";
    const fs::path inspect_output =
        working_directory / "cli_comment_range_inspect.json";

    remove_if_exists(source);
    remove_if_exists(paragraph_comment);
    remove_if_exists(cross_comment);
    remove_if_exists(paragraph_moved);
    remove_if_exists(range_moved);
    remove_if_exists(resolved);
    remove_if_exists(threaded);
    remove_if_exists(metadata_updated);
    remove_if_exists(thread_removed);
    remove_if_exists(output);
    remove_if_exists(inspect_output);

    featherdoc::Document source_document(source);
    REQUIRE_FALSE(source_document.create_empty());
    auto first = source_document.paragraphs();
    REQUIRE(first.has_next());
    REQUIRE(first.add_run("Alpha ").has_next());
    REQUIRE(first.add_run("Beta").has_next());
    REQUIRE(first.add_run(" Gamma").has_next());
    auto second = first.insert_paragraph_after("Middle ");
    REQUIRE(second.has_next());
    REQUIRE(second.add_run("Text").has_next());
    auto third = second.insert_paragraph_after("Gamma");
    REQUIRE(third.has_next());
    REQUIRE(third.add_run("Delta").has_next());
    REQUIRE_FALSE(source_document.save());

    CHECK_EQ(run_cli({"append-paragraph-text-comment",
                      source.string(),
                      "0",
                      "0",
                      "3",
                      "--comment-text",
                      "CLI paragraph range comment",
                      "--author",
                      "CLI Commenter",
                      "--initials",
                      "CC",
                      "--date",
                      "2026-05-02T10:00:00Z",
                      "--output",
                      paragraph_comment.string(),
                      "--json"},
                     output),
             0);
    auto output_json = read_text_file(output);
    CHECK_NE(output_json.find(R"("start_paragraph_index":0)"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("text_length":3)"), std::string::npos);

    CHECK_EQ(run_cli({"append-text-range-comment",
                      paragraph_comment.string(),
                      "0",
                      "6",
                      "2",
                      "5",
                      "--comment-text",
                      "CLI cross paragraph comment",
                      "--author",
                      "CLI Cross Commenter",
                      "--initials",
                      "CX",
                      "--date",
                      "2026-05-02T10:10:00Z",
                      "--output",
                      cross_comment.string(),
                      "--json"},
                     output),
             0);
    output_json = read_text_file(output);
    CHECK_NE(output_json.find(R"("end_paragraph_index":2)"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("end_text_offset":5)"),
             std::string::npos);

    CHECK_EQ(run_cli({"inspect-review", cross_comment.string(), "--json"},
                     inspect_output),
             0);
    const auto inspect_json = read_text_file(inspect_output);
    CHECK_NE(inspect_json.find(R"("comments_count":2)"), std::string::npos);
    CHECK_NE(inspect_json.find(R"("anchor_text":"Alp")"),
             std::string::npos);
    CHECK_NE(inspect_json.find(
                 R"("anchor_text":"Beta GammaMiddle TextGamma")"),
             std::string::npos);
    CHECK_NE(inspect_json.find(R"("text":"CLI paragraph range comment")"),
             std::string::npos);
    CHECK_NE(inspect_json.find(R"("text":"CLI cross paragraph comment")"),
             std::string::npos);
    CHECK_NE(inspect_json.find(R"("date":"2026-05-02T10:00:00Z")"),
             std::string::npos);
    CHECK_NE(inspect_json.find(R"("date":"2026-05-02T10:10:00Z")"),
             std::string::npos);

    CHECK_EQ(run_cli({"set-paragraph-text-comment-range",
                      cross_comment.string(),
                      "0",
                      "0",
                      "6",
                      "4",
                      "--output",
                      paragraph_moved.string(),
                      "--json"},
                     output),
             0);
    output_json = read_text_file(output);
    CHECK_NE(output_json.find(R"("comment_index":0)"), std::string::npos);
    CHECK_NE(output_json.find(R"("text_length":4)"), std::string::npos);

    CHECK_EQ(run_cli({"set-text-range-comment-range",
                      paragraph_moved.string(),
                      "1",
                      "1",
                      "0",
                      "2",
                      "5",
                      "--output",
                      range_moved.string(),
                      "--json"},
                     output),
             0);
    output_json = read_text_file(output);
    CHECK_NE(output_json.find(R"("end_paragraph_index":2)"),
             std::string::npos);

    CHECK_EQ(run_cli({"inspect-review", range_moved.string(), "--json"},
                     inspect_output),
             0);
    const auto moved_json = read_text_file(inspect_output);
    CHECK_NE(moved_json.find(R"("comments_count":2)"), std::string::npos);
    CHECK_NE(moved_json.find(R"("anchor_text":"Beta")"),
             std::string::npos);
    CHECK_NE(moved_json.find(R"("anchor_text":"Middle TextGamma")"),
             std::string::npos);
    CHECK_NE(moved_json.find(R"("resolved":false)"), std::string::npos);

    CHECK_EQ(run_cli({"set-comment-resolved",
                      range_moved.string(),
                      "1",
                      "true",
                      "--output",
                      resolved.string(),
                      "--json"},
                     output),
             0);
    output_json = read_text_file(output);
    CHECK_NE(output_json.find(R"("comment_index":1)"), std::string::npos);
    CHECK_NE(output_json.find(R"("resolved":true)"), std::string::npos);

    CHECK_EQ(run_cli({"inspect-review", resolved.string(), "--json"},
                     inspect_output),
             0);
    const auto resolved_json = read_text_file(inspect_output);
    CHECK_NE(resolved_json.find(R"("comments_count":2)"), std::string::npos);
    CHECK_NE(resolved_json.find(R"("anchor_text":"Middle TextGamma")"),
             std::string::npos);
    CHECK_NE(resolved_json.find(R"("resolved":true)"), std::string::npos);

    CHECK_EQ(run_cli({"append-comment-reply",
                      resolved.string(),
                      "1",
                      "--comment-text",
                      "CLI threaded reply",
                      "--author",
                      "CLI Responder",
                      "--initials",
                      "RS",
                      "--date",
                      "2026-05-02T10:20:00Z",
                      "--output",
                      threaded.string(),
                      "--json"},
                     output),
             0);
    output_json = read_text_file(output);
    CHECK_NE(output_json.find(R"("parent_comment_index":1)"),
             std::string::npos);

    CHECK_EQ(run_cli({"inspect-review", threaded.string(), "--json"},
                     inspect_output),
             0);
    const auto threaded_json = read_text_file(inspect_output);
    CHECK_NE(threaded_json.find(R"("comments_count":3)"), std::string::npos);
    CHECK_NE(threaded_json.find(R"("text":"CLI threaded reply")"),
             std::string::npos);
    CHECK_NE(threaded_json.find(R"("date":"2026-05-02T10:20:00Z")"),
             std::string::npos);
    CHECK_NE(threaded_json.find(R"("parent_index":1)"), std::string::npos);
    CHECK_NE(threaded_json.find(R"("parent_id":")"), std::string::npos);

    CHECK_EQ(run_cli({"set-comment-metadata",
                      threaded.string(),
                      "2",
                      "--author",
                      "CLI Updated Responder",
                      "--clear-initials",
                      "--date",
                      "2026-05-02T10:25:00Z",
                      "--output",
                      metadata_updated.string(),
                      "--json"},
                     output),
             0);
    output_json = read_text_file(output);
    CHECK_NE(output_json.find(R"("comment_index":2)"), std::string::npos);
    CHECK_EQ(run_cli({"inspect-review", metadata_updated.string(), "--json"},
                     inspect_output),
             0);
    const auto metadata_json = read_text_file(inspect_output);
    CHECK_NE(metadata_json.find(R"("author":"CLI Updated Responder")"),
             std::string::npos);
    CHECK_NE(metadata_json.find(R"("date":"2026-05-02T10:25:00Z")"),
             std::string::npos);
    CHECK_NE(metadata_json.find(R"("initials":null)"), std::string::npos);

    CHECK_EQ(run_cli({"remove-comment",
                      metadata_updated.string(),
                      "1",
                      "--output",
                      thread_removed.string(),
                      "--json"},
                     output),
             0);
    CHECK_EQ(run_cli({"inspect-review", thread_removed.string(), "--json"},
                     inspect_output),
             0);
    const auto thread_removed_json = read_text_file(inspect_output);
    CHECK_NE(thread_removed_json.find(R"("comments_count":1)"),
             std::string::npos);
    CHECK_EQ(thread_removed_json.find("CLI threaded reply"), std::string::npos);

    remove_if_exists(source);
    remove_if_exists(paragraph_comment);
    remove_if_exists(cross_comment);
    remove_if_exists(paragraph_moved);
    remove_if_exists(range_moved);
    remove_if_exists(resolved);
    remove_if_exists(threaded);
    remove_if_exists(metadata_updated);
    remove_if_exists(thread_removed);
    remove_if_exists(output);
    remove_if_exists(inspect_output);
}

} // namespace
