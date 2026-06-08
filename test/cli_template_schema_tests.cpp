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

TEST_CASE("cli export-template-schema prints reusable schema json") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_export_template_schema_source.docx";
    const fs::path output =
        working_directory / "cli_export_template_schema_output.json";

    remove_if_exists(source);
    remove_if_exists(output);

    create_cli_part_template_validation_fixture(source);

    CHECK_EQ(run_cli({"export-template-schema", source.string()}, output), 0);
    CHECK_EQ(read_text_file(output),
             std::string{
                 "{\"targets\":["
                 "{\"part\":\"header\",\"index\":0,\"slots\":["
                 "{\"bookmark\":\"header_title\",\"kind\":\"text\"},"
                 "{\"bookmark\":\"header_note\",\"kind\":\"block\"},"
                 "{\"bookmark\":\"header_rows\",\"kind\":\"table_rows\"}]},"
                 "{\"part\":\"footer\",\"index\":0,\"slots\":["
                 "{\"bookmark\":\"footer_company\",\"kind\":\"text\",\"count\":2},"
                 "{\"bookmark\":\"footer_summary\",\"kind\":\"text\"}]}]}\n"});

    remove_if_exists(source);
    remove_if_exists(output);
}

TEST_CASE("cli export-template-schema includes content control slots") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_export_template_schema_content_controls.docx";
    const fs::path output =
        working_directory / "cli_export_template_schema_content_controls.json";

    remove_if_exists(source);
    remove_if_exists(output);

    create_cli_content_controls_fixture(source);

    CHECK_EQ(run_cli({"export-template-schema", source.string()}, output), 0);
    const auto exported_json = read_text_file(output);
    CHECK_NE(exported_json.find(
                 R"({"content_control_tag":"customer_name","kind":"block"})"),
             std::string::npos);
    CHECK_NE(exported_json.find(
                 R"({"content_control_tag":"order_no","kind":"text"})"),
             std::string::npos);
    CHECK_NE(exported_json.find(
                 R"({"content_control_tag":"line_items","kind":"table_rows"})"),
             std::string::npos);
    CHECK_NE(exported_json.find(
                 R"({"content_control_tag":"header_review","kind":"block"})"),
             std::string::npos);

    remove_if_exists(source);
    remove_if_exists(output);
}

TEST_CASE("cli export-template-schema writes schema file and summary json") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_export_template_schema_summary_source.docx";
    const fs::path schema_output =
        working_directory / "cli_export_template_schema_summary.schema.json";
    const fs::path output =
        working_directory / "cli_export_template_schema_summary_output.json";

    remove_if_exists(source);
    remove_if_exists(schema_output);
    remove_if_exists(output);

    create_cli_body_template_validation_fixture(source);

    CHECK_EQ(run_cli({"export-template-schema",
                      source.string(),
                      "--output",
                      schema_output.string(),
                      "--json"},
                     output),
             0);
    CHECK_EQ(read_text_file(output),
             std::string{
                 "{\"command\":\"export-template-schema\",\"ok\":true,"
                 "\"output_path\":\"" +
                 json_escape_text(schema_output.string()) +
                 "\",\"target_count\":1,\"slot_count\":2,"
                 "\"skipped_count\":0,\"skipped_bookmarks\":[]}\n"});
    CHECK_EQ(read_text_file(schema_output),
             std::string{
                 "{\"targets\":[{\"part\":\"body\",\"slots\":["
                 "{\"bookmark\":\"customer\",\"kind\":\"text\",\"count\":2},"
                 "{\"bookmark\":\"summary_block\",\"kind\":\"text\"}]}]}\n"});

    remove_if_exists(source);
    remove_if_exists(schema_output);
    remove_if_exists(output);
}

TEST_CASE("cli export-template-schema emits section targets when requested") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_export_template_schema_section_source.docx";
    const fs::path output =
        working_directory / "cli_export_template_schema_section_output.json";

    remove_if_exists(source);
    remove_if_exists(output);

    create_cli_part_template_validation_fixture(source);

    CHECK_EQ(run_cli({"export-template-schema",
                      source.string(),
                      "--section-targets"},
                     output),
             0);
    CHECK_EQ(read_text_file(output),
             std::string{
                 "{\"targets\":["
                 "{\"part\":\"section-header\",\"section\":0,"
                 "\"kind\":\"default\",\"slots\":["
                 "{\"bookmark\":\"header_title\",\"kind\":\"text\"},"
                 "{\"bookmark\":\"header_note\",\"kind\":\"block\"},"
                 "{\"bookmark\":\"header_rows\",\"kind\":\"table_rows\"}]},"
                 "{\"part\":\"section-footer\",\"section\":0,"
                 "\"kind\":\"default\",\"slots\":["
                 "{\"bookmark\":\"footer_company\",\"kind\":\"text\",\"count\":2},"
                 "{\"bookmark\":\"footer_summary\",\"kind\":\"text\"}]}]}\n"});

    remove_if_exists(source);
    remove_if_exists(output);
}

