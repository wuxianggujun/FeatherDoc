#include "featherdoc_cli_review_mutation_plan.hpp"

#include <cstddef>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace featherdoc_cli {
namespace {

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

} // namespace

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
