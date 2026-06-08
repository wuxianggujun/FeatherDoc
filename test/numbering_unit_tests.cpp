#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "basic_docx_archive_test_support.hpp"
#include "basic_document_xml_test_support.hpp"

#include <featherdoc.hpp>

TEST_CASE("set_paragraph_list creates numbering parts and preserves them across reopen save") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "paragraph_list_roundtrip.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto first_item = doc.paragraphs();
    CHECK(doc.set_paragraph_list(first_item, featherdoc::list_kind::bullet));
    CHECK(first_item.add_run("bullet 0").has_next());

    auto nested_item = first_item.insert_paragraph_after("");
    CHECK(doc.set_paragraph_list(nested_item, featherdoc::list_kind::bullet, 1U));
    CHECK(nested_item.add_run("bullet 1").has_next());

    auto decimal_item = nested_item.insert_paragraph_after("");
    CHECK(doc.set_paragraph_list(decimal_item, featherdoc::list_kind::decimal));
    CHECK(decimal_item.add_run("decimal 0").has_next());

    CHECK_FALSE(doc.save());

    CHECK(test_docx_entry_exists(target, "word/numbering.xml"));

    const auto saved_relationships =
        read_test_docx_entry(target, "word/_rels/document.xml.rels");
    CHECK_NE(saved_relationships.find(
                 "Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/numbering\""),
             std::string::npos);
    CHECK_NE(saved_relationships.find("Target=\"numbering.xml\""), std::string::npos);

    const auto saved_content_types = read_test_docx_entry(target, test_content_types_xml_entry);
    CHECK_NE(saved_content_types.find("PartName=\"/word/numbering.xml\""), std::string::npos);
    CHECK_NE(saved_content_types.find(
                 "application/vnd.openxmlformats-officedocument.wordprocessingml.numbering+xml"),
             std::string::npos);

    const auto saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    CHECK_EQ(count_substring_occurrences(saved_document_xml, "<w:numPr>"), 3);

    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(saved_document_xml.c_str()));
    const auto body = xml_document.child("w:document").child("w:body");
    auto first_paragraph = body.child("w:p");
    REQUIRE(first_paragraph != pugi::xml_node{});
    auto second_paragraph = first_paragraph.next_sibling("w:p");
    REQUIRE(second_paragraph != pugi::xml_node{});
    auto third_paragraph = second_paragraph.next_sibling("w:p");
    REQUIRE(third_paragraph != pugi::xml_node{});

    const auto first_num_pr = first_paragraph.child("w:pPr").child("w:numPr");
    const auto second_num_pr = second_paragraph.child("w:pPr").child("w:numPr");
    const auto third_num_pr = third_paragraph.child("w:pPr").child("w:numPr");
    REQUIRE(first_num_pr != pugi::xml_node{});
    REQUIRE(second_num_pr != pugi::xml_node{});
    REQUIRE(third_num_pr != pugi::xml_node{});

    CHECK_EQ(std::string{first_num_pr.child("w:ilvl").attribute("w:val").value()}, "0");
    CHECK_EQ(std::string{second_num_pr.child("w:ilvl").attribute("w:val").value()}, "1");
    CHECK_EQ(std::string{third_num_pr.child("w:ilvl").attribute("w:val").value()}, "0");
    CHECK_EQ(std::string{first_num_pr.child("w:numId").attribute("w:val").value()},
             std::string{second_num_pr.child("w:numId").attribute("w:val").value()});
    CHECK_NE(std::string{first_num_pr.child("w:numId").attribute("w:val").value()},
             std::string{third_num_pr.child("w:numId").attribute("w:val").value()});

    const auto numbering_xml = read_test_docx_entry(target, "word/numbering.xml");
    CHECK_NE(numbering_xml.find("FeatherDocBulletList"), std::string::npos);
    CHECK_NE(numbering_xml.find("FeatherDocDecimalList"), std::string::npos);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK(reopened.paragraphs().add_run("tail").has_next());
    CHECK_FALSE(reopened.save());
    CHECK(test_docx_entry_exists(target, "word/numbering.xml"));

    fs::remove(target);
}

TEST_CASE("clear_paragraph_list removes numbering markup and invalid level is rejected") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "paragraph_list_clear.docx";
    fs::remove(target);

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p><w:r><w:t>seed</w:t></w:r></w:p>
  </w:body>
</w:document>
)";
    write_test_docx(target, document_xml);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    auto paragraph = doc.paragraphs();
    CHECK_FALSE(doc.set_paragraph_list(paragraph, featherdoc::list_kind::bullet, 9U));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));

    CHECK(doc.set_paragraph_list(paragraph, featherdoc::list_kind::bullet));
    CHECK(doc.clear_paragraph_list(paragraph));
    CHECK_FALSE(doc.save());

    const auto saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    CHECK_EQ(count_substring_occurrences(saved_document_xml, "<w:numPr>"), 0);
    CHECK_EQ(count_substring_occurrences(saved_document_xml, "<w:pPr>"), 0);

    fs::remove(target);
}

