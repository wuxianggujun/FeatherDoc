#pragma once

#include <string_view>
#include <vector>

namespace featherdoc_cli {

[[nodiscard]] auto run_set_template_table_from_json_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int;

[[nodiscard]] auto run_set_template_tables_from_json_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int;

} // namespace featherdoc_cli
