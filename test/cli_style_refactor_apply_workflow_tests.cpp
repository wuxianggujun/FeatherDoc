#include "cli_style_test_support.hpp"

TEST_CASE("cli suggest-style-merges filters by style ids") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_suggest_style_merges_filters_source.docx";
    const fs::path source_filter_output =
        working_directory / "cli_suggest_style_merges_filters_source.json";
    const fs::path target_filter_output =
        working_directory / "cli_suggest_style_merges_filters_target.json";
    const fs::path missing_filter_output =
        working_directory / "cli_suggest_style_merges_filters_missing.json";
    const fs::path parse_output =
        working_directory / "cli_suggest_style_merges_filters_parse.json";

    remove_if_exists(source);
    remove_if_exists(source_filter_output);
    remove_if_exists(target_filter_output);
    remove_if_exists(missing_filter_output);
    remove_if_exists(parse_output);

    create_cli_duplicate_style_profile_fixture(source);

    CHECK_EQ(run_cli({"suggest-style-merges",
                      source.string(),
                      "--source-style",
                      "DuplicateBodyC",
                      "--json"},
                     source_filter_output),
             0);
    const auto source_filter_json = read_text_file(source_filter_output);
    CHECK_NE(source_filter_json.find(R"("operation_count":1)"),
             std::string::npos);
    CHECK_NE(source_filter_json.find(R"("source_style_id":"DuplicateBodyC")"),
             std::string::npos);
    CHECK_EQ(source_filter_json.find(R"("source_style_id":"DuplicateBodyB")"),
             std::string::npos);
    CHECK_NE(source_filter_json.find(R"("target_style_id":"DuplicateBodyA")"),
             std::string::npos);
    CHECK_NE(source_filter_json.find(R"("confidence":80)"), std::string::npos);
    CHECK_NE(source_filter_json.find(R"("exact_xml_match_count":0)"),
             std::string::npos);
    CHECK_NE(source_filter_json.find(R"("xml_difference_count":1)"),
             std::string::npos);

    CHECK_EQ(run_cli({"suggest-style-merges",
                      source.string(),
                      "--target-style",
                      "DuplicateBodyA",
                      "--json"},
                     target_filter_output),
             0);
    const auto target_filter_json = read_text_file(target_filter_output);
    CHECK_NE(target_filter_json.find(R"("operation_count":2)"),
             std::string::npos);
    CHECK_NE(target_filter_json.find(R"("source_style_id":"DuplicateBodyB")"),
             std::string::npos);
    CHECK_NE(target_filter_json.find(R"("source_style_id":"DuplicateBodyC")"),
             std::string::npos);
    CHECK_NE(target_filter_json.find(R"("target_style_id":"DuplicateBodyA")"),
             std::string::npos);

    CHECK_EQ(run_cli({"suggest-style-merges",
                      source.string(),
                      "--source-style",
                      "MissingStyle",
                      "--json"},
                     missing_filter_output),
             0);
    const auto missing_filter_json = read_text_file(missing_filter_output);
    CHECK_NE(missing_filter_json.find(R"("operation_count":0)"),
             std::string::npos);
    CHECK_EQ(missing_filter_json.find(R"("source_style_id":"DuplicateBodyB")"),
             std::string::npos);
    CHECK_EQ(missing_filter_json.find(R"("source_style_id":"DuplicateBodyC")"),
             std::string::npos);

    CHECK_EQ(run_cli({"suggest-style-merges",
                      source.string(),
                      "--source-style",
                      "DuplicateBodyB",
                      "--source-style",
                      "DuplicateBodyB",
                      "--json"},
                     parse_output),
             2);
    CHECK_NE(read_text_file(parse_output).find("duplicate --source-style id"),
             std::string::npos);

    CHECK_EQ(run_cli({"suggest-style-merges",
                      source.string(),
                      "--target-style",
                      "DuplicateBodyA",
                      "--target-style",
                      "DuplicateBodyA",
                      "--json"},
                     parse_output),
             2);
    CHECK_NE(read_text_file(parse_output).find("duplicate --target-style id"),
             std::string::npos);

    remove_if_exists(source);
    remove_if_exists(source_filter_output);
    remove_if_exists(target_filter_output);
    remove_if_exists(missing_filter_output);
    remove_if_exists(parse_output);
}

