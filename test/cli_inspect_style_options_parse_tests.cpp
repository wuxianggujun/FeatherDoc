#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "featherdoc_cli_inspect_style_options_parse.hpp"

#include <string>
#include <string_view>
#include <vector>

TEST_CASE("cli inspect style options parse accepts generic inspect json") {
    featherdoc_cli::inspect_options options;
    std::string error;

    CHECK(featherdoc_cli::parse_inspect_options(
        {"inspect", "input.docx", "--json"}, 2U, options, error));
    CHECK(options.json_output);

    featherdoc_cli::inspect_options unknown;
    std::string unknown_error;
    CHECK_FALSE(featherdoc_cli::parse_inspect_options(
        {"inspect", "input.docx", "--verbose"}, 2U, unknown, unknown_error));
    CHECK(unknown_error == "unknown option: --verbose");
}

TEST_CASE("cli inspect style options parse accepts style audit flags") {
    featherdoc_cli::audit_style_numbering_options numbering;
    std::string numbering_error;
    CHECK(featherdoc_cli::parse_audit_style_numbering_options(
        {"audit-style-numbering", "input.docx", "--fail-on-issue", "--json"},
        2U, numbering, numbering_error));
    CHECK(numbering.fail_on_issue);
    CHECK(numbering.json_output);

    featherdoc_cli::audit_table_style_quality_options quality;
    std::string quality_error;
    CHECK(featherdoc_cli::parse_audit_table_style_quality_options(
        {"audit-table-style-quality", "input.docx", "--fail-on-issue",
         "--json"},
        2U, quality, quality_error));
    CHECK(quality.fail_on_issue);
    CHECK(quality.json_output);

    featherdoc_cli::plan_table_style_quality_fixes_options plan;
    std::string plan_error;
    CHECK(featherdoc_cli::parse_plan_table_style_quality_fixes_options(
        {"plan-table-style-quality-fixes", "input.docx", "--fail-on-issue",
         "--json"},
        2U, plan, plan_error));
    CHECK(plan.fail_on_issue);
    CHECK(plan.json_output);
}

TEST_CASE("cli inspect style options parse validates table style filters") {
    featherdoc_cli::audit_table_style_regions_options regions;
    std::string regions_error;
    CHECK(featherdoc_cli::parse_audit_table_style_regions_options(
        {"audit-table-style-regions", "input.docx", "--style", "Invoice",
         "--fail-on-issue", "--json"},
        2U, regions, regions_error));
    REQUIRE(regions.style_id.has_value());
    CHECK(*regions.style_id == "Invoice");
    CHECK(regions.fail_on_issue);
    CHECK(regions.json_output);

    featherdoc_cli::audit_table_style_inheritance_options inheritance;
    std::string inheritance_error;
    CHECK_FALSE(featherdoc_cli::parse_audit_table_style_inheritance_options(
        {"audit-table-style-inheritance", "input.docx", "--style", ""},
        2U, inheritance, inheritance_error));
    CHECK(inheritance_error == "--style requires a non-empty style id");
}

TEST_CASE("cli inspect style options parse validates quality apply options") {
    featherdoc_cli::apply_table_style_quality_fixes_options options;
    std::string error;
    CHECK(featherdoc_cli::parse_apply_table_style_quality_fixes_options(
        {"apply-table-style-quality-fixes", "input.docx", "--look-only",
         "--output", "out.docx", "--json"},
        2U, options, error));
    CHECK(options.look_only);
    REQUIRE(options.output_path.has_value());
    CHECK(options.output_path->filename().string() == "out.docx");
    CHECK(options.json_output);

    featherdoc_cli::apply_table_style_quality_fixes_options missing_look_only;
    std::string missing_look_only_error;
    CHECK_FALSE(featherdoc_cli::parse_apply_table_style_quality_fixes_options(
        {"apply-table-style-quality-fixes", "input.docx", "--output",
         "out.docx"},
        2U, missing_look_only, missing_look_only_error));
    CHECK(missing_look_only_error ==
          "apply-table-style-quality-fixes requires --look-only");

    featherdoc_cli::apply_table_style_quality_fixes_options missing_output;
    std::string missing_output_error;
    CHECK_FALSE(featherdoc_cli::parse_apply_table_style_quality_fixes_options(
        {"apply-table-style-quality-fixes", "input.docx", "--look-only"}, 2U,
        missing_output, missing_output_error));
    CHECK(missing_output_error ==
          "apply-table-style-quality-fixes requires --output <path>");
}

TEST_CASE("cli inspect style options parse validates repair style numbering") {
    featherdoc_cli::repair_style_numbering_options plan_only;
    std::string plan_only_error;
    CHECK(featherdoc_cli::parse_repair_style_numbering_options(
        {"repair-style-numbering", "input.docx", "--json"}, 2U, plan_only,
        plan_only_error));
    CHECK(plan_only.plan_only);
    CHECK_FALSE(plan_only.apply);
    CHECK(plan_only.json_output);

    featherdoc_cli::repair_style_numbering_options apply;
    std::string apply_error;
    CHECK(featherdoc_cli::parse_repair_style_numbering_options(
        {"repair-style-numbering", "input.docx", "--apply", "--catalog-file",
         "catalog.json", "--output", "out.docx", "--json"},
        2U, apply, apply_error));
    CHECK(apply.apply);
    CHECK_FALSE(apply.plan_only);
    REQUIRE(apply.catalog_path.has_value());
    CHECK(apply.catalog_path->filename().string() == "catalog.json");
    REQUIRE(apply.output_path.has_value());
    CHECK(apply.output_path->filename().string() == "out.docx");
    CHECK(apply.json_output);

    featherdoc_cli::repair_style_numbering_options conflict;
    std::string conflict_error;
    CHECK_FALSE(featherdoc_cli::parse_repair_style_numbering_options(
        {"repair-style-numbering", "input.docx", "--plan-only", "--apply"},
        2U, conflict, conflict_error));
    CHECK(conflict_error == "--plan-only and --apply are mutually exclusive");

    featherdoc_cli::repair_style_numbering_options output_without_apply;
    std::string output_without_apply_error;
    CHECK_FALSE(featherdoc_cli::parse_repair_style_numbering_options(
        {"repair-style-numbering", "input.docx", "--output", "out.docx"},
        2U, output_without_apply, output_without_apply_error));
    CHECK(output_without_apply_error == "--output requires --apply");
}