TEST_CASE("cli export-template-schema emits resolved section targets and round-trips") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_export_template_schema_resolved_source.docx";
    const fs::path schema_output =
        working_directory / "cli_export_template_schema_resolved.schema.json";
    const fs::path export_output =
        working_directory / "cli_export_template_schema_resolved_output.json";
    const fs::path validate_output =
        working_directory / "cli_export_template_schema_resolved_validate.json";

    remove_if_exists(source);
    remove_if_exists(schema_output);
    remove_if_exists(export_output);
    remove_if_exists(validate_output);

    create_cli_resolved_part_template_validation_fixture(source);

    CHECK_EQ(run_cli({"export-template-schema",
                      source.string(),
                      "--resolved-section-targets",
                      "--output",
                      schema_output.string(),
                      "--json"},
                     export_output),
             0);
    CHECK_EQ(read_text_file(export_output),
             std::string{
                 "{\"command\":\"export-template-schema\",\"ok\":true,"
                 "\"output_path\":\"" +
                 json_escape_text(schema_output.string()) +
                 "\",\"target_count\":4,\"slot_count\":10,"
                 "\"skipped_count\":0,\"skipped_bookmarks\":[]}\n"});
    CHECK_EQ(read_text_file(schema_output),
             std::string{
                 "{\"targets\":["
                 "{\"part\":\"section-header\",\"section\":0,"
                 "\"kind\":\"default\",\"resolved_from_section\":0,"
                 "\"slots\":[{\"bookmark\":\"header_title\",\"kind\":\"text\"},"
                 "{\"bookmark\":\"header_note\",\"kind\":\"block\"},"
                 "{\"bookmark\":\"header_rows\",\"kind\":\"table_rows\"}]},"
                 "{\"part\":\"section-footer\",\"section\":0,"
                 "\"kind\":\"default\",\"resolved_from_section\":0,"
                 "\"slots\":[{\"bookmark\":\"footer_company\",\"kind\":\"text\",\"count\":2},"
                 "{\"bookmark\":\"footer_summary\",\"kind\":\"text\"}]},"
                 "{\"part\":\"section-header\",\"section\":1,"
                 "\"kind\":\"default\",\"resolved_from_section\":0,"
                 "\"linked_to_previous\":true,"
                 "\"slots\":[{\"bookmark\":\"header_title\",\"kind\":\"text\"},"
                 "{\"bookmark\":\"header_note\",\"kind\":\"block\"},"
                 "{\"bookmark\":\"header_rows\",\"kind\":\"table_rows\"}]},"
                 "{\"part\":\"section-footer\",\"section\":1,"
                 "\"kind\":\"default\",\"resolved_from_section\":0,"
                 "\"linked_to_previous\":true,"
                 "\"slots\":[{\"bookmark\":\"footer_company\",\"kind\":\"text\",\"count\":2},"
                 "{\"bookmark\":\"footer_summary\",\"kind\":\"text\"}]}"
                 "]}\n"});

    CHECK_EQ(run_cli({"validate-template-schema",
                      source.string(),
                      "--schema-file",
                      schema_output.string(),
                      "--json"},
                     validate_output),
             0);
    CHECK_EQ(read_text_file(validate_output),
             std::string{
                 "{\"passed\":true,\"part_results\":["
                 "{\"part\":\"section-header\",\"section\":0,"
                 "\"kind\":\"default\",\"available\":true,"
                 "\"entry_name\":\"word/header1.xml\",\"passed\":true,"
                 "\"missing_required\":[],\"duplicate_bookmarks\":[],"
                 "\"malformed_placeholders\":[],\"unexpected_bookmarks\":[],"
                 "\"kind_mismatches\":[],\"occurrence_mismatches\":[]},"
                 "{\"part\":\"section-footer\",\"section\":0,"
                 "\"kind\":\"default\",\"available\":true,"
                 "\"entry_name\":\"word/footer1.xml\",\"passed\":true,"
                 "\"missing_required\":[],\"duplicate_bookmarks\":[],"
                 "\"malformed_placeholders\":[],\"unexpected_bookmarks\":[],"
                 "\"kind_mismatches\":[],\"occurrence_mismatches\":[]},"
                 "{\"part\":\"section-header\",\"section\":1,"
                 "\"kind\":\"default\",\"available\":true,"
                 "\"entry_name\":\"word/header1.xml\",\"passed\":true,"
                 "\"missing_required\":[],\"duplicate_bookmarks\":[],"
                 "\"malformed_placeholders\":[],\"unexpected_bookmarks\":[],"
                 "\"kind_mismatches\":[],\"occurrence_mismatches\":[]},"
                 "{\"part\":\"section-footer\",\"section\":1,"
                 "\"kind\":\"default\",\"available\":true,"
                 "\"entry_name\":\"word/footer1.xml\",\"passed\":true,"
                 "\"missing_required\":[],\"duplicate_bookmarks\":[],"
                 "\"malformed_placeholders\":[],\"unexpected_bookmarks\":[],"
                 "\"kind_mismatches\":[],\"occurrence_mismatches\":[]}]}\n"});

    remove_if_exists(source);
    remove_if_exists(schema_output);
    remove_if_exists(export_output);
    remove_if_exists(validate_output);
}

