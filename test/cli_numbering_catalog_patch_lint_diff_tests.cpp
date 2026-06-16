#include "cli_numbering_catalog_test_support.hpp"

TEST_CASE("cli patch-numbering-catalog upserts and removes overrides") {
    const fs::path working_directory = fs::current_path();
    const fs::path source = working_directory / "cli_numbering_catalog_patch_source.docx";
    const fs::path catalog_json =
        working_directory / "cli_numbering_catalog_patch_catalog.json";
    const fs::path patch_json =
        working_directory / "cli_numbering_catalog_patch_patch.json";
    const fs::path patched_json =
        working_directory / "cli_numbering_catalog_patch_patched.json";
    const fs::path export_output =
        working_directory / "cli_numbering_catalog_patch_export_output.json";
    const fs::path patch_output =
        working_directory / "cli_numbering_catalog_patch_output.json";
    const fs::path lint_output =
        working_directory / "cli_numbering_catalog_patch_lint_output.json";

    remove_if_exists(source);
    remove_if_exists(catalog_json);
    remove_if_exists(patch_json);
    remove_if_exists(patched_json);
    remove_if_exists(export_output);
    remove_if_exists(patch_output);
    remove_if_exists(lint_output);

    auto document = featherdoc::Document(source);
    REQUIRE_FALSE(document.create_empty());

    auto catalog = featherdoc::numbering_catalog{};
    auto catalog_definition = featherdoc::numbering_catalog_definition{};
    catalog_definition.definition.name = "PatchableOutline";
    catalog_definition.definition.levels = {
        featherdoc::numbering_level_definition{
            featherdoc::list_kind::decimal, 1U, 0U, "%1."},
        featherdoc::numbering_level_definition{
            featherdoc::list_kind::decimal, 1U, 1U, "%1.%2."},
    };
    catalog_definition.instances.push_back(
        featherdoc::numbering_instance_summary{3U, {}});
    catalog_definition.instances.push_back(
        featherdoc::numbering_instance_summary{
            4U,
            {featherdoc::numbering_level_override_summary{0U, 2U,
                                                          std::nullopt}}});
    catalog.definitions.push_back(std::move(catalog_definition));

    const auto import_summary = document.import_numbering_catalog(catalog);
    REQUIRE(static_cast<bool>(import_summary));
    REQUIRE_FALSE(document.save());

    CHECK_EQ(run_cli({"export-numbering-catalog",
                      source.string(),
                      "--output",
                      catalog_json.string(),
                      "--json"},
                     export_output),
             0);

    write_binary_file(
        patch_json,
        "{\"upsert_levels\":["
        "{\"definition_name\":\"PatchableOutline\",\"level\":"
        "{\"level\":2,\"kind\":\"decimal\",\"start\":1,"
        "\"text_pattern\":\"%1.%2.%3.\"}}],"
        "\"upsert_overrides\":["
        "{\"definition_name\":\"PatchableOutline\",\"instance_index\":0,"
        "\"level\":1,\"start_override\":5},"
        "{\"definition_name\":\"PatchableOutline\",\"instance_index\":1,"
        "\"level\":2,\"level_definition\":{\"level\":2,\"kind\":\"bullet\","
        "\"start\":1,\"text_pattern\":\"•\"}}],"
        "\"remove_overrides\":["
        "{\"definition_name\":\"PatchableOutline\",\"instance_index\":1,"
        "\"level\":0},"
        "{\"definition_name\":\"PatchableOutline\",\"instance_index\":1,"
        "\"level\":7}]}\n");

    CHECK_EQ(run_cli({"patch-numbering-catalog",
                      catalog_json.string(),
                      "--patch-file",
                      patch_json.string(),
                      "--output",
                      patched_json.string(),
                      "--json"},
                     patch_output),
             0);

    CHECK_EQ(read_text_file(patch_output),
             std::string{"{\"command\":\"patch-numbering-catalog\",\"ok\":true,"} +
                 "\"output_path\":" + json_quote(patched_json.string()) +
                 ",\"definition_count\":1,\"instance_count\":2,"
                 "\"upserted_level_count\":1,\"upserted_override_count\":2,"
                 "\"removed_override_count\":1,\"missing_override_count\":1}\n");

    const auto patched_catalog = read_text_file(patched_json);
    CHECK_NE(patched_catalog.find(
                 "\"levels\":[{\"level\":0,\"kind\":\"decimal\",\"start\":1,"
                 "\"text_pattern\":\"%1.\"},{\"level\":1,\"kind\":\"decimal\","
                 "\"start\":1,\"text_pattern\":\"%1.%2.\"},{\"level\":2,"
                 "\"kind\":\"decimal\",\"start\":1,"
                 "\"text_pattern\":\"%1.%2.%3.\"}]"),
             std::string::npos);
    CHECK_NE(patched_catalog.find(
                 "{\"instance_id\":1,\"level_overrides\":["
                 "{\"level\":1,\"start_override\":5,"
                 "\"level_definition\":null}]}"),
             std::string::npos);
    CHECK_NE(patched_catalog.find(
                 "{\"instance_id\":2,\"level_overrides\":["
                 "{\"level\":2,\"start_override\":null,\"level_definition\":"
                 "{\"level\":2,\"kind\":\"bullet\",\"start\":1,"
                 "\"text_pattern\":\"•\"}}]}"),
             std::string::npos);
    CHECK_EQ(patched_catalog.find("\"start_override\":2"), std::string::npos);

    CHECK_EQ(run_cli({"lint-numbering-catalog", patched_json.string(), "--json"},
                     lint_output),
             0);
    CHECK_EQ(read_text_file(lint_output),
             std::string{"{\"command\":\"lint-numbering-catalog\",\"ok\":true,"} +
                 "\"clean\":true,\"definition_count\":1,\"instance_count\":2,"
                 "\"level_count\":3,\"override_count\":2,\"issue_count\":0,"
                 "\"issues\":[]}\n");

    remove_if_exists(source);
    remove_if_exists(catalog_json);
    remove_if_exists(patch_json);
    remove_if_exists(patched_json);
    remove_if_exists(export_output);
    remove_if_exists(patch_output);
    remove_if_exists(lint_output);
}

