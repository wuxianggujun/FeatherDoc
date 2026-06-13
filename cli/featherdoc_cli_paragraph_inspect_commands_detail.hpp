#pragma once

#include <string_view>
#include <vector>

namespace featherdoc_cli {

auto run_inspect_paragraphs_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int;

auto run_inspect_runs_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int;

} // namespace featherdoc_cli