TEST_CASE("restart_paragraph_list creates a fresh numbering instance and adjacent items continue it") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "paragraph_list_restart.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto first_item = doc.paragraphs();
    CHECK(doc.set_paragraph_list(first_item, featherdoc::list_kind::decimal));
    CHECK(first_item.add_run("first list item").has_next());

    auto second_item = first_item.insert_paragraph_after("");
    CHECK(doc.set_paragraph_list(second_item, featherdoc::list_kind::decimal));
    CHECK(second_item.add_run("first list item 2").has_next());

    auto spacer = second_item.insert_paragraph_after("restart here");
    REQUIRE(spacer.has_next());

    auto restarted_first = spacer.insert_paragraph_after("");
    CHECK(doc.restart_paragraph_list(restarted_first, featherdoc::list_kind::decimal));
    CHECK(restarted_first.add_run("second list item 1").has_next());

    auto restarted_second = restarted_first.insert_paragraph_after("");
    CHECK(doc.set_paragraph_list(restarted_second, featherdoc::list_kind::decimal));
    CHECK(restarted_second.add_run("second list item 2").has_next());

    CHECK_FALSE(doc.save());

    const auto saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(saved_document_xml.c_str()));

    const auto body = xml_document.child("w:document").child("w:body");
    auto paragraph = body.child("w:p");
    REQUIRE(paragraph != pugi::xml_node{});
    auto second_paragraph = paragraph.next_sibling("w:p");
    REQUIRE(second_paragraph != pugi::xml_node{});
    auto spacer_paragraph = second_paragraph.next_sibling("w:p");
    REQUIRE(spacer_paragraph != pugi::xml_node{});
    auto restarted_first_paragraph = spacer_paragraph.next_sibling("w:p");
    REQUIRE(restarted_first_paragraph != pugi::xml_node{});
    auto restarted_second_paragraph = restarted_first_paragraph.next_sibling("w:p");
    REQUIRE(restarted_second_paragraph != pugi::xml_node{});

    const auto first_num_pr = paragraph.child("w:pPr").child("w:numPr");
    const auto second_num_pr = second_paragraph.child("w:pPr").child("w:numPr");
    const auto restarted_first_num_pr =
        restarted_first_paragraph.child("w:pPr").child("w:numPr");
    const auto restarted_second_num_pr =
        restarted_second_paragraph.child("w:pPr").child("w:numPr");
    REQUIRE(first_num_pr != pugi::xml_node{});
    REQUIRE(second_num_pr != pugi::xml_node{});
    REQUIRE(restarted_first_num_pr != pugi::xml_node{});
    REQUIRE(restarted_second_num_pr != pugi::xml_node{});

    const auto first_num_id = std::string{first_num_pr.child("w:numId").attribute("w:val").value()};
    const auto second_num_id =
        std::string{second_num_pr.child("w:numId").attribute("w:val").value()};
    const auto restarted_first_num_id =
        std::string{restarted_first_num_pr.child("w:numId").attribute("w:val").value()};
    const auto restarted_second_num_id =
        std::string{restarted_second_num_pr.child("w:numId").attribute("w:val").value()};

    CHECK_EQ(first_num_id, second_num_id);
    CHECK_NE(first_num_id, restarted_first_num_id);
    CHECK_EQ(restarted_first_num_id, restarted_second_num_id);

    const auto numbering_xml = read_test_docx_entry(target, "word/numbering.xml");
    pugi::xml_document numbering_document;
    REQUIRE(numbering_document.load_string(numbering_xml.c_str()));
    const auto numbering_root = numbering_document.child("w:numbering");
    REQUIRE(numbering_root != pugi::xml_node{});

    auto restarted_numbering_instance = pugi::xml_node{};
    for (auto numbering_instance = numbering_root.child("w:num");
         numbering_instance != pugi::xml_node{};
         numbering_instance = numbering_instance.next_sibling("w:num")) {
        if (std::string_view{numbering_instance.attribute("w:numId").value()} ==
            restarted_first_num_id) {
            restarted_numbering_instance = numbering_instance;
            break;
        }
    }
    REQUIRE(restarted_numbering_instance != pugi::xml_node{});
    const auto restart_level_override = restarted_numbering_instance.child("w:lvlOverride");
    REQUIRE(restart_level_override != pugi::xml_node{});
    CHECK_EQ(std::string_view{restart_level_override.attribute("w:ilvl").value()}, "0");
    CHECK_EQ(std::string_view{
                 restart_level_override.child("w:startOverride").attribute("w:val").value()},
             "1");

    CHECK_EQ(count_substring_occurrences(numbering_xml, "<w:num "), 2);
    CHECK_EQ(count_substring_occurrences(numbering_xml, "FeatherDocDecimalList"), 1);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());

    auto reopened_paragraph = reopened.paragraphs();
    REQUIRE(reopened_paragraph.has_next());
    reopened_paragraph.next();
    REQUIRE(reopened_paragraph.has_next());
    reopened_paragraph.next();
    REQUIRE(reopened_paragraph.has_next());
    reopened_paragraph.next();
    REQUIRE(reopened_paragraph.has_next());
    reopened_paragraph.next();
    REQUIRE(reopened_paragraph.has_next());

    CHECK(reopened.restart_paragraph_list(reopened_paragraph, featherdoc::list_kind::decimal));
    CHECK_FALSE(reopened.save());

    fs::remove(target);
}

