#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "cli_template_schema_test_support.hpp"

TEST_CASE("cli build-template-schema-patch generates reusable patch output") {
    const fs::path working_directory = fs::current_path();
    const fs::path left_schema =
        working_directory / "cli_build_template_schema_patch_left.json";
    const fs::path right_schema =
        working_directory / "cli_build_template_schema_patch_right.json";
    const fs::path patch_output =
        working_directory / "cli_build_template_schema_patch_output.json";
    const fs::path build_output =
        working_directory / "cli_build_template_schema_patch_summary.json";
    const fs::path applied_schema =
        working_directory / "cli_build_template_schema_patch_applied.json";
    const fs::path apply_output =
        working_directory / "cli_build_template_schema_patch_apply_summary.json";

    remove_if_exists(left_schema);
    remove_if_exists(right_schema);
    remove_if_exists(patch_output);
    remove_if_exists(build_output);
    remove_if_exists(applied_schema);
    remove_if_exists(apply_output);

    write_binary_file(left_schema,
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
                          "{\"bookmark\":\"customer\",\"kind\":\"text\"},"
                          "{\"bookmark\":\"summary_block\",\"kind\":\"text\"}"
                          "]"
                          "}"
                          "]"
                          "}"});
    write_binary_file(right_schema,
                      std::string{
                          "{"
                          "\"targets\":["
                          "{"
                          "\"part\":\"body\","
                          "\"slots\":["
                          "{\"bookmark\":\"customer\",\"kind\":\"text\",\"count\":2},"
                          "{\"bookmark\":\"invoice_no\",\"kind\":\"text\"}"
                          "]"
                          "},"
                          "{"
                          "\"part\":\"section-header\","
                          "\"section\":1,"
                          "\"kind\":\"default\","
                          "\"resolved_from_section\":0,"
                          "\"linked_to_previous\":true,"
                          "\"slots\":[{\"bookmark\":\"header_title\",\"kind\":\"text\"}]"
                          "}"
                          "]"
                          "}"});

    CHECK_EQ(run_cli({"build-template-schema-patch",
                      left_schema.string(),
                      right_schema.string(),
                      "--output",
                      patch_output.string(),
                      "--json"},
                     build_output),
             0);
    CHECK_EQ(read_text_file(build_output),
             std::string{
                 "{\"command\":\"build-template-schema-patch\",\"ok\":true,"
                 "\"output_path\":\"" +
                 json_escape_text(patch_output.string()) +
                 "\",\"added_target_count\":1,\"removed_target_count\":1,"
                 "\"changed_target_count\":1,\"generated_remove_target_count\":1,"
                 "\"generated_remove_slot_count\":0,"
                 "\"generated_rename_slot_count\":1,"
                 "\"generated_update_slot_count\":1,"
                 "\"generated_upsert_target_count\":1,"
                 "\"empty_patch\":false}\n"});
    CHECK_EQ(read_text_file(patch_output),
             std::string{
                 "{\"remove_targets\":["
                 "{\"part\":\"section-footer\",\"section\":1,\"kind\":\"default\"}],"
                 "\"rename_slots\":["
                 "{\"part\":\"body\",\"bookmark\":\"summary_block\","
                 "\"new_bookmark\":\"invoice_no\"}],"
                 "\"update_slots\":[{\"part\":\"body\",\"bookmark\":"
                 "\"customer\",\"min_occurrences\":2,"
                 "\"max_occurrences\":2}],"
                 "\"upsert_targets\":["
                 "{\"part\":\"section-header\",\"section\":1,\"kind\":\"default\","
                 "\"resolved_from_section\":0,\"linked_to_previous\":true,"
                 "\"slots\":[{\"bookmark\":\"header_title\",\"kind\":\"text\"}]}]}\n"});

    CHECK_EQ(run_cli({"patch-template-schema",
                      left_schema.string(),
                      "--patch-file",
                      patch_output.string(),
                      "--output",
                      applied_schema.string(),
                      "--json"},
                     apply_output),
             0);
    CHECK_EQ(read_text_file(applied_schema), read_text_file(right_schema) + "\n");

    remove_if_exists(left_schema);
    remove_if_exists(right_schema);
    remove_if_exists(patch_output);
    remove_if_exists(build_output);
    remove_if_exists(applied_schema);
    remove_if_exists(apply_output);
}

