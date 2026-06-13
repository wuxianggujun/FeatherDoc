#pragma once

#include <string_view>
#include <vector>

namespace featherdoc_cli {

[[nodiscard]] auto run_plan_table_position_presets_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int;

[[nodiscard]] auto run_apply_table_position_plan_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int;

[[nodiscard]] auto run_set_table_position_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int;

[[nodiscard]] auto run_clear_table_position_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int;

} // namespace featherdoc_cli
