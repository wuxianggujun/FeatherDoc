#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "cli_test_support.hpp"

namespace {
void create_cli_paragraph_list_fixture(const fs::path &path) {
    remove_if_exists(path);

    featherdoc::Document document(path);
    REQUIRE_FALSE(document.create_empty());

    REQUIRE(document.paragraphs().add_run("item 0").has_next());
    append_body_paragraph(document, "item 1");
    append_body_paragraph(document, "item 2");

    REQUIRE_FALSE(document.save());
}
} // namespace

TEST_CASE("cli inspect-numbering lists custom numbering definitions") {
    const fs::path working_directory = fs::current_path();
    const fs::path source = working_directory / "cli_numbering_source.docx";
    const fs::path output = working_directory / "cli_numbering_output.txt";

    remove_if_exists(source);
    remove_if_exists(output);

    featherdoc::Document document(source);
    REQUIRE_FALSE(document.create_empty());

    auto definition = featherdoc::numbering_definition{};
    definition.name = "LegalOutline";
    definition.levels = {
        featherdoc::numbering_level_definition{
            featherdoc::list_kind::decimal, 3U, 0U, "%1."},
        featherdoc::numbering_level_definition{
            featherdoc::list_kind::decimal, 1U, 1U, "%1.%2."},
    };

    const auto numbering_id = document.ensure_numbering_definition(definition);
    REQUIRE(numbering_id.has_value());
    auto paragraph = document.paragraphs();
    REQUIRE(paragraph.has_next());
    REQUIRE(document.set_paragraph_numbering(paragraph, *numbering_id));
    REQUIRE(paragraph.add_run("legal heading").has_next());
    REQUIRE_FALSE(document.save());

    const auto numbering_xml = read_docx_entry(source, "word/numbering.xml");
    pugi::xml_document numbering_document;
    REQUIRE(numbering_document.load_string(numbering_xml.c_str()));
    const auto numbering_root = numbering_document.child("w:numbering");
    REQUIRE(numbering_root != pugi::xml_node{});
    const auto numbering_instance = numbering_root.child("w:num");
    REQUIRE(numbering_instance != pugi::xml_node{});
    const auto num_id = std::string{numbering_instance.attribute("w:numId").value()};

    CHECK_EQ(run_cli({"inspect-numbering", source.string()}, output), 0);
    const auto inspect_text = read_text_file(output);
    CHECK_NE(inspect_text.find("definitions: 1"), std::string::npos);
    CHECK_NE(inspect_text.find("definition[0]: id=" + std::to_string(*numbering_id) +
                                   " name=LegalOutline levels=2 instances=" + num_id),
             std::string::npos);
    CHECK_NE(inspect_text.find("level[0]: kind=decimal start=3 text_pattern=%1."),
             std::string::npos);
    CHECK_NE(inspect_text.find("level[1]: kind=decimal start=1 text_pattern=%1.%2."),
             std::string::npos);
    CHECK_NE(inspect_text.find("instance[" + num_id + "]: override_count=0"),
             std::string::npos);

    remove_if_exists(source);
    remove_if_exists(output);
}

