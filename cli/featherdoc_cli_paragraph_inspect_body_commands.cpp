#include "featherdoc_cli_paragraph_inspect_commands_detail.hpp"

#include "featherdoc_cli_command_support.hpp"
#include "featherdoc_cli_errors.hpp"
#include "featherdoc_cli_inspect_content_options_parse.hpp"
#include "featherdoc_cli_json.hpp"
#include "featherdoc_cli_paragraph_inspect_load.hpp"
#include "featherdoc_cli_paragraph_inspect_output.hpp"
#include "featherdoc_cli_paragraph_run_options_parse.hpp"
#include "featherdoc_cli_parse.hpp"

#include <featherdoc.hpp>

#include <cstddef>
#include <iostream>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

namespace featherdoc_cli {

auto run_inspect_paragraphs_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 2U) {
        print_parse_error(command, "inspect-paragraphs expects an input path",
                          json_output);
        return 2;
    }

    inspect_paragraphs_options options;
    std::string error_message;
    if (!parse_inspect_paragraphs_options(arguments, 2U, options,
                                          error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    const auto input_path = path_type(std::string(arguments[1]));
    featherdoc::Document doc;
    if (!open_document(input_path, doc, command, options.json_output)) {
        return 1;
    }

    std::vector<inspected_body_paragraph> paragraphs;
    if (!load_body_paragraph_summaries(input_path, paragraphs, error_message)) {
        if (options.json_output) {
            write_json_command_error(std::cerr, command, "inspect",
                                     error_message);
        } else {
            std::cerr << error_message << '\n';
        }
        return 1;
    }

    if (options.paragraph_index.has_value()) {
        if (*options.paragraph_index >= paragraphs.size()) {
            featherdoc::document_error_info error_info{};
            error_info.code = std::make_error_code(std::errc::invalid_argument);
            error_info.detail =
                "paragraph index '" +
                std::to_string(*options.paragraph_index) + "' is out of range";
            error_info.entry_name = "word/document.xml";
            report_operation_failure(command, "inspect",
                                     "paragraph index is out of range",
                                     error_info, options.json_output);
            return 1;
        }

        inspect_paragraph(paragraphs[*options.paragraph_index],
                          options.json_output);
        return 0;
    }

    inspect_paragraphs(paragraphs, options.json_output);
    return 0;
}

auto run_inspect_runs_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 3U) {
        print_parse_error(
            command,
            "inspect-runs expects an input path and a paragraph index",
            json_output);
        return 2;
    }

    std::size_t paragraph_index = 0U;
    if (!parse_index(arguments[2], paragraph_index)) {
        print_parse_error(command,
                          "invalid paragraph index: " +
                              std::string(arguments[2]),
                          json_output);
        return 2;
    }

    inspect_runs_options options;
    std::string error_message;
    if (!parse_inspect_runs_options(arguments, 3U, options, error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    const auto input_path = path_type(std::string(arguments[1]));
    featherdoc::Document doc;
    if (!open_document(input_path, doc, command, options.json_output)) {
        return 1;
    }

    std::vector<inspected_body_run> runs;
    bool paragraph_found = false;
    if (!load_body_run_summaries(input_path, paragraph_index, runs,
                                 paragraph_found, error_message)) {
        if (options.json_output) {
            write_json_command_error(std::cerr, command, "inspect",
                                     error_message);
        } else {
            std::cerr << error_message << '\n';
        }
        return 1;
    }

    if (!paragraph_found) {
        featherdoc::document_error_info error_info{};
        error_info.code = std::make_error_code(std::errc::invalid_argument);
        error_info.detail = "paragraph index '" +
                            std::to_string(paragraph_index) +
                            "' is out of range";
        error_info.entry_name = "word/document.xml";
        report_operation_failure(command, "inspect",
                                 "paragraph index is out of range",
                                 error_info, options.json_output);
        return 1;
    }

    if (options.run_index.has_value()) {
        if (*options.run_index >= runs.size()) {
            featherdoc::document_error_info error_info{};
            error_info.code =
                std::make_error_code(std::errc::invalid_argument);
            error_info.detail =
                "run index '" + std::to_string(*options.run_index) +
                "' is out of range for paragraph index '" +
                std::to_string(paragraph_index) + "'";
            error_info.entry_name = "word/document.xml";
            report_operation_failure(command, "inspect",
                                     "run index is out of range", error_info,
                                     options.json_output);
            return 1;
        }

        inspect_run(paragraph_index, runs[*options.run_index],
                    options.json_output);
        return 0;
    }

    inspect_runs(paragraph_index, runs, options.json_output);
    return 0;
}

} // namespace featherdoc_cli
