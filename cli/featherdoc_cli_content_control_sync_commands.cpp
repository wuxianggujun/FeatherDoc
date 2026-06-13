#include "featherdoc_cli_content_control_sync_commands.hpp"

#include "featherdoc_cli_bookmark_text_options_parse.hpp"
#include "featherdoc_cli_command_support.hpp"
#include "featherdoc_cli_content_control_mutation_output.hpp"
#include "featherdoc_cli_errors.hpp"
#include "featherdoc_cli_parse.hpp"

#include <ostream>
#include <string>
#include <vector>

namespace featherdoc_cli {

auto run_sync_content_controls_from_custom_xml_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 2U) {
        print_parse_error(
            command, "sync-content-controls-from-custom-xml expects an input path",
            json_output);
        return 2;
    }

    sync_content_controls_from_custom_xml_options options;
    std::string error_message;
    if (!parse_sync_content_controls_from_custom_xml_options(
            arguments, 2U, options, error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return 1;
    }

    const auto sync_result = doc.sync_content_controls_from_custom_xml();
    if (!sync_result.has_value()) {
        report_document_error(command, "sync", doc.last_error(),
                              options.json_output);
        return 1;
    }

    if (!save_document(doc, options.output_path, command, options.json_output)) {
        return 1;
    }

    if (options.json_output) {
        write_json_mutation_result(
            command, doc, options.output_path,
            [&sync_result](std::ostream &stream) {
                write_json_custom_xml_sync_result(stream, *sync_result);
            });
    } else {
        print_custom_xml_sync_result(options.output_path, *sync_result);
    }

    return 0;
}

} // namespace featherdoc_cli
