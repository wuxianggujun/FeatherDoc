#include "featherdoc_cli_paragraph_run_commands.hpp"

#include "featherdoc_cli_command_support.hpp"
#include "featherdoc_cli_errors.hpp"
#include "featherdoc_cli_json.hpp"
#include "featherdoc_cli_paragraph_run_commands_detail.hpp"
#include "featherdoc_cli_paragraph_run_options_parse.hpp"
#include "featherdoc_cli_parse.hpp"

#include <featherdoc.hpp>

#include <cstdint>
#include <ostream>
#include <string>
#include <string_view>
#include <vector>

namespace featherdoc_cli {

auto run_set_paragraph_style_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 4U) {
        print_parse_error(
            command,
            "set-paragraph-style expects an input path, a paragraph index, and a style id",
            json_output);
        return 2;
    }

    std::uint32_t paragraph_index = 0U;
    if (!parse_body_paragraph_index(command, arguments, 2U, paragraph_index,
                                    json_output)) {
        return 2;
    }

    const auto style_id = arguments[3];
    set_paragraph_style_options options;
    std::string error_message;
    if (!parse_set_paragraph_style_options(arguments, 4U, options,
                                           error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    featherdoc::Document doc;
    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return 1;
    }

    featherdoc::Paragraph paragraph;
    if (!resolve_body_paragraph_for_command(command, doc, paragraph_index,
                                            paragraph, options.json_output)) {
        return 1;
    }

    if (!doc.set_paragraph_style(paragraph, style_id)) {
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
            [paragraph_index, style_id](std::ostream &stream) {
                stream << ",\"paragraph_index\":" << paragraph_index
                       << ",\"style_id\":";
                write_json_string(stream, style_id);
            });
    }

    return 0;
}

auto run_clear_paragraph_style_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 3U) {
        print_parse_error(
            command,
            "clear-paragraph-style expects an input path and a paragraph index",
            json_output);
        return 2;
    }

    std::uint32_t paragraph_index = 0U;
    if (!parse_body_paragraph_index(command, arguments, 2U, paragraph_index,
                                    json_output)) {
        return 2;
    }

    clear_paragraph_style_options options;
    std::string error_message;
    if (!parse_clear_paragraph_style_options(arguments, 3U, options,
                                             error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    featherdoc::Document doc;
    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return 1;
    }

    featherdoc::Paragraph paragraph;
    if (!resolve_body_paragraph_for_command(command, doc, paragraph_index,
                                            paragraph, options.json_output)) {
        return 1;
    }

    if (!doc.clear_paragraph_style(paragraph)) {
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
            [paragraph_index](std::ostream &stream) {
                stream << ",\"paragraph_index\":" << paragraph_index;
            });
    }

    return 0;
}

} // namespace featherdoc_cli
