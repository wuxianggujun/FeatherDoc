#include "featherdoc_cli_style_refactor_commands_detail.hpp"

#include "featherdoc_cli_command_support.hpp"
#include "featherdoc_cli_errors.hpp"
#include "featherdoc_cli_json.hpp"
#include "featherdoc_cli_parse.hpp"
#include "featherdoc_cli_style_options_parse.hpp"
#include "featherdoc_cli_style_output.hpp"

#include <cstddef>
#include <iostream>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <vector>

namespace featherdoc_cli {

auto run_rename_style_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 4U) {
        print_parse_error(
            command,
            "rename-style expects an input path, an old style id, and a new style id",
            json_output);
        return 2;
    }

    const auto old_style_id = std::string(arguments[2]);
    const auto new_style_id = std::string(arguments[3]);
    rename_style_options options;
    std::string error_message;
    if (!parse_rename_style_options(arguments, 4U, options, error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return 1;
    }

    if (!doc.rename_style(old_style_id, new_style_id)) {
        report_document_error(command, "mutate", doc.last_error(),
                              options.json_output);
        return 1;
    }

    const auto style = doc.find_style(new_style_id);
    if (!style.has_value()) {
        report_document_error(command, "inspect", doc.last_error(),
                              options.json_output);
        return 1;
    }

    if (!save_document(doc, options.output_path, command, options.json_output)) {
        return 1;
    }

    if (options.json_output) {
        write_json_mutation_result(
            command, doc, options.output_path,
            [&old_style_id, &new_style_id, &style](std::ostream &stream) {
                stream << ",\"old_style_id\":";
                write_json_string(stream, old_style_id);
                stream << ",\"new_style_id\":";
                write_json_string(stream, new_style_id);
                stream << ",\"style\":";
                write_json_style_summary(stream, *style);
            });
        return 0;
    }

    inspect_style(*style, std::nullopt, false);
    return 0;
}

auto run_merge_style_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 4U) {
        print_parse_error(
            command,
            "merge-style expects an input path, a source style id, and a target style id",
            json_output);
        return 2;
    }

    const auto source_style_id = std::string(arguments[2]);
    const auto target_style_id = std::string(arguments[3]);
    merge_style_options options;
    std::string error_message;
    if (!parse_rename_style_options(arguments, 4U, options, error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return 1;
    }

    if (!doc.merge_style(source_style_id, target_style_id)) {
        report_document_error(command, "mutate", doc.last_error(),
                              options.json_output);
        return 1;
    }

    const auto style = doc.find_style(target_style_id);
    if (!style.has_value()) {
        report_document_error(command, "inspect", doc.last_error(),
                              options.json_output);
        return 1;
    }

    if (!save_document(doc, options.output_path, command, options.json_output)) {
        return 1;
    }

    if (options.json_output) {
        write_json_mutation_result(
            command, doc, options.output_path,
            [&source_style_id, &target_style_id, &style](
                std::ostream &stream) {
                stream << ",\"source_style_id\":";
                write_json_string(stream, source_style_id);
                stream << ",\"target_style_id\":";
                write_json_string(stream, target_style_id);
                stream << ",\"style\":";
                write_json_style_summary(stream, *style);
            });
        return 0;
    }

    inspect_style(*style, std::nullopt, false);
    return 0;
}

auto run_plan_prune_unused_styles_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 2U || arguments[1].starts_with("--")) {
        print_parse_error(command, "plan-prune-unused-styles expects an input path",
                          json_output);
        return 2;
    }

    for (std::size_t index = 2U; index < arguments.size(); ++index) {
        if (arguments[index] == "--json") {
            continue;
        }
        print_parse_error(command,
                          "unknown option: " + std::string(arguments[index]),
                          json_output);
        return 2;
    }

    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       json_output)) {
        return 1;
    }

    const auto plan = doc.plan_prune_unused_styles();
    if (!plan.has_value()) {
        report_document_error(command, "inspect", doc.last_error(), json_output);
        return 1;
    }

    if (json_output) {
        std::cout << "{\"command\":\"plan-prune-unused-styles\",\"ok\":true";
        write_json_style_prune_plan(std::cout, *plan);
        std::cout << "}\n";
        return 0;
    }

    print_style_prune_plan(*plan);
    return 0;
}

auto run_prune_unused_styles_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 2U || arguments[1].starts_with("--")) {
        print_parse_error(command, "prune-unused-styles expects an input path",
                          json_output);
        return 2;
    }

    prune_unused_styles_options options;
    std::string error_message;
    if (!parse_rename_style_options(arguments, 2U, options, error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return 1;
    }

    const auto summary = doc.prune_unused_styles();
    if (!summary.has_value()) {
        report_document_error(command, "mutate", doc.last_error(),
                              options.json_output);
        return 1;
    }

    if (!save_document(doc, options.output_path, command, options.json_output)) {
        return 1;
    }

    if (options.json_output) {
        write_json_mutation_result(
            command, doc, options.output_path,
            [&summary](std::ostream &stream) {
                write_json_style_prune_summary(stream, *summary);
            });
        return 0;
    }

    print_style_prune_summary(*summary);
    return 0;
}

} // namespace featherdoc_cli
