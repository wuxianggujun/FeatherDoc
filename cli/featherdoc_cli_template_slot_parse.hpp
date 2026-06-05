#pragma once

#include <featherdoc.hpp>

#include <string>
#include <string_view>

namespace featherdoc_cli {

[[nodiscard]] auto parse_template_slot_kind(
    std::string_view text, featherdoc::template_slot_kind &kind) -> bool;
[[nodiscard]] auto parse_template_slot_requirement(
    std::string_view text, featherdoc::template_slot_requirement &requirement,
    std::string &error_message) -> bool;

} // namespace featherdoc_cli
