#include "featherdoc_cli_paragraph_numbering_commands.hpp"

#include "featherdoc_cli_command_support.hpp"
#include "featherdoc_cli_errors.hpp"
#include "featherdoc_cli_json.hpp"
#include "featherdoc_cli_numbering_options_parse.hpp"
#include "featherdoc_cli_paragraph_numbering_commands_support.hpp"
#include "featherdoc_cli_parse.hpp"

#include <featherdoc.hpp>

#include <ostream>
#include <string>
#include <string_view>
#include <vector>

namespace featherdoc_cli {

auto run_ensure_style_linked_numbering_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 2U) {
        print_parse_error(command,
                          "ensure-style-linked-numbering expects an input path",
                          json_output);
        return 2;
    }

    ensure_style_linked_numbering_options options;
    std::string error_message;
    if (!parse_ensure_style_linked_numbering_options(arguments, 2U, options,
                                                     error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    featherdoc::Document doc;
    if (!open_paragraph_numbering_document_for_command(
            command, arguments, doc, options.json_output)) {
        return 1;
    }

    featherdoc::numbering_definition definition;
    definition.name = *options.definition_name;
    definition.levels = options.levels;

    const auto definition_id =
        doc.ensure_style_linked_numbering(definition, options.style_links);
    if (!definition_id.has_value()) {
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
            [definition_id, &definition, &options](std::ostream &stream) {
                stream << ",\"definition_id\":" << *definition_id
                       << ",\"definition_name\":";
                write_json_string(stream, definition.name);
                stream << ",\"definition_levels\":";
                write_json_numbering_definition_levels(stream, definition.levels);
                stream << ",\"style_links\":";
                write_json_paragraph_style_numbering_links(stream,
                                                           options.style_links);
            });
    }

    return 0;
}

auto run_set_paragraph_style_numbering_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 3U) {
        print_parse_error(
            command,
            "set-paragraph-style-numbering expects an input path and a style id",
            json_output);
        return 2;
    }

    const auto style_id = arguments[2];
    set_paragraph_style_numbering_options options;
    std::string error_message;
    if (!parse_set_paragraph_style_numbering_options(arguments, 3U, options,
                                                     error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    featherdoc::Document doc;
    if (!open_paragraph_numbering_document_for_command(
            command, arguments, doc, options.json_output)) {
        return 1;
    }

    featherdoc::numbering_definition definition;
    definition.name = *options.definition_name;
    definition.levels = options.levels;

    const auto numbering_id = doc.ensure_numbering_definition(definition);
    if (!numbering_id.has_value()) {
        report_document_error(command, "mutate", doc.last_error(),
                              options.json_output);
        return 1;
    }

    const auto style_level = options.style_level.value_or(0U);
    if (!doc.set_paragraph_style_numbering(style_id, *numbering_id, style_level)) {
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
            [style_id, style_level, &definition](std::ostream &stream) {
                stream << ",\"style_id\":";
                write_json_string(stream, style_id);
                stream << ",\"definition_name\":";
                write_json_string(stream, definition.name);
                stream << ",\"style_level\":" << style_level
                       << ",\"definition_levels\":";
                write_json_numbering_definition_levels(stream, definition.levels);
            });
    }

    return 0;
}

auto run_clear_paragraph_style_numbering_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 3U) {
        print_parse_error(
            command,
            "clear-paragraph-style-numbering expects an input path and a style id",
            json_output);
        return 2;
    }

    const auto style_id = arguments[2];
    clear_paragraph_style_numbering_options options;
    std::string error_message;
    if (!parse_clear_paragraph_style_numbering_options(arguments, 3U, options,
                                                       error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    featherdoc::Document doc;
    if (!open_paragraph_numbering_document_for_command(
            command, arguments, doc, options.json_output)) {
        return 1;
    }

    if (!doc.clear_paragraph_style_numbering(style_id)) {
        report_document_error(command, "mutate", doc.last_error(),
                              options.json_output);
        return 1;
    }

    if (!save_document(doc, options.output_path, command, options.json_output)) {
        return 1;
    }

    if (options.json_output) {
        write_json_mutation_result(command, doc, options.output_path,
                                   [style_id](std::ostream &stream) {
                                       stream << ",\"style_id\":";
                                       write_json_string(stream, style_id);
                                   });
    }

    return 0;
}

} // namespace featherdoc_cli