TEST_CASE("cli build-template-schema-patch emits slot-level rename update and removal") {
    const fs::path working_directory = fs::current_path();
    const fs::path left_schema =
        working_directory / "cli_build_template_schema_patch_slot_ops_left.json";
    const fs::path right_schema =
        working_directory / "cli_build_template_schema_patch_slot_ops_right.json";
    const fs::path patch_output =
        working_directory / "cli_build_template_schema_patch_slot_ops_output.json";
    const fs::path build_output =
        working_directory / "cli_build_template_schema_patch_slot_ops_summary.json";
    const fs::path applied_schema =
        working_directory / "cli_build_template_schema_patch_slot_ops_applied.json";
    const fs::path apply_output =
        working_directory / "cli_build_template_schema_patch_slot_ops_apply_summary.json";

    remove_if_exists(left_schema);
    remove_if_exists(right_schema);
    remove_if_exists(patch_output);
    remove_if_exists(build_output);
    remove_if_exists(applied_schema);
    remove_if_exists(apply_output);

    write_binary_file(left_schema,
                      std::string{
                          "{"
                          "\"targets\":["
                          "{"
                          "\"part\":\"body\","
                          "\"slots\":["
                          "{\"bookmark\":\"customer\",\"kind\":\"text\"},"
                          "{\"bookmark\":\"obsolete_note\",\"kind\":\"block\","
                          "\"required\":false},"
                          "{\"bookmark\":\"summary_block\",\"kind\":\"text\"}"
                          "]"
                          "}"
                          "]"
                          "}"});
    write_binary_file(right_schema,
                      std::string{
                          "{"
                          "\"targets\":["
                          "{"
                          "\"part\":\"body\","
                          "\"slots\":["
                          "{\"bookmark\":\"customer\",\"kind\":\"text\"},"
                          "{\"bookmark\":\"invoice_no\",\"kind\":\"text\",\"count\":2}"
                          "]"
                          "}"
                          "]"
                          "}"});

    CHECK_EQ(run_cli({"build-template-schema-patch",
                      left_schema.string(),
                      right_schema.string(),
                      "--output",
                      patch_output.string(),
                      "--json"},
                     build_output),
             0);
    CHECK_EQ(read_text_file(build_output),
             std::string{
                 "{\"command\":\"build-template-schema-patch\",\"ok\":true,"
                 "\"output_path\":\"" +
                 json_escape_text(patch_output.string()) +
                 "\",\"added_target_count\":0,\"removed_target_count\":0,"
                 "\"changed_target_count\":1,\"generated_remove_target_count\":0,"
                 "\"generated_remove_slot_count\":1,"
                 "\"generated_rename_slot_count\":1,"
                 "\"generated_update_slot_count\":1,"
                 "\"generated_upsert_target_count\":0,"
                 "\"empty_patch\":false}\n"});
    CHECK_EQ(read_text_file(patch_output),
             std::string{
                 "{\"remove_slots\":[{\"part\":\"body\",\"bookmark\":"
                 "\"obsolete_note\"}],"
                 "\"rename_slots\":[{\"part\":\"body\",\"bookmark\":"
                 "\"summary_block\",\"new_bookmark\":\"invoice_no\"}],"
                 "\"update_slots\":[{\"part\":\"body\",\"bookmark\":"
                 "\"invoice_no\",\"min_occurrences\":2,"
                 "\"max_occurrences\":2}]}\n"});

    CHECK_EQ(run_cli({"patch-template-schema",
                      left_schema.string(),
                      "--patch-file",
                      patch_output.string(),
                      "--output",
                      applied_schema.string(),
                      "--json"},
                     apply_output),
             0);
    CHECK_EQ(read_text_file(applied_schema), read_text_file(right_schema) + "\n");

    remove_if_exists(left_schema);
    remove_if_exists(right_schema);
    remove_if_exists(patch_output);
    remove_if_exists(build_output);
    remove_if_exists(applied_schema);
    remove_if_exists(apply_output);
}

