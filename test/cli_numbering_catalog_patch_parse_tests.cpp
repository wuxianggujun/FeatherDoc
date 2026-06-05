#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <system_error>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "featherdoc_cli_numbering_catalog_patch_parse.hpp"

namespace {

auto temp_patch_path(std::string_view suffix) -> std::filesystem::path {
    const auto stamp =
        std::chrono::steady_clock::now().time_since_epoch().count();
    return std::filesystem::temp_directory_path() /
           ("featherdoc_cli_numbering_catalog_patch_parse_" +
            std::to_string(stamp) + std::string(suffix));
}

void write_patch_file(const std::filesystem::path &path,
                      std::string_view content) {
    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    REQUIRE(stream.good());
    stream.write(content.data(), static_cast<std::streamsize>(content.size()));
    REQUIRE(stream.good());
}

auto read_patch(std::string_view content,
                featherdoc_cli::numbering_catalog_patch_document &patch,
                std::string &error_message) -> bool {
    const auto path = temp_patch_path(".json");
    write_patch_file(path, content);

    patch = {};
    error_message.clear();
    const auto parsed =
        featherdoc_cli::read_numbering_catalog_patch_file(path, patch,
                                                          error_message);

    std::error_code ignored;
    std::filesystem::remove(path, ignored);
    return parsed;
}

} // namespace

TEST_CASE("cli numbering catalog patch parser reads all operation groups") {
    featherdoc_cli::numbering_catalog_patch_document patch;
    std::string error_message;

    REQUIRE(read_patch(
        R"({
  "upsert_levels": [
    {
      "definition_name": "Heading",
      "level": {
        "level": 2,
        "kind": "decimal",
        "start": 1,
        "text_pattern": "%3."
      }
    }
  ],
  "upsert_overrides": [
    {
      "definition_name": "Heading",
      "instance_index": 0,
      "level": 2,
      "start_override": 5
    },
    {
      "definition_name": "Heading",
      "instance_id": 42,
      "level": 1,
      "level_definition": {
        "level": 1,
        "kind": "bullet",
        "start": 1,
        "text_pattern": "*"
      }
    }
  ],
  "remove_overrides": [
    {"definition_name": "Heading", "instance_id": 42, "level": 2}
  ]
})",
        patch, error_message));

    REQUIRE(patch.upsert_levels.size() == 1U);
    CHECK(patch.upsert_levels.front().definition_name == "Heading");
    CHECK(patch.upsert_levels.front().level_definition.level == 2U);
    CHECK(patch.upsert_levels.front().level_definition.kind ==
          featherdoc::list_kind::decimal);

    REQUIRE(patch.upsert_overrides.size() == 2U);
    CHECK(patch.upsert_overrides.front().definition_name == "Heading");
    REQUIRE(patch.upsert_overrides.front().instance_index.has_value());
    CHECK(*patch.upsert_overrides.front().instance_index == 0U);
    CHECK_FALSE(patch.upsert_overrides.front().instance_id.has_value());
    CHECK(patch.upsert_overrides.front().level == 2U);
    CHECK(patch.upsert_overrides.front().saw_start_override);
    REQUIRE(patch.upsert_overrides.front().start_override.has_value());
    CHECK(*patch.upsert_overrides.front().start_override == 5U);

    REQUIRE(patch.upsert_overrides.back().instance_id.has_value());
    CHECK(*patch.upsert_overrides.back().instance_id == 42U);
    CHECK(patch.upsert_overrides.back().saw_level_definition);
    REQUIRE(patch.upsert_overrides.back().level_definition.has_value());
    CHECK(patch.upsert_overrides.back().level_definition->kind ==
          featherdoc::list_kind::bullet);

    REQUIRE(patch.remove_overrides.size() == 1U);
    CHECK(patch.remove_overrides.front().definition_name == "Heading");
    REQUIRE(patch.remove_overrides.front().instance_id.has_value());
    CHECK(*patch.remove_overrides.front().instance_id == 42U);
    CHECK(patch.remove_overrides.front().level == 2U);
}

TEST_CASE("cli numbering catalog patch parser clears reused output patch") {
    const auto path = temp_patch_path(".json");
    write_patch_file(
        path,
        R"({"upsert_levels":[{"definition_name":"Body","level":{"level":0,"kind":"decimal","start":1,"text_pattern":"%1."}}]})");

    featherdoc_cli::numbering_catalog_patch_document patch;
    featherdoc_cli::numbering_catalog_level_patch stale_level;
    stale_level.definition_name = "stale";
    patch.upsert_levels.push_back(stale_level);
    patch.upsert_overrides.push_back({});
    patch.remove_overrides.push_back({});
    std::string error_message;

    REQUIRE(featherdoc_cli::read_numbering_catalog_patch_file(path, patch,
                                                              error_message));

    std::error_code ignored;
    std::filesystem::remove(path, ignored);

    REQUIRE(patch.upsert_levels.size() == 1U);
    CHECK(patch.upsert_levels.front().definition_name == "Body");
    CHECK(patch.upsert_overrides.empty());
    CHECK(patch.remove_overrides.empty());
}

TEST_CASE("cli numbering catalog patch parser rejects duplicate root members") {
    featherdoc_cli::numbering_catalog_patch_document patch;
    std::string error_message;

    CHECK_FALSE(read_patch(R"({"upsert_levels":[],"upsert_levels":[]})", patch,
                           error_message));
    CHECK(error_message.find("must not be duplicated") != std::string::npos);
}

TEST_CASE("cli numbering catalog patch parser rejects remove override values") {
    featherdoc_cli::numbering_catalog_patch_document patch;
    std::string error_message;

    CHECK_FALSE(read_patch(
        R"({"remove_overrides":[{"definition_name":"Heading","instance_id":1,"level":0,"start_override":2}]})",
        patch, error_message));
    CHECK(error_message.find("must not contain 'start_override'") !=
          std::string::npos);
}

TEST_CASE("cli numbering catalog patch parser rejects empty upsert overrides") {
    featherdoc_cli::numbering_catalog_patch_document patch;
    std::string error_message;

    CHECK_FALSE(read_patch(
        R"({"upsert_overrides":[{"definition_name":"Heading","instance_id":1,"level":0}]})",
        patch, error_message));
    CHECK(error_message.find("'start_override' or 'level_definition'") !=
          std::string::npos);
}
