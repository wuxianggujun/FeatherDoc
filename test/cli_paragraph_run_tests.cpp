#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "cli_test_support.hpp"
#include "cli_style_test_support.hpp"

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

void create_cli_inspect_paragraphs_fixture(const fs::path &path) {
    create_cli_fixture(path);

    featherdoc::Document document(path);
    REQUIRE_FALSE(document.open());

    featherdoc::paragraph_style_definition style;
    style.name = "Custom Body";
    style.based_on = "Normal";
    style.is_quick_format = true;
    REQUIRE(document.ensure_paragraph_style("CustomBody", style));

    const auto styled_index =
        find_body_paragraph_index_by_text(document, "section 0 body");
    auto styled_paragraph = document.paragraphs();
    for (std::size_t index = 0U;
         index < styled_index && styled_paragraph.has_next(); ++index) {
        styled_paragraph.next();
    }
    REQUIRE(styled_paragraph.has_next());
    REQUIRE(document.set_paragraph_style(styled_paragraph, "CustomBody"));

    const auto numbered_index =
        find_body_paragraph_index_by_text(document, "section 1 body");
    auto numbered_paragraph = document.paragraphs();
    for (std::size_t index = 0U;
         index < numbered_index && numbered_paragraph.has_next(); ++index) {
        numbered_paragraph.next();
    }
    REQUIRE(numbered_paragraph.has_next());
    REQUIRE(document.set_paragraph_list(numbered_paragraph,
                                        featherdoc::list_kind::decimal, 0U));

    REQUIRE_FALSE(document.save());
}

} // namespace

