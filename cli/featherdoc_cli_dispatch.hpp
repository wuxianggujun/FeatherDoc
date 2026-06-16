#pragma once

#include <string_view>
#include <vector>

namespace featherdoc_cli {

[[nodiscard]] auto run_featherdoc_cli_command(
    const std::vector<std::string_view> &arguments) -> int;

} // namespace featherdoc_cli
