#pragma once

#include "featherdoc_cli_table_position_plan_parse.hpp"

#include <featherdoc.hpp>

#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace featherdoc_cli {

struct table_position_preset_plan_item {
    std::size_t table_index = 0U;
    std::string action;
    bool automatic = false;
    std::optional<featherdoc::table_position> current_position;
    featherdoc::table_position target_position{};
    table_position_table_fingerprint fingerprint;
    std::string recommended_command;
    std::optional<std::filesystem::path> resolved_output_path;
    std::optional<std::string> resolved_recommended_command;
};

struct table_position_preset_plan {
    table_position_preset preset{table_position_preset::paragraph_callout};
    featherdoc::table_position target_position{};
    std::size_t table_count = 0U;
    std::size_t positioned_count = 0U;
    std::size_t unpositioned_count = 0U;
    std::size_t already_matching_count = 0U;
    std::size_t set_count = 0U;
    std::size_t replace_count = 0U;
    std::size_t review_count = 0U;
    std::vector<std::size_t> already_matching_table_indices;
    std::vector<std::size_t> review_table_indices;
    std::vector<std::size_t> automatic_table_indices;
    std::optional<std::string> recommended_batch_command;
    std::optional<std::filesystem::path> resolved_output_path;
    std::optional<std::string> resolved_recommended_batch_command;
    std::vector<table_position_table_fingerprint> table_fingerprints;
    std::vector<table_position_preset_plan_item> items;
};

[[nodiscard]] auto make_table_position_preset(table_position_preset preset)
    -> featherdoc::table_position;
[[nodiscard]] auto make_table_position_table_fingerprint(
    const featherdoc::table_inspection_summary &table)
    -> table_position_table_fingerprint;
[[nodiscard]] auto table_position_table_fingerprints_equal(
    const table_position_table_fingerprint &left,
    const table_position_table_fingerprint &right) -> bool;
[[nodiscard]] auto describe_table_position_fingerprint_differences(
    const table_position_table_fingerprint &expected,
    const table_position_table_fingerprint &current) -> std::string;
[[nodiscard]] auto table_positions_equal(const featherdoc::table_position &left,
                                         const featherdoc::table_position &right)
    -> bool;
[[nodiscard]] auto build_table_position_preset_plan(
    const std::vector<featherdoc::table_inspection_summary> &tables,
    table_position_preset preset, bool replace_positioned,
    const std::optional<std::filesystem::path> &input_path,
    const std::optional<std::filesystem::path> &output_path)
    -> table_position_preset_plan;

} // namespace featherdoc_cli