TEST_CASE("cli inspect-numbering supports single-definition json output and errors") {
    const fs::path working_directory = fs::current_path();
    const fs::path source = working_directory / "cli_numbering_single_source.docx";
    const fs::path definition_output =
        working_directory / "cli_numbering_single_output.json";
    const fs::path missing_output =
        working_directory / "cli_numbering_missing_output.json";

    remove_if_exists(source);
    remove_if_exists(definition_output);
    remove_if_exists(missing_output);

    featherdoc::Document document(source);
    REQUIRE_FALSE(document.create_empty());

    auto definition = featherdoc::numbering_definition{};
    definition.name = "PolicyOutline";
    definition.levels = {
        featherdoc::numbering_level_definition{
            featherdoc::list_kind::decimal, 5U, 0U, "(%1)"},
        featherdoc::numbering_level_definition{
            featherdoc::list_kind::bullet, 1U, 1U, "o"},
    };

    const auto numbering_id = document.ensure_numbering_definition(definition);
    REQUIRE(numbering_id.has_value());
    auto paragraph = document.paragraphs();
    REQUIRE(paragraph.has_next());
    REQUIRE(document.set_paragraph_numbering(paragraph, *numbering_id));
    REQUIRE(paragraph.add_run("policy item").has_next());
    REQUIRE_FALSE(document.save());

    const auto numbering_xml = read_docx_entry(source, "word/numbering.xml");
    pugi::xml_document numbering_document;
    REQUIRE(numbering_document.load_string(numbering_xml.c_str()));
    const auto numbering_root = numbering_document.child("w:numbering");
    REQUIRE(numbering_root != pugi::xml_node{});
    const auto numbering_instance = numbering_root.child("w:num");
    REQUIRE(numbering_instance != pugi::xml_node{});
    const auto num_id = std::string{numbering_instance.attribute("w:numId").value()};

    CHECK_EQ(run_cli({"inspect-numbering", source.string(), "--definition",
                      std::to_string(*numbering_id), "--json"},
                     definition_output),
             0);
    CHECK_EQ(
        read_text_file(definition_output),
        std::string{
            "{\"definition\":{\"definition_id\":"} +
            std::to_string(*numbering_id) +
            ",\"name\":\"PolicyOutline\",\"levels\":[{\"level\":0,\"kind\":\"decimal\","
            "\"start\":5,\"text_pattern\":\"(%1)\"},{\"level\":1,\"kind\":\"bullet\","
            "\"start\":1,\"text_pattern\":\"o\"}],\"instance_ids\":[" +
            num_id + "],\"instances\":[{\"instance_id\":" + num_id +
            ",\"level_overrides\":[]}]}}\n");

    CHECK_EQ(run_cli({"inspect-numbering", source.string(), "--definition", "999",
                      "--json"},
                     missing_output),
             1);
    const auto missing_json = read_text_file(missing_output);
    CHECK_NE(missing_json.find("\"command\":\"inspect-numbering\""), std::string::npos);
    CHECK_NE(missing_json.find("\"stage\":\"inspect\""), std::string::npos);
    CHECK_NE(missing_json.find(
                 "\"detail\":\"numbering definition id '999' was not found in "
                 "word/numbering.xml\""),
             std::string::npos);
    CHECK_NE(missing_json.find("\"entry\":\"word/numbering.xml\""), std::string::npos);

    remove_if_exists(source);
    remove_if_exists(definition_output);
    remove_if_exists(missing_output);
}

TEST_CASE("cli inspect-numbering reports instance overrides for restarted lists") {
    const fs::path working_directory = fs::current_path();
    const fs::path source = working_directory / "cli_numbering_restart_source.docx";
    const fs::path list_output = working_directory / "cli_numbering_restart_output.txt";
    const fs::path json_output = working_directory / "cli_numbering_restart_output.json";

    remove_if_exists(source);
    remove_if_exists(list_output);
    remove_if_exists(json_output);

    featherdoc::Document document(source);
    REQUIRE_FALSE(document.create_empty());

    auto first_item = document.paragraphs();
    REQUIRE(first_item.has_next());
    REQUIRE(document.set_paragraph_list(first_item, featherdoc::list_kind::decimal));
    REQUIRE(first_item.add_run("first item").has_next());

    auto spacer = first_item.insert_paragraph_after("restart here");
    REQUIRE(spacer.has_next());

    auto restarted_item = spacer.insert_paragraph_after("");
    REQUIRE(restarted_item.has_next());
    REQUIRE(document.restart_paragraph_list(restarted_item, featherdoc::list_kind::decimal));
    REQUIRE(restarted_item.add_run("restarted item").has_next());

    const auto definitions = document.list_numbering_definitions();
    REQUIRE_FALSE(document.last_error());
    const auto managed_definition =
        std::find_if(definitions.begin(), definitions.end(), [](const auto &summary) {
            return summary.name == "FeatherDocDecimalList";
        });
    REQUIRE(managed_definition != definitions.end());
    REQUIRE_EQ(managed_definition->instance_ids.size(), 2U);

    const auto definition_id = managed_definition->definition_id;
    const auto first_num_id = std::to_string(managed_definition->instance_ids[0]);
    const auto restarted_num_id = std::to_string(managed_definition->instance_ids[1]);

    REQUIRE_FALSE(document.save());

    CHECK_EQ(run_cli({"inspect-numbering", source.string()}, list_output), 0);
    const auto inspect_text = read_text_file(list_output);
    CHECK_NE(inspect_text.find("id=" + std::to_string(definition_id) +
                                   " name=FeatherDocDecimalList"),
             std::string::npos);
    CHECK_NE(inspect_text.find("instances=" + first_num_id + "," + restarted_num_id),
             std::string::npos);
    CHECK_NE(inspect_text.find("instance[" + first_num_id + "]: override_count=0"),
             std::string::npos);
    CHECK_NE(inspect_text.find("instance[" + restarted_num_id + "]: override_count=1"),
             std::string::npos);
    CHECK_NE(inspect_text.find("override[0]: level=0 start_override=1"),
             std::string::npos);

    CHECK_EQ(run_cli({"inspect-numbering", source.string(), "--definition",
                      std::to_string(definition_id), "--json"},
                     json_output),
             0);
    const auto inspect_json = read_text_file(json_output);
    CHECK_NE(inspect_json.find("\"definition_id\":" + std::to_string(definition_id)),
             std::string::npos);
    CHECK_NE(inspect_json.find("\"instance_ids\":[" + first_num_id + "," + restarted_num_id +
                               "]"),
             std::string::npos);
    CHECK_NE(inspect_json.find("\"instances\":[{\"instance_id\":" + first_num_id +
                               ",\"level_overrides\":[]},{\"instance_id\":" +
                               restarted_num_id + ",\"level_overrides\":["),
             std::string::npos);
    CHECK_NE(inspect_json.find(
                 "{\"level\":0,\"start_override\":1,\"level_definition\":null}"),
             std::string::npos);

    remove_if_exists(source);
    remove_if_exists(list_output);
    remove_if_exists(json_output);
}

