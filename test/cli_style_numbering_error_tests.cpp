#include "cli_test_support.hpp"
#include "cli_style_test_support.hpp"

TEST_CASE("cli ensure-style-linked-numbering reports parse and mutate errors") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_style_linked_numbering_error_source.docx";
    const fs::path parse_output =
        working_directory / "cli_style_linked_numbering_parse.json";
    const fs::path mutate_output =
        working_directory / "cli_style_linked_numbering_mutate.json";

    remove_if_exists(source);
    remove_if_exists(parse_output);
    remove_if_exists(mutate_output);

    create_cli_paragraph_style_fixture(source, "LegalHeading", "Legal Heading",
                                       "Heading1", false);

    CHECK_EQ(run_cli({"ensure-style-linked-numbering",
                      source.string(),
                      "--definition-name",
                      "BrokenOutline",
                      "--numbering-level",
                      "0:decimal:1:%1.",
                      "--json"},
                     parse_output),
             2);
    CHECK_EQ(
        read_text_file(parse_output),
        std::string{
            "{\"command\":\"ensure-style-linked-numbering\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"expected at least one --style-link <style-id>:<level>\"}\n"});

    CHECK_EQ(run_cli({"ensure-style-linked-numbering",
                      source.string(),
                      "--definition-name",
                      "BrokenOutline",
                      "--numbering-level",
                      "0:decimal:1:%1.",
                      "--style-link",
                      "MissingStyle:0",
                      "--json"},
                     mutate_output),
             1);
    const auto mutate_json = read_text_file(mutate_output);
    CHECK_NE(mutate_json.find("\"command\":\"ensure-style-linked-numbering\""),
             std::string::npos);
    CHECK_NE(mutate_json.find("\"stage\":\"mutate\""), std::string::npos);
    CHECK_NE(mutate_json.find(
                 "\"detail\":\"style id 'MissingStyle' was not found in "
                 "word/styles.xml\""),
             std::string::npos);
    CHECK_NE(mutate_json.find("\"entry\":\"word/styles.xml\""),
             std::string::npos);

    remove_if_exists(source);
    remove_if_exists(parse_output);
    remove_if_exists(mutate_output);
}

TEST_CASE("cli clear-paragraph-style-numbering removes style numbering and keeps bidi") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_clear_style_numbering_source.docx";
    const fs::path numbered =
        working_directory / "cli_clear_style_numbering_numbered.docx";
    const fs::path cleared =
        working_directory / "cli_clear_style_numbering_cleared.docx";
    const fs::path output =
        working_directory / "cli_clear_style_numbering_output.json";

    remove_if_exists(source);
    remove_if_exists(numbered);
    remove_if_exists(cleared);
    remove_if_exists(output);

    create_cli_paragraph_style_fixture(source, "BodyNumbered", "Body Numbered",
                                       "Normal", true);

    CHECK_EQ(run_cli({"set-paragraph-style-numbering",
                      source.string(),
                      "BodyNumbered",
                      "--definition-name",
                      "BodyOutline",
                      "--numbering-level",
                      "0:decimal:1:%1.",
                      "--output",
                      numbered.string()},
                     std::nullopt),
             0);

    const auto numbered_styles_xml = read_docx_entry(numbered, "word/styles.xml");
    pugi::xml_document numbered_styles_document;
    REQUIRE(numbered_styles_document.load_string(numbered_styles_xml.c_str()));
    const auto numbered_style = find_style_xml_node(
        numbered_styles_document.child("w:styles"), "BodyNumbered");
    REQUIRE(numbered_style != pugi::xml_node{});
    REQUIRE(numbered_style.child("w:pPr").child("w:numPr") != pugi::xml_node{});

    CHECK_EQ(run_cli({"clear-paragraph-style-numbering",
                      numbered.string(),
                      "BodyNumbered",
                      "--output",
                      cleared.string(),
                      "--json"},
                     output),
             0);
    CHECK_EQ(
        read_text_file(output),
        std::string{
            "{\"command\":\"clear-paragraph-style-numbering\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"style_id\":\"BodyNumbered\"}\n"});

    const auto cleared_styles_xml = read_docx_entry(cleared, "word/styles.xml");
    pugi::xml_document cleared_styles_document;
    REQUIRE(cleared_styles_document.load_string(cleared_styles_xml.c_str()));
    const auto cleared_style =
        find_style_xml_node(cleared_styles_document.child("w:styles"), "BodyNumbered");
    REQUIRE(cleared_style != pugi::xml_node{});
    CHECK(cleared_style.child("w:pPr").child("w:numPr") == pugi::xml_node{});
    CHECK(cleared_style.child("w:pPr").child("w:bidi") != pugi::xml_node{});

    remove_if_exists(source);
    remove_if_exists(numbered);
    remove_if_exists(cleared);
    remove_if_exists(output);
}

TEST_CASE("cli set-paragraph-style-numbering reports json parse errors") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_style_numbering_parse_source.docx";
    const fs::path output =
        working_directory / "cli_style_numbering_parse_output.json";

    remove_if_exists(source);
    remove_if_exists(output);

    create_cli_paragraph_style_fixture(source, "LegalHeading", "Legal Heading",
                                       "Heading1", false);

    CHECK_EQ(run_cli({"set-paragraph-style-numbering",
                      source.string(),
                      "LegalHeading",
                      "--definition-name",
                      "LegalHeadingOutline",
                      "--json"},
                     output),
             2);
    CHECK_EQ(
        read_text_file(output),
        std::string{
            "{\"command\":\"set-paragraph-style-numbering\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"expected at least one "
            "--numbering-level <level>:<kind>:<start>:<text-pattern>\"}\n"});

    remove_if_exists(source);
    remove_if_exists(output);
}
