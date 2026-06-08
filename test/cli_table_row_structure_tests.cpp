#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
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

TEST_CASE("cli append-table-row appends a body row") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_append_table_row_source.docx";
    const fs::path appended =
        working_directory / "cli_append_table_row_appended.docx";
    const fs::path output =
        working_directory / "cli_append_table_row_output.json";

    remove_if_exists(source);
    remove_if_exists(appended);
    remove_if_exists(output);

    create_cli_table_inspection_fixture(source);

    CHECK_EQ(run_cli({"append-table-row",
                      source.string(),
                      "0",
                      "--output",
                      appended.string(),
                      "--json"},
                     output),
             0);
    CHECK_EQ(
        read_text_file(output),
        std::string{
            "{\"command\":\"append-table-row\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"table_index\":0,\"row_index\":2,\"cell_count\":2}\n"});

    featherdoc::Document reopened(appended);
    REQUIRE_FALSE(reopened.open());

    const auto table = reopened.inspect_table(0U);
    REQUIRE(table.has_value());
    CHECK_EQ(table->row_count, 3U);
    CHECK_EQ(table->column_count, 2U);

    const auto first_appended_cell = reopened.inspect_table_cell(0U, 2U, 0U);
    REQUIRE(first_appended_cell.has_value());
    CHECK_EQ(first_appended_cell->text, "");

    const auto second_appended_cell = reopened.inspect_table_cell(0U, 2U, 1U);
    REQUIRE(second_appended_cell.has_value());
    CHECK_EQ(second_appended_cell->text, "");

    remove_if_exists(source);
    remove_if_exists(appended);
    remove_if_exists(output);
}

TEST_CASE("cli insert-table-row-before and insert-table-row-after commands insert body rows") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_insert_table_row_source.docx";
    const fs::path inserted_before =
        working_directory / "cli_insert_table_row_before.docx";
    const fs::path inserted_after =
        working_directory / "cli_insert_table_row_after.docx";
    const fs::path before_output =
        working_directory / "cli_insert_table_row_before_output.json";
    const fs::path after_output =
        working_directory / "cli_insert_table_row_after_output.json";

    remove_if_exists(source);
    remove_if_exists(inserted_before);
    remove_if_exists(inserted_after);
    remove_if_exists(before_output);
    remove_if_exists(after_output);

    create_cli_table_inspection_fixture(source);

    CHECK_EQ(run_cli({"insert-table-row-before",
                      source.string(),
                      "0",
                      "1",
                      "--output",
                      inserted_before.string(),
                      "--json"},
                     before_output),
             0);
    CHECK_EQ(
        read_text_file(before_output),
        std::string{
            "{\"command\":\"insert-table-row-before\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"table_index\":0,\"row_index\":1,\"inserted_row_index\":1}\n"});

    featherdoc::Document reopened_before(inserted_before);
    REQUIRE_FALSE(reopened_before.open());
    const auto before_table = reopened_before.inspect_table(0U);
    REQUIRE(before_table.has_value());
    CHECK_EQ(before_table->row_count, 3U);
    CHECK_EQ(before_table->column_count, 2U);

    const auto before_inserted_first_cell =
        reopened_before.inspect_table_cell(0U, 1U, 0U);
    REQUIRE(before_inserted_first_cell.has_value());
    CHECK_EQ(before_inserted_first_cell->text, "");
    const auto before_inserted_second_cell =
        reopened_before.inspect_table_cell(0U, 1U, 1U);
    REQUIRE(before_inserted_second_cell.has_value());
    CHECK_EQ(before_inserted_second_cell->text, "");
    const auto before_shifted_first_cell =
        reopened_before.inspect_table_cell(0U, 2U, 0U);
    REQUIRE(before_shifted_first_cell.has_value());
    CHECK_EQ(before_shifted_first_cell->text, "r1c0");

    CHECK_EQ(run_cli({"insert-table-row-after",
                      source.string(),
                      "0",
                      "0",
                      "--output",
                      inserted_after.string(),
                      "--json"},
                     after_output),
             0);
    CHECK_EQ(
        read_text_file(after_output),
        std::string{
            "{\"command\":\"insert-table-row-after\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"table_index\":0,\"row_index\":0,\"inserted_row_index\":1}\n"});

    featherdoc::Document reopened_after(inserted_after);
    REQUIRE_FALSE(reopened_after.open());
    const auto after_table = reopened_after.inspect_table(0U);
    REQUIRE(after_table.has_value());
    CHECK_EQ(after_table->row_count, 3U);
    CHECK_EQ(after_table->column_count, 2U);

    const auto after_inserted_first_cell =
        reopened_after.inspect_table_cell(0U, 1U, 0U);
    REQUIRE(after_inserted_first_cell.has_value());
    CHECK_EQ(after_inserted_first_cell->text, "");
    const auto after_inserted_second_cell =
        reopened_after.inspect_table_cell(0U, 1U, 1U);
    REQUIRE(after_inserted_second_cell.has_value());
    CHECK_EQ(after_inserted_second_cell->text, "");
    const auto after_shifted_first_cell =
        reopened_after.inspect_table_cell(0U, 2U, 0U);
    REQUIRE(after_shifted_first_cell.has_value());
    CHECK_EQ(after_shifted_first_cell->text, "r1c0");

    remove_if_exists(source);
    remove_if_exists(inserted_before);
    remove_if_exists(inserted_after);
    remove_if_exists(before_output);
    remove_if_exists(after_output);
}

