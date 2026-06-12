#include "cli_table_test_support.hpp"

namespace {
TEST_CASE("cli table row height commands set and clear body row height") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_table_row_height_source.docx";
    const fs::path sized =
        working_directory / "cli_table_row_height_sized.docx";
    const fs::path cleared =
        working_directory / "cli_table_row_height_cleared.docx";
    const fs::path set_output =
        working_directory / "cli_table_row_height_set_output.json";
    const fs::path clear_output =
        working_directory / "cli_table_row_height_clear_output.json";

    remove_if_exists(source);
    remove_if_exists(sized);
    remove_if_exists(cleared);
    remove_if_exists(set_output);
    remove_if_exists(clear_output);

    create_cli_table_row_style_fixture(source);

    CHECK_EQ(run_cli({"set-table-row-height",
                      source.string(),
                      "0",
                      "0",
                      "360",
                      "exact",
                      "--output",
                      sized.string(),
                      "--json"},
                     set_output),
             0);
    CHECK_EQ(
        read_text_file(set_output),
        std::string{
            "{\"command\":\"set-table-row-height\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"table_index\":0,\"row_index\":0,\"height_twips\":360,"
            "\"height_rule\":\"exact\"}\n"});

    const auto sized_document_xml = read_docx_entry(sized, "word/document.xml");
    pugi::xml_document sized_xml_document;
    REQUIRE(sized_xml_document.load_string(sized_document_xml.c_str()));
    const auto sized_row_node =
        sized_xml_document.child("w:document").child("w:body").child("w:tbl").child("w:tr");
    REQUIRE(sized_row_node != pugi::xml_node{});
    const auto row_height = sized_row_node.child("w:trPr").child("w:trHeight");
    REQUIRE(row_height != pugi::xml_node{});
    CHECK_EQ(std::string_view{row_height.attribute("w:val").value()}, "360");
    CHECK_EQ(std::string_view{row_height.attribute("w:hRule").value()}, "exact");

    featherdoc::Document reopened(sized);
    REQUIRE_FALSE(reopened.open());
    auto reopened_row = reopened.tables().rows();
    REQUIRE(reopened_row.has_next());
    REQUIRE(reopened_row.height_twips().has_value());
    CHECK_EQ(*reopened_row.height_twips(), 360U);
    REQUIRE(reopened_row.height_rule().has_value());
    CHECK_EQ(*reopened_row.height_rule(), featherdoc::row_height_rule::exact);

    CHECK_EQ(run_cli({"clear-table-row-height",
                      sized.string(),
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
            "{\"command\":\"clear-table-row-height\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"table_index\":0,\"row_index\":0}\n"});

    const auto cleared_document_xml = read_docx_entry(cleared, "word/document.xml");
    pugi::xml_document cleared_xml_document;
    REQUIRE(cleared_xml_document.load_string(cleared_document_xml.c_str()));
    const auto cleared_row_node =
        cleared_xml_document.child("w:document").child("w:body").child("w:tbl").child("w:tr");
    REQUIRE(cleared_row_node != pugi::xml_node{});
    CHECK(cleared_row_node.child("w:trPr") == pugi::xml_node{});

    featherdoc::Document reopened_cleared(cleared);
    REQUIRE_FALSE(reopened_cleared.open());
    auto reopened_cleared_row = reopened_cleared.tables().rows();
    REQUIRE(reopened_cleared_row.has_next());
    CHECK_FALSE(reopened_cleared_row.height_twips().has_value());
    CHECK_FALSE(reopened_cleared_row.height_rule().has_value());

    remove_if_exists(source);
    remove_if_exists(sized);
    remove_if_exists(cleared);
    remove_if_exists(set_output);
    remove_if_exists(clear_output);
}

TEST_CASE("cli table row cant-split commands set and clear body row cant-split") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_table_row_cant_split_source.docx";
    const fs::path locked =
        working_directory / "cli_table_row_cant_split_locked.docx";
    const fs::path cleared =
        working_directory / "cli_table_row_cant_split_cleared.docx";
    const fs::path set_output =
        working_directory / "cli_table_row_cant_split_set_output.json";
    const fs::path clear_output =
        working_directory / "cli_table_row_cant_split_clear_output.json";

