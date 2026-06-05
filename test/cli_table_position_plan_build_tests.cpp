#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "featherdoc_cli_table_position_plan_build.hpp"

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace {

auto make_table(std::size_t index,
                std::optional<featherdoc::table_position> position)
    -> featherdoc::table_inspection_summary {
    featherdoc::table_inspection_summary table;
    table.index = index;
    table.style_id = "TableStyle";
    table.width_twips = 1440U + static_cast<std::uint32_t>(index);
    table.position = position;
    table.row_count = 2U + index;
    table.column_count = 3U;
    table.column_widths = {720U, std::nullopt, 720U};
    table.text = "Table " + std::to_string(index);
    return table;
}

} // namespace

TEST_CASE("cli table position plan build exposes preset positions") {
    const auto page_corner = featherdoc_cli::make_table_position_preset(
        featherdoc_cli::table_position_preset::page_corner);

    CHECK(page_corner.horizontal_reference ==
          featherdoc::table_position_horizontal_reference::page);
    CHECK(page_corner.horizontal_offset_twips == 720);
    CHECK(page_corner.vertical_reference ==
          featherdoc::table_position_vertical_reference::page);
    CHECK(page_corner.vertical_offset_twips == 720);
    REQUIRE(page_corner.left_from_text_twips.has_value());
    CHECK(*page_corner.left_from_text_twips == 144U);
    REQUIRE(page_corner.overlap.has_value());
    CHECK(*page_corner.overlap == featherdoc::table_overlap::never);

    auto changed = page_corner;
    changed.horizontal_offset_twips = 0;
    CHECK(featherdoc_cli::table_positions_equal(page_corner, page_corner));
    CHECK_FALSE(featherdoc_cli::table_positions_equal(page_corner, changed));
}

TEST_CASE("cli table position plan build compares table fingerprints") {
    const auto table = make_table(2U, std::nullopt);
    const auto fingerprint =
        featherdoc_cli::make_table_position_table_fingerprint(table);
    auto changed = fingerprint;
    changed.text = "Changed";
    changed.column_widths = {720U, 360U, 720U};

    CHECK(featherdoc_cli::table_position_table_fingerprints_equal(
        fingerprint, fingerprint));
    CHECK_FALSE(featherdoc_cli::table_position_table_fingerprints_equal(
        fingerprint, changed));

    const auto detail =
        featherdoc_cli::describe_table_position_fingerprint_differences(
            fingerprint, changed);
    CHECK(detail.find("table fingerprint changed since plan was generated") !=
          std::string::npos);
    CHECK(detail.find("column_widths") != std::string::npos);
    CHECK(detail.find("text expected=Table 2 current=Changed") !=
          std::string::npos);
}

TEST_CASE("cli table position plan build classifies set and review actions") {
    const auto preset = featherdoc_cli::table_position_preset::paragraph_callout;
    const auto target = featherdoc_cli::make_table_position_preset(preset);
    auto nonmatching = target;
    nonmatching.horizontal_offset_twips = 240;

    const std::vector<featherdoc::table_inspection_summary> tables{
        make_table(0U, std::nullopt), make_table(1U, target),
        make_table(2U, nonmatching)};

    const auto plan = featherdoc_cli::build_table_position_preset_plan(
        tables, preset, false, std::filesystem::path{"input file.docx"},
        std::nullopt);

    CHECK(plan.table_count == 3U);
    CHECK(plan.positioned_count == 2U);
    CHECK(plan.unpositioned_count == 1U);
    CHECK(plan.already_matching_count == 1U);
    REQUIRE(plan.already_matching_table_indices.size() == 1U);
    CHECK(plan.already_matching_table_indices.front() == 1U);
    CHECK(plan.set_count == 1U);
    CHECK(plan.replace_count == 0U);
    CHECK(plan.review_count == 1U);
    REQUIRE(plan.automatic_table_indices.size() == 1U);
    CHECK(plan.automatic_table_indices.front() == 0U);
    REQUIRE(plan.review_table_indices.size() == 1U);
    CHECK(plan.review_table_indices.front() == 2U);
    REQUIRE(plan.items.size() == 2U);
    CHECK(plan.items[0].action == "set-preset");
    CHECK(plan.items[0].automatic);
    CHECK(plan.items[1].action == "review-existing-position");
    CHECK_FALSE(plan.items[1].automatic);
    REQUIRE(plan.recommended_batch_command.has_value());
    CHECK(*plan.recommended_batch_command ==
          "set-table-position <input.docx> 0 --preset paragraph-callout");
    REQUIRE(plan.resolved_output_path.has_value());
    CHECK(plan.resolved_output_path->filename().string() ==
          "input file-table-position-paragraph-callout.docx");
    REQUIRE(plan.resolved_recommended_batch_command.has_value());
    CHECK(plan.resolved_recommended_batch_command->find(
              "\"input file.docx\" 0 --preset paragraph-callout --output "
              "\"input file-table-position-paragraph-callout.docx\"") !=
          std::string::npos);
}

TEST_CASE("cli table position plan build recommends all-table batch commands") {
    const std::vector<featherdoc::table_inspection_summary> tables{
        make_table(0U, std::nullopt), make_table(1U, std::nullopt)};

    const auto plan = featherdoc_cli::build_table_position_preset_plan(
        tables, featherdoc_cli::table_position_preset::margin_anchor, false,
        std::nullopt, std::filesystem::path{"out.docx"});

    CHECK(plan.set_count == 2U);
    CHECK(plan.automatic_table_indices == std::vector<std::size_t>{0U, 1U});
    REQUIRE(plan.recommended_batch_command.has_value());
    CHECK(*plan.recommended_batch_command ==
          "set-table-position <input.docx> all --preset margin-anchor");
    REQUIRE(plan.resolved_recommended_batch_command.has_value());
    CHECK(*plan.resolved_recommended_batch_command ==
          "set-table-position <input.docx> all --preset margin-anchor --output "
          "out.docx");
}
