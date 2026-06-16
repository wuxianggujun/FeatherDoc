#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "cli_template_schema_test_support.hpp"

TEST_CASE("cli patch-template-schema applies upserts removals and slot pruning") {
    const fs::path working_directory = fs::current_path();
    const fs::path schema_input =
        working_directory / "cli_patch_template_schema_input.json";
    const fs::path patch_input =
        working_directory / "cli_patch_template_schema_patch.json";
    const fs::path schema_output =
        working_directory / "cli_patch_template_schema_output.json";
    const fs::path output =
        working_directory / "cli_patch_template_schema_summary.json";

    remove_if_exists(schema_input);
    remove_if_exists(patch_input);
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
                          "\"resolved_from_section\":0,"
                          "\"linked_to_previous\":true,"
                          "\"slots\":[{\"bookmark\":\"header_title\",\"kind\":\"text\"}]"
                          "},"
                          "{"
                          "\"part\":\"section-footer\","
                          "\"section\":1,"
                          "\"kind\":\"default\","
                          "\"slots\":[{\"bookmark\":\"footer_summary\",\"kind\":\"text\"}]"
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
    write_binary_file(patch_input,
                      std::string{
                          "{"
                          "\"remove_targets\":[{"
                          "\"part\":\"section-footer\","
                          "\"section\":1,"
                          "\"kind\":\"default\""
                          "}],"
                          "\"remove_slots\":[{"
                          "\"part\":\"body\","
                          "\"bookmark\":\"summary_block\""
                          "}],"
                          "\"rename_slots\":[{"
                          "\"part\":\"section-header\","
                          "\"section\":1,"
                          "\"kind\":\"default\","
                          "\"resolved_from_section\":0,"
                          "\"linked_to_previous\":true,"
                          "\"bookmark\":\"header_title\","
                          "\"new_bookmark\":\"document_title\""
                          "}],"
                          "\"upsert_targets\":[{"
                          "\"part\":\"body\","
                          "\"slots\":["
                          "{\"bookmark\":\"customer\",\"kind\":\"text\",\"count\":2},"
                          "{\"bookmark\":\"invoice_no\",\"kind\":\"text\"}"
                          "]"
                          "}]"
                          "}"});

    CHECK_EQ(run_cli({"patch-template-schema",
                      schema_input.string(),
                      "--patch-file",
                      patch_input.string(),
                      "--output",
                      schema_output.string(),
                      "--json"},
                     output),
             0);
    CHECK_EQ(read_text_file(output),
             std::string{
                 "{\"command\":\"patch-template-schema\",\"ok\":true,"
                 "\"output_path\":\"" +
                 json_escape_text(schema_output.string()) +
                 "\",\"target_count\":2,\"slot_count\":3,"
                 "\"upsert_target_count\":1,\"remove_target_count\":1,"
                 "\"remove_slot_count\":1,\"rename_slot_count\":1,"
                 "\"update_slot_count\":0,\"updated_target_count\":1,"
                 "\"replaced_slot_count\":1,"
                 "\"applied_remove_target_count\":1,"
                 "\"applied_remove_slot_count\":1,"
                 "\"applied_rename_slot_count\":1,"
                 "\"applied_update_slot_count\":0,"
                 "\"pruned_empty_target_count\":0}\n"});
    CHECK_EQ(read_text_file(schema_output),
             std::string{
                 "{\"targets\":["
                 "{\"part\":\"body\",\"slots\":["
                 "{\"bookmark\":\"customer\",\"kind\":\"text\",\"count\":2},"
                  "{\"bookmark\":\"invoice_no\",\"kind\":\"text\"}]},"
                 "{\"part\":\"section-header\",\"section\":1,"
                  "\"kind\":\"default\",\"resolved_from_section\":0,"
                  "\"linked_to_previous\":true,\"slots\":[{\"bookmark\":"
                 "\"document_title\",\"kind\":\"text\"}]}]}\n"});

    remove_if_exists(schema_input);
    remove_if_exists(patch_input);
    remove_if_exists(schema_output);
    remove_if_exists(output);
}

