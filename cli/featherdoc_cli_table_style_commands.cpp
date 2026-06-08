#include "featherdoc_cli_table_style_commands.hpp"

#include "featherdoc_cli_command_support.hpp"
#include "featherdoc_cli_errors.hpp"
#include "featherdoc_cli_inspect_style_options_parse.hpp"
#include "featherdoc_cli_json.hpp"
#include "featherdoc_cli_parse.hpp"
#include "featherdoc_cli_style_ensure_options_parse.hpp"
#include "featherdoc_cli_style_output.hpp"
#include "featherdoc_cli_table_cell_options_parse.hpp"
#include "featherdoc_cli_table_style_look_options_parse.hpp"
#include "featherdoc_cli_table_style_output.hpp"
#include "featherdoc_cli_text.hpp"

#include <cstddef>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

namespace featherdoc_cli {
namespace {

void print_style_mutation_result(std::string_view command, featherdoc::Document &doc,
                                 const std::optional<path_type> &output_path,
                                 const featherdoc::style_summary &style,
                                 bool json_output) {
    if (json_output) {
        write_json_mutation_result(command, doc, output_path,
                                   [&style](std::ostream &stream) {
                                       stream << ",\"style\":";
                                       write_json_style_summary(stream, style);
                                   });
        return;
    }

    inspect_style(style, std::nullopt, false);
}

auto report_invalid_table_style_target(std::string_view command,
                                       std::string_view summary,
                                       bool json_output) -> bool {
    featherdoc::document_error_info error_info{};
    error_info.code = std::make_error_code(std::errc::invalid_argument);
    error_info.detail = "target table handle is not valid";
    error_info.entry_name = "word/document.xml";
    return report_operation_failure(command, "mutate", summary, error_info,
                                    json_output);
}

auto run_set_table_style_id_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 4U) {
        print_parse_error(
            command,
            "set-table-style-id expects an input path, a table index, and a style id",
            json_output);
        return 2;
    }

    std::size_t table_index = 0U;
    if (!parse_index(arguments[2], table_index)) {
        print_parse_error(command,
                          "invalid table index: " + std::string(arguments[2]),
                          json_output);
        return 2;
    }

    const auto style_id = std::string(arguments[3]);
    if (style_id.empty()) {
        print_parse_error(command, "style id must not be empty", json_output);
        return 2;
    }

    table_cell_style_options options;
    std::string error_message;
    if (!parse_table_cell_style_options(arguments, 4U, options, error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return 1;
    }

    featherdoc::Table table;
    if (!resolve_body_table(doc, table_index, table, command,
                            options.json_output)) {
        return 1;
    }

    if (!table.set_style_id(style_id)) {
        report_invalid_table_style_target(command, "failed to set table style id",
                                          options.json_output);
        return 1;
    }

    if (!save_document(doc, options.output_path, command, options.json_output)) {
        return 1;
    }

    if (options.json_output) {
        write_json_mutation_result(
            command, doc, options.output_path,
            [table_index, &style_id](std::ostream &stream) {
                stream << ",\"table_index\":" << table_index << ",\"style_id\":";
                write_json_string(stream, style_id);
            });
    }

    return 0;
}

auto run_clear_table_style_id_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 3U) {
        print_parse_error(command,
                          "clear-table-style-id expects an input path and a table index",
                          json_output);
        return 2;
    }

    std::size_t table_index = 0U;
    if (!parse_index(arguments[2], table_index)) {
        print_parse_error(command,
                          "invalid table index: " + std::string(arguments[2]),
                          json_output);
        return 2;
    }

    table_cell_style_options options;
    std::string error_message;
    if (!parse_table_cell_style_options(arguments, 3U, options, error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return 1;
    }

    featherdoc::Table table;
    if (!resolve_body_table(doc, table_index, table, command,
                            options.json_output)) {
        return 1;
    }

    if (!table.clear_style_id()) {
        report_invalid_table_style_target(command,
                                          "failed to clear table style id",
                                          options.json_output);
        return 1;
    }

    if (!save_document(doc, options.output_path, command, options.json_output)) {
        return 1;
    }

    if (options.json_output) {
        write_json_mutation_result(command, doc, options.output_path,
                                   [table_index](std::ostream &stream) {
                                       stream << ",\"table_index\":"
                                              << table_index;
                                   });
    }

    return 0;
}

