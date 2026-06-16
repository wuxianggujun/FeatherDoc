#pragma once

#include <string_view>
#include <vector>

namespace featherdoc_cli {

[[nodiscard]] auto run_insert_table_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    bool insert_before) -> int;

[[nodiscard]] auto run_insert_table_like_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    bool insert_before) -> int;

[[nodiscard]] auto run_remove_table_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int;

} // namespace featherdoc_cli
