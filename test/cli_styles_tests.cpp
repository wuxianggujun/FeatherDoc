#include "cli_style_test_support.hpp"

TEST_CASE("cli inspect-styles lists the default catalog") {
    const fs::path working_directory = fs::current_path();
    const fs::path source = working_directory / "cli_styles_defaults_source.docx";
    const fs::path output = working_directory / "cli_styles_defaults_output.txt";

    remove_if_exists(source);
    remove_if_exists(output);

    create_cli_style_defaults_fixture(source);

    CHECK_EQ(run_cli({"inspect-styles", source.string()}, output), 0);
    const auto inspect_text = read_text_file(output);
    CHECK_NE(inspect_text.find("styles: 9"), std::string::npos);
    CHECK_NE(inspect_text.find("id=Normal name=Normal type=paragraph kind=paragraph"),
             std::string::npos);
    CHECK_NE(inspect_text.find("numbering=none"), std::string::npos);
    CHECK_NE(inspect_text.find("id=DefaultParagraphFont"), std::string::npos);
    CHECK_NE(inspect_text.find("semi_hidden=yes unhide_when_used=yes"),
             std::string::npos);
    CHECK_NE(inspect_text.find("id=TableGrid"), std::string::npos);
    CHECK_NE(inspect_text.find("based_on=TableNormal"), std::string::npos);

    remove_if_exists(source);
    remove_if_exists(output);
}

TEST_CASE("cli inspect-styles supports single-style json output and errors") {
    const fs::path working_directory = fs::current_path();
    const fs::path source = working_directory / "cli_styles_existing_source.docx";
    const fs::path style_output = working_directory / "cli_styles_single.json";
    const fs::path missing_output = working_directory / "cli_styles_missing.json";

    remove_if_exists(source);
    remove_if_exists(style_output);
    remove_if_exists(missing_output);

    create_cli_style_existing_fixture(source);

    CHECK_EQ(run_cli({"inspect-styles", source.string(), "--style", "CustomBody",
                      "--json"},
                     style_output),
             0);
    CHECK_EQ(
        read_text_file(style_output),
        std::string{
            "{\"style\":{\"style_id\":\"CustomBody\",\"name\":\"Custom Body\","
            "\"based_on\":\"Normal\",\"kind\":\"paragraph\",\"type\":\"paragraph\","
            "\"numbering\":null,"
            "\"is_default\":false,\"is_custom\":true,\"is_semi_hidden\":false,"
            "\"is_unhide_when_used\":false,\"is_quick_format\":true}}\n"});

    CHECK_EQ(run_cli({"inspect-styles", source.string(), "--style", "MissingStyle",
                      "--json"},
                     missing_output),
             1);
    const auto missing_json = read_text_file(missing_output);
    CHECK_NE(missing_json.find("\"command\":\"inspect-styles\""), std::string::npos);
    CHECK_NE(missing_json.find("\"stage\":\"inspect\""), std::string::npos);
    CHECK_NE(missing_json.find("\"detail\":\"style id 'MissingStyle' was not found in word/styles.xml\""),
             std::string::npos);
    CHECK_NE(missing_json.find("\"entry\":\"word/styles.xml\""), std::string::npos);

    remove_if_exists(source);
    remove_if_exists(style_output);
    remove_if_exists(missing_output);
}

TEST_CASE("cli inspect-table-style reports typed definition json") {
    const fs::path working_directory = fs::current_path();
    const fs::path source = working_directory / "cli_table_style_definition_source.docx";
    const fs::path json_output = working_directory / "cli_table_style_definition.json";
    const fs::path text_output = working_directory / "cli_table_style_definition.txt";
    const fs::path error_output = working_directory / "cli_table_style_definition_error.json";

    remove_if_exists(source);
    remove_if_exists(json_output);
    remove_if_exists(text_output);
    remove_if_exists(error_output);

    {
        featherdoc::Document document(source);
        REQUIRE_FALSE(document.create_empty());

        auto whole_margins = featherdoc::table_style_margins_definition{};
        whole_margins.left_twips = 120U;
        whole_margins.right_twips = 160U;
        auto whole_borders = featherdoc::table_style_borders_definition{};
        whole_borders.top = featherdoc::border_definition{
            featherdoc::border_style::single, 12U, "4472C4", 0U};
        auto whole_table = featherdoc::table_style_region_definition{};
        whole_table.fill_color = std::string{"DDEEFF"};
        whole_table.text_color = std::string{"1F1F1F"};
        whole_table.bold = false;
        whole_table.italic = false;
        whole_table.font_size_points = 11U;
        whole_table.font_family = std::string{"Aptos"};
        whole_table.east_asia_font_family = std::string{"Microsoft YaHei"};
        whole_table.cell_vertical_alignment = featherdoc::cell_vertical_alignment::center;
        whole_table.cell_text_direction =
            featherdoc::cell_text_direction::left_to_right_top_to_bottom;
        whole_table.paragraph_alignment = featherdoc::paragraph_alignment::center;
        auto whole_paragraph_spacing =
            featherdoc::table_style_paragraph_spacing_definition{};
        whole_paragraph_spacing.before_twips = 120U;
        whole_paragraph_spacing.after_twips = 80U;
        whole_paragraph_spacing.line_twips = 360U;
        whole_paragraph_spacing.line_rule =
            featherdoc::paragraph_line_spacing_rule::exact;
        whole_table.paragraph_spacing = whole_paragraph_spacing;
        whole_table.cell_margins = whole_margins;
        whole_table.borders = whole_borders;

        auto first_row_borders = featherdoc::table_style_borders_definition{};
        first_row_borders.bottom = featherdoc::border_definition{
            featherdoc::border_style::double_line, 8U, "1F4E79", 0U};
        auto first_row = featherdoc::table_style_region_definition{};
        first_row.fill_color = std::string{"1F4E79"};
        first_row.text_color = std::string{"FFFFFF"};
        first_row.bold = true;
        first_row.italic = true;
        first_row.font_size_points = 14U;
        first_row.font_family = std::string{"Aptos Display"};
        first_row.east_asia_font_family = std::string{"SimHei"};
        first_row.cell_vertical_alignment = featherdoc::cell_vertical_alignment::bottom;
        first_row.cell_text_direction =
            featherdoc::cell_text_direction::top_to_bottom_right_to_left;
        first_row.paragraph_alignment = featherdoc::paragraph_alignment::right;
        auto first_row_paragraph_spacing =
            featherdoc::table_style_paragraph_spacing_definition{};
        first_row_paragraph_spacing.after_twips = 120U;
        first_row_paragraph_spacing.line_twips = 240U;
        first_row_paragraph_spacing.line_rule =
            featherdoc::paragraph_line_spacing_rule::at_least;
        first_row.paragraph_spacing = first_row_paragraph_spacing;
        first_row.borders = first_row_borders;

        auto second_banded_rows = featherdoc::table_style_region_definition{};
        second_banded_rows.fill_color = std::string{"F2F2F2"};
        second_banded_rows.text_color = std::string{"666666"};

        auto second_banded_columns = featherdoc::table_style_region_definition{};
        second_banded_columns.fill_color = std::string{"E2F0D9"};
        second_banded_columns.cell_vertical_alignment =
            featherdoc::cell_vertical_alignment::top;

        auto definition = featherdoc::table_style_definition{};
        definition.name = "Invoice Grid";
        definition.based_on = std::string{"TableGrid"};
        definition.is_quick_format = true;
        definition.whole_table = whole_table;
        definition.first_row = first_row;
        definition.second_banded_rows = second_banded_rows;
        definition.second_banded_columns = second_banded_columns;

        REQUIRE(document.ensure_table_style("InvoiceGrid", definition));
        REQUIRE_FALSE(document.save());
    }

    CHECK_EQ(run_cli({"inspect-table-style", source.string(), "InvoiceGrid", "--json"},
                     json_output),
             0);
    const auto json = read_text_file(json_output);
    CHECK_NE(json.find(R"("table_style_definition":{"style":{"style_id":"InvoiceGrid")"),
             std::string::npos);
    CHECK_NE(json.find(R"("whole_table":{"fill_color":"DDEEFF","text_color":"1F1F1F","bold":false,"italic":false,"font_size_points":11,"font_family":"Aptos","east_asia_font_family":"Microsoft YaHei","cell_vertical_alignment":"center","cell_text_direction":"left_to_right_top_to_bottom","paragraph_alignment":"center")"),
             std::string::npos);
    CHECK_NE(json.find(R"("paragraph_spacing":{"before_twips":120,"after_twips":80,"line_twips":360,"line_rule":"exact"})"),
             std::string::npos);
    CHECK_NE(json.find(R"("left_twips":120)"), std::string::npos);
    CHECK_NE(json.find(R"("top":{"style":"single","size_eighth_points":12,"color":"4472C4","space_points":0})"),
             std::string::npos);
    CHECK_NE(json.find(R"("first_row":{"fill_color":"1F4E79","text_color":"FFFFFF","bold":true,"italic":true,"font_size_points":14,"font_family":"Aptos Display","east_asia_font_family":"SimHei","cell_vertical_alignment":"bottom","cell_text_direction":"top_to_bottom_right_to_left","paragraph_alignment":"right")"),
             std::string::npos);
    CHECK_NE(json.find(R"("paragraph_spacing":{"before_twips":null,"after_twips":120,"line_twips":240,"line_rule":"at_least"})"),
             std::string::npos);
    CHECK_NE(json.find(R"("bottom":{"style":"double","size_eighth_points":8,"color":"1F4E79","space_points":0})"),
             std::string::npos);
    CHECK_NE(json.find(R"("second_banded_rows":{"fill_color":"F2F2F2","text_color":"666666")"),
             std::string::npos);
    CHECK_NE(json.find(R"("second_banded_columns":{"fill_color":"E2F0D9")"),
             std::string::npos);

    CHECK_EQ(run_cli({"inspect-table-style", source.string(), "InvoiceGrid"}, text_output), 0);
    const auto text = read_text_file(text_output);
    CHECK_NE(text.find("style_id: InvoiceGrid"), std::string::npos);
    CHECK_NE(text.find("region_whole_table: fill_color=DDEEFF text_color=1F1F1F bold=false italic=false font_size_points=11 font_family=Aptos east_asia_font_family=Microsoft YaHei cell_vertical_alignment=center cell_text_direction=left_to_right_top_to_bottom paragraph_alignment=center"),
             std::string::npos);
    CHECK_NE(text.find("paragraph_spacing_before_twips=120 paragraph_spacing_after_twips=80 paragraph_spacing_line_twips=360 paragraph_spacing_line_rule=exact"),
             std::string::npos);
    CHECK_NE(text.find("region_first_row: fill_color=1F4E79 text_color=FFFFFF bold=true italic=true font_size_points=14 font_family=Aptos Display east_asia_font_family=SimHei cell_vertical_alignment=bottom cell_text_direction=top_to_bottom_right_to_left paragraph_alignment=right"),
             std::string::npos);
    CHECK_NE(text.find("paragraph_spacing_before_twips=- paragraph_spacing_after_twips=120 paragraph_spacing_line_twips=240 paragraph_spacing_line_rule=at_least"),
             std::string::npos);
    CHECK_NE(text.find("region_second_banded_rows: fill_color=F2F2F2 text_color=666666"),
             std::string::npos);
    CHECK_NE(text.find("region_second_banded_columns: fill_color=E2F0D9"),
             std::string::npos);
    CHECK_NE(text.find("border_top: style=single"), std::string::npos);

    CHECK_EQ(run_cli({"inspect-table-style", source.string(), "Normal", "--json"},
                     error_output),
             1);
    CHECK_NE(read_text_file(error_output).find("is not a table style"), std::string::npos);

    remove_if_exists(source);
    remove_if_exists(json_output);
    remove_if_exists(text_output);
    remove_if_exists(error_output);
}

