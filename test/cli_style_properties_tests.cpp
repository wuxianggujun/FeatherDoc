#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "cli_test_support.hpp"
#include "cli_style_test_support.hpp"

namespace {

auto find_table_style_region_xml_node(pugi::xml_node style,
                                      std::string_view type_name)
    -> pugi::xml_node {
    for (auto region = style.child("w:tblStylePr"); region != pugi::xml_node{};
         region = region.next_sibling("w:tblStylePr")) {
        if (std::string_view{region.attribute("w:type").value()} == type_name) {
            return region;
        }
    }

    return {};
}
} // namespace

TEST_CASE("cli prune-unused-styles removes unreachable custom styles") {
    const fs::path working_directory = fs::current_path();
    const fs::path source = working_directory / "cli_prune_unused_styles_source.docx";
    const fs::path pruned = working_directory / "cli_prune_unused_styles_pruned.docx";
    const fs::path plan_output = working_directory / "cli_prune_unused_styles_plan.json";
    const fs::path output = working_directory / "cli_prune_unused_styles_output.json";

    remove_if_exists(source);
    remove_if_exists(pruned);
    remove_if_exists(plan_output);
    remove_if_exists(output);

    create_cli_style_usage_fixture(source);
    {
        featherdoc::Document document(source);
        REQUIRE_FALSE(document.open());

        featherdoc::paragraph_style_definition dependency_style;
        dependency_style.name = "Dependency Style";
        dependency_style.based_on = "Normal";
        REQUIRE(document.ensure_paragraph_style("DependencyStyle", dependency_style));

        featherdoc::paragraph_style_definition custom_body;
        custom_body.name = "Custom Body";
        custom_body.based_on = "DependencyStyle";
        custom_body.is_quick_format = true;
        REQUIRE(document.ensure_paragraph_style("CustomBody", custom_body));

        featherdoc::paragraph_style_definition unused_body;
        unused_body.name = "Unused Body";
        unused_body.based_on = "Normal";
        REQUIRE(document.ensure_paragraph_style("UnusedBody", unused_body));

        featherdoc::character_style_definition unused_character;
        unused_character.name = "Unused Character";
        REQUIRE(document.ensure_character_style("UnusedCharacter", unused_character));

        featherdoc::table_style_definition unused_table;
        unused_table.name = "Unused Table";
        REQUIRE(document.ensure_table_style("UnusedTable", unused_table));

        REQUIRE_FALSE(document.save());
    }

    CHECK_EQ(run_cli({"plan-prune-unused-styles", source.string(), "--json"},
                     plan_output),
             0);
    const auto plan_json = read_text_file(plan_output);
    CHECK_NE(plan_json.find(R"("command":"plan-prune-unused-styles")"),
             std::string::npos);
    CHECK_NE(plan_json.find(R"("removable_style_count":3)"), std::string::npos);
    CHECK_NE(plan_json.find(R"("UnusedBody")"), std::string::npos);
    CHECK_NE(plan_json.find(R"("UnusedCharacter")"), std::string::npos);
    CHECK_NE(plan_json.find(R"("UnusedTable")"), std::string::npos);
    const auto source_styles_xml = read_docx_entry(source, "word/styles.xml");
    CHECK_NE(source_styles_xml.find(R"(w:styleId="UnusedBody")"), std::string::npos);

    CHECK_EQ(run_cli({"prune-unused-styles",
                      source.string(),
                      "--output",
                      pruned.string(),
                      "--json"},
                     output),
             0);
    const auto prune_json = read_text_file(output);
    CHECK_NE(prune_json.find(R"("command":"prune-unused-styles")"),
             std::string::npos);
    CHECK_NE(prune_json.find(R"("removed_style_count":3)"), std::string::npos);
    CHECK_NE(prune_json.find(R"("UnusedBody")"), std::string::npos);
    CHECK_NE(prune_json.find(R"("UnusedCharacter")"), std::string::npos);
    CHECK_NE(prune_json.find(R"("UnusedTable")"), std::string::npos);

    const auto styles_xml = read_docx_entry(pruned, "word/styles.xml");
    CHECK_NE(styles_xml.find(R"(w:styleId="CustomBody")"), std::string::npos);
    CHECK_NE(styles_xml.find(R"(w:styleId="DependencyStyle")"), std::string::npos);
    CHECK_EQ(styles_xml.find(R"(w:styleId="UnusedBody")"), std::string::npos);
    CHECK_EQ(styles_xml.find(R"(w:styleId="UnusedCharacter")"), std::string::npos);
    CHECK_EQ(styles_xml.find(R"(w:styleId="UnusedTable")"), std::string::npos);

    remove_if_exists(source);
    remove_if_exists(pruned);
    remove_if_exists(plan_output);
    remove_if_exists(output);
}

