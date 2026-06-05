#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "featherdoc_cli_style_ensure_options_parse.hpp"

#include <string>
#include <string_view>
#include <vector>

TEST_CASE("cli style ensure options parse accepts paragraph and character styles") {
    std::string error;

    featherdoc_cli::ensure_paragraph_style_options paragraph;
    CHECK(featherdoc_cli::parse_ensure_paragraph_style_options(
        {"ensure-paragraph-style", "input.docx", "Heading1", "--name",
         "Heading1", "--based-on", "BaseStyle", "--custom", "false",
         "--semi-hidden", "true", "--unhide-when-used", "true",
         "--quick-format", "false", "--run-font-family", "Aptos",
         "--run-east-asia-font-family", "Microsoft YaHei", "--run-language",
         "en-US", "--run-east-asia-language", "zh-CN", "--run-bidi-language",
         "ar-SA", "--run-rtl", "true", "--next-style", "BodyText",
         "--paragraph-bidi", "false", "--outline-level", "2", "--output",
         "out.docx", "--json"},
        3U, paragraph, error));
    CHECK(paragraph.definition.name == "Heading1");
    REQUIRE(paragraph.definition.based_on.has_value());
    CHECK(*paragraph.definition.based_on == "BaseStyle");
    CHECK_FALSE(paragraph.definition.is_custom);
    CHECK(paragraph.definition.is_semi_hidden);
    CHECK(paragraph.definition.is_unhide_when_used);
    CHECK_FALSE(paragraph.definition.is_quick_format);
    REQUIRE(paragraph.definition.run_font_family.has_value());
    CHECK(*paragraph.definition.run_font_family == "Aptos");
    REQUIRE(paragraph.definition.run_rtl.has_value());
    CHECK(*paragraph.definition.run_rtl);
    REQUIRE(paragraph.definition.next_style.has_value());
    CHECK(*paragraph.definition.next_style == "BodyText");
    REQUIRE(paragraph.definition.paragraph_bidi.has_value());
    CHECK_FALSE(*paragraph.definition.paragraph_bidi);
    REQUIRE(paragraph.definition.outline_level.has_value());
    CHECK(*paragraph.definition.outline_level == 2U);
    REQUIRE(paragraph.output_path.has_value());
    CHECK(paragraph.output_path->filename().string() == "out.docx");
    CHECK(paragraph.json_output);

    featherdoc_cli::ensure_character_style_options character;
    error.clear();
    CHECK(featherdoc_cli::parse_ensure_character_style_options(
        {"ensure-character-style", "input.docx", "Emphasis", "--name",
         "Emphasis", "--based-on", "BaseCharacter", "--run-font-family",
         "Aptos", "--run-rtl", "false", "--output", "out.docx", "--json"},
        3U, character, error));
    CHECK(character.definition.name == "Emphasis");
    REQUIRE(character.definition.based_on.has_value());
    CHECK(*character.definition.based_on == "BaseCharacter");
    REQUIRE(character.definition.run_font_family.has_value());
    CHECK(*character.definition.run_font_family == "Aptos");
    REQUIRE(character.definition.run_rtl.has_value());
    CHECK_FALSE(*character.definition.run_rtl);
    REQUIRE(character.output_path.has_value());
    CHECK(character.output_path->filename().string() == "out.docx");
    CHECK(character.json_output);
}

TEST_CASE("cli style ensure options parse accepts table styles") {
    std::string error;

    featherdoc_cli::ensure_table_style_options table;
    CHECK(featherdoc_cli::parse_ensure_table_style_options(
        {"ensure-table-style", "input.docx", "TableStyle", "--name",
         "TableStyle", "--style-fill", "whole-table:#ffffff",
         "--style-text-color", "first-row:#000000", "--style-bold",
         "last-row:true", "--style-italic", "first-column:false",
         "--style-font-size", "banded-rows:11", "--style-font-family",
         "banded-columns:Aptos", "--style-east-asia-font-family",
         "second-banded-rows:Microsoft YaHei", "--style-cell-vertical-alignment",
         "whole-table:center", "--style-cell-text-direction",
         "whole-table:left_to_right_top_to_bottom", "--style-paragraph-alignment",
         "whole-table:justified", "--style-paragraph-spacing-before",
         "whole-table:120", "--style-paragraph-spacing-after",
         "whole-table:240", "--style-paragraph-line-spacing",
         "whole-table:360:auto", "--style-margin", "whole-table:top:72",
         "--style-border", "whole-table:top:single:8:#000000", "--output",
         "out.docx", "--json"},
        3U, table, error));
    CHECK(table.definition.name == "TableStyle");
    REQUIRE(table.definition.whole_table.has_value());
    CHECK(table.definition.whole_table->fill_color.has_value());
    CHECK(*table.definition.whole_table->fill_color == "#ffffff");
    CHECK(table.definition.whole_table->paragraph_alignment.has_value());
    CHECK(*table.definition.whole_table->paragraph_alignment ==
          featherdoc::paragraph_alignment::justified);
    CHECK(table.definition.whole_table->cell_vertical_alignment.has_value());
    CHECK(*table.definition.whole_table->cell_vertical_alignment ==
          featherdoc::cell_vertical_alignment::center);
    CHECK(table.definition.whole_table->borders.has_value());
    REQUIRE(table.definition.whole_table->borders->top.has_value());
    CHECK(table.definition.whole_table->borders->top->style ==
          featherdoc::border_style::single);
    CHECK(table.definition.whole_table->borders->top->size_eighth_points == 8U);
    CHECK(table.definition.whole_table->borders->top->color == "#000000");
    CHECK(table.definition.whole_table->borders->top->space_points == 0U);
    REQUIRE(table.output_path.has_value());
    CHECK(table.output_path->filename().string() == "out.docx");
    CHECK(table.json_output);
}

TEST_CASE("cli style ensure options parse reports validation errors") {
    featherdoc_cli::ensure_paragraph_style_options paragraph;
    std::string paragraph_error;
    CHECK_FALSE(featherdoc_cli::parse_ensure_paragraph_style_options(
        {"ensure-paragraph-style", "input.docx", "Heading1", "--json"}, 3U,
        paragraph, paragraph_error));
    CHECK(paragraph_error == "missing required --name <name>");

    featherdoc_cli::ensure_character_style_options character;
    std::string character_error;
    CHECK_FALSE(featherdoc_cli::parse_ensure_character_style_options(
        {"ensure-character-style", "input.docx", "Emphasis", "--name",
         "Emphasis", "--run-rtl", "maybe"},
        3U, character, character_error));
    CHECK(character_error == "invalid --run-rtl value: maybe");

    featherdoc_cli::ensure_table_style_options table;
    std::string table_error;
    CHECK_FALSE(featherdoc_cli::parse_ensure_table_style_options(
        {"ensure-table-style", "input.docx", "TableStyle", "--name",
         "TableStyle", "--style-fill", "bogus:#ffffff"},
        3U, table, table_error));
    CHECK(table_error == "invalid --style-fill region: bogus");
}
