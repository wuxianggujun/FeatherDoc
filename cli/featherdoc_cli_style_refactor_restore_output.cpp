#include "featherdoc_cli_style_refactor_restore_output.hpp"

#include "featherdoc_cli_domain_names.hpp"
#include "featherdoc_cli_json.hpp"
#include "featherdoc_cli_style_refactor_plan.hpp"
#include "featherdoc_cli_text.hpp"

#include <cstddef>
#include <iostream>
#include <ostream>
#include <vector>

namespace featherdoc_cli {
namespace {

void write_json_style_refactor_restore_issue(
    std::ostream &stream,
    const featherdoc::style_refactor_restore_issue &issue) {
    stream << "{\"code\":";
    write_json_string(stream, issue.code);
    stream << ",\"message\":";
    write_json_string(stream, issue.message);
    stream << ",\"suggestion\":";
    write_json_string(stream, issue.suggestion);
    stream << '}';
}

void write_json_style_refactor_restore_issue_summary(
    std::ostream &stream,
    const std::vector<featherdoc::style_refactor_restore_issue_summary>
        &summaries) {
    stream << '[';
    for (std::size_t index = 0; index < summaries.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        stream << "{\"code\":";
        write_json_string(stream, summaries[index].code);
        stream << ",\"count\":" << summaries[index].count
               << ",\"suggestion\":";
        write_json_string(stream, summaries[index].suggestion);
        stream << '}';
    }
    stream << ']';
}

void write_json_style_refactor_restore_operation(
    std::ostream &stream,
    const featherdoc::style_refactor_restore_operation_result &operation) {
    stream << "{\"action\":";
    write_json_string(stream, style_refactor_action_name(operation.action));
    stream << ",\"source_style_id\":";
    write_json_string(stream, operation.source_style_id);
    stream << ",\"target_style_id\":";
    write_json_string(stream, operation.target_style_id);
    stream << ",\"restorable\":" << json_bool(operation.restorable)
           << ",\"restored\":" << json_bool(operation.restored)
           << ",\"style_restored\":" << json_bool(operation.style_restored)
           << ",\"restored_reference_count\":"
           << operation.restored_reference_count << ",\"issues\":[";
    for (std::size_t index = 0; index < operation.issues.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        write_json_style_refactor_restore_issue(stream, operation.issues[index]);
    }
    stream << "]}";
}

} // namespace

void write_json_style_refactor_restore_selection(
    std::ostream &stream, const style_merge_restore_options &options) {
    if (!options.entry_indexes.empty()) {
        if (options.entry_indexes.size() == 1U) {
            stream << ",\"entry_index\":" << options.entry_indexes.front();
        }
        stream << ",\"entry_indexes\":[";
        for (std::size_t index = 0; index < options.entry_indexes.size(); ++index) {
            if (index != 0U) {
                stream << ',';
            }
            stream << options.entry_indexes[index];
        }
        stream << ']';
    }
    if (!options.source_style_ids.empty()) {
        stream << ",\"source_style_ids\":";
        write_json_strings(stream, options.source_style_ids);
    }
    if (!options.target_style_ids.empty()) {
        stream << ",\"target_style_ids\":";
        write_json_strings(stream, options.target_style_ids);
    }
}

void write_json_style_refactor_restore_result_fields(
    std::ostream &stream,
    const featherdoc::style_refactor_restore_result &result) {
    const auto issue_summary = result.issue_summary();
    stream << ",\"changed\":" << json_bool(result.changed)
           << ",\"dry_run\":" << json_bool(result.dry_run)
           << ",\"requested_count\":" << result.requested_count
           << ",\"restored_count\":" << result.restored_count
           << ",\"skipped_count\":" << result.skipped_count()
           << ",\"issue_count\":" << result.issue_count()
           << ",\"issue_summary\":";
    write_json_style_refactor_restore_issue_summary(stream, issue_summary);
    stream << ",\"restored_style_count\":" << result.restored_style_count
           << ",\"restored_reference_count\":"
           << result.restored_reference_count << ",\"operations\":[";
    for (std::size_t index = 0; index < result.operations.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        write_json_style_refactor_restore_operation(stream,
                                                    result.operations[index]);
    }
    stream << ']';
}

void inspect_style_refactor_restore_result(
    const featherdoc::style_refactor_restore_result &result, bool json_output,
    const style_merge_restore_options *options) {
    if (json_output) {
        std::cout << "{\"command\":\"restore-style-merge\",\"ok\":"
                  << json_bool(result.restored());
        write_json_style_refactor_restore_result_fields(std::cout, result);
        if (options != nullptr) {
            if (options->rollback_plan_path.has_value()) {
                std::cout << ",\"rollback_plan_file\":";
                write_json_string(std::cout,
                                  options->rollback_plan_path->string());
            }
            write_json_style_refactor_restore_selection(std::cout, *options);
        }
        std::cout << "}\n";
        return;
    }

    const auto issue_summary = result.issue_summary();
    std::cout << "restored: " << yes_no(result.restored()) << '\n'
              << "changed: " << yes_no(result.changed) << '\n'
              << "dry_run: " << yes_no(result.dry_run) << '\n'
              << "requested_operations: " << result.requested_count << '\n'
              << "restored_operations: " << result.restored_count << '\n'
              << "skipped_operations: " << result.skipped_count() << '\n'
              << "issues: " << result.issue_count() << '\n'
              << "issue_summary_entries: " << issue_summary.size() << '\n'
              << "restored_styles: " << result.restored_style_count << '\n'
              << "restored_references: "
              << result.restored_reference_count << '\n';
    for (std::size_t index = 0; index < issue_summary.size(); ++index) {
        std::cout << "issue_summary[" << index << "]: code="
                  << issue_summary[index].code
                  << " count=" << issue_summary[index].count
                  << " suggestion=" << issue_summary[index].suggestion << '\n';
    }
    for (std::size_t index = 0; index < result.operations.size(); ++index) {
        const auto &operation = result.operations[index];
        std::cout << "restore[" << index << "]: action="
                  << style_refactor_action_name(operation.action)
                  << " source=" << operation.source_style_id
                  << " target=" << operation.target_style_id
                  << " restorable=" << yes_no(operation.restorable)
                  << " restored=" << yes_no(operation.restored)
                  << " style_restored=" << yes_no(operation.style_restored)
                  << " references=" << operation.restored_reference_count
                  << " issues=" << operation.issues.size() << '\n';
        for (std::size_t issue_index = 0;
             issue_index < operation.issues.size(); ++issue_index) {
            const auto &issue = operation.issues[issue_index];
            std::cout << "restore[" << index << "].issue[" << issue_index
                      << "]: code=" << issue.code
                      << " message=" << issue.message << '\n';
            if (!issue.suggestion.empty()) {
                std::cout << "restore[" << index << "].issue[" << issue_index
                          << "].suggestion: " << issue.suggestion << '\n';
            }
        }
    }
}

} // namespace featherdoc_cli