TEST_CASE("cli inspect-styles reports style usage for a single style") {
    const fs::path working_directory = fs::current_path();
    const fs::path source = working_directory / "cli_styles_usage_source.docx";
    const fs::path json_output = working_directory / "cli_styles_usage.json";
    const fs::path text_output = working_directory / "cli_styles_usage.txt";
    const fs::path table_output = working_directory / "cli_styles_usage_table.txt";

    remove_if_exists(source);
    remove_if_exists(json_output);
    remove_if_exists(text_output);
    remove_if_exists(table_output);

    create_cli_style_usage_fixture(source);

    CHECK_EQ(run_cli({"inspect-styles", source.string(), "--style", "CustomBody",
                      "--usage", "--json"},
                     json_output),
             0);
    CHECK_EQ(
        read_text_file(json_output),
        std::string{
            "{\"style\":{\"style_id\":\"CustomBody\",\"name\":\"Custom Body\","
            "\"based_on\":\"Normal\",\"kind\":\"paragraph\",\"type\":\"paragraph\","
            "\"numbering\":null,"
            "\"is_default\":false,\"is_custom\":true,\"is_semi_hidden\":false,"
            "\"is_unhide_when_used\":false,\"is_quick_format\":true},"
            "\"usage\":{\"style_id\":\"CustomBody\",\"paragraph_count\":3,"
            "\"run_count\":0,\"table_count\":0,\"total_count\":3,"
            "\"body\":{\"paragraph_count\":2,\"run_count\":0,\"table_count\":0,"
            "\"total_count\":2},"
            "\"header\":{\"paragraph_count\":1,\"run_count\":0,\"table_count\":0,"
            "\"total_count\":1},"
            "\"footer\":{\"paragraph_count\":0,\"run_count\":0,\"table_count\":0,"
            "\"total_count\":0},\"hits\":["
            "{\"part\":\"body\",\"entry_name\":\"word/document.xml\","
            "\"section_index\":0,\"ordinal\":1,\"node_ordinal\":1,\"kind\":\"paragraph\",\"references\":[]},"
            "{\"part\":\"body\",\"entry_name\":\"word/document.xml\","
            "\"section_index\":0,\"ordinal\":2,\"node_ordinal\":2,\"kind\":\"paragraph\",\"references\":[]},"
            "{\"part\":\"header\",\"entry_name\":\"word/header1.xml\","
            "\"section_index\":null,\"ordinal\":1,\"node_ordinal\":1,\"kind\":\"paragraph\",\"references\":["
            "{\"section_index\":0,\"reference_kind\":\"default\"}]}]}}\n"});

    CHECK_EQ(run_cli({"inspect-styles", source.string(), "--style", "Strong",
                      "--usage"},
                     text_output),
             0);
    const auto usage_text = read_text_file(text_output);
    CHECK_NE(usage_text.find("style_id: Strong"), std::string::npos);
    CHECK_NE(usage_text.find("usage_paragraphs: 0"), std::string::npos);
    CHECK_NE(usage_text.find("usage_runs: 3"), std::string::npos);
    CHECK_NE(usage_text.find("usage_tables: 0"), std::string::npos);
    CHECK_NE(usage_text.find("usage_total: 3"), std::string::npos);
    CHECK_NE(usage_text.find("usage_body_runs: 2"), std::string::npos);
    CHECK_NE(usage_text.find("usage_body_total: 2"), std::string::npos);
    CHECK_NE(usage_text.find("usage_header_total: 0"), std::string::npos);
    CHECK_NE(usage_text.find("usage_footer_runs: 1"), std::string::npos);
    CHECK_NE(usage_text.find("usage_footer_total: 1"), std::string::npos);
    CHECK_NE(usage_text.find("usage_hits: 3"), std::string::npos);
    CHECK_NE(usage_text.find(
                 "usage_hit[0]: part=body entry_name=word/document.xml ordinal=1 section_index=0 kind=run references=0"),
             std::string::npos);
    CHECK_NE(usage_text.find(
                 "usage_hit[1]: part=body entry_name=word/document.xml ordinal=2 section_index=0 kind=run references=0"),
             std::string::npos);
    CHECK_NE(usage_text.find(
                 "usage_hit[2]: part=footer entry_name=word/footer1.xml ordinal=1 section_index=- kind=run references=1 ref[0]=section:0,kind:default"),
             std::string::npos);

    CHECK_EQ(run_cli({"inspect-styles", source.string(), "--style", "ReportTable",
                      "--usage"},
                     table_output),
             0);
    const auto table_text = read_text_file(table_output);
    CHECK_NE(table_text.find("style_id: ReportTable"), std::string::npos);
    CHECK_NE(table_text.find("usage_paragraphs: 0"), std::string::npos);
    CHECK_NE(table_text.find("usage_runs: 0"), std::string::npos);
    CHECK_NE(table_text.find("usage_tables: 2"), std::string::npos);
    CHECK_NE(table_text.find("usage_total: 2"), std::string::npos);
    CHECK_NE(table_text.find("usage_body_tables: 1"), std::string::npos);
    CHECK_NE(table_text.find("usage_header_tables: 1"), std::string::npos);
    CHECK_NE(table_text.find("usage_footer_tables: 0"), std::string::npos);
    CHECK_NE(table_text.find("usage_header_total: 1"), std::string::npos);
    CHECK_NE(table_text.find("usage_footer_total: 0"), std::string::npos);
    CHECK_NE(table_text.find("usage_hits: 2"), std::string::npos);
    CHECK_NE(table_text.find(
                 "usage_hit[0]: part=body entry_name=word/document.xml ordinal=1 section_index=0 kind=table references=0"),
             std::string::npos);
    CHECK_NE(table_text.find(
                 "usage_hit[1]: part=header entry_name=word/header1.xml ordinal=1 section_index=- kind=table references=1 ref[0]=section:0,kind:default"),
             std::string::npos);

    remove_if_exists(source);
    remove_if_exists(json_output);
    remove_if_exists(text_output);
    remove_if_exists(table_output);
}


