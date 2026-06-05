#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "featherdoc_cli_template_validation_options_parse.hpp"

#include <string>
#include <string_view>

TEST_CASE("cli template validation options parse accepts template validation") {
    featherdoc_cli::validate_template_options options;
    std::string error;

    CHECK(featherdoc_cli::parse_validate_template_options(
        {"validate-template", "input.docx", "--part", "body", "--slot",
         "customer:text:optional:min=1:max=3", "--json"},
        2U, options, error));

    CHECK(options.has_part);
    CHECK(options.part == featherdoc_cli::validation_part_family::body);
    REQUIRE(options.requirements.size() == 1U);
    CHECK(options.requirements[0].bookmark_name == "customer");
    CHECK(options.json_output);
}

TEST_CASE("cli template validation options parse accepts schema targets and files") {
    featherdoc_cli::validate_template_schema_options options;
    std::string error;

    CHECK(featherdoc_cli::parse_validate_template_schema_options(
        {"validate-template-schema", "input.docx", "--schema-file", "base.json",
         "--target", "body", "--slot", "customer:text", "--json"},
        2U, options, error));

    REQUIRE(options.schema_files.size() == 1U);
    CHECK(options.schema_files[0].filename().string() == "base.json");
    REQUIRE(options.targets.size() == 1U);
    CHECK(options.targets[0].part == featherdoc_cli::validation_part_family::body);
    REQUIRE(options.targets[0].requirements.size() == 1U);
    CHECK(options.targets[0].requirements[0].bookmark_name == "customer");
    CHECK(options.json_output);
}

TEST_CASE("cli template validation options parse validates required inputs") {
    featherdoc_cli::validate_template_options template_options;
    std::string template_error;
    CHECK_FALSE(featherdoc_cli::parse_validate_template_options(
        {"validate-template", "input.docx", "--slot", "customer:text"}, 2U,
        template_options, template_error));
    CHECK(template_error ==
          "missing --part <body|header|footer|section-header|section-footer>");

    featherdoc_cli::validate_template_schema_options schema_options;
    std::string schema_error;
    CHECK_FALSE(featherdoc_cli::parse_validate_template_schema_options(
        {"validate-template-schema", "input.docx"}, 2U, schema_options,
        schema_error));
    CHECK(schema_error ==
          "expected at least one --schema-file <path> or --target "
          "<body|header|footer|section-header|section-footer>");

    featherdoc_cli::validate_template_schema_options target_options;
    std::string target_error;
    CHECK_FALSE(featherdoc_cli::parse_validate_template_schema_options(
        {"validate-template-schema", "input.docx", "--target", "body", "--json"},
        2U, target_options, target_error));
    CHECK(target_error ==
          "target[0] requires at least one --slot <bookmark>:<kind>"
          "[:required|optional][:count=<n>|min=<n>|max=<n>...]");
}