auto run_set_table_style_look_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 3U) {
        print_parse_error(
            command,
            "set-table-style-look expects an input path, a table index, "
            "and at least one style-look flag",
            json_output);
        return 2;
    }

    std::size_t table_index = 0U;
    if (!parse_index(arguments[2], table_index)) {
        print_parse_error(command,
                          "invalid table index: " + std::string(arguments[2]),
                          json_output);
        return 2;
    }

    table_style_look_options options;
    std::string error_message;
    if (!parse_table_style_look_options(arguments, 3U, options, error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }
    if (!table_style_look_options_have_flag(options)) {
        print_parse_error(command,
                          "set-table-style-look requires at least one "
                          "style-look flag",
                          json_output);
        return 2;
    }

    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return 1;
    }

    featherdoc::Table table;
    if (!resolve_body_table(doc, table_index, table, command,
                            options.json_output)) {
        return 1;
    }

    auto style_look =
        table.style_look().value_or(featherdoc::table_style_look{});
    if (options.first_row.has_value()) {
        style_look.first_row = *options.first_row;
    }
    if (options.last_row.has_value()) {
        style_look.last_row = *options.last_row;
    }
    if (options.first_column.has_value()) {
        style_look.first_column = *options.first_column;
    }
    if (options.last_column.has_value()) {
        style_look.last_column = *options.last_column;
    }
    if (options.banded_rows.has_value()) {
        style_look.banded_rows = *options.banded_rows;
    }
    if (options.banded_columns.has_value()) {
        style_look.banded_columns = *options.banded_columns;
    }

    if (!table.set_style_look(style_look)) {
        report_invalid_table_style_target(command,
                                          "failed to set table style look",
                                          options.json_output);
        return 1;
    }

    if (!save_document(doc, options.output_path, command, options.json_output)) {
        return 1;
    }

    if (options.json_output) {
        write_json_mutation_result(
            command, doc, options.output_path,
            [table_index, &style_look](std::ostream &stream) {
                stream << ",\"table_index\":" << table_index
                       << ",\"style_look\":";
                write_json_table_style_look(stream, style_look);
            });
    }

    return 0;
}

auto run_clear_table_style_look_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 3U) {
        print_parse_error(
            command, "clear-table-style-look expects an input path and a table index",
            json_output);
        return 2;
    }

    std::size_t table_index = 0U;
    if (!parse_index(arguments[2], table_index)) {
        print_parse_error(command,
                          "invalid table index: " + std::string(arguments[2]),
                          json_output);
        return 2;
    }

    table_cell_style_options options;
    std::string error_message;
    if (!parse_table_cell_style_options(arguments, 3U, options, error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return 1;
    }

    featherdoc::Table table;
    if (!resolve_body_table(doc, table_index, table, command,
                            options.json_output)) {
        return 1;
    }

    if (!table.clear_style_look()) {
        report_invalid_table_style_target(command,
                                          "failed to clear table style look",
                                          options.json_output);
        return 1;
    }

    if (!save_document(doc, options.output_path, command, options.json_output)) {
        return 1;
    }

    if (options.json_output) {
        write_json_mutation_result(command, doc, options.output_path,
                                   [table_index](std::ostream &stream) {
                                       stream << ",\"table_index\":"
                                              << table_index;
                                   });
    }

    return 0;
}

auto run_ensure_table_style_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 3U) {
        print_parse_error(command,
                          "ensure-table-style expects an input path and a style id",
                          json_output);
        return 2;
    }

    const auto style_id = arguments[2];
    ensure_table_style_options options;
    std::string error_message;
    if (!parse_ensure_table_style_options(arguments, 3U, options,
                                          error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return 1;
    }

    if (!doc.ensure_table_style(style_id, options.definition)) {
        report_document_error(command, "mutate", doc.last_error(),
                              options.json_output);
        return 1;
    }

    if (!save_document(doc, options.output_path, command, options.json_output)) {
        return 1;
    }

    const auto style = doc.find_style(style_id);
    if (!style.has_value()) {
        report_document_error(command, "inspect", doc.last_error(),
                              options.json_output);
        return 1;
    }

    print_style_mutation_result(command, doc, options.output_path, *style,
                                options.json_output);
    return 0;
}

auto run_inspect_table_style_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 3U) {
        print_parse_error(command,
                          "inspect-table-style expects an input path and a style id",
                          json_output);
        return 2;
    }

    inspect_options options;
    std::string error_message;
    if (!parse_inspect_options(arguments, 3U, options, error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return 1;
    }

    const auto definition = doc.find_table_style_definition(arguments[2]);
    if (!definition.has_value()) {
        report_document_error(command, "inspect", doc.last_error(),
                              options.json_output);
        return 1;
    }

    inspect_table_style_definition(*definition, options.json_output);
    return 0;
}

