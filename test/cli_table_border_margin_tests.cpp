#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "cli_table_test_support.hpp"

namespace {
TEST_CASE("cli table default margin and border commands set and clear body table metadata") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_table_default_style_source.docx";
    const fs::path margined =
        working_directory / "cli_table_default_style_margined.docx";
    const fs::path margin_cleared =
        working_directory / "cli_table_default_style_margin_cleared.docx";
    const fs::path bordered =
        working_directory / "cli_table_default_style_bordered.docx";
    const fs::path border_cleared =
        working_directory / "cli_table_default_style_border_cleared.docx";
    const fs::path margin_output =
        working_directory / "cli_table_default_margin_set_output.json";
    const fs::path margin_clear_output =
        working_directory / "cli_table_default_margin_clear_output.json";
    const fs::path border_output =
        working_directory / "cli_table_border_set_output.json";
    const fs::path border_clear_output =
        working_directory / "cli_table_border_clear_output.json";
    const fs::path inspect_output =
        working_directory / "cli_table_default_style_inspect_output.json";
    const fs::path parse_error_output =
        working_directory / "cli_table_border_parse_output.json";

    remove_if_exists(source);
    remove_if_exists(margined);
    remove_if_exists(margin_cleared);
    remove_if_exists(bordered);
    remove_if_exists(border_cleared);
    remove_if_exists(margin_output);
    remove_if_exists(margin_clear_output);
    remove_if_exists(border_output);
    remove_if_exists(border_clear_output);
    remove_if_exists(inspect_output);
    remove_if_exists(parse_error_output);

    create_cli_table_cell_style_fixture(source);

    CHECK_EQ(run_cli({"set-table-default-cell-margin",
                      source.string(),
                      "0",
                      "left",
                      "240",
                      "--output",
                      margined.string(),
                      "--json"},
                     margin_output),
             0);
    CHECK_EQ(
        read_text_file(margin_output),
        std::string{
            "{\"command\":\"set-table-default-cell-margin\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"table_index\":0,\"edge\":\"left\",\"margin_twips\":240}\n"});

    const auto margined_document_xml =
        read_docx_entry(margined, "word/document.xml");
    pugi::xml_document margined_xml_document;
    REQUIRE(margined_xml_document.load_string(margined_document_xml.c_str()));
    const auto table_left_margin =
        margined_xml_document.child("w:document")
            .child("w:body")
            .child("w:tbl")
            .child("w:tblPr")
            .child("w:tblCellMar")
            .child("w:left");
    REQUIRE(table_left_margin != pugi::xml_node{});
    CHECK_EQ(std::string_view{table_left_margin.attribute("w:w").value()},
             "240");
    CHECK_EQ(std::string_view{table_left_margin.attribute("w:type").value()},
             "dxa");

    featherdoc::Document reopened_margined(margined);
    REQUIRE_FALSE(reopened_margined.open());
    const auto margined_table = reopened_margined.inspect_table(0U);
    REQUIRE(margined_table.has_value());
    REQUIRE(margined_table->cell_margin_left_twips.has_value());
    CHECK_EQ(*margined_table->cell_margin_left_twips, 240U);

    CHECK_EQ(run_cli({"inspect-tables", margined.string(), "--table", "0", "--json"},
                     inspect_output),
             0);
    const auto margined_inspect_json = read_text_file(inspect_output);
    CHECK_NE(
        margined_inspect_json.find(
            "\"cell_margins\":{\"top\":null,\"left\":240,"
            "\"bottom\":null,\"right\":null}"),
        std::string::npos);

    CHECK_EQ(run_cli({"clear-table-default-cell-margin",
                      margined.string(),
                      "0",
                      "left",
                      "--output",
                      margin_cleared.string(),
                      "--json"},
                     margin_clear_output),
             0);
    CHECK_EQ(
        read_text_file(margin_clear_output),
        std::string{
            "{\"command\":\"clear-table-default-cell-margin\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"table_index\":0,\"edge\":\"left\"}\n"});

