#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "cli_style_test_support.hpp"

namespace {

void create_cli_empty_document_fixture(const fs::path &path) {
    remove_if_exists(path);

    featherdoc::Document document(path);
    REQUIRE_FALSE(document.create_empty());
    REQUIRE_FALSE(document.save());
}

} // namespace

TEST_CASE("cli run font family commands set and clear body run font families") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_run_font_family_source.docx";
    const fs::path styled =
        working_directory / "cli_run_font_family_styled.docx";
    const fs::path cleared =
        working_directory / "cli_run_font_family_cleared.docx";
    const fs::path set_output =
        working_directory / "cli_run_font_family_set_output.json";
    const fs::path clear_output =
        working_directory / "cli_run_font_family_clear_output.json";
    const fs::path inspect_set_output =
        working_directory / "cli_run_font_family_inspect_set_output.json";
    const fs::path inspect_cleared_output =
        working_directory / "cli_run_font_family_inspect_cleared_output.json";

    remove_if_exists(source);
    remove_if_exists(styled);
    remove_if_exists(cleared);
    remove_if_exists(set_output);
    remove_if_exists(clear_output);
    remove_if_exists(inspect_set_output);
    remove_if_exists(inspect_cleared_output);

    create_cli_style_usage_fixture(source);

    CHECK_EQ(run_cli({"set-run-font-family",
                      source.string(),
                      "1",
                      "0",
                      "Courier New",
                      "--output",
                      styled.string(),
                      "--json"},
                     set_output),
             0);
    CHECK_EQ(
        read_text_file(set_output),
        std::string{
            "{\"command\":\"set-run-font-family\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":1,\"footers\":1,"
            "\"paragraph_index\":1,\"run_index\":0,\"font_family\":\"Courier New\"}\n"});

    const auto source_document_xml = read_docx_entry(source, "word/document.xml");
    pugi::xml_document source_xml_document;
    REQUIRE(source_xml_document.load_string(source_document_xml.c_str()));
    const auto source_paragraph =
        find_body_paragraph_xml_node(source_xml_document.child("w:document"), 1U);
    REQUIRE(source_paragraph != pugi::xml_node{});
    const auto source_first_run = find_run_xml_node(source_paragraph, 0U);
    REQUIRE(source_first_run != pugi::xml_node{});
    CHECK(source_first_run.child("w:rPr").child("w:rFonts") == pugi::xml_node{});

    const auto styled_document_xml = read_docx_entry(styled, "word/document.xml");
    pugi::xml_document styled_xml_document;
    REQUIRE(styled_xml_document.load_string(styled_document_xml.c_str()));
    const auto styled_paragraph =
        find_body_paragraph_xml_node(styled_xml_document.child("w:document"), 1U);
    REQUIRE(styled_paragraph != pugi::xml_node{});
    const auto styled_first_run = find_run_xml_node(styled_paragraph, 0U);
    REQUIRE(styled_first_run != pugi::xml_node{});
    const auto styled_fonts = styled_first_run.child("w:rPr").child("w:rFonts");
    REQUIRE(styled_fonts != pugi::xml_node{});
    CHECK_EQ(std::string_view{styled_fonts.attribute("w:ascii").value()},
             "Courier New");
    CHECK_EQ(std::string_view{styled_fonts.attribute("w:hAnsi").value()},
             "Courier New");
    CHECK_EQ(std::string_view{styled_fonts.attribute("w:cs").value()},
             "Courier New");

    CHECK_EQ(run_cli({"inspect-runs",
                      styled.string(),
                      "1",
                      "--run",
                      "0",
                      "--json"},
                     inspect_set_output),
             0);
    CHECK_EQ(
        read_text_file(inspect_set_output),
        std::string{
            "{\"paragraph_index\":1,\"run\":{\"index\":0,\"style_id\":null,"
            "\"font_family\":\"Courier New\",\"east_asia_font_family\":null,"
            "\"text_color\":null,\"bold\":null,\"italic\":null,"
            "\"underline\":null,\"font_size_points\":null,"
            "\"language\":null,\"text\":\"beta\"}}\n"});

    CHECK_EQ(run_cli({"clear-run-font-family",
                      styled.string(),
                      "1",
                      "0",
                      "--output",
                      cleared.string(),
                      "--json"},
                     clear_output),
             0);
    CHECK_EQ(
        read_text_file(clear_output),
        std::string{
            "{\"command\":\"clear-run-font-family\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":1,\"footers\":1,"
            "\"paragraph_index\":1,\"run_index\":0}\n"});

    const auto cleared_document_xml = read_docx_entry(cleared, "word/document.xml");
    pugi::xml_document cleared_xml_document;
    REQUIRE(cleared_xml_document.load_string(cleared_document_xml.c_str()));
    const auto cleared_paragraph =
        find_body_paragraph_xml_node(cleared_xml_document.child("w:document"), 1U);
    REQUIRE(cleared_paragraph != pugi::xml_node{});
    const auto cleared_first_run = find_run_xml_node(cleared_paragraph, 0U);
    REQUIRE(cleared_first_run != pugi::xml_node{});
    CHECK(cleared_first_run.child("w:rPr") == pugi::xml_node{});
    const auto cleared_second_run = find_run_xml_node(cleared_paragraph, 1U);
    REQUIRE(cleared_second_run != pugi::xml_node{});
    CHECK_EQ(
        std::string_view{cleared_second_run.child("w:rPr")
                             .child("w:rStyle")
                             .attribute("w:val")
                             .value()},
        "Strong");

    CHECK_EQ(run_cli({"inspect-runs",
                      cleared.string(),
                      "1",
                      "--run",
                      "0",
                      "--json"},
                     inspect_cleared_output),
             0);
    CHECK_EQ(
        read_text_file(inspect_cleared_output),
        std::string{
            "{\"paragraph_index\":1,\"run\":{\"index\":0,\"style_id\":null,"
            "\"font_family\":null,\"east_asia_font_family\":null,"
            "\"text_color\":null,\"bold\":null,\"italic\":null,"
            "\"underline\":null,\"font_size_points\":null,"
            "\"language\":null,\"text\":\"beta\"}}\n"});

    remove_if_exists(source);
    remove_if_exists(styled);
    remove_if_exists(cleared);
    remove_if_exists(set_output);
    remove_if_exists(clear_output);
    remove_if_exists(inspect_set_output);
    remove_if_exists(inspect_cleared_output);
}