TEST_CASE("cli normalize-template-schema writes canonical schema file and summary json") {
    const fs::path working_directory = fs::current_path();
    const fs::path schema_input =
        working_directory / "cli_normalize_template_schema_input.json";
    const fs::path schema_output =
        working_directory / "cli_normalize_template_schema_output.json";
    const fs::path output =
        working_directory / "cli_normalize_template_schema_summary.json";

    remove_if_exists(schema_input);
    remove_if_exists(schema_output);
    remove_if_exists(output);

    write_binary_file(schema_input,
                      std::string{
                          "{"
                          "\"targets\":["
                          "{"
                          "\"part\":\"section-footer\","
                          "\"section\":1,"
                          "\"kind\":\"default\","
                          "\"linked_to_previous\":true,"
                          "\"resolved_from_section\":0,"
                          "\"slots\":["
                          "{\"bookmark\":\"footer_summary\",\"kind\":\"text\"},"
                          "{\"bookmark_name\":\"footer_company\",\"kind\":\"text\","
                          "\"count\":2}"
                          "]"
                          "},"
                          "{"
                          "\"part\":\"body\","
                          "\"slots\":["
                          "{\"bookmark\":\"summary_block\",\"kind\":\"text\"},"
                          "{\"bookmark\":\"customer\",\"kind\":\"text\",\"count\":2}"
                          "]"
                          "},"
                          "{"
                          "\"part\":\"section-header\","
                          "\"section\":1,"
                          "\"kind\":\"default\","
                          "\"resolved_from_section\":0,"
                          "\"linked_to_previous\":true,"
                          "\"slots\":["
                          "{\"bookmark\":\"header_title\",\"kind\":\"text\"},"
                          "{\"bookmark\":\"header_note\",\"kind\":\"block\"}"
                          "]"
                          "}"
                          "]"
                          "}"});

    CHECK_EQ(run_cli({"normalize-template-schema",
                      schema_input.string(),
                      "--output",
                      schema_output.string(),
                      "--json"},
                     output),
             0);
    CHECK_EQ(read_text_file(output),
             std::string{
                 "{\"command\":\"normalize-template-schema\",\"ok\":true,"
                 "\"output_path\":\"" +
                 json_escape_text(schema_output.string()) +
                 "\",\"target_count\":3,\"slot_count\":6}\n"});
    CHECK_EQ(read_text_file(schema_output),
             std::string{
                 "{\"targets\":["
                 "{\"part\":\"body\",\"slots\":["
                 "{\"bookmark\":\"customer\",\"kind\":\"text\",\"count\":2},"
                 "{\"bookmark\":\"summary_block\",\"kind\":\"text\"}]},"
                 "{\"part\":\"section-header\",\"section\":1,"
                 "\"kind\":\"default\",\"resolved_from_section\":0,"
                 "\"linked_to_previous\":true,\"slots\":["
                 "{\"bookmark\":\"header_note\",\"kind\":\"block\"},"
                 "{\"bookmark\":\"header_title\",\"kind\":\"text\"}]},"
                 "{\"part\":\"section-footer\",\"section\":1,"
                 "\"kind\":\"default\",\"resolved_from_section\":0,"
                 "\"linked_to_previous\":true,\"slots\":["
                 "{\"bookmark\":\"footer_company\",\"kind\":\"text\",\"count\":2},"
                 "{\"bookmark\":\"footer_summary\",\"kind\":\"text\"}]}]}\n"});

    remove_if_exists(schema_input);
    remove_if_exists(schema_output);
    remove_if_exists(output);
}

TEST_CASE("cli lint-template-schema reports clean normalized schemas") {
    const fs::path working_directory = fs::current_path();
    const fs::path schema_input =
        working_directory / "cli_lint_template_schema_clean_input.json";
    const fs::path output =
        working_directory / "cli_lint_template_schema_clean_output.json";

    remove_if_exists(schema_input);
    remove_if_exists(output);

    write_binary_file(schema_input,
                      std::string{
                          "{\"targets\":[{\"part\":\"body\",\"slots\":["
                          "{\"bookmark\":\"customer\",\"kind\":\"text\"},"
                          "{\"bookmark\":\"invoice_no\",\"kind\":\"text\",\"count\":2}"
                          "]}]}\n"});

    CHECK_EQ(run_cli({"lint-template-schema", schema_input.string(), "--json"}, output),
             0);
    CHECK_EQ(read_text_file(output),
             std::string{
                 "{\"command\":\"lint-template-schema\",\"ok\":true,"
                 "\"clean\":true,\"target_count\":1,\"slot_count\":2,"
                 "\"issue_count\":0,\"duplicate_target_count\":0,"
                 "\"duplicate_slot_count\":0,\"target_order_issue_count\":0,"
                 "\"slot_order_issue_count\":0,"
                 "\"entry_name_issue_count\":0,\"issues\":[]}\n"});

    remove_if_exists(schema_input);
    remove_if_exists(output);
}