    featherdoc::Document reopened_margin_cleared(margin_cleared);
    REQUIRE_FALSE(reopened_margin_cleared.open());
    const auto margin_cleared_table =
        reopened_margin_cleared.inspect_table(0U);
    REQUIRE(margin_cleared_table.has_value());
    CHECK_FALSE(margin_cleared_table->cell_margin_left_twips.has_value());

    CHECK_EQ(run_cli({"set-table-border",
                      source.string(),
                      "0",
                      "inside_horizontal",
                      "--style",
                      "dashed",
                      "--size",
                      "8",
                      "--color",
                      "00AA00",
                      "--space",
                      "2",
                      "--output",
                      bordered.string(),
                      "--json"},
                     border_output),
             0);
    CHECK_EQ(
        read_text_file(border_output),
        std::string{
            "{\"command\":\"set-table-border\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"table_index\":0,\"edge\":\"inside_horizontal\","
            "\"style\":\"dashed\",\"size_eighth_points\":8,"
            "\"color\":\"00AA00\",\"space_points\":2}\n"});

    const auto bordered_document_xml =
        read_docx_entry(bordered, "word/document.xml");
    pugi::xml_document bordered_xml_document;
    REQUIRE(bordered_xml_document.load_string(bordered_document_xml.c_str()));
    const auto inside_horizontal_border =
        bordered_xml_document.child("w:document")
            .child("w:body")
            .child("w:tbl")
            .child("w:tblPr")
            .child("w:tblBorders")
            .child("w:insideH");
    REQUIRE(inside_horizontal_border != pugi::xml_node{});
    CHECK_EQ(std::string_view{inside_horizontal_border.attribute("w:val").value()},
             "dashed");
    CHECK_EQ(std::string_view{inside_horizontal_border.attribute("w:sz").value()},
             "8");
    CHECK_EQ(
        std::string_view{inside_horizontal_border.attribute("w:space").value()},
        "2");
    CHECK_EQ(
        std::string_view{inside_horizontal_border.attribute("w:color").value()},
        "00AA00");

    featherdoc::Document reopened_bordered(bordered);
    REQUIRE_FALSE(reopened_bordered.open());
    const auto bordered_table = reopened_bordered.inspect_table(0U);
    REQUIRE(bordered_table.has_value());
    REQUIRE(bordered_table->borders.has_value());
    REQUIRE(bordered_table->borders->inside_horizontal.has_value());
    CHECK_EQ(bordered_table->borders->inside_horizontal->style,
             featherdoc::border_style::dashed);
    CHECK_EQ(bordered_table->borders->inside_horizontal->size_eighth_points,
             8U);
    CHECK_EQ(bordered_table->borders->inside_horizontal->color, "00AA00");
    CHECK_EQ(bordered_table->borders->inside_horizontal->space_points, 2U);

    CHECK_EQ(run_cli({"inspect-tables", bordered.string(), "--table", "0", "--json"},
                     inspect_output),
             0);
    const auto bordered_inspect_json = read_text_file(inspect_output);
    CHECK_NE(
        bordered_inspect_json.find(
            "\"inside_horizontal\":{\"style\":\"dashed\","
            "\"size_eighth_points\":8,\"color\":\"00AA00\","
            "\"space_points\":2}"),
        std::string::npos);

    CHECK_EQ(run_cli({"clear-table-border",
                      bordered.string(),
                      "0",
                      "inside_horizontal",
                      "--output",
                      border_cleared.string(),
                      "--json"},
                     border_clear_output),
             0);
    CHECK_EQ(
        read_text_file(border_clear_output),
        std::string{
            "{\"command\":\"clear-table-border\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"table_index\":0,\"edge\":\"inside_horizontal\"}\n"});

    featherdoc::Document reopened_border_cleared(border_cleared);
    REQUIRE_FALSE(reopened_border_cleared.open());
    const auto border_cleared_table =
        reopened_border_cleared.inspect_table(0U);
    REQUIRE(border_cleared_table.has_value());
    CHECK_FALSE(border_cleared_table->borders.has_value());

