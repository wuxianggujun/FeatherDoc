#pragma once

#include "featherdoc_cli_table_position_options_parse.hpp"
#include "featherdoc_cli_table_position_plan_build.hpp"

#include <featherdoc.hpp>

#include <filesystem>
#include <iosfwd>
#include <optional>
#include <string>

namespace featherdoc_cli {

void write_json_table_position(std::ostream &stream,
                               const featherdoc::table_position &position);

void write_json_optional_table_position(
    std::ostream &stream,
    const std::optional<featherdoc::table_position> &position);

void write_table_position_text(
    std::ostream &stream,
    const std::optional<featherdoc::table_position> &position);

void write_json_table_position_preset_plan(
    std::ostream &stream, const table_position_preset_plan &plan,
    const plan_table_position_presets_options &options);

void write_text_table_position_preset_plan(
    const table_position_preset_plan &plan,
    const plan_table_position_presets_options &options);

[[nodiscard]] auto write_table_position_preset_plan_file(
    const std::filesystem::path &output_path,
    const table_position_preset_plan &plan,
    const plan_table_position_presets_options &options,
    std::string &error_message) -> bool;

} // namespace featherdoc_cli
