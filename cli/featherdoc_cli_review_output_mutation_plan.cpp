#include "featherdoc_cli_review_output.hpp"

#include "featherdoc_cli_json.hpp"
#include "featherdoc_cli_review_mutation_plan_parse.hpp"

#include <algorithm>
#include <ostream>
#include <string_view>
#include <vector>

namespace featherdoc_cli {
namespace {

void write_json_review_mutation_plan_preview_result(
    std::ostream &stream,
    const review_mutation_plan_preview_result &result) {
    stream << "{\"index\":" << result.index << ",\"kind\":";
    write_json_string(stream, review_mutation_plan_operation_kind_name(result.kind));
    stream << ",\"ok\":" << json_bool(result.ok) << ",\"message\":";
    write_json_string(stream, result.message);
    if (result.comment_index.has_value()) {
        stream << ",\"comment_index\":" << *result.comment_index;
    }
    stream << ",\"expected_text\":";
    if (result.expected_text.has_value()) {
        write_json_string(stream, *result.expected_text);
    } else {
        stream << "null";
    }
    if (result.expected_resolved.has_value()) {
        stream << ",\"expected_resolved\":"
               << json_bool(*result.expected_resolved);
    }
    if (result.expected_comment_text.has_value()) {
        stream << ",\"expected_comment_text\":";
        write_json_string(stream, *result.expected_comment_text);
    }
    if (result.expected_parent_index.has_value()) {
        stream << ",\"expected_parent_index\":"
               << *result.expected_parent_index;
    }
    stream << ",\"actual_text\":";
    if (result.actual_text.has_value()) {
        write_json_string(stream, *result.actual_text);
    } else if (result.preview.has_value()) {
        write_json_string(stream, result.preview->text);
    } else {
        stream << "null";
    }
    if (result.actual_resolved.has_value()) {
        stream << ",\"actual_resolved\":" << json_bool(*result.actual_resolved);
    }
    if (result.actual_comment_text.has_value()) {
        stream << ",\"actual_comment_text\":";
        write_json_string(stream, *result.actual_comment_text);
    }
    if (result.comment_index.has_value() ||
        result.expected_parent_index.has_value() ||
        result.actual_parent_index.has_value()) {
        stream << ",\"actual_parent_index\":";
        if (result.actual_parent_index.has_value()) {
            stream << *result.actual_parent_index;
        } else {
            stream << "null";
        }
    }
    stream << ",\"preview\":";
    if (result.preview.has_value()) {
        write_json_text_range_preview(stream, *result.preview);
    } else {
        stream << "null";
    }
    stream << '}';
}

void write_json_review_mutation_plan_build_resolution(
    std::ostream &stream,
    const review_mutation_plan_build_resolution &resolution) {
    stream << "{\"index\":" << resolution.index << ",\"kind\":";
    write_json_string(stream,
                      review_mutation_plan_operation_kind_name(resolution.kind));
    stream << ",\"find_text\":";
    write_json_string(stream, resolution.find_text);
    stream << ",\"before_text\":";
    if (resolution.before_text.has_value()) {
        write_json_string(stream, *resolution.before_text);
    } else {
        stream << "null";
    }
    stream << ",\"after_text\":";
    if (resolution.after_text.has_value()) {
        write_json_string(stream, *resolution.after_text);
    } else {
        stream << "null";
    }
    stream << ",\"require_unique\":" << json_bool(resolution.require_unique)
           << ",\"insert_after_match\":"
           << json_bool(resolution.insert_after_match)
           << ",\"occurrence\":" << resolution.occurrence
           << ",\"raw_matches_count\":" << resolution.raw_matches_count
           << ",\"matches_count\":" << resolution.matches_count
           << ",\"selected_match_index\":";
    if (resolution.selected_match_index.has_value()) {
        stream << *resolution.selected_match_index;
    } else {
        stream << "null";
    }
    stream << ",\"preview\":";
    write_json_text_range_preview(stream, resolution.preview);
    stream << '}';
}

} // namespace

void write_json_review_mutation_plan_preview(
    std::ostream &stream,
    const std::vector<review_mutation_plan_preview_result> &results) {
    const auto failed_count =
        static_cast<std::size_t>(std::count_if(results.begin(), results.end(),
                                               [](const auto &result) {
                                                   return !result.ok;
                                               }));
    stream << "{\"command\":\"preview-review-mutation-plan\",\"ok\":"
           << json_bool(failed_count == 0U)
           << ",\"operations_count\":" << results.size()
           << ",\"failed_count\":" << failed_count << ",\"operations\":[";
    for (std::size_t index = 0U; index < results.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        write_json_review_mutation_plan_preview_result(stream, results[index]);
    }
    stream << "]}\n";
}

void write_json_review_mutation_plan_apply(
    std::ostream &stream, featherdoc::Document &doc,
    const std::optional<path_type> &output_path,
    const std::vector<review_mutation_plan_preview_result> &results,
    std::size_t applied_count) {
    const auto failed_count =
        static_cast<std::size_t>(std::count_if(results.begin(), results.end(),
                                               [](const auto &result) {
                                                   return !result.ok;
                                               }));
    stream << "{\"command\":\"apply-review-mutation-plan\",\"ok\":true"
           << ",\"in_place\":" << json_bool(!output_path.has_value())
           << ",\"output_path\":";
    if (output_path.has_value()) {
        write_json_string(stream, output_path->string());
    } else {
        stream << "null";
    }
    stream << ",\"sections\":" << doc.section_count()
           << ",\"headers\":" << doc.header_count()
           << ",\"footers\":" << doc.footer_count()
           << ",\"operations_count\":" << results.size()
           << ",\"applied_count\":" << applied_count
           << ",\"failed_count\":" << failed_count << ",\"operations\":[";
    for (std::size_t index = 0U; index < results.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        write_json_review_mutation_plan_preview_result(stream, results[index]);
    }
    stream << "]}\n";
}

void write_json_review_mutation_plan_apply_failure(
    std::ostream &stream, std::string_view stage, std::string_view message,
    const std::vector<review_mutation_plan_preview_result> &results) {
    const auto failed_count =
        static_cast<std::size_t>(std::count_if(results.begin(), results.end(),
                                               [](const auto &result) {
                                                   return !result.ok;
                                               }));
    stream << "{\"command\":\"apply-review-mutation-plan\",\"ok\":false"
           << ",\"stage\":";
    write_json_string(stream, stage);
    stream << ",\"message\":";
    write_json_string(stream, message);
    stream << ",\"operations_count\":" << results.size()
           << ",\"failed_count\":" << failed_count << ",\"operations\":[";
    for (std::size_t index = 0U; index < results.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        write_json_review_mutation_plan_preview_result(stream, results[index]);
    }
    stream << "]}\n";
}

void write_json_review_mutation_plan_build_result(
    std::ostream &stream,
    const std::vector<review_mutation_plan_operation> &operations,
    const std::vector<review_mutation_plan_build_resolution> &resolutions,
    const std::optional<path_type> &output_plan_path) {
    stream << "{\"command\":\"build-review-mutation-plan\",\"ok\":true"
           << ",\"operations_count\":" << operations.size()
           << ",\"output_plan_path\":";
    if (output_plan_path.has_value()) {
        write_json_string(stream, output_plan_path->string());
    } else {
        stream << "null";
    }
    stream << ",\"plan\":";
    write_json_review_mutation_plan_document(stream, operations);
    stream << ",\"resolutions\":[";
    for (std::size_t index = 0U; index < resolutions.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        write_json_review_mutation_plan_build_resolution(stream,
                                                         resolutions[index]);
    }
    stream << "]}\n";
}

void write_json_review_mutation_plan_build_failure(
    std::ostream &stream, std::string_view stage, std::string_view message,
    std::size_t operation_index, std::size_t matches_count,
    std::size_t raw_matches_count) {
    stream << "{\"command\":\"build-review-mutation-plan\",\"ok\":false"
           << ",\"stage\":";
    write_json_string(stream, stage);
    stream << ",\"message\":";
    write_json_string(stream, message);
    stream << ",\"operation_index\":" << operation_index
           << ",\"matches_count\":" << matches_count
           << ",\"raw_matches_count\":" << raw_matches_count << "}\n";
}

void print_review_mutation_plan_preview(
    std::ostream &stream,
    const std::vector<review_mutation_plan_preview_result> &results) {
    const auto failed_count =
        static_cast<std::size_t>(std::count_if(results.begin(), results.end(),
                                               [](const auto &result) {
                                                   return !result.ok;
                                               }));
    stream << "ok=" << yes_no(failed_count == 0U)
           << " operations_count=" << results.size()
           << " failed_count=" << failed_count;

    for (const auto &result : results) {
        stream << '\n'
               << "operation index=" << result.index
               << " kind="
               << review_mutation_plan_operation_kind_name(result.kind)
               << " ok=" << yes_no(result.ok) << " message=";
        write_json_string(stream, result.message);
        if (result.expected_text.has_value()) {
            stream << " expected_text=";
            write_json_string(stream, *result.expected_text);
        }
        if (result.expected_resolved.has_value()) {
            stream << " expected_resolved="
                   << json_bool(*result.expected_resolved);
        }
        if (result.expected_comment_text.has_value()) {
            stream << " expected_comment_text=";
            write_json_string(stream, *result.expected_comment_text);
        }
        if (result.expected_parent_index.has_value()) {
            stream << " expected_parent_index="
                   << *result.expected_parent_index;
        }
        if (result.actual_text.has_value()) {
            stream << " actual_text=";
            write_json_string(stream, *result.actual_text);
        }
        if (result.actual_resolved.has_value()) {
            stream << " actual_resolved="
                   << json_bool(*result.actual_resolved);
        }
        if (result.actual_comment_text.has_value()) {
            stream << " actual_comment_text=";
            write_json_string(stream, *result.actual_comment_text);
        }
        if (result.actual_parent_index.has_value()) {
            stream << " actual_parent_index="
                   << *result.actual_parent_index;
        }
        if (result.preview.has_value()) {
            stream << " actual_text=";
            write_json_string(stream, result.preview->text);
            stream << '\n';
            print_text_range_preview(stream, *result.preview);
        }
    }
    stream << '\n';
}

} // namespace featherdoc_cli
