#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "featherdoc_cli_numbering_options_parse.hpp"

#include <featherdoc.hpp>

#include <string>
#include <string_view>
#include <vector>

TEST_CASE("cli numbering options parse accepts numbering level definition") {
    featherdoc::numbering_level_definition definition;
    std::string error;

    CHECK(featherdoc_cli::parse_numbering_level_definition(
        "2:decimal:4:%1:%2", definition, error));

    CHECK(definition.level == 2U);
    CHECK(definition.kind == featherdoc::list_kind::decimal);
    CHECK(definition.start == 4U);
    CHECK(definition.text_pattern == "%1:%2");
}

TEST_CASE("cli numbering options parse validates numbering level definition") {
    featherdoc::numbering_level_definition missing_fields;
    std::string missing_fields_error;
    CHECK_FALSE(featherdoc_cli::parse_numbering_level_definition(
        "0:decimal:1", missing_fields, missing_fields_error));
    CHECK(missing_fields_error ==
          "invalid --numbering-level value: expected "
          "<level>:<kind>:<start>:<text-pattern>");

    featherdoc::numbering_level_definition empty_pattern;
    std::string empty_pattern_error;
    CHECK_FALSE(featherdoc_cli::parse_numbering_level_definition(
        "0:decimal:1:", empty_pattern, empty_pattern_error));
    CHECK(empty_pattern_error ==
          "invalid --numbering-level value: text pattern must not be empty");

    featherdoc::numbering_level_definition bad_kind;
    std::string bad_kind_error;
    CHECK_FALSE(featherdoc_cli::parse_numbering_level_definition(
        "0:roman:1:%1.", bad_kind, bad_kind_error));
    CHECK(bad_kind_error == "invalid numbering kind: roman");
}

TEST_CASE("cli numbering options parse accepts style link") {
    featherdoc::paragraph_style_numbering_link link;
    std::string error;

    CHECK(featherdoc_cli::parse_paragraph_style_numbering_link(
        "Heading1:0", link, error));

    CHECK(link.style_id == "Heading1");
    CHECK(link.level == 0U);
}

TEST_CASE("cli numbering options parse validates style link") {
    featherdoc::paragraph_style_numbering_link no_separator;
    std::string no_separator_error;
    CHECK_FALSE(featherdoc_cli::parse_paragraph_style_numbering_link(
        "Heading1", no_separator, no_separator_error));
    CHECK(no_separator_error ==
          "invalid --style-link value: expected <style-id>:<level>");

    featherdoc::paragraph_style_numbering_link empty_style;
    std::string empty_style_error;
    CHECK_FALSE(featherdoc_cli::parse_paragraph_style_numbering_link(
        ":0", empty_style, empty_style_error));
    CHECK(empty_style_error ==
          "invalid --style-link value: style id must not be empty");

    featherdoc::paragraph_style_numbering_link bad_level;
    std::string bad_level_error;
    CHECK_FALSE(featherdoc_cli::parse_paragraph_style_numbering_link(
        "Heading1:x", bad_level, bad_level_error));
    CHECK(bad_level_error == "invalid style link level: x");
}

TEST_CASE("cli numbering options parse accepts style numbering command options") {
    featherdoc_cli::set_paragraph_style_numbering_options options;
    std::string error;

    CHECK(featherdoc_cli::parse_set_paragraph_style_numbering_options(
        {"set-paragraph-style-numbering", "input.docx", "Heading1",
         "--definition-name", "Legal", "--numbering-level", "0:decimal:1:%1.",
         "--style-level", "0", "--output", "out.docx", "--json"},
        3U, options, error));

    REQUIRE(options.definition_name.has_value());
    CHECK(*options.definition_name == "Legal");
    REQUIRE(options.levels.size() == 1U);
    CHECK(options.levels[0].text_pattern == "%1.");
    REQUIRE(options.style_level.has_value());
    CHECK(*options.style_level == 0U);
    REQUIRE(options.output_path.has_value());
    CHECK(options.output_path->filename().string() == "out.docx");
    CHECK(options.json_output);
}