TEST_CASE("cli lint-template-schema reports structural issues as json") {
    const fs::path working_directory = fs::current_path();
    const fs::path schema_input =
        working_directory / "cli_lint_template_schema_dirty_input.json";
    const fs::path output =
        working_directory / "cli_lint_template_schema_dirty_output.json";

    remove_if_exists(schema_input);
    remove_if_exists(output);

    write_binary_file(schema_input,
                      std::string{
                          "{"
                          "\"targets\":["
                          "{"
                          "\"part\":\"section-header\","
                          "\"section\":1,"
                          "\"kind\":\"default\","
                          "\"entry_name\":\"word/header1.xml\","
                          "\"slots\":["
                          "{\"bookmark\":\"zeta\",\"kind\":\"text\"},"
                          "{\"bookmark\":\"alpha\",\"kind\":\"text\"},"
                          "{\"bookmark\":\"alpha\",\"kind\":\"text\"}"
                          "]"
                          "},"
                          "{"
                          "\"part\":\"body\","
                          "\"slots\":[{\"bookmark\":\"account_id\",\"kind\":\"text\"}]"
                          "},"
                          "{"
                          "\"part\":\"body\","
                          "\"slots\":[{\"bookmark\":\"customer\",\"kind\":\"text\"}]"
                          "}"
                          "]"
                          "}"});

    CHECK_EQ(run_cli({"lint-template-schema", schema_input.string(), "--json"}, output),
             1);
    CHECK_EQ(
        read_text_file(output),
        std::string{
            "{\"command\":\"lint-template-schema\",\"ok\":true,"
            "\"clean\":false,\"target_count\":3,\"slot_count\":5,"
            "\"issue_count\":5,\"duplicate_target_count\":1,"
            "\"duplicate_slot_count\":1,\"target_order_issue_count\":1,"
            "\"slot_order_issue_count\":1,"
            "\"entry_name_issue_count\":1,\"issues\":["
            "{\"issue\":\"entry_name_present\",\"target_index\":0,"
            "\"target\":{\"part\":\"section-header\",\"section\":1,"
            "\"kind\":\"default\"},\"entry_name\":\"word/header1.xml\"},"
            "{\"issue\":\"slot_order\",\"target_index\":0,"
            "\"target\":{\"part\":\"section-header\",\"section\":1,"
            "\"kind\":\"default\"},\"slot_index\":1,\"previous_slot_index\":0,"
            "\"bookmark\":\"alpha\",\"previous_bookmark\":\"zeta\"},"
            "{\"issue\":\"duplicate_slot_name\",\"target_index\":0,"
            "\"target\":{\"part\":\"section-header\",\"section\":1,"
            "\"kind\":\"default\"},\"slot_index\":2,\"previous_slot_index\":1,"
            "\"bookmark\":\"alpha\"},"
            "{\"issue\":\"target_order\",\"target_index\":1,"
            "\"target\":{\"part\":\"body\"},\"previous_target_index\":0},"
            "{\"issue\":\"duplicate_target_identity\",\"target_index\":2,"
            "\"target\":{\"part\":\"body\"},\"previous_target_index\":1}]}\n"});

    remove_if_exists(schema_input);
    remove_if_exists(output);
}

TEST_CASE("cli lint-template-schema reports parse errors") {
    const fs::path working_directory = fs::current_path();
    const fs::path schema_input =
        working_directory / "cli_lint_template_schema_parse_input.json";
    const fs::path output =
        working_directory / "cli_lint_template_schema_parse_output.json";

    remove_if_exists(schema_input);
    remove_if_exists(output);

    write_binary_file(schema_input,
                      std::string{
                          "{\"targets\":[{\"part\":\"body\",\"slots\":["
                          "{\"bookmark\":\"customer\",\"kind\":\"text\"}]}]}\n"});

    CHECK_EQ(run_cli({"lint-template-schema", "--json"}, output), 2);
    CHECK_EQ(read_text_file(output),
             std::string{
                 "{\"command\":\"lint-template-schema\",\"ok\":false,"
                 "\"stage\":\"parse\",\"message\":\"lint-template-schema "
                 "expects a schema path\"}\n"});

    CHECK_EQ(run_cli({"lint-template-schema",
                      schema_input.string(),
                      "--json",
                      "--bad-option"},
                     output),
             2);
    CHECK_EQ(read_text_file(output),
             std::string{
                 "{\"command\":\"lint-template-schema\",\"ok\":false,"
                 "\"stage\":\"parse\",\"message\":\"unknown option: "
                 "--bad-option\"}\n"});

    remove_if_exists(schema_input);
    remove_if_exists(output);
}

