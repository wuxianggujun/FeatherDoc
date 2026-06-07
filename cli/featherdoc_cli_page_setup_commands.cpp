#include "featherdoc_cli_page_setup_commands.hpp"

#include "featherdoc_cli_command_support.hpp"
#include "featherdoc_cli_errors.hpp"
#include "featherdoc_cli_page_setup.hpp"
#include "featherdoc_cli_page_setup_parse.hpp"
#include "featherdoc_cli_parse.hpp"

#include <cstddef>
#include <iostream>
#include <ostream>
#include <string>
#include <string_view>
#include <vector>

namespace featherdoc_cli {
namespace {

auto run_inspect_page_setup_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 2U) {
        print_parse_error(command, "inspect-page-setup expects an input path",
                          json_output);
        return 2;
    }

    inspect_page_setup_options options;
    std::string error_message;
    if (!parse_inspect_page_setup_options(arguments, 2U, options,
                                          error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return 1;
    }

    if (options.section_index.has_value()) {
        return inspect_page_setup(doc, *options.section_index, command,
                                  options.json_output)
                   ? 0
                   : 1;
    }

    return inspect_page_setups(doc, command, options.json_output) ? 0 : 1;
}

auto run_set_section_page_setup_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 3U) {
        print_parse_error(
            command,
            "set-section-page-setup expects an input path and a section index",
            json_output);
        return 2;
    }

    std::size_t section_index = 0U;
    if (!parse_index(arguments[2], section_index)) {
        print_parse_error(command,
                          "invalid section index: " + std::string(arguments[2]),
                          json_output);
        return 2;
    }

    set_section_page_setup_options options;
    std::string error_message;
    if (!parse_set_section_page_setup_options(arguments, 3U, options,
                                              error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return 1;
    }

    featherdoc::section_page_setup setup{};
    if (!resolve_section_page_setup(doc, section_index, options, command,
                                    options.json_output, setup)) {
        return 1;
    }

    if (!doc.set_section_page_setup(section_index, setup)) {
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
            [section_index, &setup](std::ostream &stream) {
                stream << ",\"section\":" << section_index
                       << ",\"page_setup\":";
                write_json_section_page_setup(stream, setup);
            });
    }

    return 0;
}

} // namespace

auto is_page_setup_command(std::string_view command) -> bool {
    return command == "inspect-page-setup" ||
           command == "set-section-page-setup";
}

auto run_page_setup_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    if (command == "inspect-page-setup") {
        return run_inspect_page_setup_command(command, arguments, doc);
    }
    if (command == "set-section-page-setup") {
        return run_set_section_page_setup_command(command, arguments, doc);
    }

    print_parse_error(command,
                      "unsupported page setup command: " + std::string(command),
                      has_json_flag(arguments));
    return 2;
}

} // namespace featherdoc_cli