TEST_CASE("cli rename-style rewrites paragraph style ids and references") {
    const fs::path working_directory = fs::current_path();
    const fs::path source = working_directory / "cli_rename_style_source.docx";
    const fs::path renamed = working_directory / "cli_rename_style_renamed.docx";
    const fs::path output = working_directory / "cli_rename_style_output.json";
    const fs::path usage_output = working_directory / "cli_rename_style_usage.json";
    const fs::path conflict_output = working_directory / "cli_rename_style_conflict.json";

    remove_if_exists(source);
    remove_if_exists(renamed);
    remove_if_exists(output);
    remove_if_exists(usage_output);
    remove_if_exists(conflict_output);

    create_cli_style_usage_fixture(source);
    {
        featherdoc::Document document(source);
        REQUIRE_FALSE(document.open());
        featherdoc::paragraph_style_definition child_style;
        child_style.name = "Custom Child";
        child_style.based_on = "CustomBody";
        child_style.next_style = "CustomBody";
        REQUIRE(document.ensure_paragraph_style("CustomChild", child_style));
        REQUIRE_FALSE(document.save());
    }

    CHECK_EQ(run_cli({"rename-style",
                      source.string(),
                      "CustomBody",
                      "RenamedBody",
                      "--output",
                      renamed.string(),
                      "--json"},
                     output),
             0);
    CHECK_EQ(
        read_text_file(output),
        std::string{
            R"({"command":"rename-style","ok":true,)"
            R"("in_place":false,"sections":1,"headers":1,"footers":1,)"
            R"("old_style_id":"CustomBody","new_style_id":"RenamedBody",)"
            R"("style":{"style_id":"RenamedBody","name":"Custom Body",)"
            R"("based_on":"Normal","kind":"paragraph","type":"paragraph",)"
            R"("numbering":null,"is_default":false,"is_custom":true,)"
            R"("is_semi_hidden":false,"is_unhide_when_used":false,)"
            R"("is_quick_format":true}})"
            "\n"});

    CHECK_EQ(run_cli({"inspect-styles", renamed.string(), "--style", "RenamedBody",
                      "--usage", "--json"},
                     usage_output),
             0);
    const auto usage_json = read_text_file(usage_output);
    CHECK_NE(usage_json.find(R"("style_id":"RenamedBody")"), std::string::npos);
    CHECK_NE(usage_json.find(R"("paragraph_count":3)"), std::string::npos);
    CHECK_NE(usage_json.find(R"("body":{"paragraph_count":2)"),
             std::string::npos);
    CHECK_NE(usage_json.find(R"("header":{"paragraph_count":1)"),
             std::string::npos);

    const auto styles_xml = read_docx_entry(renamed, "word/styles.xml");
    pugi::xml_document styles_document;
    REQUIRE(styles_document.load_string(styles_xml.c_str()));
    const auto styles_root = styles_document.child("w:styles");
    CHECK(find_style_xml_node(styles_root, "CustomBody") == pugi::xml_node{});
    CHECK(find_style_xml_node(styles_root, "RenamedBody") != pugi::xml_node{});
    const auto child_style = find_style_xml_node(styles_root, "CustomChild");
    REQUIRE(child_style != pugi::xml_node{});
    CHECK_EQ(std::string_view{child_style.child("w:basedOn").attribute("w:val").value()},
             "RenamedBody");
    CHECK_EQ(std::string_view{child_style.child("w:next").attribute("w:val").value()},
             "RenamedBody");

    const auto document_xml = read_docx_entry(renamed, "word/document.xml");
    CHECK_NE(document_xml.find(R"(w:pStyle w:val="RenamedBody")"),
             std::string::npos);
    CHECK_EQ(document_xml.find(R"(w:pStyle w:val="CustomBody")"),
             std::string::npos);
    const auto header_xml = read_docx_entry(renamed, "word/header1.xml");
    CHECK_NE(header_xml.find(R"(w:pStyle w:val="RenamedBody")"),
             std::string::npos);
    CHECK_EQ(header_xml.find(R"(w:pStyle w:val="CustomBody")"),
             std::string::npos);

    CHECK_EQ(run_cli({"rename-style", renamed.string(), "RenamedBody", "Strong",
                      "--json"},
                     conflict_output),
             1);
    const auto conflict_json = read_text_file(conflict_output);
    CHECK_NE(conflict_json.find(R"("command":"rename-style")"),
             std::string::npos);
    CHECK_NE(conflict_json.find(R"("stage":"mutate")"), std::string::npos);
    CHECK_NE(conflict_json.find(
                 R"("detail":"target style id 'Strong' already exists in word/styles.xml")"),
             std::string::npos);

    remove_if_exists(source);
    remove_if_exists(renamed);
    remove_if_exists(output);
    remove_if_exists(usage_output);
    remove_if_exists(conflict_output);
}