TEST_CASE("cli inspect-styles reports full style usage catalog") {
    const fs::path working_directory = fs::current_path();
    const fs::path source = working_directory / "cli_styles_usage_report_source.docx";
    const fs::path output = working_directory / "cli_styles_usage_report.json";

    remove_if_exists(source);
    remove_if_exists(output);

    create_cli_style_usage_fixture(source);

    CHECK_EQ(run_cli({"inspect-styles", source.string(), "--usage", "--json"}, output), 0);
    const auto report_json = read_text_file(output);
    CHECK_NE(report_json.find(R"("count":4)"), std::string::npos);
    CHECK_NE(report_json.find(R"("used_style_count":3)"), std::string::npos);
    CHECK_NE(report_json.find(R"("unused_style_count":1)"), std::string::npos);
    CHECK_NE(report_json.find(R"("total_reference_count":8)"), std::string::npos);
    CHECK_NE(report_json.find(R"("style":{"style_id":"CustomBody")"),
             std::string::npos);
    CHECK_NE(report_json.find(R"("usage":{"style_id":"CustomBody","paragraph_count":3)"),
             std::string::npos);
    CHECK_NE(report_json.find(R"("style":{"style_id":"Normal")"), std::string::npos);
    CHECK_NE(report_json.find(R"("usage":{"style_id":"Normal","paragraph_count":0)"),
             std::string::npos);

    remove_if_exists(source);
    remove_if_exists(output);
}