TEST_CASE("cli paragraph list commands manage body numbering") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_paragraph_list_source.docx";
    const fs::path listed =
        working_directory / "cli_paragraph_list_listed.docx";
    const fs::path restarted =
        working_directory / "cli_paragraph_list_restarted.docx";
    const fs::path cleared =
        working_directory / "cli_paragraph_list_cleared.docx";
    const fs::path set_output =
        working_directory / "cli_paragraph_list_set_output.json";
    const fs::path restart_output =
        working_directory / "cli_paragraph_list_restart_output.json";
    const fs::path clear_output =
        working_directory / "cli_paragraph_list_clear_output.json";

    remove_if_exists(source);
    remove_if_exists(listed);
    remove_if_exists(restarted);
    remove_if_exists(cleared);
    remove_if_exists(set_output);
    remove_if_exists(restart_output);
    remove_if_exists(clear_output);

    create_cli_paragraph_list_fixture(source);

    CHECK_EQ(run_cli({"set-paragraph-list",
                      source.string(),
                      "0",
                      "--kind",
                      "decimal",
                      "--output",
                      listed.string(),
                      "--json"},
                     set_output),
             0);
    CHECK_EQ(
        read_text_file(set_output),
        std::string{
            "{\"command\":\"set-paragraph-list\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"paragraph_index\":0,\"kind\":\"decimal\",\"level\":0}\n"});

    featherdoc::Document listed_document(listed);
    REQUIRE_FALSE(listed_document.open());
    const auto listed_definitions = listed_document.list_numbering_definitions();
    REQUIRE_FALSE(listed_document.last_error());
    const auto listed_definition =
        std::find_if(listed_definitions.begin(), listed_definitions.end(),
                     [](const auto &summary) {
                         return summary.name == "FeatherDocDecimalList";
                     });
    REQUIRE(listed_definition != listed_definitions.end());
    REQUIRE_EQ(listed_definition->instance_ids.size(), 1U);
    const auto first_num_id = std::to_string(listed_definition->instance_ids.front());

    const auto listed_document_xml = read_docx_entry(listed, "word/document.xml");
    pugi::xml_document listed_xml_document;
    REQUIRE(listed_xml_document.load_string(listed_document_xml.c_str()));
    const auto listed_paragraph =
        find_body_paragraph_xml_node(listed_xml_document.child("w:document"), 0U);
    REQUIRE(listed_paragraph != pugi::xml_node{});
    const auto listed_num_pr = listed_paragraph.child("w:pPr").child("w:numPr");
    REQUIRE(listed_num_pr != pugi::xml_node{});
    CHECK_EQ(std::string_view{listed_num_pr.child("w:ilvl").attribute("w:val").value()},
             "0");
    CHECK_EQ(std::string{listed_num_pr.child("w:numId").attribute("w:val").value()},
             first_num_id);

    CHECK_EQ(run_cli({"restart-paragraph-list",
                      listed.string(),
                      "1",
                      "--kind",
                      "decimal",
                      "--level",
                      "1",
                      "--output",
                      restarted.string(),
                      "--json"},
                     restart_output),
             0);
    CHECK_EQ(
        read_text_file(restart_output),
        std::string{
            "{\"command\":\"restart-paragraph-list\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"paragraph_index\":1,\"kind\":\"decimal\",\"level\":1}\n"});

    featherdoc::Document restarted_document(restarted);
    REQUIRE_FALSE(restarted_document.open());
    const auto restarted_definitions = restarted_document.list_numbering_definitions();
    REQUIRE_FALSE(restarted_document.last_error());
    const auto restarted_definition =
        std::find_if(restarted_definitions.begin(), restarted_definitions.end(),
                     [](const auto &summary) {
                         return summary.name == "FeatherDocDecimalList";
                     });
    REQUIRE(restarted_definition != restarted_definitions.end());
    REQUIRE_EQ(restarted_definition->instance_ids.size(), 2U);
    CHECK_EQ(std::to_string(restarted_definition->instance_ids[0]), first_num_id);
    const auto restarted_num_id =
        std::to_string(restarted_definition->instance_ids[1]);

    const auto restarted_document_xml =
        read_docx_entry(restarted, "word/document.xml");
    pugi::xml_document restarted_xml_document;
    REQUIRE(restarted_xml_document.load_string(restarted_document_xml.c_str()));
    const auto restarted_first_paragraph =
        find_body_paragraph_xml_node(restarted_xml_document.child("w:document"), 0U);
    REQUIRE(restarted_first_paragraph != pugi::xml_node{});
    const auto restarted_first_num_pr =
        restarted_first_paragraph.child("w:pPr").child("w:numPr");
    REQUIRE(restarted_first_num_pr != pugi::xml_node{});
    CHECK_EQ(
        std::string{restarted_first_num_pr.child("w:numId").attribute("w:val").value()},
        first_num_id);

    const auto restarted_second_paragraph =
        find_body_paragraph_xml_node(restarted_xml_document.child("w:document"), 1U);
    REQUIRE(restarted_second_paragraph != pugi::xml_node{});
    const auto restarted_second_num_pr =
        restarted_second_paragraph.child("w:pPr").child("w:numPr");
    REQUIRE(restarted_second_num_pr != pugi::xml_node{});
    CHECK_EQ(
        std::string_view{restarted_second_num_pr.child("w:ilvl").attribute("w:val").value()},
        "1");
    CHECK_EQ(
        std::string{restarted_second_num_pr.child("w:numId").attribute("w:val").value()},
        restarted_num_id);

    CHECK_EQ(run_cli({"clear-paragraph-list",
                      restarted.string(),
                      "0",
                      "--output",
                      cleared.string(),
                      "--json"},
                     clear_output),
             0);
    CHECK_EQ(
        read_text_file(clear_output),
        std::string{
            "{\"command\":\"clear-paragraph-list\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"paragraph_index\":0}\n"});

    const auto cleared_document_xml = read_docx_entry(cleared, "word/document.xml");
    pugi::xml_document cleared_xml_document;
    REQUIRE(cleared_xml_document.load_string(cleared_document_xml.c_str()));
    const auto cleared_first_paragraph =
        find_body_paragraph_xml_node(cleared_xml_document.child("w:document"), 0U);
    REQUIRE(cleared_first_paragraph != pugi::xml_node{});
    CHECK(cleared_first_paragraph.child("w:pPr").child("w:numPr") == pugi::xml_node{});

    const auto cleared_second_paragraph =
        find_body_paragraph_xml_node(cleared_xml_document.child("w:document"), 1U);
    REQUIRE(cleared_second_paragraph != pugi::xml_node{});
    const auto cleared_second_num_pr =
        cleared_second_paragraph.child("w:pPr").child("w:numPr");
    REQUIRE(cleared_second_num_pr != pugi::xml_node{});
    CHECK_EQ(
        std::string{cleared_second_num_pr.child("w:numId").attribute("w:val").value()},
        restarted_num_id);

    remove_if_exists(source);
    remove_if_exists(listed);
    remove_if_exists(restarted);
    remove_if_exists(cleared);
    remove_if_exists(set_output);
    remove_if_exists(restart_output);
    remove_if_exists(clear_output);
}

