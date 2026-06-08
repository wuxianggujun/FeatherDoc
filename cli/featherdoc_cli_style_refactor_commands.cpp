#include "featherdoc_cli_style_refactor_commands.hpp"

#include "featherdoc_cli_command_support.hpp"
#include "featherdoc_cli_domain_names.hpp"
#include "featherdoc_cli_errors.hpp"
#include "featherdoc_cli_json.hpp"
#include "featherdoc_cli_parse.hpp"
#include "featherdoc_cli_style_options_parse.hpp"
#include "featherdoc_cli_style_output.hpp"
#include "featherdoc_cli_style_refactor_options_parse.hpp"
#include "featherdoc_cli_style_refactor_plan.hpp"
#include "featherdoc_cli_style_refactor_plan_parse.hpp"
#include "featherdoc_cli_style_refactor_restore_output.hpp"
#include "featherdoc_cli_style_refactor_rollback_parse.hpp"
#include "featherdoc_cli_text.hpp"

#include <cstddef>
#include <fstream>
#include <iostream>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace featherdoc_cli {
namespace {

void write_json_style_refactor_issue(
    std::ostream &stream, const featherdoc::style_refactor_issue &issue) {
    stream << "{\"code\":";
    write_json_string(stream, issue.code);
    stream << ",\"message\":";
    write_json_string(stream, issue.message);
    stream << '}';
}

void write_json_style_refactor_suggestion(
    std::ostream &stream,
    const featherdoc::style_refactor_suggestion &suggestion) {
    stream << "{\"reason_code\":";
    write_json_string(stream, suggestion.reason_code);
    stream << ",\"reason\":";
    write_json_string(stream, suggestion.reason);
    stream << ",\"confidence\":" << suggestion.confidence
           << ",\"evidence\":";
    write_json_strings(stream, suggestion.evidence);
    stream << ",\"differences\":";
    write_json_strings(stream, suggestion.differences);
    stream << '}';
}

void write_json_style_refactor_suggestion_confidence_summary(
    std::ostream &stream,
    const featherdoc::style_refactor_suggestion_confidence_summary &summary) {
    stream << "{\"suggestion_count\":" << summary.suggestion_count
           << ",\"min_confidence\":";
    write_json_optional_u32(stream, summary.min_confidence);
    stream << ",\"max_confidence\":";
    write_json_optional_u32(stream, summary.max_confidence);
    stream << ",\"exact_xml_match_count\":" << summary.exact_xml_match_count
           << ",\"xml_difference_count\":" << summary.xml_difference_count
           << ",\"recommended_min_confidence\":";
    write_json_optional_u32(stream, summary.recommended_min_confidence);
    stream << ",\"recommendation\":";
    write_json_string(stream, summary.recommendation);
    stream << '}';
}

void write_json_style_refactor_operation(
    std::ostream &stream,
    const featherdoc::style_refactor_operation_plan &operation) {
    stream << "{\"action\":";
    write_json_string(stream, style_refactor_action_name(operation.action));
    stream << ",\"source_style_id\":";
    write_json_string(stream, operation.source_style_id);
    stream << ",\"target_style_id\":";
    write_json_string(stream, operation.target_style_id);
    stream << ",\"applyable\":" << json_bool(operation.applyable)
           << ",\"source_exists\":" << json_bool(operation.source_style.has_value())
           << ",\"target_exists\":" << json_bool(operation.target_style.has_value())
           << ",\"source_kind\":";
    if (operation.source_style.has_value()) {
        write_json_string(stream, style_kind_name(operation.source_style->kind));
    } else {
        stream << "null";
    }
    stream << ",\"target_kind\":";
    if (operation.target_style.has_value()) {
        write_json_string(stream, style_kind_name(operation.target_style->kind));
    } else {
        stream << "null";
    }
    stream << ",\"source_reference_count\":";
    if (operation.source_usage.has_value()) {
        stream << operation.source_usage->total_count();
    } else {
        stream << "null";
    }
    stream << ",\"source_usage\":";
    if (operation.source_usage.has_value()) {
        write_json_style_usage_summary(stream, *operation.source_usage);
    } else {
        stream << "null";
    }
    stream << ",\"suggestion\":";
    if (operation.suggestion.has_value()) {
        write_json_style_refactor_suggestion(stream, *operation.suggestion);
    } else {
        stream << "null";
    }
    stream << ",\"issues\":[";
    for (std::size_t index = 0; index < operation.issues.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        write_json_style_refactor_issue(stream, operation.issues[index]);
    }
    stream << "],\"command_template\":";
    write_json_string(stream, style_refactor_command_template(operation));
    stream << '}';
}

void write_json_style_refactor_plan_fields(
    std::ostream &stream, const featherdoc::style_refactor_plan &plan) {
    const auto confidence_summary = plan.suggestion_confidence_summary();
    stream << "\"clean\":" << json_bool(plan.clean())
           << ",\"operation_count\":" << plan.operation_count
           << ",\"applyable_count\":" << plan.applyable_count
           << ",\"issue_count\":" << plan.issue_count
           << ",\"suggestion_confidence_summary\":";
    write_json_style_refactor_suggestion_confidence_summary(stream,
                                                            confidence_summary);
    stream << ",\"operations\":[";
    for (std::size_t index = 0; index < plan.operations.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        write_json_style_refactor_operation(stream, plan.operations[index]);
    }
    stream << ']';
}

void write_json_style_refactor_rollback_entry(
    std::ostream &stream,
    const featherdoc::style_refactor_rollback_entry &entry) {
    stream << "{\"action\":";
    write_json_string(stream, style_refactor_action_name(entry.action));
    stream << ",\"source_style_id\":";
    write_json_string(stream, entry.source_style_id);
    stream << ",\"target_style_id\":";
    write_json_string(stream, entry.target_style_id);
    stream << ",\"automatic\":" << json_bool(entry.automatic)
           << ",\"restorable\":" << json_bool(entry.restorable)
           << ",\"note\":";
    write_json_string(stream, entry.note);
    stream << ",\"source_reference_count\":";
    if (entry.source_usage.has_value()) {
        stream << entry.source_usage->total_count();
    } else {
        stream << "null";
    }
    stream << ",\"source_usage\":";
    if (entry.source_usage.has_value()) {
        write_json_style_usage_summary(stream, *entry.source_usage);
    } else {
        stream << "null";
    }
    stream << ",\"source_style_xml\":";
    if (!entry.source_style_xml.empty()) {
        write_json_string(stream, entry.source_style_xml);
    } else {
        stream << "null";
    }
    stream << ",\"command_template\":";
    if (entry.automatic) {
        write_json_string(stream, style_refactor_rollback_command_template(entry));
    } else {
        stream << "null";
    }
    stream << '}';
}

void write_json_style_refactor_rollback_entries(
    std::ostream &stream,
    const std::vector<featherdoc::style_refactor_rollback_entry> &entries) {
    stream << '[';
    for (std::size_t index = 0; index < entries.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        write_json_style_refactor_rollback_entry(stream, entries[index]);
    }
    stream << ']';
}

void write_json_style_refactor_apply_result_fields(
    std::ostream &stream,
    const featherdoc::style_refactor_apply_result &result) {
    stream << ",\"changed\":" << json_bool(result.changed)
           << ",\"requested_count\":" << result.requested_count
           << ",\"applied_count\":" << result.applied_count
           << ",\"skipped_count\":" << result.skipped_count()
           << ",\"rollback_count\":" << result.rollback_entries.size()
           << ",\"rollback_operations\":";
    write_json_style_refactor_rollback_entries(stream, result.rollback_entries);
    stream << ",\"plan\":{";
    write_json_style_refactor_plan_fields(stream, result.plan);
    stream << '}';
}

void inspect_style_refactor_plan(
    const featherdoc::style_refactor_plan &plan, bool json_output,
    std::string_view command_name = "plan-style-refactor",
    std::optional<bool> fail_on_suggestion = std::nullopt);

void inspect_style_refactor_apply_result(
    const featherdoc::style_refactor_apply_result &result, bool json_output) {
    if (json_output) {
        std::cout << "{\"command\":\"apply-style-refactor\",\"ok\":"
                  << json_bool(result.applied());
        write_json_style_refactor_apply_result_fields(std::cout, result);
        std::cout << "}\n";
        return;
    }

    std::cout << "applied: " << yes_no(result.applied()) << '\n'
              << "changed: " << yes_no(result.changed) << '\n'
              << "requested_operations: " << result.requested_count << '\n'
              << "applied_operations: " << result.applied_count << '\n'
              << "skipped_operations: " << result.skipped_count() << '\n'
              << "rollback_operations: " << result.rollback_entries.size()
              << '\n';
    for (std::size_t index = 0; index < result.rollback_entries.size();
         ++index) {
        const auto &entry = result.rollback_entries[index];
        std::cout << "rollback[" << index << "]: action="
                  << style_refactor_action_name(entry.action)
                  << " source=" << entry.source_style_id
                  << " target=" << entry.target_style_id
                  << " automatic=" << yes_no(entry.automatic)
                  << " restorable=" << yes_no(entry.restorable)
                  << " references=";
        if (entry.source_usage.has_value()) {
            std::cout << entry.source_usage->total_count();
        } else {
            std::cout << "unknown";
        }
        std::cout << " note=" << entry.note;
        if (entry.automatic) {
            std::cout << " command="
                      << style_refactor_rollback_command_template(entry);
        }
        std::cout << '\n';
    }
    inspect_style_refactor_plan(result.plan, false);
}

void inspect_style_refactor_plan(
    const featherdoc::style_refactor_plan &plan, bool json_output,
    std::string_view command_name,
    std::optional<bool> fail_on_suggestion) {
    const auto suggestion_gate_failed =
        fail_on_suggestion.value_or(false) && plan.operation_count != 0U;
    if (json_output) {
        std::cout << "{\"command\":";
        write_json_string(std::cout, command_name);
        std::cout << ',';
        if (fail_on_suggestion.has_value()) {
            std::cout << "\"fail_on_suggestion\":"
                      << json_bool(*fail_on_suggestion)
                      << ",\"suggestion_gate_failed\":"
                      << json_bool(suggestion_gate_failed) << ',';
        }
        write_json_style_refactor_plan_fields(std::cout, plan);
        std::cout << "}\n";
        return;
    }

    const auto confidence_summary = plan.suggestion_confidence_summary();
    std::cout << "operations: " << plan.operation_count << '\n'
              << "applyable_operations: " << plan.applyable_count << '\n'
              << "issues: " << plan.issue_count << '\n';
    if (fail_on_suggestion.has_value()) {
        std::cout << "fail_on_suggestion: " << yes_no(*fail_on_suggestion)
                  << '\n'
                  << "suggestion_gate_failed: "
                  << yes_no(suggestion_gate_failed) << '\n';
    }
    std::cout << "suggestions: " << confidence_summary.suggestion_count << '\n'
              << "suggestion_exact_xml_matches: "
              << confidence_summary.exact_xml_match_count << '\n'
              << "suggestion_xml_differences: "
              << confidence_summary.xml_difference_count << '\n'
              << "suggestion_min_confidence: ";
    if (confidence_summary.min_confidence.has_value()) {
        std::cout << *confidence_summary.min_confidence;
    } else {
        std::cout << "none";
    }
    std::cout << '\n' << "suggestion_max_confidence: ";
    if (confidence_summary.max_confidence.has_value()) {
        std::cout << *confidence_summary.max_confidence;
    } else {
        std::cout << "none";
    }
    std::cout << '\n' << "suggestion_recommended_min_confidence: ";
    if (confidence_summary.recommended_min_confidence.has_value()) {
        std::cout << *confidence_summary.recommended_min_confidence;
    } else {
        std::cout << "none";
    }
    std::cout << '\n'
              << "suggestion_recommendation: "
              << confidence_summary.recommendation << '\n';
    for (std::size_t index = 0; index < plan.operations.size(); ++index) {
        const auto &operation = plan.operations[index];
        std::cout << "operation[" << index << "]: action="
                  << style_refactor_action_name(operation.action)
                  << " source=" << operation.source_style_id
                  << " target=" << operation.target_style_id
                  << " applyable=" << yes_no(operation.applyable)
                  << " references=";
        if (operation.source_usage.has_value()) {
            std::cout << operation.source_usage->total_count();
        } else {
            std::cout << "unknown";
        }
        std::cout << " issues=" << operation.issues.size();
        if (operation.suggestion.has_value()) {
            std::cout << " suggestion_confidence="
                      << operation.suggestion->confidence
                      << " suggestion_reason="
                      << operation.suggestion->reason_code
                      << " suggestion_differences="
                      << operation.suggestion->differences.size();
        }
        std::cout << " command=" << style_refactor_command_template(operation)
                  << '\n';
        for (std::size_t issue_index = 0;
             issue_index < operation.issues.size(); ++issue_index) {
            std::cout << "operation[" << index << "].issue[" << issue_index
                      << "]: code=" << operation.issues[issue_index].code
                      << " message=" << operation.issues[issue_index].message
                      << '\n';
        }
    }
}

auto write_style_refactor_plan_file(
    const path_type &output_path, const featherdoc::style_refactor_plan &plan,
    std::string &error_message,
    std::string_view command_name = "plan-style-refactor") -> bool {
    std::ofstream stream(output_path, std::ios::binary | std::ios::trunc);
    if (!stream.good()) {
        error_message =
            "failed to open style refactor plan output path: " +
            output_path.string();
        return false;
    }

    stream << "{\"command\":";
    write_json_string(stream, command_name);
    stream << ',';
    write_json_style_refactor_plan_fields(stream, plan);
    stream << "}\n";
    if (!stream.good()) {
        error_message =
            "failed to write style refactor plan output path: " +
            output_path.string();
        return false;
    }

    return true;
}

auto write_style_refactor_rollback_plan_file(
    const path_type &output_path,
    const featherdoc::style_refactor_apply_result &result,
    std::string &error_message) -> bool {
    std::ofstream stream(output_path, std::ios::binary | std::ios::trunc);
    if (!stream.good()) {
        error_message =
            "failed to open style refactor rollback output path: " +
            output_path.string();
        return false;
    }

    stream << "{\"command\":\"apply-style-refactor\""
           << ",\"requested_count\":" << result.requested_count
           << ",\"applied_count\":" << result.applied_count
           << ",\"rollback_count\":" << result.rollback_entries.size()
           << ",\"rollback_operations\":";
    write_json_style_refactor_rollback_entries(stream, result.rollback_entries);
    stream << "}\n";
    if (!stream.good()) {
        error_message =
            "failed to write style refactor rollback output path: " +
            output_path.string();
        return false;
    }

    return true;
}

auto run_plan_style_refactor_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 2U || arguments[1].starts_with("--")) {
        print_parse_error(command, "plan-style-refactor expects an input path",
                          json_output);
        return 2;
    }

    auto options = style_refactor_plan_options{};
    std::string error_message;
    if (!parse_style_refactor_plan_options(arguments, 2U, options,
                                           error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return 1;
    }

    const auto plan = doc.plan_style_refactor(options.requests);
    if (!plan.has_value()) {
        report_document_error(command, "inspect", doc.last_error(),
                              options.json_output);
        return 1;
    }

    if (options.output_plan_path.has_value() &&
        !write_style_refactor_plan_file(*options.output_plan_path, *plan,
                                        error_message)) {
        featherdoc::document_error_info error_info{};
        error_info.code = std::make_error_code(std::errc::io_error);
        error_info.detail = std::move(error_message);
        report_operation_failure(command, "output",
                                 "failed to write style refactor plan output",
                                 error_info, options.json_output);
        return 1;
    }

    inspect_style_refactor_plan(*plan, options.json_output);
    return plan->clean() ? 0 : 1;
}

