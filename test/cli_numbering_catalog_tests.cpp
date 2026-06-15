#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "cli_numbering_catalog_test_support.hpp"

#include <algorithm>

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