TEST_CASE("cli inspect-numbering filters a single numbering instance") {
    const fs::path working_directory = fs::current_path();
    const fs::path source = working_directory / "cli_numbering_instance_source.docx";
    const fs::path text_output = working_directory / "cli_numbering_instance_output.txt";
    const fs::path json_output = working_directory / "cli_numbering_instance_output.json";
    const fs::path missing_output = working_directory / "cli_numbering_instance_missing.json";

    remove_if_exists(source);
    remove_if_exists(text_output);
    remove_if_exists(json_output);
    remove_if_exists(missing_output);

    featherdoc::Document document(source);
    REQUIRE_FALSE(document.create_empty());

    auto first_item = document.paragraphs();
    REQUIRE(first_item.has_next());
    REQUIRE(document.set_paragraph_list(first_item, featherdoc::list_kind::decimal));
    REQUIRE(first_item.add_run("first item").has_next());

    auto spacer = first_item.insert_paragraph_after("restart here");
    REQUIRE(spacer.has_next());

    auto restarted_item = spacer.insert_paragraph_after("");
    REQUIRE(restarted_item.has_next());
    REQUIRE(document.restart_paragraph_list(restarted_item, featherdoc::list_kind::decimal));
    REQUIRE(restarted_item.add_run("restarted item").has_next());

    const auto definitions = document.list_numbering_definitions();
    REQUIRE_FALSE(document.last_error());
    const auto managed_definition =
        std::find_if(definitions.begin(), definitions.end(), [](const auto &summary) {
            return summary.name == "FeatherDocDecimalList";
        });
    REQUIRE(managed_definition != definitions.end());
    REQUIRE_EQ(managed_definition->instance_ids.size(), 2U);

    const auto definition_id = std::to_string(managed_definition->definition_id);
    const auto restarted_num_id = std::to_string(managed_definition->instance_ids[1]);

    REQUIRE_FALSE(document.save());

    CHECK_EQ(run_cli({"inspect-numbering", source.string(), "--instance", restarted_num_id},
                     text_output),
             0);
    const auto inspect_text = read_text_file(text_output);
    CHECK_NE(inspect_text.find("definition_id: " + definition_id), std::string::npos);
    CHECK_NE(inspect_text.find("name: FeatherDocDecimalList"), std::string::npos);
    CHECK_NE(inspect_text.find("instance[" + restarted_num_id + "]: override_count=1"),
             std::string::npos);
    CHECK_NE(inspect_text.find("override[0]: level=0 start_override=1"),
             std::string::npos);

    CHECK_EQ(run_cli({"inspect-numbering", source.string(), "--instance", restarted_num_id,
                      "--json"},
                     json_output),
             0);
    CHECK_EQ(
        read_text_file(json_output),
        std::string{
            "{\"instance\":{\"definition_id\":"} +
            definition_id +
            ",\"definition_name\":\"FeatherDocDecimalList\",\"instance\":{\"instance_id\":" +
            restarted_num_id +
            ",\"level_overrides\":[{\"level\":0,\"start_override\":1,"
            "\"level_definition\":null}]}}}\n");

    CHECK_EQ(run_cli({"inspect-numbering", source.string(), "--instance", "999", "--json"},
                     missing_output),
             1);
    const auto missing_json = read_text_file(missing_output);
    CHECK_NE(missing_json.find("\"command\":\"inspect-numbering\""), std::string::npos);
    CHECK_NE(missing_json.find("\"stage\":\"inspect\""), std::string::npos);
    CHECK_NE(missing_json.find(
                 "\"detail\":\"numbering instance id '999' was not found in "
                 "word/numbering.xml\""),
             std::string::npos);
    CHECK_NE(missing_json.find("\"entry\":\"word/numbering.xml\""), std::string::npos);

    remove_if_exists(source);
    remove_if_exists(text_output);
    remove_if_exists(json_output);
    remove_if_exists(missing_output);
}

