#include "cli_test_support.hpp"
#include "cli_style_test_support.hpp"

#include <algorithm>

TEST_CASE("cli repair-style-numbering imports catalog repairs before style fixes") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_repair_style_numbering_catalog_source.docx";
    const fs::path catalog =
        working_directory / "cli_repair_style_numbering_catalog.json";
    const fs::path repaired =
        working_directory / "cli_repair_style_numbering_catalog_repaired.docx";
    const fs::path plan_output =
        working_directory / "cli_repair_style_numbering_catalog_plan.json";
    const fs::path apply_output =
        working_directory / "cli_repair_style_numbering_catalog_apply.json";
    const fs::path audit_output =
        working_directory / "cli_repair_style_numbering_catalog_audit.json";

    remove_if_exists(source);
    remove_if_exists(catalog);
    remove_if_exists(repaired);
    remove_if_exists(plan_output);
    remove_if_exists(apply_output);
    remove_if_exists(audit_output);

    create_cli_style_numbering_catalog_repair_fixture(source);
    write_binary_file(
        catalog,
        "{\"definition_count\":1,\"instance_count\":0,\"definitions\":["
        "{\"name\":\"CatalogOutline\",\"levels\":["
        "{\"level\":0,\"kind\":\"decimal\",\"start\":1,\"text_pattern\":\"%1.\"},"
        "{\"level\":1,\"kind\":\"decimal\",\"start\":1,\"text_pattern\":\"%1.%2.\"}"
        "],\"instances\":[]}]}");

    CHECK_EQ(run_cli({"repair-style-numbering", source.string(), "--json"},
                     plan_output),
             0);
    const auto plain_plan_json = read_text_file(plan_output);
    CHECK_NE(plain_plan_json.find("\"action\":\"add_numbering_level\""),
             std::string::npos);
    CHECK_NE(plain_plan_json.find(
                 "patch-numbering-catalog numbering-catalog.json --patch-file "
                 "<patch-with-upsert_levels.json>"),
             std::string::npos);

    CHECK_EQ(run_cli({"repair-style-numbering",
                      source.string(),
                      "--catalog-file",
                      catalog.string(),
                      "--json"},
                     plan_output),
             2);
    CHECK_NE(read_text_file(plan_output).find("--catalog-file requires --apply"),
             std::string::npos);

    CHECK_EQ(run_cli({"repair-style-numbering",
                      source.string(),
                      "--catalog-file",
                      catalog.string(),
                      "--apply",
                      "--output",
                      repaired.string(),
                      "--json"},
                     apply_output),
             0);
    const auto apply_json = read_text_file(apply_output);
    CHECK_NE(apply_json.find("\"before_issue_count\":1"),
             std::string::npos);
    CHECK_NE(apply_json.find("\"catalog_file\":" + json_quote(catalog.string())),
             std::string::npos);
    CHECK_NE(apply_json.find("\"catalog_import\":{"), std::string::npos);
    CHECK_NE(apply_json.find("\"input_definition_count\":1"),
             std::string::npos);
    CHECK_NE(apply_json.find("\"imported_definition_count\":1"),
             std::string::npos);
    CHECK_NE(apply_json.find("\"imported_instance_count\":0"),
             std::string::npos);
    CHECK_NE(apply_json.find("\"applyable_count\":0"), std::string::npos);
    CHECK_NE(apply_json.find("\"applied_count\":0"), std::string::npos);
    CHECK_NE(apply_json.find("\"after_clean\":true"), std::string::npos);
    CHECK_NE(apply_json.find("\"after_issue_count\":0"), std::string::npos);

    CHECK_EQ(run_cli({"audit-style-numbering",
                      repaired.string(),
                      "--json",
                      "--fail-on-issue"},
                     audit_output),
             0);
    const auto audit_json = read_text_file(audit_output);
    CHECK_NE(audit_json.find("\"clean\":true"), std::string::npos);
    CHECK_NE(audit_json.find("\"issue_count\":0"), std::string::npos);

    remove_if_exists(source);
    remove_if_exists(catalog);
    remove_if_exists(repaired);
    remove_if_exists(plan_output);
    remove_if_exists(apply_output);
    remove_if_exists(audit_output);
}


