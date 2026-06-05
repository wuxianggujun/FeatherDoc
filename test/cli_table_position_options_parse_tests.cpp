#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "featherdoc_cli_table_position_options_parse.hpp"

#include <featherdoc.hpp>

#include <string>
#include <string_view>
#include <vector>

TEST_CASE("cli table position options parse applies preset defaults") {
    featherdoc_cli::table_position_options options;
    std::string error;

    CHECK(featherdoc_cli::parse_table_position_options(
        {"set-table-position", "input.docx", "0", "--preset",
         "paragraph-callout", "--json"},
        3U, options, error));

    REQUIRE(options.preset.has_value());
    CHECK(*options.preset ==
          featherdoc_cli::table_position_preset::paragraph_callout);
    CHECK(options.has_horizontal_reference);
    CHECK(options.position.horizontal_reference ==
          featherdoc::table_position_horizontal_reference::column);
    CHECK(options.has_horizontal_offset);
    CHECK(options.position.horizontal_offset_twips == 0);
    CHECK(options.has_vertical_reference);
    CHECK(options.position.vertical_reference ==
          featherdoc::table_position_vertical_reference::paragraph);
    CHECK(options.has_vertical_offset);
    CHECK(options.position.vertical_offset_twips == 0);
    REQUIRE(options.position.left_from_text_twips.has_value());
    CHECK(*options.position.left_from_text_twips == 144U);
    REQUIRE(options.position.overlap.has_value());
    CHECK(*options.position.overlap == featherdoc::table_overlap::never);
    CHECK(options.json_output);
}

TEST_CASE("cli table position options parse keeps explicit preset overrides") {
    featherdoc_cli::table_position_options options;
    std::string error;

    CHECK(featherdoc_cli::parse_table_position_options(
        {"set-table-position", "input.docx", "0", "--preset",
         "paragraph-callout", "--horizontal-reference", "page",
         "--horizontal-offset", "360", "--vertical-offset", "-120",
         "--left-from-text", "12", "--output", "out.docx"},
        3U, options, error));

    CHECK(options.position.horizontal_reference ==
          featherdoc::table_position_horizontal_reference::page);
    CHECK(options.position.horizontal_offset_twips == 360);
    CHECK(options.position.vertical_reference ==
          featherdoc::table_position_vertical_reference::paragraph);
    CHECK(options.position.vertical_offset_twips == -120);
    REQUIRE(options.position.left_from_text_twips.has_value());
    CHECK(*options.position.left_from_text_twips == 12U);
    REQUIRE(options.output_path.has_value());
    CHECK(options.output_path->filename().string() == "out.docx");
}

TEST_CASE("cli table position options parse validates errors") {
    featherdoc_cli::table_position_options missing_required;
    std::string missing_required_error;
    CHECK_FALSE(featherdoc_cli::parse_table_position_options(
        {"set-table-position", "input.docx", "0", "--horizontal-offset", "720"},
        3U, missing_required, missing_required_error));
    CHECK(missing_required_error ==
          "missing required --horizontal-reference <margin|page|column> or --preset");

    featherdoc_cli::table_position_options duplicate_preset;
    std::string duplicate_preset_error;
    CHECK_FALSE(featherdoc_cli::parse_table_position_options(
        {"set-table-position", "input.docx", "0", "--preset",
         "paragraph-callout", "--preset", "page-corner"},
        3U, duplicate_preset, duplicate_preset_error));
    CHECK(duplicate_preset_error == "duplicate --preset option");

    featherdoc_cli::table_position_options bad_reference;
    std::string bad_reference_error;
    CHECK_FALSE(featherdoc_cli::parse_table_position_options(
        {"set-table-position", "input.docx", "0", "--horizontal-reference",
         "section", "--horizontal-offset", "0", "--vertical-reference",
         "paragraph", "--vertical-offset", "0"},
        3U, bad_reference, bad_reference_error));
    CHECK(bad_reference_error == "invalid horizontal reference: section");
}

TEST_CASE("cli table position target options parse accepts table list") {
    featherdoc_cli::table_position_target_options options;
    std::string error;

    CHECK(featherdoc_cli::parse_table_position_target_options(
        {"clear-table-position", "input.docx", "0", "--table", "2",
         "--table", "4", "--output", "out.docx", "--json"},
        3U, options, error));

    REQUIRE(options.additional_table_indices.size() == 2U);
    CHECK(options.additional_table_indices[0] == 2U);
    CHECK(options.additional_table_indices[1] == 4U);
    REQUIRE(options.output_path.has_value());
    CHECK(options.output_path->filename().string() == "out.docx");
    CHECK(options.json_output);
}

TEST_CASE("cli table position plan options parse accepts flags") {
    featherdoc_cli::plan_table_position_presets_options plan_options;
    std::string plan_error;

    CHECK(featherdoc_cli::parse_plan_table_position_presets_options(
        {"plan-table-position-presets", "input.docx", "--preset",
         "margin-anchor", "--replace-positioned", "--fail-on-change",
         "--output", "out.docx", "--output-plan", "plan.json", "--json"},
        2U, plan_options, plan_error));

    REQUIRE(plan_options.preset.has_value());
    CHECK(*plan_options.preset ==
          featherdoc_cli::table_position_preset::margin_anchor);
    CHECK(plan_options.replace_positioned);
    CHECK(plan_options.fail_on_change);
    REQUIRE(plan_options.output_path.has_value());
    CHECK(plan_options.output_path->filename().string() == "out.docx");
    REQUIRE(plan_options.output_plan_path.has_value());
    CHECK(plan_options.output_plan_path->filename().string() == "plan.json");
    CHECK(plan_options.json_output);

    featherdoc_cli::apply_table_position_plan_options apply_options;
    std::string apply_error;
    CHECK(featherdoc_cli::parse_apply_table_position_plan_options(
        {"apply-table-position-plan", "plan.json", "--output", "out.docx",
         "--dry-run", "--json"},
        2U, apply_options, apply_error));
    REQUIRE(apply_options.output_path.has_value());
    CHECK(apply_options.output_path->filename().string() == "out.docx");
    CHECK(apply_options.dry_run);
    CHECK(apply_options.json_output);
}

TEST_CASE("cli table position plan options parse validates missing preset") {
    featherdoc_cli::plan_table_position_presets_options options;
    std::string error;

    CHECK_FALSE(featherdoc_cli::parse_plan_table_position_presets_options(
        {"plan-table-position-presets", "input.docx", "--json"}, 2U,
        options, error));
    CHECK(error ==
          "missing required --preset <paragraph-callout|page-corner|margin-anchor>");
}
