#include "featherdoc_cli_paragraph_numbering_commands.hpp"

#include "featherdoc_cli_command_support.hpp"
#include "featherdoc_cli_errors.hpp"
#include "featherdoc_cli_json.hpp"
#include "featherdoc_cli_numbering_options_parse.hpp"
#include "featherdoc_cli_paragraph_numbering_commands_support.hpp"
#include "featherdoc_cli_parse.hpp"

#include <featherdoc.hpp>

#include <ostream>
#include <string>
#include <string_view>
#include <vector>

namespace featherdoc_cli {

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
    if (!open_paragraph_numbering_document_for_command(
            command, arguments, doc, options.json_output)) {
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

} // namespace featherdoc_cli