auto run_suggest_style_merges_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 2U || arguments[1].starts_with("--")) {
        print_parse_error(command, "suggest-style-merges expects an input path",
                          json_output);
        return 2;
    }

    auto options = style_merge_suggestion_options{};
    std::string error_message;
    if (!parse_style_merge_suggestion_options(arguments, 2U, options,
                                              error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return 1;
    }

    auto plan = doc.suggest_style_merges();
    if (!plan.has_value()) {
        report_document_error(command, "inspect", doc.last_error(),
                              options.json_output);
        return 1;
    }
    plan = filter_style_refactor_plan_by_style_ids(
        std::move(*plan), options.source_style_ids, options.target_style_ids);

    auto min_confidence = options.min_confidence;
    if (!min_confidence.has_value() && options.confidence_profile.has_value()) {
        if (*options.confidence_profile == "recommended") {
            min_confidence =
                plan->suggestion_confidence_summary().recommended_min_confidence;
        } else {
            min_confidence =
                style_merge_suggestion_confidence_profile_min_confidence(
                    *options.confidence_profile);
        }
    }
    if (min_confidence.has_value()) {
        plan = filter_style_refactor_plan_by_min_confidence(std::move(*plan),
                                                            *min_confidence);
    }

    if (options.output_plan_path.has_value() &&
        !write_style_refactor_plan_file(*options.output_plan_path, *plan,
                                        error_message, command)) {
        featherdoc::document_error_info error_info{};
        error_info.code = std::make_error_code(std::errc::io_error);
        error_info.detail = std::move(error_message);
        report_operation_failure(command, "output",
                                 "failed to write style merge suggestions",
                                 error_info, options.json_output);
        return 1;
    }

    inspect_style_refactor_plan(*plan, options.json_output, command,
                                options.fail_on_suggestion);
    const auto suggestion_gate_failed =
        options.fail_on_suggestion && plan->operation_count != 0U;
    return plan->clean() && !suggestion_gate_failed ? 0 : 1;
}