TEST_CASE("cli remove-table-row removes a body row") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_remove_table_row_source.docx";
    const fs::path removed =
        working_directory / "cli_remove_table_row_removed.docx";
    const fs::path output =
        working_directory / "cli_remove_table_row_output.json";

    remove_if_exists(source);
    remove_if_exists(removed);
    remove_if_exists(output);

    create_cli_table_inspection_fixture(source);

    CHECK_EQ(run_cli({"remove-table-row",
                      source.string(),
                      "0",
                      "0",
                      "--output",
                      removed.string(),
                      "--json"},
                     output),
             0);
    CHECK_EQ(
        read_text_file(output),
        std::string{
            "{\"command\":\"remove-table-row\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"table_index\":0,\"row_index\":0}\n"});

    featherdoc::Document reopened(removed);
    REQUIRE_FALSE(reopened.open());

    const auto table = reopened.inspect_table(0U);
    REQUIRE(table.has_value());
    CHECK_EQ(table->row_count, 1U);
    CHECK_EQ(table->column_count, 2U);

    const auto first_cell = reopened.inspect_table_cell(0U, 0U, 0U);
    REQUIRE(first_cell.has_value());
    CHECK_EQ(first_cell->text, "r1c0");

    const auto second_cell = reopened.inspect_table_cell(0U, 0U, 1U);
    REQUIRE(second_cell.has_value());
    CHECK_EQ(second_cell->text, "r1c1");

    remove_if_exists(source);
    remove_if_exists(removed);
    remove_if_exists(output);
}