    CHECK_EQ(run_cli({"set-table-border",
                      source.string(),
                      "0",
                      "diagonal",
                      "--style",
                      "single",
                      "--json"},
                     parse_error_output),
             2);
    CHECK_EQ(
        read_text_file(parse_error_output),
        std::string{
            "{\"command\":\"set-table-border\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"invalid table border edge: "
            "diagonal\"}\n"});

    remove_if_exists(source);
    remove_if_exists(margined);
    remove_if_exists(margin_cleared);
    remove_if_exists(bordered);
    remove_if_exists(border_cleared);
    remove_if_exists(margin_output);
    remove_if_exists(margin_clear_output);
    remove_if_exists(border_output);
    remove_if_exists(border_clear_output);
    remove_if_exists(inspect_output);
    remove_if_exists(parse_error_output);
}

TEST_CASE("cli table cell margin commands set and clear body cell margins") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_table_cell_margin_source.docx";
    const fs::path margined =
        working_directory / "cli_table_cell_margin_margined.docx";
    const fs::path cleared =
        working_directory / "cli_table_cell_margin_cleared.docx";
    const fs::path set_output =
        working_directory / "cli_table_cell_margin_set_output.json";
    const fs::path clear_output =
        working_directory / "cli_table_cell_margin_clear_output.json";

    remove_if_exists(source);
    remove_if_exists(margined);
    remove_if_exists(cleared);
    remove_if_exists(set_output);
    remove_if_exists(clear_output);

    create_cli_table_cell_style_fixture(source);

    CHECK_EQ(run_cli({"set-table-cell-margin",
                      source.string(),
                      "0",
                      "0",
                      "0",
                      "left",
                      "240",
                      "--output",
                      margined.string(),
                      "--json"},
                     set_output),
             0);
    CHECK_EQ(
        read_text_file(set_output),
        std::string{
            "{\"command\":\"set-table-cell-margin\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"table_index\":0,\"row_index\":0,\"cell_index\":0,"
            "\"edge\":\"left\",\"margin_twips\":240}\n"});

    const auto margined_document_xml =
        read_docx_entry(margined, "word/document.xml");
    pugi::xml_document margined_xml_document;
    REQUIRE(margined_xml_document.load_string(margined_document_xml.c_str()));
    const auto margined_cell_node =
        margined_xml_document.child("w:document").child("w:body").child("w:tbl").child("w:tr").child("w:tc");
    REQUIRE(margined_cell_node != pugi::xml_node{});
    const auto left_margin =
        margined_cell_node.child("w:tcPr").child("w:tcMar").child("w:left");
    REQUIRE(left_margin != pugi::xml_node{});
    CHECK_EQ(std::string_view{left_margin.attribute("w:w").value()}, "240");
    CHECK_EQ(std::string_view{left_margin.attribute("w:type").value()}, "dxa");

    featherdoc::Document reopened(margined);
    REQUIRE_FALSE(reopened.open());
    auto reopened_cell = reopened.tables().rows().cells();
    REQUIRE(reopened_cell.has_next());
    REQUIRE(reopened_cell.margin_twips(featherdoc::cell_margin_edge::left).has_value());
    CHECK_EQ(*reopened_cell.margin_twips(featherdoc::cell_margin_edge::left),
             240U);

    CHECK_EQ(run_cli({"clear-table-cell-margin",
                      margined.string(),
                      "0",
                      "0",
                      "0",
                      "left",
                      "--output",
                      cleared.string(),
                      "--json"},
                     clear_output),
             0);
    CHECK_EQ(
        read_text_file(clear_output),
        std::string{
            "{\"command\":\"clear-table-cell-margin\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"table_index\":0,\"row_index\":0,\"cell_index\":0,"
            "\"edge\":\"left\"}\n"});

    const auto cleared_document_xml = read_docx_entry(cleared, "word/document.xml");
    pugi::xml_document cleared_xml_document;
    REQUIRE(cleared_xml_document.load_string(cleared_document_xml.c_str()));
    const auto cleared_cell_node =
        cleared_xml_document.child("w:document").child("w:body").child("w:tbl").child("w:tr").child("w:tc");
    REQUIRE(cleared_cell_node != pugi::xml_node{});
    CHECK(cleared_cell_node.child("w:tcPr").child("w:tcMar") == pugi::xml_node{});

    featherdoc::Document reopened_cleared(cleared);
    REQUIRE_FALSE(reopened_cleared.open());
    auto reopened_cleared_cell = reopened_cleared.tables().rows().cells();
    REQUIRE(reopened_cleared_cell.has_next());
    CHECK_FALSE(reopened_cleared_cell.margin_twips(
        featherdoc::cell_margin_edge::left).has_value());

    remove_if_exists(source);
    remove_if_exists(margined);
    remove_if_exists(cleared);
    remove_if_exists(set_output);
    remove_if_exists(clear_output);
}

