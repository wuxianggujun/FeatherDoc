#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "cli_template_schema_test_support.hpp"

TEST_CASE("cli validate-template-schema aggregates multiple targets as json") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_validate_template_schema_source.docx";
    const fs::path output =
        working_directory / "cli_validate_template_schema_output.json";

    remove_if_exists(source);
    remove_if_exists(output);

    create_cli_part_template_validation_fixture(source);

    CHECK_EQ(run_cli({"validate-template-schema",
                      source.string(),
                      "--target",
                      "section-header",
                      "--section",
                      "0",
                      "--kind",
                      "default",
                      "--slot",
                      "header_title:text",
                      "--slot",
                      "header_note:block",
                      "--slot",
                      "header_rows:table_rows",
                      "--target",
                      "section-footer",
                      "--section",
                      "0",
                      "--kind",
                      "default",
                      "--slot",
                      "footer_company:text",
                      "--slot",
                      "footer_summary:block",
                      "--slot",
                      "footer_signature:text",
                      "--json"},
                     output),
             0);

    CHECK_EQ(read_text_file(output),
             std::string{
                 "{\"passed\":false,\"part_results\":["
                 "{\"part\":\"section-header\",\"section\":0,"
                 "\"kind\":\"default\",\"available\":true,"
                 "\"entry_name\":\"word/header1.xml\",\"passed\":true,"
                 "\"missing_required\":[],\"duplicate_bookmarks\":[],"
                 "\"malformed_placeholders\":[],\"unexpected_bookmarks\":[],"
                 "\"kind_mismatches\":[],\"occurrence_mismatches\":[]},"
                 "{\"part\":\"section-footer\",\"section\":0,"
                 "\"kind\":\"default\",\"available\":true,"
                 "\"entry_name\":\"word/footer1.xml\",\"passed\":false,"
                 "\"missing_required\":[\"footer_signature\"],"
                 "\"duplicate_bookmarks\":[\"footer_company\"],"
                 "\"malformed_placeholders\":[\"footer_summary\"],"
                 "\"unexpected_bookmarks\":[],\"kind_mismatches\":[],"
                 "\"occurrence_mismatches\":[]}]}\n"});

    remove_if_exists(source);
    remove_if_exists(output);
}

TEST_CASE("cli validate-template-schema reports unavailable targets in text output") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_validate_template_schema_unavailable_source.docx";
    const fs::path output =
        working_directory / "cli_validate_template_schema_unavailable_output.txt";

    remove_if_exists(source);
    remove_if_exists(output);

    create_cli_part_template_validation_fixture(source);

    CHECK_EQ(run_cli({"validate-template-schema",
                      source.string(),
                      "--target",
                      "section-header",
                      "--section",
                      "1",
                      "--slot",
                      "optional_header:text:optional",
                      "--target",
                      "section-footer",
                      "--section",
                      "1",
                      "--slot",
                      "required_footer:text"},
                     output),
             0);

    CHECK_EQ(read_text_file(output),
             std::string{
                 "passed: no\n"
                 "part_result_count: 2\n"
                 "\n"
                 "part_result[0]\n"
                 "part: section-header\n"
                 "section: 1\n"
                 "kind: default\n"
                 "available: no\n"
                 "entry_name: \n"
                 "passed: yes\n"
                 "missing_required: none\n"
                 "duplicate_bookmarks: none\n"
                 "malformed_placeholders: none\n"
                 "unexpected_bookmarks: none\n"
                 "kind_mismatches: none\n"
                 "occurrence_mismatches: none\n"
                 "\n"
                 "part_result[1]\n"
                 "part: section-footer\n"
                 "section: 1\n"
                 "kind: default\n"
                 "available: no\n"
                 "entry_name: \n"
                 "passed: no\n"
                 "missing_required: required_footer\n"
                 "duplicate_bookmarks: none\n"
                 "malformed_placeholders: none\n"
                 "unexpected_bookmarks: none\n"
                 "kind_mismatches: none\n"
                 "occurrence_mismatches: none\n"});

    remove_if_exists(source);
    remove_if_exists(output);
}