TEST_CASE("cli export and import numbering catalog preserves instances and overrides") {
    const fs::path working_directory = fs::current_path();
    const fs::path source = working_directory / "cli_numbering_catalog_source.docx";
    const fs::path target = working_directory / "cli_numbering_catalog_target.docx";
    const fs::path imported = working_directory / "cli_numbering_catalog_imported.docx";
    const fs::path catalog_json =
        working_directory / "cli_numbering_catalog_export.json";
    const fs::path export_output =
        working_directory / "cli_numbering_catalog_export_output.json";
    const fs::path import_output =
        working_directory / "cli_numbering_catalog_import_output.json";

    remove_if_exists(source);
    remove_if_exists(target);
    remove_if_exists(imported);
    remove_if_exists(catalog_json);
    remove_if_exists(export_output);
    remove_if_exists(import_output);

    auto source_document = featherdoc::Document(source);
    REQUIRE_FALSE(source_document.create_empty());

    auto catalog = featherdoc::numbering_catalog{};
    auto catalog_definition = featherdoc::numbering_catalog_definition{};
    catalog_definition.definition.name = "CliCatalogOutline";
    catalog_definition.definition.levels = {
        featherdoc::numbering_level_definition{
            featherdoc::list_kind::decimal, 4U, 0U, "%1."},
        featherdoc::numbering_level_definition{
            featherdoc::list_kind::bullet, 1U, 1U, "o"},
    };
    catalog_definition.instances.push_back(
        featherdoc::numbering_instance_summary{41U, {}});

    auto restarted_instance = featherdoc::numbering_instance_summary{};
    restarted_instance.instance_id = 42U;
    restarted_instance.level_overrides.push_back(
        featherdoc::numbering_level_override_summary{0U, 7U, std::nullopt});
    restarted_instance.level_overrides.push_back(
        featherdoc::numbering_level_override_summary{
            1U,
            std::nullopt,
            featherdoc::numbering_level_definition{
                featherdoc::list_kind::decimal, 9U, 1U, "(%2)"}});
    catalog_definition.instances.push_back(std::move(restarted_instance));
    catalog.definitions.push_back(std::move(catalog_definition));

    const auto import_summary = source_document.import_numbering_catalog(catalog);
    REQUIRE(static_cast<bool>(import_summary));
    REQUIRE_FALSE(source_document.save());

    auto target_document = featherdoc::Document(target);
    REQUIRE_FALSE(target_document.create_empty());
    REQUIRE_FALSE(target_document.save());

    CHECK_EQ(run_cli({"export-numbering-catalog",
                      source.string(),
                      "--output",
                      catalog_json.string(),
                      "--json"},
                     export_output),
             0);
    CHECK_EQ(read_text_file(export_output),
             std::string{"{\"command\":\"export-numbering-catalog\",\"ok\":true,"} +
                 "\"output_path\":" + json_quote(catalog_json.string()) +
                 ",\"definition_count\":1,\"instance_count\":2}\n");

    const auto exported_catalog = read_text_file(catalog_json);
    CHECK_NE(exported_catalog.find("\"name\":\"CliCatalogOutline\""),
             std::string::npos);
    CHECK_NE(exported_catalog.find("\"level_overrides\":[]"),
             std::string::npos);
    CHECK_NE(exported_catalog.find(
                 "{\"level\":0,\"start_override\":7,\"level_definition\":null}"),
             std::string::npos);
    CHECK_NE(exported_catalog.find(
                 "{\"level\":1,\"start_override\":null,\"level_definition\":"
                 "{\"level\":1,\"kind\":\"decimal\",\"start\":9,"
                 "\"text_pattern\":\"(%2)\"}}"),
             std::string::npos);

    CHECK_EQ(run_cli({"import-numbering-catalog",
                      target.string(),
                      "--catalog-file",
                      catalog_json.string(),
                      "--output",
                      imported.string(),
                      "--json"},
                     import_output),
             0);
    const auto import_json = read_text_file(import_output);
    CHECK_NE(import_json.find("\"command\":\"import-numbering-catalog\""),
             std::string::npos);
    CHECK_NE(import_json.find("\"in_place\":false"), std::string::npos);
    CHECK_NE(import_json.find("\"catalog_file\":" + json_quote(catalog_json.string())),
             std::string::npos);
    CHECK_NE(import_json.find("\"input_definition_count\":1"), std::string::npos);
    CHECK_NE(import_json.find("\"imported_definition_count\":1"),
             std::string::npos);
    CHECK_NE(import_json.find("\"imported_instance_count\":2"),
             std::string::npos);
    CHECK_NE(import_json.find("\"name\":\"CliCatalogOutline\""),
             std::string::npos);

    auto imported_document = featherdoc::Document(imported);
    REQUIRE_FALSE(imported_document.open());
    const auto imported_definitions = imported_document.list_numbering_definitions();
    REQUIRE_FALSE(imported_document.last_error());
    const auto imported_definition =
        std::find_if(imported_definitions.begin(), imported_definitions.end(),
                     [](const auto &definition) {
                         return definition.name == "CliCatalogOutline";
                     });
    REQUIRE(imported_definition != imported_definitions.end());
    CHECK_EQ(imported_definition->levels.size(), 2U);
    CHECK_EQ(imported_definition->instances.size(), 2U);

    const auto restarted_imported_instance = std::find_if(
        imported_definition->instances.begin(), imported_definition->instances.end(),
        [](const auto &instance) { return instance.level_overrides.size() == 2U; });
    REQUIRE(restarted_imported_instance != imported_definition->instances.end());
    CHECK_EQ(restarted_imported_instance->level_overrides[0].level, 0U);
    REQUIRE(restarted_imported_instance->level_overrides[0].start_override.has_value());
    CHECK_EQ(*restarted_imported_instance->level_overrides[0].start_override, 7U);
    CHECK_EQ(restarted_imported_instance->level_overrides[1].level, 1U);
    REQUIRE(restarted_imported_instance->level_overrides[1]
                .level_definition.has_value());
    CHECK_EQ(restarted_imported_instance->level_overrides[1]
                 .level_definition->text_pattern,
             "(%2)");

    remove_if_exists(source);
    remove_if_exists(target);
    remove_if_exists(imported);
    remove_if_exists(catalog_json);
    remove_if_exists(export_output);
    remove_if_exists(import_output);
}

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

