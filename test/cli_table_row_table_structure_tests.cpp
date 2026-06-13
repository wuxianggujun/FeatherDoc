#include "cli_table_test_support.hpp"

namespace {
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


} // namespace
