#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "cli_table_test_support.hpp"

namespace {
TEST_CASE("cli table geometry commands set and clear body table width and layout mode") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_table_geometry_source.docx";
    const fs::path width_set =
        working_directory / "cli_table_geometry_width_set.docx";
    const fs::path width_cleared =
        working_directory / "cli_table_geometry_width_cleared.docx";
    const fs::path layout_set =
        working_directory / "cli_table_geometry_layout_set.docx";
    const fs::path layout_cleared =
        working_directory / "cli_table_geometry_layout_cleared.docx";
    const fs::path width_set_output =
        working_directory / "cli_table_geometry_width_set_output.json";
    const fs::path width_clear_output =
        working_directory / "cli_table_geometry_width_clear_output.json";
    const fs::path layout_set_output =
        working_directory / "cli_table_geometry_layout_set_output.json";
    const fs::path layout_clear_output =
        working_directory / "cli_table_geometry_layout_clear_output.json";
    const fs::path parse_error_output =
        working_directory / "cli_table_geometry_parse_error_output.json";
    const fs::path range_error_output =
        working_directory / "cli_table_geometry_range_error_output.json";

    remove_if_exists(source);
    remove_if_exists(width_set);
    remove_if_exists(width_cleared);
    remove_if_exists(layout_set);
    remove_if_exists(layout_cleared);
    remove_if_exists(width_set_output);
    remove_if_exists(width_clear_output);
    remove_if_exists(layout_set_output);
    remove_if_exists(layout_clear_output);
    remove_if_exists(parse_error_output);
    remove_if_exists(range_error_output);

    create_cli_table_inspection_fixture(source);

    CHECK_EQ(run_cli({"set-table-width",
                      source.string(),
                      "0",
                      "6600",
                      "--output",
                      width_set.string(),
                      "--json"},
                     width_set_output),
             0);
    CHECK_EQ(
        read_text_file(width_set_output),
        std::string{
            "{\"command\":\"set-table-width\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"table_index\":0,\"width_twips\":6600}\n"});

    featherdoc::Document reopened_width_set(width_set);
    REQUIRE_FALSE(reopened_width_set.open());
    const auto sized_table = reopened_width_set.inspect_table(0U);
    REQUIRE(sized_table.has_value());
    REQUIRE(sized_table->width_twips.has_value());
    CHECK_EQ(*sized_table->width_twips, 6600U);

    CHECK_EQ(run_cli({"clear-table-width",
                      width_set.string(),
                      "0",
                      "--output",
                      width_cleared.string(),
                      "--json"},
                     width_clear_output),
             0);
    CHECK_EQ(
        read_text_file(width_clear_output),
        std::string{
            "{\"command\":\"clear-table-width\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"table_index\":0}\n"});

    featherdoc::Document reopened_width_cleared(width_cleared);
    REQUIRE_FALSE(reopened_width_cleared.open());
    const auto cleared_width_table = reopened_width_cleared.inspect_table(0U);
    REQUIRE(cleared_width_table.has_value());
    CHECK_FALSE(cleared_width_table->width_twips.has_value());

    CHECK_EQ(run_cli({"set-table-layout-mode",
                      width_cleared.string(),
                      "0",
                      "fixed",
                      "--output",
                      layout_set.string(),
                      "--json"},
                     layout_set_output),
             0);
    CHECK_EQ(
        read_text_file(layout_set_output),
        std::string{
            "{\"command\":\"set-table-layout-mode\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"table_index\":0,\"layout_mode\":\"fixed\"}\n"});

    featherdoc::Document reopened_layout_set(layout_set);
    REQUIRE_FALSE(reopened_layout_set.open());
    auto layout_table = reopened_layout_set.tables();
    REQUIRE(layout_table.has_next());
    REQUIRE(layout_table.layout_mode().has_value());
    CHECK_EQ(*layout_table.layout_mode(), featherdoc::table_layout_mode::fixed);

    CHECK_EQ(run_cli({"clear-table-layout-mode",
                      layout_set.string(),
                      "0",
                      "--output",
                      layout_cleared.string(),
                      "--json"},
                     layout_clear_output),
             0);
    CHECK_EQ(
        read_text_file(layout_clear_output),
        std::string{
            "{\"command\":\"clear-table-layout-mode\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"table_index\":0}\n"});

    featherdoc::Document reopened_layout_cleared(layout_cleared);
    REQUIRE_FALSE(reopened_layout_cleared.open());
    auto cleared_layout_table = reopened_layout_cleared.tables();
    REQUIRE(cleared_layout_table.has_next());
    CHECK_FALSE(cleared_layout_table.layout_mode().has_value());

