#include "featherdoc_cli_review_mutation_plan.hpp"

#include <algorithm>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
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

struct review_mutation_plan_range {
    std::size_t operation_index = 0U;
    std::size_t start_paragraph_index = 0U;
    std::size_t start_text_offset = 0U;
    std::size_t end_paragraph_index = 0U;
    std::size_t end_text_offset = 0U;
};

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

        if (operation.kind ==
                review_mutation_plan_operation_kind::append_comment_reply ||
            operation.kind ==
                review_mutation_plan_operation_kind::replace_comment ||
            operation.kind ==
                review_mutation_plan_operation_kind::remove_comment ||
            operation.kind ==
                review_mutation_plan_operation_kind::set_comment_resolved ||
            operation.kind ==
                review_mutation_plan_operation_kind::set_comment_metadata) {
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

auto apply_review_mutation_plan_operation(
    featherdoc::Document &doc,
    const review_mutation_plan_operation &operation) -> bool {
    const auto author =
        operation.author.has_value() ? std::string_view(*operation.author)
                                     : std::string_view{};
    const auto date =
        operation.date.has_value() ? std::string_view(*operation.date)
                                   : std::string_view{};
    const auto initials =
        operation.initials.has_value() ? std::string_view(*operation.initials)
                                       : std::string_view{};

    switch (operation.kind) {
    case review_mutation_plan_operation_kind::append_comment_reply:
        return doc.append_comment_reply(operation.comment_index,
                                        operation.text, author, initials,
                                        date) != 0U;
    case review_mutation_plan_operation_kind::replace_comment:
        return doc.replace_comment(operation.comment_index, operation.text);
    case review_mutation_plan_operation_kind::remove_comment:
        return doc.remove_comment(operation.comment_index);
    case review_mutation_plan_operation_kind::set_comment_resolved:
        return doc.set_comment_resolved(operation.comment_index,
                                        operation.resolved);
    case review_mutation_plan_operation_kind::set_comment_metadata: {
        featherdoc::comment_metadata_update metadata;
        metadata.author = operation.author;
        metadata.initials = operation.initials;
        metadata.date = operation.date;
        metadata.clear_author = operation.clear_author;
        metadata.clear_initials = operation.clear_initials;
        metadata.clear_date = operation.clear_date;
        return doc.set_comment_metadata(operation.comment_index, metadata);
    }
    case review_mutation_plan_operation_kind::append_paragraph_text_comment:
        return doc.append_paragraph_text_comment(
                   operation.paragraph_index, operation.text_offset,
                   operation.text_length, operation.text, author, initials,
                   date) != 0U;
    case review_mutation_plan_operation_kind::insert_paragraph_text_revision:
        return doc.insert_paragraph_text_revision(
            operation.paragraph_index, operation.text_offset, operation.text,
            author, date);
    case review_mutation_plan_operation_kind::delete_paragraph_text_revision:
        return doc.delete_paragraph_text_revision(
            operation.paragraph_index, operation.text_offset,
            operation.text_length, author, date);
    case review_mutation_plan_operation_kind::replace_paragraph_text_revision:
        return doc.replace_paragraph_text_revision(
            operation.paragraph_index, operation.text_offset,
            operation.text_length, operation.text, author, date);
    case review_mutation_plan_operation_kind::append_text_range_comment:
        return doc.append_text_range_comment(
                   operation.start_paragraph_index, operation.start_text_offset,
                   operation.end_paragraph_index, operation.end_text_offset,
                   operation.text, author, initials, date) != 0U;
    case review_mutation_plan_operation_kind::insert_text_range_revision:
        return doc.insert_text_range_revision(operation.start_paragraph_index,
                                              operation.start_text_offset,
                                              operation.text, author, date);
    case review_mutation_plan_operation_kind::delete_text_range_revision:
        return doc.delete_text_range_revision(
            operation.start_paragraph_index, operation.start_text_offset,
            operation.end_paragraph_index, operation.end_text_offset, author,
            date);
    case review_mutation_plan_operation_kind::replace_text_range_revision:
        return doc.replace_text_range_revision(
            operation.start_paragraph_index, operation.start_text_offset,
            operation.end_paragraph_index, operation.end_text_offset,
            operation.text, author, date);
    }

    return false;
}

auto apply_review_mutation_plan_operations(
    featherdoc::Document &doc,
    const std::vector<review_mutation_plan_operation> &operations,
    std::size_t &applied_count, std::string &error_message) -> bool {
    std::vector<std::size_t> operation_indices;
    if (!build_review_mutation_plan_apply_order(operations, operation_indices,
                                                error_message)) {
        return false;
    }

    applied_count = 0U;
    for (const auto operation_index : operation_indices) {
        if (!apply_review_mutation_plan_operation(doc,
                                                  operations[operation_index])) {
            const auto &error_info = doc.last_error();
            error_message = "failed to apply review mutation plan operation " +
                            std::to_string(operation_index) + ": " +
                            (!error_info.detail.empty()
                                 ? error_info.detail
                                 : error_info.code.message());
            return false;
        }
        ++applied_count;
    }

    return true;
}

auto build_review_mutation_plan_operation_from_match(
    const review_mutation_plan_build_request_operation &request,
    const featherdoc::text_range_preview &preview,
    review_mutation_plan_operation &operation, std::string &error_message)
    -> bool {
    operation = {};
    operation.kind = request.kind;
    operation.text = request.text;
    operation.expected_text = preview.text;
    operation.author = request.author;
    operation.initials = request.initials;
    operation.date = request.date;

    switch (request.kind) {
    case review_mutation_plan_operation_kind::append_comment_reply:
        error_message =
            "append_comment_reply is not supported by build-review-mutation-plan";
        return false;
    case review_mutation_plan_operation_kind::replace_comment:
        error_message =
            "replace_comment is not supported by build-review-mutation-plan";
        return false;
    case review_mutation_plan_operation_kind::remove_comment:
        error_message =
            "remove_comment is not supported by build-review-mutation-plan";
        return false;
    case review_mutation_plan_operation_kind::set_comment_resolved:
        error_message =
            "set_comment_resolved is not supported by build-review-mutation-plan";
        return false;
    case review_mutation_plan_operation_kind::set_comment_metadata:
        error_message =
            "set_comment_metadata is not supported by build-review-mutation-plan";
        return false;
    case review_mutation_plan_operation_kind::append_paragraph_text_comment:
    case review_mutation_plan_operation_kind::insert_paragraph_text_revision:
        if (preview.start_paragraph_index != preview.end_paragraph_index) {
            error_message =
                "matched text crosses paragraphs and cannot be used with paragraph text revision operation";
            return false;
        }
        operation.paragraph_index = preview.start_paragraph_index;
        operation.text_offset =
            operation.kind ==
                    review_mutation_plan_operation_kind::
                        insert_paragraph_text_revision &&
                    request.insert_after_match
                ? preview.end_text_offset
                : preview.start_text_offset;
        operation.text_length =
            operation.kind ==
                    review_mutation_plan_operation_kind::
                        append_paragraph_text_comment
                ? preview.text_length
                : 0U;
        if (operation.kind ==
            review_mutation_plan_operation_kind::insert_paragraph_text_revision) {
            operation.expected_text = std::nullopt;
        }
        return true;
    case review_mutation_plan_operation_kind::delete_paragraph_text_revision:
    case review_mutation_plan_operation_kind::replace_paragraph_text_revision:
        if (preview.start_paragraph_index != preview.end_paragraph_index) {
            error_message =
                "matched text crosses paragraphs and cannot be used with paragraph text revision operation";
            return false;
        }
        operation.paragraph_index = preview.start_paragraph_index;
        operation.text_offset = preview.start_text_offset;
        operation.text_length = preview.text_length;
        return true;
    case review_mutation_plan_operation_kind::append_text_range_comment:
    case review_mutation_plan_operation_kind::insert_text_range_revision:
        operation.start_paragraph_index =
            operation.kind ==
                        review_mutation_plan_operation_kind::
                            insert_text_range_revision &&
                    request.insert_after_match
                ? preview.end_paragraph_index
                : preview.start_paragraph_index;
        operation.start_text_offset = request.insert_after_match
                                          ? preview.end_text_offset
                                          : preview.start_text_offset;
        operation.end_paragraph_index =
            operation.kind ==
                    review_mutation_plan_operation_kind::append_text_range_comment
                ? preview.end_paragraph_index
                : operation.start_paragraph_index;
        operation.end_text_offset =
            operation.kind ==
                    review_mutation_plan_operation_kind::append_text_range_comment
                ? preview.end_text_offset
                : operation.start_text_offset;
        if (operation.kind ==
            review_mutation_plan_operation_kind::insert_text_range_revision) {
            operation.expected_text = std::nullopt;
        }
        return true;
    case review_mutation_plan_operation_kind::delete_text_range_revision:
    case review_mutation_plan_operation_kind::replace_text_range_revision:
        operation.start_paragraph_index = preview.start_paragraph_index;
        operation.start_text_offset = preview.start_text_offset;
        operation.end_paragraph_index = preview.end_paragraph_index;
        operation.end_text_offset = preview.end_text_offset;
        return true;
    }

    return false;
}

struct review_mutation_plan_build_candidate {
    featherdoc::text_range_preview preview;
    std::optional<std::size_t> selected_match_index;
};

auto map_review_mutation_context_offset(
    const featherdoc::text_range_preview &preview, std::size_t text_offset,
    bool prefer_next_segment_at_boundary, std::size_t &paragraph_index,
    std::size_t &paragraph_text_offset) -> bool {
    std::size_t global_offset = 0U;
    std::optional<featherdoc::text_range_preview_segment> previous_segment;
    for (const auto &segment : preview.segments) {
        const auto segment_start = global_offset;
        const auto segment_end = segment_start + segment.text_length;
        if (text_offset < segment_end) {
            paragraph_index = segment.paragraph_index;
            paragraph_text_offset =
                segment.text_offset + (text_offset - segment_start);
            return true;
        }
        if (text_offset == segment_end) {
            if (prefer_next_segment_at_boundary) {
                previous_segment = segment;
                global_offset = segment_end;
                continue;
            }
            paragraph_index = segment.paragraph_index;
            paragraph_text_offset = segment.text_offset + segment.text_length;
            return true;
        }
        previous_segment = segment;
        global_offset = segment_end;
    }

    if (text_offset == global_offset && previous_segment.has_value()) {
        paragraph_index = previous_segment->paragraph_index;
        paragraph_text_offset =
            previous_segment->text_offset + previous_segment->text_length;
        return true;
    }

    return false;
}

auto review_mutation_preview_has_same_range(
    const featherdoc::text_range_preview &left,
    const featherdoc::text_range_preview &right) -> bool {
    return left.start_paragraph_index == right.start_paragraph_index &&
           left.start_text_offset == right.start_text_offset &&
           left.end_paragraph_index == right.end_paragraph_index &&
           left.end_text_offset == right.end_text_offset;
}

auto find_review_mutation_raw_match_index(
    const std::vector<featherdoc::text_range_preview> &raw_matches,
    const featherdoc::text_range_preview &preview)
    -> std::optional<std::size_t> {
    for (std::size_t index = 0U; index < raw_matches.size(); ++index) {
        if (review_mutation_preview_has_same_range(raw_matches[index],
                                                   preview)) {
            return index;
        }
    }
    return std::nullopt;
}

auto build_review_mutation_plan_candidates(
    featherdoc::Document &doc,
    const review_mutation_plan_build_request_operation &request,
    const std::vector<featherdoc::text_range_preview> &raw_matches,
    std::vector<review_mutation_plan_build_candidate> &candidates,
    std::string &error_message) -> bool {
    candidates.clear();

    const auto has_context =
        request.before_text.has_value() || request.after_text.has_value();
    if (!has_context) {
        candidates.reserve(raw_matches.size());
        for (std::size_t index = 0U; index < raw_matches.size(); ++index) {
            review_mutation_plan_build_candidate candidate;
            candidate.preview = raw_matches[index];
            candidate.selected_match_index = index;
            candidates.push_back(std::move(candidate));
        }
        return true;
    }

    std::string context_text;
    if (request.before_text.has_value()) {
        context_text += *request.before_text;
    }
    const auto inner_start_offset = context_text.size();
    context_text += request.find_text;
    const auto inner_end_offset = context_text.size();
    if (request.after_text.has_value()) {
        context_text += *request.after_text;
    }

    auto context_matches = doc.find_text_ranges(context_text);
    if (const auto &error_info = doc.last_error(); error_info.code) {
        error_message = !error_info.detail.empty() ? error_info.detail
                                                   : error_info.code.message();
        return false;
    }

    candidates.reserve(context_matches.size());
    for (const auto &context_preview : context_matches) {
        std::size_t start_paragraph_index = 0U;
        std::size_t start_text_offset = 0U;
        std::size_t end_paragraph_index = 0U;
        std::size_t end_text_offset = 0U;
        if (!map_review_mutation_context_offset(
                context_preview, inner_start_offset, true,
                start_paragraph_index, start_text_offset) ||
            !map_review_mutation_context_offset(
                context_preview, inner_end_offset, false, end_paragraph_index,
                end_text_offset)) {
            error_message =
                "failed to map context match back to requested text range";
            return false;
        }

        auto preview = doc.preview_text_range(start_paragraph_index,
                                              start_text_offset,
                                              end_paragraph_index,
                                              end_text_offset);
        if (const auto &error_info = doc.last_error(); error_info.code) {
            error_message = !error_info.detail.empty()
                                ? error_info.detail
                                : error_info.code.message();
            return false;
        }
        if (!preview.has_value() || preview->text != request.find_text) {
            error_message =
                "context match did not resolve back to requested find_text";
            return false;
        }

        review_mutation_plan_build_candidate candidate;
        candidate.selected_match_index =
            find_review_mutation_raw_match_index(raw_matches, *preview);
        candidate.preview = std::move(*preview);
        candidates.push_back(std::move(candidate));
    }

    return true;
}

auto build_review_mutation_plan_operations(
    featherdoc::Document &doc,
    const std::vector<review_mutation_plan_build_request_operation> &requests,
    std::vector<review_mutation_plan_operation> &operations,
    std::vector<review_mutation_plan_build_resolution> &resolutions,
    std::string &error_message, std::size_t &failed_operation_index,
    std::size_t &failed_matches_count,
    std::size_t &failed_raw_matches_count) -> bool {
    operations.clear();
    operations.reserve(requests.size());
    resolutions.clear();
    resolutions.reserve(requests.size());
    failed_operation_index = 0U;
    failed_matches_count = 0U;
    failed_raw_matches_count = 0U;

    for (std::size_t index = 0U; index < requests.size(); ++index) {
        const auto &request = requests[index];
        auto raw_matches = doc.find_text_ranges(request.find_text);
        if (const auto &error_info = doc.last_error(); error_info.code) {
            failed_operation_index = index;
            error_message = !error_info.detail.empty()
                                ? error_info.detail
                                : error_info.code.message();
            return false;
        }

        std::vector<review_mutation_plan_build_candidate> candidates;
        if (!build_review_mutation_plan_candidates(
                doc, request, raw_matches, candidates, error_message)) {
            failed_operation_index = index;
            failed_raw_matches_count = raw_matches.size();
            return false;
        }

        if (request.require_unique && candidates.size() != 1U) {
            failed_operation_index = index;
            failed_matches_count = candidates.size();
            failed_raw_matches_count = raw_matches.size();
            error_message =
                "requested text did not resolve to a unique match";
            return false;
        }

        if (request.occurrence >= candidates.size()) {
            failed_operation_index = index;
            failed_matches_count = candidates.size();
            failed_raw_matches_count = raw_matches.size();
            error_message = "requested text occurrence was not found";
            return false;
        }

        const auto &candidate = candidates[request.occurrence];
        review_mutation_plan_operation operation;
        if (!build_review_mutation_plan_operation_from_match(
                request, candidate.preview, operation, error_message)) {
            failed_operation_index = index;
            failed_matches_count = candidates.size();
            failed_raw_matches_count = raw_matches.size();
            return false;
        }

        review_mutation_plan_build_resolution resolution;
        resolution.index = index;
        resolution.kind = request.kind;
        resolution.find_text = request.find_text;
        resolution.occurrence = request.occurrence;
        resolution.before_text = request.before_text;
        resolution.after_text = request.after_text;
        resolution.require_unique = request.require_unique;
        resolution.insert_after_match = request.insert_after_match;
        resolution.raw_matches_count = raw_matches.size();
        resolution.matches_count = candidates.size();
        resolution.selected_match_index = candidate.selected_match_index;
        resolution.preview = candidate.preview;
        resolutions.push_back(std::move(resolution));
        operations.push_back(std::move(operation));
    }

    return true;
}

} // namespace featherdoc_cli