TEST_CASE("cli build-template-schema-patch emits source-aware rename update") {
    const fs::path working_directory = fs::current_path();
    const fs::path left_schema =
        working_directory / "cli_build_template_schema_patch_source_left.json";
    const fs::path right_schema =
        working_directory / "cli_build_template_schema_patch_source_right.json";
    const fs::path patch_output =
        working_directory / "cli_build_template_schema_patch_source_output.json";
    const fs::path build_output =
        working_directory / "cli_build_template_schema_patch_source_summary.json";
    const fs::path applied_schema =
        working_directory / "cli_build_template_schema_patch_source_applied.json";
    const fs::path apply_output =
        working_directory / "cli_build_template_schema_patch_source_apply_summary.json";

    remove_if_exists(left_schema);
    remove_if_exists(right_schema);
    remove_if_exists(patch_output);
    remove_if_exists(build_output);
    remove_if_exists(applied_schema);
    remove_if_exists(apply_output);

    write_binary_file(left_schema,
                      std::string{
                          "{\"targets\":[{\"part\":\"body\",\"slots\":["
                          "{\"content_control_tag\":\"order_no\","
                          "\"kind\":\"text\"}]}]}"});
    write_binary_file(right_schema,
                      std::string{
                          "{\"targets\":[{\"part\":\"body\",\"slots\":["
                          "{\"content_control_tag\":\"order_id\","
                          "\"kind\":\"block\",\"count\":2}]}]}"});

    CHECK_EQ(run_cli({"build-template-schema-patch",
                      left_schema.string(),
                      right_schema.string(),
                      "--output",
                      patch_output.string(),
                      "--json"},
                     build_output),
             0);
    CHECK_EQ(read_text_file(build_output),
             std::string{
                 "{\"command\":\"build-template-schema-patch\",\"ok\":true,"
                 "\"output_path\":\"" +
                 json_escape_text(patch_output.string()) +
                 "\",\"added_target_count\":0,\"removed_target_count\":0,"
                 "\"changed_target_count\":1,\"generated_remove_target_count\":0,"
                 "\"generated_remove_slot_count\":0,"
                 "\"generated_rename_slot_count\":1,"
                 "\"generated_update_slot_count\":1,"
                 "\"generated_upsert_target_count\":0,"
                 "\"empty_patch\":false}\n"});
    CHECK_EQ(read_text_file(patch_output),
             std::string{
                 "{\"rename_slots\":[{\"part\":\"body\","
                 "\"content_control_tag\":\"order_no\","
                 "\"new_content_control_tag\":\"order_id\"}],"
                 "\"update_slots\":[{\"part\":\"body\","
                 "\"content_control_tag\":\"order_id\","
                 "\"slot_kind\":\"block\",\"min_occurrences\":2,"
                 "\"max_occurrences\":2}]}\n"});

    CHECK_EQ(run_cli({"patch-template-schema",
                      left_schema.string(),
                      "--patch-file",
                      patch_output.string(),
                      "--output",
                      applied_schema.string(),
                      "--json"},
                     apply_output),
             0);
    CHECK_EQ(read_text_file(applied_schema), read_text_file(right_schema) + "\n");

    remove_if_exists(left_schema);
    remove_if_exists(right_schema);
    remove_if_exists(patch_output);
    remove_if_exists(build_output);
    remove_if_exists(applied_schema);
    remove_if_exists(apply_output);
}