TEST_CASE("cli paragraph style property commands inspect set and clear metadata without disturbing run properties") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_paragraph_style_properties_source.docx";
    const fs::path styled =
        working_directory / "cli_paragraph_style_properties_styled.docx";
    const fs::path updated =
        working_directory / "cli_paragraph_style_properties_updated.docx";
    const fs::path cleared =
        working_directory / "cli_paragraph_style_properties_cleared.docx";
    const fs::path ensure_output =
        working_directory / "cli_paragraph_style_properties_ensure.json";
    const fs::path set_output =
        working_directory / "cli_paragraph_style_properties_set.json";
    const fs::path inspect_output =
        working_directory / "cli_paragraph_style_properties_inspect.json";
    const fs::path run_output =
        working_directory / "cli_paragraph_style_properties_run.json";
    const fs::path clear_output =
        working_directory / "cli_paragraph_style_properties_clear.json";
    const fs::path cleared_inspect_output =
        working_directory / "cli_paragraph_style_properties_cleared_inspect.json";

    remove_if_exists(source);
    remove_if_exists(styled);
    remove_if_exists(updated);
    remove_if_exists(cleared);
    remove_if_exists(ensure_output);
    remove_if_exists(set_output);
    remove_if_exists(inspect_output);
    remove_if_exists(run_output);
    remove_if_exists(clear_output);
    remove_if_exists(cleared_inspect_output);

    create_cli_style_defaults_fixture(source);

    CHECK_EQ(run_cli({"ensure-paragraph-style",
                      source.string(),
                      "WorkingStyle",
                      "--name",
                      "Working Style",
                      "--based-on",
                      "Normal",
                      "--next-style",
                      "WorkingStyle",
                      "--run-font-family",
                      "Consolas",
                      "--output",
                      styled.string(),
                      "--json"},
                     ensure_output),
             0);

    CHECK_EQ(run_cli({"set-paragraph-style-properties",
                      styled.string(),
                      "WorkingStyle",
                      "--based-on",
                      "Heading1",
                      "--next-style",
                      "BodyText",
                      "--outline-level",
                      "2",
                      "--output",
                      updated.string(),
                      "--json"},
                     set_output),
             0);
    CHECK_EQ(
        read_text_file(set_output),
        std::string{
            "{\"command\":\"set-paragraph-style-properties\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"style_id\":\"WorkingStyle\",\"based_on\":\"Heading1\","
            "\"next_style\":\"BodyText\",\"outline_level\":2}\n"});

    CHECK_EQ(run_cli({"inspect-paragraph-style-properties",
                      updated.string(),
                      "WorkingStyle",
                      "--json"},
                     inspect_output),
             0);
    CHECK_EQ(
        read_text_file(inspect_output),
        std::string{
            "{\"style_id\":\"WorkingStyle\",\"paragraph_style_properties\":{"
            "\"based_on\":\"Heading1\",\"next_style\":\"BodyText\","
            "\"outline_level\":2}}\n"});

    CHECK_EQ(run_cli({"inspect-style-run-properties",
                      updated.string(),
                      "WorkingStyle",
                      "--json"},
                     run_output),
             0);
    CHECK_EQ(
        read_text_file(run_output),
        std::string{
            "{\"style_id\":\"WorkingStyle\",\"style_run_properties\":{"
            "\"font_family\":\"Consolas\",\"east_asia_font_family\":null,"
            "\"language\":null,\"east_asia_language\":null,"
            "\"bidi_language\":null,\"rtl\":null,"
            "\"paragraph_bidi\":null}}\n"});

    CHECK_EQ(run_cli({"clear-paragraph-style-properties",
                      updated.string(),
                      "WorkingStyle",
                      "--based-on",
                      "--next-style",
                      "--outline-level",
                      "--output",
                      cleared.string(),
                      "--json"},
                     clear_output),
             0);
    CHECK_EQ(
        read_text_file(clear_output),
        std::string{
            "{\"command\":\"clear-paragraph-style-properties\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"style_id\":\"WorkingStyle\",\"cleared\":[\"based_on\","
            "\"next_style\",\"outline_level\"]}\n"});

    CHECK_EQ(run_cli({"inspect-paragraph-style-properties",
                      cleared.string(),
                      "WorkingStyle",
                      "--json"},
                     cleared_inspect_output),
             0);
    CHECK_EQ(
        read_text_file(cleared_inspect_output),
        std::string{
            "{\"style_id\":\"WorkingStyle\",\"paragraph_style_properties\":{"
            "\"based_on\":null,\"next_style\":null,"
            "\"outline_level\":null}}\n"});

    remove_if_exists(source);
    remove_if_exists(styled);
    remove_if_exists(updated);
    remove_if_exists(cleared);
    remove_if_exists(ensure_output);
    remove_if_exists(set_output);
    remove_if_exists(inspect_output);
    remove_if_exists(run_output);
    remove_if_exists(clear_output);
    remove_if_exists(cleared_inspect_output);
}

