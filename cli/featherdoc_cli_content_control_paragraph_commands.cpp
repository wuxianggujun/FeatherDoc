#include "featherdoc_cli_content_control_paragraph_commands.hpp"

#include "featherdoc_cli_bookmark_text_options_parse.hpp"
#include "featherdoc_cli_command_support.hpp"
#include "featherdoc_cli_content_control_mutation_output.hpp"
#include "featherdoc_cli_errors.hpp"
#include "featherdoc_cli_json.hpp"
#include "featherdoc_cli_parse.hpp"
#include "featherdoc_cli_template_part_selection.hpp"

#include <iostream>
#include <ostream>
#include <string>
#include <utility>
#include <vector>

namespace featherdoc_cli {
namespace {

auto resolve_text_sources(const std::vector<cli_text_source_options> &sources,
                          std::vector<std::string> &texts,
                          std::string &error_message) -> bool {
    texts.clear();
    texts.reserve(sources.size());

    for (const auto &source : sources) {
        std::string text;
        if (!read_text_source(source, text, error_message)) {
            return false;
        }
        texts.push_back(std::move(text));
    }

    return true;
}

} // namespace

auto run_replace_content_control_paragraphs_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 2U) {
        print_parse_error(
            command, "replace-content-control-paragraphs expects an input path",
            json_output);
        return 2;
    }

    replace_content_control_paragraphs_options options;
    std::string error_message;
    if (!parse_replace_content_control_paragraphs_options(
            arguments, 2U, options, error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    std::vector<std::string> paragraphs;
    if (!resolve_text_sources(options.paragraph_sources, paragraphs,
                              error_message)) {
        if (options.json_output) {
            write_json_command_error(std::cerr, command, "input",
                                     error_message);
        } else {
            std::cerr << error_message << '\n';
        }
        return 1;
    }

    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return 1;
    }

    selected_template_part selected;
    if (!select_template_part(doc, options.part, options.part_index,
                              options.section_index, options.reference_kind,
                              selected, error_message)) {
        report_operation_failure(command, "mutate", error_message,
                                 doc.last_error(), options.json_output);
        return 1;
    }

    const auto replaced =
        options.tag.has_value()
            ? selected.part.replace_content_control_with_paragraphs_by_tag(
                  *options.tag, paragraphs)
            : selected.part.replace_content_control_with_paragraphs_by_alias(
                  *options.alias, paragraphs);
    if (replaced == 0U) {
        if (const auto &error_info = doc.last_error(); error_info.code) {
            report_document_error(command, "mutate", error_info,
                                  options.json_output);
        } else {
            report_operation_failure(command, "mutate",
                                     "matching content control not found",
                                     doc.last_error(), options.json_output);
        }
        return 1;
    }

    if (!save_document(doc, options.output_path, command, options.json_output)) {
        return 1;
    }

    if (options.json_output) {
        write_json_mutation_result(
            command, doc, options.output_path,
            [&selected, &options, &paragraphs, replaced](std::ostream &stream) {
                write_json_content_control_paragraphs_result(
                    stream, selected, options, paragraphs, replaced);
            });
    } else {
        print_content_control_paragraphs_result(selected, options, paragraphs,
                                                replaced);
    }

    return 0;
}

} // namespace featherdoc_cli