TEST_CASE("cli build-template-schema-patch emits rename occurrence clear update") {
    const fs::path working_directory = fs::current_path();
    const fs::path left_schema =
        working_directory / "cli_build_template_schema_patch_rename_clear_left.json";
    const fs::path right_schema =
        working_directory / "cli_build_template_schema_patch_rename_clear_right.json";
    const fs::path patch_output =
        working_directory / "cli_build_template_schema_patch_rename_clear_output.json";
    const fs::path build_output =
        working_directory / "cli_build_template_schema_patch_rename_clear_summary.json";
    const fs::path applied_schema =
        working_directory / "cli_build_template_schema_patch_rename_clear_applied.json";
    const fs::path apply_output =
        working_directory / "cli_build_template_schema_patch_rename_clear_apply_summary.json";

    remove_if_exists(left_schema);
    remove_if_exists(right_schema);
    remove_if_exists(patch_output);
    remove_if_exists(build_output);
    remove_if_exists(applied_schema);
    remove_if_exists(apply_output);

    write_binary_file(left_schema,
                      std::string{R"({"targets":[{"part":"body","slots":[{"bookmark":"old_customer","kind":"text","count":2}]}]})"});
    write_binary_file(right_schema,
                      std::string{R"({"targets":[{"part":"body","slots":[{"bookmark":"customer","kind":"text"}]}]})"});

    CHECK_EQ(run_cli({"build-template-schema-patch",
                      left_schema.string(),
                      right_schema.string(),
                      "--output",
                      patch_output.string(),
                      "--json"},
                     build_output),
             0);
    CHECK_EQ(read_text_file(build_output),
             std::string{
                 "{\"command\":\"build-template-schema-patch\",\"ok\":true,"
                 "\"output_path\":\"" +
                 json_escape_text(patch_output.string()) +
                 "\",\"added_target_count\":0,\"removed_target_count\":0,"
                 "\"changed_target_count\":1,\"generated_remove_target_count\":0,"
                 "\"generated_remove_slot_count\":0,"
                 "\"generated_rename_slot_count\":1,"
                 "\"generated_update_slot_count\":1,"
                 "\"generated_upsert_target_count\":0,"
                 "\"empty_patch\":false}\n"});
    CHECK_EQ(read_text_file(patch_output),
             std::string{R"({"rename_slots":[{"part":"body","bookmark":"old_customer","new_bookmark":"customer"}],"update_slots":[{"part":"body","bookmark":"customer","clear_min_occurrences":true,"clear_max_occurrences":true}]})"} +
                 "\n");

    CHECK_EQ(run_cli({"patch-template-schema",
                      left_schema.string(),
                      "--patch-file",
                      patch_output.string(),
                      "--output",
                      applied_schema.string(),
                      "--json"},
                     apply_output),
             0);
    CHECK_EQ(read_text_file(applied_schema), read_text_file(right_schema) + "\n");

    remove_if_exists(left_schema);
    remove_if_exists(right_schema);
    remove_if_exists(patch_output);
    remove_if_exists(build_output);
    remove_if_exists(applied_schema);
    remove_if_exists(apply_output);
}

TEST_CASE("cli build-template-schema-patch keeps ambiguous renames explicit") {
    const fs::path working_directory = fs::current_path();
    const fs::path left_schema =
        working_directory / "cli_build_template_schema_patch_ambiguous_left.json";
    const fs::path right_schema =
        working_directory / "cli_build_template_schema_patch_ambiguous_right.json";
    const fs::path patch_output =
        working_directory / "cli_build_template_schema_patch_ambiguous_output.json";
    const fs::path build_output =
        working_directory / "cli_build_template_schema_patch_ambiguous_summary.json";
    const fs::path applied_schema =
        working_directory / "cli_build_template_schema_patch_ambiguous_applied.json";
    const fs::path apply_output =
        working_directory / "cli_build_template_schema_patch_ambiguous_apply_summary.json";

    remove_if_exists(left_schema);
    remove_if_exists(right_schema);
    remove_if_exists(patch_output);
    remove_if_exists(build_output);
    remove_if_exists(applied_schema);
    remove_if_exists(apply_output);

    write_binary_file(left_schema,
                      std::string{
                          "{\"targets\":[{\"part\":\"body\",\"slots\":["
                          "{\"bookmark\":\"old_primary\",\"kind\":\"text\"},"
                          "{\"bookmark\":\"old_secondary\",\"kind\":\"text\"}"
                          "]}]}"});
    write_binary_file(right_schema,
                      std::string{
                          "{\"targets\":[{\"part\":\"body\",\"slots\":["
                          "{\"bookmark\":\"new_primary\",\"kind\":\"text\"},"
                          "{\"bookmark\":\"new_secondary\",\"kind\":\"text\"}"
                          "]}]}"});

    CHECK_EQ(run_cli({"build-template-schema-patch",
                      left_schema.string(),
                      right_schema.string(),
                      "--output",
                      patch_output.string(),
                      "--json"},
                     build_output),
             0);
    CHECK_EQ(read_text_file(build_output),
             std::string{
                 "{\"command\":\"build-template-schema-patch\",\"ok\":true,"
                 "\"output_path\":\"" +
                 json_escape_text(patch_output.string()) +
                 "\",\"added_target_count\":0,\"removed_target_count\":0,"
                 "\"changed_target_count\":1,\"generated_remove_target_count\":0,"
                 "\"generated_remove_slot_count\":2,"
                 "\"generated_rename_slot_count\":0,"
                 "\"generated_update_slot_count\":0,"
                 "\"generated_upsert_target_count\":1,"
                 "\"empty_patch\":false}\n"});
    CHECK_EQ(read_text_file(patch_output),
             std::string{
                 "{\"remove_slots\":[{\"part\":\"body\",\"bookmark\":"
                 "\"old_primary\"},{\"part\":\"body\",\"bookmark\":"
                 "\"old_secondary\"}],\"upsert_targets\":[{\"part\":"
                 "\"body\",\"slots\":[{\"bookmark\":\"new_primary\","
                 "\"kind\":\"text\"},{\"bookmark\":\"new_secondary\","
                 "\"kind\":\"text\"}]}]}\n"});

    CHECK_EQ(run_cli({"patch-template-schema",
                      left_schema.string(),
                      "--patch-file",
                      patch_output.string(),
                      "--output",
                      applied_schema.string(),
                      "--json"},
                     apply_output),
             0);
    CHECK_EQ(read_text_file(applied_schema), read_text_file(right_schema) + "\n");

    remove_if_exists(left_schema);
    remove_if_exists(right_schema);
    remove_if_exists(patch_output);
    remove_if_exists(build_output);
    remove_if_exists(applied_schema);
    remove_if_exists(apply_output);
}

