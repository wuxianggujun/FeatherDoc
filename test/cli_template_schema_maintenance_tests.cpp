#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "cli_template_schema_test_support.hpp"

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