    CHECK_EQ(run_cli({"set-table-layout-mode",
                      source.string(),
                      "0",
                      "fluid",
                      "--json"},
                     parse_error_output),
             2);
    CHECK_EQ(
        read_text_file(parse_error_output),
        std::string{
            "{\"command\":\"set-table-layout-mode\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"invalid table layout mode: fluid\"}\n"});

    CHECK_EQ(
        run_cli({"clear-table-width", source.string(), "9", "--json"},
                range_error_output),
        1);
    const auto range_error_json = read_text_file(range_error_output);
    CHECK_NE(range_error_json.find("\"command\":\"clear-table-width\""),
             std::string::npos);
    CHECK_NE(range_error_json.find("\"stage\":\"mutate\""), std::string::npos);
    CHECK_NE(range_error_json.find("\"detail\":\"table index '9' is out of range\""),
             std::string::npos);

    remove_if_exists(source);
    remove_if_exists(width_set);
    remove_if_exists(width_cleared);
    remove_if_exists(layout_set);
    remove_if_exists(layout_cleared);
    remove_if_exists(width_set_output);
    remove_if_exists(width_clear_output);
    remove_if_exists(layout_set_output);
    remove_if_exists(layout_clear_output);
    remove_if_exists(parse_error_output);
    remove_if_exists(range_error_output);
}