TEST_CASE("cli build-template-schema-patch keeps cross-source changes explicit") {
    const fs::path working_directory = fs::current_path();
    const fs::path left_schema =
        working_directory / "cli_build_template_schema_patch_cross_source_left.json";
    const fs::path right_schema =
        working_directory / "cli_build_template_schema_patch_cross_source_right.json";
    const fs::path patch_output =
        working_directory / "cli_build_template_schema_patch_cross_source_output.json";
    const fs::path build_output =
        working_directory / "cli_build_template_schema_patch_cross_source_summary.json";
    const fs::path applied_schema =
        working_directory / "cli_build_template_schema_patch_cross_source_applied.json";
    const fs::path apply_output =
        working_directory / "cli_build_template_schema_patch_cross_source_apply_summary.json";

    remove_if_exists(left_schema);
    remove_if_exists(right_schema);
    remove_if_exists(patch_output);
    remove_if_exists(build_output);
    remove_if_exists(applied_schema);
    remove_if_exists(apply_output);

    write_binary_file(left_schema,
                      std::string{
                          "{\"targets\":[{\"part\":\"body\",\"slots\":["
                          "{\"bookmark\":\"shared_id\",\"kind\":\"text\"}"
                          "]}]}"});
    write_binary_file(right_schema,
                      std::string{
                          "{\"targets\":[{\"part\":\"body\",\"slots\":["
                          "{\"content_control_tag\":\"shared_id\","
                          "\"kind\":\"text\"}]}]}"});

    CHECK_EQ(run_cli({"build-template-schema-patch",
                      left_schema.string(),
                      right_schema.string(),
                      "--output",
                      patch_output.string(),
                      "--json"},
                     build_output),
             0);
    CHECK_EQ(read_text_file(build_output),
             std::string{
                 "{\"command\":\"build-template-schema-patch\",\"ok\":true,"
                 "\"output_path\":\"" +
                 json_escape_text(patch_output.string()) +
                 "\",\"added_target_count\":0,\"removed_target_count\":0,"
                 "\"changed_target_count\":1,\"generated_remove_target_count\":0,"
                 "\"generated_remove_slot_count\":1,"
                 "\"generated_rename_slot_count\":0,"
                 "\"generated_update_slot_count\":0,"
                 "\"generated_upsert_target_count\":1,"
                 "\"empty_patch\":false}\n"});
    CHECK_EQ(read_text_file(patch_output),
             std::string{
                 "{\"remove_slots\":[{\"part\":\"body\",\"bookmark\":"
                 "\"shared_id\"}],\"upsert_targets\":[{\"part\":\"body\","
                 "\"slots\":[{\"content_control_tag\":\"shared_id\","
                 "\"kind\":\"text\"}]}]}\n"});

    CHECK_EQ(run_cli({"patch-template-schema",
                      left_schema.string(),
                      "--patch-file",
                      patch_output.string(),
                      "--output",
                      applied_schema.string(),
                      "--json"},
                     apply_output),
             0);
    CHECK_EQ(read_text_file(applied_schema), read_text_file(right_schema) + "\n");

    remove_if_exists(left_schema);
    remove_if_exists(right_schema);
    remove_if_exists(patch_output);
    remove_if_exists(build_output);
    remove_if_exists(applied_schema);
    remove_if_exists(apply_output);
}