TEST_CASE("cli check-numbering-catalog gates docx catalog against baseline") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_check_numbering_catalog_source.docx";
    const fs::path baseline_catalog =
        working_directory / "cli_check_numbering_catalog_baseline.json";
    const fs::path drift_catalog =
        working_directory / "cli_check_numbering_catalog_drift.json";
    const fs::path dirty_catalog =
        working_directory / "cli_check_numbering_catalog_dirty.json";
    const fs::path generated_catalog =
        working_directory / "cli_check_numbering_catalog_generated.json";
    const fs::path match_output =
        working_directory / "cli_check_numbering_catalog_match_output.json";
    const fs::path drift_output =
        working_directory / "cli_check_numbering_catalog_drift_output.json";
    const fs::path dirty_output =
        working_directory / "cli_check_numbering_catalog_dirty_output.json";
    const fs::path parse_output =
        working_directory / "cli_check_numbering_catalog_parse_output.json";

    remove_if_exists(source);
    remove_if_exists(baseline_catalog);
    remove_if_exists(drift_catalog);
    remove_if_exists(dirty_catalog);
    remove_if_exists(generated_catalog);
    remove_if_exists(match_output);
    remove_if_exists(drift_output);
    remove_if_exists(dirty_output);
    remove_if_exists(parse_output);

    auto document = featherdoc::Document(source);
    REQUIRE_FALSE(document.create_empty());

    auto catalog = featherdoc::numbering_catalog{};
    auto catalog_definition = featherdoc::numbering_catalog_definition{};
    catalog_definition.definition.name = "CheckOutline";
    catalog_definition.definition.levels = {
        featherdoc::numbering_level_definition{
            featherdoc::list_kind::decimal, 1U, 0U, "%1."},
        featherdoc::numbering_level_definition{
            featherdoc::list_kind::bullet, 1U, 1U, "o"},
    };
    catalog_definition.instances.push_back(
        featherdoc::numbering_instance_summary{
            99U,
            {featherdoc::numbering_level_override_summary{0U, 3U,
                                                          std::nullopt}}});
    catalog.definitions.push_back(std::move(catalog_definition));

    const auto import_summary = document.import_numbering_catalog(catalog);
    REQUIRE(static_cast<bool>(import_summary));
    REQUIRE_FALSE(document.save());

    write_binary_file(
        baseline_catalog,
        "{\"definitions\":[{\"name\":\"CheckOutline\",\"levels\":["
        "{\"level\":0,\"kind\":\"decimal\",\"start\":1,"
        "\"text_pattern\":\"%1.\"},"
        "{\"level\":1,\"kind\":\"bullet\",\"start\":1,"
        "\"text_pattern\":\"o\"}],\"instances\":["
        "{\"instance_id\":99,\"level_overrides\":["
        "{\"level\":0,\"start_override\":3,"
        "\"level_definition\":null}]}]}]}\n");
    write_binary_file(
        drift_catalog,
        "{\"definitions\":[{\"name\":\"CheckOutline\",\"levels\":["
        "{\"level\":0,\"kind\":\"decimal\",\"start\":7,"
        "\"text_pattern\":\"%1.\"},"
        "{\"level\":1,\"kind\":\"bullet\",\"start\":1,"
        "\"text_pattern\":\"o\"}],\"instances\":["
        "{\"instance_id\":99,\"level_overrides\":["
        "{\"level\":0,\"start_override\":3,"
        "\"level_definition\":null}]}]}]}\n");
    write_binary_file(dirty_catalog,
                      "{\"definitions\":[{\"name\":\"\",\"levels\":[],"
                      "\"instances\":[]}]}\n");

    CHECK_EQ(run_cli({"check-numbering-catalog",
                      source.string(),
                      "--catalog-file",
                      baseline_catalog.string(),
                      "--output",
                      generated_catalog.string(),
                      "--json"},
                     match_output),
             0);
    const auto match_json = read_text_file(match_output);
    CHECK_NE(match_json.find("\"command\":\"check-numbering-catalog\""),
             std::string::npos);
    CHECK_NE(match_json.find("\"matches\":true"), std::string::npos);
    CHECK_NE(match_json.find("\"clean\":true"), std::string::npos);
    CHECK_NE(match_json.find("\"catalog_file\":" +
                             json_quote(baseline_catalog.string())),
             std::string::npos);
    CHECK_NE(match_json.find("\"generated_output_path\":" +
                             json_quote(generated_catalog.string())),
             std::string::npos);
    CHECK_NE(match_json.find("\"baseline_issue_count\":0"),
             std::string::npos);
    CHECK_NE(match_json.find("\"generated_issue_count\":0"),
             std::string::npos);
    CHECK_NE(match_json.find("\"changed_definition_count\":0"),
             std::string::npos);
    CHECK_NE(read_text_file(generated_catalog).find("\"name\":\"CheckOutline\""),
             std::string::npos);

    CHECK_EQ(run_cli({"check-numbering-catalog",
                      source.string(),
                      "--catalog-file",
                      drift_catalog.string(),
                      "--json"},
                     drift_output),
             1);
    const auto drift_json = read_text_file(drift_output);
    CHECK_NE(drift_json.find("\"matches\":false"), std::string::npos);
    CHECK_NE(drift_json.find("\"clean\":true"), std::string::npos);
    CHECK_NE(drift_json.find("\"changed_definition_count\":1"),
             std::string::npos);
    CHECK_NE(drift_json.find("\"changed_level_count\":1"),
             std::string::npos);

    CHECK_EQ(run_cli({"check-numbering-catalog",
                      source.string(),
                      "--catalog-file",
                      dirty_catalog.string(),
                      "--json"},
                     dirty_output),
             1);
    const auto dirty_json = read_text_file(dirty_output);
    CHECK_NE(dirty_json.find("\"matches\":false"), std::string::npos);
    CHECK_NE(dirty_json.find("\"clean\":false"), std::string::npos);
    CHECK_NE(dirty_json.find("\"baseline_issue_count\":2"),
             std::string::npos);
    CHECK_NE(dirty_json.find("\"issue\":\"empty_definition_name\""),
             std::string::npos);
    CHECK_NE(dirty_json.find("\"issue\":\"empty_levels\""),
             std::string::npos);

    CHECK_EQ(run_cli({"check-numbering-catalog", source.string(), "--json"},
                     parse_output),
             2);
    CHECK_EQ(read_text_file(parse_output),
             std::string{
                 "{\"command\":\"check-numbering-catalog\",\"ok\":false,"
                 "\"stage\":\"parse\",\"message\":\"missing "
                 "--catalog-file <catalog.json>\"}\n"});

    remove_if_exists(source);
    remove_if_exists(baseline_catalog);
    remove_if_exists(drift_catalog);
    remove_if_exists(dirty_catalog);
    remove_if_exists(generated_catalog);
    remove_if_exists(match_output);
    remove_if_exists(drift_output);
    remove_if_exists(dirty_output);
    remove_if_exists(parse_output);
}

