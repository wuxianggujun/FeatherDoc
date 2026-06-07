#include "featherdoc_cli_field_commands.hpp"

#include "featherdoc_cli_command_support.hpp"
#include "featherdoc_cli_errors.hpp"
#include "featherdoc_cli_field_parse.hpp"
#include "featherdoc_cli_json.hpp"
#include "featherdoc_cli_parse.hpp"
#include "featherdoc_cli_section_options_parse.hpp"
#include "featherdoc_cli_template_part_selection.hpp"

#include <cstddef>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace featherdoc_cli {
namespace {

auto append_template_part_field(featherdoc::Document &doc,
                                selected_template_part &selected,
                                std::string_view command,
                                const append_field_options &options,
                                bool json_output) -> bool {
    bool success = false;
    auto field_state = featherdoc::field_state_options{};
    field_state.dirty = options.dirty;
    field_state.locked = options.locked;
    if (command == "append-field") {
        success = selected.part.append_field(
            options.field_argument, options.result_text, field_state);
    } else if (command == "append-page-number-field") {
        success = selected.part.append_page_number_field(field_state);
    } else if (command == "append-total-pages-field") {
        success = selected.part.append_total_pages_field(field_state);
    } else if (command == "append-table-of-contents-field") {
        auto field_options = featherdoc::table_of_contents_field_options{};
        field_options.min_outline_level = options.min_outline_level;
        field_options.max_outline_level = options.max_outline_level;
        field_options.hyperlinks = options.hyperlinks;
        field_options.hide_page_numbers_in_web_layout =
            options.hide_page_numbers_in_web_layout;
        field_options.use_outline_levels = options.use_outline_levels;
        field_options.state = field_state;
        success = selected.part.append_table_of_contents_field(
            field_options, options.result_text);
    } else if (command == "append-reference-field") {
        auto field_options = featherdoc::reference_field_options{};
        field_options.hyperlink = options.hyperlink;
        field_options.preserve_formatting = options.preserve_formatting;
        field_options.state = field_state;
        success = selected.part.append_reference_field(
            options.field_argument, field_options, options.result_text);
    } else if (command == "append-page-reference-field") {
        auto field_options = featherdoc::page_reference_field_options{};
        field_options.hyperlink = options.hyperlink;
        field_options.relative_position = options.relative_position;
        field_options.preserve_formatting = options.preserve_formatting;
        field_options.state = field_state;
        success = selected.part.append_page_reference_field(
            options.field_argument, field_options, options.result_text);
    } else if (command == "append-style-reference-field") {
        auto field_options = featherdoc::style_reference_field_options{};
        field_options.paragraph_number = options.paragraph_number;
        field_options.relative_position = options.relative_position;
        field_options.preserve_formatting = options.preserve_formatting;
        field_options.state = field_state;
        success = selected.part.append_style_reference_field(
            options.field_argument, field_options, options.result_text);
    } else if (command == "append-document-property-field") {
        auto field_options = featherdoc::document_property_field_options{};
        field_options.preserve_formatting = options.preserve_formatting;
        field_options.state = field_state;
        success = selected.part.append_document_property_field(
            options.field_argument, field_options, options.result_text);
    } else if (command == "append-date-field") {
        auto field_options = featherdoc::date_field_options{};
        field_options.format = options.date_format;
        field_options.preserve_formatting = options.preserve_formatting;
        field_options.state = field_state;
        success = selected.part.append_date_field(field_options, options.result_text);
    } else if (command == "append-hyperlink-field") {
        auto field_options = featherdoc::hyperlink_field_options{};
        field_options.anchor = options.anchor;
        field_options.tooltip = options.tooltip;
        field_options.preserve_formatting = options.preserve_formatting;
        field_options.state = field_state;
        success = selected.part.append_hyperlink_field(
            options.field_argument, field_options, options.result_text);
    } else if (command == "append-sequence-field") {
        auto field_options = featherdoc::sequence_field_options{};
        field_options.number_format = options.number_format;
        field_options.restart_value = options.restart_value;
        field_options.preserve_formatting = options.preserve_formatting;
        field_options.state = field_state;
        success = selected.part.append_sequence_field(
            options.field_argument, field_options,
            options.has_result_text ? std::string_view{options.result_text}
                                    : std::string_view{"1"});
    } else if (command == "append-caption") {
        auto field_options = featherdoc::caption_field_options{};
        field_options.number_format = options.number_format;
        field_options.restart_value = options.restart_value;
        field_options.separator = options.separator;
        field_options.preserve_formatting = options.preserve_formatting;
        field_options.state = field_state;
        success = selected.part.append_caption(
            options.field_argument, options.caption_text, field_options,
            options.number_result_text);
    } else if (command == "append-index-field") {
        auto field_options = featherdoc::index_field_options{};
        field_options.columns = options.columns;
        field_options.preserve_formatting = options.preserve_formatting;
        field_options.state = field_state;
        success = selected.part.append_index_field(field_options,
                                                   options.result_text);
    } else if (command == "append-index-entry-field") {
        auto field_options = featherdoc::index_entry_field_options{};
        field_options.subentry = options.subentry;
        field_options.bookmark_name = options.bookmark_name;
        field_options.cross_reference = options.cross_reference;
        field_options.bold_page_number = options.bold_page_number;
        field_options.italic_page_number = options.italic_page_number;
        field_options.state = field_state;
        success = selected.part.append_index_entry_field(
            options.field_argument, field_options, options.result_text);
    } else if (command == "append-complex-field") {
        auto field_options = featherdoc::complex_field_options{};
        field_options.state = field_state;
        if (options.instruction.has_value()) {
            success = selected.part.append_complex_field(
                *options.instruction, options.result_text, field_options);
        } else {
            success = selected.part.append_complex_field(
                {featherdoc::complex_field_text_fragment(
                     *options.instruction_before),
                 featherdoc::complex_field_nested_fragment(
                     *options.nested_instruction, options.nested_result_text),
                 featherdoc::complex_field_text_fragment(
                     *options.instruction_after)},
                options.result_text, field_options);
        }
    } else if (command == "replace-field") {
        success = selected.part.replace_field(
            *options.field_index, options.field_argument, options.result_text);
    }

    if (!success) {
        return report_document_error(command, "mutate", doc.last_error(), json_output);
    }

    return true;
}

void write_json_field_command_result(
    std::ostream &stream, const selected_template_part &selected,
    const append_field_options &options, std::string_view command) {
    stream << ',';
    write_json_selected_template_part(stream, selected);
    stream << ",\"field\":";
    write_json_string(stream, template_field_name(command));
    if (options.field_index.has_value()) {
        stream << ",\"field_index\":" << *options.field_index;
    }
    if (options.has_field_argument) {
        stream << ",\"field_argument\":";
        write_json_string(stream, options.field_argument);
    }
    if (options.has_result_text) {
        stream << ",\"result_text\":";
        write_json_string(stream, options.result_text);
    }
    if (command == "append-table-of-contents-field") {
        stream << ",\"min_outline_level\":" << options.min_outline_level;
        stream << ",\"max_outline_level\":" << options.max_outline_level;
        stream << ",\"hyperlinks\":" << json_bool(options.hyperlinks);
        stream << ",\"hide_page_numbers_in_web_layout\":"
               << json_bool(options.hide_page_numbers_in_web_layout);
        stream << ",\"use_outline_levels\":"
               << json_bool(options.use_outline_levels);
    }
    if (options.has_caption_text) {
        stream << ",\"caption_text\":";
        write_json_string(stream, options.caption_text);
    }
    if (options.has_number_result_text) {
        stream << ",\"number_result_text\":";
        write_json_string(stream, options.number_result_text);
    }
    if (options.columns.has_value()) {
        stream << ",\"columns\":" << *options.columns;
    }
    if (options.restart_value.has_value()) {
        stream << ",\"restart\":" << *options.restart_value;
    }
    if (options.subentry.has_value()) {
        stream << ",\"subentry\":";
        write_json_string(stream, *options.subentry);
    }
    if (options.bookmark_name.has_value()) {
        stream << ",\"bookmark\":";
        write_json_string(stream, *options.bookmark_name);
    }
    if (options.cross_reference.has_value()) {
        stream << ",\"cross_reference\":";
        write_json_string(stream, *options.cross_reference);
    }
    if (options.instruction.has_value()) {
        stream << ",\"instruction\":";
        write_json_string(stream, *options.instruction);
    }
    if (options.instruction_before.has_value()) {
        stream << ",\"instruction_before\":";
        write_json_string(stream, *options.instruction_before);
    }
    if (options.nested_instruction.has_value()) {
        stream << ",\"nested_instruction\":";
        write_json_string(stream, *options.nested_instruction);
    }
    if (!options.nested_result_text.empty()) {
        stream << ",\"nested_result_text\":";
        write_json_string(stream, options.nested_result_text);
    }
    if (options.instruction_after.has_value()) {
        stream << ",\"instruction_after\":";
        write_json_string(stream, *options.instruction_after);
    }
}

} // namespace