TEST_CASE("cli build-template-schema-patch keeps cross-target changes explicit") {
    const fs::path working_directory = fs::current_path();
    const fs::path left_schema =
        working_directory / "cli_build_template_schema_patch_cross_target_left.json";
    const fs::path right_schema =
        working_directory / "cli_build_template_schema_patch_cross_target_right.json";
    const fs::path patch_output =
        working_directory / "cli_build_template_schema_patch_cross_target_output.json";
    const fs::path build_output =
        working_directory / "cli_build_template_schema_patch_cross_target_summary.json";
    const fs::path applied_schema =
        working_directory / "cli_build_template_schema_patch_cross_target_applied.json";
    const fs::path apply_output =
        working_directory / "cli_build_template_schema_patch_cross_target_apply_summary.json";

    remove_if_exists(left_schema);
    remove_if_exists(right_schema);
    remove_if_exists(patch_output);
    remove_if_exists(build_output);
    remove_if_exists(applied_schema);
    remove_if_exists(apply_output);

    write_binary_file(left_schema,
                      std::string{
                          "{\"targets\":["
                          "{\"part\":\"body\",\"slots\":["
                          "{\"bookmark\":\"body_keep\",\"kind\":\"text\"},"
                          "{\"bookmark\":\"old_title\",\"kind\":\"text\"}]},"
                          "{\"part\":\"section-header\",\"section\":1,"
                          "\"kind\":\"default\",\"slots\":["
                          "{\"bookmark\":\"header_keep\",\"kind\":\"text\"}]}]}"});
    write_binary_file(right_schema,
                      std::string{
                          "{\"targets\":["
                          "{\"part\":\"body\",\"slots\":["
                          "{\"bookmark\":\"body_keep\",\"kind\":\"text\"}]},"
                          "{\"part\":\"section-header\",\"section\":1,"
                          "\"kind\":\"default\",\"slots\":["
                          "{\"bookmark\":\"header_keep\",\"kind\":\"text\"},"
                          "{\"bookmark\":\"new_title\",\"kind\":\"text\"}]}]}"});

    CHECK_EQ(run_cli({"build-template-schema-patch",
                      left_schema.string(),
                      right_schema.string(),
                      "--output",
                      patch_output.string(),
                      "--json"},
                     build_output),
             0);
    CHECK_EQ(read_text_file(build_output),
             std::string{
                 "{\"command\":\"build-template-schema-patch\",\"ok\":true,"
                 "\"output_path\":\"" +
                 json_escape_text(patch_output.string()) +
                 "\",\"added_target_count\":0,\"removed_target_count\":0,"
                 "\"changed_target_count\":2,\"generated_remove_target_count\":0,"
                 "\"generated_remove_slot_count\":1,"
                 "\"generated_rename_slot_count\":0,"
                 "\"generated_update_slot_count\":0,"
                 "\"generated_upsert_target_count\":1,"
                 "\"empty_patch\":false}\n"});
    CHECK_EQ(read_text_file(patch_output),
             std::string{
                 "{\"remove_slots\":[{\"part\":\"body\",\"bookmark\":"
                 "\"old_title\"}],\"upsert_targets\":[{\"part\":"
                 "\"section-header\",\"section\":1,\"kind\":\"default\","
                 "\"slots\":[{\"bookmark\":\"new_title\","
                 "\"kind\":\"text\"}]}]}\n"});

    CHECK_EQ(run_cli({"patch-template-schema",
                      left_schema.string(),
                      "--patch-file",
                      patch_output.string(),
                      "--output",
                      applied_schema.string(),
                      "--json"},
                     apply_output),
             0);
    CHECK_EQ(read_text_file(applied_schema), read_text_file(right_schema) + "\n");

    remove_if_exists(left_schema);
    remove_if_exists(right_schema);
    remove_if_exists(patch_output);
    remove_if_exists(build_output);
    remove_if_exists(applied_schema);
    remove_if_exists(apply_output);
}