TEST_CASE("numbering metadata exposes instance overrides for restarted managed lists") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "managed_numbering_metadata.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto first_item = doc.paragraphs();
    REQUIRE(first_item.has_next());
    CHECK(doc.set_paragraph_list(first_item, featherdoc::list_kind::decimal));
    CHECK(first_item.add_run("first item").has_next());

    auto spacer = first_item.insert_paragraph_after("restart here");
    REQUIRE(spacer.has_next());

    auto restarted_item = spacer.insert_paragraph_after("");
    REQUIRE(restarted_item.has_next());
    CHECK(doc.restart_paragraph_list(restarted_item, featherdoc::list_kind::decimal));
    CHECK(restarted_item.add_run("restarted item").has_next());

    const auto definitions = doc.list_numbering_definitions();
    CHECK_FALSE(doc.last_error());
    const auto managed_definition =
        std::find_if(definitions.begin(), definitions.end(), [](const auto &summary) {
            return summary.name == "FeatherDocDecimalList";
        });
    REQUIRE(managed_definition != definitions.end());
    REQUIRE_EQ(managed_definition->instance_ids.size(), 2U);
    REQUIRE_EQ(managed_definition->instances.size(), 2U);
    CHECK_EQ(managed_definition->instances[0].instance_id,
             managed_definition->instance_ids[0]);
    CHECK(managed_definition->instances[0].level_overrides.empty());
    CHECK_EQ(managed_definition->instances[1].instance_id,
             managed_definition->instance_ids[1]);
    REQUIRE_EQ(managed_definition->instances[1].level_overrides.size(), 1U);
    CHECK_EQ(managed_definition->instances[1].level_overrides[0].level, 0U);
    REQUIRE(managed_definition->instances[1].level_overrides[0].start_override.has_value());
    CHECK_EQ(*managed_definition->instances[1].level_overrides[0].start_override, 1U);
    CHECK_FALSE(
        managed_definition->instances[1].level_overrides[0].level_definition.has_value());

    const auto found_instance =
        doc.find_numbering_instance(managed_definition->instance_ids[1]);
    CHECK_FALSE(doc.last_error());
    REQUIRE(found_instance.has_value());
    CHECK_EQ(found_instance->definition_id, managed_definition->definition_id);
    CHECK_EQ(found_instance->definition_name, managed_definition->name);
    CHECK_EQ(found_instance->instance.instance_id, managed_definition->instance_ids[1]);
    REQUIRE_EQ(found_instance->instance.level_overrides.size(), 1U);
    CHECK_EQ(found_instance->instance.level_overrides[0].level, 0U);
    REQUIRE(found_instance->instance.level_overrides[0].start_override.has_value());
    CHECK_EQ(*found_instance->instance.level_overrides[0].start_override, 1U);

    const auto found = doc.find_numbering_definition(managed_definition->definition_id);
    CHECK_FALSE(doc.last_error());
    REQUIRE(found.has_value());
    CHECK_EQ(found->instance_ids, managed_definition->instance_ids);
    CHECK_EQ(found->instances.size(), managed_definition->instances.size());
    REQUIRE_EQ(found->instances[1].level_overrides.size(), 1U);
    CHECK_EQ(found->instances[1].level_overrides[0].level, 0U);
    REQUIRE(found->instances[1].level_overrides[0].start_override.has_value());
    CHECK_EQ(*found->instances[1].level_overrides[0].start_override, 1U);

    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());

    const auto reopened_definition =
        reopened.find_numbering_definition(managed_definition->definition_id);
    CHECK_FALSE(reopened.last_error());
    REQUIRE(reopened_definition.has_value());
    CHECK_EQ(reopened_definition->instance_ids, managed_definition->instance_ids);
    CHECK_EQ(reopened_definition->instances.size(), managed_definition->instances.size());
    REQUIRE_EQ(reopened_definition->instances[1].level_overrides.size(), 1U);
    CHECK_EQ(reopened_definition->instances[1].level_overrides[0].level, 0U);
    REQUIRE(reopened_definition->instances[1].level_overrides[0].start_override.has_value());
    CHECK_EQ(*reopened_definition->instances[1].level_overrides[0].start_override, 1U);

    const auto reopened_instance =
        reopened.find_numbering_instance(managed_definition->instance_ids[1]);
    CHECK_FALSE(reopened.last_error());
    REQUIRE(reopened_instance.has_value());
    CHECK_EQ(reopened_instance->definition_id, managed_definition->definition_id);
    CHECK_EQ(reopened_instance->definition_name, managed_definition->name);
    REQUIRE_EQ(reopened_instance->instance.level_overrides.size(), 1U);
    CHECK_EQ(reopened_instance->instance.level_overrides[0].level, 0U);
    REQUIRE(reopened_instance->instance.level_overrides[0].start_override.has_value());
    CHECK_EQ(*reopened_instance->instance.level_overrides[0].start_override, 1U);

    fs::remove(target);
}

