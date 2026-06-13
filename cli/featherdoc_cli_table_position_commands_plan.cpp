#include "featherdoc_cli_table_position_commands_detail.hpp"

#include "featherdoc_cli_command_support.hpp"
#include "featherdoc_cli_errors.hpp"
#include "featherdoc_cli_json.hpp"
#include "featherdoc_cli_parse.hpp"
#include "featherdoc_cli_table_position_commands_support.hpp"
#include "featherdoc_cli_table_position_options_parse.hpp"
#include "featherdoc_cli_table_position_output.hpp"
#include "featherdoc_cli_table_position_plan_build.hpp"
#include "featherdoc_cli_table_position_plan_parse.hpp"

#include <featherdoc.hpp>

#include <cstddef>
#include <iostream>
#include <ostream>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace featherdoc_cli {

auto run_plan_table_position_presets_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 2U) {
        print_parse_error(
            command,
            "plan-table-position-presets expects an input path and --preset <name>",
            json_output);
        return 2;
    }

    plan_table_position_presets_options options;
    std::string error_message;
    if (!parse_plan_table_position_presets_options(arguments, 2U, options,
                                                   error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    options.input_path = path_type(std::string(arguments[1]));

    featherdoc::Document doc;
    if (!open_document(*options.input_path, doc, command, options.json_output)) {
        return 1;
    }

    const auto tables = doc.inspect_tables();
    if (const auto &error_info = doc.last_error(); error_info.code) {
        report_document_error(command, "inspect", error_info,
                              options.json_output);
        return 1;
    }

    const auto plan = build_table_position_preset_plan(
        tables, *options.preset, options.replace_positioned,
        options.input_path, options.output_path);
    if (options.output_plan_path.has_value() &&
        !write_table_position_preset_plan_file(*options.output_plan_path, plan,
                                               options, error_message)) {
        featherdoc::document_error_info error_info{};
        error_info.code = std::make_error_code(std::errc::io_error);
        error_info.detail = std::move(error_message);
        report_operation_failure(command, "output",
                                 "failed to write table position preset plan",
                                 error_info, options.json_output);
        return 1;
    }

    if (options.json_output) {
        write_json_table_position_preset_plan(std::cout, plan, options);
    } else {
        write_text_table_position_preset_plan(plan, options);
    }

    return options.fail_on_change && !plan.items.empty() ? 1 : 0;
}

auto run_apply_table_position_plan_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 2U) {
        print_parse_error(command,
                          "apply-table-position-plan expects a plan path",
                          json_output);
        return 2;
    }

    apply_table_position_plan_options options;
    std::string error_message;
    if (!parse_apply_table_position_plan_options(arguments, 2U, options,
                                                 error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    parsed_table_position_plan_file plan;
    if (!read_table_position_plan_file(path_type(std::string(arguments[1])),
                                       plan, error_message)) {
        report_table_position_failure(command, "plan",
                                      "failed to read table position plan",
                                      std::move(error_message),
                                      options.json_output);
        return 1;
    }

    const auto output_path = options.output_path.has_value()
                                 ? options.output_path
                                 : plan.resolved_output_path;
    if (!options.dry_run && !output_path.has_value()) {
        report_table_position_failure(
            command, "plan", "missing safe output path",
            "table position plan has no resolved output path; rerun "
            "plan-table-position-presets with --output or pass --output to "
            "apply-table-position-plan",
            options.json_output);
        return 1;
    }

    featherdoc::Document doc;
    if (!open_document(plan.input_path, doc, command, options.json_output)) {
        return 1;
    }

    const auto current_tables = doc.inspect_tables();
    if (const auto &error_info = doc.last_error(); error_info.code) {
        report_document_error(command, "inspect", error_info,
                              options.json_output);
        return 1;
    }

    if (current_tables.size() != plan.table_count) {
        report_table_position_failure(
            command, "plan", "table count mismatch",
            "table count changed since plan was generated: " +
                std::to_string(current_tables.size()) + " current, " +
                std::to_string(plan.table_count) + " planned",
            options.json_output);
        return 1;
    }

    for (const auto &expected_fingerprint : plan.table_fingerprints) {
        const auto current_fingerprint = make_table_position_table_fingerprint(
            current_tables[expected_fingerprint.table_index]);
        if (!table_position_table_fingerprints_equal(expected_fingerprint,
                                                     current_fingerprint)) {
            featherdoc::document_error_info error_info{};
            error_info.detail = describe_table_position_fingerprint_differences(
                expected_fingerprint, current_fingerprint);
            error_info.entry_name = "word/document.xml";
            report_operation_failure(command, "plan",
                                     "table fingerprint mismatch", error_info,
                                     options.json_output);
            return 1;
        }
    }

    const auto target_position = make_table_position_preset(plan.preset);
    if (options.dry_run) {
        if (options.json_output) {
            std::cout << "{\"command\":\"apply-table-position-plan\""
                      << ",\"ok\":true,\"dry_run\":true"
                      << ",\"input_path\":";
            write_json_string(std::cout, plan.input_path.string());
            std::cout << ",\"preset\":";
            write_json_string(std::cout,
                              table_position_preset_name(plan.preset));
            std::cout << ",\"table_count\":" << plan.table_count
                      << ",\"fingerprint_checked_count\":"
                      << plan.table_fingerprints.size()
                      << ",\"would_apply_count\":"
                      << plan.automatic_table_indices.size()
                      << ",\"table_indices\":";
            write_json_size_array(std::cout, plan.automatic_table_indices);
            std::cout << ",\"resolved_output_path\":";
            if (output_path.has_value()) {
                write_json_string(std::cout, output_path->string());
            } else {
                std::cout << "null";
            }
            std::cout << ",\"position\":";
            write_json_table_position(std::cout, target_position);
            std::cout << "}\n";
        } else {
            std::cout << "table_position_plan_validation: ok\n"
                      << "dry_run: true\n"
                      << "input_path: " << plan.input_path.string() << '\n'
                      << "preset: "
                      << table_position_preset_name(plan.preset) << '\n'
                      << "table_count: " << plan.table_count << '\n'
                      << "fingerprint_checked_count: "
                      << plan.table_fingerprints.size() << '\n'
                      << "would_apply_count: "
                      << plan.automatic_table_indices.size() << '\n'
                      << "table_indices: ";
            write_text_size_array(std::cout, plan.automatic_table_indices);
            std::cout << "\nresolved_output_path: ";
            if (output_path.has_value()) {
                std::cout << output_path->string();
            } else {
                std::cout << "none";
            }
            std::cout << '\n';
        }
        return 0;
    }

    std::vector<std::size_t> mutated_table_indices;
    for (const auto table_index : plan.automatic_table_indices) {
        featherdoc::Table table;
        if (!resolve_body_table(doc, table_index, table, command,
                                options.json_output)) {
            return 1;
        }
        if (!table.set_position(target_position)) {
            report_table_position_failure(command, "mutate",
                                          "failed to set table position",
                                          "target table handle is not valid",
                                          options.json_output);
            return 1;
        }
        mutated_table_indices.push_back(table_index);
    }

    if (!save_document(doc, output_path, command, options.json_output)) {
        return 1;
    }

    if (options.json_output) {
        write_json_mutation_result(
            command, doc, output_path,
            [&plan, &mutated_table_indices,
             &target_position](std::ostream &stream) {
                stream << ",\"input_path\":";
                write_json_string(stream, plan.input_path.string());
                stream << ",\"preset\":";
                write_json_string(stream,
                                  table_position_preset_name(plan.preset));
                stream << ",\"table_count\":" << plan.table_count
                       << ",\"fingerprint_checked_count\":"
                       << plan.table_fingerprints.size()
                       << ",\"applied_count\":"
                       << mutated_table_indices.size()
                       << ",\"table_indices\":";
                write_json_size_array(stream, mutated_table_indices);
                stream << ",\"position\":";
                write_json_table_position(stream, target_position);
            });
    }

    return 0;
}

} // namespace featherdoc_cli
