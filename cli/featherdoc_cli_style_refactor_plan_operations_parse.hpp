#pragma once

#include <featherdoc.hpp>

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace featherdoc_cli {

[[nodiscard]] auto consume_style_refactor_plan_separator(
    std::string_view content, std::size_t &index, char close_char,
    std::string_view error_detail, bool &closed, std::string &error_message)
    -> bool;

[[nodiscard]] auto parse_style_refactor_plan_operations(
    std::string_view content, std::size_t &index,
    std::vector<featherdoc::style_refactor_request> &requests,
    std::string &error_message) -> bool;

} // namespace featherdoc_cli