TEST_CASE("cli rename-style rewrites run and table style references") {
    const fs::path working_directory = fs::current_path();
    const fs::path source = working_directory / "cli_rename_style_refs_source.docx";
    const fs::path run_renamed = working_directory / "cli_rename_style_run.docx";
    const fs::path table_renamed = working_directory / "cli_rename_style_table.docx";
    const fs::path run_output = working_directory / "cli_rename_style_run.json";
    const fs::path table_output = working_directory / "cli_rename_style_table.json";
    const fs::path run_usage_output = working_directory / "cli_rename_style_run_usage.json";
    const fs::path table_usage_output = working_directory / "cli_rename_style_table_usage.json";

    remove_if_exists(source);
    remove_if_exists(run_renamed);
    remove_if_exists(table_renamed);
    remove_if_exists(run_output);
    remove_if_exists(table_output);
    remove_if_exists(run_usage_output);
    remove_if_exists(table_usage_output);

    create_cli_style_usage_fixture(source);

    CHECK_EQ(run_cli({"rename-style",
                      source.string(),
                      "Strong",
                      "StrongAccent",
                      "--output",
                      run_renamed.string(),
                      "--json"},
                     run_output),
             0);
    const auto run_json = read_text_file(run_output);
    CHECK_NE(run_json.find(R"("old_style_id":"Strong")"), std::string::npos);
    CHECK_NE(run_json.find(R"("new_style_id":"StrongAccent")"),
             std::string::npos);

    const auto run_document_xml = read_docx_entry(run_renamed, "word/document.xml");
    CHECK_NE(run_document_xml.find(R"(w:rStyle w:val="StrongAccent")"),
             std::string::npos);
    CHECK_EQ(run_document_xml.find(R"(w:rStyle w:val="Strong")"),
             std::string::npos);
    const auto footer_xml = read_docx_entry(run_renamed, "word/footer1.xml");
    CHECK_NE(footer_xml.find(R"(w:rStyle w:val="StrongAccent")"),
             std::string::npos);
    CHECK_EQ(footer_xml.find(R"(w:rStyle w:val="Strong")"), std::string::npos);

    CHECK_EQ(run_cli({"inspect-styles", run_renamed.string(), "--style",
                      "StrongAccent", "--usage", "--json"},
                     run_usage_output),
             0);
    const auto run_usage_json = read_text_file(run_usage_output);
    CHECK_NE(run_usage_json.find(R"("style_id":"StrongAccent")"),
             std::string::npos);
    CHECK_NE(run_usage_json.find(R"("run_count":3)"), std::string::npos);

    CHECK_EQ(run_cli({"rename-style",
                      source.string(),
                      "ReportTable",
                      "ReportTableRenamed",
                      "--output",
                      table_renamed.string(),
                      "--json"},
                     table_output),
             0);
    const auto table_json = read_text_file(table_output);
    CHECK_NE(table_json.find(R"("old_style_id":"ReportTable")"),
             std::string::npos);
    CHECK_NE(table_json.find(R"("new_style_id":"ReportTableRenamed")"),
             std::string::npos);

    const auto table_document_xml = read_docx_entry(table_renamed, "word/document.xml");
    CHECK_NE(table_document_xml.find(R"(w:tblStyle w:val="ReportTableRenamed")"),
             std::string::npos);
    CHECK_EQ(table_document_xml.find(R"(w:tblStyle w:val="ReportTable")"),
             std::string::npos);
    const auto header_xml = read_docx_entry(table_renamed, "word/header1.xml");
    CHECK_NE(header_xml.find(R"(w:tblStyle w:val="ReportTableRenamed")"),
             std::string::npos);
    CHECK_EQ(header_xml.find(R"(w:tblStyle w:val="ReportTable")"),
             std::string::npos);

    CHECK_EQ(run_cli({"inspect-styles", table_renamed.string(), "--style",
                      "ReportTableRenamed", "--usage", "--json"},
                     table_usage_output),
             0);
    const auto table_usage_json = read_text_file(table_usage_output);
    CHECK_NE(table_usage_json.find(R"("style_id":"ReportTableRenamed")"),
             std::string::npos);
    CHECK_NE(table_usage_json.find(R"("table_count":2)"), std::string::npos);

    remove_if_exists(source);
    remove_if_exists(run_renamed);
    remove_if_exists(table_renamed);
    remove_if_exists(run_output);
    remove_if_exists(table_output);
    remove_if_exists(run_usage_output);
    remove_if_exists(table_usage_output);
}

TEST_CASE("cli merge-style rewrites paragraph references and removes source style") {
    const fs::path working_directory = fs::current_path();
    const fs::path source = working_directory / "cli_merge_style_source.docx";
    const fs::path merged = working_directory / "cli_merge_style_merged.docx";
    const fs::path output = working_directory / "cli_merge_style_output.json";
    const fs::path usage_output = working_directory / "cli_merge_style_usage.json";
    const fs::path missing_output = working_directory / "cli_merge_style_missing.json";
    const fs::path mismatch_output = working_directory / "cli_merge_style_mismatch.json";

    remove_if_exists(source);
    remove_if_exists(merged);
    remove_if_exists(output);
    remove_if_exists(usage_output);
    remove_if_exists(missing_output);
    remove_if_exists(mismatch_output);

    create_cli_style_usage_fixture(source);
    {
        featherdoc::Document document(source);
        REQUIRE_FALSE(document.open());
        featherdoc::paragraph_style_definition child_style;
        child_style.name = "Custom Child";
        child_style.based_on = "CustomBody";
        child_style.next_style = "CustomBody";
        REQUIRE(document.ensure_paragraph_style("CustomChild", child_style));
        REQUIRE_FALSE(document.save());
    }

    CHECK_EQ(run_cli({"merge-style", source.string(), "Strong", "Normal", "--json"},
                     mismatch_output),
             1);
    const auto mismatch_json = read_text_file(mismatch_output);
    CHECK_NE(mismatch_json.find(R"("command":"merge-style")"), std::string::npos);
    CHECK_NE(mismatch_json.find(R"("stage":"mutate")"), std::string::npos);
    CHECK_NE(mismatch_json.find(
                 R"("detail":"source style id 'Strong' has type 'character' but target style id 'Normal' has type 'paragraph'")"),
             std::string::npos);

    CHECK_EQ(run_cli({"merge-style",
                      source.string(),
                      "CustomBody",
                      "Normal",
                      "--output",
                      merged.string(),
                      "--json"},
                     output),
             0);
    const auto merge_json = read_text_file(output);
    CHECK_NE(merge_json.find(R"("command":"merge-style")"), std::string::npos);
    CHECK_NE(merge_json.find(R"("source_style_id":"CustomBody")"),
             std::string::npos);
    CHECK_NE(merge_json.find(R"("target_style_id":"Normal")"), std::string::npos);
    CHECK_NE(merge_json.find(R"("style":{"style_id":"Normal")"),
             std::string::npos);

    CHECK_EQ(run_cli({"inspect-styles", merged.string(), "--style", "CustomBody",
                      "--json"},
                     missing_output),
             1);
    const auto missing_json = read_text_file(missing_output);
    CHECK_NE(missing_json.find(R"("command":"inspect-styles")"),
             std::string::npos);
    CHECK_NE(missing_json.find(
                 R"("detail":"style id 'CustomBody' was not found in word/styles.xml")"),
             std::string::npos);

    CHECK_EQ(run_cli({"inspect-styles", merged.string(), "--style", "Normal",
                      "--usage", "--json"},
                     usage_output),
             0);
    const auto usage_json = read_text_file(usage_output);
    CHECK_NE(usage_json.find(R"("style_id":"Normal")"), std::string::npos);
    CHECK_NE(usage_json.find(R"("paragraph_count":3)"), std::string::npos);

    const auto styles_xml = read_docx_entry(merged, "word/styles.xml");
    pugi::xml_document styles_document;
    REQUIRE(styles_document.load_string(styles_xml.c_str()));
    const auto styles_root = styles_document.child("w:styles");
    CHECK(find_style_xml_node(styles_root, "CustomBody") == pugi::xml_node{});
    CHECK(find_style_xml_node(styles_root, "Normal") != pugi::xml_node{});
    const auto child_style = find_style_xml_node(styles_root, "CustomChild");
    REQUIRE(child_style != pugi::xml_node{});
    CHECK_EQ(std::string_view{child_style.child("w:basedOn").attribute("w:val").value()},
             "Normal");
    CHECK_EQ(std::string_view{child_style.child("w:next").attribute("w:val").value()},
             "Normal");

    const auto document_xml = read_docx_entry(merged, "word/document.xml");
    CHECK_NE(document_xml.find(R"(w:pStyle w:val="Normal")"), std::string::npos);
    CHECK_EQ(document_xml.find(R"(w:pStyle w:val="CustomBody")"),
             std::string::npos);
    const auto header_xml = read_docx_entry(merged, "word/header1.xml");
    CHECK_NE(header_xml.find(R"(w:pStyle w:val="Normal")"), std::string::npos);
    CHECK_EQ(header_xml.find(R"(w:pStyle w:val="CustomBody")"),
             std::string::npos);

    remove_if_exists(source);
    remove_if_exists(merged);
    remove_if_exists(output);
    remove_if_exists(usage_output);
    remove_if_exists(missing_output);
    remove_if_exists(mismatch_output);
}