TEST_CASE("cli repair-template-schema canonicalizes and deduplicates schema") {
    const fs::path working_directory = fs::current_path();
    const fs::path schema_input =
        working_directory / "cli_repair_template_schema_input.json";
    const fs::path schema_output =
        working_directory / "cli_repair_template_schema_output.json";
    const fs::path output =
        working_directory / "cli_repair_template_schema_summary.json";

    remove_if_exists(schema_input);
    remove_if_exists(schema_output);
    remove_if_exists(output);

    write_binary_file(schema_input,
                      std::string{
                          "{"
                          "\"targets\":["
                          "{"
                          "\"part\":\"section-header\","
                          "\"section\":1,"
                          "\"kind\":\"default\","
                          "\"entry_name\":\"word/header1.xml\","
                          "\"slots\":["
                          "{\"bookmark\":\"zeta\",\"kind\":\"text\"},"
                          "{\"bookmark\":\"alpha\",\"kind\":\"text\"},"
                          "{\"bookmark\":\"alpha\",\"kind\":\"text\",\"count\":2}"
                          "]"
                          "},"
                          "{"
                          "\"part\":\"body\","
                          "\"slots\":[{\"bookmark\":\"invoice_no\",\"kind\":\"text\"}]"
                          "},"
                          "{"
                          "\"part\":\"body\","
                          "\"entry_name\":\"word/document.xml\","
                          "\"slots\":["
                          "{\"bookmark\":\"customer\",\"kind\":\"text\"},"
                          "{\"bookmark\":\"invoice_no\",\"kind\":\"text\",\"count\":2}"
                          "]"
                          "}"
                          "]"
                          "}"});

    CHECK_EQ(run_cli({"repair-template-schema",
                      schema_input.string(),
                      "--output",
                      schema_output.string(),
                      "--json"},
                     output),
             0);
    CHECK_EQ(read_text_file(output),
             std::string{
                 "{\"command\":\"repair-template-schema\",\"ok\":true,"
                 "\"output_path\":\"" +
                 json_escape_text(schema_output.string()) +
                 "\",\"input_target_count\":3,\"input_slot_count\":6,"
                 "\"target_count\":2,\"slot_count\":4,"
                 "\"merged_duplicate_target_count\":1,"
                 "\"deduplicated_target_count\":1,"
                 "\"deduplicated_slot_count\":2,"
                 "\"stripped_entry_name_count\":2,"
                 "\"replaced_slot_count\":2,\"changed\":true}\n"});
    CHECK_EQ(read_text_file(schema_output),
             std::string{
                 "{\"targets\":["
                 "{\"part\":\"body\",\"slots\":["
                 "{\"bookmark\":\"customer\",\"kind\":\"text\"},"
                 "{\"bookmark\":\"invoice_no\",\"kind\":\"text\",\"count\":2}]},"
                 "{\"part\":\"section-header\",\"section\":1,"
                 "\"kind\":\"default\",\"slots\":["
                 "{\"bookmark\":\"alpha\",\"kind\":\"text\",\"count\":2},"
                 "{\"bookmark\":\"zeta\",\"kind\":\"text\"}]}]}\n"});

    remove_if_exists(schema_input);
    remove_if_exists(schema_output);
    remove_if_exists(output);
}

TEST_CASE("cli repair-template-schema reports parse errors") {
    const fs::path working_directory = fs::current_path();
    const fs::path schema_input =
        working_directory / "cli_repair_template_schema_parse_input.json";
    const fs::path output =
        working_directory / "cli_repair_template_schema_parse_output.json";

    remove_if_exists(schema_input);
    remove_if_exists(output);

    write_binary_file(schema_input,
                      std::string{
                          "{\"targets\":[{\"part\":\"body\",\"slots\":["
                          "{\"bookmark\":\"customer\",\"kind\":\"text\"}]}]}\n"});

    CHECK_EQ(run_cli({"repair-template-schema", "--json"}, output), 2);
    CHECK_EQ(read_text_file(output),
             std::string{
                 "{\"command\":\"repair-template-schema\",\"ok\":false,"
                 "\"stage\":\"parse\",\"message\":\"repair-template-schema "
                 "expects a schema path\"}\n"});

    CHECK_EQ(run_cli({"repair-template-schema",
                      schema_input.string(),
                      "--json",
                      "--bad-option"},
                     output),
             2);
    CHECK_EQ(read_text_file(output),
             std::string{
                 "{\"command\":\"repair-template-schema\",\"ok\":false,"
                 "\"stage\":\"parse\",\"message\":\"unknown option: "
                 "--bad-option\"}\n"});

    remove_if_exists(schema_input);
    remove_if_exists(output);
}

