#include "featherdoc_cli_field_update_commands.hpp"

#include "featherdoc_cli_command_support.hpp"
#include "featherdoc_cli_errors.hpp"
#include "featherdoc_cli_field_parse.hpp"
#include "featherdoc_cli_json.hpp"
#include "featherdoc_cli_parse.hpp"
#include "featherdoc_cli_section_options_parse.hpp"

#include <cstddef>
#include <iostream>
#include <string>
#include <vector>

namespace featherdoc_cli {

auto run_inspect_update_fields_on_open_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
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
        std::cout << "update_fields_on_open: "
                  << (*enabled ? "true" : "false") << '\n';
    }
    return 0;
}

auto run_set_update_fields_on_open_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
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
            command, doc, options.output_path, [&options](std::ostream &stream) {
                stream << ",\"update_fields_on_open\":"
                       << json_bool(*options.enabled);
            });
    }
    return 0;
}

} // namespace featherdoc_cli
