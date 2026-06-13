#include "cli_table_test_support.hpp"

namespace {
TEST_CASE("cli table position commands set inspect and clear body table position") {
    const fs::path working_directory = fs::current_path();
    const fs::path source = working_directory / "cli_table_position_source.docx";
    const fs::path positioned = working_directory / "cli_table_position_set.docx";
    const fs::path cleared = working_directory / "cli_table_position_cleared.docx";
    const fs::path preset_positioned =
        working_directory / "cli_table_position_preset_set.docx";
    const fs::path all_positioned =
        working_directory / "cli_table_position_all_set.docx";
    const fs::path list_positioned =
        working_directory / "cli_table_position_list_set.docx";
    const fs::path all_cleared =
        working_directory / "cli_table_position_all_cleared.docx";
    const fs::path list_cleared =
        working_directory / "cli_table_position_list_cleared.docx";
    const fs::path set_output =
        working_directory / "cli_table_position_set_output.json";
    const fs::path inspect_output =
        working_directory / "cli_table_position_inspect_output.json";
    const fs::path clear_output =
        working_directory / "cli_table_position_clear_output.json";
    const fs::path inspect_cleared_output =
        working_directory / "cli_table_position_inspect_cleared_output.json";
    const fs::path preset_output =
        working_directory / "cli_table_position_preset_output.json";
    const fs::path all_output =
        working_directory / "cli_table_position_all_output.json";
    const fs::path list_output =
        working_directory / "cli_table_position_list_output.json";
    const fs::path clear_all_output =
        working_directory / "cli_table_position_clear_all_output.json";
    const fs::path clear_list_output =
        working_directory / "cli_table_position_clear_list_output.json";
    const fs::path parse_error_output =
        working_directory / "cli_table_position_parse_error.json";

    remove_if_exists(source);
    remove_if_exists(positioned);
    remove_if_exists(cleared);
    remove_if_exists(preset_positioned);
    remove_if_exists(all_positioned);
    remove_if_exists(list_positioned);
    remove_if_exists(all_cleared);
    remove_if_exists(list_cleared);
    remove_if_exists(set_output);
    remove_if_exists(inspect_output);
    remove_if_exists(clear_output);
    remove_if_exists(inspect_cleared_output);
    remove_if_exists(preset_output);
    remove_if_exists(all_output);
    remove_if_exists(list_output);
    remove_if_exists(clear_all_output);
    remove_if_exists(clear_list_output);
    remove_if_exists(parse_error_output);

    create_cli_table_inspection_fixture(source);

    CHECK_EQ(run_cli({"set-table-position",
                      source.string(),
                      "0",
                      "--horizontal-reference",
                      "page",
                      "--horizontal-offset",
                      "720",
                      "--horizontal-spec",
                      "center",
                      "--vertical-reference",
                      "paragraph",
                      "--vertical-offset",
                      "-120",
                      "--vertical-spec",
                      "bottom",
                      "--left-from-text",
                      "144",
                      "--right-from-text",
                      "288",
                      "--top-from-text",
                      "72",
                      "--bottom-from-text",
                      "216",
                      "--overlap",
                      "never",
                      "--output",
                      positioned.string(),
                      "--json"},
                     set_output),
             0);
    CHECK_EQ(
        read_text_file(set_output),
        std::string{
            "{\"command\":\"set-table-position\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"table_indices\":[0],\"positions\":[{\"horizontal_reference\":\"page\","
            "\"horizontal_offset_twips\":720,\"horizontal_spec\":\"center\","
            "\"vertical_reference\":\"paragraph\",\"vertical_offset_twips\":-120,"
            "\"vertical_spec\":\"bottom\",\"left_from_text_twips\":144,"
            "\"right_from_text_twips\":288,\"top_from_text_twips\":72,"
            "\"bottom_from_text_twips\":216,\"overlap\":\"never\"}]}\n"});

    const auto positioned_document_xml = read_docx_entry(positioned, "word/document.xml");
    pugi::xml_document positioned_xml_document;
    REQUIRE(positioned_xml_document.load_string(positioned_document_xml.c_str()));
    const auto positioned_table_properties = positioned_xml_document.child("w:document")
                                                 .child("w:body")
                                                 .child("w:tbl")
                                                 .child("w:tblPr");
    REQUIRE(positioned_table_properties != pugi::xml_node{});
    const auto table_position = positioned_table_properties.child("w:tblpPr");
    REQUIRE(table_position != pugi::xml_node{});
    CHECK_EQ(std::string_view{table_position.attribute("w:horzAnchor").value()},
             "page");
    CHECK_EQ(std::string_view{table_position.attribute("w:tblpX").value()},
             "720");
    CHECK_EQ(std::string_view{table_position.attribute("w:tblpXSpec").value()},
             "center");
    CHECK_EQ(std::string_view{table_position.attribute("w:vertAnchor").value()},
             "text");
    CHECK_EQ(std::string_view{table_position.attribute("w:tblpY").value()},
             "-120");
    CHECK_EQ(std::string_view{table_position.attribute("w:tblpYSpec").value()},
             "bottom");
    CHECK_EQ(std::string_view{table_position.attribute("w:leftFromText").value()},
             "144");
    CHECK_EQ(std::string_view{table_position.attribute("w:rightFromText").value()},
             "288");
    CHECK_EQ(std::string_view{table_position.attribute("w:topFromText").value()},
             "72");
    CHECK_EQ(std::string_view{table_position.attribute("w:bottomFromText").value()},
             "216");
    CHECK_EQ(std::string_view{table_position.attribute("w:tblOverlap").value()},
             "never");

    featherdoc::Document reopened(positioned);
    REQUIRE_FALSE(reopened.open());
    auto reopened_table = reopened.tables();
    REQUIRE(reopened_table.has_next());
    const auto reopened_position = reopened_table.position();
    REQUIRE(reopened_position.has_value());
    CHECK_EQ(reopened_position->horizontal_reference,
             featherdoc::table_position_horizontal_reference::page);
    CHECK_EQ(reopened_position->horizontal_offset_twips, 720);
    REQUIRE(reopened_position->horizontal_spec.has_value());
    CHECK_EQ(*reopened_position->horizontal_spec,
             featherdoc::table_position_horizontal_spec::center);
    CHECK_EQ(reopened_position->vertical_reference,
             featherdoc::table_position_vertical_reference::paragraph);
    CHECK_EQ(reopened_position->vertical_offset_twips, -120);
    REQUIRE(reopened_position->vertical_spec.has_value());
    CHECK_EQ(*reopened_position->vertical_spec,
             featherdoc::table_position_vertical_spec::bottom);
    REQUIRE(reopened_position->left_from_text_twips.has_value());
    CHECK_EQ(*reopened_position->left_from_text_twips, 144U);
    REQUIRE(reopened_position->right_from_text_twips.has_value());
    CHECK_EQ(*reopened_position->right_from_text_twips, 288U);
    REQUIRE(reopened_position->top_from_text_twips.has_value());
    CHECK_EQ(*reopened_position->top_from_text_twips, 72U);
    REQUIRE(reopened_position->bottom_from_text_twips.has_value());
    CHECK_EQ(*reopened_position->bottom_from_text_twips, 216U);
    REQUIRE(reopened_position->overlap.has_value());
    CHECK_EQ(*reopened_position->overlap, featherdoc::table_overlap::never);

    CHECK_EQ(run_cli({"inspect-tables", positioned.string(), "--table", "0", "--json"},
                     inspect_output),
             0);
    const auto inspect_json = read_text_file(inspect_output);
    CHECK_NE(inspect_json.find("\"position\":{\"horizontal_reference\":\"page\","),
             std::string::npos);
    CHECK_NE(inspect_json.find("\"horizontal_offset_twips\":720"),
             std::string::npos);
    CHECK_NE(inspect_json.find("\"horizontal_spec\":\"center\""),
             std::string::npos);
    CHECK_NE(inspect_json.find("\"vertical_reference\":\"paragraph\""),
             std::string::npos);
    CHECK_NE(inspect_json.find("\"vertical_offset_twips\":-120"),
             std::string::npos);
    CHECK_NE(inspect_json.find("\"vertical_spec\":\"bottom\""),
             std::string::npos);

    CHECK_EQ(run_cli({"set-table-position",
                      source.string(),
                      "0",
                      "--preset",
                      "page-corner",
                      "--horizontal-offset",
                      "360",
                      "--bottom-from-text",
                      "288",
                      "--output",
                      preset_positioned.string(),
                      "--json"},
                     preset_output),
             0);
    CHECK_EQ(
        read_text_file(preset_output),
        std::string{
            "{\"command\":\"set-table-position\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"table_indices\":[0],\"positions\":[{\"horizontal_reference\":\"page\","
            "\"horizontal_offset_twips\":360,\"horizontal_spec\":null,"
            "\"vertical_reference\":\"page\",\"vertical_offset_twips\":720,"
            "\"vertical_spec\":null,\"left_from_text_twips\":144,"
            "\"right_from_text_twips\":144,\"top_from_text_twips\":144,"
            "\"bottom_from_text_twips\":288,\"overlap\":\"never\"}]}\n"});

    const auto preset_document_xml =
        read_docx_entry(preset_positioned, "word/document.xml");
    pugi::xml_document preset_xml_document;
    REQUIRE(preset_xml_document.load_string(preset_document_xml.c_str()));
    const auto preset_table_position = preset_xml_document.child("w:document")
                                           .child("w:body")
                                           .child("w:tbl")
                                           .child("w:tblPr")
                                           .child("w:tblpPr");
    REQUIRE(preset_table_position != pugi::xml_node{});
    CHECK_EQ(
        std::string_view{preset_table_position.attribute("w:horzAnchor").value()},
        "page");
    CHECK_EQ(std::string_view{preset_table_position.attribute("w:tblpX").value()},
             "360");
    CHECK_EQ(
        std::string_view{preset_table_position.attribute("w:vertAnchor").value()},
        "page");
    CHECK_EQ(std::string_view{preset_table_position.attribute("w:tblpY").value()},
             "720");
    CHECK_EQ(
        std::string_view{preset_table_position.attribute("w:bottomFromText").value()},
        "288");
    CHECK_EQ(
        std::string_view{preset_table_position.attribute("w:tblOverlap").value()},
        "never");

    CHECK_EQ(run_cli({"set-table-position",
                      source.string(),
                      "all",
                      "--preset",
                      "paragraph-callout",
                      "--output",
                      all_positioned.string(),
                      "--json"},
                     all_output),
             0);
    CHECK_EQ(
        read_text_file(all_output),
        std::string{
            "{\"command\":\"set-table-position\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"table_indices\":[0,1],\"positions\":["
            "{\"horizontal_reference\":\"column\",\"horizontal_offset_twips\":0,"
            "\"horizontal_spec\":null,\"vertical_reference\":\"paragraph\","
            "\"vertical_offset_twips\":0,\"vertical_spec\":null,"
            "\"left_from_text_twips\":144,\"right_from_text_twips\":144,"
            "\"top_from_text_twips\":72,\"bottom_from_text_twips\":72,"
            "\"overlap\":\"never\"},"
            "{\"horizontal_reference\":\"column\",\"horizontal_offset_twips\":0,"
            "\"horizontal_spec\":null,\"vertical_reference\":\"paragraph\","
            "\"vertical_offset_twips\":0,\"vertical_spec\":null,"
            "\"left_from_text_twips\":144,\"right_from_text_twips\":144,"
            "\"top_from_text_twips\":72,\"bottom_from_text_twips\":72,"
            "\"overlap\":\"never\"}]}\n"});

    const auto all_document_xml = read_docx_entry(all_positioned, "word/document.xml");
    pugi::xml_document all_xml_document;
    REQUIRE(all_xml_document.load_string(all_document_xml.c_str()));
    const auto all_body = all_xml_document.child("w:document").child("w:body");
    std::size_t all_positioned_table_count = 0U;
    for (auto table_node = all_body.child("w:tbl"); table_node != pugi::xml_node{};
         table_node = table_node.next_sibling("w:tbl")) {
        const auto table_position = table_node.child("w:tblPr").child("w:tblpPr");
        REQUIRE(table_position != pugi::xml_node{});
        CHECK_EQ(std::string_view{table_position.attribute("w:horzAnchor").value()},
                 "text");
        CHECK_EQ(std::string_view{table_position.attribute("w:vertAnchor").value()},
                 "text");
        CHECK_EQ(std::string_view{table_position.attribute("w:tblOverlap").value()},
                 "never");
        ++all_positioned_table_count;
    }
    CHECK_EQ(all_positioned_table_count, 2U);

    CHECK_EQ(run_cli({"set-table-position",
                      source.string(),
                      "0",
                      "--table",
                      "1",
                      "--preset",
                      "margin-anchor",
                      "--output",
                      list_positioned.string(),
                      "--json"},
                     list_output),
             0);
    CHECK_EQ(
        read_text_file(list_output),
        std::string{
            "{\"command\":\"set-table-position\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"table_indices\":[0,1],\"positions\":["
            "{\"horizontal_reference\":\"margin\",\"horizontal_offset_twips\":0,"
            "\"horizontal_spec\":null,\"vertical_reference\":\"paragraph\","
            "\"vertical_offset_twips\":0,\"vertical_spec\":null,"
            "\"left_from_text_twips\":144,\"right_from_text_twips\":144,"
            "\"top_from_text_twips\":72,\"bottom_from_text_twips\":72,"
            "\"overlap\":\"never\"},"
            "{\"horizontal_reference\":\"margin\",\"horizontal_offset_twips\":0,"
            "\"horizontal_spec\":null,\"vertical_reference\":\"paragraph\","
            "\"vertical_offset_twips\":0,\"vertical_spec\":null,"
            "\"left_from_text_twips\":144,\"right_from_text_twips\":144,"
            "\"top_from_text_twips\":72,\"bottom_from_text_twips\":72,"
            "\"overlap\":\"never\"}]}\n"});

    CHECK_EQ(run_cli({"clear-table-position",
                      positioned.string(),
                      "0",
                      "--output",
                      cleared.string(),
                      "--json"},
                     clear_output),
             0);
    CHECK_EQ(
        read_text_file(clear_output),
        std::string{
            "{\"command\":\"clear-table-position\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"table_indices\":[0],\"positions\":[null]}\n"});

    const auto cleared_document_xml = read_docx_entry(cleared, "word/document.xml");
    pugi::xml_document cleared_xml_document;
    REQUIRE(cleared_xml_document.load_string(cleared_document_xml.c_str()));
    const auto cleared_table_properties = cleared_xml_document.child("w:document")
                                              .child("w:body")
                                              .child("w:tbl")
                                              .child("w:tblPr");
    REQUIRE(cleared_table_properties != pugi::xml_node{});
    CHECK(cleared_table_properties.child("w:tblpPr") == pugi::xml_node{});

    CHECK_EQ(run_cli({"inspect-tables", cleared.string(), "--table", "0", "--json"},
                     inspect_cleared_output),
             0);
    CHECK_NE(read_text_file(inspect_cleared_output).find("\"position\":null"),
             std::string::npos);

    CHECK_EQ(run_cli({"set-table-position",
                      source.string(),
                      "0",
                      "--horizontal-reference",
                      "page",
                      "--horizontal-offset",
                      "0",
                      "--horizontal-spec",
                      "middle",
                      "--vertical-reference",
                      "paragraph",
                      "--vertical-offset",
                      "0",
                      "--json"},
                     parse_error_output),
             2);
    CHECK_EQ(
        read_text_file(parse_error_output),
        std::string{
            "{\"command\":\"set-table-position\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"invalid horizontal spec: middle\"}\n"});


    CHECK_EQ(run_cli({"clear-table-position",
                      all_positioned.string(),
                      "all",
                      "--output",
                      all_cleared.string(),
                      "--json"},
                     clear_all_output),
             0);
    CHECK_EQ(
        read_text_file(clear_all_output),
        std::string{
            "{\"command\":\"clear-table-position\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"table_indices\":[0,1],\"positions\":[null,null]}\n"});

    const auto all_cleared_document_xml =
        read_docx_entry(all_cleared, "word/document.xml");
    pugi::xml_document all_cleared_xml_document;
    REQUIRE(all_cleared_xml_document.load_string(all_cleared_document_xml.c_str()));
    const auto all_cleared_body =
        all_cleared_xml_document.child("w:document").child("w:body");
    std::size_t all_cleared_table_count = 0U;
    for (auto table_node = all_cleared_body.child("w:tbl");
         table_node != pugi::xml_node{};
         table_node = table_node.next_sibling("w:tbl")) {
        CHECK(table_node.child("w:tblPr").child("w:tblpPr") == pugi::xml_node{});
        ++all_cleared_table_count;
    }
    CHECK_EQ(all_cleared_table_count, 2U);

    CHECK_EQ(run_cli({"clear-table-position",
                      list_positioned.string(),
                      "0",
                      "--table",
                      "1",
                      "--output",
                      list_cleared.string(),
                      "--json"},
                     clear_list_output),
             0);
    CHECK_EQ(
        read_text_file(clear_list_output),
        std::string{
            "{\"command\":\"clear-table-position\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"table_indices\":[0,1],\"positions\":[null,null]}\n"});

    const auto list_cleared_document_xml =
        read_docx_entry(list_cleared, "word/document.xml");
    pugi::xml_document list_cleared_xml_document;
    REQUIRE(
        list_cleared_xml_document.load_string(list_cleared_document_xml.c_str()));
    const auto list_cleared_body =
        list_cleared_xml_document.child("w:document").child("w:body");
    std::size_t list_cleared_table_count = 0U;
    for (auto table_node = list_cleared_body.child("w:tbl");
         table_node != pugi::xml_node{};
         table_node = table_node.next_sibling("w:tbl")) {
        CHECK(table_node.child("w:tblPr").child("w:tblpPr") == pugi::xml_node{});
        ++list_cleared_table_count;
    }
    CHECK_EQ(list_cleared_table_count, 2U);
    remove_if_exists(source);
    remove_if_exists(positioned);
    remove_if_exists(cleared);
    remove_if_exists(preset_positioned);
    remove_if_exists(all_positioned);
    remove_if_exists(list_positioned);
    remove_if_exists(all_cleared);
    remove_if_exists(list_cleared);
    remove_if_exists(set_output);
    remove_if_exists(inspect_output);
    remove_if_exists(clear_output);
    remove_if_exists(inspect_cleared_output);
    remove_if_exists(preset_output);
    remove_if_exists(all_output);
    remove_if_exists(list_output);
    remove_if_exists(clear_all_output);
    remove_if_exists(clear_list_output);
}


} // namespace
