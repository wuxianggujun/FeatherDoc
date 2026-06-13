#include "featherdoc_cli_table_style_audit_output_json.hpp"

#include "featherdoc_cli_json.hpp"

#include <cstddef>
#include <optional>
#include <ostream>
#include <string_view>

namespace featherdoc_cli {
namespace {

void write_json_table_style_region_audit_issue(
    std::ostream &stream,
    const featherdoc::table_style_region_audit_issue &issue) {
    stream << "{\"style_id\":";
    write_json_string(stream, issue.style_id);
    stream << ",\"style_name\":";
    write_json_string(stream, issue.style_name);
    stream << ",\"region\":";
    write_json_string(stream, issue.region);
    stream << ",\"issue_type\":";
    write_json_string(stream, issue.issue_type);
    stream << ",\"property_count\":" << issue.property_count
           << ",\"suggestion\":";
    write_json_string(stream, issue.suggestion);
    stream << '}';
}

void write_json_table_style_inheritance_audit_issue(
    std::ostream &stream,
    const featherdoc::table_style_inheritance_audit_issue &issue) {
    stream << "{\"style_id\":";
    write_json_string(stream, issue.style_id);
    stream << ",\"style_name\":";
    write_json_string(stream, issue.style_name);
    stream << ",\"based_on_style_id\":";
    write_json_string(stream, issue.based_on_style_id);
    stream << ",\"based_on_style_kind\":";
    write_json_string(stream, issue.based_on_style_kind);
    stream << ",\"issue_type\":";
    write_json_string(stream, issue.issue_type);
    stream << ",\"inheritance_chain\":";
    write_json_strings(stream, issue.inheritance_chain);
    stream << ",\"suggestion\":";
    write_json_string(stream, issue.suggestion);
    stream << '}';
}

void write_json_table_style_look_issue(
    std::ostream &stream,
    const featherdoc::table_style_look_consistency_issue &issue) {
    stream << "{\"table_index\":" << issue.table_index << ",\"style_id\":";
    write_json_string(stream, issue.style_id);
    stream << ",\"issue_type\":";
    write_json_string(stream, issue.issue_type);
    stream << ",\"region\":";
    write_json_string(stream, issue.region);
    stream << ",\"required_style_look_flag\":";
    write_json_string(stream, issue.required_style_look_flag);
    stream << ",\"expected_value\":" << json_bool(issue.expected_value)
           << ",\"actual_value\":";
    write_json_optional_bool_value(stream, issue.actual_value);
    stream << ",\"suggestion\":";
    write_json_string(stream, issue.suggestion);
    stream << '}';
}

void write_json_table_style_quality_fix_item(
    std::ostream &stream,
    const featherdoc::table_style_quality_fix_item &item) {
    stream << "{\"source\":";
    write_json_string(stream, item.source);
    stream << ",\"issue_type\":";
    write_json_string(stream, item.issue_type);
    stream << ",\"table_index\":";
    write_json_optional_size(stream, item.table_index);
    stream << ",\"style_id\":";
    write_json_string(stream, item.style_id);
    stream << ",\"style_name\":";
    write_json_string(stream, item.style_name);
    stream << ",\"region\":";
    write_json_string(stream, item.region);
    stream << ",\"action\":";
    write_json_string(stream, item.action);
    stream << ",\"automatic\":" << json_bool(item.automatic)
           << ",\"recommended_command\":";
    write_json_string(stream, item.command);
    stream << ",\"suggestion\":";
    write_json_string(stream, item.suggestion);
    stream << '}';
}

void write_json_table_style_look_issue_array(
    std::ostream &stream,
    const featherdoc::table_style_look_consistency_report &report) {
    stream << '[';
    for (std::size_t index = 0U; index < report.issues.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        write_json_table_style_look_issue(stream, report.issues[index]);
    }
    stream << ']';
}

} // namespace

void write_json_table_style_region_audit_report(
    std::ostream &stream,
    const featherdoc::table_style_region_audit_report &report,
    const std::optional<std::string> &style_id, bool fail_on_issue) {
    stream << "{\"command\":\"audit-table-style-regions\",\"ok\":"
           << json_bool(report.ok()) << ",\"table_style_count\":"
           << report.table_style_count << ",\"region_count\":"
           << report.region_count << ",\"issue_count\":"
           << report.issue_count() << ",\"fail_on_issue\":"
           << json_bool(fail_on_issue) << ",\"style_id\":";
    if (style_id.has_value()) {
        write_json_string(stream, *style_id);
    } else {
        stream << "null";
    }
    stream << ",\"issues\":[";
    for (std::size_t index = 0U; index < report.issues.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        write_json_table_style_region_audit_issue(stream,
                                                  report.issues[index]);
    }
    stream << "]}";
}

void write_json_table_style_inheritance_audit_report(
    std::ostream &stream,
    const featherdoc::table_style_inheritance_audit_report &report,
    const std::optional<std::string> &style_id, bool fail_on_issue) {
    stream << "{\"command\":\"audit-table-style-inheritance\",\"ok\":"
           << json_bool(report.ok()) << ",\"table_style_count\":"
           << report.table_style_count << ",\"issue_count\":"
           << report.issue_count() << ",\"fail_on_issue\":"
           << json_bool(fail_on_issue) << ",\"style_id\":";
    if (style_id.has_value()) {
        write_json_string(stream, *style_id);
    } else {
        stream << "null";
    }
    stream << ",\"issues\":[";
    for (std::size_t index = 0U; index < report.issues.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        write_json_table_style_inheritance_audit_issue(stream,
                                                       report.issues[index]);
    }
    stream << "]}";
}

void write_json_table_style_look_report(
    std::ostream &stream,
    const featherdoc::table_style_look_consistency_report &report,
    bool fail_on_issue) {
    const auto issue_count = report.issue_count();
    stream << "{\"command\":\"check-table-style-look\",\"ok\":"
           << json_bool(report.ok()) << ",\"table_count\":"
           << report.table_count << ",\"issue_count\":" << issue_count
           << ",\"fail_on_issue\":" << json_bool(fail_on_issue)
           << ",\"issues\":";
    write_json_table_style_look_issue_array(stream, report);
    stream << '}';
}

void write_json_table_style_quality_audit_report(
    std::ostream &stream,
    const featherdoc::table_style_quality_audit_report &report,
    bool fail_on_issue) {
    stream << "{\"command\":\"audit-table-style-quality\",\"ok\":"
           << json_bool(report.ok()) << ",\"issue_count\":"
           << report.issue_count() << ",\"region_issue_count\":"
           << report.region_audit.issue_count()
           << ",\"inheritance_issue_count\":"
           << report.inheritance_audit.issue_count()
           << ",\"style_look_issue_count\":"
           << report.style_look.issue_count() << ",\"fail_on_issue\":"
           << json_bool(fail_on_issue) << ",\"region_audit\":";
    write_json_table_style_region_audit_report(stream, report.region_audit,
                                               std::nullopt, fail_on_issue);
    stream << ",\"inheritance_audit\":";
    write_json_table_style_inheritance_audit_report(
        stream, report.inheritance_audit, std::nullopt, fail_on_issue);
    stream << ",\"style_look\":";
    write_json_table_style_look_report(stream, report.style_look,
                                       fail_on_issue);
    stream << '}';
}

void write_json_table_style_quality_fix_plan(
    std::ostream &stream,
    const featherdoc::table_style_quality_fix_plan &plan,
    bool fail_on_issue) {
    stream << "{\"command\":\"plan-table-style-quality-fixes\",\"ok\":"
           << json_bool(plan.ok()) << ",\"issue_count\":"
           << plan.issue_count() << ",\"plan_item_count\":"
           << plan.items.size() << ",\"automatic_fix_count\":"
           << plan.automatic_fix_count() << ",\"manual_fix_count\":"
           << plan.manual_fix_count() << ",\"fail_on_issue\":"
           << json_bool(fail_on_issue) << ",\"items\":[";
    for (std::size_t index = 0U; index < plan.items.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        write_json_table_style_quality_fix_item(stream, plan.items[index]);
    }
    stream << "],\"audit\":";
    write_json_table_style_quality_audit_report(stream, plan.audit,
                                                fail_on_issue);
    stream << '}';
}

void write_json_apply_table_style_quality_fixes_result(
    std::ostream &stream,
    const table_style_quality_apply_cli_result &result) {
    stream << "{\"command\":\"apply-table-style-quality-fixes\","
           << "\"mode\":\"look_only\",\"ok\":" << json_bool(result.after.ok())
           << ",\"before_issue_count\":" << result.before.issue_count()
           << ",\"after_issue_count\":" << result.after.issue_count()
           << ",\"before_automatic_fix_count\":"
           << result.before.automatic_fix_count()
           << ",\"after_automatic_fix_count\":"
           << result.after.automatic_fix_count()
           << ",\"before_manual_fix_count\":"
           << result.before.manual_fix_count()
           << ",\"after_manual_fix_count\":"
           << result.after.manual_fix_count()
           << ",\"changed_table_count\":" << result.changed_table_count
           << ",\"output\":";
    if (result.output_path.has_value()) {
        write_json_string(stream, result.output_path->string());
    } else {
        stream << "null";
    }
    stream << ",\"before_plan\":";
    write_json_table_style_quality_fix_plan(stream, result.before, false);
    stream << ",\"after_plan\":";
    write_json_table_style_quality_fix_plan(stream, result.after, false);
    stream << '}';
}

void write_json_repair_table_style_look_result(
    std::ostream &stream, const table_style_look_repair_cli_result &result) {
    const auto mode =
        result.apply ? std::string_view{"apply"} : std::string_view{"plan"};
    const auto after_ok =
        result.after.has_value() ? result.after->ok() : result.before.ok();
    stream << "{\"command\":\"repair-table-style-look\",\"mode\":";
    write_json_string(stream, mode);
    stream << ",\"ok\":" << json_bool(after_ok)
           << ",\"before_issue_count\":" << result.before.issue_count()
           << ",\"after_issue_count\":";
    if (result.after.has_value()) {
        stream << result.after->issue_count();
    } else {
        stream << "null";
    }
    stream << ",\"applicable_issue_count\":" << result.applicable_issue_count
           << ",\"changed_table_count\":" << result.changed_table_count
           << ",\"output\":";
    if (result.output_path.has_value()) {
        write_json_string(stream, result.output_path->string());
    } else {
        stream << "null";
    }
    stream << ",\"before_issues\":";
    write_json_table_style_look_issue_array(stream, result.before);
    stream << ",\"after_issues\":";
    if (result.after.has_value()) {
        write_json_table_style_look_issue_array(stream, *result.after);
    } else {
        stream << "null";
    }
    stream << '}';
}

} // namespace featherdoc_cli
