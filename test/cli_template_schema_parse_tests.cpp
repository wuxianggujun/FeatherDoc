#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <system_error>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "featherdoc_cli_template_schema_parse.hpp"

namespace {

auto temp_schema_path(std::string_view suffix) -> std::filesystem::path {
    const auto stamp =
        std::chrono::steady_clock::now().time_since_epoch().count();
    return std::filesystem::temp_directory_path() /
           ("featherdoc_cli_template_schema_parse_" + std::to_string(stamp) +
            std::string(suffix));
}

void write_schema_file(const std::filesystem::path &path,
                       std::string_view content) {
    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    REQUIRE(stream.good());
    stream.write(content.data(), static_cast<std::streamsize>(content.size()));
    REQUIRE(stream.good());
}

auto read_schema(std::string_view content,
                 featherdoc_cli::exported_template_schema_result &result,
                 std::string &error_message) -> bool {
    const auto path = temp_schema_path(".json");
    write_schema_file(path, content);

    result = {};
    error_message.clear();
    const auto parsed =
        featherdoc_cli::read_template_schema_file(path, result, error_message);

    std::error_code ignored;
    std::filesystem::remove(path, ignored);
    return parsed;
}

} // namespace

TEST_CASE("cli template schema parser reads pretty bookmark slots") {
    featherdoc_cli::exported_template_schema_result result;
    std::string error_message;

    REQUIRE(read_schema(
        R"({
  "targets": [
    {
      "part": "body",
      "slots": [
        {
          "bookmark": "customer",
          "kind": "text",
          "required": true,
          "count": 2
        }
      ]
    }
  ]
})",
        result, error_message));

    REQUIRE(result.targets.size() == 1U);
    const auto &target = result.targets.front();
    CHECK(target.part == featherdoc_cli::validation_part_family::body);
    CHECK_FALSE(target.part_index.has_value());
    CHECK_FALSE(target.section_index.has_value());
    REQUIRE(target.requirements.size() == 1U);

    const auto &slot = target.requirements.front();
    CHECK(slot.bookmark_name == "customer");
    CHECK(slot.kind == featherdoc::template_slot_kind::text);
    CHECK(slot.source == featherdoc::template_slot_source_kind::bookmark);
    CHECK(slot.required);
    REQUIRE(slot.min_occurrences.has_value());
    CHECK(*slot.min_occurrences == 2U);
    REQUIRE(slot.max_occurrences.has_value());
    CHECK(*slot.max_occurrences == 2U);
}

TEST_CASE("cli template schema parser reads content control section targets") {
    featherdoc_cli::exported_template_schema_result result;
    std::string error_message;

    REQUIRE(read_schema(
        R"({"targets":[{"part":"section-header","section":1,"kind":"default","resolved_from_section":"1","linked_to_previous":"false","entry_name":"word/header1.xml","slots":[{"content_control_tag":"invoice_id","kind":"text","required":"false","min":0,"max":1}]}]})",
        result, error_message));

    REQUIRE(result.targets.size() == 1U);
    const auto &target = result.targets.front();
    CHECK(target.part == featherdoc_cli::validation_part_family::section_header);
    REQUIRE(target.section_index.has_value());
    CHECK(*target.section_index == 1U);
    REQUIRE(target.reference_kind.has_value());
    CHECK(*target.reference_kind ==
          featherdoc::section_reference_kind::default_reference);
    REQUIRE(target.resolved_from_section_index.has_value());
    CHECK(*target.resolved_from_section_index == 1U);
    CHECK_FALSE(target.linked_to_previous);
    CHECK(target.entry_name == "word/header1.xml");

    REQUIRE(target.requirements.size() == 1U);
    const auto &slot = target.requirements.front();
    CHECK(slot.bookmark_name == "invoice_id");
    CHECK(slot.source ==
          featherdoc::template_slot_source_kind::content_control_tag);
    CHECK_FALSE(slot.required);
    REQUIRE(slot.min_occurrences.has_value());
    CHECK(*slot.min_occurrences == 0U);
    REQUIRE(slot.max_occurrences.has_value());
    CHECK(*slot.max_occurrences == 1U);
}

TEST_CASE("cli template schema parser clears reused output result") {
    const auto path = temp_schema_path(".json");
    write_schema_file(
        path,
        R"({"targets":[{"part":"body","slots":[{"bookmark":"customer","kind":"text"}]}]})");

    featherdoc_cli::exported_template_schema_result result;
    featherdoc_cli::exported_template_schema_target stale_target;
    stale_target.entry_name = "stale";
    result.targets.push_back(stale_target);
    result.targets.push_back(stale_target);
    std::string error_message;

    REQUIRE(featherdoc_cli::read_template_schema_file(path, result,
                                                      error_message));

    std::error_code ignored;
    std::filesystem::remove(path, ignored);

    REQUIRE(result.targets.size() == 1U);
    CHECK(result.targets.front().entry_name.empty());
    REQUIRE(result.targets.front().requirements.size() == 1U);
    CHECK(result.targets.front().requirements.front().bookmark_name ==
          "customer");
}

TEST_CASE("cli template schema parser rejects duplicate targets member") {
    featherdoc_cli::exported_template_schema_result result;
    std::string error_message;

    CHECK_FALSE(read_schema(
        R"({"targets":[{"part":"body","slots":[{"bookmark":"customer","kind":"text"}]}],"targets":[]})",
        result, error_message));
    CHECK(error_message.find("must not be duplicated") != std::string::npos);
}

TEST_CASE("cli template schema parser rejects missing targets member") {
    featherdoc_cli::exported_template_schema_result result;
    std::string error_message;

    CHECK_FALSE(read_schema("{}", result, error_message));
    CHECK(error_message.find("must contain a 'targets' array") !=
          std::string::npos);
}