TEST_CASE("numbering catalog exports and imports definitions with instance overrides") {
    namespace fs = std::filesystem;

    const fs::path source_path = fs::current_path() / "numbering_catalog_source.docx";
    const fs::path imported_path = fs::current_path() / "numbering_catalog_imported.docx";
    fs::remove(source_path);
    fs::remove(imported_path);

    featherdoc::Document source(source_path);
    CHECK_FALSE(source.create_empty());

    auto definition = featherdoc::numbering_definition{};
    definition.name = "CatalogOutline";
    definition.levels = {
        featherdoc::numbering_level_definition{
            featherdoc::list_kind::decimal, 1U, 0U, "%1."},
        featherdoc::numbering_level_definition{
            featherdoc::list_kind::decimal, 1U, 1U, "%1.%2."},
    };

    const auto definition_id = source.ensure_numbering_definition(definition);
    REQUIRE(definition_id.has_value());
    auto paragraph = source.paragraphs();
    REQUIRE(paragraph.has_next());
    CHECK(source.set_paragraph_numbering(paragraph, *definition_id, 0U));

    auto exported_catalog = source.export_numbering_catalog();
    CHECK_FALSE(source.last_error());
    const auto exported_it = std::find_if(
        exported_catalog.definitions.begin(), exported_catalog.definitions.end(),
        [](const auto &catalog_definition) {
            return catalog_definition.definition.name == "CatalogOutline";
        });
    REQUIRE(exported_it != exported_catalog.definitions.end());
    REQUIRE_EQ(exported_it->instances.size(), 1U);
    CHECK(exported_it->instances.front().level_overrides.empty());

    auto override_definition = featherdoc::numbering_catalog_definition{};
    override_definition.definition.name = "RestartedCatalog";
    override_definition.definition.levels = definition.levels;
    auto override_instance = featherdoc::numbering_instance_summary{};
    override_instance.instance_id = 42U;
    override_instance.level_overrides.push_back(
        featherdoc::numbering_level_override_summary{0U, 3U, std::nullopt});
    override_definition.instances.push_back(override_instance);
    exported_catalog.definitions = {*exported_it, override_definition};

    featherdoc::Document imported(imported_path);
    CHECK_FALSE(imported.create_empty());
    const auto import_summary = imported.import_numbering_catalog(exported_catalog);
    CHECK_FALSE(imported.last_error());
    CHECK(import_summary);
    CHECK_EQ(import_summary.input_definition_count, 2U);
    CHECK_EQ(import_summary.imported_definition_count, 2U);
    CHECK_EQ(import_summary.imported_instance_count, 2U);
    REQUIRE_EQ(import_summary.definitions.size(), 2U);

    const auto imported_catalog_definition = std::find_if(
        import_summary.definitions.begin(), import_summary.definitions.end(),
        [](const auto &summary) { return summary.name == "CatalogOutline"; });
    REQUIRE(imported_catalog_definition != import_summary.definitions.end());
    REQUIRE_EQ(imported_catalog_definition->instance_ids.size(), 1U);

    const auto imported_definition = imported.find_numbering_definition(
        imported_catalog_definition->definition_id);
    REQUIRE(imported_definition.has_value());
    CHECK_EQ(imported_definition->name, "CatalogOutline");
    CHECK_EQ(imported_definition->levels.size(), 2U);
    REQUIRE_EQ(imported_definition->instances.size(), 1U);
    CHECK(imported_definition->instances.front().level_overrides.empty());

    const auto restarted_catalog_definition = std::find_if(
        import_summary.definitions.begin(), import_summary.definitions.end(),
        [](const auto &summary) { return summary.name == "RestartedCatalog"; });
    REQUIRE(restarted_catalog_definition != import_summary.definitions.end());
    REQUIRE_EQ(restarted_catalog_definition->instance_ids.size(), 1U);
    const auto restarted_instance = imported.find_numbering_instance(
        restarted_catalog_definition->instance_ids.front());
    REQUIRE(restarted_instance.has_value());
    CHECK_EQ(restarted_instance->definition_name, "RestartedCatalog");
    REQUIRE_EQ(restarted_instance->instance.level_overrides.size(), 1U);
    CHECK_EQ(restarted_instance->instance.level_overrides.front().level, 0U);
    REQUIRE(restarted_instance->instance.level_overrides.front().start_override.has_value());
    CHECK_EQ(*restarted_instance->instance.level_overrides.front().start_override, 3U);

    CHECK_FALSE(imported.save());
    featherdoc::Document reopened(imported_path);
    CHECK_FALSE(reopened.open());
    const auto reopened_instance = reopened.find_numbering_instance(
        restarted_catalog_definition->instance_ids.front());
    REQUIRE(reopened_instance.has_value());
    REQUIRE_EQ(reopened_instance->instance.level_overrides.size(), 1U);
    CHECK_EQ(*reopened_instance->instance.level_overrides.front().start_override, 3U);

    fs::remove(source_path);
    fs::remove(imported_path);
}

