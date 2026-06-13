#include "featherdoc_cli_paragraph_run_commands.hpp"

#include "featherdoc_cli_command_support.hpp"
#include "featherdoc_cli_errors.hpp"
#include "featherdoc_cli_json.hpp"
#include "featherdoc_cli_paragraph_run_commands_detail.hpp"
#include "featherdoc_cli_paragraph_run_options_parse.hpp"
#include "featherdoc_cli_parse.hpp"

#include <featherdoc.hpp>

#include <ostream>
#include <string>
#include <string_view>
#include <vector>

namespace featherdoc_cli {

auto run_set_run_language_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int {
    const auto json_output = has_json_flag(arguments);
    body_run_target target;
    if (!parse_body_run_target(
            command, arguments,
            "set-run-language expects an input path, a paragraph index, a run index, and a language",
            target, json_output)) {
        return 2;
    }

    if (arguments.size() < 5U) {
        print_parse_error(
            command,
            "set-run-language expects an input path, a paragraph index, a run index, and a language",
            json_output);
        return 2;
    }

    const auto language = arguments[4];
    set_run_language_options options;
    std::string error_message;
    if (!parse_set_run_language_options(arguments, 5U, options,
                                        error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    featherdoc::Document doc;
    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return 1;
    }

    featherdoc::Run run;
    if (!resolve_body_run_for_command(command, doc, target, run,
                                      options.json_output)) {
        return 1;
    }

    if (!run.set_language(language)) {
        report_body_paragraph_run_failure(
            command, "run language must not be empty",
            "run language must not be empty", options.json_output);
        return 1;
    }

    if (!save_document(doc, options.output_path, command, options.json_output)) {
        return 1;
    }

    if (options.json_output) {
        write_json_mutation_result(
            command, doc, options.output_path,
            [target, language](std::ostream &stream) {
                stream << ",\"paragraph_index\":" << target.paragraph_index
                       << ",\"run_index\":" << target.run_index
                       << ",\"language\":";
                write_json_string(stream, language);
            });
    }

    return 0;
}

auto run_clear_run_language_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int {
    const auto json_output = has_json_flag(arguments);
    body_run_target target;
    if (!parse_body_run_target(
            command, arguments,
            "clear-run-language expects an input path, a paragraph index, and a run index",
            target, json_output)) {
        return 2;
    }

    clear_run_language_options options;
    std::string error_message;
    if (!parse_clear_run_language_options(arguments, 4U, options,
                                          error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    featherdoc::Document doc;
    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return 1;
    }

    featherdoc::Run run;
    if (!resolve_body_run_for_command(command, doc, target, run,
                                      options.json_output)) {
        return 1;
    }

    if (!run.clear_language()) {
        report_body_paragraph_run_failure(command,
                                          "target run handle is not valid",
                                          "target run handle is not valid",
                                          options.json_output);
        return 1;
    }

    if (!save_document(doc, options.output_path, command, options.json_output)) {
        return 1;
    }

    if (options.json_output) {
        write_json_mutation_result(
            command, doc, options.output_path,
            [target](std::ostream &stream) {
                stream << ",\"paragraph_index\":" << target.paragraph_index
                       << ",\"run_index\":" << target.run_index;
            });
    }

    return 0;
}

} // namespace featherdoc_cli
