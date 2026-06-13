#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "cli_table_test_support.hpp"

namespace {
TEST_CASE("cli table cell fill commands set and clear body cell fill") {
    const fs::path working_directory = fs::current_path();
    const fs::path source = working_directory / "cli_table_cell_fill_source.docx";
    const fs::path styled = working_directory / "cli_table_cell_fill_styled.docx";
    const fs::path cleared = working_directory / "cli_table_cell_fill_cleared.docx";
    const fs::path set_output =
        working_directory / "cli_table_cell_fill_set_output.json";
    const fs::path clear_output =
        working_directory / "cli_table_cell_fill_clear_output.json";

    remove_if_exists(source);
    remove_if_exists(styled);
    remove_if_exists(cleared);
    remove_if_exists(set_output);
    remove_if_exists(clear_output);

    create_cli_table_cell_style_fixture(source);

    CHECK_EQ(run_cli({"set-table-cell-fill",
                      source.string(),
                      "0",
                      "0",
                      "0",
                      "A1B2C3",
                      "--output",
                      styled.string(),
                      "--json"},
                     set_output),
             0);
    CHECK_EQ(
        read_text_file(set_output),
        std::string{
            "{\"command\":\"set-table-cell-fill\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"table_index\":0,\"row_index\":0,\"cell_index\":0,"
            "\"fill_color\":\"A1B2C3\"}\n"});

    const auto source_document_xml = read_docx_entry(source, "word/document.xml");
    pugi::xml_document source_xml_document;
    REQUIRE(source_xml_document.load_string(source_document_xml.c_str()));
    const auto source_cell_node =
        source_xml_document.child("w:document").child("w:body").child("w:tbl").child("w:tr").child("w:tc");
    REQUIRE(source_cell_node != pugi::xml_node{});
    CHECK(source_cell_node.child("w:tcPr").child("w:shd") == pugi::xml_node{});

    const auto styled_document_xml = read_docx_entry(styled, "word/document.xml");
    pugi::xml_document styled_xml_document;
    REQUIRE(styled_xml_document.load_string(styled_document_xml.c_str()));
    const auto styled_cell_node =
        styled_xml_document.child("w:document").child("w:body").child("w:tbl").child("w:tr").child("w:tc");
    REQUIRE(styled_cell_node != pugi::xml_node{});
    const auto shading = styled_cell_node.child("w:tcPr").child("w:shd");
    REQUIRE(shading != pugi::xml_node{});
    CHECK_EQ(std::string_view{shading.attribute("w:val").value()}, "clear");
    CHECK_EQ(std::string_view{shading.attribute("w:color").value()}, "auto");
    CHECK_EQ(std::string_view{shading.attribute("w:fill").value()}, "A1B2C3");

    featherdoc::Document reopened(styled);
    REQUIRE_FALSE(reopened.open());
    auto reopened_cell = reopened.tables().rows().cells();
    REQUIRE(reopened_cell.has_next());
    REQUIRE(reopened_cell.fill_color().has_value());
    CHECK_EQ(*reopened_cell.fill_color(), "A1B2C3");

    CHECK_EQ(run_cli({"clear-table-cell-fill",
                      styled.string(),
                      "0",
                      "0",
                      "0",
                      "--output",
                      cleared.string(),
                      "--json"},
                     clear_output),
             0);
    CHECK_EQ(
        read_text_file(clear_output),
        std::string{
            "{\"command\":\"clear-table-cell-fill\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"table_index\":0,\"row_index\":0,\"cell_index\":0}\n"});

    const auto cleared_document_xml = read_docx_entry(cleared, "word/document.xml");
    pugi::xml_document cleared_xml_document;
    REQUIRE(cleared_xml_document.load_string(cleared_document_xml.c_str()));
    const auto cleared_cell_node =
        cleared_xml_document.child("w:document").child("w:body").child("w:tbl").child("w:tr").child("w:tc");
    REQUIRE(cleared_cell_node != pugi::xml_node{});
    CHECK(cleared_cell_node.child("w:tcPr").child("w:shd") == pugi::xml_node{});

    featherdoc::Document reopened_cleared(cleared);
    REQUIRE_FALSE(reopened_cleared.open());
    auto reopened_cleared_cell = reopened_cleared.tables().rows().cells();
    REQUIRE(reopened_cleared_cell.has_next());
    CHECK_FALSE(reopened_cleared_cell.fill_color().has_value());

    remove_if_exists(source);
    remove_if_exists(styled);
    remove_if_exists(cleared);
    remove_if_exists(set_output);
    remove_if_exists(clear_output);
}

TEST_CASE("cli table cell vertical alignment commands set and clear body cell alignment") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_table_cell_vertical_alignment_source.docx";
    const fs::path aligned =
        working_directory / "cli_table_cell_vertical_alignment_aligned.docx";
    const fs::path cleared =
        working_directory / "cli_table_cell_vertical_alignment_cleared.docx";
    const fs::path set_output =
        working_directory / "cli_table_cell_vertical_alignment_set_output.json";
    const fs::path clear_output =
        working_directory / "cli_table_cell_vertical_alignment_clear_output.json";

