#pragma once

#include <string_view>
#include <vector>

namespace featherdoc_cli {

[[nodiscard]] auto run_inspect_template_paragraphs_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int;

[[nodiscard]] auto run_inspect_template_runs_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int;

} // namespace featherdoc_cli
