#include "featherdoc_cli_section_part_management_commands.hpp"

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

[[nodiscard]] auto remove_part(featherdoc::Document &doc,
                               section_part_family family,
                               std::size_t part_index,
                               std::string_view command,
                               bool json_output) -> bool {
    const auto success = family == section_part_family::header
                             ? doc.remove_header_part(part_index)
                             : doc.remove_footer_part(part_index);
    if (!success) {
        std::string message =
            std::string(section_part_name(family)) + " part not found";
        return report_operation_failure(command, "mutate", message, doc.last_error(),
                                        json_output);
    }

    return true;
}

[[nodiscard]] auto move_part(featherdoc::Document &doc, section_part_family family,
                             std::size_t source_index,
                             std::size_t target_index,
                             std::string_view command,
                             bool json_output) -> bool {
    const auto success = family == section_part_family::header
                             ? doc.move_header_part(source_index, target_index)
                             : doc.move_footer_part(source_index, target_index);
    if (!success) {
        std::string message =
            std::string(section_part_name(family)) + " part could not be reordered";
        return report_operation_failure(command, "mutate", message, doc.last_error(),
                                        json_output);
    }

    return true;
}

[[nodiscard]] auto run_remove_part_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 3U) {
        print_parse_error(command,
                          "remove-header/footer-part expects an input path and "
                          "a part index",
                          json_output);
        return 2;
    }

    std::size_t part_index = 0;
    if (!parse_index(arguments[2], part_index)) {
        print_parse_error(command,
                          "invalid part index: " + std::string(arguments[2]),
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

    const auto family = part_family_for_command(command);
    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return 1;
    }

    if (!remove_part(doc, family, part_index, command, options.json_output)) {
        return 1;
    }

    if (!save_document(doc, options.output_path, command, options.json_output)) {
        return 1;
    }

    if (options.json_output) {
        write_json_mutation_result(
            command, doc, options.output_path,
            [family, part_index](std::ostream &stream) {
                stream << ",\"part\":";
                write_json_string(stream, section_part_name(family));
                stream << ",\"part_index\":" << part_index;
            });
    }

    return 0;
}

[[nodiscard]] auto run_move_part_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 4U) {
        print_parse_error(command,
                          "move-header/footer-part expects an input path, a "
                          "source index, and a target index",
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

    const auto family = part_family_for_command(command);
    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return 1;
    }

    if (!move_part(doc, family, source_index, target_index, command,
                   options.json_output)) {
        return 1;
    }

    if (!save_document(doc, options.output_path, command, options.json_output)) {
        return 1;
    }

    if (options.json_output) {
        write_json_mutation_result(
            command, doc, options.output_path,
            [family, source_index, target_index](std::ostream &stream) {
                stream << ",\"part\":";
                write_json_string(stream, section_part_name(family));
                stream << ",\"source\":" << source_index
                       << ",\"target\":" << target_index;
            });
    }

    return 0;
}

} // namespace

auto is_section_part_management_command(std::string_view command) -> bool {
    return command == "remove-header-part" ||
           command == "remove-footer-part" ||
           command == "move-header-part" || command == "move-footer-part";
}

auto run_section_part_management_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    if (command == "remove-header-part" || command == "remove-footer-part") {
        return run_remove_part_command(command, arguments, doc);
    }
    if (command == "move-header-part" || command == "move-footer-part") {
        return run_move_part_command(command, arguments, doc);
    }

    print_parse_error(command, "unsupported section part management command",
                      has_json_flag(arguments));
    return 2;
}

} // namespace featherdoc_cli
