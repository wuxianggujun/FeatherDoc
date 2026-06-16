#include "cli_template_schema_test_support.hpp"

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