auto run_apply_style_refactor_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 2U || arguments[1].starts_with("--")) {
        print_parse_error(command, "apply-style-refactor expects an input path",
                          json_output);
        return 2;
    }

    auto options = style_refactor_apply_options{};
    std::string error_message;
    if (!parse_style_refactor_apply_options(arguments, 2U, options,
                                            error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    if (options.plan_file_path.has_value() &&
        !read_style_refactor_plan_file(*options.plan_file_path,
                                       options.requests, error_message)) {
        print_parse_error(command, error_message, options.json_output);
        return 2;
    }

    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return 1;
    }

    const auto result = doc.apply_style_refactor(options.requests);
    if (!result.has_value()) {
        report_document_error(command, "mutate", doc.last_error(),
                              options.json_output);
        return 1;
    }

    if (!result->applied()) {
        inspect_style_refactor_apply_result(*result, options.json_output);
        return 1;
    }

    if (!save_document(doc, options.output_path, command, options.json_output)) {
        return 1;
    }

    if (options.rollback_plan_path.has_value() &&
        !write_style_refactor_rollback_plan_file(*options.rollback_plan_path,
                                                 *result, error_message)) {
        featherdoc::document_error_info error_info{};
        error_info.code = std::make_error_code(std::errc::io_error);
        error_info.detail = std::move(error_message);
        report_operation_failure(
            command, "output",
            "failed to write style refactor rollback output", error_info,
            options.json_output);
        return 1;
    }

    if (options.json_output) {
        write_json_mutation_result(
            command, doc, options.output_path,
            [&result, &options](std::ostream &stream) {
                write_json_style_refactor_apply_result_fields(stream, *result);
                if (options.plan_file_path.has_value()) {
                    stream << ",\"plan_file\":";
                    write_json_string(stream,
                                      options.plan_file_path->string());
                }
                if (options.rollback_plan_path.has_value()) {
                    stream << ",\"rollback_plan_file\":";
                    write_json_string(stream,
                                      options.rollback_plan_path->string());
                }
            });
        return 0;
    }

    inspect_style_refactor_apply_result(*result, false);
    return 0;
}

