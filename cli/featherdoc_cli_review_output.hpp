#pragma once

#include "featherdoc_cli_command_support.hpp"
#include "featherdoc_cli_review_mutation_plan.hpp"

#include <featherdoc.hpp>

#include <cstddef>
#include <optional>
#include <ostream>
#include <string_view>
#include <vector>

namespace featherdoc_cli {

void write_json_text_range_preview(
    std::ostream &stream, const featherdoc::text_range_preview &preview);

void write_json_text_range_matches(
    std::ostream &stream, std::string_view command, std::string_view query,
    const std::vector<featherdoc::text_range_preview> &matches);

void print_text_range_preview(std::ostream &stream,
                              const featherdoc::text_range_preview &preview);

void print_text_range_matches(
    std::ostream &stream, std::string_view query,
    const std::vector<featherdoc::text_range_preview> &matches);

void inspect_review(
    const std::vector<featherdoc::review_note_summary> &footnotes,
    const std::vector<featherdoc::review_note_summary> &endnotes,
    const std::vector<featherdoc::review_note_summary> &comments,
    const std::vector<featherdoc::revision_summary> &revisions, bool json_output);

void print_simple_document_mutation_result(
    std::string_view command, const std::optional<path_type> &output_path,
    std::size_t affected);

void write_json_affected_result(std::ostream &stream, std::size_t affected);

[[nodiscard]] auto report_expected_revision_text_mismatch(
    std::string_view command, std::string_view expected_text,
    const featherdoc::text_range_preview &preview, bool json_output) -> bool;

void write_json_review_mutation_plan_preview(
    std::ostream &stream,
    const std::vector<review_mutation_plan_preview_result> &results);

void write_json_review_mutation_plan_apply(
    std::ostream &stream, featherdoc::Document &doc,
    const std::optional<path_type> &output_path,
    const std::vector<review_mutation_plan_preview_result> &results,
    std::size_t applied_count);

void write_json_review_mutation_plan_apply_failure(
    std::ostream &stream, std::string_view stage, std::string_view message,
    const std::vector<review_mutation_plan_preview_result> &results);

void write_json_review_mutation_plan_build_result(
    std::ostream &stream,
    const std::vector<review_mutation_plan_operation> &operations,
    const std::vector<review_mutation_plan_build_resolution> &resolutions,
    const std::optional<path_type> &output_plan_path);

void write_json_review_mutation_plan_build_failure(
    std::ostream &stream, std::string_view stage, std::string_view message,
    std::size_t operation_index, std::size_t matches_count,
    std::size_t raw_matches_count);

void print_review_mutation_plan_preview(
    std::ostream &stream,
    const std::vector<review_mutation_plan_preview_result> &results);

} // namespace featherdoc_cli
