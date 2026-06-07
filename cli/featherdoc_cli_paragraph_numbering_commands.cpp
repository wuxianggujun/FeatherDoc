#include "featherdoc_cli_paragraph_numbering_commands.hpp"

#include "featherdoc_cli_command_support.hpp"
#include "featherdoc_cli_domain_names.hpp"
#include "featherdoc_cli_errors.hpp"
#include "featherdoc_cli_json.hpp"
#include "featherdoc_cli_numbering_json.hpp"
#include "featherdoc_cli_numbering_options_parse.hpp"
#include "featherdoc_cli_paragraph_list_options_parse.hpp"
#include "featherdoc_cli_parse.hpp"

#include <featherdoc.hpp>

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <ostream>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

namespace featherdoc_cli {
namespace {

auto parse_paragraph_index_argument(
    std::string_view command, const std::vector<std::string_view> &arguments,
    std::uint32_t &paragraph_index, bool json_output) -> bool {
    if (parse_uint32(arguments[2], paragraph_index)) {
        return true;
    }

    print_parse_error(command,
                      "invalid paragraph index: " +
                          std::string(arguments[2]),
                      json_output);
    return false;
}

auto resolve_body_paragraph_for_command(std::string_view command,
                                        featherdoc::Document &doc,
                                        std::uint32_t paragraph_index,
                                        featherdoc::Paragraph &paragraph,
                                        bool json_output) -> bool {
    paragraph = doc.paragraphs();
    for (std::uint32_t current_index = 0U;
         current_index < paragraph_index && paragraph.has_next();
         ++current_index) {
        paragraph.next();
    }

    if (paragraph.has_next()) {
        return true;
    }

    featherdoc::document_error_info error_info{};
    error_info.code = std::make_error_code(std::errc::invalid_argument);
    error_info.detail =
        "paragraph index '" + std::to_string(paragraph_index) + "' is out of range";
    error_info.entry_name = "word/document.xml";
    return report_operation_failure(command, "mutate",
                                    "paragraph index is out of range", error_info,
                                    json_output);
}

void write_json_numbering_definition_levels(
    std::ostream &stream,
    const std::vector<featherdoc::numbering_level_definition> &levels) {
    stream << '[';
    for (std::size_t index = 0U; index < levels.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        write_json_numbering_level_definition(stream, levels[index]);
    }
    stream << ']';
}

void write_json_paragraph_style_numbering_links(
    std::ostream &stream,
    const std::vector<featherdoc::paragraph_style_numbering_link> &style_links) {
    stream << '[';
    for (std::size_t index = 0U; index < style_links.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        write_json_paragraph_style_numbering_link(stream, style_links[index]);
    }
    stream << ']';
}

auto open_document_for_command(std::string_view command,
                               const std::vector<std::string_view> &arguments,
                               featherdoc::Document &doc,
                               bool json_output) -> bool {
    return open_document(path_type(std::string(arguments[1])), doc, command,
                         json_output);
}

} // namespace

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
    if (!open_document_for_command(command, arguments, doc, options.json_output)) {
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
    if (!open_document_for_command(command, arguments, doc, options.json_output)) {
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
    if (!open_document_for_command(command, arguments, doc, options.json_output)) {
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

auto run_ensure_numbering_definition_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 2U) {
        print_parse_error(command,
                          "ensure-numbering-definition expects an input path",
                          json_output);
        return 2;
    }

    ensure_numbering_definition_options options;
    std::string error_message;
    if (!parse_ensure_numbering_definition_options(arguments, 2U, options,
                                                   error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    featherdoc::Document doc;
    if (!open_document_for_command(command, arguments, doc, options.json_output)) {
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

    if (!save_document(doc, options.output_path, command, options.json_output)) {
        return 1;
    }

    if (options.json_output) {
        write_json_mutation_result(
            command, doc, options.output_path,
            [numbering_id, &definition](std::ostream &stream) {
                stream << ",\"definition_id\":" << *numbering_id
                       << ",\"definition_name\":";
                write_json_string(stream, definition.name);
                stream << ",\"definition_levels\":";
                write_json_numbering_definition_levels(stream, definition.levels);
            });
    }

    return 0;
}

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
    if (!parse_paragraph_index_argument(command, arguments, paragraph_index,
                                        json_output)) {
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
    if (!open_document_for_command(command, arguments, doc, options.json_output)) {
        return 1;
    }

    featherdoc::Paragraph paragraph;
    if (!resolve_body_paragraph_for_command(command, doc, paragraph_index,
                                            paragraph, options.json_output)) {
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
    if (!parse_paragraph_index_argument(command, arguments, paragraph_index,
                                        json_output)) {
        return 2;
    }

    paragraph_list_options options;
    std::string error_message;
    if (!parse_paragraph_list_options(arguments, 3U, options, error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    featherdoc::Document doc;
    if (!open_document_for_command(command, arguments, doc, options.json_output)) {
        return 1;
    }

    featherdoc::Paragraph paragraph;
    if (!resolve_body_paragraph_for_command(command, doc, paragraph_index,
                                            paragraph, options.json_output)) {
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
    if (!parse_paragraph_index_argument(command, arguments, paragraph_index,
                                        json_output)) {
        return 2;
    }

    paragraph_list_options options;
    std::string error_message;
    if (!parse_paragraph_list_options(arguments, 3U, options, error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    featherdoc::Document doc;
    if (!open_document_for_command(command, arguments, doc, options.json_output)) {
        return 1;
    }

    featherdoc::Paragraph paragraph;
    if (!resolve_body_paragraph_for_command(command, doc, paragraph_index,
                                            paragraph, options.json_output)) {
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
    if (!parse_paragraph_index_argument(command, arguments, paragraph_index,
                                        json_output)) {
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
    if (!open_document_for_command(command, arguments, doc, options.json_output)) {
        return 1;
    }

    featherdoc::Paragraph paragraph;
    if (!resolve_body_paragraph_for_command(command, doc, paragraph_index,
                                            paragraph, options.json_output)) {
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