auto run_restore_style_merge_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 2U || arguments[1].starts_with("--")) {
        print_parse_error(command, "restore-style-merge expects an input path",
                          json_output);
        return 2;
    }

    auto options = style_merge_restore_options{};
    std::string error_message;
    if (!parse_style_merge_restore_options(arguments, 2U, options,
                                           error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    auto rollback_entries =
        std::vector<featherdoc::style_refactor_rollback_entry>{};
    if (!read_style_refactor_rollback_file(*options.rollback_plan_path,
                                           options.entry_indexes,
                                           options.source_style_ids,
                                           options.target_style_ids,
                                           rollback_entries, error_message)) {
        print_parse_error(command, error_message, options.json_output);
        return 2;
    }

    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return 1;
    }

    const auto result = options.dry_run
        ? doc.plan_style_refactor_restore(rollback_entries)
        : doc.restore_style_refactor(rollback_entries);
    if (!result.has_value()) {
        report_document_error(command, "mutate", doc.last_error(),
                              options.json_output);
        return 1;
    }

    if (!result->restored()) {
        inspect_style_refactor_restore_result(*result, options.json_output,
                                              &options);
        return 1;
    }

    if (options.dry_run) {
        inspect_style_refactor_restore_result(*result, options.json_output,
                                              &options);
        return 0;
    }

    if (!save_document(doc, options.output_path, command, options.json_output)) {
        return 1;
    }

    if (options.json_output) {
        write_json_mutation_result(
            command, doc, options.output_path,
            [&result, &options](std::ostream &stream) {
                write_json_style_refactor_restore_result_fields(stream, *result);
                stream << ",\"rollback_plan_file\":";
                write_json_string(stream, options.rollback_plan_path->string());
                write_json_style_refactor_restore_selection(stream, options);
            });
        return 0;
    }

    inspect_style_refactor_restore_result(*result, false);
    return 0;
}