auto run_audit_table_style_regions_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 2U) {
        print_parse_error(command, "audit-table-style-regions expects an input path",
                          json_output);
        return 2;
    }

    audit_table_style_regions_options options;
    std::string error_message;
    if (!parse_audit_table_style_regions_options(arguments, 2U, options,
                                                 error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return 1;
    }

    const auto report = doc.audit_table_style_regions(
        options.style_id.has_value()
            ? std::optional<std::string_view>{std::string_view{*options.style_id}}
            : std::optional<std::string_view>{});
    if (const auto &error_info = doc.last_error(); error_info.code) {
        report_document_error(command, "audit", error_info, options.json_output);
        return 1;
    }

    audit_table_style_regions(report, options.style_id, options.fail_on_issue,
                              options.json_output);
    return report.ok() || !options.fail_on_issue ? 0 : 1;
}

auto run_audit_table_style_inheritance_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 2U) {
        print_parse_error(command,
                          "audit-table-style-inheritance expects an input path",
                          json_output);
        return 2;
    }

    audit_table_style_inheritance_options options;
    std::string error_message;
    if (!parse_audit_table_style_inheritance_options(arguments, 2U, options,
                                                     error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return 1;
    }

    const auto report = doc.audit_table_style_inheritance(
        options.style_id.has_value()
            ? std::optional<std::string_view>{std::string_view{*options.style_id}}
            : std::optional<std::string_view>{});
    if (const auto &error_info = doc.last_error(); error_info.code) {
        report_document_error(command, "audit", error_info, options.json_output);
        return 1;
    }

    audit_table_style_inheritance(report, options.style_id,
                                  options.fail_on_issue, options.json_output);
    return report.ok() || !options.fail_on_issue ? 0 : 1;
}

auto run_audit_table_style_quality_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 2U) {
        print_parse_error(command, "audit-table-style-quality expects an input path",
                          json_output);
        return 2;
    }

    audit_table_style_quality_options options;
    std::string error_message;
    if (!parse_audit_table_style_quality_options(arguments, 2U, options,
                                                 error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return 1;
    }

    const auto report = doc.audit_table_style_quality();
    if (const auto &error_info = doc.last_error(); error_info.code) {
        report_document_error(command, "audit", error_info, options.json_output);
        return 1;
    }

    audit_table_style_quality(report, options.fail_on_issue, options.json_output);
    return report.ok() || !options.fail_on_issue ? 0 : 1;
}

auto run_plan_table_style_quality_fixes_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 2U) {
        print_parse_error(
            command, "plan-table-style-quality-fixes expects an input path",
            json_output);
        return 2;
    }

    plan_table_style_quality_fixes_options options;
    std::string error_message;
    if (!parse_plan_table_style_quality_fixes_options(arguments, 2U, options,
                                                      error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return 1;
    }

    const auto plan = doc.plan_table_style_quality_fixes();
    if (const auto &error_info = doc.last_error(); error_info.code) {
        report_document_error(command, "plan", error_info, options.json_output);
        return 1;
    }

    plan_table_style_quality_fixes(plan, options.fail_on_issue,
                                   options.json_output);
    return plan.ok() || !options.fail_on_issue ? 0 : 1;
}

auto run_apply_table_style_quality_fixes_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 2U) {
        print_parse_error(
            command, "apply-table-style-quality-fixes expects an input path",
            json_output);
        return 2;
    }

    apply_table_style_quality_fixes_options options;
    std::string error_message;
    if (!parse_apply_table_style_quality_fixes_options(arguments, 2U, options,
                                                       error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return 1;
    }

    auto result = table_style_quality_apply_cli_result{};
    result.output_path = options.output_path;
    result.before = doc.plan_table_style_quality_fixes();
    if (const auto &error_info = doc.last_error(); error_info.code) {
        report_document_error(command, "plan", error_info, options.json_output);
        return 1;
    }

    const auto repair_report = doc.repair_table_style_look_consistency();
    if (const auto &error_info = doc.last_error(); error_info.code) {
        report_document_error(command, "repair", error_info, options.json_output);
        return 1;
    }
    result.changed_table_count = repair_report.changed_table_count;
    result.after = doc.plan_table_style_quality_fixes();
    if (const auto &error_info = doc.last_error(); error_info.code) {
        report_document_error(command, "plan", error_info, options.json_output);
        return 1;
    }

    if (!save_document(doc, options.output_path, command, options.json_output)) {
        return 1;
    }

    apply_table_style_quality_fixes(result, options.json_output);
    return 0;
}

