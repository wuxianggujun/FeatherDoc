#include "featherdoc_cli_review_mutation_plan_ranges.hpp"

#include <algorithm>
#include <limits>
#include <string>
#include <vector>

namespace featherdoc_cli {

auto get_review_mutation_plan_range(
    const review_mutation_plan_operation &operation,
    std::size_t &start_paragraph_index, std::size_t &start_text_offset,
    std::size_t &end_paragraph_index, std::size_t &end_text_offset,
    std::string &error_message) -> bool {
    switch (operation.kind) {
    case review_mutation_plan_operation_kind::append_paragraph_text_comment:
    case review_mutation_plan_operation_kind::insert_paragraph_text_revision:
        if (operation.kind ==
                review_mutation_plan_operation_kind::
                    append_paragraph_text_comment &&
            operation.text_length >
                std::numeric_limits<std::size_t>::max() -
                    operation.text_offset) {
            error_message = "text range is out of range";
            return false;
        }
        start_paragraph_index = operation.paragraph_index;
        start_text_offset = operation.text_offset;
        end_paragraph_index = operation.paragraph_index;
        end_text_offset =
            operation.kind ==
                    review_mutation_plan_operation_kind::
                        append_paragraph_text_comment
                ? operation.text_offset + operation.text_length
                : operation.text_offset;
        return true;
    case review_mutation_plan_operation_kind::delete_paragraph_text_revision:
    case review_mutation_plan_operation_kind::replace_paragraph_text_revision:
        if (operation.text_length >
            std::numeric_limits<std::size_t>::max() - operation.text_offset) {
            error_message = "text range is out of range";
            return false;
        }
        start_paragraph_index = operation.paragraph_index;
        start_text_offset = operation.text_offset;
        end_paragraph_index = operation.paragraph_index;
        end_text_offset = operation.text_offset + operation.text_length;
        return true;
    case review_mutation_plan_operation_kind::append_text_range_comment:
    case review_mutation_plan_operation_kind::insert_text_range_revision:
        start_paragraph_index = operation.start_paragraph_index;
        start_text_offset = operation.start_text_offset;
        end_paragraph_index =
            operation.kind ==
                    review_mutation_plan_operation_kind::append_text_range_comment
                ? operation.end_paragraph_index
                : operation.start_paragraph_index;
        end_text_offset =
            operation.kind ==
                    review_mutation_plan_operation_kind::append_text_range_comment
                ? operation.end_text_offset
                : operation.start_text_offset;
        return true;
    case review_mutation_plan_operation_kind::delete_text_range_revision:
    case review_mutation_plan_operation_kind::replace_text_range_revision:
        start_paragraph_index = operation.start_paragraph_index;
        start_text_offset = operation.start_text_offset;
        end_paragraph_index = operation.end_paragraph_index;
        end_text_offset = operation.end_text_offset;
        return true;
    case review_mutation_plan_operation_kind::append_comment_reply:
    case review_mutation_plan_operation_kind::replace_comment:
    case review_mutation_plan_operation_kind::remove_comment:
    case review_mutation_plan_operation_kind::set_comment_resolved:
    case review_mutation_plan_operation_kind::set_comment_metadata:
        return false;
    }

    return false;
}

auto is_review_mutation_plan_insert_operation(
    review_mutation_plan_operation_kind kind) -> bool {
    return kind ==
               review_mutation_plan_operation_kind::insert_paragraph_text_revision ||
           kind == review_mutation_plan_operation_kind::insert_text_range_revision;
}

auto is_review_mutation_plan_text_range_operation(
    review_mutation_plan_operation_kind kind) -> bool {
    return kind != review_mutation_plan_operation_kind::append_comment_reply &&
           kind != review_mutation_plan_operation_kind::replace_comment &&
           kind != review_mutation_plan_operation_kind::remove_comment &&
           kind != review_mutation_plan_operation_kind::set_comment_resolved &&
           kind != review_mutation_plan_operation_kind::set_comment_metadata;
}

auto is_review_mutation_plan_comment_operation(
    review_mutation_plan_operation_kind kind) -> bool {
    return kind ==
               review_mutation_plan_operation_kind::append_paragraph_text_comment ||
           kind == review_mutation_plan_operation_kind::append_text_range_comment;
}

namespace {

auto compare_review_mutation_plan_position(std::size_t left_paragraph_index,
                                           std::size_t left_text_offset,
                                           std::size_t right_paragraph_index,
                                           std::size_t right_text_offset)
    -> int {
    if (left_paragraph_index < right_paragraph_index) {
        return -1;
    }
    if (left_paragraph_index > right_paragraph_index) {
        return 1;
    }
    if (left_text_offset < right_text_offset) {
        return -1;
    }
    if (left_text_offset > right_text_offset) {
        return 1;
    }
    return 0;
}

auto compare_review_mutation_plan_range_start(
    const review_mutation_plan_range &left,
    const review_mutation_plan_range &right) -> int {
    return compare_review_mutation_plan_position(
        left.start_paragraph_index, left.start_text_offset,
        right.start_paragraph_index, right.start_text_offset);
}

auto compare_review_mutation_plan_range_end(
    const review_mutation_plan_range &left,
    const review_mutation_plan_range &right) -> int {
    return compare_review_mutation_plan_position(
        left.end_paragraph_index, left.end_text_offset,
        right.end_paragraph_index, right.end_text_offset);
}

auto is_review_mutation_plan_range_empty(
    const review_mutation_plan_range &range) -> bool {
    return compare_review_mutation_plan_position(
               range.start_paragraph_index, range.start_text_offset,
               range.end_paragraph_index, range.end_text_offset) == 0;
}

auto review_mutation_plan_ranges_overlap(
    const review_mutation_plan_range &left,
    const review_mutation_plan_range &right) -> bool {
    if (is_review_mutation_plan_range_empty(left) ||
        is_review_mutation_plan_range_empty(right)) {
        return false;
    }
    return compare_review_mutation_plan_position(
               left.start_paragraph_index, left.start_text_offset,
               right.end_paragraph_index, right.end_text_offset) < 0 &&
           compare_review_mutation_plan_position(
               right.start_paragraph_index, right.start_text_offset,
               left.end_paragraph_index, left.end_text_offset) < 0;
}

auto collect_review_mutation_plan_ranges(
    const std::vector<review_mutation_plan_operation> &operations,
    std::vector<review_mutation_plan_range> &ranges,
    std::string &error_message) -> bool {
    ranges.clear();
    ranges.reserve(operations.size());

    for (std::size_t index = 0U; index < operations.size(); ++index) {
        if (!is_review_mutation_plan_text_range_operation(
                operations[index].kind)) {
            continue;
        }
        review_mutation_plan_range range;
        range.operation_index = index;
        if (!get_review_mutation_plan_range(
                operations[index], range.start_paragraph_index,
                range.start_text_offset, range.end_paragraph_index,
                range.end_text_offset, error_message)) {
            return false;
        }
        ranges.push_back(range);
    }

    return true;
}

} // namespace

auto find_review_mutation_plan_overlap(
    const std::vector<review_mutation_plan_operation> &operations,
    std::string &error_message) -> bool {
    std::vector<review_mutation_plan_range> ranges;
    if (!collect_review_mutation_plan_ranges(operations, ranges,
                                             error_message)) {
        return true;
    }

    for (std::size_t left_index = 0U; left_index < ranges.size();
         ++left_index) {
        for (std::size_t right_index = left_index + 1U;
             right_index < ranges.size(); ++right_index) {
            const auto &previous = ranges[left_index];
            const auto &current = ranges[right_index];
            const auto previous_is_comment =
                is_review_mutation_plan_comment_operation(
                    operations[previous.operation_index].kind);
            const auto current_is_comment =
                is_review_mutation_plan_comment_operation(
                    operations[current.operation_index].kind);
            if ((previous_is_comment && current_is_comment) ||
                !review_mutation_plan_ranges_overlap(previous, current)) {
                continue;
            }
            error_message = "review mutation plan operation ranges overlap: "
                            "operation " +
                            std::to_string(previous.operation_index) +
                            " and operation " +
                            std::to_string(current.operation_index);
            return true;
        }
    }

    return false;
}

auto build_review_mutation_plan_apply_order(
    const std::vector<review_mutation_plan_operation> &operations,
    std::vector<std::size_t> &operation_indices, std::string &error_message)
    -> bool {
    std::vector<review_mutation_plan_range> ranges;
    if (!collect_review_mutation_plan_ranges(operations, ranges,
                                             error_message)) {
        return false;
    }

    std::stable_sort(ranges.begin(), ranges.end(),
                     [&operations](const auto &left, const auto &right) {
                         const auto left_is_comment =
                             is_review_mutation_plan_comment_operation(
                                 operations[left.operation_index].kind);
                         const auto right_is_comment =
                             is_review_mutation_plan_comment_operation(
                                 operations[right.operation_index].kind);
                         const auto start_order =
                             compare_review_mutation_plan_range_start(left,
                                                                      right);
                         if (left_is_comment && right_is_comment) {
                             if (start_order != 0) {
                                 return start_order < 0;
                             }
                             return compare_review_mutation_plan_range_end(
                                        left, right) > 0;
                         }
                         if (start_order != 0) {
                             return start_order > 0;
                         }
                         return compare_review_mutation_plan_range_end(left,
                                                                       right) > 0;
                     });

    operation_indices.clear();
    operation_indices.reserve(operations.size());
    for (const auto &range : ranges) {
        operation_indices.push_back(range.operation_index);
    }
    for (std::size_t index = 0U; index < operations.size(); ++index) {
        if (!is_review_mutation_plan_text_range_operation(
                operations[index].kind)) {
            operation_indices.push_back(index);
        }
    }
    return true;
}

} // namespace featherdoc_cli
