#pragma once

#include "featherdoc_cli_review_mutation_plan_parse.hpp"

#include <string>
#include <string_view>
#include <vector>

namespace featherdoc_cli {

[[nodiscard]] auto parse_review_mutation_plan_build_request_operations(
    std::string_view content, std::size_t &index,
    std::vector<review_mutation_plan_build_request_operation> &operations,
    std::string &error_message) -> bool;

} // namespace featherdoc_cli