TEST_CASE(
    "cli ensure-numbering-definition and set-paragraph-numbering manage custom numbering") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_custom_numbering_source.docx";
    const fs::path defined =
        working_directory / "cli_custom_numbering_defined.docx";
    const fs::path numbered =
        working_directory / "cli_custom_numbering_numbered.docx";
    const fs::path nested =
        working_directory / "cli_custom_numbering_nested.docx";
    const fs::path ensure_output =
        working_directory / "cli_custom_numbering_ensure_output.json";
    const fs::path numbered_output =
        working_directory / "cli_custom_numbering_numbered_output.json";
    const fs::path nested_output =
        working_directory / "cli_custom_numbering_nested_output.json";

    remove_if_exists(source);
    remove_if_exists(defined);
    remove_if_exists(numbered);
    remove_if_exists(nested);
    remove_if_exists(ensure_output);
    remove_if_exists(numbered_output);
    remove_if_exists(nested_output);

    create_cli_paragraph_list_fixture(source);

    CHECK_EQ(run_cli({"ensure-numbering-definition",
                      source.string(),
                      "--definition-name",
                      "LegalOutline",
                      "--numbering-level",
                      "0:decimal:3:%1.",
                      "--numbering-level",
                      "1:decimal:1:%1.%2.",
                      "--output",
                      defined.string(),
                      "--json"},
                     ensure_output),
             0);

    featherdoc::Document defined_document(defined);
    REQUIRE_FALSE(defined_document.open());
    const auto defined_definitions = defined_document.list_numbering_definitions();
    REQUIRE_FALSE(defined_document.last_error());
    const auto defined_definition =
        std::find_if(defined_definitions.begin(), defined_definitions.end(),
                     [](const auto &summary) {
                         return summary.name == "LegalOutline";
                     });
    REQUIRE(defined_definition != defined_definitions.end());
    CHECK(defined_definition->instance_ids.empty());
    const auto definition_id = std::to_string(defined_definition->definition_id);

    CHECK_EQ(
        read_text_file(ensure_output),
        std::string{
            "{\"command\":\"ensure-numbering-definition\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"definition_id\":"} +
            definition_id +
            ",\"definition_name\":\"LegalOutline\",\"definition_levels\":["
            "{\"level\":0,\"kind\":\"decimal\",\"start\":3,\"text_pattern\":\"%1.\"},"
            "{\"level\":1,\"kind\":\"decimal\",\"start\":1,\"text_pattern\":\"%1.%2.\"}]}\n");

    CHECK_EQ(run_cli({"set-paragraph-numbering",
                      defined.string(),
                      "0",
                      "--definition",
                      definition_id,
                      "--output",
                      numbered.string(),
                      "--json"},
                     numbered_output),
             0);
    CHECK_EQ(
        read_text_file(numbered_output),
        std::string{
            "{\"command\":\"set-paragraph-numbering\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"paragraph_index\":0,\"definition_id\":"} +
            definition_id + ",\"level\":0}\n");

    CHECK_EQ(run_cli({"set-paragraph-numbering",
                      numbered.string(),
                      "1",
                      "--definition",
                      definition_id,
                      "--level",
                      "1",
                      "--output",
                      nested.string(),
                      "--json"},
                     nested_output),
             0);
    CHECK_EQ(
        read_text_file(nested_output),
        std::string{
            "{\"command\":\"set-paragraph-numbering\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"paragraph_index\":1,\"definition_id\":"} +
            definition_id + ",\"level\":1}\n");

    featherdoc::Document nested_document(nested);
    REQUIRE_FALSE(nested_document.open());
    const auto nested_definitions = nested_document.list_numbering_definitions();
    REQUIRE_FALSE(nested_document.last_error());
    const auto nested_definition =
        std::find_if(nested_definitions.begin(), nested_definitions.end(),
                     [](const auto &summary) {
                         return summary.name == "LegalOutline";
                     });
    REQUIRE(nested_definition != nested_definitions.end());
    REQUIRE_EQ(nested_definition->instance_ids.size(), 1U);
    const auto num_id = std::to_string(nested_definition->instance_ids.front());

    const auto nested_document_xml = read_docx_entry(nested, "word/document.xml");
    pugi::xml_document nested_xml_document;
    REQUIRE(nested_xml_document.load_string(nested_document_xml.c_str()));
    const auto first_paragraph =
        find_body_paragraph_xml_node(nested_xml_document.child("w:document"), 0U);
    const auto second_paragraph =
        find_body_paragraph_xml_node(nested_xml_document.child("w:document"), 1U);
    const auto third_paragraph =
        find_body_paragraph_xml_node(nested_xml_document.child("w:document"), 2U);
    REQUIRE(first_paragraph != pugi::xml_node{});
    REQUIRE(second_paragraph != pugi::xml_node{});
    REQUIRE(third_paragraph != pugi::xml_node{});

    const auto first_num_pr = first_paragraph.child("w:pPr").child("w:numPr");
    const auto second_num_pr = second_paragraph.child("w:pPr").child("w:numPr");
    REQUIRE(first_num_pr != pugi::xml_node{});
    REQUIRE(second_num_pr != pugi::xml_node{});
    CHECK_EQ(std::string_view{first_num_pr.child("w:ilvl").attribute("w:val").value()},
             "0");
    CHECK_EQ(std::string_view{second_num_pr.child("w:ilvl").attribute("w:val").value()},
             "1");
    CHECK_EQ(std::string{first_num_pr.child("w:numId").attribute("w:val").value()},
             num_id);
    CHECK_EQ(std::string{second_num_pr.child("w:numId").attribute("w:val").value()},
             num_id);
    CHECK(third_paragraph.child("w:pPr").child("w:numPr") == pugi::xml_node{});

    remove_if_exists(source);
    remove_if_exists(defined);
    remove_if_exists(numbered);
    remove_if_exists(nested);
    remove_if_exists(ensure_output);
    remove_if_exists(numbered_output);
    remove_if_exists(nested_output);
}