TEST_CASE("cli merge-template-schema merges targets and replaces later slot definitions") {
    const fs::path working_directory = fs::current_path();
    const fs::path left_schema =
        working_directory / "cli_merge_template_schema_left.json";
    const fs::path right_schema =
        working_directory / "cli_merge_template_schema_right.json";
    const fs::path merged_schema =
        working_directory / "cli_merge_template_schema_output.json";
    const fs::path output =
        working_directory / "cli_merge_template_schema_summary.json";

    remove_if_exists(left_schema);
    remove_if_exists(right_schema);
    remove_if_exists(merged_schema);
    remove_if_exists(output);

    write_binary_file(left_schema,
                      std::string{
                          "{"
                          "\"targets\":["
                          "{"
                          "\"part\":\"section-header\","
                          "\"section\":1,"
                          "\"kind\":\"default\","
                          "\"resolved_from_section\":0,"
                          "\"linked_to_previous\":true,"
                          "\"slots\":[{\"bookmark\":\"header_title\",\"kind\":\"text\"}]"
                          "},"
                          "{"
                          "\"part\":\"body\","
                          "\"slots\":["
                          "{\"bookmark\":\"summary_block\",\"kind\":\"text\"},"
                          "{\"bookmark\":\"customer\",\"kind\":\"text\"}"
                          "]"
                          "}"
                          "]"
                          "}"});
    write_binary_file(right_schema,
                      std::string{
                          "{"
                          "\"targets\":["
                          "{"
                          "\"part\":\"section-footer\","
                          "\"section\":1,"
                          "\"kind\":\"default\","
                          "\"slots\":[{\"bookmark\":\"footer_summary\",\"kind\":\"text\"}]"
                          "},"
                          "{"
                          "\"part\":\"body\","
                          "\"slots\":["
                          "{\"bookmark\":\"customer\",\"kind\":\"text\",\"count\":2},"
                          "{\"bookmark\":\"invoice_no\",\"kind\":\"text\"}"
                          "]"
                          "}"
                          "]"
                          "}"});

    CHECK_EQ(run_cli({"merge-template-schema",
                      left_schema.string(),
                      right_schema.string(),
                      "--output",
                      merged_schema.string(),
                      "--json"},
                     output),
             0);
    CHECK_EQ(read_text_file(output),
             std::string{
                 "{\"command\":\"merge-template-schema\",\"ok\":true,"
                 "\"output_path\":\"" +
                 json_escape_text(merged_schema.string()) +
                 "\",\"input_count\":2,\"target_count\":3,\"slot_count\":5,"
                 "\"updated_target_count\":1,\"replaced_slot_count\":1}\n"});
    CHECK_EQ(read_text_file(merged_schema),
             std::string{
                 "{\"targets\":["
                 "{\"part\":\"body\",\"slots\":["
                 "{\"bookmark\":\"customer\",\"kind\":\"text\",\"count\":2},"
                 "{\"bookmark\":\"invoice_no\",\"kind\":\"text\"},"
                 "{\"bookmark\":\"summary_block\",\"kind\":\"text\"}]},"
                 "{\"part\":\"section-header\",\"section\":1,"
                 "\"kind\":\"default\",\"resolved_from_section\":0,"
                 "\"linked_to_previous\":true,\"slots\":[{\"bookmark\":"
                 "\"header_title\",\"kind\":\"text\"}]},"
                 "{\"part\":\"section-footer\",\"section\":1,"
                 "\"kind\":\"default\",\"slots\":[{\"bookmark\":"
                 "\"footer_summary\",\"kind\":\"text\"}]}]}\n"});

    remove_if_exists(left_schema);
    remove_if_exists(right_schema);
    remove_if_exists(merged_schema);
    remove_if_exists(output);
}

TEST_CASE("cli merge-template-schema reports parse errors") {
    const fs::path working_directory = fs::current_path();
    const fs::path schema =
        working_directory / "cli_merge_template_schema_parse_schema.json";
    const fs::path output =
        working_directory / "cli_merge_template_schema_parse_output.json";

    remove_if_exists(schema);
    remove_if_exists(output);

    write_binary_file(schema,
                      std::string{
                          "{\"targets\":[{\"part\":\"body\",\"slots\":["
                          "{\"bookmark\":\"customer\",\"kind\":\"text\"}]}]}\n"});

    CHECK_EQ(run_cli({"merge-template-schema", schema.string(), "--json"}, output), 2);
    CHECK_EQ(read_text_file(output),
             std::string{
                 "{\"command\":\"merge-template-schema\",\"ok\":false,"
                 "\"stage\":\"parse\",\"message\":\"merge-template-schema "
                 "expects at least two schema paths\"}\n"});

    remove_if_exists(schema);
    remove_if_exists(output);
}

TEST_CASE("cli diff-template-schema reports added removed and changed targets as json") {
    const fs::path working_directory = fs::current_path();
    const fs::path left_schema =
        working_directory / "cli_diff_template_schema_left.json";
    const fs::path right_schema =
        working_directory / "cli_diff_template_schema_right.json";
    const fs::path output =
        working_directory / "cli_diff_template_schema_output.json";

    remove_if_exists(left_schema);
    remove_if_exists(right_schema);
    remove_if_exists(output);

    write_binary_file(left_schema,
                      std::string{
                          "{"
                          "\"targets\":["
                          "{"
                          "\"part\":\"section-header\","
                          "\"section\":1,"
                          "\"kind\":\"default\","
                          "\"resolved_from_section\":0,"
                          "\"linked_to_previous\":true,"
                          "\"slots\":["
                          "{\"bookmark\":\"header_title\",\"kind\":\"text\"}"
                          "]"
                          "},"
                          "{"
                          "\"part\":\"body\","
                          "\"slots\":["
                          "{\"bookmark\":\"customer\",\"kind\":\"text\",\"count\":2}"
                          "]"
                          "}"
                          "]"
                          "}"});
    write_binary_file(right_schema,
                      std::string{
                          "{"
                          "\"targets\":["
                          "{"
                          "\"part\":\"section-footer\","
                          "\"section\":1,"
                          "\"kind\":\"default\","
                          "\"slots\":["
                          "{\"bookmark\":\"footer_summary\",\"kind\":\"text\"}"
                          "]"
                          "},"
                          "{"
                          "\"part\":\"body\","
                          "\"slots\":["
                          "{\"bookmark\":\"customer\",\"kind\":\"text\",\"count\":3}"
                          "]"
                          "}"
                          "]"
                          "}"});

    CHECK_EQ(run_cli({"diff-template-schema",
                      left_schema.string(),
                      right_schema.string(),
                      "--json"},
                     output),
             0);
    CHECK_EQ(read_text_file(output),
             std::string{
                 "{\"equal\":false,\"added_target_count\":1,"
                 "\"removed_target_count\":1,\"changed_target_count\":1,"
                 "\"added_targets\":[{\"part\":\"section-footer\",\"section\":1,"
                 "\"kind\":\"default\",\"slots\":[{\"bookmark\":\"footer_summary\","
                 "\"kind\":\"text\"}]}],"
                 "\"removed_targets\":[{\"part\":\"section-header\",\"section\":1,"
                 "\"kind\":\"default\",\"resolved_from_section\":0,"
                 "\"linked_to_previous\":true,\"slots\":[{\"bookmark\":"
                 "\"header_title\",\"kind\":\"text\"}]}],"
                 "\"changed_targets\":[{\"left\":{\"part\":\"body\",\"slots\":["
                 "{\"bookmark\":\"customer\",\"kind\":\"text\",\"count\":2}]},"
                 "\"right\":{\"part\":\"body\",\"slots\":[{\"bookmark\":"
                 "\"customer\",\"kind\":\"text\",\"count\":3}]}}]}\n"});

    remove_if_exists(left_schema);
    remove_if_exists(right_schema);
    remove_if_exists(output);
}