TEST_CASE("cli numbering options parse accepts ensure linked numbering options") {
    featherdoc_cli::ensure_style_linked_numbering_options options;
    std::string error;

    CHECK(featherdoc_cli::parse_ensure_style_linked_numbering_options(
        {"ensure-style-linked-numbering", "input.docx", "--definition-name",
         "Legal", "--numbering-level", "0:decimal:1:%1.", "--style-link",
         "Heading1:0", "--output", "out.docx", "--json"},
        2U, options, error));

    REQUIRE(options.definition_name.has_value());
    CHECK(*options.definition_name == "Legal");
    REQUIRE(options.levels.size() == 1U);
    REQUIRE(options.style_links.size() == 1U);
    CHECK(options.style_links[0].style_id == "Heading1");
    CHECK(options.style_links[0].level == 0U);
    REQUIRE(options.output_path.has_value());
    CHECK(options.output_path->filename().string() == "out.docx");
    CHECK(options.json_output);
}

TEST_CASE("cli numbering options parse validates required style numbering options") {
    featherdoc_cli::set_paragraph_style_numbering_options missing_name;
    std::string missing_name_error;
    CHECK_FALSE(featherdoc_cli::parse_set_paragraph_style_numbering_options(
        {"set-paragraph-style-numbering", "input.docx", "Heading1",
         "--numbering-level", "0:decimal:1:%1."},
        3U, missing_name, missing_name_error));
    CHECK(missing_name_error == "missing --definition-name <name>");

    featherdoc_cli::ensure_style_linked_numbering_options missing_link;
    std::string missing_link_error;
    CHECK_FALSE(featherdoc_cli::parse_ensure_style_linked_numbering_options(
        {"ensure-style-linked-numbering", "input.docx", "--definition-name",
         "Legal", "--numbering-level", "0:decimal:1:%1."},
        2U, missing_link, missing_link_error));
    CHECK(missing_link_error ==
          "expected at least one --style-link <style-id>:<level>");
}

TEST_CASE("cli numbering options parse accepts clear and custom numbering options") {
    featherdoc_cli::clear_paragraph_style_numbering_options clear_options;
    std::string clear_error;
    CHECK(featherdoc_cli::parse_clear_paragraph_style_numbering_options(
        {"clear-paragraph-style-numbering", "input.docx", "Heading1",
         "--output", "out.docx", "--json"},
        3U, clear_options, clear_error));
    REQUIRE(clear_options.output_path.has_value());
    CHECK(clear_options.output_path->filename().string() == "out.docx");
    CHECK(clear_options.json_output);

    featherdoc_cli::ensure_numbering_definition_options ensure_options;
    std::string ensure_error;
    CHECK(featherdoc_cli::parse_ensure_numbering_definition_options(
        {"ensure-numbering-definition", "input.docx", "--definition-name",
         "Legal", "--numbering-level", "0:decimal:1:%1.", "--json"},
        2U, ensure_options, ensure_error));
    REQUIRE(ensure_options.definition_name.has_value());
    CHECK(*ensure_options.definition_name == "Legal");
    REQUIRE(ensure_options.levels.size() == 1U);
    CHECK(ensure_options.json_output);

    featherdoc_cli::set_paragraph_numbering_options set_options;
    std::string set_error;
    CHECK(featherdoc_cli::parse_set_paragraph_numbering_options(
        {"set-paragraph-numbering", "input.docx", "0", "--definition", "3",
         "--level", "1", "--output", "out.docx", "--json"},
        3U, set_options, set_error));
    REQUIRE(set_options.definition_id.has_value());
    CHECK(*set_options.definition_id == 3U);
    REQUIRE(set_options.level.has_value());
    CHECK(*set_options.level == 1U);
    REQUIRE(set_options.output_path.has_value());
    CHECK(set_options.output_path->filename().string() == "out.docx");
    CHECK(set_options.json_output);
}

TEST_CASE("cli numbering options parse validates custom numbering options") {
    featherdoc_cli::ensure_numbering_definition_options missing_levels;
    std::string missing_levels_error;
    CHECK_FALSE(featherdoc_cli::parse_ensure_numbering_definition_options(
        {"ensure-numbering-definition", "input.docx", "--definition-name", "Legal"},
        2U, missing_levels, missing_levels_error));
    CHECK(missing_levels_error ==
          "expected at least one --numbering-level "
          "<level>:<kind>:<start>:<text-pattern>");

    featherdoc_cli::set_paragraph_numbering_options missing_definition;
    std::string missing_definition_error;
    CHECK_FALSE(featherdoc_cli::parse_set_paragraph_numbering_options(
        {"set-paragraph-numbering", "input.docx", "0", "--json"}, 3U,
        missing_definition, missing_definition_error));
    CHECK(missing_definition_error == "missing --definition <id>");
}