TEST_CASE(
    "ensure_numbering_definition and set_paragraph_numbering create custom numbering "
    "definitions and round-trip") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "custom_numbering_roundtrip.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto definition = featherdoc::numbering_definition{};
    definition.name = "LegalOutline";
    definition.levels = {
        featherdoc::numbering_level_definition{
            featherdoc::list_kind::decimal, 3U, 0U, "%1."},
        featherdoc::numbering_level_definition{
            featherdoc::list_kind::decimal, 1U, 1U, "%1.%2."},
    };

    const auto numbering_id = doc.ensure_numbering_definition(definition);
    REQUIRE(numbering_id.has_value());

    const auto reused_numbering_id = doc.ensure_numbering_definition(definition);
    REQUIRE(reused_numbering_id.has_value());
    CHECK_EQ(*reused_numbering_id, *numbering_id);

    auto first = doc.paragraphs();
    REQUIRE(first.has_next());
    CHECK(doc.set_paragraph_numbering(first, *numbering_id));
    CHECK(first.add_run("chapter 3").has_next());

    auto second = first.insert_paragraph_after("");
    REQUIRE(second.has_next());
    CHECK(doc.set_paragraph_numbering(second, *numbering_id, 1U));
    CHECK(second.add_run("chapter 3.1").has_next());

    auto third = second.insert_paragraph_after("");
    REQUIRE(third.has_next());
    CHECK(doc.set_paragraph_numbering(third, *numbering_id));
    CHECK(third.add_run("chapter 4").has_next());

    CHECK_FALSE(doc.save());

    const auto numbering_xml = read_test_docx_entry(target, "word/numbering.xml");
    pugi::xml_document numbering_document;
    REQUIRE(numbering_document.load_string(numbering_xml.c_str()));

    const auto numbering_root = numbering_document.child("w:numbering");
    REQUIRE(numbering_root != pugi::xml_node{});

    const auto abstract_num = find_numbering_abstract_xml_node(numbering_root, "LegalOutline");
    REQUIRE(abstract_num != pugi::xml_node{});
    CHECK_EQ(std::string_view{abstract_num.attribute("w:abstractNumId").value()},
             std::to_string(*numbering_id));
    CHECK_EQ(count_named_children(abstract_num, "w:lvl"), 2U);

    const auto level_zero = find_numbering_level_xml_node(abstract_num, 0U);
    const auto level_one = find_numbering_level_xml_node(abstract_num, 1U);
    REQUIRE(level_zero != pugi::xml_node{});
    REQUIRE(level_one != pugi::xml_node{});
    CHECK_EQ(std::string_view{level_zero.child("w:start").attribute("w:val").value()}, "3");
    CHECK_EQ(std::string_view{level_zero.child("w:numFmt").attribute("w:val").value()},
             "decimal");
    CHECK_EQ(std::string_view{level_zero.child("w:lvlText").attribute("w:val").value()},
             "%1.");
    CHECK_EQ(std::string_view{level_one.child("w:start").attribute("w:val").value()}, "1");
    CHECK_EQ(std::string_view{level_one.child("w:lvlText").attribute("w:val").value()},
             "%1.%2.");
    CHECK_EQ(count_substring_occurrences(numbering_xml, "<w:num "), 1U);

    const auto saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document document_xml;
    REQUIRE(document_xml.load_string(saved_document_xml.c_str()));
    const auto body = document_xml.child("w:document").child("w:body");
    auto first_paragraph = body.child("w:p");
    REQUIRE(first_paragraph != pugi::xml_node{});
    auto second_paragraph = first_paragraph.next_sibling("w:p");
    REQUIRE(second_paragraph != pugi::xml_node{});
    auto third_paragraph = second_paragraph.next_sibling("w:p");
    REQUIRE(third_paragraph != pugi::xml_node{});

    const auto first_num_pr = first_paragraph.child("w:pPr").child("w:numPr");
    const auto second_num_pr = second_paragraph.child("w:pPr").child("w:numPr");
    const auto third_num_pr = third_paragraph.child("w:pPr").child("w:numPr");
    REQUIRE(first_num_pr != pugi::xml_node{});
    REQUIRE(second_num_pr != pugi::xml_node{});
    REQUIRE(third_num_pr != pugi::xml_node{});
    CHECK_EQ(std::string_view{first_num_pr.child("w:ilvl").attribute("w:val").value()}, "0");
    CHECK_EQ(std::string_view{second_num_pr.child("w:ilvl").attribute("w:val").value()}, "1");
    CHECK_EQ(std::string_view{third_num_pr.child("w:ilvl").attribute("w:val").value()}, "0");

    const auto first_num_id =
        std::string{first_num_pr.child("w:numId").attribute("w:val").value()};
    CHECK_EQ(first_num_id,
             std::string{second_num_pr.child("w:numId").attribute("w:val").value()});
    CHECK_EQ(first_num_id,
             std::string{third_num_pr.child("w:numId").attribute("w:val").value()});

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());

    const auto reopened_numbering_id = reopened.ensure_numbering_definition(definition);
    REQUIRE(reopened_numbering_id.has_value());
    CHECK_EQ(*reopened_numbering_id, *numbering_id);

    auto reopened_paragraph = reopened.paragraphs();
    REQUIRE(reopened_paragraph.has_next());
    auto inserted = reopened_paragraph.insert_paragraph_after("");
    REQUIRE(inserted.has_next());
    CHECK(reopened.set_paragraph_numbering(inserted, *reopened_numbering_id, 1U));
    CHECK(inserted.add_run("inserted subsection").has_next());
    CHECK_FALSE(reopened.save());

    fs::remove(target);
}

TEST_CASE("ensure_numbering_definition updates existing custom numbering definitions") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "custom_numbering_update.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto definition = featherdoc::numbering_definition{};
    definition.name = "PolicyOutline";
    definition.levels = {
        featherdoc::numbering_level_definition{
            featherdoc::list_kind::decimal, 1U, 0U, "%1)"},
    };

    const auto initial_numbering_id = doc.ensure_numbering_definition(definition);
    REQUIRE(initial_numbering_id.has_value());

    definition.levels = {
        featherdoc::numbering_level_definition{
            featherdoc::list_kind::decimal, 5U, 0U, "(%1)"},
        featherdoc::numbering_level_definition{
            featherdoc::list_kind::bullet, 1U, 1U, "o"},
    };

    const auto updated_numbering_id = doc.ensure_numbering_definition(definition);
    REQUIRE(updated_numbering_id.has_value());
    CHECK_EQ(*updated_numbering_id, *initial_numbering_id);

    auto paragraph = doc.paragraphs();
    REQUIRE(paragraph.has_next());
    CHECK(doc.set_paragraph_numbering(paragraph, *updated_numbering_id));
    CHECK(paragraph.add_run("policy item").has_next());

    CHECK_FALSE(doc.save());

    const auto numbering_xml = read_test_docx_entry(target, "word/numbering.xml");
    pugi::xml_document numbering_document;
    REQUIRE(numbering_document.load_string(numbering_xml.c_str()));
    const auto numbering_root = numbering_document.child("w:numbering");
    REQUIRE(numbering_root != pugi::xml_node{});

    const auto abstract_num = find_numbering_abstract_xml_node(numbering_root, "PolicyOutline");
    REQUIRE(abstract_num != pugi::xml_node{});
    CHECK_EQ(std::string_view{abstract_num.attribute("w:abstractNumId").value()},
             std::to_string(*initial_numbering_id));
    CHECK_EQ(count_named_children(abstract_num, "w:lvl"), 2U);

    const auto level_zero = find_numbering_level_xml_node(abstract_num, 0U);
    const auto level_one = find_numbering_level_xml_node(abstract_num, 1U);
    REQUIRE(level_zero != pugi::xml_node{});
    REQUIRE(level_one != pugi::xml_node{});
    CHECK_EQ(std::string_view{level_zero.child("w:start").attribute("w:val").value()}, "5");
    CHECK_EQ(std::string_view{level_zero.child("w:lvlText").attribute("w:val").value()},
             "(%1)");
    CHECK_EQ(std::string_view{level_one.child("w:numFmt").attribute("w:val").value()},
             "bullet");
    CHECK_EQ(std::string_view{level_one.child("w:lvlText").attribute("w:val").value()}, "o");

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());

    auto reopened_paragraph = reopened.paragraphs();
    REQUIRE(reopened_paragraph.has_next());
    auto nested = reopened_paragraph.insert_paragraph_after("");
    REQUIRE(nested.has_next());
    CHECK(reopened.set_paragraph_numbering(nested, *updated_numbering_id, 1U));
    CHECK(nested.add_run("policy sub item").has_next());
    CHECK_FALSE(reopened.save());

    fs::remove(target);
}