TEST_CASE("cli paragraph style property commands report parse and inspect errors") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_paragraph_style_properties_error_source.docx";
    const fs::path parse_output =
        working_directory / "cli_paragraph_style_properties_parse.json";
    const fs::path inspect_output =
        working_directory / "cli_paragraph_style_properties_inspect.json";

    remove_if_exists(source);
    remove_if_exists(parse_output);
    remove_if_exists(inspect_output);

    create_cli_style_defaults_fixture(source);

    CHECK_EQ(run_cli({"set-paragraph-style-properties",
                      source.string(),
                      "Normal",
                      "--outline-level",
                      "9",
                      "--json"},
                     parse_output),
             2);
    CHECK_EQ(
        read_text_file(parse_output),
        std::string{
            "{\"command\":\"set-paragraph-style-properties\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"invalid --outline-level value: expected 0-8\"}\n"});

    CHECK_EQ(run_cli({"inspect-paragraph-style-properties",
                      source.string(),
                      "MissingStyle",
                      "--json"},
                     inspect_output),
             1);
    const auto inspect_json = read_text_file(inspect_output);
    CHECK_NE(inspect_json.find("\"command\":\"inspect-paragraph-style-properties\""),
             std::string::npos);
    CHECK_NE(inspect_json.find("\"stage\":\"inspect\""), std::string::npos);
    CHECK_NE(inspect_json.find(
                 "\"detail\":\"style id 'MissingStyle' was not found in "
                 "word/styles.xml\""),
             std::string::npos);
    CHECK_NE(inspect_json.find("\"entry\":\"word/styles.xml\""),
             std::string::npos);

    remove_if_exists(source);
    remove_if_exists(parse_output);
    remove_if_exists(inspect_output);
}

TEST_CASE("cli ensure-paragraph-style creates a paragraph style with optional metadata") {
    const fs::path working_directory = fs::current_path();
    const fs::path source = working_directory / "cli_ensure_paragraph_style_source.docx";
    const fs::path updated = working_directory / "cli_ensure_paragraph_style_updated.docx";
    const fs::path output = working_directory / "cli_ensure_paragraph_style_output.json";

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(output);

    create_cli_style_defaults_fixture(source);

    CHECK_EQ(run_cli({"ensure-paragraph-style",
                      source.string(),
                      "LegalBody",
                      "--name",
                      "Legal Body",
                      "--based-on",
                      "Normal",
                      "--next-style",
                      "LegalBody",
                      "--custom",
                      "true",
                      "--semi-hidden",
                      "false",
                      "--unhide-when-used",
                      "true",
                      "--quick-format",
                      "true",
                      "--run-font-family",
                      "Segoe UI",
                      "--run-east-asia-font-family",
                      "Microsoft YaHei",
                      "--run-language",
                      "en-US",
                      "--run-east-asia-language",
                      "zh-CN",
                      "--run-bidi-language",
                      "ar-SA",
                      "--run-rtl",
                      "true",
                      "--paragraph-bidi",
                      "true",
                      "--outline-level",
                      "2",
                      "--output",
                      updated.string(),
                      "--json"},
                     output),
             0);
    CHECK_EQ(
        read_text_file(output),
        std::string{
            "{\"command\":\"ensure-paragraph-style\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"style\":{\"style_id\":\"LegalBody\",\"name\":\"Legal Body\","
            "\"based_on\":\"Normal\",\"kind\":\"paragraph\",\"type\":\"paragraph\","
            "\"numbering\":null,\"is_default\":false,\"is_custom\":true,"
            "\"is_semi_hidden\":false,\"is_unhide_when_used\":true,"
            "\"is_quick_format\":true}}\n"});

    const auto styles_xml = read_docx_entry(updated, "word/styles.xml");
    pugi::xml_document styles_document;
    REQUIRE(styles_document.load_string(styles_xml.c_str()));
    const auto style =
        find_style_xml_node(styles_document.child("w:styles"), "LegalBody");
    REQUIRE(style != pugi::xml_node{});
    CHECK_EQ(std::string_view{style.attribute("w:type").value()}, "paragraph");
    CHECK_EQ(std::string_view{style.child("w:name").attribute("w:val").value()},
             "Legal Body");
    CHECK_EQ(std::string_view{style.child("w:basedOn").attribute("w:val").value()},
             "Normal");
    CHECK_EQ(std::string_view{style.child("w:next").attribute("w:val").value()},
             "LegalBody");
    CHECK(style.child("w:qFormat") != pugi::xml_node{});
    CHECK(style.child("w:semiHidden") == pugi::xml_node{});
    CHECK(style.child("w:unhideWhenUsed") != pugi::xml_node{});
    const auto paragraph_properties = style.child("w:pPr");
    REQUIRE(paragraph_properties != pugi::xml_node{});
    CHECK(paragraph_properties.child("w:bidi") != pugi::xml_node{});
    CHECK_EQ(std::string_view{paragraph_properties.child("w:outlineLvl")
                                  .attribute("w:val")
                                  .value()},
             "2");
    const auto run_properties = style.child("w:rPr");
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

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(output);
}