TEST_CASE("cli lint-numbering-catalog reports clean and dirty catalogs") {
    const fs::path working_directory = fs::current_path();
    const fs::path clean_catalog =
        working_directory / "cli_lint_numbering_catalog_clean.json";
    const fs::path dirty_catalog =
        working_directory / "cli_lint_numbering_catalog_dirty.json";
    const fs::path clean_output =
        working_directory / "cli_lint_numbering_catalog_clean_output.json";
    const fs::path dirty_output =
        working_directory / "cli_lint_numbering_catalog_dirty_output.json";
    const fs::path parse_output =
        working_directory / "cli_lint_numbering_catalog_parse_output.json";

    remove_if_exists(clean_catalog);
    remove_if_exists(dirty_catalog);
    remove_if_exists(clean_output);
    remove_if_exists(dirty_output);
    remove_if_exists(parse_output);

    write_binary_file(
        clean_catalog,
        "{\"definition_count\":1,\"instance_count\":1,\"definitions\":["
        "{\"name\":\"CleanOutline\",\"levels\":["
        "{\"level\":0,\"kind\":\"decimal\",\"start\":1,"
        "\"text_pattern\":\"%1.\"}],\"instances\":["
        "{\"instance_id\":1,\"level_overrides\":["
        "{\"level\":0,\"start_override\":3,"
        "\"level_definition\":null}]}]}]}\n");

    CHECK_EQ(run_cli({"lint-numbering-catalog", clean_catalog.string(), "--json"},
                     clean_output),
             0);
    CHECK_EQ(read_text_file(clean_output),
             std::string{
                 "{\"command\":\"lint-numbering-catalog\",\"ok\":true,"
                 "\"clean\":true,\"definition_count\":1,\"instance_count\":1,"
                 "\"level_count\":1,\"override_count\":1,\"issue_count\":0,"
                 "\"issues\":[]}\n"});

    write_binary_file(
        dirty_catalog,
        "{\"definitions\":["
        "{\"name\":\"Broken\",\"levels\":["
        "{\"level\":0,\"kind\":\"decimal\",\"start\":0,"
        "\"text_pattern\":\"\"},"
        "{\"level\":0,\"kind\":\"bullet\",\"start\":1,"
        "\"text_pattern\":\"o\"}],\"instances\":["
        "{\"instance_id\":5,\"level_overrides\":["
        "{\"level\":9,\"start_override\":0,"
        "\"level_definition\":null},"
        "{\"level\":9,\"start_override\":1,"
        "\"level_definition\":{\"level\":9,\"kind\":\"decimal\","
        "\"start\":0,\"text_pattern\":\"\"}}]},"
        "{\"instance_id\":5,\"level_overrides\":[]}]},"
        "{\"name\":\"Broken\",\"levels\":[],\"instances\":[]}]}\n");

    CHECK_EQ(run_cli({"lint-numbering-catalog", dirty_catalog.string(), "--json"},
                     dirty_output),
             1);
    const auto dirty_json = read_text_file(dirty_output);
    CHECK_NE(dirty_json.find("\"command\":\"lint-numbering-catalog\""),
             std::string::npos);
    CHECK_NE(dirty_json.find("\"clean\":false"), std::string::npos);
    CHECK_NE(dirty_json.find("\"definition_count\":2"), std::string::npos);
    CHECK_NE(dirty_json.find("\"instance_count\":2"), std::string::npos);
    CHECK_NE(dirty_json.find("\"level_count\":2"), std::string::npos);
    CHECK_NE(dirty_json.find("\"override_count\":2"), std::string::npos);
    CHECK_NE(dirty_json.find("\"issue\":\"duplicate_definition_name\""),
             std::string::npos);
    CHECK_NE(dirty_json.find("\"issue\":\"empty_levels\""),
             std::string::npos);
    CHECK_NE(dirty_json.find("\"issue\":\"duplicate_level\""),
             std::string::npos);
    CHECK_NE(dirty_json.find("\"issue\":\"invalid_start\""),
             std::string::npos);
    CHECK_NE(dirty_json.find("\"issue\":\"empty_text_pattern\""),
             std::string::npos);
    CHECK_NE(dirty_json.find("\"issue\":\"duplicate_instance_id\""),
             std::string::npos);
    CHECK_NE(dirty_json.find("\"issue\":\"invalid_override_level\""),
             std::string::npos);
    CHECK_NE(dirty_json.find("\"issue\":\"duplicate_override_level\""),
             std::string::npos);
    CHECK_NE(dirty_json.find("\"issue\":\"invalid_override_start\""),
             std::string::npos);
    CHECK_NE(dirty_json.find("\"issue\":\"invalid_override_definition\""),
             std::string::npos);

    CHECK_EQ(run_cli({"lint-numbering-catalog", "--json"}, parse_output), 2);
    CHECK_EQ(read_text_file(parse_output),
             std::string{
                 "{\"command\":\"lint-numbering-catalog\",\"ok\":false,"
                 "\"stage\":\"parse\",\"message\":\"lint-numbering-catalog "
                 "expects a catalog path\"}\n"});

    remove_if_exists(clean_catalog);
    remove_if_exists(dirty_catalog);
    remove_if_exists(clean_output);
    remove_if_exists(dirty_output);
    remove_if_exists(parse_output);
}