auto run_rename_style_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 4U) {
        print_parse_error(
            command,
            "rename-style expects an input path, an old style id, and a new style id",
            json_output);
        return 2;
    }

    const auto old_style_id = std::string(arguments[2]);
    const auto new_style_id = std::string(arguments[3]);
    rename_style_options options;
    std::string error_message;
    if (!parse_rename_style_options(arguments, 4U, options, error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return 1;
    }

    if (!doc.rename_style(old_style_id, new_style_id)) {
        report_document_error(command, "mutate", doc.last_error(),
                              options.json_output);
        return 1;
    }

    const auto style = doc.find_style(new_style_id);
    if (!style.has_value()) {
        report_document_error(command, "inspect", doc.last_error(),
                              options.json_output);
        return 1;
    }

    if (!save_document(doc, options.output_path, command, options.json_output)) {
        return 1;
    }

    if (options.json_output) {
        write_json_mutation_result(
            command, doc, options.output_path,
            [&old_style_id, &new_style_id, &style](std::ostream &stream) {
                stream << ",\"old_style_id\":";
                write_json_string(stream, old_style_id);
                stream << ",\"new_style_id\":";
                write_json_string(stream, new_style_id);
                stream << ",\"style\":";
                write_json_style_summary(stream, *style);
            });
        return 0;
    }

    inspect_style(*style, std::nullopt, false);
    return 0;
}