TEST_CASE("list_numbering_definitions and find_numbering_definition expose numbering metadata") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "custom_numbering_metadata.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto definition = featherdoc::numbering_definition{};
    definition.name = "LegalOutline";
    definition.levels = {
        featherdoc::numbering_level_definition{
            featherdoc::list_kind::decimal, 3U, 0U, "%1."},
        featherdoc::numbering_level_definition{
            featherdoc::list_kind::decimal, 1U, 1U, "%1.%2."},
    };

    const auto numbering_id = doc.ensure_numbering_definition(definition);
    REQUIRE(numbering_id.has_value());

    auto paragraph = doc.paragraphs();
    REQUIRE(paragraph.has_next());
    CHECK(doc.set_paragraph_numbering(paragraph, *numbering_id));
    CHECK(paragraph.add_run("legal heading").has_next());

    const auto definitions = doc.list_numbering_definitions();
    CHECK_FALSE(doc.last_error());
    REQUIRE_EQ(definitions.size(), 1U);

    const auto &summary = definitions.front();
    CHECK_EQ(summary.definition_id, *numbering_id);
    CHECK_EQ(summary.name, "LegalOutline");
    REQUIRE_EQ(summary.levels.size(), 2U);
    CHECK_EQ(summary.levels[0].level, 0U);
    CHECK_EQ(summary.levels[0].kind, featherdoc::list_kind::decimal);
    CHECK_EQ(summary.levels[0].start, 3U);
    CHECK_EQ(summary.levels[0].text_pattern, "%1.");
    CHECK_EQ(summary.levels[1].level, 1U);
    CHECK_EQ(summary.levels[1].kind, featherdoc::list_kind::decimal);
    CHECK_EQ(summary.levels[1].start, 1U);
    CHECK_EQ(summary.levels[1].text_pattern, "%1.%2.");
    REQUIRE_EQ(summary.instance_ids.size(), 1U);
    CHECK_NE(summary.instance_ids.front(), 0U);
    REQUIRE_EQ(summary.instances.size(), 1U);
    CHECK_EQ(summary.instances.front().instance_id, summary.instance_ids.front());
    CHECK(summary.instances.front().level_overrides.empty());

    const auto found = doc.find_numbering_definition(*numbering_id);
    CHECK_FALSE(doc.last_error());
    REQUIRE(found.has_value());
    CHECK_EQ(found->definition_id, summary.definition_id);
    CHECK_EQ(found->name, summary.name);
    CHECK_EQ(found->levels.size(), summary.levels.size());
    CHECK_EQ(found->instance_ids, summary.instance_ids);
    REQUIRE_EQ(found->instances.size(), 1U);
    CHECK_EQ(found->instances.front().instance_id, summary.instance_ids.front());
    CHECK(found->instances.front().level_overrides.empty());

    const auto found_instance = doc.find_numbering_instance(summary.instance_ids.front());
    CHECK_FALSE(doc.last_error());
    REQUIRE(found_instance.has_value());
    CHECK_EQ(found_instance->definition_id, summary.definition_id);
    CHECK_EQ(found_instance->definition_name, summary.name);
    CHECK_EQ(found_instance->instance.instance_id, summary.instance_ids.front());
    CHECK(found_instance->instance.level_overrides.empty());

    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());

    const auto reopened_summary = reopened.find_numbering_definition(*numbering_id);
    CHECK_FALSE(reopened.last_error());
    REQUIRE(reopened_summary.has_value());
    CHECK_EQ(reopened_summary->definition_id, *numbering_id);
    CHECK_EQ(reopened_summary->name, "LegalOutline");
    REQUIRE_EQ(reopened_summary->levels.size(), 2U);
    CHECK_EQ(reopened_summary->levels[0].start, 3U);
    CHECK_EQ(reopened_summary->levels[1].text_pattern, "%1.%2.");
    CHECK_EQ(reopened_summary->instance_ids, summary.instance_ids);
    REQUIRE_EQ(reopened_summary->instances.size(), 1U);
    CHECK_EQ(reopened_summary->instances.front().instance_id, summary.instance_ids.front());
    CHECK(reopened_summary->instances.front().level_overrides.empty());

    const auto reopened_instance =
        reopened.find_numbering_instance(summary.instance_ids.front());
    CHECK_FALSE(reopened.last_error());
    REQUIRE(reopened_instance.has_value());
    CHECK_EQ(reopened_instance->definition_id, *numbering_id);
    CHECK_EQ(reopened_instance->definition_name, "LegalOutline");
    CHECK_EQ(reopened_instance->instance.instance_id, summary.instance_ids.front());
    CHECK(reopened_instance->instance.level_overrides.empty());

    const auto reopened_definitions = reopened.list_numbering_definitions();
    CHECK_FALSE(reopened.last_error());
    REQUIRE_EQ(reopened_definitions.size(), 1U);
    CHECK_EQ(reopened_definitions.front().definition_id, *numbering_id);
    CHECK_EQ(reopened_definitions.front().instance_ids, summary.instance_ids);
    REQUIRE_EQ(reopened_definitions.front().instances.size(), 1U);
    CHECK_EQ(reopened_definitions.front().instances.front().instance_id,
             summary.instance_ids.front());
    CHECK(reopened_definitions.front().instances.front().level_overrides.empty());

    fs::remove(target);
}