TEST_CASE("cli insert-table-before and insert-table-after insert body tables") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_insert_table_source.docx";
    const fs::path inserted_before =
        working_directory / "cli_insert_table_before.docx";
    const fs::path inserted_after =
        working_directory / "cli_insert_table_after.docx";
    const fs::path before_output =
        working_directory / "cli_insert_table_before_output.json";
    const fs::path after_output =
        working_directory / "cli_insert_table_after_output.json";
    const fs::path parse_error_output =
        working_directory / "cli_insert_table_parse_output.json";

    remove_if_exists(source);
    remove_if_exists(inserted_before);
    remove_if_exists(inserted_after);
    remove_if_exists(before_output);
    remove_if_exists(after_output);
    remove_if_exists(parse_error_output);

    create_cli_table_inspection_fixture(source);

    CHECK_EQ(run_cli({"insert-table-before",
                      source.string(),
                      "1",
                      "1",
                      "2",
                      "--output",
                      inserted_before.string(),
                      "--json"},
                     before_output),
             0);
    CHECK_EQ(
        read_text_file(before_output),
        std::string{
            "{\"command\":\"insert-table-before\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"table_index\":1,\"inserted_table_index\":1,"
            "\"row_count\":1,\"column_count\":2}\n"});

    featherdoc::Document reopened_before(inserted_before);
    REQUIRE_FALSE(reopened_before.open());
    const auto before_tables = reopened_before.inspect_tables();
    REQUIRE_EQ(before_tables.size(), 3U);
    CHECK_EQ(before_tables[0].row_count, 2U);
    CHECK_EQ(before_tables[0].column_count, 2U);
    CHECK_NE(before_tables[0].text.find("r0c0"), std::string::npos);
    CHECK_EQ(before_tables[1].row_count, 1U);
    CHECK_EQ(before_tables[1].column_count, 2U);
    const auto before_inserted_first_cell =
        reopened_before.inspect_table_cell(1U, 0U, 0U);
    REQUIRE(before_inserted_first_cell.has_value());
    CHECK_EQ(before_inserted_first_cell->text, "");
    const auto before_inserted_second_cell =
        reopened_before.inspect_table_cell(1U, 0U, 1U);
    REQUIRE(before_inserted_second_cell.has_value());
    CHECK_EQ(before_inserted_second_cell->text, "");
    CHECK_EQ(before_tables[2].row_count, 2U);
    CHECK_EQ(before_tables[2].column_count, 3U);
    CHECK_NE(before_tables[2].text.find("merged-top"), std::string::npos);

    CHECK_EQ(run_cli({"insert-table-after",
                      source.string(),
                      "0",
                      "1",
                      "3",
                      "--output",
                      inserted_after.string(),
                      "--json"},
                     after_output),
             0);
    CHECK_EQ(
        read_text_file(after_output),
        std::string{
            "{\"command\":\"insert-table-after\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"table_index\":0,\"inserted_table_index\":1,"
            "\"row_count\":1,\"column_count\":3}\n"});

    featherdoc::Document reopened_after(inserted_after);
    REQUIRE_FALSE(reopened_after.open());
    const auto after_tables = reopened_after.inspect_tables();
    REQUIRE_EQ(after_tables.size(), 3U);
    CHECK_EQ(after_tables[0].row_count, 2U);
    CHECK_EQ(after_tables[0].column_count, 2U);
    CHECK_NE(after_tables[0].text.find("r0c0"), std::string::npos);
    CHECK_EQ(after_tables[1].row_count, 1U);
    CHECK_EQ(after_tables[1].column_count, 3U);
    const auto after_inserted_first_cell =
        reopened_after.inspect_table_cell(1U, 0U, 0U);
    REQUIRE(after_inserted_first_cell.has_value());
    CHECK_EQ(after_inserted_first_cell->text, "");
    const auto after_inserted_second_cell =
        reopened_after.inspect_table_cell(1U, 0U, 1U);
    REQUIRE(after_inserted_second_cell.has_value());
    CHECK_EQ(after_inserted_second_cell->text, "");
    const auto after_inserted_third_cell =
        reopened_after.inspect_table_cell(1U, 0U, 2U);
    REQUIRE(after_inserted_third_cell.has_value());
    CHECK_EQ(after_inserted_third_cell->text, "");
    CHECK_EQ(after_tables[2].row_count, 2U);
    CHECK_EQ(after_tables[2].column_count, 3U);
    CHECK_NE(after_tables[2].text.find("merged-top"), std::string::npos);

    CHECK_EQ(run_cli({"insert-table-before",
                      source.string(),
                      "0",
                      "0",
                      "2",
                      "--json"},
                     parse_error_output),
             2);
    CHECK_EQ(
        read_text_file(parse_error_output),
        std::string{
            "{\"command\":\"insert-table-before\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"row count must be greater than "
            "0\"}\n"});

    remove_if_exists(source);
    remove_if_exists(inserted_before);
    remove_if_exists(inserted_after);
    remove_if_exists(before_output);
    remove_if_exists(after_output);
    remove_if_exists(parse_error_output);
}