auto run_check_table_style_look_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 2U) {
        print_parse_error(command, "check-table-style-look expects an input path",
                          json_output);
        return 2;
    }

    check_table_style_look_options options;
    std::string error_message;
    if (!parse_check_table_style_look_options(arguments, 2U, options,
                                              error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    const auto input_path = path_type(std::string(arguments[1]));
    if (!open_document(input_path, doc, command, options.json_output)) {
        return 1;
    }

    const auto report = doc.check_table_style_look_consistency();
    if (const auto &error_info = doc.last_error(); error_info.code) {
        report_document_error(command, "check", error_info, options.json_output);
        return 1;
    }

    check_table_style_look(report, options.fail_on_issue, options.json_output);
    return report.ok() || !options.fail_on_issue ? 0 : 1;
}

auto run_repair_table_style_look_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 2U) {
        print_parse_error(command, "repair-table-style-look expects an input path",
                          json_output);
        return 2;
    }

    repair_table_style_look_options options;
    std::string error_message;
    if (!parse_repair_table_style_look_options(arguments, 2U, options,
                                               error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    const auto input_path = path_type(std::string(arguments[1]));
    if (!open_document(input_path, doc, command, options.json_output)) {
        return 1;
    }

    auto result = table_style_look_repair_cli_result{};
    result.apply = options.apply;
    result.output_path = options.output_path;
    if (options.apply) {
        const auto repair_report = doc.repair_table_style_look_consistency();
        if (const auto &error_info = doc.last_error(); error_info.code) {
            report_document_error(command, "repair", error_info,
                                  options.json_output);
            return 1;
        }
        result.before = repair_report.before;
        result.after = repair_report.after;
        result.changed_table_count = repair_report.changed_table_count;
        result.applicable_issue_count =
            count_applicable_table_style_look_issues(result.before);
        if (!save_document(doc, options.output_path, command,
                           options.json_output)) {
            return 1;
        }
    } else {
        result.before = doc.check_table_style_look_consistency();
        if (const auto &error_info = doc.last_error(); error_info.code) {
            report_document_error(command, "check", error_info,
                                  options.json_output);
            return 1;
        }
        result.applicable_issue_count =
            count_applicable_table_style_look_issues(result.before);
    }

    repair_table_style_look(result, options.json_output);
    return 0;
}

} // namespace

auto is_table_style_command(std::string_view command) -> bool {
    return command == "set-table-style-id" ||
           command == "clear-table-style-id" ||
           command == "set-table-style-look" ||
           command == "clear-table-style-look" ||
           command == "ensure-table-style" ||
           command == "inspect-table-style" ||
           command == "audit-table-style-regions" ||
           command == "audit-table-style-inheritance" ||
           command == "audit-table-style-quality" ||
           command == "plan-table-style-quality-fixes" ||
           command == "apply-table-style-quality-fixes" ||
           command == "check-table-style-look" ||
           command == "repair-table-style-look";
}

auto run_table_style_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    if (command == "set-table-style-id") {
        return run_set_table_style_id_command(command, arguments, doc);
    }
    if (command == "clear-table-style-id") {
        return run_clear_table_style_id_command(command, arguments, doc);
    }
    if (command == "set-table-style-look") {
        return run_set_table_style_look_command(command, arguments, doc);
    }
    if (command == "clear-table-style-look") {
        return run_clear_table_style_look_command(command, arguments, doc);
    }
    if (command == "ensure-table-style") {
        return run_ensure_table_style_command(command, arguments, doc);
    }
    if (command == "inspect-table-style") {
        return run_inspect_table_style_command(command, arguments, doc);
    }
    if (command == "audit-table-style-regions") {
        return run_audit_table_style_regions_command(command, arguments, doc);
    }
    if (command == "audit-table-style-inheritance") {
        return run_audit_table_style_inheritance_command(command, arguments, doc);
    }
    if (command == "audit-table-style-quality") {
        return run_audit_table_style_quality_command(command, arguments, doc);
    }
    if (command == "plan-table-style-quality-fixes") {
        return run_plan_table_style_quality_fixes_command(command, arguments, doc);
    }
    if (command == "apply-table-style-quality-fixes") {
        return run_apply_table_style_quality_fixes_command(command, arguments, doc);
    }
    if (command == "check-table-style-look") {
        return run_check_table_style_look_command(command, arguments, doc);
    }
    if (command == "repair-table-style-look") {
        return run_repair_table_style_look_command(command, arguments, doc);
    }

    return 2;
}

} // namespace featherdoc_cli