TEST_CASE("cli validate-template-schema reads targets from a schema file") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_validate_template_schema_file_source.docx";
    const fs::path schema_file =
        working_directory / "cli_validate_template_schema_file.json";
    const fs::path output =
        working_directory / "cli_validate_template_schema_file_output.json";

    remove_if_exists(source);
    remove_if_exists(schema_file);
    remove_if_exists(output);

    create_cli_part_template_validation_fixture(source);
    write_binary_file(
        schema_file,
        std::string{
            "{\n"
            "  \"targets\": [\n"
            "    {\n"
            "      \"part\": \"section-header\",\n"
            "      \"section\": 0,\n"
            "      \"kind\": \"default\",\n"
            "      \"slots\": [\"header_title:text\", \"header_note:block\", "
            "\"header_rows:table_rows\"]\n"
            "    },\n"
            "    {\n"
            "      \"part\": \"section-footer\",\n"
            "      \"section\": 0,\n"
            "      \"kind\": \"default\",\n"
            "      \"slots\": [\"footer_company:text\", "
            "\"footer_summary:block\", \"footer_signature:text\"]\n"
            "    }\n"
            "  ]\n"
            "}\n"});

    CHECK_EQ(run_cli({"validate-template-schema",
                      source.string(),
                      "--schema-file",
                      schema_file.string(),
                      "--json"},
                     output),
             0);

    CHECK_EQ(read_text_file(output),
             std::string{
                 "{\"passed\":false,\"part_results\":["
                 "{\"part\":\"section-header\",\"section\":0,"
                 "\"kind\":\"default\",\"available\":true,"
                 "\"entry_name\":\"word/header1.xml\",\"passed\":true,"
                 "\"missing_required\":[],\"duplicate_bookmarks\":[],"
                 "\"malformed_placeholders\":[],\"unexpected_bookmarks\":[],"
                 "\"kind_mismatches\":[],\"occurrence_mismatches\":[]},"
                 "{\"part\":\"section-footer\",\"section\":0,"
                 "\"kind\":\"default\",\"available\":true,"
                 "\"entry_name\":\"word/footer1.xml\",\"passed\":false,"
                 "\"missing_required\":[\"footer_signature\"],"
                 "\"duplicate_bookmarks\":[\"footer_company\"],"
                 "\"malformed_placeholders\":[\"footer_summary\"],"
                 "\"unexpected_bookmarks\":[],\"kind_mismatches\":[],"
                 "\"occurrence_mismatches\":[]}]}\n"});

    remove_if_exists(source);
    remove_if_exists(schema_file);
    remove_if_exists(output);
}
TEST_CASE("cli validate-template-schema reads structured slot objects from a schema file") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_validate_template_schema_object_file_source.docx";
    const fs::path schema_file =
        working_directory / "cli_validate_template_schema_object_file.json";
    const fs::path output =
        working_directory / "cli_validate_template_schema_object_file_output.json";

    remove_if_exists(source);
    remove_if_exists(schema_file);
    remove_if_exists(output);

    create_cli_part_template_validation_fixture(source);
    write_binary_file(
        schema_file,
        std::string{
            "{\n"
            "  \"targets\": [\n"
            "    {\n"
            "      \"part\": \"section-header\",\n"
            "      \"section\": 0,\n"
            "      \"kind\": \"default\",\n"
            "      \"slots\": [\n"
            "        {\"bookmark\": \"header_title\", \"kind\": \"text\"},\n"
            "        {\"bookmark_name\": \"header_note\", \"kind\": \"block\", "
            "\"required\": true},\n"
            "        {\"bookmark\": \"header_rows\", \"kind\": \"table_rows\", "
            "\"count\": 1}\n"
            "      ]\n"
            "    },\n"
            "    {\n"
            "      \"part\": \"section-footer\",\n"
            "      \"section\": 0,\n"
            "      \"kind\": \"default\",\n"
            "      \"slots\": [\n"
            "        {\"bookmark\": \"footer_company\", \"kind\": \"text\"},\n"
            "        {\"bookmark\": \"footer_summary\", \"kind\": \"block\"},\n"
            "        {\"bookmark\": \"footer_signature\", \"kind\": \"text\"}\n"
            "      ]\n"
            "    }\n"
            "  ]\n"
            "}\n"});

    CHECK_EQ(run_cli({"validate-template-schema",
                      source.string(),
                      "--schema-file",
                      schema_file.string(),
                      "--json"},
                     output),
             0);

    CHECK_EQ(read_text_file(output),
             std::string{
                 "{\"passed\":false,\"part_results\":["
                 "{\"part\":\"section-header\",\"section\":0,"
                 "\"kind\":\"default\",\"available\":true,"
                 "\"entry_name\":\"word/header1.xml\",\"passed\":true,"
                 "\"missing_required\":[],\"duplicate_bookmarks\":[],"
                 "\"malformed_placeholders\":[],\"unexpected_bookmarks\":[],"
                 "\"kind_mismatches\":[],\"occurrence_mismatches\":[]},"
                 "{\"part\":\"section-footer\",\"section\":0,"
                 "\"kind\":\"default\",\"available\":true,"
                 "\"entry_name\":\"word/footer1.xml\",\"passed\":false,"
                 "\"missing_required\":[\"footer_signature\"],"
                 "\"duplicate_bookmarks\":[\"footer_company\"],"
                 "\"malformed_placeholders\":[\"footer_summary\"],"
                 "\"unexpected_bookmarks\":[],\"kind_mismatches\":[],"
                 "\"occurrence_mismatches\":[]}]}\n"});

    remove_if_exists(source);
    remove_if_exists(schema_file);
    remove_if_exists(output);
}

