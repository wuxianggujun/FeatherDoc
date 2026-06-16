#include "featherdoc_cli_table_border_margin_commands_detail.hpp"

#include "featherdoc_cli_command_support.hpp"
#include "featherdoc_cli_domain_names.hpp"
#include "featherdoc_cli_domain_parse.hpp"
#include "featherdoc_cli_errors.hpp"
#include "featherdoc_cli_json.hpp"
#include "featherdoc_cli_parse.hpp"
#include "featherdoc_cli_table_format_support.hpp"

#include <cstddef>
#include <ostream>
#include <string>
#include <string_view>
#include <vector>

namespace featherdoc_cli {

auto run_set_table_border_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 4U) {
        print_parse_error(
            command,
            "set-table-border expects an input path, a table index, and a border edge",
            json_output);
        return 2;
    }

    std::size_t table_index = 0U;
    if (!parse_table_index(command, arguments, table_index, json_output)) {
        return 2;
    }

    featherdoc::table_border_edge edge = featherdoc::table_border_edge::top;
    if (!parse_table_border_edge_text(arguments[3], edge)) {
        print_parse_error(command,
                          "invalid table border edge: " +
                              std::string(arguments[3]),
                          json_output);
        return 2;
    }

    table_cell_border_options options;
    if (!parse_border_options(command, arguments, 4U, options, json_output)) {
        return 2;
    }

    featherdoc::Table table;
    if (!open_table_for_format(command, arguments, doc, table_index, options,
                               table)) {
        return 1;
    }

    const auto border = make_border_definition(options);
    if (!table.set_border(edge, border)) {
        report_invalid_table_target(command, "failed to set table border",
                                    options.json_output);
        return 1;
    }

    if (!save_document(doc, options.output_path, command, options.json_output)) {
        return 1;
    }

    if (options.json_output) {
        write_json_mutation_result(
            command, doc, options.output_path,
            [table_index, edge, border](std::ostream &stream) {
                write_json_table_location(stream, table_index);
                stream << ",\"edge\":";
                write_json_string(stream, table_border_edge_name(edge));
                write_json_border_definition(stream, border);
            });
    }
    return 0;
}

auto run_clear_table_border_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 4U) {
        print_parse_error(
            command,
            "clear-table-border expects an input path, a table index, and a border edge",
            json_output);
        return 2;
    }

    std::size_t table_index = 0U;
    if (!parse_table_index(command, arguments, table_index, json_output)) {
        return 2;
    }

    featherdoc::table_border_edge edge = featherdoc::table_border_edge::top;
    if (!parse_table_border_edge_text(arguments[3], edge)) {
        print_parse_error(command,
                          "invalid table border edge: " +
                              std::string(arguments[3]),
                          json_output);
        return 2;
    }

    table_cell_style_options options;
    if (!parse_style_options(command, arguments, 4U, options, json_output)) {
        return 2;
    }

    featherdoc::Table table;
    if (!open_table_for_format(command, arguments, doc, table_index, options,
                               table)) {
        return 1;
    }

    if (!table.clear_border(edge)) {
        report_invalid_table_target(command, "failed to clear table border",
                                    options.json_output);
        return 1;
    }

    if (!save_document(doc, options.output_path, command, options.json_output)) {
        return 1;
    }

    if (options.json_output) {
        write_json_mutation_result(
            command, doc, options.output_path,
            [table_index, edge](std::ostream &stream) {
                write_json_table_location(stream, table_index);
                stream << ",\"edge\":";
                write_json_string(stream, table_border_edge_name(edge));
            });
    }
    return 0;
}

} // namespace featherdoc_cli
