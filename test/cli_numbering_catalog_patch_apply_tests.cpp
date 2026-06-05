#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "featherdoc_cli_numbering_catalog_patch_apply.hpp"

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

auto make_catalog() -> featherdoc::numbering_catalog {
    featherdoc::numbering_catalog catalog;
    featherdoc::numbering_catalog_definition definition;
    definition.definition.name = "Heading";
    definition.definition.levels.push_back(
        make_level(0U, featherdoc::list_kind::decimal, 1U, "%1."));

    featherdoc::numbering_instance_summary instance;
    instance.instance_id = 42U;
    featherdoc::numbering_level_override_summary override_summary;
    override_summary.level = 0U;
    override_summary.start_override = 2U;
    instance.level_overrides.push_back(override_summary);
    definition.instances.push_back(instance);

    catalog.definitions.push_back(definition);
    return catalog;
}

} // namespace

TEST_CASE("cli numbering catalog patch apply mutates catalog and summary") {
    auto catalog = make_catalog();
    featherdoc_cli::numbering_catalog_patch_document patch;

    featherdoc_cli::numbering_catalog_level_patch level_patch;
    level_patch.definition_name = "Heading";
    level_patch.level_definition =
        make_level(1U, featherdoc::list_kind::bullet, 1U, "*");
    patch.upsert_levels.push_back(level_patch);

    featherdoc_cli::numbering_catalog_override_patch upsert_override;
    upsert_override.definition_name = "Heading";
    upsert_override.instance_id = 42U;
    upsert_override.level = 1U;
    upsert_override.saw_start_override = true;
    upsert_override.start_override = 5U;
    patch.upsert_overrides.push_back(upsert_override);

    featherdoc_cli::numbering_catalog_override_patch remove_override;
    remove_override.definition_name = "Heading";
    remove_override.instance_index = 0U;
    remove_override.level = 0U;
    patch.remove_overrides.push_back(remove_override);

    featherdoc_cli::numbering_catalog_override_patch missing_override;
    missing_override.definition_name = "Heading";
    missing_override.instance_index = 0U;
    missing_override.level = 7U;
    patch.remove_overrides.push_back(missing_override);

    featherdoc_cli::numbering_catalog_patch_summary summary;
    std::string error_message;
    REQUIRE(featherdoc_cli::apply_numbering_catalog_patch(catalog, patch,
                                                          summary,
                                                          error_message));

    CHECK(summary.upserted_level_count == 1U);
    CHECK(summary.upserted_override_count == 1U);
    CHECK(summary.removed_override_count == 1U);
    CHECK(summary.missing_override_count == 1U);

    REQUIRE(catalog.definitions.size() == 1U);
    const auto &definition = catalog.definitions.front();
    REQUIRE(definition.definition.levels.size() == 2U);
    CHECK(definition.definition.levels.front().level == 0U);
    CHECK(definition.definition.levels.back().level == 1U);
    CHECK(definition.definition.levels.back().kind ==
          featherdoc::list_kind::bullet);

    REQUIRE(definition.instances.size() == 1U);
    const auto &overrides = definition.instances.front().level_overrides;
    REQUIRE(overrides.size() == 1U);
    CHECK(overrides.front().level == 1U);
    REQUIRE(overrides.front().start_override.has_value());
    CHECK(*overrides.front().start_override == 5U);
}

TEST_CASE("cli numbering catalog patch apply rejects missing definitions") {
    auto catalog = make_catalog();
    featherdoc_cli::numbering_catalog_patch_document patch;
    featherdoc_cli::numbering_catalog_level_patch level_patch;
    level_patch.definition_name = "Missing";
    level_patch.level_definition =
        make_level(1U, featherdoc::list_kind::decimal, 1U, "%2.");
    patch.upsert_levels.push_back(level_patch);

    featherdoc_cli::numbering_catalog_patch_summary summary;
    std::string error_message;
    CHECK_FALSE(featherdoc_cli::apply_numbering_catalog_patch(
        catalog, patch, summary, error_message));
    CHECK(error_message.find("definition was not found") != std::string::npos);
}

TEST_CASE("cli numbering catalog patch apply rejects duplicate instance ids") {
    auto catalog = make_catalog();
    catalog.definitions.front().instances.push_back(
        catalog.definitions.front().instances.front());

    featherdoc_cli::numbering_catalog_patch_document patch;
    featherdoc_cli::numbering_catalog_override_patch override_patch;
    override_patch.definition_name = "Heading";
    override_patch.instance_id = 42U;
    override_patch.level = 1U;
    override_patch.saw_start_override = true;
    override_patch.start_override = 3U;
    patch.upsert_overrides.push_back(override_patch);

    featherdoc_cli::numbering_catalog_patch_summary summary;
    std::string error_message;
    CHECK_FALSE(featherdoc_cli::apply_numbering_catalog_patch(
        catalog, patch, summary, error_message));
    CHECK(error_message.find("duplicate instance id") != std::string::npos);
}