TEST_CASE("cli insert-table-like-before and insert-table-like-after clone body tables") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_insert_table_like_source.docx";
    const fs::path inserted_before =
        working_directory / "cli_insert_table_like_before.docx";
    const fs::path inserted_after =
        working_directory / "cli_insert_table_like_after.docx";
    const fs::path before_output =
        working_directory / "cli_insert_table_like_before_output.json";
    const fs::path after_output =
        working_directory / "cli_insert_table_like_after_output.json";
    const fs::path parse_error_output =
        working_directory / "cli_insert_table_like_parse_output.json";

    remove_if_exists(source);
    remove_if_exists(inserted_before);
    remove_if_exists(inserted_after);
    remove_if_exists(before_output);
    remove_if_exists(after_output);
    remove_if_exists(parse_error_output);

    create_cli_table_inspection_fixture(source);

    CHECK_EQ(run_cli({"insert-table-like-before",
                      source.string(),
                      "1",
                      "--output",
                      inserted_before.string(),
                      "--json"},
                     before_output),
             0);
    CHECK_EQ(
        read_text_file(before_output),
        std::string{
            "{\"command\":\"insert-table-like-before\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"table_index\":1,\"inserted_table_index\":1}\n"});

    featherdoc::Document reopened_before(inserted_before);
    REQUIRE_FALSE(reopened_before.open());
    const auto before_tables = reopened_before.inspect_tables();
    REQUIRE_EQ(before_tables.size(), 3U);
    CHECK_NE(before_tables[0].text.find("r0c0"), std::string::npos);
    CHECK_EQ(before_tables[1].row_count, 2U);
    CHECK_EQ(before_tables[1].column_count, 3U);
    REQUIRE_EQ(before_tables[1].column_widths.size(), 3U);
    REQUIRE(before_tables[1].column_widths[0].has_value());
    REQUIRE(before_tables[1].column_widths[1].has_value());
    REQUIRE(before_tables[1].column_widths[2].has_value());
    CHECK_EQ(*before_tables[1].column_widths[0], 1200U);
    CHECK_EQ(*before_tables[1].column_widths[1], 2200U);
    CHECK_EQ(*before_tables[1].column_widths[2], 3200U);
    CHECK_NE(before_tables[2].text.find("merged-top"), std::string::npos);

    const auto before_cloned_cell =
        reopened_before.inspect_table_cell(1U, 0U, 0U);
    REQUIRE(before_cloned_cell.has_value());
    CHECK_EQ(before_cloned_cell->text, "");
    CHECK_EQ(before_cloned_cell->column_span, 2U);
    REQUIRE(before_cloned_cell->vertical_alignment.has_value());
    CHECK_EQ(*before_cloned_cell->vertical_alignment,
             featherdoc::cell_vertical_alignment::center);
    REQUIRE(before_cloned_cell->text_direction.has_value());
    CHECK_EQ(*before_cloned_cell->text_direction,
             featherdoc::cell_text_direction::top_to_bottom_right_to_left);

    CHECK_EQ(run_cli({"insert-table-like-after",
                      source.string(),
                      "0",
                      "--output",
                      inserted_after.string(),
                      "--json"},
                     after_output),
             0);
    CHECK_EQ(
        read_text_file(after_output),
        std::string{
            "{\"command\":\"insert-table-like-after\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"table_index\":0,\"inserted_table_index\":1}\n"});

    featherdoc::Document reopened_after(inserted_after);
    REQUIRE_FALSE(reopened_after.open());
    const auto after_tables = reopened_after.inspect_tables();
    REQUIRE_EQ(after_tables.size(), 3U);
    CHECK_NE(after_tables[0].text.find("r0c0"), std::string::npos);
    CHECK_EQ(after_tables[1].row_count, 2U);
    CHECK_EQ(after_tables[1].column_count, 2U);
    REQUIRE(after_tables[1].style_id.has_value());
    CHECK_EQ(*after_tables[1].style_id, "TableGrid");
    REQUIRE(after_tables[1].width_twips.has_value());
    CHECK_EQ(*after_tables[1].width_twips, 7200U);
    REQUIRE_EQ(after_tables[1].column_widths.size(), 2U);
    REQUIRE(after_tables[1].column_widths[0].has_value());
    REQUIRE(after_tables[1].column_widths[1].has_value());
    CHECK_EQ(*after_tables[1].column_widths[0], 1800U);
    CHECK_EQ(*after_tables[1].column_widths[1], 5400U);
    CHECK_NE(after_tables[2].text.find("merged-top"), std::string::npos);

    for (std::size_t row_index = 0U; row_index < 2U; ++row_index) {
        for (std::size_t cell_index = 0U; cell_index < 2U; ++cell_index) {
            const auto cell =
                reopened_after.inspect_table_cell(1U, row_index, cell_index);
            REQUIRE(cell.has_value());
            CHECK_EQ(cell->text, "");
        }
    }

    CHECK_EQ(run_cli({"insert-table-like-before", source.string(), "x", "--json"},
                     parse_error_output),
             2);
    CHECK_EQ(
        read_text_file(parse_error_output),
        std::string{
            "{\"command\":\"insert-table-like-before\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"invalid table index: x\"}\n"});

    remove_if_exists(source);
    remove_if_exists(inserted_before);
    remove_if_exists(inserted_after);
    remove_if_exists(before_output);
    remove_if_exists(after_output);
    remove_if_exists(parse_error_output);
}