TEST_CASE("cli patch-template-schema applies content control slot selectors") {
    const fs::path working_directory = fs::current_path();
    const fs::path schema_input =
        working_directory / "cli_patch_template_schema_content_controls_input.json";
    const fs::path patch_input =
        working_directory / "cli_patch_template_schema_content_controls_patch.json";
    const fs::path schema_output =
        working_directory / "cli_patch_template_schema_content_controls_output.json";
    const fs::path output =
        working_directory / "cli_patch_template_schema_content_controls_summary.json";

    remove_if_exists(schema_input);
    remove_if_exists(patch_input);
    remove_if_exists(schema_output);
    remove_if_exists(output);

    write_binary_file(schema_input,
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
    write_binary_file(patch_input,
                      std::string{
                          "{"
                          "\"remove_slots\":[{"
                          "\"part\":\"body\","
                          "\"content_control_alias\":\"Line Items\""
                          "}],"
                          "\"rename_slots\":[{"
                          "\"part\":\"body\","
                          "\"content_control_tag\":\"order_no\","
                          "\"new_content_control_tag\":\"order_id\""
                          "}]"
                          "}"});

    CHECK_EQ(run_cli({"patch-template-schema",
                      schema_input.string(),
                      "--patch-file",
                      patch_input.string(),
                      "--output",
                      schema_output.string(),
                      "--json"},
                     output),
             0);
    CHECK_EQ(read_text_file(output),
             std::string{
                 "{\"command\":\"patch-template-schema\",\"ok\":true,"
                 "\"output_path\":\"" +
                 json_escape_text(schema_output.string()) +
                 "\",\"target_count\":1,\"slot_count\":1,"
                 "\"upsert_target_count\":0,\"remove_target_count\":0,"
                 "\"remove_slot_count\":1,\"rename_slot_count\":1,"
                 "\"update_slot_count\":0,\"updated_target_count\":0,"
                 "\"replaced_slot_count\":0,"
                 "\"applied_remove_target_count\":0,"
                 "\"applied_remove_slot_count\":1,"
                 "\"applied_rename_slot_count\":1,"
                 "\"applied_update_slot_count\":0,"
                 "\"pruned_empty_target_count\":0}\n"});
    CHECK_EQ(read_text_file(schema_output),
             std::string{
                 "{\"targets\":[{\"part\":\"body\",\"slots\":["
                 "{\"content_control_tag\":\"order_id\",\"kind\":\"text\"}]}]}\n"});

    remove_if_exists(schema_input);
    remove_if_exists(patch_input);
    remove_if_exists(schema_output);
    remove_if_exists(output);
}

TEST_CASE("cli patch-template-schema updates slot properties") {
    const fs::path working_directory = fs::current_path();
    const fs::path schema_input =
        working_directory / "cli_patch_template_schema_update_slots_input.json";
    const fs::path patch_input =
        working_directory / "cli_patch_template_schema_update_slots_patch.json";
    const fs::path schema_output =
        working_directory / "cli_patch_template_schema_update_slots_output.json";
    const fs::path output =
        working_directory / "cli_patch_template_schema_update_slots_summary.json";

    remove_if_exists(schema_input);
    remove_if_exists(patch_input);
    remove_if_exists(schema_output);
    remove_if_exists(output);

    write_binary_file(schema_input,
                      std::string{
                          "{"
                          "\"targets\":[{"
                          "\"part\":\"body\","
                          "\"slots\":["
                          "{\"bookmark\":\"customer\",\"kind\":\"text\",\"count\":1},"
                          "{\"content_control_tag\":\"status\",\"kind\":\"text\",\"count\":1}"
                          "]"
                          "}]"
                          "}"});
    write_binary_file(patch_input,
                      std::string{
                          "{"
                          "\"update_slots\":["
                          "{\"part\":\"body\",\"bookmark\":\"customer\","
                          "\"kind\":\"block\",\"required\":false,"
                          "\"min_occurrences\":2,\"max_occurrences\":5},"
                          "{\"part\":\"body\",\"content_control_tag\":\"status\","
                          "\"clear_min_occurrences\":true,"
                          "\"clear_max_occurrences\":true}"
                          "]"
                          "}"});

    CHECK_EQ(run_cli({"patch-template-schema",
                      schema_input.string(),
                      "--patch-file",
                      patch_input.string(),
                      "--output",
                      schema_output.string(),
                      "--json"},
                     output),
             0);
    CHECK_EQ(read_text_file(output),
             std::string{
                 "{\"command\":\"patch-template-schema\",\"ok\":true,"
                 "\"output_path\":\"" +
                 json_escape_text(schema_output.string()) +
                 "\",\"target_count\":1,\"slot_count\":2,"
                 "\"upsert_target_count\":0,\"remove_target_count\":0,"
                 "\"remove_slot_count\":0,\"rename_slot_count\":0,"
                 "\"update_slot_count\":2,\"updated_target_count\":0,"
                 "\"replaced_slot_count\":0,"
                 "\"applied_remove_target_count\":0,"
                 "\"applied_remove_slot_count\":0,"
                 "\"applied_rename_slot_count\":0,"
                 "\"applied_update_slot_count\":2,"
                 "\"pruned_empty_target_count\":0}\n"});
    CHECK_EQ(read_text_file(schema_output),
             std::string{
                 "{\"targets\":[{\"part\":\"body\",\"slots\":["
                 "{\"bookmark\":\"customer\",\"kind\":\"block\","
                 "\"required\":false,\"min\":2,\"max\":5},"
                 "{\"content_control_tag\":\"status\",\"kind\":\"text\"}]}]}\n"});

    remove_if_exists(schema_input);
    remove_if_exists(patch_input);
    remove_if_exists(schema_output);
    remove_if_exists(output);
}

TEST_CASE("cli patch-template-schema reports parse errors") {
    const fs::path working_directory = fs::current_path();
    const fs::path schema_input =
        working_directory / "cli_patch_template_schema_parse_input.json";
    const fs::path patch_input =
        working_directory / "cli_patch_template_schema_parse_patch.json";
    const fs::path output =
        working_directory / "cli_patch_template_schema_parse_output.json";

    remove_if_exists(schema_input);
    remove_if_exists(patch_input);
    remove_if_exists(output);

    write_binary_file(schema_input,
                      std::string{
                          "{\"targets\":[{\"part\":\"body\",\"slots\":["
                          "{\"bookmark\":\"customer\",\"kind\":\"text\"}]}]}\n"});

    CHECK_EQ(run_cli({"patch-template-schema",
                      schema_input.string(),
                      "--json"},
                     output),
             2);
    CHECK_EQ(read_text_file(output),
             std::string{
                 "{\"command\":\"patch-template-schema\",\"ok\":false,"
                 "\"stage\":\"parse\",\"message\":\"missing required "
                 "--patch-file <path> option\"}\n"});

    write_binary_file(patch_input, std::string{"{}\n"});
    CHECK_EQ(run_cli({"patch-template-schema",
                      schema_input.string(),
                      "--patch-file",
                      patch_input.string(),
                      "--json"},
                     output),
             0);
    CHECK_EQ(read_text_file(output),
             std::string{
                 "{\"targets\":[{\"part\":\"body\",\"slots\":[{\"bookmark\":"
                 "\"customer\",\"kind\":\"text\"}]}]}\n"});

    write_binary_file(patch_input,
                      std::string{"{\"remove_targets\":[],\"remove_targets\":[]}\n"});
    CHECK_EQ(run_cli({"patch-template-schema",
                      schema_input.string(),
                      "--patch-file",
                      patch_input.string(),
                      "--json"},
                     output),
             2);
    CHECK_EQ(read_text_file(output),
             std::string{
                 "{\"command\":\"patch-template-schema\",\"ok\":false,"
                 "\"stage\":\"parse\",\"message\":\"JSON schema patch member "
                 "'remove_targets' must not be duplicated\"}\n"});

    write_binary_file(patch_input,
                      std::string{"{\"rename_slots\":[{\"part\":\"body\","
                                  "\"bookmark\":\"customer\"}]}\n"});
    CHECK_EQ(run_cli({"patch-template-schema",
                      schema_input.string(),
                      "--patch-file",
                      patch_input.string(),
                      "--json"},
                     output),
             2);
    CHECK_EQ(read_text_file(output),
             std::string{
                 "{\"command\":\"patch-template-schema\",\"ok\":false,"
                 "\"stage\":\"parse\",\"message\":\"JSON schema patch "
                 "rename-slot object must contain 'new_bookmark' or "
                 "'new_bookmark_name'\"}\n"});

    write_binary_file(patch_input,
                      std::string{"{\"update_slots\":[{\"part\":\"body\","
                                  "\"bookmark\":\"customer\"}]}\n"});
    CHECK_EQ(run_cli({"patch-template-schema",
                      schema_input.string(),
                      "--patch-file",
                      patch_input.string(),
                      "--json"},
                     output),
             2);
    CHECK_EQ(read_text_file(output),
             std::string{
                 "{\"command\":\"patch-template-schema\",\"ok\":false,"
                 "\"stage\":\"parse\",\"message\":\"JSON schema patch "
                 "update-slot object must contain at least one update field\"}\n"});

    write_binary_file(patch_input,
                      std::string{"{\"update_slots\":[{\"part\":\"body\","
                                  "\"bookmark\":\"customer\",\"min\":1,"
                                  "\"clear_min_occurrences\":true}]}\n"});
    CHECK_EQ(run_cli({"patch-template-schema",
                      schema_input.string(),
                      "--patch-file",
                      patch_input.string(),
                      "--json"},
                     output),
             2);
    CHECK_EQ(read_text_file(output),
             std::string{
                 "{\"command\":\"patch-template-schema\",\"ok\":false,"
                 "\"stage\":\"parse\",\"message\":\"JSON schema patch "
                 "update-slot member 'min' conflicts with "
                 "'clear_min_occurrences'\"}\n"});

    remove_if_exists(schema_input);
    remove_if_exists(patch_input);
    remove_if_exists(output);
}

TEST_CASE("cli preview-template-schema-patch copies patch file to output patch") {
    const fs::path working_directory = fs::current_path();
    const fs::path left_schema =
        working_directory / "cli_preview_template_schema_patch_copy_left.json";
    const fs::path patch_input =
        working_directory / "cli_preview_template_schema_patch_copy_input.json";
    const fs::path copied_patch_output =
        working_directory / "cli_preview_template_schema_patch_copy_output.json";
    const fs::path preview_output =
        working_directory / "cli_preview_template_schema_patch_copy_summary.json";

    remove_if_exists(left_schema);
    remove_if_exists(patch_input);
    remove_if_exists(copied_patch_output);
    remove_if_exists(preview_output);

    write_binary_file(left_schema,
                      std::string{
                          "{\"targets\":[{\"part\":\"body\",\"slots\":["
                          "{\"bookmark\":\"customer\",\"kind\":\"text\"}]}]}\n"});
    write_binary_file(patch_input,
                      std::string{
                          "{\"upsert_targets\":[{\"part\":\"body\",\"slots\":["
                          "{\"bookmark\":\"total\",\"kind\":\"text\"}]}]}\n"});

    CHECK_EQ(run_cli({"preview-template-schema-patch",
                      left_schema.string(),
                      "--patch-file",
                      patch_input.string(),
                      "--output-patch",
                      copied_patch_output.string(),
                      "--json"},
                     preview_output),
             0);
    CHECK_EQ(read_text_file(preview_output),
             std::string{
                 "{\"command\":\"preview-template-schema-patch\",\"ok\":true,"
                 "\"output_patch_path\":\""} +
                 json_escape_text(copied_patch_output.string()) +
                 "\",\"left_slot_count\":1,\"upsert_slot_count\":1,"
                 "\"remove_target_count\":0,\"remove_slot_count\":0,"
                 "\"rename_slot_count\":0,\"update_slot_count\":0,"
                 "\"removed_targets\":0,"
                 "\"removed_slots\":0,\"renamed_slots\":0,"
                 "\"inserted_slots\":1,\"replaced_slots\":0,\"changed\":true}\n");
    CHECK_EQ(read_text_file(copied_patch_output), read_text_file(patch_input));

    remove_if_exists(left_schema);
    remove_if_exists(patch_input);
    remove_if_exists(copied_patch_output);
    remove_if_exists(preview_output);
}

TEST_CASE("cli preview-template-schema-patch writes update slot output patch") {
    const fs::path working_directory = fs::current_path();
    const fs::path left_schema =
        working_directory / "cli_preview_template_schema_patch_update_left.json";
    const fs::path patch_input =
        working_directory / "cli_preview_template_schema_patch_update_input.json";
    const fs::path copied_patch_output =
        working_directory / "cli_preview_template_schema_patch_update_output.json";
    const fs::path preview_output =
        working_directory / "cli_preview_template_schema_patch_update_summary.json";

    remove_if_exists(left_schema);
    remove_if_exists(patch_input);
    remove_if_exists(copied_patch_output);
    remove_if_exists(preview_output);

    write_binary_file(left_schema,
                      std::string{
                          "{\"targets\":[{\"part\":\"body\",\"slots\":["
                          "{\"bookmark\":\"customer\",\"kind\":\"text\",\"count\":1}]}]}\n"});
    write_binary_file(patch_input,
                      std::string{
                          "{\"update_slots\":[{\"part\":\"body\","
                          "\"bookmark\":\"customer\",\"kind\":\"block\","
                          "\"required\":false,\"min_occurrences\":2,"
                          "\"max_occurrences\":5}]}\n"});

    CHECK_EQ(run_cli({"preview-template-schema-patch",
                      left_schema.string(),
                      "--patch-file",
                      patch_input.string(),
                      "--output-patch",
                      copied_patch_output.string(),
                      "--json"},
                     preview_output),
             0);
    CHECK_EQ(read_text_file(preview_output),
             std::string{
                 "{\"command\":\"preview-template-schema-patch\",\"ok\":true,"
                 "\"output_patch_path\":\""} +
                 json_escape_text(copied_patch_output.string()) +
                 "\",\"left_slot_count\":1,\"upsert_slot_count\":0,"
                 "\"remove_target_count\":0,\"remove_slot_count\":0,"
                 "\"rename_slot_count\":0,\"update_slot_count\":1,"
                 "\"removed_targets\":0,\"removed_slots\":0,"
                 "\"renamed_slots\":0,\"inserted_slots\":0,"
                 "\"replaced_slots\":1,\"changed\":true}\n");
    CHECK_EQ(read_text_file(copied_patch_output),
             std::string{
                 "{\"update_slots\":[{\"part\":\"body\",\"bookmark\":"
                 "\"customer\",\"slot_kind\":\"block\",\"required\":false,"
                 "\"min_occurrences\":2,\"max_occurrences\":5}]}\n"});

    remove_if_exists(left_schema);
    remove_if_exists(patch_input);
    remove_if_exists(copied_patch_output);
    remove_if_exists(preview_output);
}

TEST_CASE("cli preview-template-schema-patch writes generated output patch file") {
    const fs::path working_directory = fs::current_path();
    const fs::path left_schema =
        working_directory / "cli_preview_template_schema_patch_generate_left.json";
    const fs::path right_schema =
        working_directory / "cli_preview_template_schema_patch_generate_right.json";
    const fs::path patch_input =
        working_directory / "cli_preview_template_schema_patch_generate_expected.json";
    const fs::path generated_patch_output =
        working_directory / "cli_preview_template_schema_patch_generate_output.json";
    const fs::path preview_output =
        working_directory / "cli_preview_template_schema_patch_generate_summary.json";

    remove_if_exists(left_schema);
    remove_if_exists(right_schema);
    remove_if_exists(patch_input);
    remove_if_exists(generated_patch_output);
    remove_if_exists(preview_output);

    write_binary_file(left_schema,
                      std::string{
                          "{\"targets\":[{\"part\":\"body\",\"slots\":["
                          "{\"bookmark\":\"customer\",\"kind\":\"text\"}]}]}\n"});
    write_binary_file(right_schema,
                      std::string{
                          "{\"targets\":[{\"part\":\"body\",\"slots\":["
                          "{\"bookmark\":\"customer\",\"kind\":\"text\"},"
                          "{\"bookmark\":\"total\",\"kind\":\"text\"}]}]}\n"});
    write_binary_file(patch_input,
                      std::string{
                          "{\"upsert_targets\":[{\"part\":\"body\",\"slots\":["
                          "{\"bookmark\":\"total\",\"kind\":\"text\"}]}]}\n"});

    CHECK_EQ(run_cli({"preview-template-schema-patch",
                      left_schema.string(),
                      right_schema.string(),
                      "--output-patch",
                      generated_patch_output.string(),
                      "--json"},
                     preview_output),
             0);
    CHECK_EQ(read_text_file(preview_output),
             std::string{
                 "{\"command\":\"preview-template-schema-patch\",\"ok\":true,"
                 "\"output_patch_path\":\""} +
                 json_escape_text(generated_patch_output.string()) +
                 "\",\"left_slot_count\":1,\"right_slot_count\":2,"
                 "\"upsert_slot_count\":1,\"remove_target_count\":0,"
                 "\"remove_slot_count\":0,\"rename_slot_count\":0,"
                 "\"update_slot_count\":0,\"removed_targets\":0,"
                 "\"removed_slots\":0,"
                 "\"renamed_slots\":0,\"inserted_slots\":1,"
                 "\"replaced_slots\":0,\"changed\":true}\n");
    CHECK_EQ(read_text_file(generated_patch_output), read_text_file(patch_input));

    remove_if_exists(left_schema);
    remove_if_exists(right_schema);
    remove_if_exists(patch_input);
    remove_if_exists(generated_patch_output);
    remove_if_exists(preview_output);
}


TEST_CASE("cli template schema patch review json writes stable file") {
    const fs::path working_directory = fs::current_path();
    const fs::path left_schema =
        working_directory / "cli_template_schema_patch_review_left.json";
    const fs::path right_schema =
        working_directory / "cli_template_schema_patch_review_right.json";
    const fs::path preview_patch_output =
        working_directory / "cli_template_schema_patch_review_preview_patch.json";
    const fs::path build_patch_output =
        working_directory / "cli_template_schema_patch_review_build_patch.json";
    const fs::path preview_review_output =
        working_directory / "cli_template_schema_patch_review_preview.json";
    const fs::path build_review_output =
        working_directory / "cli_template_schema_patch_review_build.json";
    const fs::path preview_output =
        working_directory / "cli_template_schema_patch_review_preview_summary.json";
    const fs::path build_output =
        working_directory / "cli_template_schema_patch_review_build_summary.json";

    remove_if_exists(left_schema);
    remove_if_exists(right_schema);
    remove_if_exists(preview_patch_output);
    remove_if_exists(build_patch_output);
    remove_if_exists(preview_review_output);
    remove_if_exists(build_review_output);
    remove_if_exists(preview_output);
    remove_if_exists(build_output);

    write_binary_file(left_schema,
                      std::string{
                          "{\"targets\":[{\"part\":\"body\",\"slots\":["
                          "{\"bookmark\":\"customer\",\"kind\":\"text\","
                          "\"required\":false},"
                          "{\"bookmark\":\"obsolete\",\"kind\":\"text\","
                          "\"required\":true}]}]}"});
    write_binary_file(right_schema,
                      std::string{
                          "{\"targets\":[{\"part\":\"body\",\"slots\":["
                          "{\"bookmark\":\"customer\",\"kind\":\"text\","
                          "\"required\":true},"
                          "{\"content_control_tag\":\"order_no\","
                          "\"kind\":\"text\",\"required\":true}]}]}"});

    const std::string expected_review_json{
        R"({"schema":"featherdoc.template_schema_patch_review.v1","baseline_slot_count":2,"generated_slot_count":2,"patch":{"upsert_slot_count":1,"remove_target_count":0,"remove_slot_count":1,"rename_slot_count":0,"update_slot_count":1},"preview":{"removed_targets":0,"removed_slots":1,"renamed_slots":0,"inserted_slots":1,"replaced_slots":1,"changed":true},"changed":true})"};

    CHECK_EQ(run_cli({"preview-template-schema-patch",
                      left_schema.string(),
                      right_schema.string(),
                      "--output-patch",
                      preview_patch_output.string(),
                      "--review-json",
                      preview_review_output.string(),
                      "--json"},
                     preview_output),
             0);
    CHECK_EQ(read_text_file(preview_output),
             std::string{
                 "{\"command\":\"preview-template-schema-patch\",\"ok\":true,"
                 "\"output_patch_path\":\""} +
                 json_escape_text(preview_patch_output.string()) +
                 "\",\"review_json_path\":\"" +
                 json_escape_text(preview_review_output.string()) +
                 "\",\"left_slot_count\":2,\"right_slot_count\":2,"
                 "\"upsert_slot_count\":1,\"remove_target_count\":0,"
                 "\"remove_slot_count\":1,\"rename_slot_count\":0,"
                 "\"update_slot_count\":1,\"removed_targets\":0,"
                 "\"removed_slots\":1,\"renamed_slots\":0,"
                 "\"inserted_slots\":1,\"replaced_slots\":1,"
                 "\"changed\":true}\n");
    CHECK_EQ(read_text_file(preview_review_output), expected_review_json + "\n");

    CHECK_EQ(run_cli({"build-template-schema-patch",
                      left_schema.string(),
                      right_schema.string(),
                      "--output",
                      build_patch_output.string(),
                      "--review-json",
                      build_review_output.string(),
                      "--json"},
                     build_output),
             0);
    CHECK_EQ(read_text_file(build_output),
             std::string{
                 "{\"command\":\"build-template-schema-patch\",\"ok\":true,"
                 "\"output_path\":\""} +
                 json_escape_text(build_patch_output.string()) +
                 "\",\"review_json_path\":\"" +
                 json_escape_text(build_review_output.string()) +
                 "\",\"added_target_count\":0,\"removed_target_count\":0,"
                 "\"changed_target_count\":1,"
                 "\"generated_remove_target_count\":0,"
                 "\"generated_remove_slot_count\":1,"
                 "\"generated_rename_slot_count\":0,"
                 "\"generated_update_slot_count\":1,"
                 "\"generated_upsert_target_count\":1,"
                 "\"empty_patch\":false}\n");
    CHECK_EQ(read_text_file(build_review_output), expected_review_json + "\n");

    remove_if_exists(left_schema);
    remove_if_exists(right_schema);
    remove_if_exists(preview_patch_output);
    remove_if_exists(build_patch_output);
    remove_if_exists(preview_review_output);
    remove_if_exists(build_review_output);
    remove_if_exists(preview_output);
    remove_if_exists(build_output);
}
