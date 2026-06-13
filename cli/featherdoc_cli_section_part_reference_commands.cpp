#include "featherdoc_cli_section_part_reference_commands.hpp"

#include "featherdoc_cli_command_support.hpp"
#include "featherdoc_cli_errors.hpp"
#include "featherdoc_cli_json.hpp"
#include "featherdoc_cli_parse.hpp"
#include "featherdoc_cli_section_options_parse.hpp"
#include "featherdoc_cli_section_part_support.hpp"

#include <cstddef>
#include <ostream>
#include <string>
#include <string_view>
#include <vector>

namespace featherdoc_cli {
namespace {

[[nodiscard]] auto assign_section_part(
    featherdoc::Document &doc, section_part_family family,
    std::size_t section_index, std::size_t part_index,
    featherdoc::section_reference_kind reference_kind, std::string_view command,
    bool json_output) -> bool {
    auto &paragraphs =
        family == section_part_family::header
            ? doc.assign_section_header_paragraphs(section_index, part_index,
                                                   reference_kind)
            : doc.assign_section_footer_paragraphs(section_index, part_index,
                                                   reference_kind);
    if (!paragraphs.has_next()) {
        std::string message = "failed to assign section ";
        message += section_part_name(family);
        message += " reference";
        return report_operation_failure(command, "mutate", message, doc.last_error(),
                                        json_output);
    }

    return true;
}

[[nodiscard]] auto remove_section_part_reference(
    featherdoc::Document &doc, section_part_family family,
    std::size_t section_index,
    featherdoc::section_reference_kind reference_kind, std::string_view command,
    bool json_output) -> bool {
    const auto success =
        family == section_part_family::header
            ? doc.remove_section_header_reference(section_index, reference_kind)
            : doc.remove_section_footer_reference(section_index, reference_kind);
    if (!success) {
        std::string message = "section ";
        message += section_part_name(family);
        message += " reference not found";
        return report_operation_failure(command, "mutate", message, doc.last_error(),
                                        json_output);
    }

    return true;
}

[[nodiscard]] auto run_assign_section_part_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 4U) {
        print_parse_error(command,
                          "assign-section-header/footer expects an input path, "
                          "a section index, and a part index",
                          json_output);
        return 2;
    }

    std::size_t section_index = 0;
    std::size_t part_index = 0;
    if (!parse_index(arguments[2], section_index)) {
        print_parse_error(command,
                          "invalid section index: " + std::string(arguments[2]),
                          json_output);
        return 2;
    }
    if (!parse_index(arguments[3], part_index)) {
        print_parse_error(command,
                          "invalid part index: " + std::string(arguments[3]),
                          json_output);
        return 2;
    }

    section_text_options options;
    std::string error_message;
    if (!parse_section_text_options(arguments, 4U, false, true, true, options,
                                    error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    const auto family = part_family_for_command(command);
    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return 1;
    }

    if (!assign_section_part(doc, family, section_index, part_index,
                             options.reference_kind, command,
                             options.json_output)) {
        return 1;
    }

    if (!save_document(doc, options.output_path, command, options.json_output)) {
        return 1;
    }

    if (options.json_output) {
        write_json_mutation_result(
            command, doc, options.output_path,
            [family, section_index, part_index, &options](std::ostream &stream) {
                stream << ",\"part\":";
                write_json_string(stream, section_part_name(family));
                stream << ",\"section\":" << section_index << ",\"kind\":";
                write_json_string(
                    stream, featherdoc::to_xml_reference_type(options.reference_kind));
                stream << ",\"part_index\":" << part_index;
            });
    }

    return 0;
}

[[nodiscard]] auto run_remove_section_part_reference_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 3U) {
        print_parse_error(command,
                          "remove-section-header/footer expects an input path "
                          "and a section index",
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

    section_text_options options;
    std::string error_message;
    if (!parse_section_text_options(arguments, 3U, false, true, true, options,
                                    error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    const auto family = part_family_for_command(command);
    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return 1;
    }

    if (!remove_section_part_reference(doc, family, section_index,
                                       options.reference_kind, command,
                                       options.json_output)) {
        return 1;
    }

    if (!save_document(doc, options.output_path, command, options.json_output)) {
        return 1;
    }

    if (options.json_output) {
        write_json_mutation_result(
            command, doc, options.output_path,
            [family, section_index, &options](std::ostream &stream) {
                stream << ",\"part\":";
                write_json_string(stream, section_part_name(family));
                stream << ",\"section\":" << section_index << ",\"kind\":";
                write_json_string(
                    stream, featherdoc::to_xml_reference_type(options.reference_kind));
            });
    }

    return 0;
}

} // namespace

auto is_section_part_reference_command(std::string_view command) -> bool {
    return command == "assign-section-header" ||
           command == "assign-section-footer" ||
           command == "remove-section-header" ||
           command == "remove-section-footer";
}

auto run_section_part_reference_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    if (command == "assign-section-header" ||
        command == "assign-section-footer") {
        return run_assign_section_part_command(command, arguments, doc);
    }
    if (command == "remove-section-header" ||
        command == "remove-section-footer") {
        return run_remove_section_part_reference_command(command, arguments, doc);
    }

    print_parse_error(command, "unsupported section part reference command",
                      has_json_flag(arguments));
    return 2;
}

} // namespace featherdoc_cli
