#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <system_error>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "featherdoc_cli_numbering_catalog_parse.hpp"

namespace {

auto temp_catalog_path(std::string_view suffix) -> std::filesystem::path {
    const auto stamp =
        std::chrono::steady_clock::now().time_since_epoch().count();
    return std::filesystem::temp_directory_path() /
           ("featherdoc_cli_numbering_catalog_parse_" +
            std::to_string(stamp) + std::string(suffix));
}

void write_catalog_file(const std::filesystem::path &path,
                        std::string_view content) {
    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    REQUIRE(stream.good());
    stream.write(content.data(), static_cast<std::streamsize>(content.size()));
    REQUIRE(stream.good());
}

auto read_catalog(std::string_view content,
                  featherdoc::numbering_catalog &catalog,
                  std::string &error_message) -> bool {
    const auto path = temp_catalog_path(".json");
    write_catalog_file(path, content);

    catalog = {};
    error_message.clear();
    const auto parsed =
        featherdoc_cli::read_numbering_catalog_file(path, catalog,
                                                    error_message);

    std::error_code ignored;
    std::filesystem::remove(path, ignored);
    return parsed;
}

} // namespace

TEST_CASE("cli numbering catalog parser reads definitions and overrides") {
    featherdoc::numbering_catalog catalog;
    std::string error_message;

    REQUIRE(read_catalog(
        R"json({
  "definition_count": 1,
  "instance_count": 1,
  "definitions": [
    {
      "name": "Heading",
      "levels": [
        {"level": 0, "kind": "decimal", "start": 1, "text_pattern": "%1."},
        {"level": "1", "kind": "bullet", "start": "1", "text_pattern": "*"}
      ],
      "instances": [
        {
          "instance_id": 42,
          "level_overrides": [
            {
              "level": 1,
              "start_override": 3,
              "level_definition": {
                "level": 1,
                "kind": "decimal",
                "start": 3,
                "text_pattern": "%2)"
              }
            }
          ]
        }
      ]
    }
  ]
})json",
        catalog, error_message));

    REQUIRE(catalog.definitions.size() == 1U);
    const auto &definition = catalog.definitions.front();
    CHECK(definition.definition.name == "Heading");
    REQUIRE(definition.definition.levels.size() == 2U);
    CHECK(definition.definition.levels.front().kind ==
          featherdoc::list_kind::decimal);
    CHECK(definition.definition.levels.front().level == 0U);
    CHECK(definition.definition.levels.front().start == 1U);
    CHECK(definition.definition.levels.front().text_pattern == "%1.");
    CHECK(definition.definition.levels.back().kind ==
          featherdoc::list_kind::bullet);

    REQUIRE(definition.instances.size() == 1U);
    CHECK(definition.instances.front().instance_id == 42U);
    REQUIRE(definition.instances.front().level_overrides.size() == 1U);
    const auto &override_summary =
        definition.instances.front().level_overrides.front();
    CHECK(override_summary.level == 1U);
    REQUIRE(override_summary.start_override.has_value());
    CHECK(*override_summary.start_override == 3U);
    REQUIRE(override_summary.level_definition.has_value());
    CHECK(override_summary.level_definition->text_pattern == "%2)");
}

TEST_CASE("cli numbering catalog parser clears reused output catalog") {
    const auto path = temp_catalog_path(".json");
    write_catalog_file(
        path,
        R"({"definitions":[{"name":"Body","levels":[{"level":0,"kind":"decimal","start":1,"text_pattern":"%1."}],"instances":[]}]})");

    featherdoc::numbering_catalog catalog;
    featherdoc::numbering_catalog_definition stale_definition;
    stale_definition.definition.name = "stale";
    catalog.definitions.push_back(stale_definition);
    catalog.definitions.push_back(stale_definition);
    std::string error_message;

    REQUIRE(featherdoc_cli::read_numbering_catalog_file(path, catalog,
                                                        error_message));

    std::error_code ignored;
    std::filesystem::remove(path, ignored);

    REQUIRE(catalog.definitions.size() == 1U);
    CHECK(catalog.definitions.front().definition.name == "Body");
}

TEST_CASE("cli numbering catalog parser rejects duplicate root members") {
    featherdoc::numbering_catalog catalog;
    std::string error_message;

    CHECK_FALSE(read_catalog(R"({"definitions":[],"definitions":[]})", catalog,
                             error_message));
    CHECK(error_message.find("must not be duplicated") != std::string::npos);
}

TEST_CASE("cli numbering catalog parser rejects missing definitions") {
    featherdoc::numbering_catalog catalog;
    std::string error_message;

    CHECK_FALSE(read_catalog("{}", catalog, error_message));
    CHECK(error_message.find("must contain a 'definitions' array") !=
          std::string::npos);
}
