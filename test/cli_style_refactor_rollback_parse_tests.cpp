#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "featherdoc_cli_style_refactor_rollback_parse.hpp"

namespace {

auto temp_plan_path(std::string_view suffix) -> std::filesystem::path {
    const auto stamp =
        std::chrono::steady_clock::now().time_since_epoch().count();
    return std::filesystem::temp_directory_path() /
           ("featherdoc_cli_style_refactor_rollback_parse_" +
            std::to_string(stamp) + std::string(suffix));
}

void write_text_file(const std::filesystem::path &path,
                     std::string_view content) {
    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    REQUIRE(stream.good());
    stream << content;
    REQUIRE(stream.good());
}

} // namespace

TEST_CASE("cli style refactor rollback parse reads generated rollback files") {
    const auto path = temp_plan_path("_valid.json");
    write_text_file(
        path,
        R"({
  "command": "merge-style",
  "rollback_operations": [
    {
      "action": "rename",
      "source_style_id": "HeadingOld",
      "target_style_id": "HeadingNew"
    },
    {
      "action": "merge",
      "source_style_id": "BodyA",
      "target_style_id": "BodyB",
      "automatic": true,
      "restorable": true,
      "note": null,
      "source_style_xml": "<w:style w:styleId=\"BodyA\"/>",
      "source_usage": {
        "style_id": "BodyA",
        "paragraph_count": 1,
        "run_count": 0,
        "table_count": 0,
        "hits": [
          {
            "part": "body",
            "entry_name": "word/document.xml",
            "section_index": null,
            "ordinal": 3,
            "node_ordinal": 4,
            "kind": "paragraph",
            "future_member": {"nested": [true, null]}
          }
        ]
      }
    }
  ]
})");

    std::vector<featherdoc::style_refactor_rollback_entry> entries;
    std::string error_message;
    const bool parsed = featherdoc_cli::read_style_refactor_rollback_file(
        path, {}, {}, {}, entries, error_message);

    INFO(error_message);
    CHECK(parsed);
    REQUIRE_EQ(entries.size(), 1U);
    CHECK(entries[0].action == featherdoc::style_refactor_action::merge);
    CHECK_EQ(entries[0].source_style_id, "BodyA");
    CHECK_EQ(entries[0].target_style_id, "BodyB");
    CHECK(entries[0].automatic);
    CHECK(entries[0].restorable);
    CHECK_EQ(entries[0].note, "");
    CHECK_EQ(entries[0].source_style_xml, "<w:style w:styleId=\"BodyA\"/>");
    REQUIRE(entries[0].source_usage.has_value());
    CHECK_EQ(entries[0].source_usage->style_id, "BodyA");
    CHECK_EQ(entries[0].source_usage->paragraph_count, 1U);
    REQUIRE_EQ(entries[0].source_usage->hits.size(), 1U);
    CHECK(entries[0].source_usage->hits[0].part ==
          featherdoc::style_usage_part_kind::body);
    CHECK(entries[0].source_usage->hits[0].kind ==
          featherdoc::style_usage_hit_kind::paragraph);
    CHECK_EQ(entries[0].source_usage->hits[0].entry_name,
             "word/document.xml");
    CHECK_FALSE(entries[0].source_usage->hits[0].section_index.has_value());
    CHECK_EQ(entries[0].source_usage->hits[0].ordinal, 3U);
    CHECK_EQ(entries[0].source_usage->hits[0].node_ordinal, 4U);

    std::filesystem::remove(path);
}

TEST_CASE("cli style refactor rollback parse applies restore selection filters") {
    const auto path = temp_plan_path("_filter.json");
    write_text_file(
        path,
        R"({
  "rollback_operations": [
    {
      "action": "merge",
      "source_style_id": "BodyA",
      "target_style_id": "BodyB"
    },
    {
      "action": "merge",
      "source_style_id": "BodyC",
      "target_style_id": "BodyD"
    }
  ]
})");

    std::vector<featherdoc::style_refactor_rollback_entry> entries;
    std::string error_message;
    const bool parsed = featherdoc_cli::read_style_refactor_rollback_file(
        path, {}, {"BodyC"}, {"BodyD"}, entries, error_message);

    INFO(error_message);
    CHECK(parsed);
    REQUIRE_EQ(entries.size(), 1U);
    CHECK_EQ(entries[0].source_style_id, "BodyC");
    CHECK_EQ(entries[0].target_style_id, "BodyD");

    std::filesystem::remove(path);
}

TEST_CASE("cli style refactor rollback parse rejects non merge entry indexes") {
    const auto path = temp_plan_path("_entry.json");
    write_text_file(
        path,
        R"({
  "rollback_operations": [
    {
      "action": "rename",
      "source_style_id": "HeadingOld",
      "target_style_id": "HeadingNew"
    },
    {
      "action": "merge",
      "source_style_id": "BodyA",
      "target_style_id": "BodyB"
    }
  ]
})");

    std::vector<featherdoc::style_refactor_rollback_entry> entries;
    std::string error_message;
    const bool parsed = featherdoc_cli::read_style_refactor_rollback_file(
        path, {0U}, {}, {}, entries, error_message);

    INFO(error_message);
    CHECK_FALSE(parsed);
    CHECK(error_message.find("merge rollback operation") != std::string::npos);

    std::filesystem::remove(path);
}

TEST_CASE("cli style refactor rollback parse rejects duplicate root members") {
    const auto path = temp_plan_path("_duplicate.json");
    write_text_file(
        path,
        R"({
  "rollback_operations": [
    {
      "action": "merge",
      "source_style_id": "BodyA",
      "target_style_id": "BodyB"
    }
  ],
  "rollback_operations": []
})");

    std::vector<featherdoc::style_refactor_rollback_entry> entries;
    std::string error_message;
    const bool parsed = featherdoc_cli::read_style_refactor_rollback_file(
        path, {}, {}, {}, entries, error_message);

    INFO(error_message);
    CHECK_FALSE(parsed);
    CHECK(error_message.find("must not be duplicated") != std::string::npos);

    std::filesystem::remove(path);
}