TEST_CASE("cli insert-paragraph-after-table inserts a body paragraph after a table") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_insert_paragraph_after_table_source.docx";
    const fs::path inserted =
        working_directory / "cli_insert_paragraph_after_table_inserted.docx";
    const fs::path output =
        working_directory / "cli_insert_paragraph_after_table_output.json";
    const fs::path parse_error_output =
        working_directory / "cli_insert_paragraph_after_table_parse_output.json";

    remove_if_exists(source);
    remove_if_exists(inserted);
    remove_if_exists(output);
    remove_if_exists(parse_error_output);

    create_cli_table_inspection_fixture(source);

    CHECK_EQ(run_cli({"insert-paragraph-after-table",
                      source.string(),
                      "0",
                      "--text",
                      "after first table",
                      "--output",
                      inserted.string(),
                      "--json"},
                     output),
             0);
    CHECK_EQ(
        read_text_file(output),
        std::string{
            "{\"command\":\"insert-paragraph-after-table\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"table_index\":0,\"paragraph_index\":1,\"text_length\":17}\n"});

    featherdoc::Document reopened(inserted);
    REQUIRE_FALSE(reopened.open());
    const auto tables = reopened.inspect_tables();
    REQUIRE_EQ(tables.size(), 2U);
    CHECK_NE(tables[0].text.find("r0c0"), std::string::npos);
    CHECK_NE(tables[1].text.find("merged-top"), std::string::npos);

    const auto body_blocks = reopened.inspect_body_blocks();
    const auto table_block = std::find_if(
        body_blocks.begin(), body_blocks.end(), [](const auto &block) {
            return block.kind == featherdoc::body_block_kind::table &&
                   block.item_index == 0U;
        });
    REQUIRE(table_block != body_blocks.end());
    REQUIRE(std::next(table_block) != body_blocks.end());
    CHECK_EQ(std::next(table_block)->kind,
             featherdoc::body_block_kind::paragraph);

    const auto paragraph =
        reopened.inspect_paragraph(std::next(table_block)->item_index);
    REQUIRE(paragraph.has_value());
    CHECK_EQ(paragraph->text, "after first table");

    CHECK_EQ(run_cli({"insert-paragraph-after-table",
                      source.string(),
                      "x",
                      "--text",
                      "after",
                      "--json"},
                     parse_error_output),
             2);
    CHECK_EQ(
        read_text_file(parse_error_output),
        std::string{
            "{\"command\":\"insert-paragraph-after-table\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"invalid table index: x\"}\n"});

    remove_if_exists(source);
    remove_if_exists(inserted);
    remove_if_exists(output);
    remove_if_exists(parse_error_output);
}

TEST_CASE("cli remove-table removes a body table") {
    const fs::path working_directory = fs::current_path();
    const fs::path source = working_directory / "cli_remove_table_source.docx";
    const fs::path removed = working_directory / "cli_remove_table_removed.docx";
    const fs::path output = working_directory / "cli_remove_table_output.json";
    const fs::path parse_error_output =
        working_directory / "cli_remove_table_parse_output.json";

    remove_if_exists(source);
    remove_if_exists(removed);
    remove_if_exists(output);
    remove_if_exists(parse_error_output);

    create_cli_table_inspection_fixture(source);

    CHECK_EQ(run_cli({"remove-table",
                      source.string(),
                      "0",
                      "--output",
                      removed.string(),
                      "--json"},
                     output),
             0);
    CHECK_EQ(
        read_text_file(output),
        std::string{
            "{\"command\":\"remove-table\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"table_index\":0}\n"});

    featherdoc::Document reopened(removed);
    REQUIRE_FALSE(reopened.open());

    const auto tables = reopened.inspect_tables();
    REQUIRE_EQ(tables.size(), 1U);
    CHECK_EQ(tables[0].row_count, 2U);
    CHECK_EQ(tables[0].column_count, 3U);
    CHECK_NE(tables[0].text.find("merged-top"), std::string::npos);

    CHECK_EQ(run_cli({"remove-table", source.string(), "x", "--json"},
                     parse_error_output),
             2);
    CHECK_EQ(
        read_text_file(parse_error_output),
        std::string{
            "{\"command\":\"remove-table\",\"ok\":false,\"stage\":\"parse\","
            "\"message\":\"invalid table index: x\"}\n"});

    remove_if_exists(source);
    remove_if_exists(removed);
    remove_if_exists(output);
    remove_if_exists(parse_error_output);
}

