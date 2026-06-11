#pragma once

#include "featherdoc_cli_table_style_output.hpp"

#include <featherdoc.hpp>

#include <iosfwd>
#include <optional>
#include <string>

namespace featherdoc_cli {

void write_json_table_style_region_audit_report(
    std::ostream &stream,
    const featherdoc::table_style_region_audit_report &report,
    const std::optional<std::string> &style_id, bool fail_on_issue);

void write_json_table_style_inheritance_audit_report(
    std::ostream &stream,
    const featherdoc::table_style_inheritance_audit_report &report,
    const std::optional<std::string> &style_id, bool fail_on_issue);

void write_json_table_style_look_report(
    std::ostream &stream,
    const featherdoc::table_style_look_consistency_report &report,
    bool fail_on_issue);

void write_json_table_style_quality_audit_report(
    std::ostream &stream,
    const featherdoc::table_style_quality_audit_report &report,
    bool fail_on_issue);

void write_json_table_style_quality_fix_plan(
    std::ostream &stream,
    const featherdoc::table_style_quality_fix_plan &plan,
    bool fail_on_issue);

void write_json_apply_table_style_quality_fixes_result(
    std::ostream &stream, const table_style_quality_apply_cli_result &result);

void write_json_repair_table_style_look_result(
    std::ostream &stream, const table_style_look_repair_cli_result &result);

} // namespace featherdoc_cli