    remove_if_exists(source);
    remove_if_exists(aligned);
    remove_if_exists(cleared);
    remove_if_exists(set_output);
    remove_if_exists(clear_output);

    create_cli_table_cell_style_fixture(source);

    CHECK_EQ(run_cli({"set-table-cell-vertical-alignment",
                      source.string(),
                      "0",
                      "0",
                      "0",
                      "center",
                      "--output",
                      aligned.string(),
                      "--json"},
                     set_output),
             0);
    CHECK_EQ(
        read_text_file(set_output),
        std::string{
            "{\"command\":\"set-table-cell-vertical-alignment\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"table_index\":0,\"row_index\":0,\"cell_index\":0,"
            "\"vertical_alignment\":\"center\"}\n"});

    const auto source_document_xml = read_docx_entry(source, "word/document.xml");
    pugi::xml_document source_xml_document;
    REQUIRE(source_xml_document.load_string(source_document_xml.c_str()));
    const auto source_cell_node =
        source_xml_document.child("w:document").child("w:body").child("w:tbl").child("w:tr").child("w:tc");
    REQUIRE(source_cell_node != pugi::xml_node{});
    CHECK(source_cell_node.child("w:tcPr").child("w:vAlign") == pugi::xml_node{});

    const auto aligned_document_xml = read_docx_entry(aligned, "word/document.xml");
    pugi::xml_document aligned_xml_document;
    REQUIRE(aligned_xml_document.load_string(aligned_document_xml.c_str()));
    const auto aligned_cell_node =
        aligned_xml_document.child("w:document").child("w:body").child("w:tbl").child("w:tr").child("w:tc");
    REQUIRE(aligned_cell_node != pugi::xml_node{});
    const auto vertical_alignment = aligned_cell_node.child("w:tcPr").child("w:vAlign");
    REQUIRE(vertical_alignment != pugi::xml_node{});
    CHECK_EQ(std::string_view{vertical_alignment.attribute("w:val").value()},
             "center");

    featherdoc::Document reopened(aligned);
    REQUIRE_FALSE(reopened.open());
    auto reopened_cell = reopened.tables().rows().cells();
    REQUIRE(reopened_cell.has_next());
    REQUIRE(reopened_cell.vertical_alignment().has_value());
    CHECK_EQ(*reopened_cell.vertical_alignment(),
             featherdoc::cell_vertical_alignment::center);

    CHECK_EQ(run_cli({"clear-table-cell-vertical-alignment",
                      aligned.string(),
                      "0",
                      "0",
                      "0",
                      "--output",
                      cleared.string(),
                      "--json"},
                     clear_output),
             0);
    CHECK_EQ(
        read_text_file(clear_output),
        std::string{
            "{\"command\":\"clear-table-cell-vertical-alignment\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"table_index\":0,\"row_index\":0,\"cell_index\":0}\n"});

    const auto cleared_document_xml = read_docx_entry(cleared, "word/document.xml");
    pugi::xml_document cleared_xml_document;
    REQUIRE(cleared_xml_document.load_string(cleared_document_xml.c_str()));
    const auto cleared_cell_node =
        cleared_xml_document.child("w:document").child("w:body").child("w:tbl").child("w:tr").child("w:tc");
    REQUIRE(cleared_cell_node != pugi::xml_node{});
    CHECK(cleared_cell_node.child("w:tcPr").child("w:vAlign") == pugi::xml_node{});

