#pragma once

#include <string_view>
#include <vector>

namespace featherdoc_cli {

[[nodiscard]] auto is_template_paragraph_inspect_command(
    std::string_view command) -> bool;

[[nodiscard]] auto run_template_paragraph_inspect_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int;

} // namespace featherdoc_cli