TEST_CASE("cli run font family commands report parse and mutate errors") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_run_font_family_error_source.docx";
    const fs::path parse_output =
        working_directory / "cli_run_font_family_parse_output.json";
    const fs::path mutate_output =
        working_directory / "cli_run_font_family_mutate_output.json";

    remove_if_exists(source);
    remove_if_exists(parse_output);
    remove_if_exists(mutate_output);

    create_cli_style_usage_fixture(source);

    CHECK_EQ(run_cli({"set-run-font-family",
                      source.string(),
                      "1",
                      "oops",
                      "Courier New",
                      "--json"},
                     parse_output),
             2);
    CHECK_EQ(
        read_text_file(parse_output),
        std::string{
            "{\"command\":\"set-run-font-family\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"invalid run index: oops\"}\n"});

    CHECK_EQ(run_cli({"clear-run-font-family", source.string(), "1", "99", "--json"},
                     mutate_output),
             1);
    const auto mutate_json = read_text_file(mutate_output);
    CHECK_NE(mutate_json.find("\"command\":\"clear-run-font-family\""),
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

TEST_CASE("cli run language commands set and clear body run language") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_run_language_source.docx";
    const fs::path updated =
        working_directory / "cli_run_language_updated.docx";
    const fs::path cleared =
        working_directory / "cli_run_language_cleared.docx";
    const fs::path set_output =
        working_directory / "cli_run_language_set_output.json";
    const fs::path clear_output =
        working_directory / "cli_run_language_clear_output.json";
    const fs::path inspect_set_output =
        working_directory / "cli_run_language_inspect_set_output.json";
    const fs::path inspect_cleared_output =
        working_directory / "cli_run_language_inspect_cleared_output.json";

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(cleared);
    remove_if_exists(set_output);
    remove_if_exists(clear_output);
    remove_if_exists(inspect_set_output);
    remove_if_exists(inspect_cleared_output);

    create_cli_style_usage_fixture(source);

    CHECK_EQ(run_cli({"set-run-language",
                      source.string(),
                      "1",
                      "0",
                      "ja-JP",
                      "--output",
                      updated.string(),
                      "--json"},
                     set_output),
             0);
    CHECK_EQ(
        read_text_file(set_output),
        std::string{
            "{\"command\":\"set-run-language\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":1,\"footers\":1,"
            "\"paragraph_index\":1,\"run_index\":0,\"language\":\"ja-JP\"}\n"});

    const auto source_document_xml = read_docx_entry(source, "word/document.xml");
    pugi::xml_document source_xml_document;
    REQUIRE(source_xml_document.load_string(source_document_xml.c_str()));
    const auto source_paragraph =
        find_body_paragraph_xml_node(source_xml_document.child("w:document"), 1U);
    REQUIRE(source_paragraph != pugi::xml_node{});
    const auto source_first_run = find_run_xml_node(source_paragraph, 0U);
    REQUIRE(source_first_run != pugi::xml_node{});
    CHECK(source_first_run.child("w:rPr").child("w:lang") == pugi::xml_node{});

    const auto updated_document_xml = read_docx_entry(updated, "word/document.xml");
    pugi::xml_document updated_xml_document;
    REQUIRE(updated_xml_document.load_string(updated_document_xml.c_str()));
    const auto updated_paragraph =
        find_body_paragraph_xml_node(updated_xml_document.child("w:document"), 1U);
    REQUIRE(updated_paragraph != pugi::xml_node{});
    const auto updated_first_run = find_run_xml_node(updated_paragraph, 0U);
    REQUIRE(updated_first_run != pugi::xml_node{});
    const auto updated_language = updated_first_run.child("w:rPr").child("w:lang");
    REQUIRE(updated_language != pugi::xml_node{});
    CHECK_EQ(std::string_view{updated_language.attribute("w:val").value()},
             "ja-JP");

    CHECK_EQ(run_cli({"inspect-runs",
                      updated.string(),
                      "1",
                      "--run",
                      "0",
                      "--json"},
                     inspect_set_output),
             0);
    CHECK_EQ(
        read_text_file(inspect_set_output),
        std::string{
            "{\"paragraph_index\":1,\"run\":{\"index\":0,\"style_id\":null,"
            "\"font_family\":null,\"east_asia_font_family\":null,"
            "\"text_color\":null,\"bold\":null,\"italic\":null,"
            "\"underline\":null,\"font_size_points\":null,"
            "\"language\":\"ja-JP\",\"text\":\"beta\"}}\n"});

    CHECK_EQ(run_cli({"clear-run-language",
                      updated.string(),
                      "1",
                      "0",
                      "--output",
                      cleared.string(),
                      "--json"},
                     clear_output),
             0);
    CHECK_EQ(
        read_text_file(clear_output),
        std::string{
            "{\"command\":\"clear-run-language\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":1,\"footers\":1,"
            "\"paragraph_index\":1,\"run_index\":0}\n"});

    const auto cleared_document_xml = read_docx_entry(cleared, "word/document.xml");
    pugi::xml_document cleared_xml_document;
    REQUIRE(cleared_xml_document.load_string(cleared_document_xml.c_str()));
    const auto cleared_paragraph =
        find_body_paragraph_xml_node(cleared_xml_document.child("w:document"), 1U);
    REQUIRE(cleared_paragraph != pugi::xml_node{});
    const auto cleared_first_run = find_run_xml_node(cleared_paragraph, 0U);
    REQUIRE(cleared_first_run != pugi::xml_node{});
    CHECK(cleared_first_run.child("w:rPr") == pugi::xml_node{});
    const auto cleared_second_run = find_run_xml_node(cleared_paragraph, 1U);
    REQUIRE(cleared_second_run != pugi::xml_node{});
    CHECK_EQ(
        std::string_view{cleared_second_run.child("w:rPr")
                             .child("w:rStyle")
                             .attribute("w:val")
                             .value()},
        "Strong");

    CHECK_EQ(run_cli({"inspect-runs",
                      cleared.string(),
                      "1",
                      "--run",
                      "0",
                      "--json"},
                     inspect_cleared_output),
             0);
    CHECK_EQ(
        read_text_file(inspect_cleared_output),
        std::string{
            "{\"paragraph_index\":1,\"run\":{\"index\":0,\"style_id\":null,"
            "\"font_family\":null,\"east_asia_font_family\":null,"
            "\"text_color\":null,\"bold\":null,\"italic\":null,"
            "\"underline\":null,\"font_size_points\":null,"
            "\"language\":null,\"text\":\"beta\"}}\n"});

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(cleared);
    remove_if_exists(set_output);
    remove_if_exists(clear_output);
    remove_if_exists(inspect_set_output);
    remove_if_exists(inspect_cleared_output);
}

