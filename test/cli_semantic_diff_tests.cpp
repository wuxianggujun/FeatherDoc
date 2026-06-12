#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "cli_semantic_diff_test_support.hpp"

TEST_CASE("cli semantic-diff reports document changes and can fail on diff") {
    const fs::path working_directory = fs::current_path();
    const fs::path left = working_directory / "cli_semantic_diff_left.docx";
    const fs::path right = working_directory / "cli_semantic_diff_right.docx";
    const fs::path json_output = working_directory / "cli_semantic_diff.json";
    const fs::path fail_output = working_directory / "cli_semantic_diff_fail.json";
    const fs::path text_output = working_directory / "cli_semantic_diff.txt";
    const fs::path changed_text_output =
        working_directory / "cli_semantic_diff_changed.txt";

    remove_if_exists(left);
    remove_if_exists(right);
    remove_if_exists(json_output);
    remove_if_exists(fail_output);
    remove_if_exists(text_output);

    create_cli_semantic_diff_fixture(left, "draft", "Ada Lovelace", "$120");
    create_cli_semantic_diff_fixture(right, "approved", "Grace Hopper", "$150");

    CHECK_EQ(run_cli({"semantic-diff", left.string(), right.string(), "--json"},
                     json_output),
             0);
    const auto json = read_text_file(json_output);
    CHECK_NE(json.find(R"("command":"semantic-diff")"), std::string::npos);
    CHECK_NE(json.find(R"("different":true)"), std::string::npos);
    CHECK_NE(json.find(R"("change_count":3)"), std::string::npos);
    CHECK_NE(json.find(R"("paragraph_changes")"), std::string::npos);
    CHECK_NE(json.find(R"("table_changes")"), std::string::npos);
    CHECK_NE(json.find(R"("content_control_changes")"), std::string::npos);
    CHECK_NE(json.find(R"("section_changes")"), std::string::npos);
    CHECK_NE(json.find(R"("sections":{"left_count":1)"), std::string::npos);
    CHECK_NE(json.find(R"("field_changes")"), std::string::npos);
    CHECK_NE(json.find(R"("field_path":"text")"), std::string::npos);
    CHECK_NE(json.find("Status: draft"), std::string::npos);
    CHECK_NE(json.find("Grace Hopper"), std::string::npos);

    CHECK_EQ(run_cli({"semantic-diff", left.string(), right.string(),
                      "--fail-on-diff", "--json"},
                     fail_output),
             1);
    const auto fail_json = read_text_file(fail_output);
    CHECK_NE(fail_json.find(R"("fail_on_diff":true)"), std::string::npos);
    CHECK_NE(fail_json.find(R"("different":true)"), std::string::npos);

    CHECK_EQ(run_cli({"semantic-diff", left.string(), right.string()},
                     changed_text_output),
             0);
    const auto changed_text = read_text_file(changed_text_output);
    CHECK_NE(changed_text.find("paragraph_change[0].field[0]: text"),
             std::string::npos);
    CHECK_NE(changed_text.find("content_control_change[0].field[0]: text"),
             std::string::npos);

    CHECK_EQ(run_cli({"semantic-diff", left.string(), left.string()}, text_output), 0);
    const auto text = read_text_file(text_output);
    CHECK_NE(text.find("different: no"), std::string::npos);
    CHECK_NE(text.find("change_count: 0"), std::string::npos);

    remove_if_exists(left);
    remove_if_exists(right);
    remove_if_exists(json_output);
    remove_if_exists(fail_output);
    remove_if_exists(text_output);
    remove_if_exists(changed_text_output);
}