    remove_if_exists(source);
    remove_if_exists(locked);
    remove_if_exists(cleared);
    remove_if_exists(set_output);
    remove_if_exists(clear_output);

    create_cli_table_row_style_fixture(source);

    CHECK_EQ(run_cli({"set-table-row-cant-split",
                      source.string(),
                      "0",
                      "0",
                      "--output",
                      locked.string(),
                      "--json"},
                     set_output),
             0);
    CHECK_EQ(
        read_text_file(set_output),
        std::string{
            "{\"command\":\"set-table-row-cant-split\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"table_index\":0,\"row_index\":0,\"cant_split\":true}\n"});

    const auto locked_document_xml = read_docx_entry(locked, "word/document.xml");
    pugi::xml_document locked_xml_document;
    REQUIRE(locked_xml_document.load_string(locked_document_xml.c_str()));
    const auto locked_row_node =
        locked_xml_document.child("w:document").child("w:body").child("w:tbl").child("w:tr");
    REQUIRE(locked_row_node != pugi::xml_node{});
    const auto cant_split = locked_row_node.child("w:trPr").child("w:cantSplit");
    REQUIRE(cant_split != pugi::xml_node{});
    CHECK_EQ(std::string_view{cant_split.attribute("w:val").value()}, "1");

    featherdoc::Document reopened(locked);
    REQUIRE_FALSE(reopened.open());
    auto reopened_row = reopened.tables().rows();
    REQUIRE(reopened_row.has_next());
    CHECK(reopened_row.cant_split());

    CHECK_EQ(run_cli({"clear-table-row-cant-split",
                      locked.string(),
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
            "{\"command\":\"clear-table-row-cant-split\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"table_index\":0,\"row_index\":0}\n"});

    const auto cleared_document_xml = read_docx_entry(cleared, "word/document.xml");
    pugi::xml_document cleared_xml_document;
    REQUIRE(cleared_xml_document.load_string(cleared_document_xml.c_str()));
    const auto cleared_row_node =
        cleared_xml_document.child("w:document").child("w:body").child("w:tbl").child("w:tr");
    REQUIRE(cleared_row_node != pugi::xml_node{});
    CHECK(cleared_row_node.child("w:trPr") == pugi::xml_node{});

    featherdoc::Document reopened_cleared(cleared);
    REQUIRE_FALSE(reopened_cleared.open());
    auto reopened_cleared_row = reopened_cleared.tables().rows();
    REQUIRE(reopened_cleared_row.has_next());
    CHECK_FALSE(reopened_cleared_row.cant_split());

    remove_if_exists(source);
    remove_if_exists(locked);
    remove_if_exists(cleared);
    remove_if_exists(set_output);
    remove_if_exists(clear_output);
}

TEST_CASE("cli table row repeat-header commands set and clear body row repeat-header") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_table_row_repeat_header_source.docx";
    const fs::path repeated =
        working_directory / "cli_table_row_repeat_header_repeated.docx";
    const fs::path cleared =
        working_directory / "cli_table_row_repeat_header_cleared.docx";
    const fs::path set_output =
        working_directory / "cli_table_row_repeat_header_set_output.json";
    const fs::path clear_output =
        working_directory / "cli_table_row_repeat_header_clear_output.json";

    remove_if_exists(source);
    remove_if_exists(repeated);
    remove_if_exists(cleared);
    remove_if_exists(set_output);
    remove_if_exists(clear_output);

    create_cli_table_row_style_fixture(source);

    CHECK_EQ(run_cli({"set-table-row-repeat-header",
                      source.string(),
                      "0",
                      "0",
                      "--output",
                      repeated.string(),
                      "--json"},
                     set_output),
             0);
    CHECK_EQ(
        read_text_file(set_output),
        std::string{
            "{\"command\":\"set-table-row-repeat-header\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"table_index\":0,\"row_index\":0,\"repeats_header\":true}\n"});

