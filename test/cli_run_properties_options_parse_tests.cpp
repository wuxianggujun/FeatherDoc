#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "featherdoc_cli_run_properties_options_parse.hpp"

#include <string>
#include <string_view>
#include <vector>

TEST_CASE("cli run properties options parse accepts inspect and output-only commands") {
    featherdoc_cli::inspect_default_run_properties_options inspect_default;
    std::string error;

    CHECK(featherdoc_cli::parse_inspect_default_run_properties_options(
        {"inspect-default-run-properties", "input.docx", "--json"}, 2U,
        inspect_default, error));
    CHECK(inspect_default.json_output);

    featherdoc_cli::inspect_style_run_properties_options inspect_style;
    error.clear();
    CHECK(featherdoc_cli::parse_inspect_style_run_properties_options(
        {"inspect-style-run-properties", "input.docx", "Heading1", "--json"},
        3U, inspect_style, error));
    CHECK(inspect_style.json_output);

    featherdoc_cli::inspect_paragraph_style_properties_options inspect_paragraph;
    error.clear();
    CHECK(featherdoc_cli::parse_inspect_paragraph_style_properties_options(
        {"inspect-paragraph-style-properties", "input.docx", "Heading1",
         "--json"},
        3U, inspect_paragraph, error));
    CHECK(inspect_paragraph.json_output);

    featherdoc_cli::materialize_style_run_properties_options materialize;
    error.clear();
    CHECK(featherdoc_cli::parse_materialize_style_run_properties_options(
        {"materialize-style-run-properties", "input.docx", "Heading1",
         "--output", "out.docx", "--json"},
        3U, materialize, error));
    REQUIRE(materialize.output_path.has_value());
    CHECK(materialize.output_path->filename().string() == "out.docx");
    CHECK(materialize.json_output);

    featherdoc_cli::rebase_style_based_on_options rebase;
    error.clear();
    CHECK(featherdoc_cli::parse_rebase_style_based_on_options(
        {"rebase-style-based-on", "input.docx", "Heading1", "BaseStyle",
         "--output", "out.docx", "--json"},
        4U, rebase, error));
    REQUIRE(rebase.output_path.has_value());
    CHECK(rebase.output_path->filename().string() == "out.docx");
    CHECK(rebase.json_output);
}

TEST_CASE("cli run properties options parse accepts default and style run mutations") {
    featherdoc_cli::set_default_run_properties_options default_set;
    std::string default_set_error;
    CHECK(featherdoc_cli::parse_set_default_run_properties_options(
        {"set-default-run-properties", "input.docx", "--font-family", "Aptos",
         "--east-asia-font-family", "Microsoft YaHei", "--language", "en-US",
         "--east-asia-language", "zh-CN", "--bidi-language", "ar-SA",
         "--rtl", "true", "--paragraph-bidi", "false", "--output",
         "out.docx", "--json"},
        2U, default_set, default_set_error));
    REQUIRE(default_set.font_family.has_value());
    REQUIRE(default_set.east_asia_font_family.has_value());
    REQUIRE(default_set.language.has_value());
    REQUIRE(default_set.east_asia_language.has_value());
    REQUIRE(default_set.bidi_language.has_value());
    REQUIRE(default_set.rtl.has_value());
    REQUIRE(default_set.paragraph_bidi.has_value());
    CHECK(*default_set.font_family == "Aptos");
    CHECK(*default_set.east_asia_font_family == "Microsoft YaHei");
    CHECK(*default_set.language == "en-US");
    CHECK(*default_set.east_asia_language == "zh-CN");
    CHECK(*default_set.bidi_language == "ar-SA");
    CHECK(*default_set.rtl);
    CHECK_FALSE(*default_set.paragraph_bidi);
    REQUIRE(default_set.output_path.has_value());
    CHECK(default_set.output_path->filename().string() == "out.docx");
    CHECK(default_set.json_output);

    featherdoc_cli::clear_default_run_properties_options default_clear;
    std::string default_clear_error;
    CHECK(featherdoc_cli::parse_clear_default_run_properties_options(
        {"clear-default-run-properties", "input.docx", "--font-family",
         "--east-asia-font-family", "--primary-language", "--language",
         "--east-asia-language", "--bidi-language", "--rtl",
         "--paragraph-bidi", "--output", "out.docx", "--json"},
        2U, default_clear, default_clear_error));
    CHECK(default_clear.clear_font_family);
    CHECK(default_clear.clear_east_asia_font_family);
    CHECK(default_clear.clear_primary_language);
    CHECK(default_clear.clear_language);
    CHECK(default_clear.clear_east_asia_language);
    CHECK(default_clear.clear_bidi_language);
    CHECK(default_clear.clear_rtl);
    CHECK(default_clear.clear_paragraph_bidi);
    REQUIRE(default_clear.output_path.has_value());
    CHECK(default_clear.output_path->filename().string() == "out.docx");
    CHECK(default_clear.json_output);

    featherdoc_cli::set_style_run_properties_options style_set;
    std::string style_set_error;
    CHECK(featherdoc_cli::parse_set_style_run_properties_options(
        {"set-style-run-properties", "input.docx", "Heading1", "--font-family",
         "Aptos", "--east-asia-font-family", "Microsoft YaHei", "--language",
         "en-US", "--east-asia-language", "zh-CN", "--bidi-language",
         "ar-SA", "--rtl", "true", "--paragraph-bidi", "false", "--output",
         "out.docx", "--json"},
        3U, style_set, style_set_error));
    REQUIRE(style_set.font_family.has_value());
    REQUIRE(style_set.rtl.has_value());
    CHECK(*style_set.font_family == "Aptos");
    CHECK(*style_set.rtl);
    CHECK_FALSE(*style_set.paragraph_bidi);
    REQUIRE(style_set.output_path.has_value());
    CHECK(style_set.output_path->filename().string() == "out.docx");
    CHECK(style_set.json_output);

    featherdoc_cli::clear_style_run_properties_options style_clear;
    std::string style_clear_error;
    CHECK(featherdoc_cli::parse_clear_style_run_properties_options(
        {"clear-style-run-properties", "input.docx", "Heading1",
         "--font-family", "--east-asia-font-family", "--primary-language",
         "--language", "--east-asia-language", "--bidi-language", "--rtl",
         "--paragraph-bidi", "--output", "out.docx", "--json"},
        3U, style_clear, style_clear_error));
    CHECK(style_clear.clear_font_family);
    CHECK(style_clear.clear_east_asia_font_family);
    CHECK(style_clear.clear_primary_language);
    CHECK(style_clear.clear_language);
    CHECK(style_clear.clear_east_asia_language);
    CHECK(style_clear.clear_bidi_language);
    CHECK(style_clear.clear_rtl);
    CHECK(style_clear.clear_paragraph_bidi);
    REQUIRE(style_clear.output_path.has_value());
    CHECK(style_clear.output_path->filename().string() == "out.docx");
    CHECK(style_clear.json_output);
}

