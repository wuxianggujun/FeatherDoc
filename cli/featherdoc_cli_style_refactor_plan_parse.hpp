#pragma once

#include <cstddef>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

#include <featherdoc.hpp>

namespace featherdoc_cli {

[[nodiscard]] auto parse_style_refactor_plan_action(
    std::string_view content, std::size_t &index,
    featherdoc::style_refactor_action &action, std::string &error_message)
    -> bool;
[[nodiscard]] auto read_style_refactor_plan_file(
    const std::filesystem::path &plan_path,
    std::vector<featherdoc::style_refactor_request> &requests,
    std::string &error_message) -> bool;

} // namespace featherdoc_cli
