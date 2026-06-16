#include "featherdoc_cli_review_mutation_plan.hpp"

#include "featherdoc_cli_review_mutation_plan_ranges.hpp"

#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace featherdoc_cli {
namespace {

auto preview_review_mutation_plan_insertion_point(
    featherdoc::Document &doc, std::size_t paragraph_index,
    std::size_t text_offset, review_mutation_plan_preview_result &result)
    -> bool {
    const auto paragraph = doc.inspect_paragraph(paragraph_index);
    if (!paragraph.has_value()) {
        const auto &error_info = doc.last_error();
        if (!error_info.detail.empty()) {
            result.message = error_info.detail;
        } else if (error_info.code) {
            result.message = error_info.code.message();
        } else {
            result.message = "paragraph index is out of range";
        }
        return false;
    }
    if (text_offset > paragraph->text.size()) {
        result.message = "text offset is out of range";
        return false;
    }

    featherdoc::text_range_preview preview;
    preview.start_paragraph_index = paragraph_index;
    preview.start_text_offset = text_offset;
    preview.end_paragraph_index = paragraph_index;
    preview.end_text_offset = text_offset;
    preview.text_length = 0U;
    preview.plain_text_runs_supported = true;
    result.preview = std::move(preview);
    result.ok = true;
    result.message = "ok";
    return true;
}

auto preview_review_mutation_plan_comment_operation(
    featherdoc::Document &doc,
    const review_mutation_plan_operation &operation,
    review_mutation_plan_preview_result &result) -> bool {
    result.comment_index = operation.comment_index;
    result.expected_text = operation.expected_text;
    result.expected_comment_text = operation.expected_comment_text;
    result.expected_resolved = operation.expected_resolved;
    result.expected_parent_index = operation.expected_parent_index;

    const auto comments = doc.list_comments();
    if (operation.comment_index >= comments.size()) {
        result.ok = false;
        result.message = "comment index is out of range";
        return false;
    }

    const auto &comment = comments[operation.comment_index];
    result.actual_comment_text = comment.text;
    result.actual_resolved = comment.resolved;
    if (comment.anchor_text.has_value()) {
        result.actual_text = *comment.anchor_text;
    }
    if (comment.parent_index.has_value()) {
        result.actual_parent_index = *comment.parent_index;
    }

    if (operation.expected_text.has_value() &&
        (!comment.anchor_text.has_value() ||
         *comment.anchor_text != *operation.expected_text)) {
        result.ok = false;
        result.message = "expected text did not match comment anchor text";
        return false;
    }
    if (operation.expected_comment_text.has_value() &&
        comment.text != *operation.expected_comment_text) {
        result.ok = false;
        result.message = "expected comment text did not match comment body";
        return false;
    }
    if (operation.expected_resolved.has_value() &&
        comment.resolved != *operation.expected_resolved) {
        result.ok = false;
        result.message = "expected resolved state did not match comment state";
        return false;
    }
    if (operation.expected_parent_index.has_value() &&
        (!comment.parent_index.has_value() ||
         *comment.parent_index != *operation.expected_parent_index)) {
        result.ok = false;
        result.message =
            "expected parent index did not match comment parent";
        return false;
    }

    result.ok = true;
    result.message = "ok";
    return true;
}

auto is_review_mutation_plan_comment_only_operation(
    review_mutation_plan_operation_kind kind) -> bool {
    return kind == review_mutation_plan_operation_kind::append_comment_reply ||
           kind == review_mutation_plan_operation_kind::replace_comment ||
           kind == review_mutation_plan_operation_kind::remove_comment ||
           kind == review_mutation_plan_operation_kind::set_comment_resolved ||
           kind == review_mutation_plan_operation_kind::set_comment_metadata;
}

} // namespace

auto preview_review_mutation_plan_operations(
    featherdoc::Document &doc,
    const std::vector<review_mutation_plan_operation> &operations)
    -> std::vector<review_mutation_plan_preview_result> {
    std::vector<review_mutation_plan_preview_result> results;
    results.reserve(operations.size());

    for (std::size_t index = 0U; index < operations.size(); ++index) {
        const auto &operation = operations[index];
        review_mutation_plan_preview_result result;
        result.index = index;
        result.kind = operation.kind;
        result.expected_text = operation.expected_text;

        if (is_review_mutation_plan_comment_only_operation(operation.kind)) {
            preview_review_mutation_plan_comment_operation(doc, operation,
                                                           result);
            results.push_back(std::move(result));
            continue;
        }

        std::size_t start_paragraph_index = 0U;
        std::size_t start_text_offset = 0U;
        std::size_t end_paragraph_index = 0U;
        std::size_t end_text_offset = 0U;
        if (!get_review_mutation_plan_range(
                operation, start_paragraph_index, start_text_offset,
                end_paragraph_index, end_text_offset, result.message)) {
            result.ok = false;
            results.push_back(std::move(result));
            continue;
        }

        if (is_review_mutation_plan_insert_operation(operation.kind)) {
            preview_review_mutation_plan_insertion_point(
                doc, start_paragraph_index, start_text_offset, result);
            results.push_back(std::move(result));
            continue;
        }

        auto preview = doc.preview_text_range(
            start_paragraph_index, start_text_offset, end_paragraph_index,
            end_text_offset);
        if (!preview.has_value()) {
            result.ok = false;
            const auto &error_info = doc.last_error();
            result.message = !error_info.detail.empty()
                                 ? error_info.detail
                                 : error_info.code.message();
            results.push_back(std::move(result));
            continue;
        }

        result.preview = *preview;
        if (operation.expected_text.has_value() &&
            preview->text != *operation.expected_text) {
            result.ok = false;
            result.message = "expected text did not match selected text";
            results.push_back(std::move(result));
            continue;
        }

        result.ok = true;
        result.message = "ok";
        results.push_back(std::move(result));
    }

    return results;
}

} // namespace featherdoc_cli