TEST_CASE("cli set-paragraph-list reports json parse errors") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_paragraph_list_parse_source.docx";
    const fs::path output =
        working_directory / "cli_paragraph_list_parse_output.json";

    remove_if_exists(source);
    remove_if_exists(output);

    create_cli_paragraph_list_fixture(source);

    CHECK_EQ(run_cli({"set-paragraph-list", source.string(), "0", "--json"},
                     output),
             2);
    CHECK_EQ(
        read_text_file(output),
        std::string{
            "{\"command\":\"set-paragraph-list\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"missing --kind "
            "<bullet|decimal>\"}\n"});

    remove_if_exists(source);
    remove_if_exists(output);
}

TEST_CASE("cli inspect-paragraphs lists body paragraphs with section/style metadata") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_inspect_paragraphs_source.docx";
    const fs::path output =
        working_directory / "cli_inspect_paragraphs_output.txt";

    remove_if_exists(source);
    remove_if_exists(output);

    create_cli_inspect_paragraphs_fixture(source);

    CHECK_EQ(run_cli({"inspect-paragraphs", source.string()}, output), 0);
    const auto inspect_text = read_text_file(output);
    CHECK_NE(inspect_text.find("paragraphs: 5"), std::string::npos);
    CHECK_NE(
        inspect_text.find(
            "paragraph[0]: section_index=0 style_id=CustomBody numbering=none "
            "section_break=no text=section 0 body"),
        std::string::npos);
    CHECK_NE(
        inspect_text.find(
            "paragraph[1]: section_index=0 style_id=none numbering=none "
            "section_break=yes text=<empty>"),
        std::string::npos);
    CHECK_NE(
        inspect_text.find(
            "paragraph[2]: section_index=1 style_id=none numbering=num_id="),
        std::string::npos);
    CHECK_NE(inspect_text.find("level=0"), std::string::npos);
    CHECK_NE(inspect_text.find("text=section 1 body"), std::string::npos);
    CHECK_NE(
        inspect_text.find(
            "paragraph[3]: section_index=1 style_id=none numbering=none "
            "section_break=yes text=<empty>"),
        std::string::npos);
    CHECK_NE(
        inspect_text.find(
            "paragraph[4]: section_index=2 style_id=none numbering=none "
            "section_break=no text=section 2 body"),
        std::string::npos);

    remove_if_exists(source);
    remove_if_exists(output);
}

TEST_CASE("cli inspect-paragraphs supports single-paragraph json output and errors") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_inspect_paragraphs_single_source.docx";
    const fs::path json_output =
        working_directory / "cli_inspect_paragraphs_single_output.json";
    const fs::path missing_output =
        working_directory / "cli_inspect_paragraphs_missing_output.json";
    const fs::path parse_output =
        working_directory / "cli_inspect_paragraphs_parse_output.json";

    remove_if_exists(source);
    remove_if_exists(json_output);
    remove_if_exists(missing_output);
    remove_if_exists(parse_output);

    create_cli_inspect_paragraphs_fixture(source);

    featherdoc::Document document(source);
    REQUIRE_FALSE(document.open());
    const auto paragraph_index =
        find_body_paragraph_index_by_text(document, "section 1 body");

    const auto document_xml = read_docx_entry(source, "word/document.xml");
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(document_xml.c_str()));
    const auto paragraph =
        find_body_paragraph_xml_node(xml_document.child("w:document"), paragraph_index);
    REQUIRE(paragraph != pugi::xml_node{});
    const auto numbering = paragraph.child("w:pPr").child("w:numPr");
    REQUIRE(numbering != pugi::xml_node{});
    const auto numbering_id =
        std::string{numbering.child("w:numId").attribute("w:val").value()};
    REQUIRE_FALSE(numbering_id.empty());
    const auto has_section_break =
        paragraph.child("w:pPr").child("w:sectPr") != pugi::xml_node{};

    CHECK_EQ(run_cli({"inspect-paragraphs",
                      source.string(),
                      "--paragraph",
                      std::to_string(paragraph_index),
                      "--json"},
                     json_output),
             0);
    CHECK_EQ(
        read_text_file(json_output),
        std::string{"{\"paragraph\":{\"index\":"} +
            std::to_string(paragraph_index) +
            ",\"section_index\":1,\"style_id\":null,\"numbering\":{\"num_id\":" +
            numbering_id +
            ",\"level\":0},\"section_break\":" +
            std::string{has_section_break ? "true" : "false"} +
            ",\"text\":\"section 1 body\"}}\n");

    CHECK_EQ(run_cli({"inspect-paragraphs",
                      source.string(),
                      "--paragraph",
                      "999",
                      "--json"},
                     missing_output),
             1);
    const auto missing_json = read_text_file(missing_output);
    CHECK_NE(missing_json.find("\"command\":\"inspect-paragraphs\""),
             std::string::npos);
    CHECK_NE(missing_json.find("\"stage\":\"inspect\""), std::string::npos);
    CHECK_NE(missing_json.find("\"detail\":\"paragraph index '999' is out of range\""),
             std::string::npos);
    CHECK_NE(missing_json.find("\"entry\":\"word/document.xml\""),
             std::string::npos);

    CHECK_EQ(run_cli({"inspect-paragraphs", source.string(), "--json", "--paragraph"},
                     parse_output),
             2);
    CHECK_EQ(
        read_text_file(parse_output),
        std::string{
            "{\"command\":\"inspect-paragraphs\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"missing value after "
            "--paragraph\"}\n"});

    remove_if_exists(source);
    remove_if_exists(json_output);
    remove_if_exists(missing_output);
    remove_if_exists(parse_output);
}