TEST_CASE("cli semantic-diff reports TOC REF and SEQ field changes") {
    const fs::path working_directory = fs::current_path();
    const fs::path left = working_directory / "cli_semantic_diff_fields_left.docx";
    const fs::path right = working_directory / "cli_semantic_diff_fields_right.docx";
    const fs::path json_output = working_directory / "cli_semantic_diff_fields.json";
    const fs::path disabled_output =
        working_directory / "cli_semantic_diff_fields_disabled.json";
    const fs::path text_output = working_directory / "cli_semantic_diff_fields.txt";

    remove_if_exists(left);
    remove_if_exists(right);
    remove_if_exists(json_output);
    remove_if_exists(disabled_output);
    remove_if_exists(text_output);

    create_cli_semantic_diff_field_fixture(left, 2U, "Referenced heading", 1U,
                                           "Figure 1");
    create_cli_semantic_diff_field_fixture(right, 3U, "Approved heading", 2U,
                                           "Figure 2");

    CHECK_EQ(run_cli({"semantic-diff", left.string(), right.string(), "--json",
                      "--no-paragraphs", "--no-tables", "--no-images",
                      "--no-content-controls", "--no-sections"},
                     json_output),
             0);
    const auto json = read_text_file(json_output);
    CHECK_NE(json.find(R"("fields":{"left_count":3)"), std::string::npos);
    CHECK_NE(json.find(R"("field_changes")"), std::string::npos);
    CHECK_NE(json.find(R"("field":"field")"), std::string::npos);
    CHECK_NE(json.find(R"("kind":"changed")"), std::string::npos);
    CHECK_NE(json.find(R"("field_path":"instruction")"), std::string::npos);
    CHECK_NE(json.find(R"("field_path":"result_text")"), std::string::npos);
    CHECK_NE(json.find("kind=table_of_contents"), std::string::npos);
    CHECK_NE(json.find("kind=reference"), std::string::npos);
    CHECK_NE(json.find("kind=sequence"), std::string::npos);
    CHECK_NE(json.find("Approved heading"), std::string::npos);
    CHECK_NE(json.find(R"("template_part_results")"), std::string::npos);

    CHECK_EQ(run_cli({"semantic-diff", left.string(), right.string(), "--json",
                      "--no-paragraphs", "--no-tables", "--no-images",
                      "--no-content-controls", "--no-sections", "--no-fields"},
                     disabled_output),
             0);
    const auto disabled_json = read_text_file(disabled_output);
    CHECK_NE(disabled_json.find(R"("change_count":0)"), std::string::npos);
    CHECK_NE(disabled_json.find(R"("fields":{"left_count":0)"),
             std::string::npos);

    CHECK_EQ(run_cli({"semantic-diff", left.string(), right.string(),
                      "--no-paragraphs", "--no-tables", "--no-images",
                      "--no-content-controls", "--no-sections"},
                     text_output),
             0);
    const auto text = read_text_file(text_output);
    CHECK_NE(text.find("fields: left=3 right=3 added=0 removed=0 changed=3"),
             std::string::npos);
    CHECK_NE(text.find("field_change[0]: changed left=0 right=0 field=field"),
             std::string::npos);
    CHECK_NE(text.find("field_change[0].field[0]: instruction"),
             std::string::npos);
    CHECK_NE(text.find("template_part_field_change[0]"), std::string::npos);

    remove_if_exists(left);
    remove_if_exists(right);
    remove_if_exists(json_output);
    remove_if_exists(disabled_output);
    remove_if_exists(text_output);
}



