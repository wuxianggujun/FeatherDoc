#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "featherdoc_cli_table_position_plan_parse.hpp"

#include <featherdoc/detail/path.hpp>

namespace {

auto temp_plan_path(std::string_view suffix) -> std::filesystem::path {
    const auto stamp =
        std::chrono::steady_clock::now().time_since_epoch().count();
    return std::filesystem::temp_directory_path() /
           ("featherdoc_cli_table_position_plan_parse_" +
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

TEST_CASE("cli table position preset names parse documented tokens") {
    using featherdoc_cli::table_position_preset;

    CHECK_EQ(featherdoc_cli::table_position_preset_name(
                 table_position_preset::paragraph_callout),
             "paragraph-callout");
    CHECK_EQ(featherdoc_cli::table_position_preset_name(
                 table_position_preset::page_corner),
             "page-corner");

    table_position_preset preset{};
    CHECK(featherdoc_cli::parse_table_position_preset("margin-anchor", preset));
    CHECK(preset == table_position_preset::margin_anchor);
    CHECK_FALSE(
        featherdoc_cli::parse_table_position_preset("floating", preset));
}

TEST_CASE("cli table position plan parse reads valid generated plan files") {
    const auto path = temp_plan_path("_valid.json");
    write_text_file(path,
                    R"({
  "command": "plan-table-position-presets",
  "input_path": "input.docx",
  "preset": "page-corner",
  "table_count": 2,
  "automatic_table_indices": [1],
  "resolved_output_path": "output.docx",
  "table_fingerprints": [
    {
      "table_index": 1,
      "style_id": null,
      "width_twips": 1440,
      "row_count": 2,
      "column_count": 3,
      "column_widths": [720, null, 720],
      "text": "Alpha"
    }
  ],
  "ignored_future_member": {"ok": true}
})");

    featherdoc_cli::parsed_table_position_plan_file plan;
    std::string error_message;
    const bool parsed = featherdoc_cli::read_table_position_plan_file(
        path, plan, error_message);
    INFO(error_message);
    CHECK(parsed);
    CHECK_EQ(plan.input_path.string(), "input.docx");
    CHECK(plan.preset == featherdoc_cli::table_position_preset::page_corner);
    CHECK_EQ(plan.table_count, 2U);
    REQUIRE_EQ(plan.automatic_table_indices.size(), 1U);
    CHECK_EQ(plan.automatic_table_indices[0], 1U);
    REQUIRE(plan.resolved_output_path.has_value());
    CHECK_EQ(plan.resolved_output_path->string(), "output.docx");
    REQUIRE_EQ(plan.table_fingerprints.size(), 1U);
    CHECK_EQ(plan.table_fingerprints[0].table_index, 1U);
    CHECK_FALSE(plan.table_fingerprints[0].style_id.has_value());
    REQUIRE(plan.table_fingerprints[0].width_twips.has_value());
    CHECK_EQ(*plan.table_fingerprints[0].width_twips, 1440U);
    CHECK_EQ(plan.table_fingerprints[0].column_widths.size(), 3U);

    std::filesystem::remove(path);
}

TEST_CASE("cli table position plan decodes UTF-8 paths from JSON") {
    const auto path = temp_plan_path("_unicode_paths.json");
    write_text_file(path,
                    R"({
  "command": "plan-table-position-presets",
  "input_path": "目录/输入-日本語-🙂.docx",
  "preset": "page-corner",
  "table_count": 1,
  "automatic_table_indices": [0],
  "resolved_output_path": "输出 目录/结果-中文.docx",
  "table_fingerprints": [{
    "table_index": 0,
    "style_id": null,
    "width_twips": null,
    "row_count": 0,
    "column_count": 0,
    "column_widths": [],
    "text": ""
  }]
})");

    featherdoc_cli::parsed_table_position_plan_file plan;
    std::string error_message;
    INFO(error_message);
    REQUIRE(featherdoc_cli::read_table_position_plan_file(path, plan,
                                                          error_message));
    CHECK_EQ(featherdoc::detail::path_to_utf8(plan.input_path),
             "目录/输入-日本語-🙂.docx");
    REQUIRE(plan.resolved_output_path.has_value());
    CHECK_EQ(featherdoc::detail::path_to_utf8(*plan.resolved_output_path),
             "输出 目录/结果-中文.docx");

    std::filesystem::remove(path);
}

TEST_CASE("cli table position plan rejects escaped NUL in input paths") {
    const auto path = temp_plan_path("_nul_input_path.json");
    write_text_file(path,
                    R"({
  "command": "plan-table-position-presets",
  "input_path": "safe.docx\u0000ignored.docx",
  "preset": "page-corner",
  "table_count": 0,
  "automatic_table_indices": [],
  "table_fingerprints": []
})");

    featherdoc_cli::parsed_table_position_plan_file plan;
    std::string error_message;
    CHECK_FALSE(featherdoc_cli::read_table_position_plan_file(path, plan,
                                                              error_message));
    CHECK_NE(error_message.find("control characters"), std::string::npos);

    std::filesystem::remove(path);
}

TEST_CASE("cli table position plan rejects escaped NUL in output paths") {
    const auto path = temp_plan_path("_nul_output_path.json");
    write_text_file(path,
                    R"({
  "command": "plan-table-position-presets",
  "input_path": "input.docx",
  "preset": "page-corner",
  "table_count": 0,
  "automatic_table_indices": [],
  "resolved_output_path": "safe.docx\u0000ignored.docx",
  "table_fingerprints": []
})");

    featherdoc_cli::parsed_table_position_plan_file plan;
    std::string error_message;
    CHECK_FALSE(featherdoc_cli::read_table_position_plan_file(path, plan,
                                                              error_message));
    CHECK_NE(error_message.find("control characters"), std::string::npos);

    std::filesystem::remove(path);
}

TEST_CASE("cli table position plan parse rejects missing fingerprints") {
    const auto path = temp_plan_path("_missing_fingerprint.json");
    write_text_file(path,
                    R"({
  "command": "plan-table-position-presets",
  "input_path": "input.docx",
  "preset": "paragraph-callout",
  "table_count": 1,
  "automatic_table_indices": [0],
  "table_fingerprints": []
})");

    featherdoc_cli::parsed_table_position_plan_file plan;
    std::string error_message;
    const bool parsed = featherdoc_cli::read_table_position_plan_file(
        path, plan, error_message);
    INFO(error_message);
    CHECK_FALSE(parsed);
    CHECK(error_message.find("missing table fingerprint") != std::string::npos);

    std::filesystem::remove(path);
}
