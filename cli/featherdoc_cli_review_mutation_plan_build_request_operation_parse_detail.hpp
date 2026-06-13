#pragma once

#include "featherdoc_cli_review_mutation_plan_build_request_operation_parse.hpp"

#include <cstddef>
#include <string>
#include <string_view>

namespace featherdoc_cli {

struct review_mutation_plan_build_request_operation_members {
    bool saw_kind = false;
    bool saw_find_text = false;
    bool saw_occurrence = false;
    bool saw_before_text = false;
    bool saw_after_text = false;
    bool saw_require_unique = false;
    bool saw_insert_after_match = false;
    bool saw_text = false;
    bool saw_comment_text = false;
    bool saw_author = false;
    bool saw_initials = false;
    bool saw_date = false;
};

[[nodiscard]] auto parse_review_mutation_plan_build_request_operation_member(
    std::string_view content, std::size_t &index, std::string_view member_name,
    review_mutation_plan_build_request_operation &operation,
    review_mutation_plan_build_request_operation_members &members,
    std::string &error_message) -> bool;

[[nodiscard]] auto validate_review_mutation_plan_build_request_operation(
    const review_mutation_plan_build_request_operation &operation,
    const review_mutation_plan_build_request_operation_members &members,
    std::string &error_message) -> bool;

} // namespace featherdoc_cli
