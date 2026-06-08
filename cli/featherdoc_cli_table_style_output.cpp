#include "featherdoc_cli_table_style_output.hpp"

#include "featherdoc_cli_domain_names.hpp"
#include "featherdoc_cli_json.hpp"
#include "featherdoc_cli_style_output.hpp"
#include "featherdoc_cli_text.hpp"

#include <cstddef>
#include <iostream>
#include <optional>
#include <ostream>
#include <string_view>

namespace featherdoc_cli {

void write_json_table_style_look(
    std::ostream &stream, const featherdoc::table_style_look &style_look) {
    stream << "{\"first_row\":" << json_bool(style_look.first_row)
           << ",\"last_row\":" << json_bool(style_look.last_row)
           << ",\"first_column\":" << json_bool(style_look.first_column)
           << ",\"last_column\":" << json_bool(style_look.last_column)
           << ",\"banded_rows\":" << json_bool(style_look.banded_rows)
           << ",\"banded_columns\":" << json_bool(style_look.banded_columns)
           << '}';
}


void write_json_optional_table_style_cell_vertical_alignment(
    std::ostream &stream,
    const std::optional<featherdoc::cell_vertical_alignment> &alignment) {
    if (!alignment.has_value()) {
        stream << "null";
        return;
    }

    write_json_string(stream,
                      table_style_cell_vertical_alignment_name(*alignment));
}

void write_json_optional_table_style_cell_text_direction(
    std::ostream &stream,
    const std::optional<featherdoc::cell_text_direction> &direction) {
    if (!direction.has_value()) {
        stream << "null";
        return;
    }

    write_json_string(stream, table_style_cell_text_direction_name(*direction));
}

void write_json_optional_table_style_paragraph_alignment(
    std::ostream &stream,
    const std::optional<featherdoc::paragraph_alignment> &alignment) {
    if (!alignment.has_value()) {
        stream << "null";
        return;
    }

    write_json_string(stream, table_style_paragraph_alignment_name(*alignment));
}

void write_json_optional_table_style_paragraph_line_spacing_rule(
    std::ostream &stream,
    const std::optional<featherdoc::paragraph_line_spacing_rule> &rule) {
    if (!rule.has_value()) {
        stream << "null";
        return;
    }

    write_json_string(stream,
                      table_style_paragraph_line_spacing_rule_name(*rule));
}

void write_json_table_style_paragraph_spacing(
    std::ostream &stream,
    const featherdoc::table_style_paragraph_spacing_definition &spacing) {
    stream << "{\"before_twips\":";
    write_json_optional_u32(stream, spacing.before_twips);
    stream << ",\"after_twips\":";
    write_json_optional_u32(stream, spacing.after_twips);
    stream << ",\"line_twips\":";
    write_json_optional_u32(stream, spacing.line_twips);
    stream << ",\"line_rule\":";
    write_json_optional_table_style_paragraph_line_spacing_rule(
        stream, spacing.line_rule);
    stream << '}';
}

void write_json_table_style_margins(
    std::ostream &stream,
    const featherdoc::table_style_margins_definition &margins) {
    stream << "{\"top_twips\":";
    write_json_optional_u32(stream, margins.top_twips);
    stream << ",\"left_twips\":";
    write_json_optional_u32(stream, margins.left_twips);
    stream << ",\"bottom_twips\":";
    write_json_optional_u32(stream, margins.bottom_twips);
    stream << ",\"right_twips\":";
    write_json_optional_u32(stream, margins.right_twips);
    stream << '}';
}

void write_json_table_style_border(
    std::ostream &stream,
    const featherdoc::table_style_border_summary &border) {
    stream << "{\"style\":";
    write_json_optional_string(stream, border.style);
    stream << ",\"size_eighth_points\":";
    write_json_optional_u32(stream, border.size_eighth_points);
    stream << ",\"color\":";
    write_json_optional_string(stream, border.color);
    stream << ",\"space_points\":";
    write_json_optional_u32(stream, border.space_points);
    stream << '}';
}

void write_json_optional_table_style_border(
    std::ostream &stream,
    const std::optional<featherdoc::table_style_border_summary> &border) {
    if (!border.has_value()) {
        stream << "null";
        return;
    }

    write_json_table_style_border(stream, *border);
}

void write_json_table_style_borders(
    std::ostream &stream,
    const featherdoc::table_style_borders_summary &borders) {
    stream << "{\"top\":";
    write_json_optional_table_style_border(stream, borders.top);
    stream << ",\"left\":";
    write_json_optional_table_style_border(stream, borders.left);
    stream << ",\"bottom\":";
    write_json_optional_table_style_border(stream, borders.bottom);
    stream << ",\"right\":";
    write_json_optional_table_style_border(stream, borders.right);
    stream << ",\"inside_horizontal\":";
    write_json_optional_table_style_border(stream, borders.inside_horizontal);
    stream << ",\"inside_vertical\":";
    write_json_optional_table_style_border(stream, borders.inside_vertical);
    stream << '}';
}

void write_json_table_style_region(
    std::ostream &stream,
    const featherdoc::table_style_region_summary &region) {
    stream << "{\"fill_color\":";
    write_json_optional_string(stream, region.fill_color);
    stream << ",\"text_color\":";
    write_json_optional_string(stream, region.text_color);
    stream << ",\"bold\":";
    write_json_optional_bool(stream, region.bold);
    stream << ",\"italic\":";
    write_json_optional_bool(stream, region.italic);
    stream << ",\"font_size_points\":";
    write_json_optional_u32(stream, region.font_size_points);
    stream << ",\"font_family\":";
    write_json_optional_string(stream, region.font_family);
    stream << ",\"east_asia_font_family\":";
    write_json_optional_string(stream, region.east_asia_font_family);
    stream << ",\"cell_vertical_alignment\":";
    write_json_optional_table_style_cell_vertical_alignment(
        stream, region.cell_vertical_alignment);
    stream << ",\"cell_text_direction\":";
    write_json_optional_table_style_cell_text_direction(
        stream, region.cell_text_direction);
    stream << ",\"paragraph_alignment\":";
    write_json_optional_table_style_paragraph_alignment(
        stream, region.paragraph_alignment);
    stream << ",\"paragraph_spacing\":";
    if (region.paragraph_spacing.has_value()) {
        write_json_table_style_paragraph_spacing(stream,
                                                 *region.paragraph_spacing);
    } else {
        stream << "null";
    }
    stream << ",\"cell_margins\":";
    if (region.cell_margins.has_value()) {
        write_json_table_style_margins(stream, *region.cell_margins);
    } else {
        stream << "null";
    }
    stream << ",\"borders\":";
    if (region.borders.has_value()) {
        write_json_table_style_borders(stream, *region.borders);
    } else {
        stream << "null";
    }
    stream << '}';
}

void write_json_optional_table_style_region(
    std::ostream &stream,
    const std::optional<featherdoc::table_style_region_summary> &region) {
    if (!region.has_value()) {
        stream << "null";
        return;
    }

    write_json_table_style_region(stream, *region);
}

void write_json_table_style_definition_summary(
    std::ostream &stream,
    const featherdoc::table_style_definition_summary &definition) {
    stream << "{\"style\":";
    write_json_style_summary(stream, definition.style);
    stream << ",\"regions\":{\"whole_table\":";
    write_json_optional_table_style_region(stream, definition.whole_table);
    stream << ",\"first_row\":";
    write_json_optional_table_style_region(stream, definition.first_row);
    stream << ",\"last_row\":";
    write_json_optional_table_style_region(stream, definition.last_row);
    stream << ",\"first_column\":";
    write_json_optional_table_style_region(stream, definition.first_column);
    stream << ",\"last_column\":";
    write_json_optional_table_style_region(stream, definition.last_column);
    stream << ",\"banded_rows\":";
    write_json_optional_table_style_region(stream, definition.banded_rows);
    stream << ",\"banded_columns\":";
    write_json_optional_table_style_region(stream, definition.banded_columns);
    stream << ",\"second_banded_rows\":";
    write_json_optional_table_style_region(stream,
                                           definition.second_banded_rows);
    stream << ",\"second_banded_columns\":";
    write_json_optional_table_style_region(stream,
                                           definition.second_banded_columns);
    stream << "}}";
}

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

void write_json_table_style_look_report(
    std::ostream &stream,
    const featherdoc::table_style_look_consistency_report &report,
    bool fail_on_issue) {
    const auto issue_count = report.issue_count();
    stream << "{\"command\":\"check-table-style-look\",\"ok\":"
           << json_bool(report.ok()) << ",\"table_count\":"
           << report.table_count << ",\"issue_count\":" << issue_count
           << ",\"fail_on_issue\":" << json_bool(fail_on_issue)
           << ",\"issues\":[";
    for (std::size_t index = 0U; index < report.issues.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        write_json_table_style_look_issue(stream, report.issues[index]);
    }
    stream << "]}";
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

void print_optional_u32_field(std::ostream &stream, std::string_view name,
                              const std::optional<std::uint32_t> &value) {
    stream << ' ' << name << '=';
    if (value.has_value()) {
        stream << *value;
    } else {
        stream << '-';
    }
}

void print_optional_string_field(
    std::ostream &stream, std::string_view name,
    const std::optional<std::string> &value) {
    stream << ' ' << name << '=';
    if (value.has_value()) {
        stream << *value;
    } else {
        stream << '-';
    }
}

void print_table_style_margins(
    std::ostream &stream,
    const featherdoc::table_style_margins_definition &margins) {
    print_optional_u32_field(stream, "top_twips", margins.top_twips);
    print_optional_u32_field(stream, "left_twips", margins.left_twips);
    print_optional_u32_field(stream, "bottom_twips", margins.bottom_twips);
    print_optional_u32_field(stream, "right_twips", margins.right_twips);
}

void print_optional_table_style_paragraph_line_spacing_rule(
    std::ostream &stream, std::string_view name,
    const std::optional<featherdoc::paragraph_line_spacing_rule> &rule) {
    stream << ' ' << name << '=';
    if (rule.has_value()) {
        stream << table_style_paragraph_line_spacing_rule_name(*rule);
    } else {
        stream << '-';
    }
}

void print_table_style_paragraph_spacing(
    std::ostream &stream,
    const featherdoc::table_style_paragraph_spacing_definition &spacing) {
    print_optional_u32_field(stream, "paragraph_spacing_before_twips",
                             spacing.before_twips);
    print_optional_u32_field(stream, "paragraph_spacing_after_twips",
                             spacing.after_twips);
    print_optional_u32_field(stream, "paragraph_spacing_line_twips",
                             spacing.line_twips);
    print_optional_table_style_paragraph_line_spacing_rule(
        stream, "paragraph_spacing_line_rule", spacing.line_rule);
}

void print_table_style_border(
    std::ostream &stream, std::string_view name,
    const featherdoc::table_style_border_summary &border) {
    stream << "  border_" << name << ':';
    print_optional_string_field(stream, "style", border.style);
    print_optional_u32_field(stream, "size_eighth_points",
                             border.size_eighth_points);
    print_optional_string_field(stream, "color", border.color);
    print_optional_u32_field(stream, "space_points", border.space_points);
    stream << '\n';
}

void print_optional_table_style_border(
    std::ostream &stream, std::string_view name,
    const std::optional<featherdoc::table_style_border_summary> &border) {
    if (border.has_value()) {
        print_table_style_border(stream, name, *border);
    }
}

void print_table_style_region(
    std::ostream &stream, std::string_view name,
    const featherdoc::table_style_region_summary &region) {
    stream << "region_" << name << ": fill_color=";
    if (region.fill_color.has_value()) {
        stream << *region.fill_color;
    } else {
        stream << '-';
    }
    stream << " text_color=";
    if (region.text_color.has_value()) {
        stream << *region.text_color;
    } else {
        stream << '-';
    }
    stream << " bold=";
    if (region.bold.has_value()) {
        stream << json_bool(*region.bold);
    } else {
        stream << '-';
    }
    stream << " italic=";
    if (region.italic.has_value()) {
        stream << json_bool(*region.italic);
    } else {
        stream << '-';
    }
    print_optional_u32_field(stream, "font_size_points",
                             region.font_size_points);
    print_optional_string_field(stream, "font_family", region.font_family);
    print_optional_string_field(stream, "east_asia_font_family",
                                region.east_asia_font_family);
    stream << " cell_vertical_alignment=";
    if (region.cell_vertical_alignment.has_value()) {
        stream << table_style_cell_vertical_alignment_name(
            *region.cell_vertical_alignment);
    } else {
        stream << '-';
    }
    stream << " cell_text_direction=";
    if (region.cell_text_direction.has_value()) {
        stream << table_style_cell_text_direction_name(
            *region.cell_text_direction);
    } else {
        stream << '-';
    }
    stream << " paragraph_alignment=";
    if (region.paragraph_alignment.has_value()) {
        stream << table_style_paragraph_alignment_name(
            *region.paragraph_alignment);
    } else {
        stream << '-';
    }
    if (region.paragraph_spacing.has_value()) {
        print_table_style_paragraph_spacing(stream, *region.paragraph_spacing);
    }
    if (region.cell_margins.has_value()) {
        print_table_style_margins(stream, *region.cell_margins);
    }
    stream << '\n';

    if (region.borders.has_value()) {
        print_optional_table_style_border(stream, "top", region.borders->top);
        print_optional_table_style_border(stream, "left", region.borders->left);
        print_optional_table_style_border(stream, "bottom",
                                          region.borders->bottom);
        print_optional_table_style_border(stream, "right",
                                          region.borders->right);
        print_optional_table_style_border(
            stream, "inside_horizontal", region.borders->inside_horizontal);
        print_optional_table_style_border(
            stream, "inside_vertical", region.borders->inside_vertical);
    }
}

void print_optional_table_style_region(
    std::ostream &stream, std::string_view name,
    const std::optional<featherdoc::table_style_region_summary> &region) {
    if (region.has_value()) {
        print_table_style_region(stream, name, *region);
    } else {
        stream << "region_" << name << ": none\n";
    }
}

void inspect_table_style_definition(
    const featherdoc::table_style_definition_summary &definition,
    bool json_output) {
    if (json_output) {
        std::cout << "{\"table_style_definition\":";
        write_json_table_style_definition_summary(std::cout, definition);
        std::cout << "}\n";
        return;
    }

    inspect_style(definition.style, std::nullopt, false);
    print_optional_table_style_region(std::cout, "whole_table",
                                      definition.whole_table);
    print_optional_table_style_region(std::cout, "first_row",
                                      definition.first_row);
    print_optional_table_style_region(std::cout, "last_row",
                                      definition.last_row);
    print_optional_table_style_region(std::cout, "first_column",
                                      definition.first_column);
    print_optional_table_style_region(std::cout, "last_column",
                                      definition.last_column);
    print_optional_table_style_region(std::cout, "banded_rows",
                                      definition.banded_rows);
    print_optional_table_style_region(std::cout, "banded_columns",
                                      definition.banded_columns);
    print_optional_table_style_region(std::cout, "second_banded_rows",
                                      definition.second_banded_rows);
    print_optional_table_style_region(std::cout, "second_banded_columns",
                                      definition.second_banded_columns);
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
