#include "featherdoc_cli_table_position_commands.hpp"

#include "featherdoc_cli_command_support.hpp"
#include "featherdoc_cli_errors.hpp"
#include "featherdoc_cli_json.hpp"
#include "featherdoc_cli_parse.hpp"
#include "featherdoc_cli_table_position_options_parse.hpp"
#include "featherdoc_cli_table_position_output.hpp"
#include "featherdoc_cli_table_position_plan_build.hpp"
#include "featherdoc_cli_table_position_plan_parse.hpp"

#include <featherdoc.hpp>

#include <algorithm>
#include <cstddef>
#include <iostream>
#include <ostream>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace featherdoc_cli {
namespace {

void write_text_size_array(std::ostream &stream,
                           const std::vector<std::size_t> &values) {
    stream << '[';
    for (std::size_t index = 0; index < values.size(); ++index) {
        if (index > 0U) {
            stream << ',';
        }
        stream << values[index];
    }
    stream << ']';
}

auto report_table_position_failure(std::string_view command,
                                   std::string_view stage,
                                   std::string_view summary,
                                   std::string detail,
                                   bool json_output) -> bool {
    featherdoc::document_error_info error_info{};
    error_info.code = std::make_error_code(std::errc::invalid_argument);
    error_info.detail = std::move(detail);
    error_info.entry_name = "word/document.xml";
    return report_operation_failure(command, stage, summary, error_info,
                                    json_output);
}

auto parse_table_position_targets(
    std::string_view command, const std::vector<std::string_view> &arguments,
    bool target_all,
    const std::vector<std::size_t> &additional_table_indices,
    std::vector<std::size_t> &requested_table_indices, bool json_output)
    -> bool {
    if (!target_all) {
        std::size_t table_index = 0U;
        if (!parse_index(arguments[2], table_index)) {
            print_parse_error(command,
                              "invalid table index: " +
                                  std::string(arguments[2]),
                              json_output);
            return false;
        }
        requested_table_indices.push_back(table_index);
    }

    if (target_all && !additional_table_indices.empty()) {
        print_parse_error(command,
                          "--table cannot be combined with all table target",
                          json_output);
        return false;
    }

    for (const auto additional_table_index : additional_table_indices) {
        if (std::find(requested_table_indices.begin(),
                      requested_table_indices.end(),
                      additional_table_index) != requested_table_indices.end()) {
            print_parse_error(command,
                              "duplicate table target: " +
                                  std::to_string(additional_table_index),
                              json_output);
            return false;
        }
        requested_table_indices.push_back(additional_table_index);
    }

    return true;
}

template <typename Mutator>
auto mutate_table_positions(
    std::string_view command, featherdoc::Document &doc, bool target_all,
    const std::vector<std::size_t> &requested_table_indices, bool json_output,
    std::string_view failure_summary, Mutator &&mutator,
    std::vector<std::size_t> &mutated_table_indices) -> bool {
    if (target_all) {
        std::size_t table_index = 0U;
        for (featherdoc::Table table = doc.tables(); table.has_next();
             table.next(), ++table_index) {
            if (!mutator(table)) {
                return report_table_position_failure(
                    command, "mutate", failure_summary,
                    "target table handle is not valid", json_output);
            }
            mutated_table_indices.push_back(table_index);
        }

        if (mutated_table_indices.empty()) {
            return report_table_position_failure(command, "mutate",
                                                 "no body tables found",
                                                 "no body tables found",
                                                 json_output);
        }
        return true;
    }

    for (const auto table_index : requested_table_indices) {
        featherdoc::Table table;
        if (!resolve_body_table(doc, table_index, table, command,
                                json_output)) {
            return false;
        }

        if (!mutator(table)) {
            return report_table_position_failure(
                command, "mutate", failure_summary,
                "target table handle is not valid", json_output);
        }
        mutated_table_indices.push_back(table_index);
    }

    return true;
}

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

auto run_set_table_position_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 3U) {
        print_parse_error(command,
                          "set-table-position expects an input path and a table index",
                          json_output);
        return 2;
    }

    const auto target_all = arguments[2] == "all";

    table_position_options options;
    std::string error_message;
    if (!parse_table_position_options(arguments, 3U, options, error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    std::vector<std::size_t> requested_table_indices;
    if (!parse_table_position_targets(command, arguments, target_all,
                                      options.additional_table_indices,
                                      requested_table_indices,
                                      options.json_output)) {
        return 2;
    }

    featherdoc::Document doc;
    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return 1;
    }

    std::vector<std::size_t> mutated_table_indices;
    if (!mutate_table_positions(
            command, doc, target_all, requested_table_indices,
            options.json_output, "failed to set table position",
            [&options](featherdoc::Table &table) {
                return table.set_position(options.position);
            },
            mutated_table_indices)) {
        return 1;
    }

    if (!save_document(doc, options.output_path, command,
                       options.json_output)) {
        return 1;
    }

    if (options.json_output) {
        write_json_mutation_result(
            command, doc, options.output_path,
            [&mutated_table_indices, &options](std::ostream &stream) {
                stream << ",\"table_indices\":[";
                for (std::size_t index = 0;
                     index < mutated_table_indices.size(); ++index) {
                    if (index > 0U) {
                        stream << ',';
                    }
                    stream << mutated_table_indices[index];
                }
                stream << "],\"positions\":[";
                for (std::size_t index = 0;
                     index < mutated_table_indices.size(); ++index) {
                    if (index > 0U) {
                        stream << ',';
                    }
                    write_json_table_position(stream, options.position);
                }
                stream << ']';
            });
    }

    return 0;
}

