#include "featherdoc_cli_style_refactor_output.hpp"

#include "featherdoc_cli_domain_names.hpp"
#include "featherdoc_cli_json.hpp"
#include "featherdoc_cli_style_output.hpp"
#include "featherdoc_cli_style_refactor_plan.hpp"
#include "featherdoc_cli_text.hpp"

#include <cstddef>
#include <fstream>
#include <iostream>
#include <optional>
#include <ostream>
#include <string_view>
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

} // namespace

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
    std::string_view command_name,
    std::optional<bool> fail_on_suggestion);

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
    std::string_view command_name) -> bool {
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

} // namespace featherdoc_cli