TEST_CASE("cli table cell border commands set and clear body cell borders") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_table_cell_border_source.docx";
    const fs::path bordered =
        working_directory / "cli_table_cell_border_bordered.docx";
    const fs::path cleared =
        working_directory / "cli_table_cell_border_cleared.docx";
    const fs::path set_output =
        working_directory / "cli_table_cell_border_set_output.json";
    const fs::path clear_output =
        working_directory / "cli_table_cell_border_clear_output.json";

    remove_if_exists(source);
    remove_if_exists(bordered);
    remove_if_exists(cleared);
    remove_if_exists(set_output);
    remove_if_exists(clear_output);

    create_cli_table_cell_style_fixture(source);

    CHECK_EQ(run_cli({"set-table-cell-border",
                      source.string(),
                      "0",
                      "0",
                      "0",
                      "right",
                      "--style",
                      "dotted",
                      "--size",
                      "6",
                      "--color",
                      "0000FF",
                      "--space",
                      "1",
                      "--output",
                      bordered.string(),
                      "--json"},
                     set_output),
             0);
    CHECK_EQ(
        read_text_file(set_output),
        std::string{
            "{\"command\":\"set-table-cell-border\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"table_index\":0,\"row_index\":0,\"cell_index\":0,"
            "\"edge\":\"right\",\"style\":\"dotted\","
            "\"size_eighth_points\":6,\"color\":\"0000FF\","
            "\"space_points\":1}\n"});

    const auto bordered_document_xml =
        read_docx_entry(bordered, "word/document.xml");
    pugi::xml_document bordered_xml_document;
    REQUIRE(bordered_xml_document.load_string(bordered_document_xml.c_str()));
    const auto bordered_cell_node =
        bordered_xml_document.child("w:document").child("w:body").child("w:tbl").child("w:tr").child("w:tc");
    REQUIRE(bordered_cell_node != pugi::xml_node{});
    const auto right_border =
        bordered_cell_node.child("w:tcPr").child("w:tcBorders").child("w:right");
    REQUIRE(right_border != pugi::xml_node{});
    CHECK_EQ(std::string_view{right_border.attribute("w:val").value()}, "dotted");
    CHECK_EQ(std::string_view{right_border.attribute("w:sz").value()}, "6");
    CHECK_EQ(std::string_view{right_border.attribute("w:space").value()}, "1");
    CHECK_EQ(std::string_view{right_border.attribute("w:color").value()}, "0000FF");

    featherdoc::Document reopened(bordered);
    REQUIRE_FALSE(reopened.open());
    auto reopened_cell = reopened.tables().rows().cells();
    REQUIRE(reopened_cell.has_next());

    CHECK_EQ(run_cli({"clear-table-cell-border",
                      bordered.string(),
                      "0",
                      "0",
                      "0",
                      "right",
                      "--output",
                      cleared.string(),
                      "--json"},
                     clear_output),
             0);
    CHECK_EQ(
        read_text_file(clear_output),
        std::string{
            "{\"command\":\"clear-table-cell-border\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"table_index\":0,\"row_index\":0,\"cell_index\":0,"
            "\"edge\":\"right\"}\n"});

    const auto cleared_document_xml = read_docx_entry(cleared, "word/document.xml");
    pugi::xml_document cleared_xml_document;
    REQUIRE(cleared_xml_document.load_string(cleared_document_xml.c_str()));
    const auto cleared_cell_node =
        cleared_xml_document.child("w:document").child("w:body").child("w:tbl").child("w:tr").child("w:tc");
    REQUIRE(cleared_cell_node != pugi::xml_node{});
    CHECK(cleared_cell_node.child("w:tcPr").child("w:tcBorders").child("w:right") ==
          pugi::xml_node{});

    featherdoc::Document reopened_cleared(cleared);
    REQUIRE_FALSE(reopened_cleared.open());
    auto reopened_cleared_cell = reopened_cleared.tables().rows().cells();
    REQUIRE(reopened_cleared_cell.has_next());

    remove_if_exists(source);
    remove_if_exists(bordered);
    remove_if_exists(cleared);
    remove_if_exists(set_output);
    remove_if_exists(clear_output);
}

