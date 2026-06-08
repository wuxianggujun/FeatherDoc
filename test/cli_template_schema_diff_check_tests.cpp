#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "cli_template_schema_test_support.hpp"

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
