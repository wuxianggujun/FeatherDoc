#pragma once

#include <string_view>
#include <vector>

namespace featherdoc_cli {

[[nodiscard]] auto run_inspect_template_tables_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int;

[[nodiscard]] auto run_inspect_template_table_rows_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int;

[[nodiscard]] auto run_inspect_template_table_cells_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int;

} // namespace featherdoc_cli