TEST_CASE("cli merge-style rewrites run and table style references") {
    const fs::path working_directory = fs::current_path();
    const fs::path source = working_directory / "cli_merge_style_refs_source.docx";
    const fs::path run_merged = working_directory / "cli_merge_style_run.docx";
    const fs::path table_merged = working_directory / "cli_merge_style_table.docx";
    const fs::path run_output = working_directory / "cli_merge_style_run.json";
    const fs::path table_output = working_directory / "cli_merge_style_table.json";
    const fs::path run_usage_output = working_directory / "cli_merge_style_run_usage.json";
    const fs::path table_usage_output = working_directory / "cli_merge_style_table_usage.json";

    remove_if_exists(source);
    remove_if_exists(run_merged);
    remove_if_exists(table_merged);
    remove_if_exists(run_output);
    remove_if_exists(table_output);
    remove_if_exists(run_usage_output);
    remove_if_exists(table_usage_output);

    create_cli_style_usage_fixture(source);
    {
        featherdoc::Document document(source);
        REQUIRE_FALSE(document.open());
        featherdoc::character_style_definition character_style;
        character_style.name = "Strong Target";
        REQUIRE(document.ensure_character_style("StrongTarget", character_style));
        featherdoc::table_style_definition table_style;
        table_style.name = "Report Table Target";
        REQUIRE(document.ensure_table_style("ReportTableTarget", table_style));
        REQUIRE_FALSE(document.save());
    }

    CHECK_EQ(run_cli({"merge-style",
                      source.string(),
                      "Strong",
                      "StrongTarget",
                      "--output",
                      run_merged.string(),
                      "--json"},
                     run_output),
             0);
    const auto run_json = read_text_file(run_output);
    CHECK_NE(run_json.find(R"("source_style_id":"Strong")"), std::string::npos);
    CHECK_NE(run_json.find(R"("target_style_id":"StrongTarget")"),
             std::string::npos);

    const auto run_document_xml = read_docx_entry(run_merged, "word/document.xml");
    CHECK_NE(run_document_xml.find(R"(w:rStyle w:val="StrongTarget")"),
             std::string::npos);
    CHECK_EQ(run_document_xml.find(R"(w:rStyle w:val="Strong")"),
             std::string::npos);
    const auto footer_xml = read_docx_entry(run_merged, "word/footer1.xml");
    CHECK_NE(footer_xml.find(R"(w:rStyle w:val="StrongTarget")"),
             std::string::npos);
    CHECK_EQ(footer_xml.find(R"(w:rStyle w:val="Strong")"), std::string::npos);

    CHECK_EQ(run_cli({"inspect-styles", run_merged.string(), "--style",
                      "StrongTarget", "--usage", "--json"},
                     run_usage_output),
             0);
    const auto run_usage_json = read_text_file(run_usage_output);
    CHECK_NE(run_usage_json.find(R"("style_id":"StrongTarget")"),
             std::string::npos);
    CHECK_NE(run_usage_json.find(R"("run_count":3)"), std::string::npos);

    CHECK_EQ(run_cli({"merge-style",
                      source.string(),
                      "ReportTable",
                      "ReportTableTarget",
                      "--output",
                      table_merged.string(),
                      "--json"},
                     table_output),
             0);
    const auto table_json = read_text_file(table_output);
    CHECK_NE(table_json.find(R"("source_style_id":"ReportTable")"),
             std::string::npos);
    CHECK_NE(table_json.find(R"("target_style_id":"ReportTableTarget")"),
             std::string::npos);

    const auto table_document_xml = read_docx_entry(table_merged, "word/document.xml");
    CHECK_NE(table_document_xml.find(R"(w:tblStyle w:val="ReportTableTarget")"),
             std::string::npos);
    CHECK_EQ(table_document_xml.find(R"(w:tblStyle w:val="ReportTable")"),
             std::string::npos);
    const auto header_xml = read_docx_entry(table_merged, "word/header1.xml");
    CHECK_NE(header_xml.find(R"(w:tblStyle w:val="ReportTableTarget")"),
             std::string::npos);
    CHECK_EQ(header_xml.find(R"(w:tblStyle w:val="ReportTable")"),
             std::string::npos);

    CHECK_EQ(run_cli({"inspect-styles", table_merged.string(), "--style",
                      "ReportTableTarget", "--usage", "--json"},
                     table_usage_output),
             0);
    const auto table_usage_json = read_text_file(table_usage_output);
    CHECK_NE(table_usage_json.find(R"("style_id":"ReportTableTarget")"),
             std::string::npos);
    CHECK_NE(table_usage_json.find(R"("table_count":2)"), std::string::npos);

    remove_if_exists(source);
    remove_if_exists(run_merged);
    remove_if_exists(table_merged);
    remove_if_exists(run_output);
    remove_if_exists(table_output);
    remove_if_exists(run_usage_output);
    remove_if_exists(table_usage_output);
}