TEST_CASE("cli diff-numbering-catalog reports catalog changes as json") {
    const fs::path working_directory = fs::current_path();
    const fs::path left_catalog =
        working_directory / "cli_diff_numbering_catalog_left.json";
    const fs::path right_catalog =
        working_directory / "cli_diff_numbering_catalog_right.json";
    const fs::path diff_output =
        working_directory / "cli_diff_numbering_catalog_output.json";
    const fs::path equal_output =
        working_directory / "cli_diff_numbering_catalog_equal_output.json";
    const fs::path parse_output =
        working_directory / "cli_diff_numbering_catalog_parse_output.json";

    remove_if_exists(left_catalog);
    remove_if_exists(right_catalog);
    remove_if_exists(diff_output);
    remove_if_exists(equal_output);
    remove_if_exists(parse_output);

    write_binary_file(
        left_catalog,
        "{\"definitions\":["
        "{\"name\":\"RemovedOutline\",\"levels\":["
        "{\"level\":0,\"kind\":\"decimal\",\"start\":1,"
        "\"text_pattern\":\"%1.\"}],\"instances\":[]},"
        "{\"name\":\"SharedOutline\",\"levels\":["
        "{\"level\":0,\"kind\":\"decimal\",\"start\":1,"
        "\"text_pattern\":\"%1.\"},"
        "{\"level\":1,\"kind\":\"decimal\",\"start\":1,"
        "\"text_pattern\":\"%1.%2.\"}],\"instances\":["
        "{\"instance_id\":10,\"level_overrides\":["
        "{\"level\":0,\"start_override\":2,"
        "\"level_definition\":null}]},"
        "{\"instance_id\":11,\"level_overrides\":[]}]}]}\n");

    write_binary_file(
        right_catalog,
        "{\"definitions\":["
        "{\"name\":\"SharedOutline\",\"levels\":["
        "{\"level\":0,\"kind\":\"decimal\",\"start\":3,"
        "\"text_pattern\":\"%1)\"},"
        "{\"level\":2,\"kind\":\"bullet\",\"start\":1,"
        "\"text_pattern\":\"•\"}],\"instances\":["
        "{\"instance_id\":20,\"level_overrides\":["
        "{\"level\":0,\"start_override\":4,"
        "\"level_definition\":null},"
        "{\"level\":2,\"start_override\":null,"
        "\"level_definition\":{\"level\":2,\"kind\":\"bullet\","
        "\"start\":1,\"text_pattern\":\"•\"}}]},"
        "{\"instance_id\":21,\"level_overrides\":[]},"
        "{\"instance_id\":22,\"level_overrides\":[]}]},"
        "{\"name\":\"AddedOutline\",\"levels\":["
        "{\"level\":0,\"kind\":\"bullet\",\"start\":1,"
        "\"text_pattern\":\"•\"}],\"instances\":[]}]}\n");

    CHECK_EQ(run_cli({"diff-numbering-catalog",
                      left_catalog.string(),
                      right_catalog.string(),
                      "--json"},
                     diff_output),
             0);
    const auto diff_json = read_text_file(diff_output);
    CHECK_NE(diff_json.find("\"equal\":false"), std::string::npos);
    CHECK_NE(diff_json.find("\"added_definition_count\":1"),
             std::string::npos);
    CHECK_NE(diff_json.find("\"removed_definition_count\":1"),
             std::string::npos);
    CHECK_NE(diff_json.find("\"changed_definition_count\":1"),
             std::string::npos);
    CHECK_NE(diff_json.find("\"name\":\"AddedOutline\""),
             std::string::npos);
    CHECK_NE(diff_json.find("\"name\":\"RemovedOutline\""),
             std::string::npos);
    CHECK_NE(diff_json.find("\"name\":\"SharedOutline\""),
             std::string::npos);
    CHECK_NE(diff_json.find("\"added_level_count\":1"), std::string::npos);
    CHECK_NE(diff_json.find("\"removed_level_count\":1"), std::string::npos);
    CHECK_NE(diff_json.find("\"changed_level_count\":1"), std::string::npos);
    CHECK_NE(diff_json.find("\"added_instance_count\":1"), std::string::npos);
    CHECK_NE(diff_json.find("\"changed_instance_count\":1"), std::string::npos);
    CHECK_NE(diff_json.find("\"added_override_count\":1"), std::string::npos);
    CHECK_NE(diff_json.find("\"changed_override_count\":1"), std::string::npos);

    CHECK_EQ(run_cli({"diff-numbering-catalog",
                      left_catalog.string(),
                      left_catalog.string(),
                      "--fail-on-diff",
                      "--json"},
                     equal_output),
             0);
    CHECK_NE(read_text_file(equal_output).find("\"equal\":true"),
             std::string::npos);

    CHECK_EQ(run_cli({"diff-numbering-catalog",
                      left_catalog.string(),
                      right_catalog.string(),
                      "--fail-on-diff",
                      "--json"},
                     parse_output),
             1);

    remove_if_exists(left_catalog);
    remove_if_exists(right_catalog);
    remove_if_exists(diff_output);
    remove_if_exists(equal_output);
    remove_if_exists(parse_output);
}