TEST_CASE(
    "custom numbering definition APIs validate inputs and reject missing definitions") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "custom_numbering_validation.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto invalid_definition = featherdoc::numbering_definition{};
    CHECK_FALSE(doc.ensure_numbering_definition(invalid_definition).has_value());
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));

    invalid_definition.name = "EmptyLevels";
    CHECK_FALSE(doc.ensure_numbering_definition(invalid_definition).has_value());
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));

    invalid_definition.name = "ReservedName";
    invalid_definition.levels = {
        featherdoc::numbering_level_definition{
            featherdoc::list_kind::decimal, 0U, 0U, "%1."},
    };
    CHECK_FALSE(doc.ensure_numbering_definition(invalid_definition).has_value());
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));

    invalid_definition.levels = {
        featherdoc::numbering_level_definition{
            featherdoc::list_kind::decimal, 1U, 9U, "%1."},
    };
    CHECK_FALSE(doc.ensure_numbering_definition(invalid_definition).has_value());
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));

    invalid_definition.levels = {
        featherdoc::numbering_level_definition{
            featherdoc::list_kind::decimal, 1U, 0U, ""},
    };
    CHECK_FALSE(doc.ensure_numbering_definition(invalid_definition).has_value());
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));

    invalid_definition.levels = {
        featherdoc::numbering_level_definition{
            featherdoc::list_kind::decimal, 1U, 0U, "%1."},
        featherdoc::numbering_level_definition{
            featherdoc::list_kind::bullet, 1U, 0U, "o"},
    };
    CHECK_FALSE(doc.ensure_numbering_definition(invalid_definition).has_value());
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));

    invalid_definition.name = "FeatherDocBulletList";
    invalid_definition.levels = {
        featherdoc::numbering_level_definition{
            featherdoc::list_kind::bullet, 1U, 0U, "o"},
    };
    CHECK_FALSE(doc.ensure_numbering_definition(invalid_definition).has_value());
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));

    auto valid_definition = featherdoc::numbering_definition{};
    valid_definition.name = "ValidOutline";
    valid_definition.levels = {
        featherdoc::numbering_level_definition{
            featherdoc::list_kind::decimal, 1U, 0U, "%1."},
    };

    const auto numbering_id = doc.ensure_numbering_definition(valid_definition);
    REQUIRE(numbering_id.has_value());

    CHECK_FALSE(doc.set_paragraph_numbering(featherdoc::Paragraph{}, *numbering_id));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));

    CHECK_FALSE(doc.find_numbering_instance(9999U).has_value());
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));

    auto paragraph = doc.paragraphs();
    REQUIRE(paragraph.has_next());
    CHECK_FALSE(doc.set_paragraph_numbering(paragraph, 9999U));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));

    CHECK_FALSE(doc.set_paragraph_numbering(paragraph, *numbering_id, 1U));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));

    CHECK_FALSE(doc.set_paragraph_numbering(paragraph, *numbering_id, 9U));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));

    fs::remove(target);
}