TEST_CASE("cli table column commands insert and remove body columns") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_table_column_source.docx";
    const fs::path inserted_before =
        working_directory / "cli_table_column_before.docx";
    const fs::path inserted_after =
        working_directory / "cli_table_column_after.docx";
    const fs::path removed =
        working_directory / "cli_table_column_removed.docx";
    const fs::path before_output =
        working_directory / "cli_table_column_before_output.json";
    const fs::path after_output =
        working_directory / "cli_table_column_after_output.json";
    const fs::path remove_output =
        working_directory / "cli_table_column_remove_output.json";

    remove_if_exists(source);
    remove_if_exists(inserted_before);
    remove_if_exists(inserted_after);
    remove_if_exists(removed);
    remove_if_exists(before_output);
    remove_if_exists(after_output);
    remove_if_exists(remove_output);

    create_cli_table_inspection_fixture(source);

    CHECK_EQ(run_cli({"insert-table-column-before",
                      source.string(),
                      "0",
                      "0",
                      "1",
                      "--output",
                      inserted_before.string(),
                      "--json"},
                     before_output),
             0);
    CHECK_EQ(
        read_text_file(before_output),
        std::string{
            "{\"command\":\"insert-table-column-before\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"table_index\":0,\"row_index\":0,\"cell_index\":1,"
            "\"inserted_cell_index\":1,\"inserted_column_index\":1}\n"});

    featherdoc::Document reopened_before(inserted_before);
    REQUIRE_FALSE(reopened_before.open());
    const auto before_table = reopened_before.inspect_table(0U);
    REQUIRE(before_table.has_value());
    CHECK_EQ(before_table->row_count, 2U);
    CHECK_EQ(before_table->column_count, 3U);
    REQUIRE(before_table->column_widths.size() >= 3U);
    REQUIRE(before_table->column_widths[0].has_value());
    REQUIRE(before_table->column_widths[1].has_value());
    REQUIRE(before_table->column_widths[2].has_value());
    CHECK_EQ(*before_table->column_widths[0], 1800U);
    CHECK_EQ(*before_table->column_widths[1], 5400U);
    CHECK_EQ(*before_table->column_widths[2], 5400U);
    const auto before_inserted_header =
        reopened_before.inspect_table_cell(0U, 0U, 1U);
    REQUIRE(before_inserted_header.has_value());
    CHECK_EQ(before_inserted_header->text, "");
    const auto before_shifted_header =
        reopened_before.inspect_table_cell(0U, 0U, 2U);
    REQUIRE(before_shifted_header.has_value());
    CHECK_EQ(before_shifted_header->text, "r0c1");
    const auto before_inserted_body =
        reopened_before.inspect_table_cell(0U, 1U, 1U);
    REQUIRE(before_inserted_body.has_value());
    CHECK_EQ(before_inserted_body->text, "");

    CHECK_EQ(run_cli({"insert-table-column-after",
                      source.string(),
                      "0",
                      "0",
                      "0",
                      "--output",
                      inserted_after.string(),
                      "--json"},
                     after_output),
             0);
    CHECK_EQ(
        read_text_file(after_output),
        std::string{
            "{\"command\":\"insert-table-column-after\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"table_index\":0,\"row_index\":0,\"cell_index\":0,"
            "\"inserted_cell_index\":1,\"inserted_column_index\":1}\n"});

    featherdoc::Document reopened_after(inserted_after);
    REQUIRE_FALSE(reopened_after.open());
    const auto after_table = reopened_after.inspect_table(0U);
    REQUIRE(after_table.has_value());
    CHECK_EQ(after_table->row_count, 2U);
    CHECK_EQ(after_table->column_count, 3U);
    REQUIRE(after_table->column_widths.size() >= 3U);
    REQUIRE(after_table->column_widths[0].has_value());
    REQUIRE(after_table->column_widths[1].has_value());
    REQUIRE(after_table->column_widths[2].has_value());
    CHECK_EQ(*after_table->column_widths[0], 1800U);
    CHECK_EQ(*after_table->column_widths[1], 1800U);
    CHECK_EQ(*after_table->column_widths[2], 5400U);
    const auto after_inserted_header =
        reopened_after.inspect_table_cell(0U, 0U, 1U);
    REQUIRE(after_inserted_header.has_value());
    CHECK_EQ(after_inserted_header->text, "");
    const auto after_shifted_header =
        reopened_after.inspect_table_cell(0U, 0U, 2U);
    REQUIRE(after_shifted_header.has_value());
    CHECK_EQ(after_shifted_header->text, "r0c1");

    CHECK_EQ(run_cli({"remove-table-column",
                      source.string(),
                      "0",
                      "0",
                      "0",
                      "--output",
                      removed.string(),
                      "--json"},
                     remove_output),
             0);
    CHECK_EQ(
        read_text_file(remove_output),
        std::string{
            "{\"command\":\"remove-table-column\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"table_index\":0,\"row_index\":0,\"cell_index\":0,"
            "\"column_index\":0}\n"});

    featherdoc::Document reopened_removed(removed);
    REQUIRE_FALSE(reopened_removed.open());
    const auto removed_table = reopened_removed.inspect_table(0U);
    REQUIRE(removed_table.has_value());
    CHECK_EQ(removed_table->row_count, 2U);
    CHECK_EQ(removed_table->column_count, 1U);
    const auto removed_header = reopened_removed.inspect_table_cell(0U, 0U, 0U);
    REQUIRE(removed_header.has_value());
    CHECK_EQ(removed_header->text, "r0c1");
    const auto removed_body = reopened_removed.inspect_table_cell(0U, 1U, 0U);
    REQUIRE(removed_body.has_value());
    CHECK_EQ(removed_body->text, "r1c1");

    remove_if_exists(source);
    remove_if_exists(inserted_before);
    remove_if_exists(inserted_after);
    remove_if_exists(removed);
    remove_if_exists(before_output);
    remove_if_exists(after_output);
    remove_if_exists(remove_output);
}