TEST_CASE("cli diff-template-schema supports fail-on-diff gate exit code") {
    const fs::path working_directory = fs::current_path();
    const fs::path left_schema =
        working_directory / "cli_diff_template_schema_fail_left.json";
    const fs::path right_schema =
        working_directory / "cli_diff_template_schema_fail_right.json";
    const fs::path output =
        working_directory / "cli_diff_template_schema_fail_output.json";

    remove_if_exists(left_schema);
    remove_if_exists(right_schema);
    remove_if_exists(output);

    write_binary_file(left_schema,
                      std::string{
                          "{"
                          "\"targets\":[{"
                          "\"part\":\"body\","
                          "\"slots\":[{\"bookmark\":\"customer\",\"kind\":\"text\"}]"
                          "}]"
                          "}"});
    write_binary_file(right_schema,
                      std::string{
                          "{"
                          "\"targets\":[{"
                          "\"part\":\"body\","
                          "\"slots\":[{\"bookmark\":\"customer\",\"kind\":\"text\","
                          "\"count\":2}]"
                          "}]"
                          "}"});

    CHECK_EQ(run_cli({"diff-template-schema",
                      left_schema.string(),
                      right_schema.string(),
                      "--fail-on-diff",
                      "--json"},
                     output),
             1);
    CHECK_EQ(read_text_file(output),
             std::string{
                 "{\"equal\":false,\"added_target_count\":0,"
                 "\"removed_target_count\":0,\"changed_target_count\":1,"
                 "\"added_targets\":[],\"removed_targets\":[],"
                 "\"changed_targets\":[{\"left\":{\"part\":\"body\",\"slots\":["
                 "{\"bookmark\":\"customer\",\"kind\":\"text\"}]},"
                 "\"right\":{\"part\":\"body\",\"slots\":[{\"bookmark\":"
                 "\"customer\",\"kind\":\"text\",\"count\":2}]}}]}\n"});

    remove_if_exists(left_schema);
    remove_if_exists(right_schema);
    remove_if_exists(output);
}