TEST_CASE("cli plan-style-refactor validates batch rename and merge operations") {
    const fs::path working_directory = fs::current_path();
    const fs::path source = working_directory / "cli_style_refactor_plan_source.docx";
    const fs::path clean_output = working_directory / "cli_style_refactor_plan_clean.json";
    const fs::path dirty_output = working_directory / "cli_style_refactor_plan_dirty.json";
    const fs::path clean_plan_file =
        working_directory / "cli_style_refactor_plan_clean.plan.json";
    const fs::path parse_output = working_directory / "cli_style_refactor_plan_parse.json";

    remove_if_exists(source);
    remove_if_exists(clean_output);
    remove_if_exists(dirty_output);
    remove_if_exists(clean_plan_file);
    remove_if_exists(parse_output);

    create_cli_style_usage_fixture(source);

    CHECK_EQ(run_cli({"plan-style-refactor",
                      source.string(),
                      "--rename",
                      "CustomBody:ReviewBody",
                      "--merge",
                      "CustomBody:Normal",
                      "--output-plan",
                      clean_plan_file.string(),
                      "--json"},
                     clean_output),
             0);
    const auto clean_json = read_text_file(clean_output);
    CHECK_NE(clean_json.find(R"("command":"plan-style-refactor")"),
             std::string::npos);
    CHECK_NE(clean_json.find(R"("clean":true)"), std::string::npos);
    CHECK_NE(clean_json.find(R"("operation_count":2)"), std::string::npos);
    CHECK_NE(clean_json.find(R"("applyable_count":2)"), std::string::npos);
    CHECK_NE(clean_json.find(R"("source_reference_count":3)"), std::string::npos);
    CHECK_NE(clean_json.find(
                 R"("command_template":"featherdoc_cli rename-style <input.docx> CustomBody ReviewBody --output <output.docx> --json")"),
             std::string::npos);
    CHECK_NE(clean_json.find(
                 R"("command_template":"featherdoc_cli merge-style <input.docx> CustomBody Normal --output <output.docx> --json")"),
             std::string::npos);
    const auto clean_plan_json = read_text_file(clean_plan_file);
    CHECK_NE(clean_plan_json.find(R"("command":"plan-style-refactor")"),
             std::string::npos);
    CHECK_NE(clean_plan_json.find(R"("operations":[)"), std::string::npos);

    CHECK_EQ(run_cli({"plan-style-refactor",
                      source.string(),
                      "--merge",
                      "Strong:Normal",
                      "--rename",
                      "MissingBody:NewBody",
                      "--rename",
                      "CustomBody:Strong",
                      "--json"},
                     dirty_output),
             1);
    const auto dirty_json = read_text_file(dirty_output);
    CHECK_NE(dirty_json.find(R"("clean":false)"), std::string::npos);
    CHECK_NE(dirty_json.find(R"("operation_count":3)"), std::string::npos);
    CHECK_NE(dirty_json.find(R"("applyable_count":0)"), std::string::npos);
    CHECK_NE(dirty_json.find(R"("issue_count":3)"), std::string::npos);
    CHECK_NE(dirty_json.find(R"("code":"style_type_mismatch")"), std::string::npos);
    CHECK_NE(dirty_json.find(R"("code":"missing_source_style")"), std::string::npos);
    CHECK_NE(dirty_json.find(R"("code":"target_style_exists")"), std::string::npos);

    const auto source_styles_xml = read_docx_entry(source, "word/styles.xml");
    CHECK_NE(source_styles_xml.find(R"(w:styleId="CustomBody")"), std::string::npos);
    CHECK_EQ(source_styles_xml.find(R"(w:styleId="ReviewBody")"), std::string::npos);

    CHECK_EQ(run_cli({"plan-style-refactor", source.string(), "--json"}, parse_output), 2);
    CHECK_NE(read_text_file(parse_output)
                 .find("plan-style-refactor expects at least one --rename or --merge option"),
             std::string::npos);

    remove_if_exists(source);
    remove_if_exists(clean_output);
    remove_if_exists(dirty_output);
    remove_if_exists(clean_plan_file);
    remove_if_exists(parse_output);
}

