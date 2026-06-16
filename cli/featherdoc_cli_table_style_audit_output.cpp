#include "featherdoc_cli_table_style_output.hpp"

#include "featherdoc_cli_json.hpp"
#include "featherdoc_cli_table_style_audit_output_json.hpp"

#include <cstddef>
#include <iostream>
#include <optional>
#include <ostream>
#include <string_view>

namespace featherdoc_cli {

auto count_applicable_table_style_look_issues(
    const featherdoc::table_style_look_consistency_report &report)
    -> std::size_t {
    auto count = std::size_t{0U};
    for (const auto &issue : report.issues) {
        if ((issue.issue_type == "style_look_missing" ||
             issue.issue_type == "style_look_disabled") &&
            !issue.required_style_look_flag.empty()) {
            ++count;
        }
    }
    return count;
}

void audit_table_style_regions(
    const featherdoc::table_style_region_audit_report &report,
    const std::optional<std::string> &style_id, bool fail_on_issue,
    bool json_output) {
    if (json_output) {
        write_json_table_style_region_audit_report(std::cout, report, style_id,
                                                   fail_on_issue);
        std::cout << '\n';
        return;
    }

    std::cout << "table_style_region_audit: "
              << (report.ok() ? "ok" : "issues") << '\n'
              << "table_style_count: " << report.table_style_count << '\n'
              << "region_count: " << report.region_count << '\n'
              << "issue_count: " << report.issue_count() << '\n';
    if (style_id.has_value()) {
        std::cout << "style_id: " << *style_id << '\n';
    }
    for (std::size_t index = 0U; index < report.issues.size(); ++index) {
        const auto &issue = report.issues[index];
        std::cout << "issue[" << index << "]: style_id=" << issue.style_id
                  << " region=" << issue.region
                  << " issue_type=" << issue.issue_type
                  << " property_count=" << issue.property_count
                  << " suggestion=" << issue.suggestion << '\n';
    }
}

void audit_table_style_inheritance(
    const featherdoc::table_style_inheritance_audit_report &report,
    const std::optional<std::string> &style_id, bool fail_on_issue,
    bool json_output) {
    if (json_output) {
        write_json_table_style_inheritance_audit_report(
            std::cout, report, style_id, fail_on_issue);
        std::cout << '\n';
        return;
    }

    std::cout << "table_style_inheritance_audit: "
              << (report.ok() ? "ok" : "issues") << '\n'
              << "table_style_count: " << report.table_style_count << '\n'
              << "issue_count: " << report.issue_count() << '\n';
    if (style_id.has_value()) {
        std::cout << "style_id: " << *style_id << '\n';
    }
    for (std::size_t index = 0U; index < report.issues.size(); ++index) {
        const auto &issue = report.issues[index];
        std::cout << "issue[" << index << "]: style_id=" << issue.style_id
                  << " based_on_style_id=" << issue.based_on_style_id
                  << " based_on_style_kind=" << issue.based_on_style_kind
                  << " issue_type=" << issue.issue_type << " chain=";
        for (std::size_t chain_index = 0U;
             chain_index < issue.inheritance_chain.size(); ++chain_index) {
            if (chain_index != 0U) {
                std::cout << " -> ";
            }
            std::cout << issue.inheritance_chain[chain_index];
        }
        std::cout << " suggestion=" << issue.suggestion << '\n';
    }
}

void print_table_style_look_report(
    std::ostream &stream,
    const featherdoc::table_style_look_consistency_report &report) {
    stream << "table_style_look_consistency: "
           << (report.ok() ? "ok" : "issues") << '\n'
           << "table_count: " << report.table_count << '\n'
           << "issue_count: " << report.issue_count() << '\n';
    for (std::size_t index = 0U; index < report.issues.size(); ++index) {
        const auto &issue = report.issues[index];
        stream << "issue[" << index << "]: table_index=" << issue.table_index
               << " style_id=" << issue.style_id
               << " issue_type=" << issue.issue_type;
        if (!issue.region.empty()) {
            stream << " region=" << issue.region;
        }
        if (!issue.required_style_look_flag.empty()) {
            stream << " required_style_look_flag="
                   << issue.required_style_look_flag
                   << " expected=" << json_bool(issue.expected_value)
                   << " actual=";
            if (issue.actual_value.has_value()) {
                stream << json_bool(*issue.actual_value);
            } else {
                stream << "none";
            }
        }
        stream << " suggestion=" << issue.suggestion << '\n';
    }
}

void check_table_style_look(
    const featherdoc::table_style_look_consistency_report &report,
    bool fail_on_issue, bool json_output) {
    if (json_output) {
        write_json_table_style_look_report(std::cout, report, fail_on_issue);
        std::cout << '\n';
        return;
    }

    print_table_style_look_report(std::cout, report);
}

void audit_table_style_quality(
    const featherdoc::table_style_quality_audit_report &report,
    bool fail_on_issue, bool json_output) {
    if (json_output) {
        write_json_table_style_quality_audit_report(std::cout, report,
                                                    fail_on_issue);
        std::cout << '\n';
        return;
    }

    std::cout << "table_style_quality_audit: "
              << (report.ok() ? "ok" : "issues") << '\n'
              << "issue_count: " << report.issue_count() << '\n'
              << "region_issue_count: " << report.region_audit.issue_count()
              << '\n'
              << "inheritance_issue_count: "
              << report.inheritance_audit.issue_count() << '\n'
              << "style_look_issue_count: " << report.style_look.issue_count()
              << '\n';
    audit_table_style_regions(report.region_audit, std::nullopt, fail_on_issue,
                              false);
    audit_table_style_inheritance(report.inheritance_audit, std::nullopt,
                                  fail_on_issue, false);
    print_table_style_look_report(std::cout, report.style_look);
}

void plan_table_style_quality_fixes(
    const featherdoc::table_style_quality_fix_plan &plan,
    bool fail_on_issue, bool json_output) {
    if (json_output) {
        write_json_table_style_quality_fix_plan(std::cout, plan, fail_on_issue);
        std::cout << '\n';
        return;
    }

    std::cout << "table_style_quality_fix_plan: "
              << (plan.ok() ? "ok" : "issues") << '\n'
              << "issue_count: " << plan.issue_count() << '\n'
              << "plan_item_count: " << plan.items.size() << '\n'
              << "automatic_fix_count: " << plan.automatic_fix_count() << '\n'
              << "manual_fix_count: " << plan.manual_fix_count() << '\n';
    for (std::size_t index = 0U; index < plan.items.size(); ++index) {
        const auto &item = plan.items[index];
        std::cout << "item[" << index << "]: source=" << item.source
                  << " issue_type=" << item.issue_type
                  << " automatic=" << json_bool(item.automatic)
                  << " action=" << item.action;
        if (item.table_index.has_value()) {
            std::cout << " table_index=" << *item.table_index;
        }
        if (!item.style_id.empty()) {
            std::cout << " style_id=" << item.style_id;
        }
        if (!item.region.empty()) {
            std::cout << " region=" << item.region;
        }
        if (!item.command.empty()) {
            std::cout << " recommended_command=" << item.command;
        }
        std::cout << " suggestion=" << item.suggestion << '\n';
    }
}

void apply_table_style_quality_fixes(
    const table_style_quality_apply_cli_result &result, bool json_output) {
    if (json_output) {
        write_json_apply_table_style_quality_fixes_result(std::cout, result);
        std::cout << '\n';
        return;
    }

    std::cout << "table_style_quality_apply: look_only" << '\n'
              << "before_issue_count: " << result.before.issue_count() << '\n'
              << "after_issue_count: " << result.after.issue_count() << '\n'
              << "before_automatic_fix_count: "
              << result.before.automatic_fix_count() << '\n'
              << "after_automatic_fix_count: "
              << result.after.automatic_fix_count() << '\n'
              << "before_manual_fix_count: "
              << result.before.manual_fix_count() << '\n'
              << "after_manual_fix_count: " << result.after.manual_fix_count()
              << '\n'
              << "changed_table_count: " << result.changed_table_count << '\n';
    if (result.output_path.has_value()) {
        std::cout << "output: " << result.output_path->string() << '\n';
    }
}

void repair_table_style_look(
    const table_style_look_repair_cli_result &result, bool json_output) {
    if (json_output) {
        write_json_repair_table_style_look_result(std::cout, result);
        std::cout << '\n';
        return;
    }

    const auto mode =
        result.apply ? std::string_view{"apply"} : std::string_view{"plan"};
    std::cout << "table_style_look_repair: " << mode << '\n'
              << "before_issue_count: " << result.before.issue_count() << '\n'
              << "applicable_issue_count: " << result.applicable_issue_count
              << '\n'
              << "changed_table_count: " << result.changed_table_count << '\n'
              << "after_issue_count: ";
    if (result.after.has_value()) {
        std::cout << result.after->issue_count();
    } else {
        std::cout << "none";
    }
    std::cout << '\n';
    if (result.output_path.has_value()) {
        std::cout << "output: " << result.output_path->string() << '\n';
    }
}

} // namespace featherdoc_cli