    featherdoc::Document reopened_cleared(cleared);
    REQUIRE_FALSE(reopened_cleared.open());
    auto reopened_cleared_cell = reopened_cleared.tables().rows().cells();
    REQUIRE(reopened_cleared_cell.has_next());
    CHECK_FALSE(reopened_cleared_cell.vertical_alignment().has_value());

    remove_if_exists(source);
    remove_if_exists(aligned);
    remove_if_exists(cleared);
    remove_if_exists(set_output);
    remove_if_exists(clear_output);
}

TEST_CASE("cli table cell text direction commands set and clear body cell direction") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_table_cell_text_direction_source.docx";
    const fs::path directed =
        working_directory / "cli_table_cell_text_direction_directed.docx";
    const fs::path cleared =
        working_directory / "cli_table_cell_text_direction_cleared.docx";
    const fs::path set_output =
        working_directory / "cli_table_cell_text_direction_set_output.json";
    const fs::path clear_output =
        working_directory / "cli_table_cell_text_direction_clear_output.json";

    remove_if_exists(source);
    remove_if_exists(directed);
    remove_if_exists(cleared);
    remove_if_exists(set_output);
    remove_if_exists(clear_output);

    create_cli_table_cell_style_fixture(source);

    CHECK_EQ(run_cli({"set-table-cell-text-direction",
                      source.string(),
                      "0",
                      "0",
                      "0",
                      "top_to_bottom_right_to_left",
                      "--output",
                      directed.string(),
                      "--json"},
                     set_output),
             0);
    CHECK_EQ(
        read_text_file(set_output),
        std::string{
            "{\"command\":\"set-table-cell-text-direction\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"table_index\":0,\"row_index\":0,\"cell_index\":0,"
            "\"text_direction\":\"top_to_bottom_right_to_left\"}\n"});

    const auto source_document_xml = read_docx_entry(source, "word/document.xml");
    pugi::xml_document source_xml_document;
    REQUIRE(source_xml_document.load_string(source_document_xml.c_str()));
    const auto source_cell_node =
        source_xml_document.child("w:document").child("w:body").child("w:tbl").child("w:tr").child("w:tc");
    REQUIRE(source_cell_node != pugi::xml_node{});
    CHECK(source_cell_node.child("w:tcPr").child("w:textDirection") ==
          pugi::xml_node{});

    const auto directed_document_xml = read_docx_entry(directed, "word/document.xml");
    pugi::xml_document directed_xml_document;
    REQUIRE(directed_xml_document.load_string(directed_document_xml.c_str()));
    const auto directed_cell_node =
        directed_xml_document.child("w:document").child("w:body").child("w:tbl").child("w:tr").child("w:tc");
    REQUIRE(directed_cell_node != pugi::xml_node{});
    const auto text_direction =
        directed_cell_node.child("w:tcPr").child("w:textDirection");
    REQUIRE(text_direction != pugi::xml_node{});
    CHECK_EQ(std::string_view{text_direction.attribute("w:val").value()}, "tbRl");

    featherdoc::Document reopened(directed);
    REQUIRE_FALSE(reopened.open());
    auto reopened_cell = reopened.tables().rows().cells();
    REQUIRE(reopened_cell.has_next());
    REQUIRE(reopened_cell.text_direction().has_value());
    CHECK_EQ(*reopened_cell.text_direction(),
             featherdoc::cell_text_direction::top_to_bottom_right_to_left);

    CHECK_EQ(run_cli({"clear-table-cell-text-direction",
                      directed.string(),
                      "0",
                      "0",
                      "0",
                      "--output",
                      cleared.string(),
                      "--json"},
                     clear_output),
             0);
    CHECK_EQ(
        read_text_file(clear_output),
        std::string{
            "{\"command\":\"clear-table-cell-text-direction\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"table_index\":0,\"row_index\":0,\"cell_index\":0}\n"});

    const auto cleared_document_xml = read_docx_entry(cleared, "word/document.xml");
    pugi::xml_document cleared_xml_document;
    REQUIRE(cleared_xml_document.load_string(cleared_document_xml.c_str()));
    const auto cleared_cell_node =
        cleared_xml_document.child("w:document").child("w:body").child("w:tbl").child("w:tr").child("w:tc");
    REQUIRE(cleared_cell_node != pugi::xml_node{});
    CHECK(cleared_cell_node.child("w:tcPr").child("w:textDirection") ==
          pugi::xml_node{});

    featherdoc::Document reopened_cleared(cleared);
    REQUIRE_FALSE(reopened_cleared.open());
    auto reopened_cleared_cell = reopened_cleared.tables().rows().cells();
    REQUIRE(reopened_cleared_cell.has_next());
    CHECK_FALSE(reopened_cleared_cell.text_direction().has_value());

    remove_if_exists(source);
    remove_if_exists(directed);
    remove_if_exists(cleared);
    remove_if_exists(set_output);
    remove_if_exists(clear_output);
}