TEST_CASE("cli inspect-runs lists paragraph runs with style metadata") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_inspect_runs_source.docx";
    const fs::path output =
        working_directory / "cli_inspect_runs_output.txt";

    remove_if_exists(source);
    remove_if_exists(output);

    create_cli_style_usage_fixture(source);

    CHECK_EQ(run_cli({"inspect-runs", source.string(), "1"}, output), 0);
    const auto inspect_text = read_text_file(output);
    CHECK_NE(inspect_text.find("paragraph_index: 1"), std::string::npos);
    CHECK_NE(inspect_text.find("runs: 2"), std::string::npos);
    CHECK_NE(inspect_text.find("run[0]: style_id=none font_family=none "
                               "east_asia_font_family=none text_color=none "
                               "bold=none italic=none underline=none "
                               "font_size_points=none language=none text=beta"),
             std::string::npos);
    CHECK_NE(inspect_text.find("run[1]: style_id=Strong font_family=none "
                               "east_asia_font_family=none text_color=none "
                               "bold=none italic=none underline=none "
                               "font_size_points=none language=none text=gamma"),
             std::string::npos);

    remove_if_exists(source);
    remove_if_exists(output);
}

TEST_CASE("cli inspect-runs supports single-run json output and errors") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_inspect_runs_single_source.docx";
    const fs::path json_output =
        working_directory / "cli_inspect_runs_single_output.json";
    const fs::path missing_paragraph_output =
        working_directory / "cli_inspect_runs_missing_paragraph_output.json";
    const fs::path missing_run_output =
        working_directory / "cli_inspect_runs_missing_run_output.json";
    const fs::path parse_output =
        working_directory / "cli_inspect_runs_parse_output.json";

    remove_if_exists(source);
    remove_if_exists(json_output);
    remove_if_exists(missing_paragraph_output);
    remove_if_exists(missing_run_output);
    remove_if_exists(parse_output);

    create_cli_style_usage_fixture(source);

    CHECK_EQ(run_cli({"inspect-runs",
                      source.string(),
                      "1",
                      "--run",
                      "1",
                      "--json"},
                     json_output),
             0);
    CHECK_EQ(
        read_text_file(json_output),
        std::string{
            "{\"paragraph_index\":1,\"run\":{\"index\":1,\"style_id\":\"Strong\","
            "\"font_family\":null,\"east_asia_font_family\":null,"
            "\"text_color\":null,\"bold\":null,\"italic\":null,"
            "\"underline\":null,\"font_size_points\":null,"
            "\"language\":null,\"text\":\"gamma\"}}\n"});

    CHECK_EQ(run_cli({"inspect-runs", source.string(), "999", "--json"},
                     missing_paragraph_output),
             1);
    const auto missing_paragraph_json = read_text_file(missing_paragraph_output);
    CHECK_NE(missing_paragraph_json.find("\"command\":\"inspect-runs\""),
             std::string::npos);
    CHECK_NE(missing_paragraph_json.find("\"stage\":\"inspect\""),
             std::string::npos);
    CHECK_NE(
        missing_paragraph_json.find(
            "\"detail\":\"paragraph index '999' is out of range\""),
        std::string::npos);
    CHECK_NE(missing_paragraph_json.find("\"entry\":\"word/document.xml\""),
             std::string::npos);

    CHECK_EQ(run_cli({"inspect-runs",
                      source.string(),
                      "1",
                      "--run",
                      "99",
                      "--json"},
                     missing_run_output),
             1);
    const auto missing_run_json = read_text_file(missing_run_output);
    CHECK_NE(missing_run_json.find("\"command\":\"inspect-runs\""),
             std::string::npos);
    CHECK_NE(missing_run_json.find("\"stage\":\"inspect\""),
             std::string::npos);
    CHECK_NE(
        missing_run_json.find(
            "\"detail\":\"run index '99' is out of range for paragraph index '1'\""),
        std::string::npos);
    CHECK_NE(missing_run_json.find("\"entry\":\"word/document.xml\""),
             std::string::npos);

    CHECK_EQ(run_cli({"inspect-runs",
                      source.string(),
                      "1",
                      "--run",
                      "oops",
                      "--json"},
                     parse_output),
             2);
    CHECK_EQ(
        read_text_file(parse_output),
        std::string{
            "{\"command\":\"inspect-runs\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"invalid run index: oops\"}\n"});

    remove_if_exists(source);
    remove_if_exists(json_output);
    remove_if_exists(missing_paragraph_output);
    remove_if_exists(missing_run_output);
    remove_if_exists(parse_output);
}