auto run_merge_style_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 4U) {
        print_parse_error(
            command,
            "merge-style expects an input path, a source style id, and a target style id",
            json_output);
        return 2;
    }

    const auto source_style_id = std::string(arguments[2]);
    const auto target_style_id = std::string(arguments[3]);
    merge_style_options options;
    std::string error_message;
    if (!parse_rename_style_options(arguments, 4U, options, error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return 1;
    }

    if (!doc.merge_style(source_style_id, target_style_id)) {
        report_document_error(command, "mutate", doc.last_error(),
                              options.json_output);
        return 1;
    }

    const auto style = doc.find_style(target_style_id);
    if (!style.has_value()) {
        report_document_error(command, "inspect", doc.last_error(),
                              options.json_output);
        return 1;
    }

    if (!save_document(doc, options.output_path, command, options.json_output)) {
        return 1;
    }

    if (options.json_output) {
        write_json_mutation_result(
            command, doc, options.output_path,
            [&source_style_id, &target_style_id, &style](
                std::ostream &stream) {
                stream << ",\"source_style_id\":";
                write_json_string(stream, source_style_id);
                stream << ",\"target_style_id\":";
                write_json_string(stream, target_style_id);
                stream << ",\"style\":";
                write_json_style_summary(stream, *style);
            });
        return 0;
    }

    inspect_style(*style, std::nullopt, false);
    return 0;
}

