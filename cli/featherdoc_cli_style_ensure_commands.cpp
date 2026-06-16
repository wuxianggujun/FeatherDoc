#include "featherdoc_cli_style_ensure_commands.hpp"

#include "featherdoc_cli_command_support.hpp"
#include "featherdoc_cli_errors.hpp"
#include "featherdoc_cli_parse.hpp"
#include "featherdoc_cli_style_ensure_options_parse.hpp"
#include "featherdoc_cli_style_output.hpp"

#include <featherdoc.hpp>

#include <optional>
#include <ostream>
#include <string>
#include <string_view>
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

} // namespace

auto run_ensure_paragraph_style_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 3U) {
        print_parse_error(
            command, "ensure-paragraph-style expects an input path and a style id",
            json_output);
        return 2;
    }

    const auto style_id = arguments[2];
    ensure_paragraph_style_options options;
    std::string error_message;
    if (!parse_ensure_paragraph_style_options(arguments, 3U, options,
                                              error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    featherdoc::Document doc;
    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return 1;
    }

    if (!doc.ensure_paragraph_style(style_id, options.definition)) {
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

auto run_ensure_character_style_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 3U) {
        print_parse_error(
            command, "ensure-character-style expects an input path and a style id",
            json_output);
        return 2;
    }

    const auto style_id = arguments[2];
    ensure_character_style_options options;
    std::string error_message;
    if (!parse_ensure_character_style_options(arguments, 3U, options,
                                              error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    featherdoc::Document doc;
    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return 1;
    }

    if (!doc.ensure_character_style(style_id, options.definition)) {
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

} // namespace featherdoc_cli