TEST_CASE("cli table placement commands set and clear body table metadata") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_table_placement_source.docx";
    const fs::path aligned =
        working_directory / "cli_table_placement_aligned.docx";
    const fs::path alignment_cleared =
        working_directory / "cli_table_placement_alignment_cleared.docx";
    const fs::path indented =
        working_directory / "cli_table_placement_indented.docx";
    const fs::path indent_cleared =
        working_directory / "cli_table_placement_indent_cleared.docx";
    const fs::path spaced =
        working_directory / "cli_table_placement_spaced.docx";
    const fs::path spacing_cleared =
        working_directory / "cli_table_placement_spacing_cleared.docx";
    const fs::path alignment_output =
        working_directory / "cli_table_placement_alignment_output.json";
    const fs::path alignment_clear_output =
        working_directory / "cli_table_placement_alignment_clear_output.json";
    const fs::path indent_output =
        working_directory / "cli_table_placement_indent_output.json";
    const fs::path indent_clear_output =
        working_directory / "cli_table_placement_indent_clear_output.json";
    const fs::path spacing_output =
        working_directory / "cli_table_placement_spacing_output.json";
    const fs::path spacing_clear_output =
        working_directory / "cli_table_placement_spacing_clear_output.json";
    const fs::path parse_error_output =
        working_directory / "cli_table_placement_parse_error_output.json";

    remove_if_exists(source);
    remove_if_exists(aligned);
    remove_if_exists(alignment_cleared);
    remove_if_exists(indented);
    remove_if_exists(indent_cleared);
    remove_if_exists(spaced);
    remove_if_exists(spacing_cleared);
    remove_if_exists(alignment_output);
    remove_if_exists(alignment_clear_output);
    remove_if_exists(indent_output);
    remove_if_exists(indent_clear_output);
    remove_if_exists(spacing_output);
    remove_if_exists(spacing_clear_output);
    remove_if_exists(parse_error_output);

    create_cli_table_inspection_fixture(source);

    CHECK_EQ(run_cli({"set-table-alignment",
                      source.string(),
                      "0",
                      "center",
                      "--output",
                      aligned.string(),
                      "--json"},
                     alignment_output),
             0);
    CHECK_EQ(
        read_text_file(alignment_output),
        std::string{
            "{\"command\":\"set-table-alignment\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"table_index\":0,\"alignment\":\"center\"}\n"});

    featherdoc::Document reopened_aligned(aligned);
    REQUIRE_FALSE(reopened_aligned.open());
    const auto aligned_table = reopened_aligned.inspect_table(0U);
    REQUIRE(aligned_table.has_value());
    REQUIRE(aligned_table->alignment.has_value());
    CHECK_EQ(*aligned_table->alignment, featherdoc::table_alignment::center);

    CHECK_EQ(run_cli({"clear-table-alignment",
                      aligned.string(),
                      "0",
                      "--output",
                      alignment_cleared.string(),
                      "--json"},
                     alignment_clear_output),
             0);
    CHECK_EQ(
        read_text_file(alignment_clear_output),
        std::string{
            "{\"command\":\"clear-table-alignment\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"table_index\":0}\n"});

    featherdoc::Document reopened_alignment_cleared(alignment_cleared);
    REQUIRE_FALSE(reopened_alignment_cleared.open());
    const auto alignment_cleared_table =
        reopened_alignment_cleared.inspect_table(0U);
    REQUIRE(alignment_cleared_table.has_value());
    CHECK_FALSE(alignment_cleared_table->alignment.has_value());

    CHECK_EQ(run_cli({"set-table-indent",
                      source.string(),
                      "0",
                      "360",
                      "--output",
                      indented.string(),
                      "--json"},
                     indent_output),
             0);
    CHECK_EQ(
        read_text_file(indent_output),
        std::string{
            "{\"command\":\"set-table-indent\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"table_index\":0,\"indent_twips\":360}\n"});

    featherdoc::Document reopened_indented(indented);
    REQUIRE_FALSE(reopened_indented.open());
    const auto indented_table = reopened_indented.inspect_table(0U);
    REQUIRE(indented_table.has_value());
    REQUIRE(indented_table->indent_twips.has_value());
    CHECK_EQ(*indented_table->indent_twips, 360U);

    CHECK_EQ(run_cli({"clear-table-indent",
                      indented.string(),
                      "0",
                      "--output",
                      indent_cleared.string(),
                      "--json"},
                     indent_clear_output),
             0);
    CHECK_EQ(
        read_text_file(indent_clear_output),
        std::string{
            "{\"command\":\"clear-table-indent\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"table_index\":0}\n"});

    featherdoc::Document reopened_indent_cleared(indent_cleared);
    REQUIRE_FALSE(reopened_indent_cleared.open());
    const auto indent_cleared_table = reopened_indent_cleared.inspect_table(0U);
    REQUIRE(indent_cleared_table.has_value());
    CHECK_FALSE(indent_cleared_table->indent_twips.has_value());

    CHECK_EQ(run_cli({"set-table-cell-spacing",
                      source.string(),
                      "0",
                      "180",
                      "--output",
                      spaced.string(),
                      "--json"},
                     spacing_output),
             0);
    CHECK_EQ(
        read_text_file(spacing_output),
        std::string{
            "{\"command\":\"set-table-cell-spacing\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"table_index\":0,\"cell_spacing_twips\":180}\n"});

    featherdoc::Document reopened_spaced(spaced);
    REQUIRE_FALSE(reopened_spaced.open());
    const auto spaced_table = reopened_spaced.inspect_table(0U);
    REQUIRE(spaced_table.has_value());
    REQUIRE(spaced_table->cell_spacing_twips.has_value());
    CHECK_EQ(*spaced_table->cell_spacing_twips, 180U);

    CHECK_EQ(run_cli({"clear-table-cell-spacing",
                      spaced.string(),
                      "0",
                      "--output",
                      spacing_cleared.string(),
                      "--json"},
                     spacing_clear_output),
             0);
    CHECK_EQ(
        read_text_file(spacing_clear_output),
        std::string{
            "{\"command\":\"clear-table-cell-spacing\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"table_index\":0}\n"});

    featherdoc::Document reopened_spacing_cleared(spacing_cleared);
    REQUIRE_FALSE(reopened_spacing_cleared.open());
    const auto spacing_cleared_table =
        reopened_spacing_cleared.inspect_table(0U);
    REQUIRE(spacing_cleared_table.has_value());
    CHECK_FALSE(spacing_cleared_table->cell_spacing_twips.has_value());

    CHECK_EQ(run_cli({"set-table-alignment",
                      source.string(),
                      "0",
                      "middle",
                      "--json"},
                     parse_error_output),
             2);
    CHECK_EQ(
        read_text_file(parse_error_output),
        std::string{
            "{\"command\":\"set-table-alignment\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"invalid table alignment: middle\"}\n"});

    remove_if_exists(source);
    remove_if_exists(aligned);
    remove_if_exists(alignment_cleared);
    remove_if_exists(indented);
    remove_if_exists(indent_cleared);
    remove_if_exists(spaced);
    remove_if_exists(spacing_cleared);
    remove_if_exists(alignment_output);
    remove_if_exists(alignment_clear_output);
    remove_if_exists(indent_output);
    remove_if_exists(indent_clear_output);
    remove_if_exists(spacing_output);
    remove_if_exists(spacing_clear_output);
    remove_if_exists(parse_error_output);
}

} // namespace