TEST_CASE("cli table cell width commands set and clear body cell width") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_table_cell_width_source.docx";
    const fs::path sized =
        working_directory / "cli_table_cell_width_sized.docx";
    const fs::path cleared =
        working_directory / "cli_table_cell_width_cleared.docx";
    const fs::path set_output =
        working_directory / "cli_table_cell_width_set_output.json";
    const fs::path clear_output =
        working_directory / "cli_table_cell_width_clear_output.json";

    remove_if_exists(source);
    remove_if_exists(sized);
    remove_if_exists(cleared);
    remove_if_exists(set_output);
    remove_if_exists(clear_output);

    create_cli_table_cell_style_fixture(source);

    CHECK_EQ(run_cli({"set-table-cell-width",
                      source.string(),
                      "0",
                      "0",
                      "0",
                      "2222",
                      "--output",
                      sized.string(),
                      "--json"},
                     set_output),
             0);
    CHECK_EQ(
        read_text_file(set_output),
        std::string{
            "{\"command\":\"set-table-cell-width\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"table_index\":0,\"row_index\":0,\"cell_index\":0,"
            "\"width_twips\":2222}\n"});

    const auto sized_document_xml = read_docx_entry(sized, "word/document.xml");
    pugi::xml_document sized_xml_document;
    REQUIRE(sized_xml_document.load_string(sized_document_xml.c_str()));
    const auto sized_cell_node =
        sized_xml_document.child("w:document").child("w:body").child("w:tbl").child("w:tr").child("w:tc");
    REQUIRE(sized_cell_node != pugi::xml_node{});
    const auto width_node = sized_cell_node.child("w:tcPr").child("w:tcW");
    REQUIRE(width_node != pugi::xml_node{});
    CHECK_EQ(std::string_view{width_node.attribute("w:type").value()}, "dxa");
    CHECK_EQ(std::string_view{width_node.attribute("w:w").value()}, "2222");

    featherdoc::Document reopened(sized);
    REQUIRE_FALSE(reopened.open());
    auto reopened_cell = reopened.tables().rows().cells();
    REQUIRE(reopened_cell.has_next());
    REQUIRE(reopened_cell.width_twips().has_value());
    CHECK_EQ(*reopened_cell.width_twips(), 2222U);

    CHECK_EQ(run_cli({"clear-table-cell-width",
                      sized.string(),
                      "0",
                      "0",
                      "0",
                      "--output",
                      cleared.string(),
                      "--json"},
                     clear_output),
             0);
    CHECK_EQ(
        read_text_file(clear_output),
        std::string{
            "{\"command\":\"clear-table-cell-width\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"table_index\":0,\"row_index\":0,\"cell_index\":0}\n"});

    const auto cleared_document_xml = read_docx_entry(cleared, "word/document.xml");
    pugi::xml_document cleared_xml_document;
    REQUIRE(cleared_xml_document.load_string(cleared_document_xml.c_str()));
    const auto cleared_cell_node =
        cleared_xml_document.child("w:document").child("w:body").child("w:tbl").child("w:tr").child("w:tc");
    REQUIRE(cleared_cell_node != pugi::xml_node{});
    CHECK(cleared_cell_node.child("w:tcPr").child("w:tcW") == pugi::xml_node{});

    featherdoc::Document reopened_cleared(cleared);
    REQUIRE_FALSE(reopened_cleared.open());
    auto reopened_cleared_cell = reopened_cleared.tables().rows().cells();
    REQUIRE(reopened_cleared_cell.has_next());
    CHECK_FALSE(reopened_cleared_cell.width_twips().has_value());

    remove_if_exists(source);
    remove_if_exists(sized);
    remove_if_exists(cleared);
    remove_if_exists(set_output);
    remove_if_exists(clear_output);
}