TEST_CASE("cli table column commands report merge and last-column errors") {
    const fs::path working_directory = fs::current_path();
    const fs::path merged_source =
        working_directory / "cli_table_column_merge_error_source.docx";
    const fs::path single_column_source =
        working_directory / "cli_table_column_single_error_source.docx";
    const fs::path insert_error_output =
        working_directory / "cli_table_column_insert_error.json";
    const fs::path remove_merge_error_output =
        working_directory / "cli_table_column_remove_merge_error.json";
    const fs::path remove_last_error_output =
        working_directory / "cli_table_column_remove_last_error.json";

    remove_if_exists(merged_source);
    remove_if_exists(single_column_source);
    remove_if_exists(insert_error_output);
    remove_if_exists(remove_merge_error_output);
    remove_if_exists(remove_last_error_output);

    create_cli_table_column_horizontal_merge_fixture(merged_source);

    CHECK_EQ(run_cli({"insert-table-column-before",
                      merged_source.string(),
                      "0",
                      "0",
                      "1",
                      "--json"},
                     insert_error_output),
             1);
    const auto insert_error_json = read_text_file(insert_error_output);
    CHECK_NE(insert_error_json.find("\"command\":\"insert-table-column-before\""),
             std::string::npos);
    CHECK_NE(insert_error_json.find("\"stage\":\"mutate\""), std::string::npos);
    CHECK_NE(
        insert_error_json.find(
            "\"detail\":\"cannot insert a column before cell index '1' at row "
            "index '0' in table index '0' because the insertion boundary "
            "intersects a horizontal merge span\""),
        std::string::npos);

    CHECK_EQ(run_cli({"remove-table-column",
                      merged_source.string(),
                      "0",
                      "0",
                      "0",
                      "--json"},
                     remove_merge_error_output),
             1);
    const auto remove_merge_error_json =
        read_text_file(remove_merge_error_output);
    CHECK_NE(remove_merge_error_json.find("\"command\":\"remove-table-column\""),
             std::string::npos);
    CHECK_NE(remove_merge_error_json.find("\"stage\":\"mutate\""),
             std::string::npos);
    CHECK_NE(
        remove_merge_error_json.find(
            "\"detail\":\"cannot remove column index '0' from table index '0' "
            "because it intersects a horizontal merge span\""),
        std::string::npos);

    featherdoc::Document single_column_document(single_column_source);
    REQUIRE_FALSE(single_column_document.create_empty());
    auto single_column_table = single_column_document.append_table(2, 1);
    REQUIRE(single_column_table.has_next());
    populate_table_cells(single_column_table, {{"only-header"}, {"only-body"}});
    REQUIRE_FALSE(single_column_document.save());

    CHECK_EQ(run_cli({"remove-table-column",
                      single_column_source.string(),
                      "0",
                      "0",
                      "0",
                      "--json"},
                     remove_last_error_output),
             1);
    const auto remove_last_error_json =
        read_text_file(remove_last_error_output);
    CHECK_NE(remove_last_error_json.find("\"command\":\"remove-table-column\""),
             std::string::npos);
    CHECK_NE(remove_last_error_json.find("\"stage\":\"mutate\""),
             std::string::npos);
    CHECK_NE(
        remove_last_error_json.find(
            "\"detail\":\"cannot remove the last column from table index '0'\""),
        std::string::npos);

    remove_if_exists(merged_source);
    remove_if_exists(single_column_source);
    remove_if_exists(insert_error_output);
    remove_if_exists(remove_merge_error_output);
    remove_if_exists(remove_last_error_output);
}

