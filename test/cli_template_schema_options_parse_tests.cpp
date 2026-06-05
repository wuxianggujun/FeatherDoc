#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "featherdoc_cli_template_schema_options_parse.hpp"

#include <string>
#include <string_view>
#include <vector>

TEST_CASE("cli template schema options parse accepts export modes") {
    featherdoc_cli::export_template_schema_options options;
    std::string error;

    CHECK(featherdoc_cli::parse_export_template_schema_options(
        {"export-template-schema", "input.docx", "--resolved-section-targets",
         "--output", "schema.json", "--json"},
        2U, options, error));

    CHECK(options.resolved_section_targets);
    CHECK_FALSE(options.section_targets);
    REQUIRE(options.output_path.has_value());
    CHECK(options.output_path->filename().string() == "schema.json");
    CHECK(options.json_output);
}

TEST_CASE("cli template schema options parse rejects target mode conflicts") {
    featherdoc_cli::export_template_schema_options export_options;
    std::string export_error;
    CHECK_FALSE(featherdoc_cli::parse_export_template_schema_options(
        {"export-template-schema", "input.docx", "--section-targets",
         "--resolved-section-targets"},
        2U, export_options, export_error));
    CHECK(export_error ==
          "--resolved-section-targets conflicts with --section-targets");

    featherdoc_cli::check_template_schema_options check_options;
    std::string check_error;
    CHECK_FALSE(featherdoc_cli::parse_check_template_schema_options(
        {"check-template-schema", "input.docx", "--schema-file", "base.json",
         "--resolved-section-targets", "--section-targets"},
        2U, check_options, check_error));
    CHECK(check_error ==
          "--section-targets conflicts with --resolved-section-targets");
}

TEST_CASE("cli template schema options parse validates common output options") {
    featherdoc_cli::normalize_template_schema_options normalize_options;
    std::string normalize_error;
    CHECK_FALSE(featherdoc_cli::parse_normalize_template_schema_options(
        {"normalize-template-schema", "schema.json", "--output", "a.json",
         "--output", "b.json"},
        2U, normalize_options, normalize_error));
    CHECK(normalize_error == "duplicate --output option");

    featherdoc_cli::repair_template_schema_options repair_options;
    std::string repair_error;
    CHECK(featherdoc_cli::parse_repair_template_schema_options(
        {"repair-template-schema", "schema.json", "--output", "fixed.json",
         "--json"},
        2U, repair_options, repair_error));
    REQUIRE(repair_options.output_path.has_value());
    CHECK(repair_options.output_path->filename().string() == "fixed.json");
    CHECK(repair_options.json_output);
}

TEST_CASE("cli template schema options parse validates lint and diff flags") {
    featherdoc_cli::lint_template_schema_options lint_options;
    std::string lint_error;
    CHECK(featherdoc_cli::parse_lint_template_schema_options(
        {"lint-template-schema", "schema.json", "--json"}, 2U, lint_options,
        lint_error));
    CHECK(lint_options.json_output);

    featherdoc_cli::diff_template_schema_options diff_options;
    std::string diff_error;
    CHECK(featherdoc_cli::parse_diff_template_schema_options(
        {"diff-template-schema", "left.json", "right.json", "--fail-on-diff",
         "--json"},
        3U, diff_options, diff_error));
    CHECK(diff_options.fail_on_diff);
    CHECK(diff_options.json_output);
}

TEST_CASE("cli template schema options parse collects merge schema paths") {
    featherdoc_cli::merge_template_schema_options options;
    std::string error;
    CHECK(featherdoc_cli::parse_merge_template_schema_options(
        {"merge-template-schema", "left.json", "right.json", "--output",
         "merged.json", "--json"},
        1U, options, error));

    REQUIRE(options.schema_paths.size() == 2U);
    CHECK(options.schema_paths[0].filename().string() == "left.json");
    CHECK(options.schema_paths[1].filename().string() == "right.json");
    REQUIRE(options.output_path.has_value());
    CHECK(options.output_path->filename().string() == "merged.json");
    CHECK(options.json_output);

    featherdoc_cli::merge_template_schema_options missing;
    std::string missing_error;
    CHECK_FALSE(featherdoc_cli::parse_merge_template_schema_options(
        {"merge-template-schema", "only.json"}, 1U, missing, missing_error));
    CHECK(missing_error ==
          "merge-template-schema expects at least two schema paths");
}

