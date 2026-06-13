#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "cli_template_schema_test_support.hpp"

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
