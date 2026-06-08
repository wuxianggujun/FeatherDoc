#include "featherdoc_cli_section_management_commands.hpp"

#include "featherdoc_cli_command_support.hpp"
#include "featherdoc_cli_errors.hpp"
#include "featherdoc_cli_json.hpp"
#include "featherdoc_cli_parse.hpp"
#include "featherdoc_cli_section_part_support.hpp"

#include <cstddef>
#include <ostream>
#include <string>
#include <string_view>
#include <vector>

namespace featherdoc_cli {
namespace {

[[nodiscard]] auto run_insert_section_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 3U) {
        print_parse_error(command,
                          "insert-section expects an input path and a section index",
                          json_output);
        return 2;
    }

    std::size_t section_index = 0;
    if (!parse_index(arguments[2], section_index)) {
        print_parse_error(command,
                          "invalid section index: " + std::string(arguments[2]),
                          json_output);
        return 2;
    }

    section_part_command_options options;
    std::string error_message;
    if (!parse_section_part_command_options(arguments, 3U, true, options,
                                            error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return 1;
    }

    if (!doc.insert_section(section_index, options.inherit_header_footer)) {
        report_document_error(command, "mutate", doc.last_error(),
                              options.json_output);
        return 1;
    }

    if (!save_document(doc, options.output_path, command, options.json_output)) {
        return 1;
    }

    if (options.json_output) {
        const auto inserted_section = section_index + 1U;
        write_json_mutation_result(
            command, doc, options.output_path,
            [inserted_section, &options](std::ostream &stream) {
                stream << ",\"section\":" << inserted_section
                       << ",\"inherit_header_footer\":"
                       << json_bool(options.inherit_header_footer);
            });
    }

    return 0;
}

[[nodiscard]] auto run_remove_section_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 3U) {
        print_parse_error(command,
                          "remove-section expects an input path and a section index",
                          json_output);
        return 2;
    }

    std::size_t section_index = 0;
    if (!parse_index(arguments[2], section_index)) {
        print_parse_error(command,
                          "invalid section index: " + std::string(arguments[2]),
                          json_output);
        return 2;
    }

    section_part_command_options options;
    std::string error_message;
    if (!parse_section_part_command_options(arguments, 3U, false, options,
                                            error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return 1;
    }

    if (!doc.remove_section(section_index)) {
        report_document_error(command, "mutate", doc.last_error(),
                              options.json_output);
        return 1;
    }

    if (!save_document(doc, options.output_path, command, options.json_output)) {
        return 1;
    }

    if (options.json_output) {
        write_json_mutation_result(command, doc, options.output_path,
                                   [section_index](std::ostream &stream) {
                                       stream << ",\"section\":"
                                              << section_index;
                                   });
    }

    return 0;
}

[[nodiscard]] auto run_move_section_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 4U) {
        print_parse_error(command,
                          "move-section expects an input path, a source index, "
                          "and a target index",
                          json_output);
        return 2;
    }

    std::size_t source_index = 0;
    std::size_t target_index = 0;
    if (!parse_index(arguments[2], source_index)) {
        print_parse_error(command,
                          "invalid source index: " + std::string(arguments[2]),
                          json_output);
        return 2;
    }
    if (!parse_index(arguments[3], target_index)) {
        print_parse_error(command,
                          "invalid target index: " + std::string(arguments[3]),
                          json_output);
        return 2;
    }

    section_part_command_options options;
    std::string error_message;
    if (!parse_section_part_command_options(arguments, 4U, false, options,
                                            error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return 1;
    }

    if (!doc.move_section(source_index, target_index)) {
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
            [source_index, target_index](std::ostream &stream) {
                stream << ",\"source\":" << source_index
                       << ",\"target\":" << target_index;
            });
    }

    return 0;
}

[[nodiscard]] auto run_copy_section_layout_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 4U) {
        print_parse_error(command,
                          "copy-section-layout expects an input path, a source "
                          "index, and a target index",
                          json_output);
        return 2;
    }

    std::size_t source_index = 0;
    std::size_t target_index = 0;
    if (!parse_index(arguments[2], source_index)) {
        print_parse_error(command,
                          "invalid source index: " + std::string(arguments[2]),
                          json_output);
        return 2;
    }
    if (!parse_index(arguments[3], target_index)) {
        print_parse_error(command,
                          "invalid target index: " + std::string(arguments[3]),
                          json_output);
        return 2;
    }

    section_part_command_options options;
    std::string error_message;
    if (!parse_section_part_command_options(arguments, 4U, false, options,
                                            error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return 1;
    }

    if (!doc.copy_section_header_references(source_index, target_index) ||
        !doc.copy_section_footer_references(source_index, target_index)) {
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
            [source_index, target_index](std::ostream &stream) {
                stream << ",\"source\":" << source_index
                       << ",\"target\":" << target_index;
            });
    }

    return 0;
}

} // namespace

auto is_section_management_command(std::string_view command) -> bool {
    return command == "insert-section" || command == "remove-section" ||
           command == "move-section" || command == "copy-section-layout";
}

auto run_section_management_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    if (command == "insert-section") {
        return run_insert_section_command(command, arguments, doc);
    }
    if (command == "remove-section") {
        return run_remove_section_command(command, arguments, doc);
    }
    if (command == "move-section") {
        return run_move_section_command(command, arguments, doc);
    }
    if (command == "copy-section-layout") {
        return run_copy_section_layout_command(command, arguments, doc);
    }

    print_parse_error(command, "unsupported section management command",
                      has_json_flag(arguments));
    return 2;
}

} // namespace featherdoc_cli