TEST_CASE("cli template schema options parse validates patch modes") {
    featherdoc_cli::patch_template_schema_options patch_options;
    std::string patch_error;
    CHECK(featherdoc_cli::parse_patch_template_schema_options(
        {"patch-template-schema", "base.json", "--patch-file", "patch.json",
         "--output", "patched.json", "--json"},
        2U, patch_options, patch_error));
    REQUIRE(patch_options.patch_path.has_value());
    CHECK(patch_options.patch_path->filename().string() == "patch.json");
    REQUIRE(patch_options.output_path.has_value());
    CHECK(patch_options.output_path->filename().string() == "patched.json");
    CHECK(patch_options.json_output);

    featherdoc_cli::patch_template_schema_options missing_patch;
    std::string missing_patch_error;
    CHECK_FALSE(featherdoc_cli::parse_patch_template_schema_options(
        {"patch-template-schema", "base.json", "--json"}, 2U, missing_patch,
        missing_patch_error));
    CHECK(missing_patch_error ==
          "missing required --patch-file <path> option");
}

TEST_CASE("cli template schema options parse validates preview exclusivity") {
    featherdoc_cli::preview_template_schema_patch_options patch_preview;
    std::string patch_preview_error;
    CHECK(featherdoc_cli::parse_preview_template_schema_patch_options(
        {"preview-template-schema-patch", "base.json", "--patch-file",
         "patch.json", "--output-patch", "preview.json", "--review-json",
         "review.json", "--json"},
        2U, patch_preview, patch_preview_error));
    REQUIRE(patch_preview.patch_path.has_value());
    CHECK(patch_preview.patch_path->filename().string() == "patch.json");
    CHECK_FALSE(patch_preview.right_schema_path.has_value());
    CHECK(patch_preview.json_output);

    featherdoc_cli::preview_template_schema_patch_options right_preview;
    std::string right_preview_error;
    CHECK(featherdoc_cli::parse_preview_template_schema_patch_options(
        {"preview-template-schema-patch", "base.json", "right.json"}, 2U,
        right_preview, right_preview_error));
    REQUIRE(right_preview.right_schema_path.has_value());
    CHECK(right_preview.right_schema_path->filename().string() == "right.json");

    featherdoc_cli::preview_template_schema_patch_options invalid_preview;
    std::string invalid_preview_error;
    CHECK_FALSE(featherdoc_cli::parse_preview_template_schema_patch_options(
        {"preview-template-schema-patch", "base.json", "--patch-file",
         "patch.json", "right.json"},
        2U, invalid_preview, invalid_preview_error));
    CHECK(invalid_preview_error ==
          "preview-template-schema-patch expects exactly one of "
          "--patch-file <path> or <right-schema.json>");
}

TEST_CASE("cli template schema options parse validates build and check options") {
    featherdoc_cli::build_template_schema_patch_options build_options;
    std::string build_error;
    CHECK(featherdoc_cli::parse_build_template_schema_patch_options(
        {"build-template-schema-patch", "left.json", "right.json", "--output",
         "patch.json", "--review-json", "review.json", "--json"},
        3U, build_options, build_error));
    REQUIRE(build_options.output_path.has_value());
    CHECK(build_options.output_path->filename().string() == "patch.json");
    REQUIRE(build_options.review_json_path.has_value());
    CHECK(build_options.review_json_path->filename().string() == "review.json");
    CHECK(build_options.json_output);

    featherdoc_cli::check_template_schema_options check_options;
    std::string check_error;
    CHECK(featherdoc_cli::parse_check_template_schema_options(
        {"check-template-schema", "input.docx", "--schema-file", "base.json",
         "--output", "generated.json", "--json"},
        2U, check_options, check_error));
    REQUIRE(check_options.schema_path.has_value());
    CHECK(check_options.schema_path->filename().string() == "base.json");
    REQUIRE(check_options.output_path.has_value());
    CHECK(check_options.output_path->filename().string() == "generated.json");
    CHECK(check_options.json_output);

    featherdoc_cli::check_template_schema_options missing_schema;
    std::string missing_schema_error;
    CHECK_FALSE(featherdoc_cli::parse_check_template_schema_options(
        {"check-template-schema", "input.docx", "--json"}, 2U,
        missing_schema, missing_schema_error));
    CHECK(missing_schema_error ==
          "missing required --schema-file <path> option");
}
