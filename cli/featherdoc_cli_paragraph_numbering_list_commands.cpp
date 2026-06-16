#include "featherdoc_cli_paragraph_numbering_commands.hpp"

#include "featherdoc_cli_command_support.hpp"
#include "featherdoc_cli_domain_names.hpp"
#include "featherdoc_cli_errors.hpp"
#include "featherdoc_cli_json.hpp"
#include "featherdoc_cli_numbering_options_parse.hpp"
#include "featherdoc_cli_paragraph_list_options_parse.hpp"
#include "featherdoc_cli_paragraph_numbering_commands_support.hpp"
#include "featherdoc_cli_parse.hpp"

#include <featherdoc.hpp>

#include <cstdint>
#include <ostream>
#include <string>
#include <string_view>
#include <vector>

namespace featherdoc_cli {

auto run_set_paragraph_numbering_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 3U) {
        print_parse_error(
            command,
            "set-paragraph-numbering expects an input path and a paragraph index",
            json_output);
        return 2;
    }

    std::uint32_t paragraph_index = 0U;
    if (!parse_paragraph_numbering_index_argument(command, arguments,
                                                 paragraph_index, json_output)) {
        return 2;
    }

    set_paragraph_numbering_options options;
    std::string error_message;
    if (!parse_set_paragraph_numbering_options(arguments, 3U, options,
                                               error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    featherdoc::Document doc;
    if (!open_paragraph_numbering_document_for_command(
            command, arguments, doc, options.json_output)) {
        return 1;
    }

    featherdoc::Paragraph paragraph;
    if (!resolve_paragraph_numbering_body_paragraph(
            command, doc, paragraph_index, paragraph, options.json_output)) {
        return 1;
    }

    const auto level = options.level.value_or(0U);
    if (!doc.set_paragraph_numbering(paragraph, *options.definition_id, level)) {
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
            [paragraph_index, level, &options](std::ostream &stream) {
                stream << ",\"paragraph_index\":" << paragraph_index
                       << ",\"definition_id\":" << *options.definition_id
                       << ",\"level\":" << level;
            });
    }

    return 0;
}

auto run_set_paragraph_list_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 3U) {
        print_parse_error(
            command,
            "set-paragraph-list expects an input path and a paragraph index",
            json_output);
        return 2;
    }

    std::uint32_t paragraph_index = 0U;
    if (!parse_paragraph_numbering_index_argument(command, arguments,
                                                 paragraph_index, json_output)) {
        return 2;
    }

    paragraph_list_options options;
    std::string error_message;
    if (!parse_paragraph_list_options(arguments, 3U, options, error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    featherdoc::Document doc;
    if (!open_paragraph_numbering_document_for_command(
            command, arguments, doc, options.json_output)) {
        return 1;
    }

    featherdoc::Paragraph paragraph;
    if (!resolve_paragraph_numbering_body_paragraph(
            command, doc, paragraph_index, paragraph, options.json_output)) {
        return 1;
    }

    const auto level = options.level.value_or(0U);
    if (!doc.set_paragraph_list(paragraph, options.kind, level)) {
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
            [paragraph_index, level, &options](std::ostream &stream) {
                stream << ",\"paragraph_index\":" << paragraph_index
                       << ",\"kind\":";
                write_json_string(stream, list_kind_name(options.kind));
                stream << ",\"level\":" << level;
            });
    }

    return 0;
}

auto run_restart_paragraph_list_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 3U) {
        print_parse_error(
            command,
            "restart-paragraph-list expects an input path and a paragraph index",
            json_output);
        return 2;
    }

    std::uint32_t paragraph_index = 0U;
    if (!parse_paragraph_numbering_index_argument(command, arguments,
                                                 paragraph_index, json_output)) {
        return 2;
    }

    paragraph_list_options options;
    std::string error_message;
    if (!parse_paragraph_list_options(arguments, 3U, options, error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    featherdoc::Document doc;
    if (!open_paragraph_numbering_document_for_command(
            command, arguments, doc, options.json_output)) {
        return 1;
    }

    featherdoc::Paragraph paragraph;
    if (!resolve_paragraph_numbering_body_paragraph(
            command, doc, paragraph_index, paragraph, options.json_output)) {
        return 1;
    }

    const auto level = options.level.value_or(0U);
    if (!doc.restart_paragraph_list(paragraph, options.kind, level)) {
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
            [paragraph_index, level, &options](std::ostream &stream) {
                stream << ",\"paragraph_index\":" << paragraph_index
                       << ",\"kind\":";
                write_json_string(stream, list_kind_name(options.kind));
                stream << ",\"level\":" << level;
            });
    }

    return 0;
}

auto run_clear_paragraph_list_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 3U) {
        print_parse_error(
            command,
            "clear-paragraph-list expects an input path and a paragraph index",
            json_output);
        return 2;
    }

    std::uint32_t paragraph_index = 0U;
    if (!parse_paragraph_numbering_index_argument(command, arguments,
                                                 paragraph_index, json_output)) {
        return 2;
    }

    clear_paragraph_list_options options;
    std::string error_message;
    if (!parse_clear_paragraph_list_options(arguments, 3U, options,
                                            error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    featherdoc::Document doc;
    if (!open_paragraph_numbering_document_for_command(
            command, arguments, doc, options.json_output)) {
        return 1;
    }

    featherdoc::Paragraph paragraph;
    if (!resolve_paragraph_numbering_body_paragraph(
            command, doc, paragraph_index, paragraph, options.json_output)) {
        return 1;
    }

    if (!doc.clear_paragraph_list(paragraph)) {
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