TEST_CASE("cli suggest-style-merges writes reviewable duplicate merge plan") {
    const fs::path working_directory = fs::current_path();
    const fs::path source = working_directory / "cli_suggest_style_merges_source.docx";
    const fs::path output = working_directory / "cli_suggest_style_merges_output.json";
    const fs::path filtered_output =
        working_directory / "cli_suggest_style_merges_filtered.json";
    const fs::path gate_output =
        working_directory / "cli_suggest_style_merges_gate.json";
    const fs::path filtered_gate_output =
        working_directory / "cli_suggest_style_merges_filtered_gate.json";
    const fs::path plan_file =
        working_directory / "cli_suggest_style_merges_plan.json";
    const fs::path filtered_plan_file =
        working_directory / "cli_suggest_style_merges_filtered.plan.json";
    const fs::path applied = working_directory / "cli_suggest_style_merges_applied.docx";
    const fs::path apply_output =
        working_directory / "cli_suggest_style_merges_apply.json";
    const fs::path rollback_file =
        working_directory / "cli_suggest_style_merges_rollback.json";
    const fs::path restored =
        working_directory / "cli_suggest_style_merges_restored.docx";
    const fs::path restore_output =
        working_directory / "cli_suggest_style_merges_restore.json";
    const fs::path dry_run_output =
        working_directory / "cli_suggest_style_merges_restore_dry_run.json";
    const fs::path parse_output =
        working_directory / "cli_suggest_style_merges_parse.json";

    remove_if_exists(source);
    remove_if_exists(output);
    remove_if_exists(filtered_output);
    remove_if_exists(gate_output);
    remove_if_exists(filtered_gate_output);
    remove_if_exists(plan_file);
    remove_if_exists(filtered_plan_file);
    remove_if_exists(applied);
    remove_if_exists(apply_output);
    remove_if_exists(rollback_file);
    remove_if_exists(restored);
    remove_if_exists(restore_output);
    remove_if_exists(dry_run_output);
    remove_if_exists(parse_output);

    create_cli_duplicate_style_fixture(source);

    CHECK_EQ(run_cli({"suggest-style-merges",
                      source.string(),
                      "--output-plan",
                      plan_file.string(),
                      "--json"},
                     output),
             0);

    const auto suggestion_json = read_text_file(output);
    CHECK_NE(suggestion_json.find(R"("command":"suggest-style-merges")"),
             std::string::npos);
    CHECK_NE(suggestion_json.find(R"("fail_on_suggestion":false)"),
             std::string::npos);
    CHECK_NE(suggestion_json.find(R"("suggestion_gate_failed":false)"),
             std::string::npos);
    CHECK_NE(suggestion_json.find(R"("operation_count":1)"), std::string::npos);
    CHECK_NE(suggestion_json.find(R"("action":"merge")"), std::string::npos);
    CHECK_NE(suggestion_json.find(R"("source_style_id":"DuplicateBodyB")"),
             std::string::npos);
    CHECK_NE(suggestion_json.find(R"("target_style_id":"DuplicateBodyA")"),
             std::string::npos);
    CHECK_NE(suggestion_json.find(
                 R"("reason_code":"matching_style_signature_and_xml")"),
             std::string::npos);
    CHECK_NE(suggestion_json.find(R"("confidence":95)"), std::string::npos);
    CHECK_NE(suggestion_json.find(R"("target_has_more_references")"),
             std::string::npos);
    CHECK_NE(suggestion_json.find(R"("style_definition_xml_matches")"),
             std::string::npos);
    CHECK_NE(suggestion_json.find(R"("differences":[])"), std::string::npos);
    CHECK_NE(suggestion_json.find(
                 R"("suggestion_confidence_summary":{"suggestion_count":1)"),
             std::string::npos);
    CHECK_NE(suggestion_json.find(R"("min_confidence":95)"),
             std::string::npos);
    CHECK_NE(suggestion_json.find(R"("max_confidence":95)"),
             std::string::npos);
    CHECK_NE(suggestion_json.find(R"("exact_xml_match_count":1)"),
             std::string::npos);
    CHECK_NE(suggestion_json.find(R"("xml_difference_count":0)"),
             std::string::npos);
    CHECK_NE(suggestion_json.find(R"("recommended_min_confidence":95)"),
             std::string::npos);
    CHECK_NE(suggestion_json.find("automation gates"), std::string::npos);

    CHECK_EQ(run_cli({"suggest-style-merges",
                      source.string(),
                      "--fail-on-suggestion",
                      "--json"},
                     gate_output),
             1);
    const auto gate_json = read_text_file(gate_output);
    CHECK_NE(gate_json.find(R"("command":"suggest-style-merges")"),
             std::string::npos);
    CHECK_NE(gate_json.find(R"("fail_on_suggestion":true)"),
             std::string::npos);
    CHECK_NE(gate_json.find(R"("suggestion_gate_failed":true)"),
             std::string::npos);
    CHECK_NE(gate_json.find(R"("clean":true)"), std::string::npos);
    CHECK_NE(gate_json.find(R"("operation_count":1)"), std::string::npos);
    CHECK_NE(gate_json.find(R"("source_style_id":"DuplicateBodyB")"),
             std::string::npos);

    const auto plan_json = read_text_file(plan_file);
    CHECK_NE(plan_json.find(R"("command":"suggest-style-merges")"),
             std::string::npos);
    CHECK_NE(plan_json.find(R"("operations":[)"), std::string::npos);
    CHECK_NE(plan_json.find(R"("source_style_id":"DuplicateBodyB")"),
             std::string::npos);
    CHECK_NE(plan_json.find(R"("target_style_id":"DuplicateBodyA")"),
             std::string::npos);
    CHECK_NE(plan_json.find(R"("confidence":95)"), std::string::npos);
    CHECK_NE(plan_json.find(R"("recommended_min_confidence":95)"),
             std::string::npos);
    CHECK_EQ(plan_json.find(R"("fail_on_suggestion")"), std::string::npos);
    CHECK_EQ(plan_json.find(R"("suggestion_gate_failed")"), std::string::npos);

    CHECK_EQ(run_cli({"suggest-style-merges",
                      source.string(),
                      "--min-confidence",
                      "96",
                      "--output-plan",
                      filtered_plan_file.string(),
                      "--json"},
                     filtered_output),
             0);
    const auto filtered_json = read_text_file(filtered_output);
    CHECK_NE(filtered_json.find(R"("command":"suggest-style-merges")"),
             std::string::npos);
    CHECK_NE(filtered_json.find(R"("operation_count":0)"), std::string::npos);
    CHECK_NE(filtered_json.find(R"("applyable_count":0)"), std::string::npos);
    CHECK_NE(filtered_json.find(R"("suggestion_confidence_summary":{"suggestion_count":0)"),
             std::string::npos);
    CHECK_NE(filtered_json.find(R"("min_confidence":null)"),
             std::string::npos);
    CHECK_NE(filtered_json.find(R"("max_confidence":null)"),
             std::string::npos);
    CHECK_NE(filtered_json.find(R"("recommended_min_confidence":null)"),
             std::string::npos);
    CHECK_NE(filtered_json.find(R"("operations":[])"), std::string::npos);
    CHECK_EQ(filtered_json.find(R"("source_style_id":"DuplicateBodyB")"),
             std::string::npos);

    CHECK_EQ(run_cli({"suggest-style-merges",
                      source.string(),
                      "--min-confidence",
                      "96",
                      "--fail-on-suggestion",
                      "--json"},
                     filtered_gate_output),
             0);
    const auto filtered_gate_json = read_text_file(filtered_gate_output);
    CHECK_NE(filtered_gate_json.find(R"("fail_on_suggestion":true)"),
             std::string::npos);
    CHECK_NE(filtered_gate_json.find(R"("suggestion_gate_failed":false)"),
             std::string::npos);
    CHECK_NE(filtered_gate_json.find(R"("operation_count":0)"),
             std::string::npos);
    CHECK_NE(filtered_gate_json.find(R"("operations":[])"), std::string::npos);

    const auto filtered_plan_json = read_text_file(filtered_plan_file);
    CHECK_NE(filtered_plan_json.find(R"("operation_count":0)"),
             std::string::npos);
    CHECK_NE(filtered_plan_json.find(R"("recommended_min_confidence":null)"),
             std::string::npos);
    CHECK_NE(filtered_plan_json.find(R"("operations":[])"), std::string::npos);

    CHECK_EQ(run_cli({"apply-style-refactor",
                      source.string(),
                      "--plan-file",
                      plan_file.string(),
                      "--rollback-plan",
                      rollback_file.string(),
                      "--output",
                      applied.string(),
                      "--json"},
                     apply_output),
             0);
    CHECK_NE(read_text_file(apply_output).find(R"("command":"apply-style-refactor")"),
             std::string::npos);
    const auto suggestion_rollback_json = read_text_file(rollback_file);
    CHECK_NE(suggestion_rollback_json.find(R"("rollback_count":1)"),
             std::string::npos);
    CHECK_NE(suggestion_rollback_json.find(R"("restorable":true)"),
             std::string::npos);
    CHECK_NE(suggestion_rollback_json.find(R"("source_reference_count":1)"),
             std::string::npos);
    CHECK_NE(suggestion_rollback_json.find(R"("source_style_xml":")"),
             std::string::npos);
    CHECK_NE(suggestion_rollback_json.find(R"("node_ordinal":3)"),
             std::string::npos);
    CHECK_NE(suggestion_rollback_json.find(R"(w:styleId=\"DuplicateBodyB\")"),
             std::string::npos);
    {
        featherdoc::Document document(applied);
        REQUIRE_FALSE(document.open());
        CHECK(document.find_style("DuplicateBodyA").has_value());
        CHECK_FALSE(document.find_style("DuplicateBodyB").has_value());
    }

    CHECK_EQ(run_cli({"restore-style-merge",
                      applied.string(),
                      "--rollback-plan",
                      rollback_file.string(),
                      "--entry",
                      "0",
                      "--dry-run",
                      "--json"},
                     dry_run_output),
             0);
    const auto dry_run_json = read_text_file(dry_run_output);
    CHECK_NE(dry_run_json.find(R"("command":"restore-style-merge")"),
             std::string::npos);
    CHECK_NE(dry_run_json.find(R"("dry_run":true)"), std::string::npos);
    CHECK_NE(dry_run_json.find(R"("changed":false)"), std::string::npos);
    CHECK_NE(dry_run_json.find(R"("issue_count":0)"), std::string::npos);
    CHECK_NE(dry_run_json.find(R"("issue_summary":[])"), std::string::npos);
    CHECK_NE(dry_run_json.find(R"("restored_count":1)"), std::string::npos);
    CHECK_NE(dry_run_json.find(R"("restored_style_count":1)"),
             std::string::npos);
    CHECK_NE(dry_run_json.find(R"("restored_reference_count":1)"),
             std::string::npos);
    CHECK_NE(dry_run_json.find(R"("entry_index":0)"), std::string::npos);
    {
        featherdoc::Document document(applied);
        REQUIRE_FALSE(document.open());
        CHECK(document.find_style("DuplicateBodyA").has_value());
        CHECK_FALSE(document.find_style("DuplicateBodyB").has_value());
        const auto target_usage = document.find_style_usage("DuplicateBodyA");
        REQUIRE(target_usage.has_value());
        CHECK_EQ(target_usage->paragraph_count, 3U);
    }

    CHECK_EQ(run_cli({"restore-style-merge",
                      applied.string(),
                      "--rollback-plan",
                      rollback_file.string(),
                      "--entry",
                      "0",
                      "--output",
                      restored.string(),
                      "--json"},
                     restore_output),
             0);
    const auto restore_json = read_text_file(restore_output);
    CHECK_NE(restore_json.find(R"("command":"restore-style-merge")"),
             std::string::npos);
    CHECK_NE(restore_json.find(R"("ok":true)"), std::string::npos);
    CHECK_NE(restore_json.find(R"("restored_count":1)"), std::string::npos);
    CHECK_NE(restore_json.find(R"("restored_style_count":1)"),
             std::string::npos);
    CHECK_NE(restore_json.find(R"("restored_reference_count":1)"),
             std::string::npos);
    {
        featherdoc::Document document(restored);
        REQUIRE_FALSE(document.open());
        CHECK(document.find_style("DuplicateBodyA").has_value());
        CHECK(document.find_style("DuplicateBodyB").has_value());
        const auto target_usage = document.find_style_usage("DuplicateBodyA");
        REQUIRE(target_usage.has_value());
        CHECK_EQ(target_usage->paragraph_count, 2U);
        const auto source_usage = document.find_style_usage("DuplicateBodyB");
        REQUIRE(source_usage.has_value());
        CHECK_EQ(source_usage->paragraph_count, 1U);
    }

    CHECK_EQ(run_cli({"restore-style-merge",
                      restored.string(),
                      "--rollback-plan",
                      rollback_file.string(),
                      "--entry",
                      "0",
                      "--dry-run",
                      "--json"},
                     parse_output),
             1);
    const auto restore_conflict_json = read_text_file(parse_output);
    CHECK_NE(restore_conflict_json.find(R"("ok":false)"), std::string::npos);
    CHECK_NE(restore_conflict_json.find(R"("issue_count":1)"),
             std::string::npos);
    CHECK_NE(restore_conflict_json.find(
                 R"("issue_summary":[{"code":"source_style_exists","count":1)"),
             std::string::npos);
    CHECK_NE(restore_conflict_json.find(R"("code":"source_style_exists")"),
             std::string::npos);
    CHECK_NE(restore_conflict_json.find(R"("suggestion":"The source style already exists;)"),
             std::string::npos);
    CHECK_NE(restore_conflict_json.find("skip this rollback entry"),
             std::string::npos);

    CHECK_EQ(run_cli({"restore-style-merge",
                      applied.string(),
                      "--rollback-plan",
                      rollback_file.string(),
                      "--dry-run",
                      "--output",
                      restored.string(),
                      "--json"},
                     parse_output),
             2);
    CHECK_NE(read_text_file(parse_output)
                 .find("restore-style-merge --dry-run cannot be combined with --output"),
             std::string::npos);

    CHECK_EQ(run_cli({"suggest-style-merges",
                      source.string(),
                      "--min-confidence",
                      "101",
                      "--json"},
                     parse_output),
             2);
    CHECK_NE(read_text_file(parse_output)
                 .find("--min-confidence expects an integer from 0 to 100"),
             std::string::npos);

    CHECK_EQ(run_cli({"suggest-style-merges", source.string(), "--bad", "--json"},
                     parse_output),
             2);
    CHECK_NE(read_text_file(parse_output).find(R"("stage":"parse")"),
             std::string::npos);

    remove_if_exists(source);
    remove_if_exists(output);
    remove_if_exists(filtered_output);
    remove_if_exists(gate_output);
    remove_if_exists(filtered_gate_output);
    remove_if_exists(plan_file);
    remove_if_exists(filtered_plan_file);
    remove_if_exists(applied);
    remove_if_exists(apply_output);
    remove_if_exists(rollback_file);
    remove_if_exists(restored);
    remove_if_exists(restore_output);
    remove_if_exists(dry_run_output);
    remove_if_exists(parse_output);
}

