#pragma once

#include <featherdoc.hpp>

#include <string_view>
#include <vector>

namespace featherdoc_cli {

[[nodiscard]] auto run_inspect_update_fields_on_open_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int;

[[nodiscard]] auto run_set_update_fields_on_open_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int;

} // namespace featherdoc_cli
