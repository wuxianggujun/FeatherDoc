#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "featherdoc_cli_numbering_catalog_diff.hpp"

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

auto make_override(std::uint32_t level, std::uint32_t start_override)
    -> featherdoc::numbering_level_override_summary {
    featherdoc::numbering_level_override_summary override_summary;
    override_summary.level = level;
    override_summary.start_override = start_override;
    return override_summary;
}

auto make_definition(std::string name)
    -> featherdoc::numbering_catalog_definition {
    featherdoc::numbering_catalog_definition definition;
    definition.definition.name = std::move(name);
    return definition;
}

} // namespace

TEST_CASE("cli numbering catalog diff treats levels and overrides as unordered") {
    featherdoc::numbering_catalog left;
    auto left_definition = make_definition("Body");
    left_definition.definition.levels.push_back(
        make_level(1U, featherdoc::list_kind::bullet, 1U, "-"));
    left_definition.definition.levels.push_back(
        make_level(0U, featherdoc::list_kind::decimal, 1U, "%1."));
    featherdoc::numbering_instance_summary left_instance;
    left_instance.instance_id = 10U;
    left_instance.level_overrides.push_back(make_override(1U, 3U));
    left_instance.level_overrides.push_back(make_override(0U, 2U));
    left_definition.instances.push_back(left_instance);
    left.definitions.push_back(left_definition);

    featherdoc::numbering_catalog right;
    auto right_definition = make_definition("Body");
    right_definition.definition.levels.push_back(
        make_level(0U, featherdoc::list_kind::decimal, 1U, "%1."));
    right_definition.definition.levels.push_back(
        make_level(1U, featherdoc::list_kind::bullet, 1U, "-"));
    featherdoc::numbering_instance_summary right_instance;
    right_instance.instance_id = 10U;
    right_instance.level_overrides.push_back(make_override(0U, 2U));
    right_instance.level_overrides.push_back(make_override(1U, 3U));
    right_definition.instances.push_back(right_instance);
    right.definitions.push_back(right_definition);

    const auto result = featherdoc_cli::diff_numbering_catalogs(left, right);

    CHECK(result.equal());
    CHECK(result.added_definitions.empty());
    CHECK(result.removed_definitions.empty());
    CHECK(result.changed_definitions.empty());
}

TEST_CASE("cli numbering catalog diff reports definition, level, and override changes") {
    featherdoc::numbering_catalog left;
    auto left_body = make_definition("Body");
    left_body.definition.levels.push_back(
        make_level(0U, featherdoc::list_kind::decimal, 1U, "%1."));
    left_body.definition.levels.push_back(
        make_level(2U, featherdoc::list_kind::bullet, 1U, "-"));
    featherdoc::numbering_instance_summary left_instance;
    left_instance.instance_id = 10U;
    left_instance.level_overrides.push_back(make_override(0U, 2U));
    left_instance.level_overrides.push_back(make_override(2U, 4U));
    left_body.instances.push_back(left_instance);
    left_body.instances.push_back(featherdoc::numbering_instance_summary{11U, {}});
    left.definitions.push_back(left_body);
    auto removed_definition = make_definition("Removed");
    removed_definition.definition.levels.push_back(
        make_level(0U, featherdoc::list_kind::decimal, 1U, "%1."));
    left.definitions.push_back(removed_definition);

    featherdoc::numbering_catalog right;
    auto right_body = make_definition("Body");
    right_body.definition.levels.push_back(
        make_level(0U, featherdoc::list_kind::decimal, 2U, "%1)"));
    right_body.definition.levels.push_back(
        make_level(1U, featherdoc::list_kind::bullet, 1U, "*"));
    featherdoc::numbering_instance_summary right_instance;
    right_instance.instance_id = 10U;
    right_instance.level_overrides.push_back(make_override(0U, 3U));
    right_instance.level_overrides.push_back(make_override(1U, 5U));
    right_body.instances.push_back(right_instance);
    right_body.instances.push_back(featherdoc::numbering_instance_summary{11U, {}});
    right_body.instances.push_back(featherdoc::numbering_instance_summary{12U, {}});
    right.definitions.push_back(right_body);
    auto added_definition = make_definition("Added");
    added_definition.definition.levels.push_back(
        make_level(0U, featherdoc::list_kind::decimal, 1U, "%1."));
    right.definitions.push_back(added_definition);

    const auto result = featherdoc_cli::diff_numbering_catalogs(left, right);

    CHECK_FALSE(result.equal());
    REQUIRE(result.added_definitions.size() == 1U);
    CHECK(result.added_definitions.front().definition.name == "Added");
    REQUIRE(result.removed_definitions.size() == 1U);
    CHECK(result.removed_definitions.front().definition.name == "Removed");
    REQUIRE(result.changed_definitions.size() == 1U);

    const auto &definition_diff = result.changed_definitions.front();
    CHECK(definition_diff.name == "Body");
    REQUIRE(definition_diff.added_levels.size() == 1U);
    CHECK(definition_diff.added_levels.front().level == 1U);
    REQUIRE(definition_diff.removed_levels.size() == 1U);
    CHECK(definition_diff.removed_levels.front().level == 2U);
    REQUIRE(definition_diff.changed_levels.size() == 1U);
    CHECK(definition_diff.changed_levels.front().left.start == 1U);
    CHECK(definition_diff.changed_levels.front().right.start == 2U);

    REQUIRE(definition_diff.added_instances.size() == 1U);
    CHECK(definition_diff.added_instances.front().instance_id == 12U);
    CHECK(definition_diff.removed_instances.empty());
    REQUIRE(definition_diff.changed_instances.size() == 1U);

    const auto &instance_diff = definition_diff.changed_instances.front();
    CHECK(instance_diff.instance_index == 0U);
    REQUIRE(instance_diff.added_overrides.size() == 1U);
    CHECK(instance_diff.added_overrides.front().level == 1U);
    REQUIRE(instance_diff.removed_overrides.size() == 1U);
    CHECK(instance_diff.removed_overrides.front().level == 2U);
    REQUIRE(instance_diff.changed_overrides.size() == 1U);
    CHECK(*instance_diff.changed_overrides.front().left.start_override == 2U);
    CHECK(*instance_diff.changed_overrides.front().right.start_override == 3U);
}
