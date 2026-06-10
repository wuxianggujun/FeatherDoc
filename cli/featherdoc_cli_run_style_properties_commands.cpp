#include "featherdoc_cli_run_style_properties_commands.hpp"

#include "featherdoc_cli_command_support.hpp"
#include "featherdoc_cli_errors.hpp"
#include "featherdoc_cli_json.hpp"
#include "featherdoc_cli_parse.hpp"
#include "featherdoc_cli_run_properties_options_parse.hpp"
#include "featherdoc_cli_run_style_properties_support.hpp"
#include "featherdoc_cli_text.hpp"

#include <featherdoc.hpp>

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

namespace featherdoc_cli {
namespace {

enum class style_rebase_kind {
    character,
    paragraph,
};

auto run_rebase_style_based_on_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    std::string_view usage_message, style_rebase_kind kind) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 4U) {
        print_parse_error(command, std::string(usage_message), json_output);
        return 2;
    }

    const auto style_id = std::string(arguments[2]);
    const auto based_on = std::string(arguments[3]);
    rebase_style_based_on_options options;
    std::string error_message;
    if (!parse_rebase_style_based_on_options(arguments, 4U, options,
                                             error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    featherdoc::Document doc;
    if (!open_document_for_command(command, arguments, doc, options.json_output)) {
        return 1;
    }

    const auto resolved =
        resolve_style_properties_for_command(command, doc, style_id,
                                             options.json_output);
    if (!resolved.has_value()) {
        return 1;
    }

    const auto preserved_properties =
        collect_materialized_style_run_properties(style_id, *resolved);

    const auto rebased =
        kind == style_rebase_kind::character
            ? doc.rebase_character_style_based_on(style_id, based_on)
            : doc.rebase_paragraph_style_based_on(style_id, based_on);
    if (!rebased) {
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
            [&style_id, &based_on, &preserved_properties](std::ostream &stream) {
                stream << ",\"style_id\":";
                write_json_string(stream, style_id);
                stream << ",\"based_on\":";
                write_json_string(stream, based_on);
                stream << ",\"preserved\":";
                write_json_materialized_style_run_properties(stream,
                                                             preserved_properties);
            });
    }

    return 0;
}

} // namespace

#include "featherdoc_cli_run_style_properties_inspect_commands.inc"

#include "featherdoc_cli_run_style_properties_rebase_commands.inc"

#include "featherdoc_cli_run_style_properties_mutation_commands.inc"

} // namespace featherdoc_cli
