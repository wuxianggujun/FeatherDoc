#pragma once

#include "featherdoc_cli_template_schema_model.hpp"

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace featherdoc_cli {

[[nodiscard]] auto parse_template_schema_json_slots(
    std::string_view content, std::size_t &index,
    std::vector<featherdoc::template_slot_requirement> &slots,
    std::string &error_message) -> bool;

} // namespace featherdoc_cli