    const auto repeated_document_xml =
        read_docx_entry(repeated, "word/document.xml");
    pugi::xml_document repeated_xml_document;
    REQUIRE(repeated_xml_document.load_string(repeated_document_xml.c_str()));
    const auto repeated_row_node =
        repeated_xml_document.child("w:document").child("w:body").child("w:tbl").child("w:tr");
    REQUIRE(repeated_row_node != pugi::xml_node{});
    const auto table_header =
        repeated_row_node.child("w:trPr").child("w:tblHeader");
    REQUIRE(table_header != pugi::xml_node{});
    CHECK_EQ(std::string_view{table_header.attribute("w:val").value()}, "1");

    featherdoc::Document reopened(repeated);
    REQUIRE_FALSE(reopened.open());
    auto reopened_row = reopened.tables().rows();
    REQUIRE(reopened_row.has_next());
    CHECK(reopened_row.repeats_header());

    CHECK_EQ(run_cli({"clear-table-row-repeat-header",
                      repeated.string(),
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
            "{\"command\":\"clear-table-row-repeat-header\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"table_index\":0,\"row_index\":0}\n"});

    const auto cleared_document_xml = read_docx_entry(cleared, "word/document.xml");
    pugi::xml_document cleared_xml_document;
    REQUIRE(cleared_xml_document.load_string(cleared_document_xml.c_str()));
    const auto cleared_row_node =
        cleared_xml_document.child("w:document").child("w:body").child("w:tbl").child("w:tr");
    REQUIRE(cleared_row_node != pugi::xml_node{});
    CHECK(cleared_row_node.child("w:trPr") == pugi::xml_node{});

    featherdoc::Document reopened_cleared(cleared);
    REQUIRE_FALSE(reopened_cleared.open());
    auto reopened_cleared_row = reopened_cleared.tables().rows();
    REQUIRE(reopened_cleared_row.has_next());
    CHECK_FALSE(reopened_cleared_row.repeats_header());

    remove_if_exists(source);
    remove_if_exists(repeated);
    remove_if_exists(cleared);
    remove_if_exists(set_output);
    remove_if_exists(clear_output);
}

TEST_CASE("cli table row commands report parse and mutate errors") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_table_row_error_source.docx";
    const fs::path parse_output =
        working_directory / "cli_table_row_parse_output.json";
    const fs::path mutate_output =
        working_directory / "cli_table_row_mutate_output.json";

    remove_if_exists(source);
    remove_if_exists(parse_output);
    remove_if_exists(mutate_output);

    create_cli_table_row_style_fixture(source);

    CHECK_EQ(run_cli({"set-table-row-height",
                      source.string(),
                      "0",
                      "0",
                      "360",
                      "minimum",
                      "--json"},
                     parse_output),
             2);
    CHECK_EQ(
        read_text_file(parse_output),
        std::string{
            "{\"command\":\"set-table-row-height\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"invalid row height rule: "
            "minimum\"}\n"});

    CHECK_EQ(run_cli({"clear-table-row-cant-split",
                      source.string(),
                      "0",
                      "9",
                      "--json"},
                     mutate_output),
             1);
    const auto mutate_json = read_text_file(mutate_output);
    CHECK_NE(mutate_json.find("\"command\":\"clear-table-row-cant-split\""),
             std::string::npos);
    CHECK_NE(mutate_json.find("\"stage\":\"mutate\""), std::string::npos);
    CHECK_NE(
        mutate_json.find(
            "\"detail\":\"row index '9' is out of range for table index '0'\""),
        std::string::npos);
    CHECK_NE(mutate_json.find("\"entry\":\"word/document.xml\""),
             std::string::npos);

    remove_if_exists(source);
    remove_if_exists(parse_output);
    remove_if_exists(mutate_output);
}


} // namespace