auto is_field_command(std::string_view command) -> bool {
    return is_append_field_command(command) ||
           command == "inspect-update-fields-on-open" ||
           command == "set-update-fields-on-open";
}

auto run_field_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    if (command == "inspect-update-fields-on-open") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 2U || arguments[1].starts_with("--")) {
            print_parse_error(command,
                              "inspect-update-fields-on-open expects an input path",
                              json_output);
            return 2;
        }
        for (std::size_t index = 2U; index < arguments.size(); ++index) {
            if (arguments[index] != "--json") {
                print_parse_error(command,
                                  "unknown option: " + std::string(arguments[index]),
                                  json_output);
                return 2;
            }
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           json_output)) {
            return 1;
        }

        const auto enabled = doc.update_fields_on_open_enabled();
        if (!enabled.has_value()) {
            report_document_error(command, "inspect", doc.last_error(), json_output);
            return 1;
        }

        if (json_output) {
            std::cout << "{\"command\":\"inspect-update-fields-on-open\","
                      << "\"ok\":true,\"update_fields_on_open\":"
                      << json_bool(*enabled) << "}\n";
        } else {
            std::cout << "update_fields_on_open: " << (*enabled ? "true" : "false")
                      << '\n';
        }
        return 0;
    }

    if (command == "set-update-fields-on-open") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 2U || arguments[1].starts_with("--")) {
            print_parse_error(command,
                              "set-update-fields-on-open expects an input path",
                              json_output);
            return 2;
        }

        set_update_fields_on_open_options options;
        std::string error_message;
        if (!parse_set_update_fields_on_open_options(arguments, 2U, options,
                                                     error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        const auto mutated = *options.enabled ? doc.enable_update_fields_on_open()
                                             : doc.clear_update_fields_on_open();
        if (!mutated) {
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
                [&options](std::ostream &stream) {
                    stream << ",\"update_fields_on_open\":"
                           << json_bool(*options.enabled);
                });
        }
        return 0;
    }

    if (is_append_field_command(command)) {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 3U) {
            print_parse_error(
                command,
                std::string(command) + " expects an input path and field target options",
                json_output);
            return 2;
        }

        append_field_options options;
        std::string error_message;
        if (!parse_append_field_options(command, arguments, 2U, options,
                                        error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        selected_template_part selected;
        if (!select_mutable_template_part(doc, options.part, options.part_index,
                                          options.section_index,
                                          options.reference_kind, selected,
                                          error_message)) {
            report_operation_failure(command, "mutate", error_message,
                                     doc.last_error(), options.json_output);
            return 1;
        }

        if (!append_template_part_field(doc, selected, command, options,
                                        options.json_output)) {
            return 1;
        }

        if (!save_document(doc, options.output_path, command, options.json_output)) {
            return 1;
        }

        if (options.json_output) {
            write_json_mutation_result(
                command, doc, options.output_path,
                [&selected, &options, command](std::ostream &stream) {
                    write_json_field_command_result(stream, selected, options, command);
                });
        }

        return 0;
    }

    return 2;
}

} // namespace featherdoc_cli