TEST_CASE("cli suggest-style-merges applies confidence profiles") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_suggest_style_merges_profiles_source.docx";
    const fs::path recommended_output =
        working_directory / "cli_suggest_style_merges_profiles_recommended.json";
    const fs::path strict_output =
        working_directory / "cli_suggest_style_merges_profiles_strict.json";
    const fs::path review_output =
        working_directory / "cli_suggest_style_merges_profiles_review.json";
    const fs::path parse_output =
        working_directory / "cli_suggest_style_merges_profiles_parse.json";

    remove_if_exists(source);
    remove_if_exists(recommended_output);
    remove_if_exists(strict_output);
    remove_if_exists(review_output);
    remove_if_exists(parse_output);

    create_cli_duplicate_style_profile_fixture(source);

    CHECK_EQ(run_cli({"suggest-style-merges",
                      source.string(),
                      "--confidence-profile",
                      "recommended",
                      "--json"},
                     recommended_output),
             0);
    const auto recommended_json = read_text_file(recommended_output);
    CHECK_NE(recommended_json.find(R"("operation_count":1)"), std::string::npos);
    CHECK_NE(recommended_json.find(R"("source_style_id":"DuplicateBodyB")"),
             std::string::npos);
    CHECK_EQ(recommended_json.find(R"("source_style_id":"DuplicateBodyC")"),
             std::string::npos);
    CHECK_NE(recommended_json.find(R"("min_confidence":95)"),
             std::string::npos);
    CHECK_NE(recommended_json.find(R"("recommended_min_confidence":95)"),
             std::string::npos);
    CHECK_NE(recommended_json.find("automation gates"), std::string::npos);

    CHECK_EQ(run_cli({"suggest-style-merges",
                      source.string(),
                      "--confidence-profile",
                      "strict",
                      "--json"},
                     strict_output),
             0);
    const auto strict_json = read_text_file(strict_output);
    CHECK_NE(strict_json.find(R"("operation_count":1)"), std::string::npos);
    CHECK_NE(strict_json.find(R"("source_style_id":"DuplicateBodyB")"),
             std::string::npos);
    CHECK_EQ(strict_json.find(R"("source_style_id":"DuplicateBodyC")"),
             std::string::npos);
    CHECK_NE(strict_json.find(R"("min_confidence":95)"), std::string::npos);
    CHECK_NE(strict_json.find(R"("recommended_min_confidence":95)"),
             std::string::npos);
    CHECK_NE(strict_json.find(R"("exact_xml_match_count":1)"),
             std::string::npos);
    CHECK_NE(strict_json.find(R"("xml_difference_count":0)"),
             std::string::npos);

    CHECK_EQ(run_cli({"suggest-style-merges",
                      source.string(),
                      "--confidence-profile",
                      "review",
                      "--json"},
                     review_output),
             0);
    const auto review_json = read_text_file(review_output);
    CHECK_NE(review_json.find(R"("operation_count":2)"), std::string::npos);
    CHECK_NE(review_json.find(R"("source_style_id":"DuplicateBodyB")"),
             std::string::npos);
    CHECK_NE(review_json.find(R"("source_style_id":"DuplicateBodyC")"),
             std::string::npos);
    CHECK_NE(review_json.find(R"("confidence":80)"), std::string::npos);
    CHECK_NE(review_json.find(R"("min_confidence":80)"), std::string::npos);
    CHECK_NE(review_json.find(R"("exact_xml_match_count":1)"),
             std::string::npos);
    CHECK_NE(review_json.find(R"("xml_difference_count":1)"),
             std::string::npos);
    CHECK_NE(review_json.find("review lower-confidence XML differences manually"),
             std::string::npos);

    CHECK_EQ(run_cli({"suggest-style-merges",
                      source.string(),
                      "--confidence-profile",
                      "strict",
                      "--min-confidence",
                      "80",
                      "--json"},
                     parse_output),
             2);
    CHECK_NE(read_text_file(parse_output)
                 .find("--confidence-profile cannot be combined with --min-confidence"),
             std::string::npos);

    CHECK_EQ(run_cli({"suggest-style-merges",
                      source.string(),
                      "--confidence-profile",
                      "unsafe",
                      "--json"},
                     parse_output),
             2);
    CHECK_NE(read_text_file(parse_output)
                 .find("--confidence-profile expects recommended, strict, review, or exploratory"),
             std::string::npos);

    remove_if_exists(source);
    remove_if_exists(recommended_output);
    remove_if_exists(strict_output);
    remove_if_exists(review_output);
    remove_if_exists(parse_output);
}


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