TEST_CASE("cli semantic-diff reports style and numbering summary changes") {
    const fs::path working_directory = fs::current_path();
    const fs::path left =
        working_directory / "cli_semantic_diff_style_numbering_left.docx";
    const fs::path right =
        working_directory / "cli_semantic_diff_style_numbering_right.docx";
    const fs::path json_output =
        working_directory / "cli_semantic_diff_style_numbering.json";
    const fs::path disabled_output =
        working_directory / "cli_semantic_diff_style_numbering_disabled.json";
    const fs::path text_output =
        working_directory / "cli_semantic_diff_style_numbering.txt";

    remove_if_exists(left);
    remove_if_exists(right);
    remove_if_exists(json_output);
    remove_if_exists(disabled_output);
    remove_if_exists(text_output);

    create_cli_semantic_diff_style_numbering_fixture(
        left, "CLI Draft Heading", "Heading1", "CliDraftOutline",
        featherdoc::list_kind::decimal, 1U, "%1.");
    create_cli_semantic_diff_style_numbering_fixture(
        right, "CLI Approved Heading", "Title", "CliApprovedOutline",
        featherdoc::list_kind::bullet, 3U, "o");

    CHECK_EQ(run_cli({"semantic-diff", left.string(), right.string(), "--json",
                      "--no-paragraphs", "--no-tables", "--no-images",
                      "--no-content-controls", "--no-fields", "--no-sections",
                      "--no-template-parts"},
                     json_output),
             0);
    const auto json = read_text_file(json_output);
    CHECK_NE(json.find(R"("change_count":2)"), std::string::npos);
    CHECK_NE(json.find(R"("styles":{")"), std::string::npos);
    CHECK_NE(json.find(R"("numbering":{")"), std::string::npos);
    CHECK_NE(json.find(R"("style_changes")"), std::string::npos);
    CHECK_NE(json.find(R"("numbering_changes")"), std::string::npos);
    CHECK_NE(json.find(R"("field":"style")"), std::string::npos);
    CHECK_NE(json.find(R"("field":"numbering")"), std::string::npos);
    CHECK_NE(json.find(R"("field_path":"name")"), std::string::npos);
    CHECK_NE(json.find(R"("field_path":"levels.kind")"), std::string::npos);
    CHECK_NE(json.find("CLI Approved Heading"), std::string::npos);
    CHECK_NE(json.find("kind=bullet"), std::string::npos);

    CHECK_EQ(run_cli({"semantic-diff", left.string(), right.string(), "--json",
                      "--no-paragraphs", "--no-tables", "--no-images",
                      "--no-content-controls", "--no-fields", "--no-sections",
                      "--no-template-parts", "--no-styles", "--no-numbering"},
                     disabled_output),
             0);
    const auto disabled_json = read_text_file(disabled_output);
    CHECK_NE(disabled_json.find(R"("change_count":0)"), std::string::npos);
    CHECK_NE(disabled_json.find(R"("styles":{"left_count":0)"),
             std::string::npos);
    CHECK_NE(disabled_json.find(R"("numbering":{"left_count":0)"),
             std::string::npos);

    CHECK_EQ(run_cli({"semantic-diff", left.string(), right.string(),
                      "--no-paragraphs", "--no-tables", "--no-images",
                      "--no-content-controls", "--no-fields", "--no-sections",
                      "--no-template-parts"},
                     text_output),
             0);
    const auto text = read_text_file(text_output);
    CHECK_NE(text.find("styles: left="), std::string::npos);
    CHECK_NE(text.find("numbering: left="), std::string::npos);
    CHECK_NE(text.find("style_change[0]: changed"), std::string::npos);
    CHECK_NE(text.find("numbering_change[0]: changed"), std::string::npos);
    CHECK_NE(text.find("style_change[0].field[0]: name"), std::string::npos);
    CHECK_NE(text.find("numbering_change[0].field"), std::string::npos);

    remove_if_exists(left);
    remove_if_exists(right);
    remove_if_exists(json_output);
    remove_if_exists(disabled_output);
    remove_if_exists(text_output);
}


TEST_CASE("cli semantic-diff reports review object changes") {
    const fs::path working_directory = fs::current_path();
    const fs::path left = working_directory / "cli_semantic_diff_review_left.docx";
    const fs::path right = working_directory / "cli_semantic_diff_review_right.docx";
    const fs::path json_output = working_directory / "cli_semantic_diff_review.json";
    const fs::path disabled_output =
        working_directory / "cli_semantic_diff_review_disabled.json";
    const fs::path text_output = working_directory / "cli_semantic_diff_review.txt";

    remove_if_exists(left);
    remove_if_exists(right);
    remove_if_exists(json_output);
    remove_if_exists(disabled_output);
    remove_if_exists(text_output);

    create_cli_semantic_diff_review_fixture(
        left, "CLI original footnote", "CLI original endnote",
        "CLI original comment", "CLI inserted draft", "Ada");
    create_cli_semantic_diff_review_fixture(
        right, "CLI approved footnote", "CLI approved endnote",
        "CLI approved comment", "CLI inserted approved", "Grace");

    CHECK_EQ(run_cli({"semantic-diff", left.string(), right.string(), "--json",
                      "--no-paragraphs", "--no-tables", "--no-images",
                      "--no-content-controls", "--no-fields", "--no-styles",
                      "--no-numbering", "--no-sections", "--no-template-parts"},
                     json_output),
             0);
    const auto json = read_text_file(json_output);
    CHECK_NE(json.find(R"("change_count":4)"), std::string::npos);
    CHECK_NE(json.find(R"("footnotes":{"left_count":1,"right_count":1)"),
             std::string::npos);
    CHECK_NE(json.find(R"("endnotes":{"left_count":1,"right_count":1)"),
             std::string::npos);
    CHECK_NE(json.find(R"("comments":{"left_count":1,"right_count":1)"),
             std::string::npos);
    CHECK_NE(json.find(R"("revisions":{"left_count":1,"right_count":1)"),
             std::string::npos);
    CHECK_NE(json.find(R"("footnote_changes")"), std::string::npos);
    CHECK_NE(json.find(R"("endnote_changes")"), std::string::npos);
    CHECK_NE(json.find(R"("comment_changes")"), std::string::npos);
    CHECK_NE(json.find(R"("revision_changes")"), std::string::npos);
    CHECK_NE(json.find(R"("field":"footnote")"), std::string::npos);
    CHECK_NE(json.find(R"("field":"endnote")"), std::string::npos);
    CHECK_NE(json.find(R"("field":"comment")"), std::string::npos);
    CHECK_NE(json.find(R"("field":"revision")"), std::string::npos);
    CHECK_NE(json.find(R"("field_path":"text")"), std::string::npos);
    CHECK_NE(json.find(R"("field_path":"author")"), std::string::npos);
    CHECK_NE(json.find("CLI approved footnote"), std::string::npos);
    CHECK_NE(json.find("CLI inserted approved"), std::string::npos);
    CHECK_NE(json.find("Grace"), std::string::npos);

    CHECK_EQ(run_cli({"semantic-diff", left.string(), right.string(), "--json",
                      "--no-paragraphs", "--no-tables", "--no-images",
                      "--no-content-controls", "--no-fields", "--no-styles",
                      "--no-numbering", "--no-sections", "--no-template-parts",
                      "--no-footnotes", "--no-endnotes", "--no-comments",
                      "--no-revisions"},
                     disabled_output),
             0);
    const auto disabled_json = read_text_file(disabled_output);
    CHECK_NE(disabled_json.find(R"("change_count":0)"), std::string::npos);
    CHECK_NE(disabled_json.find(R"("footnotes":{"left_count":0)"),
             std::string::npos);
    CHECK_NE(disabled_json.find(R"("revisions":{"left_count":0)"),
             std::string::npos);

    CHECK_EQ(run_cli({"semantic-diff", left.string(), right.string(),
                      "--no-paragraphs", "--no-tables", "--no-images",
                      "--no-content-controls", "--no-fields", "--no-styles",
                      "--no-numbering", "--no-sections", "--no-template-parts"},
                     text_output),
             0);
    const auto text = read_text_file(text_output);
    CHECK_NE(text.find("footnotes: left=1 right=1"), std::string::npos);
    CHECK_NE(text.find("endnotes: left=1 right=1"), std::string::npos);
    CHECK_NE(text.find("comments: left=1 right=1"), std::string::npos);
    CHECK_NE(text.find("revisions: left=1 right=1"), std::string::npos);
    CHECK_NE(text.find("footnote_change[0]: changed"), std::string::npos);
    CHECK_NE(text.find("revision_change[0].field"), std::string::npos);

    remove_if_exists(left);
    remove_if_exists(right);
    remove_if_exists(json_output);
    remove_if_exists(disabled_output);
    remove_if_exists(text_output);
}