TEST_CASE("cli build-template-schema-patch replaces changed target identity") {
    const fs::path working_directory = fs::current_path();
    const fs::path left_schema =
        working_directory / "cli_build_template_schema_patch_target_identity_left.json";
    const fs::path right_schema =
        working_directory / "cli_build_template_schema_patch_target_identity_right.json";
    const fs::path patch_output =
        working_directory / "cli_build_template_schema_patch_target_identity_output.json";
    const fs::path build_output =
        working_directory / "cli_build_template_schema_patch_target_identity_summary.json";
    const fs::path applied_schema =
        working_directory / "cli_build_template_schema_patch_target_identity_applied.json";
    const fs::path apply_output =
        working_directory / "cli_build_template_schema_patch_target_identity_apply_summary.json";

    remove_if_exists(left_schema);
    remove_if_exists(right_schema);
    remove_if_exists(patch_output);
    remove_if_exists(build_output);
    remove_if_exists(applied_schema);
    remove_if_exists(apply_output);

    write_binary_file(left_schema,
                      std::string{
                          "{\"targets\":[{\"part\":\"section-header\","
                          "\"section\":1,\"kind\":\"default\","
                          "\"resolved_from_section\":0,"
                          "\"linked_to_previous\":true,\"slots\":["
                          "{\"bookmark\":\"header_title\","
                          "\"kind\":\"text\"}]}]}"});
    write_binary_file(right_schema,
                      std::string{
                          "{\"targets\":[{\"part\":\"section-header\","
                          "\"section\":1,\"kind\":\"default\","
                          "\"resolved_from_section\":1,"
                          "\"linked_to_previous\":true,\"slots\":["
                          "{\"bookmark\":\"header_title\","
                          "\"kind\":\"text\"}]}]}"});

    CHECK_EQ(run_cli({"build-template-schema-patch",
                      left_schema.string(),
                      right_schema.string(),
                      "--output",
                      patch_output.string(),
                      "--json"},
                     build_output),
             0);
    CHECK_EQ(read_text_file(build_output),
             std::string{
                 "{\"command\":\"build-template-schema-patch\",\"ok\":true,"
                 "\"output_path\":\"" +
                 json_escape_text(patch_output.string()) +
                 "\",\"added_target_count\":0,\"removed_target_count\":0,"
                 "\"changed_target_count\":1,\"generated_remove_target_count\":1,"
                 "\"generated_remove_slot_count\":0,"
                 "\"generated_rename_slot_count\":0,"
                 "\"generated_update_slot_count\":0,"
                 "\"generated_upsert_target_count\":1,"
                 "\"empty_patch\":false}\n"});
    CHECK_EQ(read_text_file(patch_output),
             std::string{
                 "{\"remove_targets\":[{\"part\":\"section-header\","
                 "\"section\":1,\"kind\":\"default\","
                 "\"resolved_from_section\":0,"
                 "\"linked_to_previous\":true}],\"upsert_targets\":["
                 "{\"part\":\"section-header\",\"section\":1,"
                 "\"kind\":\"default\",\"resolved_from_section\":1,"
                 "\"linked_to_previous\":true,\"slots\":["
                 "{\"bookmark\":\"header_title\","
                 "\"kind\":\"text\"}]}]}\n"});

    CHECK_EQ(run_cli({"patch-template-schema",
                      left_schema.string(),
                      "--patch-file",
                      patch_output.string(),
                      "--output",
                      applied_schema.string(),
                      "--json"},
                     apply_output),
             0);
    CHECK_EQ(read_text_file(applied_schema), read_text_file(right_schema) + "\n");

    remove_if_exists(left_schema);
    remove_if_exists(right_schema);
    remove_if_exists(patch_output);
    remove_if_exists(build_output);
    remove_if_exists(applied_schema);
    remove_if_exists(apply_output);
}

