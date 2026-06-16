#include "cli_numbering_catalog_test_support.hpp"

#include <algorithm>

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