auto run_clear_table_position_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 3U) {
        print_parse_error(
            command,
            "clear-table-position expects an input path and a table index",
            json_output);
        return 2;
    }

    const auto target_all = arguments[2] == "all";

    table_position_target_options options;
    std::string error_message;
    if (!parse_table_position_target_options(arguments, 3U, options,
                                             error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    std::vector<std::size_t> requested_table_indices;
    if (!parse_table_position_targets(command, arguments, target_all,
                                      options.additional_table_indices,
                                      requested_table_indices,
                                      options.json_output)) {
        return 2;
    }

    featherdoc::Document doc;
    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return 1;
    }

    std::vector<std::size_t> mutated_table_indices;
    if (!mutate_table_positions(
            command, doc, target_all, requested_table_indices,
            options.json_output, "failed to clear table position",
            [](featherdoc::Table &table) { return table.clear_position(); },
            mutated_table_indices)) {
        return 1;
    }

    if (!save_document(doc, options.output_path, command,
                       options.json_output)) {
        return 1;
    }

    if (options.json_output) {
        write_json_mutation_result(
            command, doc, options.output_path,
            [&mutated_table_indices](std::ostream &stream) {
                stream << ",\"table_indices\":[";
                for (std::size_t index = 0;
                     index < mutated_table_indices.size(); ++index) {
                    if (index > 0U) {
                        stream << ',';
                    }
                    stream << mutated_table_indices[index];
                }
                stream << "],\"positions\":[";
                for (std::size_t index = 0;
                     index < mutated_table_indices.size(); ++index) {
                    if (index > 0U) {
                        stream << ',';
                    }
                    stream << "null";
                }
                stream << ']';
            });
    }

    return 0;
}

} // namespace

auto is_table_position_command(std::string_view command) -> bool {
    return command == "plan-table-position-presets" ||
           command == "apply-table-position-plan" ||
           command == "set-table-position" ||
           command == "clear-table-position";
}

auto run_table_position_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int {
    if (command == "plan-table-position-presets") {
        return run_plan_table_position_presets_command(command, arguments);
    }
    if (command == "apply-table-position-plan") {
        return run_apply_table_position_plan_command(command, arguments);
    }
    if (command == "set-table-position") {
        return run_set_table_position_command(command, arguments);
    }
    if (command == "clear-table-position") {
        return run_clear_table_position_command(command, arguments);
    }

    return 2;
}

} // namespace featherdoc_cli
