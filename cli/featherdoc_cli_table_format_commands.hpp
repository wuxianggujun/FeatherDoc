#pragma once

#include <featherdoc.hpp>

#include <string_view>
#include <vector>

namespace featherdoc_cli {

[[nodiscard]] auto is_table_format_command(std::string_view command) -> bool;

[[nodiscard]] auto run_table_format_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int;

} // namespace featherdoc_cli