TEST_CASE("cli table column width commands set and clear body column widths") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_table_column_width_source.docx";
    const fs::path sized =
        working_directory / "cli_table_column_width_sized.docx";
    const fs::path cleared =
        working_directory / "cli_table_column_width_cleared.docx";
    const fs::path set_output =
        working_directory / "cli_table_column_width_set_output.json";
    const fs::path clear_output =
        working_directory / "cli_table_column_width_clear_output.json";
    const fs::path error_output =
        working_directory / "cli_table_column_width_error_output.json";

    remove_if_exists(source);
    remove_if_exists(sized);
    remove_if_exists(cleared);
    remove_if_exists(set_output);
    remove_if_exists(clear_output);
    remove_if_exists(error_output);

    create_cli_table_inspection_fixture(source);

    CHECK_EQ(run_cli({"set-table-column-width",
                      source.string(),
                      "0",
                      "1",
                      "2400",
                      "--output",
                      sized.string(),
                      "--json"},
                     set_output),
             0);
    CHECK_EQ(
        read_text_file(set_output),
        std::string{
            "{\"command\":\"set-table-column-width\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"table_index\":0,\"column_index\":1,\"width_twips\":2400}\n"});

    featherdoc::Document reopened_sized(sized);
    REQUIRE_FALSE(reopened_sized.open());
    const auto sized_table = reopened_sized.inspect_table(0U);
    REQUIRE(sized_table.has_value());
    CHECK_EQ(sized_table->column_count, 2U);
    REQUIRE(sized_table->column_widths.size() >= 2U);
    REQUIRE(sized_table->column_widths[0].has_value());
    REQUIRE(sized_table->column_widths[1].has_value());
    CHECK_EQ(*sized_table->column_widths[0], 1800U);
    CHECK_EQ(*sized_table->column_widths[1], 2400U);

    CHECK_EQ(run_cli({"clear-table-column-width",
                      sized.string(),
                      "0",
                      "1",
                      "--output",
                      cleared.string(),
                      "--json"},
                     clear_output),
             0);
    CHECK_EQ(
        read_text_file(clear_output),
        std::string{
            "{\"command\":\"clear-table-column-width\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"table_index\":0,\"column_index\":1}\n"});

    featherdoc::Document reopened_cleared(cleared);
    REQUIRE_FALSE(reopened_cleared.open());
    const auto cleared_table = reopened_cleared.inspect_table(0U);
    REQUIRE(cleared_table.has_value());
    CHECK_EQ(cleared_table->column_count, 2U);
    REQUIRE(cleared_table->column_widths.size() >= 2U);
    REQUIRE(cleared_table->column_widths[0].has_value());
    CHECK_EQ(*cleared_table->column_widths[0], 1800U);
    CHECK_FALSE(cleared_table->column_widths[1].has_value());

    CHECK_EQ(run_cli({"set-table-column-width",
                      source.string(),
                      "0",
                      "9",
                      "2400",
                      "--json"},
                     error_output),
             1);
    const auto error_json = read_text_file(error_output);
    CHECK_NE(error_json.find("\"command\":\"set-table-column-width\""),
             std::string::npos);
    CHECK_NE(error_json.find("\"stage\":\"mutate\""), std::string::npos);
    CHECK_NE(
        error_json.find(
            "\"detail\":\"column index '9' is out of range for table index '0'\""),
        std::string::npos);

    remove_if_exists(source);
    remove_if_exists(sized);
    remove_if_exists(cleared);
    remove_if_exists(set_output);
    remove_if_exists(clear_output);
    remove_if_exists(error_output);
}

} // namespace