TEST_CASE("cli custom numbering commands report parse and mutate errors") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_custom_numbering_error_source.docx";
    const fs::path ensure_output =
        working_directory / "cli_custom_numbering_error_ensure.json";
    const fs::path parse_output =
        working_directory / "cli_custom_numbering_error_parse.json";
    const fs::path mutate_output =
        working_directory / "cli_custom_numbering_error_mutate.json";

    remove_if_exists(source);
    remove_if_exists(ensure_output);
    remove_if_exists(parse_output);
    remove_if_exists(mutate_output);

    create_cli_paragraph_list_fixture(source);

    CHECK_EQ(run_cli({"ensure-numbering-definition",
                      source.string(),
                      "--definition-name",
                      "BrokenDefinition",
                      "--json"},
                     ensure_output),
             2);
    CHECK_EQ(
        read_text_file(ensure_output),
        std::string{
            "{\"command\":\"ensure-numbering-definition\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"expected at least one "
            "--numbering-level "
            "<level>:<kind>:<start>:<text-pattern>\"}\n"});

    CHECK_EQ(run_cli({"set-paragraph-numbering", source.string(), "0", "--json"},
                     parse_output),
             2);
    CHECK_EQ(
        read_text_file(parse_output),
        std::string{
            "{\"command\":\"set-paragraph-numbering\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"missing --definition <id>\"}\n"});

    CHECK_EQ(run_cli({"set-paragraph-numbering",
                      source.string(),
                      "0",
                      "--definition",
                      "999",
                      "--json"},
                     mutate_output),
             1);
    const auto mutate_json = read_text_file(mutate_output);
    CHECK_NE(mutate_json.find("\"command\":\"set-paragraph-numbering\""),
             std::string::npos);
    CHECK_NE(mutate_json.find("\"stage\":\"mutate\""), std::string::npos);
    CHECK_NE(mutate_json.find("\"detail\":\"numbering definition id does not exist\""),
             std::string::npos);
    CHECK_NE(mutate_json.find("\"entry\":\"word/numbering.xml\""),
             std::string::npos);

    remove_if_exists(source);
    remove_if_exists(ensure_output);
    remove_if_exists(parse_output);
    remove_if_exists(mutate_output);
}