TEST_CASE("cli ensure-character-style creates a character style with run metadata") {
    const fs::path working_directory = fs::current_path();
    const fs::path source = working_directory / "cli_ensure_character_style_source.docx";
    const fs::path updated = working_directory / "cli_ensure_character_style_updated.docx";
    const fs::path output = working_directory / "cli_ensure_character_style_output.json";

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(output);

    create_cli_style_defaults_fixture(source);

    CHECK_EQ(run_cli({"ensure-character-style",
                      source.string(),
                      "AccentStrong",
                      "--name",
                      "Accent Strong",
                      "--based-on",
                      "DefaultParagraphFont",
                      "--semi-hidden",
                      "true",
                      "--quick-format",
                      "true",
                      "--run-font-family",
                      "Segoe UI",
                      "--run-language",
                      "fr-FR",
                      "--run-rtl",
                      "false",
                      "--output",
                      updated.string(),
                      "--json"},
                     output),
             0);

    const auto styles_xml = read_docx_entry(updated, "word/styles.xml");
    pugi::xml_document styles_document;
    REQUIRE(styles_document.load_string(styles_xml.c_str()));
    const auto style =
        find_style_xml_node(styles_document.child("w:styles"), "AccentStrong");
    REQUIRE(style != pugi::xml_node{});
    CHECK_EQ(std::string_view{style.attribute("w:type").value()}, "character");
    CHECK_EQ(std::string_view{style.child("w:name").attribute("w:val").value()},
             "Accent Strong");
    CHECK_EQ(std::string_view{style.child("w:basedOn").attribute("w:val").value()},
             "DefaultParagraphFont");
    CHECK(style.child("w:semiHidden") != pugi::xml_node{});
    CHECK(style.child("w:qFormat") != pugi::xml_node{});
    const auto run_properties = style.child("w:rPr");
    REQUIRE(run_properties != pugi::xml_node{});
    CHECK_EQ(std::string_view{run_properties.child("w:rFonts")
                                  .attribute("w:ascii")
                                  .value()},
             "Segoe UI");
    CHECK_EQ(std::string_view{run_properties.child("w:lang")
                                  .attribute("w:val")
                                  .value()},
             "fr-FR");
    CHECK_EQ(std::string_view{run_properties.child("w:rtl")
                                  .attribute("w:val")
                                  .value()},
             "0");

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(output);
}

