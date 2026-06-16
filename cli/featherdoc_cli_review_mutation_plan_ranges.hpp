#pragma once

#include "featherdoc_cli_review_mutation_plan.hpp"

#include <cstddef>
#include <string>
#include <vector>

namespace featherdoc_cli {

struct review_mutation_plan_range {
    std::size_t operation_index = 0U;
    std::size_t start_paragraph_index = 0U;
    std::size_t start_text_offset = 0U;
    std::size_t end_paragraph_index = 0U;
    std::size_t end_text_offset = 0U;
};

[[nodiscard]] auto get_review_mutation_plan_range(
    const review_mutation_plan_operation &operation,
    std::size_t &start_paragraph_index, std::size_t &start_text_offset,
    std::size_t &end_paragraph_index, std::size_t &end_text_offset,
    std::string &error_message) -> bool;

[[nodiscard]] auto is_review_mutation_plan_insert_operation(
    review_mutation_plan_operation_kind kind) -> bool;

[[nodiscard]] auto is_review_mutation_plan_text_range_operation(
    review_mutation_plan_operation_kind kind) -> bool;

[[nodiscard]] auto is_review_mutation_plan_comment_operation(
    review_mutation_plan_operation_kind kind) -> bool;

[[nodiscard]] auto build_review_mutation_plan_apply_order(
    const std::vector<review_mutation_plan_operation> &operations,
    std::vector<std::size_t> &operation_indices,
    std::string &error_message) -> bool;

} // namespace featherdoc_cli
