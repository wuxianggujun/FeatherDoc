#pragma once

#include <featherdoc.hpp>

#include <string>
#include <string_view>

namespace featherdoc_cli {

[[nodiscard]] auto parse_style_refactor_pair(
    std::string_view text, featherdoc::style_refactor_action action,
    featherdoc::style_refactor_request &request, std::string &error_message)
    -> bool;

} // namespace featherdoc_cli