TEST_CASE("cli ensure-table-style creates a table style with catalog flags") {
    const fs::path working_directory = fs::current_path();
    const fs::path source = working_directory / "cli_ensure_table_style_source.docx";
    const fs::path updated = working_directory / "cli_ensure_table_style_updated.docx";
    const fs::path output = working_directory / "cli_ensure_table_style_output.json";

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(output);

    create_cli_style_defaults_fixture(source);

    CHECK_EQ(run_cli({"ensure-table-style",
                      source.string(),
                      "ReportTable",
                      "--name",
                      "Report Table",
                      "--based-on",
                      "TableGrid",
                      "--unhide-when-used",
                      "true",
                       "--quick-format",
                       "true",
                       "--style-fill",
                       "whole_table:DDEEFF",
                       "--style-text-color",
                       "whole_table:333333",
                       "--style-bold",
                       "whole_table:false",
                       "--style-italic",
                       "whole_table:false",
                       "--style-font-size",
                       "whole_table:11",
                       "--style-font-family",
                       "whole_table:Aptos",
                       "--style-east-asia-font-family",
                       "whole_table:Microsoft YaHei",
                       "--style-cell-vertical-alignment",
                       "whole_table:center",
                       "--style-cell-text-direction",
                       "whole_table:left_to_right_top_to_bottom",
                       "--style-paragraph-alignment",
                       "whole_table:center",
                       "--style-paragraph-spacing-before",
                       "whole_table:120",
                       "--style-paragraph-spacing-after",
                       "whole_table:80",
                       "--style-paragraph-line-spacing",
                       "whole_table:360:exact",
                       "--style-margin",
                       "whole_table:left:120",
                       "--style-margin",
                       "whole_table:right:160",
                       "--style-border",
                       "whole_table:top:single:12:4472C4",
                       "--style-fill",
                       "first_row:1F4E79",
                       "--style-text-color",
                       "first_row:FFFFFF",
                       "--style-bold",
                       "first_row:true",
                       "--style-italic",
                       "first_row:true",
                       "--style-font-size",
                       "first_row:14",
                       "--style-font-family",
                       "first_row:Aptos Display",
                       "--style-east-asia-font-family",
                       "first_row:SimHei",
                       "--style-cell-vertical-alignment",
                       "first_row:bottom",
                       "--style-cell-text-direction",
                       "first_row:top_to_bottom_right_to_left",
                       "--style-paragraph-alignment",
                       "first_row:right",
                       "--style-paragraph-spacing-after",
                       "first_row:120",
                       "--style-paragraph-line-spacing",
                       "first_row:240:at_least",
                       "--style-border",
                       "first_row:bottom:double_line:8:1F4E79",
                       "--style-fill",
                       "second_banded_rows:F2F2F2",
                       "--style-text-color",
                       "second_banded_rows:666666",
                       "--style-fill",
                       "second_banded_columns:E2F0D9",
                       "--style-cell-vertical-alignment",
                       "second_banded_columns:top",
                       "--output",
                       updated.string(),
                       "--json"},
                     output),
             0);
    CHECK_EQ(
        read_text_file(output),
        std::string{
            "{\"command\":\"ensure-table-style\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"style\":{\"style_id\":\"ReportTable\",\"name\":\"Report Table\","
            "\"based_on\":\"TableGrid\",\"kind\":\"table\",\"type\":\"table\","
            "\"numbering\":null,\"is_default\":false,\"is_custom\":true,"
            "\"is_semi_hidden\":false,\"is_unhide_when_used\":true,"
            "\"is_quick_format\":true}}\n"});

    const auto styles_xml = read_docx_entry(updated, "word/styles.xml");
    pugi::xml_document styles_document;
    REQUIRE(styles_document.load_string(styles_xml.c_str()));
    const auto style =
        find_style_xml_node(styles_document.child("w:styles"), "ReportTable");
    REQUIRE(style != pugi::xml_node{});
    CHECK_EQ(std::string_view{style.attribute("w:type").value()}, "table");
    CHECK_EQ(std::string_view{style.child("w:name").attribute("w:val").value()},
             "Report Table");
    CHECK_EQ(std::string_view{style.child("w:basedOn").attribute("w:val").value()},
             "TableGrid");
    CHECK(style.child("w:qFormat") != pugi::xml_node{});
    CHECK(style.child("w:semiHidden") == pugi::xml_node{});
    CHECK(style.child("w:unhideWhenUsed") != pugi::xml_node{});
    const auto table_properties = style.child("w:tblPr");
    REQUIRE(table_properties != pugi::xml_node{});
    CHECK_EQ(std::string_view{table_properties.child("w:tblCellMar")
                                  .child("w:left")
                                  .attribute("w:w")
                                  .value()},
             "120");
    CHECK_EQ(std::string_view{table_properties.child("w:tblBorders")
                                  .child("w:top")
                                  .attribute("w:color")
                                  .value()},
             "4472C4");
    CHECK_EQ(std::string_view{style.child("w:tcPr").child("w:shd").attribute("w:fill").value()},
             "DDEEFF");
    CHECK_EQ(std::string_view{style.child("w:tcPr")
                                  .child("w:vAlign")
                                  .attribute("w:val")
                                  .value()},
             "center");
    CHECK_EQ(std::string_view{style.child("w:tcPr")
                                  .child("w:textDirection")
                                  .attribute("w:val")
                                  .value()},
             "lrTb");
    CHECK_EQ(std::string_view{style.child("w:pPr")
                                  .child("w:jc")
                                  .attribute("w:val")
                                  .value()},
             "center");
    const auto whole_paragraph_spacing_node =
        style.child("w:pPr").child("w:spacing");
    REQUIRE(whole_paragraph_spacing_node != pugi::xml_node{});
    CHECK_EQ(std::string_view{whole_paragraph_spacing_node.attribute("w:before")
                                  .value()},
             "120");
    CHECK_EQ(std::string_view{whole_paragraph_spacing_node.attribute("w:after")
                                  .value()},
             "80");
    CHECK_EQ(std::string_view{whole_paragraph_spacing_node.attribute("w:line")
                                  .value()},
             "360");
    CHECK_EQ(std::string_view{whole_paragraph_spacing_node.attribute("w:lineRule")
                                  .value()},
             "exact");
    CHECK_EQ(std::string_view{style.child("w:rPr")
                                  .child("w:color")
                                  .attribute("w:val")
                                  .value()},
             "333333");
    CHECK_EQ(std::string_view{style.child("w:rPr")
                                  .child("w:b")
                                  .attribute("w:val")
                                  .value()},
             "0");
    CHECK_EQ(std::string_view{style.child("w:rPr")
                                  .child("w:i")
                                  .attribute("w:val")
                                  .value()},
             "0");
    CHECK_EQ(std::string_view{style.child("w:rPr")
                                  .child("w:sz")
                                  .attribute("w:val")
                                  .value()},
             "22");
    CHECK_EQ(std::string_view{style.child("w:rPr")
                                  .child("w:szCs")
                                  .attribute("w:val")
                                  .value()},
             "22");
    CHECK_EQ(std::string_view{style.child("w:rPr")
                                  .child("w:rFonts")
                                  .attribute("w:ascii")
                                  .value()},
             "Aptos");
    CHECK_EQ(std::string_view{style.child("w:rPr")
                                  .child("w:rFonts")
                                  .attribute("w:eastAsia")
                                  .value()},
             "Microsoft YaHei");
    const auto first_row_region = find_table_style_region_xml_node(style, "firstRow");
    REQUIRE(first_row_region != pugi::xml_node{});
    CHECK_EQ(std::string_view{first_row_region.child("w:tcPr")
                                  .child("w:shd")
                                  .attribute("w:fill")
                                  .value()},
             "1F4E79");
    CHECK_EQ(std::string_view{first_row_region.child("w:tcPr")
                                  .child("w:vAlign")
                                  .attribute("w:val")
                                  .value()},
             "bottom");
    CHECK_EQ(std::string_view{first_row_region.child("w:tcPr")
                                  .child("w:textDirection")
                                  .attribute("w:val")
                                  .value()},
             "tbRl");
    CHECK_EQ(std::string_view{first_row_region.child("w:pPr")
                                  .child("w:jc")
                                  .attribute("w:val")
                                  .value()},
             "right");
    const auto first_row_paragraph_spacing_node =
        first_row_region.child("w:pPr").child("w:spacing");
    REQUIRE(first_row_paragraph_spacing_node != pugi::xml_node{});
    CHECK_EQ(std::string_view{first_row_paragraph_spacing_node.attribute("w:after")
                                  .value()},
             "120");
    CHECK_EQ(std::string_view{first_row_paragraph_spacing_node.attribute("w:line")
                                  .value()},
             "240");
    CHECK_EQ(std::string_view{first_row_paragraph_spacing_node.attribute("w:lineRule")
                                  .value()},
             "atLeast");
    CHECK_EQ(std::string_view{first_row_region.child("w:rPr")
                                  .child("w:color")
                                  .attribute("w:val")
                                  .value()},
             "FFFFFF");
    CHECK_EQ(std::string_view{first_row_region.child("w:rPr")
                                  .child("w:b")
                                  .attribute("w:val")
                                  .value()},
             "1");
    CHECK_EQ(std::string_view{first_row_region.child("w:rPr")
                                  .child("w:i")
                                  .attribute("w:val")
                                  .value()},
             "1");
    CHECK_EQ(std::string_view{first_row_region.child("w:rPr")
                                  .child("w:sz")
                                  .attribute("w:val")
                                  .value()},
             "28");
    CHECK_EQ(std::string_view{first_row_region.child("w:rPr")
                                  .child("w:szCs")
                                  .attribute("w:val")
                                  .value()},
             "28");
    CHECK_EQ(std::string_view{first_row_region.child("w:rPr")
                                  .child("w:rFonts")
                                  .attribute("w:ascii")
                                  .value()},
             "Aptos Display");
    CHECK_EQ(std::string_view{first_row_region.child("w:rPr")
                                  .child("w:rFonts")
                                  .attribute("w:eastAsia")
                                  .value()},
             "SimHei");
    CHECK_EQ(std::string_view{first_row_region.child("w:tcPr")
                                  .child("w:tcBorders")
                                  .child("w:bottom")
                                  .attribute("w:val")
                                  .value()},
             "double");

    const auto second_banded_rows_region =
        find_table_style_region_xml_node(style, "band2Horz");
    REQUIRE(second_banded_rows_region != pugi::xml_node{});
    CHECK_EQ(std::string_view{second_banded_rows_region.child("w:tcPr")
                                  .child("w:shd")
                                  .attribute("w:fill")
                                  .value()},
             "F2F2F2");
    CHECK_EQ(std::string_view{second_banded_rows_region.child("w:rPr")
                                  .child("w:color")
                                  .attribute("w:val")
                                  .value()},
             "666666");
    const auto second_banded_columns_region =
        find_table_style_region_xml_node(style, "band2Vert");
    REQUIRE(second_banded_columns_region != pugi::xml_node{});
    CHECK_EQ(std::string_view{second_banded_columns_region.child("w:tcPr")
                                  .child("w:shd")
                                  .attribute("w:fill")
                                  .value()},
             "E2F0D9");
    CHECK_EQ(std::string_view{second_banded_columns_region.child("w:tcPr")
                                  .child("w:vAlign")
                                  .attribute("w:val")
                                  .value()},
             "top");

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(output);
}

TEST_CASE("cli ensure-paragraph-style reports json parse errors") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_ensure_paragraph_style_parse_source.docx";
    const fs::path output =
        working_directory / "cli_ensure_paragraph_style_parse_output.json";

    remove_if_exists(source);
    remove_if_exists(output);

    create_cli_style_defaults_fixture(source);

    CHECK_EQ(run_cli({"ensure-paragraph-style",
                      source.string(),
                      "BodyText",
                      "--json"},
                     output),
             2);
    CHECK_EQ(
        read_text_file(output),
        std::string{
            "{\"command\":\"ensure-paragraph-style\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"missing required --name <name>\"}\n"});

    remove_if_exists(source);
    remove_if_exists(output);
}

