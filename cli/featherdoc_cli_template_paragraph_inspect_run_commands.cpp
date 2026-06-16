#include "featherdoc_cli_template_paragraph_inspect_commands_detail.hpp"

#include "featherdoc_cli_command_support.hpp"
#include "featherdoc_cli_errors.hpp"
#include "featherdoc_cli_parse.hpp"
#include "featherdoc_cli_template_inspect_options_parse.hpp"
#include "featherdoc_cli_template_paragraph_inspect_output.hpp"
#include "featherdoc_cli_template_part_selection.hpp"

#include <featherdoc.hpp>

#include <cstddef>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

namespace featherdoc_cli {

auto run_inspect_template_runs_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 3U) {
        print_parse_error(
            command,
            "inspect-template-runs expects an input path and a paragraph index",
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

    template_inspect_runs_options options;
    std::string error_message;
    if (!parse_template_inspect_runs_options(arguments, 3U, options,
                                             error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    featherdoc::Document doc;
    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return 1;
    }

    selected_template_part selected;
    if (!select_template_part(doc, options.part, options.part_index,
                              options.section_index, options.reference_kind,
                              selected, error_message)) {
        report_operation_failure(command, "inspect", error_message,
                                 doc.last_error(), options.json_output);
        return 1;
    }

    const auto paragraph = selected.part.inspect_paragraph(paragraph_index);
    if (!paragraph.has_value()) {
        featherdoc::document_error_info error_info{};
        error_info.code = std::make_error_code(std::errc::invalid_argument);
        error_info.detail =
            "paragraph index '" + std::to_string(paragraph_index) +
            "' is out of range";
        error_info.entry_name = std::string(selected.part.entry_name());
        report_operation_failure(command, "inspect",
                                 "paragraph index is out of range", error_info,
                                 options.json_output);
        return 1;
    }

    if (options.run_index.has_value()) {
        const auto run = selected.part.inspect_paragraph_run(
            paragraph_index, *options.run_index);
        if (!run.has_value()) {
            featherdoc::document_error_info error_info{};
            error_info.code = std::make_error_code(std::errc::invalid_argument);
            error_info.detail =
                "run index '" + std::to_string(*options.run_index) +
                "' is out of range for paragraph index '" +
                std::to_string(paragraph_index) + "'";
            error_info.entry_name = std::string(selected.part.entry_name());
            report_operation_failure(command, "inspect",
                                     "run index is out of range", error_info,
                                     options.json_output);
            return 1;
        }

        inspect_template_run(selected, paragraph_index, *run,
                             options.json_output);
        return 0;
    }

    const auto runs = selected.part.inspect_paragraph_runs(paragraph_index);
    if (const auto &error_info = doc.last_error(); error_info.code) {
        report_document_error(command, "inspect", error_info,
                              options.json_output);
        return 1;
    }

    inspect_template_runs(selected, paragraph_index, runs, options.json_output);
    return 0;
}

} // namespace featherdoc_cli