TEST_CASE("cli table cell style and geometry commands report parse and mutate errors") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_table_cell_style_error_source.docx";
    const fs::path parse_output =
        working_directory / "cli_table_cell_style_parse_output.json";
    const fs::path mutate_output =
        working_directory / "cli_table_cell_style_mutate_output.json";

    remove_if_exists(source);
    remove_if_exists(parse_output);
    remove_if_exists(mutate_output);

    create_cli_table_cell_style_fixture(source);

    CHECK_EQ(run_cli({"set-table-cell-vertical-alignment",
                      source.string(),
                      "0",
                      "0",
                      "0",
                      "middle",
                      "--json"},
                     parse_output),
             2);
    CHECK_EQ(
        read_text_file(parse_output),
        std::string{
            "{\"command\":\"set-table-cell-vertical-alignment\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"invalid vertical alignment: "
            "middle\"}\n"});

    CHECK_EQ(run_cli({"set-table-cell-border",
                      source.string(),
                      "0",
                      "0",
                      "0",
                      "right",
                      "--style",
                      "groove",
                      "--json"},
                     parse_output),
             2);
    CHECK_EQ(
        read_text_file(parse_output),
        std::string{
            "{\"command\":\"set-table-cell-border\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"invalid border style: "
            "groove\"}\n"});

    CHECK_EQ(run_cli({"set-table-cell-text-direction",
                      source.string(),
                      "0",
                      "0",
                      "9",
                      "top_to_bottom_right_to_left",
                      "--json"},
                     mutate_output),
             1);
    const auto mutate_json = read_text_file(mutate_output);
    CHECK_NE(mutate_json.find("\"command\":\"set-table-cell-text-direction\""),
             std::string::npos);
    CHECK_NE(mutate_json.find("\"stage\":\"mutate\""), std::string::npos);
    CHECK_NE(
        mutate_json.find(
            "\"detail\":\"cell index '9' is out of range for row index '0' in "
            "table index '0'\""),
        std::string::npos);
    CHECK_NE(mutate_json.find("\"entry\":\"word/document.xml\""),
             std::string::npos);

    CHECK_EQ(run_cli({"set-table-cell-width",
                      source.string(),
                      "0",
                      "0",
                      "9",
                      "2400",
                      "--json"},
                     mutate_output),
             1);
    const auto width_mutate_json = read_text_file(mutate_output);
    CHECK_NE(width_mutate_json.find("\"command\":\"set-table-cell-width\""),
             std::string::npos);
    CHECK_NE(width_mutate_json.find("\"stage\":\"mutate\""), std::string::npos);
    CHECK_NE(
        width_mutate_json.find(
            "\"detail\":\"cell index '9' is out of range for row index '0' in "
            "table index '0'\""),
        std::string::npos);
    CHECK_NE(width_mutate_json.find("\"entry\":\"word/document.xml\""),
             std::string::npos);

    remove_if_exists(source);
    remove_if_exists(parse_output);
    remove_if_exists(mutate_output);
}

} // namespace