TEST_CASE("cli run language commands report parse and mutate errors") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_run_language_error_source.docx";
    const fs::path parse_output =
        working_directory / "cli_run_language_parse_output.json";
    const fs::path mutate_output =
        working_directory / "cli_run_language_mutate_output.json";

    remove_if_exists(source);
    remove_if_exists(parse_output);
    remove_if_exists(mutate_output);

    create_cli_style_usage_fixture(source);

    CHECK_EQ(run_cli({"set-run-language",
                      source.string(),
                      "1",
                      "oops",
                      "ja-JP",
                      "--json"},
                     parse_output),
             2);
    CHECK_EQ(
        read_text_file(parse_output),
        std::string{
            "{\"command\":\"set-run-language\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"invalid run index: oops\"}\n"});

    CHECK_EQ(run_cli({"clear-run-language", source.string(), "1", "99", "--json"},
                     mutate_output),
             1);
    const auto mutate_json = read_text_file(mutate_output);
    CHECK_NE(mutate_json.find("\"command\":\"clear-run-language\""),
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

TEST_CASE("cli default run properties commands set inspect and clear docDefaults") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_default_run_properties_source.docx";
    const fs::path updated =
        working_directory / "cli_default_run_properties_updated.docx";
    const fs::path cleared =
        working_directory / "cli_default_run_properties_cleared.docx";
    const fs::path set_output =
        working_directory / "cli_default_run_properties_set.json";
    const fs::path inspect_updated_output =
        working_directory / "cli_default_run_properties_inspect_updated.json";
    const fs::path clear_output =
        working_directory / "cli_default_run_properties_clear.json";
    const fs::path inspect_cleared_output =
        working_directory / "cli_default_run_properties_inspect_cleared.json";

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(cleared);
    remove_if_exists(set_output);
    remove_if_exists(inspect_updated_output);
    remove_if_exists(clear_output);
    remove_if_exists(inspect_cleared_output);

    create_cli_empty_document_fixture(source);

    CHECK_EQ(run_cli({"set-default-run-properties",
                      source.string(),
                      "--font-family",
                      "Segoe UI",
                      "--east-asia-font-family",
                      "Microsoft YaHei",
                      "--language",
                      "en-US",
                      "--east-asia-language",
                      "zh-CN",
                      "--bidi-language",
                      "ar-SA",
                      "--rtl",
                      "true",
                      "--paragraph-bidi",
                      "true",
                      "--output",
                      updated.string(),
                      "--json"},
                     set_output),
             0);
    const auto set_json = read_text_file(set_output);
    CHECK_NE(set_json.find("\"command\":\"set-default-run-properties\""),
             std::string::npos);
    CHECK_NE(set_json.find("\"font_family\":\"Segoe UI\""), std::string::npos);
    CHECK_NE(set_json.find("\"east_asia_font_family\":\"Microsoft YaHei\""),
             std::string::npos);
    CHECK_NE(set_json.find("\"language\":\"en-US\""), std::string::npos);
    CHECK_NE(set_json.find("\"east_asia_language\":\"zh-CN\""),
             std::string::npos);
    CHECK_NE(set_json.find("\"bidi_language\":\"ar-SA\""),
             std::string::npos);
    CHECK_NE(set_json.find("\"rtl\":true"), std::string::npos);
    CHECK_NE(set_json.find("\"paragraph_bidi\":true"), std::string::npos);

    CHECK_EQ(run_cli({"inspect-default-run-properties", updated.string(), "--json"},
                     inspect_updated_output),
             0);
    CHECK_EQ(
        read_text_file(inspect_updated_output),
        std::string{
            "{\"default_run_properties\":{\"font_family\":\"Segoe UI\","
            "\"east_asia_font_family\":\"Microsoft YaHei\","
            "\"language\":\"en-US\",\"east_asia_language\":\"zh-CN\","
            "\"bidi_language\":\"ar-SA\",\"rtl\":true,"
            "\"paragraph_bidi\":true}}\n"});

    CHECK_EQ(run_cli({"clear-default-run-properties",
                      updated.string(),
                      "--east-asia-font-family",
                      "--primary-language",
                      "--rtl",
                      "--output",
                      cleared.string(),
                      "--json"},
                     clear_output),
             0);
    const auto clear_json = read_text_file(clear_output);
    CHECK_NE(clear_json.find("\"command\":\"clear-default-run-properties\""),
             std::string::npos);
    CHECK_NE(clear_json.find("\"cleared\":[\"east_asia_font_family\","
                             "\"primary_language\",\"rtl\"]"),
             std::string::npos);

    CHECK_EQ(run_cli({"inspect-default-run-properties", cleared.string(), "--json"},
                     inspect_cleared_output),
             0);
    CHECK_EQ(
        read_text_file(inspect_cleared_output),
        std::string{
            "{\"default_run_properties\":{\"font_family\":\"Segoe UI\","
            "\"east_asia_font_family\":null,\"language\":null,"
            "\"east_asia_language\":\"zh-CN\",\"bidi_language\":\"ar-SA\","
            "\"rtl\":null,\"paragraph_bidi\":true}}\n"});

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(cleared);
    remove_if_exists(set_output);
    remove_if_exists(inspect_updated_output);
    remove_if_exists(clear_output);
    remove_if_exists(inspect_cleared_output);
}