TEST_CASE("cli validate-template-schema accepts content control slots") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_validate_template_schema_content_controls.docx";
    const fs::path schema_file =
        working_directory / "cli_validate_template_schema_content_controls.json";
    const fs::path slot_output =
        working_directory / "cli_validate_template_schema_content_controls_slot.json";
    const fs::path schema_output =
        working_directory / "cli_validate_template_schema_content_controls_file.json";
    const fs::path missing_output =
        working_directory / "cli_validate_template_schema_content_controls_missing.json";

    remove_if_exists(source);
    remove_if_exists(schema_file);
    remove_if_exists(slot_output);
    remove_if_exists(schema_output);
    remove_if_exists(missing_output);

    create_cli_content_controls_fixture(source);

    CHECK_EQ(run_cli({"validate-template-schema",
                      source.string(),
                      "--target",
                      "body",
                      "--slot",
                      "content_control_tag=order_no:text",
                      "--slot",
                      "content_control_alias=Line Items:table_rows",
                      "--json"},
                     slot_output),
             0);
    const auto slot_json = read_text_file(slot_output);
    CHECK_NE(slot_json.find(R"("passed":true)"), std::string::npos);
    CHECK_NE(slot_json.find(R"("missing_required":[])"), std::string::npos);

    write_binary_file(schema_file,
                      std::string{
                          "{"
                          "\"targets\":[{"
                          "\"part\":\"body\","
                          "\"slots\":["
                          "{\"content_control_tag\":\"order_no\",\"kind\":\"text\"},"
                          "{\"content_control_alias\":\"Line Items\","
                          "\"kind\":\"table_rows\"}"
                          "]"
                          "}]"
                          "}"});
    CHECK_EQ(run_cli({"validate-template-schema",
                      source.string(),
                      "--schema-file",
                      schema_file.string(),
                      "--json"},
                     schema_output),
             0);
    const auto schema_json = read_text_file(schema_output);
    CHECK_NE(schema_json.find(R"("passed":true)"), std::string::npos);
    CHECK_NE(schema_json.find(R"("kind_mismatches":[])"), std::string::npos);

    CHECK_EQ(run_cli({"validate-template-schema",
                      source.string(),
                      "--target",
                      "body",
                      "--slot",
                      "content_control_tag=missing:text",
                      "--json"},
                     missing_output),
             0);
    CHECK_NE(read_text_file(missing_output).find(
                 R"("missing_required":["content_control_tag:missing"])"),
             std::string::npos);

    remove_if_exists(source);
    remove_if_exists(schema_file);
    remove_if_exists(slot_output);
    remove_if_exists(schema_output);
    remove_if_exists(missing_output);
}

TEST_CASE("cli validate-template-schema reports parse errors") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_validate_template_schema_parse_source.docx";
    const fs::path schema_file =
        working_directory / "cli_validate_template_schema_parse_schema.json";
    const fs::path output =
        working_directory / "cli_validate_template_schema_parse_output.json";

    remove_if_exists(source);
    remove_if_exists(schema_file);
    remove_if_exists(output);

    create_cli_part_template_validation_fixture(source);

    CHECK_EQ(run_cli({"validate-template-schema",
                      source.string(),
                      "--slot",
                      "header_title:text",
                      "--json"},
                     output),
             2);
    CHECK_EQ(
        read_text_file(output),
        std::string{
            "{\"command\":\"validate-template-schema\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"--slot requires a preceding "
            "--target <body|header|footer|section-header|section-footer>\"}\n"});

    CHECK_EQ(run_cli({"validate-template-schema",
                      source.string(),
                      "--target",
                      "section-header",
                      "--slot",
                      "header_title:text",
                      "--json"},
                     output),
             2);
    CHECK_EQ(
        read_text_file(output),
        std::string{
            "{\"command\":\"validate-template-schema\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"section-header/section-footer "
            "schema validation requires --section <index>\"}\n"});

    write_binary_file(schema_file,
                      std::string{
                          "{"
                          "\"targets\":[{"
                          "\"part\":\"section-header\","
                          "\"slots\":[\"header_title:text\"]"
                          "}]"
                          "}"});
    CHECK_EQ(run_cli({"validate-template-schema",
                      source.string(),
                      "--schema-file",
                      schema_file.string(),
                      "--json"},
                     output),
             2);
    CHECK_EQ(
        read_text_file(output),
        std::string{
            "{\"command\":\"validate-template-schema\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"section-header/section-footer "
            "schema validation requires --section <index>\"}\n"});

    write_binary_file(schema_file,
                      std::string{
                          "{"
                          "\"targets\":[{"
                          "\"part\":\"body\","
                          "\"slots\":[{"
                          "\"bookmark\":\"customer\","
                          "\"kind\":\"text\","
                          "\"count\":1,"
                          "\"min\":1"
                          "}]"
                          "}]"
                          "}"});
    CHECK_EQ(run_cli({"validate-template-schema",
                      source.string(),
                      "--schema-file",
                      schema_file.string(),
                      "--json"},
                     output),
             2);
    CHECK_EQ(
        read_text_file(output),
        std::string{
            "{\"command\":\"validate-template-schema\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"JSON schema slot member "
            "'count' conflicts with 'min'/'max'\"}\n"});

    remove_if_exists(source);
    remove_if_exists(schema_file);
    remove_if_exists(output);
}