TEST_CASE(
    "set_paragraph_style_numbering links custom numbering definitions to paragraph styles") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "style_numbering_roundtrip.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto paragraph_style = featherdoc::paragraph_style_definition{};
    paragraph_style.name = "Legal Heading";
    paragraph_style.based_on = std::string{"Heading1"};
    paragraph_style.paragraph_bidi = false;
    CHECK(doc.ensure_paragraph_style("LegalHeading", paragraph_style));

    auto numbering_definition = featherdoc::numbering_definition{};
    numbering_definition.name = "LegalHeadingOutline";
    numbering_definition.levels = {
        featherdoc::numbering_level_definition{
            featherdoc::list_kind::decimal, 1U, 0U, "%1."},
        featherdoc::numbering_level_definition{
            featherdoc::list_kind::decimal, 1U, 1U, "%1.%2."},
    };

    const auto numbering_id = doc.ensure_numbering_definition(numbering_definition);
    REQUIRE(numbering_id.has_value());
    CHECK(doc.set_paragraph_style_numbering("LegalHeading", *numbering_id, 1U));

    const auto style_summary = doc.find_style("LegalHeading");
    REQUIRE(style_summary.has_value());
    REQUIRE(style_summary->numbering.has_value());
    REQUIRE(style_summary->numbering->num_id.has_value());
    REQUIRE(style_summary->numbering->level.has_value());
    CHECK_EQ(*style_summary->numbering->level, 1U);
    REQUIRE(style_summary->numbering->definition_id.has_value());
    CHECK_EQ(*style_summary->numbering->definition_id, *numbering_id);
    REQUIRE(style_summary->numbering->definition_name.has_value());
    CHECK_EQ(*style_summary->numbering->definition_name, "LegalHeadingOutline");
    REQUIRE(style_summary->numbering->instance.has_value());
    CHECK_EQ(style_summary->numbering->instance->instance_id,
             *style_summary->numbering->num_id);
    CHECK(style_summary->numbering->instance->level_overrides.empty());

    const auto styles = doc.list_styles();
    const auto *listed_style = find_style_summary(styles, "LegalHeading");
    REQUIRE(listed_style != nullptr);
    REQUIRE(listed_style->numbering.has_value());
    REQUIRE(listed_style->numbering->instance.has_value());
    CHECK_EQ(listed_style->numbering->instance->instance_id,
             *style_summary->numbering->num_id);

    auto paragraph = doc.paragraphs();
    REQUIRE(paragraph.has_next());
    CHECK(doc.set_paragraph_style(paragraph, "LegalHeading"));
    CHECK(paragraph.add_run("styled numbered heading").has_next());

    CHECK_FALSE(doc.save());

    const auto saved_styles_xml = read_test_docx_entry(target, "word/styles.xml");
    pugi::xml_document styles_document;
    REQUIRE(styles_document.load_string(saved_styles_xml.c_str()));
    const auto styles_root = styles_document.child("w:styles");
    REQUIRE(styles_root != pugi::xml_node{});

    const auto style = find_style_xml_node(styles_root, "LegalHeading");
    REQUIRE(style != pugi::xml_node{});
    const auto style_num_pr = style.child("w:pPr").child("w:numPr");
    REQUIRE(style_num_pr != pugi::xml_node{});
    CHECK_EQ(std::string_view{style_num_pr.child("w:ilvl").attribute("w:val").value()}, "1");
    CHECK_NE(std::string_view{style_num_pr.child("w:numId").attribute("w:val").value()}, "");
    CHECK(style.child("w:pPr").child("w:bidi") != pugi::xml_node{});

    const auto saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document document_xml;
    REQUIRE(document_xml.load_string(saved_document_xml.c_str()));
    const auto body = document_xml.child("w:document").child("w:body");
    const auto first_paragraph = body.child("w:p");
    REQUIRE(first_paragraph != pugi::xml_node{});
    CHECK(first_paragraph.child("w:pPr").child("w:pStyle") != pugi::xml_node{});
    CHECK(first_paragraph.child("w:pPr").child("w:numPr") == pugi::xml_node{});

    const auto numbering_xml = read_test_docx_entry(target, "word/numbering.xml");
    pugi::xml_document numbering_document;
    REQUIRE(numbering_document.load_string(numbering_xml.c_str()));
    const auto numbering_root = numbering_document.child("w:numbering");
    REQUIRE(numbering_root != pugi::xml_node{});
    CHECK_EQ(count_substring_occurrences(numbering_xml, "<w:num "), 1U);
    CHECK(find_numbering_abstract_xml_node(numbering_root, "LegalHeadingOutline") !=
          pugi::xml_node{});

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    const auto reopened_style = reopened.find_style("LegalHeading");
    REQUIRE(reopened_style.has_value());
    REQUIRE(reopened_style->numbering.has_value());
    REQUIRE(reopened_style->numbering->num_id.has_value());
    CHECK_EQ(*reopened_style->numbering->num_id, *style_summary->numbering->num_id);
    REQUIRE(reopened_style->numbering->level.has_value());
    CHECK_EQ(*reopened_style->numbering->level, 1U);
    REQUIRE(reopened_style->numbering->definition_id.has_value());
    CHECK_EQ(*reopened_style->numbering->definition_id, *numbering_id);
    REQUIRE(reopened_style->numbering->definition_name.has_value());
    CHECK_EQ(*reopened_style->numbering->definition_name, "LegalHeadingOutline");
    REQUIRE(reopened_style->numbering->instance.has_value());
    CHECK_EQ(reopened_style->numbering->instance->instance_id,
             *reopened_style->numbering->num_id);
    CHECK(reopened_style->numbering->instance->level_overrides.empty());
    auto inserted = reopened.paragraphs().insert_paragraph_after("");
    REQUIRE(inserted.has_next());
    CHECK(reopened.set_paragraph_style(inserted, "LegalHeading"));
    CHECK(inserted.add_run("reopened styled heading").has_next());
    CHECK_FALSE(reopened.save());

    fs::remove(target);
}

TEST_CASE(
    "paragraph style numbering APIs validate inputs and clear numbering without removing unrelated markup") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "style_numbering_validation.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto paragraph_style = featherdoc::paragraph_style_definition{};
    paragraph_style.name = "Body Numbered";
    paragraph_style.paragraph_bidi = true;
    CHECK(doc.ensure_paragraph_style("BodyNumbered", paragraph_style));

    auto numbering_definition = featherdoc::numbering_definition{};
    numbering_definition.name = "BodyOutline";
    numbering_definition.levels = {
        featherdoc::numbering_level_definition{
            featherdoc::list_kind::decimal, 1U, 0U, "%1."},
    };

    const auto numbering_id = doc.ensure_numbering_definition(numbering_definition);
    REQUIRE(numbering_id.has_value());

    CHECK_FALSE(doc.set_paragraph_style_numbering("", *numbering_id));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));
    CHECK_FALSE(doc.set_paragraph_style_numbering("MissingStyle", *numbering_id));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));
    CHECK_FALSE(doc.set_paragraph_style_numbering("Emphasis", *numbering_id));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));
    CHECK_FALSE(doc.set_paragraph_style_numbering("BodyNumbered", 9999U));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));
    CHECK_FALSE(doc.set_paragraph_style_numbering("BodyNumbered", *numbering_id, 1U));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));

    CHECK(doc.set_paragraph_style_numbering("BodyNumbered", *numbering_id));
    CHECK(doc.clear_paragraph_style_numbering("BodyNumbered"));
    CHECK_FALSE(doc.clear_paragraph_style_numbering(""));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));
    CHECK_FALSE(doc.clear_paragraph_style_numbering("MissingStyle"));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));
    CHECK_FALSE(doc.clear_paragraph_style_numbering("Emphasis"));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));

    CHECK_FALSE(doc.save());

    const auto saved_styles_xml = read_test_docx_entry(target, "word/styles.xml");
    pugi::xml_document styles_document;
    REQUIRE(styles_document.load_string(saved_styles_xml.c_str()));
    const auto styles_root = styles_document.child("w:styles");
    REQUIRE(styles_root != pugi::xml_node{});

    const auto style = find_style_xml_node(styles_root, "BodyNumbered");
    REQUIRE(style != pugi::xml_node{});
    CHECK(style.child("w:pPr").child("w:numPr") == pugi::xml_node{});
    CHECK(style.child("w:pPr").child("w:bidi") != pugi::xml_node{});

    fs::remove(target);
}