TEST_CASE("cli default run properties commands report parse errors") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_default_run_properties_error_source.docx";
    const fs::path set_output =
        working_directory / "cli_default_run_properties_set_error.json";
    const fs::path clear_output =
        working_directory / "cli_default_run_properties_clear_error.json";
    const fs::path inspect_output =
        working_directory / "cli_default_run_properties_inspect_error.json";

    remove_if_exists(source);
    remove_if_exists(set_output);
    remove_if_exists(clear_output);
    remove_if_exists(inspect_output);

    create_cli_empty_document_fixture(source);

    CHECK_EQ(run_cli({"set-default-run-properties",
                      source.string(),
                      "--rtl",
                      "maybe",
                      "--json"},
                     set_output),
             2);
    CHECK_EQ(
        read_text_file(set_output),
        std::string{
            "{\"command\":\"set-default-run-properties\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"invalid --rtl value: maybe\"}\n"});

    CHECK_EQ(run_cli({"clear-default-run-properties", source.string(), "--json"},
                     clear_output),
             2);
    CHECK_EQ(
        read_text_file(clear_output),
        std::string{
            "{\"command\":\"clear-default-run-properties\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"clear-default-run-properties "
            "requires at least one clear option\"}\n"});

    CHECK_EQ(run_cli({"inspect-default-run-properties",
                      source.string(),
                      "--bogus",
                      "--json"},
                     inspect_output),
             2);
    CHECK_EQ(
        read_text_file(inspect_output),
        std::string{
            "{\"command\":\"inspect-default-run-properties\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"unknown option: --bogus\"}\n"});

    remove_if_exists(source);
    remove_if_exists(set_output);
    remove_if_exists(clear_output);
    remove_if_exists(inspect_output);
}

