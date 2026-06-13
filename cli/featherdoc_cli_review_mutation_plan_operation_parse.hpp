#pragma once

#include "featherdoc_cli_review_mutation_plan_parse.hpp"

#include <cstddef>
#include <string>
#include <string_view>

namespace featherdoc_cli {

[[nodiscard]] auto parse_review_mutation_plan_operation(
    std::string_view content, std::size_t &index,
    review_mutation_plan_operation &operation, std::string &error_message)
    -> bool;

} // namespace featherdoc_cli
