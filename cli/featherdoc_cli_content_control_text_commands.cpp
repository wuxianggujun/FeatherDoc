#include "featherdoc_cli_content_control_text_commands.hpp"

#include "featherdoc_cli_bookmark_text_options_parse.hpp"
#include "featherdoc_cli_command_support.hpp"
#include "featherdoc_cli_content_control_mutation_output.hpp"
#include "featherdoc_cli_errors.hpp"
#include "featherdoc_cli_parse.hpp"
#include "featherdoc_cli_template_part_selection.hpp"

#include <ostream>
#include <string>
#include <vector>

namespace featherdoc_cli {

auto run_replace_content_control_text_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 2U) {
        print_parse_error(command,
                          "replace-content-control-text expects an input path",
                          json_output);
        return 2;
    }

    replace_content_control_text_options options;
    std::string error_message;
    if (!parse_replace_content_control_text_options(arguments, 2U, options,
                                                    error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
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
            ? selected.part.replace_content_control_text_by_tag(*options.tag,
                                                                 options.text)
            : selected.part.replace_content_control_text_by_alias(*options.alias,
                                                                   options.text);
    if (const auto &error_info = doc.last_error(); error_info.code) {
        report_document_error(command, "mutate", error_info,
                              options.json_output);
        return 1;
    }
    if (replaced == 0U) {
        report_operation_failure(command, "mutate",
                                 "matching content control not found",
                                 doc.last_error(), options.json_output);
        return 1;
    }

    if (!save_document(doc, options.output_path, command, options.json_output)) {
        return 1;
    }

    if (options.json_output) {
        write_json_mutation_result(
            command, doc, options.output_path,
            [&selected, &options, replaced](std::ostream &stream) {
                write_json_content_control_text_result(stream, selected, options,
                                                       replaced);
            });
    } else {
        print_content_control_text_result(selected, options, options.output_path,
                                          replaced);
    }

    return 0;
}

auto run_set_content_control_form_state_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 2U) {
        print_parse_error(command,
                          "set-content-control-form-state expects an input path",
                          json_output);
        return 2;
    }

    set_content_control_form_state_options options;
    std::string error_message;
    if (!parse_set_content_control_form_state_options(arguments, 2U, options,
                                                      error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
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

    const auto updated =
        options.tag.has_value()
            ? selected.part.set_content_control_form_state_by_tag(*options.tag,
                                                                   options.state)
            : selected.part.set_content_control_form_state_by_alias(
                  *options.alias, options.state);
    if (const auto &error_info = doc.last_error(); error_info.code) {
        report_document_error(command, "mutate", error_info,
                              options.json_output);
        return 1;
    }
    if (updated == 0U) {
        report_operation_failure(command, "mutate",
                                 "matching content control not found",
                                 doc.last_error(), options.json_output);
        return 1;
    }

    if (!save_document(doc, options.output_path, command, options.json_output)) {
        return 1;
    }

    if (options.json_output) {
        write_json_mutation_result(
            command, doc, options.output_path,
            [&selected, &options, updated](std::ostream &stream) {
                write_json_content_control_form_state_result(
                    stream, selected, options, updated);
            });
    } else {
        print_content_control_form_state_result(selected, options, updated);
    }

    return 0;
}

} // namespace featherdoc_cli