TEST_CASE("cli run properties options parse accepts paragraph style property mutations") {
    featherdoc_cli::set_paragraph_style_properties_options paragraph_set;
    std::string paragraph_set_error;
    CHECK(featherdoc_cli::parse_set_paragraph_style_properties_options(
        {"set-paragraph-style-properties", "input.docx", "Heading1",
         "--based-on", "BaseStyle", "--next-style", "BodyText",
         "--outline-level", "2", "--output", "out.docx", "--json"},
        3U, paragraph_set, paragraph_set_error));
    REQUIRE(paragraph_set.based_on.has_value());
    REQUIRE(paragraph_set.next_style.has_value());
    REQUIRE(paragraph_set.outline_level.has_value());
    CHECK(*paragraph_set.based_on == "BaseStyle");
    CHECK(*paragraph_set.next_style == "BodyText");
    CHECK(*paragraph_set.outline_level == 2U);
    REQUIRE(paragraph_set.output_path.has_value());
    CHECK(paragraph_set.output_path->filename().string() == "out.docx");
    CHECK(paragraph_set.json_output);

    featherdoc_cli::clear_paragraph_style_properties_options paragraph_clear;
    std::string paragraph_clear_error;
    CHECK(featherdoc_cli::parse_clear_paragraph_style_properties_options(
        {"clear-paragraph-style-properties", "input.docx", "Heading1",
         "--based-on", "--next-style", "--outline-level", "--output",
         "out.docx", "--json"},
        3U, paragraph_clear, paragraph_clear_error));
    CHECK(paragraph_clear.clear_based_on);
    CHECK(paragraph_clear.clear_next_style);
    CHECK(paragraph_clear.clear_outline_level);
    REQUIRE(paragraph_clear.output_path.has_value());
    CHECK(paragraph_clear.output_path->filename().string() == "out.docx");
    CHECK(paragraph_clear.json_output);
}

TEST_CASE("cli run properties options parse validates shared errors") {
    featherdoc_cli::set_default_run_properties_options default_set;
    std::string default_set_error;
    CHECK_FALSE(featherdoc_cli::parse_set_default_run_properties_options(
        {"set-default-run-properties", "input.docx", "--json"}, 2U,
        default_set, default_set_error));
    CHECK(default_set_error ==
          "set-default-run-properties requires at least one mutation option");

    featherdoc_cli::clear_default_run_properties_options default_clear;
    std::string default_clear_error;
    CHECK_FALSE(featherdoc_cli::parse_clear_default_run_properties_options(
        {"clear-default-run-properties", "input.docx", "--json"}, 2U,
        default_clear, default_clear_error));
    CHECK(default_clear_error ==
          "clear-default-run-properties requires at least one clear option");

    featherdoc_cli::set_paragraph_style_properties_options paragraph_set;
    std::string paragraph_set_error;
    CHECK_FALSE(featherdoc_cli::parse_set_paragraph_style_properties_options(
        {"set-paragraph-style-properties", "input.docx", "Heading1",
         "--outline-level", "9"},
        3U, paragraph_set, paragraph_set_error));
    CHECK(paragraph_set_error ==
          "invalid --outline-level value: expected 0-8");

    featherdoc_cli::clear_paragraph_style_properties_options paragraph_clear;
    std::string paragraph_clear_error;
    CHECK_FALSE(featherdoc_cli::parse_clear_paragraph_style_properties_options(
        {"clear-paragraph-style-properties", "input.docx", "Heading1",
         "--json"},
        3U, paragraph_clear, paragraph_clear_error));
    CHECK(paragraph_clear_error ==
          "clear-paragraph-style-properties requires at least one clear option");

    featherdoc_cli::set_style_run_properties_options style_set;
    std::string style_set_error;
    CHECK_FALSE(featherdoc_cli::parse_set_style_run_properties_options(
        {"set-style-run-properties", "input.docx", "Heading1", "--bogus"},
        3U, style_set, style_set_error));
    CHECK(style_set_error == "unknown option: --bogus");

    featherdoc_cli::clear_style_run_properties_options style_clear;
    std::string style_clear_error;
    CHECK_FALSE(featherdoc_cli::parse_clear_style_run_properties_options(
        {"clear-style-run-properties", "input.docx", "Heading1", "--json"},
        3U, style_clear, style_clear_error));
    CHECK(style_clear_error ==
          "clear-style-run-properties requires at least one clear option");
}
