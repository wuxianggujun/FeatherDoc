#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "cli_template_schema_test_support.hpp"

TEST_CASE("cli validate-template reports body schema mismatches as json") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_validate_template_body_source.docx";
    const fs::path output =
        working_directory / "cli_validate_template_body_output.json";

    remove_if_exists(source);
    remove_if_exists(output);

    create_cli_body_template_validation_fixture(source);

    CHECK_EQ(run_cli({"validate-template", source.string(), "--part", "body", "--slot",
                      "customer:text", "--slot", "summary_block:block", "--slot",
                      "signature_image:image", "--json"},
                     output),
             0);
    CHECK_EQ(read_text_file(output),
             std::string{
                 "{\"part\":\"body\",\"entry_name\":\"word/document.xml\","
                 "\"passed\":false,\"missing_required\":[\"signature_image\"],"
                 "\"duplicate_bookmarks\":[\"customer\"],"
                 "\"malformed_placeholders\":[\"summary_block\"],"
                 "\"unexpected_bookmarks\":[],\"kind_mismatches\":[],"
                 "\"occurrence_mismatches\":[]}\n"});

    remove_if_exists(source);
    remove_if_exists(output);
}

TEST_CASE("cli validate-template supports header and section footer targets") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_validate_template_parts_source.docx";
    const fs::path header_output =
        working_directory / "cli_validate_template_header_output.json";
    const fs::path footer_output =
        working_directory / "cli_validate_template_footer_output.txt";

    remove_if_exists(source);
    remove_if_exists(header_output);
    remove_if_exists(footer_output);

    create_cli_part_template_validation_fixture(source);

    CHECK_EQ(run_cli({"validate-template", source.string(), "--part", "header",
                      "--index", "0", "--slot", "header_title:text", "--slot",
                      "header_note:block", "--slot", "header_rows:table_rows",
                      "--json"},
                     header_output),
             0);
    CHECK_EQ(read_text_file(header_output),
             std::string{
                 "{\"part\":\"header\",\"part_index\":0,"
                 "\"entry_name\":\"word/header1.xml\",\"passed\":true,"
                 "\"missing_required\":[],\"duplicate_bookmarks\":[],"
                 "\"malformed_placeholders\":[],\"unexpected_bookmarks\":[],"
                 "\"kind_mismatches\":[],\"occurrence_mismatches\":[]}\n"});

    CHECK_EQ(run_cli({"validate-template", source.string(), "--part",
                      "section-footer", "--section", "0", "--kind", "default",
                      "--slot", "footer_company:text", "--slot",
                      "footer_summary:block", "--slot",
                      "footer_extra:text:optional"},
                     footer_output),
             0);
    CHECK_EQ(read_text_file(footer_output),
             std::string{
                 "part: section-footer\n"
                 "section: 0\n"
                 "kind: default\n"
                 "entry_name: word/footer1.xml\n"
                 "passed: no\n"
                 "missing_required: none\n"
                 "duplicate_bookmarks: footer_company\n"
                 "malformed_placeholders: footer_summary\n"
                 "unexpected_bookmarks: none\n"
                 "kind_mismatches: none\n"
                 "occurrence_mismatches: none\n"});

    remove_if_exists(source);
    remove_if_exists(header_output);
    remove_if_exists(footer_output);
}

TEST_CASE("cli validate-template reports unexpected bookmarks kind mismatches and occurrence mismatches") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_validate_template_schema_v2_source.docx";
    const fs::path output =
        working_directory / "cli_validate_template_schema_v2_output.json";

    remove_if_exists(source);
    remove_if_exists(output);

    create_cli_schema_v2_template_validation_fixture(source);

    CHECK_EQ(run_cli({"validate-template",
                      source.string(),
                      "--part",
                      "body",
                      "--slot",
                      "summary_block:text",
                      "--slot",
                      "approver:text:count=2",
                      "--json"},
                     output),
             0);

    CHECK_EQ(read_text_file(output),
             std::string{
                 "{\"part\":\"body\",\"entry_name\":\"word/document.xml\","
                 "\"passed\":false,\"missing_required\":[],"
                 "\"duplicate_bookmarks\":[],\"malformed_placeholders\":[],"
                 "\"unexpected_bookmarks\":[{\"bookmark_name\":\"extra_slot\","
                 "\"occurrence_count\":1,\"kind\":\"text\","
                 "\"is_duplicate\":false}],"
                 "\"kind_mismatches\":[{\"bookmark_name\":\"summary_block\","
                 "\"expected_kind\":\"text\",\"actual_kind\":\"block\","
                 "\"occurrence_count\":1}],"
                 "\"occurrence_mismatches\":[{\"bookmark_name\":\"approver\","
                 "\"actual_occurrences\":1,\"min_occurrences\":2,"
                 "\"max_occurrences\":2}]}\n"});

    remove_if_exists(source);
    remove_if_exists(output);
}

TEST_CASE("cli validate-template reports json parse errors") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_validate_template_parse_source.docx";
    const fs::path output =
        working_directory / "cli_validate_template_parse_output.json";

    remove_if_exists(source);
    remove_if_exists(output);

    create_cli_part_template_validation_fixture(source);

    CHECK_EQ(run_cli({"validate-template", source.string(), "--part",
                      "section-header", "--slot", "header_title:text", "--json"},
                     output),
             2);
    CHECK_EQ(
        read_text_file(output),
        std::string{
            "{\"command\":\"validate-template\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"section-header/section-footer "
            "validation requires --section <index>\"}\n"});

    CHECK_EQ(run_cli({"validate-template", source.string(), "--part", "body", "--slot",
                      "header_title:text:min=3:max=1", "--json"},
                     output),
             2);
    CHECK_EQ(read_text_file(output),
             std::string{
                 "{\"command\":\"validate-template\",\"ok\":false,"
                 "\"stage\":\"parse\",\"message\":\"invalid --slot occurrence "
                 "range: max must be greater than or equal to min\"}\n"});

    remove_if_exists(source);
    remove_if_exists(output);
}