TEST_CASE("cli apply-style-refactor applies clean batch and blocks conflicts") {
    const fs::path working_directory = fs::current_path();
    const fs::path source = working_directory / "cli_style_refactor_apply_source.docx";
    const fs::path applied = working_directory / "cli_style_refactor_apply_output.docx";
    const fs::path dirty_output =
        working_directory / "cli_style_refactor_apply_dirty.docx";
    const fs::path output = working_directory / "cli_style_refactor_apply.json";
    const fs::path plan_file =
        working_directory / "cli_style_refactor_apply.plan.json";
    const fs::path plan_output =
        working_directory / "cli_style_refactor_apply_plan_output.json";
    const fs::path rollback_file =
        working_directory / "cli_style_refactor_apply.rollback.json";
    const fs::path restore_dry_run_output =
        working_directory / "cli_style_refactor_apply_restore_dry_run.json";
    const fs::path dirty_json =
        working_directory / "cli_style_refactor_apply_dirty.json";
    const fs::path parse_output =
        working_directory / "cli_style_refactor_apply_parse.json";

    remove_if_exists(source);
    remove_if_exists(applied);
    remove_if_exists(dirty_output);
    remove_if_exists(output);
    remove_if_exists(plan_file);
    remove_if_exists(plan_output);
    remove_if_exists(rollback_file);
    remove_if_exists(restore_dry_run_output);
    remove_if_exists(dirty_json);
    remove_if_exists(parse_output);

    create_cli_style_usage_fixture(source);
    {
        featherdoc::Document document(source);
        REQUIRE_FALSE(document.open());
        featherdoc::paragraph_style_definition archive_body;
        archive_body.name = "Archive Body";
        archive_body.based_on = "Normal";
        REQUIRE(document.ensure_paragraph_style("ArchiveBody", archive_body));
        featherdoc::paragraph_style_definition archive_note;
        archive_note.name = "Archive Note";
        archive_note.based_on = "Normal";
        REQUIRE(document.ensure_paragraph_style("ArchiveNote", archive_note));
        REQUIRE_FALSE(document.save());
    }

    CHECK_EQ(run_cli({"plan-style-refactor",
                      source.string(),
                      "--rename",
                      "CustomBody:ReviewBody",
                      "--merge",
                      "ArchiveBody:Normal",
                      "--merge",
                      "ArchiveNote:Normal",
                      "--output-plan",
                      plan_file.string(),
                      "--json"},
                     plan_output),
             0);
    const auto plan_json = read_text_file(plan_file);
    CHECK_NE(plan_json.find(R"("source_style_id":"ArchiveBody")"),
             std::string::npos);
    CHECK_NE(plan_json.find(R"("source_style_id":"ArchiveNote")"),
             std::string::npos);

    CHECK_EQ(run_cli({"apply-style-refactor",
                      source.string(),
                      "--plan-file",
                      plan_file.string(),
                      "--rollback-plan",
                      rollback_file.string(),
                      "--output",
                      applied.string(),
                      "--json"},
                     output),
             0);
    const auto apply_json = read_text_file(output);
    CHECK_NE(apply_json.find(R"("command":"apply-style-refactor")"),
             std::string::npos);
    CHECK_NE(apply_json.find(R"("ok":true)"), std::string::npos);
    CHECK_NE(apply_json.find(R"("changed":true)"), std::string::npos);
    CHECK_NE(apply_json.find(R"("requested_count":3)"), std::string::npos);
    CHECK_NE(apply_json.find(R"("applied_count":3)"), std::string::npos);
    CHECK_NE(apply_json.find(R"("skipped_count":0)"), std::string::npos);
    CHECK_NE(apply_json.find(R"("rollback_count":3)"), std::string::npos);
    CHECK_NE(apply_json.find(R"("plan_file":)"), std::string::npos);
    CHECK_NE(apply_json.find(R"("rollback_plan_file":)"), std::string::npos);
    CHECK_NE(apply_json.find(R"("clean":true)"), std::string::npos);
    const auto rollback_json = read_text_file(rollback_file);
    CHECK_NE(rollback_json.find(R"("rollback_count":3)"), std::string::npos);
    CHECK_NE(rollback_json.find(
                 R"("command_template":"featherdoc_cli rename-style <input.docx> ReviewBody CustomBody --output <output.docx> --json")"),
             std::string::npos);
    CHECK_NE(rollback_json.find(R"("automatic":false)"), std::string::npos);
    CHECK_NE(rollback_json.find(R"("restorable":true)"), std::string::npos);
    CHECK_NE(rollback_json.find(R"("source_style_xml":")"), std::string::npos);
    CHECK_NE(rollback_json.find(R"(w:styleId=\"ArchiveBody\")"),
             std::string::npos);
    CHECK_NE(rollback_json.find(R"(w:styleId=\"ArchiveNote\")"),
             std::string::npos);

    const auto styles_xml = read_docx_entry(applied, "word/styles.xml");
    CHECK_EQ(styles_xml.find(R"(w:styleId="CustomBody")"), std::string::npos);
    CHECK_NE(styles_xml.find(R"(w:styleId="ReviewBody")"),
             std::string::npos);
    CHECK_EQ(styles_xml.find(R"(w:styleId="ArchiveBody")"),
             std::string::npos);
    CHECK_EQ(styles_xml.find(R"(w:styleId="ArchiveNote")"),
             std::string::npos);
    const auto document_xml = read_docx_entry(applied, "word/document.xml");
    CHECK_NE(document_xml.find(R"(w:pStyle w:val="ReviewBody")"),
             std::string::npos);
    CHECK_EQ(document_xml.find(R"(w:pStyle w:val="CustomBody")"),
             std::string::npos);

    CHECK_EQ(run_cli({"restore-style-merge",
                      applied.string(),
                      "--rollback-plan",
                      rollback_file.string(),
                      "--entry",
                      "1",
                      "--entry",
                      "2",
                      "--dry-run",
                      "--json"},
                     restore_dry_run_output),
             0);
    const auto restore_dry_run_json = read_text_file(restore_dry_run_output);
    CHECK_NE(restore_dry_run_json.find(R"("command":"restore-style-merge")"),
             std::string::npos);
    CHECK_NE(restore_dry_run_json.find(R"("dry_run":true)"), std::string::npos);
    CHECK_NE(restore_dry_run_json.find(R"("requested_count":2)"),
             std::string::npos);
    CHECK_NE(restore_dry_run_json.find(R"("restored_count":2)"),
             std::string::npos);
    CHECK_NE(restore_dry_run_json.find(R"("entry_indexes":[1,2])"),
             std::string::npos);
    {
        featherdoc::Document document(applied);
        REQUIRE_FALSE(document.open());
        CHECK_FALSE(document.find_style("ArchiveBody").has_value());
        CHECK_FALSE(document.find_style("ArchiveNote").has_value());
    }

    CHECK_EQ(run_cli({"restore-style-merge",
                      applied.string(),
                      "--rollback-plan",
                      rollback_file.string(),
                      "--source-style",
                      "ArchiveNote",
                      "--target-style",
                      "Normal",
                      "--plan-only",
                      "--json"},
                     restore_dry_run_output),
             0);
    const auto restore_filter_json = read_text_file(restore_dry_run_output);
    CHECK_NE(restore_filter_json.find(R"("command":"restore-style-merge")"),
             std::string::npos);
    CHECK_NE(restore_filter_json.find(R"("dry_run":true)"), std::string::npos);
    CHECK_NE(restore_filter_json.find(R"("requested_count":1)"),
             std::string::npos);
    CHECK_NE(restore_filter_json.find(R"("restored_count":1)"),
             std::string::npos);
    CHECK_NE(restore_filter_json.find(R"("source_style_ids":["ArchiveNote"])"),
             std::string::npos);
    CHECK_NE(restore_filter_json.find(R"("target_style_ids":["Normal"])"),
             std::string::npos);
    CHECK_NE(restore_filter_json.find(R"("source_style_id":"ArchiveNote")"),
             std::string::npos);

    CHECK_EQ(run_cli({"apply-style-refactor",
                      source.string(),
                      "--rename",
                      "CustomBody:ReviewBody",
                      "--merge",
                      "CustomBody:Normal",
                      "--output",
                      dirty_output.string(),
                      "--json"},
                     dirty_json),
             1);
    const auto dirty_text = read_text_file(dirty_json);
    CHECK_NE(dirty_text.find(R"("ok":false)"), std::string::npos);
    CHECK_NE(dirty_text.find(R"("changed":false)"), std::string::npos);
    CHECK_NE(dirty_text.find(R"("applied_count":0)"), std::string::npos);
    CHECK_NE(dirty_text.find(R"("skipped_count":2)"), std::string::npos);
    CHECK_NE(dirty_text.find(R"("code":"duplicate_source_operation")"),
             std::string::npos);
    CHECK_FALSE(fs::exists(dirty_output));

    CHECK_EQ(run_cli({"restore-style-merge",
                      applied.string(),
                      "--rollback-plan",
                      rollback_file.string(),
                      "--entry",
                      "1",
                      "--source-style",
                      "ArchiveBody",
                      "--dry-run",
                      "--json"},
                     parse_output),
             2);
    CHECK_NE(read_text_file(parse_output)
                 .find("restore-style-merge --entry cannot be combined with --source-style or --target-style"),
             std::string::npos);

    CHECK_EQ(run_cli({"apply-style-refactor", source.string(), "--json"},
                     parse_output),
             2);
    CHECK_NE(read_text_file(parse_output)
                 .find("apply-style-refactor expects --plan-file or at least one --rename or --merge option"),
             std::string::npos);

    remove_if_exists(source);
    remove_if_exists(applied);
    remove_if_exists(dirty_output);
    remove_if_exists(output);
    remove_if_exists(plan_file);
    remove_if_exists(plan_output);
    remove_if_exists(rollback_file);
    remove_if_exists(restore_dry_run_output);
    remove_if_exists(dirty_json);
    remove_if_exists(parse_output);
}