TEST_CASE("cli table row structure commands report parse and mutate errors") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_table_row_structure_error_source.docx";
    const fs::path single_row_source =
        working_directory / "cli_table_row_structure_single_row_source.docx";
    const fs::path parse_output =
        working_directory / "cli_table_row_structure_parse_output.json";
    const fs::path merge_error_output =
        working_directory / "cli_table_row_structure_merge_error_output.json";
    const fs::path remove_error_output =
        working_directory / "cli_table_row_structure_remove_error_output.json";

    remove_if_exists(source);
    remove_if_exists(single_row_source);
    remove_if_exists(parse_output);
    remove_if_exists(merge_error_output);
    remove_if_exists(remove_error_output);

    create_cli_table_unmerge_down_fixture(source);

    CHECK_EQ(run_cli({"append-table-row",
                      source.string(),
                      "0",
                      "--cell-count",
                      "0",
                      "--json"},
                     parse_output),
             2);
    CHECK_EQ(
        read_text_file(parse_output),
        std::string{
            "{\"command\":\"append-table-row\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"cell count must be greater than "
            "0\"}\n"});

    CHECK_EQ(run_cli({"insert-table-row-after",
                      source.string(),
                      "0",
                      "0",
                      "--json"},
                     merge_error_output),
             1);
    const auto merge_error_json = read_text_file(merge_error_output);
    CHECK_NE(merge_error_json.find("\"command\":\"insert-table-row-after\""),
             std::string::npos);
    CHECK_NE(merge_error_json.find("\"stage\":\"mutate\""), std::string::npos);
    CHECK_NE(
        merge_error_json.find(
            "\"detail\":\"cannot insert a row adjacent to row index '0' in "
            "table index '0' because the row participates in a vertical "
            "merge\""),
        std::string::npos);

    featherdoc::Document single_row_document(single_row_source);
    REQUIRE_FALSE(single_row_document.create_empty());
    auto single_row_table = single_row_document.append_table(1, 1);
    REQUIRE(single_row_table.has_next());
    auto single_row = single_row_table.rows();
    REQUIRE(single_row.has_next());
    auto only_cell = single_row.cells();
    REQUIRE(only_cell.has_next());
    REQUIRE(only_cell.set_text("only-row"));
    REQUIRE_FALSE(single_row_document.save());

    CHECK_EQ(run_cli({"remove-table-row",
                      single_row_source.string(),
                      "0",
                      "0",
                      "--json"},
                     remove_error_output),
             1);
    const auto remove_error_json = read_text_file(remove_error_output);
    CHECK_NE(remove_error_json.find("\"command\":\"remove-table-row\""),
             std::string::npos);
    CHECK_NE(remove_error_json.find("\"stage\":\"mutate\""), std::string::npos);
    CHECK_NE(
        remove_error_json.find(
            "\"detail\":\"cannot remove the last row from table index '0'\""),
        std::string::npos);

    remove_if_exists(source);
    remove_if_exists(single_row_source);
    remove_if_exists(parse_output);
    remove_if_exists(merge_error_output);
    remove_if_exists(remove_error_output);
}

} // namespace
