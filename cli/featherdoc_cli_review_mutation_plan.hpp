#pragma once

#include "featherdoc_cli_review_mutation_plan_parse.hpp"

#include <featherdoc.hpp>

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace featherdoc_cli {

struct review_mutation_plan_preview_result {
    std::size_t index = 0U;
    review_mutation_plan_operation_kind kind =
        review_mutation_plan_operation_kind::delete_paragraph_text_revision;
    bool ok = false;
    std::string message;
    std::optional<std::string> expected_text;
    std::optional<std::string> actual_text;
    std::optional<std::string> expected_comment_text;
    std::optional<std::string> actual_comment_text;
    std::optional<std::size_t> comment_index;
    std::optional<bool> expected_resolved;
    std::optional<bool> actual_resolved;
    std::optional<std::size_t> expected_parent_index;
    std::optional<std::size_t> actual_parent_index;
    std::optional<featherdoc::text_range_preview> preview;
};

struct review_mutation_plan_build_resolution {
    std::size_t index = 0U;
    review_mutation_plan_operation_kind kind =
        review_mutation_plan_operation_kind::replace_text_range_revision;
    std::string find_text;
    std::size_t occurrence = 0U;
    std::optional<std::string> before_text;
    std::optional<std::string> after_text;
    bool require_unique = false;
    bool insert_after_match = false;
    std::size_t raw_matches_count = 0U;
    std::size_t matches_count = 0U;
    std::optional<std::size_t> selected_match_index;
    featherdoc::text_range_preview preview;
};

[[nodiscard]] auto find_review_mutation_plan_overlap(
    const std::vector<review_mutation_plan_operation> &operations,
    std::string &error_message) -> bool;

[[nodiscard]] auto preview_review_mutation_plan_operations(
    featherdoc::Document &doc,
    const std::vector<review_mutation_plan_operation> &operations)
    -> std::vector<review_mutation_plan_preview_result>;

[[nodiscard]] auto apply_review_mutation_plan_operations(
    featherdoc::Document &doc,
    const std::vector<review_mutation_plan_operation> &operations,
    std::size_t &applied_count, std::string &error_message) -> bool;

[[nodiscard]] auto build_review_mutation_plan_operations(
    featherdoc::Document &doc,
    const std::vector<review_mutation_plan_build_request_operation> &requests,
    std::vector<review_mutation_plan_operation> &operations,
    std::vector<review_mutation_plan_build_resolution> &resolutions,
    std::string &error_message, std::size_t &failed_operation_index,
    std::size_t &failed_matches_count,
    std::size_t &failed_raw_matches_count) -> bool;

} // namespace featherdoc_cli
