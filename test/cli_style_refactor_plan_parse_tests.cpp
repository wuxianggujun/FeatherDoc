#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "featherdoc_cli_style_refactor_plan_parse.hpp"

namespace {

auto temp_plan_path(std::string_view suffix) -> std::filesystem::path {
    const auto stamp = std::chrono::steady_clock::now()
                           .time_since_epoch()
                           .count();
    return std::filesystem::temp_directory_path() /
           ("featherdoc_cli_style_refactor_plan_parse_" +
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

TEST_CASE("cli style refactor plan parse accepts documented actions") {
    const std::string text = "\"merge\"";
    std::size_t index = 0U;
    featherdoc::style_refactor_action action{
        featherdoc::style_refactor_action::rename};
    std::string error_message;

    CHECK(featherdoc_cli::parse_style_refactor_plan_action(
        text, index, action, error_message));
    CHECK(action == featherdoc::style_refactor_action::merge);
    CHECK_EQ(index, text.size());
}

TEST_CASE("cli style refactor plan parse reads valid generated plan files") {
    const auto path = temp_plan_path("_valid.json");
    write_text_file(
        path,
        R"({
  "command": "plan-style-refactor",
  "operations": [
    {
      "action": "rename",
      "source_style_id": "HeadingOld",
      "target_style_id": "HeadingNew"
    },
    {
      "action": "merge",
      "source_style_id": "BodyA",
      "target_style_id": "BodyB",
      "ignored_future_member": [true, null]
    }
  ]
})");

    std::vector<featherdoc::style_refactor_request> requests;
    std::string error_message;
    const bool parsed =
        featherdoc_cli::read_style_refactor_plan_file(path, requests,
                                                      error_message);
    INFO(error_message);
    CHECK(parsed);
    REQUIRE_EQ(requests.size(), 2U);
    CHECK(requests[0].action == featherdoc::style_refactor_action::rename);
    CHECK_EQ(requests[0].source_style_id, "HeadingOld");
    CHECK_EQ(requests[0].target_style_id, "HeadingNew");
    CHECK(requests[1].action == featherdoc::style_refactor_action::merge);
    CHECK_EQ(requests[1].source_style_id, "BodyA");
    CHECK_EQ(requests[1].target_style_id, "BodyB");

    std::filesystem::remove(path);
}

TEST_CASE("cli style refactor plan parse rejects empty operations") {
    const auto path = temp_plan_path("_empty.json");
    write_text_file(path, R"({"operations": []})");

    std::vector<featherdoc::style_refactor_request> requests;
    std::string error_message;
    const bool parsed =
        featherdoc_cli::read_style_refactor_plan_file(path, requests,
                                                      error_message);
    INFO(error_message);
    CHECK_FALSE(parsed);
    CHECK(error_message.find("at least one operation") != std::string::npos);

    std::filesystem::remove(path);
}