TEST_CASE("cli paragraph style commands set and clear body paragraph styles") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_paragraph_style_source.docx";
    const fs::path styled =
        working_directory / "cli_paragraph_style_styled.docx";
    const fs::path cleared =
        working_directory / "cli_paragraph_style_cleared.docx";
    const fs::path set_output =
        working_directory / "cli_paragraph_style_set_output.json";
    const fs::path clear_output =
        working_directory / "cli_paragraph_style_clear_output.json";

    remove_if_exists(source);
    remove_if_exists(styled);
    remove_if_exists(cleared);
    remove_if_exists(set_output);
    remove_if_exists(clear_output);

    create_cli_inspect_paragraphs_fixture(source);

    featherdoc::Document source_document(source);
    REQUIRE_FALSE(source_document.open());
    const auto section0_body_index =
        find_body_paragraph_index_by_text(source_document, "section 0 body");
    const auto section1_body_index =
        find_body_paragraph_index_by_text(source_document, "section 1 body");

    CHECK_EQ(run_cli({"set-paragraph-style",
                      source.string(),
                      std::to_string(section1_body_index),
                      "CustomBody",
                      "--output",
                      styled.string(),
                      "--json"},
                     set_output),
             0);
    CHECK_EQ(
        read_text_file(set_output),
        std::string{
            "{\"command\":\"set-paragraph-style\",\"ok\":true,"
            "\"in_place\":false,\"sections\":3,\"headers\":2,\"footers\":1,"
            "\"paragraph_index\":"} +
            std::to_string(section1_body_index) +
            ",\"style_id\":\"CustomBody\"}\n");

    const auto source_document_xml = read_docx_entry(source, "word/document.xml");
    pugi::xml_document source_xml_document;
    REQUIRE(source_xml_document.load_string(source_document_xml.c_str()));
    const auto source_section1_paragraph =
        find_body_paragraph_xml_node(source_xml_document.child("w:document"),
                                     section1_body_index);
    REQUIRE(source_section1_paragraph != pugi::xml_node{});
    CHECK(source_section1_paragraph.child("w:pPr").child("w:pStyle") ==
          pugi::xml_node{});

    const auto styled_document_xml = read_docx_entry(styled, "word/document.xml");
    pugi::xml_document styled_xml_document;
    REQUIRE(styled_xml_document.load_string(styled_document_xml.c_str()));
    const auto styled_section1_paragraph =
        find_body_paragraph_xml_node(styled_xml_document.child("w:document"),
                                     section1_body_index);
    REQUIRE(styled_section1_paragraph != pugi::xml_node{});
    CHECK_EQ(
        std::string_view{styled_section1_paragraph.child("w:pPr")
                             .child("w:pStyle")
                             .attribute("w:val")
                             .value()},
        "CustomBody");

    CHECK_EQ(run_cli({"clear-paragraph-style",
                      styled.string(),
                      std::to_string(section0_body_index),
                      "--output",
                      cleared.string(),
                      "--json"},
                     clear_output),
             0);
    CHECK_EQ(
        read_text_file(clear_output),
        std::string{
            "{\"command\":\"clear-paragraph-style\",\"ok\":true,"
            "\"in_place\":false,\"sections\":3,\"headers\":2,\"footers\":1,"
            "\"paragraph_index\":"} +
            std::to_string(section0_body_index) + "}\n");

    const auto cleared_document_xml = read_docx_entry(cleared, "word/document.xml");
    pugi::xml_document cleared_xml_document;
    REQUIRE(cleared_xml_document.load_string(cleared_document_xml.c_str()));
    const auto cleared_section0_paragraph =
        find_body_paragraph_xml_node(cleared_xml_document.child("w:document"),
                                     section0_body_index);
    REQUIRE(cleared_section0_paragraph != pugi::xml_node{});
    CHECK(cleared_section0_paragraph.child("w:pPr").child("w:pStyle") ==
          pugi::xml_node{});
    const auto cleared_section1_paragraph =
        find_body_paragraph_xml_node(cleared_xml_document.child("w:document"),
                                     section1_body_index);
    REQUIRE(cleared_section1_paragraph != pugi::xml_node{});
    CHECK_EQ(
        std::string_view{cleared_section1_paragraph.child("w:pPr")
                             .child("w:pStyle")
                             .attribute("w:val")
                             .value()},
        "CustomBody");

    remove_if_exists(source);
    remove_if_exists(styled);
    remove_if_exists(cleared);
    remove_if_exists(set_output);
    remove_if_exists(clear_output);
}