TEST_CASE("cli style run properties commands set inspect and clear style metadata") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_style_run_properties_source.docx";
    const fs::path updated =
        working_directory / "cli_style_run_properties_updated.docx";
    const fs::path cleared =
        working_directory / "cli_style_run_properties_cleared.docx";
    const fs::path set_output =
        working_directory / "cli_style_run_properties_set.json";
    const fs::path inspect_updated_output =
        working_directory / "cli_style_run_properties_inspect_updated.json";
    const fs::path clear_output =
        working_directory / "cli_style_run_properties_clear.json";
    const fs::path inspect_cleared_output =
        working_directory / "cli_style_run_properties_inspect_cleared.json";

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(cleared);
    remove_if_exists(set_output);
    remove_if_exists(inspect_updated_output);
    remove_if_exists(clear_output);
    remove_if_exists(inspect_cleared_output);

    create_cli_style_defaults_fixture(source);

    CHECK_EQ(run_cli({"set-style-run-properties",
                      source.string(),
                      "Normal",
                      "--font-family",
                      "Segoe UI",
                      "--east-asia-font-family",
                      "Microsoft YaHei",
                      "--language",
                      "en-US",
                      "--east-asia-language",
                      "zh-CN",
                      "--bidi-language",
                      "ar-SA",
                      "--rtl",
                      "true",
                      "--paragraph-bidi",
                      "true",
                      "--output",
                      updated.string(),
                      "--json"},
                     set_output),
             0);
    const auto set_json = read_text_file(set_output);
    CHECK_NE(set_json.find("\"command\":\"set-style-run-properties\""),
             std::string::npos);
    CHECK_NE(set_json.find("\"style_id\":\"Normal\""), std::string::npos);
    CHECK_NE(set_json.find("\"font_family\":\"Segoe UI\""), std::string::npos);
    CHECK_NE(set_json.find("\"east_asia_font_family\":\"Microsoft YaHei\""),
             std::string::npos);
    CHECK_NE(set_json.find("\"language\":\"en-US\""), std::string::npos);
    CHECK_NE(set_json.find("\"east_asia_language\":\"zh-CN\""),
             std::string::npos);
    CHECK_NE(set_json.find("\"bidi_language\":\"ar-SA\""),
             std::string::npos);
    CHECK_NE(set_json.find("\"rtl\":true"), std::string::npos);
    CHECK_NE(set_json.find("\"paragraph_bidi\":true"), std::string::npos);

    CHECK_EQ(run_cli({"inspect-style-run-properties",
                      updated.string(),
                      "Normal",
                      "--json"},
                     inspect_updated_output),
             0);
    CHECK_EQ(
        read_text_file(inspect_updated_output),
        std::string{
            "{\"style_id\":\"Normal\",\"style_run_properties\":{"
            "\"font_family\":\"Segoe UI\","
            "\"east_asia_font_family\":\"Microsoft YaHei\","
            "\"language\":\"en-US\",\"east_asia_language\":\"zh-CN\","
            "\"bidi_language\":\"ar-SA\",\"rtl\":true,"
            "\"paragraph_bidi\":true}}\n"});

    const auto styles_xml = read_docx_entry(updated, "word/styles.xml");
    pugi::xml_document styles_document;
    REQUIRE(styles_document.load_string(styles_xml.c_str()));
    const auto normal_style =
        find_style_xml_node(styles_document.child("w:styles"), "Normal");
    REQUIRE(normal_style != pugi::xml_node{});
    const auto run_properties = normal_style.child("w:rPr");
    REQUIRE(run_properties != pugi::xml_node{});
    const auto fonts = run_properties.child("w:rFonts");
    REQUIRE(fonts != pugi::xml_node{});
    CHECK_EQ(std::string_view{fonts.attribute("w:ascii").value()}, "Segoe UI");
    CHECK_EQ(std::string_view{fonts.attribute("w:eastAsia").value()},
             "Microsoft YaHei");
    const auto language = run_properties.child("w:lang");
    REQUIRE(language != pugi::xml_node{});
    CHECK_EQ(std::string_view{language.attribute("w:val").value()}, "en-US");
    CHECK_EQ(std::string_view{language.attribute("w:eastAsia").value()}, "zh-CN");
    CHECK_EQ(std::string_view{language.attribute("w:bidi").value()}, "ar-SA");
    CHECK(run_properties.child("w:rtl") != pugi::xml_node{});
    CHECK(normal_style.child("w:pPr").child("w:bidi") != pugi::xml_node{});

    CHECK_EQ(run_cli({"clear-style-run-properties",
                      updated.string(),
                      "Normal",
                      "--east-asia-font-family",
                      "--primary-language",
                      "--rtl",
                      "--paragraph-bidi",
                      "--output",
                      cleared.string(),
                      "--json"},
                     clear_output),
             0);
    const auto clear_json = read_text_file(clear_output);
    CHECK_NE(clear_json.find("\"command\":\"clear-style-run-properties\""),
             std::string::npos);
    CHECK_NE(clear_json.find("\"style_id\":\"Normal\""), std::string::npos);
    CHECK_NE(clear_json.find("\"cleared\":[\"east_asia_font_family\","
                             "\"primary_language\",\"rtl\",\"paragraph_bidi\"]"),
             std::string::npos);

    CHECK_EQ(run_cli({"inspect-style-run-properties",
                      cleared.string(),
                      "Normal",
                      "--json"},
                     inspect_cleared_output),
             0);
    CHECK_EQ(
        read_text_file(inspect_cleared_output),
        std::string{
            "{\"style_id\":\"Normal\",\"style_run_properties\":{"
            "\"font_family\":\"Segoe UI\","
            "\"east_asia_font_family\":null,"
            "\"language\":null,\"east_asia_language\":\"zh-CN\","
            "\"bidi_language\":\"ar-SA\",\"rtl\":null,"
            "\"paragraph_bidi\":null}}\n"});

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(cleared);
    remove_if_exists(set_output);
    remove_if_exists(inspect_updated_output);
    remove_if_exists(clear_output);
    remove_if_exists(inspect_cleared_output);
}

