#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "featherdoc_cli_review_mutation_plan_parse.hpp"

namespace {

auto temp_plan_path(std::string_view suffix) -> std::filesystem::path {
    const auto stamp = std::chrono::steady_clock::now()
                           .time_since_epoch()
                           .count();
    return std::filesystem::temp_directory_path() /
           ("featherdoc_cli_review_mutation_plan_parse_" +
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

TEST_CASE("cli review mutation plan kind names parse canonical and CLI tokens") {
    using featherdoc_cli::review_mutation_plan_operation_kind;

    CHECK_EQ(featherdoc_cli::review_mutation_plan_operation_kind_name(
                 review_mutation_plan_operation_kind::set_comment_metadata),
             "set_comment_metadata");

    const std::string text = "\"replace-text-range-revision\"";
    std::size_t index = 0U;
    review_mutation_plan_operation_kind kind{};
    std::string error_message;
    CHECK(featherdoc_cli::parse_review_mutation_plan_operation_kind(
        text, index, kind, error_message));
    CHECK(kind ==
          review_mutation_plan_operation_kind::replace_text_range_revision);
    CHECK_EQ(index, text.size());
}

TEST_CASE("cli review mutation plan parse reads valid operations") {
    using featherdoc_cli::review_mutation_plan_operation_kind;

    const auto path = temp_plan_path("_valid.json");
    write_text_file(
        path,
        R"({
  "operations": [
    {
      "kind": "replace_text_range_revision",
      "start_paragraph_index": 1,
      "start_text_offset": 2,
      "end_paragraph_index": 1,
      "end_text_offset": 7,
      "text": "updated",
      "expected_text": "before",
      "author": "Ada",
      "date": "2026-06-04T00:00:00Z"
    },
    {
      "kind": "set_comment_metadata",
      "comment_index": 3,
      "author": "Grace",
      "clear_initials": true,
      "expected_parent_index": 1,
      "ignored_future_member": {"ok": true}
    }
  ]
})");

    std::vector<featherdoc_cli::review_mutation_plan_operation> operations;
    std::string error_message;
    const bool parsed =
        featherdoc_cli::read_review_mutation_plan_file(path, operations,
                                                       error_message);
    INFO(error_message);
    CHECK(parsed);
    REQUIRE_EQ(operations.size(), 2U);
    CHECK(operations[0].kind ==
          review_mutation_plan_operation_kind::replace_text_range_revision);
    CHECK_EQ(operations[0].start_paragraph_index, 1U);
    CHECK_EQ(operations[0].end_text_offset, 7U);
    CHECK_EQ(operations[0].text, "updated");
    REQUIRE(operations[0].expected_text.has_value());
    CHECK_EQ(*operations[0].expected_text, "before");
    CHECK(operations[1].kind ==
          review_mutation_plan_operation_kind::set_comment_metadata);
    CHECK_EQ(operations[1].comment_index, 3U);
    REQUIRE(operations[1].author.has_value());
    CHECK_EQ(*operations[1].author, "Grace");
    CHECK(operations[1].clear_initials);

    std::filesystem::remove(path);
}

TEST_CASE("cli review mutation plan parse writes readable plan documents") {
    using featherdoc_cli::review_mutation_plan_operation_kind;

    const auto path = temp_plan_path("_roundtrip.json");
    featherdoc_cli::review_mutation_plan_operation operation;
    operation.kind =
        review_mutation_plan_operation_kind::insert_paragraph_text_revision;
    operation.paragraph_index = 2U;
    operation.text_offset = 4U;
    operation.text = "inserted";
    operation.author = "Ada";

    std::string error_message;
    CHECK(featherdoc_cli::write_review_mutation_plan_file(
        path, {operation}, error_message));

    std::vector<featherdoc_cli::review_mutation_plan_operation> operations;
    CHECK(featherdoc_cli::read_review_mutation_plan_file(path, operations,
                                                         error_message));
    REQUIRE_EQ(operations.size(), 1U);
    CHECK(operations[0].kind ==
          review_mutation_plan_operation_kind::insert_paragraph_text_revision);
    CHECK_EQ(operations[0].paragraph_index, 2U);
    CHECK_EQ(operations[0].text, "inserted");
    REQUIRE(operations[0].author.has_value());
    CHECK_EQ(*operations[0].author, "Ada");

    std::filesystem::remove(path);
}

TEST_CASE("cli review mutation plan parse reads build requests") {
    using featherdoc_cli::review_mutation_plan_operation_kind;

    const auto path = temp_plan_path("_build_request.json");
    write_text_file(
        path,
        R"({
  "operations": [
    {
      "kind": "append-text-range-comment",
      "find_text": "needle",
      "comment_text": "review",
      "before_text": "before",
      "after_text": "after",
      "require_unique": "true",
      "author": "Ada",
      "initials": "AL"
    }
  ]
})");

    std::vector<featherdoc_cli::review_mutation_plan_build_request_operation>
        operations;
    std::string error_message;
    const bool parsed =
        featherdoc_cli::read_review_mutation_plan_build_request_file(
            path, operations, error_message);
    INFO(error_message);
    CHECK(parsed);
    REQUIRE_EQ(operations.size(), 1U);
    CHECK(operations[0].kind ==
          review_mutation_plan_operation_kind::append_text_range_comment);
    CHECK_EQ(operations[0].find_text, "needle");
    CHECK_EQ(operations[0].text, "review");
    CHECK(operations[0].require_unique);
    REQUIRE(operations[0].initials.has_value());
    CHECK_EQ(*operations[0].initials, "AL");

    std::filesystem::remove(path);
}

TEST_CASE("cli review mutation plan parse rejects invalid combinations") {
    const auto path = temp_plan_path("_invalid.json");
    write_text_file(
        path,
        R"({
  "operations": [
    {
      "kind": "set_comment_metadata",
      "comment_index": 0,
      "author": "Ada",
      "clear_author": true
    }
  ]
})");

    std::vector<featherdoc_cli::review_mutation_plan_operation> operations;
    std::string error_message;
    const bool parsed =
        featherdoc_cli::read_review_mutation_plan_file(path, operations,
                                                       error_message);
    INFO(error_message);
    CHECK_FALSE(parsed);
    CHECK(error_message.find("cannot set and clear") != std::string::npos);

    std::filesystem::remove(path);
}