TEST_CASE("cli paragraph style commands report parse and mutate errors") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_paragraph_style_error_source.docx";
    const fs::path parse_output =
        working_directory / "cli_paragraph_style_parse_output.json";
    const fs::path mutate_output =
        working_directory / "cli_paragraph_style_mutate_output.json";

    remove_if_exists(source);
    remove_if_exists(parse_output);
    remove_if_exists(mutate_output);

    create_cli_inspect_paragraphs_fixture(source);

    CHECK_EQ(run_cli({"set-paragraph-style",
                      source.string(),
                      "oops",
                      "CustomBody",
                      "--json"},
                     parse_output),
             2);
    CHECK_EQ(
        read_text_file(parse_output),
        std::string{
            "{\"command\":\"set-paragraph-style\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"invalid paragraph index: "
            "oops\"}\n"});

    CHECK_EQ(run_cli({"clear-paragraph-style", source.string(), "999", "--json"},
                     mutate_output),
             1);
    const auto mutate_json = read_text_file(mutate_output);
    CHECK_NE(mutate_json.find("\"command\":\"clear-paragraph-style\""),
             std::string::npos);
    CHECK_NE(mutate_json.find("\"stage\":\"mutate\""), std::string::npos);
    CHECK_NE(mutate_json.find("\"detail\":\"paragraph index '999' is out of range\""),
             std::string::npos);
    CHECK_NE(mutate_json.find("\"entry\":\"word/document.xml\""),
             std::string::npos);

    remove_if_exists(source);
    remove_if_exists(parse_output);
    remove_if_exists(mutate_output);
}