TEST_CASE("cli style run properties commands report parse and mutate errors") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_style_run_properties_error_source.docx";
    const fs::path set_output =
        working_directory / "cli_style_run_properties_set_error.json";
    const fs::path clear_output =
        working_directory / "cli_style_run_properties_clear_error.json";
    const fs::path inspect_output =
        working_directory / "cli_style_run_properties_inspect_error.json";

    remove_if_exists(source);
    remove_if_exists(set_output);
    remove_if_exists(clear_output);
    remove_if_exists(inspect_output);

    create_cli_style_defaults_fixture(source);

    CHECK_EQ(run_cli({"set-style-run-properties",
                      source.string(),
                      "Normal",
                      "--rtl",
                      "maybe",
                      "--json"},
                     set_output),
             2);
    CHECK_EQ(
        read_text_file(set_output),
        std::string{
            "{\"command\":\"set-style-run-properties\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"invalid --rtl value: maybe\"}\n"});

    CHECK_EQ(run_cli({"clear-style-run-properties",
                      source.string(),
                      "Normal",
                      "--json"},
                     clear_output),
             2);
    CHECK_EQ(
        read_text_file(clear_output),
        std::string{
            "{\"command\":\"clear-style-run-properties\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"clear-style-run-properties "
            "requires at least one clear option\"}\n"});

    CHECK_EQ(run_cli({"inspect-style-run-properties",
                      source.string(),
                      "MissingStyle",
                      "--json"},
                     inspect_output),
             1);
    const auto inspect_json = read_text_file(inspect_output);
    CHECK_NE(inspect_json.find("\"command\":\"inspect-style-run-properties\""),
             std::string::npos);
    CHECK_NE(inspect_json.find("\"stage\":\"inspect\""), std::string::npos);
    CHECK_NE(inspect_json.find(
                 "\"detail\":\"style id 'MissingStyle' was not found in "
                 "word/styles.xml\""),
             std::string::npos);
    CHECK_NE(inspect_json.find("\"entry\":\"word/styles.xml\""),
             std::string::npos);

    remove_if_exists(source);
    remove_if_exists(set_output);
    remove_if_exists(clear_output);
    remove_if_exists(inspect_output);
}