TEST_CASE("cli ensure-style-linked-numbering links multiple paragraph styles to one shared instance") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_style_linked_numbering_source.docx";
    const fs::path updated =
        working_directory / "cli_style_linked_numbering_updated.docx";
    const fs::path output =
        working_directory / "cli_style_linked_numbering_output.json";

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(output);

    {
        featherdoc::Document document(source);
        REQUIRE_FALSE(document.create_empty());

        auto heading_style = featherdoc::paragraph_style_definition{};
        heading_style.name = "Legal Heading";
        heading_style.based_on = std::string{"Heading1"};
        heading_style.paragraph_bidi = false;
        REQUIRE(document.ensure_paragraph_style("LegalHeading", heading_style));

        auto subheading_style = featherdoc::paragraph_style_definition{};
        subheading_style.name = "Legal Subheading";
        subheading_style.based_on = std::string{"Heading2"};
        subheading_style.paragraph_bidi = false;
        REQUIRE(document.ensure_paragraph_style("LegalSubheading", subheading_style));

        REQUIRE_FALSE(document.save());
    }

    CHECK_EQ(run_cli({"ensure-style-linked-numbering",
                      source.string(),
                      "--definition-name",
                      "LegalOutlineShared",
                      "--numbering-level",
                      "0:decimal:7:(%1)",
                      "--numbering-level",
                      "1:decimal:1:(%1.%2)",
                      "--style-link",
                      "LegalHeading:0",
                      "--style-link",
                      "LegalSubheading:1",
                      "--output",
                      updated.string(),
                      "--json"},
                     output),
             0);

    featherdoc::Document updated_document(updated);
    REQUIRE_FALSE(updated_document.open());
    const auto definitions = updated_document.list_numbering_definitions();
    REQUIRE_FALSE(updated_document.last_error());
    const auto definition = std::find_if(definitions.begin(), definitions.end(),
                                         [](const auto &summary) {
                                             return summary.name == "LegalOutlineShared";
                                         });
    REQUIRE(definition != definitions.end());
    REQUIRE_EQ(definition->instance_ids.size(), 1U);
    const auto definition_id = std::to_string(definition->definition_id);

    CHECK_EQ(
        read_text_file(output),
        std::string{
            "{\"command\":\"ensure-style-linked-numbering\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"definition_id\":"} +
            definition_id +
            ",\"definition_name\":\"LegalOutlineShared\",\"definition_levels\":["
            "{\"level\":0,\"kind\":\"decimal\",\"start\":7,\"text_pattern\":\"(%1)\"},"
            "{\"level\":1,\"kind\":\"decimal\",\"start\":1,\"text_pattern\":\"(%1.%2)\"}],"
            "\"style_links\":[{\"style_id\":\"LegalHeading\",\"level\":0},"
            "{\"style_id\":\"LegalSubheading\",\"level\":1}]}\n");

    const auto heading_style = updated_document.find_style("LegalHeading");
    const auto subheading_style = updated_document.find_style("LegalSubheading");
    REQUIRE(heading_style.has_value());
    REQUIRE(subheading_style.has_value());
    REQUIRE(heading_style->numbering.has_value());
    REQUIRE(subheading_style->numbering.has_value());
    REQUIRE(heading_style->numbering->num_id.has_value());
    REQUIRE(subheading_style->numbering->num_id.has_value());
    CHECK_EQ(*heading_style->numbering->num_id, *subheading_style->numbering->num_id);
    REQUIRE(heading_style->numbering->level.has_value());
    REQUIRE(subheading_style->numbering->level.has_value());
    CHECK_EQ(*heading_style->numbering->level, 0U);
    CHECK_EQ(*subheading_style->numbering->level, 1U);

    const auto styles_xml = read_docx_entry(updated, "word/styles.xml");
    pugi::xml_document styles_document;
    REQUIRE(styles_document.load_string(styles_xml.c_str()));
    const auto styles_root = styles_document.child("w:styles");
    REQUIRE(styles_root != pugi::xml_node{});
    const auto heading_style_xml = find_style_xml_node(styles_root, "LegalHeading");
    const auto subheading_style_xml = find_style_xml_node(styles_root, "LegalSubheading");
    REQUIRE(heading_style_xml != pugi::xml_node{});
    REQUIRE(subheading_style_xml != pugi::xml_node{});
    CHECK_EQ(std::string_view{
                 heading_style_xml.child("w:pPr").child("w:numPr").child("w:ilvl").attribute("w:val").value()},
             "0");
    CHECK_EQ(std::string_view{
                 subheading_style_xml.child("w:pPr").child("w:numPr").child("w:ilvl").attribute("w:val").value()},
             "1");

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(output);
}