TEST_CASE("cli check-template-schema matches resolved section baseline and writes generated schema") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_check_template_schema_match_source.docx";
    const fs::path schema_file =
        working_directory / "cli_check_template_schema_match_schema.json";
    const fs::path generated_output =
        working_directory / "cli_check_template_schema_match_generated.json";
    const fs::path output =
        working_directory / "cli_check_template_schema_match_output.json";

    remove_if_exists(source);
    remove_if_exists(schema_file);
    remove_if_exists(generated_output);
    remove_if_exists(output);

    create_cli_resolved_part_template_validation_fixture(source);

    write_binary_file(schema_file,
                      std::string{
                          "{\"targets\":["
                          "{\"part\":\"section-header\",\"section\":0,"
                          "\"kind\":\"default\",\"resolved_from_section\":0,"
                          "\"slots\":[{\"bookmark\":\"header_note\",\"kind\":\"block\"},"
                          "{\"bookmark\":\"header_rows\",\"kind\":\"table_rows\"},"
                          "{\"bookmark\":\"header_title\",\"kind\":\"text\"}]},"
                          "{\"part\":\"section-footer\",\"section\":0,"
                          "\"kind\":\"default\",\"resolved_from_section\":0,"
                          "\"slots\":[{\"bookmark\":\"footer_company\",\"kind\":\"text\",\"count\":2},"
                          "{\"bookmark\":\"footer_summary\",\"kind\":\"text\"}]},"
                          "{\"part\":\"section-header\",\"section\":1,"
                          "\"kind\":\"default\",\"resolved_from_section\":0,"
                          "\"linked_to_previous\":true,"
                          "\"slots\":[{\"bookmark\":\"header_note\",\"kind\":\"block\"},"
                          "{\"bookmark\":\"header_rows\",\"kind\":\"table_rows\"},"
                          "{\"bookmark\":\"header_title\",\"kind\":\"text\"}]},"
                          "{\"part\":\"section-footer\",\"section\":1,"
                          "\"kind\":\"default\",\"resolved_from_section\":0,"
                          "\"linked_to_previous\":true,"
                          "\"slots\":[{\"bookmark\":\"footer_company\",\"kind\":\"text\",\"count\":2},"
                          "{\"bookmark\":\"footer_summary\",\"kind\":\"text\"}]}]}\n"});

    CHECK_EQ(run_cli({"check-template-schema",
                      source.string(),
                      "--schema-file",
                      schema_file.string(),
                      "--resolved-section-targets",
                      "--output",
                      generated_output.string(),
                      "--json"},
                     output),
             0);
    CHECK_EQ(read_text_file(output),
             std::string{
                 "{\"command\":\"check-template-schema\",\"matches\":true,"
                 "\"schema_file\":\"" +
                 json_escape_text(schema_file.string()) +
                 "\",\"generated_output_path\":\"" +
                 json_escape_text(generated_output.string()) +
                 "\",\"added_target_count\":0,\"removed_target_count\":0,"
                 "\"changed_target_count\":0,\"added_targets\":[],"
                 "\"removed_targets\":[],\"changed_targets\":[]}\n"});
    CHECK_EQ(read_text_file(generated_output),
             std::string{
                 "{\"targets\":["
                 "{\"part\":\"section-header\",\"section\":0,"
                 "\"kind\":\"default\",\"resolved_from_section\":0,"
                 "\"slots\":[{\"bookmark\":\"header_note\",\"kind\":\"block\"},"
                 "{\"bookmark\":\"header_rows\",\"kind\":\"table_rows\"},"
                 "{\"bookmark\":\"header_title\",\"kind\":\"text\"}]},"
                 "{\"part\":\"section-header\",\"section\":1,"
                 "\"kind\":\"default\",\"resolved_from_section\":0,"
                 "\"linked_to_previous\":true,"
                 "\"slots\":[{\"bookmark\":\"header_note\",\"kind\":\"block\"},"
                 "{\"bookmark\":\"header_rows\",\"kind\":\"table_rows\"},"
                 "{\"bookmark\":\"header_title\",\"kind\":\"text\"}]},"
                 "{\"part\":\"section-footer\",\"section\":0,"
                 "\"kind\":\"default\",\"resolved_from_section\":0,"
                 "\"slots\":[{\"bookmark\":\"footer_company\",\"kind\":\"text\",\"count\":2},"
                 "{\"bookmark\":\"footer_summary\",\"kind\":\"text\"}]},"
                 "{\"part\":\"section-footer\",\"section\":1,"
                 "\"kind\":\"default\",\"resolved_from_section\":0,"
                 "\"linked_to_previous\":true,"
                 "\"slots\":[{\"bookmark\":\"footer_company\",\"kind\":\"text\",\"count\":2},"
                 "{\"bookmark\":\"footer_summary\",\"kind\":\"text\"}]}]}\n"});

    remove_if_exists(source);
    remove_if_exists(schema_file);
    remove_if_exists(generated_output);
    remove_if_exists(output);
}

TEST_CASE("cli check-template-schema fails when generated schema drifts from baseline") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_check_template_schema_drift_source.docx";
    const fs::path schema_file =
        working_directory / "cli_check_template_schema_drift_schema.json";
    const fs::path output =
        working_directory / "cli_check_template_schema_drift_output.json";

    remove_if_exists(source);
    remove_if_exists(schema_file);
    remove_if_exists(output);

    create_cli_body_template_validation_fixture(source);

    write_binary_file(schema_file,
                      std::string{
                          "{\"targets\":[{\"part\":\"body\",\"slots\":["
                          "{\"bookmark\":\"customer\",\"kind\":\"text\"},"
                          "{\"bookmark\":\"summary_block\",\"kind\":\"text\"}]}]}\n"});

    CHECK_EQ(run_cli({"check-template-schema",
                      source.string(),
                      "--schema-file",
                      schema_file.string(),
                      "--json"},
                     output),
             1);
    CHECK_EQ(read_text_file(output),
             std::string{
                 "{\"command\":\"check-template-schema\",\"matches\":false,"
                 "\"schema_file\":\"" +
                 json_escape_text(schema_file.string()) +
                 "\",\"added_target_count\":0,\"removed_target_count\":0,"
                 "\"changed_target_count\":1,\"added_targets\":[],"
                 "\"removed_targets\":[],\"changed_targets\":[{\"left\":"
                 "{\"part\":\"body\",\"slots\":[{\"bookmark\":\"customer\","
                 "\"kind\":\"text\"},{\"bookmark\":\"summary_block\","
                 "\"kind\":\"text\"}]},\"right\":{\"part\":\"body\",\"slots\":["
                 "{\"bookmark\":\"customer\",\"kind\":\"text\",\"count\":2},"
                 "{\"bookmark\":\"summary_block\",\"kind\":\"text\"}]}}]}\n"});

    remove_if_exists(source);
    remove_if_exists(schema_file);
    remove_if_exists(output);
}