TEST_CASE("cli run style commands set and clear body run styles") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_run_style_source.docx";
    const fs::path styled =
        working_directory / "cli_run_style_styled.docx";
    const fs::path cleared =
        working_directory / "cli_run_style_cleared.docx";
    const fs::path set_output =
        working_directory / "cli_run_style_set_output.json";
    const fs::path clear_output =
        working_directory / "cli_run_style_clear_output.json";

    remove_if_exists(source);
    remove_if_exists(styled);
    remove_if_exists(cleared);
    remove_if_exists(set_output);
    remove_if_exists(clear_output);

    create_cli_style_usage_fixture(source);

    CHECK_EQ(run_cli({"set-run-style",
                      source.string(),
                      "1",
                      "0",
                      "Strong",
                      "--output",
                      styled.string(),
                      "--json"},
                     set_output),
             0);
    CHECK_EQ(
        read_text_file(set_output),
        std::string{
            "{\"command\":\"set-run-style\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":1,\"footers\":1,"
            "\"paragraph_index\":1,\"run_index\":0,\"style_id\":\"Strong\"}\n"});

    const auto source_document_xml = read_docx_entry(source, "word/document.xml");
    pugi::xml_document source_xml_document;
    REQUIRE(source_xml_document.load_string(source_document_xml.c_str()));
    const auto source_paragraph =
        find_body_paragraph_xml_node(source_xml_document.child("w:document"), 1U);
    REQUIRE(source_paragraph != pugi::xml_node{});
    const auto source_first_run = find_run_xml_node(source_paragraph, 0U);
    REQUIRE(source_first_run != pugi::xml_node{});
    CHECK(source_first_run.child("w:rPr").child("w:rStyle") == pugi::xml_node{});

    const auto styled_document_xml = read_docx_entry(styled, "word/document.xml");
    pugi::xml_document styled_xml_document;
    REQUIRE(styled_xml_document.load_string(styled_document_xml.c_str()));
    const auto styled_paragraph =
        find_body_paragraph_xml_node(styled_xml_document.child("w:document"), 1U);
    REQUIRE(styled_paragraph != pugi::xml_node{});
    const auto styled_first_run = find_run_xml_node(styled_paragraph, 0U);
    REQUIRE(styled_first_run != pugi::xml_node{});
    CHECK_EQ(
        std::string_view{styled_first_run.child("w:rPr")
                             .child("w:rStyle")
                             .attribute("w:val")
                             .value()},
        "Strong");

    CHECK_EQ(run_cli({"clear-run-style",
                      styled.string(),
                      "1",
                      "1",
                      "--output",
                      cleared.string(),
                      "--json"},
                     clear_output),
             0);
    CHECK_EQ(
        read_text_file(clear_output),
        std::string{
            "{\"command\":\"clear-run-style\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":1,\"footers\":1,"
            "\"paragraph_index\":1,\"run_index\":1}\n"});

    const auto cleared_document_xml = read_docx_entry(cleared, "word/document.xml");
    pugi::xml_document cleared_xml_document;
    REQUIRE(cleared_xml_document.load_string(cleared_document_xml.c_str()));
    const auto cleared_paragraph =
        find_body_paragraph_xml_node(cleared_xml_document.child("w:document"), 1U);
    REQUIRE(cleared_paragraph != pugi::xml_node{});
    const auto cleared_first_run = find_run_xml_node(cleared_paragraph, 0U);
    REQUIRE(cleared_first_run != pugi::xml_node{});
    CHECK_EQ(
        std::string_view{cleared_first_run.child("w:rPr")
                             .child("w:rStyle")
                             .attribute("w:val")
                             .value()},
        "Strong");
    const auto cleared_second_run = find_run_xml_node(cleared_paragraph, 1U);
    REQUIRE(cleared_second_run != pugi::xml_node{});
    CHECK(cleared_second_run.child("w:rPr").child("w:rStyle") == pugi::xml_node{});

    remove_if_exists(source);
    remove_if_exists(styled);
    remove_if_exists(cleared);
    remove_if_exists(set_output);
    remove_if_exists(clear_output);
}

TEST_CASE("cli run style commands report parse and mutate errors") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_run_style_error_source.docx";
    const fs::path parse_output =
        working_directory / "cli_run_style_parse_output.json";
    const fs::path mutate_output =
        working_directory / "cli_run_style_mutate_output.json";

    remove_if_exists(source);
    remove_if_exists(parse_output);
    remove_if_exists(mutate_output);

    create_cli_style_usage_fixture(source);

    CHECK_EQ(run_cli({"set-run-style",
                      source.string(),
                      "1",
                      "oops",
                      "Strong",
                      "--json"},
                     parse_output),
             2);
    CHECK_EQ(
        read_text_file(parse_output),
        std::string{
            "{\"command\":\"set-run-style\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"invalid run index: oops\"}\n"});

    CHECK_EQ(run_cli({"clear-run-style", source.string(), "1", "99", "--json"},
                     mutate_output),
             1);
    const auto mutate_json = read_text_file(mutate_output);
    CHECK_NE(mutate_json.find("\"command\":\"clear-run-style\""),
             std::string::npos);
    CHECK_NE(mutate_json.find("\"stage\":\"mutate\""), std::string::npos);
    CHECK_NE(
        mutate_json.find(
            "\"detail\":\"run index '99' is out of range for paragraph index '1'\""),
        std::string::npos);
    CHECK_NE(mutate_json.find("\"entry\":\"word/document.xml\""),
             std::string::npos);

    remove_if_exists(source);
    remove_if_exists(parse_output);
    remove_if_exists(mutate_output);
}
