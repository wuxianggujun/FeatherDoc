#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <system_error>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "featherdoc_cli_template_schema_patch_parse.hpp"

namespace {

auto temp_patch_path(std::string_view suffix) -> std::filesystem::path {
    const auto stamp =
        std::chrono::steady_clock::now().time_since_epoch().count();
    return std::filesystem::temp_directory_path() /
           ("featherdoc_cli_template_schema_patch_parse_" +
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
                featherdoc_cli::template_schema_patch_document &patch,
                std::string &error_message) -> bool {
    const auto path = temp_patch_path(".json");
    write_patch_file(path, content);

    patch = {};
    error_message.clear();
    const auto parsed =
        featherdoc_cli::read_template_schema_patch_file(path, patch,
                                                        error_message);

    std::error_code ignored;
    std::filesystem::remove(path, ignored);
    return parsed;
}

} // namespace

TEST_CASE("cli template schema patch parser reads all operation groups") {
    featherdoc_cli::template_schema_patch_document patch;
    std::string error_message;

    REQUIRE(read_patch(
        R"({
  "upsert_targets": [
    {"part": "body", "slots": [{"bookmark": "customer", "kind": "text"}]}
  ],
  "remove_targets": [
    {"part": "section-footer", "section": 1, "kind": "default"}
  ],
  "remove_slots": [
    {"part": "body", "bookmark": "old_customer"}
  ],
  "rename_slots": [
    {"part": "body", "bookmark": "old_name", "new_bookmark": "new_name"}
  ],
  "update_slots": [
    {
      "part": "body",
      "content_control_tag": "invoice_id",
      "slot_kind": "image",
      "required": false,
      "min_occurrences": 0,
      "max_occurrences": 1
    }
  ]
})",
        patch, error_message));

    REQUIRE(patch.upsert_targets.size() == 1U);
    CHECK(patch.upsert_targets.front().requirements.front().bookmark_name ==
          "customer");

    REQUIRE(patch.remove_targets.size() == 1U);
    CHECK(patch.remove_targets.front().part ==
          featherdoc_cli::validation_part_family::section_footer);
    REQUIRE(patch.remove_targets.front().section_index.has_value());
    CHECK(*patch.remove_targets.front().section_index == 1U);

    REQUIRE(patch.remove_slots.size() == 1U);
    CHECK(patch.remove_slots.front().bookmark_name == "old_customer");
    CHECK(patch.remove_slots.front().source ==
          featherdoc::template_slot_source_kind::bookmark);

    REQUIRE(patch.rename_slots.size() == 1U);
    CHECK(patch.rename_slots.front().bookmark_name == "old_name");
    CHECK(patch.rename_slots.front().new_bookmark_name == "new_name");

    REQUIRE(patch.update_slots.size() == 1U);
    const auto &update = patch.update_slots.front();
    CHECK(update.bookmark_name == "invoice_id");
    CHECK(update.source ==
          featherdoc::template_slot_source_kind::content_control_tag);
    REQUIRE(update.update.kind.has_value());
    CHECK(*update.update.kind == featherdoc::template_slot_kind::image);
    REQUIRE(update.update.required.has_value());
    CHECK_FALSE(*update.update.required);
    REQUIRE(update.update.min_occurrences.has_value());
    CHECK(*update.update.min_occurrences == 0U);
    REQUIRE(update.update.max_occurrences.has_value());
    CHECK(*update.update.max_occurrences == 1U);
}

TEST_CASE("cli template schema patch parser rejects duplicate root members") {
    featherdoc_cli::template_schema_patch_document patch;
    std::string error_message;

    CHECK_FALSE(read_patch(R"({"remove_targets":[],"remove_targets":[]})",
                           patch, error_message));
    CHECK(error_message.find("must not be duplicated") != std::string::npos);
}

TEST_CASE("cli template schema patch parser rejects empty update operations") {
    featherdoc_cli::template_schema_patch_document patch;
    std::string error_message;

    CHECK_FALSE(read_patch(
        R"({"update_slots":[{"part":"body","bookmark":"customer"}]})", patch,
        error_message));
    CHECK(error_message.find("at least one update field") != std::string::npos);
}

TEST_CASE("cli template schema patch parser rewrites upsert schema errors") {
    featherdoc_cli::template_schema_patch_document patch;
    std::string error_message;

    CHECK_FALSE(read_patch(
        R"({"upsert_targets":[{"part":"body","slots":[{"bookmark":"customer","kind":"unknown"}]}]})",
        patch, error_message));
    CHECK(error_message.find("JSON schema patch slot member 'kind'") !=
          std::string::npos);
}

TEST_CASE("cli template schema patch parser rejects rename source changes") {
    featherdoc_cli::template_schema_patch_document patch;
    std::string error_message;

    CHECK_FALSE(read_patch(
        R"({"rename_slots":[{"part":"body","content_control_tag":"old_id","new_content_control_alias":"New ID"}]})",
        patch, error_message));
    CHECK(error_message.find("cannot change slot source") != std::string::npos);
}