auto run_plan_prune_unused_styles_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 2U || arguments[1].starts_with("--")) {
        print_parse_error(command, "plan-prune-unused-styles expects an input path",
                          json_output);
        return 2;
    }

    for (std::size_t index = 2U; index < arguments.size(); ++index) {
        if (arguments[index] == "--json") {
            continue;
        }
        print_parse_error(command,
                          "unknown option: " + std::string(arguments[index]),
                          json_output);
        return 2;
    }

    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       json_output)) {
        return 1;
    }

    const auto plan = doc.plan_prune_unused_styles();
    if (!plan.has_value()) {
        report_document_error(command, "inspect", doc.last_error(), json_output);
        return 1;
    }

    if (json_output) {
        std::cout << "{\"command\":\"plan-prune-unused-styles\",\"ok\":true";
        write_json_style_prune_plan(std::cout, *plan);
        std::cout << "}\n";
        return 0;
    }

    print_style_prune_plan(*plan);
    return 0;
}

auto run_prune_unused_styles_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 2U || arguments[1].starts_with("--")) {
        print_parse_error(command, "prune-unused-styles expects an input path",
                          json_output);
        return 2;
    }

    prune_unused_styles_options options;
    std::string error_message;
    if (!parse_rename_style_options(arguments, 2U, options, error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return 1;
    }

    const auto summary = doc.prune_unused_styles();
    if (!summary.has_value()) {
        report_document_error(command, "mutate", doc.last_error(),
                              options.json_output);
        return 1;
    }

    if (!save_document(doc, options.output_path, command, options.json_output)) {
        return 1;
    }

    if (options.json_output) {
        write_json_mutation_result(
            command, doc, options.output_path,
            [&summary](std::ostream &stream) {
                write_json_style_prune_summary(stream, *summary);
            });
        return 0;
    }

    print_style_prune_summary(*summary);
    return 0;
}

} // namespace

auto is_style_refactor_command(std::string_view command) -> bool {
    return command == "plan-style-refactor" ||
           command == "suggest-style-merges" ||
           command == "apply-style-refactor" ||
           command == "restore-style-merge" || command == "rename-style" ||
           command == "merge-style" ||
           command == "plan-prune-unused-styles" ||
           command == "prune-unused-styles";
}

auto run_style_refactor_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    if (command == "plan-style-refactor") {
        return run_plan_style_refactor_command(command, arguments, doc);
    }
    if (command == "suggest-style-merges") {
        return run_suggest_style_merges_command(command, arguments, doc);
    }
    if (command == "apply-style-refactor") {
        return run_apply_style_refactor_command(command, arguments, doc);
    }
    if (command == "restore-style-merge") {
        return run_restore_style_merge_command(command, arguments, doc);
    }
    if (command == "rename-style") {
        return run_rename_style_command(command, arguments, doc);
    }
    if (command == "merge-style") {
        return run_merge_style_command(command, arguments, doc);
    }
    if (command == "plan-prune-unused-styles") {
        return run_plan_prune_unused_styles_command(command, arguments, doc);
    }
    if (command == "prune-unused-styles") {
        return run_prune_unused_styles_command(command, arguments, doc);
    }

    return 2;
}

} // namespace featherdoc_cli
