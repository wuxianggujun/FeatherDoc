#pragma once

#include "featherdoc_cli_command_support.hpp"

#include <featherdoc.hpp>

#include <cstddef>
#include <iosfwd>
#include <optional>
#include <string>

namespace featherdoc_cli {

struct table_style_look_repair_cli_result {
    bool apply = false;
    std::optional<path_type> output_path;
    featherdoc::table_style_look_consistency_report before;
    std::optional<featherdoc::table_style_look_consistency_report> after;
    std::size_t applicable_issue_count{0};
    std::size_t changed_table_count{0};
};

struct table_style_quality_apply_cli_result {
    std::optional<path_type> output_path;
    featherdoc::table_style_quality_fix_plan before;
    featherdoc::table_style_quality_fix_plan after;
    std::size_t changed_table_count{0};
};

void write_json_table_style_look(
    std::ostream &stream, const featherdoc::table_style_look &style_look);

void inspect_table_style_definition(
    const featherdoc::table_style_definition_summary &definition,
    bool json_output);
void audit_table_style_regions(
    const featherdoc::table_style_region_audit_report &report,
    const std::optional<std::string> &style_id, bool fail_on_issue,
    bool json_output);
void audit_table_style_inheritance(
    const featherdoc::table_style_inheritance_audit_report &report,
    const std::optional<std::string> &style_id, bool fail_on_issue,
    bool json_output);
void check_table_style_look(
    const featherdoc::table_style_look_consistency_report &report,
    bool fail_on_issue, bool json_output);
void audit_table_style_quality(
    const featherdoc::table_style_quality_audit_report &report,
    bool fail_on_issue, bool json_output);
void plan_table_style_quality_fixes(
    const featherdoc::table_style_quality_fix_plan &plan,
    bool fail_on_issue, bool json_output);
void apply_table_style_quality_fixes(
    const table_style_quality_apply_cli_result &result, bool json_output);
void repair_table_style_look(
    const table_style_look_repair_cli_result &result, bool json_output);
[[nodiscard]] auto count_applicable_table_style_look_issues(
    const featherdoc::table_style_look_consistency_report &report)
    -> std::size_t;

} // namespace featherdoc_cli
