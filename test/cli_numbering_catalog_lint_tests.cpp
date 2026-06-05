#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "featherdoc_cli_numbering_catalog_lint.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>

namespace {

auto make_level(std::uint32_t level, featherdoc::list_kind kind,
                std::uint32_t start, std::string text_pattern)
    -> featherdoc::numbering_level_definition {
    featherdoc::numbering_level_definition definition;
    definition.level = level;
    definition.kind = kind;
    definition.start = start;
    definition.text_pattern = std::move(text_pattern);
    return definition;
}

auto issue_count(const featherdoc_cli::numbering_catalog_lint_result &result,
                 featherdoc_cli::numbering_catalog_lint_issue_kind kind)
    -> std::size_t {
    return static_cast<std::size_t>(
        std::count_if(result.issues.begin(), result.issues.end(),
                      [kind](const auto &issue) {
                          return issue.kind == kind;
                      }));
}

auto make_clean_catalog() -> featherdoc::numbering_catalog {
    featherdoc::numbering_catalog catalog;
    featherdoc::numbering_catalog_definition definition;
    definition.definition.name = "Body";
    definition.definition.levels.push_back(
        make_level(0U, featherdoc::list_kind::decimal, 1U, "%1."));

    featherdoc::numbering_instance_summary instance;
    instance.instance_id = 42U;
    definition.instances.push_back(instance);

    catalog.definitions.push_back(definition);
    return catalog;
}

} // namespace

TEST_CASE("cli numbering catalog lint reports clean catalogs") {
    const auto result =
        featherdoc_cli::lint_numbering_catalog(make_clean_catalog());

    CHECK(result.clean());
    CHECK(result.definition_count == 1U);
    CHECK(result.instance_count == 1U);
    CHECK(result.level_count == 1U);
    CHECK(result.override_count == 0U);
    CHECK(result.issues.empty());
}

TEST_CASE("cli numbering catalog lint reports definition and level issues") {
    featherdoc::numbering_catalog catalog;

    featherdoc::numbering_catalog_definition empty_definition;

    featherdoc::numbering_catalog_definition invalid_definition;
    invalid_definition.definition.name = "Heading";
    invalid_definition.definition.levels.push_back(
        make_level(9U, featherdoc::list_kind::decimal, 0U, ""));
    invalid_definition.definition.levels.push_back(
        make_level(9U, featherdoc::list_kind::decimal, 1U, "%10."));

    featherdoc::numbering_catalog_definition duplicate_definition;
    duplicate_definition.definition.name = "Heading";
    duplicate_definition.definition.levels.push_back(
        make_level(0U, featherdoc::list_kind::decimal, 1U, "%1."));

    catalog.definitions.push_back(empty_definition);
    catalog.definitions.push_back(invalid_definition);
    catalog.definitions.push_back(duplicate_definition);

    const auto result = featherdoc_cli::lint_numbering_catalog(catalog);

    CHECK_FALSE(result.clean());
    CHECK(result.definition_count == 3U);
    CHECK(result.level_count == 3U);
    CHECK(issue_count(
              result,
              featherdoc_cli::numbering_catalog_lint_issue_kind::
                  empty_definition_name) == 1U);
    CHECK(issue_count(result,
                      featherdoc_cli::numbering_catalog_lint_issue_kind::
                          empty_levels) == 1U);
    CHECK(issue_count(result,
                      featherdoc_cli::numbering_catalog_lint_issue_kind::
                          duplicate_definition_name) == 1U);
    CHECK(issue_count(
              result,
              featherdoc_cli::numbering_catalog_lint_issue_kind::invalid_level) ==
          2U);
    CHECK(issue_count(
              result,
              featherdoc_cli::numbering_catalog_lint_issue_kind::duplicate_level) ==
          1U);
    CHECK(issue_count(
              result,
              featherdoc_cli::numbering_catalog_lint_issue_kind::invalid_start) ==
          1U);
    CHECK(issue_count(result,
                      featherdoc_cli::numbering_catalog_lint_issue_kind::
                          empty_text_pattern) == 1U);
    CHECK(featherdoc_cli::numbering_catalog_lint_issue_name(
              featherdoc_cli::numbering_catalog_lint_issue_kind::
                  duplicate_level) == "duplicate_level");
}

TEST_CASE("cli numbering catalog lint reports instance and override issues") {
    auto catalog = make_clean_catalog();
    auto &definition = catalog.definitions.front();

    auto invalid_override = featherdoc::numbering_level_override_summary{};
    invalid_override.level = 9U;
    invalid_override.start_override = 0U;
    invalid_override.level_definition =
        make_level(9U, featherdoc::list_kind::decimal, 0U, "");

    auto duplicate_override = featherdoc::numbering_level_override_summary{};
    duplicate_override.level = 9U;
    duplicate_override.start_override = 1U;

    definition.instances.front().level_overrides.push_back(invalid_override);
    definition.instances.front().level_overrides.push_back(duplicate_override);
    definition.instances.push_back(definition.instances.front());

    const auto result = featherdoc_cli::lint_numbering_catalog(catalog);

    CHECK_FALSE(result.clean());
    CHECK(result.instance_count == 2U);
    CHECK(result.override_count == 4U);
    CHECK(issue_count(result,
                      featherdoc_cli::numbering_catalog_lint_issue_kind::
                          duplicate_instance_id) == 1U);
    CHECK(issue_count(result,
                      featherdoc_cli::numbering_catalog_lint_issue_kind::
                          invalid_override_level) == 4U);
    CHECK(issue_count(result,
                      featherdoc_cli::numbering_catalog_lint_issue_kind::
                          duplicate_override_level) == 2U);
    CHECK(issue_count(result,
                      featherdoc_cli::numbering_catalog_lint_issue_kind::
                          invalid_override_start) == 2U);
    CHECK(issue_count(result,
                      featherdoc_cli::numbering_catalog_lint_issue_kind::
                          invalid_override_definition) == 2U);
}