TEST_CASE("cli build-template-schema-patch emits occurrence clear update") {
    const fs::path working_directory = fs::current_path();
    const fs::path left_schema =
        working_directory / "cli_build_template_schema_patch_clear_left.json";
    const fs::path right_schema =
        working_directory / "cli_build_template_schema_patch_clear_right.json";
    const fs::path patch_output =
        working_directory / "cli_build_template_schema_patch_clear_output.json";
    const fs::path build_output =
        working_directory / "cli_build_template_schema_patch_clear_summary.json";
    const fs::path applied_schema =
        working_directory / "cli_build_template_schema_patch_clear_applied.json";
    const fs::path apply_output =
        working_directory / "cli_build_template_schema_patch_clear_apply_summary.json";

    remove_if_exists(left_schema);
    remove_if_exists(right_schema);
    remove_if_exists(patch_output);
    remove_if_exists(build_output);
    remove_if_exists(applied_schema);
    remove_if_exists(apply_output);

    write_binary_file(left_schema,
                      std::string{
                          "{\"targets\":[{\"part\":\"body\",\"slots\":["
                          "{\"bookmark\":\"customer\",\"kind\":\"text\","
                          "\"count\":2}]}]}"});
    write_binary_file(right_schema,
                      std::string{
                          "{\"targets\":[{\"part\":\"body\",\"slots\":["
                          "{\"bookmark\":\"customer\",\"kind\":\"text\"}]}]}"});

    CHECK_EQ(run_cli({"build-template-schema-patch",
                      left_schema.string(),
                      right_schema.string(),
                      "--output",
                      patch_output.string(),
                      "--json"},
                     build_output),
             0);
    CHECK_EQ(read_text_file(build_output),
             std::string{
                 "{\"command\":\"build-template-schema-patch\",\"ok\":true,"
                 "\"output_path\":\"" +
                 json_escape_text(patch_output.string()) +
                 "\",\"added_target_count\":0,\"removed_target_count\":0,"
                 "\"changed_target_count\":1,\"generated_remove_target_count\":0,"
                 "\"generated_remove_slot_count\":0,"
                 "\"generated_rename_slot_count\":0,"
                 "\"generated_update_slot_count\":1,"
                 "\"generated_upsert_target_count\":0,"
                 "\"empty_patch\":false}\n"});
    CHECK_EQ(read_text_file(patch_output),
             std::string{
                 "{\"update_slots\":[{\"part\":\"body\",\"bookmark\":"
                 "\"customer\",\"clear_min_occurrences\":true,"
                 "\"clear_max_occurrences\":true}]}\n"});

    CHECK_EQ(run_cli({"patch-template-schema",
                      left_schema.string(),
                      "--patch-file",
                      patch_output.string(),
                      "--output",
                      applied_schema.string(),
                      "--json"},
                     apply_output),
             0);
    CHECK_EQ(read_text_file(applied_schema), read_text_file(right_schema) + "\n");

    remove_if_exists(left_schema);
    remove_if_exists(right_schema);
    remove_if_exists(patch_output);
    remove_if_exists(build_output);
    remove_if_exists(applied_schema);
    remove_if_exists(apply_output);
}

TEST_CASE("cli build-template-schema-patch emits empty patch for equivalent schemas") {
    const fs::path working_directory = fs::current_path();
    const fs::path left_schema =
        working_directory / "cli_build_template_schema_patch_equal_left.json";
    const fs::path right_schema =
        working_directory / "cli_build_template_schema_patch_equal_right.json";
    const fs::path output =
        working_directory / "cli_build_template_schema_patch_equal_output.json";

    remove_if_exists(left_schema);
    remove_if_exists(right_schema);
    remove_if_exists(output);

    write_binary_file(left_schema,
                      std::string{
                          "{\"targets\":[{\"part\":\"body\",\"slots\":["
                          "{\"bookmark\":\"customer\",\"kind\":\"text\"}]}]}\n"});
    write_binary_file(right_schema,
                      std::string{
                          "{\"targets\":[{\"part\":\"body\",\"slots\":["
                          "{\"bookmark\":\"customer\",\"kind\":\"text\"}]}]}\n"});

    CHECK_EQ(run_cli({"build-template-schema-patch",
                      left_schema.string(),
                      right_schema.string(),
                      "--json"},
                     output),
             0);
    CHECK_EQ(read_text_file(output), std::string{"{}\n"});

    remove_if_exists(left_schema);
    remove_if_exists(right_schema);
    remove_if_exists(output);
}

TEST_CASE("cli build-template-schema-patch reports parse errors") {
    const fs::path working_directory = fs::current_path();
    const fs::path schema =
        working_directory / "cli_build_template_schema_patch_parse_schema.json";
    const fs::path output =
        working_directory / "cli_build_template_schema_patch_parse_output.json";

    remove_if_exists(schema);
    remove_if_exists(output);

    write_binary_file(schema,
                      std::string{
                          "{\"targets\":[{\"part\":\"body\",\"slots\":["
                          "{\"bookmark\":\"customer\",\"kind\":\"text\"}]}]}\n"});

    CHECK_EQ(
        run_cli({"build-template-schema-patch", schema.string(), "--json"}, output), 2);
    CHECK_EQ(read_text_file(output),
             std::string{
                 "{\"command\":\"build-template-schema-patch\",\"ok\":false,"
                 "\"stage\":\"parse\",\"message\":\"build-template-schema-patch "
                 "expects left and right schema paths\"}\n"});

    remove_if_exists(schema);
    remove_if_exists(output);
}
